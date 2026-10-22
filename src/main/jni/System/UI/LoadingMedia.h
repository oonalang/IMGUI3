#pragma once

#include "imgui.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>

#if defined(__ANDROID__)
#include <jni.h>
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

        inline std::string ResolveMedia(const char* const* names, int nameCount,
                                        const unsigned char* embedded, unsigned embeddedSize)
        {
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

    inline jobject g_videoPlayer    = nullptr;
    inline jobject g_surface       = nullptr;
    inline jobject g_surfaceTexture = nullptr;
    inline jobject g_audioPlayer   = nullptr;

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

        inline bool EnsureTarget(int width, int height)
        {
            if (width <= 0 || height <= 0) return false;
            if (g_targetTex != 0 && g_targetW == width && g_targetH == height) return true;

            if (g_targetTex != 0) { glDeleteTextures(1, &g_targetTex); g_targetTex = 0; }
            if (g_fbo != 0) { glDeleteFramebuffers(1, &g_fbo); g_fbo = 0; }

            glGenTextures(1, &g_targetTex);
            glBindTexture(GL_TEXTURE_2D, g_targetTex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glGenFramebuffers(1, &g_fbo);
            glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_targetTex, 0);
            const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            if (status != GL_FRAMEBUFFER_COMPLETE)
            {
                glDeleteFramebuffers(1, &g_fbo);
                g_fbo = 0;
                return false;
            }

            g_targetW  = width;
            g_targetH  = height;
            g_videoTex = (ImTextureID)(intptr_t)g_targetTex;
            return true;
        }
    }

    inline bool StartVideo()
    {
        using namespace detail;
        if (g_videoLive) return true;
        if (!JniReady()) return false;
        if (!BuildProgram()) return false;

        JNIEnv* env = Env();
        if (env == nullptr) return false;

        static const char* names[2] = { "loading.mp4", "velina-aegard.3840x2160.mp4" };
        const std::string path = ResolveMedia(names, 2, ethnir_loading_video, ethnir_loading_video_len);
        if (path.empty()) return false;

        if (g_oesTexture == 0)
        {
            glGenTextures(1, &g_oesTexture);
            if (g_oesTexture == 0) return false;
        }
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, g_oesTexture);

        jobject surfaceTexture = env->NewObject(g_jni.surfaceTexture, g_jni.stCtor, (jint)g_oesTexture);
        if (surfaceTexture == nullptr) { env->ExceptionClear(); return false; }

        jobject surface = env->NewObject(g_jni.surface, g_jni.surfaceCtor, surfaceTexture);
        if (surface == nullptr) { env->ExceptionClear(); return false; }

        jobject player = env->NewObject(g_jni.mediaPlayer, g_jni.mpCtor);
        if (player == nullptr) { env->ExceptionClear(); return false; }

        if (!SetSource(env, player, path)) return false;

        env->CallVoidMethod(player, g_jni.mpSetSurface, surface);
        env->CallVoidMethod(player, g_jni.mpLoop, (jboolean)JNI_TRUE);
        env->CallVoidMethod(player, g_jni.mpVolume, 0.0f, 0.0f);
        env->CallVoidMethod(player, g_jni.mpPrepare);
        if (env->ExceptionCheck()) { env->ExceptionClear(); return false; }

        g_videoW = (int)env->CallIntMethod(player, g_jni.mpWidth);
        g_videoH = (int)env->CallIntMethod(player, g_jni.mpHeight);
        if (env->ExceptionCheck()) { env->ExceptionClear(); g_videoW = 0; g_videoH = 0; }

        env->CallVoidMethod(player, g_jni.mpStart);
        if (env->ExceptionCheck()) { env->ExceptionClear(); return false; }

        g_videoPlayer    = env->NewGlobalRef(player);
        g_surface        = env->NewGlobalRef(surface);
        g_surfaceTexture = env->NewGlobalRef(surfaceTexture);

        g_videoLive = g_videoPlayer != nullptr;
        if (!EnsureTarget(g_videoW, g_videoH))
        {
            g_videoW = 0;
            g_videoH = 0;
        }
        return g_videoLive;
    }

    inline void UpdateVideo()
    {
        using namespace detail;
        if (!g_videoLive || g_videoPlayer == nullptr || g_surfaceTexture == nullptr) return;
        if (g_program == 0) return;

        JNIEnv* env = Env();
        if (env == nullptr) return;

        if (g_targetTex == 0 || g_fbo == 0)
        {
            if (g_videoW <= 0 || g_videoH <= 0)
            {
                g_videoW = (int)env->CallIntMethod(g_videoPlayer, g_jni.mpWidth);
                g_videoH = (int)env->CallIntMethod(g_videoPlayer, g_jni.mpHeight);
                if (env->ExceptionCheck()) { env->ExceptionClear(); g_videoW = 0; g_videoH = 0; }
            }
            if (g_videoW <= 0 || g_videoH <= 0) return;
            if (!EnsureTarget(g_videoW, g_videoH)) { g_videoW = 0; g_videoH = 0; return; }
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

        GLint prevFbo = 0;
        GLint prevView[4] = {0, 0, 0, 0};
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);
        glGetIntegerv(GL_VIEWPORT, prevView);

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
        glBindVertexArray(0);
        glUseProgram(0);
        glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)prevFbo);
        glViewport(prevView[0], prevView[1], prevView[2], prevView[3]);
        glEnable(GL_BLEND);
    }

    inline bool StartAudio()
    {
        using namespace detail;
        if (g_audioLive && g_audioPlayer != nullptr) return true;
        if (!JniReady()) return false;

        JNIEnv* env = Env();
        if (env == nullptr) return false;

        static const char* names[2] = { "loading.mp3", "lofi.mp3" };
        const std::string path = ResolveMedia(names, 2, ethnir_loading_audio, ethnir_loading_audio_len);
        if (path.empty()) return false;

        jobject player = env->NewObject(g_jni.mediaPlayer, g_jni.mpCtor);
        if (player == nullptr) { env->ExceptionClear(); return false; }

        if (!SetSource(env, player, path)) return false;

        env->CallVoidMethod(player, g_jni.mpLoop, (jboolean)JNI_FALSE);
        env->CallVoidMethod(player, g_jni.mpVolume, 1.0f, 1.0f);
        env->CallVoidMethod(player, g_jni.mpPrepare);
        if (env->ExceptionCheck()) { env->ExceptionClear(); return false; }

        env->CallVoidMethod(player, g_jni.mpStart);
        if (env->ExceptionCheck()) { env->ExceptionClear(); return false; }

        g_audioPlayer = env->NewGlobalRef(player);
        g_audioLive   = g_audioPlayer != nullptr;
        return g_audioLive;
    }

    inline void StopAudio()
    {
        using namespace detail;
        g_audioLive = false;
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

    inline void StopVideo()
    {
        using namespace detail;
        g_videoLive = false;
        JNIEnv* env = Env();
        if (env != nullptr && g_jni.ok)
        {
            if (g_videoPlayer != nullptr)
            {
                env->CallVoidMethod(g_videoPlayer, g_jni.mpRelease);
                if (env->ExceptionCheck()) env->ExceptionClear();
                env->DeleteGlobalRef(g_videoPlayer);
            }
            if (g_surface != nullptr) env->DeleteGlobalRef(g_surface);
            if (g_surfaceTexture != nullptr) env->DeleteGlobalRef(g_surfaceTexture);
        }
        g_videoPlayer    = nullptr;
        g_surface        = nullptr;
        g_surfaceTexture = nullptr;
    }

    inline void Tick()
    {
        static int retries = 0;
        static int frames  = 0;

        if (!g_inited)
        {
            g_inited = true;
            StartVideo();
            if (!g_audioLive) StartAudio();
        }
        else if (!g_videoLive && retries < 3 && ++frames >= 30)
        {
            frames = 0;
            retries++;
            StartVideo();
        }
        UpdateVideo();
    }

    inline void Shutdown()
    {
        StopAudio();
        StopVideo();
    }

    inline bool  VideoReady()   { return g_videoLive && g_targetTex != 0; }
    inline float VideoAspect()
    {
        if (g_videoW <= 0 || g_videoH <= 0) return 16.0f / 9.0f;
        return (float)g_videoW / (float)g_videoH;
    }

#else

    inline void  Tick()      {}
    inline void  Shutdown()  {}
    inline bool  StartAudio(){ return false; }
    inline void  StopAudio() {}
    inline bool  VideoReady(){ return false; }
    inline float VideoAspect(){ return 16.0f / 9.0f; }

#endif

    inline ImTextureID VideoTexture()
    {
#if defined(__ANDROID__)
        return ui_media::VideoReady() ? g_videoTex : nullptr;
#else
        return nullptr;
#endif
    }
}
