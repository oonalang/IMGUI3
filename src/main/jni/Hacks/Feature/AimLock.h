#pragma once

#include <cmath>
#include <chrono>

#include "Init_Hacks.h"
#include "../../IL2CppSDKGenerator/BasicStructs/Quaternion.h"
#include "../../System/Core/FaultGuard.h"

// Aim Lock.
//
// Aim Assist only moves the aim inside the bullet hook, so it does nothing until
// you fire - and while you are scoping in, nothing is being fired. Aim Lock is
// the other half: on every local-pawn tick it turns the aim toward the chosen
// target, with or without firing or scoping, so the crosshair is already there
// when you take the shot.
//
// Config.Aim.AimLock is the strength: 0 turns it off, 100 snaps straight to the
// target on the next tick. Anything in between turns at a rate that rises with
// the square of the slider, so the low half stays usable for tracking a moving
// player and the top of the range is instant.
//
// It follows the same targeting rules as the assist: Config.Aim.Trigger decides
// when it may turn, Config.Aim.By chooses the closest target or the one nearest
// the crosshair, and in that second mode the lock is limited to the FOV circle
// the ESP draws, in pixels, so the circle on screen is the area it works in.
namespace aim_lock
{
    // Same symbols NewFeatures resolves for the triggerbot; duplicated so this
    // header stays usable on its own (the LOS harness compiles it without the
    // whole triggerbot).
    constexpr uintptr_t kTransformGetForwardRva = 0x509C650;   // Transform.get_forward
    constexpr uintptr_t kPawnGetTeamIdRva       = 0x51B89CC;   // Pawn.GetTeamId

    inline Vector3  (*g_cameraForward)(void *) = nullptr;
    inline uint32_t (*g_getTeamId)(void *)     = nullptr;   // injectable for tests
    inline bool  g_forwardMissing = false;
    inline int   g_turns       = 0;
    inline double g_lastTick   = -1000.0;
    inline void *g_lastTarget  = nullptr;
    inline void *g_lastLocal   = nullptr;

