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
 *  @date     2014/10/23
 *  @par function description:
 *  - 1 set display mode
 */

#define LOG_TAG "SystemControl"
//#define LOG_NDEBUG 0
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <linux/netlink.h>
#include <cutils/properties.h>
#include "DisplayMode.h"
#include "SysTokenizer.h"

#include "logo_img_packer/res_pack_i.h"

#ifndef RECOVERY_MODE
#include <binder/IBinder.h>
#include <binder/IServiceManager.h>
#include <binder/Parcel.h>

using namespace android;
#endif
#include "UEventObserver.h"
#include "DisplayModeMgr.h"

#include <DisplayAdapter.h>
using ConnectorType = meson::DisplayAdapter::ConnectorType;

static const char* DISPLAY_MODE_LIST[DISPLAY_MODE_TOTAL] = {
    MODE_480I,
    MODE_480P,
    MODE_480CVBS,
    MODE_576I,
    MODE_576P,
    MODE_576CVBS,
    MODE_720P,
    MODE_720P50HZ,
    MODE_1080P24HZ,
    MODE_1080I50HZ,
    MODE_1080P50HZ,
    MODE_1080I,
    MODE_1080P,
    MODE_4K2K24HZ,
    MODE_4K2K25HZ,
    MODE_4K2K30HZ,
    MODE_4K2K50HZ,
    MODE_4K2K60HZ,
    MODE_4K2KSMPTE,
    MODE_4K2KSMPTE30HZ,
    MODE_4K2KSMPTE50HZ,
    MODE_4K2KSMPTE60HZ,
    MODE_768P,
    MODE_PANEL,
    MODE_PAL_M,
    MODE_PAL_N,
    MODE_NTSC_M,
    MODE_480x320P,
    MODE_640x480P,
    MODE_800x480P,
    MODE_800x600P,
    MODE_1024x600P,
    MODE_1024x768P,
    MODE_1280x480P,
    MODE_1280x800P,
    MODE_1280x1024P,
    MODE_1360x768P,
    MODE_1440x900P,
    MODE_1600x900P,
    MODE_1600x1200P,
    MODE_1680x1050P,
    MODE_1920x1200P,
    MODE_2560x1080P,
    MODE_2560x1440P,
    MODE_2560x1600P,
    MODE_3440x1440P,
};
static const char* MODE_RESOLUTION_FIRST[] = {
    MODE_480I,
    MODE_576I,
    MODE_480P,
    MODE_576P,
    MODE_720P50HZ,
    MODE_720P,
    MODE_1080I50HZ,
    MODE_1080I,
    MODE_1080P50HZ,
    MODE_1080P,
    MODE_4K2K24HZ,
    MODE_4K2K25HZ,
    MODE_4K2K30HZ,
    MODE_4K2K50HZ,
    MODE_4K2K60HZ,
};
static const char* MODE_FRAMERATE_FIRST[] = {
    MODE_480I,
    MODE_576I,
    MODE_480P,
    MODE_576P,
    MODE_720P50HZ,
    MODE_720P,
    MODE_1080I50HZ,
    MODE_1080I,
    MODE_4K2K24HZ,
    MODE_4K2K25HZ,
    MODE_4K2K30HZ,
    MODE_1080P50HZ,
    MODE_1080P,
    MODE_4K2K50HZ,
    MODE_4K2K60HZ,
};

// Sink reference table, sorted by priority, per CDF
static const char* MODES_SINK[] = {
    "2160p60hz",
    "2160p50hz",
    "2160p30hz",
    "2160p25hz",
    "2160p24hz",
    "1080p60hz",
    "1080p50hz",
    "1080p30hz",
    "1080p25hz",
    "1080p24hz",
    "720p60hz",
    "720p50hz",
    "480p60hz",
    "576p50hz",
    "480x320p60hz",
    "640x480p60hz",
    "800x480p60hz",
    "800x600p60hz",
    "1024x600p60hz",
    "1024x768p60hz",
    "1280x480p60hz",
    "1280x800p60hz",
    "1280x1024p60hz",
    "1360x768p60hz",
    "1440x900p60hz",
    "1600x900p60hz",
    "1600x1200p60hz",
    "1680x1050p60hz",
    "1920x1200p60hz",
    "2560x1080p60hz",
    "2560x1440p60hz",
    "2560x1600p60hz",
    "3440x1440p60hz",
};

// Repeater reference table, sorted by priority, per CDF
static const char* MODES_REPEATER[] = {
    "1080p60hz",
    "1080p50hz",
    "1080p30hz",
    "1080p25hz",
    "1080p24hz",
    "720p60hz",
    "720p50hz",
    "480p60hz",
    "576p50hz",
    "480x320p60hz",
    "640x480p60hz",
    "800x480p60hz",
    "800x600p60hz",
    "1024x600p60hz",
    "1024x768p60hz",
    "1280x480p60hz",
    "1280x800p60hz",
    "1280x1024p60hz",
    "1360x768p60hz",
    "1440x900p60hz",
    "1600x900p60hz",
    "1600x1200p60hz",
    "1680x1050p60hz",
    "1920x1200p60hz",
    "2560x1080p60hz",
    "2560x1440p60hz",
    "2560x1600p60hz",
    "3440x1440p60hz",
};

static const char* DV_MODE_TYPE[] = {
    "DV_RGB_444_8BIT",
    "DV_YCbCr_422_12BIT",
    "LL_YCbCr_422_12BIT",
    "LL_RGB_444_12BIT",
    "LL_RGB_444_10BIT"
};

static const char* ALLM_MODE_CAP[] = {
    "0",
    "1",
};

static const char* ALLM_MODE[] = {
    "-1",
    "0",
    "1",
};

static const char* CONTENT_TYPE_CAP[] = {
    "graphics",
    "photo",
    "cinema",
    "game",
};

static const char* CONTENT_TYPE[] = {
    "0",
    "graphics",
    "photo",
    "cinema",
    "game",
};


/**
 * strstr - Find the first substring in a %NUL terminated string
 * @s1: The string to be searched
 * @s2: The string to search for
 */
char *_strstr(const char *s1, const char *s2)
{
    size_t l1, l2;

    l2 = strlen(s2);
    if (!l2)
        return (char *)s1;
    l1 = strlen(s1);
    while (l1 >= l2) {
        l1--;
        if (!memcmp(s1, s2, l2))
            return (char *)s1;
        s1++;
    }
    return NULL;
}

static void copy_if_gt0(uint32_t *src, uint32_t *dst, unsigned cnt)
{
    do {
        if ((int32_t) *src > 0)
            *dst = *src;
        src++;
        dst++;
    } while (--cnt);
}

DisplayMode::DisplayMode(const char *path) {
    DisplayMode(path, NULL);
}

DisplayMode::DisplayMode(const char *path, Ubootenv *ubootenv)
    :mDisplayType(DISPLAY_TYPE_MBOX),
    mEnvLock(PTHREAD_MUTEX_INITIALIZER),
    mDisplayWidth(FULL_WIDTH_1080),
    mDisplayHeight(FULL_HEIGHT_1080),
    mLogLevel(LOG_LEVEL_DEFAULT) {

    if (NULL == path) {
        pConfigPath = DISPLAY_CFG_FILE;
    }
    else {
        pConfigPath = path;
    }

    if (NULL == ubootenv)
        mUbootenv = new Ubootenv();
    else
        mUbootenv = ubootenv;
    pmDeepColor = new FormatColorDepth(mUbootenv);

    SYS_LOGI("display mode config path: %s", pConfigPath);
    pSysWrite = new SysWrite();
    mpSceneProcess = new SceneProcess();

    parseConfigFile();
    parseFilterEdidConfigFile();
    pFrameRateAutoAdaption = new FrameRateAutoAdaption(this);

    SYS_LOGI("type: %d [0:none 1:tablet 2:mbox 3:tv], soc type:%s, default UI:%s",
        mDisplayType, mSocType, mDefaultUI);

    // check dolby vision is support or not
    setDolbyVisionSupport();

    if (DISPLAY_TYPE_MBOX == mDisplayType) {
        pTxAuth = new HDCPTxAuth();
        pUEventObserver = new UEventObserver();
        pUEventObserver->setUevntCallback(this);
        pUEventObserver->setFRAutoAdpt(pFrameRateAutoAdaption);
        pUEventObserver->setHDCPTxAuth(pTxAuth);
        pUEventObserver->start_hdmitxuevent_thread();
        dumpCaps();
    } else if (DISPLAY_TYPE_TV == mDisplayType) {
        setTvModelName();
#ifndef RECOVERY_MODE
        SYS_LOGI("init: not RECOVERY_MODE\n");
        pTxAuth = new HDCPTxAuth();
        pUEventObserver = new UEventObserver();
        pUEventObserver->setUevntCallback(this);
        pUEventObserver->setFRAutoAdpt(pFrameRateAutoAdaption);
        pUEventObserver->setHDCPTxAuth(pTxAuth);
        pUEventObserver->tv_framerateevent_thread();
        pRxAuth = new HDCPRxAuth(pTxAuth);
#endif
    } else if (DISPLAY_TYPE_TABLET == mDisplayType) {
        pTxAuth = new HDCPTxAuth();
        pUEventObserver = new UEventObserver();
        pUEventObserver->setUevntCallback(this);
        pUEventObserver->setFRAutoAdpt(pFrameRateAutoAdaption);
        pUEventObserver->setHDCPTxAuth(pTxAuth);
        pUEventObserver->start_hdmitxuevent_thread();
        dumpCaps();
    } else if (DISPLAY_TYPE_REPEATER == mDisplayType) {
        pTxAuth = new HDCPTxAuth();
        pUEventObserver = new UEventObserver();
        pUEventObserver->setUevntCallback(this);
        pUEventObserver->setFRAutoAdpt(pFrameRateAutoAdaption);
        pUEventObserver->setHDCPTxAuth(pTxAuth);
        pUEventObserver->start_hdmitxuevent_thread();
        pRxAuth = new HDCPRxAuth(pTxAuth);
        dumpCaps();
    }
    resetMemc();
    resetAisr();
    resetAipq();
}

DisplayMode::~DisplayMode() {
    delete pSysWrite;
    delete pmDeepColor;
    delete mpSceneProcess;
}

void DisplayMode::init() {
    /*TODO:tmp solution for DRM MODE., will remove later.*/
    if (mIsRecovery && access("/dev/dri/card0", R_OK | W_OK) == 0) {
        pSysWrite->writeSysfs("/sys/class/amhdmitx/amhdmitx0/attr", "rgb,8bit", strlen("rgb,8bit"));
        return ;
    }

    if (DISPLAY_TYPE_MBOX == mDisplayType) {
        setSourceDisplay(OUPUT_MODE_STATE_INIT);
    } else if (DISPLAY_TYPE_TV == mDisplayType) {
#ifndef RECOVERY_MODE
        setSinkDisplay(true);
#else
        setTvRecoveryDisplay();
#endif
    } else if (DISPLAY_TYPE_TABLET == mDisplayType) {
#ifndef RECOVERY_MODE
        //hwc switch mode for vpp
#else
        setMIDRecoveryDisplay();
#endif
    } else if (DISPLAY_TYPE_REPEATER == mDisplayType) {
        setSourceDisplay(OUPUT_MODE_STATE_INIT);
    }
}

void DisplayMode::reInit() {
    char boot_type[MODE_LEN] = {0};
    /*
     * boot_type would be "normal", "fast", "snapshotted", or "instabooting"
     * "normal": normal boot, the boot_type can not be it here;
     * "fast": fast boot;
     * "snapshotted": this boot contains instaboot image making;
     * "instabooting": doing the instabooting operation, the boot_type can not be it here;
     * for fast boot, need to reinit the display, but for snapshotted, reInit display would make a screen flicker
     */
    pSysWrite->readSysfs(SYSFS_BOOT_TYPE, boot_type);
    if (strcmp(boot_type, "snapshotted")) {
        SYS_LOGI("display mode reinit type: %d [0:none 1:tablet 2:mbox 3:tv], soc type:%s, default UI:%s",
            mDisplayType, mSocType, mDefaultUI);
        if ((DISPLAY_TYPE_MBOX == mDisplayType) || (DISPLAY_TYPE_REPEATER == mDisplayType)) {
            setSourceDisplay(OUPUT_MODE_STATE_POWER);
        } else if (DISPLAY_TYPE_TV == mDisplayType) {
            setSinkDisplay(false);
        }
    }
}

void DisplayMode::setTvModelName() {
    char modelName[MODE_LEN] = {0};
    if (getBootEnv("ubootenv.var.model_name", modelName)) {
        pSysWrite->setProperty("vendor.tv.model_name", modelName);
    }
}

void DisplayMode::setRecoveryMode(bool isRecovery) {
    mIsRecovery = isRecovery;
}

HDCPTxAuth *DisplayMode:: geTxAuth() {
    return pTxAuth;
}

void DisplayMode::setLogLevel(int level){
    mLogLevel = level;
}

bool DisplayMode::getBootEnv(const char* key, char* value) {
    const char* p_value = mUbootenv->getValue(key);

    //if (mLogLevel > LOG_LEVEL_1)
        SYS_LOGI("key:%s value:%s", key, p_value);

    if (p_value) {
        strcpy(value, p_value);
        return true;
    }
    return false;
}

void DisplayMode::setBootEnv(const char* key, const char* value) {
    if (mLogLevel > LOG_LEVEL_1)
        SYS_LOGI("key:%s value:%s", key, value);

    mUbootenv->updateValue(key, value);
}

int DisplayMode::parseConfigFile(){
    const char* WHITESPACE = " \t\r";

    SysTokenizer* tokenizer;
    int status = SysTokenizer::open(pConfigPath, &tokenizer);
    if (status) {
        SYS_LOGE("Error %d opening display config file %s.", status, pConfigPath);
    } else {
        while (!tokenizer->isEof()) {

            if(mLogLevel > LOG_LEVEL_1)
                SYS_LOGI("Parsing %s: %s", tokenizer->getLocation(), tokenizer->peekRemainderOfLine());

            tokenizer->skipDelimiters(WHITESPACE);

            if (!tokenizer->isEol() && tokenizer->peekChar() != '#') {

                char *token = tokenizer->nextToken(WHITESPACE);
                if (!strcmp(token, DEVICE_STR_MBOX)) {
                    mDisplayType = DISPLAY_TYPE_MBOX;

                    tokenizer->skipDelimiters(WHITESPACE);
                    strcpy(mSocType, tokenizer->nextToken(WHITESPACE));
                    tokenizer->skipDelimiters(WHITESPACE);
                    strcpy(mDefaultUI, tokenizer->nextToken(WHITESPACE));
                } else if (!strcmp(token, DEVICE_STR_TV)) {
                    mDisplayType = DISPLAY_TYPE_TV;

                    tokenizer->skipDelimiters(WHITESPACE);
                    strcpy(mSocType, tokenizer->nextToken(WHITESPACE));
                    tokenizer->skipDelimiters(WHITESPACE);
                    strcpy(mDefaultUI, tokenizer->nextToken(WHITESPACE));
                } else if (!strcmp(token, DEVICE_STR_MID)) {
                    mDisplayType = DISPLAY_TYPE_TABLET;

                    tokenizer->skipDelimiters(WHITESPACE);
                    strcpy(mSocType, tokenizer->nextToken(WHITESPACE));
                    tokenizer->skipDelimiters(WHITESPACE);
                    strcpy(mDefaultUI, tokenizer->nextToken(WHITESPACE));
                } else {
                    SYS_LOGE("%s: Expected keyword, got '%s'.", tokenizer->getLocation(), token);
                    break;
                }
            }

            tokenizer->nextLine();
        }
        delete tokenizer;
    }
    //if TVSOC as Mbox, change mDisplayType to DISPLAY_TYPE_REPEATER. and it will be in REPEATER process.
    if ((DISPLAY_TYPE_TV == mDisplayType) && (pSysWrite->getPropertyBoolean(PROP_TVSOC_AS_MBOX, false))) {
        mDisplayType = DISPLAY_TYPE_REPEATER;
    }
    return status;
}

int DisplayMode::parseFilterEdidConfigFile(){
    const char* WHITESPACE = " \t\r";

    SysTokenizer* tokenizer;
    int status = SysTokenizer::open(FILTER_EDID_CFG_FILE, &tokenizer);
    if (status) {
        SYS_LOGE("Error %d opening display config file %s.", status, FILTER_EDID_CFG_FILE);
    } else {
        std::string edid;
        std::map<int, std::string> filterEdidList;
        unsigned int u32EdidCount = 0;

        while (!tokenizer->isEof()) {

            if (mLogLevel > LOG_LEVEL_1)
                SYS_LOGI("Parsing %s: %s", tokenizer->getLocation(), tokenizer->peekRemainderOfLine());

            tokenizer->skipDelimiters(WHITESPACE);
            if (tokenizer->peekChar() == '*') {
                filterEdidList[u32EdidCount++] = edid;
                SYS_LOGI("parseFilterEdidConfigFile EdidCount = %d, edid = %s", u32EdidCount, edid.c_str());

                edid = "";
            }
            if (!tokenizer->isEol() && tokenizer->peekChar() != '*' && tokenizer->peekChar() != '#') {
                char *token = tokenizer->nextToken(WHITESPACE);
                edid += token;
            }
            tokenizer->nextLine();
        }
        delete tokenizer;

        pmDeepColor->setFilterEdidList(filterEdidList);
    }
    return status;
}

void DisplayMode::setMIDRecoveryDisplay() {
    SYS_LOGI("%s\n", __FUNCTION__);

    //1. get osd buffer size
    updateDefaultUI();

    usleep(1000000LL);
    //1.need close fb1, because uboot logo show in fb1 for old soc(g12a)
    pSysWrite->writeSysfs(DISPLAY_FB0_BLANK, "1");

    //2. update free_scale_axis for osd buffer size
    updateFreeScaleAxis();

    //3. update windows axis
    char display_height[MODE_LEN] = {0};
    int axis_height;
    //const char ch = '=';
    //char *ret;

    getBootEnv(UBOOTENV_DISPLAY_HEIGHT, display_height);
    SYS_LOGI("display_height: %s\n", display_height);

    //ret = strchr(display_height, ch);
    //printf("ret: %s\n", ret);
    axis_height = atoi(display_height);

    char display_width[MODE_LEN] = {0};
    int axis_width;
    getBootEnv(UBOOTENV_DISPLAY_WIDTH, display_width);
    SYS_LOGI("display_width: %s\n", display_width);

    //ret = strchr(display_width, ch);
    //printf("ret: %s\n", ret);
    axis_width = atoi(display_width);


    char axis[MAX_STR_LEN] = {0};
    sprintf(axis, "%d %d %d %d",
            0, 0, axis_width - 1, axis_height - 1);
    SYS_LOGI("window axis: %s\n", axis);
    pSysWrite->writeSysfs(DISPLAY_FB0_WINDOW_AXIS, axis);

    //4.enable
    pSysWrite->writeSysfs(DISPLAY_FB0_FREESCALE, "0x10001");

    //4.1 recovery will open fb0
    //pSysWrite->writeSysfs(DISPLAY_FB0_BLANK, "0");
}

