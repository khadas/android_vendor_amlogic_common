/*
 * Copyright (C) 2016 The Android Open Source Project
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
 *  @author   Tellen Yu
 *  @version  1.0
 *  @date     2017/9/25
 *  @par function description:
 *  - 1 droidlogic hdmi cec daemon
 */

#define LOG_TAG "hdmicecd"

#include <hidl/HidlTransportSupport.h>

#include "DroidHdmiCec.h"

using namespace android;

using ::vendor::amlogic::hardware::hdmicec::V1_0::implementation::DroidHdmiCec;
using ::vendor::amlogic::hardware::hdmicec::V1_0::IDroidHdmiCEC;


int main(int argc __unused, char** argv __unused)
{
    ALOGI("hdmi cec daemon starting in treble mode.");
    ::android::hardware::configureRpcThreadpool(4, false);

    sp<IDroidHdmiCEC>hidlHdmiCec = new DroidHdmiCec();

    if (hidlHdmiCec->registerAsService() != OK) {
        ALOGE("Cannot register IDroidHdmiCEC service.");
        return 1;
    }

    ALOGI("Treble IDroidHdmiCEC service registered.");

    ::android::hardware::joinRpcThreadpool();
    return 1; // joinRpcThreadpool should never return
}

