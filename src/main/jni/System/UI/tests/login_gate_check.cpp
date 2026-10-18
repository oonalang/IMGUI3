// Host test for System/UI/LoginGate.h - the license-key page.
//
// The gate is the old menu's AUTHENTICATION page rebuilt on the ImGui core. It
// was deleted with that menu (9d10e44) while Login() stayed behind with no
// caller, so the things pinned here are the ones that decide whether the mod is
// gated at all:
//
//   * a fresh gate is LOCKED, and an empty key never reaches the network;
//   * PASTE takes the clipboard, trimmed, and shows it in the field;
//   * AUTHENTICATE runs the authenticator OFF the render thread (the phase is
//     Checking while the worker is inside it, and a second press does nothing);
//   * a rejected key leaves the gate locked, shows the server's message, and
//     saves nothing;
//   * an accepted key flips Locked() to false, saves EXACTLY the key that was
//     checked, and stays open;
//   * the saved key is read once and prefills the field;
//   * touches: press+release on a button fires it, press then drag off does not.
//
// Host test, no NDK, no ImGui context needed (the gate draws with the draw list
// but nothing here calls Draw()):
//   g++ -std=c++17 -DIMGUI_DEFINE_MATH_OPERATORS -I src/main/jni -I src/main/jni/ImGui
//       src/main/jni/System/UI/tests/login_gate_check.cpp -o /tmp/login_gate_check
//       -lpthread && /tmp/login_gate_check
// Exits 0 and prints "OK: all login-gate checks passed" on success.
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>

#include "imgui.h"
#include "System/UI/LoginGate.h"

static int g_failures = 0;
#define CHECK(cond, msg)                                                  \
    do {                                                                  \
        if (!(cond)) { printf("FAIL: %s  (line %d)\n", msg, __LINE__);    \
                       g_failures++; }                                    \
        else        { printf("ok  : %s\n", msg); }                        \
    } while (0)

// ---- fakes ----------------------------------------------------------------
static int         g_authCalls = 0;
static std::string g_lastChecked;
static std::string g_savedKey;
static int         g_saveCalls = 0;
static std::string g_clipboard = "   AAAA-BBBB-CCCC-DDDD  \n";
static bool        g_haveSaved = false;
static std::string g_savedOnDisk;
static int         g_loadCalls = 0;

static std::string FakeAuthenticate(const char* key)
{
    g_authCalls++;
    g_lastChecked = key;
    std::this_thread::sleep_for(std::chrono::milliseconds(30));  // a network round trip
    if (std::string(key) == "GOOD-KEY-1234")
        return "OK";
    return "Invalid license key";
}

static bool FakeLoadSaved(char* out, int cap)
{
    g_loadCalls++;
    if (!g_haveSaved)
        return false;
    std::strncpy(out, g_savedOnDisk.c_str(), (size_t)cap - 1);
    out[cap - 1] = '\0';
    return true;
}

static bool FakeSaveKey(const char* key)
{
    g_saveCalls++;
    g_savedKey = key;
    return true;
}

static std::string FakeClipboard() { return g_clipboard; }