void DisplayMode::setTvRecoveryDisplay() {
    SYS_LOGI("setTvRecoveryDisplay\n");
    char outputmode[MODE_LEN] = {0};
    getDisplayMode(outputmode);
    updateDefaultUI();

    usleep(1000000LL);
    pSysWrite->writeSysfs(DISPLAY_FB0_BLANK, "1");
    //1.need close fb1, because uboot logo show in fb1 for old soc(g12a)
    //2.recovery will open fb0
    updateFreeScaleAxis();
    updateWindowAxis(outputmode);
    pSysWrite->writeSysfs(DISPLAY_FB1_BLANK, "1");
    pSysWrite->writeSysfs(DISPLAY_FB1_FREESCALE, "0");
    pSysWrite->writeSysfs(DISPLAY_FB0_FREESCALE, "0x10001");
    //pSysWrite->writeSysfs(DISPLAY_FB0_BLANK, "0");
}

bool DisplayMode::getModeSupportDeepColorAttr(const char* outputmode,const char * color){
    bool ret;
    if (outputmode == NULL || color == NULL) {
        SYS_LOGI("outputmode or color is NULL");
        return false;
    }
    ret = pmDeepColor->isModeSupportDeepColorAttr(outputmode,color);
    return ret;
}

void DisplayMode::sceneProcess(hdmi_data_t* data) {
    char tvmode[MAX_STR_LEN] = {0};

    SYS_LOGI("scene mode:%d\n", data->state);

    //1. read screne input info
    scene_input_info_t scene_input_info;

    //1.1 common input info
    if ((data->state == OUPUT_MODE_STATE_INIT) ||
        (data->state == OUPUT_MODE_STATE_POWER)) {
        strcpy(scene_input_info.cur_displaymode, data->ubootenv_hdmimode);
    } else if (OUPUT_MODE_STATE_SWITCH == data->state) {
        strcpy(scene_input_info.cur_displaymode, data->ui_hdmimode);
    }

    scene_input_info.state          = (scene_state)data->state;
    scene_input_info.isbestpolicy   = isBestOutputmode();
    scene_input_info.isDvEnable     = isDolbyVisionEnable();
    scene_input_info.isTvSupportDv  = isTvSupportDolbyVision(tvmode);
    scene_input_info.isTvSupportHDR = isTvSupportHDR();
    scene_input_info.hdr_policy     = data->hdr_policy;
    scene_input_info.hdr_priority   = data->hdr_priority;

   //1.2 dolby vision input info
    strcpy(scene_input_info.dv_input_info.ubootenv_dv_type, data->dv_info.ubootenv_dv_type);
    strcpy(scene_input_info.dv_input_info.dv_cap, data->dv_info.dv_cap);
    strcpy(scene_input_info.dv_input_info.dv_deepcolor, data->dv_info.dv_deepcolor);
    strcpy(scene_input_info.dv_input_info.dv_displaymode, data->dv_info.dv_displaymode);

    //1.3 hdmi input info
    scene_input_info.hdmi_input_info.sinkType            = data->sinkType;
    scene_input_info.hdmi_input_info.isSupport4K         = isSupport4K();
    scene_input_info.hdmi_input_info.isSupport4K30Hz     = isSupport4K30Hz();
    scene_input_info.hdmi_input_info.isframeratepriority = isFrameratePriority();
    scene_input_info.hdmi_input_info.isDeepColor         = isSupportDeepColor();
    scene_input_info.hdmi_input_info.isLowPowerMode      = isLowPowerMode();

    strcpy(scene_input_info.hdmi_input_info.edidParsing, data->edidParsing);
    strcpy(scene_input_info.hdmi_input_info.disp_cap, data->disp_cap);
    strcpy(scene_input_info.hdmi_input_info.dc_cap, data->dc_cap);
    strcpy(scene_input_info.hdmi_input_info.ubootenv_cvbsmode, data->ubootenv_cvbsmode);

    getBootEnv(UBOOTENV_COLORATTRIBUTE, data->ubootenv_colorattribute);
    strcpy(scene_input_info.hdmi_input_info.ubootenv_colorattribute, data->ubootenv_colorattribute);

    //2 scene process
    mpSceneProcess->UpdateSceneInputInfo(&scene_input_info);
    mpSceneProcess->Process(&mScene_output_info);

    //3 return output final result
    strcpy(data->final_displaymode, mScene_output_info.final_displaymode);
    strcpy(data->final_deepcolor, mScene_output_info.final_deepcolor);
    data->dv_info.dv_type = mScene_output_info.dv_type;

    SYS_LOGI("final_displaymode:%s, final_deepcolor:%s, dv_type:%d\n",
        data->final_displaymode, data->final_deepcolor, data->dv_info.dv_type);
}

/*
* OUPUT_MODE_STATE_INIT for boot
* OUPUT_MODE_STATE_POWER for hdmi plug and suspend/resume
*/
void DisplayMode::setSourceDisplay(output_mode_state state) {
#ifndef RECOVERY_MODE
    AutoMutex _l( mLock );
#endif

    //hdmi used and plugout when boot
    if ((isHdmiUsed() == true) &&
        (isHdmiHpd() == false)) {
        if (isVMXCertification()) {
            setDisplayMode("576cvbs");
        } else {
            setDisplayMode("dummy_l");
        }

        SYS_LOGI("hdmi used but plugout when boot\n");
        return;
    }

    //hdmi edid parse error
    if ((isHdmiEdidParseOK() == false) &&
        (isHdmiHpd() == true)) {
        pSysWrite->writeSysfs(DISPLAY_HDMI_AVMUTE_SYSFS, "1");
        DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_HDMI_COLOR_ATTR, COLOR_RGB_8BIT);
        //set hdmi mode
        setDisplayMode(DEFAULT_OUTPUT_MODE);

        //update display position
        int position[4] = { 0, 0, 0, 0 };//x,y,w,h
        getPosition(DEFAULT_OUTPUT_MODE, position);
        setPosition(DEFAULT_OUTPUT_MODE, position[0], position[1],position[2], position[3]);

        pSysWrite->writeSysfs(DISPLAY_HDMI_AVMUTE_SYSFS, "-1");
        SYS_LOGE("EDID parsing error detected\n");
        return;
    }

    //1. read hdmi info for boot and hdmi plug/suspend/resume
    if ((state == OUPUT_MODE_STATE_INIT) ||
        (state == OUPUT_MODE_STATE_POWER)) {
        memset(&mHdmidata, 0, sizeof(hdmi_data_t));
        mHdmidata.state = state;
        getHdmiData(&mHdmidata);
     }

    //2. scene logic process
    sceneProcess(&mHdmidata);

    if (OUPUT_MODE_STATE_INIT == state) {
        updateDefaultUI();
    }

    //3. setting apply
    applyDisplaySetting(state);
}

/*
* OUPUT_MODE_STATE_SWITCH for UI set
*/
void DisplayMode::setSourceOutputMode(const char* outputmode) {
#ifndef RECOVERY_MODE
    AutoMutex _l( mLock );
#endif

    //1. get hdmi data
    mHdmidata.state = OUPUT_MODE_STATE_SWITCH;
    strcpy(mHdmidata.ui_hdmimode, outputmode);

    if (DISPLAY_TYPE_TABLET == mDisplayType) {
        getHdmiData(&mHdmidata);
    } else {
        getCommonData(&mHdmidata);
    }

    //2. scene logic process
    sceneProcess(&mHdmidata);

    //3. setting apply
    applyDisplaySetting(mHdmidata.state);
}

/*
* apply setting
*/
void DisplayMode::applyDisplaySetting(output_mode_state state) {
    //check cvbs mode
    bool cvbsMode = false;

    if (!strcmp(mHdmidata.final_displaymode, MODE_480CVBS) || !strcmp(mHdmidata.final_displaymode, MODE_576CVBS)
        || !strcmp(mHdmidata.final_displaymode, MODE_PAL_M) || !strcmp(mHdmidata.final_displaymode, MODE_PAL_N)
        || !strcmp(mHdmidata.final_displaymode, MODE_NTSC_M)
        || !strcmp(mHdmidata.final_displaymode, "null") || !strcmp(mHdmidata.final_displaymode, "dummy_l")
        || !strcmp(mHdmidata.final_displaymode, MODE_PANEL)) {
        cvbsMode = true;
    }

    /* not enable phy in systemcontrol by default
     * as phy will be enabled in driver when set mode
     * only enable phy if phy is disabled but not enabled
     */
    bool phy_enabled_already = true;

    // 1. update hdmi frac_rate_policy
    char frac_rate_policy[MODE_LEN]     = {0};
    char cur_frac_rate_policy[MODE_LEN] = {0};
    bool frac_rate_policy_change        = false;

    pSysWrite->readSysfs(HDMI_TX_FRAMRATE_POLICY, cur_frac_rate_policy);
    getBootEnv(UBOOTENV_FRAC_RATE_POLICY, frac_rate_policy);
    if (strstr(frac_rate_policy, cur_frac_rate_policy) == NULL) {
        pSysWrite->writeSysfs(HDMI_TX_FRAMRATE_POLICY, frac_rate_policy);
        frac_rate_policy_change = true;
    }  else {
        SYS_LOGI("cur frac_rate_policy is equals\n");
    }

    // 2. set hdmi final color space
    char curColorAttribute[MODE_LEN] = {0};
    char final_deepcolor[MODE_LEN]   = {0};
    bool attr_change                 = false;

    std::string cur_ColorAttribute;
    DisplayModeMgr::getInstance().getDisplayAttribute(DISPLAY_HDMI_COLOR_ATTR, cur_ColorAttribute);
    strcpy(curColorAttribute, cur_ColorAttribute.c_str());
    strcpy(final_deepcolor, mHdmidata.final_deepcolor);
    SYS_LOGI("curDeepcolor[%s] final_deepcolor[%s]\n", curColorAttribute, final_deepcolor);

    if (strstr(curColorAttribute, final_deepcolor) == NULL) {
        SYS_LOGI("set color space from:%s to %s\n", curColorAttribute, final_deepcolor);
        DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_HDMI_COLOR_ATTR, final_deepcolor);
        attr_change = true;
    } else {
        SYS_LOGI("cur deepcolor is equals\n");
    }

    // 3. update sdr/hdr strategy
    char hdr_policy[MODE_LEN] = {0};
    getHdrStrategy(hdr_policy);
    if (!cvbsMode && (isMboxSupportDolbyVision() == false)) {
        if (pSysWrite->getPropertyBoolean(PROP_DOLBY_VISION_FEATURE, false)) {
            if (strstr(hdr_policy, HDR_POLICY_SINK)) {
                DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_HDR_POLICY, HDR_POLICY_SINK, ConnectorType::CONN_TYPE_HDMI);
            } else if (strstr(hdr_policy, HDR_POLICY_SOURCE)) {
                DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_HDR_POLICY, HDR_POLICY_SOURCE, ConnectorType::CONN_TYPE_HDMI);
            }
        } else {
            initHdrSdrMode();
        }
    }

    // 4. check dolby vision
    int  dv_type    = DOLBY_VISION_SET_DISABLE;
    bool dv_change  = false;

    dv_type   = mHdmidata.dv_info.dv_type;
    dv_change = checkDolbyVisionStatusChanged(dv_type);
    if (isMboxSupportDolbyVision()
        && dv_change) {
        //4.1 set avmute when signal change at boot
        if ((OUPUT_MODE_STATE_INIT == state)
            && (strstr(hdr_policy, HDR_POLICY_SINK))) {
            pSysWrite->writeSysfs(DISPLAY_HDMI_AVMUTE_SYSFS, "1");
        }
        //4.2 set dummy_l mode when dv change at UI switch
        if ((OUPUT_MODE_STATE_SWITCH == state) && dv_change) {
            setDisplayMode("dummy_l");
        }
        //4.3 enable or disable dolby vision core
        if (DOLBY_VISION_SET_DISABLE != dv_type) {
            enableDolbyVision(dv_type);
        } else {
            disableDolbyVision(dv_type);
        }

        SYS_LOGI("isDolbyVisionEnable [%d] dolby vision type:%d", isDolbyVisionEnable(), getDolbyVisionType());
    } else {
        SYS_LOGI("cur DvMode is equals\n");
    }

    // 5. check hdmi output mode
    char final_displaymode[MODE_LEN] = {0};
    char curDisplayMode[MODE_LEN]    = {0};
    bool modeChange                  = false;

    getDisplayMode(curDisplayMode);
    strcpy(final_displaymode, mHdmidata.final_displaymode);
    SYS_LOGI("curMode:[%s] ,final_displaymode[%s]\n", curDisplayMode, final_displaymode);

    if (!isMatchMode(curDisplayMode, final_displaymode)) {
        modeChange = true;
    } else {
        SYS_LOGI("cur mode is equals\n");
    }

    //6. check any change
    bool isNeedChange = false;

    if (modeChange || attr_change || frac_rate_policy_change) {
        isNeedChange = true;
    } else {
        SYS_LOGI("nothing need to be changed\n");
    }

    // 7. stop hdcp
    if (isNeedChange) {
        pSysWrite->writeSysfs(DISPLAY_HDMI_AVMUTE_SYSFS, "1");
        if (OUPUT_MODE_STATE_POWER != state) {
            usleep(100000);//100ms
            pSysWrite->writeSysfs(DISPLAY_HDMI_HDCP_MODE, "-1");
            //usleep(100000);//100ms
            pSysWrite->writeSysfs(DISPLAY_HDMI_PHY, "0"); /* Turn off TMDS PHY */
            phy_enabled_already = false;
            usleep(50000);//50ms
        }
        // stop hdcp tx
        pTxAuth->stop();
    } else if (OUPUT_MODE_STATE_INIT == state) {
        // stop hdcp tx
        pTxAuth->stop();
    }

    // 8. set hdmi final output mode
    if (isNeedChange) {
        //set hdmi mode
        setDisplayMode(final_displaymode);
        /* phy already turned on after write display/mode node */
        phy_enabled_already     = true;
    } else {
        SYS_LOGI("curDisplayMode is equal  final_displaymode, Do not need set it\n");
    }

    // graphic
    char final_Mode[MODE_LEN] = {0};
    getDisplayMode(final_Mode);
    if (pSysWrite->getPropertyBoolean(PROP_DISPLAY_SIZE_CHECK, true)) {
        char resolution[MODE_LEN] = {0};
        char defaultResolution[MODE_LEN] = {0};
        char finalResolution[MODE_LEN] = {0};
        int w = 0, h = 0, w1 =0, h1 = 0;
        pSysWrite->readSysfs(SYS_DISPLAY_RESOLUTION, resolution);
        pSysWrite->getPropertyString(PROP_DISPLAY_SIZE, defaultResolution, "0x0");
        sscanf(resolution, "%dx%d", &w, &h);
        sscanf(defaultResolution, "%dx%d", &w1, &h1);
        if ((w != w1) || (h != h1)) {
            if (strstr(final_Mode, "null") && w1 != 0) {
                sprintf(finalResolution, "%dx%d", w1, h1);
            } else {
                sprintf(finalResolution, "%dx%d", w, h);
            }
            pSysWrite->setProperty(PROP_DISPLAY_SIZE, finalResolution);
        }
    }
    pSysWrite->setProperty(PROP_DISPLAY_ALLM, isTvSupportALLM() ? "1" : "0");
    pSysWrite->setProperty(PROP_DISPLAY_GAME, getGameContentTypeSupport() ? "1" : "0");

    char defaultResolution[MODE_LEN] = {0};
    pSysWrite->getPropertyString(PROP_DISPLAY_SIZE, defaultResolution, "0x0");
    SYS_LOGI("set display-size:%s\n", defaultResolution);

    int position[4] = { 0, 0, 0, 0 };//x,y,w,h
    getPosition(final_Mode, position);
    setPosition(final_Mode, position[0], position[1],position[2], position[3]);

    // no need to update
    // update free_scale_axis and window_axis in recovery mode
#ifdef RECOVERY_MODE
    updateFreeScaleAxis();
    updateWindowAxis(final_displaymode);
#endif

    // 9. turn on phy and clear avmute
    if (isNeedChange) {
        if (!phy_enabled_already) {
            pSysWrite->writeSysfs(DISPLAY_HDMI_PHY, "1"); /* Turn on TMDS PHY */
        }
        usleep(20000);
        pSysWrite->writeSysfs(DISPLAY_HDMI_AUDIO_MUTE, "1");
        pSysWrite->writeSysfs(DISPLAY_HDMI_AUDIO_MUTE, "0");
        if (isDolbyVisionEnable()) {
            usleep(20000);
        }
    }

    //must clear avmute for new policy(driver maybe set mute)
    pSysWrite->writeSysfs(DISPLAY_HDMI_AVMUTE_SYSFS, "-1");

    // 10. start HDMI HDCP authenticate
    if (isNeedChange) {
        if (!cvbsMode) {
            pTxAuth->start();
        }
    } else if (OUPUT_MODE_STATE_INIT == state) {
        if (!cvbsMode) {
            pTxAuth->start();
        }
    }

    if (OUPUT_MODE_STATE_INIT == state) {
#ifdef RECOVERY_MODE
        startBootanimDetectThread();
#endif
        if (!cvbsMode) {
            pSysWrite->readSysfs(DISPLAY_EDID_RAW, mEdid);
        }
    }
#ifndef RECOVERY_MODE
    notifyEvent(EVENT_OUTPUT_MODE_CHANGE);
#endif

    //audio
    char value[MAX_STR_LEN] = {0};
    memset(value, 0, sizeof(0));
    getBootEnv(UBOOTENV_DIGITAUDIO, value);
    setDigitalMode(value);

#ifndef RECOVERY_MODE
    saveHdmiParamToEnv();
#endif
}

