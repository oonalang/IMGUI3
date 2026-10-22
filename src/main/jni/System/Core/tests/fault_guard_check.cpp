#include <cstdio>
#include <cstdint>

#include "System/Core/FaultGuard.h"

static int g_failures = 0;

static void Check(const char *what, bool ok)
{
    std::printf("%s: %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok) ++g_failures;
}

static volatile int *BadPointer()
{
    return (volatile int *)(uintptr_t)0x10;
}

int main()
{
    int ran = 0;
    Check("a clean call reports success", fault_guard::Run([&] { ran++; }));
    Check("the clean call body ran once", ran == 1);

    bool survived = fault_guard::Run([] { (void)*BadPointer(); });
    Check("a faulting call reports failure", !survived);

    ran = 0;
    Check("the guard still works after a fault", fault_guard::Run([&] { ran++; }));
    Check("the body after the fault ran", ran == 1);

    int writeRan = 0;
    const bool writeOk = fault_guard::Run([&] {
        *const_cast<int *>(reinterpret_cast<const int *>(uintptr_t(0x8))) = 1;
        writeRan++;
    });
    Check("a faulting write reports failure", !writeOk);
    Check("nothing after the fault ran", writeRan == 0);

    ran = 0;
    fault_guard::Run([&] { ran++; });
    Check("recovery is repeatable", ran == 1);

    if (g_failures)
    {
        std::printf("FAIL: %d check(s)\n", g_failures);
        return 1;
    }

    std::printf("OK: fault_guard_check - faults are contained and the guard re-arms\n");
    return 0;
}
