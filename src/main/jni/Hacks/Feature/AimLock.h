#pragma once

#include <cmath>
#include <chrono>

#include "Init_Hacks.h"
#include "../../IL2CppSDKGenerator/BasicStructs/Quaternion.h"

// Aim Lock.
//
// Aim Assist only moves the aim inside the bullet hook, so it does nothing until
// you fire - and while you are scoping in, nothing is being fired. Aim Lock is
// the other half: on every local-pawn tick it turns the aim toward the chosen
// target, with or without firing or scoping, so the crosshair is already there
// when you take the shot.
//
// Config.Aim.AimLock is the strength: 0 turns it off, 100 is fast enough to read
// as instant for a flick. The turn is rate limited in degrees per second, so the
// same number behaves the same at any frame rate.
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

    inline double Now()
    {
        return std::chrono::duration<double>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
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

    inline Pawn *PickTarget(Pawn *localPawn, const Vector3 &eyePos, const Vector3 &forward)
    {
        void *game = ((void *(*)())(Class_Gameplay_get_MatchGame))();
        if (!Tools::IsPtrValid(game)) return nullptr;

        List<Pawn *> *enemies = (List<Pawn *> *)*(void **)((uintptr_t)game + Class_BaseGame_EnemyPawns);
        if (!Tools::IsPtrValid((void *)enemies)) return nullptr;
        Pawn **items = (Pawn **)enemies->getItems();
        const int count = enemies->getSize();
        if (!Tools::IsPtrValid((void *)items) || count <= 0 || count > 1024) return nullptr;

        const float fov   = Config.Aim.Cross > 0.0f ? Config.Aim.Cross : 180.0f;
        const float range = Config.Aim.size > 0.0f ? Config.Aim.size : 1000.0f;
        const bool  byAngle = (Config.Aim.By == EAim::Crosshair) &&
                              Vector3::Magnitude(forward) > 0.01f;

        Pawn *best = nullptr;
        float bestScore = 0.0f;
        for (int i = 0; i < count; ++i) {
            Pawn *pawn = items[i];
            if (!Alive((void *)pawn) || pawn == localPawn) continue;
            if (!*(bool *)((uintptr_t)pawn + Class_Pawn_m_IsAlive)) continue;
            if (!IsEnemy(localPawn, pawn)) continue;

            Vector3 point;
            if (!AimPoint(pawn, point)) continue;

            const Vector3 to(point.x - eyePos.x, point.y - eyePos.y, point.z - eyePos.z);
            const float dist = Vector3::Magnitude(to);
            if (!(dist > 0.05f) || dist > range) continue;

            float score = dist;
            if (byAngle) {
                float cosA = (to.x * forward.x + to.y * forward.y + to.z * forward.z) / dist;
                if (cosA > 1.0f) cosA = 1.0f;
                if (cosA < -1.0f) cosA = -1.0f;
                const float degrees = acosf(cosA) * 57.29578f;
                if (degrees > fov) continue;
                score = degrees * 1000.0f + dist * 0.001f;
            }
            if (best == nullptr || score < bestScore) {
                best = pawn;
                bestScore = score;
            }
        }
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
        const float strength = Config.Aim.AimLock;
        if (!(strength > 0.0f)) return;
        if (!Alive((void *)localPawn)) return;

        const double now = Now();
        double dt = (g_lastTick < 0.0) ? 0.016 : now - g_lastTick;
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

        Pawn *target = PickTarget(localPawn, eye, current);
        g_lastTarget = (void *)target;
        if (target == nullptr) return;

        Vector3 point;
        if (!AimPoint(target, point)) return;
        const Vector3 want(point.x - eye.x, point.y - eye.y, point.z - eye.z);
        if (!(Vector3::Magnitude(want) > 0.05f)) return;

        // 0..100 -> 6..600 degrees per second.
        const float degPerSecond = 6.0f + strength * 5.94f;
        const float maxRad = degPerSecond * (float)dt * 0.01745329f;

        const Vector3 stepped = StepToward(current, want, maxRad);
        void (*setAim)(uintptr_t, Quaternion) =
            (void (*)(uintptr_t, Quaternion))(Class_Pawn_set_AimRotation);
        if (setAim == nullptr) return;

        setAim((uintptr_t)localPawn, Quaternion::LookRotation(stepped, Vector3::Up()));
        g_turns++;
    }
}