void DisplayMode::setSourceOutputMode(const char* outputmode, output_mode_state state) {
    char value[MAX_STR_LEN] = {0};

    bool cvbsMode = false;
    char tmpMode[MODE_LEN] = {0};

    bool deepColorEnabled = pSysWrite->getPropertyBoolean(PROP_DEEPCOLOR, true);
    pSysWrite->readSysfs(HDMI_TX_FRAMRATE_POLICY, value);
    char curDisplayMode[MODE_LEN] = {0};
    getDisplayMode(curDisplayMode);

    if ((OUPUT_MODE_STATE_SWITCH == state) && (strcmp(value, "0") == 0)) {
        if (!strcmp(outputmode, curDisplayMode)) {
            //if cur mode is cvbsmode, and same to outputmode, return.
            if (!strcmp(outputmode, MODE_480CVBS) || !strcmp(outputmode, MODE_576CVBS)
                || !strcmp(outputmode, MODE_PANEL)
                || !strcmp(outputmode, MODE_PAL_M) || !strcmp(outputmode, MODE_PAL_N) || !strcmp(outputmode, MODE_NTSC_M)
                || !strcmp(outputmode, "null")) {
                return;
            }
            //deep color disabled, only need check output mode same or not
            if (!deepColorEnabled) {
                SYS_LOGI("deep color is Disabled, and curDisplayMode is same to outputmode, return\n");
                return;
            }

            //deep color enabled, check the deep color same or not
            std::string curColorAttribute;
            char saveColorAttribute[MODE_LEN] = {0};
            DisplayModeMgr::getInstance().getDisplayAttribute(DISPLAY_HDMI_COLOR_ATTR, curColorAttribute);
            getBootEnv(UBOOTENV_COLORATTRIBUTE, saveColorAttribute);
            //if bestOutputmode is enable, need change deepcolor to best deepcolor.
            if (isBestOutputmode()) {
                pmDeepColor->getBestHdmiDeepColorAttr(outputmode, saveColorAttribute);
            }
            SYS_LOGI("curColorAttribute:[%s] ,saveColorAttribute: [%s]\n", curColorAttribute.c_str(), saveColorAttribute);
            if (NULL != strstr(curColorAttribute.c_str(), saveColorAttribute))
                return;
        }
    }
    // 1.set avmute and close phy
    if (OUPUT_MODE_STATE_INIT != state) {
        pSysWrite->writeSysfs(DISPLAY_HDMI_AVMUTE_SYSFS, "1");
        if (OUPUT_MODE_STATE_POWER != state) {
            usleep(50000);//50ms
            pSysWrite->writeSysfs(DISPLAY_HDMI_HDCP_MODE, "-1");
            //usleep(100000);//100ms
            pSysWrite->writeSysfs(DISPLAY_HDMI_PHY, "0"); /* Turn off TMDS PHY */
            usleep(50000);//50ms
        }
    }

    // 2.stop hdcp tx
    pTxAuth->stop();

    if (!strcmp(outputmode, MODE_480CVBS) || !strcmp(outputmode, MODE_576CVBS)
        || !strcmp(outputmode, MODE_PAL_M) || !strcmp(outputmode, MODE_PAL_N)
        || !strcmp(outputmode, MODE_NTSC_M)
        || !strcmp(outputmode, "null") || !strcmp(outputmode, "dummy_l")
        || !strcmp(outputmode, MODE_PANEL)) {
        cvbsMode = true;
    }
    // 3. set deep color and outputmode
    updateDeepColor(cvbsMode, state, outputmode);
    char curMode[MODE_LEN] = {0};
    getDisplayMode(curMode);

    SYS_LOGI("curMode = %s outputmode = %s", curMode, outputmode);
    if (strstr(curMode, outputmode) == NULL) {
        setDisplayMode(outputmode);
    }

    char finalMode[MODE_LEN] = {0};
    getDisplayMode(finalMode);
    if (pSysWrite->getPropertyBoolean(PROP_DISPLAY_SIZE_CHECK, true)) {
        char resolution[MODE_LEN] = {0};
        char defaultResolution[MODE_LEN] = {0};
        char finalResolution[MODE_LEN] = {0};
        int w = 0, h = 0, w1 =0, h1 = 0;
        pSysWrite->readSysfs(SYS_DISPLAY_RESOLUTION, resolution);
        pSysWrite->getPropertyString(PROP_DISPLAY_SIZE, defaultResolution, "0x0");
        sscanf(resolution, "%dx%d", &w, &h);
        sscanf(defaultResolution, "%dx%d", &w1, &h1);
        if ((w != w1) || (h != h1)) {
            if (strstr(finalMode, "null") && w1 != 0) {
                sprintf(finalResolution, "%dx%d", w1, h1);
            } else {
                sprintf(finalResolution, "%dx%d", w, h);
            }
            pSysWrite->setProperty(PROP_DISPLAY_SIZE, finalResolution);
        }
    }
    pSysWrite->setProperty(PROP_DISPLAY_ALLM, isTvSupportALLM() ? "1" : "0");
    pSysWrite->setProperty(PROP_DISPLAY_GAME, getGameContentTypeSupport() ? "1" : "0");

    char defaultResolution[MODE_LEN] = {0};
    pSysWrite->getPropertyString(PROP_DISPLAY_SIZE, defaultResolution, "0x0");
    SYS_LOGI("set display-size:%s\n", defaultResolution);

    int position[4] = { 0, 0, 0, 0 };//x,y,w,h
    getPosition(outputmode, position);
    setPosition(outputmode, position[0], position[1],position[2], position[3]);

    // no need to update
    // update free_scale_axis and window_axis in recovery mode
#ifdef RECOVERY_MODE
    updateFreeScaleAxis();
    updateWindowAxis(outputmode);
#endif

    if (!cvbsMode && (pSysWrite->getPropertyBoolean(PROP_SUPPORT_DOLBY_VISION, false) == false)) {
        if (pSysWrite->getPropertyBoolean(PROP_DOLBY_VISION_FEATURE, false)) {
            char hdr_policy[MODE_LEN] = {0};
            getHdrStrategy(hdr_policy);
            if (strstr(hdr_policy, HDR_POLICY_SINK)) {
                DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_HDR_POLICY, HDR_POLICY_SINK, ConnectorType::CONN_TYPE_HDMI);
            } else if (strstr(hdr_policy, HDR_POLICY_SOURCE)) {
                DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_HDR_POLICY, HDR_POLICY_SOURCE, ConnectorType::CONN_TYPE_HDMI);
            }
        } else {
            initHdrSdrMode();
        }
    }

    SYS_LOGI("setMboxOutputMode cvbsMode = %d\n", cvbsMode);
    //4. turn on phy and clear avmute
    if (OUPUT_MODE_STATE_INIT != state && !cvbsMode) {
        pSysWrite->writeSysfs(DISPLAY_HDMI_PHY, "1"); /* Turn on TMDS PHY */
        usleep(20000);
        pSysWrite->writeSysfs(DISPLAY_HDMI_AUDIO_MUTE, "1");
        pSysWrite->writeSysfs(DISPLAY_HDMI_AUDIO_MUTE, "0");
        if ((state == OUPUT_MODE_STATE_SWITCH) && isDolbyVisionEnable())
            usleep(20000);
        pSysWrite->writeSysfs(DISPLAY_HDMI_AVMUTE_SYSFS, "-1");
    }

    //5. start HDMI HDCP authenticate
    if (!cvbsMode) {
        pTxAuth->start();
        //pTxAuth->setBootAnimFinished(true);
    }

    if (OUPUT_MODE_STATE_INIT == state) {
#ifdef RECOVERY_MODE
        startBootanimDetectThread();
#endif
        if (!cvbsMode) {
            pSysWrite->readSysfs(DISPLAY_EDID_RAW, mEdid);
        }
    }
#ifndef RECOVERY_MODE
    notifyEvent(EVENT_OUTPUT_MODE_CHANGE);
#endif

    if (!cvbsMode && pSysWrite->getPropertyBoolean(PROP_SUPPORT_DOLBY_VISION, false)
            && setDolbyVisionState) {
        initDolbyVision(state);
    }
    //audio
    memset(value, 0, sizeof(0));
    getBootEnv(UBOOTENV_DIGITAUDIO, value);
    setDigitalMode(value);

    if (DISPLAY_TYPE_TABLET != mDisplayType) {
        setBootEnv(UBOOTENV_OUTPUTMODE, (char *)finalMode);
    }
    if (strstr(finalMode, "cvbs") != NULL) {
        setBootEnv(UBOOTENV_CVBSMODE, (char *)finalMode);
    } else if (strstr(finalMode, "hz") != NULL) {
        setBootEnv(UBOOTENV_HDMIMODE, (char *)finalMode);
    }

    SYS_LOGI("set output mode:%s done\n", finalMode);
}

void DisplayMode::setDigitalMode(const char* mode) {
    if (mode == NULL) return;

    if (!strcmp("PCM", mode)) {
        pSysWrite->writeSysfs(AUDIO_DSP_DIGITAL_RAW, "0");
        pSysWrite->writeSysfs(AV_HDMI_CONFIG, "audio_on");
    } else if (!strcmp("SPDIF passthrough", mode))  {
        pSysWrite->writeSysfs(AUDIO_DSP_DIGITAL_RAW, "1");
        pSysWrite->writeSysfs(AV_HDMI_CONFIG, "audio_on");
    } else if (!strcmp("HDMI passthrough", mode)) {
        pSysWrite->writeSysfs(AUDIO_DSP_DIGITAL_RAW, "2");
        pSysWrite->writeSysfs(AV_HDMI_CONFIG, "audio_on");
    }
}

int DisplayMode::readHdcpRX22Key(char *value, int size) {
    SYS_LOGI("read HDCP rx 2.2 key \n");
    HDCPRxKey key22(HDCP_RX_22_KEY);
    int ret = key22.getHdcpRX22key(value, size);
    return ret;
}

bool DisplayMode::writeHdcpRX22Key(const char *value, const int size) {
    SYS_LOGI("write HDCP rx 2.2 key \n");
    HDCPRxKey key22(HDCP_RX_22_KEY);
    int ret = key22.setHdcpRX22key(value, size);
    if (ret == 0)
        return true;
    else
        return false;
}

int DisplayMode::readHdcpRX14Key(char *value, int size) {
    SYS_LOGI("read HDCP rx 1.4 key \n");
    HDCPRxKey key14(HDCP_RX_14_KEY);
    int ret = key14.getHdcpRX14key(value, size);
    return ret;
}

bool DisplayMode::writeHdcpRX14Key(const char *value, const int size) {
    SYS_LOGI("write HDCP rx 1.4 key \n");
    HDCPRxKey key14(HDCP_RX_14_KEY);
    int ret = key14.setHdcpRX14key(value,size);
    if (ret == 0)
        return true;
    else
        return false;
}

bool DisplayMode::writeHdcpRXImg(const char *path) {
    SYS_LOGI("write HDCP key from Img \n");
    int ret = setImgPath(path);
    if (ret == 0)
        return true;
    else
        return false;
}

bool DisplayMode::updataLogoBmp(const char *path) {
    int ret = -1;
    SYS_LOGI("path: %s\n", path);
    ret = res_img_pack_bmp(path);
    if (ret == 0) {
        mUbootenv->updateValue("ubootenv.var.board_defined_bootup", "test_logo");
        return true;
    } else
        return false;
}


//get the best hdmi mode by edid
void DisplayMode::getBestHdmiMode(char* mode, hdmi_data_t* data) {
    char* pos = strchr(data->disp_cap, '*');
    if (pos != NULL) {
        char* findReturn = pos;
        while (*findReturn != 0x0a && findReturn >= data->disp_cap) {
            findReturn--;
        }

        findReturn = findReturn + 1;
        strncpy(mode, findReturn, pos - findReturn);
        SYS_LOGI("set HDMI to best edid mode: %s\n", mode);
    }

    if (strlen(mode) == 0) {
        pSysWrite->getPropertyString(PROP_BEST_OUTPUT_MODE, mode, DEFAULT_OUTPUT_MODE);
    }
}

//get the highest hdmi mode by edid
void DisplayMode::getHighestHdmiMode(char* mode, hdmi_data_t* data) {
    char value[MODE_LEN] = {0};
    char tempMode[MODE_LEN] = {0};

    char* startpos;
    char* destpos;
    int resolution_num = 0;
    int index = 0;

    startpos = data->disp_cap;
    strcpy(value, DEFAULT_OUTPUT_MODE);

    while (strlen(startpos) > 0) {
        //get edid resolution to tempMode in order.
        destpos = strstr(startpos, "\n");
        if (NULL == destpos)
            break;
        memset(tempMode, 0, MODE_LEN);
        strncpy(tempMode, startpos, destpos - startpos);
        startpos = destpos + 1;
        resolution_num ++;
        if (!pSysWrite->getPropertyBoolean(PROP_SUPPORT_4K, true)
            &&(strstr(tempMode, "2160") || strstr(tempMode, "smpte"))) {
            SYS_LOGE("This platform not support : %s\n", tempMode);
            continue;
        }

        if (tempMode[strlen(tempMode) - 1] == '*') {
            tempMode[strlen(tempMode) - 1] = '\0';
        }

        if (resolveResolutionValue(tempMode, FRAMERATE_PRIORITY) > resolveResolutionValue(value, FRAMERATE_PRIORITY)) {
            memset(value, 0, MODE_LEN);
            strcpy(value, tempMode);
        }
    }

    strcpy(mode, value);
    strcpy(mode, value);
    if(resolution_num == 1) {
    index = modeToIndex(tempMode);
    switch(index)
    {
        case DISPLAY_MODE_480x320P:
        case DISPLAY_MODE_640x480P:
        case DISPLAY_MODE_800x480P:
        case DISPLAY_MODE_800x600P:
        case DISPLAY_MODE_1024x600P:
        case DISPLAY_MODE_1024x768P:
        case DISPLAY_MODE_1280x480P:
        case DISPLAY_MODE_1280x800P:
        case DISPLAY_MODE_1280x1024P:
        case DISPLAY_MODE_1360x768P:
        case DISPLAY_MODE_1440x900P:
        case DISPLAY_MODE_1600x900P:
        case DISPLAY_MODE_1600x1200P:
        case DISPLAY_MODE_1680x1050P:
        case DISPLAY_MODE_1920x1200P:
        case DISPLAY_MODE_2560x1080P:
        case DISPLAY_MODE_2560x1440P:
        case DISPLAY_MODE_2560x1600P:
        case DISPLAY_MODE_3440x1440P:
        strcpy(mode, tempMode);
        SYS_LOGI("set single HDMI mode to highest edid mode: %s\n", mode);
        break;
        default:
        strcpy(mode, value);
        break;
    }
    }
    SYS_LOGI("set HDMI to highest edid mode: %s\n", mode);
}

int64_t DisplayMode::resolveResolutionValue(const char *mode) {
    return resolveResolutionValue(mode, RESOLUTION_PRIORITY);
}

int64_t DisplayMode::resolveResolutionValue(const char *mode, int flag) {
    bool validMode = false;
    if (strlen(mode) != 0) {
        for (int i = 0; i < DISPLAY_MODE_TOTAL; i++) {
            if (strcmp(mode, DISPLAY_MODE_LIST[i]) == 0) {
                validMode = true;
                break;
            }
        }
    }
    if (!validMode) {
        SYS_LOGI("the resolveResolution mode [%s] is not valid\n", mode);
        return -1;
    }

    if (pSysWrite->getPropertyBoolean(PROP_HDMI_FRAMERATE_PRIORITY, true) && flag == FRAMERATE_PRIORITY) {
        for (int64_t index = 0; index < sizeof(MODE_FRAMERATE_FIRST)/sizeof(char *); index++) {
            if (strcmp(mode, MODE_FRAMERATE_FIRST[index]) == 0) {
                return index;
            }
        }
    } else {
        for (int64_t index = 0; index < sizeof(MODE_RESOLUTION_FIRST)/sizeof(char *); index++) {
            if (strcmp(mode, MODE_RESOLUTION_FIRST[index]) == 0) {
                return index;
            }
        }
    }
    return -1;
}

//get the highest priority mode defined by CDF table
void DisplayMode::getHighestPriorityMode(char* mode, hdmi_data_t* data) {
    char **pMode = NULL;
    int modeSize = 0;

    if (HDMI_SINK_TYPE_SINK == data->sinkType) {
        pMode= (char **)MODES_SINK;
        modeSize = ARRAY_SIZE(MODES_SINK);
    }
    else if (HDMI_SINK_TYPE_REPEATER == data->sinkType) {
        pMode= (char **)MODES_REPEATER;
        modeSize = ARRAY_SIZE(MODES_REPEATER);
    }

    for (int i = 0; i < modeSize; i++) {
        if (strstr(data->disp_cap, pMode[i]) != NULL) {
            strcpy(mode, pMode[i]);
            return;
        }
    }

    pSysWrite->getPropertyString(PROP_BEST_OUTPUT_MODE, mode, DEFAULT_OUTPUT_MODE);
}

bool DisplayMode::isMatchMode(char* curmode, const char* outputmode) {
    bool ret = false;
    char tmpMode[MODE_LEN] = {0};

    char *pCmp = curmode;
    //check line feed key
    char *pos = strchr(pCmp, 0x0a);
    if (NULL == pos) {
        //check return key
        char *pos = strchr(pCmp, 0x0d);
        if (NULL == pos) {
            strcpy(tmpMode, pCmp);
        } else {
            strncpy(tmpMode, pCmp, pos - pCmp);
        }
    } else {
        strncpy(tmpMode, pCmp, pos - pCmp);
    }

    SYS_LOGI("curmode:%s, tmpMode:%s, outputmode:%s\n", curmode, tmpMode, outputmode);

    if (!strcmp(tmpMode, outputmode)) {
        ret = true;
    }

    return ret;
}

//check if the edid support current hdmi mode
void DisplayMode::filterHdmiMode(char* mode, hdmi_data_t* data) {
    char *pCmp = data->disp_cap;
    while ((pCmp - data->disp_cap) < (int)strlen(data->disp_cap)) {
        char *pos = strchr(pCmp, 0x0a);
        if (NULL == pos)
            break;

        int step = 1;
        if (*(pos - 1) == '*') {
            pos -= 1;
            step += 1;
        }
        if (!strncmp(pCmp, data->ubootenv_hdmimode, pos - pCmp)) {
            strncpy(mode, pCmp, pos - pCmp);
            return;
        }
        pCmp = pos + step;
    }

    if (DISPLAY_TYPE_TV == mDisplayType) {
        #ifdef TEST_UBOOT_MODE
            getBootEnv(UBOOTENV_TESTMODE, mode);
            if (strlen(mode) != 0)
               return;
        #endif
    }

    //old mode is not support in this TV, so switch to best mode.
#ifdef USE_BEST_MODE
    getBestHdmiMode(mode, data);
#else
    getHighestHdmiMode(mode, data);
#endif
}

void DisplayMode::getHdmiOutputMode(char* mode, hdmi_data_t* data) {
    char edidParsing[MODE_LEN] = {0};
    pSysWrite->readSysfs(DISPLAY_EDID_STATUS, edidParsing);

    /* Fall back to 480p if EDID can't be parsed */
    if (strcmp(edidParsing, "ok")) {
        strcpy(mode, DEFAULT_OUTPUT_MODE);
        SYS_LOGE("EDID parsing error detected\n");
        return;
    }

    if (pSysWrite->getPropertyBoolean(PROP_HDMIONLY, true)) {
        if (isBestOutputmode()) {
        #ifdef USE_BEST_MODE
            getBestHdmiMode(mode, data);
        #else
            getHighestHdmiMode(mode, data);
            //getHighestPriorityMode(mode, data);
        #endif
        } else {
            filterHdmiMode(mode, data);
        }
    }
    SYS_LOGI("set HDMI mode to %s\n", mode);
}

