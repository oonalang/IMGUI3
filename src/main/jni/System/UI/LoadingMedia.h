#pragma once

#include "imgui.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <atomic>

#include <sys/stat.h>
#include <sys/types.h>

#if defined(__ANDROID__)
#include <jni.h>
#include <pthread.h>
#include <unistd.h>
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
extern JavaVM* jvm;
#endif

#if defined(__has_include)
#if __has_include("../../Assets/LoadingAssets.h")
#include "../../Assets/LoadingAssets.h"
#define UI_MEDIA_ASSET_VIDEO 1
#endif
#endif

#ifndef UI_MEDIA_ASSET_VIDEO
#define UI_MEDIA_ASSET_VIDEO 0
#endif

#if defined(__has_include)
#if __has_include("../../Assets/LoginAssets.h")
#include "../../Assets/LoginAssets.h"
#define UI_MEDIA_ASSET_LOGIN 1
#endif
#endif
#ifndef UI_MEDIA_ASSET_LOGIN
#define UI_MEDIA_ASSET_LOGIN 0
#endif
#if !UI_MEDIA_ASSET_LOGIN
static inline const unsigned char ethnir_login_video[] = {0};
static inline const unsigned int ethnir_login_video_len = 0;
#endif

#if !UI_MEDIA_ASSET_VIDEO
static inline const unsigned char ethnir_loading_video[] = {0};
static inline const unsigned int  ethnir_loading_video_len = 0;
#endif

namespace ui_media
{

    enum class VideoKind : int { Login = 0, Loading = 1 };
    inline std::atomic<int> g_requestedVideo{static_cast<int>(VideoKind::Login)};
    inline std::atomic<int> g_activeVideo{-1};

    inline bool        g_inited    = false;
    inline ImTextureID g_videoTex  = nullptr;
    inline int         g_videoW    = 0;
    inline int         g_videoH    = 0;
    inline bool        g_videoLive = false;
    inline bool        g_audioLive = false;

#if !defined(__ANDROID__)
    // Host builds only. A test can pretend a video is playing so the loader can
    // be checked with one - the loader has to keep painting its field
    // underneath the video either way.
    inline ImTextureID g_hostVideoTex   = nullptr;
    inline int         g_hostVideoW     = 0;
    inline int         g_hostVideoH     = 0;
    inline bool        g_hostVideoPending = false;
#endif

    namespace detail
    {
        inline std::string CachePath(const char* name)
        {
            const char* dir = getenv("TMPDIR");
            if (dir == nullptr || dir[0] == '\0') dir = "/data/local/tmp";
            std::string path(dir);
            if (!path.empty() && path[path.size() - 1] != '/') path += '/';
            path += name;
            return path;
        }

        inline bool WriteAsset(const std::string& path, const unsigned char* data, unsigned size)
        {
            if (data == nullptr || size == 0) return false;
            FILE* probe = fopen(path.c_str(), "rb");
            if (probe != nullptr)
            {
                fseek(probe, 0, SEEK_END);
                const long have = ftell(probe);
                fclose(probe);
                if (have == (long)size) return true;
            }
            FILE* out = fopen(path.c_str(), "wb");
            if (out == nullptr) return false;
            const size_t wrote = fwrite(data, 1, size, out);
            fclose(out);
            return wrote == (size_t)size;
        }

        inline bool FileSize(const std::string& path, long* out)
        {
            FILE* f = fopen(path.c_str(), "rb");
            if (f == nullptr) return false;
            fseek(f, 0, SEEK_END);
            const long size = ftell(f);
            fclose(f);
            if (size <= 0) return false;
            if (out != nullptr) *out = size;
            return true;
        }

        inline std::string MediaRoot()
        {
            return "/storage/emulated/0/Android/data/com.garena.game.codm/files/ethnir/";
        }

        inline bool EnsureDir(const std::string& path)
        {
            struct stat st {};
            if (stat(path.c_str(), &st) == 0) return S_ISDIR(st.st_mode) != 0;
            return mkdir(path.c_str(), 0775) == 0;
        }

