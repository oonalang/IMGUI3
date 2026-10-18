#include "Includes/Logger.h"
#include "Includes/Macros.h"
#include "Includes/obfuscate.h"
#include "Includes/Utils.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "backends/imgui_impl_opengl3.h"
#include "Includes/OverlayGeometry.h"
#include "IL2CppSDKGenerator/BasicStructs/Call_BasicStructs.h"
#include "IL2CppSDKGenerator/IL2Cpp/Call_IL2Cpp.h"
#include "Hacks/Hacks.h"
#include "IL2CppSDKGenerator/KittyMemory/MemoryPatch.h"
#include "AstralPrtctn/AntiHook.h"
#include "foxcheats/include/ScanEngine.hpp"
#include "Includes/Jni_Stuff.h"

#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <string>
#include <functional>
#include <cstring>
#include <cfloat>
#include <jni.h>
#include <pthread.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>

#include "oxorany/source/oxorany.h"
#include "oxorany/source/oxorany.cpp"
#include "oxorany/source/oxorany_include.h"

#include "FeedbackService.h"
// NewFeatures must be visible before MainFeatureIncludes.h: the existing
// Feats.h firing-timing hooks (Firerate / No Sprint Fire Delay / No Fire
// Delay) call into NewFeatures::NoFireDelayActive() /
// NoSprintFireDelayActive() so that each dump address is hooked exactly once.
#include "Hacks/Feature/NewFeatures.h"
#include "Hacks/Feature/Crosshair.h"
// Crash breadcrumb: the triggerbot's live state, written from a signal handler
// so a native crash in the game process leaves a name behind.
#include "Hacks/Feature/CrashBreadcrumb.h"
#include "MainFeatureIncludes.h"
// Apply Skin On Equip: takes the id the equip gates recorded and puts that skin
// on the gun locally (Skin/AutoEquipSkin.h).
#include "Skin/AutoEquipSkin.h"

#include <jni.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_activity.h>
#include <android/native_window_jni.h>
#include <dlfcn.h>
#include <sys/system_properties.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <array>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <mutex>

static auto lastScreenshotTime = std::chrono::steady_clock::now();
constexpr std::chrono::seconds screenshotInterval(680);

namespace android::anwcreator::detail::types
{
    enum class SurfaceControlFlags : uint32_t
    {
        eHidden          = 0x00000004,
        eSkipScreenshot  = 0x00000040,
        eSecure          = 0x00000080,
        eNonPremultiplied= 0x00000100,
        eOpaque          = 0x00000400,
        eNoColorFill     = 0x00004000,
    };
    enum class DisplayRotation : int32_t
    { Rotation0 = 0, Rotation90 = 1, Rotation180 = 2, Rotation270 = 3 };

    template <typename enum_t, typename = std::enable_if_t<std::is_enum_v<enum_t>>>
    constexpr enum_t operator|(enum_t lhs, enum_t rhs) {
        using U = std::underlying_type_t<enum_t>;
        return static_cast<enum_t>(static_cast<U>(lhs) | static_cast<U>(rhs));
    }
    template <typename enum_t, typename = std::enable_if_t<std::is_enum_v<enum_t>>>
    constexpr enum_t operator|=(enum_t &lhs, enum_t rhs) { return (lhs = lhs | rhs); }
}

namespace android::anwcreator::detail::jni
{
    struct JNIEnvironment {
        JNIEnv *env; JavaVM *vm;
        JNIEnvironment(JavaVM *javaVM) : env(nullptr), vm(javaVM) { if (vm) vm->AttachCurrentThread(&env, nullptr); }
        ~JNIEnvironment() {}
        inline bool IsValid() const { return env != nullptr; }
        inline operator JNIEnv *() const { return env; }
        inline JNIEnv *operator->() const { return env; }
        inline bool CheckException(const char * = nullptr) {
            if (!env || !env->ExceptionCheck()) return false;
            env->ExceptionDescribe(); env->ExceptionClear(); return true;
        }
    };
    struct LocalRef {
        JNIEnv *env; jobject obj;
        LocalRef(JNIEnv *e, jobject o) : env(e), obj(o) {}
        ~LocalRef() { if (env && obj) env->DeleteLocalRef(obj); }
        LocalRef(const LocalRef &) = delete;
        LocalRef &operator=(const LocalRef &) = delete;
        LocalRef(LocalRef &&o) noexcept : env(o.env), obj(o.obj) { o.obj = nullptr; }
        inline operator jobject() const { return obj; }
        inline operator jclass() const { return reinterpret_cast<jclass>(obj); }
        inline jobject get() const { return obj; }
        inline jclass getClass() const { return reinterpret_cast<jclass>(obj); }
        inline bool IsValid() const { return obj != nullptr; }
        inline explicit operator bool() const { return obj != nullptr; }
    };
    struct GlobalRef {
        JNIEnv *env; jobject obj;
        GlobalRef(JNIEnv *e, jobject o) : env(e), obj(nullptr) { if (e && o) obj = e->NewGlobalRef(o); }
        ~GlobalRef() { Release(); }
        void Release() { if (env && obj) { env->DeleteGlobalRef(obj); obj = nullptr; } }
        GlobalRef(const GlobalRef &) = delete;
        GlobalRef &operator=(const GlobalRef &) = delete;
        GlobalRef(GlobalRef &&o) noexcept : env(o.env), obj(o.obj) { o.obj = nullptr; }
        inline operator jobject() const { return obj; }
        inline jobject get() const { return obj; }
        inline bool IsValid() const { return obj != nullptr; }
        inline explicit operator bool() const { return obj != nullptr; }
    };
}

namespace android::anwcreator::detail::framework
{
    struct DisplayMetrics { int32_t widthPixels, heightPixels; float density; int32_t densityDpi; };
    struct DisplayInfo { int32_t width, height; types::DisplayRotation rotation; float refreshRate; };

    class Display {
    public:
        static DisplayInfo GetDisplayInfo(JNIEnv *env, ANativeActivity *activity) {
            DisplayInfo info{};
            if (!env || !activity || !activity->clazz) return info;
            jni::LocalRef activityCls(env, env->GetObjectClass(activity->clazz));
            if (!activityCls) return info;
            jmethodID getWindowManager = env->GetMethodID(activityCls, "getWindowManager", "()Landroid/view/WindowManager;");
            if (!getWindowManager) return info;
            jni::LocalRef windowManager(env, env->CallObjectMethod(activity->clazz, getWindowManager));
            if (!windowManager || jni::JNIEnvironment(activity->vm).CheckException()) return info;
            jni::LocalRef windowManagerCls(env, env->GetObjectClass(windowManager));
            jmethodID getDefaultDisplay = env->GetMethodID(windowManagerCls, "getDefaultDisplay", "()Landroid/view/Display;");
            if (!getDefaultDisplay) return info;
            jni::LocalRef display(env, env->CallObjectMethod(windowManager, getDefaultDisplay));
            if (!display || jni::JNIEnvironment(activity->vm).CheckException()) return info;
            jni::LocalRef displayMetricsCls(env, env->FindClass("android/util/DisplayMetrics"));
            jmethodID ctor = env->GetMethodID(displayMetricsCls, "<init>", "()V");
            jni::LocalRef displayMetrics(env, env->NewObject(displayMetricsCls, ctor));
            jni::LocalRef displayCls(env, env->GetObjectClass(display));
            jmethodID getRealMetrics = env->GetMethodID(displayCls, "getRealMetrics", "(Landroid/util/DisplayMetrics;)V");
            env->CallVoidMethod(display, getRealMetrics, displayMetrics.get());
            jfieldID widthPixelsField  = env->GetFieldID(displayMetricsCls, "widthPixels", "I");
            jfieldID heightPixelsField = env->GetFieldID(displayMetricsCls, "heightPixels", "I");
            info.width  = env->GetIntField(displayMetrics, widthPixelsField);
            info.height = env->GetIntField(displayMetrics, heightPixelsField);
            jmethodID getRotation = env->GetMethodID(displayCls, "getRotation", "()I");
            info.rotation = static_cast<types::DisplayRotation>(env->CallIntMethod(display, getRotation));
            jmethodID getRefreshRate = env->GetMethodID(displayCls, "getRefreshRate", "()F");
            if (getRefreshRate) info.refreshRate = env->CallFloatMethod(display, getRefreshRate);
            return info;
        }
    };