void DisplayMode::filterHdmiDispcap(hdmi_data_t* data) {
    const char *delim = "\n";
    char filter_dispcap[MAX_STR_LEN] = {0};
    char supportedColorList[MAX_STR_LEN];

    if (!(pmDeepColor->initColorAttribute(supportedColorList, MAX_STR_LEN))) {
        SYS_LOGE("initColorAttribute fail\n");
        return;
    }

    SYS_LOGI("before filtered HdmiDispcap: %s\n", data->disp_cap);

    char *hdmi_mode = strtok(data->disp_cap, delim);
    while (hdmi_mode != NULL) {
        //recommend mode or not
        bool recomMode = false;
        int   len = strlen(hdmi_mode);
        if (hdmi_mode[len - 1] == '*') {
            hdmi_mode[len - 1] = '\0';
            recomMode = true;
        }

        if (pmDeepColor->isSupportHdmiMode(hdmi_mode, supportedColorList)) {
            if ((strlen(filter_dispcap) + strlen(hdmi_mode)) < (MAX_STR_LEN-1)) {
                strcat(filter_dispcap, hdmi_mode);
                if (recomMode)
                    strcat(filter_dispcap, "*");
                    strcat(filter_dispcap, delim);
            } else {
                SYS_LOGE("DisplayMode strcat overflow: src=%s, dst=%s\n", hdmi_mode, filter_dispcap);
                break;
            }
        }

        hdmi_mode = strtok(NULL, delim);
    }

    strcpy(data->disp_cap, filter_dispcap);

    SYS_LOGI("after filtered HdmiDispcap: %s\n", data->disp_cap);
}

bool DisplayMode::isHdmiEdidParseOK(void) {
    bool ret = true;

    char edidParsing[MODE_LEN] = {0};
    pSysWrite->readSysfs(DISPLAY_EDID_STATUS, edidParsing);

    if (strcmp(edidParsing, "ok")) {
        ret = false;
    }

    return ret;
}

bool DisplayMode::isHdmiHpd(void) {
    bool ret = true;
    char hpd_state[MODE_LEN] = {0};
    pSysWrite->readSysfs(DISPLAY_HPD_STATE, hpd_state);

    if (strstr(hpd_state, "1") == NULL) {
        ret = false;
    }

    return ret;
}

bool DisplayMode::isHdmiUsed(void) {
    bool ret = true;
    char hdmi_state[MODE_LEN] = {0};
    pSysWrite->readSysfs(DISPLAY_HDMI_USED, hdmi_state);

    if (strstr(hdmi_state, "1") == NULL) {
        ret = false;
    }

    return ret;
}

bool DisplayMode::isVMXCertification() {
    return pSysWrite->getPropertyBoolean(PROP_VMX, false);
}

void DisplayMode::getCommonData(hdmi_data_t* data) {
    if (!data) {
        SYS_LOGE("%s data is NULL\n", __FUNCTION__);
        return;
    }

    char hdr_policy[MODE_LEN] = {0};
    getHdrStrategy(hdr_policy);
    data->hdr_policy = (hdr_policy_e)atoi(hdr_policy);

    data->hdr_priority = (hdr_priority_e)getHdrPriority();

    SYS_LOGI("hdr_policy:%d, hdr_priority :%d\n",
            data->hdr_policy,
            data->hdr_priority);

    getDisplayMode(data->hdmi_current_mode);
    getBootEnv(UBOOTENV_HDMIMODE, data->ubootenv_hdmimode);
    getBootEnv(UBOOTENV_CVBSMODE, data->ubootenv_cvbsmode);
    SYS_LOGI("hdmi_current_mode:%s, ubootenv hdmimode:%s cvbsmode:%s\n",
            data->hdmi_current_mode,
            data->ubootenv_hdmimode,
            data->ubootenv_cvbsmode);

    std::string curColorAttribute;
    DisplayModeMgr::getInstance().getDisplayAttribute(DISPLAY_HDMI_COLOR_ATTR, curColorAttribute);
    strcpy(data->hdmi_current_attr, curColorAttribute.c_str());
    getBootEnv(UBOOTENV_COLORATTRIBUTE, data->ubootenv_colorattribute);
    SYS_LOGI("hdmi_current_attr:%s, ubootenv_colorattribute:%s\n",
            data->hdmi_current_attr,
            data->ubootenv_colorattribute);

    //if no dolby_status env set to std for enable dolby vision
    //if box support dolby vision
    bool ret;
    char dv_enable[MODE_LEN];
    ret = getBootEnv(UBOOTENV_DV_ENABLE, dv_enable);
    if (ret) {
        strcpy(data->dv_info.dv_enable, dv_enable);
    } else if (isMboxSupportDolbyVision()) {
        strcpy(data->dv_info.dv_enable, "1");
    } else {
        strcpy(data->dv_info.dv_enable, "0");
    }
    SYS_LOGI("dv_enable:%s\n", data->dv_info.dv_enable);

    char ubootenv_dv_type[MODE_LEN];
    ret = getBootEnv(UBOOTENV_DV_TYPE, ubootenv_dv_type);
    if (ret) {
        strcpy(data->dv_info.ubootenv_dv_type, ubootenv_dv_type);
    } else if (isMboxSupportDolbyVision()) {
        strcpy(data->dv_info.ubootenv_dv_type, "1");
    } else {
        strcpy(data->dv_info.ubootenv_dv_type, "0");
    }
    SYS_LOGI("ubootenv_dv_type:%s\n", data->dv_info.ubootenv_dv_type);

}

void DisplayMode::getHdmiDvCap(hdmi_data_t* data) {
    if (!data) {
        SYS_LOGE("%s data is NULL\n", __FUNCTION__);
        return;
    }

    std::string dv_cap;
    DisplayModeMgr::getInstance().getDisplayAttribute(DISPLAY_DOLBY_VISION_CAP, dv_cap, ConnectorType::CONN_TYPE_HDMI);
    strcpy(data->dv_info.dv_cap, dv_cap.c_str());

    if (strstr(data->dv_info.dv_cap, "DolbyVision RX support list") != NULL) {
        for (int i = DISPLAY_MODE_TOTAL - 1; i >= 0; i--) {
            if (strstr(data->dv_info.dv_cap, DISPLAY_MODE_LIST[i]) != NULL) {
                if ((strlen(data->dv_info.dv_displaymode) + strlen(DISPLAY_MODE_LIST[i])) < sizeof(data->dv_info.dv_displaymode)) {
                    strcat(data->dv_info.dv_displaymode, DISPLAY_MODE_LIST[i]);
                    strcat(data->dv_info.dv_displaymode, ",");
                } else {
                    SYS_LOGE("DisplayMode strcat overflow: src=%s, dst=%s\n", DISPLAY_MODE_LIST[i], data->dv_info.dv_displaymode);
                }
                break;
            }
        }

        for (int i = 0; i < sizeof(DV_MODE_TYPE)/sizeof(DV_MODE_TYPE[0]); i++) {
            if (strstr(data->dv_info.dv_cap, DV_MODE_TYPE[i])) {
                if ((strlen(data->dv_info.dv_deepcolor) + strlen(DV_MODE_TYPE[i])) < sizeof(data->dv_info.dv_deepcolor)) {
                    strcat(data->dv_info.dv_deepcolor, DV_MODE_TYPE[i]);
                    strcat(data->dv_info.dv_deepcolor, ",");
                } else {
                    SYS_LOGE("DisplayMode strcat overflow: src=%s, dst=%s\n", DV_MODE_TYPE[i], data->dv_info.dv_deepcolor);
                    break;
                }
            }
        }
        SYS_LOGI("TV dv info: mode:%s deepcolor: %s\n", data->dv_info.dv_displaymode, data->dv_info.dv_deepcolor);
    } else {
        SYS_LOGE("TV isn't support dolby vision: %s\n", data->dv_info.dv_cap);
    }
}

void DisplayMode::getHdmiDispCap(char* disp_cap) {
    if (!disp_cap) {
        SYS_LOGE("%s disp_cap is NULL\n", __FUNCTION__);
        return;
    }

    int count = 0;
    while (true) {
        pSysWrite->readSysfsOriginal(DISPLAY_HDMI_DISP_CAP, disp_cap);
        if (strlen(disp_cap) > 0)
            break;

        if (count >= 5) {
            strcpy(disp_cap, "null edid");
            break;
        }
        count++;
        usleep(500000);
    }
}

void DisplayMode::getHdmiVesaEdid(char* vesa_edid) {
    if (!vesa_edid) {
        SYS_LOGE("%s vesa_edid is NULL\n", __FUNCTION__);
        return;
    }

    int count = 0;
    while (true) {
        pSysWrite->readSysfsOriginal(DISPLAY_HDMI_DISP_CAP_VESA, vesa_edid);
        if (strlen(vesa_edid) > 0)
            break;

        if (count >= 5) {
            strcpy(vesa_edid, "null vesa_edid");
            break;
        }
        count++;
        usleep(500000);
    }
}

void DisplayMode::getHdmiDcCap(char* dc_cap) {
    if (!dc_cap) {
        SYS_LOGE("%s dc_cap is NULL\n", __FUNCTION__);
        return;
    }

    int count = 0;
    while (true) {
        pSysWrite->readSysfsOriginal(DISPLAY_HDMI_DEEP_COLOR, dc_cap);
        if (strlen(dc_cap) > 0)
            break;

        if (count >= 5) {
            strcpy(dc_cap, "444,8bit");
            break;
        }
        count++;
        usleep(500000);
    }
}

int DisplayMode::getHdmiSinkType(void) {
    char sinkType[MODE_LEN] = {0};
    pSysWrite->readSysfsOriginal(DISPLAY_HDMI_SINK_TYPE, sinkType);

    if (NULL != strstr(sinkType, "sink")) {
        return HDMI_SINK_TYPE_SINK;
    } else if (NULL != strstr(sinkType, "repeater")) {
        return HDMI_SINK_TYPE_REPEATER;
    } else {
        return HDMI_SINK_TYPE_NONE;
    }
}

void DisplayMode::getHdmiEdidStatus(char* edidstatus) {
    if (!edidstatus) {
        SYS_LOGE("%s edidstatus is NULL\n", __FUNCTION__);
        return;
    }

    pSysWrite->readSysfs(DISPLAY_EDID_STATUS, edidstatus);
}

void DisplayMode::getHdmiData(hdmi_data_t* data) {
    char vesa_edid[MAX_STR_LEN];
    if (!data) {
        SYS_LOGE("%s data is NULL\n", __FUNCTION__);
        return;
    }

    //common info
    getCommonData(data);

    //hdmi info
    getHdmiEdidStatus(data->edidParsing);
    //three sink types: sink, repeater, none
    data->sinkType = getHdmiSinkType();
    SYS_LOGI("display sink type:%d [0:none, 1:sink, 2:repeater]\n", data->sinkType);

    if (HDMI_SINK_TYPE_NONE != data->sinkType) {
        //read hdmi disp_cap
        char disp_cap[MAX_STR_LEN];
        getHdmiDispCap(disp_cap);
        getHdmiVesaEdid(vesa_edid);
        SYS_LOGI("DisplayMode getHdmiData vesa_edid:%s\n",vesa_edid);
        strcat(disp_cap, vesa_edid);
        SYS_LOGI("DisplayMode getHdmiData data->edid:%s\n",disp_cap);
        strcpy(data->disp_cap, disp_cap);

        //read hdmi dc_cap
        char dc_cap[MAX_STR_LEN];
        getHdmiDcCap(dc_cap);
        strcpy(data->dc_cap, dc_cap);
    }

    //filter hdmi disp_cap mode for compatibility
    filterHdmiDispcap(data);

    //get hdmi dv_info
    getHdmiDvCap(data);
}

bool DisplayMode::modeSupport(char *mode, int sinkType) {
    char **pMode = NULL;
    int modeSize = 0;

    if (HDMI_SINK_TYPE_SINK == sinkType) {
        pMode= (char **)MODES_SINK;
        modeSize = ARRAY_SIZE(MODES_SINK);
    }
    else if (HDMI_SINK_TYPE_REPEATER == sinkType) {
        pMode= (char **)MODES_REPEATER;
        modeSize = ARRAY_SIZE(MODES_REPEATER);
    }

    for (int i = 0; i < modeSize; i++) {
        //SYS_LOGI("modeSupport mode=%s, filerMode:%s, size:%d\n", mode, pMode[i], modeSize);
        if (!strcmp(mode, pMode[i]))
            return true;
    }

    return false;
}

void DisplayMode::startBootanimDetectThread() {
    pthread_t id;
    int ret = pthread_create(&id, NULL, bootanimDetect, this);
    if (ret != 0) {
        SYS_LOGE("Create BootanimDetect error!\n");
    }
}

//if detected bootanim is running, then close uboot logo
void* DisplayMode::bootanimDetect(void* data) {
    DisplayMode *pThiz = (DisplayMode*)data;
    char fs_mode[MODE_LEN] = {0};
    char recovery_state[MODE_LEN] = {0};
    char bootvideo[MODE_LEN] = {0};

    if (pThiz->mIsRecovery) {
        SYS_LOGI("this is recovery mode");
        usleep(1000000LL);
        pThiz->pSysWrite->writeSysfs(DISPLAY_FB0_BLANK, "1");
        //need close fb1, because uboot logo show in fb1
        pThiz->pSysWrite->writeSysfs(DISPLAY_FB1_BLANK, "1");
        pThiz->pSysWrite->writeSysfs(DISPLAY_FB1_FREESCALE, "0");
        pThiz->pSysWrite->writeSysfs(DISPLAY_FB0_FREESCALE, "0x10001");
        pThiz->pSysWrite->writeSysfs(DISPLAY_FB0_BLANK, "0");
    } else {
        SYS_LOGI("this is android mode");
    }

    return NULL;
}

//get edid crc value to check edid change
bool DisplayMode::isEdidChange() {
    char edid[MAX_STR_LEN] = {0};
    char crcvalue[MAX_STR_LEN] = {0};
    unsigned int crcheadlength = strlen(DEFAULT_EDID_CRCHEAD);
    pSysWrite->readSysfs(DISPLAY_EDID_VALUE, edid);
    char *p = strstr(edid, DEFAULT_EDID_CRCHEAD);
    if (p != NULL && strlen(p) > crcheadlength) {
        p += crcheadlength;
        if (!getBootEnv(UBOOTENV_EDIDCRCVALUE, crcvalue) || strncmp(p, crcvalue, strlen(p))) {
            SYS_LOGI("update edidcrc: %s->%s\n", crcvalue, p);
            setBootEnv(UBOOTENV_EDIDCRCVALUE, p);
            return true;
        }
    }
    return false;
}

bool DisplayMode::isBestOutputmode() {
    char isBestMode[MODE_LEN] = {0};
    if (DISPLAY_TYPE_TV == mDisplayType) {
        return false;
    }
    return !getBootEnv(UBOOTENV_ISBESTMODE, isBestMode) || strcmp(isBestMode, "true") == 0;
}

bool DisplayMode::isFrameratePriority() {
    return pSysWrite->getPropertyBoolean(PROP_HDMI_FRAMERATE_PRIORITY, false);
}

bool DisplayMode::isSupport4K() {
    return pSysWrite->getPropertyBoolean(PROP_SUPPORT_4K, true);
}

bool DisplayMode::isSupport4K30Hz() {
    return pSysWrite->getPropertyBoolean(PROP_SUPPORT_OVER_4K30, true);
}

bool DisplayMode::isSupportDeepColor() {
    return pSysWrite->getPropertyBoolean(PROP_DEEPCOLOR, true);
}

bool DisplayMode::isLowPowerMode() {
    return pSysWrite->getPropertyBoolean(LOW_POWER_DEFAULT_COLOR, false);
}

void DisplayMode::setSinkOutputMode(const char* outputmode) {
    setSinkOutputMode(outputmode, false);
}

void DisplayMode::setSinkOutputMode(const char* outputmode, bool initState) {
    SYS_LOGI("set sink output mode:%s, init state:%d\n", outputmode, initState?1:0);

    //set output mode
    char curMode[MODE_LEN] = {0};
    getDisplayMode(curMode);

    SYS_LOGI("curMode = %s outputmode = %s", curMode, outputmode);
    if (strstr(curMode, outputmode) == NULL) {
        DisplayModeMgr::getInstance().setDisplayMode(outputmode);
    }

    if (pSysWrite->getPropertyBoolean(PROP_DISPLAY_SIZE_CHECK, true)) {
        char resolution[MODE_LEN] = {0};
        char defaultResolution[MODE_LEN] = {0};
        char finalResolution[MODE_LEN] = {0};
        int w = 0, h = 0, w1 =0, h1 = 0;
        pSysWrite->readSysfs(SYS_DISPLAY_RESOLUTION, resolution);
        pSysWrite->getPropertyString(PROP_DISPLAY_SIZE, defaultResolution, "0x0");
        sscanf(resolution, "%dx%d", &w, &h);
        sscanf(defaultResolution, "%dx%d", &w1, &h1);
        if ((w != w1) || (h != h1)) {
            if (strstr(outputmode, "null") && w1 != 0) {
                sprintf(finalResolution, "%dx%d", w1, h1);
            } else {
                sprintf(finalResolution, "%dx%d", w, h);
            }
            pSysWrite->setProperty(PROP_DISPLAY_SIZE, finalResolution);
        }
    }

    char defaultResolution[MODE_LEN] = {0};
    pSysWrite->getPropertyString(PROP_DISPLAY_SIZE, defaultResolution, "0x0");
    SYS_LOGI("set display-size:%s\n", defaultResolution);

    //update hwc windows size
    int position[4] = { 0, 0, 0, 0 };//x,y,w,h
    getPosition(outputmode, position);
    setPosition(outputmode, position[0], position[1],position[2], position[3]);

    // no need to update
    // update free_scale_axis and window_axis in recovery mode
#ifdef RECOVERY_MODE
    updateFreeScaleAxis();
    updateWindowAxis(outputmode);
#endif

    //update hdr policy
    if ((isMboxSupportDolbyVision() == false)) {
        if (pSysWrite->getPropertyBoolean(PROP_DOLBY_VISION_FEATURE, false)) {
            char hdr_policy[MODE_LEN] = {0};
            getHdrStrategy(hdr_policy);
            if (strstr(hdr_policy, HDR_POLICY_SINK)) {
                DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_HDR_POLICY, HDR_POLICY_SINK, ConnectorType::CONN_TYPE_HDMI);
            } else if (strstr(hdr_policy, HDR_POLICY_SOURCE)) {
                DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_HDR_POLICY, HDR_POLICY_SOURCE, ConnectorType::CONN_TYPE_HDMI);
            }
        } else {
            initHdrSdrMode();
        }
    }

    if (isMboxSupportDolbyVision()) {
        if (isTvDolbyVisionEnable()) {
            setTvDolbyVisionEnable();
        } else {
            setTvDolbyVisionDisable();
        }
    }

    if (initState) {
#ifdef RECOVERY_MODE
        startBootanimDetectThread();
#endif
    }
