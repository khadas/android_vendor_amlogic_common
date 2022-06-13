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
 *  @author   Tellen Yu
 *  @version  2.0
 *  @date     2014/09/09
 *  @par function description:
 *  - 1 write property or sysfs in daemon
 */

#define LOG_TAG "SystemControl"
#define LOG_NDEBUG 0
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <sys/types.h>

#include "FrameRateMessage.h"
#include "FrameRateAutoAdaption.h"

MessageTask::MessageTask(FrameRateAutoAdaption* framerate) {
    mLooper = new Looper(true);
    mMsgHandler = new MsgHandler;
    mStop = false;
    mFrameRate = framerate;
    mMsgHandler->setFrameRateHandler(framerate);
}
void MessageTask::sendMessage(nsecs_t time) {
    if (mLooper != NULL) {
        mLooper->removeMessages(mMsgHandler);
        mLooper->sendMessageDelayed(time,mMsgHandler,Message(MsgHandler::kWhatCheck));
    }
}
void MessageTask::resetPlayFlag(nsecs_t time) {
    if (mLooper != NULL) {
        mLooper->removeMessages(mMsgHandler);
        mLooper->sendMessageDelayed(time,mMsgHandler,Message(MsgHandler::kWhatReset));
    }
}
void MessageTask::requestStop() {
    mStop = true;
}
void MessageTask::cancelTask() {
    if (mLooper != NULL)
        mLooper->removeMessages(mMsgHandler);
}
bool MessageTask::threadLoop() {
    int32_t ret = mLooper->pollOnce(-1);
    if (mStop) {
        return false;
    }
    return true;
}
MsgHandler::MsgHandler() {
}
void MsgHandler::setFrameRateHandler(FrameRateAutoAdaption* framerate) {
    mFrameRate = framerate;
}
void MsgHandler::handleMessage(const Message& message){
    ALOGD("really getLastFrame %d %d\n",mFrameRate->getLastFrame(),message.what);
    switch (message.what) {
        case MsgHandler::kWhatCheck:
        if (mFrameRate->getLastFrame() == 0) {
            mFrameRate->restoreEnv();
        }
        break;
        case MsgHandler::kWhatReset:
        if (mFrameRate->getLastFrame() > 0) {
            mFrameRate->setPlayFlag(false);
        }
        break;
    }


}
