#pragma once

#include <cstdint>

// "Apply the skin the moment it is equipped" - the state half.
//
// The equip gates in System/Hooks/BlueprintEquip.h run on the game's own thread,
// so they only RECORD the item the player is putting in a bag. The config-table
// walk and the write happen once per rendered frame (Skin/AutoEquipSkin.h), the
// same rule the kill-decoration writes follow: never walk a game container from
// inside a hooked native call.
//
// This header deliberately depends on nothing but <cstdint>, because
// System/Hooks/BlueprintEquip.h includes it in the middle of the hook graph.
inline uint32_t g_autoEquipSkinPendingID = 0;
inline int      g_autoEquipSkinPendingFrames = 0;
inline int      g_autoEquipSkinCandidates = 0;

// Called by an equip gate with the item id it was asked about. The last id wins
// - the gates are asked about the item under the finger, and the frame that
// follows applies it, so two ids in one frame is already a losing race.
inline void AutoEquipSkin_NoteCandidate(uint32_t itemID)
{
    if (itemID == 0)
        return;
    g_autoEquipSkinCandidates++;
    g_autoEquipSkinPendingID = itemID;
    g_autoEquipSkinPendingFrames = 0;
}