#ifndef RECOVERY_MODE
    notifyEvent(EVENT_OUTPUT_MODE_CHANGE);
#endif

    //audio
    char value[MAX_STR_LEN] = {0};
    memset(value, 0, sizeof(0));
    getBootEnv(UBOOTENV_DIGITAUDIO, value);
    setDigitalMode(value);

    //save output mode
    char finalMode[MODE_LEN] = {0};
    getDisplayMode(finalMode);
    if (DISPLAY_TYPE_TABLET != mDisplayType) {
        setBootEnv(UBOOTENV_OUTPUTMODE, (char *)finalMode);
    }
    if (strstr(finalMode, "cvbs") != NULL) {
        setBootEnv(UBOOTENV_CVBSMODE, (char *)finalMode);
    } else if (strstr(finalMode, "hz") != NULL) {
        setBootEnv(UBOOTENV_HDMIMODE, (char *)finalMode);
    }

    SYS_LOGI("set output mode:%s done\n", finalMode);
}

void DisplayMode::setSinkDisplay(bool initState) {
    char current_mode[MODE_LEN] = {0};
    char outputmode[MODE_LEN] = {0};

    getDisplayMode(current_mode);
    getBootEnv(UBOOTENV_OUTPUTMODE, outputmode);
    SYS_LOGD("init tv display old outputmode:%s, outputmode:%s\n", current_mode, outputmode);

    if (strlen(outputmode) == 0)
        strcpy(outputmode, mDefaultUI);

    updateDefaultUI();

    setSinkOutputMode(outputmode, initState);
}

int DisplayMode::getBootenvInt(const char* key, int defaultVal) {
    int value = defaultVal;
    const char* p_value = mUbootenv->getValue(key);
    if (p_value) {
        value = atoi(p_value);
    }
    return value;
}

void DisplayMode::updateDefaultUI() {
    if (!strncmp(mDefaultUI, "720", 3)) {
        mDisplayWidth= FULL_WIDTH_720;
        mDisplayHeight = FULL_HEIGHT_720;
    } else if (!strncmp(mDefaultUI, "1080", 4)) {
        mDisplayWidth = FULL_WIDTH_1080;
        mDisplayHeight = FULL_HEIGHT_1080;
    } else if (!strncmp(mDefaultUI, "4k2k", 4)) {
        mDisplayWidth = FULL_WIDTH_4K2K;
        mDisplayHeight = FULL_HEIGHT_4K2K;
    }
}

void DisplayMode::updateDeepColor(bool cvbsMode, output_mode_state state, const char* outputmode) {
    if (!cvbsMode && (mDisplayType != DISPLAY_TYPE_TV)) {
        char colorAttribute[MODE_LEN] = {0};
        if (pSysWrite->getPropertyBoolean(PROP_DEEPCOLOR, true)) {
            char mode[MAX_STR_LEN] = {0};
            if (isDolbyVisionEnable() && isTvSupportDolbyVision(mode) && (mHdmidata.hdr_priority == DOLBY_VISION_PRIORITY)) {
                 char type[MODE_LEN] = {0};
                 strcpy(type, mHdmidata.dv_info.ubootenv_dv_type);
                if (((atoi(type) == 2) && (strstr(mode, DV_MODE_TYPE[2]) == NULL))
                        || ((atoi(type) == 3) && (strstr(mode, DV_MODE_TYPE[3]) == NULL)
                            && (strstr(mode, DV_MODE_TYPE[4]) == NULL))) {
                    strcpy(type, "1");
                }
                switch (atoi(type)) {
                    case DOLBY_VISION_SET_ENABLE:
                        strcpy(colorAttribute, "444,8bit");
                        break;
                    case DOLBY_VISION_SET_ENABLE_LL_YUV:
                        strcpy(colorAttribute, "422,12bit");
                        break;
                    case DOLBY_VISION_SET_ENABLE_LL_RGB:
                        if (strstr(mode, "LL_RGB_444_12BIT") != NULL) {
                            strcpy(colorAttribute, "444,12bit");
                        } else if (strstr(mode, "LL_RGB_444_10BIT") != NULL) {
                            strcpy(colorAttribute, "444,10bit");
                        }
                        break;
                }
            } else {
                pmDeepColor->getHdmiColorAttribute(outputmode, colorAttribute, (int)state);
            }
        } else {
            strcpy(colorAttribute, "default");
        }
        std::string attr;
        DisplayModeMgr::getInstance().getDisplayAttribute(DISPLAY_HDMI_COLOR_ATTR, attr);
        if (strstr(attr.c_str(), colorAttribute) == NULL) {
            SYS_LOGI("set DeepcolorAttr value is different from attr sysfs value\n");
            DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_HDMI_COLOR_ATTR, colorAttribute);
        } else {
            SYS_LOGI("cur deepcolor attr value is equals to colorAttribute, Do not need set it\n");
        }
        SYS_LOGI("setMboxOutputMode colorAttribute = %s\n", colorAttribute);
        //save to ubootenv
        saveDeepColorAttr(outputmode, colorAttribute);
        setBootEnv(UBOOTENV_COLORATTRIBUTE, colorAttribute);
    }
}

void DisplayMode::updateFreeScaleAxis() {
    char axis[MAX_STR_LEN] = {0};
    sprintf(axis, "%d %d %d %d",
            0, 0, mDisplayWidth - 1, mDisplayHeight - 1);

    SYS_LOGI("axis: %s\n", axis);

    pSysWrite->writeSysfs(DISPLAY_FB0_FREESCALE_AXIS, axis);
}

void DisplayMode::updateWindowAxis(const char* outputmode) {
    char axis[MAX_STR_LEN] = {0};
    int position[4] = { 0, 0, 0, 0 };//x,y,w,h
    getPosition(outputmode, position);
    sprintf(axis, "%d %d %d %d",
            position[0], position[1], position[0] + position[2] - 1, position[1] + position[3] -1);
    pSysWrite->writeSysfs(DISPLAY_FB0_WINDOW_AXIS, axis);
}

void DisplayMode::getBootanimStatus(int *status) {
    SYS_LOGI("getBootanimStatus is no longer supported in Android P or later\n");
    *status = 0;
    return;
}

void DisplayMode::getPosition(const char* curMode, int *position) {
    char keyValue[20] = {0};
    char ubootvar[100] = {0};
    int defaultWidth = 0;
    int defaultHeight = 0;
    if (strstr(curMode, MODE_480CVBS)) {
        strcpy(keyValue, MODE_480CVBS);
        defaultWidth = FULL_WIDTH_480;
        defaultHeight = FULL_HEIGHT_480;
    } else if (strstr(curMode, "480")&& !strstr(curMode, "640x480p") && !strstr(curMode, "800x480p")) {
        strcpy(keyValue, strstr(curMode, MODE_480P_PREFIX) ? MODE_480P_PREFIX : MODE_480I_PREFIX);
        defaultWidth = FULL_WIDTH_480;
        defaultHeight = FULL_HEIGHT_480;
    } else if (strstr(curMode, MODE_576CVBS)) {
        strcpy(keyValue, MODE_576CVBS);
        defaultWidth = FULL_WIDTH_576;
        defaultHeight = FULL_HEIGHT_576;
    } else if (strstr(curMode, "576")) {
        strcpy(keyValue, strstr(curMode, MODE_576P_PREFIX) ? MODE_576P_PREFIX : MODE_576I_PREFIX);
        defaultWidth = FULL_WIDTH_576;
        defaultHeight = FULL_HEIGHT_576;
    } else if (strstr(curMode, MODE_PAL_M)) {
        strcpy(keyValue, MODE_PAL_M);
        defaultWidth = FULL_WIDTH_480;
        defaultHeight = FULL_HEIGHT_480;
    } else if (strstr(curMode, MODE_PAL_N)) {
        strcpy(keyValue, MODE_PAL_N);
        defaultWidth = FULL_WIDTH_576;
        defaultHeight = FULL_HEIGHT_576;
    } else if (strstr(curMode, MODE_NTSC_M)) {
        strcpy(keyValue, MODE_NTSC_M);
        defaultWidth = FULL_WIDTH_480;
        defaultHeight = FULL_HEIGHT_480;
    } else if (strstr(curMode, MODE_720P_PREFIX)) {
        strcpy(keyValue, MODE_720P_PREFIX);
        defaultWidth = FULL_WIDTH_720;
        defaultHeight = FULL_HEIGHT_720;
    } else if (strstr(curMode, MODE_768P_PREFIX)&& !strstr(curMode, "1024x768p")&& !strstr(curMode, "1360x768p")) {
        strcpy(keyValue, MODE_768P_PREFIX);
        defaultWidth = FULL_WIDTH_768;
        defaultHeight = FULL_HEIGHT_768;
    } else if (strstr(curMode, MODE_1080I_PREFIX)) {
        strcpy(keyValue, MODE_1080I_PREFIX);
        defaultWidth = FULL_WIDTH_1080;
        defaultHeight = FULL_HEIGHT_1080;
    } else if (strstr(curMode, MODE_1080P_PREFIX)) {
        strcpy(keyValue, MODE_1080P_PREFIX);
        defaultWidth = FULL_WIDTH_1080;
        defaultHeight = FULL_HEIGHT_1080;
    } else if (strstr(curMode, MODE_4K2K_PREFIX)) {
        strcpy(keyValue, MODE_4K2K_PREFIX);
        defaultWidth = FULL_WIDTH_4K2K;
        defaultHeight = FULL_HEIGHT_4K2K;
    } else if (strstr(curMode, MODE_4K2KSMPTE_PREFIX)) {
        strcpy(keyValue, "4k2ksmpte");
        defaultWidth = FULL_WIDTH_4K2KSMPTE;
        defaultHeight = FULL_HEIGHT_4K2KSMPTE;
    } else if (strstr(curMode, MODE_480x320P_PREFIX)) {
        strcpy(keyValue, MODE_480x320P_PREFIX);
        defaultWidth = FULL_WIDTH_480x320;
        defaultHeight = FULL_HEIGHT_480x320;
    } else if (strstr(curMode, MODE_640x480P_PREFIX)) {
        strcpy(keyValue, MODE_640x480P_PREFIX);
        defaultWidth = FULL_WIDTH_640x480;
        defaultHeight = FULL_HEIGHT_640x480;
    } else if (strstr(curMode, MODE_800x480P_PREFIX)) {
        strcpy(keyValue, MODE_800x480P_PREFIX);
        defaultWidth = FULL_WIDTH_800x480;
        defaultHeight = FULL_HEIGHT_800x480;
    } else if (strstr(curMode, MODE_800x600P_PREFIX)) {
        strcpy(keyValue, MODE_800x600P_PREFIX);
        defaultWidth = FULL_WIDTH_800x600;
        defaultHeight = FULL_HEIGHT_800x600;
    } else if (strstr(curMode, MODE_1024x600P_PREFIX)) {
        strcpy(keyValue, MODE_1024x600P_PREFIX);
        defaultWidth = FULL_WIDTH_1024x600;
        defaultHeight = FULL_HEIGHT_1024x600;
    } else if (strstr(curMode, MODE_1024x768P_PREFIX)) {
        strcpy(keyValue, MODE_1024x768P_PREFIX);
        defaultWidth = FULL_WIDTH_1024x768;
        defaultHeight = FULL_HEIGHT_1024x768;
    } else if (strstr(curMode, MODE_1280x480P_PREFIX)) {
        strcpy(keyValue, MODE_1280x480P_PREFIX);
        defaultWidth = FULL_WIDTH_1280x480;
        defaultHeight = FULL_HEIGHT_1280x480;
    } else if (strstr(curMode, MODE_1280x800P_PREFIX)) {
        strcpy(keyValue, MODE_1280x800P_PREFIX);
        defaultWidth = FULL_WIDTH_1280x800;
        defaultHeight = FULL_HEIGHT_1280x800;
    } else if (strstr(curMode, MODE_1280x1024P_PREFIX)) {
        strcpy(keyValue, MODE_1280x1024P_PREFIX);
        defaultWidth = FULL_WIDTH_1280x1024;
        defaultHeight = FULL_HEIGHT_1280x1024;
    } else if (strstr(curMode, MODE_1360x768P_PREFIX)) {
        strcpy(keyValue, MODE_1360x768P_PREFIX);
        defaultWidth = FULL_WIDTH_1360x768;
        defaultHeight = FULL_HEIGHT_1360x768;
    } else if (strstr(curMode, MODE_1440x900P_PREFIX)) {
        strcpy(keyValue, MODE_1440x900P_PREFIX);
        defaultWidth = FULL_WIDTH_1440x900;
        defaultHeight = FULL_HEIGHT_1440x900;
    } else if (strstr(curMode, MODE_1600x900P_PREFIX)) {
        strcpy(keyValue, MODE_1600x900P_PREFIX);
        defaultWidth = FULL_WIDTH_1600x900;
        defaultHeight = FULL_HEIGHT_1600x900;
    } else if (strstr(curMode, MODE_1600x1200P_PREFIX)) {
        strcpy(keyValue, MODE_1600x1200P_PREFIX);
        defaultWidth = FULL_WIDTH_1600x1200;
        defaultHeight = FULL_HEIGHT_1600x1200;
    } else if (strstr(curMode, MODE_1680x1050P_PREFIX)) {
        strcpy(keyValue, MODE_1680x1050P_PREFIX);
        defaultWidth = FULL_WIDTH_1680x1050;
        defaultHeight = FULL_HEIGHT_1680x1050;
    } else if (strstr(curMode, MODE_1920x1200P_PREFIX)) {
        strcpy(keyValue, MODE_1920x1200P_PREFIX);
        defaultWidth = FULL_WIDTH_1920x1200;
        defaultHeight = FULL_HEIGHT_1920x1200;
    } else if (strstr(curMode, MODE_2560x1080P_PREFIX)) {
        strcpy(keyValue, MODE_2560x1080P_PREFIX);
        defaultWidth = FULL_WIDTH_2560x1080;
        defaultHeight = FULL_HEIGHT_2560x1080;
    } else if (strstr(curMode, MODE_2560x1440P_PREFIX)) {
        strcpy(keyValue, MODE_2560x1440P_PREFIX);
        defaultWidth = FULL_WIDTH_2560x1440;
        defaultHeight = FULL_HEIGHT_2560x1440;
    } else if (strstr(curMode, MODE_2560x1600P_PREFIX)) {
        strcpy(keyValue, MODE_2560x1600P_PREFIX);
        defaultWidth = FULL_WIDTH_2560x1600;
        defaultHeight = FULL_HEIGHT_2560x1600;
    } else if (strstr(curMode, MODE_3440x1440P_PREFIX)) {
        strcpy(keyValue, MODE_3440x1440P_PREFIX);
        defaultWidth = FULL_WIDTH_3440x1440;
        defaultHeight = FULL_HEIGHT_3440x1440;
    } else if (strstr(curMode, MODE_PANEL)) {
        strcpy(keyValue, MODE_PANEL);
        defaultWidth = FULL_WIDTH_PANEL;
        defaultHeight = FULL_HEIGHT_PANEL;
    } else {
        strcpy(keyValue, MODE_1080P_PREFIX);
        defaultWidth = FULL_WIDTH_1080;
        defaultHeight = FULL_HEIGHT_1080;
    }

    pthread_mutex_lock(&mEnvLock);
    sprintf(ubootvar, "ubootenv.var.%s_x", keyValue);
    position[0] = getBootenvInt(ubootvar, 0);
    sprintf(ubootvar, "ubootenv.var.%s_y", keyValue);
    position[1] = getBootenvInt(ubootvar, 0);
    sprintf(ubootvar, "ubootenv.var.%s_w", keyValue);
    position[2] = getBootenvInt(ubootvar, defaultWidth);
    sprintf(ubootvar, "ubootenv.var.%s_h", keyValue);
    position[3] = getBootenvInt(ubootvar, defaultHeight);

    SYS_LOGI("%s curMode:%s position[0]:%d position[1]:%d position[2]:%d position[3]:%d\n", __FUNCTION__, curMode, position[0], position[1], position[2], position[3]);

    pthread_mutex_unlock(&mEnvLock);

}

void DisplayMode::setPosition(const char* curMode, int left, int top, int width, int height) {
    char x[512] = {0};
    char y[512] = {0};
    char w[512] = {0};
    char h[512] = {0};
    sprintf(x, "%d", left);
    sprintf(y, "%d", top);
    sprintf(w, "%d", width);
    sprintf(h, "%d", height);

    SYS_LOGI("%s curMode:%s left:%d top:%d width:%d height:%d\n", __FUNCTION__, curMode, left, top, width, height);

    char keyValue[20] = {0};
    char ubootvar[100] = {0};
    if (strstr(curMode, MODE_480CVBS)) {
        strcpy(keyValue, MODE_480CVBS);
    } else if (strstr(curMode, "480")) {
        strcpy(keyValue, strstr(curMode, MODE_480P_PREFIX) ? MODE_480P_PREFIX : MODE_480I_PREFIX);
    } else if (strstr(curMode, MODE_576CVBS)) {
        strcpy(keyValue, MODE_576CVBS);
    } else if (strstr(curMode, "576")) {
        strcpy(keyValue, strstr(curMode, MODE_576P_PREFIX) ? MODE_576P_PREFIX : MODE_576I_PREFIX);
    } else if (strstr(curMode, MODE_PAL_M)) {
        strcpy(keyValue, MODE_PAL_M);
    } else if (strstr(curMode, MODE_PAL_N)) {
        strcpy(keyValue, MODE_PAL_N);
    } else if (strstr(curMode, MODE_NTSC_M)) {
        strcpy(keyValue, MODE_NTSC_M);
    } else if (strstr(curMode, MODE_720P_PREFIX)) {
        strcpy(keyValue, MODE_720P_PREFIX);
    } else if (strstr(curMode, MODE_768P_PREFIX)) {
        strcpy(keyValue, MODE_768P_PREFIX);
    } else if (strstr(curMode, MODE_1080I_PREFIX)) {
        strcpy(keyValue, MODE_1080I_PREFIX);
    } else if (strstr(curMode, MODE_1080P_PREFIX)) {
        strcpy(keyValue, MODE_1080P_PREFIX);
    } else if (strstr(curMode, MODE_4K2K_PREFIX)) {
        strcpy(keyValue, MODE_4K2K_PREFIX);
    } else if (strstr(curMode, MODE_4K2KSMPTE_PREFIX)) {
        strcpy(keyValue, "4k2ksmpte");
    } else if (strstr(curMode, MODE_PANEL)) {
        strcpy(keyValue, MODE_PANEL);
		return;
    }

    pthread_mutex_lock(&mEnvLock);
    sprintf(ubootvar, "ubootenv.var.%s_x", keyValue);
    setBootEnv(ubootvar, x);
    sprintf(ubootvar, "ubootenv.var.%s_y", keyValue);
    setBootEnv(ubootvar, y);
    sprintf(ubootvar, "ubootenv.var.%s_w", keyValue);
    setBootEnv(ubootvar, w);
    sprintf(ubootvar, "ubootenv.var.%s_h", keyValue);
    setBootEnv(ubootvar, h);
    DisplayModeMgr::getInstance().setDisplayRect({left, top, width , height});
    pthread_mutex_unlock(&mEnvLock);

}

