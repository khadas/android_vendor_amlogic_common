/*
 * Copyright 2020, The Android Open Source Project
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

#define LOG_TAG "AudioSPDIF"
//#define LOG_NDEBUG 0

#include <string.h>
#include <assert.h>

#include <log/log.h>
#include <FrameScanner.h>

#include "MatFrameScanner.h"

namespace android {

// MAT Transport stream sync word
const uint8_t MatFrameScanner::kSyncBytes[] = { 0x07, 0x9E };

// Defined in IEC61937-9
#define SPDIF_DATA_TYPE_MAT 22

#define MAT_MAX_CHUNK_SIZE 30720

// MAT transport format
// { Syncword (16 bits) }
// { metadata_payload_length   (16 bits), Metadata Payload, Payload CRC (16 bits) }
// { top_of_channels_length    (16 bits), Top of Channels Audio Chunk, Payload CRC (16 bits) }
// { bottom_of_channels_length (16 bits), Bottom of Channels Audio Chunk, Payload CRC (16 bits) }
// Length does not include bytes for length itself and the CRC after payload.

// For frame parsing, we use three frame chunks for a single IEC61937 frame
// 1. {syncword, metadata_payload_length, Metadata Payload}
// 2. {metadata_payload_crc, top_of_channels_length, Top of Channels Audio Chunk}
// 3. {Top channels chunk crc, bottom_of_channels_length, Bottom of Channels Audio Chunk, Bottom Channels Chunk crc}
// such that the header buffer for the three "virtual" frames have fixed 4 bytes size (mHeaderLength = 4)
// and we can control the frame size to collect in each chunk state.

#define CHUNK_TYPE_METADATA 0
#define CHUNK_TYPE_TOP      1
#define CHUNK_TYPE_BOTTOM   2

MatFrameScanner::MatFrameScanner()
 : FrameScanner(SPDIF_DATA_TYPE_MAT,
        MatFrameScanner::kSyncBytes,
        sizeof(MatFrameScanner::kSyncBytes), 4)
 , mChunkType(CHUNK_TYPE_METADATA)
 , mLastChunk(false)
{
}

MatFrameScanner::~MatFrameScanner()
{
}

void MatFrameScanner::resetBurst()
{
    mChunkType = CHUNK_TYPE_METADATA;
}

// Per IEC 61973-9:5.3.1, for MAT burst-length shall be in bytes.
uint16_t MatFrameScanner::convertBytesToLengthCode(uint16_t numBytes) const
{
    return numBytes;
}

bool MatFrameScanner::isLastInBurst()
{
    return mLastChunk;
}

bool MatFrameScanner::parseHeader()
{
    size_t payload_length = ((size_t)(mHeaderBuffer[2]) << 8) | mHeaderBuffer[3];

    if ((payload_length <= 0) || (payload_length > MAT_MAX_CHUNK_SIZE))
        return false;

    payload_length <<= 1;   // convert to bytes

    if (mChunkType == CHUNK_TYPE_METADATA) {
        mFrameSizeBytes = mHeaderLength;    // sync word, metadata length
        mFrameSizeBytes += payload_length;
        mChunkType = CHUNK_TYPE_TOP;
        mLastChunk = false;
    } else if (mChunkType == CHUNK_TYPE_TOP) {
        mFrameSizeBytes = mHeaderLength;    // metadata crc, top length
        mFrameSizeBytes += payload_length;
        mChunkType = CHUNK_TYPE_BOTTOM;
        mLastChunk = false;
    } else {
        mFrameSizeBytes = mHeaderLength;    // top crc, bottom length
        mFrameSizeBytes += payload_length;
        mFrameSizeBytes += 2;               // bottom crc
        mChunkType = CHUNK_TYPE_METADATA;
        mLastChunk = true;
    }

    return true;
}

// State machine that scans for headers in a byte stream.
// @return true if we have detected a complete and valid header.
bool MatFrameScanner::scan(uint8_t byte)
{
    bool result = false;

    //ALOGV("MatFrameScanner: byte = 0x%02X, mCursor = %d", byte, mCursor);
    assert(mCursor < sizeof(mHeaderBuffer));

    if ((mChunkType == CHUNK_TYPE_METADATA) && (mCursor < mSyncLength)) {
        // match sync word
        if (byte == mSyncBytes[mCursor]) {
            mHeaderBuffer[mCursor++] = byte;
        } else {
            mBytesSkipped += 1; // skip unsynchronized data
            mCursor = 0;
        }
    } else if (mCursor < mHeaderLength) {
        // gather header for parsing metadata payload length
        mHeaderBuffer[mCursor++] = byte;
        if (mCursor >= mHeaderLength) {
            if (parseHeader()) {
                result = true;
            } else {
                ALOGE("MatFrameScanner: ERROR - parseHeader() failed.");
            }
            mCursor = 0;
        }
    }

    return result;
}

}  // namespace android
