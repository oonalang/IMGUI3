#pragma once
// Fake JNI for the loading-media start check. Records what the media layer asked
// the Android framework to do so the test can assert on it.

#include <cstdint>
#include <map>
#include <string>
#include <vector>

typedef int32_t jint;
typedef int32_t jsize;
typedef float   jfloat;
typedef uint8_t jboolean;

#define JNI_TRUE  1
#define JNI_FALSE 0
#define JNI_OK    0

struct _jobject {};
typedef _jobject* jobject;
typedef _jobject* jclass;
typedef _jobject* jstring;
typedef _jobject* jarray;
typedef jarray    jfloatArray;

struct _jmethodID { std::string name; };
typedef _jmethodID* jmethodID;

namespace fakejni
{
    struct State
    {
        int findClass = 0;
        int newObjects = 0;
        int newGlobalRefs = 0;
        std::vector<std::string> classes;
        std::vector<std::string> constructed;
        std::map<std::string, int> voidCalls;
        std::map<std::string, int> intCalls;
        std::string lastString;
        std::vector<std::string> strings;
    };

    inline State& st()
    {
        static State s;
        return s;
    }

    inline void reset()
    {
        st() = State{};
    }
}

struct JNIEnv
{
    jclass FindClass(const char* name)
    {
        fakejni::State& s = fakejni::st();
        s.findClass++;
        s.classes.push_back(name ? name : "");
        return (jclass)(intptr_t)s.classes.size();
    }

    jobject NewGlobalRef(jobject obj)
    {
        fakejni::st().newGlobalRefs++;
        return obj ? obj : (jobject)(intptr_t)0x1000;
    }

    void DeleteGlobalRef(jobject) {}
    void DeleteLocalRef(jobject) {}

    jmethodID GetMethodID(jclass, const char* name, const char*)
    {
        static std::map<std::string, _jmethodID> ids;
        const std::string key = name ? name : "";
        _jmethodID& id = ids[key];
        id.name = key;
        return &id;
    }

    jboolean ExceptionCheck() { return 0; }
    void     ExceptionClear() {}

    jobject NewObject(jclass, jmethodID ctor, ...)
    {
        fakejni::State& s = fakejni::st();
        s.newObjects++;
        s.constructed.push_back(ctor ? ctor->name : "?");
        return (jobject)(intptr_t)(0x2000 + s.newObjects);
    }

    jstring NewStringUTF(const char* text)
    {
        fakejni::State& s = fakejni::st();
        s.lastString = text ? text : "";
        s.strings.push_back(s.lastString);
        return (jstring)(intptr_t)0x3000;
    }

    void CallVoidMethod(jobject, jmethodID mid, ...)
    {
        fakejni::st().voidCalls[mid ? mid->name : "?"]++;
    }

    jint CallIntMethod(jobject, jmethodID mid, ...)
    {
        fakejni::State& s = fakejni::st();
        const std::string name = mid ? mid->name : "";
        s.intCalls[name]++;
        if (name == "getVideoWidth")  return 1280;
        if (name == "getVideoHeight") return 720;
        return 0;
    }

    jfloatArray NewFloatArray(jsize) { return (jfloatArray)(intptr_t)0x4000; }

    void GetFloatArrayRegion(jfloatArray, jsize, jsize len, jfloat* out)
    {
        if (out == nullptr) return;
        static const jfloat identity[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
        for (jsize i = 0; i < len && i < 16; ++i) out[i] = identity[i];
    }

    jsize GetArrayLength(jarray) { return 0; }
};

struct JavaVM
{
    jint AttachCurrentThread(JNIEnv** env, void*)
    {
        static JNIEnv instance;
        if (env != nullptr) *env = &instance;
        return JNI_OK;
    }
    jint DetachCurrentThread() { return JNI_OK; }
};
