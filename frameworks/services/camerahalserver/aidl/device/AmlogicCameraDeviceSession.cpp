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

#define LOG_TAG "ExtCamDevSsn"
// #define LOG_NDEBUG 0
#include <log/log.h>

#include "AmlogicCameraDeviceSession.h"

#include <Exif.h>
#include <aidl/android/hardware/camera/device/CameraBlob.h>
#include <aidl/android/hardware/camera/device/CameraBlobId.h>
#include <aidl/android/hardware/camera/device/ErrorMsg.h>
#include <aidl/android/hardware/camera/device/ShutterMsg.h>
#include <aidl/android/hardware/camera/device/StreamBufferRet.h>
#include <aidl/android/hardware/camera/device/StreamBuffersVal.h>
#include <aidl/android/hardware/camera/device/StreamConfigurationMode.h>
#include <aidl/android/hardware/camera/device/StreamRotation.h>
#include <aidl/android/hardware/camera/device/StreamType.h>
#include <aidl/android/hardware/graphics/common/Dataspace.h>
#include <aidlcommonsupport/NativeHandle.h>
#include <convert.h>
#include <linux/videodev2.h>
#include <sync/sync.h>
#include <utils/Trace.h>
#include <deque>
#include <cutils/properties.h>

#define HAVE_JPEG  // required for libyuv.h to export MJPEG decode APIs
#include <libyuv.h>
#include <libyuv/convert.h>

