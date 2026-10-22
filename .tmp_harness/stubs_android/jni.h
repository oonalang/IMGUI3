#pragma once
#include <cstdint>
typedef int32_t jint;
typedef int32_t jsize;
typedef float  jfloat;
typedef double jdouble;
typedef uint8_t jboolean;
#define JNI_TRUE 1
#define JNI_FALSE 0
#define JNI_OK 0
struct _jobject {};
struct _jclass : _jobject {};
struct _jstring : _jobject {};
typedef _jobject* jobject;
typedef _jclass* jclass;
typedef _jstring* jstring;
struct _jarray : _jobject {};
typedef _jarray* jarray;
typedef _jarray* jfloatArray;
struct _jfieldID;
struct _jmethodID;
typedef _jfieldID* jfieldID;
typedef _jmethodID* jmethodID;
struct JNIEnv
{
    jclass   FindClass(const char*) { return nullptr; }
    jobject  NewGlobalRef(jobject) { return nullptr; }
    void     DeleteGlobalRef(jobject) {}
    void     DeleteLocalRef(jobject) {}
    jmethodID GetMethodID(jclass, const char*, const char*) { return nullptr; }
    jboolean  ExceptionCheck() { return 0; }
    void      ExceptionClear() {}
    jobject   NewObject(jclass, jmethodID, ...) { return nullptr; }
    jstring   NewStringUTF(const char*) { return nullptr; }
    void      CallVoidMethod(jobject, jmethodID, ...) {}
    jint      CallIntMethod(jobject, jmethodID, ...) { return 0; }
    jfloatArray NewFloatArray(jsize) { return nullptr; }
    void      GetFloatArrayRegion(jfloatArray, jsize, jsize, jfloat*) {}
    jsize     GetArrayLength(jarray) { return 0; }
};
struct JavaVM
{
    jint AttachCurrentThread(JNIEnv**, void*) { return JNI_OK; }
    jint DetachCurrentThread() { return JNI_OK; }
};