    class SurfaceControl {
    public:
        struct Builder {
            JNIEnv *env; jobject builder; jclass builderClass;
            Builder(JNIEnv *e) : env(e), builder(nullptr), builderClass(nullptr) {
                builderClass = env->FindClass("android/view/SurfaceControl$Builder");
                if (!builderClass) return;
                jmethodID ctor = env->GetMethodID(builderClass, "<init>", "()V");
                builder = env->NewObject(builderClass, ctor);
            }
            ~Builder() {
                if (env && builder) env->DeleteLocalRef(builder);
                if (env && builderClass) env->DeleteLocalRef(builderClass);
            }
            Builder &SetName(const char *name) {
                if (!builder) return *this;
                jmethodID m = env->GetMethodID(builderClass, "setName", "(Ljava/lang/String;)Landroid/view/SurfaceControl$Builder;");
                if (m) { jni::LocalRef jName(env, env->NewStringUTF(name)); builder = env->CallObjectMethod(builder, m, jName.get()); }
                return *this;
            }
            Builder &SetParent(jobject parent) {
                if (!builder || !parent) return *this;
                jmethodID m = env->GetMethodID(builderClass, "setParent", "(Landroid/view/SurfaceControl;)Landroid/view/SurfaceControl$Builder;");
                if (m) builder = env->CallObjectMethod(builder, m, parent);
                return *this;
            }
            Builder &SetBufferSize(int32_t w, int32_t h) {
                if (!builder) return *this;
                jmethodID m = env->GetMethodID(builderClass, "setBufferSize", "(II)Landroid/view/SurfaceControl$Builder;");
                if (m) builder = env->CallObjectMethod(builder, m, w, h);
                return *this;
            }
            Builder &SetFormat(int32_t f) {
                if (!builder) return *this;
                jmethodID m = env->GetMethodID(builderClass, "setFormat", "(I)Landroid/view/SurfaceControl$Builder;");
                if (m) builder = env->CallObjectMethod(builder, m, (jint)f);
                return *this;
            }
            Builder &SetOpaque(bool o) {
                if (!builder) return *this;
                jmethodID m = env->GetMethodID(builderClass, "setOpaque", "(Z)Landroid/view/SurfaceControl$Builder;");
                if (m) builder = env->CallObjectMethod(builder, m, (jboolean)o);
                return *this;
            }
            Builder &SetFlags(uint32_t f, uint32_t mask) {
                if (!builder) return *this;
                jmethodID m = env->GetMethodID(builderClass, "setFlags", "(II)Landroid/view/SurfaceControl$Builder;");
                if (m) builder = env->CallObjectMethod(builder, m, (jint)f, (jint)mask);
                return *this;
            }
            // SurfaceControl.SKIP_SCREENSHOT (0x40, AOSP API 31+) makes the
            // layer and its children invisible to screenshots, mirroring and
            // screen recording. It is only reachable through the builder's
            // private setFlags(int, int) helper (see AOSP SurfaceControl.java),
            // which JNI can call. Returns true only when the call really
            // reached the framework, so a caller can refuse to build a
            // "hidden" overlay that is not hidden at all.
            bool SetSkipScreenshot(bool skip) {
                if (!skip || !builder || !builderClass) return false;
                constexpr int32_t FLAG_SKIP_SCREENSHOT = 0x40;
                jmethodID m = env->GetMethodID(builderClass, "setFlags",
                    "(II)Landroid/view/SurfaceControl$Builder;");
                if (!m) { if (env->ExceptionCheck()) env->ExceptionClear(); return false; }
                jobject r = env->CallObjectMethod(builder, m,
                    (jint)FLAG_SKIP_SCREENSHOT, (jint)FLAG_SKIP_SCREENSHOT);
                if (env->ExceptionCheck()) { env->ExceptionClear(); return false; }
                if (r) builder = r;
                return true;
            }
            jobject Build() {
                if (!builder) return nullptr;
                jmethodID m = env->GetMethodID(builderClass, "build", "()Landroid/view/SurfaceControl;");
                if (!m) return nullptr;
                return env->CallObjectMethod(builder, m);
            }
            inline bool IsValid() const { return builder != nullptr; }
        };

        class Transaction {
        public:
            JNIEnv *env; jobject transaction; jclass transactionClass;
            Transaction(JNIEnv *e) : env(e), transaction(nullptr), transactionClass(nullptr) {
                transactionClass = env->FindClass("android/view/SurfaceControl$Transaction");
                if (!transactionClass) return;
                jmethodID ctor = env->GetMethodID(transactionClass, "<init>", "()V");
                transaction = env->NewObject(transactionClass, ctor);
            }
            ~Transaction() {
                if (env && transaction) env->DeleteLocalRef(transaction);
                if (env && transactionClass) env->DeleteLocalRef(transactionClass);
            }
            Transaction &SetAlpha(jobject sc, float a) {
                if (!transaction || !sc) return *this;
                jmethodID m = env->GetMethodID(transactionClass, "setAlpha", "(Landroid/view/SurfaceControl;F)Landroid/view/SurfaceControl$Transaction;");
                if (m) transaction = env->CallObjectMethod(transaction, m, sc, a);
                return *this;
            }
            Transaction &SetLayer(jobject sc, int32_t z) {
                if (!transaction || !sc) return *this;
                jmethodID m = env->GetMethodID(transactionClass, "setLayer", "(Landroid/view/SurfaceControl;I)Landroid/view/SurfaceControl$Transaction;");
                if (m) transaction = env->CallObjectMethod(transaction, m, sc, z);
                return *this;
            }
            Transaction &SetPosition(jobject sc, float x, float y) {
                if (!transaction || !sc) return *this;
                jmethodID m = env->GetMethodID(transactionClass, "setPosition", "(Landroid/view/SurfaceControl;FF)Landroid/view/SurfaceControl$Transaction;");
                if (m) transaction = env->CallObjectMethod(transaction, m, sc, x, y);
                return *this;
            }
            Transaction &SetMatrix(jobject sc, float a, float b, float c, float d) {
                if (!transaction || !sc) return *this;
                jmethodID m = env->GetMethodID(transactionClass, "setMatrix", "(Landroid/view/SurfaceControl;FFFF)Landroid/view/SurfaceControl$Transaction;");
                if (m) transaction = env->CallObjectMethod(transaction, m, sc, a, b, c, d);
                return *this;
            }
            Transaction &Show(jobject sc) {
                if (!transaction || !sc) return *this;
                jmethodID m = env->GetMethodID(transactionClass, "show", "(Landroid/view/SurfaceControl;)Landroid/view/SurfaceControl$Transaction;");
                if (m) transaction = env->CallObjectMethod(transaction, m, sc);
                return *this;
            }
            Transaction &Hide(jobject sc) {
                if (!transaction || !sc) return *this;
                jmethodID m = env->GetMethodID(transactionClass, "hide", "(Landroid/view/SurfaceControl;)Landroid/view/SurfaceControl$Transaction;");
                if (m) transaction = env->CallObjectMethod(transaction, m, sc);
                return *this;
            }
            Transaction &SetParent(jobject c, jobject p) {
                if (!transaction || !c || !p) return *this;
                jmethodID m = env->GetMethodID(transactionClass, "setParent", "(Landroid/view/SurfaceControl;Landroid/view/SurfaceControl;)Landroid/view/SurfaceControl$Transaction;");
                if (m) transaction = env->CallObjectMethod(transaction, m, c, p);
                return *this;
            }
            // Public @hide SurfaceControl.Transaction#setSkipScreenshot
            // (API 31+): the framework's own way of flagging an existing layer
            // as screenshot / screen-recording invisible. Returns true when the
            // method exists on this Android version.
            bool SetSkipScreenshot(jobject sc, bool skip) {
                if (!transaction || !sc) return false;
                jmethodID m = env->GetMethodID(transactionClass, "setSkipScreenshot",
                    "(Landroid/view/SurfaceControl;Z)Landroid/view/SurfaceControl$Transaction;");
                if (!m) { if (env->ExceptionCheck()) env->ExceptionClear(); return false; }
                jobject r = env->CallObjectMethod(transaction, m, sc, (jboolean)skip);
                if (env->ExceptionCheck()) { env->ExceptionClear(); return false; }
                if (r) transaction = r;
                return true;
            }
            Transaction &Remove(jobject sc) {
                if (!transaction || !sc) return *this;
                jmethodID m = env->GetMethodID(transactionClass, "remove", "(Landroid/view/SurfaceControl;)Landroid/view/SurfaceControl$Transaction;");
                if (m) transaction = env->CallObjectMethod(transaction, m, sc);
                return *this;
            }
            void Apply() {
                if (!transaction) return;
                jmethodID m = env->GetMethodID(transactionClass, "apply", "()V");
                if (m) env->CallVoidMethod(transaction, m);
            }
            inline bool IsValid() const { return transaction != nullptr; }
        };

        static jobject GetParentSurfaceControl(JNIEnv *env, ANativeActivity *activity) {
            if (!env || !activity || !activity->clazz) return nullptr;
            jni::LocalRef activityCls(env, env->GetObjectClass(activity->clazz));
            jmethodID getWindow = env->GetMethodID(activityCls, "getWindow", "()Landroid/view/Window;");
            if (!getWindow) return nullptr;
            jni::LocalRef window(env, env->CallObjectMethod(activity->clazz, getWindow));
            if (!window) return nullptr;
            jni::LocalRef windowCls(env, env->GetObjectClass(window));
            jmethodID getDecorView = env->GetMethodID(windowCls, "getDecorView", "()Landroid/view/View;");
            if (!getDecorView) return nullptr;
            jni::LocalRef decorView(env, env->CallObjectMethod(window, getDecorView));
            if (!decorView) return nullptr;
            jni::LocalRef viewCls(env, env->GetObjectClass(decorView));
            jmethodID getViewRootImpl = env->GetMethodID(viewCls, "getViewRootImpl", "()Landroid/view/ViewRootImpl;");
            if (!getViewRootImpl) return nullptr;
            jni::LocalRef viewRootImpl(env, env->CallObjectMethod(decorView, getViewRootImpl));
            if (!viewRootImpl) return nullptr;
            jni::LocalRef vriCls(env, env->GetObjectClass(viewRootImpl));
            jmethodID getSC = env->GetMethodID(vriCls, "getSurfaceControl", "()Landroid/view/SurfaceControl;");
            if (!getSC) return nullptr;
            return env->CallObjectMethod(viewRootImpl, getSC);
        }
    };