namespace android {
namespace hardware {
namespace camera {
namespace device {
namespace implementation {

namespace {
// Size of request metadata fast message queue. Change to 0 to always use hwbinder buffer.
static constexpr int32_t CAMERA_REQUEST_METADATA_QUEUE_SIZE = 1 << 20 /* 1MB */;
// Size of result metadata fast message queue. Change to 0 to always use hwbinder buffer.
static constexpr int32_t CAMERA_RESULT_METADATA_QUEUE_SIZE  = 1 << 20 /* 1MB */;


// Metadata sent by HAL will be replaced by a compact copy
// if their (total size >= compact size + METADATA_SHRINK_ABS_THRESHOLD &&
//           total_size >= compact size * METADATA_SHRINK_REL_THRESHOLD)
// Heuristically picked by size of one page
static constexpr int METADATA_SHRINK_ABS_THRESHOLD = 4096;
static constexpr int METADATA_SHRINK_REL_THRESHOLD = 2;

#if 0
const int kBadFramesAfterStreamOn = 1;  // drop x frames after streamOn to get rid of some initial
                                        // bad frames. TODO: develop a better bad frame detection
                                        // method
constexpr int MAX_RETRY = 15;  // Allow retry some ioctl failures a few times to account for some
                               // webcam showing temporarily ioctl failures.
constexpr int IOCTL_RETRY_SLEEP_US = 33000;  // 33ms * MAX_RETRY = 0.5 seconds


// Constants for tryLock during dumpstate
static constexpr int kDumpLockRetries = 50;
static constexpr int kDumpLockSleep = 60000;

bool tryLock(Mutex& mutex) {
    bool locked = false;
    for (int i = 0; i < kDumpLockRetries; ++i) {
        if (mutex.tryLock() == NO_ERROR) {
            locked = true;
            break;
        }
        usleep(kDumpLockSleep);
    }
    return locked;
}

bool tryLock(std::mutex& mutex) {
    bool locked = false;
    for (int i = 0; i < kDumpLockRetries; ++i) {
        if (mutex.try_lock()) {
            locked = true;
            break;
        }
        usleep(kDumpLockSleep);
    }
    return locked;
}
#endif

}  // anonymous namespace

using ::aidl::android::hardware::camera::device::BufferRequestStatus;
using ::aidl::android::hardware::camera::device::CameraBlob;
using ::aidl::android::hardware::camera::device::CameraBlobId;
using ::aidl::android::hardware::camera::device::ErrorMsg;
using ::aidl::android::hardware::camera::device::ShutterMsg;
using ::aidl::android::hardware::camera::device::StreamBuffer;
using ::aidl::android::hardware::camera::device::StreamBufferRet;
using ::aidl::android::hardware::camera::device::StreamBuffersVal;
using ::aidl::android::hardware::camera::device::StreamConfigurationMode;
using ::aidl::android::hardware::camera::device::StreamRotation;
using ::aidl::android::hardware::camera::device::StreamType;
using ::aidl::android::hardware::graphics::common::Dataspace;
using ::android::hardware::camera::common::V1_0::helper::ExifUtils;

// Static instances
HandleImporter AmlogicCameraDeviceSession::sHandleImporter;

AmlogicCameraDeviceSession::AmlogicCameraDeviceSession(
    aml_camera_device_t* device,
    const camera_metadata_t* deviceInfo,
    const std::shared_ptr<ICameraDeviceCallback>& callback) :
        aml_camera_callback_ops({&sProcessCaptureResult, &sNotify, nullptr, nullptr}),
        mDevice(device),
        mDeviceVersion(device->common.version),
        mFreeBufEarly(shouldFreeBufEarly()),
        mIsAELockAvailable(false),
        mDerivePostRawSensKey(false),
        mNumPartialResults(1),
        mResultBatcher(callback) {
    mDeviceInfo = deviceInfo;
    camera_metadata_entry partialResultsCount =
            mDeviceInfo.find(ANDROID_REQUEST_PARTIAL_RESULT_COUNT);
    if (partialResultsCount.count > 0) {
        mNumPartialResults = partialResultsCount.data.i32[0];
    }

    mResultBatcher.setNumPartialResults(mNumPartialResults);

    camera_metadata_entry aeLockAvailableEntry = mDeviceInfo.find(
            ANDROID_CONTROL_AE_LOCK_AVAILABLE);
    if (aeLockAvailableEntry.count > 0) {
        mIsAELockAvailable = (aeLockAvailableEntry.data.u8[0] ==
                ANDROID_CONTROL_AE_LOCK_AVAILABLE_TRUE);
    }

    // Determine whether we need to derive sensitivity boost values for older devices.
    // If post-RAW sensitivity boost range is listed, so should post-raw sensitivity control
    // be listed (as the default value 100)
    if (mDeviceInfo.exists(ANDROID_CONTROL_POST_RAW_SENSITIVITY_BOOST_RANGE)) {
        mDerivePostRawSensKey = true;
    }

    mInitFail = initialize();
}

bool AmlogicCameraDeviceSession::initialize() {
    ATRACE_BEGIN("aml_camera->initialize");
    status_t res = mDevice->ops->initialize(mDevice, this);
    ATRACE_END();

    if (res != OK) {
        ALOGE("%s: Unable to initialize HAL device: %s (%d)",
                __FUNCTION__, strerror(-res), res);
        mDevice->common.close(&mDevice->common);
        mClosed = true;
        return true;
    }

    // "ro.camera" properties are no longer supported on vendor side.
    //  Support a fall back for the fmq size override that uses "ro.vendor.camera"
    //  properties.
    int32_t reqFMQSize = property_get_int32("ro.vendor.camera.req.fmq.size", /*default*/-1);
    if (reqFMQSize < 0) {
        reqFMQSize = property_get_int32("ro.camera.req.fmq.size", /*default*/-1);
        if (reqFMQSize < 0) {
            reqFMQSize = CAMERA_REQUEST_METADATA_QUEUE_SIZE;
        } else {
            ALOGV("%s: request FMQ size overridden to %d", __FUNCTION__, reqFMQSize);
        }
    } else {
        ALOGV("%s: request FMQ size overridden to %d via fallback property", __FUNCTION__,
                reqFMQSize);
    }

    mRequestMetadataQueue = std::make_unique<RequestMetadataQueue>(
            static_cast<size_t>(reqFMQSize),
            false /* non blocking */);
    if (!mRequestMetadataQueue->isValid()) {
        ALOGE("%s: invalid request fmq", __FUNCTION__);
        return true;
    }

    // "ro.camera" properties are no longer supported on vendor side.
    //  Support a fall back for the fmq size override that uses "ro.vendor.camera"
    //  properties.
    int32_t resFMQSize = property_get_int32("ro.vendor.camera.res.fmq.size", /*default*/-1);
    if (resFMQSize < 0) {
        resFMQSize = property_get_int32("ro.camera.res.fmq.size", /*default*/-1);
        if (resFMQSize < 0) {
            resFMQSize = CAMERA_RESULT_METADATA_QUEUE_SIZE;
        } else {
            ALOGV("%s: result FMQ size overridden to %d", __FUNCTION__, resFMQSize);
        }
    } else {
        ALOGV("%s: result FMQ size overridden to %d via fallback property", __FUNCTION__,
                resFMQSize);
    }

    mResultMetadataQueue = std::make_shared<RequestMetadataQueue>(
            static_cast<size_t>(resFMQSize),
            false /* non blocking */);
    if (!mResultMetadataQueue->isValid()) {
        ALOGE("%s: invalid result fmq", __FUNCTION__);
        return true;
    }
    mResultBatcher.setResultMetadataQueue(mResultMetadataQueue);

    return false;

}

Status AmlogicCameraDeviceSession::initStatus() const {
    Mutex::Autolock _l(mStateLock);
    Status status = Status::OK;
    if (mInitFail) {
        status = Status::INTERNAL_ERROR;
    } else if (mDisconnected) {
        status = Status::CAMERA_DISCONNECTED;
    } else if (mClosed) {
        status = Status::INTERNAL_ERROR;
    }
    return status;
}

bool AmlogicCameraDeviceSession::shouldFreeBufEarly() {
    return property_get_bool("ro.vendor.camera.free_buf_early", 0) == 1;
}


void AmlogicCameraDeviceSession::disconnect() {
    Mutex::Autolock _l(mStateLock);
    mDisconnected = true;
    ALOGW("%s: Camera device is disconnected. Closing.", __FUNCTION__);
    if (!mClosed) {
        mDevice->common.close(&mDevice->common);
        mClosed = true;
    }
}

AmlogicCameraDeviceSession::~AmlogicCameraDeviceSession() {
    if (!isClosed()) {
        ALOGE("AmlogicCameraDeviceSession deleted before close!");
        close();
    }
}

Status AmlogicCameraDeviceSession::importRequest(
        const CaptureRequest& request,
        std::vector<buffer_handle_t*>& allBufPtrs,
        std::vector<int>& allFences) {
    return importRequestImpl(request, allBufPtrs, allFences);
}
Status AmlogicCameraDeviceSession::importRequestImpl(
        const CaptureRequest& request,
        std::vector<buffer_handle_t*>& allBufPtrs,
        std::vector<int>& allFences) {
    size_t numOutputBufs = request.outputBuffers.size();
    size_t numBufs = numOutputBufs;
    // Validate all I/O buffers
    std::vector<buffer_handle_t> allBufs;
    std::vector<uint64_t> allBufIds;
    allBufs.resize(numBufs);
    allBufIds.resize(numBufs);
    allBufPtrs.resize(numBufs);
    allFences.resize(numBufs);
    std::vector<int32_t> streamIds(numBufs);

    for (size_t i = 0; i < numOutputBufs; i++) {
        allBufs[i] = ::android::makeFromAidl(request.outputBuffers[i].buffer);
        allBufIds[i] = request.outputBuffers[i].bufferId;
        //allBufPtrs[i] = &allBufs[i];
        streamIds[i] = request.outputBuffers[i].streamId;
    }

    {
        Mutex::Autolock _l(mCbsLock);
        for (size_t i = 0; i < numBufs; i++) {
            Status st = importBufferLocked(streamIds[i], allBufIds[i], allBufs[i], &allBufPtrs[i]);
            native_handle_delete((native_handle_t *)allBufs[i]);
            if (st != Status::OK) {
                // Detailed error logs printed in importBuffer
                return st;
            }
        }
    }


    // All buffers are imported. Now validate output buffer acquire fences
    for (size_t i = 0; i < numOutputBufs; i++) {
        buffer_handle_t fenceHandle = ::android::makeFromAidl(request.outputBuffers[i].acquireFence);
        if (!sHandleImporter.importFence(fenceHandle, allFences[i])) {
            ALOGE("%s: output buffer %zu acquire fence is invalid", __FUNCTION__, i);
            cleanupInflightFences(allFences, i);
            native_handle_delete((native_handle_t *)fenceHandle);
            return Status::INTERNAL_ERROR;
        }
        native_handle_delete((native_handle_t *)fenceHandle);
    }
    return Status::OK;
}

Status AmlogicCameraDeviceSession::importBuffer(int32_t streamId, uint64_t bufId,
                                                 buffer_handle_t buf,
                                                 /*out*/ buffer_handle_t** outBufPtr) {
    Mutex::Autolock _l(mCbsLock);
    return importBufferLocked(streamId, bufId, buf, outBufPtr);
}

Status AmlogicCameraDeviceSession::importBufferLocked(int32_t streamId, uint64_t bufId,
                                                       buffer_handle_t buf,
                                                       buffer_handle_t** outBufPtr) {
    return importBufferImpl(mCirculatingBuffers, sHandleImporter, streamId, bufId, buf, outBufPtr);
}

void AmlogicCameraDeviceSession::cleanupInflightFences(
        std::vector<int>& allFences, size_t numFences) {
    for (size_t j = 0; j < numFences; j++) {
        sHandleImporter.closeFence(allFences[j]);
    }
}

AmlogicCameraDeviceSession::ResultBatcher::ResultBatcher(
        const std::shared_ptr<ICameraDeviceCallback>& callback) : mNumPartialResults(0), mCallback(callback) {};

bool AmlogicCameraDeviceSession::ResultBatcher::InflightBatch::allDelivered() const {
    if (!mShutterDelivered) return false;

    if (mPartialResultProgress < mNumPartialResults) {
        return false;
    }

    for (const auto& pair : mBatchBufs) {
        if (!pair.second.mDelivered) {
            return false;
        }
    }
    return true;
}

void AmlogicCameraDeviceSession::ResultBatcher::setNumPartialResults(uint32_t n) {
    Mutex::Autolock _l(mLock);
    mNumPartialResults = n;
}

void AmlogicCameraDeviceSession::ResultBatcher::setBatchedStreams(
        const std::vector<int>& streamsToBatch) {
    Mutex::Autolock _l(mLock);
    mStreamsToBatch = streamsToBatch;
}

void AmlogicCameraDeviceSession::ResultBatcher::setResultMetadataQueue(
        std::shared_ptr<ResultMetadataQueue> q) {
    Mutex::Autolock _l(mLock);
    mResultMetadataQueue = q;
}

void AmlogicCameraDeviceSession::ResultBatcher::registerBatch(uint32_t frameNumber, uint32_t batchSize) {
    auto batch = std::make_shared<InflightBatch>();
    batch->mFirstFrame = frameNumber;
    batch->mBatchSize = batchSize;
    batch->mLastFrame = batch->mFirstFrame + batch->mBatchSize - 1;
    batch->mNumPartialResults = mNumPartialResults;
    for (int id : mStreamsToBatch) {
        batch->mBatchBufs.emplace(id, batch->mBatchSize);
    }
    Mutex::Autolock _l(mLock);
    mInflightBatches.push_back(batch);
}

std::pair<int, std::shared_ptr<AmlogicCameraDeviceSession::ResultBatcher::InflightBatch>>
AmlogicCameraDeviceSession::ResultBatcher::getBatch(
        uint32_t frameNumber) {
    Mutex::Autolock _l(mLock);
    int numBatches = mInflightBatches.size();

    if (numBatches == 0) {
        return std::make_pair(NOT_BATCHED, nullptr);
    }

    uint32_t frameMin = mInflightBatches[0]->mFirstFrame;
    uint32_t frameMax = mInflightBatches[numBatches - 1]->mLastFrame;
    if (frameNumber < frameMin || frameNumber > frameMax) {
        return std::make_pair(NOT_BATCHED, nullptr);
    }

    for (int i = 0; i < numBatches; i++) {
        if (frameNumber >= mInflightBatches[i]->mFirstFrame &&
                frameNumber <= mInflightBatches[i]->mLastFrame) {
            return std::make_pair(i, mInflightBatches[i]);
        }
    }

    return std::make_pair(NOT_BATCHED, nullptr);
}

void AmlogicCameraDeviceSession::ResultBatcher::checkAndRemoveFirstBatch() {
    Mutex::Autolock _l(mLock);
    if (mInflightBatches.size() > 0) {
        std::shared_ptr<InflightBatch> batch = mInflightBatches[0];
        bool shouldRemove = false;
        {
            Mutex::Autolock _l(batch->mLock);
            if (batch->allDelivered()) {
                batch->mRemoved = true;
                shouldRemove = true;
            }
        }
        if (shouldRemove) {
            mInflightBatches.pop_front();
        }
    }
}

void AmlogicCameraDeviceSession::ResultBatcher::sendBatchShutterCbsLocked(
        std::shared_ptr<InflightBatch> batch) {

    if (batch->mShutterDelivered) {
        ALOGW("%s: batch shutter callback already sent!", __FUNCTION__);
        return;
    }

    auto ret = mCallback->notify(batch->mShutterMsgs);
    if (!ret.isOk()) {
        ALOGE("%s: notify shutter transaction failed: %d:%d", __FUNCTION__,
            ret.getExceptionCode(), ret.getServiceSpecificError());
    }
    batch->mShutterDelivered = true;
    batch->mShutterMsgs.clear();
}

void AmlogicCameraDeviceSession::ResultBatcher::moveStreamBuffer(StreamBuffer&& src, StreamBuffer& dst) {

    // Only dealing with releaseFence here. Assume buffer/acquireFence are null
    const native_handle_t* srcFenceHandle = ::android::makeFromAidl(src.releaseFence);
    dst.streamId = src.streamId;
    dst.bufferId = src.bufferId;
    dst.releaseFence = ::android::makeToAidl(srcFenceHandle);
    native_handle_t* dtsFenceHandle = ::android::makeFromAidl(dst.releaseFence);
    if (srcFenceHandle != dtsFenceHandle) {
        ALOGE("%s: native handle cloned!", __FUNCTION__);
    }
    native_handle_delete((native_handle_t *)srcFenceHandle);
    native_handle_delete(dtsFenceHandle);
}

void AmlogicCameraDeviceSession::ResultBatcher::pushStreamBuffer(
        StreamBuffer&& src, std::vector<StreamBuffer>& dst) {

    // Only dealing with releaseFence here. Assume buffer/acquireFence are null
    const native_handle_t* srcFenceHandle = ::android::makeFromAidl(src.releaseFence);
    StreamBuffer sb;
    sb.streamId = src.streamId;
    sb.bufferId = src.bufferId;
    dst.push_back(std::move(src));
    dst.back().releaseFence = ::android::makeToAidl(srcFenceHandle);
    native_handle_t* dstFenceHandle = ::android::makeFromAidl(dst.back().releaseFence);
    if (srcFenceHandle != dstFenceHandle) {
        ALOGE("%s: native handle cloned!", __FUNCTION__);
    }
    native_handle_delete((native_handle_t *)srcFenceHandle);
    native_handle_delete(dstFenceHandle);
}

void AmlogicCameraDeviceSession::ResultBatcher::sendBatchBuffersLocked(
        std::shared_ptr<InflightBatch> batch) {
    sendBatchBuffersLocked(batch, mStreamsToBatch);
}

void AmlogicCameraDeviceSession::ResultBatcher::sendBatchBuffersLocked(
        std::shared_ptr<InflightBatch> batch, const std::vector<int>& streams) {

    size_t batchSize = 0;
    for (int streamId : streams) {
        auto it = batch->mBatchBufs.find(streamId);
        if (it != batch->mBatchBufs.end()) {
            InflightBatch::BufferBatch& bb = it->second;
            if (bb.mDelivered) {
                continue;
            }
            if (bb.mBuffers.size() > batchSize) {
                batchSize = bb.mBuffers.size();
            }
        } else {
            ALOGE("%s: stream ID %d is not batched!", __FUNCTION__, streamId);
            return;
        }
    }

    if (batchSize == 0) {
        ALOGW("%s: there is no buffer to be delivered for this batch.", __FUNCTION__);
        for (int streamId : streams) {
            auto it = batch->mBatchBufs.find(streamId);
            if (it == batch->mBatchBufs.end()) {
                ALOGE("%s: cannot find stream %d in batched buffers!", __FUNCTION__, streamId);
                return;
            }
            InflightBatch::BufferBatch& bb = it->second;
            bb.mDelivered = true;
        }
        return;
    }

    std::vector<CaptureResult> results;
    results.resize(batchSize);
    for (size_t i = 0; i < batchSize; i++) {
        results[i].frameNumber = batch->mFirstFrame + i;
        results[i].fmqResultSize = 0;
        results[i].partialResult = 0; // 0 for buffer only results
        results[i].inputBuffer.streamId = -1;
        results[i].inputBuffer.bufferId = 0;
        //results[i].inputBuffer.buffer = nullptr;
        std::vector<StreamBuffer> outBufs;
        outBufs.reserve(streams.size());
        for (int streamId : streams) {
            auto it = batch->mBatchBufs.find(streamId);
            if (it == batch->mBatchBufs.end()) {
                ALOGE("%s: cannot find stream %d in batched buffers!", __FUNCTION__, streamId);
                return;
            }
            InflightBatch::BufferBatch& bb = it->second;
            if (bb.mDelivered) {
                continue;
            }
            if (i < bb.mBuffers.size()) {
                pushStreamBuffer(std::move(bb.mBuffers[i]), outBufs);
            }
        }
        results[i].outputBuffers.resize(outBufs.size());
        for (size_t j = 0; j < outBufs.size(); j++) {
            moveStreamBuffer(std::move(outBufs[j]), results[i].outputBuffers[j]);
        }
    }
    invokeProcessCaptureResultCallback(results, /* tryWriteFmq */false);
    freeReleaseFences(results);
    for (int streamId : streams) {
        auto it = batch->mBatchBufs.find(streamId);
        if (it == batch->mBatchBufs.end()) {
            ALOGE("%s: cannot find stream %d in batched buffers!", __FUNCTION__, streamId);
            return;
        }
        InflightBatch::BufferBatch& bb = it->second;
        bb.mDelivered = true;
        bb.mBuffers.clear();
    }
}

void AmlogicCameraDeviceSession::ResultBatcher::sendBatchMetadataLocked(
    std::shared_ptr<InflightBatch> batch, uint32_t lastPartialResultIdx) {
    if (lastPartialResultIdx <= batch->mPartialResultProgress) {
        // Result has been delivered. Return
        ALOGW("%s: partial result %u has been delivered", __FUNCTION__, lastPartialResultIdx);
        return;
    }

    std::vector<CaptureResult> results;
    std::vector<uint32_t> toBeRemovedIndexes;
    for (auto& pair : batch->mResultMds) {
        uint32_t partialIdx = pair.first;
        if (partialIdx > lastPartialResultIdx) {
            continue;
        }
        toBeRemovedIndexes.push_back(partialIdx);
        InflightBatch::MetadataBatch& mb = pair.second;
        for (const auto& p : mb.mMds) {
            CaptureResult result;
            result.frameNumber = p.first;
            result.result = std::move(p.second);
            result.fmqResultSize = 0;
            result.inputBuffer.streamId = -1;
            result.inputBuffer.bufferId = 0;
            //result.inputBuffer.buffer = nullptr;
            result.partialResult = partialIdx;
            results.push_back(std::move(result));
        }
        mb.mMds.clear();
    }
    std::vector<CaptureResult> &hResults = results;
    invokeProcessCaptureResultCallback(hResults, /* tryWriteFmq */true);
    batch->mPartialResultProgress = lastPartialResultIdx;
    for (uint32_t partialIdx : toBeRemovedIndexes) {
        batch->mResultMds.erase(partialIdx);
    }
}

void AmlogicCameraDeviceSession::ResultBatcher::notifySingleMsg(NotifyMsg& msg) {
    auto ret = mCallback->notify({msg});
    if (!ret.isOk()) {
        ALOGE("%s: notify transaction failed: %d:%d", __FUNCTION__,
            ret.getExceptionCode(), ret.getServiceSpecificError());
    }
    return;
}

void AmlogicCameraDeviceSession::ResultBatcher::notify(NotifyMsg& msg) {
    uint32_t frameNumber;
    if (CC_LIKELY(msg.getTag() == NotifyMsg::Tag::shutter)) {
        ShutterMsg shutter = msg.get<NotifyMsg::Tag::shutter>();
        frameNumber = shutter.frameNumber;
    } else {
        ErrorMsg error = msg.get<NotifyMsg::Tag::error>();
        frameNumber = error.frameNumber;
    }

    auto pair = getBatch(frameNumber);
    int batchIdx = pair.first;
    if (batchIdx == NOT_BATCHED) {
        notifySingleMsg(msg);
        return;
    }

    // When error happened, stop batching for all batches earlier
    if (CC_UNLIKELY(msg.getTag() == NotifyMsg::Tag::error)) {
        Mutex::Autolock _l(mLock);
        for (int i = 0; i <= batchIdx; i++) {
            // Send batched data up
            std::shared_ptr<InflightBatch> batch = mInflightBatches[0];
            {
                Mutex::Autolock _l(batch->mLock);
                sendBatchShutterCbsLocked(batch);
                sendBatchBuffersLocked(batch);
                sendBatchMetadataLocked(batch, mNumPartialResults);
                if (!batch->allDelivered()) {
                    ALOGE("%s: error: some batch data not sent back to framework!",
                            __FUNCTION__);
                }
                batch->mRemoved = true;
            }
            mInflightBatches.pop_front();
        }
        // Send the error up
        notifySingleMsg(msg);
        return;
    }
    // Queue shutter callbacks for future delivery
    std::shared_ptr<InflightBatch> batch = pair.second;
    {
        Mutex::Autolock _l(batch->mLock);
        // Check if the batch is removed (mostly by notify error) before lock was acquired
        if (batch->mRemoved) {
            // Fall back to non-batch path
            notifySingleMsg(msg);
            return;
        }

        batch->mShutterMsgs.push_back(msg);
        if (frameNumber == batch->mLastFrame) {
            sendBatchShutterCbsLocked(batch);
        }
    } // end of batch lock scope

    // see if the batch is complete
    if (frameNumber == batch->mLastFrame) {
        checkAndRemoveFirstBatch();
    }
}

void AmlogicCameraDeviceSession::ResultBatcher::invokeProcessCaptureResultCallback(
        std::vector<CaptureResult> &results, bool tryWriteFmq) {

    if (mProcessCaptureResultLock.tryLock() != OK) {
        ALOGE("%s: previous call is not finished! waiting 1s...", __FUNCTION__);
        if (mProcessCaptureResultLock.timedLock(1000000000 /* 1s */) != OK) {
            ALOGE("%s: cannot acquire lock in 1s, cannot proceed",
                    __FUNCTION__);
            return;
        }
    }

    if (tryWriteFmq && mResultMetadataQueue->availableToWrite() > 0) {
        for (CaptureResult &result : results) {
            CameraMetadata& md = result.result;
            if (result.result.metadata.size() > 0) {
                if (mResultMetadataQueue->write(reinterpret_cast<int8_t*>(md.metadata.data()), md.metadata.size())) {
                    result.fmqResultSize = md.metadata.size();
                    md.metadata.resize(0);
                } else {
                    ALOGE("%s: couldn't utilize fmq, fall back to hwbinder, result size: %zu,"
                    "shared message queue available size: %zu",
                        __FUNCTION__, md.metadata.size(),
                        mResultMetadataQueue->availableToWrite());
                    result.fmqResultSize = 0;
                }
            }
        }
    }

    auto ret = mCallback->processCaptureResult(results);

    if (!ret.isOk()) {
        ALOGE("%s: processCaptureResult transaction failed: %d:%d", __FUNCTION__,
            ret.getExceptionCode(), ret.getServiceSpecificError());
    }
    mProcessCaptureResultLock.unlock();
}

void AmlogicCameraDeviceSession::ResultBatcher::processOneCaptureResult(CaptureResult& result) {

    std::vector<CaptureResult> results;
    results.resize(1);
    results[0] = std::move(result);
    invokeProcessCaptureResultCallback(results, /* tryWriteFmq */true);
    freeReleaseFences(results);
    return;
}

void AmlogicCameraDeviceSession::ResultBatcher::processCaptureResult(CaptureResult& result) {
    auto pair = getBatch(result.frameNumber);
    int batchIdx = pair.first;
    if (batchIdx == NOT_BATCHED) {
        processOneCaptureResult(result);
        return;
    }
    std::shared_ptr<InflightBatch> batch = pair.second;
    {
        Mutex::Autolock _l(batch->mLock);
        // Check if the batch is removed (mostly by notify error) before lock was acquired
        if (batch->mRemoved) {
            // Fall back to non-batch path
            processOneCaptureResult(result);
            return;
        }

        // queue metadata
        if (result.result.metadata.size() != 0) {
            // Save a copy of metadata
            batch->mResultMds[result.partialResult].mMds.push_back(
                    std::make_pair(result.frameNumber, result.result));
        }

        // queue buffer
        std::vector<int> filledStreams;
        std::vector<StreamBuffer> nonBatchedBuffers;
        for (auto& buffer : result.outputBuffers) {
            auto it = batch->mBatchBufs.find(buffer.streamId);
            if (it != batch->mBatchBufs.end()) {
                InflightBatch::BufferBatch& bb = it->second;
                auto id = buffer.streamId;
                pushStreamBuffer(std::move(buffer), bb.mBuffers);
                filledStreams.push_back(id);
            } else {
                pushStreamBuffer(std::move(buffer), nonBatchedBuffers);
            }
        }

        // send non-batched buffers up
        if (nonBatchedBuffers.size() > 0 || result.inputBuffer.streamId != -1) {
            CaptureResult nonBatchedResult;
            nonBatchedResult.frameNumber = result.frameNumber;
            nonBatchedResult.fmqResultSize = 0;
            nonBatchedResult.outputBuffers.resize(nonBatchedBuffers.size());
            for (size_t i = 0; i < nonBatchedBuffers.size(); i++) {
                moveStreamBuffer(
                        std::move(nonBatchedBuffers[i]), nonBatchedResult.outputBuffers[i]);
            }
            moveStreamBuffer(std::move(result.inputBuffer), nonBatchedResult.inputBuffer);
            nonBatchedResult.partialResult = 0; // 0 for buffer only results
            processOneCaptureResult(nonBatchedResult);
        }

        if (result.frameNumber == batch->mLastFrame) {
            // Send data up
            if (result.partialResult > 0) {
                sendBatchMetadataLocked(batch, result.partialResult);
            }
            // send buffer up
            if (filledStreams.size() > 0) {
                sendBatchBuffersLocked(batch, filledStreams);
            }
        }
    } // end of batch lock scope

    // see if the batch is complete
    if (result.frameNumber == batch->mLastFrame) {
        checkAndRemoveFirstBatch();
    }
}

ScopedAStatus AmlogicCameraDeviceSession::constructDefaultRequestSettings(
        RequestTemplate in_type, CameraMetadata* _aidl_return) {
    CameraMetadata outMetadata;
    Status status = constructDefaultRequestSettingsRaw( (int) in_type, &outMetadata);
    *_aidl_return = outMetadata;
    return fromStatus(status);
}

Status AmlogicCameraDeviceSession::constructDefaultRequestSettingsRaw(int type, CameraMetadata *outMetadata) {
    Status status = initStatus();
    const camera_metadata_t *rawRequest;
    if (status == Status::OK) {
        ATRACE_BEGIN("aml_camera->construct_default_request_settings");
        rawRequest = mDevice->ops->construct_default_request_settings(mDevice, (int) type);
        ATRACE_END();
        if (rawRequest == nullptr) {
            ALOGI("%s: template %d is not supported on this camera device",
                  __FUNCTION__, type);
            status = Status::ILLEGAL_ARGUMENT;
        } else {
            mOverriddenRequest.clear();
            mOverriddenRequest.append(rawRequest);
            // Derive some new keys for backward compatibility
            if (mDerivePostRawSensKey && !mOverriddenRequest.exists(
                    ANDROID_CONTROL_POST_RAW_SENSITIVITY_BOOST)) {
                int32_t defaultBoost[1] = {100};
                mOverriddenRequest.update(
                        ANDROID_CONTROL_POST_RAW_SENSITIVITY_BOOST,
                        defaultBoost, 1);
            }
            const camera_metadata_t *metaBuffer =
                    mOverriddenRequest.getAndLock();
            convertToAidl(metaBuffer, outMetadata);
            mOverriddenRequest.unlock(metaBuffer);
        }
    }
    return status;
}

/**
 * Map Android N dataspace definitions back to Android M definitions, for
 * use with HALv3.3 or older.
 *
 * Only map where correspondences exist, and otherwise preserve the value.
 */
android_dataspace AmlogicCameraDeviceSession::mapToLegacyDataspace(
        android_dataspace dataSpace) const {
    if (mDeviceVersion <= CAMERA_DEVICE_API_VERSION_3_3) {
        switch (dataSpace) {
            case HAL_DATASPACE_V0_SRGB_LINEAR:
                return HAL_DATASPACE_SRGB_LINEAR;
            case HAL_DATASPACE_V0_SRGB:
                return HAL_DATASPACE_SRGB;
            case HAL_DATASPACE_V0_JFIF:
                return HAL_DATASPACE_JFIF;
            case HAL_DATASPACE_V0_BT601_625:
                return HAL_DATASPACE_BT601_625;
            case HAL_DATASPACE_V0_BT601_525:
                return HAL_DATASPACE_BT601_525;
            case HAL_DATASPACE_V0_BT709:
                return HAL_DATASPACE_BT709;
            default:
                return dataSpace;
        }
    }

   return dataSpace;
}

/**
 * For devices <= CAMERA_DEVICE_API_VERSION_3_2, AE_PRECAPTURE_TRIGGER_CANCEL is not supported so
 * we need to override AE_PRECAPTURE_TRIGGER_CANCEL to AE_PRECAPTURE_TRIGGER_IDLE and AE_LOCK_OFF
 * to AE_LOCK_ON to start cancelling AE precapture. If AE lock is not available, it still overrides
 * AE_PRECAPTURE_TRIGGER_CANCEL to AE_PRECAPTURE_TRIGGER_IDLE but doesn't add AE_LOCK_ON to the
 * request.
 */
bool AmlogicCameraDeviceSession::handleAePrecaptureCancelRequestLocked(
        const aml_camera_capture_request_t &halRequest,
        ::android::hardware::camera::common::V1_0::helper::CameraMetadata *settings /*out*/,
         AETriggerCancelOverride *override /*out*/) {
    if ((mDeviceVersion > CAMERA_DEVICE_API_VERSION_3_2) ||
            (nullptr == halRequest.settings) || (nullptr == settings) ||
            (0 == get_camera_metadata_entry_count(halRequest.settings))) {
        return false;
    }

    settings->clear();
    settings->append(halRequest.settings);
    camera_metadata_entry_t aePrecaptureTrigger =
            settings->find(ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER);
    if (aePrecaptureTrigger.count > 0 &&
            aePrecaptureTrigger.data.u8[0] ==
                    ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER_CANCEL) {
        // Always override CANCEL to IDLE
        uint8_t aePrecaptureTrigger =
                ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER_IDLE;
        settings->update(ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER,
                &aePrecaptureTrigger, 1);
        *override = { false, ANDROID_CONTROL_AE_LOCK_OFF,
                true, ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER_CANCEL };

        if (mIsAELockAvailable == true) {
            camera_metadata_entry_t aeLock = settings->find(
                    ANDROID_CONTROL_AE_LOCK);
            if (aeLock.count == 0 || aeLock.data.u8[0] ==
                    ANDROID_CONTROL_AE_LOCK_OFF) {
                uint8_t aeLock = ANDROID_CONTROL_AE_LOCK_ON;
                settings->update(ANDROID_CONTROL_AE_LOCK, &aeLock, 1);
                override->applyAeLock = true;
                override->aeLock = ANDROID_CONTROL_AE_LOCK_OFF;
            }
        }

        return true;
    }

    return false;
}

/**
  * Override result metadata for cancelling AE precapture trigger applied in
  * handleAePrecaptureCancelRequestLocked().
  */
void AmlogicCameraDeviceSession::overrideResultForPrecaptureCancelLocked(
        const AETriggerCancelOverride &aeTriggerCancelOverride,
        ::android::hardware::camera::common::V1_0::helper::CameraMetadata *settings /*out*/) {
    if (aeTriggerCancelOverride.applyAeLock) {
        // Only devices <= v3.2 should have this override
        assert(mDeviceVersion <= CAMERA_DEVICE_API_VERSION_3_2);
        settings->update(ANDROID_CONTROL_AE_LOCK, &aeTriggerCancelOverride.aeLock, 1);
    }

    if (aeTriggerCancelOverride.applyAePrecaptureTrigger) {
        // Only devices <= v3.2 should have this override
        assert(mDeviceVersion <= CAMERA_DEVICE_API_VERSION_3_2);
        settings->update(ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER,
                &aeTriggerCancelOverride.aePrecaptureTrigger, 1);
    }
}

bool AmlogicCameraDeviceSession::preProcessConfigurationLocked(
        const StreamConfiguration& requestedConfiguration,
        aml_camera_stream_configuration_t *stream_list /*out*/,
        std::vector<aml_camera_stream_t*> *streams /*out*/) {
    if ((stream_list == nullptr) || (streams == nullptr)) {
        return false;
    }

    stream_list->operation_mode = (uint32_t) requestedConfiguration.operationMode;
    stream_list->num_streams = requestedConfiguration.streams.size();
    streams->resize(stream_list->num_streams);
    stream_list->streams = streams->data();

    for (uint32_t i = 0; i < stream_list->num_streams; i++) {
        int id = requestedConfiguration.streams[i].id;
        if (mStreamMap.count(id) == 0) {
            AmlCameraStream stream;
            convertFromAidl(requestedConfiguration.streams[i], &stream);
            mStreamMap[id] = stream;
            mStreamMap[id].data_space = mapToLegacyDataspace(
                    mStreamMap[id].data_space);
            mCirculatingBuffers.emplace(stream.mId, CirculatingBuffers{});
        } else {
            // width/height/format must not change, but usage/rotation might need to change
            // if (mStreamMap[id].stream_type !=
            //         (int) requestedConfiguration.streams[i].streamType ||
            //         mStreamMap[id].width != requestedConfiguration.streams[i].width ||
            //         mStreamMap[id].height != requestedConfiguration.streams[i].height ||
            //         mStreamMap[id].format != (int) requestedConfiguration.streams[i].format ||
            //         mStreamMap[id].data_space !=
            //                 mapToLegacyDataspace( static_cast<android_dataspace_t> (
            //                         requestedConfiguration.streams[i].dataSpace))) {
            //     ALOGE("%s: stream %d configuration changed!", __FUNCTION__, id);
            //     return false;
            // }
            mStreamMap[id].stream_type =(int) requestedConfiguration.streams[i].streamType;
            mStreamMap[id].width = requestedConfiguration.streams[i].width;
            mStreamMap[id].height = requestedConfiguration.streams[i].height;
            mStreamMap[id].format = (int) requestedConfiguration.streams[i].format;
            mStreamMap[id].data_space = mapToLegacyDataspace( static_cast<android_dataspace_t> (
                                    requestedConfiguration.streams[i].dataSpace));
            mStreamMap[id].rotation = (int) requestedConfiguration.streams[i].rotation;
            mStreamMap[id].usage = (uint32_t) requestedConfiguration.streams[i].usage;
        }
        (*streams)[i] = &mStreamMap[id];
    }

    if (mFreeBufEarly) {

        // Remove buffers of deleted streams
        for (auto it = mStreamMap.begin(); it != mStreamMap.end(); it++) {
            int id = it->first;
            bool found = false;
            for (const auto& stream : requestedConfiguration.streams) {
                if (id == stream.id) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                // Unmap all buffers of deleted stream
                cleanupBuffersLocked(id);
            }
        }
    }

    return true;
}

void AmlogicCameraDeviceSession::postProcessConfigurationLocked(
        const StreamConfiguration& requestedConfiguration) {
    // delete unused streams, note we do this after adding new streams to ensure new stream
    // will not have the same address as deleted stream, and HAL has a chance to reference
    // the to be deleted stream in configure_streams call
    for (auto it = mStreamMap.begin(); it != mStreamMap.end();) {
        int id = it->first;
        bool found = false;
        for (const auto& stream : requestedConfiguration.streams) {
            if (id == stream.id) {
                found = true;
                break;
            }
        }
        if (!found) {
            // Unmap all buffers of deleted stream
            // in case the configuration call succeeds and HAL
            // is able to release the corresponding resources too.
            if (!mFreeBufEarly) {
                cleanupBuffersLocked(id);
            }
            it = mStreamMap.erase(it);
        } else {
            ++it;
        }
    }

    // Track video streams
    mVideoStreamIds.clear();
    for (const auto& stream : requestedConfiguration.streams) {
        if (stream.streamType == StreamType::OUTPUT &&
            static_cast<uint64_t>(stream.usage) &
                static_cast<uint64_t>(graphics::common::V1_0::BufferUsage::VIDEO_ENCODER)) {
            mVideoStreamIds.push_back(stream.id);
        }
    }
    mResultBatcher.setBatchedStreams(mVideoStreamIds);
}

void AmlogicCameraDeviceSession::postProcessConfigurationFailureLocked(
        const StreamConfiguration& requestedConfiguration) {
    if (mFreeBufEarly) {
        // Re-build the buf cache entry for deleted streams
        for (auto it = mStreamMap.begin(); it != mStreamMap.end(); it++) {
            int id = it->first;
            bool found = false;
            for (const auto& stream : requestedConfiguration.streams) {
                if (id == stream.id) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                mCirculatingBuffers.emplace(id, CirculatingBuffers{});
            }
        }
    }
}

ScopedAStatus AmlogicCameraDeviceSession::configureStreams(
        const StreamConfiguration& in_requestedConfiguration,
        std::vector<HalStream>* _aidl_return) {
    Status status = initStatus();
    _aidl_return->clear();
    std::vector<HalStream>& out = *_aidl_return;

    // hold the inflight lock for entire configureStreams scope since there must not be any
    // inflight request/results during stream configuration.
    Mutex::Autolock _l(mInflightLock);

    if (!mInflightBuffers.empty()) {
        ALOGE("%s: trying to configureStreams while there are still %zu inflight buffers!",
                __FUNCTION__, mInflightBuffers.size());
        return fromStatus(Status::INTERNAL_ERROR);
    }

    if (!mInflightAETriggerOverrides.empty()) {
        ALOGE("%s: trying to configureStreams while there are still %zu inflight"
                " trigger overrides!", __FUNCTION__,
                mInflightAETriggerOverrides.size());
        return fromStatus(Status::INTERNAL_ERROR);
    }

    if (!mInflightRawBoostPresent.empty()) {
        ALOGE("%s: trying to configureStreams while there are still %zu inflight"
                " boost overrides!", __FUNCTION__,
                mInflightRawBoostPresent.size());
        return fromStatus(Status::INTERNAL_ERROR);
    }

    if (status != Status::OK) {
        return fromStatus(status);
    }

    aml_camera_stream_configuration_t stream_list{};
    std::vector<aml_camera_stream_t*> streams;

    for (const auto &it : in_requestedConfiguration.streams) {
        if (it.useCase != aidl::android::hardware::camera::metadata::ScalerAvailableStreamUseCases::ANDROID_SCALER_AVAILABLE_STREAM_USE_CASES_DEFAULT) {
            return fromStatus(Status::ILLEGAL_ARGUMENT);
        }
    }

    if (!preProcessConfigurationLocked(in_requestedConfiguration, &stream_list, &streams)) {
        return fromStatus(Status::INTERNAL_ERROR);
    }

    ATRACE_BEGIN("amlcamera->configure_streams");
    status_t ret = mDevice->ops->configure_streams(mDevice, &stream_list);
    ATRACE_END();

    if (ret == OK) {
        postProcessConfigurationLocked(in_requestedConfiguration);
    } else {
        postProcessConfigurationFailureLocked(in_requestedConfiguration);
    }

    if (ret == -EINVAL) {
        status = Status::ILLEGAL_ARGUMENT;
    } else if (ret != OK) {
        status = Status::INTERNAL_ERROR;
    } else {
        out.resize(stream_list.num_streams);
        for (int i = 0; i < stream_list.num_streams; i++) {
            convertToAidl(static_cast<AmlCameraStream*>(stream_list.streams[i]), &out[i]);
        }
        mFirstRequest = true;
    }
    return fromStatus(status);
}