    inline double Now()
    {
        return std::chrono::duration<double>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    // Same failure the triggerbot guards against: a spawning or despawning
    // pawn leaves a head-bone / aim-rotation pointer inside a mapped page, so
    // IsPtrValid() passes and the game's own get_position() then walks the torn
    // object - which is a crash the moment an enemy appears. Each target is
    // read under fault_guard and a pawn that faulted is skipped for 1.5 s.
    struct LockFaultEntry
    {
        uintptr_t key;
        double    until;
    };
    inline LockFaultEntry g_lockFaults[16] = {};
    inline int            g_lockFaultNext = 0;
    inline const char *   g_lockLastFaultStage = "idle";
    inline double         g_lockLastFaultTime  = -1000.0;

    inline bool LockFaultCooldownActive(uintptr_t key)
    {
        if (key == 0) return false;
        const double now = Now();
        for (int i = 0; i < 16; i++)
            if (g_lockFaults[i].key == key && now < g_lockFaults[i].until)
                return true;
        return false;
    }

    inline void LockFaultStartCooldown(uintptr_t key)
    {
        g_lockFaults[g_lockFaultNext].key   = key;
        g_lockFaults[g_lockFaultNext].until = Now() + 1.5;
        g_lockFaultNext = (g_lockFaultNext + 1) % 16;
    }

    template <typename Fn>
    inline bool LockGuardedRun(const char *stage, uintptr_t key, Fn &&fn)
    {
        if (LockFaultCooldownActive(key)) return false;
        const bool ok = fault_guard::Run([&] { fn(); });
        if (!ok) {
            LockFaultStartCooldown(key);
            const double now = Now();
            if (stage != g_lockLastFaultStage || now - g_lockLastFaultTime >= 5.0) {
                LOGE("[AimLock] native fault during '%s' (key %p) - skipped for 1.5s",
                     stage, (void *)key);
                g_lockLastFaultStage = stage;
                g_lockLastFaultTime  = now;
            }
        }
        return ok;
    }

    inline Vector3 CameraForward(void *camTransform)
    {
        if (g_cameraForward == nullptr) {
            static auto lazy = (Vector3 (*)(void *))getAbsoluteAddress(
                OBFUSCATE("libunity.so"), kTransformGetForwardRva);
            g_cameraForward = lazy;
        }
        if (g_cameraForward == nullptr || camTransform == nullptr) {
            g_forwardMissing = true;
            return Vector3::zero();
        }
        return g_cameraForward(camTransform);
    }

    inline bool Alive(void *obj)
    {
        if (!Tools::IsPtrValid(obj)) return false;
        return Tools::IsPtrValid(*(void **)((uintptr_t)obj + 0x10));
    }

    inline uint32_t TeamOf(void *pawn)
    {
        if (g_getTeamId == nullptr) {
            static auto lazy = (uint32_t (*)(void *))getAbsoluteAddress(
                OBFUSCATE("libunity.so"), kPawnGetTeamIdRva);
            g_getTeamId = lazy;
        }
        return g_getTeamId != nullptr ? g_getTeamId(pawn) : 0u;
    }

    inline bool IsEnemy(Pawn *localPawn, Pawn *other)
    {
        const uint32_t mine = TeamOf(localPawn);
        const uint32_t his  = TeamOf(other);
        return mine == 0u || mine != his;
    }

    // The point on the pawn the player asked for. Head by default; the chest and
    // body options step down the body, exactly as the assist does.
    inline bool AimPoint(Pawn *pawn, Vector3 &out)
    {
        Transform *head = *(Transform **)((uintptr_t)pawn + Class_Pawn_m_HeadBone);
        if (!Tools::IsPtrValid((void *)head)) return false;
        out = head->get_position();
        if (out.x == 0.0f && out.y == 0.0f && out.z == 0.0f) return false;

        if (Config.Aim.Target == EAimTarget::Chests) out.y -= 0.20f;
        else if (Config.Aim.Target == EAimTarget::Body) out.y -= 0.40f;
        return true;
    }

    // Only turn while the assist would be allowed to act. The menu says the lock
    // keeps the aim while firing or scoping; before this gate it turned all the
    // time, whatever the trigger was set to.
    inline bool TriggerReady(Pawn *localPawn)
    {
        if (m_unity == 0) return false;

        switch (Config.Aim.Trigger) {
            case EAimTrigger::Shooting: {
                auto isFiring = (bool (*)(Pawn *))(Class_Pawn_get_IsFiring);
                return isFiring != nullptr && isFiring(localPawn);
            }
            case EAimTrigger::Scoping: {
                auto isAiming = (bool (*)(Pawn *))(Class_Pawn_IsAiming);
                return isAiming != nullptr && isAiming(localPawn);
            }
            default:
                return true;
        }
    }

    // The FOV circle is drawn with a radius in pixels around the centre of the
    // screen, so the lock measures against the same circle instead of treating
    // that number as degrees.
    inline bool InsideFovCircle(Camera *cam, const Vector3 &point)
    {
        const float radius = Config.Aim.Cross;
        if (!(radius > 0.0f) || cam == nullptr) return true;

        auto toScreen = (Vector3 (*)(Camera *, Vector3))Class_Camera_WorldToScreenPoint;
        if (toScreen == nullptr) return true;

        const Vector3 screen = toScreen(cam, point);
        if (!(screen.z > 0.01f)) return false;

        const float centerX = (float)get_width() * 0.5f;
        const float centerY = (float)get_height() * 0.5f;
        const float dx = screen.x - centerX;
        const float dy = ((float)get_height() - screen.y) - centerY;
        return dx * dx + dy * dy <= radius * radius;
    }

    inline float Range()
    {
        return Config.Aim.size > 0.0f ? Config.Aim.size : 1000.0f;
    }

    // Everything a pawn has to satisfy to be turn-to material: alive, an enemy,
    // with a readable head, inside range, and - in crosshair mode - inside both
    // the aim cone and the circle that is actually drawn.
    inline bool CandidateOkInner(Pawn *localPawn, Pawn *pawn, const Vector3 &eye,
                            const Vector3 &forward, Camera *cam,
                            Vector3 &point, float &distance)
    {
        if (pawn == nullptr || pawn == localPawn) return false;
        if (!Alive((void *)pawn)) return false;
        if (!*(bool *)((uintptr_t)pawn + Class_Pawn_m_IsAlive)) return false;
        if (!IsEnemy(localPawn, pawn)) return false;
        if (!AimPoint(pawn, point)) return false;

        const Vector3 to(point.x - eye.x, point.y - eye.y, point.z - eye.z);
        const float dist = Vector3::Magnitude(to);
        if (!(dist > 0.05f) || dist > Range()) return false;

        if (Config.Aim.By == EAim::Crosshair) {
            if (Vector3::Magnitude(forward) > 0.01f) {
                float cosA = (to.x * forward.x + to.y * forward.y + to.z * forward.z) / dist;
                if (cosA > 1.0f) cosA = 1.0f;
                if (cosA < -1.0f) cosA = -1.0f;
                const float fov = Config.Aim.Cross > 0.0f ? Config.Aim.Cross : 180.0f;
                if (acosf(cosA) * 57.29578f > fov) return false;
            }
            if (!InsideFovCircle(cam, point)) return false;
        }

        distance = dist;
        return true;
    }

    inline bool CandidateOk(Pawn *localPawn, Pawn *pawn, const Vector3 &eye,
                            const Vector3 &forward, Camera *cam,
                            Vector3 &point, float &distance)
    {
        if (pawn == nullptr || pawn == localPawn) return false;
        bool ok = false;
        const bool safe = LockGuardedRun("aim candidate", (uintptr_t)pawn, [&] {
            ok = CandidateOkInner(localPawn, pawn, eye, forward, cam, point, distance);
        });
        return safe && ok;
    }

    inline Pawn *PickTarget(Pawn *localPawn, const Vector3 &eye, const Vector3 &forward,
                            Camera *cam, Vector3 &outPoint)
    {
        void *game = ((void *(*)())(Class_Gameplay_get_MatchGame))();
        if (!Tools::IsPtrValid(game)) return nullptr;

        List<Pawn *> *enemies = (List<Pawn *> *)*(void **)((uintptr_t)game + Class_BaseGame_EnemyPawns);
        if (!Tools::IsPtrValid((void *)enemies)) return nullptr;
        Pawn **items = (Pawn **)enemies->getItems();
        const int count = enemies->getSize();
        if (!Tools::IsPtrValid((void *)items) || count <= 0 || count > 1024) return nullptr;

        Pawn *best = nullptr;
        Vector3 bestPoint;
        float bestScore = 0.0f;
        for (int i = 0; i < count; ++i) {
            Vector3 point;
            float dist = 0.0f;
            Pawn *pawn = items[i];
            if (!CandidateOk(localPawn, pawn, eye, forward, cam, point, dist)) continue;

            float score = dist;
            if (Config.Aim.By == EAim::Crosshair) {
                const Vector3 to(point.x - eye.x, point.y - eye.y, point.z - eye.z);
                float cosA = (to.x * forward.x + to.y * forward.y + to.z * forward.z) / dist;
                if (cosA > 1.0f) cosA = 1.0f;
                if (cosA < -1.0f) cosA = -1.0f;
                score = acosf(cosA) * 57295.78f + dist * 0.001f;
            }
            if (best == nullptr || score < bestScore) {
                best = pawn;
                bestPoint = point;
                bestScore = score;
            }
        }

        if (best != nullptr) outPoint = bestPoint;
        return best;
    }

    // Rotates `from` toward `to` by at most maxRad.
    inline Vector3 StepToward(const Vector3 &from, const Vector3 &to, float maxRad)
    {
        const float toLen = Vector3::Magnitude(to);
        if (!(toLen > 1e-4f)) return from;
        const Vector3 b(to.x / toLen, to.y / toLen, to.z / toLen);

        const float fromLen = Vector3::Magnitude(from);
        if (!(fromLen > 1e-4f)) return b;
        const Vector3 a(from.x / fromLen, from.y / fromLen, from.z / fromLen);

        float cosA = a.x * b.x + a.y * b.y + a.z * b.z;
        if (cosA > 1.0f) cosA = 1.0f;
        if (cosA < -1.0f) cosA = -1.0f;
        const float angle = acosf(cosA);
        if (angle <= maxRad) return b;

        const float sinA = sinf(angle);
        if (sinA < 1e-5f) return b;   // straight behind: take the target

        const float t0 = sinf(angle - maxRad) / sinA;
        const float t1 = sinf(maxRad) / sinA;
        Vector3 out(a.x * t0 + b.x * t1, a.y * t0 + b.y * t1, a.z * t0 + b.z * t1);
        const float len = Vector3::Magnitude(out);
        if (!(len > 1e-4f)) return b;
        return Vector3(out.x / len, out.y / len, out.z / len);
    }

    // Called from the local pawn's tick, on the game thread.
    inline void Tick(Pawn *localPawn)
    {
        // A new local pawn means a new match or a respawn. The held target and
        // the tick clock describe the old one and must not carry over.
        if ((void *)localPawn != g_lastLocal) {
            g_lastLocal = (void *)localPawn;
            g_lastTarget = nullptr;
            g_lastTick = Now();
        }

        const float strength = Config.Aim.AimLock;
        if (!(strength > 0.0f)) {
            g_lastTarget = nullptr;
            return;
        }
        if (!Alive((void *)localPawn)) {
            g_lastTarget = nullptr;
            return;
        }
        if (!TriggerReady(localPawn))
            return;

        const double now = Now();
        double dt = now - g_lastTick;
        g_lastTick = now;
        if (dt < 0.001) dt = 0.001;
        if (dt > 0.25)  dt = 0.25;          // a stalled thread must not teleport

        Camera *cam = Camera::get_main();
        if (!Tools::IsPtrValid((void *)cam)) return;
        Transform *camTransform = cam->get_transform();
        if (!Tools::IsPtrValid((void *)camTransform)) return;
        const Vector3 eye = camTransform->get_position();

        // The camera follows the aim, so its forward is the direction the shot
        // would take right now - what the turn has to start from.
        const Vector3 current = CameraForward((void *)camTransform);
        if (!(Vector3::Magnitude(current) > 1e-4f)) return;

        // Stay on the pawn that was picked, as long as it is still a legal
        // target. Re-picking the closest one every tick made the aim flick
        // between two enemies standing next to each other.
        Vector3 point;
        float dist = 0.0f;
        Pawn *target = nullptr;
        if (g_lastTarget != nullptr &&
            CandidateOk(localPawn, (Pawn *)g_lastTarget, eye, current, cam, point, dist)) {
            target = (Pawn *)g_lastTarget;
        }
        if (target == nullptr)
            target = PickTarget(localPawn, eye, current, cam, point);
        g_lastTarget = (void *)target;
        if (target == nullptr) return;

        const Vector3 want(point.x - eye.x, point.y - eye.y, point.z - eye.z);
        if (!(Vector3::Magnitude(want) > 0.05f)) return;

        // 100 is a snap: no rate limit at the top of the range. Below it the
        // speed rises with the square of the slider, 6 deg/s at 1 and ~600 deg/s
        // at 99.
        Vector3 stepped;
        if (strength >= 99.5f) {
            stepped = want;
        } else {
            const float t = strength * 0.01f;
            const float degPerSecond = 6.0f + 594.0f * t * t;
            const float maxRad = degPerSecond * (float)dt * 0.01745329f;
            stepped = StepToward(current, want, maxRad);
        }

        void (*setAim)(uintptr_t, Quaternion) =
            (void (*)(uintptr_t, Quaternion))(Class_Pawn_set_AimRotation);
        if (setAim == nullptr) return;

        const bool turned = LockGuardedRun("set aim rotation", (uintptr_t)localPawn, [&] {
            setAim((uintptr_t)localPawn, Quaternion::LookRotation(stepped, Vector3::Up()));
        });
        if (!turned) return;
        g_turns++;
    }
}