    class Surface {
    public:
        static jobject CreateFromSurfaceControl(JNIEnv *env, jobject sc) {
            if (!env || !sc) return nullptr;
            jni::LocalRef cls(env, env->FindClass("android/view/Surface"));
            if (!cls) return nullptr;
            jmethodID ctor = env->GetMethodID(cls, "<init>", "(Landroid/view/SurfaceControl;)V");
            if (!ctor) return nullptr;
            return env->NewObject(cls, ctor, sc);
        }
    };
}

namespace android::anwcreator::detail
{
    struct WindowContext {
        std::unique_ptr<jni::GlobalRef> surfaceControl, surface, parentSurfaceControl;
        ANativeWindow *nativeWindow; int32_t width, height;
        bool skipScreenshot, skipScreenshotApplied;
        WindowContext() : surfaceControl(nullptr), surface(nullptr), parentSurfaceControl(nullptr),
                          nativeWindow(nullptr), width(0), height(0), skipScreenshot(false),
                          skipScreenshotApplied(false) {}
        ~WindowContext() { Release(); }
        void Release() {
            if (nativeWindow) { ANativeWindow_release(nativeWindow); nativeWindow = nullptr; }
            surface.reset(); surfaceControl.reset(); parentSurfaceControl.reset();
        }
    };
}

namespace android
{
    class ANwCreator {
    public:
        struct DisplayInfo { int32_t theta, width, height; float refreshRate;
            DisplayInfo() : theta(0), width(0), height(0), refreshRate(60.0f) {} };
        struct CreateOptions { const char *name; int32_t width, height; bool skipScreenshot;
            CreateOptions() : name(""), width(-1), height(-1), skipScreenshot(false) {} };

        static DisplayInfo GetDisplayInfo(ANativeActivity *a) {
            DisplayInfo r{};
            if (!a || !a->vm || !a->clazz) return r;
            anwcreator::detail::jni::JNIEnvironment je(a->vm);
            if (!je.IsValid()) return r;
            auto d = anwcreator::detail::framework::Display::GetDisplayInfo(je, a);
            r.width = d.width; r.height = d.height;
            r.theta = 90 * static_cast<int32_t>(d.rotation);
            r.refreshRate = d.refreshRate;
            return r;
        }

        static ANativeWindow *Create(ANativeActivity *a, const CreateOptions &o = CreateOptions()) {
            if (!a || !a->vm || !a->clazz) return nullptr;
            anwcreator::detail::jni::JNIEnvironment je(a->vm);
            if (!je.IsValid()) return nullptr;
            int32_t w = o.width, h = o.height;
            if (w <= 0 || h <= 0) {
                auto d = anwcreator::detail::framework::Display::GetDisplayInfo(je, a);
                w = d.width; h = d.height;
            }
            jobject parentSC = anwcreator::detail::framework::SurfaceControl::GetParentSurfaceControl(je, a);
            if (!parentSC && je.CheckException()) return nullptr;
            anwcreator::detail::framework::SurfaceControl::Builder b(je);
            if (!b.IsValid()) return nullptr;
            // Format 1 == WINDOW_FORMAT_RGBA_8888 == it can carry per-pixel
            // alpha, which the overlay needs to stay see-through.
            b.SetName(o.name).SetBufferSize(w, h).SetFormat(1).SetOpaque(false);
            const bool skipViaBuilder = b.SetSkipScreenshot(o.skipScreenshot);
            if (parentSC) b.SetParent(parentSC);
            jobject lsc = b.Build();
            if (!lsc || je.CheckException()) { if (parentSC) je->DeleteLocalRef(parentSC); return nullptr; }
            auto ctx = std::make_unique<anwcreator::detail::WindowContext>();
            ctx->width = w; ctx->height = h; ctx->skipScreenshot = o.skipScreenshot;
            bool skipViaTransaction = false;
            ctx->surfaceControl = std::make_unique<anwcreator::detail::jni::GlobalRef>(je, lsc);
            if (parentSC) ctx->parentSurfaceControl = std::make_unique<anwcreator::detail::jni::GlobalRef>(je, parentSC);
            {
                anwcreator::detail::framework::SurfaceControl::Transaction t(je);
                if (t.IsValid() && o.skipScreenshot)
                    skipViaTransaction = t.SetSkipScreenshot(ctx->surfaceControl->get(), true);
                if (t.IsValid())
                    t.SetAlpha(ctx->surfaceControl->get(), 1.0f)
                     .SetLayer(ctx->surfaceControl->get(), 0x7FFFFFFE)
                     .SetPosition(ctx->surfaceControl->get(), 0.0f, 0.0f)
                     .SetMatrix(ctx->surfaceControl->get(), 1.0f, 0.0f, 0.0f, 1.0f)
                     .Show(ctx->surfaceControl->get()).Apply();
            }
            jobject ls = anwcreator::detail::framework::Surface::CreateFromSurfaceControl(je, ctx->surfaceControl->get());
            if (!ls || je.CheckException()) {
                je->DeleteLocalRef(lsc); if (parentSC) je->DeleteLocalRef(parentSC);
                return nullptr;
            }
            ctx->surface = std::make_unique<anwcreator::detail::jni::GlobalRef>(je, ls);
            ctx->nativeWindow = ANativeWindow_fromSurface(je, ctx->surface->get());
            if (!ctx->nativeWindow) {
                je->DeleteLocalRef(ls); je->DeleteLocalRef(lsc); if (parentSC) je->DeleteLocalRef(parentSC);
                return nullptr;
            }
            ctx->skipScreenshotApplied = o.skipScreenshot && (skipViaBuilder || skipViaTransaction);
            ANativeWindow *result = ctx->nativeWindow;
            m_windowContexts.emplace(result, std::move(ctx));
            je->DeleteLocalRef(ls); je->DeleteLocalRef(lsc); if (parentSC) je->DeleteLocalRef(parentSC);
            return result;
        }

        static void Destroy(ANativeActivity *a, ANativeWindow *nw) {
            if (!nw) return;
            auto it = m_windowContexts.find(nw);
            if (it == m_windowContexts.end()) { ANativeWindow_release(nw); return; }
            auto &ctx = it->second;
            if (a && a->vm && ctx->surfaceControl && ctx->surfaceControl->IsValid()) {
                anwcreator::detail::jni::JNIEnvironment je(a->vm);
                if (je.IsValid()) {
                    anwcreator::detail::framework::SurfaceControl::Transaction t(je);
                    if (t.IsValid()) t.Remove(ctx->surfaceControl->get()).Apply();
                }
            }
            ctx->Release();
            m_windowContexts.erase(it);
        }

        static bool IsValid(ANativeWindow *nw) { return nw && m_windowContexts.count(nw) > 0; }

        static bool GetWindowSize(ANativeWindow *nw, int32_t *w, int32_t *h) {
            auto it = m_windowContexts.find(nw);
            if (it == m_windowContexts.end()) return false;
            if (w) *w = it->second->width;
            if (h) *h = it->second->height;
            return true;
        }

        // True only when the layer really carries SurfaceControl.SKIP_SCREENSHOT.
        static bool SkipScreenshotApplied(ANativeWindow *nw) {
            auto it = m_windowContexts.find(nw);
            return it != m_windowContexts.end() && it->second->skipScreenshotApplied;
        }

        static void EnsureVisible(ANativeActivity *a, ANativeWindow *nw) {
            if (!a || !a->vm || !nw) return;
            auto it = m_windowContexts.find(nw);
            if (it == m_windowContexts.end()) return;
            auto &ctx = it->second;
            if (!ctx || !ctx->surfaceControl || !ctx->surfaceControl->IsValid()) return;
            anwcreator::detail::jni::JNIEnvironment je(a->vm);
            if (!je.IsValid()) return;
            anwcreator::detail::framework::SurfaceControl::Transaction t(je);
            if (!t.IsValid()) return;
            jobject cp = anwcreator::detail::framework::SurfaceControl::GetParentSurfaceControl(je, a);
            bool needReparent = false;
            if (cp && ctx->parentSurfaceControl && ctx->parentSurfaceControl->IsValid()) {
                if (!je->IsSameObject(cp, ctx->parentSurfaceControl->get())) needReparent = true;
            } else if (cp && !ctx->parentSurfaceControl) needReparent = true;
            if (needReparent && cp) {
                t.SetParent(ctx->surfaceControl->get(), cp);
                ctx->parentSurfaceControl = std::make_unique<anwcreator::detail::jni::GlobalRef>(je, cp);
            }
            float sx = 1.0f, sy = 1.0f;
            auto d = anwcreator::detail::framework::Display::GetDisplayInfo(je, a);
            if (ctx->width > 0 && ctx->height > 0 && d.width > 0 && d.height > 0) {
                int32_t tw = d.width, th = d.height;
                const bool bL = ctx->width >= ctx->height, tL = tw >= th;
                if (bL != tL) std::swap(tw, th);
                sx = (float)tw / (float)ctx->width;
                sy = (float)th / (float)ctx->height;
                if (!(sx > 0.0f) || !(sy > 0.0f) || sx > 4.0f || sy > 4.0f) sx = sy = 1.0f;
            }
            t.SetAlpha(ctx->surfaceControl->get(), 1.0f)
             .SetLayer(ctx->surfaceControl->get(), 0x7FFFFFFE)
             .SetPosition(ctx->surfaceControl->get(), 0.0f, 0.0f)
             .SetMatrix(ctx->surfaceControl->get(), sx, 0.0f, 0.0f, sy)
             .Show(ctx->surfaceControl->get()).Apply();
            if (cp) je->DeleteLocalRef(cp);
        }

