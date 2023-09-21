/*
 * Copyright (C) 2011 The Android Open Source Project
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
#define LOG_NDEBUG 0
#define LOG_TAG "VideoEncoderWrapper"
#include <utils/Log.h>

#include <OMX_Video.h>
#include <media/stagefright/MediaCodecConstants.h>
#include "media/stagefright/foundation/avc_utils.h"
#include "ScreenControlH264.h"
#include "VideoEncoderWrapper.h"
#include "ScreenControlDebug.h"

#include <ALooper.h>
#include <binder/ProcessState.h>
#include <binder/IPCThreadState.h>


namespace android {

VideoEncoderWrapper::VideoEncoderWrapper(VideoEncoderWrapperCallback * client):
        mIsSoftwareEncoder(false),
        mEncoder(nullptr),
        mStart(false),
        mWorkingFrameNum(0),
        mCSDbuffer(nullptr),
        mVideoEncoderWrapperCallback(client) {
        int fd1 = open("/dev/amvenc_avc", O_RDWR);
        int fd2 = open("/dev/amvenc_multi", O_RDWR);
        int fd3 = open("/dev/vc8000", O_RDWR);
        if (fd1 < 0 && fd2 < 0 && fd3 < 0) {
            mIsSoftwareEncoder = true;
            ALOGW("%s Open /dev/amvenc_avc failed, use software encoder instead!", __FUNCTION__);
        }
        fd1 >= 0?close(fd1):fd2 >= 0?close(fd2):fd3 >= 0?close(fd3):1;
        if (fd1 >= 0 ) {
            close(fd1);
        }
        if (fd2 >= 0 ) {
            close(fd2);
        }
        if (fd3 >= 0 ) {
            close(fd3);
        }
        ALOGI("VideoEncoderWrapper");
}



VideoEncoderWrapper::~VideoEncoderWrapper() {
    ALOGI("~VideoEncoderWrapper");
    if (mCSDbuffer) {
        delete []mCSDbuffer;
        mCSDbuffer = nullptr;
    }


}

bool VideoEncoderWrapper::init(int32_t width, int32_t height, int32_t bit_rate, int32_t frame_rate, int32_t i_frame_interval/*default as 0*/) {
    std::lock_guard<std::mutex> lock(mLock);
    media_status_t err = AMEDIA_OK;
     ALOGI("[%s %d] width:%d,height=%d,bit_rate=%d,frame_rate=%d,i_frame_interval=%d", __FUNCTION__, __LINE__,
                    width,height,bit_rate,frame_rate,i_frame_interval);

    const char *outputMIME = NULL;
    mEncoder = AMediaCodec_createEncoderByType("video/avc");
    if (mEncoder == NULL) {
        ALOGE("[%s %d] create fail !!", __FUNCTION__, __LINE__);
        return false;
    }
    mOutputFormat = AMediaFormat_new();
    AMediaFormat_setInt32(mOutputFormat, AMEDIAFORMAT_KEY_WIDTH, width);
    AMediaFormat_setInt32(mOutputFormat, AMEDIAFORMAT_KEY_HEIGHT, height);
    AMediaFormat_setString(mOutputFormat, AMEDIAFORMAT_KEY_MIME, "video/avc");

    AMediaFormat_setInt32(mOutputFormat, AMEDIAFORMAT_KEY_BIT_RATE, bit_rate);
    AMediaFormat_setInt32(mOutputFormat, AMEDIAFORMAT_KEY_BITRATE_MODE, OMX_Video_ControlRateConstant);
    AMediaFormat_setInt32(mOutputFormat, AMEDIAFORMAT_KEY_FRAME_RATE, frame_rate);

    if (mIsSoftwareEncoder) {
        AMediaFormat_setInt32(mOutputFormat, AMEDIAFORMAT_KEY_I_FRAME_INTERVAL, 5);
        AMediaFormat_setInt32(mOutputFormat, AMEDIAFORMAT_KEY_COLOR_FORMAT, COLOR_FormatYUV420SemiPlanar);
        AMediaFormat_setInt32(mOutputFormat, "store-metadata-in-buffers", false);
        AMediaFormat_setInt32(mOutputFormat, "prepend-sps-pps-to-idr-frames", 0);
    } else {
        AMediaFormat_setInt32(mOutputFormat, AMEDIAFORMAT_KEY_I_FRAME_INTERVAL, 15);  // Iframes every 15 secs
        AMediaFormat_setInt32(mOutputFormat, AMEDIAFORMAT_KEY_COLOR_FORMAT, OMX_COLOR_FormatAndroidOpaque);
        AMediaFormat_setInt32(mOutputFormat, "store-metadata-in-buffers", true);
        AMediaFormat_setInt32(mOutputFormat, "prepend-sps-pps-to-idr-frames", 1);
        AMediaFormat_setInt32(mOutputFormat, "vendor.venc.canvasmode.value", 1);
    }
    err = AMediaCodec_configure(mEncoder,
              mOutputFormat,
              nullptr,
              nullptr,
              AMEDIACODEC_CONFIGURE_FLAG_ENCODE);
    if (err == AMEDIA_OK) {
        // Encoder supported prepending SPS/PPS, we don't need to emulate
        // it.
    } else {
        ALOGE("We going to manually prepend SPS and PPS to IDR frames.");
    }

    err = AMediaCodec_start(mEncoder);
    if (err != AMEDIA_OK) {
        ALOGE("[%s %d] err:%d\n", __FUNCTION__, __LINE__, err);
        return false;
    }
    ALOGE("[%s %d]\n", __FUNCTION__, __LINE__);

    mStart = true;
    ts.push_back(std::thread(&VideoEncoderWrapper::threadVideoFunc,this));
    return true;
}

