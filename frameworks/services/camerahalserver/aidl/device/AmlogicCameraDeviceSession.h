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

#ifndef HARDWARE_INTERFACES_CAMERA_DEVICE_DEFAULT_AMLOGICCAMERADEVICESESSION_H_
#define HARDWARE_INTERFACES_CAMERA_DEVICE_DEFAULT_AMLOGICCAMERADEVICESESSION_H_

#include <ExternalCameraUtils.h>
#include <SimpleThread.h>
#include <aidl/android/hardware/camera/common/Status.h>
#include <aidl/android/hardware/camera/device/BnCameraDeviceSession.h>
#include <aidl/android/hardware/camera/device/BufferRequest.h>
#include <aidl/android/hardware/camera/device/Stream.h>
#include <aidl/android/hardware/camera/device/StreamBuffer.h>
#include <aidl/android/hardware/camera/device/NotifyMsg.h>
#include <android-base/unique_fd.h>
#include <fmq/AidlMessageQueue.h>
#include <utils/Thread.h>
#include <deque>
#include <list>
#include <map>

//#include "hardware/camera3.h"
#include "amlogic_camera.h"
#include "hardware/camera_common.h"
#include "utils/Mutex.h"
#include "HandleImporter.h"
namespace android {
namespace hardware {
namespace camera {
namespace device {
namespace implementation {
using ::aidl::android::hardware::camera::common::Status;
using ::aidl::android::hardware::camera::device::BnCameraDeviceSession;
using ::aidl::android::hardware::camera::device::BufferCache;
using ::aidl::android::hardware::camera::device::BufferRequest;
using ::aidl::android::hardware::camera::device::CameraMetadata;
using ::aidl::android::hardware::camera::device::CameraOfflineSessionInfo;
using ::aidl::android::hardware::camera::device::CaptureRequest;
using ::aidl::android::hardware::camera::device::HalStream;
using ::aidl::android::hardware::camera::device::ICameraDeviceCallback;
using ::aidl::android::hardware::camera::device::ICameraOfflineSession;
using ::aidl::android::hardware::camera::device::RequestTemplate;
using ::aidl::android::hardware::camera::device::Stream;
using ::aidl::android::hardware::camera::device::StreamBuffer;
using ::aidl::android::hardware::camera::device::NotifyMsg;
using ::aidl::android::hardware::camera::device::StreamConfiguration;
using ::aidl::android::hardware::common::fmq::MQDescriptor;
using ::aidl::android::hardware::common::fmq::SynchronizedReadWrite;
using ::android::AidlMessageQueue;
using ::android::base::unique_fd;
using ::android::hardware::camera::common::helper::SimpleThread;
//using ::android::hardware::camera::external::common::ExternalCameraConfig;
//using ::android::hardware::camera::external::common::SizeHasher;
using ::ndk::ScopedAStatus;

struct AmlCameraStream;
/**
 * Function pointer types with C calling convention to
 * use for HAL callback functions.
 */
extern "C" {
    typedef void (callbacks_process_capture_result_t)(
        const struct aml_camera_callback_ops *,
        const aml_camera_capture_result_t *);

    typedef void (callbacks_notify_t)(
        const struct aml_camera_callback_ops *,
        const aml_notify_message_t *);
}


class AmlogicCameraDeviceSession : public BnCameraDeviceSession, protected  aml_camera_callback_ops {
public:
    AmlogicCameraDeviceSession(aml_camera_device_t*,
                        const camera_metadata_t* deviceInfo,
                        const std::shared_ptr<ICameraDeviceCallback>&);
    ~AmlogicCameraDeviceSession() override;

    // Caller must use this method to check if CameraDeviceSession ctor failed
    bool isInitFailed() {return mInitFail;}
    void disconnect();
    bool isClosed();

public:
    ScopedAStatus close() override; //

    ScopedAStatus configureStreams(const StreamConfiguration& in_requestedConfiguration,
                                   std::vector<HalStream>* _aidl_return) override; //
    ScopedAStatus constructDefaultRequestSettings(RequestTemplate in_type,
                                                  CameraMetadata* _aidl_return) override; //
    ScopedAStatus flush() override; //

    ScopedAStatus getCaptureRequestMetadataQueue(
            MQDescriptor<int8_t, SynchronizedReadWrite>* _aidl_return) override; //

