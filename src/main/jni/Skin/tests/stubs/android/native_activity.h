#pragma once

#include <jni.h>
#include <stdint.h>
#include "native_window.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ANativeActivity;
typedef struct ANativeActivity ANativeActivity;

struct ANativeActivityCallbacks;

typedef struct ANativeActivity {
    struct ANativeActivityCallbacks *callbacks;
    JavaVM   *vm;
    JNIEnv   *env;
    jobject   clazz;
    const char *internalDataPath;
    const char *externalDataPath;
    int32_t   sdkVersion;
    void     *instance;
    void     *assetManager;
    const char *obbPath;
} ANativeActivity;

void ANativeActivity_finish(ANativeActivity *activity);
void ANativeActivity_setWindowFlags(ANativeActivity *activity, uint32_t addFlags,
                                    uint32_t removeFlags);
void ANativeActivity_showSoftInput(ANativeActivity *activity, uint32_t flags);
void ANativeActivity_hideSoftInput(ANativeActivity *activity, uint32_t flags);

#ifdef __cplusplus
}
#endif