bool VideoEncoderWrapper::encodec(void* data,const int32_t size,const int64_t pts) {
    if (!data || size <= 0 || pts < 0) {
        ALOGE("[%s %d] the input data is abnormal", __FUNCTION__, __LINE__);
        return false;
    }
    ALOGI("[%s %d] pts=%ld", __FUNCTION__, __LINE__,pts);
    auto input = std::make_unique<InputData>(data,size,pts);
    mPendingInputdQueue.push_back(std::move(input));
    return false;
}

bool VideoEncoderWrapper::EnqueueInput(std::unique_ptr<InputData>& input) {
    if (!input || !input->data_ || input->size_ <= 0) {
        ALOGE("[%s %d] the input data is abnormal", __FUNCTION__, __LINE__);
        return false;
    }
    size_t bufSize = 0;
    size_t index = *mInputBufferIds.begin();
    mInputBufferIds.pop_front();
    uint8_t* buffer = AMediaCodec_getInputBuffer(mEncoder, index, &bufSize);
    if (bufSize < input->size_) {
        ALOGE("[%s %d] the input buffer from mediacodec is abnormal index:%d", __FUNCTION__, __LINE__,index);
        return false;
    }
    if (buffer) {
        memcpy(buffer,input->data_,input->size_);
        if (mIsSoftwareEncoder && mVideoEncoderWrapperCallback) {
            mVideoEncoderWrapperCallback->onInputBufferAvailable(input->pts_);
        }
    }
    media_status_t err = AMediaCodec_queueInputBuffer(mEncoder, index, 0,
                (buffer) ?input->size_ : 0 , input->pts_, 0);
    if (err != AMEDIA_OK) {
        ALOGE("[%s %d] queueInputBuffer fail\n", __FUNCTION__, __LINE__);
        return false;
    }
    VDLog("[%s %d] queue input buffer to encoder index =%d,pts = %ld", __FUNCTION__, __LINE__,index,input->pts_);
    if (!mIsSoftwareEncoder) {
        input->encoder_index_ = index;
        mWorkingInputQueue.push_back(std::move(input));
    }
    mWorkingFrameNum ++;
    return true;
}
bool VideoEncoderWrapper::stop() {
    std::unique_lock<std::mutex> lock(mLock);
    if (mCSDbuffer) {
        delete []mCSDbuffer;
        mCSDbuffer = nullptr;
    }
    if (!mStart) {
        ALOGE("[%s %d] the ScreenCatch has been started !", __FUNCTION__, __LINE__);
        return false;
    }
    mStart = false;
    VDLog("[%s %d] wait ", __FUNCTION__, __LINE__);
    mCondition.wait(lock);
    VDLog("[%s %d] join in ", __FUNCTION__, __LINE__);
    for (int i = 0; i < ts.size(); i++) {
        ts[i].join();

    }
    VDLog("[%s %d] thread join out ", __FUNCTION__, __LINE__);
    ts.clear();
    mWorkingFrameNum = 0;
    mInputBufferIds.clear();
    mPendingInputdQueue.clear();
    mWorkingInputQueue.clear();
    AMediaFormat_delete(mOutputFormat);
    AMediaCodec_stop(mEncoder);
    mVideoEncoderWrapperCallback = nullptr;
    ALOGI("[%s %d] stop done", __FUNCTION__, __LINE__);
    return true;
}
void VideoEncoderWrapper::threadVideoFunc() {
    while (1) {
        std::lock_guard<std::mutex> lock(mLock);
        if (!mStart) {
            mCondition.notify_one();
            break;
        }
        onDequeueInputWork();
        if (!mPendingInputdQueue.empty() && !mInputBufferIds.empty()) {
            auto input = mPendingInputdQueue.begin();
            if (EnqueueInput(*input)) {
                mPendingInputdQueue.pop_front();
            }

        }
        if (mWorkingFrameNum > 0)
            onDequeueOutputWork();
        usleep(5*1000);//5ms
    }
    ALOGI("[%s %d]  video thread out\n", __FUNCTION__, __LINE__);

}
void VideoEncoderWrapper::onDequeueInputWork() {
    int index = AMediaCodec_dequeueInputBuffer(mEncoder, 0ll);
    if (index <= AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
        // ALOGE("[%s %d] don't get the usable input buffer", __FUNCTION__, __LINE__);
        return;
    }
    VDLog("[%s %d] dequeue input buffer from encoder index =%d", __FUNCTION__, __LINE__,index);
    if (!mIsSoftwareEncoder && !mWorkingInputQueue.empty()) {
        auto input_info = std::find_if(mWorkingInputQueue.begin(), mWorkingInputQueue.end(),
                [=](std::unique_ptr<InputData>& info) {
                    return info->encoder_index_ == index;
                });
        if (input_info == mWorkingInputQueue.end()) {
            ALOGE("don't find the buffer in working input queue: index %d", index);
        }else {
            if (mVideoEncoderWrapperCallback) {
                mVideoEncoderWrapperCallback->onInputBufferAvailable((*input_info)->pts_);
            }
            mWorkingInputQueue.erase(input_info);
        }

    }
    mInputBufferIds.push_back(index);
    if (!mPendingInputdQueue.empty()) {
        auto input = mPendingInputdQueue.begin();
        VDLog("[%s %d] EnqueueInput", __FUNCTION__, __LINE__);
        if (EnqueueInput(*input)) {
            mPendingInputdQueue.pop_front();
        }
    }
    return;
}


