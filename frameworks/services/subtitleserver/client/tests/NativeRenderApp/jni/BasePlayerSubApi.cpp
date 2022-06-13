#define LOG_TAG "SubtitleControl-api"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <dlfcn.h>
 #include <time.h>
 #include <android/log.h>

#include "BasePlayerNativeInterface_v2.h"
#define ALOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#define ALOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#define ALOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))


#include "AmlNativeSubRender.h"

typedef bool (*AmlRegisterNativeWindowCbFn)(SubNativeRenderCallback cb);
typedef bool (*AmlAttachSurfaceWindowFn)(int sessionId, SubNativeRenderHnd hnd);
typedef bool (*AmlSetSubtitleEnableFn)(int sessionId, bool enabled);
typedef bool (*AmlGetSubtitleEnableFn)(int sessionId);
typedef bool (*AmlSetSubtitleSmiUrlFn)(int sessionId, char* url);
typedef bool (*AmlRegisterQtoneDataCbFn)(int sessionId, QToneDataCallback cb);

AmlRegisterNativeWindowCbFn AmlRegisterNativeWindowCb = nullptr;
AmlAttachSurfaceWindowFn AmlAttachSurfaceWindow = nullptr;
AmlSetSubtitleEnableFn AmlSetSubtitleEnable = nullptr;
AmlGetSubtitleEnableFn AmlGetSubtitleEnable = nullptr;
AmlSetSubtitleSmiUrlFn AmlSetSubtitleSmiUrl = nullptr;
AmlRegisterQtoneDataCbFn AmlRegisterQtoneDataCb = nullptr;


// ================= Navtive Window API Wrapper Start =====================
static int32_t lock(SubNativeRenderHnd hnd, SubNativeRenderBuffer *buf) {
    ANativeWindow_Buffer abuf;
    if (buf == nullptr) {
        ALOGE("Error! no buffer!");
        return -1;
    }
    int32_t r = ANativeWindow_lock((ANativeWindow*)hnd, &abuf, 0);
    buf->bits   = abuf.bits;
    buf->format = abuf.format;
    buf->height = abuf.height;
    buf->width  = abuf.width;
    buf->stride = abuf.stride;
    ALOGD("lock: r=%d w%d h:%d", r, abuf.stride, abuf.height);
    return r;
}

static int32_t unlockAndPost(SubNativeRenderHnd hnd) {
    ALOGD("unlockAndPost");
    return ANativeWindow_unlockAndPost((ANativeWindow*)hnd);
}

static int32_t setBuffersGeometry(SubNativeRenderHnd hnd, int w, int h, int format) {
    return ANativeWindow_setBuffersGeometry((ANativeWindow*)hnd, w, h, format);
}


static int64_t systemTime(int clock) {
      static const clockid_t clocks[] = {
              CLOCK_REALTIME,
              CLOCK_MONOTONIC,
              CLOCK_PROCESS_CPUTIME_ID,
              CLOCK_THREAD_CPUTIME_ID,
              CLOCK_BOOTTIME
      };

       struct timespec t;
      t.tv_sec = t.tv_nsec = 0;
      clock_gettime(clocks[clock], &t);
      return int64_t(t.tv_sec)*1000LL + t.tv_nsec/1000000LL;
}

static int64_t test_getPts(int sessionId) {
    static int64_t orig = systemTime(1);

    return systemTime(1) - orig;
}
// ================= Navtive Window API Wrapper END =====================


///QTone CallBack Demo
// after calling this cb, the 'buf' will released, so must copy out!
void QToneDataReceived(int session, const char*buf, int size) {
    ALOGD("QToneDataReceived: %d %p %d", session, buf, size);
}




/**
 *  We can built libraries in system_ext to access some system APIs. however
 *  system libraries cannot access native_window api due to build check.
 *  initialze the system implemens APIs here through dynamic loader
 */
