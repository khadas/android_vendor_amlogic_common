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
#define LOG_TAG "TSPacker"
#include <utils/Log.h>
#include "tspack.h"

namespace android {


TSPacker::TSPacker() :
        mStart(false),
        mFirstVideoFrame(false),
        mPATContinuityCounter(0),
        mPMTContinuityCounter(0),
        mVideoContinuityCounter(0),
        mPrevTimeUs(-1){
    ALOGI("TSPacker construct\n");
    mVideoDescriptor = new uint8_t[6];
    mVideoDescriptor[0] = 40;  // descriptor_tag
    mVideoDescriptor[1] = 4;  // descriptor_length
    mVideoDescriptor[2] = 0x67;    // profile_idc
    mVideoDescriptor[3] = 0x42; // constraint_set*
    mVideoDescriptor[4] = 0x00;      // level_idc
    // AVC_still_present=0, AVC_24_hour_picture_flag=0, reserved
    mVideoDescriptor[5] = 0x3f;

    mHdrDescriptor = new uint8_t[4];
    mHdrDescriptor[0] = 42;  // descriptor_tag
    mHdrDescriptor[1] = 2;  // descriptor_length
    mHdrDescriptor[2] = 0x7e;
    mHdrDescriptor[3] = 0x1f;
    mOutputQueue.clear();


}
TSPacker::~TSPacker() {

    ALOGI("~TSPacker");
    if (mVideoDescriptor)
        delete []mVideoDescriptor;
    if (mHdrDescriptor)
        delete []mHdrDescriptor;
}

bool TSPacker::start(std::unique_ptr<ESConvertorParmeter>& input) {
    std::lock_guard<std::mutex> lock(mLock);
    mConvertor = std::make_unique<ESConvertor>();
    if (!mConvertor->start(input,this)) {
        ALOGE("[%s %d] ESConvertor start fail", __FUNCTION__, __LINE__);
        return false;
    }
    mFirstVideoFrame = true;
    mStart = true;
    mOutputQueue.clear();
    return true;
}

bool TSPacker::stop() {
    std::lock_guard<std::mutex> lock(mLock);
    if (!mStart) {
        ALOGE("[%s %d] the tspacker has been not started", __FUNCTION__, __LINE__);
        return false;
    }
    if (!mConvertor || !mConvertor->stop()) {
        ALOGE("[%s %d] ESConvertor stop fail", __FUNCTION__, __LINE__);
        return false;
    }
    while (!mOutputQueue.empty()) {
        auto output = mOutputQueue.begin();
        uint8_t* buffer = (*output)->mTsbuffer;
        mOutputQueue.erase(output);
        delete []buffer;
    }
    mStart = false;
    mFirstVideoFrame = false;
    mPATContinuityCounter = 0;
    mPMTContinuityCounter = 0;
    mVideoContinuityCounter = 0;
    mPrevTimeUs = -1;
    return true;
}

bool TSPacker::readBuffer(uint8_t** buffer, int32_t* size, int64_t* pts) {
    std::lock_guard<std::mutex> lock(mLock);
    if (!mStart || mOutputQueue.empty()) {
        return false;
    }
    auto output = mOutputQueue.begin();
    if (!(*output)->mTsbuffer || (*output)->mSize <=0 || (*output)->mPts <= 0) {
        ALOGE("[%s %d] the info of output is not legal ! ", __FUNCTION__, __LINE__);
        return false;
    }
    *buffer = (*output)->mTsbuffer;
    *size = (*output)->mSize;
    *pts = (*output)->mPts;
    mOutputQueue.erase(output);
    return true;
}

bool TSPacker::packetize(const uint8_t *es_buffer, int32_t es_size, uint8_t** packets
                            ,int32_t * ts_size,int64_t timeUs, uint32_t flags) {
    int32_t stream_pid = kPID_VIDEO;
    int32_t stream_id = 0xe0;
    int32_t PES_packet_length = es_size + 8;
    int32_t numTSPackets = 1;

    {
        // Make sure the PES header fits into a single TS packet:
        int32_t sizeAvailableForPayload = 170;
        int32_t numBytesOfPayload = es_size;
        if (numBytesOfPayload > sizeAvailableForPayload) {
            numBytesOfPayload = sizeAvailableForPayload;
        }
        int32_t numBytesOfPayloadRemaining = es_size - numBytesOfPayload;
        // This is how many bytes of payload each subsequent TS packet
        // can contain at most.
        sizeAvailableForPayload = 188 - 4;
        int32_t sizeAvailableForAlignedPayload = sizeAvailableForPayload;
        int32_t numFullTSPackets = numBytesOfPayloadRemaining / sizeAvailableForAlignedPayload;
        numTSPackets += numFullTSPackets;
        numBytesOfPayloadRemaining -= numFullTSPackets * sizeAvailableForAlignedPayload;
        // numBytesOfPayloadRemaining < sizeAvailableForAlignedPayload
        if (numFullTSPackets == 0 && numBytesOfPayloadRemaining > 0) {
            // There wasn't enough payload left to form a full aligned payload,
            // the last packet doesn't have to be aligned.
            ++numTSPackets;
        } else if (numFullTSPackets > 0 && (numBytesOfPayloadRemaining
            + sizeAvailableForAlignedPayload) > sizeAvailableForPayload) {
            // The last packet emitted had a full aligned payload and together
            // with the bytes remaining does exceed the unaligned payload
            // size, so we need another packet.
            ++numTSPackets;
        }
    }
    if (flags & EMIT_PAT_AND_PMT) {
        numTSPackets += 2;
    }
    if (flags & EMIT_PCR) {
        ++numTSPackets;
    }
    uint8_t* buffer = new uint8_t[numTSPackets * 188];
    *ts_size = numTSPackets * 188;
    uint8_t *packetDataStart = buffer;
    if (flags & EMIT_PAT_AND_PMT) {
        if (++mPATContinuityCounter == 16) {
            mPATContinuityCounter = 0;
        }
        uint8_t *ptr = packetDataStart;
        *ptr++ = 0x47;
        *ptr++ = 0x40;
        *ptr++ = 0x00;
        *ptr++ = 0x10 | mPATContinuityCounter;
        *ptr++ = 0x00;

        uint8_t *crcDataStart = ptr;
        *ptr++ = 0x00;
        *ptr++ = 0xb0;
        *ptr++ = 0x0d;
        *ptr++ = 0x00;
        *ptr++ = 0x00;
        *ptr++ = 0xc3;
        *ptr++ = 0x00;
        *ptr++ = 0x00;
        *ptr++ = 0x00;
        *ptr++ = 0x01;
        *ptr++ = 0xe0 | (kPID_PMT >> 8);
        *ptr++ = kPID_PMT & 0xff;

        //crc
        *ptr++ = 0x2d;
        *ptr++ = 0xf6;
        *ptr++ = 0x52;
        *ptr++ = 0x95;

        int32_t sizeLeft = packetDataStart + 188 - ptr;
        memset(ptr, 0xff, sizeLeft);
        packetDataStart += 188;
        if (++mPMTContinuityCounter == 16) {
            mPMTContinuityCounter = 0;
        }

        ptr = packetDataStart;
        *ptr++ = 0x47;
        *ptr++ = 0x40 | (kPID_PMT >> 8);
        *ptr++ = kPID_PMT & 0xff;
        *ptr++ = 0x10 | mPMTContinuityCounter;
        *ptr++ = 0x00;

        crcDataStart = ptr;
        *ptr++ = 0x02;

        *ptr++ = 0x00;	// section_length to be filled in below.
        *ptr++ = 0x00;

        *ptr++ = 0x00;
        *ptr++ = 0x01;
        *ptr++ = 0xc3;
        *ptr++ = 0x00;
        *ptr++ = 0x00;
        *ptr++ = 0xe0 | (kPID_PCR >> 8);
        *ptr++ = kPID_PCR & 0xff;

        *ptr++ = 0xf0 | (0 >> 8);
        *ptr++ = (0 & 0xff);

        int32_t ES_info_length = 0;
        //***************video info******************//
        ES_info_length = 10;
        *ptr++ = 0x1b;//0x1b avc
        *ptr++ = 0xe0 | (kPID_VIDEO >> 8);
        *ptr++ = kPID_VIDEO & 0xff;

        *ptr++ = 0xf0 | (ES_info_length >> 8);
        *ptr++ = (ES_info_length & 0xff);
        {
            memcpy(ptr, mVideoDescriptor, 6);
            ptr += 6;
        }
        {
            memcpy(ptr, mHdrDescriptor, 4);
            ptr += 4;
        }
        size_t section_length = ptr - (crcDataStart + 3) + 4 /* CRC */;

        crcDataStart[1] = 0xb0 | (section_length >> 8);
        crcDataStart[2] = section_length & 0xff;

        *ptr++ = 0xb6;
        *ptr++ = 0xa8;
        *ptr++ = 0x13;
        *ptr++ = 0x1e;

        sizeLeft = packetDataStart + 188 - ptr;
        memset(ptr, 0xff, sizeLeft);
        packetDataStart += 188;
    }
    if (flags & EMIT_PCR) {
        int64_t timeNow64;
        struct timeval timeNow;
        gettimeofday(&timeNow, NULL);
        int64_t nowUs = (int64_t)timeNow.tv_sec*1000*1000 + (int64_t)timeNow.tv_usec;


        uint64_t PCR = nowUs * 27;	// PCR based on a 27MHz clock
        uint64_t PCR_base = PCR / 300;
        uint32_t PCR_ext = PCR % 300;

        uint8_t *ptr = packetDataStart;
        *ptr++ = 0x47;
        *ptr++ = 0x40 | (kPID_PCR >> 8);
        *ptr++ = kPID_PCR & 0xff;
        *ptr++ = 0x20;
        *ptr++ = 0xb7;	// adaptation_field_length
        *ptr++ = 0x10;
        *ptr++ = (PCR_base >> 25) & 0xff;
        *ptr++ = (PCR_base >> 17) & 0xff;
        *ptr++ = (PCR_base >> 9) & 0xff;
        *ptr++ = ((PCR_base & 1) << 7) | 0x7e | ((PCR_ext >> 8) & 1);
        *ptr++ = (PCR_ext & 0xff);

        size_t sizeLeft = packetDataStart + 188 - ptr;
        memset(ptr, 0xff, sizeLeft);

        packetDataStart += 188;

    }
    uint64_t PTS = (timeUs * 9ll) / 100ll;
    if (PES_packet_length >= 65536) {
        // This really should only happen for video.
        // It's valid to set this to 0 for video according to the specs.
        PES_packet_length = 0;
    }
    size_t sizeAvailableForPayload = 188 - 4 - 14;
    size_t copy = es_size;
    if (copy > sizeAvailableForPayload) {
        copy = sizeAvailableForPayload;

    }
    size_t numPaddingBytes = sizeAvailableForPayload - copy;

    uint8_t *ptr = packetDataStart;
    *ptr++ = 0x47;
    *ptr++ = 0x40 | (stream_pid >> 8);
    *ptr++ = stream_pid & 0xff;
    *ptr++ = (numPaddingBytes > 0 ? 0x30 : 0x10) | incrementContinuityCounter();
    if (numPaddingBytes > 0) {
        *ptr++ = numPaddingBytes - 1;
        if (numPaddingBytes >= 2) {
            *ptr++ = 0x00;
            memset(ptr, 0xff, numPaddingBytes - 2);
            ptr += numPaddingBytes - 2;
        }
    }
    *ptr++ = 0x00;
    *ptr++ = 0x00;
    *ptr++ = 0x01;
    *ptr++ = stream_id;
    *ptr++ = PES_packet_length >> 8;
    *ptr++ = PES_packet_length & 0xff;
    *ptr++ = 0x84;
    *ptr++ = 0x80;

    size_t headerLength = 0x05;
    *ptr++ = headerLength;
    *ptr++ = 0x20 | (((PTS >> 30) & 7) << 1) | 1;
    *ptr++ = (PTS >> 22) & 0xff;
    *ptr++ = (((PTS >> 15) & 0x7f) << 1) | 1;
    *ptr++ = (PTS >> 7) & 0xff;
    *ptr++ = ((PTS & 0x7f) << 1) | 1;
    memcpy(ptr, es_buffer, copy);
    ptr += copy;
    if (ptr != packetDataStart + 188) {
        ALOGE("check the ptr fail!");
        return false;
    }
    packetDataStart += 188;

    size_t offset = copy;

    while (offset < es_size) {

        size_t sizeAvailableForPayload = 188 - 4;

        size_t copy = es_size - offset;

        if (copy > sizeAvailableForPayload) {
            copy = sizeAvailableForPayload;
        }

        size_t numPaddingBytes = sizeAvailableForPayload - copy;

        uint8_t *ptr = packetDataStart;
        *ptr++ = 0x47;
        *ptr++ = 0x00 | (stream_pid >> 8);
        *ptr++ = stream_pid & 0xff;

        *ptr++ = (numPaddingBytes > 0 ? 0x30 : 0x10) | incrementContinuityCounter();

        if (numPaddingBytes > 0) {
            *ptr++ = numPaddingBytes - 1;
            if (numPaddingBytes >= 2) {
                *ptr++ = 0x00;
                memset(ptr, 0xff, numPaddingBytes - 2);
                ptr += numPaddingBytes - 2;
            }
        }

        memcpy(ptr, es_buffer + offset, copy);
        ptr += copy;
        offset += copy;
        packetDataStart += 188;
    }
    *packets = buffer;
    return true;
}

int32_t TSPacker::incrementContinuityCounter() {
    int32_t prevCounter = mVideoContinuityCounter;
    if (++mVideoContinuityCounter == 16) {
        mVideoContinuityCounter = 0;
    }
    return prevCounter;
}

void TSPacker::onEsBufferAvailable(void* const data, int32_t size, int32_t frame_type, int64_t pts) {
    if (!data || size <= 0 || pts < 0 || frame_type < 0)
        return;
    ALOGI("[%s %d] size=%d,frame_type=%d,pts=%ld", __FUNCTION__, __LINE__,size,frame_type,pts);

    uint8_t *buffer = nullptr;
    int32_t buffer_size = 0;
    struct timeval timeNow;
    gettimeofday(&timeNow, NULL);
    int64_t timeNow64 = (int64_t)timeNow.tv_sec*1000*1000 + (int64_t)timeNow.tv_usec;
    int32_t flags = 0;
    if (mPrevTimeUs < 0ll || mPrevTimeUs + 100000ll <= timeNow64 || mFirstVideoFrame) {
        flags |= EMIT_PCR;
        flags |= EMIT_PAT_AND_PMT;
        mPrevTimeUs = timeNow64;
        mFirstVideoFrame = false;
    }
    packetize((const uint8_t *)data,size,&buffer,&buffer_size,pts,flags);
    if (!buffer || buffer_size <= 0)
        return;
    std::unique_ptr<TSBufferInfo> output = std::make_unique<TSBufferInfo>(buffer,buffer_size,pts);
    mOutputQueue.push_back(std::move(output));
}


};//namespace android