        inline std::string ResolveMedia(const char* const* names, int nameCount,
                                        const unsigned char* embedded, unsigned embeddedSize)
        {
            // Explicit HD overrides take precedence over the low-resolution
            // bundled clips. Older files can stay on disk as fallback.
            // Named overrides are deliberate to avoid arbitrary Download videos.
            if (nameCount > 0)
            {
                const std::string root = MediaRoot();
                const std::string overrideDirs[] = {root, "/sdcard/Download/"};
                for (const auto& dir : overrideDirs)
                    for (int n = 0; n < nameCount; ++n)
                    {
                        std::string base = names[n];
                        const size_t ext = base.rfind(".mp4");
                        if (ext == std::string::npos) continue;
                        base.resize(ext);
                        for (const char* suffix : {"_4k.mp4", "_hd.mp4"})
                        {
                            const std::string overridePath = dir + base + suffix;
                            if (FileSize(overridePath, nullptr)) return overridePath;
                        }
                    }
            }
            // Keep existing deployed media deterministic when HD overrides are
            // absent. Embedded contents only rewrite files with different size.
            if (embedded != nullptr && embeddedSize > 0 && nameCount > 0)
            {
                const std::string dir = MediaRoot();
                if (EnsureDir(dir))
                    WriteAsset(dir + names[0], embedded, embeddedSize);
            }

            std::string dirs[8];
            int dirCount = 0;
            dirs[dirCount++] = MediaRoot();
            dirs[dirCount++] = "/storage/emulated/0/Android/data/com.garena.game.codm/files/";
            dirs[dirCount++] = "/storage/emulated/0/Android/media/com.garena.game.codm/ethnir/";
            dirs[dirCount++] = "/sdcard/Android/data/com.garena.game.codm/files/ethnir/";
            dirs[dirCount++] = "/sdcard/Download/";
            dirs[dirCount++] = "/sdcard/Movies/";
            dirs[dirCount++] = CachePath("");
            dirs[dirCount++] = "/data/local/tmp/";

            for (int d = 0; d < dirCount; ++d)
                for (int n = 0; n < nameCount; ++n)
                {
                    const std::string candidate = dirs[d] + names[n];
                    if (FileSize(candidate, nullptr)) return candidate;
                }

            for (int n = 0; n < nameCount; ++n)
            {
                const std::string target = CachePath(names[n]);
                if (FileSize(target, nullptr)) return target;
                if (WriteAsset(target, embedded, embeddedSize)) return target;
            }
            return std::string();
        }
    }

#if defined(__ANDROID__)

    inline GLuint g_oesTextures[2] = {0, 0}; // login / loading, separate frame queues
    inline GLuint g_targetTex    = 0;
    inline GLuint g_fbo          = 0;
    inline GLuint g_program      = 0;
    inline GLuint g_vao          = 0;
    inline GLint  g_uTex         = -1;
    inline GLint  g_uMatrix      = -1;
    inline int    g_targetW      = 0;
    inline int    g_targetH      = 0;
    inline float  g_matrix[16]   = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    // 1080p intermediate gives HD replacements their original detail without
    // allocating a large 33 MB 4K FBO on lower-memory Android GPUs.
    // No artificial enlargement: 360p and 540p bundled files stay source-limited.
    static constexpr int kMaxVideoTexW = 1920;
    static constexpr int kMaxVideoTexH = 1080;

    inline jobject g_surfaceTextures[2] = {nullptr, nullptr};
    // Reused transform scratch: allocating one per frame was a per-frame JNI
    // allocation on the loader's hottest path.
    inline jfloatArray g_matrixArray = nullptr;
    inline jobject g_audioPlayer   = nullptr;

    // Set once the video has actually been drawn into the target texture. Until
    // then the texture holds nothing and must not be shown: drawing it is what
    // turned the screen black.
    inline bool g_targetReady = false;

    // The audio player is prepared on its own thread too: prepare() blocks,
    // and the loader starts the sound from the render thread.
    inline pthread_t    g_audioWorker  = 0;
    inline volatile int g_audioState   = 0;      // 0 idle, 1 busy, 2 playing
    inline volatile int g_audioStop    = 0;

    // The MediaPlayer lives on its own thread. prepare() blocks for as long as
    // it takes to read the file, and it used to run on the game's render thread:
    // the game stopped presenting frames for seconds at the exact moment the
    // loading screen appeared, which is both the freeze and the crash.
    inline jobject      g_videoPlayer   = nullptr;   // owned by the worker thread
    inline pthread_t    g_worker        = 0;
    inline volatile int g_workerState   = 0;         // 0 idle, 1 preparing, 2 playing, 3 dead
    inline volatile int g_workerGen     = 0;         // guards against a stale worker
    inline volatile int g_stopRequested = 0;
    inline volatile int g_workerW       = 0;
    inline volatile int g_workerH       = 0;

    namespace detail
    {
        struct Jni
        {
            bool      ok           = false;
            bool      tried        = false;
            jclass    surfaceTexture = nullptr;
            jclass    surface        = nullptr;
            jclass    mediaPlayer    = nullptr;
            jmethodID stCtor         = nullptr;
            jmethodID stUpdate       = nullptr;
            jmethodID stTransform    = nullptr;
            jmethodID stTimestamp    = nullptr;
            jmethodID surfaceCtor    = nullptr;
            jmethodID mpCtor         = nullptr;
            jmethodID mpSource       = nullptr;
            jmethodID mpSetSurface   = nullptr;
            jmethodID mpLoop         = nullptr;
            jmethodID mpVolume       = nullptr;
            jmethodID mpPrepare      = nullptr;
            jmethodID mpStart        = nullptr;
            jmethodID mpRelease      = nullptr;
            jmethodID mpWidth        = nullptr;
            jmethodID mpHeight       = nullptr;
        };

