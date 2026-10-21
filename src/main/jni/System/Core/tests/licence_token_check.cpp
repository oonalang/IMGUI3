#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>

#include "System/Core/LicenceToken.h"

static int g_checks = 0;
static int g_failures = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        g_checks++;                                                            \
        if (cond) {                                                            \
            printf("ok  : %s\n", msg);                                         \
        } else {                                                               \
            g_failures++;                                                      \
            printf("FAIL: %s  (line %d)\n", msg, __LINE__);                    \
        }                                                                      \
    } while (0)

struct Key
{
    RSA *rsa = nullptr;
    std::string modulusHex;

    bool generate()
    {
        BIGNUM *e = BN_new();
        BN_set_word(e, 65537);
        rsa = RSA_new();
        const bool ok = rsa != nullptr && RSA_generate_key_ex(rsa, 2048, e, nullptr) == 1;
        BN_free(e);
        if (!ok) return false;

        const BIGNUM *n = nullptr;
        RSA_get0_key(rsa, &n, nullptr, nullptr);
        if (n == nullptr) return false;

        char *hex = BN_bn2hex(n);
        if (hex == nullptr) return false;
        modulusHex = hex;
        OPENSSL_free(hex);
        return true;
    }

    std::string sign(const std::string &payload) const
    {
        unsigned char digest[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char *>(payload.data()), payload.size(), digest);

        std::vector<unsigned char> sig((size_t)RSA_size(rsa));
        unsigned int sigLen = 0;
        if (RSA_sign(NID_sha256, digest, SHA256_DIGEST_LENGTH, sig.data(), &sigLen, rsa) != 1)
            return std::string();
        sig.resize(sigLen);

        std::string out(((sig.size() + 2) / 3) * 4, '\0');
        const int n = EVP_EncodeBlock(reinterpret_cast<unsigned char *>(&out[0]),
                                      sig.data(), (int)sig.size());
        out.resize(n > 0 ? (size_t)n : 0);
        return out;
    }

    ~Key() { if (rsa) RSA_free(rsa); }
};

int main()
{
    Key vendor, cracker;
    if (!vendor.generate() || !cracker.generate())
    {
        printf("FAIL: could not generate a test keypair\n");
        return 1;
    }

    const std::string key   = "ETHNIR-4F2A-91C0";
    const std::string hwid  = "7f3c1a54-9c8b-4c1e-b2a1-0d5e6f708192";
    const std::string other = "9a9a9a9a-1111-2222-3333-444455556666";

    const time_t now = 1'800'000'000;
    const time_t in30d = now + 30 * 86400;
    const std::string payload = licence::BuildPayload(key, hwid, in30d, licence::kBuild);

    CHECK(payload == "v1|" + key + "|" + hwid + "|1802592000|" +
                      std::to_string(licence::kBuild),
          "L1 the payload is v1|key|hwid|expires|build");

    {
        const licence::Result r = licence::CheckSession(
            payload, vendor.sign(payload), key, hwid, now, vendor.modulusHex.c_str());
        CHECK(r.ok, "L2 a token signed by the vendor key is accepted");
        CHECK(r.expires == in30d, "L2 ... and its expiry comes back");
    }

    {
        const std::string theirSig = cracker.sign(payload);
        const licence::Result r = licence::CheckSession(
            payload, theirSig, key, hwid, now, vendor.modulusHex.c_str());
        CHECK(!r.ok, "L3 a token signed by another key is REFUSED");
        CHECK(std::strstr(r.reason, "not issued") != nullptr,
              "L3 ... with the \"not issued by this service\" reason");
    }

    {
        const std::string sig = vendor.sign(payload);
        std::string edited = payload;
        edited.replace(edited.find("1802592000"), 10, "9999999999");
        CHECK(!licence::CheckSession(edited, sig, key, hwid, now,
                                     vendor.modulusHex.c_str()).ok,
              "L4 a payload edited after signing is refused");
    }

    {
        const std::string sig = vendor.sign(payload);
        CHECK(!licence::CheckSession(payload, sig, "ETHNIR-OTHER-KEY", hwid, now,
                                     vendor.modulusHex.c_str()).ok,
              "L5 a token is refused for a different access key");
        CHECK(!licence::CheckSession(payload, sig, key, other, now,
                                     vendor.modulusHex.c_str()).ok,
              "L5 ... and for a different device");
    }

    {
        const std::string expired = licence::BuildPayload(key, hwid, now - 1, licence::kBuild);
        const licence::Result r = licence::CheckSession(
            expired, vendor.sign(expired), key, hwid, now, vendor.modulusHex.c_str());
        CHECK(!r.ok, "L6 an expired token is refused even with a good signature");
        CHECK(std::strstr(r.reason, "expired") != nullptr, "L6 ... and says why");

        const std::string oldBuild = licence::BuildPayload(key, hwid, in30d, licence::kBuild - 1);
        CHECK(!licence::CheckSession(oldBuild, vendor.sign(oldBuild), key, hwid, now,
                                     vendor.modulusHex.c_str()).ok,
              "L6 a token minted for another build is refused");
    }

    {
        const licence::Result r = licence::CheckSession(payload, vendor.sign(payload), key, hwid,
                                                        now, "", 65537);
        CHECK(!r.ok, "L7 with no public key configured nothing is accepted");
        CHECK(std::strstr(r.reason, "not configured") != nullptr, "L7 ... and says so");
    }

    {
        CHECK(!licence::CheckSession(payload, "", key, hwid, now,
                                     vendor.modulusHex.c_str()).ok,
              "L8 an empty signature is refused");
        CHECK(!licence::CheckSession(payload, "!!! not base64 !!!", key, hwid, now,
                                     vendor.modulusHex.c_str()).ok,
              "L8 a non-base64 signature is refused");
        CHECK(!licence::CheckSession("", vendor.sign(""), key, hwid, now,
                                     vendor.modulusHex.c_str()).ok,
              "L8 an empty payload is refused");
        CHECK(!licence::CheckSession(payload, "AAAA", key, hwid, now,
                                     vendor.modulusHex.c_str()).ok,
              "L8 a truncated signature is refused");
        CHECK(!licence::CheckSession("garbage", "AAAA", key, hwid, now,
                                     vendor.modulusHex.c_str()).ok,
              "L8 a signed-looking but malformed token is refused");
    }

    {
        const std::vector<unsigned char> in = {0x00, 0x01, 0xFE, 0xFF, 0x7F, 0x10, 0x42};
        std::string enc(((in.size() + 2) / 3) * 4, '\0');
        const int n = EVP_EncodeBlock(reinterpret_cast<unsigned char *>(&enc[0]),
                                      in.data(), (int)in.size());
        enc.resize(n > 0 ? (size_t)n : 0);
        CHECK(licence::Base64Decode(enc) == in, "L9 base64 decodes what the panel encodes");
        CHECK(licence::Base64Decode("AAAA\nAAAA").size() == 6,
              "L9 ... and ignores the newlines a JSON writer may add");
    }

    printf("\n");
    if (g_failures == 0)
    {
        printf("OK: all licence-token checks passed (%d)\n", g_checks);
        return 0;
    }
    printf("%d of %d check(s) FAILED\n", g_failures, g_checks);
    return 1;
}

