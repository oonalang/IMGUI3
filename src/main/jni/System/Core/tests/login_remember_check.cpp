#include <cstdio>
#include <string>

#include "System/Core/LoginRemember.h"

static int g_failures = 0;

static void Check(const char* what, bool ok)
{
    std::printf("%s: %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok) ++g_failures;
}

int main()
{
    // A box that was never touched keeps the behaviour it always had: ticked, so
    // an access key stored by an older build is still filled in.
    Check("no stored flag leaves the box ticked", login_remember::FlagFromStoredText("") == true);
    Check("stored on keeps the box ticked", login_remember::FlagFromStoredText("1") == true);
    Check("stored off unticked the box", login_remember::FlagFromStoredText("0") == false);

    // Only the one byte the writer emits counts; anything else is a torn or
    // foreign file and must not be read as "forget".
    Check("garbage is read as no preference", login_remember::FlagFromStoredText("maybe") == true);
    Check("a newline alone is no preference", login_remember::FlagFromStoredText("\n") == true);
    Check("a leading zero is off even with trailing bytes", login_remember::FlagFromStoredText("0\n") == false);

    // What is written round-trips through what is read.
    Check("on round-trips", login_remember::FlagFromStoredText(login_remember::StoredFlagText(true)) == true);
    Check("off round-trips", login_remember::FlagFromStoredText(login_remember::StoredFlagText(false)) == false);

    // The key and the flag must never share a name, or forgetting one would drop
    // the other.
    Check("flag and key are different files",
          std::string(login_remember::kFlagFile) != std::string(login_remember::kKeyFile));

    return g_failures == 0 ? 0 : 1;
}