        inline Jni g_jni;

        inline JNIEnv* Env()
        {
            if (jvm == nullptr) return nullptr;
            JNIEnv* env = nullptr;
            if (jvm->AttachCurrentThread(&env, nullptr) != JNI_OK) return nullptr;
            return env;
        }

        inline jclass GlobalClass(JNIEnv* env, const char* name)
        {
            jclass local = env->FindClass(name);
            if (local == nullptr) { env->ExceptionClear(); return nullptr; }
            jclass global = (jclass)env->NewGlobalRef(local);
            env->DeleteLocalRef(local);
            return global;
        }

        inline jmethodID Method(JNIEnv* env, jclass cls, const char* name, const char* sig)
        {
            if (cls == nullptr) return nullptr;
            jmethodID id = env->GetMethodID(cls, name, sig);
            if (id == nullptr) env->ExceptionClear();
            return id;
        }

        inline bool JniReady()
        {
            Jni& j = g_jni;
            if (j.ok) return true;
            if (j.tried) return false;
            j.tried = true;

            JNIEnv* env = Env();
            if (env == nullptr) return false;

            j.surfaceTexture = GlobalClass(env, "android/graphics/SurfaceTexture");
            j.surface        = GlobalClass(env, "android/view/Surface");
            j.mediaPlayer    = GlobalClass(env, "android/media/MediaPlayer");
            if (j.surfaceTexture == nullptr || j.surface == nullptr || j.mediaPlayer == nullptr)
                return false;

            j.stCtor      = Method(env, j.surfaceTexture, "<init>", "(I)V");
            j.stUpdate    = Method(env, j.surfaceTexture, "updateTexImage", "()V");
            j.stTransform = Method(env, j.surfaceTexture, "getTransformMatrix", "([F)V");
            j.stTimestamp = Method(env, j.surfaceTexture, "getTimestamp", "()J");
            j.surfaceCtor = Method(env, j.surface, "<init>", "(Landroid/graphics/SurfaceTexture;)V");

            j.mpCtor       = Method(env, j.mediaPlayer, "<init>", "()V");
            j.mpSource     = Method(env, j.mediaPlayer, "setDataSource", "(Ljava/lang/String;)V");
            j.mpSetSurface = Method(env, j.mediaPlayer, "setSurface", "(Landroid/view/Surface;)V");
            j.mpLoop       = Method(env, j.mediaPlayer, "setLooping", "(Z)V");
            j.mpVolume     = Method(env, j.mediaPlayer, "setVolume", "(FF)V");
            j.mpPrepare    = Method(env, j.mediaPlayer, "prepare", "()V");
            j.mpStart      = Method(env, j.mediaPlayer, "start", "()V");
            j.mpRelease    = Method(env, j.mediaPlayer, "release", "()V");
            j.mpWidth      = Method(env, j.mediaPlayer, "getVideoWidth", "()I");
            j.mpHeight     = Method(env, j.mediaPlayer, "getVideoHeight", "()I");

            j.ok = j.stCtor && j.stUpdate && j.stTimestamp && j.surfaceCtor && j.mpCtor &&
                   j.mpSetSurface && j.mpLoop && j.mpPrepare && j.mpStart && j.mpRelease;
            return j.ok;
        }

        inline bool SetSource(JNIEnv* env, jobject player, const std::string& path)
        {
            jstring src = env->NewStringUTF(path.c_str());
            if (src == nullptr) { env->ExceptionClear(); return false; }
            env->CallVoidMethod(player, g_jni.mpSource, src);
            env->DeleteLocalRef(src);
            if (env->ExceptionCheck()) { env->ExceptionClear(); return false; }
            return true;
        }

        inline GLuint Compile(GLenum type, const char* src)
        {
            GLuint sh = glCreateShader(type);
            if (sh == 0) return 0;
            glShaderSource(sh, 1, &src, nullptr);
            glCompileShader(sh);
            GLint ok = 0;
            glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
            if (!ok) { glDeleteShader(sh); return 0; }
            return sh;
        }