    private:
        inline static std::unordered_map<ANativeWindow *, std::unique_ptr<anwcreator::detail::WindowContext>> m_windowContexts;
    };
}

#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <android/native_window.h>
#include <android/native_activity.h>

namespace android {
class HideRecAImGui {
public:
    struct Options { ANativeActivity* activity=nullptr; bool skipScreenshot=true; };
    using SwapBuffersFn=EGLBoolean (*)(EGLDisplay,EGLSurface);
    HideRecAImGui():HideRecAImGui(Options{}){}
    explicit HideRecAImGui(const Options& options);
    ~HideRecAImGui();
    bool RenderDrawData(ImDrawData*,EGLDisplay,EGLSurface,EGLContext,SwapBuffersFn);
    void Shutdown();
    constexpr operator bool() const{return m_state;}
private:
    bool EnsureEnvironment(EGLDisplay,EGLContext,int32_t,int32_t);
    bool InitEnvironment(EGLDisplay,EGLContext,int32_t,int32_t);
    void UnInitEnvironment();
    EGLConfig FindConfig(EGLDisplay,EGLContext);
    bool m_state=false;
    int32_t m_screenWidth=-1,m_screenHeight=-1;
    Options m_options{};
    ANativeWindow* m_nativeWindow=nullptr;
    EGLDisplay m_display=EGL_NO_DISPLAY;
    EGLSurface m_surface=EGL_NO_SURFACE;
    EGLContext m_overlayContext=EGL_NO_CONTEXT;
    EGLContext m_gameContext=EGL_NO_CONTEXT;
};
}

#include <EGL/eglext.h>
#include <cmath>

namespace android {
HideRecAImGui::HideRecAImGui(const Options& o):m_options(o){}
HideRecAImGui::~HideRecAImGui(){UnInitEnvironment();}

// WINDOW_FORMAT_RGBA_8888 == AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM == 1;
// no other Android window format carries an alpha channel.
static bool HideRecConfigIsTranslucent(EGLDisplay d, EGLConfig cfg) {
    if (d == EGL_NO_DISPLAY || !cfg) return false;
    EGLint alpha = 0, visual = 0;
    if (eglGetConfigAttrib(d, cfg, EGL_ALPHA_SIZE, &alpha) != EGL_TRUE || alpha < 8) return false;
    if (eglGetConfigAttrib(d, cfg, EGL_NATIVE_VISUAL_ID, &visual) != EGL_TRUE) return false;
    return visual == 1;
}

EGLConfig HideRecAImGui::FindConfig(EGLDisplay d, EGLContext gc) {
    (void)gc;
    // The overlay is composited ON TOP of the game, so every pixel ImGui does
    // not draw stays (0,0,0,0) and has to blend through to the game. A config
    // without an alpha channel turns those pixels into opaque black and the
    // overlay blacks the whole display out - that is the "Hide Stream black
    // screen". Do NOT inherit the game's EGLConfig here: the game draws to an
    // opaque window, so its config usually has EGL_ALPHA_SIZE == 0.
    if (d == EGL_NO_DISPLAY) return nullptr;
    const EGLint attrs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, 0x00000040,
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
        EGL_NONE
    };
    EGLConfig cfg = nullptr; EGLint count = 0;
    if (eglChooseConfig(d, attrs, &cfg, 1, &count) == EGL_TRUE && count > 0 && cfg &&
        HideRecConfigIsTranslucent(d, cfg)) return cfg;
    EGLint total = 0;
    if (eglGetConfigs(d, nullptr, 0, &total) != EGL_TRUE || total <= 0) return nullptr;
    std::vector<EGLConfig> cfgs((size_t)total);
    if (eglGetConfigs(d, cfgs.data(), total, &total) != EGL_TRUE) return nullptr;
    for (int i = 0; i < total; i++) {
        EGLint st = 0, rt = 0;
        eglGetConfigAttrib(d, cfgs[i], EGL_SURFACE_TYPE, &st);
        if (!(st & EGL_WINDOW_BIT)) continue;
        eglGetConfigAttrib(d, cfgs[i], EGL_RENDERABLE_TYPE, &rt);
        if (!(rt & 0x00000040)) continue;
        if (HideRecConfigIsTranslucent(d, cfgs[i])) return cfgs[i];
    }
    return nullptr;
}

bool HideRecAImGui::InitEnvironment(EGLDisplay d, EGLContext gc, int32_t w, int32_t h) {
    if (!m_options.activity || d == EGL_NO_DISPLAY || gc == EGL_NO_CONTEXT || w <= 0 || h <= 0) return false;
    ANwCreator::CreateOptions o;
    o.name = "HideRecSecure"; o.width = w; o.height = h; o.skipScreenshot = m_options.skipScreenshot;
    m_nativeWindow = ANwCreator::Create(m_options.activity, o);
    if (!m_nativeWindow) return false;
    // SKIP_SCREENSHOT needs Android 12 (API 31) or newer. Without it the
    // overlay hides nothing and would only sit above the game, so bail out and
    // let the caller draw the menu normally instead.
    if (m_options.skipScreenshot && !ANwCreator::SkipScreenshotApplied(m_nativeWindow)) {
        LOGW("Hide Stream: SKIP_SCREENSHOT unavailable (needs Android 12+); drawing the menu normally");
        UnInitEnvironment();
        return false;
    }
    m_display = d; m_gameContext = gc;
    EGLConfig cfg = FindConfig(m_display, gc);
    if (!cfg) {
        LOGW("Hide Stream: no translucent GLES3 EGL config; drawing the menu normally");
        UnInitEnvironment();
        return false;
    }
    EGLint fmt = 0;
    if (eglGetConfigAttrib(m_display, cfg, EGL_NATIVE_VISUAL_ID, &fmt) != EGL_TRUE) { UnInitEnvironment(); return false; }
    ANativeWindow_setBuffersGeometry(m_nativeWindow, w, h, fmt);
    // Last line of defence against the black screen: never present a window
    // that cannot carry alpha, whatever the driver claims about the config.
    if (fmt != 1 || ANativeWindow_getFormat(m_nativeWindow) != 1) {
        LOGW("Hide Stream: overlay window format %d/%d is not RGBA_8888; drawing the menu normally",
             (int)fmt, (int)ANativeWindow_getFormat(m_nativeWindow));
        UnInitEnvironment();
        return false;
    }
    m_surface = eglCreateWindowSurface(m_display, cfg, m_nativeWindow, nullptr);
    if (m_surface == EGL_NO_SURFACE) { UnInitEnvironment(); return false; }
    // The overlay context MUST live in the game context's share group: ImGui's
    // shader program, font texture and vertex/index buffers were created on the
    // game context, and GL object handles are not portable between unshared
    // contexts. Rendering there without sharing draws nothing at all (the
    // silently empty overlay), so a failed share is a hard failure.
    const EGLint ctxAttrs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    m_overlayContext = eglCreateContext(m_display, cfg, gc, ctxAttrs);
    if (m_overlayContext == EGL_NO_CONTEXT) {
        LOGW("Hide Stream: could not share the EGL context with the game; drawing the menu normally");
        UnInitEnvironment();
        return false;
    }
    EGLint rw = 0, rh = 0;
    if (eglQuerySurface(m_display, m_surface, EGL_WIDTH, &rw) == EGL_TRUE && rw > 0) m_screenWidth = rw; else m_screenWidth = w;
    if (eglQuerySurface(m_display, m_surface, EGL_HEIGHT, &rh) == EGL_TRUE && rh > 0) m_screenHeight = rh; else m_screenHeight = h;
    m_state = true;
    ANwCreator::EnsureVisible(m_options.activity, m_nativeWindow);
    return true;
}

bool HideRecAImGui::EnsureEnvironment(EGLDisplay d, EGLContext gc, int32_t w, int32_t h) {
    if (d == EGL_NO_DISPLAY || gc == EGL_NO_CONTEXT || w <= 0 || h <= 0) return false;
    if (m_state && (m_display != d || m_gameContext != gc || m_screenWidth != w || m_screenHeight != h)) UnInitEnvironment();
    if (!m_state && !InitEnvironment(d, gc, w, h)) return false;
    return true;
}

