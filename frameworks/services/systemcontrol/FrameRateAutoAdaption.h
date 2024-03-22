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
#include <time.h>
#include <string>
#include <sys/ioctl.h>
#ifdef FRAMERATE_MODE
#include "FrameRateMessage.h"
#include "PQ/include/CPQControl.h"
#endif
//Frame rate switch
#define FRAME_RATE_DECODER_UEVENT               "DEVPATH=/devices/virtual/framerate_adapter/framerate_dev"
#define FRAME_RATE_VDIN0_UEVENT                 "DEVTYPE=vdin0event"
#define FRAME_RATE_VDIN1_UEVENT                 "DEVTYPE=vdin1event"
#define FRAME_RATE_VDIN0_UEVENT_N               "DEVNAME=vdin0event"
#define FRAME_RATE_VDIN1_UEVENT_N               "DEVNAME=vdin1event"
#define VDIN_EVENT_FILE                         "/dev/vdin0"
#define SYSFS_DISPLAY_VINFO                     "/sys/class/display/vinfo"
#define SYSFS_DLG_PROP                          "persist.vendor.sys.display.dlg"
#define VENDOR_BOOT_COMPLETE                    "vendor.sys.display.boot_complete"
#define HDMI_TX_FRAMERATE_POLICY                 "/sys/class/amhdmitx/amhdmitx0/frac_rate_policy"
#define FRAME_RATE_POLICY_CONFIG                  "/vendor/etc/framerate.cfg"
#define PANEL_FRAME_RATE                        "/sys/class/lcd/frame_rate"
#define HDMI_FRAME_RATE_AUTO                    "/sys/class/display/fr_policy"
//sysfs of panel framerate change
#define FRAMERAT_PANEL_OUT                      "/sys/class/display/fr_hint"
#define VOUT_DISPLAY_RANGE                      "/sys/class/display/fr_range"
#define VOUT_24P_PROP                           "persist.vendor.sys.framerate.24p"
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
#define FRAME_RATE_DURATION_72                  1333
#define FRAME_RATE_DURATION_144                 666
#define FRAME_RATE_DURATION_125                 7680
#define FRAME_RATE_DURATION_48                  2000
#define FRAME_RATE_DURATION_120                 800
#define FRAME_RATE_DURATION_100                 960
#define FRAME_RATE_DURATION_119                 806
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
    class Callback {
    public:
        Callback() {};
        virtual ~Callback() {};

        virtual void onDispModeSyncEvent (const char* outputmode, int state) = 0;
        virtual void setDisplayModeinner(const char* outputmode) = 0;
        virtual void setActiveModeRemote(int width, int height, int framerate) = 0;
    };

    FrameRateAutoAdaption(Callback *cb);
    ~FrameRateAutoAdaption();

    int parseConfigFile();
    void inputValidateAndParse(void* data, int inType = 0);
    void outputDispatch(char* outputMode, int outType, int state, int frameRate, bool isVdin);
    void policyControl(int frameRateValue);
    void delayControl(int frameRate);
    void onTxUeventReceived(uevent_data_t* ueventData);
    void readSinkEdid(char *edid);
    int getOutputAdaptType();
    void restoreEnv();
    void setVideoLayerOn(bool on);
    bool isFrameRateOn();
    int getLastFrame();
    bool getVideoLayerOn();
    bool enter4k1kByUI(bool on);
    void enter4k1korBack();
#ifdef FRAMERATE_MODE
    void setPQHandle(CPQControl* handle);
#endif
    int mFracDefaultValue;
private:
    int findNearlyFrame(int frameRate);
    bool afrOnly(int frameRate);
    bool freesyncFrame(int frameRate);
    bool frameRateChange(const char* curDisplayMode, const char* newDisplayMode,int frameRateValue,int outType);
    bool currentDisplayIsFloat(int outputType);
    bool frameRateIsFloat(int framerate);
    bool afrInDLG(std::string customStr, int frameValue,bool frameOnly);
    bool enter4k1k(int framerate);
    bool switch144Special(int width, int height, int framerate);
    bool backFrom4k1k(int frameRate);
    int mVdinEventFd;
    Callback *mHdmiCallback;
    void initialDefaultValue();
    int isDLGOn();
    int mLastFrameRate;
    bool mLastFromVdin;
    bool videoLayerOn;
    bool mPictureMode;
    //Callback *mNonHdmiCallback;
    SysWrite mSysWrite;
    char mLastVideoMode[MODE_LEN] = {0};
    std::map<int, std::vector<std::string>> configMap;

    struct timeval mClock;
    std::vector<int> mFramerateList;
#ifdef FRAMERATE_MODE
    CPQControl *pCPQControl = NULL;
    sp<MessageTask> mTask;
#endif
    bool mAFRDisabled{false};
};
#endif // FRAMERATE_ADAPTER_H
