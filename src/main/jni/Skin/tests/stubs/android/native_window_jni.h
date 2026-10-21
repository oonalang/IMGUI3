#pragma once

#include <jni.h>
#include "native_window.h"

#ifdef __cplusplus
extern "C" {
#endif

ANativeWindow *ANativeWindow_fromSurface(JNIEnv *env, jobject surface);
void ANativeWindow_acquire(ANativeWindow *window);
void ANativeWindow_release(ANativeWindow *window);

#ifdef __cplusplus
}
#endif

