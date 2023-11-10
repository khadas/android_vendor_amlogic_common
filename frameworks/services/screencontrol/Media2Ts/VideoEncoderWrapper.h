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

#ifndef AMLOGIC_SCREENCONTROL_VIDEOENCODERWRAPPER_H
#define AMLOGIC_SCREENCONTROL_VIDEOENCODERWRAPPER_H

#include <vector>
#include <thread>
#include <list>
#include <mutex>

#include <media/NdkMediaCodec.h>
#include <media/NdkMediaFormat.h>
#include <media/NdkMediaError.h>


namespace android {

struct InputData {
    InputData(void* data, int32_t size, int64_t pts): data_(data),
                    size_(size),pts_(pts),encoder_index_(-1){};
    InputData(InputData&&) = default;
    ~InputData() = default;
    void *data_;
    int32_t size_;
    int64_t pts_;
    size_t encoder_index_;
};

class VideoEncoderWrapper {
public:
    class VideoEncoderWrapperCallback {
    public:
        VideoEncoderWrapperCallback() = default;
        virtual ~VideoEncoderWrapperCallback() = default;
        virtual void onInputBufferAvailable(int64_t pts) = 0;
        virtual void onOutputBufferAvailable(void* const buffer, int32_t size, int32_t frame_type, int64_t pts) = 0;
    };
    VideoEncoderWrapper(VideoEncoderWrapperCallback * client);
    virtual ~VideoEncoderWrapper();
    bool init(int32_t width, int32_t height, int32_t bit_rate, int32_t frame_rate, int32_t i_frame_interval = 0);
    bool encodec(void* data,const int32_t size,const int64_t pts);
    bool stop();
    bool isSoftwareEncoder() { return mIsSoftwareEncoder;}
private:
    void onDequeueInputWork();
    void onDequeueOutputWork();
    void threadVideoFunc();
    bool EnqueueInput(std::unique_ptr<InputData>& input);
    int32_t get_frame_type(void* buffer, int32_t size);

    bool mIsSoftwareEncoder;
    AMediaCodec *mEncoder;
    std::vector<std::thread> ts;
    std::mutex mLock;
    bool mStart;
    std::list<size_t> mInputBufferIds;
    std::list<std::unique_ptr<InputData>> mPendingInputdQueue;
    std::list<std::unique_ptr<InputData>> mWorkingInputQueue;
    int32_t mWorkingFrameNum;
    int32_t mCSDbufferSize;
    VideoEncoderWrapperCallback* mVideoEncoderWrapperCallback;
    std::condition_variable mCondition;

};

};// namespace android

#endif // AMLOGIC_SCREENCONTROL_VIDEOENCODERWRAPPER_H