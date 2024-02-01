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

#define LOG_TAG "SystemControl-FRA"
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

#define PROP_KEY_AFR_DISABLED "vendor.sysctl.afr_disabled"

FrameRateAutoAdaption::FrameRateAutoAdaption(Callback *cb): mVdinEventFd(-1), mHdmiCallback(cb){
    mLastFrameRate = 0;
    videoLayerOn = false;
    mPlayFlag = false;
    mLastFromVdin = true;
    mFracDefaultValue = -1;
    mClock.tv_sec = 0;
    mClock.tv_usec = 0;

    mAFRDisabled = mSysWrite.getPropertyBoolean(PROP_KEY_AFR_DISABLED, false);
    SYS_LOGD("### AFR %s ###", (mAFRDisabled ? "Disabled" : "Enabled"));
    if (!mAFRDisabled) {
        mVdinEventFd = open(VDIN_EVENT_FILE, O_RDWR);
#ifdef FRAMERATE_MODE
        mTask = new MessageTask(this);
        mTask->run("DealyCheckFrame");
#endif
        if (mVdinEventFd < 0) {
            SYS_LOGE("can't open device /dev/vdin0");
        }

        parseConfigFile();
    }
}

FrameRateAutoAdaption::~FrameRateAutoAdaption() {
    if (mVdinEventFd >0) {
        close(mVdinEventFd);
        mVdinEventFd = -1;
    }
}
int FrameRateAutoAdaption::isDLGOn() {
#ifdef FRAMERATE_MODE
    if (pCPQControl != NULL) {
        SYS_LOGD("get pCPQControl dlgenable");
        return pCPQControl->GetDLGEnable();
    }
#endif
    return 0;
}
int FrameRateAutoAdaption::getLastFrame() {
    return mLastFrameRate;
}