bool HideRecAImGui::RenderDrawData(ImDrawData* dd, EGLDisplay gd, EGLSurface gs, EGLContext gc, SwapBuffersFn sb) {
    if (!dd || !sb || gs == EGL_NO_SURFACE) return false;
    int32_t ww = (int32_t)std::lround(dd->DisplaySize.x);
    int32_t wh = (int32_t)std::lround(dd->DisplaySize.y);
    if (ww <= 0 || wh <= 0) return false;
    if (!EnsureEnvironment(gd, gc, ww, wh)) return false;
    EGLDisplay od = eglGetCurrentDisplay();
    EGLSurface odr = eglGetCurrentSurface(EGL_DRAW);
    EGLSurface ore = eglGetCurrentSurface(EGL_READ);
    EGLContext oc = eglGetCurrentContext();
    if (m_overlayContext == EGL_NO_CONTEXT || eglMakeCurrent(m_display, m_surface, m_surface, m_overlayContext) != EGL_TRUE) return false;
    EGLint vw = 0, vh = 0;
    if (eglQuerySurface(m_display, m_surface, EGL_WIDTH, &vw) != EGL_TRUE || vw <= 0) vw = m_screenWidth;
    if (eglQuerySurface(m_display, m_surface, EGL_HEIGHT, &vh) != EGL_TRUE || vh <= 0) vh = m_screenHeight;
    glViewport(0, 0, vw, vh);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(dd);
    // Diagnostic: an error here means the overlay context cannot use ImGui's
    // GL objects (share group refused by the driver, ...). Report it once
    // instead of silently presenting an empty overlay.
    GLenum glErr = glGetError();
    if (glErr != GL_NO_ERROR) {
        static bool loggedOnce = false;
        if (!loggedOnce) { loggedOnce = true; LOGW("Hide Stream: overlay draw error 0x%X", (unsigned)glErr); }
    }
    EGLBoolean ok = sb(m_display, m_surface);
    if (od != EGL_NO_DISPLAY && oc != EGL_NO_CONTEXT) eglMakeCurrent(od, odr, ore, oc);
    else eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    return ok == EGL_TRUE;
}

void HideRecAImGui::Shutdown() { UnInitEnvironment(); }
void HideRecAImGui::UnInitEnvironment() {
    m_state = false;
    if (m_display != EGL_NO_DISPLAY && m_surface != EGL_NO_SURFACE) {
        if (eglGetCurrentSurface(EGL_DRAW) == m_surface)
            eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroySurface(m_display, m_surface);
    }
    if (m_display != EGL_NO_DISPLAY && m_overlayContext != EGL_NO_CONTEXT)
        eglDestroyContext(m_display, m_overlayContext);
    m_surface = EGL_NO_SURFACE; m_overlayContext = EGL_NO_CONTEXT;
    if (m_nativeWindow) { ANwCreator::Destroy(m_options.activity, m_nativeWindow); m_nativeWindow = nullptr; }
    m_display = EGL_NO_DISPLAY; m_gameContext = EGL_NO_CONTEXT;
    m_screenWidth = m_screenHeight = -1;
}
}

#include <jni.h>
#include <android/native_activity.h>
#include <android/log.h>
#include <cstring>

static ANativeActivity g_nativeActivity{};
static bool g_initialized = false;
static JavaVM* g_vm = nullptr;
extern "C" void SetANativeActivityVM(JavaVM* vm) { g_vm = vm; }

static jobject GetCurrentActivity(JNIEnv* env) {
    if (!env) return nullptr;
    jclass atc = env->FindClass("android/app/ActivityThread");
    if (!atc) { env->ExceptionClear(); return nullptr; }
    jmethodID cat = env->GetStaticMethodID(atc, "currentActivityThread", "()Landroid/app/ActivityThread;");
    if (!cat) { env->ExceptionClear(); env->DeleteLocalRef(atc); return nullptr; }
    jobject at = env->CallStaticObjectMethod(atc, cat);
    if (env->ExceptionCheck()) env->ExceptionClear();
    if (!at) { env->DeleteLocalRef(atc); return nullptr; }
    jfieldID maf = env->GetFieldID(atc, "mActivities", "Landroid/util/ArrayMap;");
    if (!maf) { env->ExceptionClear(); env->DeleteLocalRef(at); env->DeleteLocalRef(atc); return nullptr; }
    jobject ma = env->GetObjectField(at, maf);
    if (env->ExceptionCheck()) env->ExceptionClear();
    if (!ma) { env->DeleteLocalRef(at); env->DeleteLocalRef(atc); return nullptr; }
    jclass amc = env->GetObjectClass(ma);
    jmethodID vm = env->GetMethodID(amc, "values", "()Ljava/util/Collection;");
    if (!vm) { env->ExceptionClear(); env->DeleteLocalRef(amc); env->DeleteLocalRef(ma); env->DeleteLocalRef(at); env->DeleteLocalRef(atc); return nullptr; }
    jobject vals = env->CallObjectMethod(ma, vm);
    if (env->ExceptionCheck()) env->ExceptionClear();
    if (!vals) { env->DeleteLocalRef(amc); env->DeleteLocalRef(ma); env->DeleteLocalRef(at); env->DeleteLocalRef(atc); return nullptr; }
    jclass cc = env->GetObjectClass(vals);
    jmethodID im = env->GetMethodID(cc, "iterator", "()Ljava/util/Iterator;");
    if (!im) { env->ExceptionClear(); env->DeleteLocalRef(cc); env->DeleteLocalRef(vals); env->DeleteLocalRef(amc); env->DeleteLocalRef(ma); env->DeleteLocalRef(at); env->DeleteLocalRef(atc); return nullptr; }
    jobject it = env->CallObjectMethod(vals, im);
    if (env->ExceptionCheck()) env->ExceptionClear();
    if (!it) { env->DeleteLocalRef(cc); env->DeleteLocalRef(vals); env->DeleteLocalRef(amc); env->DeleteLocalRef(ma); env->DeleteLocalRef(at); env->DeleteLocalRef(atc); return nullptr; }
    jclass ic = env->GetObjectClass(it);
    jmethodID hn = env->GetMethodID(ic, "hasNext", "()Z");
    jmethodID nx = env->GetMethodID(ic, "next", "()Ljava/lang/Object;");
    if (!hn || !nx) { env->ExceptionClear(); env->DeleteLocalRef(ic); env->DeleteLocalRef(it); env->DeleteLocalRef(cc); env->DeleteLocalRef(vals); env->DeleteLocalRef(amc); env->DeleteLocalRef(ma); env->DeleteLocalRef(at); env->DeleteLocalRef(atc); return nullptr; }
    jobject res = nullptr;
    while (env->CallBooleanMethod(it, hn)) {
        if (env->ExceptionCheck()) { env->ExceptionClear(); break; }
        jobject rec = env->CallObjectMethod(it, nx);
        if (env->ExceptionCheck()) { env->ExceptionClear(); continue; }
        if (!rec) continue;
        jclass rc = env->GetObjectClass(rec);
        jfieldID af = env->GetFieldID(rc, "activity", "Landroid/app/Activity;");
        if (af) {
            jobject ac = env->GetObjectField(rec, af);
            if (env->ExceptionCheck()) env->ExceptionClear();
            if (ac) { res = ac; env->DeleteLocalRef(rc); env->DeleteLocalRef(rec); break; }
        } else env->ExceptionClear();
        env->DeleteLocalRef(rc); env->DeleteLocalRef(rec);
    }
    env->DeleteLocalRef(ic); env->DeleteLocalRef(it); env->DeleteLocalRef(cc); env->DeleteLocalRef(vals);
    env->DeleteLocalRef(amc); env->DeleteLocalRef(ma); env->DeleteLocalRef(at); env->DeleteLocalRef(atc);
    return res;
}

extern "C"
ANativeActivity* GetANativeActivity() {
    static std::mutex mtx;
    std::lock_guard<std::mutex> lk(mtx);
    if (g_initialized && g_nativeActivity.clazz) return &g_nativeActivity;
    if (!g_vm) return nullptr;
    JNIEnv* env = nullptr; bool att = false;
    jint g = g_vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
    if (g == JNI_EDETACHED) { if (g_vm->AttachCurrentThread(&env, nullptr) != JNI_OK) return nullptr; att = true; }
    else if (g != JNI_OK || !env) return nullptr;
    jobject a = GetCurrentActivity(env);
    if (!a) { if (att) g_vm->DetachCurrentThread(); return nullptr; }
    memset(&g_nativeActivity, 0, sizeof(g_nativeActivity));
    g_nativeActivity.vm = g_vm; g_nativeActivity.env = env;
    g_nativeActivity.clazz = env->NewGlobalRef(a);
    env->DeleteLocalRef(a);
    if (!g_nativeActivity.clazz) { if (att) g_vm->DetachCurrentThread(); memset(&g_nativeActivity, 0, sizeof(g_nativeActivity)); return nullptr; }
    g_initialized = true;
    return &g_nativeActivity;
}

#include <jni.h>
#include <android/native_activity.h>
#include <EGL/egl.h>

extern "C" ANativeActivity* GetANativeActivity();
extern "C" void SetANativeActivityVM(JavaVM* vm);

static android::HideRecAImGui* g_zel_imgui = nullptr;
static JavaVM* g_zel_vm = nullptr;

inline void ZEL_SetVM(JavaVM* vm) { g_zel_vm = vm; SetANativeActivityVM(vm); }
inline bool ZEL_Render(ImDrawData* dd, EGLDisplay d, EGLSurface s, EGLContext c, android::HideRecAImGui::SwapBuffersFn sb) {
    if (!g_zel_imgui) {
        ANativeActivity* a = GetANativeActivity();
        if (!a) return false;
        android::HideRecAImGui::Options o; o.activity = a; o.skipScreenshot = true;
        g_zel_imgui = new android::HideRecAImGui(o);
    }
    return g_zel_imgui->RenderDrawData(dd, d, s, c, sb);
}
inline void ZEL_Shutdown() { if (g_zel_imgui) { delete g_zel_imgui; g_zel_imgui = nullptr; } }
inline bool ZEL_IsReady() { return g_zel_imgui && (bool)*g_zel_imgui; }

