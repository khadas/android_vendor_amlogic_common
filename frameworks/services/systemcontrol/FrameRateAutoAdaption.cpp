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

#define LOG_TAG "FRA"
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
    mFracDefaultValue = -1;
    mClock.tv_sec = 0;
    mClock.tv_usec = 0;
    mLastFromVdin = false;
    mPictureMode = false;
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
        return pCPQControl->GetDLGEnable();
    }
#endif
    return 0;
}
int FrameRateAutoAdaption::getLastFrame() {
    return mLastFrameRate;
}
bool FrameRateAutoAdaption::enter4k1kByUI(bool on) {
    if (!isFrameRateOn()) return true;
    SYS_LOGD("enter4k1k by ui %d",on);
#ifdef FRAMERATE_MODE
    mTask->sendMessageDlg(0);
#endif
    return true;
}
void FrameRateAutoAdaption::enter4k1korBack() {
    if (!isFrameRateOn()) return;
    SYS_LOGD("enter4k4k");
    int framerate = mLastFrameRate <= 0?FRAME_RATE_DURATION_60:mLastFrameRate;
    if (!videoLayerOn) {
        framerate = FRAME_RATE_DURATION_60;
    }
    char curDisplayMode[MODE_LEN] = {0};
    DisplayModeMgr::getInstance().getDisplayMode(curDisplayMode, MODE_LEN);
    std::string customStr("3840x1080p");
    std::string customStr2("2160p");
    if (isDLGOn() && strstr(curDisplayMode,customStr.c_str()) == NULL) {
        afrInDLG(customStr,framerate,false);
    } else if (!isDLGOn() && strstr(curDisplayMode,customStr2.c_str()) == NULL) {
        afrInDLG(customStr2,framerate,false);
    }
    mLastFrameRate = -1;
}