    ScopedAStatus getCaptureResultMetadataQueue(
            MQDescriptor<int8_t, SynchronizedReadWrite>* _aidl_return) override; //

    ScopedAStatus isReconfigurationRequired(const CameraMetadata& in_oldSessionParams,
                                            const CameraMetadata& in_newSessionParams,
                                            bool* _aidl_return) override;

    ScopedAStatus processCaptureRequest(const std::vector<CaptureRequest>& in_requests,
                                        const std::vector<BufferCache>& in_cachesToRemove,
                                        int32_t* _aidl_return) override; //

    ScopedAStatus signalStreamFlush(const std::vector<int32_t>& in_streamIds,
                                    int32_t in_streamConfigCounter) override;
    ScopedAStatus switchToOffline(const std::vector<int32_t>& in_streamsToKeep,
                                  CameraOfflineSessionInfo* out_offlineSessionInfo,
                                  std::shared_ptr<ICameraOfflineSession>* _aidl_return) override;
    ScopedAStatus repeatingRequestEnd(int32_t in_frameNumber,
                                      const std::vector<int32_t>& in_streamIds) override;


public:
  //Help methods
  bool preProcessConfigurationLocked(const StreamConfiguration& requestedConfiguration,
            aml_camera_stream_configuration_t *stream_list /*out*/,
            std::vector<aml_camera_stream_t*> *streams /*out*/);

  void postProcessConfigurationLocked(
        const StreamConfiguration& requestedConfiguration);

  void postProcessConfigurationFailureLocked(
        const StreamConfiguration& requestedConfiguration);
  Status constructDefaultRequestSettingsRaw(int type, CameraMetadata *outMetadata);

protected:
    mutable Mutex mStateLock;  // Protect all private members except otherwise noted
    bool mClosed = false;
    bool mDisconnected = false;

    struct AETriggerCancelOverride {
        bool applyAeLock;
        uint8_t aeLock;
        bool applyAePrecaptureTrigger;
        uint8_t aePrecaptureTrigger;
    };

    aml_camera_device_t* mDevice;
    const uint32_t mDeviceVersion;
    const bool mFreeBufEarly;
    bool mIsAELockAvailable;
    bool mDerivePostRawSensKey;
    uint32_t mNumPartialResults;
    // Stream ID -> AmlCameraStream cache
    std::map<int, AmlCameraStream> mStreamMap;

    mutable Mutex mInflightLock; // protecting mInflightBuffers and mCirculatingBuffers
    // (streamID, frameNumber) -> inflight buffer cache
    std::map<std::pair<int, uint32_t>, aml_camera_stream_buffer_t>  mInflightBuffers;

    // (frameNumber, AETriggerOverride) -> inflight request AETriggerOverrides
    std::map<uint32_t, AETriggerCancelOverride> mInflightAETriggerOverrides;
    ::android::hardware::camera::common::V1_0::helper::CameraMetadata mOverriddenResult;
    std::map<uint32_t, bool> mInflightRawBoostPresent;
    ::android::hardware::camera::common::V1_0::helper::CameraMetadata mOverriddenRequest;

    static const uint64_t BUFFER_ID_NO_BUFFER = 0;
    // buffers currently circulating between HAL and camera service
    // key: bufferId sent via HIDL interface
    // value: imported buffer_handle_t
    // Buffer will be imported during process_capture_request and will be freed
    // when the its stream is deleted or camera device session is closed
    typedef std::unordered_map<uint64_t, buffer_handle_t> CirculatingBuffers;
    // Stream ID -> circulating buffers map
    std::map<int, CirculatingBuffers> mCirculatingBuffers;
    // Protect mCirculatingBuffers, must not lock mLock after acquiring this lock
    mutable Mutex mCbsLock;

    static HandleImporter sHandleImporter;
    static buffer_handle_t sEmptyBuffer;

    bool mInitFail;
    bool mFirstRequest = false;

    common::V1_0::helper::CameraMetadata mDeviceInfo;

    mutable Mutex mLock;  // Protect all private members except otherwise noted
    const std::shared_ptr<ICameraDeviceCallback> mCallback;

    /* Beginning of members not changed after initialize() */
    using RequestMetadataQueue = AidlMessageQueue<int8_t, SynchronizedReadWrite>;
    std::unique_ptr<RequestMetadataQueue> mRequestMetadataQueue;
    using ResultMetadataQueue = AidlMessageQueue<int8_t, SynchronizedReadWrite>;
    std::shared_ptr<ResultMetadataQueue> mResultMetadataQueue;

