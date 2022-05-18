/*
 * Copyright (C) 2006 The Android Open Source Project
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
 */

#ifndef ANDROID_DROIDLOGIC_SCREENCONTROL_V1_0_SCREENCONTROLHAL_H
#define ANDROID_DROIDLOGIC_SCREENCONTROL_V1_0_SCREENCONTROLHAL_H


#include <vendor/amlogic/hardware/screencontrol/1.0/IScreenControl.h>
#include <vendor/amlogic/hardware/screencontrol/1.0/types.h>
#include "ScreenControlService.h"
#include <utils/Mutex.h>
#include "ScreenControlH264.h"
namespace vendor {
namespace amlogic {
namespace hardware {
namespace screencontrol {
namespace V1_0 {
namespace implementation {
    using ::vendor::amlogic::hardware::screencontrol::V1_0::IScreenControl;
    using ::vendor::amlogic::hardware::screencontrol::V1_0::Result;
    using ::android::hardware::hidl_string;
    using ::android::hardware::Return;
    using ::android::hardware::Void;
    using ::android::sp;
    using ::android::ScreenControlService;

class ScreenControlHal : public IScreenControl {
    public:
        ScreenControlHal();
        ~ScreenControlHal();

        Return<Result> startScreenRecord(int32_t width, int32_t height, int32_t frameRate, int32_t bitRate, int32_t limitTimeSec, int32_t sourceType, const hidl_string& filename) override;
        Return<Result> startScreenRecordByCrop(int32_t left, int32_t top, int32_t right, int32_t bottom, int32_t width, int32_t height,
                                                     int32_t frameRate, int32_t bitRate, int32_t limitTimeSec, int32_t sourceType, const hidl_string& filename) override;

        Return<Result> startScreenCap(int32_t left, int32_t top, int32_t right, int32_t bottom, int32_t width, int32_t height, int32_t sourceType, const hidl_string& filename) override;

        Return<void> startScreenCapBuffer(int32_t left, int32_t top, int32_t right, int32_t bottom, int32_t width, int32_t height, int32_t sourceType, startScreenCapBuffer_cb _hidl_cb);

        Return<void> forceStop();
        //YUV record
        Return<Result> startYuvRecord(int32_t width, int32_t height,int32_t frameRate ,int32_t sourceType);

        Return<Result> checkYuvRecordDone();

        Return<void> getYuvRecordData(getYuvRecordData_cb _hidl_cb);

         // H264/AVC record
        Return<Result> startAvcRecord(int32_t width, int32_t height, int32_t frameRate, int32_t bitRate, int32_t sourceType);

        Return<void> getAvcRecordData(getAvcRecordData_cb _hidl_cb);

        Return<Result> checkAvcRecordDone();

    private:
        void handleServiceDeath(uint32_t cookie);
        ScreenControlService* mScreenControl;
        int32_t mYuvRecordWidth;
        int32_t mYuvRecordHeight;
        mutable android::Mutex  mLock;
        int32_t mAvcRecordWidth;
        int32_t mAvcRecordHeight;
        int32_t mAvcRecordFramerate;
        int32_t mAvcRecordBitrate;
        int32_t mAvcRecordSourceType;
        class  DeathRecipient : public android::hardware::hidl_death_recipient  {
            public:
                DeathRecipient(sp<ScreenControlHal> sch);

                // hidl_death_recipient interface
                void serviceDied(uint64_t cookie,
                    const ::android::wp<::android::hidl::base::V1_0::IBase>& who) override;
            private:
                sp<ScreenControlHal> mScreenControlHal;
        };
        sp<DeathRecipient> mDeathRecipient;
    };//ScreenControl
} //namespace implementation
}//namespace V1_0
} //namespace screencontrol
}//namespace hardware
} //namespace android
} //namespace vendor

#endif // ANDROID_DROIDLOGIC_SCREENCONTROL_V1_0_SCREENCONTROLHAL_H
