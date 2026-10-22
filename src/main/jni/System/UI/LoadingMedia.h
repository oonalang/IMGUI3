#pragma once

#include "imgui.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>

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
#define UI_MEDIA_ASSET_AUDIO 1
#endif
#endif

#ifndef UI_MEDIA_ASSET_VIDEO
#define UI_MEDIA_ASSET_VIDEO 0
#endif
#ifndef UI_MEDIA_ASSET_AUDIO
#define UI_MEDIA_ASSET_AUDIO 0
#endif

#if !UI_MEDIA_ASSET_VIDEO
static inline const unsigned char ethnir_loading_video[] = {0};
static inline const unsigned int  ethnir_loading_video_len = 0;
#endif
#if !UI_MEDIA_ASSET_AUDIO
static inline const unsigned char ethnir_loading_audio[] = {0};
static inline const unsigned int  ethnir_loading_audio_len = 0;
#endif

namespace ui_media
{
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
            // The compiled-in clip is written into the game's own files directory
            // before anything else is looked at. That directory is the first one
            // searched, so the embedded clip is what the card plays: there is no
            // file to push to the device first, and no other video on the SD card
            // can take its place.
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

    inline GLuint g_oesTexture   = 0;
    inline GLuint g_targetTex    = 0;
    inline GLuint g_fbo          = 0;
    inline GLuint g_program      = 0;
    inline GLuint g_vao          = 0;
    inline GLint  g_uTex         = -1;
    inline GLint  g_uMatrix      = -1;
    inline int    g_targetW      = 0;
    inline int    g_targetH      = 0;
    inline float  g_matrix[16]   = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    // The video is only ever shown scaled down to the screen, so there is no
    // reason to hold a 4K copy of it. A 3840x2160 RGBA8 target is 33 MB on its
    // own; on a mid-range phone that allocation fails, and the failed texture
    // was then drawn fullscreen - that was the black screen.
    static constexpr int kMaxVideoTexW = 1920;
    static constexpr int kMaxVideoTexH = 1080;

    inline jobject g_surfaceTexture = nullptr;
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

            j.ok = j.stCtor && j.stUpdate && j.surfaceCtor && j.mpCtor && j.mpSource &&
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
                "precision mediump float;\n"
                "uniform samplerExternalOES uTex;\n"
                "in vec2 vUv;\n"
                "out vec4 fragColor;\n"
                "void main() { fragColor = texture(uTex, vUv); }\n";

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

        // Caps the video target to what the screen can actually show. A 4K
        // source is 33 MB of RGBA8 that many devices refuse to allocate, and a
        // failed allocation used to leave an empty texture behind that the
        // loader drew fullscreen - that was the black screen.
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

