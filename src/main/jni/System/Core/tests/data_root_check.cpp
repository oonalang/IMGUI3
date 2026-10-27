#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>
#include "System/Core/DataRoot.h"

static void TouchFile(const std::string &path)
{
    std::ofstream file(path.c_str());
    file << "x";
}

int main()
{
    using data_root::Candidate;
    using data_root::ContainerCandidates;

    // The two layouts reported from real devices are held verbatim, and neither
    // is ever created: they live under another app's package.
    const std::vector<Candidate> containers = ContainerCandidates();
    assert(containers.size() == 2);
    assert(containers[0].first ==
           "/storage/emulated/0/Android/data/com.gspace.android/virtual/0/com.garena.game.codm");
    assert(containers[1].first ==
           "/storage/emulated/0/Android/data/com.excean.parallelspace/files/sdcard/0"
           "/Android/data/com.excean.parallelspace/files/sdcard/0/Android/data/com.garena.game.codm");
    assert(!containers[0].second && !containers[1].second);

    const std::string own = data_root::kGameExternal;
    assert(!own.empty() && own[0] == '/' && own.back() != '/');
    for (const Candidate &candidate : containers)
        assert(!candidate.first.empty() && candidate.first[0] == '/' &&
               candidate.first.back() != '/');

    char temp[] = "/tmp/ethnir_data_root_XXXXXX";
    char *base = mkdtemp(temp);
    assert(base != nullptr);
    const std::string root(base);

    // An existing directory that can be written is accepted, with or without
    // the permission to create it.
    assert(data_root::ProbeWritableDir(root));
    assert(data_root::ProbeWritableDir(root + "/", false));

    // A missing directory is refused when creation is not allowed, then created
    // once it is, and probing it again finds it there.
    const std::string files = root + "/files";
    assert(!data_root::ProbeWritableDir(files, false));
    assert(data_root::ProbeWritableDir(files, true));
    assert(data_root::ProbeWritableDir(files, false));

    // A missing parent is refused either way: building it would mean writing
    // into a directory that is not ours.
    const std::string orphan = root + "/no/such/parent";
    assert(!data_root::ProbeWritableDir(orphan, true));
    assert(!data_root::ProbeWritableDir(orphan, false));

    // A regular file is not a directory, however often it is probed.
    const std::string regular = root + "/codm.ini";
    TouchFile(regular);
    assert(!data_root::ProbeWritableDir(regular, false));
    assert(!data_root::ProbeWritableDir(regular, true));

    // Empty and relative paths are never roots.
    assert(!data_root::ProbeWritableDir(""));
    assert(!data_root::ProbeWritableDir("relative/path"));
    assert(data_root::Select({}).empty());

    // Select walks the candidates in order, takes the first that passes, and
    // hands back a path without a trailing slash so callers add exactly one.
    std::vector<Candidate> candidates;
    candidates.emplace_back(root + "/missing", false);
    candidates.emplace_back(files, false);
    candidates.emplace_back(root, false);
    assert(data_root::Select(candidates) == files);

    std::vector<Candidate> none;
    none.emplace_back(root + "/missing", false);
    none.emplace_back(root + "/also-missing", false);
    assert(data_root::Select(none).empty());

    std::remove(regular.c_str());
    rmdir(files.c_str());
    rmdir(base);

    puts("PASS: container layouts held verbatim, probe refuses foreign and missing parents, "
         "select returns the first usable root without a trailing slash");
    return 0;
}