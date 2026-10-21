#pragma once

// ---------------------------------------------------------------------------
// Licence tokens - the panel signs, the app only checks
// ---------------------------------------------------------------------------
// The old scheme could not survive a swapped panel: the client held a shared
// secret ("CODMDGR-<key>-<hwid>-Vm8Lk7Uj2JmsjCPVPVjrLa7zgfx3uz9E", MD5) and
// compared the panel's token to it. That secret is in the APK, so anyone can
// compute a valid "token" for any key without asking a panel at all - and a
// cracker's own panel only has to hand back that same MD5 to mint working keys.
// Replacing the URL pointed at their panel was enough; the app could not tell.
//
// So the trust anchor moves off the URL and off anything shared:
//
//   * key minting happens against an RSA private key that never leaves the
//     vendor (tools/licence/, or the panel's private/ directory),
//   * the app embeds only the matching PUBLIC key (LicencePubKey.h),
//   * the panel returns a payload plus a PKCS#1 v1.5 RSA-SHA256 signature over
//     its exact bytes; the app re-computes the digest and verifies it.
//
// A cracker's panel can still answer - but it cannot sign, so its keys are
// rejected by the same code path that accepts the real panel's. Swapping the
// endpoint stops being a way to mint keys. (It can still be patched out of the
// app; nothing client-side survives a determined patcher - this raises the cost
// from "swap one string" to "rewrite the verifier".)
//
// Token format (the payload the signature covers, exact bytes):
//
//   v1|<access key>|<hwid>|<expires unix>|<build>
//
// "v1" is this format, the access key and hwid are bound into the signature so
// a token cannot be moved to another key or device, and <build> stops an old
// token being replayed against a newer APK.
//
// OpenSSL here is the vendored 1.1.0 (jni/curl/openssl-android-*): RSA_verify,
// BN_hex2bn, SHA256 and BN_set_word all exist there, and Ed25519 (1.1.1+) does
// not, which is why this is RSA rather than a modern curve.
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
    // Bumped together with the panel when the payload layout changes; a token
    // minted for another build is refused.
    constexpr int kBuild = 1;

    // "No expiry" is not a special wire value: a token always carries a concrete
    // timestamp, so the client's rule stays one comparison and there is no
    // zero-means-forever branch for an attacker to aim at. A key sold as
    // permanent is signed with this one (2100-01-01), which is what
    // tools/licence/mint.sh and panel/lib.php use for days=0.
    constexpr long long kNeverExpires = 4102444800LL;   // 2100-01-01 00:00:00Z

    struct Result
    {
        bool    ok = false;
        const char *reason = "";
        time_t  expires = 0;
    };

    // -----------------------------------------------------------------------
    // base64 (the signature travels as text in the JSON reply)
    // -----------------------------------------------------------------------
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
            if (v == -2) break;          // '=' padding ends the payload
            if (v < 0) continue;         // whitespace / newlines
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

    // -----------------------------------------------------------------------
    // RSA PKCS#1 v1.5 / SHA-256 verification
    // -----------------------------------------------------------------------
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
        RSA_free(rsa);                   // owns n and e once set0_key took them
        return ok;
    }

    // -----------------------------------------------------------------------
    // The check the app makes on every sign-in
    // -----------------------------------------------------------------------
    // The payload is verified exactly as it arrived (no re-serialisation, so
    // there is no canonicalisation to get wrong), and only then are its fields
    // compared with what this run is: the key the player typed, this device,
    // this build, and "not expired".
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
            // This is what a swapped panel looks like: whatever it answers, it
            // cannot sign with the vendor's key.
            r.reason = "Token was not issued by this service";
            return r;
        }

        // v1|key|hwid|expires|build - the key itself may not contain '|'.
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

    // Builds the payload the way the minting side does - used by the panel-side
    // tooling and by the host test, not by the app at runtime (the app always
    // verifies the server's exact bytes).
    inline std::string BuildPayload(const std::string &key, const std::string &hwid,
                                    time_t expires, int build = kBuild)
    {
        char buf[32];
        std::snprintf(buf, sizeof buf, "%lld", (long long)expires);
        return "v1|" + key + "|" + hwid + "|" + buf + "|" + std::to_string(build);
    }
}
