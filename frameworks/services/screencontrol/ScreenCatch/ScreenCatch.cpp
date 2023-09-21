/*
**
** Copyright 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/
//#define LOG_NDEBUG 0
#define LOG_TAG "ScreenCatch"
#include <utils/Log.h>
#include <cutils/properties.h>
#include <ui/GraphicBuffer.h>
#include "am_gralloc_ext.h"
#include "DisplayAdapter.h"
#include "../ScreenControlDebug.h"
#include "ScreenCatch.h"

namespace android {

#define PROP_KEYSTONE "persist.vendor.hwc.keystone"

//////////////////////////////  screen capture when use keystone  //////////////////////////////

static int32_t gralloc_unref_dma_buf(native_handle_t * hnd) {
    static GraphicBufferMapper & maper = GraphicBufferMapper::get();

    bool bfreed = false;
    if (am_gralloc_is_valid_graphic_buffer(hnd)) {
        if (NO_ERROR == maper.freeBuffer(hnd)) {
            bfreed = true;
        }
    }

    if (bfreed == false) {
        /*may be we got handle not alloc by gralloc*/
        native_handle_close(hnd);
        native_handle_delete(hnd);
    }

    return 0;
}

static int32_t gralloc_lock_dma_buf(
    native_handle_t * handle, void** vaddr) {
    static GraphicBufferMapper & maper = GraphicBufferMapper::get();
    uint32_t usage = GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN;
    int w = am_gralloc_get_width(handle);
    int h = am_gralloc_get_height(handle);

    Rect r(w, h);
    if (NO_ERROR == maper.lock(handle, usage, r, vaddr))
        return 0;

    ALOGE("lock buffer failed\n");
    return -EINVAL;
}

static int32_t gralloc_unlock_dma_buf(native_handle_t * handle) {
    static GraphicBufferMapper & maper = GraphicBufferMapper::get();
    if (NO_ERROR == maper.unlock(handle))
        return 0;
    return -EINVAL;
}

static inline void rgb24_to_rgb32(unsigned char *src, unsigned char *dist, int srcWidth, int srcHeight)
{
    int srcIdx = 0, dstIdx = 0;
    int size = srcWidth * srcHeight * 3;
    for (;srcIdx < size; srcIdx+=3, dstIdx+=4) {
        memmove(&dist[dstIdx], &src[srcIdx], 3);
        dist[dstIdx+4] = 0xff;
    }
}

ScreenCatch::ScreenCatch():
            mScreenManager(nullptr),
            mStart(false),
            mRawBufferSize(0),
            mClientId(-1) {
    ALOGI("[%s %d] Construct", __FUNCTION__, __LINE__);
    ScreenControlDebug::initDebug();
    mOutputQueue.clear();

}

ScreenCatch::~ScreenCatch() {
    ALOGI("~ScreenCatch");
}
bool ScreenCatch::start(std::unique_ptr<InputParmeter>& input) {
    std::lock_guard<std::mutex> lock(mLock);
    int32_t size = 0;
    char keystone[256] = {0};
    if (mStart) {
        ALOGE("[%s %d] it has been started", __FUNCTION__, __LINE__);
        return false;
    }
    if (input->source_type < AML_CAPTURE_VIDEO || input->source_type > SCAML_CAPTURE_UNKNOWN) {
        ALOGE("[%s %d] dont't support the type=%d", __FUNCTION__, __LINE__,input->source_type);
        return false;
    }
    ALOGI("[%s %d]  ScreenManager start finish source_type = %d (%d/%d)", __FUNCTION__, __LINE__,
                input->source_type,input->size->width(),input->size->height());
    if (property_get(PROP_KEYSTONE, keystone, "") > 0 && strlen(keystone) > 0 )
        return captureforKeystone()?true:false;

    mScreenManager = ScreenManager::getInstance();
    auto screenInput = std::make_unique<InputParmeter>();
    screenInput->source_type = input->source_type;
    screenInput->format = SCREENCONTROL_PIX_FMT_RGBA888;
    screenInput->frame_rate = 1;
    size = input->size->width() * input->size->height() * 4;
    screenInput->size = std::move(input->size);
    screenInput->area = std::move(input->area);
    bool ret = mScreenManager->start(screenInput, this, &mClientId, false);
    if (!ret) {
        ALOGE("[%s %d] ScreenManager start fail!", __FUNCTION__, __LINE__);
        return false;
    }
    ALOGI("[%s %d]  ScreenManager start finish mClientId=%d", __FUNCTION__, __LINE__, mClientId);
    mRawBufferSize = size;
    mStart =true;
    return true;
}
bool ScreenCatch::stop() {
    std::lock_guard<std::mutex> lock(mLock);
    if (!mStart) {
        ALOGE("[%s %d] the ScreenCatch has been started !", __FUNCTION__, __LINE__);
        return false;
    }
    mScreenManager->stop(mClientId);
    while (!mOutputQueue.empty()) {
        auto output = mOutputQueue.begin();
        if (!(*output)->raw) {
            ALOGV("[%s %d] the buffer is not legal", __FUNCTION__, __LINE__);
            mOutputQueue.erase(output);
            continue;
        }
        if (mClientId > 0) {
            delete [](*output)->raw;
        }
        mOutputQueue.erase(output);
    }
    mStart = false;
    mScreenManager = nullptr;
    mRawBufferSize = 0;
    mClientId = -1;
    return true;
}

