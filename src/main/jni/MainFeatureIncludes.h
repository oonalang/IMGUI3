#pragma once

#include "System/Hooks/Feats.h"

#include "System/Core/FileUtils.h"
#include "System/Core/ConfigManager.h"
#include "System/Core/SaveConfig.h"
#include "System/Core/Bypass.h"

#include "System/UI/GuestAccount.h"
#include "System/UI/FloatingInfo.h"
#include "System/UI/LoginGate.h"

// Skin/Fields.h is a fragment without a guard: include exactly this order.
#include "Skin/Fields.h"
#include "Skin/Thread.h"
#include "Skin/AutoEquipSkin.h"
#include "Skin/Camo.h"

#include "AstralPrtctn/StrEnc.h"
#include "AstralPrtctn/json.hpp"
#include "AstralPrtctn/md5.h"
#include "AstralPrtctn/AntiHook.h"
#include "curl/curl.h"
#include "xhook.h"
#include "Includes/Jni_Stuff.h"

// Neverlose/HexSync (IMGUI5) menu: the only UI. nl_ui is the adapter the
// render loop calls (Init/BeginFrame/Draw/EndFrame/OnTouch/Shutdown).
#include "nl_layout.hpp"
#include "menu/nl_ui.hpp"
#include "menu/nl_bindings.hpp"
#include "menu/nl_embedded_assets.hpp"
