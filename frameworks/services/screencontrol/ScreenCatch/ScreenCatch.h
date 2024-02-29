/*
**
** Copyright 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef ANDROID_SCREENCONTROL_SCREENCATCH_H
#define ANDROID_SCREENCONTROL_SCREENCATCH_H
#include "../ScreenManager.h"


namespace android {

struct OutputInfo {
    OutputInfo(uint8_t* a,int32_t i): raw(a),index(i){};
    OutputInfo(OutputInfo&&) = default;
    ~OutputInfo() = default;
    uint8_t* raw;
    int32_t index;
};

class ScreenCatch : public ScreenManager::ScreenMangerCallback {
public:
    ScreenCatch();
    virtual ~ScreenCatch();
    bool start(std::unique_ptr<InputParmeter>& input);
    bool stop();
    void setVideoRotation(int degree);
    void PictureReady(const OutputRecord &output);
    void EventNotify(int32_t event);
    bool readBuffer(uint8_t* buffer, int32_t* size);
    int32_t getErrorEvent();

private:
    bool captureforKeystone();
    std::mutex mLock;
    ScreenManager* mScreenManager;
    bool mStart;
    std::list<std::unique_ptr<OutputInfo>> mOutputQueue;
    int32_t mRawBufferSize;
    int32_t mClientId;
    int32_t mErrorEvent;
};

};

#endif // ANDROID_SCREENCONTROL_SCREENCATCH_H