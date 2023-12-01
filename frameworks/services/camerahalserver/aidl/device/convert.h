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

#ifndef HARDWARE_INTERFACES_CAMERA_DEVICE_DEFAULT_CONVERT_H_
#define HARDWARE_INTERFACES_CAMERA_DEVICE_DEFAULT_CONVERT_H_

#include <aidl/android/hardware/camera/common/Status.h>
#include <aidl/android/hardware/camera/device/BufferStatus.h>
#include <aidl/android/hardware/camera/device/CameraMetadata.h>
#include <aidl/android/hardware/camera/device/HalStream.h>
#include <aidl/android/hardware/camera/device/NotifyMsg.h>
#include <aidl/android/hardware/camera/device/Stream.h>
#include <aidl/android/hardware/camera/device/StreamConfiguration.h>
//#include <hardware/camera3.h>
#include "amlogic_camera.h"
#include <system/camera_metadata.h>


namespace android {
namespace hardware {
namespace camera {
namespace device {
namespace implementation {

struct AmlCameraStream : public aml_camera_stream_t {
    int mId;
};

using ::aidl::android::hardware::camera::common::Status;
using ::aidl::android::hardware::camera::device::BufferStatus;
using ::aidl::android::hardware::camera::device::CameraMetadata;
using ::aidl::android::hardware::camera::device::HalStream;
using ::aidl::android::hardware::camera::device::NotifyMsg;
using ::aidl::android::hardware::camera::device::Stream;
using ::aidl::android::hardware::camera::device::StreamConfiguration;

void convertToAidl(const camera_metadata_t* src, CameraMetadata* dest);
bool convertFromAidl(const CameraMetadata& src, const camera_metadata_t** dst);

void convertFromAidl(const Stream &src, AmlCameraStream* dst);
void convertToAidl(const AmlCameraStream* src, HalStream* dst);

void convertToAidl(const aml_camera_stream_configuration_t& src, StreamConfiguration* dst);
void convertFromAidl(
        buffer_handle_t* bufPtr, BufferStatus status, aml_camera_stream_t* stream, int acquireFence,
        aml_camera_stream_buffer_t* dst);
void convertToAidl(const aml_notify_message_t* src, NotifyMsg* dst);

inline ndk::ScopedAStatus fromStatus(Status status) {
    return status == Status::OK
                   ? ndk::ScopedAStatus::ok()
                   : ndk::ScopedAStatus::fromServiceSpecificError(static_cast<int32_t>(status));
}

}  // namespace implementation
}  // namespace device
}  // namespace camera
}  // namespace hardware
}  // namespace android

#endif  // HARDWARE_INTERFACES_CAMERA_DEVICE_DEFAULT_CONVERT_H_
