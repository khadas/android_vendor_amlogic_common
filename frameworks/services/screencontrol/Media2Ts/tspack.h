/*
* Copyright (C) 2011 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* 	 http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#ifndef AMLOGIC_SCREENCONTROL_TSPACK_H
#define AMLOGIC_SCREENCONTROL_TSPACK_H

#include "esconvertor.h"

namespace android {

enum {
    kPID_PMT = 0x100,
    kPID_PCR = 0x1000,
    kPID_VIDEO = 0x1100,
    kPID_AUDIO = 0x1110,
};
enum {
    EMIT_PAT_AND_PMT                = 1,
    EMIT_PCR                        = 2,
    IS_ENCRYPTED                    = 4,
    PREPEND_SPS_PPS_TO_IDR_FRAMES   = 8,
};
struct TSBufferInfo {
    TSBufferInfo(uint8_t* buffer, int32_t size, int64_t pts): mTsbuffer(buffer),mSize(size),mPts(pts){};
    TSBufferInfo(TSBufferInfo&&) = default;
    ~TSBufferInfo() = default;
    uint8_t* mTsbuffer;
    int32_t mSize;
    int64_t mPts;
};

class TSPacker : public ESConvertor::ESConvertorCallback {
public:
    TSPacker();
    virtual ~TSPacker();
    bool start(std::unique_ptr<ESConvertorParmeter>& input);
    bool stop();
    bool readBuffer(uint8_t** buffer, int32_t* size, int64_t* pts);
    void onEsBufferAvailable(void* const data, int32_t size, int32_t frame_type, int64_t pts);
private:
    bool packetize(const uint8_t *es_buffer, int32_t es_size, uint8_t** packets
                            ,int32_t * ts_size,int64_t timeUs, uint32_t flags);
    int32_t incrementContinuityCounter();
    bool mStart;
    bool mFirstVideoFrame;
    std::mutex mLock;
    std::unique_ptr<ESConvertor> mConvertor;
    int32_t mPATContinuityCounter;
    int32_t mPMTContinuityCounter;
    int32_t mVideoContinuityCounter;
    int64_t mPrevTimeUs;
    int32_t mCSDbufferSize;
    uint8_t* mCSDbuffer;
    uint8_t* mVideoDescriptor;
    uint8_t* mHdrDescriptor;
    std::list<std::unique_ptr<TSBufferInfo>> mOutputQueue;

};

};//namespace android
#endif // AMLOGIC_SCREENCONTROL_TSPACK_H