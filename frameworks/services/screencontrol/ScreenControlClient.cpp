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
using ::android::hardware::mapMemory;

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

    mScreenCtrl = ctrl;
    mRecordType = RECORD_TYPE_TS;
}

ScreenControlClient::~ScreenControlClient()
{
    if (mInstance != NULL)
        delete mInstance;
}

ScreenControlClient *ScreenControlClient::getInstance()
{
    if (NULL == mInstance)
         mInstance = new ScreenControlClient();
    return mInstance;
}

void *ScreenControlClient::ThreadWrapper(void *me) {
      ScreenControlClient *client = static_cast<ScreenControlClient *>(me);
      client->threadFunc();
      return NULL;
}
void ScreenControlClient::threadFunc(){
    if (mRecordType == RECORD_TYPE_AVC) {
        while (!checkAvcRecordDone()) {
            void *buffer = NULL;
            int bufferSize = 0;
            uint8_t frameType = 0;
            int64_t pts = 0;
            if (getAvcRecordData((void **)&buffer, &bufferSize,&frameType,&pts)) {
                sp<AvcCallback> f = mAvcCb.promote();
                if ( f != NULL )
                    f->onAvcDataArouse(buffer,bufferSize,frameType, pts);
                delete [] buffer;
            }
            usleep(5*1000);
        }
        sp<AvcCallback> f = mAvcCb.promote();
        if (f != NULL)
            f->onAvcDataOver();
        return;
    }else if (mRecordType == RECORD_TYPE_YUV) {
         while (!checkYuvRecordDone()) {
            void *buffer = NULL;
            int bufferSize = 0;
            if (getYuvRecordData((void **)&buffer, &bufferSize)) {
                sp<YuvCallback> f = mYuvCb.promote();
                if ( f != NULL )
                    f->onYuvDataArouse(buffer,bufferSize);
                delete [] buffer;
            }
            usleep(5*1000);
        }
        sp<YuvCallback> f = mYuvCb.promote();
        if (f != NULL)
            f->onYuvDataOver();
        return;
    }
}

void ScreenControlClient::setAvcCallback(const sp<AvcCallback>&f) {
        mAvcCb=f;
}
void ScreenControlClient::setYuvCallback(const sp<YuvCallback>&f) {
        mYuvCb=f;
}

int ScreenControlClient::startScreenRecord(int32_t width, int32_t height, int32_t frameRate,
    int32_t bitRate, int32_t limitTimeSec, int32_t sourceType, const char* filename)
{
    Mutex::Autolock autoLock(mLock);
    int result = -1;
    ALOGI("enter %s,width=%d,height=%d,rate=%d,bitrate=%d,timesec=%d,srctype=%d,filename=%s",
        __func__, width, height, frameRate, bitRate, limitTimeSec, sourceType, filename);
    if (Result::OK == mScreenCtrl->startScreenRecord(width, height, frameRate,
        bitRate, limitTimeSec, sourceType, filename))
        result = 0;
    return result;
}

int ScreenControlClient::startScreenRecord(int32_t left, int32_t top, int32_t right, int32_t bottom, int32_t width, int32_t height, int32_t frameRate,
    int32_t bitRate, int32_t limitTimeSec, int32_t sourceType, const char* filename)
{
    Mutex::Autolock autoLock(mLock);
    int result = -1;
    ALOGI("enter %s,left=%d,top=%d,right=%d,bottom=%d, width=%d,height=%d,rate=%d,bitrate=%d,timesec=%d,srctype=%d,filename=%s",
        __func__, left, top, right, bottom, width, height, frameRate, bitRate, limitTimeSec, sourceType, filename);
    if (Result::OK == mScreenCtrl->startScreenRecordByCrop(left, top, right, bottom, width, height, frameRate,
        bitRate, limitTimeSec, sourceType, filename))
        result = 0;
    return result;
}

int ScreenControlClient::startScreenCap(int32_t left, int32_t top, int32_t right, int32_t bottom,
    int32_t width, int32_t height, int32_t sourceType, const char* filename)
{
    Mutex::Autolock autoLock(mLock);
    int result = -1;
    ALOGI("enter %s,left=%d,top=%d,right=%d,bottom=%d,width=%d,height=%d,srctype=%d,filename=%s",
        __func__, left, top, right, bottom, width, height, sourceType, filename);

    if (Result::OK == mScreenCtrl->startScreenCap(left, top, right, bottom, width,
        height, sourceType, filename))
        result = 0;
    return result;
}

