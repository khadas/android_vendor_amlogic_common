#ifndef ANDROID_INCLUDE_AML_CAMERA_H
#define ANDROID_INCLUDE_AML_CAMERA_H

#include <system/camera_metadata.h>
#include "hardware/camera_common.h"

extern "C" {
struct aml_camera_device;

typedef enum aml_camera_stream_type {
    AML_CAMERA_STREAM_OUTPUT = 0,
    AML_CAMERA_STREAM_INPUT = 1,
    AML_CAMERA_STREAM_BIDIRECTIONAL = 2,
    AML_CAMERA_NUM_STREAM_TYPES
} aml_camera_stream_type_t;


typedef enum aml_camera_stream_rotation {
    /* No rotation */
    AML_CAMERA_STREAM_ROTATION_0 = 0,

    /* Rotate by 90 degree counterclockwise */
    AML_CAMERA_STREAM_ROTATION_90 = 1,

    /* Rotate by 180 degree counterclockwise */
    AML_CAMERA_STREAM_ROTATION_180 = 2,

    /* Rotate by 270 degree counterclockwise */
    AML_CAMERA_STREAM_ROTATION_270 = 3
} aml_camera_stream_rotation_t;


typedef enum aml_camera_stream_configuration_mode {
    AML_CAMERA_STREAM_CONFIGURATION_NORMAL_MODE = 0,
    AML_CAMERA_STREAM_CONFIGURATION_CONSTRAINED_HIGH_SPEED_MODE = 1,
    AML_CAMERA_VENDOR_STREAM_CONFIGURATION_MODE_START = 0x8000
} aml_camera_stream_configuration_mode_t;


typedef struct aml_camera_stream {
    int stream_type;
    uint32_t width;
    uint32_t height;
    int format;
    uint32_t usage;
    uint32_t max_buffers;
    void *priv;
    android_dataspace_t data_space;
    int rotation;
    const char* physical_camera_id;
    void *reserved[6];
} aml_camera_stream_t;


typedef struct aml_camera_stream_configuration {
    uint32_t num_streams;
    aml_camera_stream_t **streams;
    uint32_t operation_mode;
    const camera_metadata_t *session_parameters;
} aml_camera_stream_configuration_t;

typedef enum aml_camera_buffer_status {
    AML_CAMERA_BUFFER_STATUS_OK = 0,
    AML_CAMERA_BUFFER_STATUS_ERROR = 1,
} aml_camera_buffer_status_t;


typedef struct aml_camera_stream_buffer {
    aml_camera_stream_t *stream;
    buffer_handle_t *buffer;
    int status;
    int acquire_fence;
    int release_fence;
} aml_camera_stream_buffer_t;


typedef struct aml_camera_stream_buffer_set {
    aml_camera_stream_t *stream;
    uint32_t num_buffers;
    buffer_handle_t **buffers;
} aml_camera_stream_buffer_set_t;


typedef struct aml_camera_jpeg_blob {
    uint16_t jpeg_blob_id;
    uint32_t jpeg_size;
} aml_camera_jpeg_blob_t;

enum {
    AML_CAMERA_JPEG_BLOB_ID = 0x00FF,
    AML_CAMERA_JPEG_APP_SEGMENTS_BLOB_ID = 0x0100,
};

typedef enum aml_camera_msg_type {
    /**
     * An error has occurred. camera3_notify_msg.message.error contains the
     * error information.
     */
    AML_CAMERA_MSG_ERROR = 1,

    /**
     * The exposure of a given request or processing a reprocess request has
     * begun. camera3_notify_msg.message.shutter contains the information
     * the capture.
     */
    AML_CAMERA_MSG_SHUTTER = 2,

    /**
     * Number of framework message types
     */
    AML_CAMERA_NUM_MESSAGES
} aml_camera_msg_type_t;

typedef enum aml_camera_error_msg_code {
    AML_CAMERA_MSG_ERROR_DEVICE = 1,
    AML_CAMERA_MSG_ERROR_REQUEST = 2,
    AML_CAMERA_MSG_ERROR_RESULT = 3,
    AML_CAMERA_MSG_ERROR_BUFFER = 4,
    ALM_CAMERA_MSG_NUM_ERRORS
} aml_camera_error_msg_code_t;


typedef struct aml_camera_error_msg {
    uint32_t frame_number;
    int32_t error_stream_id;
    int error_code;
} aml_camera_error_msg_t;

typedef struct aml_camera_shutter_msg {
    uint32_t frame_number;
    uint64_t timestamp; //exposure time
    uint64_t readout_timestamp;
} aml_camera_shutter_msg_t;

typedef struct aml_notify_message {
    int type;
    aml_camera_error_msg_t error;
    aml_camera_shutter_msg_t shutter;
} aml_notify_message_t;

typedef enum aml_camera_buffer_request_status {
    /**
     * request_stream_buffers() call succeeded and all requested buffers are
     * returned.
     */
    AML_CAMERA_BUF_REQ_OK = 0,

    /**
     * request_stream_buffers() call failed for some streams.
     * Check per stream status for each returned camera3_stream_buffer_ret_t.
     */
    AML_CAMERA_BUF_REQ_FAILED_PARTIAL = 1,

    /**
     * request_stream_buffers() call failed for all streams and no buffers are
     * returned at all. Camera service is about to or is performing
     * configure_streams() call. HAL must wait until next configure_streams()
     * call is finished before requesting buffers again.
     */
    AML_CAMERA_BUF_REQ_FAILED_CONFIGURING = 2,

    /**
     * request_stream_buffers() call failed for all streams and no buffers are
     * returned at all. Failure due to bad camera3_buffer_request input, eg:
     * unknown stream or repeated stream in the list of buffer requests.
     */
    AML_CAMERA_BUF_REQ_FAILED_ILLEGAL_ARGUMENTS = 3,

    /**
     * request_stream_buffers() call failed for all streams and no buffers are
     * returned at all. This can happen for unknown reasons or a combination
     * of different failure reasons per stream. For the latter case, caller can
     * check per stream failure reason returned in camera3_stream_buffer_ret.
     */
    AML_CAMERA_BUF_REQ_FAILED_UNKNOWN = 4,

    /**
     * Number of buffer request status
     */
    AML_CAMERA_BUF_REQ_NUM_STATUS

} aml_camera_buffer_request_status_t;

typedef enum aml_camera_stream_buffer_req_status {
    /**
     * Get buffer succeeds and all requested buffers are returned.
     */
    AML_CAMERA_PS_BUF_REQ_OK = 0,

    /**
     * Get buffer failed due to timeout waiting for an available buffer. This is
     * likely due to the client application holding too many buffers, or the
     * system is under memory pressure.
     * This is not a fatal error. HAL can try to request buffer for this stream
     * later. If HAL cannot get a buffer for certain capture request in time
     * due to this error, HAL can send an ERROR_REQUEST to camera service and
     * drop processing that request.
     */
    AML_CAMERA_PS_BUF_REQ_NO_BUFFER_AVAILABLE = 1,

    /**
     * Get buffer failed due to HAL has reached its maxBuffer count. This is not
     * a fatal error. HAL can try to request buffer for this stream again after
     * it returns at least one buffer of that stream to camera service.
     */
    AML_CAMERA_PS_BUF_REQ_MAX_BUFFER_EXCEEDED = 2,

    /**
     * Get buffer failed due to the stream is disconnected by client
     * application, has been removed, or not recognized by camera service.
     * This means application is no longer interested in this stream.
     * Requesting buffer for this stream will never succeed after this error is
     * returned. HAL must safely return all buffers of this stream after
     * getting this error. If HAL gets another capture request later targeting
     * a disconnected stream, HAL must send an ERROR_REQUEST to camera service
     * and drop processing that request.
     */
    AML_CAMERA_PS_BUF_REQ_STREAM_DISCONNECTED = 3,

    /**
     * Get buffer failed for unknown reason. This is a fatal error and HAL must
     * send ERROR_DEVICE to camera service and be ready to be closed.
     */
    AML_CAMERA_PS_BUF_REQ_UNKNOWN_ERROR = 4,

    /**
     * Number of buffer request status
     */
    AML_CAMERA_PS_BUF_REQ_NUM_STATUS
} aml_camera_stream_buffer_req_status_t;

typedef struct aml_camera_buffer_request {
    /**
     * The stream HAL wants to request buffer from
     */
    aml_camera_stream_t *stream;

    /**
     * The number of buffers HAL requested
     */
    uint32_t num_buffers_requested;
} aml_camera_buffer_request_t;

typedef struct aml_camera_stream_buffer_ret {
    /**
     * The stream HAL wants to request buffer from
     */
    aml_camera_stream_t *stream;

    /**
     * The status of buffer request of this stream
     */
    aml_camera_stream_buffer_req_status_t status;

    /**
     * Number of output buffers returned. Must be 0 when above status is not
     * CAMERA3_PS_BUF_REQ_OK; otherwise the value must be equal to
     * num_buffers_requested in the corresponding camera3_buffer_request_t
     */
    uint32_t num_output_buffers;

    /**
     * The returned output buffers for the stream.
     * Caller of request_stream_buffers() should supply this with enough memory
     * (num_buffers_requested * sizeof(camera3_stream_buffer_t))
     */
    aml_camera_stream_buffer_t *output_buffers;
} aml_camera_stream_buffer_ret_t;


typedef enum aml_camera_request_template {
    /**
     * Standard camera preview operation with 3A on auto.
     */
    AML_CAMERA_TEMPLATE_PREVIEW = 1,

    /**
     * Standard camera high-quality still capture with 3A and flash on auto.
     */
    AML_CAMERA_TEMPLATE_STILL_CAPTURE = 2,

    /**
     * Standard video recording plus preview with 3A on auto, torch off.
     */
    AML_CAMERA_TEMPLATE_VIDEO_RECORD = 3,

    /**
     * High-quality still capture while recording video. Application will
     * include preview, video record, and full-resolution YUV or JPEG streams in
     * request. Must not cause stuttering on video stream. 3A on auto.
     */
    AML_CAMERA_TEMPLATE_VIDEO_SNAPSHOT = 4,

    /**
     * Zero-shutter-lag mode. Application will request preview and
     * full-resolution data for each frame, and reprocess it to JPEG when a
     * still image is requested by user. Settings should provide highest-quality
     * full-resolution images without compromising preview frame rate. 3A on
     * auto.
     */
    AML_CAMERA_TEMPLATE_ZERO_SHUTTER_LAG = 5,

    /**
     * A basic template for direct application control of capture
     * parameters. All automatic control is disabled (auto-exposure, auto-white
     * balance, auto-focus), and post-processing parameters are set to preview
     * quality. The manual capture parameters (exposure, sensitivity, etc.)
     * are set to reasonable defaults, but should be overridden by the
     * application depending on the intended use case.
     */
    AML_CAMERA_TEMPLATE_MANUAL = 6,

    /* Total number of templates */
    AML_CAMERA_TEMPLATE_COUNT,

    /**
     * First value for vendor-defined request templates
     */
    AML_CAMERA_VENDOR_TEMPLATE_START = 0x40000000

} aml_camera_request_template_t;


typedef struct aml_camera_capture_request {
    /**
     * The frame number is an incrementing integer set by the framework to
     * uniquely identify this capture. It needs to be returned in the result
     * call, and is also used to identify the request in asynchronous
     * notifications sent to camera3_callback_ops_t.notify().
     */
    uint32_t frame_number;

    /**
     * The settings buffer contains the capture and processing parameters for
     * the request. As a special case, a NULL settings buffer indicates that the
     * settings are identical to the most-recently submitted capture request. A
     * NULL buffer cannot be used as the first submitted request after a
     * configure_streams() call.
     */
    const camera_metadata_t *settings;

    /**
     * The input stream buffer to use for this request, if any.
     *
     * If input_buffer is NULL, then the request is for a new capture from the
     * imager. If input_buffer is valid, the request is for reprocessing the
     * image contained in input_buffer.
     *
     * In the latter case, the HAL must set the release_fence of the
     * input_buffer to a valid sync fence, or to -1 if the HAL does not support
     * sync, before process_capture_request() returns.
     *
     * The HAL is required to wait on the acquire sync fence of the input buffer
     * before accessing it.
     *
     * <= CAMERA_DEVICE_API_VERSION_3_1:
     *
     * Any input buffer included here will have been registered with the HAL
     * through register_stream_buffers() before its inclusion in a request.
     *
     * >= CAMERA_DEVICE_API_VERSION_3_2:
     *
     * The buffers will not have been pre-registered with the HAL.
     * Subsequent requests may reuse buffers, or provide entirely new buffers.
     */
    aml_camera_stream_buffer_t *input_buffer;

    /**
     * The number of output buffers for this capture request. Must be at least
     * 1.
     */
    uint32_t num_output_buffers;

    /**
     * An array of num_output_buffers stream buffers, to be filled with image
     * data from this capture/reprocess. The HAL must wait on the acquire fences
     * of each stream buffer before writing to them.
     *
     * The HAL takes ownership of the actual buffer_handle_t entries in
     * output_buffers; the framework does not access them until they are
     * returned in a camera3_capture_result_t.
     *
     * <= CAMERA_DEVICE_API_VERSION_3_1:
     *
     * All the buffers included  here will have been registered with the HAL
     * through register_stream_buffers() before their inclusion in a request.
     *
     * >= CAMERA_DEVICE_API_VERSION_3_2:
     *
     * Any or all of the buffers included here may be brand new in this
     * request (having never before seen by the HAL).
     */
    const aml_camera_stream_buffer_t *output_buffers;

    /**
     * <= CAMERA_DEVICE_API_VERSION_3_4:
     *
     *    Not defined and must not be accessed.
     *
     * >= CAMERA_DEVICE_API_VERSION_3_5:
     *    The number of physical camera settings to be applied. If 'num_physcam_settings'
     *    equals 0 or a physical device is not included, then Hal must decide the
     *    specific physical device settings based on the default 'settings'.
     */
    uint32_t num_physcam_settings;

    /**
     * <= CAMERA_DEVICE_API_VERSION_3_4:
     *
     *    Not defined and must not be accessed.
     *
     * >= CAMERA_DEVICE_API_VERSION_3_5:
     *    The physical camera ids. The array will contain 'num_physcam_settings'
     *    camera id strings for all physical devices that have specific settings.
     *    In case some id is invalid, the process capture request must fail and return
     *    -EINVAL.
     */
    const char **physcam_id;

    /**
     * <= CAMERA_DEVICE_API_VERSION_3_4:
     *
     *    Not defined and must not be accessed.
     *
     * >= CAMERA_DEVICE_API_VERSION_3_5:
     *    The capture settings for the physical cameras. The array will contain
     *    'num_physcam_settings' settings for individual physical devices. In
     *    case the settings at some particular index are empty, the process capture
     *    request must fail and return -EINVAL.
     */
    const camera_metadata_t **physcam_settings;

} aml_camera_capture_request_t;

typedef struct aml_camera_capture_result {
    /**
     * The frame number is an incrementing integer set by the framework in the
     * submitted request to uniquely identify this capture. It is also used to
     * identify the request in asynchronous notifications sent to
     * camera3_callback_ops_t.notify().
    */
    uint32_t frame_number;

    /**
     * The result metadata for this capture. This contains information about the
     * final capture parameters, the state of the capture and post-processing
     * hardware, the state of the 3A algorithms, if enabled, and the output of
     * any enabled statistics units.
     *
     * Only one call to process_capture_result() with a given frame_number may
     * include the result metadata. All other calls for the same frame_number
     * must set this to NULL.
     *
     * If there was an error producing the result metadata, result must be an
     * empty metadata buffer, and notify() must be called with ERROR_RESULT.
     *
     * >= CAMERA_DEVICE_API_VERSION_3_2:
     *
     * Multiple calls to process_capture_result() with a given frame_number
     * may include the result metadata.
     *
     * Partial metadata submitted should not include any metadata key returned
     * in a previous partial result for a given frame. Each new partial result
     * for that frame must also set a distinct partial_result value.
     *
     * If notify has been called with ERROR_RESULT, all further partial
     * results for that frame are ignored by the framework.
     */
    const camera_metadata_t *result;

    /**
     * The number of output buffers returned in this result structure. Must be
     * less than or equal to the matching capture request's count. If this is
     * less than the buffer count in the capture request, at least one more call
     * to process_capture_result with the same frame_number must be made, to
     * return the remaining output buffers to the framework. This may only be
     * zero if the structure includes valid result metadata or an input buffer
     * is returned in this result.
     */
    uint32_t num_output_buffers;

    /**
     * The handles for the output stream buffers for this capture. They may not
     * yet be filled at the time the HAL calls process_capture_result(); the
     * framework will wait on the release sync fences provided by the HAL before
     * reading the buffers.
     *
     * The HAL must set the stream buffer's release sync fence to a valid sync
     * fd, or to -1 if the buffer has already been filled.
     *
     * If the HAL encounters an error while processing the buffer, and the
     * buffer is not filled, the buffer's status field must be set to
     * CAMERA3_BUFFER_STATUS_ERROR. If the HAL did not wait on the acquire fence
     * before encountering the error, the acquire fence should be copied into
     * the release fence, to allow the framework to wait on the fence before
     * reusing the buffer.
     *
     * The acquire fence must be set to -1 for all output buffers.  If
     * num_output_buffers is zero, this may be NULL. In that case, at least one
     * more process_capture_result call must be made by the HAL to provide the
     * output buffers.
     *
     * When process_capture_result is called with a new buffer for a frame,
     * all previous frames' buffers for that corresponding stream must have been
     * already delivered (the fences need not have yet been signaled).
     *
     * >= CAMERA_DEVICE_API_VERSION_3_2:
     *
     * Gralloc buffers for a frame may be sent to framework before the
     * corresponding SHUTTER-notify.
     *
     * Performance considerations:
     *
     * Buffers delivered to the framework will not be dispatched to the
     * application layer until a start of exposure timestamp has been received
     * via a SHUTTER notify() call. It is highly recommended to
     * dispatch that call as early as possible.
     */
     const aml_camera_stream_buffer_t *output_buffers;

     /**
      * >= CAMERA_DEVICE_API_VERSION_3_2:
      *
      * The handle for the input stream buffer for this capture. It may not
      * yet be consumed at the time the HAL calls process_capture_result(); the
      * framework will wait on the release sync fences provided by the HAL before
      * reusing the buffer.
      *
      * The HAL should handle the sync fences the same way they are done for
      * output_buffers.
      *
      * Only one input buffer is allowed to be sent per request. Similarly to
      * output buffers, the ordering of returned input buffers must be
      * maintained by the HAL.
      *
      * Performance considerations:
      *
      * The input buffer should be returned as early as possible. If the HAL
      * supports sync fences, it can call process_capture_result to hand it back
      * with sync fences being set appropriately. If the sync fences are not
      * supported, the buffer can only be returned when it is consumed, which
      * may take long time; the HAL may choose to copy this input buffer to make
      * the buffer return sooner.
      */
      const aml_camera_stream_buffer_t *input_buffer;

     /**
      * >= CAMERA_DEVICE_API_VERSION_3_2:
      *
      * In order to take advantage of partial results, the HAL must set the
      * static metadata android.request.partialResultCount to the number of
      * partial results it will send for each frame.
      *
      * Each new capture result with a partial result must set
      * this field (partial_result) to a distinct inclusive value between
      * 1 and android.request.partialResultCount.
      *
      * HALs not wishing to take advantage of this feature must not
      * set an android.request.partialResultCount or partial_result to a value
      * other than 1.
      *
      * This value must be set to 0 when a capture result contains buffers only
      * and no metadata.
      */
     uint32_t partial_result;

     /**
      * >= CAMERA_DEVICE_API_VERSION_3_5:
      *
      * Specifies the number of physical camera metadata this capture result
      * contains. It must be equal to the number of physical cameras being
      * requested from.
      *
      * If the current camera device is not a logical multi-camera, or the
      * corresponding capture_request doesn't request on any physical camera,
      * this field must be 0.
      */
     uint32_t num_physcam_metadata;

     /**
      * >= CAMERA_DEVICE_API_VERSION_3_5:
      *
      * An array of strings containing the physical camera ids for the returned
      * physical camera metadata. The length of the array is
      * num_physcam_metadata.
      */
     const char **physcam_ids;

     /**
      * >= CAMERA_DEVICE_API_VERSION_3_5:
      *
      * The array of physical camera metadata for the physical cameras being
      * requested upon. This array should have a 1-to-1 mapping with the
      * physcam_ids. The length of the array is num_physcam_metadata.
      */
     const camera_metadata_t **physcam_metadata;

} aml_camera_capture_result_t;



typedef struct aml_camera_callback_ops {
    void (*process_capture_result)(const struct aml_camera_callback_ops *,
              const aml_camera_capture_result_t *result);

    void (*notify)(const struct aml_camera_callback_ops *,
              const aml_notify_message *msg);

    aml_camera_buffer_request_status_t (*request_stream_buffers)(
              const struct aml_camera_callback_ops *,
              uint32_t num_buffer_reqs,
              const aml_camera_buffer_request_t *buffer_reqs,
              /*out*/uint32_t *num_returned_buf_reqs,
              /*out*/aml_camera_stream_buffer_ret_t *returned_buf_reqs);

    void (*return_stream_buffers)(
              const struct aml_camera_callback_ops *,
              uint32_t num_buffers,
              const aml_camera_stream_buffer_t* const* buffers);

} aml_camera_callback_ops_t;

typedef struct aml_camera_device_ops {
    int (*initialize)(const struct aml_camera_device *,
              const aml_camera_callback_ops *callback_ops);

    int (*configure_streams)(const struct aml_camera_device *,
              aml_camera_stream_configuration_t *stream_list);


    int (*register_stream_buffers)(const struct aml_camera_device *,
              const aml_camera_stream_buffer_set_t *buffer_set);

    const camera_metadata_t* (*construct_default_request_settings)(
              const struct aml_camera_device *,
              int type);

    int (*process_capture_request)(const struct aml_camera_device *,
              aml_camera_capture_request_t *request);

    void (*get_metadata_vendor_tag_ops)(const struct aml_camera_device*,
              vendor_tag_query_ops_t* ops);

    void (*dump)(const struct aml_camera_device *, int fd);

    int (*flush)(const struct aml_camera_device *);



    void (*signal_stream_flush)(const struct aml_camera_device*,
              uint32_t num_streams,
              const aml_camera_stream_t* const* streams);

    /**
    * is_reconfiguration_required:
    *
    * <= CAMERA_DEVICE_API_VERSION_3_5:
    *
    *    Not defined and must be NULL
    *
    * >= CAMERA_DEVICE_API_VERSION_3_6:
    *
    * Check whether complete stream reconfiguration is required for possible new session
    * parameter values.
    *
    * This method must be called by the camera framework in case the client changes
    * the value of any advertised session parameters. Depending on the specific values
    * the HAL can decide whether a complete stream reconfiguration is required. In case
    * the HAL returns -ENVAL, the camera framework must skip the internal reconfiguration.
    * In case Hal returns 0, the framework must reconfigure the streams and pass the
    * new session parameter values accordingly.
    * This call may be done by the framework some time before the request with new parameters
    * is submitted to the HAL, and the request may be cancelled before it ever gets submitted.
    * Therefore, the HAL must not use this query as an indication to change its behavior in any
    * way.
    * ------------------------------------------------------------------------
    *
    * Preconditions:
    *
    * The framework can call this method at any time after active
    * session configuration. There must be no impact on the performance of
    * pending camera requests in any way. In particular there must not be
    * any glitches or delays during normal camera streaming.
    *
    * Performance requirements:
    * HW and SW camera settings must not be changed and there must not be
    * a user-visible impact on camera performance.
    *
    * @param oldSessionParams The currently applied session parameters.
    * @param newSessionParams The new session parameters set by client.
    *
    * @return Status Status code for the operation, one of:
    * 0:                    In case the stream reconfiguration is required
    *
    * -EINVAL:              In case the stream reconfiguration is not required.
    *
    * -ENOSYS:              In case the camera device does not support the
    *                       reconfiguration query.
    */

    int (*is_reconfiguration_required)(const struct aml_camera_device*,
              const camera_metadata_t* old_session_params,
              const camera_metadata_t* new_session_params);

    /* reserved for future use */
    void *reserved[6];
} aml_camera_device_ops_t;


typedef struct aml_camera_device {
    /**
     * common.version must equal CAMERA_DEVICE_API_VERSION_3_0 to identify this
     * device as implementing version 3.0 of the camera device HAL.
     *
     * Performance requirements:
     *
     * Camera open (common.module->common.methods->open) should return in 200ms, and must return
     * in 500ms.
     * Camera close (common.close) should return in 200ms, and must return in 500ms.
     *
     */
    hw_device_t common;
    aml_camera_device_ops_t *ops;
    void *priv;
} aml_camera_device_t;

}
#endif
