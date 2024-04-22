/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *  @author   huijie huang
 *  @version  1.0
 *  @date     2019/05/06
 *  @par function description:
 *  - screen capture
 *  - screen record
 *  @warning This class may explode in your face.
 *  @note If you inherit anything from this class, you're doomed.
 */
#define LOG_TAG "ScreenControlClient"

#include <utils/Log.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <android-base/logging.h>
#include <android/hidl/memory/1.0/IMemory.h>
#include <hidlmemory/mapping.h>

#include "ScreenControlClient.h"

using ::android::hidl::memory::V1_0::IMemory;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::hardware::mapMemory;
using ::android::hardware::Void;
using ::vendor::amlogic::hardware::screencontrol::V1_0::Result;

namespace android {

ScreenControlClient *ScreenControlClient::mInstance = NULL;

ScreenControlClient::ScreenControlClient()
{
    sp<IScreenControl> ctrl = IScreenControl::tryGetService();
    while (ctrl == nullptr) {
         usleep(200*1000);//sleep 200ms
         ctrl = IScreenControl::tryGetService();
         ALOGE("tryGet screen control daemon Service");
    };


    ALOGI("ScreenControlClient :%p",this);
    mScreenCtrl = std::move(ctrl);
}

ScreenControlClient::~ScreenControlClient()
{
    ALOGI("~ScreenControlClient :%p",this);
    if (mInstance != NULL)
        delete mInstance;
}

ScreenControlClient *ScreenControlClient::getInstance()
{
    if (NULL == mInstance)
         mInstance = new ScreenControlClient();
    return mInstance;
}



int ScreenControlClient::startScreenCapBuffer(int32_t left, int32_t top, int32_t right, int32_t bottom,
        int32_t width, int32_t height, int32_t sourceType, void **buffer, int *bufSize)
{
    Mutex::Autolock autoLock(mScreenCapLock);
    int result = -1;
    ALOGI("enter %s,left=%d,top=%d,right=%d,bottom=%d,width=%d,height=%d,srctype=%d",
            __func__, left, top, right, bottom, width, height, sourceType);
    mScreenCtrl->startScreenCapBuffer(left, top, right, bottom, width, height, sourceType,
        [&](const int32_t &ret, const hidl_memory &mem){
            if (ret == 0) {
                sp<IMemory> memory = mapMemory(mem);
                *bufSize = memory->getSize();
                *buffer = new uint8_t[*bufSize];
                memcpy(*buffer, memory->getPointer(), *bufSize);
                ALOGI("get memory, size=%d", *bufSize);
            }
            result = ret;
        });

    return result;
}

int ScreenControlClient::startScreenRecord(int32_t left, int32_t top, int32_t right, int32_t bottom, int32_t width, int32_t height, int32_t frameRate,
    int32_t bitRate, int32_t limitTimeSec, int32_t sourceType, const char* filename)
{
    Mutex::Autolock autoLock(mLock);
    int result = -1;
    ALOGI("enter %s,left=%d,top=%d,right=%d,bottom=%d, width=%d,height=%d,rate=%d,bitrate=%d,timesec=%d,srctype=%d,filename=%s",
        __func__, left, top, right, bottom, width, height, frameRate, bitRate, limitTimeSec, sourceType, filename);
    if (Result::OK == mScreenCtrl->startScreenRecord(left, top, right, bottom, width, height, frameRate,
                                                        bitRate, limitTimeSec, sourceType, filename))
        result = 0;
    return result;
}

int ScreenControlClient::startScreenRecord(int32_t width, int32_t height, int32_t frameRate,
                        int32_t bitRate, int32_t limitTimeSec, int32_t sourceType, const char* filename)
{
    return startScreenRecord(0,0,width,height,width, height, frameRate,
                                bitRate, limitTimeSec, sourceType, filename);
}

int ScreenControlClient::startAvcScreenRecord(int32_t left, int32_t top, int32_t right, int32_t bottom, int32_t width,
                                int32_t height, int32_t frameRate,int32_t bitRate, int32_t sourceType)
{
    Mutex::Autolock autoLock(mLock);
    Mutex::Autolock autoLock1(mScreenCapLock);
    int result = -1;
    if (!mScreenControlHidlCallback)
        mScreenControlHidlCallback = new ScreenControlHidlCallback(this);
    Return<void> ret = mScreenCtrl->setCallback(mScreenControlHidlCallback);
    if (!ret.isOk()) {
        ALOGE("Failed to setCallback %s", ret.description().c_str());
    }
    ALOGI("enter %s,left=%d,top=%d,right=%d,bottom=%d, width=%d,height=%d,rate=%d,bitrate=%d,srctype=%d",
            __func__, left, top, right, bottom, width, height, frameRate, bitRate, sourceType);
    if (Result::OK == mScreenCtrl->startAvcRecord(left, top, right, bottom, width, height,
                                                        frameRate,bitRate, sourceType))
        result = 0;
    return result;
}

int ScreenControlClient::startAvcScreenRecord(int32_t width, int32_t height, int32_t frameRate,int32_t bitRate, int32_t sourceType)
{
    return startAvcScreenRecord(0,0,width,height,width,height,frameRate,bitRate,sourceType);
}

int ScreenControlClient::startYuvScreenRecord(int32_t left, int32_t top, int32_t right, int32_t bottom, int32_t width,
                                int32_t height, int32_t frameRate, int32_t sourceType) {
    Mutex::Autolock autoLock(mLock);
    Mutex::Autolock autoLock1(mScreenCapLock);
    int result = -1;
     if (!mScreenControlHidlCallback)
        mScreenControlHidlCallback = new ScreenControlHidlCallback(this);
    Return<void> ret = mScreenCtrl->setCallback(mScreenControlHidlCallback);
    if (!ret.isOk()) {
        ALOGE("Failed to setCallback %s", ret.description().c_str());
    }
    ALOGI("enter %s,left=%d,top=%d,right=%d,bottom=%d, width=%d,height=%d,rate=%d,srctype=%d",
            __func__, left, top, right, bottom, width, height, frameRate, sourceType);
    if (Result::OK == mScreenCtrl->startYuvRecord(left, top, right, bottom, width, height,
                                                    frameRate, sourceType))
        result = 0;
    return result;
}

int ScreenControlClient::startYuvScreenRecord(int32_t width, int32_t height, int32_t frameRate, int32_t sourceType)
{
    return startYuvScreenRecord(0,0,width,height,width,height,frameRate,sourceType);

}

int32_t ScreenControlClient::startMicroDim(int32_t width, int32_t height)
{
    Mutex::Autolock autoLock(mLock);
    Mutex::Autolock autoLock1(mScreenCapLock);
    int result = -1;
    if (!mScreenControlHidlCallback)
        mScreenControlHidlCallback = new ScreenControlHidlCallback(this);
    Return<void> ret = mScreenCtrl->setCallback(mScreenControlHidlCallback);
    if (!ret.isOk()) {
        ALOGE("Failed to setCallback %s", ret.description().c_str());
    }
    ALOGI("enter %s, width=%d,height=%d",__func__, width, height);
    if (Result::OK == mScreenCtrl->startMicroDim(width, height))
        result = 0;
    return result;
}

void ScreenControlClient::setAvcCallback(const sp<AvcRecordCallback>&f) {
    ALOGI("setAvcCallback :%p",f.get());
    mAvcCb = f;
}

void ScreenControlClient::setYuvCallback(const sp<YuvRecordCallback>&f) {
    ALOGI("setYuvCallback :%p",f.get());
    mYuvCb = f;
}

void ScreenControlClient::setMicroDimCallback(const sp<MicroDimCallback>&f) {
    ALOGI("setMicroDimCallback :%p",f.get());
    mMicroDimCb = f;
}
void ScreenControlClient::forceStop()
{
    ALOGD("[%s %d]", __FUNCTION__, __LINE__);
    mScreenCtrl->forceStop();
}

void ScreenControlClient::setExtraInt32Config(const std::map<std::string, int32_t>& config) {
    Mutex::Autolock autoLock(mLock);
    if (config.size() <= 0)
        return;
    hidl_vec<hidl_string> keys;
    keys.resize(config.size());
    hidl_vec<int32_t> values;
    values.resize(config.size());
    int32_t i = 0;
    for (auto it = config.begin(); it != config.end(); it++,i++) {
        keys[i] = it->first;
        values[i] = it->second;
        ALOGI("setExtraInt32Config  keys[%d]:%s,values[%d]:%d ",i,keys[i].c_str(),values[i]);
    }
    mScreenCtrl->setExtraInt32Config(keys,values);
}

Return<void> ScreenControlClient::ScreenControlHidlCallback::onAvcDataArouse(const hidl_memory &mem,int32_t size, int32_t frame_type,int64_t pts)
{
    ALOGI("onAvcDataArouse size = %d,frame_type=%d,pts = %ld",size, frame_type, pts);
    sp<AvcRecordCallback> f = mScrCtrlClient->mAvcCb.promote();
    if (f != nullptr) {
        sp<IMemory> memory = mapMemory(mem);
        f->onAvcDataArouse(memory->getPointer(),size,frame_type,pts);
    }
    return Void();

}
Return<void> ScreenControlClient::ScreenControlHidlCallback::onYuvDataArouse(const hidl_memory &mem,int32_t size)
{
    ALOGI("onYuvDataArouse size = %d",size);
    if (!mScrCtrlClient)
        return Void();
    sp<YuvRecordCallback> f = mScrCtrlClient->mYuvCb.promote();
    if (f != nullptr) {
        sp<IMemory> memory = mapMemory(mem);
        f->onYuvDataArouse(memory->getPointer(),size);
    }
    return Void();
}

Return<void> ScreenControlClient::ScreenControlHidlCallback::onMicroDimArouse(const hidl_memory &mem,int32_t size) {
    ALOGI("onMicroDimArouse size = %d",size);
    sp<MicroDimCallback> f = mScrCtrlClient->mMicroDimCb.promote();
    if (f != nullptr) {
        sp<IMemory> memory = mapMemory(mem);
        f->onMicroDimArouse(memory->getPointer(),size);
    }
    return Void();

}

ScreenControlClient::ScreenControlHidlCallback::ScreenControlHidlCallback(ScreenControlClient *client):
                            mScrCtrlClient(client) {
    ALOGI("ScreenControlHidlCallback :%p",this);
};
ScreenControlClient::ScreenControlHidlCallback::~ScreenControlHidlCallback() {
    ALOGI("~ScreenControlHidlCallback :%p",this);

}




}

