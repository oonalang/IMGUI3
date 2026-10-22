// End-to-end check of the sign-in answer: the token connect.php just minted is
// put through the SAME verifier the APK runs (System/Core/LicenceToken.h), with
// the test deployment's modulus. Everything the app checks - signature, key,
// device, build, expiry - is exercised, including the failures.
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include "System/Core/LicenceToken.h"

static int g_fail = 0;
static void check(const char* what, bool ok)
{
    std::printf("%s: %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok) ++g_fail;
}

static std::string slurp(const char* path)
{
    FILE* f = std::fopen(path, "rb");
    if (!f) { std::printf("cannot read %s\n", path); std::exit(2); }
    std::string s;
    char buf[4096];
    size_t n;
    while ((n = std::fread(buf, 1, sizeof buf, f)) > 0) s.append(buf, n);
    std::fclose(f);
    // Trim the trailing newline a text editor or echo would leave.
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
    return s;
}

int main(int argc, char** argv)
{
    if (argc < 3) { std::printf("usage: %s payload.txt sig.b64 [modulus.hex]\n", argv[0]); return 2; }
    const std::string payload = slurp(argv[1]);
    const std::string sig     = slurp(argv[2]);
    const std::string modulus = argc > 3 ? slurp(argv[3]) : std::string(kPubModulusHex);

    const std::string key  = argc > 4 ? argv[4] : "ETHNIR-TEST01";
    const std::string hwid = argc > 5 ? argv[5] : "TESTDEVICE-0002";

    // 1. The token the panel just signed, checked the way the app checks it.
    licence::Result ok = licence::CheckSession(payload, sig, key, hwid, 0, modulus.c_str());
    check("panel token verifies with the deployment key", ok.ok);
    if (ok.ok)
        std::printf("     expires=%lld\n", (long long)ok.expires);
    else
        std::printf("     reason=%s\n", ok.reason ? ok.reason : "");

    // 2. The same token against the key compiled into this repo: a swapped or
    //    unconfigured panel has to be refused exactly here.
    licence::Result wrongKey = licence::CheckSession(payload, sig, key, hwid);
    check("token signed by another key is refused", !wrongKey.ok &&
          wrongKey.reason && std::string(wrongKey.reason) == "Token was not issued by this service");

    // 3. The fields the app compares after the signature.
    check("a different typed key is refused",
          !licence::CheckSession(payload, sig, "ETHNIR-OTHER", hwid, 0, modulus.c_str()).ok);
    check("a different device is refused",
          !licence::CheckSession(payload, sig, key, "OTHERDEVICE-01", 0, modulus.c_str()).ok);

    // 4. Expiry: one second past the signed timestamp.
    const long long exp = ok.ok ? (long long)ok.expires : 0;
    licence::Result expired = licence::CheckSession(payload, sig, key, hwid, (time_t)(exp + 1), modulus.c_str());
    check("an expired token is refused", !expired.ok && expired.reason && std::string(expired.reason) == "This licence key has expired");

    // 5. Tampering with any signed byte must break the signature.
    std::string tampered = payload;
    const size_t bar = tampered.rfind('|');
    if (bar != std::string::npos && bar + 1 < tampered.size()) tampered[bar + 1] = (tampered[bar + 1] == '1' ? '2' : '1');
    check("a tampered payload is refused",
          !licence::CheckSession(tampered, sig, key, hwid, 0, modulus.c_str()).ok);

    // 6. Garbage in the signature field.
    check("garbage base64 is refused",
          !licence::CheckSession(payload, "!!!!", key, hwid, 0, modulus.c_str()).ok);

    // 7. The shape the panel produces.
    check("payload is v1|key|hwid|expires|build",
          payload.rfind("v1|" + key + "|" + hwid + "|", 0) == 0 &&
          payload.compare(payload.size() - 2, 2, "|1") == 0);

    if (g_fail == 0) { std::printf("\nOK: the panel's token signs in under the app's own verifier\n"); return 0; }
    std::printf("\n%d check(s) FAILED\n", g_fail);
    return 1;
}
