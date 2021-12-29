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
#include <cutils/properties.h>
#include <stdint.h>
#include <sys/types.h>

#include "DisplayMode.h"
#include "FrameRateAutoAdaption.h"
#include "SysTokenizer.h"
#include "PQ/include/PQType.h"
#define VIDEORATE        96000
#define FRAMERATEBIT     8

FrameRateAutoAdaption::FrameRateAutoAdaption(Callbak *cb): mVdinEventFd(-1), mHdmiCallback(cb){
    mVdinEventFd = open(VDIN_EVENT_FILE, O_RDWR);
    mLastFrameRate = 0;
    videoLayerOn = false;
    mPlayFlag = false;
    mFracDefaultValue = -1;
    SYS_LOGE("can't open device /dev/vdin0");
#ifdef FRAMERATE_MODE
    mTask = new MessageTask(this);
    mTask->run("DealyCheckFrame");
#endif
    if (mVdinEventFd < 0) {
        SYS_LOGE("can't open device /dev/vdin0");
    }
    parseConfigFile();
}

FrameRateAutoAdaption::~FrameRateAutoAdaption() {
}

int FrameRateAutoAdaption::getLastFrame() {
    return mLastFrameRate;
}

int FrameRateAutoAdaption::parseConfigFile() {
    const char* WHITESPACE = " \t\r";

    SysTokenizer* tokenizer;
    int status = SysTokenizer::open(FRAME_RATE_POLIY_COFIG, &tokenizer);
    if (status) {
        SYS_LOGE("Error %d opening framerate config file %s.", status, FRAME_RATE_POLIY_COFIG);
    } else {
        while (!tokenizer->isEof()) {
            tokenizer->skipDelimiters(WHITESPACE);
            if (!tokenizer->isEol() && tokenizer->peekChar() != '#') {
                char *token = tokenizer->nextToken(WHITESPACE);
                if (NULL != token) {
                    std::vector<std::string> modes;
                    int framerate = atoi(token);
                    tokenizer->skipDelimiters(WHITESPACE);
                    modes.push_back(tokenizer->nextToken(WHITESPACE));
                    tokenizer->skipDelimiters(WHITESPACE);
                    modes.push_back(tokenizer->nextToken(WHITESPACE));
                    configMap.insert(std::pair<int, std::vector<std::string>>(framerate,modes));
                    mFramerateList.push_back(VIDEORATE*1.0/framerate);
                } else {
                    SYS_LOGE("%s: Expected keyword, got '%s'.", tokenizer->getLocation(), token);
                    break;
                }
            }
            tokenizer->nextLine();
        }
        delete tokenizer;
    }
    return status;
}

void FrameRateAutoAdaption::readSinkEdid(char *edid) {
    int count = 0;
    while (true) {
        mSysWrite.readSysfsOriginal(DISPLAY_HDMI_DISP_CAP, edid);
        if (strlen(edid) > 0)
            break;

        if (count >= 5) {
            strcpy(edid, "null edid");
            break;
        }
        count++;
        usleep(500000);
    }
}

void FrameRateAutoAdaption::initalDefaultValue() {

    if (mFracDefaultValue == -1) {
        char policyVal[MODE_LEN] = {0};
        mSysWrite.readSysfs(HDMI_TX_FRAMRATE_POLICY, policyVal);
        mFracDefaultValue = atoi(policyVal);
    }
}

void FrameRateAutoAdaption::onTxUeventReceived(uevent_data_t* ueventData){
    initalDefaultValue();
    if ( isFrameRateOn() ) {
        inputValidateAndParse(ueventData, INPUT_TYPE_UEVENT);
    }
}

bool FrameRateAutoAdaption::isFrameRateOn() {
    char framerateMode[FRAMERATEBIT] ={0};
    mSysWrite.readSysfs(HDMI_FRAME_RATE_AUTO, framerateMode);
    if (OUTPUT_TYPE_LCD_PANEL == getOutputAdaptType()) {
        return true;
    }
    if (!strcmp(framerateMode, FRAME_RATE_HDMI_OFF)) {
        SYS_LOGI("hdmi frame rate is off\n");
        return false;
    }
    SYS_LOGI("hdmi frame rate is on\n");
    return true;
}

