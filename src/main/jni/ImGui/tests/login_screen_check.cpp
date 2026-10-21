#include <cstdio>
#include <cstring>
#include <cmath>
#include <string>

#define IMGUI_DEFINE_MATH_OPERATORS
#include "ImGui/imgui.h"
#include "ImGui/login_screen.h"

namespace font { ImFont* inter_semibold = nullptr; }
ImFont* F50 = nullptr;

static char g_key[256];
static bool g_reveal = false;
static bool g_remember = true;
static int g_failures = 0;

static void Check(const char* what, bool ok)
{
    std::printf("%s: %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok) ++g_failures;
}

static login_screen::Result Frame(const ImVec2& pos, float W, float H)
{
    ImGuiIO& io = ImGui::GetIO();
    io.DeltaTime = 1.0f / 60.0f;

    ImGui::NewFrame();
    ImGui::SetNextWindowPos(pos);
    ImGui::SetNextWindowSize(ImVec2(W, H));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    ImGui::Begin("##premium_login", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoMove);

    login_screen::Params p;
    p.pos = pos;
    p.W = W;
    p.H = H;
    p.key = g_key;
    p.keySize = (int)sizeof(g_key);
    p.reveal = &g_reveal;
    p.remember = &g_remember;
    p.error = "";
    p.art = 0;
    p.artW = 0;
    p.artH = 0;

    login_screen::Result r = login_screen::Render(p);

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::Render();
    return r;
}

static void MoveTo(ImVec2 p)
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent(p.x, p.y);
    io.MousePos = p;
}

static void Tap(const ImVec2& pos, float W, float H, const ImVec2& at, login_screen::Result* tapped = nullptr)
{
    ImGuiIO& io = ImGui::GetIO();

    MoveTo(at);
    Frame(pos, W, H);
    io.AddMouseButtonEvent(0, true);
    ImGui::GetIO().MouseDown[0] = true;
    login_screen::Result r = Frame(pos, W, H);
    io.AddMouseButtonEvent(0, false);
    ImGui::GetIO().MouseDown[0] = false;
    login_screen::Result r2 = Frame(pos, W, H);
    if (tapped)
    {
        tapped->authenticate = r.authenticate || r2.authenticate;
        tapped->paste = r.paste || r2.paste;
        tapped->close = r.close || r2.close;
    }
    Frame(pos, W, H);
}

static void TestPanelIsMenuSized()
{
    const ImVec2 surfaces[] = { ImVec2(1280, 720), ImVec2(2400, 1080),
                                ImVec2(640, 480),  ImVec2(1920, 1080) };

    for (const ImVec2& d : surfaces)
    {
        const login_screen::Layout l = login_screen::ComputeLayout(ImVec2(0, 0), d.x, d.y, false);
        const MenuWindow w = ComputeMenuWindow(d);

        char what[160];
        const float pw = l.cardMax.x - l.cardMin.x, ph = l.cardMax.y - l.cardMin.y;

        std::snprintf(what, sizeof what, "%.0fx%.0f: panel is the menu's width", d.x, d.y);
        Check(what, std::fabs(pw - w.size.x) < 0.5f);
        std::snprintf(what, sizeof what, "%.0fx%.0f: panel is the menu's height", d.x, d.y);
        Check(what, std::fabs(ph - w.size.y) < 0.5f);
        std::snprintf(what, sizeof what, "%.0fx%.0f: panel is the menu's position", d.x, d.y);
        Check(what, std::fabs(l.cardMin.x - w.pos.x) < 0.5f &&
                    std::fabs(l.cardMin.y - w.pos.y) < 0.5f);
        std::snprintf(what, sizeof what, "%.0fx%.0f: panel leaves the surface around it visible", d.x, d.y);
        Check(what, pw < d.x - 1.0f && ph < d.y - 1.0f);

        std::snprintf(what, sizeof what, "%.0fx%.0f: panel stays within 900x592", d.x, d.y);
        Check(what, pw <= 900.5f && ph <= 592.5f);
    }
}

int main()
{
    const float W = 1280.0f, H = 720.0f;
    const ImVec2 pos(0.0f, 0.0f);

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.DisplaySize = ImVec2(1280.0f, 720.0f);
    io.DeltaTime = 1.0f / 60.0f;
    io.BackendPlatformName = "headless";

    login_screen::LoadFonts(io);
    io.Fonts->Build();

    TestPanelIsMenuSized();

    const login_screen::Layout L = login_screen::ComputeLayout(pos, W, H, false);

    for (int i = 0; i < 3; ++i) Frame(pos, W, H);

    Tap(pos, W, H, L.fieldHitCenter());

    g_key[0] = '\0';
    for (const char* c = "ETHNIR-KEY-1"; *c; ++c)
        io.AddInputCharacter((unsigned int)*c);
    Frame(pos, W, H);
    Frame(pos, W, H);
    Check("typing into the field lands in the access key buffer", std::strcmp(g_key, "ETHNIR-KEY-1") == 0);
    if (std::strcmp(g_key, "ETHNIR-KEY-1") != 0)
        std::printf("     buffer was \"%s\"\n", g_key);

    const bool beforeReveal = g_reveal;
    Tap(pos, W, H, L.eyeCenter());
    Check("reveal eye toggles", g_reveal != beforeReveal);
    Tap(pos, W, H, L.eyeCenter());
    Check("reveal eye toggles back", g_reveal == beforeReveal);

    const bool beforeRemember = g_remember;
    Tap(pos, W, H, L.rememberCenter());
    Check("remember me toggles", g_remember != beforeRemember);

    login_screen::Result clicked;
    Tap(pos, W, H, L.pillCenter(), &clicked);
    Check("the pill reports authenticate", clicked.authenticate);

    login_screen::Result paste;
    Tap(pos, W, H, L.pasteCenter(), &paste);
    Check("the paste row reports paste", paste.paste);

    login_screen::Result closed;
    Tap(pos, W, H, L.closeCenter(), &closed);
    Check("the title bar's X reports close", closed.close);

    Tap(pos, W, H, L.fieldHitCenter());
    io.AddKeyEvent(ImGuiKey_Enter, true);
    login_screen::Result entered = Frame(pos, W, H);
    io.AddKeyEvent(ImGuiKey_Enter, false);
    Frame(pos, W, H);
    Check("Enter in the field submits", entered.authenticate);

    ImGui::DestroyContext();

    if (g_failures)
    {
        std::printf("FAIL: %d check(s)\n", g_failures);
        return 1;
    }
    std::printf("OK: login card input\n");
    return 0;
}