void VideoEncoderWrapper::onDequeueOutputWork() {
    AMediaCodecBufferInfo outInfo;

    uint8_t* es_buffer = nullptr;
    int32_t  es_size = 0;
    size_t bufSize = 0;
    outInfo.flags = AMEDIACODEC_INFO_TRY_AGAIN_LATER;
    outInfo.size = 0;
    outInfo.presentationTimeUs = 0;
    size_t index = AMediaCodec_dequeueOutputBuffer(mEncoder, &outInfo,0ll);
    if (index <= 0) {
        // ALOGE("[%s %d] don't get the usable out buffer", __FUNCTION__, __LINE__);
        return;
    }
    if (outInfo.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM) {
        // ALOGE("[%s %d] get the EOS buffer", __FUNCTION__, __LINE__);
        return;
    }
    VDLog("[%s %d] dequeue output buffer from encoder index = %d,pts = %ld", __FUNCTION__, __LINE__,index,outInfo.presentationTimeUs);
    uint8_t* output = AMediaCodec_getOutputBuffer(mEncoder, index, &bufSize);
    if (output && outInfo.size > 0) {
        bool isIDR = false;
        es_size = outInfo.size;
        ALOGI("[%s %d] get output buffer to encoder index = %d,size =%d,pts = %ld", __FUNCTION__, __LINE__,index,outInfo.size,outInfo.presentationTimeUs);
        if (outInfo.flags & AMEDIACODEC_BUFFER_FLAG_CODEC_CONFIG) {
            if (mCSDbuffer)
                delete []mCSDbuffer;
            mCSDbuffer = new uint8_t[outInfo.size];
            memcpy(mCSDbuffer, output, outInfo.size);
            es_buffer = mCSDbuffer;
        }else {
            if (IsIDR(output, outInfo.size) && mCSDbuffer) {
                VDLog("[%s %d] the csd buffer len = %d ", __FUNCTION__, __LINE__,sizeof(mCSDbuffer));
                es_buffer = new uint8_t[(outInfo.size + sizeof(mCSDbuffer))];
                memcpy(es_buffer,mCSDbuffer,sizeof(mCSDbuffer));
                memcpy(es_buffer + sizeof(mCSDbuffer),output,outInfo.size);
                es_size = outInfo.size + sizeof(mCSDbuffer);
                isIDR = true;
            }
            es_buffer = output;
        }
        VDLog("[%s %d] onOutputBufferAvailable es_size=%d,pts=%ld", __FUNCTION__, __LINE__,es_size,outInfo.presentationTimeUs);
        if (mVideoEncoderWrapperCallback)
            mVideoEncoderWrapperCallback->onOutputBufferAvailable(es_buffer, es_size, get_frame_type(es_buffer,es_size), outInfo.presentationTimeUs);
        if (isIDR)
            delete []es_buffer;
        es_buffer = nullptr;
        es_size = 0;
        mWorkingFrameNum --;
    }
    AMediaCodec_releaseOutputBuffer(mEncoder, index, false);
    return;

}
int32_t VideoEncoderWrapper::get_frame_type(void* buffer, int32_t size) {
    uint8_t *h264 =new uint8_t[size];
    int32_t frameType = AVC_TYPE_FRAME_TYPE_UNKNOWN;
    memcpy(h264, buffer, size);
    uint8_t naltype = (*(h264+4)) & 0x1F;
    switch (naltype) {
        case NAL_SLICE: {
            NALU_t nal[1];
            if (h264[0]== 0x00 && h264[1]== 0x00 && h264[2]== 0x00 && h264[3]== 0x01 ) {
                nal->startcodeprefix_len = 4;
                nal->buf = h264 + 4;
                nal->len = size - 4;
            }else if (h264[0]== 0x00 && h264[1]== 0x00 && h264[2]== 0x01) {
                nal->startcodeprefix_len = 3;
                nal->buf = h264 + 3;
                nal->len = size - 3;
            }else
                break;
            nal->nal_unit_type = naltype;
            int ret =GetFrameType(nal);
            if (ret < 1) {
                frameType = AVC_TYPE_FRAME_TYPE_UNKNOWN;
                ALOGE(" find frame type error !!");
                break;
            }
            switch (nal->Frametype) {
                case FRAME_I:
                    frameType = AVC_TYPE_FRAME_TYPE_FRAME_I;
                    break;
                case FRAME_P:
                    frameType = AVC_TYPE_FRAME_TYPE_FRAME_P;
                    break;
                case FRAME_B:
                    frameType = AVC_TYPE_FRAME_TYPE_FRAME_B;
                    break;
                default:
                    frameType = AVC_TYPE_FRAME_TYPE_UNKNOWN;
                    break;
            }
            break;
        }
        case NAL_SLICE_DPA :
            frameType = AVC_TYPE_FRAME_TYPE_SLICE_A;
            break;
        case NAL_SLICE_DPB:
            frameType = AVC_TYPE_FRAME_TYPE_SLICE_B;
            break;
        case NAL_SLICE_DPC:
            frameType = AVC_TYPE_FRAME_TYPE_SLICE_C;
            break;
        case NAL_SLICE_IDR:
            frameType = AVC_TYPE_FRAME_TYPE_IDR;
            break;
        case NAL_SEI:
            frameType = AVC_TYPE_FRAME_TYPE_SEI;
            break;
        case NAL_SPS:
            frameType = AVC_TYPE_FRAME_TYPE_SPS;
            break;
        case NAL_PPS:
            frameType = AVC_TYPE_FRAME_TYPE_PPS;
            break;
        case NAL_DELIMITER:
            frameType = AVC_TYPE_FRAME_TYPE_DELIMITER;
            break;
        case NAL_SEQUENCE_END:
            frameType = AVC_TYPE_FRAME_TYPE_SEQUENCE_END;
            break;
        case NAL_STEAM_END:
            frameType = AVC_TYPE_FRAME_TYPE_STEAM_END;
            break;
        default:
            frameType = AVC_TYPE_FRAME_TYPE_UNKNOWN;
            break;
    }
    delete [] h264;
    return frameType;
}



}; //namespace android