        inline bool BuildProgram()
        {
            if (g_program != 0) return g_vao != 0;

            static const char* vert =
                "#version 300 es\n"
                "uniform mat4 uMatrix;\n"
                "out vec2 vUv;\n"
                "void main() {\n"
                "  const vec2 p[4] = vec2[4](vec2(-1.0,-1.0), vec2(1.0,-1.0),"
                " vec2(-1.0,1.0), vec2(1.0,1.0));\n"
                "  const vec2 t[4] = vec2[4](vec2(0.0,1.0), vec2(1.0,1.0),"
                " vec2(0.0,0.0), vec2(1.0,0.0));\n"
                "  vec4 uv = uMatrix * vec4(t[gl_VertexID], 0.0, 1.0);\n"
                "  vUv = uv.xy;\n"
                "  gl_Position = vec4(p[gl_VertexID], 0.0, 1.0);\n"
                "}\n";
            static const char* frag =
                "#version 300 es\n"
                "#extension GL_OES_EGL_image_external_essl3 : require\n"
                "precision highp float;\n"
                "uniform samplerExternalOES uTex;\n"
                "in vec2 vUv;\n"
                "out vec4 fragColor;\n"
                "void main() {\n"
                "  fragColor = vec4(texture(uTex, vUv).rgb, 1.0);\n"
                "}\n";

            GLuint vs = Compile(GL_VERTEX_SHADER, vert);
            GLuint fs = Compile(GL_FRAGMENT_SHADER, frag);
            if (vs == 0 || fs == 0)
            {
                if (vs) glDeleteShader(vs);
                if (fs) glDeleteShader(fs);
                return false;
            }

            GLuint program = glCreateProgram();
            glAttachShader(program, vs);
            glAttachShader(program, fs);
            glLinkProgram(program);
            glDeleteShader(vs);
            glDeleteShader(fs);

            GLint ok = 0;
            glGetProgramiv(program, GL_LINK_STATUS, &ok);
            if (!ok) { glDeleteProgram(program); return false; }

            g_program = program;
            g_uMatrix = glGetUniformLocation(program, "uMatrix");
            g_uTex    = glGetUniformLocation(program, "uTex");
            glGenVertexArrays(1, &g_vao);
            return g_vao != 0;
        }

        // Cap the intermediate frame at 1080p to avoid the 33 MB RGBA8
        // allocation required for full 4K. The media itself is never transcoded.
        inline void ClampTargetSize(int* width, int* height)
        {
            if (width == nullptr || height == nullptr) return;
            int w = *width, h = *height;
            if (w <= 0 || h <= 0) { *width = 0; *height = 0; return; }
            if (w > kMaxVideoTexW) { h = (int)((long)h * kMaxVideoTexW / w); w = kMaxVideoTexW; }
            if (h > kMaxVideoTexH) { w = (int)((long)w * kMaxVideoTexH / h); h = kMaxVideoTexH; }
            if (w <= 0 || h <= 0) { *width = 0; *height = 0; return; }
            *width = w; *height = h;
        }

        // Creates the texture the video is sampled into. On any failure every
        // object is released again and the target stays invalid, so the loader
        // never ends up drawing a texture that was never written into.
        inline bool EnsureTarget(int width, int height)
        {
            if (width <= 0 || height <= 0) return false;

            // Keep GPU allocation bounded, even with a 4K source.
            ClampTargetSize(&width, &height);
            if (width <= 0 || height <= 0) return false;

            if (g_targetTex != 0 && g_targetW == width && g_targetH == height) return true;

            if (g_targetTex != 0) { glDeleteTextures(1, &g_targetTex); g_targetTex = 0; }
            if (g_fbo != 0) { glDeleteFramebuffers(1, &g_fbo); g_fbo = 0; }
            g_targetW = 0;
            g_targetH = 0;
            g_videoTex = nullptr;
            g_targetReady = false;

            glGenTextures(1, &g_targetTex);
            if (g_targetTex == 0) return false;
            glBindTexture(GL_TEXTURE_2D, g_targetTex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glGenFramebuffers(1, &g_fbo);
            if (g_fbo == 0)
            {
                glDeleteTextures(1, &g_targetTex);
                g_targetTex = 0;
                return false;
            }
            glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_targetTex, 0);
            const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            if (status != GL_FRAMEBUFFER_COMPLETE)
            {
                glDeleteFramebuffers(1, &g_fbo);
                g_fbo = 0;
                glDeleteTextures(1, &g_targetTex);
                g_targetTex = 0;
                return false;
            }

            g_targetW  = width;
            g_targetH  = height;
            g_videoTex = (ImTextureID)(intptr_t)g_targetTex;
            return true;
        }

        struct WorkerArgs
        {
            jobject surfaces[2]; // global refs, owned by the worker
            int     gen;
        };