int FrameRateAutoAdaption::parseConfigFile() {
    const char* WHITESPACE = " \t\r";

    SysTokenizer* tokenizer;
    int status = SysTokenizer::open(FRAME_RATE_POLICY_CONFIG, &tokenizer);
    if (status) {
        SYS_LOGE("Error %d opening framerate config file %s.", status, FRAME_RATE_POLICY_CONFIG);
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
                        mFramerateList.push_back(framerate);
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
    initialDefaultValue();
    if (isFrameRateOn()) {
        inputValidateAndParse(ueventData, INPUT_TYPE_UEVENT);
    }
}

bool FrameRateAutoAdaption::isFrameRateOn() {
    if (mAFRDisabled) {
        return false;
    }

    char framerateMode[FRAMERATEBIT] ={0};
    int exit = mSysWrite.getPropertyInt(VENDOR_BOOT_COMPLETE,0);
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

    SYS_LOGD("setVideoLayerOn,on: %d, mLastFrameRate: %d ", on, mLastFrameRate);
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
        //SYS_LOGD("last is also videolayer %d",videoLayerOn);
        return;
    }
    videoLayerOn = on;
    if (on) {
#ifdef FRAMERATE_MODE
        mTask->cancelTask();
#endif
        if (mLastFrameRate > 0) {
            SYS_LOGD("policyControl, last video layer on %d",mLastFrameRate);
            policyControl(mLastFrameRate);
        }
    }else {
        SYS_LOGD("video layer off (mLastFrameRate: %d)", mLastFrameRate);

        char curDisplayMode[MODE_LEN] = {0};
        DisplayModeMgr::getInstance().getDisplayMode(curDisplayMode, MODE_LEN);
        std::string customStr("3840x1080p");
        if (isDLGOn() && strstr(curDisplayMode,customStr.c_str()) == NULL) {
            SYS_LOGD("video layer off and restore to dlg if dlg on %d",mLastFrameRate);
#ifdef FRAMERATE_MODE
            mTask->sendMessageDlg(0);
            return ;
#endif
        }
        if (mLastFrameRate > 0) {
            return;
        }
#ifdef FRAMERATE_MODE
        SYS_LOGD("message send here");
        mTask->sendMessage(ms2ns(100));
#endif
    }
}
bool FrameRateAutoAdaption::getVideoLayerOn() {
    return videoLayerOn;
}
bool FrameRateAutoAdaption::backFrom4k1k(int frameRate) {
    char curDisplayMode[MODE_LEN] = {0};
    DisplayModeMgr::getInstance().getDisplayMode(curDisplayMode, MODE_LEN);
    if (strstr(curDisplayMode,"3840x1080p") != NULL) {
        SYS_LOGD("in backFrom4k1k %s",curDisplayMode);
        return afrInDLG("2160",mLastFrameRate,false);
    }
    return false;
}
void FrameRateAutoAdaption::policyControl(int frameRateValue) {
    SYS_LOGD("policyControl mLastFrameRate %d frameRateValue %d mLastFromVdin %d",mLastFrameRate, frameRateValue,mLastFromVdin);
    if (!isFrameRateOn()) {
        return;
    }
#ifdef FRAMERATE_MODE
    if (frameRateValue == -1) {
        bool picmode = ((pCPQControl != NULL)
                    && (pCPQControl->GetPQMode() == 6 || pCPQControl->GetPQMode() == 7));
        SYS_LOGD("picmode %d mPictureMode %d,videoLayerOn %d isDLGOn %d",picmode, mPictureMode, videoLayerOn, isDLGOn());
        if (picmode == mPictureMode || !videoLayerOn || !isDLGOn()) {
            return;
        }else {
            mPictureMode = picmode;
        }
        if (mLastFrameRate <= 0 && videoLayerOn) {
            mLastFrameRate = FRAME_RATE_DURATION_60;
        }
    }
    mTask->sendMessage(0);
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
    }
    if (frameRateValue == -1) {
        frameRateValue = mLastFrameRate == 0 ? FRAME_RATE_DURATION_60:mLastFrameRate;
        mLastFromVdin = true;
        SYS_LOGD("DLG Control %d",frameRateValue);
    } else {
        if (NULL != strstr(curDisplayMode, "smpte")) {
            SYS_LOGD("smpte mode do not need auto frame rate\n");
            return;
        }
    }
        //always change when panel output

        std::vector<std::string> modes = configMap[frameRateValue];
        if (modes.size() <= 0) {
            frameRateValue = findNearlyFrame(frameRateValue);
        SYS_LOGD("decoder find new framerate is %d",frameRateValue);
            if (frameRateValue <= 0) return;
        }
        outputDispatch(NULL, outType, OUTPUT_MODE_STATE_SWITCH, frameRateValue, mLastFromVdin);
    }

