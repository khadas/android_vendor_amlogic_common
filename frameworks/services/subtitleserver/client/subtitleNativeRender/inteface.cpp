#define LOG_TAG "SubtitleControl-api"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>


#include "AmlNativeSubRender.h"

#include "MyLog.h"
#include "SubtitleContext.h"

static SubNativeRenderCallback gRenderCallback;


// ========= Render API, Canvas manipulate wrapper.=====================
// the API wrappers are const globally, so can directly use global variable.
int32_t NativeWindowLock(SubNativeRenderHnd hnd, SubNativeRenderBuffer *buf) {
    if (gRenderCallback.lock == nullptr) {
        ALOGE("Error! NativeWindow callback function not initialize properly!");
        return -1;
    }

    return gRenderCallback.lock(hnd, buf);
}

int32_t NativeWindowUnlockAndPost(SubNativeRenderHnd hnd) {
    if (gRenderCallback.unlockAndPost == nullptr) {
        ALOGE("Error! NativeWindow callback function not initialize properly!");
        return -1;
    }

    return gRenderCallback.unlockAndPost(hnd);
}
int32_t NativeWindowSetBuffersGeometry(SubNativeRenderHnd hnd, int w, int h, int format) {
    if (gRenderCallback.setBuffersGeometry == nullptr) {
        ALOGE("Error! NativeWindow callback function not initialize properly!");
        return -1;
    }

    return gRenderCallback.setBuffersGeometry(hnd, w, h, format);
}


int64_t NativeWindowGetPts(int sessionId) {
    if (gRenderCallback.getPts == nullptr) {
        ALOGE("Error! NativeWindow callback function not initialize properly!");
        return -1;
    }

    return gRenderCallback.getPts(sessionId);
}
// ========= Render API END =========================





/** Regist global nativewindow callback to client
 *
 *  native window can only handle by app. client common api can register callback to call.
 */
bool aml_RegisterNativeWindowCallback(SubNativeRenderCallback cb) {
    gRenderCallback = cb;
    return true;
}

bool aml_AttachSurfaceWindow(int sessionId, SubNativeRenderHnd win) {
    SubtitleContext::GetInstance().addSubWindow(sessionId, win);

    return true;
}

bool aml_SetSubtitleSessionEnabled(int sessionId, bool enabled) {
    if (enabled) {
        SubtitleContext::GetInstance().startPlaySubtitle(sessionId, "kor");
    } else {
        SubtitleContext::GetInstance().stopPlaySubtitle(sessionId);
    }
    return true;
}

bool aml_RegisterQtoneDataCb(int sessionId, QToneDataCallback cb) {
    return SubtitleContext::GetInstance().registerQtoneDataCb(sessionId, cb);
}

bool aml_GetSubtitleSessionEnabled(int sessionId) {
    return false;
}


extern int AmlUrl2Fd(const char *url);
/* if play external subtitle, need set url */
bool aml_SetSubtitleURI(int sessionId, const char*url) {

    // got fd, no need close here, when finish use, it will auto close it.
    int fd = AmlUrl2Fd(url);
    SubtitleContext::GetInstance().setupExternalFd(sessionId, fd);
    /* NOTE: should not close fd here, fd will auto-closed when finish using */

    return false;
}