        // One MediaPlayer at a time, but distinct SurfaceTexture queues per clip.
        // This prevents previously decoded login frames from flashing in the loading card.
        inline void* VideoWorker(void* arg)
        {
            WorkerArgs* args = (WorkerArgs*)arg;
            if (args == nullptr) return nullptr;
            jobject surfaces[2] = {args->surfaces[0], args->surfaces[1]};
            const int gen = args->gen;
            delete args;

            JNIEnv* env = Env();
            if (env == nullptr || surfaces[0] == nullptr || surfaces[1] == nullptr)
            {
                if (env != nullptr)
                    for (jobject surface : surfaces) if (surface) env->DeleteGlobalRef(surface);
                if (gen == g_workerGen) g_workerState = 3;
                return nullptr;
            }

            while (!g_stopRequested && gen == g_workerGen)
            {
                const int requested = g_requestedVideo.load(std::memory_order_acquire);
                g_activeVideo.store(-1, std::memory_order_release);
                g_workerState = 1;
                g_workerW = g_workerH = 0;

                jobject player = env->NewObject(g_jni.mediaPlayer, g_jni.mpCtor);
                if (player == nullptr)
                {
                    env->ExceptionClear();
                    break;
                }

                static const char* loginNames[] = {"hachiware_login.mp4"};
                static const char* loadingNames[] = {"arknight_card.mp4"};
                const bool loading = requested == static_cast<int>(VideoKind::Loading);
                const std::string path = loading
                    ? ResolveMedia(loadingNames, 1, ethnir_loading_video, ethnir_loading_video_len)
                    : ResolveMedia(loginNames, 1, ethnir_login_video, ethnir_login_video_len);

                bool prepared = !path.empty() && SetSource(env, player, path);
                if (prepared)
                {
                    env->CallVoidMethod(player, g_jni.mpSetSurface, surfaces[loading ? 1 : 0]);
                    env->CallVoidMethod(player, g_jni.mpLoop, (jboolean)JNI_TRUE);
                    env->CallVoidMethod(player, g_jni.mpVolume, 0.0f, 0.0f);
                    prepared = !env->ExceptionCheck();
                    if (!prepared) env->ExceptionClear();
                }
                if (prepared)
                {
                    env->CallVoidMethod(player, g_jni.mpPrepare);
                    prepared = !env->ExceptionCheck();
                    if (!prepared) env->ExceptionClear();
                }
                if (prepared && !g_stopRequested && gen == g_workerGen &&
                    requested == g_requestedVideo.load(std::memory_order_acquire))
                {
                    g_workerW = (int)env->CallIntMethod(player, g_jni.mpWidth);
                    g_workerH = (int)env->CallIntMethod(player, g_jni.mpHeight);
                    if (env->ExceptionCheck())
                    {
                        env->ExceptionClear();
                        g_workerW = g_workerH = 0;
                    }
                    env->CallVoidMethod(player, g_jni.mpStart);
                    prepared = !env->ExceptionCheck();
                    if (!prepared) env->ExceptionClear();
                    if (prepared)
                    {
                        g_videoPlayer = player;
                        g_activeVideo.store(requested, std::memory_order_release);
                        g_workerState = 2;
                        while (!g_stopRequested && gen == g_workerGen &&
                               requested == g_requestedVideo.load(std::memory_order_acquire))
                            usleep(50 * 1000);
                        g_activeVideo.store(-1, std::memory_order_release);
                        g_workerState = 1;
                        g_videoPlayer = nullptr;
                    }
                }
                env->CallVoidMethod(player, g_jni.mpRelease);
                if (env->ExceptionCheck()) env->ExceptionClear();
                env->DeleteLocalRef(player);
                // A failed load is terminal. A requested clip change is not.
                if (!prepared) break;
            }

            g_activeVideo.store(-1, std::memory_order_release);
            for (jobject surface : surfaces) env->DeleteGlobalRef(surface);
            if (gen == g_workerGen) g_workerState = 3;
            return nullptr;
        }
    }

