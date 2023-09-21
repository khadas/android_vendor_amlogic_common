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

#ifndef AMLOGIC_SCREENCONTROL_ESCONVERTOR_H
#define AMLOGIC_SCREENCONTROL_ESCONVERTOR_H

#include "../ScreenManager.h"
#include "VideoEncoderWrapper.h"


namespace android {

struct BufferPtsInfo {
    int32_t index;
    int64_t pts;
};

struct ESConvertorParmeter : public InputParmeter {
    ESConvertorParmeter(): bit_rate_(0),i_frame_interval(0){};
    ESConvertorParmeter(ESConvertorParmeter&&) = default;
    ~ESConvertorParmeter() = default;
    int32_t bit_rate_;
    int32_t i_frame_interval;
};

class ESConvertor : public ScreenManager::ScreenMangerCallback,
                    public VideoEncoderWrapper::VideoEncoderWrapperCallback {

public:
    class ESConvertorCallback {
    public:
        ESConvertorCallback() = default;
        virtual ~ESConvertorCallback() = default;
        virtual void onEsBufferAvailable(void* const data, int32_t size, int32_t frame_type, int64_t pts) = 0;
    };
    ESConvertor();
    virtual ~ESConvertor();
    bool start(std::unique_ptr<ESConvertorParmeter>& input, ESConvertorCallback *client);
    bool stop();
    void PictureReady(const OutputRecord &output);
    void onInputBufferAvailable(int64_t pts);
    void onOutputBufferAvailable(void* const buffer, int32_t size, int32_t frame_type, int64_t pts);

private:
    ESConvertorCallback* mESConvertorCallback;
    ScreenManager* mScreenManager;
    std::mutex mLock;
    bool mStart;
    int32_t mClientId;
    std::unique_ptr<ESConvertorParmeter> mInput;
    std::unique_ptr<VideoEncoderWrapper> mEncoder;
    std::unique_ptr<DataDumper> mDumper;
    std::list<std::unique_ptr<BufferPtsInfo>> mWorkingInfoQueue;




};


}; // namespace android

#endif // AMLOGIC_SCREENCONTROL_ESCONVERTOR_H