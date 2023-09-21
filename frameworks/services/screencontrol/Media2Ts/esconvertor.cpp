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
#define LOG_TAG "ESConvertor"
#include <utils/Log.h>
#include "../ScreenControlDebug.h"
#include "esconvertor.h"

namespace android {

ESConvertor::ESConvertor() :
            mClientId(-1),
            mStart(false),
            mScreenManager(nullptr) {
    ALOGI("ESConvertor construct");
    ScreenControlDebug::initDebug();
    if (ScreenControlDebug::isNeedDumpEs())
        mDumper = std::make_unique<DataDumper>("/data/temp/dump.es");

}

ESConvertor::~ESConvertor() {
    ALOGI("~ESConvertor");
}

bool ESConvertor::start(std::unique_ptr<ESConvertorParmeter>& input, ESConvertorCallback *client) {
    std::lock_guard<std::mutex> lock(mLock);
    if (input->source_type < AML_CAPTURE_VIDEO || input->source_type > SCAML_CAPTURE_UNKNOWN) {
        ALOGE("[%s %d] dont't support the type=%d", __FUNCTION__, __LINE__,input->source_type);
        return false;
    }
    if (mStart) {
        ALOGE("[%s %d] it has been started", __FUNCTION__, __LINE__);
        return false;
    }
    if (client)
        mESConvertorCallback = client;

    mEncoder = std::make_unique<VideoEncoderWrapper>(this);
    if (!mEncoder)
        return false;

    mScreenManager = ScreenManager::getInstance();
    if (!mScreenManager)
        return false;

    if (!mEncoder->init(input->size->width(), input->size->height(), input->bit_rate_, input->frame_rate,input->i_frame_interval)) {
        ALOGE("[%s %d] encoder init fail!", __FUNCTION__, __LINE__);
        return false;
    }
    auto screenInput = std::make_unique<InputParmeter>();
    screenInput->source_type = input->source_type;
    screenInput->format = mEncoder->isSoftwareEncoder()?SCREENCONTROL_PIX_FMT_NV12:SCREENCONTROL_PIX_FMT_NV21;
    screenInput->frame_rate = input->frame_rate;
    screenInput->size = std::move(input->size);
    screenInput->area = std::move(input->area);
    if (!mScreenManager->start(screenInput,this,&mClientId) || mClientId < 0) {
        ALOGE("[%s %d] ScreenManager init fail! mClientId= %d", __FUNCTION__, __LINE__,mClientId);
        return false;
    }

    mStart = true;
    ALOGI("[%s %d] start finish  mStart=%s", __FUNCTION__, __LINE__,mStart?"true":"false");
    return true;
}

bool ESConvertor::stop() {
    std::lock_guard<std::mutex> lock(mLock);
    if (!mStart) {
        ALOGE("[%s %d] the ESConvertor has been started !", __FUNCTION__, __LINE__);
        return false;
    }
    mStart = false;
    mScreenManager->stop(mClientId);
    if (!mEncoder->stop()) {
        ALOGE("[%s %d] the mEncoder stop fail!", __FUNCTION__, __LINE__);
        return false;
    }
    mWorkingInfoQueue.clear();
    mESConvertorCallback = nullptr;
     ALOGI("[%s %d] stop done", __FUNCTION__, __LINE__);
    return true;
}
void ESConvertor::PictureReady(const OutputRecord &output) {
    ALOGI("PictureReady index =%d",output.index);
    if (!output.raw_buffer || !output.canvas_buffer || output.raw_buffer_size <= 0 || !mStart) {
        ALOGE("[%s %d] the buffer is wrong or has been stoped mStart=%s", __FUNCTION__, __LINE__,mStart?"true":"false");
        return;
    }
    if (mEncoder->isSoftwareEncoder()) {
        mEncoder->encodec(output.raw_buffer, output.raw_buffer_size,output.tv_usec);
    }else {
        mEncoder->encodec(output.canvas_buffer, 3 * sizeof(long),output.tv_usec);
    }
    auto info = std::make_unique<BufferPtsInfo>();
    info->index = output.index;
    info->pts = output.tv_usec;
    mWorkingInfoQueue.push_back(std::move(info));

}
void ESConvertor::onInputBufferAvailable(int64_t pts) {
    if (pts <= 0 || !mStart)
        return;
    ALOGI("onInputBufferAvailable pts =%ld",pts);
    auto outinfo = std::find_if(mWorkingInfoQueue.begin(), mWorkingInfoQueue.end(),
                    [=](std::unique_ptr<BufferPtsInfo>& info) {
                        return info->pts == pts;
                    });
    if (outinfo == mWorkingInfoQueue.end()) {
        ALOGE("can't find this buffer pts: %lld", pts);
        return;
    }
    mScreenManager->realseBuffer(mClientId,(*outinfo)->index);
}
void ESConvertor::onOutputBufferAvailable(void* const buffer, int32_t size, int32_t frame_type, int64_t pts) {
    ALOGI("onOutputBufferAvailable frame_type=%d,pts =%ld,size=%d",frame_type,pts,size);
    if (mDumper)
        mDumper->dump((uint8_t*) buffer,size);
    if (mESConvertorCallback && mStart)
        mESConvertorCallback->onEsBufferAvailable(buffer, size, frame_type, pts);
}



};//namespace android