/*
 * Copyright (C) 2022 The Android Open Source Project
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

#ifndef HARDWARE_INTERFACES_CAMERA_PROVIDER_DEFAULT_AMLOGICCAMERAPROVIDER_H_
#define HARDWARE_INTERFACES_CAMERA_PROVIDER_DEFAULT_AMLOGICCAMERAPROVIDER_H_

#include <memory>
#include <ExternalCameraUtils.h>
#include <SimpleThread.h>
#include <aidl/android/hardware/camera/common/CameraDeviceStatus.h>
#include <aidl/android/hardware/camera/common/TorchModeStatus.h>
#include <aidl/android/hardware/camera/common/VendorTagSection.h>
#include <aidl/android/hardware/camera/device/ICameraDevice.h>
#include <aidl/android/hardware/camera/provider/BnCameraProvider.h>
#include <aidl/android/hardware/camera/provider/CameraIdAndStreamCombination.h>
#include <aidl/android/hardware/camera/provider/ConcurrentCameraIdCombination.h>
#include <aidl/android/hardware/camera/provider/ICameraProviderCallback.h>
#include <poll.h>
#include <utils/Mutex.h>
#include <utils/Thread.h>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include "utils/SortedVector.h"
#include "hardware/camera_common.h"
#include "CameraModule.h"

namespace android {
namespace hardware {
namespace camera {
namespace provider {
namespace implementation {

using ::aidl::android::hardware::camera::common::CameraDeviceStatus;
using ::aidl::android::hardware::camera::common::VendorTagSection;
using ::aidl::android::hardware::camera::common::TorchModeStatus;
using ::aidl::android::hardware::camera::device::ICameraDevice;
using ::aidl::android::hardware::camera::provider::BnCameraProvider;
using ::aidl::android::hardware::camera::provider::CameraIdAndStreamCombination;
using ::aidl::android::hardware::camera::provider::ConcurrentCameraIdCombination;
using ::aidl::android::hardware::camera::provider::ICameraProviderCallback;
using ::android::hardware::camera::common::helper::SimpleThread;
//using ::android::hardware::camera::external::common::ExternalCameraConfig;
using ::android::hardware::camera::common::helper::CameraModule;
using ::android::sp;

class AmlogicCameraProvider : public BnCameraProvider, public camera_module_callbacks_t {
  public:
    AmlogicCameraProvider();
    ~AmlogicCameraProvider() override;
    bool isInitFailed() { return mInitFailed; }

    ndk::ScopedAStatus setCallback(
            const std::shared_ptr<ICameraProviderCallback>& in_callback) override;
    ndk::ScopedAStatus getVendorTags(std::vector<VendorTagSection>* _aidl_return) override;
    ndk::ScopedAStatus getCameraIdList(std::vector<std::string>* _aidl_return) override;
    ndk::ScopedAStatus getCameraDeviceInterface(
            const std::string& in_cameraDeviceName,
            std::shared_ptr<ICameraDevice>* _aidl_return) override;
    ndk::ScopedAStatus notifyDeviceStateChange(int64_t in_deviceState) override;
    ndk::ScopedAStatus getConcurrentCameraIds(
            std::vector<ConcurrentCameraIdCombination>* _aidl_return) override;
    ndk::ScopedAStatus isConcurrentStreamCombinationSupported(
            const std::vector<CameraIdAndStreamCombination>& in_configs,
            bool* _aidl_return) override;

  private:
    SortedVector<std::string> mCameraIds;
    Mutex mLock;
    std::shared_ptr<ICameraProviderCallback> mCallbacks = nullptr;

    std::shared_ptr<CameraModule> mModule;

    int mNumberOfLegacyCameras;
    std::map<std::string, CameraDeviceStatus> mCameraStatusMap;  // camera id -> status
    std::map<std::string, bool> mOpenLegacySupported;
    SortedVector<std::pair<std::string, std::string>> mCameraDeviceNames;

    //int mPreferredHal3MinorVersion;

    bool mInitFailed;
    bool initialize();

    std::vector<VendorTagSection> mVendorTagSections;
    //bool setUpVendorTags();
    int checkCameraVersion(int id, camera_info info);

    // create HIDL device name from camera ID and legacy device version
    std::string getAidlDeviceName(std::string cameraId, int deviceVersion);

   // extract legacy camera ID/device version from a HIDL device name
    static std::string getLegacyCameraId(const std::string& deviceName);

        // static callback forwarding methods
    static void sCameraDeviceStatusChange(
        const struct camera_module_callbacks* callbacks,
        int camera_id,
        int new_status);
    static void sTorchModeStatusChange(
        const struct camera_module_callbacks* callbacks,
        const char* camera_id,
        int new_status);

    void addDeviceNames(int camera_id, CameraDeviceStatus status = CameraDeviceStatus::PRESENT,
                        bool cam_new = false);
    void removeDeviceNames(int camera_id);


};

}  // namespace implementation
}  // namespace provider
}  // namespace camera
}  // namespace hardware
}  // namespace android

#endif  // HARDWARE_INTERFACES_CAMERA_PROVIDER_DEFAULT_AMLOGICCAMERAPROVIDER_H_