    // Starts the video: the GL side here on the render thread, the MediaPlayer
    // on its own thread. Returns true once the worker is running.
    inline bool StartVideo()
    {
        using namespace detail;
        if (g_workerState == 1 || g_workerState == 2) return true;

        if (!JniReady()) return false;
        if (!BuildProgram()) return false;

        JNIEnv* env = Env();
        if (env == nullptr) return false;

        // Each clip has its own external texture and SurfaceTexture buffer queue.
        // They never decode concurrently: the worker releases the old player
        // before starting the requested clip on its own surface.
        jobject surfaces[2] = {nullptr, nullptr};
        for (int index = 0; index < 2; ++index)
        {
            if (g_oesTextures[index] == 0)
                glGenTextures(1, &g_oesTextures[index]);
            if (g_oesTextures[index] == 0) return false;
            glBindTexture(GL_TEXTURE_EXTERNAL_OES, g_oesTextures[index]);
            glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            if (g_surfaceTextures[index] != nullptr)
            {
                env->DeleteGlobalRef(g_surfaceTextures[index]);
                g_surfaceTextures[index] = nullptr;
            }

            jobject surfaceTexture = env->NewObject(g_jni.surfaceTexture, g_jni.stCtor,
                                                    (jint)g_oesTextures[index]);
            if (surfaceTexture == nullptr) { env->ExceptionClear(); break; }
            jobject surface = env->NewObject(g_jni.surface, g_jni.surfaceCtor, surfaceTexture);
            if (surface == nullptr)
            {
                env->ExceptionClear();
                env->DeleteLocalRef(surfaceTexture);
                break;
            }
            g_surfaceTextures[index] = env->NewGlobalRef(surfaceTexture);
            surfaces[index] = env->NewGlobalRef(surface);
            env->DeleteLocalRef(surfaceTexture);
            env->DeleteLocalRef(surface);
            if (g_surfaceTextures[index] == nullptr || surfaces[index] == nullptr) break;
        }
        if (surfaces[0] == nullptr || surfaces[1] == nullptr)
        {
            for (int index = 0; index < 2; ++index)
            {
                if (surfaces[index]) env->DeleteGlobalRef(surfaces[index]);
                if (g_surfaceTextures[index])
                {
                    env->DeleteGlobalRef(g_surfaceTextures[index]);
                    g_surfaceTextures[index] = nullptr;
                }
            }
            return false;
        }

        g_stopRequested = 0;
        const int gen = ++g_workerGen;
        g_workerState = 1;

        detail::WorkerArgs* args = new detail::WorkerArgs{{surfaces[0], surfaces[1]}, gen};
        if (pthread_create(&g_worker, nullptr, &detail::VideoWorker, args) != 0)
        {
            delete args;
            for (jobject surface : surfaces) env->DeleteGlobalRef(surface);
            g_workerState = 3;
            return false;
        }
        pthread_detach(g_worker);
        g_worker = 0;
        return true;
    }

