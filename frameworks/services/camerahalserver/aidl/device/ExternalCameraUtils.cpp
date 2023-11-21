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

#define LOG_TAG "ExtCamUtils"
// #define LOG_NDEBUG 0

#include "ExternalCameraUtils.h"

#include <aidlcommonsupport/NativeHandle.h>
#include <jpeglib.h>
#include <linux/videodev2.h>
#include <log/log.h>
#include <algorithm>
#include <cinttypes>
#include <cmath>

#define HAVE_JPEG  // required for libyuv.h to export MJPEG decode APIs
#include <libyuv.h>

namespace android {
namespace hardware {
namespace camera {

namespace device {
namespace implementation {

aidl::android::hardware::camera::common::Status importBufferImpl(
        /*inout*/ std::map<int, CirculatingBuffers>& circulatingBuffers,
        /*inout*/ HandleImporter& handleImporter, int32_t streamId, uint64_t bufId,
        buffer_handle_t buf,
        /*out*/ buffer_handle_t** outBufPtr) {

    using ::aidl::android::hardware::camera::common::Status;
    if (buf == nullptr && bufId == BUFFER_ID_NO_BUFFER) {
        ALOGE("%s: bufferId %" PRIu64 " has null buffer handle!", __FUNCTION__, bufId);
        return Status::ILLEGAL_ARGUMENT;
    }

    CirculatingBuffers& cbs = circulatingBuffers[streamId];
    if (cbs.count(bufId) == 0) {

        if (buf == nullptr) {
            ALOGE("%s: bufferId %" PRIu64 " has null buffer handle!", __FUNCTION__, bufId);
            return Status::ILLEGAL_ARGUMENT;
        }
        // Register a newly seen buffer
        buffer_handle_t importedBuf = buf;
        handleImporter.importBuffer(importedBuf);

        if (importedBuf == nullptr) {
            ALOGE("%s: output buffer for stream %d is invalid!", __FUNCTION__, streamId);
            return Status::INTERNAL_ERROR;
        } else {
            cbs[bufId] = importedBuf;
        }
    }

    *outBufPtr = &cbs[bufId];
    return Status::OK;
}


void freeReleaseFences(std::vector<CaptureResult>& results) {
    for (auto& result : results) {
        native_handle_t* inputReleaseFence =
                ::android::makeFromAidl(result.inputBuffer.releaseFence);
        if (inputReleaseFence != nullptr) {
            native_handle_close(inputReleaseFence);
            native_handle_delete(inputReleaseFence);
        }
        for (auto& buf : result.outputBuffers) {
            native_handle_t* outReleaseFence = ::android::makeFromAidl(buf.releaseFence);
            if (outReleaseFence != nullptr) {
                native_handle_close(outReleaseFence);
                native_handle_delete(outReleaseFence);
            }
        }
    }
}

}  // namespace implementation
}  // namespace device
}  // namespace camera
}  // namespace hardware
}  // namespace android