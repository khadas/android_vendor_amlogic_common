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
 *  - 1 write property or sysfs in daemon
 */
#ifndef FRAMERATE_MESSAGE_H
#define FRAMERATE_MESSAGE_H
#include <utils/Looper.h>
#include <utils/Thread.h>
#include <utils/Timers.h>
#define VDIN_RESTORE_DELAY_DURATION             8
using namespace android;

class FrameRateAutoAdaption;

class MsgHandler: public MessageHandler
{
public:
    MsgHandler();
    void setFrameRateHandler(FrameRateAutoAdaption* framerate);
    enum {
        kWhatCheck,
        kWhatReset,
    };
    virtual void handleMessage(const Message& message);
private:
    friend class FrameRateAutoAdaption;

    virtual ~MsgHandler(){};
    FrameRateAutoAdaption* mFrameRate;
};

class MessageTask : public Thread {
    sp<Looper> mLooper;
    sp<MsgHandler> mMsgHandler;
    bool mStop;
    FrameRateAutoAdaption* mFrameRate;
public:
    explicit MessageTask(FrameRateAutoAdaption *f);
    void sendMessage(nsecs_t time);
    void resetPlayFlag(nsecs_t time);
    void cancelTask();
    void requestStop();
protected:
    virtual ~MessageTask(){};

    virtual bool threadLoop();
};
#endif