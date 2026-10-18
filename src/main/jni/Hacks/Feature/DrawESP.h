#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCDFAInspection"
#pragma once
#include "ToString.h"
#include "Icon.h"
#include "../XPremium/Skeleton.h"

#include <malloc.h>
#include <errno.h>
#include <stdarg.h>
#include <array>
#include <vector>


#define RAD2DEG( x )  ( (float)(x) * (float)(180.f / IM_PI) )
#define DEG2RAD( x ) ( (float)(x) * (float)(IM_PI / 180.f) )

extern ImFont* F50;
extern float speedHackMultiplier;

void SmartReloadMarkShotFired();

void (*oWeaponFireComponent_Instant_CreateBulletLine)(uintptr_t thiz, Vector3 startPos, Vector3 dir, bool isDualFire) = nullptr;
void (*oWeaponFireComponent_Instant_CreateBulletProjectile)(void* thiz, Vector3 startPos, Vector3 dir, void* weaponImpact, int itemID, int flySmokeAssetID, bool enableVirtualStartPos, Vector3 virtualStartPos) = nullptr;

void NewFeatures_Hook_CreateBulletLine(uintptr_t thiz, Vector3 startPos, Vector3 dir, bool isDualFire);
void NewFeatures_Hook_CreateBulletProjectile(void* thiz, Vector3 startPos, Vector3 dir, void* weaponImpact, int itemID, int flySmokeAssetID, bool enableVirtualStartPos, Vector3 virtualStartPos);

void DrawText1(ImDrawList *draw, const std::string &text, const Vector2 &position, ImU32 color, float fontSize) {
    draw->AddText(NULL, fontSize, {position.x, position.y}, color, text.c_str());
}

void DrawTextWithBorder1(ImDrawList *draw, const std::string &text, const Vector2 &position, ImU32 textColor, ImU32 borderColor, float fontSize) {
    float borderSize = 1.0f;
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            if (x == 0 && y == 0)
                continue;
            DrawText1(draw, text, {position.x + x * borderSize, position.y + y * borderSize}, borderColor, fontSize);
        }
    }
    DrawText1(draw, text, position, textColor, fontSize);
}

#include "System/Hooks/ThrowableAlert.h"
#include "System/Hooks/ArmorEsp.h"
#include "BRClassEsp.h"
#include "NoGravity.h"

void RotateTriangle(std::array<Vector3, 3> & points, float rotation) {
    const auto points_center = (points.at(0) + points.at(1) + points.at(2)) / 3;
    for (auto & point : points) {
        point = point - points_center;
        const auto temp_x = point.x;
        const auto temp_y = point.y;
        const auto theta = DEG2RAD(rotation);
        const auto c = cosf(theta);
        const auto s = sinf(theta);
        point.x = temp_x * c - temp_y * s;
        point.y = temp_x * s + temp_y * c;
        point = point + points_center;
    }
}

void VectorAnglesRadar(Vector3 & forward, Vector3 & angles) {
    if (forward.x == 0.f && forward.y == 0.f) {
        angles.x = forward.z > 0.f ? -90.f : 90.f;
        angles.y = 0.f;
    } else {
        angles.x = RAD2DEG(atan2(-forward.z, forward.Magnitude(forward)));
        angles.y = RAD2DEG(atan2(forward.y, forward.x));
    }
    angles.z = 0.f;
}

char extra[30];
int atas, kanan;

static inline int ColorToU8(float v) {
    if (v <= 1.0f) v *= 255.0f;
    if (v < 0.0f) v = 0.0f;
    if (v > 255.0f) v = 255.0f;
    return (int)(v + 0.5f);
}

int32_t ToColor(float *col) {
    if (!col) return IM_COL32(255, 255, 255, 255);
    return IM_COL32(
        ColorToU8(col[0]),
        ColorToU8(col[1]),
        ColorToU8(col[2]),
        ColorToU8(col[3])
    );
}

int totalBots, totalEnemies;
bool isEspReady;

static inline void ApplySpeedhack() {
    static float lastAppliedScale = 1.0f;
    const float targetScale = (speedHackMultiplier <= 0.0f) ? 1.0f : speedHackMultiplier;
    if (fabsf(targetScale - lastAppliedScale) <= 0.001f) return;

    auto Time_set_timeScale = reinterpret_cast<void (*)(float)>(Class_Time_set_timeScale);
    if (!Time_set_timeScale) return;

    Time_set_timeScale(targetScale);
    lastAppliedScale = targetScale;
}

// ---------------------------------------------------------------------------
// Tools::IsPtrValid() only answers "is this memory mapped and readable". A
// UnityEngine.Object the game has already Destroyed keeps its MANAGED wrapper
// sitting in readable heap - so every pointer guard passes - while the native
// object behind it is gone: Object.m_CachedPtr (dump.cs 169503) is cleared the
// moment the native side dies. The enemy pawn list keeps such an entry for a
// few frames (death, respawn, match end), which is the window the report
// describes: "~10 s after joining a match that has bots in it". Asking a
// destroyed Pawn / Transform / Camera for anything then walks a null native
// pointer and takes the client down.
//
// Same liveness rule the triggerbot uses (NewFeatures::A_FireObjectAlive); it
// lives here too because DrawESP.h is parsed before NewFeatures.h.
// Pawn, Transform and Camera all derive from UnityEngine.Object, so one offset
// covers them.
// ---------------------------------------------------------------------------
static inline bool EspObjectAlive(const void *obj) {
    if (!Tools::IsPtrValid((void *)obj))
        return false;
    return *(const void **)((const uintptr_t)obj + 0x10) != nullptr;
}