    // Protect against invokeProcessCaptureResultCallback()
    Mutex mProcessCaptureResultLock;

    class ResultBatcher {
    public:
        ResultBatcher(const std::shared_ptr<ICameraDeviceCallback>& callback);
        void setNumPartialResults(uint32_t n);
        void setBatchedStreams(const std::vector<int>& streamsToBatch);
        void setResultMetadataQueue(std::shared_ptr<ResultMetadataQueue> q);

        void registerBatch(uint32_t frameNumber, uint32_t batchSize);
        void notify(NotifyMsg& msg);
        void processCaptureResult(CaptureResult& result);

    protected:
        struct InflightBatch {
            // Protect access to entire struct. Acquire this lock before read/write any data or
            // calling any methods. processCaptureResult and notify will compete for this lock
            // HIDL IPCs might be issued while the lock is held
            Mutex mLock;

            bool allDelivered() const;

            uint32_t mFirstFrame;
            uint32_t mLastFrame;
            uint32_t mBatchSize;

            bool mShutterDelivered = false;
            std::vector<NotifyMsg> mShutterMsgs;

            struct BufferBatch {
                BufferBatch(uint32_t batchSize) {
                    mBuffers.reserve(batchSize);
                }
                bool mDelivered = false;
                // This currently assumes every batched request will output to the batched stream
                // and since HAL must always send buffers in order, no frameNumber tracking is
                // needed
                std::vector<StreamBuffer> mBuffers;
            };
            // Stream ID -> VideoBatch
            std::unordered_map<int, BufferBatch> mBatchBufs;

            struct MetadataBatch {
                //                   (frameNumber, metadata)
                std::vector<std::pair<uint32_t, CameraMetadata>> mMds;
            };
            // Partial result IDs that has been delivered to framework
            uint32_t mNumPartialResults;
            uint32_t mPartialResultProgress = 0;
            // partialResult -> MetadataBatch
            std::map<uint32_t, MetadataBatch> mResultMds;

            // Set to true when batch is removed from mInflightBatches
            // processCaptureResult and notify must check this flag after acquiring mLock to make
            // sure this batch isn't removed while waiting for mLock
            bool mRemoved = false;
        };


        // Get the batch index and pointer to InflightBatch (nullptr if the frame is not batched)
        // Caller must acquire the InflightBatch::mLock before accessing the InflightBatch
        // It's possible that the InflightBatch is removed from mInflightBatches before the
        // InflightBatch::mLock is acquired (most likely caused by an error notification), so
        // caller must check InflightBatch::mRemoved flag after the lock is acquired.
        // This method will hold ResultBatcher::mLock briefly
        std::pair<int, std::shared_ptr<InflightBatch>> getBatch(uint32_t frameNumber);

        static const int NOT_BATCHED = -1;

        // move/push function avoids "hidl_handle& operator=(hidl_handle&)", which clones native
        // handle
        void moveStreamBuffer(StreamBuffer&& src, StreamBuffer& dst);
        void pushStreamBuffer(StreamBuffer&& src, std::vector<StreamBuffer>& dst);

        void sendBatchMetadataLocked(
                std::shared_ptr<InflightBatch> batch, uint32_t lastPartialResultIdx);

        // Check if the first batch in mInflightBatches is ready to be removed, and remove it if so
        // This method will hold ResultBatcher::mLock briefly
        void checkAndRemoveFirstBatch();

        // The following sendXXXX methods must be called while the InflightBatch::mLock is locked
        // HIDL IPC methods will be called during these methods.
        void sendBatchShutterCbsLocked(std::shared_ptr<InflightBatch> batch);
        // send buffers for all batched streams
        void sendBatchBuffersLocked(std::shared_ptr<InflightBatch> batch);
        // send buffers for specified streams
        void sendBatchBuffersLocked(
                std::shared_ptr<InflightBatch> batch, const std::vector<int>& streams);
       // End of sendXXXX methods

        // helper methods
        // void freeReleaseFences(std::vector<CaptureResult>&);
        void notifySingleMsg(NotifyMsg& msg);
        void processOneCaptureResult(CaptureResult& result);
        void invokeProcessCaptureResultCallback(std::vector<CaptureResult> &results, bool tryWriteFmq);