bool FrameRateAutoAdaption::frameRateIsFloat(int frameRateValue) {
    if ((frameRateValue == FRAME_RATE_DURATION_2397)
            || (frameRateValue == FRAME_RATE_DURATION_2398)
            || (frameRateValue == FRAME_RATE_DURATION_2997)
            || (frameRateValue == FRAME_RATE_DURATION_5992)
            || (frameRateValue == FRAME_RATE_DURATION_5994)) {
                return true;
    }
    return false;
}

bool FrameRateAutoAdaption::currentDisplayIsFloat(int outputType) {
    char policyVal[MODE_LEN] = {0};
    int val = 0;
    if (outputType == -1) {
        return currentDisplayIsFloat(0) || currentDisplayIsFloat (2);
    }
    switch ( outputType ) {
        case OUTPUT_TYPE_HDMI_TX:{
            mSysWrite.readSysfs(HDMI_TX_FRAMRATE_POLICY, policyVal);
            val = atoi(policyVal);
            if ( val == 1 ) return true;
            else return false;
        }
        case OUTPUT_TYPE_LCD_PANEL:{
            mSysWrite.readSysfs(FRAMERAT_PANEL_OUT, policyVal);
            val = atoi(policyVal);
            if ( val == 1 ) return true;
            else return false;
        }
        default:break;
    }
    return false;
}


void FrameRateAutoAdaption::setVideoLayerOn(bool on) {
    if (!isFrameRateOn())
        return;
    videoLayerOn = on;
    SYS_LOGI("policycontrol by setVideoLayerOn %d on %d",mLastFrameRate,on);
    if (on) {
#ifdef FRAMERATE_MODE
        mTask->cancelTask();
#endif
        if (mLastFrameRate > 0) {
            policyControl(mLastFrameRate);
        }
    }else {
        if (mLastFrameRate == 0) {
            mPlayFlag = false;
#ifdef FRAMERATE_MODE
            mTask->sendMessage(ms2ns(500));
#else
            restoreEnv();
#endif
        }
    }
}

void FrameRateAutoAdaption::policyControl(int frameRateValue){
    char framerateMode[8] ={0};
    char curDisplayMode[MODE_LEN] = {0};
    char newDisplayMode[MODE_LEN] = {0};
    int outType = getOutputAdaptType();
    mSysWrite.readSysfs(HDMI_FRAME_RATE_AUTO, framerateMode);
    DisplayModeMgr::getInstance().getDisplayMode(curDisplayMode, MODE_LEN);
    if (outType == OUTPUT_TYPE_CVBS) {
        SYS_LOGD("cvbs mode do not need auto frame rate\n");
        return;
    } else {
        if (NULL != strstr(curDisplayMode, "smpte")) {
            SYS_LOGD("smpte mode do not need auto frame rate\n");
            return;
        }
    }
    SYS_LOGD("mPlayFlag %d frameRateValue%d\n",mPlayFlag,frameRateValue);
    if (OUTPUT_TYPE_LCD_PANEL == outType) {
        //always change when panel output
        mPlayFlag = false;
    }

    if (frameRateValue > 0 && !mPlayFlag) {
        std::vector<std::string> modes = configMap[frameRateValue];
        if (modes.size() <= 0) {
            frameRateValue = findNearlyFrame(frameRateValue);
            SYS_LOGD("find new framerate is %d",frameRateValue);
            if (frameRateValue <= 0) return;
            modes = configMap[frameRateValue];
        }
        if (modes.size() != 2) return;
        int backVal1 = atoi(modes[0].c_str());
        int backVal2 = atoi(modes[1].c_str());
        if (backVal1 > 0 && backVal1 % 25 == 0) {
            frameRateValue = FRAME_RATE_DURATION_50;
        }
        mPlayFlag = true;
#ifdef FRAMERATE_MODE
        mTask->resetPlayFlag(seconds(60*5));
#endif
        outputDispatch(NULL, outType, OUPUT_MODE_STATE_SWITCH, frameRateValue);
    }

}
void  FrameRateAutoAdaption::setPlayFlag(bool play) {
    mPlayFlag = play;
}
int  FrameRateAutoAdaption::findNearlyFrame(int frameRate) {
    int videoframe = (VIDEORATE*1.0/frameRate)*100;
    int smalldip = 6000; //0hz to 6000hz is the bigest.
    std::vector<double>::iterator itr = mFramerateList.begin();
    if (videoframe > smalldip) return smalldip;
    if (videoframe < 0) return 0;
    for (; itr != mFramerateList.end(); ++itr) {
        int dip = abs(int((*itr)*100) - videoframe);
        if (dip < smalldip) {
            smalldip = dip;
        }
    }
    itr = mFramerateList.begin();
    //either videoframe+smalldip or videoframe-smalldip is in config list
    for (; itr != mFramerateList.end(); ++itr) {
        if ( (videoframe+smalldip) == int((*itr)*100)
                || (videoframe-smalldip) == int((*itr)*100)) {
            return VIDEORATE/(*itr);
        }
    }
    return frameRate;
}