            // Cap the size: the frame is scaled down to the screen anyway, and a
            // 4K target is 33 MB that many devices refuse to allocate.
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
            jobject surface;   // global ref, owned by the worker
            int     gen;
        };

        // The worker owns the MediaPlayer from here on. It creates it, reads
        // the file (prepare blocks for as long as that takes) and starts it.
        // None of this happens on the game's render thread.
        inline void* VideoWorker(void* arg)
        {
            WorkerArgs* args = (WorkerArgs*)arg;
            if (args == nullptr) return nullptr;
            jobject surface = args->surface;
            const int gen = args->gen;
            delete args;

            JNIEnv* env = Env();
            if (env == nullptr || surface == nullptr)
            {
                if (gen == g_workerGen) g_workerState = 3;
                return nullptr;
            }

            jobject player = env->NewObject(g_jni.mediaPlayer, g_jni.mpCtor);
            if (player == nullptr)
            {
                env->ExceptionClear();
                env->DeleteGlobalRef(surface);
                if (gen == g_workerGen) g_workerState = 3;
                return nullptr;
            }

            // The arknights encounter background is the loading card video. The
            // name is first so that a copy the loader wrote itself is found ahead
            // of anything else; `loading.mp4` stays accepted for a device that
            // already has one.
            static const char* names[2] = {
                "arknights-eyjafjallas-gentle-glow.1920x1080.mp4",
                "loading.mp4"
            };
            const std::string path = ResolveMedia(names, 2, ethnir_loading_video,
                                                  ethnir_loading_video_len);
            if (path.empty() || !SetSource(env, player, path))
            {
                env->CallVoidMethod(player, g_jni.mpRelease);
                env->DeleteGlobalRef(surface);
                if (gen == g_workerGen) g_workerState = 3;
                return nullptr;
            }

            env->CallVoidMethod(player, g_jni.mpSetSurface, surface);
            env->CallVoidMethod(player, g_jni.mpLoop, (jboolean)JNI_TRUE);
            env->CallVoidMethod(player, g_jni.mpVolume, 0.0f, 0.0f);
            if (env->ExceptionCheck())
            {
                env->ExceptionClear();
                env->CallVoidMethod(player, g_jni.mpRelease);
                env->DeleteGlobalRef(surface);
                if (gen == g_workerGen) g_workerState = 3;
                return nullptr;
            }

            // The blocking part - and it is off the render thread now.
            env->CallVoidMethod(player, g_jni.mpPrepare);
            if (env->ExceptionCheck())
            {
                env->ExceptionClear();
                env->CallVoidMethod(player, g_jni.mpRelease);
                env->DeleteGlobalRef(surface);
                if (gen == g_workerGen) g_workerState = 3;
                return nullptr;
            }

            g_workerW = (int)env->CallIntMethod(player, g_jni.mpWidth);
            g_workerH = (int)env->CallIntMethod(player, g_jni.mpHeight);
            if (env->ExceptionCheck()) { env->ExceptionClear(); g_workerW = 0; g_workerH = 0; }

            if (g_stopRequested)
            {
                env->CallVoidMethod(player, g_jni.mpRelease);
                env->DeleteGlobalRef(surface);
                if (gen == g_workerGen) g_workerState = 3;
                return nullptr;
            }

            env->CallVoidMethod(player, g_jni.mpStart);
            if (env->ExceptionCheck())
            {
                env->ExceptionClear();
                env->CallVoidMethod(player, g_jni.mpRelease);
                env->DeleteGlobalRef(surface);
                if (gen == g_workerGen) g_workerState = 3;
                return nullptr;
            }

            g_videoPlayer = player;
            if (gen == g_workerGen) g_workerState = 2;

            // Stay alive until the loader is done, then release the player.
            while (!g_stopRequested)
                usleep(50 * 1000);

            env->CallVoidMethod(player, g_jni.mpRelease);
            if (env->ExceptionCheck()) env->ExceptionClear();
            g_videoPlayer = nullptr;
            env->DeleteGlobalRef(surface);
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

        if (g_oesTexture == 0)
        {
            glGenTextures(1, &g_oesTexture);
            if (g_oesTexture == 0) return false;
        }
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, g_oesTexture);

        // A previous attempt may have left a SurfaceTexture behind.
        if (g_surfaceTexture != nullptr)
        {
            env->DeleteGlobalRef(g_surfaceTexture);
            g_surfaceTexture = nullptr;
        }

        jobject surfaceTexture = env->NewObject(g_jni.surfaceTexture, g_jni.stCtor,
                                                 (jint)g_oesTexture);
        if (surfaceTexture == nullptr) { env->ExceptionClear(); return false; }

        jobject surface = env->NewObject(g_jni.surface, g_jni.surfaceCtor, surfaceTexture);
        if (surface == nullptr)
        {
            env->ExceptionClear();
            env->DeleteLocalRef(surfaceTexture);
            return false;
        }

        g_surfaceTexture = env->NewGlobalRef(surfaceTexture);
        jobject surfaceGlobal = env->NewGlobalRef(surface);
        env->DeleteLocalRef(surfaceTexture);
        env->DeleteLocalRef(surface);
        if (g_surfaceTexture == nullptr || surfaceGlobal == nullptr)
        {
            if (g_surfaceTexture != nullptr) { env->DeleteGlobalRef(g_surfaceTexture); g_surfaceTexture = nullptr; }
            if (surfaceGlobal != nullptr) env->DeleteGlobalRef(surfaceGlobal);
            return false;
        }

        g_stopRequested = 0;
        const int gen = ++g_workerGen;
        g_workerState = 1;

        detail::WorkerArgs* args = new detail::WorkerArgs{ surfaceGlobal, gen };
        if (pthread_create(&g_worker, nullptr, &detail::VideoWorker, args) != 0)
        {
            delete args;
            env->DeleteGlobalRef(surfaceGlobal);
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
        if (g_workerState != 2) return;          // the worker has not started it yet
        if (g_program == 0) return;
        if (g_surfaceTexture == nullptr) return;

        JNIEnv* env = Env();
        if (env == nullptr) return;

        if (g_targetTex == 0 || g_fbo == 0)
        {
            const int w = g_workerW;
            const int h = g_workerH;
            if (w <= 0 || h <= 0) return;
            if (!EnsureTarget(w, h)) return;
        }

        glBindTexture(GL_TEXTURE_EXTERNAL_OES, g_oesTexture);
        env->CallVoidMethod(g_surfaceTexture, g_jni.stUpdate);
        if (env->ExceptionCheck()) { env->ExceptionClear(); return; }

        if (g_jni.stTransform != nullptr)
        {
            jfloatArray arr = env->NewFloatArray(16);
            if (arr != nullptr)
            {
                env->CallVoidMethod(g_surfaceTexture, g_jni.stTransform, arr);
                if (env->ExceptionCheck()) env->ExceptionClear();
                else env->GetFloatArrayRegion(arr, 0, 16, g_matrix);
                env->DeleteLocalRef(arr);
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
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, g_oesTexture);
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

            static const char* names[2] = { "loading.mp3", "lofi.mp3" };
            const std::string path = ResolveMedia(names, 2, ethnir_loading_audio,
                                                  ethnir_loading_audio_len);
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

        if (g_surfaceTexture != nullptr)
        {
            JNIEnv* env = Env();
            if (env != nullptr) env->DeleteGlobalRef(g_surfaceTexture);
            g_surfaceTexture = nullptr;
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

        g_videoLive = (g_workerState == 2);
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
        return ethnir_loading_video_len > 0 || g_workerState == 1 || g_workerState == 2;
    }

    inline float VideoAspect()
    {
        if (g_videoW <= 0 || g_videoH <= 0) return 16.0f / 9.0f;
        return (float)g_videoW / (float)g_videoH;
    }

#else

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
