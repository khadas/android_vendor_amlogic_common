#define LOG_NDEBUG 0

#include <fcntl.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <android/log.h>

#include "AmlNativeSubRender.h"

#include "MyLog.h"
#include "SubtitleContext.h"

char *gMemory;

static int32_t lock(SubNativeRenderHnd hnd, SubNativeRenderBuffer *buf) {
    if (buf == nullptr) {
        ALOGE("Error! no buffer!");
        return -1;
    }
    buf->format = 1;
    buf->height = 1080;
    buf->stride  = buf->width = 1920;
    if (gMemory == nullptr) {
        gMemory = (char *)malloc(1920*1080*4);
    }
    buf->bits = gMemory;

    return 0;
}

static int32_t unlockAndPost(SubNativeRenderHnd hnd) {
    return 0;
}

static int32_t setBuffersGeometry(SubNativeRenderHnd hnd, int w, int h, int format) {
    return 0;
}


int main(int argc __unused, char** argv __unused) {
    SubNativeRenderCallback callback;
    callback.lock = lock;
    callback.unlockAndPost = unlockAndPost;
    callback.setBuffersGeometry = setBuffersGeometry;


    aml_RegisterNativeWindowCallback(callback);

    SubtitleContext::GetInstance().addSubWindow(1, nullptr);
    SubtitleContext::GetInstance().startPlaySubtitle(1, nullptr);

    while(1) usleep(10000);
    return 0;
}