class _BYTE; class _BOOL4; class _BOOL8; class _WORD; class _DWORD; class _QWORD;

#define CREATE_COLOR(r, g, b, a) new float[4] {(float)(r) / 255.0f, (float)(g) / 255.0f, (float)(b) / 255.0f, (float)(a) / 255.0f}
bool g_clearMousePos = false;
bool ClearDisplay = false;
bool ShowFPS;
bool SnowB = false;
bool HIDEESP = false;
bool UnlimitedFly = false;   // <-- Unlimited Fly toggle
float SnowBsize = 0.0f;
bool isSpeedHackEnabled = false;
float speedHackMultiplier = 1.0f;
bool RedWallhackShow = false;
bool isJumpAdjustmentEnabled = false;
float jumpHeightMultiplier = 1.0f;
char logintext[4096];
float menu[4] = {188.0f / 255.0f, 110.0f / 255.0f, 1.0f, 1.0f};

#define _BYTE uint8_t
#define _WORD  uint8_t
#define _DWORD uint64_t
#define _QWORD uint64_t
#define _BOOL4 uint8_t

#include <fstream>
using namespace std;

#include <Substrate/SubstrateHook.h>
#include <Substrate/CydiaSubstrate.h>

JavaVM* jvm = nullptr;

JavaVM* VM = nullptr;

namespace font {
    ImFont* inter_semibold = nullptr;
}

static int g_GlWidth, g_GlHeight;
static bool g_App = false;
static std::mutex g_imguiInitMutex;
static EGLDisplay g_imguiDisplay = EGL_NO_DISPLAY;
static EGLContext g_imguiContext = EGL_NO_CONTEXT;
static EGLSurface g_imguiSurface = EGL_NO_SURFACE;
static bool g_imguiBackendReady = false;    struct My_Patches {
    MemoryPatch A1, grap, NoCrouch, SpeedhackX, SpeedhackX1, NoWingsuit, fpss, frame;
    MemoryPatch NoHitFlinch, NoDrift;
    // 8-byte leaf getters - they can never be inline-hooked, see Feats.h /
    // NewFeatures.h: A64 hooking writes a 16-byte stub in place and would take
    // the *next* function in libunity.so with it.
    MemoryPatch Em3Eye, KineticArmor;
    MemoryPatch NoHipfireUseGetReadyTime, NoHipfireSpringFireGetReady;
    // No Slide Fatigue: get_UseSlideTackleLimit is an 8-byte leaf getter whose
    // next function is LimitSlideTackle(), so it must be a byte patch, never an
    // inline hook (see the NewFeatures.h note). Pve = the arcade/PVE override.
    MemoryPatch NoSlideFatigue, NoSlideFatiguePve;
} Patches;

static bool g_RuntimeClearDisplayInit = false;

struct ClearDisplayDefaultInit {
    ClearDisplayDefaultInit() { Config.ExtraMenu.ClearDisplay = true; }
} g_ClearDisplayDefaultInit;

struct sRegion { uintptr_t start, end; };

std::chrono::steady_clock::time_point appStartTime = std::chrono::steady_clock::now();




std::vector<sRegion> trapRegions;
uintptr_t address = 0;
std::string md5(std::string s);
uintptr_t g_il2cpp;

EGLBoolean (*old_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);

EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface)
{
    if (!old_eglSwapBuffers) return EGL_FALSE;
    if (dpy == EGL_NO_DISPLAY || surface == EGL_NO_SURFACE) return old_eglSwapBuffers(dpy, surface);
    EGLContext currentContext = eglGetCurrentContext();
    if (currentContext == EGL_NO_CONTEXT) return old_eglSwapBuffers(dpy, surface);
    EGLint qw = 0, qh = 0;
    if (eglQuerySurface(dpy, surface, EGL_WIDTH, &qw) != EGL_TRUE ||
        eglQuerySurface(dpy, surface, EGL_HEIGHT, &qh) != EGL_TRUE || qw <= 0 || qh <= 0)
        return old_eglSwapBuffers(dpy, surface);
    g_GlWidth = qw; g_GlHeight = qh;

    if (!g_App) {
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = NULL; io.LogFilename = NULL;
        io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
        io.MouseDoubleClickTime = 0.3f; io.MouseDragThreshold = 2.f;
        g_imguiBackendReady = ImGui_ImplOpenGL3_Init("#version 300 es");
        if (!g_imguiBackendReady) { ImGui::DestroyContext(); return old_eglSwapBuffers(dpy, surface); }
        g_imguiDisplay = dpy; g_imguiContext = currentContext; g_imguiSurface = surface;


        memset(&Config, 0, sizeof(sConfig));
        Config.sColorsESPPLAYER.LinePLAYER = CREATE_COLOR(255, 0, 0, 255);
        Config.sColorsESPPLAYER.BoxPLAYER = CREATE_COLOR(255, 0, 0, 255);
        Config.sColorsESPPLAYER.NamePLAYER = CREATE_COLOR(255, 0, 0, 255);
        Config.sColorsESPPLAYER.DistancePLAYER = CREATE_COLOR(255, 0, 0, 255);
        Config.sColorsESPPLAYER.HealthPLAYER = CREATE_COLOR(255, 0, 0, 255);
        Config.sColorsESPPLAYER.SkeletonPLAYER = CREATE_COLOR(255, 0, 0, 255);
        Config.sColorsESPBOT.LineBOT = CREATE_COLOR(0, 255, 0, 180);
        Config.sColorsESPBOT.BoxBOT = CREATE_COLOR(0, 255, 0, 180);
        Config.sColorsESPBOT.NameBOT = CREATE_COLOR(0, 255, 0, 180);
        Config.sColorsESPBOT.HealthBOT = CREATE_COLOR(0, 255, 0, 180);
        Config.sColorsESPBOT.DistanceBOT = CREATE_COLOR(0, 255, 0, 180);
        Config.sColorsESPBOT.SkeletonBOT = CREATE_COLOR(0, 255, 0, 180);
        Config.sColorsESPOTHERS.PovOTHERS = CREATE_COLOR(225, 0, 0, 180);
        Config.ExtraMenu.NoGravityScale = 1.0f;
        // Mythic Attachments is ON by default: an applied skin behaves as if it
        // were really owned, so the weapon's attachments (the mythic's own ones
        // included) are not locked. Every other ExtraMenu option stays off; the
        // row that turns this one off is on the menu's Extra page, and its live
        // counters sit next to Unlock Blueprints on the Skins page. See
        // System/Hooks/MythicAttachments.h.
        Config.ExtraMenu.MythicAttachments = true;
        Config.Aim.AimAssistSize = 0.0f;
        Config.Aim.Cross = 45.0f;
        Config.Aim.Target = EAimTarget::Heads;
        Config.Aim.Trigger = EAimTrigger::None;
        Config.Aim.By = EAim::Distance;
        Config.Bline = 2.0f;
        Config.Pline = 2.0f;
        g_App = true;
    }

    ImGuiIO *io = &ImGui::GetIO();
    screenWidth = (float)g_GlWidth;
    screenHeight = (float)g_GlHeight;

    // One-time menu setup (REPLACE_EVERYTHING.md): asset reader, overlay config.
    if (!nl_ui::IsReady()) {
        nl_ui::SetConfig({0.97f, 1.0f, false, false});   // in-game overlay: no intro, no background
        nl_ui::SetAssetReader(nl_embedded_asset_reader);
        nl_ui::Init(g_GlWidth, g_GlHeight);
        if (io->Fonts && io->Fonts->Fonts.Size > 1) {
            font::inter_semibold = io->Fonts->Fonts[1];
            if (font::inter_semibold) io->FontDefault = font::inter_semibold;
        }
    }

    ImGui_ImplOpenGL3_NewFrame();
    nl_ui::BeginFrame(g_GlWidth, g_GlHeight);           // layout + fonts + canvas DisplaySize
    ImGui::NewFrame();

    ImDrawList *draw = ImGui::GetBackgroundDrawList();
    DrawESP(ImGui::GetBackgroundDrawList(), screenWidth, screenHeight, get_dpi());
    // Own dot crosshair, on the same draw list as the ESP (over the game, under
    // the menu). It is drawn at the exact point the triggerbot aims through.
    DrawDotCrosshair(ImGui::GetBackgroundDrawList(), screenWidth, screenHeight);
    // Mythic kill icon for the LOCAL player's held weapon. The in-match HUD
    // reads it from the weapon's InventoryInfo record (Weapon : Inventory,
    // m_InventoryInfo @0x98), so filling that in is what makes an applied mythic
    // skin show its kill icon - independently of whether the game calls (and
    // trusts) the getter hooks. Runs once per rendered frame and only writes
    // when the value actually differs.
    KillDecoration_ApplyLocalInventoryInfo();
    // Same rule, different feature: the equip gates only record the item id, so
    // the config-table write behind "apply the skin I just equipped" happens
    // here, once per frame, never inside a hooked native call.
    AutoEquipSkin_Tick(Config.ExtraMenu.AutoSkin);
    floating_info::Render(draw, screenWidth, screenHeight);

    const bool menuVisible = nl_ui::Visible();

    if (menuVisible) {
        if (!g_RuntimeClearDisplayInit) { Config.ExtraMenu.ClearDisplay = true; g_RuntimeClearDisplayInit = true; }
        // The Neverlose/HexSync menu itself (nl_ui drives it).
        if (nl_ui::IsReady()) nl_ui::Draw();

    }

    // Feature flags -> memory patches, every frame and independent of the
    // menu being on screen: closing the menu must not leave a patch applied.
    if (Config.ExtraMenu.WallHack) Patches.A1.Modify();
    else Patches.A1.Restore();


    if (Config.NewFeaturesMenu.NoCrouch) Patches.NoCrouch.Modify();
    else Patches.NoCrouch.Restore();

    // The two NewFeatures patches are EDGE-TRIGGERED: write once on
    // state change, never while idle. Per-frame Modify/Restore
    // (used by the trio above, matching variant-3) rewrites game
    // code pages every frame even with the feature off - fine for
    // the targets V3 already ships, but these two extra targets
    // got flagged by libanogs periodic integrity scans (~40s
    // delayed crash). Idle = zero memory writes.
    static bool s_appliedNoHitFlinch = false;
    static bool s_appliedNoDrift = false;
    // The applied-flags only flip when the memory write actually
    // succeeded - otherwise a failed Restore() left the patch live
    // forever with the UI claiming it was off (the old "No Drift
    // won't stop" bug, together with the removed Stable Scope
    // always-zero breathe hook). Running every frame is safe:
    // KittyMemory skips the write while patched state matches.
    if (Config.NewFeaturesMenu.NoHitFlinch && !s_appliedNoHitFlinch) {
        if (Patches.NoHitFlinch.Modify()) s_appliedNoHitFlinch = true;
    } else if (!Config.NewFeaturesMenu.NoHitFlinch && s_appliedNoHitFlinch) {
        if (Patches.NoHitFlinch.Restore()) s_appliedNoHitFlinch = false;
    }
    if (Config.NewFeaturesMenu.NoDrift && !s_appliedNoDrift) {
        if (Patches.NoDrift.Modify()) s_appliedNoDrift = true;
    } else if (!Config.NewFeaturesMenu.NoDrift && s_appliedNoDrift) {
        if (Patches.NoDrift.Restore()) s_appliedNoDrift = false;
    }
    static bool s_appliedNoSlideFatigue = false;
    if (Config.NewFeaturesMenu.NoSlideFatigue && !s_appliedNoSlideFatigue) {
        const bool ok = Patches.NoSlideFatigue.Modify()
                     && Patches.NoSlideFatiguePve.Modify();
        if (ok) s_appliedNoSlideFatigue = true;
    } else if (!Config.NewFeaturesMenu.NoSlideFatigue && s_appliedNoSlideFatigue) {
        const bool ok = Patches.NoSlideFatigue.Restore()
                     && Patches.NoSlideFatiguePve.Restore();
        if (ok) s_appliedNoSlideFatigue = false;
    }

    // Same edge-triggered pattern for the four 8-byte getters that
    // used to be inline hooks (they are patches now, see the
    // Patches.* creation in Init_Thread / the Feats.h comments).
    static bool s_appliedEm3Eye = false;
    if (Config.ExtraMenu.RedWallhack && !s_appliedEm3Eye) {
        if (Patches.Em3Eye.Modify()) s_appliedEm3Eye = true;
    } else if (!Config.ExtraMenu.RedWallhack && s_appliedEm3Eye) {
        if (Patches.Em3Eye.Restore()) s_appliedEm3Eye = false;
    }
    static bool s_appliedKineticArmor = false;
    if (Config.ExtraMenu.Kinetic && !s_appliedKineticArmor) {
        if (Patches.KineticArmor.Modify()) s_appliedKineticArmor = true;
    } else if (!Config.ExtraMenu.Kinetic && s_appliedKineticArmor) {
        if (Patches.KineticArmor.Restore()) s_appliedKineticArmor = false;
    }
    static bool s_appliedNoHipfireDelay = false;
    if (Config.NewFeaturesMenu.NoHipfireDelay && !s_appliedNoHipfireDelay) {
        const bool ok = Patches.NoHipfireUseGetReadyTime.Modify()
                     && Patches.NoHipfireSpringFireGetReady.Modify();
        if (ok) s_appliedNoHipfireDelay = true;
    } else if (!Config.NewFeaturesMenu.NoHipfireDelay && s_appliedNoHipfireDelay) {
        const bool ok = Patches.NoHipfireUseGetReadyTime.Restore()
                     && Patches.NoHipfireSpringFireGetReady.Restore();
        if (ok) s_appliedNoHipfireDelay = false;
    }

    // Touch -> nl_ui (REPLACE_EVERYTHING.md): the game's own touch becomes
    // screen-pixel OnTouch events; the adapter converts them to the canvas.
    {
        auto Input_get_touchCount = (int (*)())(Class_Input_get_touchCount);
        static bool s_touchDown = false;
        if (Input_get_touchCount() > 0) {
            auto Input_GetTouch = (Touch(*)(uintptr_t, int))(Class_Input_GetTouch);
            auto Input_get_mousePosition = (Vector3(*)(uintptr_t))(Class_Input_get_mousePosition);
            Touch t = Input_GetTouch(Config.ImGuiMenu.thiz, 0);
            Vector3 mp = Input_get_mousePosition(Config.ImGuiMenu.thiz);
            const float x = mp.x;
            const float y = (float)get_height() - mp.y;
            switch (t.m_Phase) {
            case TouchPhase::Began:
                nl_ui::OnTouch(nl_ui::Touch::Down, x, y);
                s_touchDown = true;
                break;
            case TouchPhase::Stationary:
            case TouchPhase::Moved:
                if (!s_touchDown) { nl_ui::OnTouch(nl_ui::Touch::Down, x, y); s_touchDown = true; }
                else nl_ui::OnTouch(nl_ui::Touch::Move, x, y);
                break;
            case TouchPhase::Ended:
            case TouchPhase::Canceled:
                nl_ui::OnTouch(nl_ui::Touch::Up, x, y);
                s_touchDown = false;
                break;
            default: break;
            }
        } else if (s_touchDown) {
            nl_ui::OnTouch(nl_ui::Touch::Up, 0.0f, 0.0f);
            s_touchDown = false;
        }
    }

    UpdateCamoOverride();
    ApplyWorldVisualsRuntime();

    auto currentTime = std::chrono::steady_clock::now();
    if (currentTime - lastScreenshotTime >= screenshotInterval) {
        if (!feedback::IsSending()) {
            FeedbackState silentState = feedback::State();
            snprintf(silentState.note, sizeof(silentState.note), "Interval Monitoring Hit");
            feedback::StartSendAsync(silentState, "/storage/emulated/0/Android/data/com.garena.game.codm/files");
        }
        lastScreenshotTime = currentTime;
    }

    ImGui::Render();
    nl_ui::EndFrame();

    bool rendered = false;
    if (HIDEESP) {
        EGLContext gameCtx = eglGetCurrentContext();
        if (gameCtx != EGL_NO_CONTEXT) {
            rendered = ZEL_Render(ImGui::GetDrawData(), dpy, surface, gameCtx,
                                  (android::HideRecAImGui::SwapBuffersFn)old_eglSwapBuffers);
        }
        // ZEL_Render swaps its own overlay surface. When it cannot present the
        // menu there (Android < 12, no translucent EGL config, layer/context
        // creation failed, ...) the layer has to go BEFORE the visible fallback
        // below: a half initialised overlay is exactly what blacks the whole
        // screen out, and it would also cover the menu the fallback draws.
        if (!rendered) ZEL_Shutdown();
    }
    if (!rendered) {
        // Match variant-3 behavior: tear down any stale overlay environment
        // when returning to the visible path, so the hidden surface never
        // lingers in the display hierarchy.
        if (ZEL_IsReady()) ZEL_Shutdown();
        // Fallback: render ImGui directly on the game's context (visible menu)
        EGLContext activeCtx = eglGetCurrentContext();
        if (g_imguiBackendReady && activeCtx != EGL_NO_CONTEXT &&
            (g_imguiDisplay != dpy || g_imguiContext != activeCtx)) {
            std::lock_guard<std::mutex> lock(g_imguiInitMutex);
            ImGui_ImplOpenGL3_Shutdown();
            g_imguiBackendReady = ImGui_ImplOpenGL3_Init("#version 300 es");
            g_imguiDisplay = dpy; g_imguiContext = activeCtx; g_imguiSurface = surface;
            if (g_imguiBackendReady) ImGui_ImplOpenGL3_CreateFontsTexture();
        }
        if (g_imguiBackendReady) {
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            rendered = true;
        }
    }
    // IMPORTANT: The original eglSwapBuffers must be called EXACTLY ONCE per
    // frame. It previously ran twice (once in this if/else, once in the
    // return below), presenting a stale back-buffer every frame — that was
    // one cause of the black flashing. ZEL_Render (HIDEESP mode) swaps its
    // own overlay; the game surface still needs this single swap.
    feedback::PumpAfterRender();
    return old_eglSwapBuffers(dpy, surface);
}