int ScreenControlClient::startScreenCapBuffer(int32_t left, int32_t top, int32_t right, int32_t bottom,
        int32_t width, int32_t height, int32_t sourceType, void **buffer, int *bufSize)
{
    Mutex::Autolock autoLock(mLock);
    int result = -1;
    ALOGI("enter %s,left=%d,top=%d,right=%d,bottom=%d,width=%d,height=%d,srctype=%d",
            __func__, left, top, right, bottom, width, height, sourceType);
    mScreenCtrl->startScreenCapBuffer(left, top, right, bottom, width, height, sourceType,
        [&](const Result &ret, const hidl_memory &mem){
            if (Result::OK == ret) {
                sp<IMemory> memory = mapMemory(mem);
                *bufSize = memory->getSize();
                *buffer = new uint8_t[*bufSize];
                memcpy(*buffer, memory->getPointer(), *bufSize);
                ALOGI("get memory, size=%d", *bufSize);
                result = 0;
            }
        });

    return result;
}

int ScreenControlClient::startAvcScreenRecord(int32_t width, int32_t height, int32_t frameRate,
    int32_t bitRate, int32_t sourceType)
{
    Mutex::Autolock autoLock(mLock);
    int result = -1;
    ALOGI("enter %s,width=%d,height=%d,rate=%d,bitrate=%d,srctype=%d",
        __func__, width, height, frameRate, bitRate, sourceType);
    if (Result::OK == mScreenCtrl->startAvcRecord(width, height, frameRate,bitRate, sourceType))
        result = 0;
    mRecordType = RECORD_TYPE_AVC;
    pthread_t thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&thread, &attr, ThreadWrapper, this);
    pthread_attr_destroy(&attr);
    return result;
}
bool ScreenControlClient::getAvcRecordData(void **buffer, int *bufSize, uint8_t *frameType, int64_t *pts)
{
//    ALOGI("enter %s", __func__);
    Mutex::Autolock autoLock(mLock);
    bool success = false;
    mScreenCtrl->getAvcRecordData([&](const Result &ret, const hidl_memory &mem, const int32_t realMemSize,const uint8_t naltype ,const int64_t nowtime){
    if ( Result::OK == ret ) {
        sp<IMemory> memory = mapMemory(mem);
        *bufSize = realMemSize;
        *buffer = new uint8_t[*bufSize];
        memcpy(*buffer, memory->getPointer(), *bufSize);
        *frameType = naltype;
        *pts = nowtime;
        success = true;
      }
    });

  return success;
}

bool ScreenControlClient::checkAvcRecordDone()
{
     Mutex::Autolock autoLock(mLock);
    bool result = false;
//  ALOGI("enter %s", __func__);
    if (Result::OK == mScreenCtrl->checkAvcRecordDone())
      result = true;
    return result;
}



int ScreenControlClient::startYuvScreenRecord(int32_t width, int32_t height, int32_t frameRate, int32_t sourceType)
{
    Mutex::Autolock autoLock(mLock);
    int result = -1;
    //ALOGI("enter %s,width=%d,height=%d,srctype=%d",
    //    __func__, width, height, sourceType);
    if (Result::OK == mScreenCtrl->startYuvRecord(width, height, frameRate, sourceType))
      result = 0;
    mRecordType = RECORD_TYPE_YUV;
    pthread_t thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&thread, &attr, ThreadWrapper, this);
    pthread_attr_destroy(&attr);
    return result;
}

bool ScreenControlClient::getYuvRecordData(void **buffer, int *bufSize)
{
    Mutex::Autolock autoLock(mLock);
    //ALOGI("enter %s", __func__);

    bool success = false;
    mScreenCtrl->getYuvRecordData([&](const Result &ret, const hidl_memory &mem, const int32_t realMemSize){
      if (Result::OK == ret) {
        sp<IMemory> memory = mapMemory(mem);
        *bufSize = realMemSize;
        *buffer = new uint8_t[*bufSize];
        memcpy(*buffer, memory->getPointer(), *bufSize);
        success = true;
      }
    });

    return success;
}


bool ScreenControlClient::checkYuvRecordDone()
{
    Mutex::Autolock autoLock(mLock);
    bool result = false;
    //ALOGI("enter %s", __func__);
    if (Result::OK == mScreenCtrl->checkYuvRecordDone())
      result = true;
    return result;
}

void ScreenControlClient::forceStop()
{
    mScreenCtrl->forceStop();
}

}