int FrameRateAutoAdaption::parseConfigFile() {
    const char* WHITESPACE = " \t\r";

    SysTokenizer* tokenizer;
    int status = SysTokenizer::open(FRAME_RATE_POLIY_CONFIG, &tokenizer);
    if (status) {
        SYS_LOGE("Error %d opening framerate config file %s.", status, FRAME_RATE_POLIY_CONFIG);
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
                    if (framerate != 0)
                        mFramerateList.push_back(VIDEORATE*1.0/framerate);
                    else
                        SYS_LOGE("framerate is 0\n");
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

void FrameRateAutoAdaption::initialDefaultValue() {

    if (mFracDefaultValue == -1) {
        char policyVal[MODE_LEN] = {0};
        mSysWrite.readSysfs(HDMI_TX_FRAMERATE_POLICY, policyVal);
        mFracDefaultValue = atoi(policyVal);
    }
}

void FrameRateAutoAdaption::onTxUeventReceived(uevent_data_t* ueventData){
    SYS_LOGD("[%s] +++ ", __FUNCTION__ );
    initialDefaultValue();
    if (isFrameRateOn()) {
        inputValidateAndParse(ueventData, INPUT_TYPE_UEVENT);
    }
    SYS_LOGD("[%s] --- ", __FUNCTION__ );
}

bool FrameRateAutoAdaption::isFrameRateOn() {
    if (mAFRDisabled) {
        return false;
    }

    char framerateMode[FRAMERATEBIT] ={0};
    int exit = mSysWrite.getPropertyInt(VENDOR_BOOT_COMPLETE,0);
    SYS_LOGD("--isFrameRateOn %d",exit);
    if (exit == 0) return false;

    mSysWrite.readSysfs(HDMI_FRAME_RATE_AUTO, framerateMode);
    if (OUTPUT_TYPE_LCD_PANEL == getOutputAdaptType()) {
        SYS_LOGI("hdmi frame rate is on\n");
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
            mSysWrite.readSysfs(HDMI_TX_FRAMERATE_POLICY, policyVal);
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

    char curDisplayMode[MODE_LEN] = {0};
    DisplayModeMgr::getInstance().getDisplayMode(curDisplayMode, MODE_LEN);
    SYS_LOGD("setVideoLayerOn,on: %d, mLastFrameRate: %d curDisplayMode: %s", on, mLastFrameRate, curDisplayMode);
    if (!on) {
        struct timeval t2;
        gettimeofday( &t2, NULL );
        long dur = ((t2.tv_sec-mClock.tv_sec)*1000+(t2.tv_usec-mClock.tv_usec)/1000);
        SYS_LOGI("policycontrol by setVideoLayerOn %d on %d lastframe %d and cost %ld",videoLayerOn,on,mLastFrameRate,dur);
        if (dur < 1000) {
            return;
        }
    }
    if (videoLayerOn == on) {
        SYS_LOGD("last is also videolayer %d",videoLayerOn);
        return;
    }
    videoLayerOn = on;
    if (on) {
#ifdef FRAMERATE_MODE
        mTask->cancelTask();
#endif
        if (mLastFrameRate > 0) {
            SYS_LOGD("policyControl, last video layer on %d",mLastFromVdin);
            policyControl(mLastFrameRate);
        }
    }else {
        SYS_LOGD("backFrom4k1k(mLastFrameRate: %d)", mLastFrameRate);
        backFrom4k1k(mLastFrameRate);
       // if (mLastFrameRate == 0) {
            mPlayFlag = false;
#ifdef FRAMERATE_MODE
            mTask->sendMessage(ms2ns(500));
#else
            restoreEnv();
#endif
        //}
    }
}
void FrameRateAutoAdaption::policyControl(int frameRateValue) {
#ifdef FRAMERATE_MODE
    mTask->sendMessage(1);
#else
    delayControl(frameRateValue);
#endif
}

void FrameRateAutoAdaption::delayControl(int frameRateValue){
    char framerateMode[8] ={0};
    char curDisplayMode[MODE_LEN] = {0};
    char newDisplayMode[MODE_LEN] = {0};
    int outType = getOutputAdaptType();
    mSysWrite.readSysfs(HDMI_FRAME_RATE_AUTO, framerateMode);
    DisplayModeMgr::getInstance().getDisplayMode(curDisplayMode, MODE_LEN);
    if (outType == OUTPUT_TYPE_CVBS || !videoLayerOn) {
        SYS_LOGD("cvbs mode or !videoLayerOn do not need auto frame rate\n");
        return;
    }if (frameRateValue == -1) {
        frameRateValue = mLastFrameRate;
        SYS_LOGD("DLG Control %d",frameRateValue);
    } else {
        if (NULL != strstr(curDisplayMode, "smpte")) {
            SYS_LOGD("smpte mode do not need auto frame rate\n");
            return;
        }
    }
    SYS_LOGD("mPlayFlag %d frameRateValue%d mLastFromVdin %d\n",mPlayFlag,frameRateValue,mLastFromVdin);
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
        mPlayFlag = true;
#ifdef FRAMERATE_MODE
        mTask->resetPlayFlag(seconds(60*5));
#endif
        outputDispatch(NULL, outType, OUTPUT_MODE_STATE_SWITCH, frameRateValue, mLastFromVdin);
    }

}
void  FrameRateAutoAdaption::setPlayFlag(bool play) {
    mPlayFlag = play;
}
int  FrameRateAutoAdaption::findNearlyFrame(int frameRate) {
    int videoframe = (VIDEORATE*1.0/frameRate)*100;
    int smalldip = 12100; //0hz to 6000hz is the bigest.
    std::vector<double>::iterator itr = mFramerateList.begin();
    if (videoframe > smalldip) return 12000;
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
                SYS_LOGD("INPUT_TYPE_UEVENT mLastFromVdin:%d cur %d",mLastFrameRate,frameRateValue);
                if (mLastFrameRate == frameRateValue) {
                    //double message between play videoLayer
                    break;
                }

                mLastFrameRate = frameRateValue;
                SYS_LOGD("in event receive mLastFromVdin:%d lastFrame %d videoLayerOn%d and decide policy control or restore %p", mLastFromVdin, mLastFrameRate, videoLayerOn,this);
                if (frameRateValue > 0 && videoLayerOn) {
                    mLastFromVdin = false;
                    policyControl(frameRateValue);
                }else if (frameRateValue == 0 && !videoLayerOn) {
                    mPlayFlag = false;
                    restoreEnv();
                }
                break;
            } else if (!strcmp(ueventData->matchName, FRAME_RATE_VDIN0_UEVENT) ||
                        !strcmp(ueventData->matchName, FRAME_RATE_VDIN1_UEVENT)||
                        !strcmp(ueventData->matchName, FRAME_RATE_VDIN0_UEVENT_N) ||
                        !strcmp(ueventData->matchName, FRAME_RATE_VDIN1_UEVENT_N)) {
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
                     if (info.fps <= 0) break;
                     int fpsTemp = info.fps;
                        SYS_LOGD("read vdin fps: %d\n", fpsTemp);
                     if (fpsTemp == 48) {
                        fpsTemp = 24;
                        mLastFromVdin = false;
                     } else if (fpsTemp >= 100) {
                        fpsTemp = (fpsTemp/2);
                        mLastFromVdin = false;
                     } else {
                        mLastFromVdin = true;
                    }
                     SYS_LOGD("new vdin fps: %d %d\n", fpsTemp,mLastFromVdin);
                     frameRateValue = VIDEORATE/fpsTemp;
                }
                if ((info.event_sts & TVIN_SIG_CHG_VS_FRQ) != 0) {
                    tvin_info_s info;
                    ioctl(mVdinEventFd, TVIN_IOC_G_SIG_INFO, &info);
                    if (info.fps <= 0) break;
                    int fpsTemp = info.fps;
                    SYS_LOGD("read vdin fps: %d\n", fpsTemp);
                    if (fpsTemp == 48) {
                        fpsTemp = 24;
                        mLastFromVdin = false;
                    }else if (fpsTemp >= 100) {
                        fpsTemp = (fpsTemp/2);
                        mLastFromVdin = false;
                    }else {
                        mLastFromVdin = true;
                    }
                    SYS_LOGD("new vdin pos fps: %d %d\n", fpsTemp,mLastFromVdin);
                    frameRateValue = VIDEORATE/fpsTemp;
                }
                if (frameRateValue <= 0 || frameRateValue == mLastFrameRate) {
                    SYS_LOGD("same framerate recv by vdin %d",mLastFrameRate);
                    break;
                }
                mLastFrameRate = frameRateValue;
                SYS_LOGD("in vdin event receive mLastFromVdin:%d decide policy control or restore %d", mLastFromVdin, videoLayerOn);
                if (frameRateValue > 0 && videoLayerOn) {
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
        outputDispatch(NULL,OUTPUT_TYPE_LCD_PANEL,0,0,true);
        break;
        default:
        break;
    }
}

bool FrameRateAutoAdaption::enter4k1k(int framerate) {
    char curDisplayMode[MODE_LEN] = {0};
    DisplayModeMgr::getInstance().getDisplayMode(curDisplayMode, MODE_LEN);
    SYS_LOGD("FrameRateAutoAdaption::enter4k1k %d %d %s",framerate,videoLayerOn,curDisplayMode);
    if (videoLayerOn) {
         if (framerate == FRAME_RATE_DURATION_25 ||framerate == FRAME_RATE_DURATION_50 || framerate == FRAME_RATE_DURATION_125) {
            if (strstr(curDisplayMode,"3840x1080p") != NULL) {
                DisplayModeMgr::getInstance().setFrameRate(100.f,
                            "4k1k framerate only");
            }else {
                mHdmiCallback->setDisplayModeinner("3840x1080p100hz");
            }
            return true;
        }else if (FRAME_RATE_DURATION_5994 == framerate || FRAME_RATE_DURATION_2397 == framerate
                          ||FRAME_RATE_DURATION_2398 == framerate ||FRAME_RATE_DURATION_2997 == framerate
                          ||FRAME_RATE_DURATION_5992 == framerate){
            if (strstr(curDisplayMode,"3840x1080p") != NULL) {
                DisplayModeMgr::getInstance().setFrameRate(119.f,
                            "4k1k framerate only");
            }else {
                mHdmiCallback->setDisplayModeinner("3840x1080p119hz");
            }
            return true;
        }else {
            if (strstr(curDisplayMode,"3840x1080p") != NULL) {
                DisplayModeMgr::getInstance().setFrameRate(120.f,
                            "4k1k framerate only");
            }else {
                mHdmiCallback->setDisplayModeinner("3840x1080p120hz");
            }
            return true;
        }
    }
    return false;
}
bool FrameRateAutoAdaption::backFrom4k1k(int frameRate) {
    char curDisplayMode[MODE_LEN] = {0};
    DisplayModeMgr::getInstance().getDisplayMode(curDisplayMode, MODE_LEN);
    SYS_LOGD("backFrom4k1k %s",curDisplayMode);
    if (strstr(curDisplayMode,"3840x1080p") != NULL) {
        if (frameRate == FRAME_RATE_DURATION_25 ||frameRate == FRAME_RATE_DURATION_50 || frameRate == FRAME_RATE_DURATION_125) {
            mHdmiCallback->setDisplayModeinner("2160p50hz");
            SYS_LOGD("setDisplayModeinner 2160p50hz");
            return true;
        }
        mHdmiCallback->setDisplayModeinner("2160p60hz");
            SYS_LOGD("setDisplayModeinner 2160p60hz");
        return true;
    }
    return false;
}
#ifdef FRAMERATE_MODE
void FrameRateAutoAdaption::setPQHandle(CPQControl* handle) {
    pCPQControl = handle;
}
#endif
void FrameRateAutoAdaption::outputDispatch(char* outputMode, int outType, int state, int frameRate, bool isVdinShrink) {
    /*support hdmi out and panel output*/
    gettimeofday( &mClock, NULL );
    switch (outType) {
        case OUTPUT_TYPE_HDMI_TX: {
            break;
        }
        case OUTPUT_TYPE_LCD_PANEL: {

            char newMode[MODE_LEN] = {0};
            char DisplayRange[MODE_LEN] = {0};
            bool doubleRate = false;
            const char blank[2]=" ";
            //mSysWrite.readSysfs(FRAMERAT_PANEL_OUT,PanelValue);
            char DisplayVdin[MODE_LEN] = {0};
            if (mSysWrite.readSysfsOriginal(VOUT_DISPLAY_RANGE,DisplayRange)) {
                char *token = strtok(DisplayRange, blank);
                if (token != NULL) {
                    int lowval = atoi(token);
                    token = strtok(NULL, blank);
                    int topval = atoi(token);
                    if (topval >= 100) doubleRate = true;
                }
            }
            if (frameRate == 0) {
                if (!backFrom4k1k(0)) {
                    SYS_LOGD("tv set outputmode frameRate 0");
                    DisplayModeMgr::getInstance().setFrameRate(0,
                            "tv set outputmode frameRate 0");
                }
                return;
            }else {
                float fps =6000.f;
                int dlgOn = isDLGOn();
#ifdef FRAMERATE_MODE
                if (pCPQControl != NULL) {
                    SYS_LOGD("pCPQControl->GetPQMode() %d %d",pCPQControl->GetPQMode(),dlgOn);
                }
                //memc on+dlgOn 60->120hz
                if ((dlgOn == 1) && (pCPQControl != NULL) && pCPQControl->GetMemcMode() > 0
                    && pCPQControl->GetPQMode() != 6 && pCPQControl->GetPQMode() != 7 && enter4k1k(frameRate)) {
                    SYS_LOGD("memc on,force enter4k1k ok");
                    return;
                }
#endif
                /*dlg open, while pq is 6 or 7, then when vdin is 48 or more than 100 or decoder value, the
                *the highest value in display range. when vdin is lower than 100 , back from dlg then
                *set the value to 50/60.
                */
                SYS_LOGD("isDLGOn() %d frameRate %d videoLayerOn %d isVdinShrink %d",dlgOn,frameRate,videoLayerOn,isVdinShrink);
                if ((dlgOn == 1) && !isVdinShrink) {
                    fps = getFrameRateValue(frameRate,true);
                    SYS_LOGD("vdin shrink setframe %f",fps);
                    DisplayModeMgr::getInstance().setFrameRate(fps / 100.f,
                            "outputDispatch shrink");
                    return;
                }else if ((dlgOn == 1) && isVdinShrink){
                    SYS_LOGD("system freq is %d",doubleRate);
                    if (doubleRate) {
                        backFrom4k1k(frameRate);
                    }
                    fps = getFrameRateValue(frameRate,false);
                    SYS_LOGD("leave 4k1k inner %f",fps);
                    DisplayModeMgr::getInstance().setFrameRate(fps/ 100.f,
                            "outputDispatch 111");
                    return;
                }
                if ((dlgOn == 0) && backFrom4k1k(frameRate)) {
                    SYS_LOGD("leave 4k1k");
                    return;
                }
                fps = getFrameRateValue(frameRate,doubleRate);
                SYS_LOGD("afr output final %f", fps);
                DisplayModeMgr::getInstance().setFrameRate(fps / 100.f,
                            "outputDispatch 222");
            }
            break;
        }
        case OUTPUT_TYPE_CVBS:
            break;
        default:
            break;
    }
}

float FrameRateAutoAdaption::getFrameRateValue(int frameRate, bool doubleRate) {
    bool mode24p = false;
    int ret = mSysWrite.getPropertyInt(VOUT_24P_PROP, 0);
    if (ret == 1) {
        SYS_LOGD("VOUT_24P_PROP is on");
        mode24p = true;
    }
    const char* frameRateValue = doubleRate? "12000":"6000";//default 60hz
    if (frameRate == FRAME_RATE_DURATION_1440) {
        frameRateValue = "14400";
    }else if (frameRate == FRAME_RATE_DURATION_25 ||frameRate == FRAME_RATE_DURATION_50 || frameRate == FRAME_RATE_DURATION_125) {
        frameRateValue = doubleRate? "10000":"5000";
    }else if (FRAME_RATE_DURATION_24 == frameRate ) {
        frameRateValue = doubleRate? "12000":(mode24p ? "4800" : "6000");
    }else if (frameRate == FRAME_RATE_DURATION_1440) {
        frameRateValue = doubleRate? "14400":"7200";
    }else if (FRAME_RATE_DURATION_5994 == frameRate || FRAME_RATE_DURATION_2397 == frameRate
              ||FRAME_RATE_DURATION_2398 == frameRate ||FRAME_RATE_DURATION_2997 == frameRate
              ||FRAME_RATE_DURATION_5992 == frameRate){
        frameRateValue = doubleRate? "11988":"5994";
    }
    return atof(frameRateValue);
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
