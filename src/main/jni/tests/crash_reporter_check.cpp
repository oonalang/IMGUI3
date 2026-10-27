// The crash reporter is the only thing that tells us where a native fault lands
// on a device, so the check drives a real SIGSEGV in a child process and reads
// the report back: signal number, the module and offset of the faulting
// instruction, and the breadcrumb that was written before it.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include <sys/wait.h>
#include <unistd.h>

#include "System/Core/CrashReporter.h"

static int AnchorForModule()
{
    return 7;
}

static bool ReadFile(const char *path, std::string &out)
{
    out.clear();
    FILE *f = fopen(path, "rb");
    if (f == nullptr) return false;
    char buf[512];
    size_t got = 0;
    while ((got = fread(buf, 1, sizeof(buf), f)) > 0)
        out.append(buf, got);
    fclose(f);
    return true;
}

int main()
{
    const char *path = "/tmp/ethnir_crash_host_check.txt";
    ::unlink(path);
    setenv(crash_report::kLogEnv, path, 1);

    const pid_t pid = fork();
    if (pid == 0)
    {
        crash_report::Install();

        const uintptr_t anchor = (uintptr_t)&AnchorForModule;
        crash_report::AddModule("host_check_module", anchor - 0x10000, anchor + 0x100000);
        crash_report::Note("host check breadcrumb before the fault");

        volatile int *nullTarget = nullptr;
        *nullTarget = 1;

        _exit(0);
    }

    int status = 0;
    if (waitpid(pid, &status, 0) != pid)
    {
        printf("FAIL: waitpid\n");
        return 1;
    }

    if (!WIFSIGNALED(status) || WTERMSIG(status) != SIGSEGV)
    {
        printf("FAIL: child did not die from SIGSEGV (status=%d)\n", status);
        return 1;
    }

    std::string report;
    if (!ReadFile(path, report))
    {
        printf("FAIL: no crash report at %s\n", path);
        return 1;
    }

    struct Check { const char *needle; const char *why; };
    const Check checks[] = {
        { "CRASH sig=11",        "the signal number is not in the report" },
        { "host_check_module+0x", "the faulting pc was not resolved to a module offset" },
        { "host check breadcrumb before the fault", "the breadcrumb from before the fault is missing" },
        { "crash reporter armed", "the install note is missing" },
    };

    for (const Check &c : checks)
    {
        if (report.find(c.needle) == std::string::npos)
        {
            printf("FAIL: %s\n--- report ---\n%s\n", c.why, report.c_str());
            return 1;
        }
    }

    printf("PASS crash_reporter_check (%d bytes)\n", (int)report.size());
    return 0;
}