// Blocks until the worker answers, the way Poll()/Locked() do on the render
// thread, but bounded so a hung gate fails the test instead of hanging it.
static bool WaitForAnswer()
{
    for (int i = 0; i < 2500; i++)
    {
        if (login_gate::CurrentPhase() != login_gate::Checking)
            return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    return false;
}

int main()
{
    printf("== LoginGate: the license-key page ==\n");

    login_gate::Hooks hooks;
    hooks.authenticate = FakeAuthenticate;
    hooks.load_saved   = FakeLoadSaved;
    hooks.save_key     = FakeSaveKey;
    hooks.clipboard    = FakeClipboard;
    login_gate::Setup(hooks);

    // ---- G1: a fresh gate is locked and never touches the network ----------
    {
        login_gate::Reset();
        login_gate::detail::g_savedLoaded = true;   // isolate from the prefill test
        g_authCalls = 0;

        CHECK(login_gate::Locked(), "G1 a fresh gate starts locked");
        CHECK(login_gate::Key()[0] == '\0', "G1 ... with an empty key field");

        login_gate::Submit();
        CHECK(g_authCalls == 0, "G1 an empty key never reaches the authenticator");
        CHECK(login_gate::Locked(), "G1 ... and the gate stays locked");
        CHECK(std::strcmp(login_gate::Error(), "Paste your license key first") == 0,
              "G1 ... saying the key is missing");
    }

    // ---- G2: paste trims and fills ----------------------------------------
    {
        login_gate::Reset();
        login_gate::detail::g_savedLoaded = true;

        login_gate::PasteFromClipboard();
        CHECK(std::strcmp(login_gate::Key(), "AAAA-BBBB-CCCC-DDDD") == 0,
              "G2 PASTE trims the surrounding whitespace from the clipboard");
        CHECK(login_gate::Error()[0] == '\0', "G2 ... and clears the last error");
    }

    // ---- G3: a rejected key keeps the gate shut ----------------------------
    {
        login_gate::Reset();
        login_gate::detail::g_savedLoaded = true;
        g_authCalls = 0;
        g_saveCalls = 0;
        g_clipboard = "BAD-KEY";

        login_gate::PasteFromClipboard();
        login_gate::Submit();
        CHECK(login_gate::CurrentPhase() == login_gate::Checking,
              "G3 AUTHENTICATE moves to Checking and returns immediately");
        CHECK(WaitForAnswer(), "G3 the worker answers");
        CHECK(g_authCalls == 1, "G3 the authenticator ran exactly once");
        CHECK(g_lastChecked == "BAD-KEY", "G3 ... with the key from the field");
        CHECK(login_gate::Locked(), "G3 a rejected key leaves the gate locked");
        CHECK(std::strcmp(login_gate::Error(), "Invalid license key") == 0,
              "G3 the server's own message is shown");
        CHECK(g_saveCalls == 0 && g_savedKey.empty(),
              "G3 nothing is saved for a key that was not accepted");
    }

    // ---- G4: an accepted key opens the gate and is saved -------------------
    {
        login_gate::Reset();
        login_gate::detail::g_savedLoaded = true;
        g_authCalls = 0;
        g_saveCalls = 0;
        g_clipboard = "GOOD-KEY-1234";

        login_gate::PasteFromClipboard();
        login_gate::Submit();
        CHECK(WaitForAnswer(), "G4 the worker answers");

        CHECK(!login_gate::Locked(), "G4 OK opens the gate (Locked() is false)");
        CHECK(login_gate::CurrentPhase() == login_gate::Accepted,
              "G4 ... and the phase stays Accepted");
        CHECK(g_saveCalls == 1 && g_savedKey == "GOOD-KEY-1234",
              "G4 the accepted key is saved, exactly as checked");
        CHECK(login_gate::Error()[0] == '\0', "G4 no error is left on the card");

        // Idempotent: polling again (every frame) does not re-save or re-check.
        login_gate::Locked();
        login_gate::Locked();
        CHECK(g_saveCalls == 1 && g_authCalls == 1,
              "G4 later frames neither re-save nor re-authenticate");
    }

    // ---- G5: one press while a check is running ---------------------------
    {
        login_gate::Reset();
        login_gate::detail::g_savedLoaded = true;
        g_authCalls = 0;
        g_clipboard = "AAAA";

        login_gate::PasteFromClipboard();
        login_gate::Submit();
        login_gate::Submit();          // second tap while Checking
        login_gate::PasteFromClipboard();
        CHECK(WaitForAnswer(), "G5 the worker answers");
        CHECK(g_authCalls == 1, "G5 taps while Checking are ignored (one request)");
        CHECK(std::strcmp(login_gate::Key(), "AAAA") == 0,
              "G5 ... and the field cannot be overwritten mid-check");
    }

    // ---- G6: the saved key prefills the field once -------------------------
    {
        login_gate::Reset();
        login_gate::detail::g_savedLoaded = false;
        g_haveSaved = true;
        g_savedOnDisk = "SAVED-KEY-9999";
        g_loadCalls = 0;

        CHECK(login_gate::detail::LoadSavedKeyOnce(),
              "G6 the saved key loads on the first frame");
        CHECK(std::strcmp(login_gate::Key(), "SAVED-KEY-9999") == 0,
              "G6 ... and prefills the field");
        CHECK(!login_gate::detail::LoadSavedKeyOnce() && g_loadCalls == 1,
              "G6 it is read from disk only once per process");

        login_gate::Reset();
        login_gate::detail::g_savedLoaded = false;
        g_haveSaved = false;
        CHECK(!login_gate::detail::LoadSavedKeyOnce(),
              "G6 a device with no saved key keeps the field empty");
        g_haveSaved = true;
    }

    // ---- G7: touch press/release ------------------------------------------
    {
        login_gate::Reset();
        login_gate::detail::g_savedLoaded = true;
        g_authCalls = 0;
        g_clipboard = "TOUCH-KEY";

        // The rectangles Draw() publishes. Set by hand here: the card's layout
        // needs an ImGui frame, the hit logic does not.
        login_gate::detail::g_pasteMinX = 100; login_gate::detail::g_pasteMinY = 200;
        login_gate::detail::g_pasteMaxX = 300; login_gate::detail::g_pasteMaxY = 244;
        login_gate::detail::g_authMinX  = 100; login_gate::detail::g_authMinY  = 260;
        login_gate::detail::g_authMaxX  = 300; login_gate::detail::g_authMaxY  = 304;

        // Press and release on PASTE.
        login_gate::OnTouch(150, 210, true);
        login_gate::OnTouch(150, 210, false);
        CHECK(std::strcmp(login_gate::Key(), "TOUCH-KEY") == 0,
              "G7 press+release on PASTE fires it");

        // Press on PASTE, drag off, release elsewhere: nothing.
        login_gate::Reset();
        login_gate::detail::g_savedLoaded = true;
        login_gate::OnTouch(150, 210, true);
        login_gate::OnTouch(150, 600, false);
        CHECK(login_gate::Key()[0] == '\0', "G7 dragging off the button cancels it");

        // Release without a press (a stray Up) is ignored.
        login_gate::OnTouch(150, 210, false);
        CHECK(login_gate::Key()[0] == '\0', "G7 a release with no press does nothing");

        // Press on AUTHENTICATE: submits the pasted key.
        g_clipboard = "GOOD-KEY-1234";
        login_gate::OnTouch(150, 210, true);
        login_gate::OnTouch(150, 210, false);
        login_gate::OnTouch(150, 270, true);
        login_gate::OnTouch(150, 270, false);
        CHECK(login_gate::CurrentPhase() == login_gate::Checking,
              "G7 press+release on AUTHENTICATE submits");
        CHECK(WaitForAnswer(), "G7 the worker answers");
        CHECK(!login_gate::Locked(), "G7 ... and the touch path opens the gate too");
    }

    if (g_failures == 0)
        printf("OK: all login-gate checks passed\n");
    else
        printf("%d check(s) FAILED\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