void DisplayMode::saveDeepColorAttr(const char* mode, const char* dcValue) {
    char ubootvar[100] = {0};
    sprintf(ubootvar, "ubootenv.var.%s_deepcolor", mode);
    setBootEnv(ubootvar, (char *)dcValue);
}

void DisplayMode::getDeepColorAttr(const char* mode, char* value) {
    std::string cur_ColorAttribute;
    DisplayModeMgr::getInstance().getDisplayAttribute(DISPLAY_HDMI_COLOR_ATTR, cur_ColorAttribute);
    strcpy(value, cur_ColorAttribute.c_str());

    SYS_LOGI("%s colorspace:%s\n", __FUNCTION__, value);
}

bool DisplayMode::getDisplayMode(char* mode) {
    bool ret = false;

    if (mode != NULL) {
        ret = DisplayModeMgr::getInstance().getDisplayMode(mode, MODE_LEN);
        SYS_LOGI("%s mode:%s\n", __FUNCTION__, mode);
    } else {
        SYS_LOGE("%s mode is NULL\n", __FUNCTION__);
    }

    return ret;
}

//set hdmi output mode
void DisplayMode::setDisplayMode(std::string mode) {
    SYS_LOGI("%s mode:%s\n", __FUNCTION__, mode.c_str());
    DisplayModeMgr::getInstance().setDisplayMode(mode);
}

/* *
 * @Description: Detect Whether TV support HDR
 * @return: if TV support return true, or false
 */
bool DisplayMode::isTvSupportHDR() {
    if (DISPLAY_TYPE_TV == mDisplayType) {
        SYS_LOGI("Current Device is TV, no hdr_cap\n");
        return false;
    }

/*
    //read hdr_cap
    std::string hdr_cap;
    DisplayModeMgr::getInstance().getDisplayAttribute(DISPLAY_HDR_CAP, hdr_cap, ConnectorType::CONN_TYPE_HDMI);

    //check hdr_cap
    if ((strstr(hdr_cap.c_str(), "HDR10Plus Supported: 1") != NULL)
        || (strstr(hdr_cap.c_str(), "SMPTE ST 2084: 1") != NULL)
        || (strstr(hdr_cap.c_str(), "Hybrid Log-Gamma: 1") != NULL)) {
        SYS_LOGD("Current Tv Support HDR:%s", hdr_cap.c_str());
        return true;
    }
*/
    char hdr_cap[MAX_STR_LEN];
    pSysWrite->readSysfs(DISPLAY_HDMI_HDR_CAP2, hdr_cap);

    //check hdr_cap
    if ((strstr(hdr_cap, "HDR10Plus Supported: 1") != NULL)
        || (strstr(hdr_cap, "SMPTE ST 2084: 1") != NULL)
        || (strstr(hdr_cap, "Hybrid Log-Gamma: 1") != NULL)) {
        SYS_LOGD("Current Tv Support HDR:%s", hdr_cap);
        return true;
    }

    return false;
}

/* *
 * @Description: Detect Whether TV support Dolby Vision
 * @return: if TV support return true, or false
 * if true, mode is the Highest resolution Tv Dolby Vision supported
 * else mode is ""
 */
bool DisplayMode::isTvSupportDolbyVision(char *mode) {
    char dv_cap[MAX_STR_LEN] = {0};
    strcpy(mode, "");
    if (DISPLAY_TYPE_TV == mDisplayType) {
        SYS_LOGI("Current Device is TV, no dv_cap\n");
        return false;
    }

    if (strstr(mHdmidata.dv_info.dv_cap, "DolbyVision RX support list") == NULL) {
        return false;
    }

    strcat(mode, mHdmidata.dv_info.dv_displaymode);
    strcat(mode, mHdmidata.dv_info.dv_deepcolor);

    if (mLogLevel > LOG_LEVEL_1) {
        SYS_LOGI("Current Tv Support DV type [%s]", mode);
    }

    return true;
}

bool DisplayMode::isMboxSupportDolbyVision() {
    return pSysWrite->getPropertyBoolean(PROP_SUPPORT_DOLBY_VISION, false);
}

bool DisplayMode::isTvDolbyVisionEnable() {
    bool ret = false;
    char dv_enable[MODE_LEN];
    ret = getBootEnv(UBOOTENV_DV_ENABLE, dv_enable);
    if (!ret) {
        if (isMboxSupportDolbyVision()) {
            strcpy(dv_enable, "1");
        } else {
            strcpy(dv_enable, "0");
        }
    }
    SYS_LOGI("dv_enable:%s\n", dv_enable);

    if (!strcmp(dv_enable, "0")) {
        return false;
    } else {
        return true;
    }
}

bool DisplayMode::isDolbyVisionEnable() {
    if (isMboxSupportDolbyVision()) {
        if (!strcmp(mHdmidata.dv_info.dv_enable, "0") ) {
            return false;
        } else {
            return true;
        }
    } else {
        return false;
    }
}

void DisplayMode::setTvDolbyVisionEnable(void) {
    //if TV
    setHdrMode(HDR_MODE_OFF);
    DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_DOLBY_VISION_POLICY, DV_POLICY_FOLLOW_SINK, ConnectorType::CONN_TYPE_HDMI);

    usleep(100000);//100ms
    DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_DOLBY_VISION_ENABLE, DV_ENABLE, ConnectorType::CONN_TYPE_HDMI);
    DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_DOLBY_VISION_MODE, DV_MODE_IPT_TUNNEL, ConnectorType::CONN_TYPE_HDMI);
    usleep(100000);//100ms

    setHdrMode(HDR_MODE_AUTO);

    initGraphicsPriority();
}

void DisplayMode::setTvDolbyVisionDisable(void) {
    int  check_status_count = 0;

    //2. update sysfs
    DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_HDR_POLICY, HDR_POLICY_SINK, ConnectorType::CONN_TYPE_HDMI);

    DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_DOLBY_VISION_POLICY, DV_POLICY_FORCE_MODE, ConnectorType::CONN_TYPE_HDMI);
    DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_DOLBY_VISION_MODE, DV_MODE_BYPASS, ConnectorType::CONN_TYPE_HDMI);
    usleep(100000);//100ms
    std::string dvstatus = "";
    DisplayModeMgr::getInstance().getDisplayAttribute(DISPLAY_DOLBY_VISION_STATUS, dvstatus, ConnectorType::CONN_TYPE_HDMI);

    if (strcmp(dvstatus.c_str(), BYPASS_PROCESS)) {
        while (++check_status_count <30) {
            usleep(20000);//20ms
            DisplayModeMgr::getInstance().getDisplayAttribute(DISPLAY_DOLBY_VISION_STATUS, dvstatus, ConnectorType::CONN_TYPE_HDMI);
            if (!strcmp(dvstatus.c_str(), BYPASS_PROCESS)) {
                break;
            }
        }
    }

    SYS_LOGI("dvstatus %s, check_status_count [%d]", dvstatus.c_str(), check_status_count);
    DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_DOLBY_VISION_ENABLE, DV_DISABLE, ConnectorType::CONN_TYPE_HDMI);

    setHdrMode(HDR_MODE_AUTO);
    setSdrMode(SDR_MODE_AUTO);
}

void DisplayMode::enableDolbyVision(int DvMode) {
    char tvmode[MAX_STR_LEN] = {0};

    if (isMboxSupportDolbyVision() == false) {
        SYS_LOGI("This platform is not support dolby vision or has not dovi.ko");
        return;
    }
    SYS_LOGI("DvMode %d", DvMode);

    strcpy(mHdmidata.dv_info.dv_enable, "1");

    //if TV
    if (DISPLAY_TYPE_TV == mDisplayType) {
        setHdrMode(HDR_MODE_OFF);
        DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_DOLBY_VISION_POLICY, DV_POLICY_FOLLOW_SINK, ConnectorType::CONN_TYPE_HDMI);
    }

    //if OTT
    if ((DISPLAY_TYPE_MBOX == mDisplayType) || (DISPLAY_TYPE_REPEATER == mDisplayType)) {
        if (isTvSupportDolbyVision(tvmode) && (mHdmidata.hdr_priority == DOLBY_VISION_PRIORITY)) {
            SYS_LOGI("Tv is Support DolbyVision, tvmode is [%s]", tvmode);

            switch (DvMode) {
                case DOLBY_VISION_SET_ENABLE:
                    SYS_LOGI("Dolby Vision set Mode [DV_RGB_444_8BIT]\n");
                    DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_DOLBY_VISION_LL_POLICY, "0", ConnectorType::CONN_TYPE_HDMI);
                    break;
                case DOLBY_VISION_SET_ENABLE_LL_YUV:
                    SYS_LOGI("Dolby Vision set Mode [LL_YCbCr_422_12BIT]\n");
                    DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_DOLBY_VISION_LL_POLICY, "0", ConnectorType::CONN_TYPE_HDMI);
                    DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_DOLBY_VISION_LL_POLICY, "1", ConnectorType::CONN_TYPE_HDMI);
                    break;
                case DOLBY_VISION_SET_ENABLE_LL_RGB:
                    DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_DOLBY_VISION_LL_POLICY, "0", ConnectorType::CONN_TYPE_HDMI);
                    DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_DOLBY_VISION_LL_POLICY, "2", ConnectorType::CONN_TYPE_HDMI);
                    break;
                default:
                    DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_DOLBY_VISION_LL_POLICY, "0", ConnectorType::CONN_TYPE_HDMI);
            }
        }

        char hdr_policy[MODE_LEN] = {0};
        getHdrStrategy(hdr_policy);
        if (strstr(hdr_policy, HDR_POLICY_SINK)) {
            DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_HDR_POLICY, HDR_POLICY_SINK, ConnectorType::CONN_TYPE_HDMI);
            if (isDolbyVisionEnable()) {
                DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_DOLBY_VISION_POLICY, HDR_POLICY_SINK, ConnectorType::CONN_TYPE_HDMI);
            }
        } else if (strstr(hdr_policy, HDR_POLICY_SOURCE)) {
            DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_HDR_POLICY, HDR_POLICY_SOURCE, ConnectorType::CONN_TYPE_HDMI);
            if (isDolbyVisionEnable()) {
                DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_DOLBY_VISION_POLICY, HDR_POLICY_SOURCE, ConnectorType::CONN_TYPE_HDMI);
            }
        }

        DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_DOLBY_VISION_HDR_10_POLICY, DV_HDR_SINK_PROCESS, ConnectorType::CONN_TYPE_HDMI);
    }

    usleep(100000);//100ms
    DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_DOLBY_VISION_ENABLE, DV_ENABLE, ConnectorType::CONN_TYPE_HDMI);
    DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_DOLBY_VISION_MODE, DV_MODE_IPT_TUNNEL, ConnectorType::CONN_TYPE_HDMI);
    usleep(100000);//100ms

    if (DISPLAY_TYPE_TV == mDisplayType) {
        setHdrMode(HDR_MODE_AUTO);
    }

    initGraphicsPriority();
}

void DisplayMode::disableDolbyVision(int DvMode) {
    char tvmode[MODE_LEN]   = {0};
    int  check_status_count = 0;
    int  dv_type            = DvMode;

    SYS_LOGI("dv_type %d", dv_type);
    strcpy(mHdmidata.dv_info.dv_enable, "0");

    //2. update sysfs
    char hdr_policy[MODE_LEN] = {0};
    getHdrStrategy(hdr_policy);

    if (strstr(hdr_policy, HDR_POLICY_SINK)) {
        DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_HDR_POLICY, HDR_POLICY_SINK, ConnectorType::CONN_TYPE_HDMI);
    } else if (strstr(hdr_policy, HDR_POLICY_SOURCE)) {
        DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_HDR_POLICY, HDR_POLICY_SOURCE, ConnectorType::CONN_TYPE_HDMI);
    }

    DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_DOLBY_VISION_POLICY, DV_POLICY_FORCE_MODE, ConnectorType::CONN_TYPE_HDMI);
    DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_DOLBY_VISION_MODE, DV_MODE_BYPASS, ConnectorType::CONN_TYPE_HDMI);
    usleep(100000);//100ms
    std::string dvstatus = "";
    DisplayModeMgr::getInstance().getDisplayAttribute(DISPLAY_DOLBY_VISION_STATUS, dvstatus, ConnectorType::CONN_TYPE_HDMI);

    if (strcmp(dvstatus.c_str(), BYPASS_PROCESS)) {
        while (++check_status_count <30) {
            usleep(20000);//20ms
            DisplayModeMgr::getInstance().getDisplayAttribute(DISPLAY_DOLBY_VISION_STATUS, dvstatus, ConnectorType::CONN_TYPE_HDMI);
            if (!strcmp(dvstatus.c_str(), BYPASS_PROCESS)) {
                break;
            }
        }
    }

    SYS_LOGI("dvstatus %s, check_status_count [%d]", dvstatus.c_str(), check_status_count);
    DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_DOLBY_VISION_ENABLE, DV_DISABLE, ConnectorType::CONN_TYPE_HDMI);

    if (DISPLAY_TYPE_TV == mDisplayType) {
        setHdrMode(HDR_MODE_AUTO);
    }

    setSdrMode(SDR_MODE_AUTO);
}

void DisplayMode::setDolbyVisionEnable(int state,  output_mode_state mode_state) {
    SYS_LOGI("%s dolby vision mode:%d", __FUNCTION__, state);

    if (DISPLAY_TYPE_TV == mDisplayType) {
        //1. update prop
        char tmp[10];
        sprintf(tmp, "%d", state);
        strcpy(mHdmidata.dv_info.ubootenv_dv_type, tmp);

        if (state == DOLBY_VISION_SET_DISABLE) {
            strcpy(mHdmidata.dv_info.dv_enable, "0");
        } else {
            strcpy(mHdmidata.dv_info.dv_enable, "1");
        }

        if (state == DOLBY_VISION_SET_DISABLE) {
            setTvDolbyVisionDisable();
        } else {
            setTvDolbyVisionEnable();
        }

        //save env
        setBootEnv(UBOOTENV_DV_ENABLE, mHdmidata.dv_info.dv_enable);
    } else {
        //1. update prop
        char tmp[10];
        sprintf(tmp, "%d", state);
        strcpy(mHdmidata.dv_info.ubootenv_dv_type, tmp);

        if (state == DOLBY_VISION_SET_DISABLE) {
            strcpy(mHdmidata.dv_info.dv_enable, "0");
        } else {
            strcpy(mHdmidata.dv_info.dv_enable, "1");
        }

        //2. update setting
        char cur_displaymode[MODE_LEN] = {0};
        getDisplayMode(cur_displaymode);
        setSourceOutputMode(cur_displaymode);
    }
}

bool DisplayMode::aisrContrl(bool on) {
    int ret = -1;
    if (on) {
       pSysWrite->setProperty(PROP_MEDIA_AISR, "true");
       ret = pSysWrite->writeSysfs(MEDIA_AISR_SYSFS, "1");
    } else {
       pSysWrite->setProperty(PROP_MEDIA_AISR, "false");
       ret = pSysWrite->writeSysfs(MEDIA_AISR_SYSFS, "0");
    }

    return  ret >= 0 ? true : false;

}

 bool DisplayMode::hasAisrFunc() {
    int ret = -1;
    ret = open(MEDIA_AISR_SYSFS, O_WRONLY);
    return ret >= 0 ? true : false;
 }

void DisplayMode::resetAisr() {
    int ret = -1;
    if (pSysWrite->getPropertyBoolean(PROP_MEDIA_AISR, "false")) {
        ret = pSysWrite->writeSysfs(MEDIA_AISR_SYSFS, "1");
    } else {
        ret = pSysWrite->writeSysfs(MEDIA_AISR_SYSFS, "0");
    }
    SYS_LOGD("resetAisr ret:%d",ret);
}

bool DisplayMode::getAisr() {
    return pSysWrite->getPropertyBoolean(PROP_MEDIA_AISR, "false");
}

void DisplayMode::getHdrStrategy(char* value) {
    char hdr_policy[MODE_LEN] = {0};

    memset(hdr_policy, 0, MODE_LEN);
    getBootEnv(UBOOTENV_HDR_POLICY, hdr_policy);

    if (strstr(hdr_policy, HDR_POLICY_SOURCE)) {
        strcpy(value, HDR_POLICY_SOURCE);
    } else {
        strcpy(value, HDR_POLICY_SINK);
    }
    SYS_LOGI("getHdrStrategy is [%s]", value);
}

bool DisplayMode::setAipqEnable(bool on) {
    int ret = -1;
    if (on) {
       pSysWrite->setProperty(PROP_MEDIA_AIPQ, "true");
       ret = pSysWrite->writeSysfs(MEDIA_AIPQ_SYSFS, "1");
    } else {
       pSysWrite->setProperty(PROP_MEDIA_AIPQ, "false");
       ret = pSysWrite->writeSysfs(MEDIA_AIPQ_SYSFS, "0");
    }
    return  ret >= 0 ? true : false;

}

 bool DisplayMode::hasAipqFunc() {
    int ret = -1;
    ret = open(MEDIA_AIPQ_SYSFS, O_WRONLY);
    return ret >= 0 ? true : false;
 }

void DisplayMode::resetAipq() {
    int ret = -1;
    if (pSysWrite->getPropertyBoolean(PROP_MEDIA_AIPQ, "false")) {
        ret = pSysWrite->writeSysfs(MEDIA_AIPQ_SYSFS, "1");
    } else {
        ret = pSysWrite->writeSysfs(MEDIA_AIPQ_SYSFS, "0");
    }
    SYS_LOGD("resetAipq ret:%d",ret);
}

bool DisplayMode::getAipqEnable() {
    return pSysWrite->getPropertyBoolean(PROP_MEDIA_AIPQ, "false");
}

