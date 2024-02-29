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

#ifndef ANDROID_SCREENCONTROL_SERVICE_H
#define ANDROID_SCREENCONTROL_SERVICE_H

#include <utils/RefBase.h>
#include <utils/threads.h>
#include "ScreenCatch/ScreenCatch.h"
#include "Media2Ts/tspack.h"
#include "ScreenManager.h"



namespace android {

enum aml_screencontrol_error_code {
    AML_ERROR_CODE_HDCP_LIMIT = 1,
    AML_ERROR_CODE_TIMEOUT = 2,
    AML_ERROR_CODE_OTHER = 3,
};


class ScreenControlNotify : virtual public RefBase
{
public:
    ScreenControlNotify() {}
    virtual ~ScreenControlNotify(){}
    virtual void onEsBufferAvailable(void* data, int32_t size, int32_t frame_type, int64_t pts) = 0;
    virtual void onYuvBufferAvailable(void* data, int32_t size) = 0;
    virtual void onMicroDimAvailable(void* data, int32_t size) = 0;
};



class ScreenControlService : public ESConvertor::ESConvertorCallback,
                             public ScreenManager::ScreenMangerCallback {

public:
    static void instantiate();
    static ScreenControlService* getInstance();
    ScreenControlService();
    virtual ~ScreenControlService();
    void setListener(const sp<ScreenControlNotify>& listener);
    void forceStop();
    int32_t startScreenCapBuffer(int32_t left, int32_t top, int32_t right, int32_t bottom,
                        int32_t width, int32_t height, int32_t sourceType, void *dstBuffer, int32_t *dstBufferSize);


    int32_t startScreenRecord(int32_t left, int32_t top, int32_t right, int32_t bottom, int32_t width, int32_t height,
                                int32_t frameRate, int32_t bitRate,int32_t limitTimeSec, int32_t sourceType, const char* filename);

    int32_t startAvcRecord(int32_t left, int32_t top, int32_t right, int32_t bottom, int32_t width, int32_t height,
                            int32_t frameRate, int32_t bitRate, int32_t sourceType);

    int32_t startYuvRecord(int32_t left, int32_t top, int32_t right, int32_t bottom,
                    int32_t width, int32_t height, int32_t frameRate, int32_t sourceType);

    int32_t startMicroDim(int32_t width, int32_t height);

    void onEsBufferAvailable(void* const data, int32_t size, int32_t frame_type, int64_t pts);

    void PictureReady(const OutputRecord &output);

    void EventNotify(int32_t event);

private:
    mutable Mutex mLock;
    bool mStart;
    int32_t mMicroWidth;
    int32_t mMicroHeight;
    int32_t mYuvRecordId;
    wp<ScreenControlNotify> mNotifyListener;
    std::unique_ptr<ESConvertor> mConvertor;
    ScreenManager* mScreenManager;
};

// ----------------------------------------------------------------------------
}; // namespace android
#endif //ANDROID_SCREENCONTROL_SERVICE_H