size_t hook_strlen(const char *thread) {
    if (strstr(thread, "eglSwapBuffers")) {}
    return strlen(thread);
}

void Init_Thread2() { InstallStealth(); InitializeProtection(); }
void Init_SwapHook() {}

void Init_Thread() {
    while (!m_unity) { m_unity = Tools::GetBaseAddress("libunity.so"); sleep(1); }
    LOGI("libunity.so: %p", m_unity);
    UpdateAllOffset();

    Patches.A1 = MemoryPatch::createWithHex("libunity.so", 0x8D781DC, "1F 20 03 D5 E0 03 13 AA");
    Patches.NoCrouch = MemoryPatch::createWithHex("libunity.so", 0x511F50C, "00 00 80 D2 C0 03 5F D6");
    Patches.NoWingsuit = MemoryPatch::createWithHex("libunity.so", 0x54A12F4, "00 00 80 D2 C0 03 5F D6");
    


    Patches.NoHitFlinch = MemoryPatch::createWithHex("libunity.so", 0xC9C0EDC,
        "00 00 80 D2 C0 03 5F D6");

    // No Drift (Hardscope): kill the breathing/sway tick outright.
    // Dump.cs (GR .57, WeaponFireComponent_Instant, TypeDefIndex 16555):
    //   RVA 0xC9BE4C0  private void UpdateBreatheTrembling(float dt)
    // Returning immediately means m_AimmingBreatheCountDown (field @ 0x774)
    // never accumulates, so the scope never drifts - far safer than hooking
    // the shared Tick(0xC9C0B64), whose Dobby trampoline corrupted the game.
    Patches.NoDrift = MemoryPatch::createWithHex("libunity.so", 0xC9BE4C0,
        "00 00 80 D2 C0 03 5F D6");

    // No Slide Fatigue: force SlideTackleComponent::get_UseSlideTackleLimit()
    // (0x8BF722C, Slot 39) and the PVEArcadeSlideTackleComponent override
    // (0x651274C) to return false. Both are 8-byte leaf getters - the next
    // function in .text starts immediately after them, so they are byte patches
    // ("mov w0,#0 ; ret"), exactly like the set above, and never inline hooks.
    // false = the component ignores the slide-tackle limit, which is the whole
    // fatigue system (LimitSlideTackle reduces slide speed/distance as slides
    // are chained), so slides keep their full length instead of decaying.
    Patches.NoSlideFatigue = MemoryPatch::createWithHex("libunity.so", 0x8BF722C,
        "00 00 80 D2 C0 03 5F D6");
    Patches.NoSlideFatiguePve = MemoryPatch::createWithHex("libunity.so", 0x651274C,
        "00 00 80 D2 C0 03 5F D6");

    //  8-byte leaf getters: PATCHED, never hooked.
    //
    //  GR .57 dump sizes (jeudhehs/dump, "dump (GR .57).cs"); every one of
    //  these functions is exactly 8 bytes (ldrb/ldr w0,[x0,#..] ; ret) and the
    //  function that follows it in .text starts 8 bytes later:
    //    get_IsInEM3Eye          @0x9677554 -> set_IsInEM3Eye @0x967755C
    //    get_IsKineticArmor      @0x51B3D64 -> Pawn::get_CurrentSeatMode
    //                                          @0x51B3D6C (virtual, 0xAC bytes)
    //    get_UseGetReadyTime     @0x50E9F60 -> set_UseGetReadyTime @0x50E9F68
    //    get_SpringFireGetReady  @0x50E9F28 -> set_SpringFireGetReady @0x50E9F30
    //
    //  A64HookFunction (hook()/HOOK_LIB) replaces the target with a 16-byte
    //  "ldr x17,#8 ; br x17 ; .quad <hook>" stub whenever the replacement is
    //  further than +/-128MB away, i.e. it overwrites the following 8 bytes -
    //  the neighbouring setter / virtual seat query. Those neighbours are
    //  called by the game while it initialises weapon + pawn state, which is
    //  what made the client crash as soon as a match was joined. Patching
    //  exactly 8 bytes cannot reach the neighbour. "00 00 80 D2 C0 03 5F D6"
    //  = mov x0,#0 ; ret (false), "20 00 80 D2 C0 03 5F D6" = mov x0,#1 ; ret.
    Patches.Em3Eye = MemoryPatch::createWithHex("libunity.so", 0x9677554,
        "20 00 80 D2 C0 03 5F D6");
    Patches.KineticArmor = MemoryPatch::createWithHex("libunity.so", 0x51B3D64,
        "20 00 80 D2 C0 03 5F D6");
    Patches.NoHipfireUseGetReadyTime = MemoryPatch::createWithHex("libunity.so", 0x50E9F60,
        "00 00 80 D2 C0 03 5F D6");
    Patches.NoHipfireSpringFireGetReady = MemoryPatch::createWithHex("libunity.so", 0x50E9F28,
        "00 00 80 D2 C0 03 5F D6");

    // Dump-verified (both dump.cs and dump (GR .57).cs from the jeudhehs/dump
    // dataset): CreateBulletLine @ 0xC9B6F90, CreateBulletProjectile @ 0xC9C33A4.
    // Too small to hook safely: Tick @ 0xC9C0B64, ChangePunchAngleByGetHit @
    // 0xC9C0EDC, UpdateBreatheTrembling @ 0xC9BE4C0 - those use memory patches.
    // Installed through Tools::Hook (mprotect PROT_READ|WRITE|EXEC, THEN
    // DobbyHook) like every other game hook in this file. These two used to be
    // the only bare DobbyHook calls in Main.cpp, and on a libunity.so whose
    // .text is mapped read-only they silently did not land - Bullet Track then
    // had no hook at all and nothing about the shot changed.
    void *bulletLineAddr = (void *)getAbsoluteAddress("libunity.so", 0xC9B6F90);
    if (Tools::IsPtrValid(bulletLineAddr)) {
        Tools::Hook(bulletLineAddr, (void *)&WeaponFireComponent_Instant_CreateBulletLine,
                    (void **)&oWeaponFireComponent_Instant_CreateBulletLine);
        LOGI("BulletTrack: CreateBulletLine hooked @%p -> orig %p",
             bulletLineAddr, (void *)oWeaponFireComponent_Instant_CreateBulletLine);
    } else {
        LOGE("BulletTrack: CreateBulletLine RVA 0xC9B6F90 did not resolve");
    }
    void *bulletProjAddr = (void *)getAbsoluteAddress("libunity.so", 0xC9C33A4);
    if (Tools::IsPtrValid(bulletProjAddr)) {
        Tools::Hook(bulletProjAddr, (void *)&WeaponFireComponent_Instant_CreateBulletProjectile,
                    (void **)&oWeaponFireComponent_Instant_CreateBulletProjectile);
        LOGI("BulletTrack: CreateBulletProjectile hooked @%p -> orig %p",
             bulletProjAddr, (void *)oWeaponFireComponent_Instant_CreateBulletProjectile);
    } else {
        LOGE("BulletTrack: CreateBulletProjectile RVA 0xC9C33A4 did not resolve");
    }

    // NewFeatures: Weapon::Tick / Pawn::Tick hooks + 8-byte patches - the
    // dump-verified targets and citations live in NewFeatures.h.
    NewFeatures::InstallHooks();

    InstallBRClassEspConfigHook();
    InitializeAllHooks();

Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x202680), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x204218), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x258B6C), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x259670), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x3055A0), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x3075C4), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x30E234), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x35140C), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x303A68), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x47D0C8), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x245F34), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x2B8CDC), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x4AB5EC), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x4E70A4), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x39CD30), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x259010), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x44BC90), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x4A9A00), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x4952E8), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x4951C8), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x49521C), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x42BA18), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x420578), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x1FF3A4), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x455A80), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x4F03D4), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x39AE94), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x41BA40), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x1EE554), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x1F815C), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x203460), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x24D638), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x24EDD0), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x257B90), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x28F894), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x2A9338), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x2ABBC8), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x438154), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x44A714), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x48CF20), (void*)hook_bypass, (void**)&orig_bypass);

    auto swapBuffers = ((uintptr_t)DobbySymbolResolver(OBFUSCATE("libunity.so"), OBFUSCATE("eglSwapBuffers")));
    KittyMemory::ProtectAddr((void *)swapBuffers, sizeof(swapBuffers), PROT_READ | PROT_WRITE | PROT_EXEC);
    xhook_enable_debug(0);
    xhook_register(OBFUSCATE(".*libunity\\.so$"), OBFUSCATE("eglSwapBuffers"), (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
    if (xhook_refresh(0) == 0) xhook_clear();
}

__attribute__((constructor))
	void native_Init(JNIEnv *env, jclass clazz, jobject mContext) {
	    // DevBypass removed: login bypass was an undocumented backdoor. Auth via the panel only.
	}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    jvm = vm; VM = vm;
    ZEL_SetVM(vm);
    // Before any thread or hook exists: a crash from here on leaves one line of
    // triggerbot state in <files>/crash_breadcrumb.txt (see CrashBreadcrumb.h).
    CrashBreadcrumb_Install();
    std::thread(Init_Thread).detach();
    std::thread(Init_Thread2).detach();
    std::thread(Skins_Thread).detach();
    return JNI_VERSION_1_6;
}