void DisplayMode::setHdrStrategy(const char* type) {
    SYS_LOGI("policy:%s dv_type:%d", type, mHdmidata.dv_info.dv_type);

    char dvstatus[MODE_LEN] = {0};

    // 1. set dummy_l mode
    char cur_displaymode[MODE_LEN] = {0};
    getDisplayMode(cur_displaymode);
    setDisplayMode("dummy_l");

    //2. update sysfs and env policy
    if (strstr(type, HDR_POLICY_SINK)) {
        DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_HDR_POLICY, HDR_POLICY_SINK, ConnectorType::CONN_TYPE_HDMI);
        if (isDolbyVisionEnable()) {
            DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_DOLBY_VISION_POLICY, HDR_POLICY_SINK, ConnectorType::CONN_TYPE_HDMI);
            sprintf(dvstatus, "%d", mHdmidata.dv_info.dv_type);
            setBootEnv(UBOOTENV_DOLBYSTATUS, dvstatus);
        }
    } else if (strstr(type, HDR_POLICY_SOURCE)) {
        DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_HDR_POLICY, HDR_POLICY_SOURCE, ConnectorType::CONN_TYPE_HDMI);
        if (isDolbyVisionEnable()) {
            DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_DOLBY_VISION_POLICY, HDR_POLICY_SOURCE, ConnectorType::CONN_TYPE_HDMI);
            setBootEnv(UBOOTENV_DOLBYSTATUS, "0");
        }
    }
    setBootEnv(UBOOTENV_HDR_POLICY, (char *)type);

    // 3. set current hdmi mode
    setSourceOutputMode(cur_displaymode);

}

int DisplayMode::getHdrPriority(void) {
    char hdr_priority[MODE_LEN] = {0};
    hdr_priority_e value = DOLBY_VISION_PRIORITY;

    memset(hdr_priority, 0, MODE_LEN);
    getBootEnv(UBOOTENV_HDR_PRIORITY, hdr_priority);

    if (strstr(hdr_priority, "2")) {
        value = SDR_PRIORITY;
    } else if (strstr(hdr_priority, "1")) {
        value = HDR10_PRIORITY;
    } else {
        value = DOLBY_VISION_PRIORITY;
    }

    SYS_LOGI("getHdrPriority is [%d]", value);
    return (int)value;
}

void DisplayMode::setHdrPriority(const char* type) {
    SYS_LOGI("setHdrPriority is [%s]\n", type);

    setBootEnv(UBOOTENV_HDR_PRIORITY, (char *)type);

    if  (strstr(type, "2"))  {
        //1. get final display mode and color format
        setBootEnv(UBOOTENV_ISBESTMODE, "true");
        mHdmidata.state = OUPUT_MODE_STATE_INIT;
        getCommonData(&mHdmidata);
        sceneProcess(&mHdmidata);

        // 2. save uboot env
        //2.1 save hdmimode
        if (strstr(mHdmidata.final_displaymode, "cvbs") != NULL) {
            setBootEnv(UBOOTENV_CVBSMODE, mHdmidata.final_displaymode);
        } else if (strstr(mHdmidata.final_displaymode, "hz") != NULL) {
            setBootEnv(UBOOTENV_HDMIMODE, mHdmidata.final_displaymode);
        }
        //2.2 save colorattribute
        saveDeepColorAttr(mHdmidata.final_displaymode, mHdmidata.final_deepcolor);
        setBootEnv(UBOOTENV_COLORATTRIBUTE, mHdmidata.final_deepcolor);
    } else if  (strstr(type, "1")) {
        //1. get final display mode and color format
        setBootEnv(UBOOTENV_ISBESTMODE, "true");
        mHdmidata.state = OUPUT_MODE_STATE_INIT;
        getCommonData(&mHdmidata);
        sceneProcess(&mHdmidata);

        // 2. save uboot env
        //2.1 save hdmimode
        if (strstr(mHdmidata.final_displaymode, "cvbs") != NULL) {
            setBootEnv(UBOOTENV_CVBSMODE, mHdmidata.final_displaymode);
        } else if (strstr(mHdmidata.final_displaymode, "hz") != NULL) {
            setBootEnv(UBOOTENV_HDMIMODE, mHdmidata.final_displaymode);
        }
        //2.2 save colorattribute
        saveDeepColorAttr(mHdmidata.final_displaymode, mHdmidata.final_deepcolor);
        setBootEnv(UBOOTENV_COLORATTRIBUTE, mHdmidata.final_deepcolor);
    } else {
        char hdr_policy[MODE_LEN] = {0};
        std::string dv_cap;
        DisplayModeMgr::getInstance().getDisplayAttribute(DISPLAY_DOLBY_VISION_CAP2, dv_cap, ConnectorType::CONN_TYPE_HDMI);
        SYS_LOGI("This TV dv cap: %s", dv_cap.c_str());
        if (!(strstr(dv_cap.c_str(), "The Rx don't support DolbyVision")) &&
            (isMboxSupportDolbyVision() == true)) {
            getHdrStrategy(hdr_policy);
            if (!strcmp(hdr_policy, HDR_POLICY_SOURCE)) {
                setBootEnv(UBOOTENV_DOLBYSTATUS, "0");
            } else {
                if (strstr(dv_cap.c_str(), "2160p60hz")) {
                    setBootEnv(UBOOTENV_HDMIMODE, "2160p60hz");
                } else if (strstr(dv_cap.c_str(), "2160p30hz") || strstr(dv_cap.c_str(), "2160p25z")
                    || strstr(dv_cap.c_str(), "2160p24hz") || strstr(dv_cap.c_str(), "1080p60hz")) {
                    setBootEnv(UBOOTENV_HDMIMODE, "1080p60hz");
                } else {
                        SYS_LOGI("This DV-TV is special case: %s", dv_cap.c_str());
                        return;
                }

                if (strstr(dv_cap.c_str(), "LL_YCbCr_422_12BIT") || strstr(dv_cap.c_str(), "DV_RGB_444_8BIT")) {
                    if (pSysWrite->getPropertyBoolean(PROP_ALWAYS_DOLBY_VISION, false)) {
                        if (strstr(dv_cap.c_str(), "DV_RGB_444_8BIT")) {
                            setBootEnv(UBOOTENV_COLORATTRIBUTE, "444,8bit");
                            setBootEnv(UBOOTENV_DOLBYSTATUS, "1");
                        } else if (strstr(dv_cap.c_str(), "LL_YCbCr_422_12BIT")) {
                            setBootEnv(UBOOTENV_COLORATTRIBUTE, "422,12bit");
                            setBootEnv(UBOOTENV_DOLBYSTATUS, "2");
                        }
                    } else {
                        if (strstr(dv_cap.c_str(), "LL_YCbCr_422_12BIT")) {
                            setBootEnv(UBOOTENV_COLORATTRIBUTE, "422,12bit");
                            setBootEnv(UBOOTENV_DOLBYSTATUS, "2");
                        } else if (strstr(dv_cap.c_str(), "DV_RGB_444_8BIT")) {
                            setBootEnv(UBOOTENV_COLORATTRIBUTE, "444,8bit");
                            setBootEnv(UBOOTENV_DOLBYSTATUS, "1");
                        }
                    }
                } else if (strstr(dv_cap.c_str(), "LL_RGB_444_12BIT")) {
                    setBootEnv(UBOOTENV_COLORATTRIBUTE, "444,12bit");
                    setBootEnv(UBOOTENV_DOLBYSTATUS, "3");
                } else if (strstr(dv_cap.c_str(), "LL_RGB_444_10BIT")) {
                    setBootEnv(UBOOTENV_COLORATTRIBUTE, "444,10bit");
                    setBootEnv(UBOOTENV_DOLBYSTATUS, "3");
                }
            }
        }
    }
}

void DisplayMode::initDolbyVision(output_mode_state state) {
    int dv_type = DOLBY_VISION_SET_DISABLE;

    SYS_LOGI("state:%d\n", state);

    dv_type = updateDolbyVisionType();

    if (getCurDolbyVisionState(dv_type,  state)) {
        SYS_LOGI("Current dolby vision type is same as the set value\n");
        return;
    }

    setDolbyVisionEnable(dv_type,  state);
}

void DisplayMode::setDolbyVisionSupport() {
    char dvFile[100] = {0};

    if (DISPLAY_TYPE_TV == mDisplayType) {
        strcpy(dvFile, DOLBY_VISION_KO_DIR_TV);
    } else {
        strcpy(dvFile, DOLBY_VISION_KO_DIR);
    }

    if ((pSysWrite->getPropertyBoolean(PROP_DOLBY_VISION_FEATURE, false))
            && (access(dvFile, 0) == 0)) {
        pSysWrite->setProperty(PROP_SUPPORT_DOLBY_VISION, "true");
    } else {
        pSysWrite->setProperty(PROP_SUPPORT_DOLBY_VISION, "false");
    }
}

bool DisplayMode::getCurDolbyVisionState(int state, output_mode_state mode_state) {
    if ((mode_state != OUPUT_MODE_STATE_INIT)
            || checkDolbyVisionStatusChanged(state)
            || checkDolbyVisionDeepColorChanged(state)) {
        return false;
    }
    return true;
}

bool DisplayMode::checkDolbyVisionDeepColorChanged(int state) {
    std::string colorAttr;
    char mode[MAX_STR_LEN] = {0};
    DisplayModeMgr::getInstance().getDisplayAttribute(DISPLAY_HDMI_COLOR_ATTR, colorAttr);
    if (isTvSupportDolbyVision(mode) && (state == DOLBY_VISION_SET_ENABLE)
            && (strstr(colorAttr.c_str(), "444,8bit") == NULL)) {
        SYS_LOGI("colorAttr %s is not match with DV STD\n", colorAttr.c_str());
        return true;
    } else if ((state == DOLBY_VISION_SET_ENABLE_LL_YUV) && (strstr(colorAttr.c_str(), "422,12bit") == NULL)) {
        SYS_LOGI("colorAttr %s is not match with DV LL YUV\n", colorAttr.c_str());
        return true;
    } else if ((state == DOLBY_VISION_SET_ENABLE_LL_RGB) && (strstr(colorAttr.c_str(), "444,12bit") == NULL)
                && (strstr(colorAttr.c_str(), "444,10bit") == NULL)) {
        SYS_LOGI("colorAttr %s is not match with DV LL RGB\n", colorAttr.c_str());
        return true;
    }
    return false;
}

/* *
 * @Description: get Current State DolbyVision State
 *
 * @result: if disable Dolby Vision return DOLBY_VISION_SET_DISABLE.
 *          if Current TV support the state saved in Mbox. return that state. like value of saved is 2, and TV support LL_YUV
 *          if Current TV not support the state saved in Mbox. but isDolbyVisionEnable() is enable.
 *             return state in priority queue. like value of saved is 2, But TV only Support LL_RGB, so system will return LL_RGB
 */
int DisplayMode::getDolbyVisionType() {
    int dv_type;
    char dv_mode[MAX_STR_LEN];

    if (isTvSupportDolbyVision(dv_mode) && (mHdmidata.hdr_priority == DOLBY_VISION_PRIORITY)) {
        //1. read dolby vision mode from prop(maybe need to env)
        dv_type = mHdmidata.dv_info.dv_type;
        SYS_LOGI("dv_type %d tv dolby vision mode:%s\n", dv_type, dv_mode);

        //2. check tv support or not
        if ((dv_type == 1) && strstr(dv_mode, "DV_RGB_444_8BIT") != NULL) {
            return DOLBY_VISION_SET_ENABLE;
        } else if ((dv_type == 2) && strstr(dv_mode, "LL_YCbCr_422_12BIT") != NULL) {
            return DOLBY_VISION_SET_ENABLE_LL_YUV;
        } else if ((dv_type == 3)
                && ((strstr(dv_mode, "LL_RGB_444_12BIT") != NULL) || (strstr(dv_mode, "LL_RGB_444_10BIT") != NULL))) {
            return DOLBY_VISION_SET_ENABLE_LL_RGB;
        } else if (dv_type == 0) {
            return DOLBY_VISION_SET_DISABLE;
        }

        //3. dolby vision best policy:STD->LL_YUV->LL_RGB for netflix request
        //   dolby vision best policy:LL_YUV->STD->LL_RGB for dolby vision request
        if ((strstr(dv_mode, "DV_RGB_444_8BIT") != NULL) || (strstr(dv_mode, "LL_YCbCr_422_12BIT") != NULL)) {
            if (pSysWrite->getPropertyBoolean(PROP_ALWAYS_DOLBY_VISION, false)) {
                if (strstr(dv_mode, "DV_RGB_444_8BIT") != NULL) {
                    return DOLBY_VISION_SET_ENABLE;
                } else if (strstr(dv_mode, "LL_YCbCr_422_12BIT") != NULL) {
                    return DOLBY_VISION_SET_ENABLE_LL_YUV;
                }
            } else {
                if (strstr(dv_mode, "LL_YCbCr_422_12BIT") != NULL) {
                    return DOLBY_VISION_SET_ENABLE_LL_YUV;
                } else if (strstr(dv_mode, "DV_RGB_444_8BIT") != NULL) {
                    return DOLBY_VISION_SET_ENABLE;
                }
            }
        } else if ((strstr(dv_mode, "LL_RGB_444_12BIT") != NULL) || (strstr(dv_mode, "LL_RGB_444_10BIT") != NULL)) {
            return DOLBY_VISION_SET_ENABLE_LL_RGB;
        }
    } else {
        // enable dolby vision core
        if (isDolbyVisionEnable()) {
            return DOLBY_VISION_SET_ENABLE;
        }
    }

    return DOLBY_VISION_SET_DISABLE;
}

int DisplayMode::updateDolbyVisionType(void) {
    char type[MODE_LEN];

    //1. read dolby vision mode from prop(maybe need to env)
    strcpy(type, mHdmidata.dv_info.ubootenv_dv_type);
    SYS_LOGI("type %s tv dolby vision mode:%s\n", type, mHdmidata.dv_info.dv_deepcolor);

    //2. check tv support or not
    if ((strstr(type, "1") != NULL) && strstr(mHdmidata.dv_info.dv_deepcolor, "DV_RGB_444_8BIT") != NULL) {
        return DOLBY_VISION_SET_ENABLE;
    } else if ((strstr(type, "2") != NULL) && strstr(mHdmidata.dv_info.dv_deepcolor, "LL_YCbCr_422_12BIT") != NULL) {
        return DOLBY_VISION_SET_ENABLE_LL_YUV;
    } else if ((strstr(type, "3") != NULL)
        && ((strstr(mHdmidata.dv_info.dv_deepcolor, "LL_RGB_444_12BIT") != NULL) ||
        (strstr(mHdmidata.dv_info.dv_deepcolor, "LL_RGB_444_10BIT") != NULL))) {
        return DOLBY_VISION_SET_ENABLE_LL_RGB;
    } else if (strstr(type, "0") != NULL) {
        return DOLBY_VISION_SET_DISABLE;
    }

    //3. dolby vision best policy:STD->LL_YUV->LL_RGB for netflix request
    //   dolby vision best policy:LL_YUV->STD->LL_RGB for dolby vision request
    if ((strstr(mHdmidata.dv_info.dv_deepcolor, "DV_RGB_444_8BIT") != NULL) ||
        (strstr(mHdmidata.dv_info.dv_deepcolor, "LL_YCbCr_422_12BIT") != NULL)) {
        if (pSysWrite->getPropertyBoolean(PROP_ALWAYS_DOLBY_VISION, false)) {
            if (strstr(mHdmidata.dv_info.dv_deepcolor, "DV_RGB_444_8BIT") != NULL) {
                return DOLBY_VISION_SET_ENABLE;
            } else if (strstr(mHdmidata.dv_info.dv_deepcolor, "LL_YCbCr_422_12BIT") != NULL) {
                return DOLBY_VISION_SET_ENABLE_LL_YUV;
            }
        } else {
            if (strstr(mHdmidata.dv_info.dv_deepcolor, "LL_YCbCr_422_12BIT") != NULL) {
                return DOLBY_VISION_SET_ENABLE_LL_YUV;
            } else if (strstr(mHdmidata.dv_info.dv_deepcolor, "DV_RGB_444_8BIT") != NULL) {
                return DOLBY_VISION_SET_ENABLE;
            }
        }
    } else if ((strstr(mHdmidata.dv_info.dv_deepcolor, "LL_RGB_444_12BIT") != NULL) ||
        (strstr(mHdmidata.dv_info.dv_deepcolor, "LL_RGB_444_10BIT") != NULL)) {
        return DOLBY_VISION_SET_ENABLE_LL_RGB;
    }

    return DOLBY_VISION_SET_DISABLE;
}

bool DisplayMode::isTvSupportALLM() {
    char allm_mode_cap[PROP_VALUE_MAX];
    memset(allm_mode_cap, 0, PROP_VALUE_MAX);
    int ret = 0;

    pSysWrite->readSysfs(AUTO_LOW_LATENCY_MODE_CAP, allm_mode_cap);

    for (int i = 0; i < ARRAY_SIZE(ALLM_MODE_CAP); i++) {
        if (!strncmp(allm_mode_cap, ALLM_MODE_CAP[i], strlen(ALLM_MODE_CAP[i]))) {
            ret = i;
        }
    }

    return (ret == 1) ? true : false;
}

bool DisplayMode::getContentTypeSupport(const char* type) {
    char content_type_cap[MAX_STR_LEN] = {0};
    pSysWrite->readSysfs(HDMI_CONTENT_TYPE_CAP, content_type_cap);
    if (strstr(content_type_cap, type)) {
        SYS_LOGI("getContentTypeSupport: %s is true", type);
        return true;
    }

    SYS_LOGI("getContentTypeSupport: %s is false", type);
    return false;
}

bool DisplayMode::getGameContentTypeSupport() {
    return getContentTypeSupport(CONTENT_TYPE_CAP[3]);
}

bool DisplayMode::getSupportALLMContentTypeList(std::vector<std::string> *supportModes) {
    if (isTvSupportALLM()) {
        (*supportModes).push_back(std::string("allm"));
    }

    for (int i = 0; i < ARRAY_SIZE(CONTENT_TYPE_CAP); i++) {
        if (getContentTypeSupport(CONTENT_TYPE_CAP[i])) {
            (*supportModes).push_back(std::string(CONTENT_TYPE_CAP[i]));
        }
    }

    return true;
}

/* *
 * @Description: this is a temporary solution, should be revert when android.hardware.graphics.composer@2.4 finished
 *               set the ALLM_Mode
 * @params: "0": ALLM disable (VSIF still contain allm info)
 *          "1": ALLM enable
 *          "-1":really disable ALLM (VSIF don't contain allm info)
 * */
