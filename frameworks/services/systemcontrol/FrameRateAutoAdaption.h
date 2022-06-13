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
 *  @author   Jinping Wang
 *  @version  2.0
 *  @date     2017/01/24
 *  @par function description:
 *  - 1 write property or sysfs in daemon
 */



#ifndef FRAMERATE_ADAPTER_H
#define FRAMERATE_ADAPTER_H

#include "SysWrite.h"
#include "common.h"
#include "UEventObserver.h"
#include "DisplayModeMgr.h"
#include <vector>
#include <map>
#include <string>
#include <sys/ioctl.h>
#ifdef FRAMERATE_MODE
#include "FrameRateMessage.h"
#endif
//Frame rate switch
#define FRAME_RATE_DECODER_UEVENT               "DEVPATH=/devices/virtual/framerate_adapter/framerate_dev"
#define FRAME_RATE_VDIN0_UEVENT                 "DEVTYPE=vdin0event"
#define FRAME_RATE_VDIN1_UEVENT                 "DEVTYPE=vdin1event"
#define VDIN_EVENT_FILE                         "/dev/vdin0"
#define SYSFS_DISPLAY_VINFO                     "/sys/class/display/vinfo"

#define HDMI_TX_FRAMRATE_POLICY                 "/sys/class/amhdmitx/amhdmitx0/frac_rate_policy"
#define FRAME_RATE_POLIY_COFIG                  "/vendor/etc/framerate.cfg"
#define PANEL_FRAME_RATE                        "/sys/class/lcd/frame_rate"
#define HDMI_FRAME_RATE_AUTO                    "/sys/class/display/fr_policy"
//sysfs of panel framerate change
#define FRAMERAT_PANEL_OUT                      "/sys/class/display/fr_hint"
#define FRAME_RATE_HDMI_OFF                     "0"
#define FRAME_RATE_HDMI_CLK_PULLDOWN            "1"
#define FRAME_RATE_HDMI_SWITCH_FORCE            "2"

#define TVIN_IOC_G_EVENT_INFO                   _IOW('T', 0x0a, struct vdin_event_info)
#define TVIN_IOC_G_SIG_INFO                     _IOR('T', 0x07, struct tvin_info_s)

#define FRAME_RATE_DURATION_2397                4004
#define FRAME_RATE_DURATION_2398                4003
#define FRAME_RATE_DURATION_24                  4000
#define FRAME_RATE_DURATION_25                  3840
#define FRAME_RATE_DURATION_2997                3203
#define FRAME_RATE_DURATION_30                  3200
#define FRAME_RATE_DURATION_50                  1920
#define FRAME_RATE_DURATION_5994                1601
#define FRAME_RATE_DURATION_5992                1602
#define FRAME_RATE_DURATION_60                  1600
typedef void (*fun_t)(bool, bool, const char*);

struct vdin_event_info {
    /*enum tvin_sg_chg_flg*/
    unsigned int event_sts;
};

enum tvin_sg_chg_flg {
    TVIN_SIG_CHG_VS_FRQ = 0x80,
};

typedef enum {
    OUTPUT_TYPE_HDMI_TX            = 0,
    OUTPUT_TYPE_CVBS               = 1,
    OUTPUT_TYPE_LCD_PANEL          = 2,
} output_adapt_type;

typedef enum {
    INPUT_TYPE_UEVENT              = 0,
    INPUT_TYPE_API                 = 1,
} input_adapt_type;

class FrameRateAutoAdaption
{
public:
    class Callbak {
    public:
        Callbak() {};
        virtual ~Callbak() {};

        virtual void onDispModeSyncEvent (const char* outputmode, int state) = 0;
    };

    FrameRateAutoAdaption(Callbak *cb);
    ~FrameRateAutoAdaption();

    int parseConfigFile();
    void inputValidateAndParse(void* data, int inType = 0);
    void outputDispatch(char* outputMode, int outType, int state, int frameRate);
    void policyControl(int frameRateValue);
    void onTxUeventReceived(uevent_data_t* ueventData);
    void readSinkEdid(char *edid);
    int getOutputAdaptType();
    void restoreEnv();
    void setVideoLayerOn(bool on);
    void setPlayFlag(bool play);
    bool isFrameRateOn();
    int getLastFrame();
    int mFracDefaultValue;
private:
    int findNearlyFrame(int frameRate);
    bool frameRateChange(const char* curDisplayMode, const char* newDisplayMode,int frameRateValue,int outType);
    bool currentDisplayIsFloat(int outputType);
    bool frameRateIsFloat(int framerate);
    int mVdinEventFd;
    Callbak *mHdmiCallback;
    void initalDefaultValue();
    int mLastFrameRate;
    bool videoLayerOn;
    //Callbak *mNonHdmiCallback;
    SysWrite mSysWrite;
    char mLastVideoMode[MODE_LEN] = {0};
    std::map<int, std::vector<std::string>> configMap;

    bool mPlayFlag;
    std::vector<double> mFramerateList;
#ifdef FRAMERATE_MODE
    sp<MessageTask> mTask;
#endif
};
#endif // FRAMERATE_ADAPTER_H