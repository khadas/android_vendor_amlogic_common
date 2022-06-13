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

#ifndef ANDROID_AUDIO_MAT_FRAME_SCANNER_H
#define ANDROID_AUDIO_MAT_FRAME_SCANNER_H

#include <stdint.h>
#include <system/audio.h>
#include <FrameScanner.h>

namespace android {

#define MAT_PCM_FRAMES                          1536
#define MAT_RATE_MULTIPLIER                       10

class MatFrameScanner : public FrameScanner
{
public:
    MatFrameScanner();
    virtual ~MatFrameScanner();

    virtual int getMaxChannels()   const { return 5 + 1 + 2; } // 5.1.2 surround

    virtual int getMaxSampleFramesPerSyncFrame() const { return MAT_RATE_MULTIPLIER
            * MAT_PCM_FRAMES; }
    virtual int getSampleFramesPerSyncFrame() const { return MAT_RATE_MULTIPLIER
            * MAT_PCM_FRAMES; }

    virtual bool isFirstInBurst() { return false; }
    virtual bool isLastInBurst();
    virtual void resetBurst();

    virtual uint16_t convertBytesToLengthCode(uint16_t numBytes) const;
    virtual bool scan(uint8_t byte);
    virtual bool isFrameContainSixBlockCounts() { return false; }
    virtual bool isFrameContainDependentFrames(){ return false; }

protected:
    // used to recognize the start of a MAT sync frame
    static const uint8_t kSyncBytes[];
    int mChunkType;
    bool mLastChunk;

    virtual bool parseHeader();
};

}  // namespace android

#endif  // ANDROID_AUDIO_MAT_FRAME_SCANNER_H
