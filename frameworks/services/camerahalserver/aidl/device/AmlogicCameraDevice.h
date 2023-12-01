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

#ifndef HARDWARE_INTERFACES_CAMERA_DEVICE_DEFAULT_AMLOGICCAMERADEVICE_H_
#define HARDWARE_INTERFACES_CAMERA_DEVICE_DEFAULT_AMLOGICCAMERADEVICE_H_

#include <AmlogicCameraDeviceSession.h>
#include <aidl/android/hardware/camera/device/BnCameraDevice.h>
#include "CameraModule.h"
#include <utils/Mutex.h>

namespace android {
namespace hardware {
namespace camera {
namespace device {
namespace implementation {

using ::aidl::android::hardware::camera::common::CameraResourceCost;
using ::aidl::android::hardware::camera::device::BnCameraDevice;
using ::aidl::android::hardware::camera::device::CameraMetadata;
using ::aidl::android::hardware::camera::device::ICameraDeviceCallback;
using ::aidl::android::hardware::camera::device::ICameraDeviceSession;
using ::aidl::android::hardware::camera::device::ICameraInjectionSession;
using ::aidl::android::hardware::camera::device::StreamConfiguration;
using ::android::hardware::camera::common::helper::CameraModule;
using ::android::Mutex;

class AmlogicCameraDevice : public BnCameraDevice {
  public:
    // Called by external camera provider HAL.
    // Provider HAL must ensure the uniqueness of CameraDevice object per cameraId, or there could
    // be multiple CameraDevice trying to access the same physical camera.  Also, provider will have
    // to keep track of all CameraDevice objects in order to notify CameraDevice when the underlying
    // camera is detached.
    AmlogicCameraDevice(std::shared_ptr<CameraModule> module, const std::string& cameraId,
                              const SortedVector<std::pair<std::string, std::string>>& cameraDeviceNames);
    ~AmlogicCameraDevice() override;

    ndk::ScopedAStatus getCameraCharacteristics(CameraMetadata* _aidl_return) override;
    ndk::ScopedAStatus getPhysicalCameraCharacteristics(const std::string& in_physicalCameraId,
                                                        CameraMetadata* _aidl_return) override;
    ndk::ScopedAStatus getResourceCost(CameraResourceCost* _aidl_return) override;
    ndk::ScopedAStatus isStreamCombinationSupported(const StreamConfiguration& in_streams,
                                                    bool* _aidl_return) override;
    ndk::ScopedAStatus open(const std::shared_ptr<ICameraDeviceCallback>& in_callback,
                            std::shared_ptr<ICameraDeviceSession>* _aidl_return) override;
    ndk::ScopedAStatus openInjectionSession(
            const std::shared_ptr<ICameraDeviceCallback>& in_callback,
            std::shared_ptr<ICameraInjectionSession>* _aidl_return) override;
    ndk::ScopedAStatus setTorchMode(bool in_on) override;
    ndk::ScopedAStatus turnOnTorchWithStrengthLevel(int32_t in_torchStrength) override;
    ndk::ScopedAStatus getTorchStrengthLevel(int32_t* _aidl_return) override;


    bool isInitFailed() { return mInitFail; }
    //binder_status_t dump(int fd, const char** args, uint32_t numArgs) override;

    // Caller must use this method to check if CameraDevice ctor failed
    //bool isInitFailed();

    // Device version to be used by the external camera provider.
    // Should be of the form <major>.<minor>
    static std::string kDeviceVersion;
// public:
//     camera_device_t* mDevice = nullptr;
protected:
    mutable Mutex mLock;
    const std::shared_ptr<CameraModule> mModule;
    const std::string mCameraId;
    int mCameraIdInt;
    int mDeviceVersion;
    bool mInitFail = false;
    bool mDisconnected;
    std::shared_ptr<AmlogicCameraDeviceSession> mSession = nullptr;
    const SortedVector<std::pair<std::string, std::string>>& mCameraDeviceNames;
    Status initStatus() const;
    static Status getAidlStatus(int status);
    std::shared_ptr<AmlogicCameraDeviceSession> createSession(aml_camera_device_t* device,
        const camera_metadata_t* deviceInfo,
        const std::shared_ptr<ICameraDeviceCallback>& callback) {
            return ndk::SharedRefBase::make<AmlogicCameraDeviceSession>(device, deviceInfo, callback);
    }
};

}  // namespace implementation
}  // namespace device
}  // namespace camera
}  // namespace hardware
}  // namespace android

#endif  // HARDWARE_INTERFACES_CAMERA_DEVICE_DEFAULT_AMLOGICCAMERADEVICE_H_