    inline void UpdateVideo()
    {
        using namespace detail;
        if (g_workerState != 2 || g_activeVideo.load(std::memory_order_acquire) !=
            g_requestedVideo.load(std::memory_order_acquire)) return;
        if (g_program == 0) return;
        const int videoSlot = g_activeVideo.load(std::memory_order_acquire);
        if (videoSlot < 0 || videoSlot > 1 || g_surfaceTextures[videoSlot] == nullptr) return;

        // The loader repaints at the display rate, but the clip is 30 fps. Copying
        // the external frame into our own texture every render frame doubled the
        // GL work (a full 960x540 blit plus the state save/restore around it) and
        // produced frames identical to the previous one. One update per clip frame
        // is enough and roughly halves the cost of the loader's slowest path.
        {
            struct timespec ts;
            clock_gettime(CLOCK_MONOTONIC, &ts);
            const uint64_t nowMs = (uint64_t)ts.tv_sec * 1000ull + (uint64_t)ts.tv_nsec / 1000000ull;
            static uint64_t lastMs = 0;
            if (lastMs != 0 && nowMs - lastMs < 32ull) return;
            lastMs = nowMs;
        }

        JNIEnv* env = Env();
        if (env == nullptr) return;

        // A video switch can change resolution (Hachiware 640x360 to Arknights
        // 960x540). Recreate the FBO when needed, otherwise Arknights stays
        // stuck at the smaller previous target and looks unnecessarily soft.
        int width = g_workerW;
        int height = g_workerH;
        if (width <= 0 || height <= 0) return;
        ClampTargetSize(&width, &height);
        if (g_targetTex == 0 || g_fbo == 0 || g_targetW != width || g_targetH != height)
            if (!EnsureTarget(width, height)) return;

        glBindTexture(GL_TEXTURE_EXTERNAL_OES, g_oesTextures[videoSlot]);
        env->CallVoidMethod(g_surfaceTextures[videoSlot], g_jni.stUpdate);
        if (env->ExceptionCheck()) { env->ExceptionClear(); return; }
        // A newly created SurfaceTexture has no decoded frame yet. Wait until
        // its timestamp advances instead of sampling uninitialized black pixels.
        const jlong frameTimestamp = env->CallLongMethod(g_surfaceTextures[videoSlot], g_jni.stTimestamp);
        if (env->ExceptionCheck()) { env->ExceptionClear(); return; }
        if (frameTimestamp == 0) return;

        if (g_jni.stTransform != nullptr)
        {
            // One global array, reused. This used to allocate a 16-float JNI array
            // every frame, which the loader then threw away - pure garbage for the
            // ART collector to chase while the clip was decoding.
            if (g_matrixArray == nullptr)
            {
                jfloatArray local = env->NewFloatArray(16);
                if (local != nullptr)
                {
                    g_matrixArray = (jfloatArray)env->NewGlobalRef(local);
                    env->DeleteLocalRef(local);
                }
                else
                {
                    env->ExceptionClear();
                }
            }
            if (g_matrixArray != nullptr)
            {
                env->CallVoidMethod(g_surfaceTextures[videoSlot], g_jni.stTransform, g_matrixArray);
                if (env->ExceptionCheck()) env->ExceptionClear();
                else env->GetFloatArrayRegion(g_matrixArray, 0, 16, g_matrix);
            }
        }

        // The game owns this context: put everything back the way it was, or
        // the game's own renderer draws with the wrong state afterwards.
        GLint prevFbo = 0;
        GLint prevView[4] = {0, 0, 0, 0};
        GLint prevActive = 0;
        GLint prevArray = 0;
        GLint prevProgram = 0;
        GLboolean prevBlend = GL_FALSE;
        GLboolean prevDepth = GL_FALSE;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);
        glGetIntegerv(GL_VIEWPORT, prevView);
        glGetIntegerv(GL_ACTIVE_TEXTURE, &prevActive);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevArray);
        glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);
        prevBlend  = glIsEnabled(GL_BLEND);
        prevDepth  = glIsEnabled(GL_DEPTH_TEST);

        glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);
        glViewport(0, 0, g_targetW, g_targetH);
        glDisable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glUseProgram(g_program);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, g_oesTextures[videoSlot]);
        glUniform1i(g_uTex, 0);
        glUniformMatrix4fv(g_uMatrix, 1, GL_FALSE, g_matrix);
        glBindVertexArray(g_vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glBindVertexArray((GLuint)prevArray);
        glUseProgram((GLuint)prevProgram);
        glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)prevFbo);
        glViewport(prevView[0], prevView[1], prevView[2], prevView[3]);
        glActiveTexture((GLenum)prevActive);
        if (prevBlend)  glEnable(GL_BLEND);      else glDisable(GL_BLEND);
        if (prevDepth)  glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);

        // Only now is the texture worth showing: it has a frame in it.
        g_targetReady = true;
    }

    namespace detail
    {
        inline void* AudioWorker(void*)
        {
            JNIEnv* env = Env();
            if (env == nullptr) { g_audioState = 0; return nullptr; }

            jobject player = env->NewObject(g_jni.mediaPlayer, g_jni.mpCtor);
            if (player == nullptr) { env->ExceptionClear(); g_audioState = 0; return nullptr; }

            // The card video is silent; the sound is file-only by design, so
            // there is no compiled-in audio payload to hand over here.
            static const char* names[2] = { "loading.mp3", "lofi.mp3" };
            const std::string path = ResolveMedia(names, 2, nullptr, 0);
            bool ok = !path.empty() && SetSource(env, player, path);

            if (ok)
            {
                env->CallVoidMethod(player, g_jni.mpLoop, (jboolean)JNI_FALSE);
                env->CallVoidMethod(player, g_jni.mpVolume, 1.0f, 1.0f);
                if (env->ExceptionCheck()) { env->ExceptionClear(); ok = false; }
            }

            if (ok)
            {
                env->CallVoidMethod(player, g_jni.mpPrepare);
                if (env->ExceptionCheck()) { env->ExceptionClear(); ok = false; }
            }

            if (ok && g_audioStop)
            {
                env->CallVoidMethod(player, g_jni.mpRelease);
                g_audioState = 0;
                return nullptr;
            }

            if (ok)
            {
                env->CallVoidMethod(player, g_jni.mpStart);
                if (env->ExceptionCheck()) { env->ExceptionClear(); ok = false; }
            }

            if (!ok)
            {
                env->CallVoidMethod(player, g_jni.mpRelease);
                g_audioState = 0;
                return nullptr;
            }

            g_audioPlayer = env->NewGlobalRef(player);
            g_audioLive   = g_audioPlayer != nullptr;
            g_audioState  = g_audioLive ? 2 : 0;
            return nullptr;
        }
    }

    inline bool StartAudio()
    {
        using namespace detail;
        if (g_audioLive && g_audioPlayer != nullptr) return true;
        if (g_audioState == 1) return false;
        if (!JniReady()) return false;

        g_audioStop = 0;
        g_audioState = 1;
        if (pthread_create(&g_audioWorker, nullptr, &detail::AudioWorker, nullptr) != 0)
        {
            g_audioState = 0;
            return false;
        }
        pthread_detach(g_audioWorker);
        g_audioWorker = 0;
        return true;
    }

    inline void StopAudio()
    {
        using namespace detail;
        g_audioLive = false;
        g_audioStop = 1;
        if (g_audioPlayer == nullptr) return;
        JNIEnv* env = Env();
        if (env != nullptr && g_jni.ok)
        {
            env->CallVoidMethod(g_audioPlayer, g_jni.mpRelease);
            if (env->ExceptionCheck()) env->ExceptionClear();
            env->DeleteGlobalRef(g_audioPlayer);
        }
        g_audioPlayer = nullptr;
    }

    // Stops the worker. The MediaPlayer is released by the worker itself, so
    // this never blocks the render thread.
    inline void StopVideo()
    {
        using namespace detail;
        g_videoLive = false;
        g_stopRequested = 1;

        JNIEnv* env = Env();
        for (jobject& surfaceTexture : g_surfaceTextures)
        {
            if (surfaceTexture != nullptr)
            {
                if (env != nullptr) env->DeleteGlobalRef(surfaceTexture);
                surfaceTexture = nullptr;
            }
        }

        if (g_matrixArray != nullptr)
        {
            JNIEnv* env = Env();
            if (env != nullptr) env->DeleteGlobalRef(g_matrixArray);
            g_matrixArray = nullptr;
        }
    }

    inline void SetVideo(VideoKind kind)
    {
        const int next = static_cast<int>(kind);
        if (g_requestedVideo.exchange(next, std::memory_order_acq_rel) != next)
        {
            g_targetReady = false;
            g_videoLive = false;
            g_videoW = g_videoH = 0;
        }
    }

    inline void Tick()
    {
        static int retries = 0;
        static int frames  = 0;

        if (!g_inited)
        {
            g_inited = true;
            StartVideo();
        }
        else if (g_workerState == 3 && retries < 3 && ++frames >= 30)
        {
            frames = 0;
            retries++;
            StartVideo();
        }
        UpdateVideo();

        g_videoLive = (g_workerState == 2 &&
                       g_activeVideo.load(std::memory_order_acquire) ==
                       g_requestedVideo.load(std::memory_order_acquire));
        if (g_videoLive)
        {
            g_videoW = g_workerW;
            g_videoH = g_workerH;
        }
    }

    inline void Shutdown()
    {
        StopAudio();
        StopVideo();
        g_inited = false;
    }

    // Starts the worker before the loader is up. The login screen calls this, so
    // the player has been created, the file opened and a frame blitted into the
    // texture by the time the card appears - the gap between the two is what
    // showed as the black screen after signing in.
    inline void Warmup() { Tick(); }

    inline bool  VideoReady()   { return g_videoLive && g_targetTex != 0 && g_targetReady; }

    // True when a video is on its way: the clip is compiled in, or the worker is
    // opening it. The card uses this to hold its procedural field for the frame
    // or two that takes instead of showing the still photo and swapping later.
    inline bool VideoPending()
    {
        return (g_requestedVideo.load(std::memory_order_acquire) ==
                static_cast<int>(VideoKind::Loading)
                    ? ethnir_loading_video_len > 0 : ethnir_login_video_len > 0)
               || g_workerState == 1 || g_workerState == 2;
    }

    inline float VideoAspect()
    {
        if (g_videoW <= 0 || g_videoH <= 0) return 16.0f / 9.0f;
        return (float)g_videoW / (float)g_videoH;
    }