extern "C" void bpni_initialze() {
    void *handle = dlopen("libsubtitlenativerender.so", RTLD_NOW);
    if (handle == nullptr) {
        ALOGE("error load: libsubtitlenativerender.so");
        handle = dlopen("/system_ext/lib/libsubtitlenativerender.so", RTLD_NOW);
    }

    if (handle == nullptr) {
        ALOGE("error load: libsubtitlenativerender.so");
        return;
    }

    AmlRegisterNativeWindowCb = (AmlRegisterNativeWindowCbFn)dlsym(handle, "aml_RegisterNativeWindowCallback");
    AmlAttachSurfaceWindow = (AmlAttachSurfaceWindowFn)dlsym(handle, "aml_AttachSurfaceWindow");
    AmlSetSubtitleEnable = (AmlSetSubtitleEnableFn)dlsym(handle, "aml_SetSubtitleSessionEnabled");
    AmlGetSubtitleEnable = (AmlGetSubtitleEnableFn)dlsym(handle, "aml_GetSubtitleSessionEnabled");
    AmlSetSubtitleSmiUrl = (AmlSetSubtitleSmiUrlFn)dlsym(handle, "aml_SetSubtitleURI");
    AmlRegisterQtoneDataCb = (AmlRegisterQtoneDataCbFn)dlsym(handle, "aml_RegisterQtoneDataCb");

    if ((AmlRegisterNativeWindowCb == nullptr)
        || (AmlAttachSurfaceWindow == nullptr)
        || (AmlSetSubtitleEnable == nullptr)
        || (AmlGetSubtitleEnable == nullptr)
        || (AmlSetSubtitleSmiUrl == nullptr)
        || (AmlRegisterQtoneDataCb == nullptr)) {
        ALOGE("Error, cannot locate api function! ignore");
        return;
    }

    // register native window APIs to subtitle render
    SubNativeRenderCallback callback;
    callback.lock = lock;
    callback.unlockAndPost = unlockAndPost;
    callback.setBuffersGeometry = setBuffersGeometry;
    callback.getPts = test_getPts;
    AmlRegisterNativeWindowCb(callback);

    return;
}


/**
 * Set ANativeWindow surface for drawing ClosedCaption
 *
 * @param[in] handleId handle ID.
 * @param[in] window pointer of ANativeWindow surface for CC
 */
void bpni_setSubtitleSurfaceNativeWindow(BPNI_PLAYER_HANDLE_ID handleId, ANativeWindow* window) {
    ALOGD("bpni_setSubtitleSurfaceNativeWindow w:%d h:%d f:%d",
        ANativeWindow_getWidth(window),
        ANativeWindow_getHeight(window),
        ANativeWindow_getFormat(window)
    );

    if (AmlAttachSurfaceWindow == nullptr) {
        ALOGE("Error! native render not initialzed properly! exit!");
        return;
    }

    AmlAttachSurfaceWindow(handleId, window);
    AmlRegisterQtoneDataCb(handleId, QToneDataReceived);
}

/**
 * Subtitle (Closed Caption / VoD SMI subtitle)  On / Off
 *
 * @param[in] handleId handle ID.
 * @param[in] enabled true = On , false = Off
 */
void bpni_setSubtitleEnabled(BPNI_PLAYER_HANDLE_ID handleId, bool enabled) {
    if (AmlSetSubtitleEnable == nullptr) {
        ALOGE("Error! native render not initialzed properly! exit!");
        return;
    }

    // TEST
    //bpni_setSubtitleSmiUrl(handleId, (char *)("https://gitlab.marum.de/apirek/video.js/-/raw/v5.13.1/docs/examples/elephantsdream/captions.en.vtt"));

    //bpni_setSubtitleSmiUrl(handleId, (char *)("http://10.18.29.98/lpqj_KOR.smi"));


    AmlSetSubtitleEnable(handleId, enabled);
}

/**
 * Return Subtitle On or Off status
 *
 * @param[in] handleId handle ID.
 * @return true:On, false: Off
 */
bool bpni_getSubtitleEnabled(BPNI_PLAYER_HANDLE_ID handleId) {
    if (AmlGetSubtitleEnable == nullptr) {
        ALOGE("Error! native render not initialzed properly! exit!");
        return false;
    }

    return AmlGetSubtitleEnable(handleId);
}

/**
 * Set SMI URL for VoD
 *
 * @param[in] handleId handle ID.
 * @param[in] url SMI subtitle URL
 */
void bpni_setSubtitleSmiUrl(BPNI_PLAYER_HANDLE_ID handleId, char* url) {
    if (AmlSetSubtitleSmiUrl == nullptr) {
        ALOGE("Error! native render not initialzed properly! exit!");
        return;
    }

    AmlSetSubtitleSmiUrl(handleId, url);
}

