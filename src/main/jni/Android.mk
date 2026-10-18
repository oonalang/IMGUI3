LOCAL_PATH := $(call my-dir)
MAIN_LOCAL_PATH := $(call my-dir)

# ============================================================================ #
include $(CLEAR_VARS)
LOCAL_MODULE := libcurl
LOCAL_SRC_FILES := curl/curl-android-$(TARGET_ARCH_ABI)/lib/libcurl.a
include $(PREBUILT_STATIC_LIBRARY)
# ============================================================================ #
include $(CLEAR_VARS)
LOCAL_MODULE := libssl
LOCAL_SRC_FILES := curl/openssl-android-$(TARGET_ARCH_ABI)/lib/libssl.a
include $(PREBUILT_STATIC_LIBRARY)
# ============================================================================ #
include $(CLEAR_VARS)
LOCAL_MODULE := libcrypto
LOCAL_SRC_FILES := curl/openssl-android-$(TARGET_ARCH_ABI)/lib/libcrypto.a
include $(PREBUILT_STATIC_LIBRARY)

# ============================================================================#
# ============================================================================
include $(CLEAR_VARS)
LOCAL_MODULE            := libdobby
LOCAL_SRC_FILES         := IL2CppSDKGenerator/Dobby/libraries/$(TARGET_ARCH_ABI)/libdobby.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/IL2CppSDKGenerator/Dobby/
include $(PREBUILT_STATIC_LIBRARY)

# ============================================================================ #
include $(CLEAR_VARS)
LOCAL_MODULE := libfoxcheats
LOCAL_SRC_FILES := foxcheats/libs/$(TARGET_ARCH_ABI)/libfoxcheats.a
include $(PREBUILT_STATIC_LIBRARY)
# ============================================================================#

include $(CLEAR_VARS)
LOCAL_MODULE    := libctorHook
LOCAL_SRC_FILES := ctorHook/libs/$(TARGET_ARCH_ABI)/libctorHook.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

# ============================================================================

# ============================================================================#

# ============================================================================#
# Your existing code remains unchanged above this point

# ============================================================================#
include $(CLEAR_VARS)
LOCAL_MODULE := libxhook
LOCAL_SRC_FILES := \
    SDK/Xhook/xhook.c \
    SDK/Xhook/xh_core.c \
    SDK/Xhook/xh_jni.c \
    SDK/Xhook/xh_elf.c \
    SDK/Xhook/xh_log.c \
    SDK/Xhook/xh_util.c \
    SDK/Xhook/xh_version.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/SDK/Xhook
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/SDK/Xhook
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE           := gspace1
LOCAL_CFLAGS           := -Wno-error=format-security -fvisibility=hidden -ffunction-sections -fdata-sections -w
LOCAL_CFLAGS           += -fno-rtti -fno-exceptions -fpermissive
LOCAL_CPPFLAGS         := -Wno-error=format-security -fvisibility=hidden -ffunction-sections -fdata-sections -w -Werror -s -std=c++17
LOCAL_CPPFLAGS         += -Wno-error=c++11-narrowing -fms-extensions -fno-rtti -fno-exceptions -fpermissive -DIMGUI_IMPL_OPENGL_ES3


LOCAL_LDFLAGS          += -Wl,--gc-sections,--strip-all, -llog
LOCAL_ARM_MODE         := arm
LOCAL_LDLIBS           := -llog -landroid -lEGL -lGLESv3 -lGLESv2 -lGLESv1_CM -lz

LOCAL_C_INCLUDES       += $(LOCAL_PATH)
LOCAL_C_INCLUDES       += $(LOCAL_PATH)/menu

# ImGui is listed explicitly, never by wildcard: the menu runs on a patched
# core (gui colors, draw_blur, rounded multi-colour rects), so every core TU and
# the GLES3 backend have to end up in libgspace1.so.
# Fonts come from stb_truetype (imgui_draw.cpp); FreeType is not part of the
# NDK build, so IMGUI_ENABLE_FREETYPE must stay undefined.
IMGUI_SOURCES          := \
    ImGui/imgui.cpp \
    ImGui/imgui_draw.cpp \
    ImGui/imgui_widgets.cpp \
    ImGui/imgui_tables.cpp \
    ImGui/backends/imgui_impl_opengl3.cpp \
    menu/neverlose_menu.cpp \
    menu/hexsync_intro.cpp \
    menu/hexsync_glow.cpp \
    menu/gui.cpp \
    menu/blur.cpp \
    menu/nl_ui.cpp \
    menu/nl_bindings.cpp \
    menu/nl_embedded_assets.cpp

FILE_LIST              := $(addprefix $(LOCAL_PATH)/,$(IMGUI_SOURCES))

FILE_LIST              += $(wildcard $(LOCAL_PATH)/IL2CppSDKGenerator/IL2Cpp/*.c*)
FILE_LIST              += $(wildcard $(LOCAL_PATH)/IL2CppSDKGenerator/KittyMemory/*.c*)
FILE_LIST              += $(wildcard $(LOCAL_PATH)/System/Texture/*.c*)
FILE_LIST              += $(wildcard $(LOCAL_PATH)/*.c) $(wildcard $(LOCAL_PATH)/*.cpp)

LOCAL_SRC_FILES        := $(FILE_LIST:$(LOCAL_PATH)/%=%) \
                              Substrate/hde64.c \
                              Substrate/SubstrateDebug.cpp \
                              Substrate/SubstrateHook.cpp \
                              Substrate/SubstratePosixMemory.cpp \
                              Substrate/And64InlineHook.cpp \
                              AstralPrtctn/md5.cpp
                              
LOCAL_LDLIBS := -llog -landroid
FILE_LIST := $(wildcard $(LOCAL_PATH)/libzip/*.c)
LOCAL_SRC_FILES += $(FILE_LIST:$(LOCAL_PATH)/%=%)

LOCAL_C_INCLUDES := $(LOCAL_PATH)/curl/curl-android-$(TARGET_ARCH_ABI)/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/curl/openssl-android-$(TARGET_ARCH_ABI)/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/libzip

LOCAL_LDLIBS           := -llog -landroid -lz -lEGL -lGLESv2 -lGLESv3
LOCAL_STATIC_LIBRARIES := libcurl libssl libcrypto libdobby libfoxcheats libxhook libctorHook

LOCAL_CPP_FEATURES                      := exceptions
LOCAL_C_INCLUDES := $(LOCAL_PATH)/curl/curl-android-$(TARGET_ARCH_ABI)/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/curl/openssl-android-$(TARGET_ARCH_ABI)/include

LOCAL_C_INCLUDES += $(LOCAL_PATH)/foxcheats
LOCAL_C_INCLUDES += $(LOCAL_PATH)/foxcheats/includes

                     
include $(BUILD_SHARED_LIBRARY)
# ============================================================================