uintptr_t GetClosestTarget() {
    uintptr_t result = 0;
    float MaxDist = std::numeric_limits<float>::infinity();
    auto Gameplay_get_MatchGame = (uintptr_t (*)()) (Class_Gameplay_get_MatchGame);
    auto get_MatchGame = Gameplay_get_MatchGame();
    if (Tools::IsPtrValid((void *) get_MatchGame)) {
        auto Gameplay_get_LocalPawn = (uintptr_t (*)()) (Class_Gameplay_get_LocalPawn);
        auto LocalPawn = Gameplay_get_LocalPawn();
        if (LocalPawn) {
            Vector3 MyPos{0, 0, 0};
            auto local_m_Mesh = *(Transform **) (LocalPawn + Class_Pawn_m_Mesh);
            if (EspObjectAlive((void *) local_m_Mesh)) {
                MyPos = local_m_Mesh->get_position();
            }
            auto EnemyPawns = *(List<uintptr_t> **) (get_MatchGame + Class_BaseGame_EnemyPawns);
            if (Tools::IsPtrValid((void *) EnemyPawns)) {
                auto Items = EnemyPawns->getItems();
                const int pawnCount = EnemyPawns->getSize();
                if (Tools::IsPtrValid((void *) Items) && pawnCount > 0 && pawnCount <= 1024) {
                    for (int i = 0; i < pawnCount; i++) {
                        auto Pawn = Items[i];
                        if (EspObjectAlive((void *) Pawn)) {
                            if (!*(bool *) (Pawn + Class_Pawn_m_IsAlive))
                                continue;
                            auto m_Mesh = *(Transform **) (Pawn + Class_Pawn_m_Mesh);
                            if (!EspObjectAlive((void *) m_Mesh))
                                continue;
                            auto RootPos = m_Mesh->get_position();
                            float Distance = Vector3::Distance(MyPos, RootPos);
                            if (Distance < MaxDist) {
                                result = Pawn;
                                MaxDist = Distance;
                            }
                        }
                    }
                }
            }
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// The two per-pawn field reads the ESP loop used to do bare, every frame, for
// every pawn. Both go null in exactly the window the report describes ("~10 s
// after joining a match that has bots in it"): a pawn that just died or is
// respawning keeps its managed wrapper readable - so Tools::IsPtrValid() on the
// pawn still passes - while the game has already cleared m_AttackableInfo and
// the name string hanging off its PlayerInfo. Reading `m_AttackableInfo +
// m_Health` then dereferenced null, which is a SIGSEGV inside the render loop
// with no user action.
// ---------------------------------------------------------------------------
static inline bool EspPawnHealth(uintptr_t pawn, int &curHp, int &maxHp) {
    if (!Tools::IsPtrValid((void *)pawn))
        return false;
    auto attackableInfo = *(uintptr_t *)(pawn + Class_AttackableTarget_m_AttackableInfo);
    if (!Tools::IsPtrValid((void *)attackableInfo))
        return false;
    curHp = (int)*(float *)(attackableInfo + Class_AttackableTarget_m_Health);
    maxHp = (int)*(float *)(attackableInfo + Class_AttackableTarget_m_MaxHealth);
    return true;
}

// The name string needs the same treatment: Pawn.get_PlayerName() is a native
// call that answers null for a pawn whose PlayerInfo is gone (bots, respawns),
// and the old call site ran String::CString() on that null.
static inline std::string EspPawnName(Pawn *pawn) {
    if (!Tools::IsPtrValid((void *)pawn))
        return std::string();
    if (pawn->m_IsBot())
        return std::string("BOT");
    String *playerName = pawn->get_PlayerName();
    if (!Tools::IsPtrValid((void *)playerName))
        return std::string();
    return std::string(playerName->CString());
}

bool isInsideFOV(int x, int y) {
    if (!Config.Aim.size)
        return true;
    int circle_x = get_width() / 2;
    int circle_y = get_height() / 2;
    int rad = Config.Aim.size;
    return (x - circle_x) * (x - circle_x) + (y - circle_y) * (y - circle_y) <= rad * rad;
}

uintptr_t GetInsideFOVTarget() {
    uintptr_t result = 0;
    float MaxDist = std::numeric_limits<float>::infinity();
    auto Gameplay_get_MatchGame = (uintptr_t (*)()) (Class_Gameplay_get_MatchGame);
    auto get_MatchGame = Gameplay_get_MatchGame();
    if (Tools::IsPtrValid((void *) get_MatchGame)) {
        auto Gameplay_get_LocalPawn = (uintptr_t (*)()) (Class_Gameplay_get_LocalPawn);
        auto LocalPawn = Gameplay_get_LocalPawn();
        if (LocalPawn) {
            Vector3 MyPos{0, 0, 0};
            auto local_m_Mesh = *(Transform **) (LocalPawn + Class_Pawn_m_Mesh);
            if (EspObjectAlive((void *) local_m_Mesh)) {
                MyPos = local_m_Mesh->get_position();
            }
            auto EnemyPawns = *(List<uintptr_t> **) (get_MatchGame + Class_BaseGame_EnemyPawns);
            Camera *fovCam = Camera::get_main();
            if (!EspObjectAlive((void *) fovCam)) return 0;
            if (Tools::IsPtrValid((void *) EnemyPawns)) {
                auto Items = EnemyPawns->getItems();
                const int pawnCount = EnemyPawns->getSize();
                if (Tools::IsPtrValid((void *) Items) && pawnCount > 0 && pawnCount <= 1024) {
                    for (int i = 0; i < pawnCount; i++) {
                        auto Pawn = Items[i];
                        if (EspObjectAlive((void *) Pawn)) {
                            auto isAlive = *(bool*) ((uintptr_t)Pawn + Class_Pawn_m_IsAlive);
                            if (!isAlive)
                                continue;
                            auto m_HeadBone = *(Transform **) (Pawn + Class_Pawn_m_HeadBone);
                            if (!EspObjectAlive((void *) m_HeadBone))
                                continue;
                            auto HeadSc = fovCam->WorldToScreenPoint(m_HeadBone->get_position());
                            Vector2 v2Middle = Vector2((float) (get_width() / 2), (float) (get_height() / 2));
                            Vector2 v2Loc = Vector2(HeadSc.x, HeadSc.y);
                            if (isInsideFOV((int) HeadSc.x, (int) HeadSc.y)) {
                                float Distance = Vector2::Distance(v2Middle, v2Loc);
                                if (Distance < MaxDist) {
                                    result = Pawn;
                                    MaxDist = Distance;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return result;
}

void DrawAimLine(ImDrawList* draw, int sWidth, int sHeight) {
    uintptr_t Target = 0;
    if (Config.Aim.By == EAim::Distance) {
        Target = GetClosestTarget();
    } else if (Config.Aim.By == EAim::Crosshair) {
        Target = GetInsideFOVTarget();
    }
    if (Target) {
        Vector3 targetPos;
        auto m_HeadBone = *(Transform**)(Target + Class_Pawn_m_HeadBone);
        if (!EspObjectAlive((void *) m_HeadBone)) return;
        Camera *aimCam = Camera::get_main();
        if (!EspObjectAlive((void *) aimCam)) return;
        if (Config.Aim.Target == EAimTarget::Heads) {
            targetPos = m_HeadBone->get_position();
        } else if (Config.Aim.Target == EAimTarget::Chests) {
            targetPos = m_HeadBone->get_position();
            targetPos.y -= 0.2f;
        } else if (Config.Aim.Target == EAimTarget::Body) {
            targetPos = m_HeadBone->get_position();
            targetPos.y -= 0.4f;
        }
        auto HeadSc = aimCam->WorldToScreenPoint(targetPos);
        if (HeadSc.z > 0) {
            ImVec2 center(sWidth / 2, sHeight / 2);
            draw->AddLine(center, ImVec2(HeadSc.x, sHeight - HeadSc.y), ToColor(Config.sColorsESPPLAYER.LinePLAYER), Config.sESPMenuLineScale.lineSize);
        }
    }
}

ImVec2 pushToScreenBorder(ImVec2 Pos, ImVec2 screen, int borders, int offset) {
    float x = Pos.x;
    float y = Pos.y;
    if ((borders & 1) == 1) {
        y = 0 - offset;
    }
    if ((borders & 2) == 2) {
        x = screen.x + offset;
    }
    if ((borders & 4) == 4) {
        y = screen.y + offset;
    }
    if ((borders & 8) == 8) {
        x = 0 - offset;
    }
    return ImVec2(x, y);
}

int isOutsideSafezone(ImVec2 pos, ImVec2 screen) {
    ImVec2 mSafezoneTopLeft(screen.x * 0.04f, screen.y * 0.04f);
    ImVec2 mSafezoneBottomRight(screen.x * 0.96f, screen.y * 0.96f);
    int result = 0;
    if (pos.y < mSafezoneTopLeft.y) {
        result |= 1;
    }
    if (pos.x > mSafezoneBottomRight.x) {
        result |= 2;
    }
    if (pos.y > mSafezoneBottomRight.y) {
        result |= 4;
    }
    if (pos.x < mSafezoneTopLeft.x) {
        result |= 8;
    }
    return result;
}

class FPSCounter {
protected:
    unsigned int m_fps;
    unsigned int m_fpscount;
    long m_fpsinterval;

public:
    FPSCounter() : m_fps(0), m_fpscount(0), m_fpsinterval(0) {
    }

    void update() {
        m_fpscount++;
        if (m_fpsinterval < time(0)) {
            m_fps = m_fpscount;
            m_fpscount = 0;
            m_fpsinterval = time(0) + 1;
        }
    }

    unsigned int get() const {
        return m_fps;
    }
};

FPSCounter fps;

ImColor outlinecolor = IM_COL32(10, 10, 10, 255);

void HandleEnemyInfo(ImDrawList *draw) {
    Vector3 pLocalPawn_rootPos = Vector3::zero();
    Pawn *get_LocalPawn = GamePlay::get_LocalPawn();
    if (EspObjectAlive(get_LocalPawn)) {
        pLocalPawn_rootPos = get_LocalPawn->get_LastPawnPos();
    }
    
    int enemyBots = 0;
    int enenyEnemies = 0;
    
    BaseGame *get_MatchGame = GamePlay::get_MatchGame();
    if (Tools::IsPtrValid((void *) get_MatchGame)) {
        List<Pawn *> *EnemyPawns = get_MatchGame->EnemyPawns();
        if (Tools::IsPtrValid((void *) EnemyPawns)) {
            Pawn **pawns = (Pawn **) EnemyPawns->getItems();
            const bool pawnsValid = Tools::IsPtrValid((void *) pawns);
            const int pawnCount = pawnsValid ? EnemyPawns->getSize() : 0;
            if (pawnsValid && pawnCount > 0 && pawnCount <= 1024) {
            for (int i = 0; i < pawnCount; i++) {
                Pawn *pawn = pawns[i];
                if (EspObjectAlive(pawn)) {
                    Vector3 pEnemyPawn_headPos = pawn->get_HeadPosition();
                    Vector3 pEnemyPawn_rootPos = pawn->get_LastPawnPos();
                    
                    bool isBot = *(bool*)((uintptr_t)pawn + api24);
                    if (isBot) {
                        enemyBots++;
                    } else {
                        enenyEnemies++;
                    }
                    
                    bool isAlive = *(bool*)((uintptr_t)pawn + api25);
                    if (isAlive) {
                        enemyBots++;
                    } else {
                        enenyEnemies++;
                    }
                    
                    auto mainCamera = Camera::get_main();
                    if (mainCamera) {
                        Vector3 HeadSc = mainCamera->WorldToScreenPoint(pEnemyPawn_headPos);
                        Vector3 RootSc = mainCamera->WorldToScreenPoint(pEnemyPawn_rootPos);
                        
                        AttackableTargetInfo *m_AttackableInfo = pawn->m_AttackableInfo();
                        if (m_AttackableInfo != nullptr) {
                            bool isBot = false;
                            try {
                                isBot = *(bool*)((uintptr_t)pawn + api24);
                            } catch (...) {
                                isBot = true;
                            }
                            
                            bool isAlive = false;
                            try {
                                isAlive = *(bool*)((uintptr_t)pawn + api25);
                            } catch (...) {
                                isAlive = true;
                            }
                            
                            if (Tools::IsPtrValid(pawn) && Tools::IsPtrValid(pawn->get_PlayerName())) {
                                std::string playerName = pawn->get_PlayerName()->CString();
                                
                                auto textSize = ImGui::CalcTextSize(playerName.c_str(), 0, 18.0f);
                                float namelength = 14.0f;
                                if (playerName.length() <= 13) {
                                    namelength = 14.f;
                                } else if (playerName.length() <= 17) {
                                    namelength = 12.f;
                                } else if (playerName.length() <= 20) {
                                    namelength = 11.f;
                                } else if (playerName.length() <= 25) {
                                    namelength = 10.5f;
                                } else {
                                    namelength = 10.f;
                                }
                                
                                if (HeadSc.z < 0) continue;
                                DrawTextWithBorder1(draw, playerName, {HeadSc.x - 43.0f, glHeight - HeadSc.y - 28.0f}, IM_COL32(255, 255, 255, 255), outlinecolor, 18.0f);
                                
                            }
                        }
                    }
                }
            }
            }
        }
    }
}

static inline void DrawBoxEnemy(ImDrawList *draw, ImVec2 X, ImVec2 Y, float thicc, int color) {
    draw->AddLine({X.x, X.y}, {Y.x, Y.y}, color, thicc);
}

static inline void DrawEspBoxOutline(ImDrawList *draw, float x, float y, float w, float h, ImU32 color, float thickness) {
    draw->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), color, 0.0f, 0, thickness);
}

static inline void DrawEspBoxFilled(ImDrawList *draw, float x, float y, float w, float h, ImU32 color, float thickness) {
    ImVec4 c = ImGui::ColorConvertU32ToFloat4(color);
    draw->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), ImGui::ColorConvertFloat4ToU32(ImVec4(c.x, c.y, c.z, 0.16f)));
    draw->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), color, 0.0f, 0, thickness);
}

static inline void DrawEspBoxCorner(ImDrawList *draw, float x, float y, float w, float h, ImU32 color, float thickness) {
    float iw = w / 4.0f;
    float ih = h / 4.0f;
    draw->AddLine(ImVec2(x, y), ImVec2(x + iw, y), color, thickness);
    draw->AddLine(ImVec2(x + w - iw, y), ImVec2(x + w, y), color, thickness);
    draw->AddLine(ImVec2(x, y), ImVec2(x, y + ih), color, thickness);
    draw->AddLine(ImVec2(x + w - 1.0f, y), ImVec2(x + w - 1.0f, y + ih), color, thickness);
    draw->AddLine(ImVec2(x, y + h), ImVec2(x + iw, y + h), color, thickness);
    draw->AddLine(ImVec2(x + w - iw, y + h), ImVec2(x + w, y + h), color, thickness);
    draw->AddLine(ImVec2(x, y + h - ih), ImVec2(x, y + h), color, thickness);
    draw->AddLine(ImVec2(x + w - 1.0f, y + h - ih), ImVec2(x + w - 1.0f, y + h), color, thickness);
}

static inline void DrawEspVerticalHealthBar(ImDrawList *draw, float x, float y, float h, int curHP, int maxHP) {
    if (maxHP <= 0) return;
    float ratio = ImClamp(curHP / (float)maxHP, 0.0f, 1.0f);
    int r = (int)std::min(510.0f * (1.0f - ratio), 255.0f);
    int g = (int)std::min(510.0f * ratio, 255.0f);
    ImU32 hpColor = IM_COL32(r, g, 0, 255);
    const float barW = 4.0f;
    draw->AddRectFilled(ImVec2(x, y), ImVec2(x + barW, y + h), IM_COL32(0, 0, 0, 120));
    draw->AddRect(ImVec2(x, y), ImVec2(x + barW, y + h), IM_COL32(0, 0, 0, 220), 0.0f, 0, 1.0f);
    draw->AddRectFilled(ImVec2(x, y + h * (1.0f - ratio)), ImVec2(x + barW, y + h), hpColor);
}

static inline void DrawEsp3DBox(ImDrawList *draw, Vector3 rootPos, int glHeight, ImU32 color, float thickness) {
    Camera* mainCamera = Camera::get_main();
    if (!mainCamera) return;

    Vector3 minBounds(-0.5f, 0.0f, -0.5f);
    Vector3 maxBounds(0.5f, 1.8f, 0.5f);
    Vector3 corners[8] = {
        rootPos + Vector3(minBounds.x, minBounds.y, minBounds.z),
        rootPos + Vector3(maxBounds.x, minBounds.y, minBounds.z),
        rootPos + Vector3(maxBounds.x, maxBounds.y, minBounds.z),
        rootPos + Vector3(minBounds.x, maxBounds.y, minBounds.z),
        rootPos + Vector3(minBounds.x, minBounds.y, maxBounds.z),
        rootPos + Vector3(maxBounds.x, minBounds.y, maxBounds.z),
        rootPos + Vector3(maxBounds.x, maxBounds.y, maxBounds.z),
        rootPos + Vector3(minBounds.x, maxBounds.y, maxBounds.z)
    };

    Vector3 screenCorners[8];
    for (int i = 0; i < 8; ++i) {
        screenCorners[i] = mainCamera->WorldToScreenPoint(corners[i]);
        if (screenCorners[i].z <= 0.0f) return;
        screenCorners[i].y = glHeight - screenCorners[i].y;
    }

    auto Draw3DLine = [&](int a, int b) {
        draw->AddLine(ImVec2(screenCorners[a].x, screenCorners[a].y), ImVec2(screenCorners[b].x, screenCorners[b].y), color, thickness);
    };

    Draw3DLine(0, 1); Draw3DLine(1, 5); Draw3DLine(5, 4); Draw3DLine(4, 0);
    Draw3DLine(3, 2); Draw3DLine(2, 6); Draw3DLine(6, 7); Draw3DLine(7, 3);
    Draw3DLine(0, 3); Draw3DLine(1, 2); Draw3DLine(4, 7); Draw3DLine(5, 6);
}

static inline void DrawEspCylinder(ImDrawList* draw, Vector3 rootPos, int glHeight, ImU32 color) {
    Camera* mainCamera = Camera::get_main();
    if (!mainCamera) return;

    ImVec4 c = ImGui::ColorConvertU32ToFloat4(color);
    ImU32 edgeColor = ImGui::ColorConvertFloat4ToU32(ImVec4(c.x, c.y, c.z, 0.72f));
    ImU32 fillColor = ImGui::ColorConvertFloat4ToU32(ImVec4(c.x, c.y, c.z, 0.18f));
    const float height = 1.8f;
    const float radius = 0.6f;
    const int segments = 40;

    auto Project = [&](Vector3 world) -> ImVec2 {
        Vector3 screen = mainCamera->WorldToScreenPoint(world);
        if (screen.z <= 0.01f) return ImVec2(-9999.0f, -9999.0f);
        return ImVec2(screen.x, glHeight - screen.y);
    };

    std::vector<ImVec2> bottom;
    std::vector<ImVec2> top;
    for (int i = 0; i < segments; ++i) {
        float angle = (2.0f * IM_PI * i) / segments;
        float dx = cosf(angle) * radius;
        float dz = sinf(angle) * radius;
        ImVec2 b = Project(rootPos + Vector3(dx, 0.0f, dz));
        ImVec2 t = Project(rootPos + Vector3(dx, height, dz));
        if (b.x < 0.0f || t.x < 0.0f) return;
        bottom.push_back(b);
        top.push_back(t);
    }

    for (int i = 0; i < segments; ++i) {
        int next = (i + 1) % segments;
        ImVec2 quad[4] = { bottom[i], bottom[next], top[next], top[i] };
        draw->AddConvexPolyFilled(quad, 4, fillColor);
        draw->AddLine(bottom[i], bottom[next], edgeColor, 1.8f);
        draw->AddLine(top[i], top[next], edgeColor, 1.8f);
        draw->AddLine(bottom[i], top[i], edgeColor, 1.0f);
    }
}

static inline void DrawEspSignal(ImDrawList* draw, Vector3 rootPos, int glHeight, ImU32 color) {
    Camera* mainCamera = Camera::get_main();
    if (!mainCamera) return;

    ImVec4 c = ImGui::ColorConvertU32ToFloat4(color);
    ImU32 outerColor = ImGui::ColorConvertFloat4ToU32(ImVec4(c.x, c.y, c.z, 0.65f));
    ImU32 fillColor = ImGui::ColorConvertFloat4ToU32(ImVec4(c.x, c.y, c.z, 0.10f));
    const int segments = 56;
    const float groundY = rootPos.y + 0.03f;

    auto BuildRing = [&](float radius, std::vector<ImVec2>& out) -> bool {
        out.clear();
        out.reserve(segments);
        for (int i = 0; i < segments; ++i) {
            float angle = (2.0f * IM_PI * i) / segments;
            Vector3 world(rootPos.x + cosf(angle) * radius, groundY, rootPos.z + sinf(angle) * radius);
            Vector3 screen = mainCamera->WorldToScreenPoint(world);
            if (screen.z <= 0.01f) return false;
            out.emplace_back(screen.x, glHeight - screen.y);
        }
        return out.size() >= 3;
    };

    std::vector<ImVec2> outerRing;
    std::vector<ImVec2> innerRing;
    if (!BuildRing(1.2f, outerRing)) return;
    if (!BuildRing(0.7f, innerRing)) innerRing.clear();

    draw->AddConvexPolyFilled(outerRing.data(), (int)outerRing.size(), fillColor);
    draw->AddPolyline(outerRing.data(), (int)outerRing.size(), outerColor, true, 2.2f);
    if (!innerRing.empty()) {
        draw->AddPolyline(innerRing.data(), (int)innerRing.size(), outerColor, true, 1.6f);
    }
}

static inline void TeleportToEnemy() {
    if (!Config.TeleportEnemy) return;
    
    BaseGame *get_MatchGame = GamePlay::get_MatchGame();
    if (!Tools::IsPtrValid(get_MatchGame)) return;
    
    Pawn *get_LocalPawn = GamePlay::get_LocalPawn();
    if (!EspObjectAlive(get_LocalPawn)) return;
    
    Vector3 localPos = get_LocalPawn->get_LastPawnPos();
    
    List<Pawn *> *EnemyPawns = get_MatchGame->EnemyPawns();
    if (!Tools::IsPtrValid(EnemyPawns) || !EnemyPawns->getSize()) return;
    
    Pawn *closestEnemy = nullptr;
    float minDistance = 100.0f;
    
    Pawn **pawns = (Pawn **)EnemyPawns->getItems();
    if (!Tools::IsPtrValid(pawns)) return;
    const int pawnCount = EnemyPawns->getSize();
    for (int i = 0; i < pawnCount; i++) {
        Pawn *pawn = pawns[i];
        if (!EspObjectAlive(pawn)) continue;
        
        bool isAlive = *(bool *)((uintptr_t)pawn + Class_Pawn_m_IsAlive);
        if (!isAlive) continue;
        
        Vector3 enemyPos = pawn->get_LastPawnPos();
        float distance = Vector3::Distance(localPos, enemyPos) / 70.0f;
        
        if (distance < minDistance) {
            minDistance = distance;
            closestEnemy = pawn;
        }
    }
    
    if (closestEnemy) {
        Vector3 targetPos = closestEnemy->get_LastPawnPos();
        
        Vector3 direction = localPos - targetPos;
        direction.y = 0;
        direction = direction.Normalize();
        
        Vector3 teleportPos = targetPos + (direction * 1.5f);
        
        Transform *localTransform = get_LocalPawn->get_transform();
        if (Tools::IsPtrValid(localTransform)) {
            auto _Transform_set_position = (void (*)(Transform *, Vector3))(Class_Transform_set_position);
            if (_Transform_set_position) {
                _Transform_set_position(localTransform, teleportPos);
            }
        }
    }
}

static inline void DrawVehicleESP(ImDrawList *draw, int sWidth, int sHeight) {
    if (!Config.ESPMenu.Vehicle) return;
    
    BaseGame *get_MatchGame = GamePlay::get_MatchGame();
    if (!Tools::IsPtrValid(get_MatchGame)) return;
    
    Pawn *get_LocalPawn = GamePlay::get_LocalPawn();
    Vector3 pLocalPawn_rootPos = Vector3::zero();
    if (EspObjectAlive(get_LocalPawn)) {
        pLocalPawn_rootPos = get_LocalPawn->get_LastPawnPos();
    }
    
    List<PhysicsVehicle *> **m_AllVehicles_ptr = reinterpret_cast<List<PhysicsVehicle *> **>((uintptr_t)get_MatchGame + Class_BaseGame__m_AllVehicles);
    if (!Tools::IsPtrValid(m_AllVehicles_ptr)) return;
    
    List<PhysicsVehicle *> *m_AllVehicles = *m_AllVehicles_ptr;
    if (!Tools::IsPtrValid(m_AllVehicles) || !m_AllVehicles->getSize()) return;
    
    PhysicsVehicle **vehicles = (PhysicsVehicle **)m_AllVehicles->getItems();
    if (!Tools::IsPtrValid(vehicles)) return;
    
    for (int i = 0; i < m_AllVehicles->getSize(); i++) {
        PhysicsVehicle *vehicle = vehicles[i];
        if (!Tools::IsPtrValid(vehicle)) continue;
        
        Transform *vehicleTransform = vehicle->get_transform();
        if (!Tools::IsPtrValid(vehicleTransform)) continue;
        
        Vector3 vehiclePos = vehicleTransform->get_position();
        
        auto mainCamera = Camera::get_main();
        if (!mainCamera) continue;
        
        Vector3 screenPos = mainCamera->WorldToScreenPoint(vehiclePos);
        if (screenPos.z <= 0) continue;
        
        EVehicleType vType = vehicle->vehicleType();
        std::string vehicleName = EVehicleToString(vType);
        
        if (vehicleName.empty()) continue;
        
        float distance = Vector3::Distance(pLocalPawn_rootPos, vehiclePos);
        
        std::string displayText = vehicleName + " [" + std::to_string((int)distance) + "M]";
        
        ImVec2 textSize = ImGui::CalcTextSize(displayText.c_str());
        float textX = screenPos.x - (textSize.x / 2.0f);
        float textY = sHeight - screenPos.y;
        
        draw->AddText(NULL, 15.0f, ImVec2(textX - 1, textY - 1), IM_COL32(0, 0, 0, 255), displayText.c_str());
        draw->AddText(NULL, 15.0f, ImVec2(textX + 1, textY - 1), IM_COL32(0, 0, 0, 255), displayText.c_str());
        draw->AddText(NULL, 15.0f, ImVec2(textX - 1, textY + 1), IM_COL32(0, 0, 0, 255), displayText.c_str());
        draw->AddText(NULL, 15.0f, ImVec2(textX + 1, textY + 1), IM_COL32(0, 0, 0, 255), displayText.c_str());
        draw->AddText(NULL, 15.0f, ImVec2(textX, textY), IM_COL32(255, 255, 255, 255), displayText.c_str());
        
        if (Config.ESPMenu.VehicleHealth) {
            float curHP = vehicle->get_Health();
            
            if (curHP > 0) {
                float maxHP = 100.0f;
                float healthPercent = std::min((curHP / maxHP) * 100.0f, 100.0f);
                
                long healthColor = IM_COL32(
                    std::min(510 * (100 - healthPercent) / 100, 255.0f),
                    std::min(510 * healthPercent / 100, 255.0f),
                    0,
                    255
                );
                
                float barWidth = 60.0f;
                float barHeight = 6.0f;
                float barX = screenPos.x - (barWidth / 2.0f);
                float barY = textY + 18.0f;
                
                draw->AddRectFilled(
                    ImVec2(barX, barY),
                    ImVec2(barX + barWidth, barY + barHeight),
                    IM_COL32(30, 30, 30, 200)
                );
                
                draw->AddRectFilled(
                    ImVec2(barX, barY),
                    ImVec2(barX + (barWidth * healthPercent / 100.0f), barY + barHeight),
                    healthColor
                );
                
                draw->AddRect(
                    ImVec2(barX, barY),
                    ImVec2(barX + barWidth, barY + barHeight),
                    IM_COL32(0, 0, 0, 255),
                    0.0f,
                    0,
                    1.0f
                );
                
                std::string healthText = std::to_string((int)curHP) + "%";
                ImVec2 hpTextSize = ImGui::CalcTextSize(healthText.c_str());
                float hpTextX = screenPos.x - (hpTextSize.x / 2.0f);
                float hpTextY = barY + 8.0f;
                
                draw->AddText(NULL, 12.0f, ImVec2(hpTextX - 1, hpTextY - 1), IM_COL32(0, 0, 0, 255), healthText.c_str());
                draw->AddText(NULL, 12.0f, ImVec2(hpTextX + 1, hpTextY - 1), IM_COL32(0, 0, 0, 255), healthText.c_str());
                draw->AddText(NULL, 12.0f, ImVec2(hpTextX - 1, hpTextY + 1), IM_COL32(0, 0, 0, 255), healthText.c_str());
                draw->AddText(NULL, 12.0f, ImVec2(hpTextX + 1, hpTextY + 1), IM_COL32(0, 0, 0, 255), healthText.c_str());
                draw->AddText(NULL, 12.0f, ImVec2(hpTextX, hpTextY), IM_COL32(255, 255, 255, 255), healthText.c_str());
            }
        }
    }
}

void DrawESP(ImDrawList *draw, int sWidth, int sHeight, float density) {
    ApplySpeedhack();
    ApplyNoGravityRuntime();

    BaseGame *get_MatchGame = GamePlay::get_MatchGame();
    if (!Tools::IsPtrValid(get_MatchGame)) return;

	
    bool MatchGame = Tools::IsPtrValid((void *)get_MatchGame);
    if (!MatchGame) return;

    Vector3 pLocalPawn_rootPos = Vector3::zero();
    Pawn *get_LocalPawn = GamePlay::get_LocalPawn();
    if (EspObjectAlive(get_LocalPawn)) {
        pLocalPawn_rootPos = get_LocalPawn->get_LastPawnPos();
    }

    // Camera + WorldToScreen resolved ONCE per frame. The old form called
    // Camera::get_main() twice per enemy and the member wrapper calls it AGAIN
    // internally, i.e. two extra native calls per enemy just to project two
    // points - at 10 pawns x 60 fps that was ~1200 pointless native calls/s.
    Camera *espCam = Camera::get_main();
    auto espW2s = (Vector3 (*)(Camera *, Vector3))Class_Camera_WorldToScreenPoint;
    
    DrawThrowableAlert(draw, sWidth, sHeight, pLocalPawn_rootPos);

    totalBots = 0, totalEnemies = 0;
    // Pawn-INDEPENDENT overlays used to run once per ENEMY inside the loop
    // below (DrawAimLine re-walked every pawn per enemy, DrawVehicleESP
    // re-fetched the whole game state per enemy, the FOV ring emitted 60 lines
    // per enemy) - draw each at most once per frame.
    bool fovCircleDrawn = false, aimlineDrawn = false, vehicleEspDrawn = false;
    List<Pawn *> *EnemyPawns = get_MatchGame->EnemyPawns();
    if (!Tools::IsPtrValid(EnemyPawns) || !EnemyPawns->getSize()) return;

    // Snapshot the list ONCE: the game mutates EnemyPawns from its own thread
    // while this frame walks it (a player seen mid-loop grows the list and
    // rebuilds its element array), so getSize() must not be re-read in the
    // loop condition and the count is clamped to the array's own capacity.
    Pawn **pawns = (Pawn **)EnemyPawns->getItems();
    if (!Tools::IsPtrValid(pawns)) return;
    const int listCap = Tools::IsPtrValid(EnemyPawns->items) ? EnemyPawns->items->getLength() : 0;
    int pawnCount = EnemyPawns->getSize();
    if (pawnCount < 0) pawnCount = 0;
    if (listCap > 0 && pawnCount > listCap) pawnCount = listCap;

    for (int i = 0; i < pawnCount; i++) {
        Pawn *pawn = pawns[i];
        if (!EspObjectAlive(pawn)) continue;

        auto m_PlayerInfo = *(uintptr_t *)(pawn + Class_Pawn_m_PlayerInfo);
        if (!Tools::IsPtrValid((void *)m_PlayerInfo)) continue;

        // A dead / mid-respawn pawn may carry no attackable record yet; that
        // is not a reason to drop it - just draw it without health numbers.
        int CurHP = 0, MaxHP = 0;
        EspPawnHealth((uintptr_t)pawn, CurHP, MaxHP);

        auto m_HeadBone = *(Transform **)(pawn + Class_Pawn_m_HeadBone);
        auto m_Mesh = *(Transform **)(pawn + Class_Pawn_m_Mesh);
        if (!EspObjectAlive(m_HeadBone) || !EspObjectAlive(m_Mesh) || !*(bool *)(pawn + Class_Pawn_m_IsAlive)) continue;

        bool isBot = *(bool *)((uintptr_t)pawn + Class_Pawn_m_IsBot);
        
        if (isBot) totalBots++; else totalEnemies++;

        ImU32 lineColor, boxColor, nameColor, distanceColor, healthColor, skeletonColor;
        float lineThickness, boxThickness, skeletonThickness;

        if (isBot) {
            lineColor = boxColor = ToColor(Config.sColorsESPBOT.LineBOT);
            nameColor = ToColor(Config.sColorsESPBOT.NameBOT);
            distanceColor = ToColor(Config.sColorsESPBOT.DistanceBOT);
            healthColor = ToColor(Config.sColorsESPBOT.HealthBOT);
            skeletonColor = ToColor(Config.sColorsESPBOT.SkeletonBOT);
            lineThickness = boxThickness = Config.Bline;
            skeletonThickness = Config.BskelLine;
        } else {
            lineColor = boxColor = ToColor(Config.sColorsESPPLAYER.LinePLAYER);
            nameColor = ToColor(Config.sColorsESPPLAYER.NamePLAYER);
            distanceColor = ToColor(Config.sColorsESPPLAYER.DistancePLAYER);
            healthColor = ToColor(Config.sColorsESPPLAYER.HealthPLAYER);
            skeletonColor = ToColor(Config.sColorsESPPLAYER.SkeletonPLAYER);
            lineThickness = boxThickness = Config.Pline;
            skeletonThickness = Config.PskelLine;
        }

        Vector3 pEnemyPawn_headPos = pawn->get_HeadPosition();
        Vector3 pEnemyPawn_rootPos = pawn->get_LastPawnPos();

        Vector3 HeadSc = (espCam != nullptr && espW2s != nullptr)
                       ? espW2s(espCam, pEnemyPawn_headPos) : Vector3::zero();
        Vector3 RootSc = (espCam != nullptr && espW2s != nullptr)
                       ? espW2s(espCam, pEnemyPawn_rootPos) : Vector3::zero();

        float distanceToMe = Vector3::Distance(pLocalPawn_rootPos, pEnemyPawn_rootPos);
        float actualHeight = Vector3::Distance(pEnemyPawn_headPos, pEnemyPawn_rootPos);
        
        if (HeadSc.z > 0 && pawn->m_IsAlive()) {
            
            Vector2 screen(sWidth, sHeight);
            Vector2 location(RootSc.x, HeadSc.y);
            float magic_number = distanceToMe;
            float boxHeight = std::abs(HeadSc.y - RootSc.y);
            float boxWidth = boxHeight * 0.65f;

            if (boxHeight < 5.0f || boxHeight > sHeight * 0.85f) continue;
            if (boxWidth < 2.0f) continue;

            float mx = (glWidth / 6) / magic_number;
            float healthLength = glWidth / 20;
            if (healthLength < mx) healthLength = mx;

            Rect PlayerRect(HeadSc.x - (boxWidth / 2), sHeight - HeadSc.y, boxWidth, boxHeight);

            if (HeadSc.z > 0) {
                if (Config.ESPMenu.Alert) {
                    Vector3 angle = Vector3();
                    Vector3 forward = Vector3((float)(sWidth / 2) - HeadSc.x, (float)(sHeight / 2) - (sHeight - HeadSc.y), 0.0f);
                    VectorAnglesRadar(forward, angle);
                    const auto angle_yaw_rad = DEG2RAD(angle.y + 180.f);
                    const auto new_point_x = (sWidth / 2) + (55) / 2 * 8 * cosf(angle_yaw_rad);
                    const auto new_point_y = (sHeight / 2) + (55) / 2 * 8 * sinf(angle_yaw_rad);
                    std::array<Vector3, 3> points{
                        Vector3(new_point_x - ((90) / 4 + 3.5f) / 2, new_point_y - ((55) / 4 + 3.5f) / 2, 0.f),
                        Vector3(new_point_x + ((90) / 4 + 3.5f) / 4, new_point_y, 0.f),
                        Vector3(new_point_x - ((90) / 4 + 3.5f) / 2, new_point_y + ((55) / 4 + 3.5f) / 2, 0.f)
                    };
                    std::string strDistance;
                    auto textSize = ImGui::CalcTextSize(strDistance.c_str(), 0, ((float)density / 20.0f));
                    strDistance += std::to_string((int)distanceToMe) + "m";
                    draw->AddText(NULL, ((float)density / 20.0f), {new_point_x - (textSize.x / 2), new_point_y + 7.f}, IM_COL32(255, 255, 255, 255), strDistance.c_str());
                    RotateTriangle(points, angle.y + 180.f);
                    if (isBot) {
                        draw->AddTriangle(ImVec2(points.at(0).x, points.at(0).y), ImVec2(points.at(1).x, points.at(1).y), ImVec2(points.at(2).x, points.at(2).y), IM_COL32(0, 255, 0, 255), 1.5f);
                        draw->AddTriangleFilled(ImVec2(points.at(0).x, points.at(0).y), ImVec2(points.at(1).x, points.at(1).y), ImVec2(points.at(2).x, points.at(2).y), IM_COL32(0, 255, 0, 255));
                    } else {
                        draw->AddTriangle(ImVec2(points.at(0).x, points.at(0).y), ImVec2(points.at(1).x, points.at(1).y), ImVec2(points.at(2).x, points.at(2).y), IM_COL32(255, 0, 0, 255), 1.5f);
                        draw->AddTriangleFilled(ImVec2(points.at(0).x, points.at(0).y), ImVec2(points.at(1).x, points.at(1).y), ImVec2(points.at(2).x, points.at(2).y), IM_COL32(255, 0, 0, 255));
                    }
                }
        
                if (Config.ESPMenu.isPlayerLine && Config.ESPMenu.Target == LineTarget::Top) {
                    draw->AddLine(ImVec2(sWidth / 2, 80), ImVec2(HeadSc.x, sHeight - HeadSc.y), lineColor, lineThickness);
                }

                if (Config.ESPMenu.isPlayerLine && Config.ESPMenu.Target == LineTarget::Center) {
                    draw->AddLine(ImVec2(sWidth / 2, sHeight / 2), ImVec2(HeadSc.x, sHeight - HeadSc.y), lineColor, lineThickness);
                }

                if (Config.ESPMenu.isPlayerLine && Config.ESPMenu.Target == LineTarget::Bottom) {
                    draw->AddLine(ImVec2(sWidth / 2, sHeight), ImVec2(HeadSc.x, sHeight - RootSc.y), lineColor, lineThickness);
                }

                if (Config.ESPMenu.ShowFov && !fovCircleDrawn) {
                    fovCircleDrawn = true;   // screen-space ring: once per frame
                    ImVec2 center(get_width() / 2, get_height() / 2);
                    float radius = Config.Aim.Cross;

                    static float rotation = 0.0f;
                    rotation += 0.02f;
                    if (rotation > 2 * 3.14159f) rotation -= 2 * 3.14159f;

                    int segments = 60;
                    float angleStep = (2 * 3.14159f) / segments;
                    float lineThickness = 3.0f;

                    for (int i = 0; i < segments; i++) {
                        float angle1 = i * angleStep + rotation;
                        float angle2 = (i + 1) * angleStep + rotation;

                        ImVec2 p1(center.x + cosf(angle1) * radius, center.y + sinf(angle1) * radius);
                        ImVec2 p2(center.x + cosf(angle2) * radius, center.y + sinf(angle2) * radius);

                        float hue = (float)i / segments;
                        int r2, g2, b2;
                        if (hue < 0.166f) {
                            r2 = 255; g2 = (int)(255 * (hue * 6)); b2 = 0;
                        } else if (hue < 0.333f) {
                            r2 = (int)(255 * (1 - (hue - 0.166f) * 6)); g2 = 255; b2 = 0;
                        } else if (hue < 0.5f) {
                            r2 = 0; g2 = 255; b2 = (int)(255 * ((hue - 0.333f) * 6));
                        } else if (hue < 0.666f) {
                            r2 = 0; g2 = (int)(255 * (1 - (hue - 0.5f) * 6)); b2 = 255;
                        } else if (hue < 0.833f) {
                            r2 = (int)(255 * ((hue - 0.666f) * 6)); g2 = 0; b2 = 255;
                        } else {
                            r2 = 255; g2 = 0; b2 = (int)(255 * (1 - (hue - 0.833f) * 6));
                        }

                        draw->AddLine(p1, p2, IM_COL32(r2, g2, b2, 255), lineThickness);
                    }
                }
                
                if (Config.ESPMenu.Aimline && !aimlineDrawn) {
                    aimlineDrawn = true;     // runs its own full pawn walk: once per frame
                    DrawAimLine(draw, sWidth, sHeight);
                }
                
                if (Config.ESPMenu.Skeleton) {
                    AddSkeletonToDrawESP(draw, pawn, isBot, sHeight);
                }
                
                if (Config.ESPMenu.Vehicle && !vehicleEspDrawn) {
                    vehicleEspDrawn = true;  // re-fetches game state: once per frame
                    DrawVehicleESP(draw, sWidth, sHeight);
                }

                if ((Config.ESPMenu.Name || (Config.ESPMenu.Health && Config.ESPMenu.HealthPosition == EspHealthPosition::HealthTop)) && distanceToMe <= 60.0f) {
                    float scaleFactor = 1.0f;
                    if (distanceToMe >= 19.0f) {
                        scaleFactor = 1.3f;
                    } else if (distanceToMe >= 17.0f) {
                        scaleFactor = 1.2f;
                    } else if (distanceToMe >= 15.0f) {
                        scaleFactor = 1.1f;
                    }                     
                    
                    float infoContainerWidth = boxWidth * 1.6f * scaleFactor;
                    float infoContainerX = HeadSc.x - (infoContainerWidth / 2);
                    float infoCurrentY = sHeight - HeadSc.y - (50.0f * scaleFactor);
                    float centerX = ImGui::GetIO().DisplaySize.x / 2.0f;
                    float headY = HeadSc.y; 
                    float fontSize = 18.0f * scaleFactor;
                    float infoTotalHeight = 0.0f;
                    if (Config.ESPMenu.Name) infoTotalHeight += 24.0f * scaleFactor;
                    if (Config.ESPMenu.Health && Config.ESPMenu.HealthPosition == EspHealthPosition::HealthTop) infoTotalHeight += 10.0f * scaleFactor;

                    draw->AddRectFilled(ImVec2(infoContainerX, infoCurrentY), ImVec2(infoContainerX + infoContainerWidth, infoCurrentY + infoTotalHeight), IM_COL32(0, 0, 0, 120));
                    
                    if (Config.ESPMenu.Name) {
                        const std::string displayText = EspPawnName(pawn);
                        if (!displayText.empty()) {
                            ImVec2 infoTextSize = ImGui::CalcTextSize(displayText.c_str());
                            infoTextSize.x *= (fontSize / ImGui::GetFontSize());
                            float infoTextX = infoContainerX + (infoContainerWidth - infoTextSize.x) / 2;
                            draw->AddText(nullptr, fontSize, ImVec2(infoTextX, infoCurrentY + 3.0f * scaleFactor), IM_COL32(255, 255, 255, 255), displayText.c_str());
                        }
                        infoCurrentY += 24.0f * scaleFactor;
                    }
                    
                    if (Config.ESPMenu.Health && Config.ESPMenu.HealthPosition == EspHealthPosition::HealthTop && MaxHP > 0) {
                        float hpRatio = ImClamp((float)CurHP / (float)MaxHP, 0.0f, 1.0f);
                        long curHP_Color = IM_COL32(std::min((int)(510.0f * (1.0f - hpRatio)), 255), std::min((int)(510.0f * hpRatio), 255), 0, 255);
                        float infoHealthBarHeight = 7.0f * scaleFactor;
                        float infoHealthBarPadding = 5.0f * scaleFactor;
                        float infoHealthBarX = infoContainerX + infoHealthBarPadding;
                        float infoHealthBarWidth = infoContainerWidth - (infoHealthBarPadding * 2);
                        draw->AddRectFilled(ImVec2(infoHealthBarX, infoCurrentY), ImVec2(infoHealthBarX + infoHealthBarWidth, infoCurrentY + infoHealthBarHeight), IM_COL32(0, 0, 0, 120));
                        draw->AddRectFilled(ImVec2(infoHealthBarX, infoCurrentY), ImVec2(infoHealthBarX + infoHealthBarWidth * hpRatio, infoCurrentY + infoHealthBarHeight), curHP_Color);
                        draw->AddRect(ImVec2(infoHealthBarX, infoCurrentY), ImVec2(infoHealthBarX + infoHealthBarWidth, infoCurrentY + infoHealthBarHeight), IM_COL32(0, 0, 0, 255), 0.0f, 0, 1.0f);
                    }
                }
                                                   
                DrawBRClassEsp(draw, pawn, HeadSc.x, sHeight - HeadSc.y, boxWidth, distanceToMe);
                
                if (Config.ESPMenu.Health && Config.ESPMenu.HealthPosition == EspHealthPosition::HealthSide) {
                    float barX = std::max(PlayerRect.x - 8.0f, 2.0f);
                    DrawEspVerticalHealthBar(draw, barX, PlayerRect.y, PlayerRect.height, CurHP, MaxHP);
                }
                
                DrawArmorEsp(draw, pawn, PlayerRect);                
                
                if (Config.ESPMenu.Distance && distanceToMe <= 60.0f) {
                    float scaleFactor = 1.0f;
                    if (distanceToMe >= 19.0f) {
                        scaleFactor = 1.3f;
                    } else if (distanceToMe >= 17.0f) {
                        scaleFactor = 1.2f;
                    } else if (distanceToMe >= 15.0f) {
                        scaleFactor = 1.1f;
                    }
                    
                    std::string s = std::to_string((int)distanceToMe) + "m";
                    float distFontSize = 17.0f * scaleFactor;
                    ImVec2 distTextSize = ImGui::CalcTextSize(s.c_str());
                    distTextSize.x *= (distFontSize / ImGui::GetFontSize());
                    distTextSize.y *= (distFontSize / ImGui::GetFontSize());
                    float distContainerWidth = distTextSize.x + (8.0f * scaleFactor);
                    float distContainerX = HeadSc.x - (distContainerWidth / 2);
                    float distBoxY = sHeight - RootSc.y + (8.0f * scaleFactor);
                    float distBoxHeight = distTextSize.y + (8.0f * scaleFactor);
                    
                    draw->AddRectFilled(ImVec2(distContainerX, distBoxY), ImVec2(distContainerX + distContainerWidth, distBoxY + distBoxHeight), IM_COL32(0, 0, 0, 120));
                    float distTextX = distContainerX + (distContainerWidth - distTextSize.x) / 2;
                    draw->AddText(nullptr, distFontSize, ImVec2(distTextX, distBoxY + (4.0f * scaleFactor)), IM_COL32(255, 255, 255, 255), s.c_str());
                }
              
                if (Config.ESPMenu.Box) {
                    float x = RootSc.x - (boxWidth / 2.0f);
                    float y = sHeight - HeadSc.y;
                    float w = boxWidth;
                    float h = boxHeight;
                    if (Config.ESPMenu.BoxType == EspBoxType::Fill) {
                        DrawEspBoxFilled(draw, x, y, w, h, boxColor, boxThickness);
                    } else if (Config.ESPMenu.BoxType == EspBoxType::Outline) {
                        DrawEspBoxOutline(draw, x, y, w, h, boxColor, boxThickness);
                    } else if (Config.ESPMenu.BoxType == EspBoxType::ThreeD) {
                        DrawEsp3DBox(draw, pEnemyPawn_rootPos, sHeight, boxColor, boxThickness);
                    } else {
                        DrawEspBoxCorner(draw, x, y, w, h, boxColor, boxThickness);
                    }
                }

                if (Config.ESPMenu.EspStyle == EspStyleTarget::EspStyle3DSphere) {
                    DrawEspCylinder(draw, pEnemyPawn_rootPos, sHeight, boxColor);
                } else if (Config.ESPMenu.EspStyle == EspStyleTarget::EspStylePlayerSignal) {
                    DrawEspSignal(draw, pEnemyPawn_rootPos, sHeight, boxColor);
                }
            }
        }
    }

    // Bots / players counter. Drawn straight onto the background draw list so
    // it never creates an ImGui window next to the menu canvas.
    if (Config.ESPMenu.Count) {
        const int totalEnemyCount = totalBots + totalEnemies;
        ImFont* hudFont = ImGui::GetFont();
        const float hudWidth = 240.0f, hudHeight = 62.0f;
        const ImVec2 tl(ImGui::GetIO().DisplaySize.x * 0.5f - hudWidth * 0.5f, 32.0f);
        const ImVec2 br(tl.x + hudWidth, tl.y + hudHeight);
        draw->AddRectFilled(tl, br, IM_COL32(15, 23, 33, 242), hudHeight * 0.5f);
        draw->AddRect(tl, br, IM_COL32(51, 102, 153, 102), hudHeight * 0.5f, 0, 1.0f);
        draw->AddLine(ImVec2(tl.x + 30.0f, br.y - 9.0f), ImVec2(br.x - 30.0f, br.y - 9.0f),
                      IM_COL32(51, 102, 153, 77), 1.5f);

        auto centered = [&](float centerX, float y, float size, ImU32 col, const char* text) {
            const ImVec2 ts = hudFont->CalcTextSizeA(size, FLT_MAX, 0.0f, text);
            draw->AddText(hudFont, size, ImVec2(centerX - ts.x * 0.5f, y), col, text);
        };

        if (totalEnemyCount > 0) {
            char botStr[16], plrStr[16];
            snprintf(botStr, sizeof(botStr), "%d", totalBots);
            snprintf(plrStr, sizeof(plrStr), "%d", totalEnemies);

            const float leftX = tl.x + hudWidth * 0.25f;
            const float rightX = tl.x + hudWidth * 0.75f;
            centered(leftX, tl.y + 9.0f, 10.0f, IM_COL32(140, 153, 166, 204), "BOTS");
            centered(leftX, tl.y + 22.0f, 20.0f, IM_COL32(51, 217, 77, 255), botStr);
            centered(rightX, tl.y + 9.0f, 10.0f, IM_COL32(140, 153, 166, 204), "PLAYERS");
            centered(rightX, tl.y + 22.0f, 20.0f, IM_COL32(255, 166, 38, 255), plrStr);
            draw->AddCircleFilled(ImVec2(tl.x + hudWidth * 0.5f, tl.y + hudHeight * 0.5f - 4.0f),
                                  3.0f, IM_COL32(77, 166, 255, 255));
        } else {
            centered(tl.x + hudWidth * 0.5f, tl.y + 16.0f, 22.0f,
                     IM_COL32(0, 255, 0, 255), "[ SAFE ]");
        }
    }
}


template<typename ThisType, typename... Args>
void WeaponHandlerByAstral(ThisType thiz, Vector3 startPos, Vector3& dir, Args... args) {
    if (!Config.Aim.Aimbot360 && !Config.Aim.AimSilent) return;

    auto get_MatchGame = ((uintptr_t(*)())(Class_Gameplay_get_MatchGame))();
    if (!Tools::IsPtrValid((void*)get_MatchGame)) return;

    auto LocalPawn = ((uintptr_t(*)())(Class_Gameplay_get_LocalPawn))();
    if (!LocalPawn) return;

    bool triggerReady = Config.Aim.Trigger == EAimTrigger::None;
    if (Config.Aim.Trigger == EAimTrigger::Shooting) {
        triggerReady = ((bool(*)(uintptr_t))(Class_Pawn_get_IsFiring))(LocalPawn);
    } else if (Config.Aim.Trigger == EAimTrigger::Scoping) {
        // "Scoping" is answered by the pawn's own held weapon, read as two
        // FIELDS (Pawn::get_IsAimingSafe -> Weapon::m_IsAiming). The old call
        // was Pawn.IsAiming() 0x519BB10, a native method that walks the pawn's
        // weapon / fire-component chain - the call that crashed every "ADS
        // only" gate in this repo.
        triggerReady = ((Pawn *)LocalPawn)->get_IsAimingSafe();
    }

    if (!triggerReady) return;

    uintptr_t target = (Config.Aim.By == EAim::Distance) ? GetClosestTarget() : GetInsideFOVTarget();
    if (!target) return;

    Vector3 targetPos;
    auto m_HeadBone = *(Transform**)(target + Class_Pawn_m_HeadBone);
    if (!m_HeadBone) return;

    targetPos = m_HeadBone->get_position();
    if (Config.Aim.Target == EAimTarget::Chests) {
        targetPos.y -= 0.2f;
    } else if (Config.Aim.Target == EAimTarget::Body) {
        targetPos.y -= 0.4f;
    }

    auto main = Camera::get_main();
    if (main) {
        auto mainView = ((Component *) main)->get_transform();
        if (mainView) {
            if (Config.Aim.AimSilent) {
                dir = targetPos - mainView->get_position();
            }
            if (Config.Aim.Aimbot360) {
                auto Pawn_set_AimRotation = (void ( *)(uintptr_t, Quaternion))(Class_Pawn_set_AimRotation);
                Pawn_set_AimRotation(LocalPawn, Quaternion::LookRotation(targetPos - mainView->get_position(), Vector3::Up()));
            }
        }
    }
}

void WeaponFireComponent_Instant_CreateBulletLine(uintptr_t thiz, Vector3 startPos, Vector3 dir, bool isDualFire) {
    WeaponHandlerByAstral(thiz, startPos, dir, isDualFire);
    oWeaponFireComponent_Instant_CreateBulletLine(thiz, startPos, dir, isDualFire);
    SmartReloadMarkShotFired();
}

void WeaponFireComponent_Instant_CreateBulletProjectile(void* thiz, Vector3 startPos, Vector3 dir, void* weaponImpact, int itemID, int flySmokeAssetID, bool enableVirtualStartPos, Vector3 virtualStartPos) {
    WeaponHandlerByAstral(thiz, startPos, dir, weaponImpact, itemID, flySmokeAssetID, enableVirtualStartPos, virtualStartPos);
    oWeaponFireComponent_Instant_CreateBulletProjectile(thiz, startPos, dir, weaponImpact, itemID, flySmokeAssetID, enableVirtualStartPos, virtualStartPos);
    SmartReloadMarkShotFired();
}

// NOTE: The NewFeatures_Hook_ChangePunchAngleByGetHit and NewFeatures_Hook_Tick
// Dobby hooks were removed entirely. Their targets (libunity.so+0xC9C0EDC /
// +0xC9C0B64) are correct per the GR .57 dump, but Dobby's inline trampoline on
// these tiny shared callers corrupted game state (screen flicker, native
// crash). The safe equivalents are memory patches in Main.cpp that toggle
// per-frame: Patches.NoHitFlinch (@0xC9C0EDC, mov w0,#0 + ret) and
// Patches.NoDrift (@0xC9BE4C0 UpdateBreatheTrembling, ret) - every address
// verified against dump.cs and dump (GR .57).cs from the jeudhehs/dump
// dataset. Reference addresses from that dump:
//   Tick:                    0xC9C0B64 (Slot 51)
//   ChangePunchAngleByGetHit: 0xC9C0EDC (Slot 57)
//   UpdateBreatheTrembling:   0xC9BE4C0
//   m_AimmingBreatheCountDown: field @ 0x774 on WeaponFireComponent_Instant