bool FrameRateAutoAdaption::frameRateChange(const char* curDisplayMode, const char* newDisplayMode,int frameRateValue,int outType) {
    if (strcmp(newDisplayMode,curDisplayMode) != 0) return true;
    if (frameRateValue == 0) {
        if (currentDisplayIsFloat(outType) && mFracDefaultValue == 0) {
            return true;
        }
        else if (!currentDisplayIsFloat(outType) && mFracDefaultValue == 1) {
            return true;
        }
        return false;
    }
    if (currentDisplayIsFloat(outType) && !frameRateIsFloat(frameRateValue)) {
        return true;
    }
    if (!currentDisplayIsFloat(outType) && frameRateIsFloat(frameRateValue)) {
        return true;
    }
    return false;
}

void FrameRateAutoAdaption::inputValidateAndParse(void* data, int inType) {
    /*support mutiinput*/
    int frameRateValue = -1;
    switch (inType) {
        case INPUT_TYPE_UEVENT: {
            uevent_data_t* ueventData = (uevent_data_t*)data;
            if (!strcmp(ueventData->matchName, FRAME_RATE_DECODER_UEVENT)) {
                if (!strcmp(ueventData->switchName, "end_hint")) {
                    frameRateValue = 0;
                } else {
                    sscanf(ueventData->switchName, "%d", &frameRateValue);
                }
                if (mLastFrameRate == frameRateValue) {
                    //double message between play videoLayer
                    break;
                }

                mLastFrameRate = frameRateValue;
                SYS_LOGD("in event receive frame:%d lastFrame %d videoLayerOn%d and decide policycontrol or restore %p", frameRateValue, mLastFrameRate, videoLayerOn,this);
                if (frameRateValue > 0 && videoLayerOn) {
                    policyControl(frameRateValue);
                }else if (frameRateValue == 0 && !videoLayerOn) {
                    mPlayFlag = false;
                    restoreEnv();
                }
                break;
            } else if (!strcmp(ueventData->matchName, FRAME_RATE_VDIN0_UEVENT) ||
                        !strcmp(ueventData->matchName, FRAME_RATE_VDIN1_UEVENT)) {
                if (mVdinEventFd < 0) {
                    mVdinEventFd = open(VDIN_EVENT_FILE, O_RDWR);
                        if (mVdinEventFd < 0) {
                            SYS_LOGE("open device /dev/vdin0 fail again %s",strerror(errno));
                            break;
                        }
                }
                vdin_event_info info;
                info.event_sts = 0;
                ioctl(mVdinEventFd, TVIN_IOC_G_EVENT_INFO, &info);
                //0x80000000 ,TVIN_SIG_CHG_STS
                if ((info.event_sts & 0x80000000) != 0) {
                     tvin_info_s info;
                     ioctl(mVdinEventFd, TVIN_IOC_G_SIG_INFO, &info);
                     if ( info.status != TVIN_SIG_STATUS_STABLE ) {
                        mLastFrameRate = 0;
                        mPlayFlag = false;
#ifdef FRAMERATE_MODE
                        mTask->sendMessage(seconds(VDIN_RESTORE_DELAY_DURATION));
#else
                        restoreEnv();
#endif
                        break;
                     }
                     SYS_LOGD("vdin fps: %d\n", info.fps);
                     if (info.fps <= 0) break;
                     frameRateValue = VIDEORATE/info.fps;
                }
                if ((info.event_sts & TVIN_SIG_CHG_VS_FRQ) != 0) {
                    tvin_info_s info;
                    ioctl(mVdinEventFd, TVIN_IOC_G_SIG_INFO, &info);
                    SYS_LOGD("vdin fps: %d\n", info.fps);
                     if (info.fps <= 0) break;
                    frameRateValue = VIDEORATE/info.fps;
                }
                if (frameRateValue <= 0 || mLastFrameRate == frameRateValue) {
                    SYS_LOGD("same framerate recv by vdin");
                    break;
                }
                mLastFrameRate = frameRateValue;
                SYS_LOGD("in vdin event receive frame:%d decide policycontrol or restore %d", frameRateValue, videoLayerOn);
                if (frameRateValue > 0 ) {
                    //for vdin event we not care about whether it restore or not
                    //mPlayFlag set false make policyControl available everty time
                    mPlayFlag = false;
#ifdef FRAMERATE_MODE
                    mTask->cancelTask();
#endif
                    policyControl(frameRateValue);
                }
            }
            break;
        }
        case INPUT_TYPE_API:
            break;
        default:
            break;
    }

}
void FrameRateAutoAdaption::restoreEnv() {
    if (videoLayerOn)
        return;
    int type = getOutputAdaptType();
    switch ( type ) {
        case OUTPUT_TYPE_HDMI_TX:
        break;
        case OUTPUT_TYPE_LCD_PANEL:
        outputDispatch(NULL,OUTPUT_TYPE_LCD_PANEL,0,0);
        break;
        default:
        break;
    }
}
void FrameRateAutoAdaption::outputDispatch(char* outputMode, int outType, int state, int frameRate) {
    /*support hdmi out and panel output*/
    switch (outType) {
        case OUTPUT_TYPE_HDMI_TX: {
            break;
        }
        case OUTPUT_TYPE_LCD_PANEL: {
            char ntsrunning[PROPERTY_VALUE_MAX] = {0};
            /*bool ret = mSysWrite.getPropertyString("vendor.netflix.state", (char *)ntsrunning, "bg");
            if (!ret || strncmp(ntsrunning,"fg",2)) {
                SYS_LOGD("AFR but nts is not running");
                return;
            }*/

            char PanelValue[MODE_LEN] = {0};
            mSysWrite.readSysfs(FRAMERAT_PANEL_OUT,PanelValue);
            if (frameRate == 0) {
                if (strcmp(PanelValue,"0")) {
                    SYS_LOGD("tv set outputmode frameRate 0");
                    DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_FR_HINT,"0");
                }
                return;
            }else {
                const char* frameRateValue = "6000";//default 60hz
                if (frameRate == FRAME_RATE_DURATION_25 ||frameRate == FRAME_RATE_DURATION_50) {
                    frameRateValue = "5000";
                }else if (FRAME_RATE_DURATION_5994 == frameRate || FRAME_RATE_DURATION_2397 == frameRate
                          ||FRAME_RATE_DURATION_2398 == frameRate ||FRAME_RATE_DURATION_2997 == frameRate
                          ||FRAME_RATE_DURATION_5992 == frameRate){
                    frameRateValue = "5994";
                }
                if (strcmp(PanelValue,frameRateValue)) {
                    DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_FR_HINT,frameRateValue);
                }
                SYS_LOGD("Read panel %s final outputmode frameRate%s", PanelValue, frameRateValue);
            }
            break;
        }
        case OUTPUT_TYPE_CVBS:
            break;
        default:
            break;
    }
}

int FrameRateAutoAdaption::getOutputAdaptType() {
    int type = -1;
    char vinfo[MAX_STR_LEN] = {0};
    //mode:  0=hdmi, 1=cvbs, 2=panel
    mSysWrite.readSysfsOriginal(SYSFS_DISPLAY_VINFO, vinfo);
    char *pos = strstr(vinfo, "mode:");
    while ((NULL != pos) && (*pos != '\n')) {
        if (*pos == '0') {
            type = OUTPUT_TYPE_HDMI_TX;
            break;
        } else if (*pos == '1') {
            type = OUTPUT_TYPE_CVBS;
            break;
        } else if (*pos == '2') {
            type = OUTPUT_TYPE_LCD_PANEL;
            break;
        }
        pos++;
    }
    //SYS_LOGD("mode: %d\n", type);
    return type;
}
