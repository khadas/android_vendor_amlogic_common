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

#define LOG_TAG "screencontrol"
#define LOG_NDEBUG 0

#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <cutils/properties.h>
#include <fcntl.h>
#include <hidl/HidlBinderSupport.h>
#include <hidl/HidlLazyUtils.h>
#include <hidl/HidlTransportSupport.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <utils/Log.h>
#include "ScreenControlHal.h"
#include "ScreenControlService.h"

using namespace android;
using ::android::hardware::configureRpcThreadpool;
using android::hardware::LazyServiceRegistrar;
using ::vendor::amlogic::hardware::screencontrol::V1_0::IScreenControl;
using ::vendor::amlogic::hardware::screencontrol::V1_0::implementation::ScreenControlHal;

int main() {
    ALOGI("screen_control daemon starting");
    bool vendorTreble = property_get_bool("persist.vendor.screencontrol.treble", false);
    if (vendorTreble) {
        ALOGI("screen_control init with vndbinder");
        android::ProcessState::initWithDriver("/dev/vndbinder");
    }
    ALOGI("screen_control daemon starting in %s mode", vendorTreble ? "treble" : "lazy");
    configureRpcThreadpool(4, false);
    sp<ProcessState> proc(ProcessState::self());
    sp<ScreenControlService> service = ScreenControlService::getInstance();
    if (vendorTreble) {
        sp<IScreenControl> screen = new ScreenControlHal(service);
        if (screen == nullptr) {
            ALOGE("Cannot create IScreenControl service");
        } else if (screen->registerAsService() != OK) {
            ALOGE("Cannot register IScreenControl service.");
        } else {
            ALOGI("Treble IScreenControl service created.");
        }
    } else {
        android::status_t ret =
            LazyServiceRegistrar::getInstance().registerService(new ScreenControlHal(service), "default");
        if (ret != android::OK) {
            ALOGE("Couldn't register screen_control service!");
        }
    }
    IPCThreadState::self()->joinThreadPool();
}
