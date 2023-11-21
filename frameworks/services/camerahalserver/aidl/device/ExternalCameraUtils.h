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

#ifndef HARDWARE_INTERFACES_CAMERA_DEVICE_DEFAULT_EXTERNALCAMERAUTILS_H_
#define HARDWARE_INTERFACES_CAMERA_DEVICE_DEFAULT_EXTERNALCAMERAUTILS_H_

#include <CameraMetadata.h>
#include <HandleImporter.h>
#include <aidl/android/hardware/camera/common/Status.h>
#include <aidl/android/hardware/camera/device/CaptureResult.h>
#include <aidl/android/hardware/camera/device/ErrorCode.h>
#include <aidl/android/hardware/camera/device/NotifyMsg.h>
#include <aidl/android/hardware/graphics/common/BufferUsage.h>
#include <aidl/android/hardware/graphics/common/PixelFormat.h>
#include <tinyxml2.h>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <android/hardware/graphics/mapper/2.0/IMapper.h>
#include <android/hardware/graphics/mapper/3.0/IMapper.h>
#include <android/hardware/graphics/mapper/4.0/IMapper.h>

using ::aidl::android::hardware::camera::common::Status;
using ::aidl::android::hardware::camera::device::CaptureResult;
using ::aidl::android::hardware::camera::device::ErrorCode;
using ::aidl::android::hardware::camera::device::NotifyMsg;
using ::aidl::android::hardware::graphics::common::BufferUsage;
using ::aidl::android::hardware::graphics::common::PixelFormat;
using ::android::hardware::camera::common::V1_0::helper::CameraMetadata;
using ::android::hardware::camera::common::V1_0::helper::HandleImporter;
using ::android::hardware::graphics::mapper::V2_0::IMapper;
using ::android::hardware::graphics::mapper::V2_0::YCbCrLayout;

namespace android {
namespace hardware {
namespace camera {

namespace external {
namespace common {

struct Size {
    int32_t width;
    int32_t height;

    bool operator==(const Size& other) const {
        return (width == other.width && height == other.height);
    }
};

}  // namespace common
}  // namespace external

namespace device {
namespace implementation {

static const uint64_t BUFFER_ID_NO_BUFFER = 0;

// buffers currently circulating between HAL and camera service
// key: bufferId sent via HIDL interface
// value: imported buffer_handle_t
// Buffer will be imported during processCaptureRequest (or requestStreamBuffer
// in the case of HAL buffer manager is enabled) and will be freed
// when the stream is deleted or camera device session is closed
typedef std::unordered_map<uint64_t, buffer_handle_t> CirculatingBuffers;

aidl::android::hardware::camera::common::Status importBufferImpl(
        /*inout*/ std::map<int, CirculatingBuffers>& circulatingBuffers,
        /*inout*/ HandleImporter& handleImporter, int32_t streamId, uint64_t bufId,
        buffer_handle_t buf,
        /*out*/ buffer_handle_t** outBufPtr);

void freeReleaseFences(std::vector<CaptureResult>&);

}  // namespace implementation
}  // namespace device
}  // namespace camera
}  // namespace hardware
}  // namespace android

#endif  // HARDWARE_INTERFACES_CAMERA_DEVICE_DEFAULT_EXTERNALCAMERAUTILS_H_
