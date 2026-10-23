#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>

#include <openssl/bn.h>
#include <openssl/obj_mac.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>

#include "LicencePubKey.h"

namespace licence
{
    constexpr int kBuild = 1;

    constexpr long long kNeverExpires = 4102444800LL;

    struct Result
    {
        bool    ok = false;
        const char *reason = "";
        time_t  expires = 0;
    };

    inline std::vector<unsigned char> Base64Decode(const std::string &in)
    {
        static const signed char T[256] = {
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,
            52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-2,-1,-1,
            -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
            15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
            -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
            41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
        };

        std::vector<unsigned char> out;
        out.reserve(in.size() / 4 * 3);

        uint32_t acc = 0;
        int bits = 0;
        for (unsigned char c : in)
        {
            const signed char v = T[c];
            if (v == -2) break;
            if (v < 0) continue;
            acc = (acc << 6) | (uint32_t)v;
            bits += 6;
            if (bits >= 8)
            {
                bits -= 8;
                out.push_back((unsigned char)((acc >> bits) & 0xFF));
            }
        }
        return out;
    }

    inline bool RsaSha256Verify(const std::string &payload,
                                const std::vector<unsigned char> &sig,
                                const char *modulusHex, unsigned long exponent)
    {
        if (modulusHex == nullptr || modulusHex[0] == '\0') return false;
        if (sig.empty() || sig.size() > 8192)               return false;

        BIGNUM *n = nullptr;
        if (BN_hex2bn(&n, modulusHex) == 0 || n == nullptr) return false;

        BIGNUM *e = BN_new();
        if (e == nullptr || BN_set_word(e, exponent) != 1)
        {
            BN_free(n);
            BN_free(e);
            return false;
        }

        RSA *rsa = RSA_new();
        if (rsa == nullptr || RSA_set0_key(rsa, n, e, nullptr) != 1)
        {
            RSA_free(rsa);
            BN_free(n);
            BN_free(e);
            return false;
        }

        unsigned char digest[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char *>(payload.data()), payload.size(), digest);

        const bool ok = RSA_verify(NID_sha256, digest, SHA256_DIGEST_LENGTH,
                                   sig.data(), (unsigned int)sig.size(), rsa) == 1;
        RSA_free(rsa);
        return ok;
    }

    inline Result CheckSession(const std::string &payload, const std::string &sigB64,
                               const std::string &typedKey, const std::string &hwid,
                               time_t now = 0,
                               const char *modulusHex = kPubModulusHex,
                               unsigned long exponent = kPubExponent)
    {
        Result r;

        if (modulusHex == nullptr || modulusHex[0] == '\0')
        {
            r.reason = "Licence key not configured in this build";
            return r;
        }

        const std::vector<unsigned char> sig = Base64Decode(sigB64);
        if (sig.empty())
        {
            r.reason = "Malformed signature";
            return r;
        }

        if (!RsaSha256Verify(payload, sig, modulusHex, exponent))
        {
            r.reason = "Token was not issued by this service";
            return r;
        }

        std::string field[5];
        int count = 0;
        for (char c : payload)
        {
            if (c == '|')
            {
                if (count == 5) { count = 6; break; }
                count++;
                continue;
            }
            if (count < 5) field[count] += c;
        }
        if (count != 4 || field[0] != "v1")
        {
            r.reason = "Malformed token";
            return r;
        }

        if (field[1] != typedKey)
        {
            r.reason = "Token was issued for a different access key";
            return r;
        }
        if (field[2] != hwid)
        {
            r.reason = "Token was issued for a different device";
            return r;
        }
        if (std::atoi(field[4].c_str()) != kBuild)
        {
            r.reason = "Token is for another build - sign in again";
            return r;
        }

        const long long exp = std::atoll(field[3].c_str());
        const time_t t = now != 0 ? now : time(nullptr);
        if (exp <= (long long)t)
        {
            r.reason = "This licence key has expired";
            return r;
        }

        r.ok = true;
        r.expires = (time_t)exp;
        return r;
    }

    inline std::string BuildPayload(const std::string &key, const std::string &hwid,
                                    time_t expires, int build = kBuild)
    {
        char buf[32];
        std::snprintf(buf, sizeof buf, "%lld", (long long)expires);
        return "v1|" + key + "|" + hwid + "|" + buf + "|" + std::to_string(build);
    }
}

