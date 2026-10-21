#pragma once

#include <jni.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ANativeWindow;
typedef struct ANativeWindow ANativeWindow;

struct ANativeWindow_Buffer {
    int32_t width;
    int32_t height;
    int32_t stride;
    int32_t format;
    void   *bits;
    uint32_t reserved[6];
};

int32_t ANativeWindow_getWidth(ANativeWindow *window);
int32_t ANativeWindow_getHeight(ANativeWindow *window);
int32_t ANativeWindow_getFormat(ANativeWindow *window);
int32_t ANativeWindow_setBuffersGeometry(ANativeWindow *window, int32_t width,
                                         int32_t height, int32_t format);
int32_t ANativeWindow_lock(ANativeWindow *window, struct ANativeWindow_Buffer *outBuffer,
                           void *inOutDirtyBounds);
int32_t ANativeWindow_unlockAndPost(ANativeWindow *window);
void    ANativeWindow_acquire(ANativeWindow *window);
void    ANativeWindow_release(ANativeWindow *window);

#ifdef __cplusplus
}
#endif