bool ScreenCatch::readBuffer(uint8_t* buffer, int32_t* size) {
    std::lock_guard<std::mutex> lock(mLock);
    if (!mStart || mOutputQueue.empty()) {
        ALOGV("[%s %d] the ScreenCatch has been started or mOutputQueue don't have any buffer ", __FUNCTION__, __LINE__);
        return false;
    }
    auto output = mOutputQueue.begin();
    if (!(*output)->raw) {
        ALOGV("[%s %d] the buffer is not legal", __FUNCTION__, __LINE__);
        mOutputQueue.erase(mOutputQueue.begin());
        return false;
    }
    memcpy(buffer,(*output)->raw,mRawBufferSize);
    *size = mRawBufferSize;
    if (mClientId > 0) {
        delete [](*output)->raw;
    }else
        mScreenManager->realseBuffer(mClientId,(*output)->index);
    mOutputQueue.erase(mOutputQueue.begin());
    ALOGD("[%s %d] get the buffer size = %d", __FUNCTION__, __LINE__,mRawBufferSize);
    return true;
}

bool ScreenCatch::captureforKeystone() {
    const native_handle_t *outBufferHandle = nullptr;
    native_handle_t *bufferHandle = nullptr;
    int width=0, height=0, format=0, stride=0;
    std::unique_ptr<meson::DisplayAdapter> displayAdapter = meson::DisplayAdapterCreateRemote();
    if (!displayAdapter) {
        ALOGE("DisplayAdapter init failed");
        return false;
    }
    if ((displayAdapter->captureDisplayScreen(&outBufferHandle))
                && (nullptr != outBufferHandle)) {
        void* mapBase = nullptr;
        bufferHandle = const_cast<native_handle_t*> (outBufferHandle);
        width = am_gralloc_get_width(bufferHandle);
        height = am_gralloc_get_height(bufferHandle);
        format = am_gralloc_get_format(bufferHandle);
        stride = am_gralloc_get_stride_in_pixel(bufferHandle);
        mRawBufferSize = stride * height * 4;
        ALOGD("[%s %d]mDisplayAdapter get width=%d, height=%d, format=%d, stride=%d, bufSize=%d",
            __func__, __LINE__, width, height, format, stride, mRawBufferSize);
        if (!gralloc_lock_dma_buf(bufferHandle, &mapBase)) {
            unsigned char* buffer = (unsigned char*)malloc(mRawBufferSize);
            if (!buffer)
                return false;
            rgb24_to_rgb32((unsigned char*)mapBase, buffer, width, height);
            free(buffer);
            gralloc_unlock_dma_buf(bufferHandle);
            gralloc_unref_dma_buf(bufferHandle);
        } else {
            ALOGE("lock mem failed");
            return false;
        }

    } else {
        ALOGE("captureDisplayScreen failed");
        return false;
    }
    return true;


}

void ScreenCatch::PictureReady(const OutputRecord &output) {
    ALOGI("PictureReady index =%d ",output.index);
    if (!mStart || output.format != SCREENCONTROL_PIX_FMT_RGBA888 || !output.raw_buffer ||
            output.raw_buffer_size <= 0 || output.raw_buffer_size > mRawBufferSize) {
        ALOGE("[%s %d] the format is not RGBA888 or the buffer is wrong ,size = %d", __FUNCTION__, __LINE__,output.raw_buffer_size);
        return;
    }
    auto info = std::make_unique<OutputInfo>(output.raw_buffer,output.index);

    mOutputQueue.push_back(std::move(info));
}

};