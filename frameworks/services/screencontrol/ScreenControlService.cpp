/** @file ScreenControlService.cpp
 *  @par Copyright:
 *  - Copyright 2011 Amlogic Inc as unpublished work
 *  All Rights Reserved
 *  - The information contained herein is the confidential property
 *  of Amlogic.  The use, copying, transfer or disclosure of such information
 *  is prohibited except by express written agreement with Amlogic Inc.
 *  @author   liangzhuo xie
 *  @version  1.0
 *  @date     2018/08/18
 *  @par function description:
 *  - screen capture
 *  - screen record
 *  @warning This class may explode in your face.
 *  @note If you inherit anything from this class, you're doomed.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "ScreenControlService"
#include <utils/Log.h>
#include<string.h>
#include <utils/String16.h>
#include <hidl/HidlLazyUtils.h>
#include <hidl/HidlBinderSupport.h>
#include "ScreenControlService.h"



#define TIMEOUT_VAL  5 * 1000 * 1000 //2s




using ::android::hidl::base::V1_0::IBase;

namespace android {

static void microdimming(uint8_t *s, uint8_t *dest,int32_t W,int32_t H, int32_t w_count,int32_t h_count)
{
    ALOGE("[%s %d]", __FUNCTION__, __LINE__);
    int32_t c_width = w_count;
    int32_t c_height = h_count;
    int32_t map = 0;
    int32_t i=0;
    int32_t j=0;
    int32_t m=0;
    int32_t n=0;
    int32_t sum;
    int32_t k;
    uint8_t* d = (uint8_t *)dest;

    int32_t mFrameHeight;
    int32_t mFrameWidth;
    int32_t count_m = 0;
    int32_t pixcount = 0;

    mFrameWidth = W/c_width;
    mFrameHeight = H/c_height;
    memset(dest, 0x00, c_width*c_height);
    ALOGE("[W:%d H:%d,mW:%d mH:%d,c_height:%d,c_width:%d]",W,H, mFrameWidth, mFrameHeight,c_height,c_width);

    for (i = 0; i < c_height; i++) {
            for (j = 0;j < c_width; j++) {
                    sum = 0;
                    pixcount = 0;
                    for (m = 0; m < mFrameHeight ; m = m +4) {
                            for (n = 0; n < mFrameWidth ; n = n +4) {
                                    map = (m + i * mFrameHeight)* W + j * mFrameWidth + n;
                                    k = s[map /8 *8] * 1.1 + 3;
                                    if (k > 255)
                                            k = 255;
                                    sum = sum + k;
                                    pixcount++;
                            }
                    }
                    if (pixcount > 0)
                        sum=sum / pixcount;
                    *(d++) = (uint8_t)sum;
                    //memset(d++, (unsigned char)sum, sizeof(unsigned char) );
            }


    }
    //memset(dest+(c_width*c_height), 0x80, (c_width*c_height) / 2);
}

ScreenControlService::ScreenControlService():
                    mStart(false),
                    mWaitStop(false),
                    mMicroWidth(0),
                    mMicroHeight(0),
                    mYuvRecordId(-1),
                    mConvertor(nullptr),
                    mScreenManager(nullptr),
                    mEncoderFormat(nullptr) {
                   ALOGI("ScreenControlService :%p",this);
}

ScreenControlService::~ScreenControlService() {
    ALOGI("~ScreenControlService :%p",this);
    if (mEncoderFormat) {
        AMediaFormat_delete(mEncoderFormat);
        mEncoderFormat = nullptr;
    }
}

ScreenControlService* ScreenControlService::getInstance() {
    ScreenControlService *mScreenControl = new ScreenControlService();
    return mScreenControl;
}


void ScreenControlService::setListener(const sp<ScreenControlNotify>& listener) {
    ALOGI("setListener ");
    mNotifyListener = listener;
}

void ScreenControlService::forceStop() {
    ALOGI("forceStop()");

    mStart = false;
    if (mMicroWidth > 0)
        mMicroWidth = 0;
     if (mMicroHeight > 0)
        mMicroHeight = 0;
    if (mConvertor) {
        mConvertor->stop();
        mConvertor = nullptr;
    }
    {
        if (mScreenManager) {
            mScreenManager->setCallback(mYuvRecordId,nullptr);
            mScreenManager->stop(mYuvRecordId);
            mScreenManager = nullptr;
            mYuvRecordId = -1;
        }
    }

}

int32_t ScreenControlService::startScreenCapBuffer(int32_t left, int32_t top, int32_t right, int32_t bottom, int32_t width,
                                                int32_t height, int32_t sourceType, void *dstBuffer, int32_t *dstBufferSize) {
    ALOGI("[%s] left:%d, top:%d, right:%d, bottom:%d, width:%d, height:%d, sourceType:%d\n",
                __func__, left, top, right, bottom, width, height, sourceType);
    std::lock_guard<std::mutex> alock(mScreenCapLock);
    int result = 0;
    auto size = std::make_unique<Size>(width,height);
    auto area = std::make_unique<Area>(left,top,right,bottom);
    auto parmeter = std::make_unique<InputParmeter>();
    parmeter->size = std::move(size);
    parmeter->area = std::move(area);
    parmeter->source_type = sourceType;
    if (mScreenCatch && (*parmeter == *mScreenCapParmeter)) {
        mScreenCatch->resume();
    } else {
        if (mScreenCatch) {
            mScreenCatch->stop();
            mScreenCatch = nullptr;
        }
        mScreenCatch = std::make_unique<ScreenCatch>();
        mScreenCapParmeter = std::make_unique<InputParmeter>(*parmeter);
        if (!dstBuffer || !mScreenCatch->start(parmeter)) {
            ALOGE("[%s %d] ScreenCatch start fail !! dstBuffer=%p", __FUNCTION__, __LINE__,dstBuffer);
            return AML_ERROR_CODE_OTHER;
        }
    }
    int64_t firsetNowUs = getNowTimesUs();;
    while (!mScreenCatch->readBuffer((uint8_t*)dstBuffer,dstBufferSize)) {
        int64_t nowUs = getNowTimesUs();
        int32_t event = mScreenCatch->getErrorEvent();
        if (event == AML_ENEVENT_HDCP_LIMIT) {
            result = AML_ERROR_CODE_HDCP_LIMIT;
            goto exit;
        }

        if ((nowUs - firsetNowUs) >= TIMEOUT_VAL) {
            ALOGE("[%s %d] no data !!!! break,firsetNowUs=%lld,nowUs=%lld", __FUNCTION__, __LINE__,firsetNowUs,nowUs);
            result = AML_ERROR_CODE_TIMEOUT;
            goto exit;
        }
        usleep(5 *1000);
    }
    ALOGI("[%s %d] readed buffer size = %d", __FUNCTION__, __LINE__,*dstBufferSize);
exit:
    if (result == 0) {
        mScreenCatch->pause();
    } else {
        mScreenCatch->stop();
        mScreenCapParmeter = nullptr;
        mScreenCatch == nullptr;
    }
    return result;
}

void ScreenControlService::stopScreenCapBuffer() {
    std::unique_lock<std::mutex> alock(mScreenCapLock);
    ALOGI("stopScreenCapBuffer()");
    if (!mScreenCatch)
        return;
    mScreenCatch->stop();
    mScreenCatch = nullptr;
    mScreenCapParmeter = nullptr;
    if (mWaitStop)
        mScreenCapCondition.notify_one();
}

int32_t ScreenControlService::startScreenRecord(int32_t left, int32_t top, int32_t right, int32_t bottom, int32_t width, int32_t height,
                                int32_t frameRate, int32_t bitRate,int32_t limitTimeSec, int32_t sourceType, const char* filename) {
    ALOGI("[%s] left:%d, top:%d, right:%d, bottom:%d, width:%d, height:%d, sourceType:%d,frameRate=%d,bitRate=%d,limitTimeSec=%d",
                __func__, left, top, right, bottom, width, height, sourceType,frameRate,bitRate,limitTimeSec);
    std::lock_guard<std::mutex> alock(mLock);
    int32_t video_dump_size = 0;
    int64_t mFirstPts = 0;
    int32_t isOk = OK;
    int32_t fd = -1;
    std::unique_lock<std::mutex> sl(mScreenCapLock);
    if (mScreenCatch) {
        mWaitStop = true;
        mScreenCapCondition.wait(sl);
        mWaitStop = false;
    }
    if (!filename) {
        ALOGE("[%s %d] the filename is null", __FUNCTION__, __LINE__);
        return !OK;
    }

    if (strcmp(filename, "testLoopTsRecord") != 0) {
        fd = open(filename, O_CREAT | O_RDWR, 0666);
        if (fd < 0 ) {
            ALOGE("[%s %d] the file : %s can't open  reason:%s", __FUNCTION__, __LINE__,filename,strerror(errno));
            return !OK;
        }
    }
    std::unique_ptr<TSPacker> tspacker = std::make_unique<TSPacker>();
    auto parmeter = std::make_unique<ESConvertorParmeter>();
    parmeter->size = std::make_unique<Size>(width,height);
    parmeter->area = std::make_unique<Area>(left,top,right,bottom);
    parmeter->source_type = sourceType;
    parmeter->frame_rate = frameRate;
    parmeter->bit_rate_ = bitRate;
    bool ret = tspacker->start(parmeter,mEncoderFormat);
    AMediaFormat_delete(mEncoderFormat);
    mEncoderFormat = nullptr;
    if (!ret) {
        ALOGE("[%s %d] TSPacker start fail !!", __FUNCTION__, __LINE__);
        if (fd > 0)
            close(fd);
        return !OK;
    }
    mStart = true;
    int64_t firsetNowUs = getNowTimesUs();
    sl.unlock();
    while (mStart) {
        uint8_t * buffer = nullptr;
        int32_t size = 0;
        int64_t pts = 0;
        ret = tspacker->readBuffer(&buffer,&size,&pts);
        if (!ret || !buffer || size <= 0 || pts <= 0) {
            int64_t nowUs = getNowTimesUs();
            int64_t diff = nowUs -firsetNowUs;
            int64_t limitTimeUs = (int64_t)limitTimeSec *1000 *1000;
            if (video_dump_size == 0 && (diff >= TIMEOUT_VAL)) {
                ALOGE("[%s %d] no data !!!! break", __FUNCTION__, __LINE__);
                isOk = !OK;
                break;
            }
            usleep(5 * 1000);//5ms
            continue;
        }
        if (mFirstPts == 0)
            mFirstPts = pts;
        int64_t diffPts = pts - mFirstPts;
        if (fd > 0)
            write(fd, buffer, size);
        delete []buffer;
        video_dump_size += size;
       ALOGI("[%s %d] video dump_size = %d,pts = %lld,diffPts=%lld", __FUNCTION__, __LINE__,size,pts,diffPts);
        if (diffPts >= (int64_t)limitTimeSec * 1000 * 1000)
            break;

    }
    ALOGD("[%s %d] tspacker stop", __FUNCTION__, __LINE__);
    sl.lock();
    tspacker->stop();
    if (fd > 0)
        close(fd);
    mStart = false;
    sl.unlock();
    ALOGD("[%s %d] record finish", __FUNCTION__, __LINE__);
    return isOk;

}
int32_t ScreenControlService::startAvcRecord(int32_t left, int32_t top, int32_t right, int32_t bottom, int32_t width, int32_t height,
                            int32_t frameRate, int32_t bitRate, int32_t sourceType) {
    ALOGI("[%s] left:%d, top:%d, right:%d, bottom:%d, width:%d, height:%d, sourceType:%d,frameRate=%d,bitRate=%d",
                __func__, left, top, right, bottom, width, height, sourceType,frameRate,bitRate);
    std::lock_guard<std::mutex> alock(mLock);
    std::unique_lock<std::mutex> lock(mScreenCapLock);
    int32_t ret = OK;
    if (mScreenCatch) {
        mWaitStop = true;
        mScreenCapCondition.wait(lock);
        mWaitStop = false;
    }
    mConvertor = std::make_unique<ESConvertor>();
    auto parmeter = std::make_unique<ESConvertorParmeter>();
    parmeter->size = std::make_unique<Size>(width,height);
    parmeter->area = std::make_unique<Area>(left,top,right,bottom);
    parmeter->source_type = sourceType;
    parmeter->frame_rate = frameRate;
    parmeter->bit_rate_ = bitRate;
    if (!mConvertor->start(parmeter,this,mEncoderFormat)) {
        ALOGE("[%s %d] ESConvertor start fail", __FUNCTION__, __LINE__);
        ret = !OK;
    }
    AMediaFormat_delete(mEncoderFormat);
    mEncoderFormat = nullptr;
    mStart = true;
    return ret;
}

void ScreenControlService::onEsBufferAvailable(void* const data, int32_t size, int32_t frame_type, int64_t pts) {
    sp<ScreenControlNotify> cb = mNotifyListener.promote();
    if (cb) {
        VDLog("onEsBufferAvailable mNotifyListener\n");
        cb->onEsBufferAvailable(data, size, frame_type, pts);
    }
}

int32_t ScreenControlService::startYuvRecord(int32_t left, int32_t top, int32_t right, int32_t bottom,
                    int32_t width, int32_t height, int32_t frameRate, int32_t sourceType) {
    ALOGI("[%s] left:%d, top:%d, right:%d, bottom:%d, width:%d, height:%d, sourceType:%d,frameRate=%d",
                __func__, left, top, right, bottom, width, height, sourceType,frameRate);
    std::lock_guard<std::mutex> alock(mLock);
    std::unique_lock<std::mutex> sl(mScreenCapLock);
    if (mScreenCatch) {
        mWaitStop = true;
        mScreenCapCondition.wait(sl);
        mWaitStop = false;
    }

    if (mScreenManager) {
        ALOGE("[%s %d] the screen manger is recording now !!!", __FUNCTION__, __LINE__);
        return !OK;
    }

    mScreenManager = ScreenManager::getInstance();
    auto screenInput = std::make_unique<InputParmeter>();
    screenInput->source_type = sourceType;
    screenInput->format = SCREENCONTROL_PIX_FMT_NV21;
    screenInput->frame_rate = frameRate;
    screenInput->size = std::make_unique<Size>(width,height);
    screenInput->area = std::make_unique<Area>(left,top,right,bottom);

    bool ret = mScreenManager->start(screenInput,this,&mYuvRecordId);
    if (!ret) {
        ALOGE("[%s %d] ScreenManager start fail!", __FUNCTION__, __LINE__);
        return !OK;
    }
    mStart = true;
    return OK;
}

int32_t ScreenControlService::startMicroDim(int32_t width, int32_t height) {
    ALOGI("[%s] width:%d, height:%d", __func__, width, height);
    std::lock_guard<std::mutex> sl(mScreenCapLock);
    if (!startYuvRecord(0, 0, 1280, 720, 1280, 720, 1, AML_CAPTURE_VIDEO))
        return !OK;
    mMicroWidth = width;
    mMicroHeight = height;
    return OK;
}

void ScreenControlService::setExtreConfig(AMediaFormat *format) {
    std::lock_guard<std::mutex> alock(mLock);
    if (!format)
        return;
    if (mEncoderFormat) {
        AMediaFormat_delete(mEncoderFormat);
        mEncoderFormat = nullptr;
    }
    mEncoderFormat = format;
    ALOGD("[%s %d] mEncoderFormat: %s", __FUNCTION__, __LINE__, AMediaFormat_toString(mEncoderFormat));
}

void ScreenControlService::PictureReady(const OutputRecord &output) {
    int id = mYuvRecordId;
    sp<ScreenControlNotify> cb = mNotifyListener.promote();
    if (cb) {
        VDLog("PictureReady mNotifyListener\n");
        if (mMicroWidth > 0 && mMicroHeight > 0) {
            uint8_t* buffer = new uint8_t[mMicroWidth * mMicroHeight];
            microdimming((uint8_t*)output.raw_buffer, buffer, 1280, 720, mMicroWidth, mMicroHeight);
            cb->onMicroDimAvailable(buffer, mMicroWidth * mMicroHeight);
            delete []buffer;
        }else
            cb->onYuvBufferAvailable(output.raw_buffer, output.raw_buffer_size);
    }
    if (id == 0) {
        if (mScreenManager)
            mScreenManager->realseBuffer(id, output.index);
    } else
        delete []output.raw_buffer;


}

void ScreenControlService::EventNotify(int32_t event) {

}

} // namespace android