 ScopedAStatus AmlogicCameraDeviceSession::flush() {
    Status status = initStatus();
    if (status != Status::OK) {
        status_t ret = mDevice->ops->flush(mDevice);
        if (ret != OK) {
            status = Status::INTERNAL_ERROR;
        }
    }
    return fromStatus(status);
}

ScopedAStatus AmlogicCameraDeviceSession::getCaptureRequestMetadataQueue(
        MQDescriptor<int8_t, SynchronizedReadWrite>* _aidl_return) {
    *_aidl_return = mRequestMetadataQueue->dupeDesc();
    return fromStatus(Status::OK);
}

ScopedAStatus AmlogicCameraDeviceSession::getCaptureResultMetadataQueue(
        MQDescriptor<int8_t, SynchronizedReadWrite>* _aidl_return) {
    *_aidl_return = mResultMetadataQueue->dupeDesc();
    return fromStatus(Status::OK);
}

ScopedAStatus AmlogicCameraDeviceSession::isReconfigurationRequired(
        const CameraMetadata& in_oldSessionParams, const CameraMetadata& in_newSessionParams,
        bool* _aidl_return) {
    // reconfiguration required if there is any change in the session params
    *_aidl_return = in_oldSessionParams != in_newSessionParams;
    return fromStatus(Status::OK);
}

ScopedAStatus AmlogicCameraDeviceSession::processCaptureRequest(
        const std::vector<CaptureRequest>& in_requests,
        const std::vector<BufferCache>& in_cachesToRemove, int32_t* _aidl_return) {

    updateBufferCaches(in_cachesToRemove);

    int32_t& numRequestProcessed = *_aidl_return;
    numRequestProcessed = 0;
    Status s = Status::OK;
    for (size_t i = 0; i < in_requests.size(); i++, numRequestProcessed++) {
        s = processOneCaptureRequest(in_requests[i]);
        if (s != Status::OK) {
            break;
        }
    }

    if (s == Status::OK && in_requests.size() > 1) {
        mResultBatcher.registerBatch(in_requests[0].frameNumber, in_requests.size());
    }
    return fromStatus(s);
}

Status AmlogicCameraDeviceSession::processOneCaptureRequest(const CaptureRequest& request) {
    ATRACE_CALL();
    Status status = initStatus();
    if (status != Status::OK) {
        ALOGE("%s: camera init failed or disconnected", __FUNCTION__);
        return status;
    }

    aml_camera_capture_request_t halRequest;
    halRequest.frame_number = request.frameNumber;
    //const camera_metadata_t* rawSettings = nullptr;
    bool converted;
    // settings from FMQ
    CameraMetadata settingsFmq;
    if (request.fmqSettingsSize > 0) {
        // non-blocking read; client must write metadata before calling
        // processOneCaptureRequest
        settingsFmq.metadata.resize(request.fmqSettingsSize);
        bool read = mRequestMetadataQueue->read(
                reinterpret_cast<int8_t*>(settingsFmq.metadata.data()), request.fmqSettingsSize);
        if (read) {
            converted = convertFromAidl(settingsFmq, &halRequest.settings);
        } else {
            ALOGE("%s: capture request settings metadata couldn't be read from fmq!", __FUNCTION__);
            converted = false;
        }
    } else {
        converted = convertFromAidl(request.settings, &halRequest.settings);
    }

    if (!converted) {
        ALOGE("%s: capture request settings metadata is corrupt!", __FUNCTION__);
        return Status::ILLEGAL_ARGUMENT;
    }

    if (mFirstRequest && halRequest.settings == nullptr) {
        ALOGE("%s: capture request settings must not be null for first request!", __FUNCTION__);
        return Status::ILLEGAL_ARGUMENT;
    }

    std::vector<buffer_handle_t*> allBufPtrs;
    std::vector<int> allFences;
    bool hasInputBuf = (request.inputBuffer.streamId != -1 &&
            request.inputBuffer.bufferId != 0);
    size_t numOutputBufs = request.outputBuffers.size();
    size_t numBufs = numOutputBufs + (hasInputBuf ? 1 : 0);


    if (numOutputBufs == 0) {
        ALOGE("%s: capture request must have at least one output buffer!", __FUNCTION__);
        return Status::ILLEGAL_ARGUMENT;
    }

    status = importRequest(request, allBufPtrs, allFences);
    if (status != Status::OK) {
        return status;
    }

    std::vector<aml_camera_stream_buffer_t> outHalBufs;
    outHalBufs.resize(numOutputBufs);
    bool aeCancelTriggerNeeded = false;
    ::android::hardware::camera::common::V1_0::helper::CameraMetadata settingsOverride;
    {
        Mutex::Autolock _l(mInflightLock);
        if (hasInputBuf) {
            auto key = std::make_pair(request.inputBuffer.streamId, request.frameNumber);
            auto& bufCache = mInflightBuffers[key] = aml_camera_stream_buffer_t{};
            convertFromAidl(
                    allBufPtrs[numOutputBufs], request.inputBuffer.status,
                    &mStreamMap[request.inputBuffer.streamId], allFences[numOutputBufs],
                    &bufCache);
            halRequest.input_buffer = &bufCache;
        } else {
            halRequest.input_buffer = nullptr;
        }

        halRequest.num_output_buffers = numOutputBufs;
        for (size_t i = 0; i < numOutputBufs; i++) {
            auto key = std::make_pair(request.outputBuffers[i].streamId, request.frameNumber);
            auto& bufCache = mInflightBuffers[key] = aml_camera_stream_buffer_t{};
            convertFromAidl(
                    allBufPtrs[i], request.outputBuffers[i].status,
                    &mStreamMap[request.outputBuffers[i].streamId], allFences[i],
                    &bufCache);
            outHalBufs[i] = bufCache;
        }
        halRequest.output_buffers = outHalBufs.data();

        AETriggerCancelOverride triggerOverride;
        aeCancelTriggerNeeded = handleAePrecaptureCancelRequestLocked(
                halRequest, &settingsOverride /*out*/, &triggerOverride/*out*/);
        if (aeCancelTriggerNeeded) {
            mInflightAETriggerOverrides[halRequest.frame_number] =
                    triggerOverride;
            halRequest.settings = settingsOverride.getAndLock();
        }
    }

    halRequest.num_physcam_settings = 0;

    ATRACE_ASYNC_BEGIN("frame capture", request.frameNumber);
    ATRACE_BEGIN("amlcamera->process_capture_request");
    status_t ret = mDevice->ops->process_capture_request(mDevice, &halRequest);
    ATRACE_END();
    if (aeCancelTriggerNeeded) {
        settingsOverride.unlock(halRequest.settings);
    }

    if (ret != OK) {
        Mutex::Autolock _l(mInflightLock);
        ALOGE("%s: HAL process_capture_request call failed!", __FUNCTION__);

        cleanupInflightFences(allFences, numBufs);
        if (hasInputBuf) {
            auto key = std::make_pair(request.inputBuffer.streamId, request.frameNumber);
            mInflightBuffers.erase(key);
        }
        for (size_t i = 0; i < numOutputBufs; i++) {
            auto key = std::make_pair(request.outputBuffers[i].streamId, request.frameNumber);
            mInflightBuffers.erase(key);
        }
        if (aeCancelTriggerNeeded) {
            mInflightAETriggerOverrides.erase(request.frameNumber);
        }
        if (ret == BAD_VALUE)
            return Status::ILLEGAL_ARGUMENT;
        return Status::INTERNAL_ERROR;
    }

    mFirstRequest = false;
    return Status::OK;
}

ScopedAStatus AmlogicCameraDeviceSession::signalStreamFlush(
        const std::vector<int32_t>& /*in_streamIds*/, int32_t in_streamConfigCounter) {
    {
        Mutex::Autolock _l(mLock);
        if (in_streamConfigCounter < 3) {
            // stale call. new streams have been configured since this call was issued.
            // Do nothing.
            return fromStatus(Status::OK);
        }
    }
    // TODO: implement if needed.
    return fromStatus(Status::OK);
}

ScopedAStatus AmlogicCameraDeviceSession::switchToOffline(
        const std::vector<int32_t>& in_streamsToKeep,
        CameraOfflineSessionInfo* out_offlineSessionInfo,
        std::shared_ptr<ICameraOfflineSession>* _aidl_return) {
    if (_aidl_return == nullptr) {
        ALOGE("%s, output arguments (%p, %p, %p) must not be null", __FUNCTION__, &in_streamsToKeep, out_offlineSessionInfo,
                _aidl_return);
    }
    return fromStatus(Status::OK);
}

ScopedAStatus AmlogicCameraDeviceSession::close() {
    Mutex::Autolock _l(mStateLock);
    if (!mClosed) {
        {
            Mutex::Autolock _l(mInflightLock);
            if (!mInflightBuffers.empty()) {
                ALOGE("%s: trying to close while there are still %zu inflight buffers!",
                        __FUNCTION__, mInflightBuffers.size());
            }
            if (!mInflightAETriggerOverrides.empty()) {
                ALOGE("%s: trying to close while there are still %zu inflight "
                        "trigger overrides!", __FUNCTION__,
                        mInflightAETriggerOverrides.size());
            }
            if (!mInflightRawBoostPresent.empty()) {
                ALOGE("%s: trying to close while there are still %zu inflight "
                        " RAW boost overrides!", __FUNCTION__,
                        mInflightRawBoostPresent.size());
            }
        }

        ATRACE_BEGIN("amlcamera->close");
        mDevice->common.close(&mDevice->common);
        ATRACE_END();

        // free all imported buffers
        Mutex::Autolock _l(mInflightLock);
        for (auto& pair : mCirculatingBuffers) {
            CirculatingBuffers& buffers = pair.second;
            for (auto& p2 : buffers) {
                sHandleImporter.freeBuffer(p2.second);
            }
            buffers.clear();
        }
        mCirculatingBuffers.clear();

        mClosed = true;
    }
    return fromStatus(Status::OK);
}


bool AmlogicCameraDeviceSession::isClosed() {
    Mutex::Autolock _l(mStateLock);
    return mClosed;
}

ScopedAStatus AmlogicCameraDeviceSession::repeatingRequestEnd(
        int32_t /*in_frameNumber*/, const std::vector<int32_t>& /*in_streamIds*/) {
    // TODO: Figure this one out.
    return fromStatus(Status::OK);
}

void AmlogicCameraDeviceSession::cleanupBuffersLocked(int id) {
    for (auto& pair : mCirculatingBuffers.at(id)) {
        sHandleImporter.freeBuffer(pair.second);
    }
    mCirculatingBuffers[id].clear();
    mCirculatingBuffers.erase(id);
}

void AmlogicCameraDeviceSession::notifyShutter(int32_t frameNumber, nsecs_t shutterTs) {
    NotifyMsg msg;
    msg.set<NotifyMsg::Tag::shutter>(ShutterMsg{
            .frameNumber = frameNumber,
            .timestamp = shutterTs,
    });
    mCallback->notify({msg});
}

void AmlogicCameraDeviceSession::notifyError(int32_t frameNumber, int32_t streamId, ErrorCode ec) {
    NotifyMsg msg;
    msg.set<NotifyMsg::Tag::error>(ErrorMsg{
            .frameNumber = frameNumber,
            .errorStreamId = streamId,
            .errorCode = ec,
    });
    mCallback->notify({msg});
}

void AmlogicCameraDeviceSession::invokeProcessCaptureResultCallback(
        std::vector<CaptureResult>& results, bool tryWriteFmq) {
    if (mProcessCaptureResultLock.tryLock() != OK) {
        const nsecs_t NS_TO_SECOND = 1000000000;
        ALOGV("%s: previous call is not finished! waiting 1s...", __FUNCTION__);
        if (mProcessCaptureResultLock.timedLock(/* 1s */ NS_TO_SECOND) != OK) {
            ALOGE("%s: cannot acquire lock in 1s, cannot proceed", __FUNCTION__);
            return;
        }
    }
    if (tryWriteFmq && mResultMetadataQueue->availableToWrite() > 0) {
        for (CaptureResult& result : results) {
            CameraMetadata& md = result.result;
            ALOGE("md %s",md.toString().c_str());
            if (!md.metadata.empty()) {
                if (mResultMetadataQueue->write(reinterpret_cast<int8_t*>(md.metadata.data()),
                                                md.metadata.size())) {
                    result.fmqResultSize = md.metadata.size();
                    md.metadata.resize(0);
                } else {
                    ALOGW("%s: couldn't utilize fmq, fall back to hwbinder", __FUNCTION__);
                    result.fmqResultSize = 0;
                }
            } else {
                result.fmqResultSize = 0;
            }
        }
    }
    auto status = mCallback->processCaptureResult(results);
    if (!status.isOk()) {
        ALOGE("%s: processCaptureResult ERROR : %d:%d", __FUNCTION__, status.getExceptionCode(),
              status.getServiceSpecificError());
    }

    mProcessCaptureResultLock.unlock();
}

bool AmlogicCameraDeviceSession::supportOfflineLocked(int32_t streamId) {
    const AmlCameraStream& stream = mStreamMap[streamId];
    if (static_cast<int32_t>(stream.format) == static_cast<int32_t>(PixelFormat::BLOB) &&
        static_cast<int32_t>(stream.data_space) == static_cast<int32_t>(Dataspace::JFIF)) {
        return true;
    }
    // TODO: support YUV output stream?
    return false;
}

void AmlogicCameraDeviceSession::updateBufferCaches(
        const std::vector<BufferCache>& cachesToRemove) {
    for (auto& cache : cachesToRemove) {
        auto cbsIt = mCirculatingBuffers.find(cache.streamId);
        if (cbsIt == mCirculatingBuffers.end()) {
            // The stream could have been removed
            continue;
        }
        CirculatingBuffers& cbs = cbsIt->second;
        auto it = cbs.find(cache.bufferId);
        if (it != cbs.end()) {
            sHandleImporter.freeBuffer(it->second);
            cbs.erase(it);
        } else {
            ALOGE("%s: stream %d buffer %" PRIu64 " is not cached", __FUNCTION__, cache.streamId,
                  cache.bufferId);
        }
    }
}

uint64_t AmlogicCameraDeviceSession::getCapResultBufferId(const buffer_handle_t&, int) {
    // No need to fill in bufferId by default
    return BUFFER_ID_NO_BUFFER;
}

status_t AmlogicCameraDeviceSession::constructCaptureResult(CaptureResult& result,
                                                 const aml_camera_capture_result *hal_result) {
    uint32_t frameNumber = hal_result->frame_number;
    bool hasInputBuf = (hal_result->input_buffer != nullptr);
    size_t numOutputBufs = hal_result->num_output_buffers;
    size_t numBufs = numOutputBufs + (hasInputBuf ? 1 : 0);
    if (numBufs > 0) {
        Mutex::Autolock _l(mInflightLock);
        if (hasInputBuf) {
            int streamId = static_cast<AmlCameraStream*>(hal_result->input_buffer->stream)->mId;
            // validate if buffer is inflight
            auto key = std::make_pair(streamId, frameNumber);
            if (mInflightBuffers.count(key) != 1) {
                ALOGE("%s: input buffer for stream %d frame %d is not inflight!",
                        __FUNCTION__, streamId, frameNumber);
                return -EINVAL;
            }
        }

        for (size_t i = 0; i < numOutputBufs; i++) {
            int streamId = static_cast<AmlCameraStream*>(hal_result->output_buffers[i].stream)->mId;
            // validate if buffer is inflight
            auto key = std::make_pair(streamId, frameNumber);
            if (mInflightBuffers.count(key) != 1) {
                ALOGE("%s: output buffer for stream %d frame %d is not inflight!",
                        __FUNCTION__, streamId, frameNumber);
                return -EINVAL;
            }
        }
    }
    // We don't need to validate/import fences here since we will be passing them to camera service
    // within the scope of this function
    result.frameNumber = frameNumber;
    result.fmqResultSize = 0;
    result.partialResult = hal_result->partial_result;
    convertToAidl(hal_result->result, &result.result);
    if (nullptr != hal_result->result) {
        bool resultOverridden = false;
        Mutex::Autolock _l(mInflightLock);

        // Derive some new keys for backward compatibility
        if (mDerivePostRawSensKey) {
            camera_metadata_ro_entry entry;
            if (find_camera_metadata_ro_entry(hal_result->result,
                    ANDROID_CONTROL_POST_RAW_SENSITIVITY_BOOST, &entry) == 0) {
                mInflightRawBoostPresent[frameNumber] = true;
            } else {
                auto entry = mInflightRawBoostPresent.find(frameNumber);
                if (mInflightRawBoostPresent.end() == entry) {
                    mInflightRawBoostPresent[frameNumber] = false;
                }
            }
            if ((hal_result->partial_result == mNumPartialResults)) {
                if (!mInflightRawBoostPresent[frameNumber]) {
                    if (!resultOverridden) {
                        mOverriddenResult.clear();
                        mOverriddenResult.append(hal_result->result);
                        resultOverridden = true;
                    }
                    int32_t defaultBoost[1] = {100};
                    mOverriddenResult.update(
                            ANDROID_CONTROL_POST_RAW_SENSITIVITY_BOOST,
                            defaultBoost, 1);
                }

                mInflightRawBoostPresent.erase(frameNumber);
            }
        }
        auto entry = mInflightAETriggerOverrides.find(frameNumber);
        if (mInflightAETriggerOverrides.end() != entry) {
            if (!resultOverridden) {
                mOverriddenResult.clear();
                mOverriddenResult.append(hal_result->result);
                resultOverridden = true;
            }
            overrideResultForPrecaptureCancelLocked(entry->second,
                    &mOverriddenResult);
            if (hal_result->partial_result == mNumPartialResults) {
                mInflightAETriggerOverrides.erase(frameNumber);
            }
        }

        if (resultOverridden) {
            const camera_metadata_t *metaBuffer =
                    mOverriddenResult.getAndLock();
            convertToAidl(metaBuffer, &result.result);
            mOverriddenResult.unlock(metaBuffer);
        }
    }
    if (hasInputBuf) {
        result.inputBuffer.streamId =
                static_cast<AmlCameraStream*>(hal_result->input_buffer->stream)->mId;
        //result.inputBuffer.buffer = nullptr;
        result.inputBuffer.status = (BufferStatus) hal_result->input_buffer->status;
        // skip acquire fence since it's no use to camera service
        if (hal_result->input_buffer->release_fence != -1) {
            native_handle_t* handle = native_handle_create(/*numFds*/1, /*numInts*/0);
            handle->data[0] = hal_result->input_buffer->release_fence;
            result.inputBuffer.releaseFence = android::makeToAidl(handle);
        }
    } else {
        result.inputBuffer.streamId = -1;
    }

    result.outputBuffers.resize(numOutputBufs);
    for (size_t i = 0; i < numOutputBufs; i++) {
        result.outputBuffers[i].streamId =
                static_cast<AmlCameraStream*>(hal_result->output_buffers[i].stream)->mId;
        //result.outputBuffers[i].buffer = nullptr;
        if (hal_result->output_buffers[i].buffer != nullptr) {
            result.outputBuffers[i].bufferId = getCapResultBufferId(
                    *(hal_result->output_buffers[i].buffer),
                    result.outputBuffers[i].streamId);
        } else {
            result.outputBuffers[i].bufferId = 0;
        }

        result.outputBuffers[i].status = (BufferStatus) hal_result->output_buffers[i].status;
        // skip acquire fence since it's of no use to camera service
        if (hal_result->output_buffers[i].release_fence != -1) {
            native_handle_t* handle = native_handle_create(/*numFds*/1, /*numInts*/0);
            handle->data[0] = hal_result->output_buffers[i].release_fence;
            result.outputBuffers[i].releaseFence = android::makeToAidl(handle);
        }
    }

    // Free inflight record/fences.
    // Do this before call back to camera service because camera service might jump to
    // configure_streams right after the processCaptureResult call so we need to finish
    // updating inflight queues first
    if (numBufs > 0) {
        Mutex::Autolock _l(mInflightLock);
        if (hasInputBuf) {
            int streamId = static_cast<AmlCameraStream*>(hal_result->input_buffer->stream)->mId;
            auto key = std::make_pair(streamId, frameNumber);
            mInflightBuffers.erase(key);
        }

        for (size_t i = 0; i < numOutputBufs; i++) {
            int streamId = static_cast<AmlCameraStream*>(hal_result->output_buffers[i].stream)->mId;
            auto key = std::make_pair(streamId, frameNumber);
            mInflightBuffers.erase(key);
        }

        if (mInflightBuffers.empty()) {
            ALOGV("%s: inflight buffer queue is now empty!", __FUNCTION__);
        }
    }
    return OK;
}

// Static helper method to copy/shrink capture result metadata sent by HAL
void AmlogicCameraDeviceSession::sShrinkCaptureResult(
        aml_camera_capture_result* dst, const aml_camera_capture_result* src,
        std::vector<::android::hardware::camera::common::V1_0::helper::CameraMetadata>* mds,
        std::vector<const camera_metadata_t*>* physCamMdArray,
        bool handlePhysCam) {
    *dst = *src;
    // Reserve maximum number of entries to avoid metadata re-allocation.
    mds->reserve(1 + (handlePhysCam ? src->num_physcam_metadata : 0));
    if (sShouldShrink(src->result)) {
        mds->emplace_back(sCreateCompactCopy(src->result));
        dst->result = mds->back().getAndLock();
    }

    if (handlePhysCam) {
        // First determine if we need to create new camera_metadata_t* array
        bool needShrink = false;
        for (uint32_t i = 0; i < src->num_physcam_metadata; i++) {
            if (sShouldShrink(src->physcam_metadata[i])) {
                needShrink = true;
            }
        }

        if (!needShrink) return;

        physCamMdArray->reserve(src->num_physcam_metadata);
        dst->physcam_metadata = physCamMdArray->data();
        for (uint32_t i = 0; i < src->num_physcam_metadata; i++) {
            if (sShouldShrink(src->physcam_metadata[i])) {
                mds->emplace_back(sCreateCompactCopy(src->physcam_metadata[i]));
                dst->physcam_metadata[i] = mds->back().getAndLock();
            } else {
                dst->physcam_metadata[i] = src->physcam_metadata[i];
            }
        }
    }
}

bool AmlogicCameraDeviceSession::sShouldShrink(const camera_metadata_t* md) {
    size_t compactSize = get_camera_metadata_compact_size(md);
    size_t totalSize = get_camera_metadata_size(md);
    if (totalSize >= compactSize + METADATA_SHRINK_ABS_THRESHOLD &&
            totalSize >= compactSize * METADATA_SHRINK_REL_THRESHOLD) {
        ALOGV("Camera metadata should be shrunk from %zu to %zu", totalSize, compactSize);
        return true;
    }
    return false;
}

camera_metadata_t* AmlogicCameraDeviceSession::sCreateCompactCopy(const camera_metadata_t* src) {
    size_t compactSize = get_camera_metadata_compact_size(src);
    void* buffer = calloc(1, compactSize);
    if (buffer == nullptr) {
        ALOGE("%s: Allocating %zu bytes failed", __FUNCTION__, compactSize);
    }
    return copy_camera_metadata(buffer, compactSize, src);
}

/**
 * Static callback forwarding methods from HAL to instance
 */
void AmlogicCameraDeviceSession::sProcessCaptureResult(
        const aml_camera_callback_ops *cb,
        const aml_camera_capture_result *hal_result) {
    AmlogicCameraDeviceSession *d =
            const_cast<AmlogicCameraDeviceSession*>(static_cast<const AmlogicCameraDeviceSession*>(cb));
    CaptureResult result = {};
    aml_camera_capture_result shadowResult;
    bool handlePhysCam = (d->mDeviceVersion >= CAMERA_DEVICE_API_VERSION_3_5);
    std::vector<::android::hardware::camera::common::V1_0::helper::CameraMetadata> compactMds;
    std::vector<const camera_metadata_t*> physCamMdArray;
    sShrinkCaptureResult(&shadowResult, hal_result, &compactMds, &physCamMdArray, handlePhysCam);
    //ALOGE("AmlogicCameraDeviceSession::sProcessCaptureResult %d %d",result.result.metadata.data(), result.result.metadata.size());
    status_t ret = d->constructCaptureResult(result, &shadowResult);
    if (ret == OK) {
        d->mResultBatcher.processCaptureResult(result);
    }
}

void AmlogicCameraDeviceSession::sNotify(
        const aml_camera_callback_ops *cb,
        const aml_notify_message_t *msg) {
    AmlogicCameraDeviceSession *d =
            const_cast<AmlogicCameraDeviceSession*>(static_cast<const AmlogicCameraDeviceSession*>(cb));
    NotifyMsg aidlMsg;
    convertToAidl(msg, &aidlMsg);

    if (aidlMsg.getTag() == NotifyMsg::Tag::error) {
        ErrorMsg error = aidlMsg.get<NotifyMsg::Tag::error>();
        if (error.errorStreamId != -1 && d->mStreamMap.count(error.errorStreamId) != 1) {
            ALOGE("%s: unknown stream ID %d reports an error!",
                    __FUNCTION__, error.errorStreamId);
            return;
        }
    }

    if (aidlMsg.getTag() == NotifyMsg::Tag::error) {
        ErrorMsg error = aidlMsg.get<NotifyMsg::Tag::error>();
        switch (error.errorCode) {
            case ErrorCode::ERROR_DEVICE:
            case ErrorCode::ERROR_REQUEST:
            case ErrorCode::ERROR_RESULT: {
                Mutex::Autolock _l(d->mInflightLock);
                auto entry = d->mInflightAETriggerOverrides.find(
                        error.frameNumber);
                if (d->mInflightAETriggerOverrides.end() != entry) {
                    d->mInflightAETriggerOverrides.erase(
                            error.frameNumber);
                }

                auto boostEntry = d->mInflightRawBoostPresent.find(
                        error.frameNumber);
                if (d->mInflightRawBoostPresent.end() != boostEntry) {
                    d->mInflightRawBoostPresent.erase(
                            error.frameNumber);
                }
            }
                break;
            case ErrorCode::ERROR_BUFFER:
            default:
                break;
        }

    }

    d->mResultBatcher.notify(aidlMsg);
}

}  // namespace implementation
}  // namespace device
}  // namespace camera
}  // namespace hardware
}  // namespace android
