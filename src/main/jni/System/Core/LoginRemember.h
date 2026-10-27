#pragma once

#include <string>

// Whether the access key survives a restart is one stored byte beside it,
// written by the login screen's "Remember me" box. It lives in its own file so a
// stored key can never be mistaken for the flag that says whether to keep it.
namespace login_remember
{
    inline constexpr const char *kKeyFile  = "codm.ini";
    inline constexpr const char *kFlagFile = "codm.remember";

    // No file, or a file that is not ours, means the box was never changed and it
    // starts ticked, which is the behaviour it always had. Only an explicit '0'
    // counts as "do not remember".
    inline bool FlagFromStoredText(const std::string &text)
    {
        if (text.empty()) return true;
        return text[0] != '0';
    }

    inline const char *StoredFlagText(bool remember)
    {
        return remember ? "1" : "0";
    }
}