void DisplayMode::setALLMMode(int state) {
    /***************************************************************
     *         Comment for special solution in this func           *
     ***************************************************************
     *                                                             *
     * In HDMI Standard only 0 to disable ALLM and 1 to enable ALLM*
     * but ALLM and Dobly Vision share the same bit in VSIF        *
     * it cause conflict                                           *
     *                                                             *
     * So in amlogic special solution:                             *
     * we add -1 to                                                *
     *     1: disable ALLM                                         *
     *     2: clean ALLM info in VSIF conflict bit                 *
     * when user set 0 to ALLM                                     *
     * we will froce change 0 into -1 here                         *
     *                                                             *
     ***************************************************************/

    int perState = pSysWrite->getPropertyInt("persist.vendor.sys.display.allm", -1);
    if (perState == state) {
        SYS_LOGI("setALLMMode: the ALLM_Mode is not changed");
        return;
    }

    char dv_mode[MAX_STR_LEN];
    bool isTVSupportDV = isTvSupportDolbyVision(dv_mode);

    switch (state) {
        case -1:
            [[fallthrough]];
        case 0:
            pSysWrite->writeSysfs(AUTO_LOW_LATENCY_MODE, ALLM_MODE[0]);
            pSysWrite->setProperty("persist.vendor.sys.display.allm", ALLM_MODE[0]);
            SYS_LOGI("setALLMMode: ALLM_Mode: %s", ALLM_MODE[0]);

            if (isTVSupportDV) {
                // reset doblyvision when set -1/0 to ALLM
                //setBootEnv(UBOOTENV_BESTDOLBYVISION, "true");
                //initDolbyVision(OUPUT_MODE_STATE_SWITCH);
                setDolbyVisionEnable(DOLBY_VISION_SET_ENABLE,OUPUT_MODE_STATE_SWITCH);
            }
            break;
        case 1:
            pSysWrite->writeSysfs(AUTO_LOW_LATENCY_MODE, ALLM_MODE[2]);
            pSysWrite->setProperty("persist.vendor.sys.display.allm", ALLM_MODE[2]);
            SYS_LOGI("setALLMMode: ALLM_Mode: %s", ALLM_MODE[2]);

            if (isTVSupportDV && isDolbyVisionEnable()) {
                // disable the doblyvision when ALLM enable
               // setBootEnv(UBOOTENV_BESTDOLBYVISION, "false");
                setDolbyVisionEnable(DOLBY_VISION_SET_DISABLE,OUPUT_MODE_STATE_SWITCH);
            }
            break;
        default:
            SYS_LOGE("setALLMMode: ALLM_Mode: error state[%d]", state);
            break;
    }
}

/* *
 * @Description: this is a temporary solution, should be revert when android.hardware.graphics.composer@2.4 finished
 *               set the ALLM_Mode
 * @params: "0": GameContentType is not supported or not active
 *          "1": enable the Sink's GameContentType : graphics
 *          "2": enable the Sink's GameContentType : photo
 *          "3": enable the Sink's GameContentType : cinema
 *          "4": enable the Sink's GameContentType : game
 * */
void DisplayMode::sendHDMIContentType(int state) {
    if (state < ARRAY_SIZE(CONTENT_TYPE)) {
        pSysWrite->writeSysfs(HDMI_CONTENT_TYPE, CONTENT_TYPE[state]);
        SYS_LOGI("sendGameContentType: GameContentType: %s", CONTENT_TYPE[state]);
    } else {
        SYS_LOGE("sendGameContentType: GameContentType: error index[%d]", state);
    }
}

/* *
 * @Description: set dolby vision graphics priority only when dolby vision enable.
 * @params: "0": Video Priority    "1": Graphics Priority
 * */
void DisplayMode::setGraphicsPriority(const char* mode) {
    if (NULL != mode) {
        SYS_LOGI("setGraphicsPriority [%s]", mode);
    }
    if ((NULL != mode) && (atoi(mode) == 0 || atoi(mode) == 1)) {
        DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_DOLBY_VISION_GRAPHICS_PRIORITY, mode, ConnectorType::CONN_TYPE_HDMI);
        pSysWrite->setProperty(PROP_DOLBY_VISION_PRIORITY, mode);
        SYS_LOGI("setGraphicsPriority [%s]",
                atoi(mode) == 0 ? "Video Priority" : "Graphics Priority");
    } else {
        DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_DOLBY_VISION_GRAPHICS_PRIORITY, "0", ConnectorType::CONN_TYPE_HDMI);
        pSysWrite->setProperty(PROP_DOLBY_VISION_PRIORITY, "0");
        SYS_LOGI("setGraphicsPriority default [Video Priority]");
    }
}

/* *
 * @Description: get dolby vision graphics priority.
 * @params: store current priority mode.
 * */
void DisplayMode::getGraphicsPriority(char* mode) {
    pSysWrite->getPropertyString(PROP_DOLBY_VISION_PRIORITY, mode, "0");
    SYS_LOGI("getGraphicsPriority [%s]",
        atoi(mode) == 0 ? "Video Priority" : "Graphics Priority");
}

/* *
 * @Description: init dolby vision graphics priority when bootup.
 * */
void DisplayMode::initGraphicsPriority() {
    char mode[MODE_LEN] = {0};
    pSysWrite->getPropertyString(PROP_DOLBY_VISION_PRIORITY, mode, "1");
    DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_DOLBY_VISION_GRAPHICS_PRIORITY, mode, ConnectorType::CONN_TYPE_HDMI);
    pSysWrite->setProperty(PROP_DOLBY_VISION_PRIORITY, mode);
}

/* *
 * @Description: set hdr mode
 * @params: mode "0":off "1":on "2":auto
 * */
void DisplayMode::setHdrMode(const char* mode) {
    if ((atoi(mode) >= 0) && (atoi(mode) <= 2)) {
        SYS_LOGI("setHdrMode state: %s\n", mode);
        DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_HDR_MODE, mode, ConnectorType::CONN_TYPE_HDMI);
        pSysWrite->setProperty(PROP_HDR_MODE_STATE, mode);
    }
}

/* *
 * @Description: set sdr mode
 * @params: mode "0":off "2":auto
 * */
void DisplayMode::setSdrMode(const char* mode) {
    if ((atoi(mode) == 0) || atoi(mode) == 2) {
        SYS_LOGI("setSdrMode state: %s\n", mode);
        DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_SDR_MODE, mode, ConnectorType::CONN_TYPE_HDMI);
        pSysWrite->setProperty(PROP_SDR_MODE_STATE, mode);
        setBootEnv(UBOOTENV_SDR2HDR, (char *)mode);
    }
}

void DisplayMode::initHdrSdrMode() {
    char mode[MODE_LEN] = {0};
    pSysWrite->getPropertyString(PROP_HDR_MODE_STATE, mode, HDR_MODE_AUTO);
    setHdrMode(mode);
    memset(mode, 0, sizeof(mode));
    bool flag = pSysWrite->getPropertyBoolean(PROP_ENABLE_SDR2HDR, false);
    if (flag & isDolbyVisionEnable()) {
        strcpy(mode, SDR_MODE_OFF);
    } else {
        pSysWrite->getPropertyString(PROP_SDR_MODE_STATE, mode, flag ? SDR_MODE_AUTO : SDR_MODE_OFF);
    }
    setSdrMode(mode);
}

int DisplayMode::modeToIndex(const char *mode) {
    int index = DISPLAY_MODE_1080P;
    for (int i = 0; i < DISPLAY_MODE_TOTAL; i++) {
        if (!strcmp(mode, DISPLAY_MODE_LIST[i])) {
            index = i;
            break;
        }
    }

    //SYS_LOGI("modeToIndex mode:%s index:%d", mode, index);
    return index;
}

void DisplayMode::isHDCPTxAuthSuccess(int *status) {
#ifndef RECOVERY_MODE
    pTxAuth->isAuthSuccess(status);
#endif
}

void DisplayMode::onTxEvent (char* switchName, char* hpdstate, int outputState) {
    SYS_LOGI("onTxEvent switchName:%s hpdstate:%s state: %d\n", switchName, hpdstate, outputState);
#ifndef RECOVERY_MODE
    DisplayModeMgr::getInstance().updateConnectorType();

    if (!strcmp(switchName, HDMI_UEVENT_HDMI_AUDIO) || !strcmp(switchName, UEVENT_HDMI_AUDIO)) {
        notifyEvent(hpdstate[0] == '1' ? EVENT_HDMI_AUDIO_IN : EVENT_HDMI_AUDIO_OUT);
        return;
    }
    if (hpdstate) {
        if (hpdstate[0] == '1' && !strcmp(mHdmidata.dv_info.dv_enable, "0")) {
            char temp[EDID_MAX_SIZE] = {0};
            pSysWrite->readSysfs(DISPLAY_EDID_RAW, temp);
            if (memcmp(mEdid, temp, EDID_MAX_SIZE)) {
                setBootEnv(UBOOTENV_BESTDOLBYVISION, "true");
                memcpy(mEdid, temp, EDID_MAX_SIZE);
            }
        }

        if (hpdstate[0] == '1')
            dumpCaps();
    }
#endif
    //plugout hdmi
    if (hpdstate && hpdstate[0] == '0') {
        if (isVMXCertification()) {
            setDisplayMode("576cvbs");
        } else {
            setDisplayMode("dummy_l");
        }
        return;
    }

    //hdmi edid parse error
    if ((isHdmiEdidParseOK() == false) &&
        (isHdmiHpd() == true)) {
        pSysWrite->writeSysfs(DISPLAY_HDMI_AVMUTE_SYSFS, "1");
        DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_HDMI_COLOR_ATTR, COLOR_RGB_8BIT);

        //set hdmi mode
        setDisplayMode(DEFAULT_OUTPUT_MODE);
        //update display position
        int position[4] = { 0, 0, 0, 0 };//x,y,w,h
        getPosition(DEFAULT_OUTPUT_MODE, position);
        setPosition(DEFAULT_OUTPUT_MODE, position[0], position[1],position[2], position[3]);
        pSysWrite->writeSysfs(DISPLAY_HDMI_AVMUTE_SYSFS, "-1");
        return;
    }

    setSourceDisplay((output_mode_state)outputState);

    //notify framework hdmi plug uevent
#ifndef RECOVERY_MODE
    if (hpdstate) {
        notifyEvent((hpdstate[0] == '1') ? EVENT_HDMI_PLUG_IN : EVENT_HDMI_PLUG_OUT);
    }
#endif

}
bool DisplayMode::frameRateDisplay(bool on) {
    pFrameRateAutoAdaption->setVideoLayerOn(on);
    return true;
}
void DisplayMode::onDispModeSyncEvent (const char* outputmode, int state) {
    SYS_LOGI("onDispModeSyncEvent outputmode:%s state: %d\n", outputmode, state);
    setSourceOutputMode(outputmode, (output_mode_state)state);
}

//for debug
void DisplayMode::hdcpSwitch() {
    SYS_LOGI("hdcpSwitch for debug hdcp authenticate\n");
}

#ifndef RECOVERY_MODE
void DisplayMode::notifyEvent(int event) {
    if (mNotifyListener != NULL) {
        mNotifyListener->onEvent(event);
    }
}

void DisplayMode::setListener(const sp<SystemControlNotify>& listener) {
    mNotifyListener = listener;
}
#endif

void DisplayMode::dumpCap(const char * path, const char * hint, char *result) {
    char logBuf[MAX_STR_LEN];
    pSysWrite->readSysfsOriginal(path, logBuf);

    if (mLogLevel > LOG_LEVEL_0)
        SYS_LOGI("%s%s", hint, logBuf);

    if (NULL != result) {
        strcat(result, hint);
        strcat(result, logBuf);
        strcat(result, "\n");
    }
}

void DisplayMode::dumpCaps(char *result) {
    dumpCap(DISPLAY_EDID_STATUS, "\nEDID parsing status: ", result);
    dumpCap(DISPLAY_EDID_VALUE, "General caps\n", result);
    dumpCap(DISPLAY_HDMI_DEEP_COLOR, "Deep color\n", result);
    dumpCap(DISPLAY_HDMI_HDR, "HDR\n", result);
    dumpCap(DISPLAY_HDMI_MODE_PREF, "Preferred mode: ", result);
    dumpCap(DISPLAY_HDMI_SINK_TYPE, "Sink type: ", result);
    dumpCap(DISPLAY_HDMI_AUDIO, "Audio caps\n", result);
    dumpCap(DISPLAY_EDID_RAW, "Raw EDID\n", result);
}

int DisplayMode::dump(char *result) {
    if (NULL == result)
        return -1;

    char buf[2048] = {0};
    sprintf(buf, "\ndisplay type: %d [0:none 1:tablet 2:mbox 3:tv], soc type:%s\n", mDisplayType, mSocType);
    strcat(result, buf);

    if ((DISPLAY_TYPE_MBOX == mDisplayType) || (DISPLAY_TYPE_REPEATER == mDisplayType)) {
        sprintf(buf, "default ui:%s\n", mDefaultUI);
        strcat(result, buf);
        dumpCaps(result);
    }
    return 0;
}

bool DisplayMode::checkDolbyVisionStatusChanged(int state) {
    std::string curDvEnable = "";
    std::string curDvLLPolicy = "";
    int curDvMode = -1;

    DisplayModeMgr::getInstance().getDisplayAttribute(DISPLAY_DOLBY_VISION_ENABLE, curDvEnable, ConnectorType::CONN_TYPE_HDMI);
    DisplayModeMgr::getInstance().getDisplayAttribute(DISPLAY_DOLBY_VISION_LL_POLICY, curDvLLPolicy, ConnectorType::CONN_TYPE_HDMI);

    if (!strcmp(curDvEnable.c_str(), DV_DISABLE) ||
        !strcmp(curDvEnable.c_str(), "0"))
        curDvMode = DOLBY_VISION_SET_DISABLE;
    else if (!strcmp(curDvLLPolicy.c_str(), "0"))
        curDvMode = DOLBY_VISION_SET_ENABLE;
    else if (!strcmp(curDvLLPolicy.c_str(), "1"))
        curDvMode = DOLBY_VISION_SET_ENABLE_LL_YUV;
    else if (!strcmp(curDvLLPolicy.c_str(), "2"))
        curDvMode = DOLBY_VISION_SET_ENABLE_LL_RGB;

    SYS_LOGI("curDvMode %d, want DvMode %d\n", curDvMode, state);

    if (curDvMode != state) {
        return true;
    } else {
        return false;
    }
}
void DisplayMode::saveHdmiParamToEnv(){
    std::string colorAttr;
    char colorDepth[MODE_LEN] = {0};
    char colorSpace[MODE_LEN] = {0};
    char outputMode[MODE_LEN] = {0};
    char dvstatus[MODE_LEN]   = {0};
    char dv_type[MODE_LEN]    = {0};
    char hdr_policy[MODE_LEN] = {0};

    getDisplayMode(outputMode);

    // 1. check whether the TV changed or not, if changed save crc
    if (isEdidChange()) {
        SYS_LOGD("tv sink changed\n");
    }

    // 2. save rawEdid/coloattr/hdmimode to bootenv if mode is not null
    //if the value we try to save is the same with the last saved value,then Logoparam will prohibited to write
    if (strcmp(outputMode, "null") || strcmp(outputMode, "dummy_l")) {
        // 2.1 save color attr
        DisplayModeMgr::getInstance().getDisplayAttribute(DISPLAY_HDMI_COLOR_ATTR, colorAttr);
        saveDeepColorAttr(outputMode, colorAttr.c_str());
        setBootEnv(UBOOTENV_COLORATTRIBUTE, colorAttr.c_str());
        //colorDepth&&colorSpace is used for uboot hdmi to find
        //best color attributes for the selected hdmi mode when TV changed
        pSysWrite->getPropertyString(PROP_DEEPCOLOR_CTL, colorDepth, "8");
        pSysWrite->getPropertyString(PROP_PIXFMT, colorSpace, "auto");
        setBootEnv(UBOOTENV_HDMICOLORDEPTH, colorDepth);
        setBootEnv(UBOOTENV_HDMICOLORSPACE, colorSpace);

        // 2.2 save output mode
        if (DISPLAY_TYPE_TABLET != mDisplayType) {
            setBootEnv(UBOOTENV_OUTPUTMODE, (char *)outputMode);
        }

        if (strstr(outputMode, "cvbs") != NULL) {
            setBootEnv(UBOOTENV_CVBSMODE, (char *)outputMode);
        } else if (strstr(outputMode, "hz") != NULL) {
            setBootEnv(UBOOTENV_HDMIMODE, (char *)outputMode);
        }

        // 2.3 save dolby status/dv_type
        // In follow sink mode: 0:disable 1:STD(or enable dv) 2:LL YUV 3: LL RGB
        // In follow source mode: dv is diable in uboot.
        if (isMboxSupportDolbyVision()) {
            getHdrStrategy(hdr_policy);
            if (!strcmp(hdr_policy, HDR_POLICY_SOURCE)) {
                sprintf(dvstatus, "%d", 0);
            } else {
                sprintf(dvstatus, "%d", mHdmidata.dv_info.dv_type);
            }
            setBootEnv(UBOOTENV_DOLBYSTATUS, dvstatus);

            sprintf(dv_type, "%d", mHdmidata.dv_info.dv_type);
            setBootEnv(UBOOTENV_DV_TYPE, dv_type);

            setBootEnv(UBOOTENV_DV_ENABLE, mHdmidata.dv_info.dv_enable);

            SYS_LOGI("dvstatus %s dv_type %s dv_enable %s\n",
                dvstatus, dv_type, mHdmidata.dv_info.dv_enable);

        } else {
            SYS_LOGI("MBOX is not support dolby vision, dvstatus %s dv_type %d dv_enable %s\n",
                dvstatus, mHdmidata.dv_info.dv_type, mHdmidata.dv_info.dv_enable);
        }

        SYS_LOGI("colorattr: %s, outputMode %s, cd %s, cs %s\n",
            colorAttr.c_str(), outputMode, colorDepth, colorSpace);
    }
    // for debugging, print all logoparam
    if (pSysWrite->getPropertyBoolean("persist.systemcontrol.debug", false))
        mUbootenv->printValues();
}

/* *
 * @Description: get perf hdmi display mode priority.
 * @params: store current perf hdmi mode.
 * */
bool DisplayMode::getPrefHdmiDispMode(char* mode) {
    bool ret = true;

    //1. get hdmi data
    memset(&mHdmidata, 0, sizeof(hdmi_data_t));
    mHdmidata.state = OUPUT_MODE_STATE_INIT;
    getHdmiData(&mHdmidata);

    //2. scene logic process
    sceneProcess(&mHdmidata);

    strcpy(mode, mHdmidata.final_displaymode);

    SYS_LOGI("getPrefHdmiDispMode [%s]", mode);
    return ret;
}


bool DisplayMode::memcContrl(bool on) {
    int memDev = open(DISPLAY_MEMC_SYSFS, O_WRONLY);
    if (memDev < 0 ) {
         SYS_LOGE("can't open device /dev/frc %s",strerror(errno));
         return false;
    }
    int value = 0;
    if (on) {
       pSysWrite->setProperty(PROP_DISPLAY_MEMC, "true");
       value = 1;
    } else {
       pSysWrite->setProperty(PROP_DISPLAY_MEMC, "false");
    }

    ioctl(memDev, MEMDEV_CONTRL, &value);
    return true;

}

void DisplayMode::resetMemc() {
    int memDev = open(DISPLAY_MEMC_SYSFS, O_WRONLY);
    if (pSysWrite->getPropertyBoolean(PROP_DISPLAY_MEMC, false) && memDev > 0) {
        int value = 1;
        ioctl(memDev, MEMDEV_CONTRL, &value);
    }
}
