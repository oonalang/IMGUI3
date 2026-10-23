#include <cstdio>
#include <cstdint>
#include <cstring>

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

    const uintptr_t unityBase = (uintptr_t)0x7000000000ull;
    fault_guard::SetModuleBase((void *)unityBase);

    char buf[128];
    fault_guard::FormatAddress(buf, sizeof(buf), unityBase + 0x9B5B850ull);
    Check("an address inside the module reports its offset",
          std::strcmp(buf, "libunity.so+0x9b5b850") == 0);

    fault_guard::FormatAddress(buf, sizeof(buf), (uintptr_t)0x1234);
    Check("an address outside the module reports itself",
          std::strcmp(buf, "0x1234") == 0);

    fault_guard::NotePhase("esp walk");
    fault_guard::DescribeFault(buf, sizeof(buf), 11, (uintptr_t)0xDEAD,
                               unityBase + 0x9B5B850ull, (uintptr_t)0x1234,
                               fault_guard::g_phase);
    Check("the crash line carries the signal", std::strstr(buf, "signal=11") != nullptr);
    Check("the crash line carries the fault address", std::strstr(buf, "fault=0xdead") != nullptr);
    Check("the crash line carries the pc as a module offset",
          std::strstr(buf, "pc=libunity.so+0x9b5b850") != nullptr);
    Check("the crash line carries the link register", std::strstr(buf, "lr=0x1234") != nullptr);
    Check("the crash line carries the phase the mod was in",
          std::strstr(buf, "phase=esp walk") != nullptr);

    if (g_failures)
    {
        std::printf("FAIL: %d check(s)\n", g_failures);
        return 1;
    }

    std::printf("OK: fault_guard_check - faults are contained and the guard re-arms\n");
    return 0;
}