#else

    // Host-side stubs exercise video selection and the same scaling rules.
    inline int g_workerState = 0;
    inline bool StartVideo() { return false; }
    namespace detail
    {
        inline void ClampTargetSize(int* width, int* height)
        {
            if (!width || !height) return;
            int w = *width, h = *height;
            if (w <= 0 || h <= 0) { *width = 0; *height = 0; return; }
            if (w > 1920) { h = (int)((long long)h * 1920 / w); w = 1920; }
            if (h > 1080) { w = (int)((long long)w * 1080 / h); h = 1080; }
            *width = w;
            *height = h;
        }
    }

    inline void  SetVideo(VideoKind kind)
    {
        const int next = static_cast<int>(kind);
        if (g_requestedVideo.exchange(next, std::memory_order_acq_rel) != next)
        {
            g_hostVideoTex = nullptr;
            g_hostVideoW = g_hostVideoH = 0;
            g_hostVideoPending = true;
        }
    }
    inline void  Tick()      {}
    inline void  Shutdown()  {}
    inline void  Warmup()    {}
    inline bool  StartAudio(){ return false; }
    inline void  StopAudio() {}
    inline bool  VideoReady(){ return ui_media::g_hostVideoTex != nullptr; }
    inline bool  VideoPending(){ return ui_media::g_hostVideoPending; }
    inline float VideoAspect()
    {
        if (ui_media::g_hostVideoW <= 0 || ui_media::g_hostVideoH <= 0) return 16.0f / 9.0f;
        return (float)ui_media::g_hostVideoW / (float)ui_media::g_hostVideoH;
    }

#endif

    inline ImTextureID VideoTexture()
    {
#if defined(__ANDROID__)
        return ui_media::VideoReady() ? g_videoTex : nullptr;
#else
        return ui_media::g_hostVideoTex;
#endif
    }
}