/*
* the input fps is reliable since freesync or decoder calculate.
* make the input fps to known in framerate.cfg
*/
int  FrameRateAutoAdaption::findNearlyFrame(int frameRate) {

    int smalldip = 7681;//less than 12.5hz
    std::vector<int>::iterator itr = mFramerateList.begin();

    for (; itr != mFramerateList.end(); ++itr) {
        int dip = abs((*itr) - frameRate);
        if (dip < smalldip) {
            smalldip = dip;
        }
    }
    itr = mFramerateList.begin();
    //either videoframe+smalldip or videoframe-smalldip is in config list
    for (; itr != mFramerateList.end(); ++itr) {
        if ( (frameRate+smalldip) == (*itr)
                || (frameRate-smalldip) == (*itr)) {
            return (*itr);
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
                if (frameRateValue == 0 && mLastFrameRate != -1) {
                    SYS_LOGD("not afr just skip");
                    mLastFrameRate = -1;
                    return;
                }
                SYS_LOGD("INPUT_TYPE_UEVENT mLastFrameRate:%d cur %d",mLastFrameRate,frameRateValue);
                if (mLastFrameRate == frameRateValue) {
                    //double message between play videoLayer
                    break;
                }

                mLastFrameRate = frameRateValue;
                SYS_LOGD("in event receive lastFrame %d videoLayerOn%d and decide policy control or restore %p", mLastFrameRate, videoLayerOn,this);
                if (frameRateValue > 0 && videoLayerOn) {
                    mLastFromVdin = false;
                    policyControl(frameRateValue);
                }else if (frameRateValue == 0 && !videoLayerOn) {
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
                        SYS_LOGD("signal unstable and last frame %d mLastFromVdin %d",mLastFrameRate, mLastFromVdin);
                        break;
                     }
                     if (info.fps <= 0) break;
                     int fpsTemp = info.fps;

                     SYS_LOGD("new vdin fps: %d\n", fpsTemp);
                     frameRateValue = VIDEORATE/fpsTemp;
                }
                if ((info.event_sts & TVIN_SIG_CHG_VS_FRQ) != 0) {
                    tvin_info_s info;
                    ioctl(mVdinEventFd, TVIN_IOC_G_SIG_INFO, &info);
                    if (info.fps <= 0) break;
                    int fpsTemp = info.fps;
                    SYS_LOGD("new vdin pos fps: %d\n", fpsTemp);
                    frameRateValue = VIDEORATE/fpsTemp;
                }
                if (frameRateValue <= 0 || frameRateValue == mLastFrameRate) {
                    SYS_LOGD("same framerate recv by vdin %d",mLastFrameRate);
                    break;
                }
                mLastFrameRate = frameRateValue;
                mLastFromVdin = true;
                SYS_LOGD("in vdin event receive %d decode policy control or restore %d", frameRateValue, videoLayerOn);
                if (frameRateValue > 0 && videoLayerOn) {
                    //for vdin event we not care about whether it restore or not
                    //mPlayFlag set false make policyControl available everty time
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
    SYS_LOGD("restore");
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

bool FrameRateAutoAdaption::afrInDLG(std::string customStr, int frameValue, bool frameOnly) {
    int ret = mSysWrite.getPropertyInt(VOUT_24P_PROP, 0);
    bool isRestore = false;
    if (frameValue == 0 ) {
        frameValue = FRAME_RATE_DURATION_60;
        isRestore = true;
    }
    if (ret == 0 && frameValue == FRAME_RATE_DURATION_24) {
        frameValue = FRAME_RATE_DURATION_60;
    }
    std::map<int, std::string> list;
    DisplayModeMgr::getInstance().getSupportDisplayModes(list,customStr);
    std::map<int, std::string>::iterator p;
    std::multimap<int, std::string, std::greater<int> > modelist;
    typedef std::multimap<int, std::string, std::greater<int> >::value_type vt;
    for (p = list.begin(); p != list.end(); p++) {
        modelist.insert(vt(p->first, p->second));
    }
    std::multimap<int, std::string, std::greater<int>>::iterator iter;
    int width =0;
    int height =0;
    for (iter = modelist.begin(); iter != modelist.end(); iter++) {
        SYS_LOGD("afrInDLG frameRate %d mode %d %s",frameValue,iter->first,iter->second.c_str());
        if ((int)(iter->first/100) % (int)(VIDEORATE/frameValue) == 0) {
            if (frameOnly) {
                if (isRestore) {
                    SYS_LOGD("restore framerate");
                    DisplayModeMgr::getInstance().setFrameRate(0,
                            "outputDispatch afrInDLG");
            }else {
                    SYS_LOGD("framerateOnly %d %s",iter->first,iter->second.c_str());
                    DisplayModeMgr::getInstance().setFrameRate((iter->first)/100.0f,
                            "outputDispatch afrInDLG");
            }
            }else {
                SYS_LOGD("setDisplayMode %d %s and restore %d",iter->first,iter->second.c_str(),isRestore);
                DisplayModeMgr::getInstance().getModeDetail(iter->second.c_str(),width,height);
                SYS_LOGD("mgr update name to size %dx%d",width,height);
                if (width >0 && height >0) {
                    if (height == 1080) width = width/2;
                    mHdmiCallback->setActiveModeRemote(width,height,(int)(iter->first));
            }
                if (isRestore) {
                    DisplayModeMgr::getInstance().setFrameRate(0,
                            "outputDispatch afrInDLG");
                }
            }
            return true;
        }
    }
    return false;
}
bool FrameRateAutoAdaption::enter4k1k(int framerate) {
    char curDisplayMode[MODE_LEN] = {0};
    DisplayModeMgr::getInstance().getDisplayMode(curDisplayMode, MODE_LEN);
    SYS_LOGD("FrameRateAutoAdaption::enter4k1k %d %d %s",framerate,videoLayerOn,curDisplayMode);
    std::string customStr("3840x1080p");
    if (strstr(curDisplayMode,customStr.c_str()) != NULL) {
        SYS_LOGD("already in dlg,just afr");
        afrInDLG(customStr,framerate,true);
            return true;
        }
    if (strstr(curDisplayMode,"2160") != NULL) {
        SYS_LOGD("dlg change mode");
        return afrInDLG(customStr,framerate,false);
    }
    return false;
}
bool FrameRateAutoAdaption::freesyncFrame(int frameRate) {
    char curDisplayMode[MODE_LEN] = {0};
    DisplayModeMgr::getInstance().getDisplayMode(curDisplayMode, MODE_LEN);
    std::string mode(curDisplayMode);
    int pos = mode.find("p") <0? mode.find("i"):mode.find("p");
    std::string str = mode.substr(0,pos);
    SYS_LOGD("freesyncFrame %s",str.c_str());
    std::map<int, std::string> list;
    std::string filter;
    DisplayModeMgr::getInstance().getSupportDisplayModes(list,filter);
    std::map<int, std::string>::iterator p;
    std::multimap<int, std::string, std::less<int> > modelist;
    typedef std::multimap<int, std::string, std::less<int> >::value_type vt;
    for (p = list.begin(); p != list.end(); p++) {
        modelist.insert(vt(p->first, p->second));
    }
    std::multimap<int, std::string, std::less<int>>::iterator iter;
    int width = 0;
    int height = 0;
    for (iter = modelist.begin(); iter != modelist.end(); iter++) {
        SYS_LOGD("freesyncFrame modelist  %s, iter->first %d",iter->second.c_str(),iter->first);
        if ((int)(iter->first/100) == (int)(VIDEORATE/frameRate) && iter->second.find(str) == std::string::npos) {
            SYS_LOGD("freesyncFrame change display mode %s",iter->second.c_str());
            DisplayModeMgr::getInstance().getModeDetail(iter->second.c_str(),width,height);
            if (width >0 && height >0) {
                mHdmiCallback->setActiveModeRemote(width,height,(int)(iter->first));
            }
            return true;
        }else  if ((int)(iter->first/100) == (int)(VIDEORATE/frameRate)) {
            SYS_LOGD("freesyncFrame set rate only %s",iter->second.c_str());
            DisplayModeMgr::getInstance().setFrameRate((iter->first)/100.0f,
                            "outputDispatch freesync");
    return false;
        }
    }
    return -1;
}
#ifdef FRAMERATE_MODE
void FrameRateAutoAdaption::setPQHandle(CPQControl* handle) {
    pCPQControl = handle;
}
#endif
void FrameRateAutoAdaption::outputDispatch(char* outputMode, int outType, int state, int frameRate, bool isVdin) {
    /*support hdmi out and panel output*/
    gettimeofday( &mClock, NULL );
    switch (outType) {
        case OUTPUT_TYPE_HDMI_TX: {
            break;
        }
        case OUTPUT_TYPE_LCD_PANEL: {

            //mSysWrite.readSysfs(FRAMERAT_PANEL_OUT,PanelValue);
            if (frameRate == 0) {
                SYS_LOGD("restore mLastFrame %d",mLastFrameRate);
               // if (!backFrom4k1k(1600)) {
                    SYS_LOGD("tv set outputmode frameRate 0");
                    DisplayModeMgr::getInstance().setFrameRate(0,
                            "tv set outputmode frameRate 0");
               // }
                return;
            }else {
                int dlgOn = isDLGOn();
#ifdef FRAMERATE_MODE
                if (pCPQControl != NULL) {
                    SYS_LOGD("pCPQControl->GetPQMode() %d %d",pCPQControl->GetPQMode(),dlgOn);
                }
                SYS_LOGD("isDLGOn() %d frameRate %d videoLayerOn %d isVdin %d",dlgOn,frameRate,videoLayerOn,isVdin);
                //memc on+dlgOn 60->120hz
                /*if ((dlgOn == 1) && (((pCPQControl != NULL) && pCPQControl->GetMemcMode() > 0
                    && pCPQControl->GetPQMode() != 6 && pCPQControl->GetPQMode() != 7) || !isVdin) && enter4k1k(frameRate)) {
                    SYS_LOGD("memc on,force enter4k1k ok");
                    return;
                }*/
                /*dlg open, while pq is 6 or 7, then when vdin is 48 or more than 100 or decoder value, the
                *the highest value in display range. when vdin is lower than 100 , back from dlg then
                *set the value to 50/60.
                */
                if ((dlgOn == 1) && isVdin && ((pCPQControl != NULL)
                    && (pCPQControl->GetPQMode() == 6 || pCPQControl->GetPQMode() == 7))){
                    freesyncFrame(frameRate);
                    mLastFrameRate = -1;
                    return;
                }
#endif
                if ((dlgOn == 0) && backFrom4k1k(frameRate)) {
                    mLastFrameRate = -1;
                    SYS_LOGD("leave 4k1k");
                    return;
                }
                if (!afrOnly(frameRate)) {
                    SYS_LOGD("afr output final %f", VIDEORATE*1.0f/frameRate);
                    DisplayModeMgr::getInstance().setFrameRate(VIDEORATE*1.0f/frameRate,
                            "outputDispatch 222");
                }
                mLastFrameRate = -1;
            }
            break;
        }
        case OUTPUT_TYPE_CVBS:
            break;
        default:
            break;
    }
}

bool FrameRateAutoAdaption::afrOnly(int frameValue) {
    int ret = mSysWrite.getPropertyInt(VOUT_24P_PROP, 0);
    if (frameValue == 0 ) {
        DisplayModeMgr::getInstance().setFrameRate(0,
                            "outputDispatch afr only");
        return true;
    }
    if (ret == 0 && frameValue == FRAME_RATE_DURATION_24) {
        frameValue = FRAME_RATE_DURATION_60;
    }
    char curDisplayMode[MODE_LEN] = {0};
    DisplayModeMgr::getInstance().getDisplayMode(curDisplayMode, MODE_LEN);
    std::string mode(curDisplayMode);
    int pos = mode.find("p") <0? mode.find("i"):mode.find("p");
    std::string str = mode.substr(0,pos);
    SYS_LOGD("afrOnly getFrameRateValue %s",str.c_str());
    std::map<int, std::string> list;
    DisplayModeMgr::getInstance().getSupportDisplayModes(list,str);
    std::map<int, std::string>::iterator p;
    std::multimap<int, std::string, std::greater<int> > modelist;
    typedef std::multimap<int, std::string, std::greater<int> >::value_type vt;
    for (p = list.begin(); p != list.end(); p++) {
        modelist.insert(vt(p->first, p->second));
    }
    std::multimap<int, std::string, std::greater<int>>::iterator iter;
    for (iter = modelist.begin(); iter != modelist.end(); iter++) {
        if (((int)(iter->first/100) % (int)(VIDEORATE/frameValue) ==0) ) {
            SYS_LOGD("afr only change set framerate %s %d",iter->second.c_str(),iter->first);
            DisplayModeMgr::getInstance().setFrameRate((iter->first)/100.0f,
                            "outputDispatch afr only");
            return true;
        }
    }
    return false;
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
