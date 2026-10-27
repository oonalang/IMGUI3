#pragma once

#include <string>
#include <utility>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>

// Where the mod keeps the things it persists as files: the profiles, the
// autoload pointer, and the log directory. The path it used to assume - the
// game's own external directory - is one layout of many. On a phone where the
// game runs inside a virtual container (GSpace, ParallelSpace) that directory is
// not where anything appears, so every write went to a path that did not exist
// and every read came back empty: no profiles, no autoload.
//
// Each candidate is a directory plus whether its missing parents may be created
// for it. Only the game's own external directory gets that permission: it is
// ours to create, while a container's mount of someone else's package is not,
// and would not be writable anyway. The first candidate that is really there
// wins, which keeps a plain install on the path it has always used and lets a
// container install fall through to its own layout.
namespace data_root
{
    inline const char *kGameExternal = "/storage/emulated/0/Android/data/com.garena.game.codm/files";
    inline const char *kGameSdcard   = "/sdcard/Android/data/com.garena.game.codm/files";

    // The two layouts reported from real devices, verbatim: the guest's data is
    // mounted under the container's own package, and the game's external
    // directory never exists at the path above it.
    inline const char *kGspace =
        "/storage/emulated/0/Android/data/com.gspace.android/virtual/0/com.garena.game.codm";
    inline const char *kParallelSpace =
        "/storage/emulated/0/Android/data/com.excean.parallelspace/files/sdcard/0"
        "/Android/data/com.excean.parallelspace/files/sdcard/0/Android/data/com.garena.game.codm";

    using Candidate = std::pair<std::string, bool>;   // directory, may create its parents

    // The two container layouts, in the order they were reported. No trailing
    // slash anywhere: callers add the one they need, so a root can never end up
    // as "...//file".
    inline std::vector<Candidate> ContainerCandidates()
    {
        std::vector<Candidate> out;
        out.emplace_back(kGspace, false);
        out.emplace_back(kParallelSpace, false);
        return out;
    }

    inline std::string StripSlash(const std::string &raw)
    {
        std::string path = raw;
        while (path.size() > 1 && path.back() == '/') path.pop_back();
        return path;
    }

    // True when the directory is there and writable, creating it when allowed
    // and only its last component is missing. A parent that is absent is a
    // refusal either way: building it would mean writing into another app's
    // directory, and we would not be able to.
    inline bool ProbeWritableDir(const std::string &raw, bool createParents = false)
    {
        if (raw.empty() || raw[0] != '/') return false;

        const std::string path = StripSlash(raw);
        if (path.empty()) return false;

        struct stat st{};
        if (stat(path.c_str(), &st) == 0)
            return S_ISDIR(st.st_mode) != 0 && access(path.c_str(), W_OK) == 0;

        if (!createParents) return false;

        const size_t slash = path.find_last_of('/');
        if (slash == std::string::npos || slash == 0) return false;
        const std::string parent = path.substr(0, slash);
        if (stat(parent.c_str(), &st) != 0 || S_ISDIR(st.st_mode) == 0) return false;

        if (mkdir(path.c_str(), 0775) != 0) return false;
        return access(path.c_str(), W_OK) == 0;
    }

    // The first candidate the probe accepts, returned without a trailing slash.
    // An empty result means nothing was usable, and the caller decides what
    // that falls back to.
    inline std::string Select(const std::vector<Candidate> &candidates)
    {
        for (const Candidate &candidate : candidates)
            if (ProbeWritableDir(candidate.first, candidate.second)) return StripSlash(candidate.first);
        return std::string();
    }
}