        // Protect access to mInflightBatches, mNumPartialResults and mStreamsToBatch
        // processCaptureRequest, processCaptureResult, notify will compete for this lock
        // Do NOT issue HIDL IPCs while holding this lock (except when HAL reports error)
        mutable Mutex mLock;
        std::deque<std::shared_ptr<InflightBatch>> mInflightBatches;
        uint32_t mNumPartialResults;
        std::vector<int> mStreamsToBatch;
        std::shared_ptr<ICameraDeviceCallback> mCallback;
        std::shared_ptr<ResultMetadataQueue> mResultMetadataQueue;

        // Protect against invokeProcessCaptureResultCallback()
        Mutex mProcessCaptureResultLock;

    } mResultBatcher;

    std::vector<int> mVideoStreamIds;

    bool initialize();
    Status initStatus() const;
    static bool shouldFreeBufEarly();

    Status importRequest(
      const CaptureRequest& request,
      std::vector<buffer_handle_t*>& allBufPtrs,
      std::vector<int>& allFences);

    Status importRequestImpl(
      const CaptureRequest& request,
      std::vector<buffer_handle_t*>& allBufPtrs,
      std::vector<int>& allFences);

    Status importBuffer(int32_t streamId, uint64_t bufId, buffer_handle_t buf,
                        buffer_handle_t** outBufPtr);

    Status importBufferLocked(int32_t streamId, uint64_t bufId, buffer_handle_t buf,
                                    /*out*/ buffer_handle_t** outBufPtr);

    static void cleanupInflightFences(std::vector<int>& allFences, size_t numFences);
    void cleanupBuffersLocked(int id);

    void notifyShutter(int32_t frameNumber, nsecs_t shutterTs);

    void notifyError(int32_t frameNumber, int32_t streamId, ErrorCode ec);

    void invokeProcessCaptureResultCallback(
            std::vector<CaptureResult>& results, bool tryWriteFmq);

    void updateBufferCaches(const std::vector<BufferCache>& cachesToRemove);

    android_dataspace mapToLegacyDataspace(
        android_dataspace dataSpace) const;

    bool handleAePrecaptureCancelRequestLocked(
            const aml_camera_capture_request_t &halRequest,
            android::hardware::camera::common::V1_0::helper::CameraMetadata *settings /*out*/,
            AETriggerCancelOverride *override /*out*/);

    void overrideResultForPrecaptureCancelLocked(
            const AETriggerCancelOverride &aeTriggerCancelOverride,
            ::android::hardware::camera::common::V1_0::helper::CameraMetadata *settings /*out*/);

    Status processOneCaptureRequest(const CaptureRequest& request);
    /**
     * Static callback forwarding methods from HAL to instance
     */
    static callbacks_process_capture_result_t sProcessCaptureResult;
    static callbacks_notify_t sNotify;

    Status switchToOffline(const std::vector<int32_t>& offlineStreams,
                           /*out*/ std::vector<NotifyMsg>* msgs,
                           /*out*/ std::vector<CaptureResult>* results,
                           /*out*/ CameraOfflineSessionInfo* info,
                           /*out*/ std::shared_ptr<ICameraOfflineSession>* session);

    bool supportOfflineLocked(int32_t streamId);

    // By default camera service uses frameNumber/streamId pair to retrieve the buffer that
    // was sent to HAL. Override this implementation if HAL is using buffers from buffer management
    // APIs to send output buffer.
    virtual uint64_t getCapResultBufferId(const buffer_handle_t& buf, int streamId);

    status_t constructCaptureResult(CaptureResult& result,
                                const aml_camera_capture_result *hal_result);

    // Static helper method to copy/shrink capture result metadata sent by HAL
    // Temporarily allocated metadata copy will be hold in mds
    static void sShrinkCaptureResult(
            aml_camera_capture_result* dst, const aml_camera_capture_result* src,
            std::vector<::android::hardware::camera::common::V1_0::helper::CameraMetadata>* mds,
            std::vector<const camera_metadata_t*>* physCamMdArray,
            bool handlePhysCam);
    static bool sShouldShrink(const camera_metadata_t* md);
    static camera_metadata_t* sCreateCompactCopy(const camera_metadata_t* src);

};

}  // namespace implementation
}  // namespace device
}  // namespace camera
}  // namespace hardware
}  // namespace android

#endif  // HARDWARE_INTERFACES_CAMERA_DEVICE_DEFAULT_AMLOGICCAMERADEVICESESSION_H_
