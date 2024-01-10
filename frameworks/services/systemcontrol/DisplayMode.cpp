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
};

static const char* DV_MODE_TYPE[] = {
    "DV_RGB_444_8BIT",
    "DV_YCbCr_422_12BIT",
    "LL_YCbCr_422_12BIT",
    "LL_RGB_444_12BIT",
    "LL_RGB_444_10BIT"
};

/*
 * 0:parse original tv edid,hdmi output dv/hdr/sdr signal
 * 1:mark dv capability,hdmi only output hdr/sdr signal
 * 2:mark dv and hdr capability,hdmi always output sdr signal
 */
static const char* HDR_PRIORITY_TYPE[] = {
    "0",
    "1",
    "2"
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

#define SYS_BOOT_COMPLETE       "/sys/class/tee_info/sys_boot_complete"
#define HWC_BOOT_CONFIG_PROP "ro.vendor.hwc.default.config"

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
    pConfigPath = DISPLAY_CFG_FILE;
    mDisplayType = DISPLAY_TYPE_MBOX;
    mDisplayWidth = FULL_WIDTH_1080;
    mDisplayHeight = FULL_HEIGHT_1080;
    mLogLevel = LOG_LEVEL_DEFAULT;
    memset(&mHdmidata, 0, sizeof(hdmi_data_t));
    mScene_output_info.dv_type = DOLBY_VISION_SET_DISABLE;
    strcpy(mScene_output_info.final_displaymode, DEFAULT_HDMI_MODE);
    strcpy(mScene_output_info.final_deepcolor, DEFAULT_COLOR_FORMAT);
    DisplayMode(path, NULL);
}

static int set_sys_boot_complete(void)
{
    int fd;
    int len;
    char buf[] = "1";

    fd = open(SYS_BOOT_COMPLETE, O_WRONLY);
    if (fd < 0) {
        SYS_LOGE("open %s failed", SYS_BOOT_COMPLETE);
        return -1;
    }

    len = write(fd, buf, sizeof(buf));

    close(fd);

    if (len != sizeof(buf))
        return -1;
    else
        return 0;
}

DisplayMode::DisplayMode(const char *path, Ubootenv *ubootenv)
    :mDisplayType(DISPLAY_TYPE_MBOX),
    mEnvLock(PTHREAD_MUTEX_INITIALIZER),
    mDisplayWidth(FULL_WIDTH_1080),
    mDisplayHeight(FULL_HEIGHT_1080),
    mLogLevel(LOG_LEVEL_DEFAULT) {

    memset(&mHdmidata, 0, sizeof(hdmi_data_t));
    mScene_output_info.dv_type = DOLBY_VISION_SET_DISABLE;
    strcpy(mSocType, "meson8");
    strcpy(mDefaultUI, "4k2k");

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

#ifdef SYSTEMCONTROL_DISPLAY_TYPE
    SYS_LOGI("display type: %s", SYSTEMCONTROL_DISPLAY_TYPE);
    if (!strcmp(SYSTEMCONTROL_DISPLAY_TYPE, DEVICE_STR_MBOX)) {
        mDisplayType = DISPLAY_TYPE_MBOX;
    } else if (!strcmp(SYSTEMCONTROL_DISPLAY_TYPE, DEVICE_STR_MID)) {
        mDisplayType = DISPLAY_TYPE_TABLET;
    } else if (!strcmp(SYSTEMCONTROL_DISPLAY_TYPE, DEVICE_STR_TV)) {
        if (pSysWrite->getPropertyBoolean(PROP_TVSOC_AS_MBOX, false)) {
            mDisplayType = DISPLAY_TYPE_REPEATER;
        } else {
            mDisplayType = DISPLAY_TYPE_TV;
        }
    }
#endif

#ifdef SYSTEMCONTROL_UI_TYPE
    int ui_type;
    ui_type = SYSTEMCONTROL_UI_TYPE;
    SYS_LOGI("UI type: %d", ui_type);
    sprintf(mDefaultUI, "%d", ui_type);
#endif

    pFrameRateAutoAdaption = new FrameRateAutoAdaption(this);

    SYS_LOGI("type: %d [0:none 1:tablet 2:mbox 3:tv 4:repeater], soc type:%s, default UI:%s",
        mDisplayType, mSocType, mDefaultUI);

    // check dolby vision is support or not
    setDolbyVisionSupport();

    if (DISPLAY_TYPE_MBOX == mDisplayType) {
        pTxAuth = new HDCPTxAuth();
        pTxAuth->setHDCPCallback(this);
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
        pTxAuth->setHDCPCallback(this);
        pUEventObserver = new UEventObserver();
        pUEventObserver->setUevntCallback(this);
        pUEventObserver->setFRAutoAdpt(pFrameRateAutoAdaption);
        pUEventObserver->setHDCPTxAuth(pTxAuth);
        pUEventObserver->tv_framerateevent_thread();
        pRxAuth = new HDCPRxAuth(pTxAuth);
#endif
    } else if (DISPLAY_TYPE_TABLET == mDisplayType) {
        pTxAuth = new HDCPTxAuth();
        pTxAuth->setHDCPCallback(this);
        pUEventObserver = new UEventObserver();
        pUEventObserver->setUevntCallback(this);
        pUEventObserver->setFRAutoAdpt(pFrameRateAutoAdaption);
        pUEventObserver->setHDCPTxAuth(pTxAuth);
        pUEventObserver->start_hdmitxuevent_thread();
        dumpCaps();
    } else if (DISPLAY_TYPE_REPEATER == mDisplayType) {
        pTxAuth = new HDCPTxAuth();
        pTxAuth->setHDCPCallback(this);
        pUEventObserver = new UEventObserver();
        pUEventObserver->setUevntCallback(this);
        pUEventObserver->setFRAutoAdpt(pFrameRateAutoAdaption);
        pUEventObserver->setHDCPTxAuth(pTxAuth);
        pUEventObserver->start_hdmitxuevent_thread();
        pRxAuth = new HDCPRxAuth(pTxAuth);
        dumpCaps();
    }
    resetMemc();
}
#ifdef FRAMERATE_MODE
void DisplayMode::setPQHandle(CPQControl* handle) {
    pFrameRateAutoAdaption->setPQHandle(handle);
}
#endif
DisplayMode::~DisplayMode() {
    delete pSysWrite;
    delete pmDeepColor;
    delete mpSceneProcess;
    delete pFrameRateAutoAdaption;
}

void DisplayMode::init() {
    /* enter gpio key to power on
     * for ohm: boot_flag = 0 will enter recovery mode
     *                      1 will enter update mode
     *                      2 will enter fastboot mode
     * for other board: boot_flag = 0 will enter fastboot mode
     *                              1 will enter update mode
     *                              2 will enter recovery mode
     */
    char buf[PROPERTY_VALUE_MAX];

    property_get("ro.boot.slot_suffix", buf, "");
    SYS_LOGI("buf :%s\n", buf);
    if ((strcmp(buf, "_a") != 0) && (strcmp(buf, "_b") != 0)) {
        set_sys_boot_complete();
        SYS_LOGI("call set_sys_boot_complete in systemcontrol");
    }

    if (mIsRecovery) {
        mUbootenv->updateValue("ubootenv.var.boot_flag", "0");
    }

    /*TODO:tmp solution for DRM MODE., will remove later.*/
    if (mIsRecovery && access("/dev/dri/card0", R_OK | W_OK) == 0) {
        pSysWrite->writeSysfs("/sys/class/amhdmitx/amhdmitx0/attr", "rgb,8bit", strlen("rgb,8bit"));
        return ;
    }

    if (DISPLAY_TYPE_MBOX == mDisplayType) {
#ifndef RECOVERY_MODE
        /* boot config enable, hwc will take care of it */
        if (isHWCProcess()) {
            memset(&mHdmidata, 0, sizeof(hdmi_data_t));
            mHdmidata.state = OUTPUT_MODE_STATE_INIT;
            getHdmiData(&mHdmidata);
            SYS_LOGI("init return, hwc boot config enable");
            return;
        }
#endif
        setSourceDisplay(OUTPUT_MODE_STATE_INIT);
    } else if (DISPLAY_TYPE_TV == mDisplayType) {
#ifndef RECOVERY_MODE
        setSinkDisplay(true);
#else
        setTvRecoveryDisplay();
#endif
    } else if (DISPLAY_TYPE_TABLET == mDisplayType) {

    } else if (DISPLAY_TYPE_REPEATER == mDisplayType) {
#ifndef RECOVERY_MODE
        /* boot config enable, hwc will take care of it */
        if (isHWCProcess()) {
            memset(&mHdmidata, 0, sizeof(hdmi_data_t));
            mHdmidata.state = OUTPUT_MODE_STATE_INIT;
            getHdmiData(&mHdmidata);
            SYS_LOGI("init return, hwc boot config enable");
            return;
        }
#endif
        setSourceDisplay(OUTPUT_MODE_STATE_INIT);
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
            setSourceDisplay(OUTPUT_MODE_STATE_POWER);
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

    SYS_LOGD("key:%s value:%s", key, p_value);

    if (p_value) {
        strcpy(value, p_value);
        return true;
    }
    return false;
}

void DisplayMode::setBootEnv(const char* key, const char* value) {
    SYS_LOGD("key:%s value:%s", key, value);

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

            SYS_LOGD("Parsing %s: %s", tokenizer->getLocation(), tokenizer->peekRemainderOfLine());

            tokenizer->skipDelimiters(WHITESPACE);

            if (!tokenizer->isEol() && tokenizer->peekChar() != '#') {

                char *token = tokenizer->nextToken(WHITESPACE);
                if (!strcmp(token, DEVICE_STR_MBOX)) {
                    mDisplayType = DISPLAY_TYPE_MBOX;

                    memset(mSocType, 0, sizeof(mSocType));
                    memset(mDefaultUI, 0, sizeof(mDefaultUI));
                    tokenizer->skipDelimiters(WHITESPACE);
                    strncpy(mSocType, tokenizer->nextToken(WHITESPACE), sizeof(mSocType)-1);
                    tokenizer->skipDelimiters(WHITESPACE);
                    strncpy(mDefaultUI, tokenizer->nextToken(WHITESPACE), sizeof(mDefaultUI)-1);
                } else if (!strcmp(token, DEVICE_STR_TV)) {
                    mDisplayType = DISPLAY_TYPE_TV;

                    memset(mSocType, 0, sizeof(mSocType));
                    memset(mDefaultUI, 0, sizeof(mDefaultUI));
                    tokenizer->skipDelimiters(WHITESPACE);
                    strncpy(mSocType, tokenizer->nextToken(WHITESPACE), sizeof(mSocType)-1);
                    tokenizer->skipDelimiters(WHITESPACE);
                    strncpy(mDefaultUI, tokenizer->nextToken(WHITESPACE), sizeof(mDefaultUI)-1);
                } else if (!strcmp(token, DEVICE_STR_MID)) {
                    mDisplayType = DISPLAY_TYPE_TABLET;

                    memset(mSocType, 0, sizeof(mSocType));
                    memset(mDefaultUI, 0, sizeof(mDefaultUI));
                    tokenizer->skipDelimiters(WHITESPACE);
                    strncpy(mSocType, tokenizer->nextToken(WHITESPACE), sizeof(mSocType)-1);
                    tokenizer->skipDelimiters(WHITESPACE);
                    strncpy(mDefaultUI, tokenizer->nextToken(WHITESPACE), sizeof(mDefaultUI)-1);
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

            SYS_LOGD("Parsing %s: %s", tokenizer->getLocation(), tokenizer->peekRemainderOfLine());

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

void DisplayMode::setTvRecoveryDisplay() {
    SYS_LOGI("setTvRecoveryDisplay\n");
    char outputmode[MODE_LEN] = {0};
    getDisplayMode(outputmode);
    updateDefaultUI();

    if (usleep(1000000LL) < 0)
        SYS_LOGE("usleep interrupt!\n");
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
    memset(&scene_input_info, 0, sizeof(scene_input_info_t));

    //1.1 common input info
    if ((data->state == OUTPUT_MODE_STATE_INIT) ||
        (data->state == OUTPUT_MODE_STATE_POWER)) {
        strcpy(scene_input_info.cur_displaymode, data->ubootenv_hdmimode);
    } else if (OUTPUT_MODE_STATE_SWITCH == data->state) {
        strcpy(scene_input_info.cur_displaymode, data->ui_hdmimode);
    }

    scene_input_info.state                   = (scene_state)data->state;
    scene_input_info.isbestcolorspace        = data->isbestcolorspace;
    scene_input_info.isbestpolicy            = data->isbestpolicy;
    scene_input_info.isDvEnable              = isDolbyVisionEnable();
    scene_input_info.isTvSupportDv           = isTvSupportDolbyVision(tvmode);
    scene_input_info.isTvSupportHDR          = isTvSupportHDR();
    scene_input_info.isHdrResolutionPriority = isHdrResolutionPriority();
    scene_input_info.hdr_policy              = data->hdr_policy;
    scene_input_info.hdr_priority            = data->hdr_priority;
    scene_input_info.hdr_force_mode          = data->hdr_force_mode;

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

    bool ret = getBootEnv(UBOOTENV_USER_COLORATTRIBUTE, data->ubootenv_colorattribute);
    if (!ret) {
        //if env is null,use none as default value
        strcpy(data->ubootenv_colorattribute, "none");
    }
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

void DisplayMode::setDefaultMode() {
    SYS_LOGE("EDID parsing error detected\n");

    // check hdmi output mode
    char curDisplayMode[MODE_LEN]    = {0};
    getDisplayMode(curDisplayMode);

    if (!isMatchMode(curDisplayMode, DEFAULT_HDMI_MODE)) {
        //set avmute
        pSysWrite->writeSysfs(DISPLAY_HDMI_AVMUTE_SYSFS, "1");
        //set default color format
        DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_HDMI_COLOR_ATTR, DEFAULT_COLOR_FORMAT);
        //set default resolution
        setDisplayMode(DEFAULT_HDMI_MODE);

        //update display position
        int position[4] = { 0, 0, 0, 0 };//x,y,w,h
        getPosition(DEFAULT_HDMI_MODE, position);
        setPosition(DEFAULT_HDMI_MODE, position[0], position[1],position[2], position[3]);

        //clear avmute
        pSysWrite->writeSysfs(DISPLAY_HDMI_AVMUTE_SYSFS, "-1");
    } else {
        SYS_LOGI("cur mode is default mode\n");
    }
}

/*
* OUTPUT_MODE_STATE_INIT for boot
* OUTPUT_MODE_STATE_POWER for hdmi plug and suspend/resume
*/
void DisplayMode::setSourceDisplay(output_mode_state state) {
#ifndef RECOVERY_MODE
    AutoMutex _l( mLock );
#endif

    //1. hdmi used and hpd = 0
    //set dummy_l mode
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

    //2. update hdmi info when boot and hdmi plug/suspend/resume
    memset(&mHdmidata, 0, sizeof(hdmi_data_t));
    mHdmidata.state = state;
    getHdmiData(&mHdmidata);

    //3. hdmi edid parse error and hpd = 1
    //set default reolsution and color format
    if ((isHdmiEdidParseOK() == false) &&
        (isHdmiHpd() == true)) {
        setDefaultMode();
        return;
    }

    //4. scene logic process
    sceneProcess(&mHdmidata);

    if (OUTPUT_MODE_STATE_INIT == state) {
        updateDefaultUI();
    }

    //5. apply settings to driver
    hdmi_output_info_t output_info;
    strcpy(output_info.final_displaymode, mHdmidata.final_displaymode);
    strcpy(output_info.final_deepcolor, mHdmidata.final_deepcolor);
    output_info.dv_type = mHdmidata.dv_info.dv_type;
    output_info.reason  = mHdmidata.state;

    applyDisplaySetting(&output_info);
}

void DisplayMode::clearUserDisplayConfig() {
    SYS_LOGI("clear user display config\n");

    //clear user color format
    setBootEnv(UBOOTENV_USER_COLORATTRIBUTE, "none");
    //clear user dv
    setBootEnv(UBOOTENV_USER_DV_TYPE, "none");

    //2. set hdmi mode for trigger setting
    if (isHWCProcess()) {
        DisplayModeMgr::getInstance().clearUserDisplayConfig();
    } else {
        char cur_displaymode[MODE_LEN] = {0};
        getDisplayMode(cur_displaymode);
        setSourceOutputMode(cur_displaymode);
    }
}

void DisplayMode::clearBootDisplayConfig(const char*value) {
    SYS_LOGI("clear boot display config to %s\n",  value);
    setBootEnv(UBOOTENV_ISBESTMODE, value);
    // after clear boot config, need save the bestMode to uenv
    if (!strcmp(value, "true")) {
        //save hdmi resolution env to default value
        setBootEnv(UBOOTENV_HDMIMODE, "none");
        //need to keep the same value with the defaul value
        setBootEnv(UBOOTENV_FRAC_RATE_POLICY, "1");
    }
}

void DisplayMode::setBootDisplayConfig(const char* savemode) {
    SYS_LOGI("set boot display config to %s\n", savemode);

    setBootEnv(UBOOTENV_ISBESTMODE, "false");

    if (strstr(savemode, "cvbs") != NULL
        || strstr(savemode, "pal") != NULL
        || strstr(savemode, "ntsc") != NULL) {
        setBootEnv(UBOOTENV_CVBSMODE, savemode);
    } else if (strstr(savemode, "hz") != NULL) {
        setBootEnv(UBOOTENV_HDMIMODE, savemode);
    }
}

bool DisplayMode::getPreferredDisplayConfig(char* mode) {
    if (DISPLAY_TYPE_TV == mDisplayType) {
        char curMode[MODE_LEN] = {0};
        getDisplayMode(curMode);
        strcpy(mode, curMode);
    } else {
        //1. get hdmi data
        hdmi_data_t data;

        memset(&data, 0, sizeof(hdmi_data_t));
        getHdmiData(&data);
        data.state = OUTPUT_MODE_STATE_INIT;
        data.isbestpolicy     = true;
        data.isbestcolorspace = true;

        //2. scene logic process
        sceneProcess(&data);

        strcpy(mode, data.final_displaymode);
    }

    SYS_LOGI("getPreferredDisplayConfig [%s]", mode);
    return true;
}

void DisplayMode::setActiveDispMode(const char*value) {
    char hdcpauth[8] = {0};
    pSysWrite->getPropertyString("vendor.sys.hdcp_result", hdcpauth, "1");

    if (!strcmp(hdcpauth, "0")) {
        SYS_LOGD("hdcp auth fail, set default mode.\n");
        pSysWrite->writeSysfs(DISPLAY_HDMI_AVMUTE_SYSFS, "1");
        DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_HDMI_COLOR_ATTR, DEFAULT_COLOR_FORMAT);
        //set hdmi default mode
        setDisplayMode(DEFAULT_HDMI_MODE);

        //update display position
        int position[4] = { 0, 0, 0, 0 };//x,y,w,h
        getPosition(DEFAULT_HDMI_MODE, position);
        setPosition(DEFAULT_HDMI_MODE, position[0], position[1],position[2], position[3]);
        pSysWrite->writeSysfs(DISPLAY_HDMI_AVMUTE_SYSFS, "-1");
        pSysWrite->setProperty("vendor.sys.hdcp_result", "1");
    } else {
        mHdmidata.reason = OUTPUT_CHANGE_BY_HWC;
        SYS_LOGI("setDisplayed by hwc %s", value);
        setSourceOutputMode(value);
        mHdmidata.reason = OUTPUT_CHANGE_BY_INIT;
    }
}

void DisplayMode::notifyPlugin() {
#ifndef RECOVERY_MODE
    notifyEvent(EVENT_HDMI_PLUG_IN);
#endif
}

void DisplayMode::onHdcpTxAuthEvent(const char* status) {
#ifndef RECOVERY_MODE
    if (!strcmp(status, HDMI_TX_AUTH_SUCCESS)) {
        notifyEvent(EVENT_HDMI_TX_AUTH_SUCCESS);
    } else if (!strcmp(status, HDMI_TX_AUTH_FAIL)) {
        notifyEvent(EVENT_HDMI_TX_AUTH_FAIL);
    }
#endif
}

/*
* OUTPUT_MODE_STATE_SWITCH for UI set
*/
void DisplayMode::setSourceOutputMode(const char* outputmode) {
#ifndef RECOVERY_MODE
    AutoMutex _l( mLock );
#endif
    if (DISPLAY_TYPE_TV == mDisplayType) {
        setSinkOutputMode(outputmode, false);
    } else {
        //1. get hdmi data
        if (DISPLAY_TYPE_TABLET == mDisplayType) {
            getHdmiData(&mHdmidata);
        } else {
            getCommonData(&mHdmidata);
        }

        mHdmidata.state = OUTPUT_MODE_STATE_SWITCH;
        strcpy(mHdmidata.ui_hdmimode, outputmode);

        //2. scene logic process
        sceneProcess(&mHdmidata);

        //3. setting apply
        hdmi_output_info_t output_info;

        strcpy(output_info.final_displaymode, mHdmidata.final_displaymode);
        strcpy(output_info.final_deepcolor, mHdmidata.final_deepcolor);
        output_info.dv_type = mHdmidata.dv_info.dv_type;
        output_info.reason  = mHdmidata.state;

        applyDisplaySetting(&output_info);
    }
}

/*
* apply setting
*/
void DisplayMode::applyDisplaySetting(hdmi_output_info_t* output_info) {
    if (!output_info) {
        SYS_LOGE("output_info is NULL\n");
        return;
    }

    //quiescent boot need not output
    char quiescent_mode[8] = {0};
    pSysWrite->getPropertyString("ro.boot.quiescent", quiescent_mode, "0");
    SYS_LOGI("quiescent_mode is %s\n", quiescent_mode);
    if ((strcmp(quiescent_mode, "1") == 0) && (output_info->reason == OUTPUT_MODE_STATE_INIT)) {
        SYS_LOGI("don't need to setting hdmi when quiescent mode\n");
        return;
    }

    //check cvbs mode
    bool cvbsMode = false;

    if (!strcmp(output_info->final_displaymode, MODE_480CVBS) || !strcmp(output_info->final_displaymode, MODE_576CVBS)
        || !strcmp(output_info->final_displaymode, MODE_PAL_M) || !strcmp(output_info->final_displaymode, MODE_PAL_N)
        || !strcmp(output_info->final_displaymode, MODE_NTSC_M)
        || !strcmp(output_info->final_displaymode, "null") || !strcmp(output_info->final_displaymode, "dummy_l")
        || !strcmp(output_info->final_displaymode, MODE_PANEL)) {
        cvbsMode = true;
    }

    /* not enable phy in systemcontrol by default
     * as phy will be enabled in driver when set mode
     * only enable phy if phy is disabled but not enabled
     */
    bool phy_enabled_already = true;

    // 1. update hdmi frac_rate_policy
    //    hwc will maybe change frc_policy for support 59.94hz or 60hz
    char frac_rate_policy[MODE_LEN]     = {0};
    char cur_frac_rate_policy[MODE_LEN] = {0};
    bool frac_rate_policy_change        = false;

    if (output_info->reason != OUTPUT_MODE_STATE_SWITCH) {
        pSysWrite->readSysfs(HDMI_TX_FRAMERATE_POLICY, cur_frac_rate_policy);
        getBootEnv(UBOOTENV_FRAC_RATE_POLICY, frac_rate_policy);
        if (strstr(frac_rate_policy, cur_frac_rate_policy) == NULL) {
            pSysWrite->writeSysfs(HDMI_TX_FRAMERATE_POLICY, frac_rate_policy);
            frac_rate_policy_change = true;
        }  else {
            SYS_LOGI("cur frac_rate_policy is equals\n");
        }
    } else {
         pSysWrite->readSysfs(HDMI_TX_FRAMERATE_POLICY, cur_frac_rate_policy);
         pSysWrite->getPropertyString(HDMI_FRC_POLICY_PROP,frac_rate_policy,"2");
         if (strstr(frac_rate_policy,"2")) {
             getBootEnv(UBOOTENV_FRAC_RATE_POLICY, frac_rate_policy);
         }
         SYS_LOGI("get frc policy from hwc is %s and current value is %s\n",frac_rate_policy, cur_frac_rate_policy);
         if (strstr(frac_rate_policy, cur_frac_rate_policy) == NULL) {
             pSysWrite->writeSysfs(HDMI_TX_FRAMERATE_POLICY, frac_rate_policy);
             frac_rate_policy_change = true;
         }
    }
    // 2. set hdmi final color space
    char curColorAttribute[MODE_LEN] = {0};
    char final_deepcolor[MODE_LEN]   = {0};
    bool attr_change                 = false;

    std::string cur_ColorAttribute;
    DisplayModeMgr::getInstance().getDisplayAttribute(DISPLAY_HDMI_COLOR_ATTR, cur_ColorAttribute);
    strcpy(curColorAttribute, cur_ColorAttribute.c_str());
    strcpy(final_deepcolor, output_info->final_deepcolor);
    SYS_LOGI("curDeepcolor[%s] final_deepcolor[%s]\n", curColorAttribute, final_deepcolor);

    if (strstr(curColorAttribute, final_deepcolor) == NULL) {
        SYS_LOGI("set color space from:%s to %s\n", curColorAttribute, final_deepcolor);
        DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_HDMI_COLOR_ATTR, final_deepcolor);
        attr_change = true;
    } else {
        SYS_LOGI("cur deepcolor is equals\n");
    }

    // 3. update hdr strategy
    bool hdr_policy_change = false;
    std::string cur_hdr_policy;
    DisplayModeMgr::getInstance().getDisplayAttribute(DISPLAY_HDR_POLICY, cur_hdr_policy);
    SYS_LOGI("cur hdr policy:%s\n", cur_hdr_policy.c_str());

    std::string cur_hdr_force_mode;
    DisplayModeMgr::getInstance().getDisplayAttribute(DISPLAY_FORCE_HDR_MODE, cur_hdr_force_mode);
    SYS_LOGI("cur hdr force mode:%s\n", cur_hdr_force_mode.c_str());

    std::string cur_dv_mode;
    DisplayModeMgr::getInstance().getDisplayAttribute(DISPLAY_DOLBY_VISION_MODE, cur_dv_mode);
    SYS_LOGI("cur dv mode:%s\n", cur_dv_mode.c_str());

    char hdr_force_mode[MODE_LEN] = {0};
    gethdrforcemode(hdr_force_mode);

    char hdr_policy[MODE_LEN] = {0};
    getHdrStrategy(hdr_policy);

    if (strstr(cur_hdr_policy.c_str(), hdr_policy) == NULL) {
        SYS_LOGI("set hdr policy from:%s to %s\n", cur_hdr_policy.c_str(), hdr_policy);
        hdr_policy_change = true;
    } else if (!strcmp(hdr_policy, HDR_POLICY_FORCE) && (strstr(cur_hdr_force_mode.c_str(), hdr_force_mode) == NULL)) {
        SYS_LOGI("set hdr force mode from:%s to %s\n", cur_hdr_force_mode.c_str(), hdr_force_mode);
        hdr_policy_change = true;
    } else if ((output_info->dv_type != DOLBY_VISION_SET_DISABLE)
        && (!strcmp(hdr_policy, HDR_POLICY_FORCE) && (strstr(dvModeTypeToString(cur_dv_mode.c_str()), hdr_force_mode) == NULL))) {
        SYS_LOGI("set dv force mode from:%s to %s\n", dvModeTypeToString(cur_dv_mode.c_str()), hdr_force_mode);
        hdr_policy_change = true;
    }

    // 4. update hdr priority
    bool hdr_priority_change = false;
    hdr_priority_e cur_hdr_priority;
    cur_hdr_priority = (hdr_priority_e)getCurrentHdrPriority();

    hdr_priority_e hdr_priority;
    hdr_priority = (hdr_priority_e)getHdrPriority();

    if (cur_hdr_priority != hdr_priority) {
        SYS_LOGI("set hdr priority from:%d to %d\n", cur_hdr_priority, hdr_priority);
        hdr_priority_change = true;
    }

    // 5. check dolby vision
    int  dv_type    = DOLBY_VISION_SET_DISABLE;
    bool dv_change  = false;

    dv_type   = output_info->dv_type;
    dv_change = checkDolbyVisionStatusChanged(dv_type);
    if (isMboxSupportDolbyVision()
        && dv_change) {
        //5.1 set avmute when signal change at boot
        if ((OUTPUT_MODE_STATE_INIT == output_info->reason)
            && (strstr(hdr_policy, HDR_POLICY_SINK))) {
            pSysWrite->writeSysfs(DISPLAY_HDMI_AVMUTE_SYSFS, "1");
        }
        //5.2 set dummy_l mode when dv change at UI switch
        if ((OUTPUT_MODE_STATE_SWITCH == output_info->reason) && dv_change) {
            setDisplayMode("dummy_l");
        }
        //5.3 enable or disable dolby vision core
        if (DOLBY_VISION_SET_DISABLE != dv_type) {
            enableDolbyVision(dv_type);
        } else {
            disableDolbyVision(dv_type);
        }

        SYS_LOGI("isDolbyVisionEnable [%d] dolby vision type:%d", isDolbyVisionEnable(), getDolbyVisionType());
    } else {
        SYS_LOGI("cur DvMode is equals\n");
    }

    // 6. check hdmi output resolution
    char final_displaymode[MODE_LEN] = {0};
    char curDisplayMode[MODE_LEN]    = {0};
    bool modeChange                  = false;

    getDisplayMode(curDisplayMode);
    strcpy(final_displaymode, output_info->final_displaymode);
    SYS_LOGI("curMode:[%s] ,final_displaymode[%s]\n", curDisplayMode, final_displaymode);

    if (!isMatchMode(curDisplayMode, final_displaymode)) {
        modeChange = true;
    } else {
        SYS_LOGI("cur mode is equals\n");
    }

    //7. check any change
    bool isNeedChange = false;

    if (modeChange || attr_change || frac_rate_policy_change || hdr_policy_change || hdr_priority_change) {
        isNeedChange = true;
    } else {
        SYS_LOGI("nothing need to be changed\n");
    }

    // 8. stop hdcp
    if (isNeedChange) {
        pSysWrite->writeSysfs(DISPLAY_HDMI_AVMUTE_SYSFS, "1");
        if (OUTPUT_MODE_STATE_POWER != output_info->reason) {
            if (usleep(100000) < 0)//100ms
                SYS_LOGE("usleep interrupt!\n");
            pSysWrite->writeSysfs(DISPLAY_HDMI_HDCP_MODE, "-1");
            //usleep(100000);//100ms
            pSysWrite->writeSysfs(DISPLAY_HDMI_PHY, "0"); /* Turn off TMDS PHY */
            phy_enabled_already = false;
            if (usleep(50000) < 0)//50ms
                SYS_LOGE("usleep interrupt!\n");
        }
        // stop hdcp tx
        pTxAuth->stop();
    } else if (OUTPUT_MODE_STATE_INIT == output_info->reason) {
        // stop hdcp tx
        pTxAuth->stop();
        char fail_case[8] = {0};
        pSysWrite->getPropertyString(HDCP_TX_AUTH_FAIL, fail_case, "4");
        if (!strcmp(fail_case, "1")) {
            pSysWrite->writeSysfs(DISPLAY_HDMI_VIDEO_MUTE, "1");
            pSysWrite->writeSysfs(DISPLAY_HDMI_AUDIO_MUTE, "1");
        } else if (!strcmp(fail_case, "2")) {
            pSysWrite->writeSysfs(DISPLAY_HDMI_AUDIO_MUTE, "1");
            pSysWrite->writeSysfs(DISPLAY_MEDIA_VIDEO_MUTE, "1");
        }
    }

    // 9. set hdmi final output mode
    if (isNeedChange) {
        //apply hdr policy to driver sysfs
        if (hdr_policy_change) {
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
            } else if (strstr(hdr_policy, HDR_POLICY_FORCE)) {
                char hdr_force_mode[MODE_LEN] = {0};
                gethdrforcemode(hdr_force_mode);
                DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_FORCE_HDR_MODE, hdr_force_mode, ConnectorType::CONN_TYPE_HDMI);
                DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_HDR_POLICY, HDR_POLICY_FORCE, ConnectorType::CONN_TYPE_HDMI);
                if (isDolbyVisionEnable()) {
                    DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_DOLBY_VISION_POLICY, HDR_POLICY_FORCE, ConnectorType::CONN_TYPE_HDMI);
                    if (strstr(hdr_force_mode, FORCE_DV)) {
                        DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_DOLBY_VISION_MODE, FORCE_DV, ConnectorType::CONN_TYPE_HDMI);
                    } else if (strstr(hdr_force_mode, FORCE_HDR10)) {
                        DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_DOLBY_VISION_MODE, FORCE_HDR10, ConnectorType::CONN_TYPE_HDMI);
                    } else if (strstr(hdr_force_mode, FORCE_SDR)) {
                        // 8bit or not
                        std::string cur_ColorAttribute;
                        DisplayModeMgr::getInstance().getDisplayAttribute(DISPLAY_HDMI_COLOR_ATTR, cur_ColorAttribute);
                        if (cur_ColorAttribute.find("8bit", 0) != std::string::npos) {
                            DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_DOLBY_VISION_MODE, DV_ENABLE_FORCE_SDR_8BIT, ConnectorType::CONN_TYPE_HDMI);
                        } else {
                            DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_DOLBY_VISION_MODE, DV_ENABLE_FORCE_SDR_10BIT, ConnectorType::CONN_TYPE_HDMI);
                        }
                    }
                }
            }
        }

        //apply hdr priority to driver sysfs
        if (hdr_priority_change) {
            DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_HDR_PRIORITY, HDR_PRIORITY_TYPE[hdr_priority], ConnectorType::CONN_TYPE_HDMI);
        }

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
            if (strstr(final_displaymode, "null") && w1 != 0) {
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
    getPosition(final_displaymode, position);
    setPosition(final_displaymode, position[0], position[1],position[2], position[3]);

    // no need to update
    // update free_scale_axis and window_axis in recovery mode
#ifdef RECOVERY_MODE
    updateFreeScaleAxis();
    updateWindowAxis(final_displaymode);
#endif

    // 9. turn on phy and clear avmute
    if (isNeedChange) {
        /*if (!phy_enabled_already) {
            pSysWrite->writeSysfs(DISPLAY_HDMI_PHY, "1"); // Turn on TMDS PHY
        }*/
        if (usleep(20000) < 0)
                SYS_LOGE("usleep interrupt!\n");
        char fail_case[8] = {0};
        pSysWrite->getPropertyString(HDCP_TX_AUTH_FAIL, fail_case, "4");
        if (!strcmp(fail_case, "1")) {
            pSysWrite->writeSysfs(DISPLAY_HDMI_VIDEO_MUTE, "1");
            pSysWrite->writeSysfs(DISPLAY_HDMI_AUDIO_MUTE, "1");
        } else if (!strcmp(fail_case, "2")) {
            pSysWrite->writeSysfs(DISPLAY_HDMI_AUDIO_MUTE, "1");
            pSysWrite->writeSysfs(DISPLAY_MEDIA_VIDEO_MUTE, "1");
        } else {
            pSysWrite->writeSysfs(DISPLAY_HDMI_AUDIO_MUTE, "1");
            pSysWrite->writeSysfs(DISPLAY_HDMI_AUDIO_MUTE, "0");
        }
        if (isDolbyVisionEnable()) {
            if (usleep(20000) < 0)
                SYS_LOGE("usleep interrupt!\n");
        }
    }

    //must clear avmute for new policy(driver maybe set mute)
    pSysWrite->writeSysfs(DISPLAY_HDMI_AVMUTE_SYSFS, "-1");

    // 10. start HDMI HDCP authenticate
    if (isNeedChange) {
        if (!cvbsMode) {
            pTxAuth->start();
        }
    } else if (OUTPUT_MODE_STATE_INIT == output_info->reason) {
        if (!cvbsMode) {
            pTxAuth->start();
        }
    }

    if (OUTPUT_MODE_STATE_INIT == output_info->reason) {
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

int64_t DisplayMode::resolveResolutionValue(const char *mode) {
    return resolveResolutionValue(mode, RESOLUTION_PRIORITY);
}

int64_t DisplayMode::resolveResolutionValue(const char *mode, int flag) {
    if (mpSceneProcess == NULL) {
        mpSceneProcess = new SceneProcess();
    }
    return mpSceneProcess->resolveResolutionValue(mode, flag);
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

    pSysWrite->getPropertyString(PROP_BEST_OUTPUT_MODE, mode, DEFAULT_HDMI_MODE);
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

void DisplayMode::filterHdmiDispcap(hdmi_data_t* data) {
    const char *delim = "\n";
    char filter_dispcap[MAX_STR_LEN] = {0};
    char supportedColorList[MAX_STR_LEN];
    char *save_ptr = NULL;

    if (!(pmDeepColor->initColorAttribute(supportedColorList, MAX_STR_LEN))) {
        SYS_LOGE("initColorAttribute fail\n");
        return;
    }

    SYS_LOGI("before filtered HdmiDispcap: %s\n", data->disp_cap);

    char *hdmi_mode = strtok_r(data->disp_cap, delim, &save_ptr);
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

        hdmi_mode = strtok_r(NULL, delim, &save_ptr);
    }

    strcpy(data->disp_cap, filter_dispcap);

    SYS_LOGI("after filtered HdmiDispcap: %s\n", data->disp_cap);
}

bool DisplayMode::filterHdmiDccap(char* color) {
    bool ret = false;
    const char *delim = "\n";
    char cur_displaymode[MAX_STR_LEN] = {0};
    char dc_cap[MAX_STR_LEN] = {0};
    char *save_ptr = NULL;

    getDisplayMode(cur_displaymode);
    strcpy(dc_cap, mHdmidata.dc_cap);

    SYS_LOGD("before filtered HdmiDccap: %s\n", dc_cap);

    char *colorspace = strtok_r(dc_cap, delim, &save_ptr);
    while (colorspace != NULL) {
        if (getModeSupportDeepColorAttr(cur_displaymode, colorspace)) {
            if ((strlen(color) + strlen(colorspace)) < (MAX_STR_LEN-1)) {
                strcat(color, colorspace);
                strcat(color, delim);
                ret = true;
            } else {
                SYS_LOGE("DisplayMode strcat overflow: src=%s, dst=%s\n", colorspace, color);
                break;
            }
        }

        colorspace = strtok_r(NULL, delim, &save_ptr);
    }

    SYS_LOGD("after filtered HdmiDccap: %s\n", color);
    return ret;
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

    bool ret;

    //hdmi resolution best policy flag
    data->isbestpolicy = isBestOutputmode();

    //hdmi color space best policy flag
    data->isbestcolorspace = isBestColorSpace();

    char hdr_force_mode[MODE_LEN] = {0};
    gethdrforcemode(hdr_force_mode);
    data->hdr_force_mode = (hdr_force_mode_e)atoi(hdr_force_mode);

    //hdmi hdr policy flag:always hdr or match content hdr
    char hdr_policy[MODE_LEN] = {0};
    getHdrStrategy(hdr_policy);
    data->hdr_policy = (hdr_policy_e)atoi(hdr_policy);

    //hdmi hdr priority flag:dv/hdr/sdr
    data->hdr_priority = (hdr_priority_e)getHdrPriority();

    SYS_LOGI("isbestcolorspace:%d, isbestpolicy:%d, hdr_policy:%d, hdr_priority :%d, hdr_force_mode:%d\n",
            data->isbestcolorspace,
            data->isbestpolicy,
            data->hdr_policy,
            data->hdr_priority,
            data->hdr_force_mode);

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
    ret = getBootEnv(UBOOTENV_USER_COLORATTRIBUTE, data->ubootenv_colorattribute);
    if (!ret) {
        //if env is null,use none as default value
        strcpy(data->ubootenv_colorattribute, "none");
    }
    SYS_LOGI("hdmi_current_attr:%s, ubootenv_colorattribute:%s\n",
            data->hdmi_current_attr,
            data->ubootenv_colorattribute);

    //if no dolby_status env set to std for enable dolby vision
    //if box support dolby vision
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
    ret = getBootEnv(UBOOTENV_USER_DV_TYPE, ubootenv_dv_type);
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
    DisplayModeMgr::getInstance().getDisplayAttribute(DISPLAY_DOLBY_VISION_CAP2, dv_cap, ConnectorType::CONN_TYPE_HDMI);
    strcpy(data->dv_info.dv_cap, dv_cap.c_str());

    if (strstr(data->dv_info.dv_cap, "DolbyVision RX support list") != NULL) {
        for (int i = sizeof(DISPLAY_MODE_LIST)/sizeof(char *) - 1; i >= 0; i--) {
            if (strstr(data->dv_info.dv_cap, DISPLAY_MODE_LIST[i]) != NULL) {
                if ((strlen(data->dv_info.dv_displaymode) + strlen(DISPLAY_MODE_LIST[i]) + 1) < sizeof(data->dv_info.dv_displaymode)) {
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
                if ((strlen(data->dv_info.dv_deepcolor) + strlen(DV_MODE_TYPE[i]) + 1) < sizeof(data->dv_info.dv_deepcolor)) {
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
        if (usleep(500000) < 0)
            SYS_LOGE("usleep interrupt!\n");
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
            SYS_LOGE("read dc_cap fail\n");
            break;
        }
        count++;
        if (usleep(500000) < 0)
            SYS_LOGE("usleep interrupt!\n");
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

void DisplayMode::getHdmiData_cached(hdmi_data_t* data) {
    memcpy(data, &mHdmidata, sizeof(hdmi_data_t));
}

void DisplayMode::getSupportDispModeList(char * modelist) {
    if (!modelist) {
        SYS_LOGE("%s modelist is NULL\n", __FUNCTION__);
    } else {
        //read hdmi disp_cap
        hdmi_data_t data;
        getHdmiDispCap(data.disp_cap);

        //filter hdmi disp_cap mode for compatibility
        filterHdmiDispcap(&data);
        strcpy(modelist, data.disp_cap);
    }

    return;
}

void DisplayMode::getHdmiData(hdmi_data_t* data) {
    if (!data) {
        SYS_LOGE("%s data is NULL\n", __FUNCTION__);
        return;
    }

    //common info
    getCommonData(data);

    //hdmi driver info
    getHdmiEdidStatus(data->edidParsing);
    //three sink types: sink, repeater, none
    data->sinkType = getHdmiSinkType();
    SYS_LOGI("display sink type:%d [0:none, 1:sink, 2:repeater]\n", data->sinkType);

    if (HDMI_SINK_TYPE_NONE != data->sinkType) {
        //read hdmi disp_cap
        char disp_cap[MAX_STR_LEN];
        getHdmiDispCap(disp_cap);
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
        if (usleep(1000000LL) < 0)
            SYS_LOGE("usleep interrupt!\n");
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

/* boot config enable, hwc will take care of it */
bool DisplayMode::isHWCProcess() {
    return pSysWrite->getPropertyBoolean(HWC_BOOT_CONFIG_PROP, false);
}

bool DisplayMode::isBestOutputmode() {
    char isBestMode[MODE_LEN] = {0};
    char hdmimode[MODE_LEN] = {0};
    if (DISPLAY_TYPE_TV == mDisplayType) {
        return false;
    }

    if (!getBootEnv(UBOOTENV_HDMIMODE, hdmimode) || (strstr(hdmimode, "hz") == NULL)) {
        // when hdmimode is empty or no resolution,
        // still used Best mode to avoid dummy mode
        return true;
    }

    return !getBootEnv(UBOOTENV_ISBESTMODE, isBestMode) || strcmp(isBestMode, "true") == 0;
}

bool DisplayMode::isBestColorSpace() {
    bool ret = false;
    char user_colorattr[MODE_LEN] = {0};
    if (DISPLAY_TYPE_TV == mDisplayType) {
        return false;
    }

    ret = getBootEnv(UBOOTENV_USER_COLORATTRIBUTE, user_colorattr);

    if (!ret) {
        return true;
    } else if (strstr(user_colorattr, "bit") == NULL) {
        return true;
    }

    return false;
}

bool DisplayMode::isHdrResolutionPriority() {
    return pSysWrite->getPropertyBoolean(PROP_HDR_RESOLUTION_PRIORITY, true);
}

bool DisplayMode::isFrameratePriority() {
    return pSysWrite->getPropertyBoolean(PROP_HDMI_FRAMERATE_PRIORITY, true);
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
void DisplayMode::setDisplayModeinner(const char* outputmode) {
    setSinkOutputMode(outputmode, false);
}

void DisplayMode::setSinkOutputMode(const char* outputmode, bool initState) {
    SYS_LOGI("set sink output mode:%s, init state:%d\n", outputmode, initState?1:0);

    char curMode[MODE_LEN] = {0};
    getDisplayMode(curMode);

    SYS_LOGI("curMode = %s outputmode = %s", curMode, outputmode);
    if (strstr(curMode, outputmode) == NULL) {
        //set output mode
        DisplayModeMgr::getInstance().setDisplayMode(outputmode);

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

        SYS_LOGI("set output mode:%s done\n", finalMode);
    } else {
        if (initState) {
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
#ifdef RECOVERY_MODE
            startBootanimDetectThread();
#endif
        }
        SYS_LOGI("cur mode is equals\n");
    }
    // no need to update
    // update free_scale_axis and window_axis in recovery mode
#ifdef RECOVERY_MODE
    updateFreeScaleAxis();
    updateWindowAxis(outputmode);
#endif
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

/*
 * *
 * @Description: select diff policy base on output mode state.
 * @params: outputmode state.
 * author: luan.yuan@amlogic.com
 *
 * only set 'null' to display/mode in switch adapter state.
 * auto switch frame rate need set 1 to /sys/class/amhdmitx/amhdmitx0/frac_rate_policy, to get CLK 0.1% offset.
 * But only change frac_rate_policy can not update CLOCK, unless mode and frac_rate_policy.
 * and can not set same mode to mode node. so need like 1080p60hz--->null--->1080p60hz.
 * this function will set mode to 'null', policy to 1, and set mode to previous value later.
 */
void DisplayMode::setAutoSwitchFrameRate(int state __unused) {
//default force shift 0.001 clk, if you do not want to do that, open this marco.
//so you can control switch it from DroidTvSetings app.
//#define DEFAULT_NO_CLK_OFFSET
#ifdef DEFAULT_NO_CLK_OFFSET
    if ((state == OUTPUT_MODE_STATE_SWITCH_ADAPTER) || pFrameRateAutoAdaption->autoSwitchFlag == true) {
        SYS_LOGI("FrameRate video need set mode to null, and policy to 1 to into adapter policy\n");
        pSysWrite->writeSysfs(HDMI_TX_FRAMERATE_POLICY, "1");
    } else {
        if (state == OUTPUT_MODE_STATE_ADAPTER_END) {
            SYS_LOGI("End Hint FrameRate video need set mode to null to exit adapter policy\n");
        }
        pSysWrite->writeSysfs(HDMI_TX_FRAMERATE_POLICY, "0");
    }
#endif
}

void DisplayMode::updateDefaultUI() {
    if (!strncmp(mDefaultUI, "720", 3)) {
        mDisplayWidth= FULL_WIDTH_720;
        mDisplayHeight = FULL_HEIGHT_720;
    } else if (!strncmp(mDefaultUI, "1080", 4)) {
        mDisplayWidth = FULL_WIDTH_1080;
        mDisplayHeight = FULL_HEIGHT_1080;
    } else if (!strncmp(mDefaultUI, "4k2k", 4) || !strncmp(mDefaultUI, "2160", 4)) {
        mDisplayWidth = FULL_WIDTH_4K2K;
        mDisplayHeight = FULL_HEIGHT_4K2K;
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
    } else if (strstr(curMode, MODE_640x480P)) {
        strcpy(keyValue, MODE_640x480P);
        defaultWidth = FULL_WIDTH_640x480;
        defaultHeight = FULL_HEIGHT_640x480;
    } else if (strstr(curMode, MODE_800x480p)) {
        strcpy(keyValue, MODE_800x480p);
        defaultWidth = FULL_WIDTH_800x480;
        defaultHeight = FULL_HEIGHT_800x480;
    } else if (strstr(curMode, MODE_1024x600p)) {
        strcpy(keyValue, MODE_1024x600p);
        defaultWidth = FULL_WIDTH_1024x600;
        defaultHeight = FULL_HEIGHT_1024x600;
    } else if (strstr(curMode, "480")) {
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
    } else if (strstr(curMode, MODE_768P_PREFIX)) {
        strcpy(keyValue, MODE_768P_PREFIX);
        defaultWidth = FULL_WIDTH_768;
        defaultHeight = FULL_HEIGHT_768;
    } else if (strstr(curMode, MODE_4K1K_PREFIX)) {
        strcpy(keyValue, MODE_4K1K_PREFIX);
        defaultWidth = FULL_WIDTH_4K2K;
        defaultHeight = FULL_HEIGHT_1080;
    } else if (strstr(curMode, MODE_1080I_PREFIX)) {
        strcpy(keyValue, MODE_1080I_PREFIX);
        defaultWidth = FULL_WIDTH_1080;
        defaultHeight = FULL_HEIGHT_1080;
    } else if (strstr(curMode, MODE_1080P_PREFIX)) {
        strcpy(keyValue, MODE_1080P_PREFIX);
        defaultWidth = FULL_WIDTH_1080;
        defaultHeight = FULL_HEIGHT_1080;
    } else if (strstr(curMode, MODE_1440P_PREFIX)) {
        strcpy(keyValue, MODE_1440P_PREFIX);
        defaultWidth = FULL_WIDTH_1440;
        defaultHeight = FULL_HEIGHT_1440;
    } else if (strstr(curMode, MODE_4K2K_PREFIX)) {
        strcpy(keyValue, MODE_4K2K_PREFIX);
        defaultWidth = FULL_WIDTH_4K2K;
        defaultHeight = FULL_HEIGHT_4K2K;
    } else if (strstr(curMode, MODE_4K2KSMPTE_PREFIX)) {
        strcpy(keyValue, "4k2ksmpte");
        defaultWidth = FULL_WIDTH_4K2KSMPTE;
        defaultHeight = FULL_HEIGHT_4K2KSMPTE;
    } else if (strstr(curMode, MODE_8K4K_PREFIX)) {
        strcpy(keyValue, MODE_8K4K_PREFIX);
        defaultWidth = FULL_WIDTH_8K4K;
        defaultHeight = FULL_HEIGHT_8K4K;
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

    if (isHWCProcess()) {
        bool ret = false;
        std::string value;
        sprintf(ubootvar, "ubootenv.var.%s_x", keyValue);
        ret = DisplayModeMgr::getInstance().getUbootenv(ubootvar, value);
        if (ret) {
            position[0] = atoi(value.c_str());
        } else {
            position[0] = 0;
        }

        sprintf(ubootvar, "ubootenv.var.%s_y", keyValue);
        ret = DisplayModeMgr::getInstance().getUbootenv(ubootvar, value);
        if (ret) {
            position[1] = atoi(value.c_str());
        } else {
            position[1] = 0;
        }

        sprintf(ubootvar, "ubootenv.var.%s_w", keyValue);
        ret = DisplayModeMgr::getInstance().getUbootenv(ubootvar, value);
        if (ret) {
            position[2] = atoi(value.c_str());
        } else {
            position[2] = defaultWidth;
        }

        sprintf(ubootvar, "ubootenv.var.%s_h", keyValue);
        ret = DisplayModeMgr::getInstance().getUbootenv(ubootvar, value);
        if (ret) {
            position[3] = atoi(value.c_str());
        } else {
            position[3] = defaultHeight;
        }
    } else {
        sprintf(ubootvar, "ubootenv.var.%s_x", keyValue);
        position[0] = getBootenvInt(ubootvar, 0);
        sprintf(ubootvar, "ubootenv.var.%s_y", keyValue);
        position[1] = getBootenvInt(ubootvar, 0);
        sprintf(ubootvar, "ubootenv.var.%s_w", keyValue);
        position[2] = getBootenvInt(ubootvar, defaultWidth);
        sprintf(ubootvar, "ubootenv.var.%s_h", keyValue);
        position[3] = getBootenvInt(ubootvar, defaultHeight);
    }


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
    } else if (strstr(curMode, MODE_640x480P)) {
        strcpy(keyValue, MODE_640x480P);
    } else if (strstr(curMode, MODE_800x480p)) {
        strcpy(keyValue, MODE_800x480p);
    } else if (strstr(curMode, MODE_1024x600p)) {
        strcpy(keyValue, MODE_1024x600p);
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
    } else if (strstr(curMode, MODE_4K1K_PREFIX)){
        strcpy(keyValue, MODE_4K1K_PREFIX);
    } else if (strstr(curMode, MODE_1080I_PREFIX)) {
        strcpy(keyValue, MODE_1080I_PREFIX);
    } else if (strstr(curMode, MODE_1080P_PREFIX)) {
        strcpy(keyValue, MODE_1080P_PREFIX);
    } else if (strstr(curMode, MODE_1440P_PREFIX)) {
        strcpy(keyValue, MODE_1440P_PREFIX);
    } else if (strstr(curMode, MODE_4K2K_PREFIX)) {
        strcpy(keyValue, MODE_4K2K_PREFIX);
    } else if (strstr(curMode, MODE_4K2KSMPTE_PREFIX)) {
        strcpy(keyValue, "4k2ksmpte");
    } else if (strstr(curMode, MODE_8K4K_PREFIX)) {
        strcpy(keyValue, MODE_8K4K_PREFIX);
    } else if (strstr(curMode, MODE_PANEL)) {
        strcpy(keyValue, MODE_PANEL);
    }

    pthread_mutex_lock(&mEnvLock);
    if (mHdmidata.reason != OUTPUT_CHANGE_BY_HWC) {
        if (isHWCProcess()) {
            sprintf(ubootvar, "ubootenv.var.%s_x", keyValue);
            DisplayModeMgr::getInstance().setUbootenv(ubootvar, x);
            sprintf(ubootvar, "ubootenv.var.%s_y", keyValue);
            DisplayModeMgr::getInstance().setUbootenv(ubootvar, y);
            sprintf(ubootvar, "ubootenv.var.%s_w", keyValue);
            DisplayModeMgr::getInstance().setUbootenv(ubootvar, w);
            sprintf(ubootvar, "ubootenv.var.%s_h", keyValue);
            DisplayModeMgr::getInstance().setUbootenv(ubootvar, h);
        } else {
            sprintf(ubootvar, "ubootenv.var.%s_x", keyValue);
            setBootEnv(ubootvar, x);
            sprintf(ubootvar, "ubootenv.var.%s_y", keyValue);
            setBootEnv(ubootvar, y);
            sprintf(ubootvar, "ubootenv.var.%s_w", keyValue);
            setBootEnv(ubootvar, w);
            sprintf(ubootvar, "ubootenv.var.%s_h", keyValue);
            setBootEnv(ubootvar, h);
        }
    }
    pthread_mutex_unlock(&mEnvLock);
    DisplayModeMgr::getInstance().setDisplayRect({left, top, width , height});

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

bool DisplayMode::setColorSpace(const char* colorspace) {
    SYS_LOGI("user change color space to %s\n", colorspace);
    setBootEnv(UBOOTENV_USER_COLORATTRIBUTE, colorspace);

    char outputmode[MODE_LEN] = {0};
    getDisplayMode(outputmode);
    saveDeepColorAttr(outputmode, colorspace);

    if (isHWCProcess()) {
        std::string colorformat = colorspace;
        DisplayModeMgr::getInstance().setColorSpace(colorformat);
    } else {
        //2. set hdmi mode for trigger setting
        setSourceOutputMode(outputmode);
    }

    return true;
}

bool DisplayMode::getColorSpaceList(std::string& list) {
    bool ret = false;
    if (isHWCProcess()) {
        ret = DisplayModeMgr::getInstance().getColorSpaceList(list);
    } else {
        char dc_cap[MAX_STR_LEN] = {0};
        ret = filterHdmiDccap(dc_cap);
        list = dc_cap;
    }
    SYS_LOGD("%s list:%s\n", __FUNCTION__, list.c_str());

    return ret;
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

void DisplayMode::setFrameRate(float frameRate) {
    SYS_LOGI("%s frameRate:%f\n", __FUNCTION__, frameRate);
    DisplayModeMgr::getInstance().setFrameRate(frameRate);
}

void DisplayMode::setPerferredMode(const char* mode) {
    SYS_LOGI("%s mode:%s\n", __FUNCTION__, mode);
    DisplayModeMgr::getInstance().setPerferredMode(mode);
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
    bool ret = false;

    if (DISPLAY_TYPE_TV == mDisplayType) {
        SYS_LOGI("Current Device is TV, no dv_cap\n");
    } else if (!mode) {
        SYS_LOGI("%s mode is NULL\n", __FUNCTION__);
    } else {
        strcpy(mode, "");
        hdmi_data_t data;
        memset(&data, 0, sizeof(hdmi_data_t));
        if (strlen(mHdmidata.dv_info.dv_cap) != 0) {
            strcpy(data.dv_info.dv_cap, mHdmidata.dv_info.dv_cap);
            strcpy(data.dv_info.dv_displaymode, mHdmidata.dv_info.dv_displaymode);
            strcpy(data.dv_info.dv_deepcolor, mHdmidata.dv_info.dv_deepcolor);
        } else {
            getHdmiDvCap(&data);
        }

        if (strstr(data.dv_info.dv_cap, "DolbyVision RX support list") == NULL) {
            SYS_LOGI("TV not support DV\n");
        } else {
            strcat(mode, data.dv_info.dv_displaymode);
            strcat(mode, data.dv_info.dv_deepcolor);
            ret = true;
            SYS_LOGD("Current Tv Support DV type [%s]", mode);
        }
    }

    return ret;
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

void DisplayMode::setDvHdrPolicy(const char* policy) {
    char dv_hdr10_policy[MODE_LEN] = {0};
    int target_dv_hdr10_policy = 0;

    std::string cur_dv_hdr10_policy;
    DisplayModeMgr::getInstance().getDisplayAttribute(DISPLAY_DOLBY_VISION_HDR_10_POLICY, cur_dv_hdr10_policy);

    target_dv_hdr10_policy = atoi(cur_dv_hdr10_policy.c_str()) | atoi(policy);
    sprintf(dv_hdr10_policy, "%d", target_dv_hdr10_policy);
    DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_DOLBY_VISION_HDR_10_POLICY, dv_hdr10_policy, ConnectorType::CONN_TYPE_HDMI);
}

void DisplayMode::setTvDolbyVisionEnable(void) {
    //if TV
    setHdrMode(HDR_MODE_OFF);
    DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_DOLBY_VISION_POLICY, DV_POLICY_FOLLOW_SINK, ConnectorType::CONN_TYPE_HDMI);

    if (usleep(100000) < 0)//100ms
        SYS_LOGE("usleep interrupt!\n");
    DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_DOLBY_VISION_ENABLE, DV_ENABLE, ConnectorType::CONN_TYPE_HDMI);
    DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_DOLBY_VISION_MODE, DV_MODE_IPT_TUNNEL, ConnectorType::CONN_TYPE_HDMI);
    if (usleep(100000) < 0)//100ms
        SYS_LOGE("usleep interrupt!\n");

    setHdrMode(HDR_MODE_AUTO);

    initGraphicsPriority();
}

void DisplayMode::setTvDolbyVisionDisable(void) {
    int  check_status_count = 0;

    //2. update sysfs
    DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_HDR_POLICY, HDR_POLICY_SINK, ConnectorType::CONN_TYPE_HDMI);

    DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_DOLBY_VISION_POLICY, DV_POLICY_FORCE_MODE, ConnectorType::CONN_TYPE_HDMI);
    DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_DOLBY_VISION_MODE, DV_MODE_BYPASS, ConnectorType::CONN_TYPE_HDMI);
    if (usleep(100000) < 0)//100ms
        SYS_LOGE("usleep interrupt!\n");
    std::string dvstatus = "";
    DisplayModeMgr::getInstance().getDisplayAttribute(DISPLAY_DOLBY_VISION_STATUS, dvstatus, ConnectorType::CONN_TYPE_HDMI);

    if (strcmp(dvstatus.c_str(), BYPASS_PROCESS)) {
        while (++check_status_count <30) {
            if (usleep(20000) < 0)//20ms
                SYS_LOGE("usleep interrupt!\n");
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
    char hdr_policy[MODE_LEN] = {0};
    getHdrStrategy(hdr_policy);

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
        } else if (strstr(hdr_policy, HDR_POLICY_FORCE)) {
            DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_HDR_POLICY, HDR_POLICY_FORCE, ConnectorType::CONN_TYPE_HDMI);
            if (isDolbyVisionEnable()) {
                DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_DOLBY_VISION_POLICY, HDR_POLICY_FORCE, ConnectorType::CONN_TYPE_HDMI);
            }
        }
    }

    if (usleep(100000) < 0)//100ms
        SYS_LOGE("usleep interrupt!\n");
    DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_DOLBY_VISION_ENABLE, DV_ENABLE, ConnectorType::CONN_TYPE_HDMI);

    if (strstr(hdr_policy, HDR_POLICY_FORCE)) {
        char hdr_force_mode[MODE_LEN] = {0};
        memset(hdr_force_mode, 0, MODE_LEN);
        getBootEnv(UBOOTENV_HDR_FORCE_MODE, hdr_force_mode);
        if (strstr(hdr_force_mode, FORCE_DV)) {
            DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_DOLBY_VISION_MODE, FORCE_DV, ConnectorType::CONN_TYPE_HDMI);
        } else if (strstr(hdr_force_mode, FORCE_HDR10)) {
            DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_DOLBY_VISION_MODE, FORCE_HDR10, ConnectorType::CONN_TYPE_HDMI);
        } else if (strstr(hdr_force_mode, FORCE_SDR)) {
            // 8bit or not
            std::string cur_ColorAttribute;
            DisplayModeMgr::getInstance().getDisplayAttribute(DISPLAY_HDMI_COLOR_ATTR, cur_ColorAttribute, ConnectorType::CONN_TYPE_HDMI);
            if (cur_ColorAttribute.find("8bit", 0) != std::string::npos) {
                DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_DOLBY_VISION_MODE, DV_ENABLE_FORCE_SDR_8BIT, ConnectorType::CONN_TYPE_HDMI);
            } else {
                DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_DOLBY_VISION_MODE, DV_ENABLE_FORCE_SDR_10BIT, ConnectorType::CONN_TYPE_HDMI);
            }
        }
    } else {
        DisplayModeMgr::getInstance().setDisplayAttribute(DISPLAY_DOLBY_VISION_MODE, DV_MODE_IPT_TUNNEL, ConnectorType::CONN_TYPE_HDMI);
    }

    if (usleep(100000) < 0)//100ms
        SYS_LOGE("usleep interrupt!\n");

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
    if (usleep(100000) < 0)//100ms
        SYS_LOGE("usleep interrupt!\n");
    std::string dvstatus = "";
    DisplayModeMgr::getInstance().getDisplayAttribute(DISPLAY_DOLBY_VISION_STATUS, dvstatus, ConnectorType::CONN_TYPE_HDMI);

    if (strcmp(dvstatus.c_str(), BYPASS_PROCESS)) {
        while (++check_status_count <30) {
            if (usleep(20000) < 0)//20ms
                SYS_LOGE("usleep interrupt!\n");
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
        //1. update dv env
        char tmp[10];
        char dvstatus[MODE_LEN]   = {0};

        sprintf(tmp, "%d", state);
        strcpy(mHdmidata.dv_info.ubootenv_dv_type, tmp);

        if (state == DOLBY_VISION_SET_DISABLE) {
            strcpy(mHdmidata.dv_info.dv_enable, "0");
        } else {
            strcpy(mHdmidata.dv_info.dv_enable, "1");
        }

        //Save user prefer dv mode only user change dv through UI
        setBootEnv(UBOOTENV_USER_DV_TYPE, mHdmidata.dv_info.ubootenv_dv_type);
        setBootEnv(UBOOTENV_DV_ENABLE, mHdmidata.dv_info.dv_enable);

        sprintf(dvstatus, "%d", state);
        setBootEnv(UBOOTENV_DOLBYSTATUS, dvstatus);

        if (isHWCProcess()) {
            std::string dv_mode = tmp;
            DisplayModeMgr::getInstance().setDvMode(dv_mode);
        } else {
            char cur_displaymode[MODE_LEN] = {0};
            getDisplayMode(cur_displaymode);
            setSourceOutputMode(cur_displaymode);
        }
    }
}

void DisplayMode::gethdrforcemode(char* value) {
    if (!value) {
        SYS_LOGE("%s value is NULL\n", __FUNCTION__);
        return;
    }

    bool ret = false;
    char hdr_force_mode[MODE_LEN] = {0};

    memset(hdr_force_mode, 0, MODE_LEN);
    ret = getBootEnv(UBOOTENV_HDR_FORCE_MODE, hdr_force_mode);
    if (ret) {
        strcpy(value, hdr_force_mode);
    } else {
        strcpy(value, FORCE_DV);
    }

    SYS_LOGI("get hdr force mode is [%s]", value);
}

void DisplayMode::getHdrStrategy(char* value) {
    char hdr_policy[MODE_LEN] = {0};

    memset(hdr_policy, 0, MODE_LEN);
    getBootEnv(UBOOTENV_HDR_POLICY, hdr_policy);

    if (strstr(hdr_policy, HDR_POLICY_SOURCE)) {
        strcpy(value, HDR_POLICY_SOURCE);
    } else if (strstr(hdr_policy, HDR_POLICY_SINK)){
        strcpy(value, HDR_POLICY_SINK);
    }  else if (strstr(hdr_policy, HDR_POLICY_FORCE)) {
        strcpy(value, HDR_POLICY_FORCE);
    }
    SYS_LOGI("%s is [%s]", __FUNCTION__, value);
}

void DisplayMode::getCurrentHdrStrategy(char* value) {
    std::string cur_hdr_policy;
    DisplayModeMgr::getInstance().getDisplayAttribute(DISPLAY_HDR_POLICY, cur_hdr_policy);

    strcpy(value, cur_hdr_policy.c_str());

    SYS_LOGI("%s is [%s]", __FUNCTION__, value);
}

void DisplayMode::setHdrStrategy(const char* type) {
    SYS_LOGI("policy:%s dv_type:%d", type, mHdmidata.dv_info.dv_type);

    char dvstatus[MODE_LEN] = {0};

    //1. update env policy
    setBootEnv(UBOOTENV_HDR_POLICY, (char *)type);
    if (strstr(type, HDR_POLICY_SINK)) {
        if (isDolbyVisionEnable()) {
            sprintf(dvstatus, "%d", mHdmidata.dv_info.dv_type);
            setBootEnv(UBOOTENV_DOLBYSTATUS, dvstatus);
        }
    } else if (strstr(type, HDR_POLICY_SOURCE)) {
        if (isDolbyVisionEnable()) {
            setBootEnv(UBOOTENV_DOLBYSTATUS, "0");
        }
    }

    //2. set current hdmi mode
    char cur_displaymode[MODE_LEN] = {0};
    getDisplayMode(cur_displaymode);
    setSourceOutputMode(cur_displaymode);
}

int32_t DisplayMode::getCurrentHdrPriority(void) {
    hdr_priority_e value = DOLBY_VISION_PRIORITY;

    std::string cur_hdr_priority;
    DisplayModeMgr::getInstance().getDisplayAttribute(DISPLAY_HDR_PRIORITY, cur_hdr_priority);

    uint32_t temp = 0;
    temp = atoi(cur_hdr_priority.c_str());
    switch (temp) {
        case DOLBY_VISION_PRIORITY:
            value = DOLBY_VISION_PRIORITY;
            break;
        case HDR10_PRIORITY:
            value = HDR10_PRIORITY;
            break;
        case SDR_PRIORITY:
            value = SDR_PRIORITY;
            break;
        case MESON_G_DV_HDR10_HLG:
            value = MESON_G_DV_HDR10_HLG;
            break;
        case MESON_G_DV_HDR10:
            value = MESON_G_DV_HDR10;
            break;
        case MESON_G_DV_HLG:
            value = MESON_G_DV_HLG;
            break;
        case MESON_G_HDR10_HLG:
            value = MESON_G_HDR10_HLG;
            break;
        case MESON_G_DV:
            value = MESON_G_DV;
            break;
        case MESON_G_HDR10:
            value = MESON_G_HDR10;
            break;
        case MESON_G_HLG:
            value = MESON_G_HLG;
            break;
        case MESON_G_SDR:
            value = MESON_G_SDR;
            break;
        default :
            break;
    }

    SYS_LOGI("%s is [0x%x]", __FUNCTION__, value);
    return (int32_t)value;
}

int32_t DisplayMode::getHdrPriority(void) {
    char hdr_priority[MODE_LEN] = {0};
    hdr_priority_e value = DOLBY_VISION_PRIORITY;

    memset(hdr_priority, 0, MODE_LEN);
    getBootEnv(UBOOTENV_HDR_PRIORITY, hdr_priority);

    uint32_t temp = 0;
    temp = atoi(hdr_priority);
    switch (temp) {
        case DOLBY_VISION_PRIORITY:
            value = DOLBY_VISION_PRIORITY;
            break;
        case HDR10_PRIORITY:
            value = HDR10_PRIORITY;
            break;
        case SDR_PRIORITY:
            value = SDR_PRIORITY;
            break;
        case MESON_G_DV_HDR10_HLG:
            value = MESON_G_DV_HDR10_HLG;
            break;
        case MESON_G_DV_HDR10:
            value = MESON_G_DV_HDR10;
            break;
        case MESON_G_DV_HLG:
            value = MESON_G_DV_HLG;
            break;
        case MESON_G_HDR10_HLG:
            value = MESON_G_HDR10_HLG;
            break;
        case MESON_G_DV:
            value = MESON_G_DV;
            break;
        case MESON_G_HDR10:
            value = MESON_G_HDR10;
            break;
        case MESON_G_HLG:
            value = MESON_G_HLG;
            break;
        case MESON_G_SDR:
            value = MESON_G_SDR;
            break;
        default :
            break;
    }

    SYS_LOGI("%s is [0x%x]", __FUNCTION__, value);
    return (int32_t)value;
}

void DisplayMode::setHdrPriority(const char* type) {
    SYS_LOGI("setHdrPriority is [%s]\n", type);

    setBootEnv(UBOOTENV_HDR_PRIORITY, (char *)type);

    //2. set hdmi mode
    char cur_displaymode[MODE_LEN] = {0};
    getDisplayMode(cur_displaymode);

    if (strstr(type, HDR_PRIORITY_TYPE[HDR10_PRIORITY])) {
        if (mpSceneProcess->isHDRSupportMode(cur_displaymode)) {
            setSourceOutputMode(cur_displaymode);
        } else {
            //1. get hdmi data
            char displaymode[MODE_LEN] = {0};
            hdmi_data_t data;

            memset(&data, 0, sizeof(hdmi_data_t));
            getHdmiData_cached(&data);
            getCommonData(&data);
            data.state = OUTPUT_MODE_STATE_INIT;
            data.isbestpolicy     = true;

            //2. scene logic process
            sceneProcess(&data);
            strcpy(displaymode, data.final_displaymode);
            setSourceOutputMode(displaymode);
        }
    } else {
        setSourceOutputMode(cur_displaymode);
    }
}

bool DisplayMode::isExitDovi() {
    bool ret = false;
    if (DISPLAY_TYPE_TV == mDisplayType) {
        ret = ((access(DOLBY_VISION_KO_DIR0_TV, F_OK) == 0)
                || (access(DOLBY_VISION_KO_DIR1_TV, F_OK) == 0));
    } else {
        ret = ((access(DOLBY_VISION_KO_DIR0, F_OK) == 0)
                || (access(DOLBY_VISION_KO_DIR1, F_OK) == 0));
    }

    SYS_LOGI("ret:%d\n", ret);

    return ret;
}

bool DisplayMode::isLoadDovi() {
    /*bit0: 0-> efuse, 1->no efuse; */
    /*bit1: 1->ko loaded*/
    /*bit2: 1-> value updated*/
    constexpr int dvSupported = ((1 << 0) | (1 << 1) | (1 <<2));
    int supportInfo;
    int len;
    char dv_info[MODE_LEN] = {0};

    len = pSysWrite->readSysfs(DV_SUPPORT_INFO, dv_info);
    if (len < 0) {
        SYS_LOGI("read %s error: %s\n", pSysWrite->getSysNode(DV_SUPPORT_INFO), strerror(errno));
        return false;
    } else {
        SYS_LOGI("dv_info:%s\n", dv_info);
        sscanf(dv_info, "%d", &supportInfo);
        return ((supportInfo & dvSupported) == dvSupported) ? true : false;
    }
}

void DisplayMode::setDolbyVisionSupport() {

    if ((pSysWrite->getPropertyBoolean(PROP_DOLBY_VISION_FEATURE, false))
            && (isExitDovi() || isLoadDovi())) {
        pSysWrite->setProperty(PROP_SUPPORT_DOLBY_VISION, "true");
    } else {
        pSysWrite->setProperty(PROP_SUPPORT_DOLBY_VISION, "false");
    }
}

/* *
 * @Description: get Current DV mode
 *
 * @result: if disable Dolby Vision return DOLBY_VISION_SET_DISABLE.
 *          if Current TV support the state saved in Mbox. return that state. like value of saved is 2, and TV support LL_YUV
 *          if Current TV not support the state saved in Mbox. but isDolbyVisionEnable() is enable.
 *             return state in priority queue. like value of saved is 2, But TV only Support LL_RGB, so system will return LL_RGB
 */
int DisplayMode::getDolbyVisionType() {
    std::string curDvEnable = "";
    std::string curDvLLPolicy = "";
    int curDvMode = DOLBY_VISION_SET_DISABLE;

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

    SYS_LOGI("%s curDvMode %d\n", __FUNCTION__, curDvMode);

    return curDvMode;
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
     * we will force change 0 into -1 here                         *
     *                                                             *
     ***************************************************************/

    if (!isTvSupportALLM()) {
        SYS_LOGI("setALLMMode: TV not support ALLM\n");
        return;
    }

    int perState = -1;
    char cur_allm_state[MODE_LEN] = {0};
    pSysWrite->readSysfs(AUTO_LOW_LATENCY_MODE, cur_allm_state);
    perState = atoi(cur_allm_state);
    if (perState == state) {
        SYS_LOGI("setALLMMode: the ALLM_Mode is not changed :%d\n", state);
        return;
    }

    char dv_mode[MAX_STR_LEN];
    bool isTVSupportDV = isTvSupportDolbyVision(dv_mode);

    char ubootenv_dv_enable[MODE_LEN] = {0};
    char cur_displaymode[MODE_LEN] = {0};
    char dv_displaymode[MODE_LEN] = {0};
    char ubootenv_dv_type[MODE_LEN] = {0};
    char curColorAttribute[MODE_LEN] = {0};
    std::string cur_ColorAttribute;

    switch (state) {
        case -1:
            [[fallthrough]];
        case 0:
            //1. disable allm
            pSysWrite->writeSysfs(AUTO_LOW_LATENCY_MODE, ALLM_MODE[0]);
            SYS_LOGI("setALLMMode: ALLM_Mode: %s", ALLM_MODE[0]);
            //2.1 get dv status before enable allm
            getBootEnv(UBOOTENV_DV_ENABLE, ubootenv_dv_enable);
            //2.2 get current hdmi output resolution
            getDisplayMode(cur_displaymode);
            //2.3 get current hdmi output color space
            DisplayModeMgr::getInstance().getDisplayAttribute(DISPLAY_HDMI_COLOR_ATTR, cur_ColorAttribute);
            strcpy(curColorAttribute, cur_ColorAttribute.c_str());
            //2.4 get dv max support resolution
            for (int i = sizeof(DISPLAY_MODE_LIST)/sizeof(char *) - 1; i >= 0; i--) {
                if (strstr(mHdmidata.dv_info.dv_displaymode, DISPLAY_MODE_LIST[i]) != NULL) {
                    strcpy(dv_displaymode, DISPLAY_MODE_LIST[i]);
                    break;
                }
            }
            //2.4 get dv type before enable allm
            getBootEnv(UBOOTENV_USER_DV_TYPE, ubootenv_dv_type);
            //3 enable dv
            //when TV and current resolution support dv and dv is enable before enable allm
            if (isTVSupportDV
                && !strcmp(ubootenv_dv_enable, "1")
                && (resolveResolutionValue(cur_displaymode, RESOLUTION_PRIORITY) <= resolveResolutionValue(dv_displaymode, RESOLUTION_PRIORITY))) {
                pSysWrite->writeSysfs(DISPLAY_HDMI_AVMUTE_SYSFS, "1");
                // restore doblyvision when set -1/0 to ALLM
                if (!strcmp(ubootenv_dv_type, "2") && strstr(curColorAttribute, "422,12bit") != NULL) {
                    enableDolbyVision(DOLBY_VISION_SET_ENABLE_LL_YUV);
                    mHdmidata.dv_info.dv_type = DOLBY_VISION_SET_ENABLE_LL_YUV;
                } else if (!strcmp(ubootenv_dv_type, "1") && strstr(curColorAttribute, "444,8bit") != NULL) {
                    enableDolbyVision(DOLBY_VISION_SET_ENABLE);
                    mHdmidata.dv_info.dv_type = DOLBY_VISION_SET_ENABLE;
                } else {
                    SYS_LOGE("can't enable dv for curColorAttribute: %s\n", curColorAttribute);
                }
                pSysWrite->writeSysfs(DISPLAY_HDMI_AVMUTE_SYSFS, "-1");
            }
            break;
        case 1:
            //1 disable dv
            //when TV support dv and dv is enable
            if (isTVSupportDV && isDolbyVisionEnable()) {
                mHdmidata.dv_info.dv_type = DOLBY_VISION_SET_DISABLE;
                // disable the doblyvision when ALLM enable
                pSysWrite->writeSysfs(DISPLAY_HDMI_AVMUTE_SYSFS, "1");
                disableDolbyVision(DOLBY_VISION_SET_DISABLE);
                pSysWrite->writeSysfs(DISPLAY_HDMI_AVMUTE_SYSFS, "-1");
            }
            //2. enable allm
            pSysWrite->writeSysfs(AUTO_LOW_LATENCY_MODE, ALLM_MODE[2]);
            SYS_LOGI("setALLMMode: ALLM_Mode: %s", ALLM_MODE[2]);
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
    if (!strcmp(switchName, UEVENT_HPD) && hpdstate[0] == '0') {
        notifyEvent(EVENT_HDMI_PLUG_OUT);
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
    }

    if (isHWCProcess()) {
        //update hdmi info when hdmi plug/suspend/resume
        memset(&mHdmidata, 0, sizeof(hdmi_data_t));
        mHdmidata.state = OUTPUT_MODE_STATE_POWER;
        getHdmiData(&mHdmidata);
        SYS_LOGI("onTxEvent boot config enable do nothing, just return");
        return;
    }
#endif

    //plugout or suspend,set dummy_l
    if (hpdstate && hpdstate[0] == '0') {
        SYS_LOGI("hwc will set dummy when hdmi plugout or suspend \n");
        return;
    }

    //hdmi edid parse ok
    setSourceDisplay((output_mode_state)outputState);
}

bool DisplayMode::frameRateDisplay(bool on) {
    pFrameRateAutoAdaption->setVideoLayerOn(on);
    return true;
}

void DisplayMode::onDispModeSyncEvent (const char* outputmode, int state) {
    SYS_LOGI("onDispModeSyncEvent outputmode:%s state: %d\n", outputmode, state);
    setSourceOutputMode(outputmode);
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

void DisplayMode::dumpCap(const ConstCharforSysNodeIndex index, const char * hint, char *result) {
    char logBuf[MAX_STR_LEN];
    pSysWrite->readSysfsOriginal(index, logBuf);

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
const char *DisplayMode::dvModeTypeToString(const char *dvMode) {
    const char * typeStr;
    if (strstr(dvMode, "current dv_mode = HDR10")) {
        typeStr = FORCE_MODE_TYPE[MESON_HDR_FORCE_MODE_HDR10];
    } else if (strstr(dvMode, "current dv_mode = IPT_TUNNEL")) {
        typeStr = FORCE_MODE_TYPE[MESON_HDR_FORCE_MODE_DV];
    } else if (strstr(dvMode, "current dv_mode = SDR8") ||
                  strstr(dvMode, "current dv_mode = SDR10")) {
        typeStr = FORCE_MODE_TYPE[MESON_HDR_FORCE_MODE_SDR];
    } else {
        typeStr = FORCE_MODE_TYPE[MESON_HDR_FORCE_MODE_INVALID];
    }
    return typeStr;
}

void DisplayMode::saveHdmiParamToEnv() {
    char outputMode[MODE_LEN] = {0};

    getDisplayMode(outputMode);

    // 1. check whether the TV changed or not, if changed save crc
    if (isEdidChange()) {
        SYS_LOGD("tv sink changed\n");
    }

    // 2. save coloattr/hdmimode to bootenv if mode is not null and not dummy_l
    if (strstr(outputMode, "cvbs") != NULL
        || strstr(outputMode, "pal") != NULL
        || strstr(outputMode, "ntsc") != NULL) {
        setBootEnv(UBOOTENV_CVBSMODE, (char *)outputMode);
    } else if (strcmp(outputMode, "null") && strcmp(outputMode, "dummy_l")) {
        std::string colorAttr;
        char colorDepth[MODE_LEN] = {0};
        char colorSpace[MODE_LEN] = {0};
        char dvstatus[MODE_LEN]   = {0};

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

        // 2.3 save dolby status/dv_type
        // In follow sink mode: 0:disable 1:STD(or enable dv) 2:LL YUV 3: LL RGB
        // In follow source mode: dv is disable  in uboot.
        if (isMboxSupportDolbyVision()) {
            sprintf(dvstatus, "%d", mHdmidata.dv_info.dv_type);
            setBootEnv(UBOOTENV_DOLBYSTATUS, dvstatus);

            setBootEnv(UBOOTENV_DV_ENABLE, mHdmidata.dv_info.dv_enable);

            SYS_LOGI("dvstatus %s dv_type %d dv_enable %s\n",
                dvstatus, mHdmidata.dv_info.dv_type, mHdmidata.dv_info.dv_enable);

        } else {
            SYS_LOGI("MBOX is not support dolby vision, dvstatus %s dv_type %d dv_enable %s\n",
                dvstatus, mHdmidata.dv_info.dv_type, mHdmidata.dv_info.dv_enable);
        }

        SYS_LOGI("colorattr: %s, outputMode %s, cd %s, cs %s\n",
            colorAttr.c_str(), outputMode, colorDepth, colorSpace);
    }
}

/* *
 * @Description: get perf hdmi display mode priority.
 * @params: store current perf hdmi mode.
 * */
bool DisplayMode::getPrefHdmiDispMode(char* mode) {
    bool ret = true;

    if (DISPLAY_TYPE_TV == mDisplayType) {
        char curMode[MODE_LEN] = {0};
        getDisplayMode(curMode);
        strcpy(mode, curMode);
    } else {
        //1. get hdmi data
        hdmi_data_t data;

        memset(&data, 0, sizeof(hdmi_data_t));
        getHdmiData(&data);
        data.state = OUTPUT_MODE_STATE_INIT;

        //2. scene logic process
        sceneProcess(&data);

        strcpy(mode, data.final_displaymode);
    }

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
    close(memDev);
    return true;

}

void DisplayMode::resetMemc() {
    int memDev = open(DISPLAY_MEMC_SYSFS, O_WRONLY);
    if (memDev < 0) {
        SYS_LOGE("resetMemc open %s fail. Error info [%s]\n", DISPLAY_MEMC_SYSFS, strerror(errno));
        return;
    }
    if (pSysWrite->getPropertyBoolean(PROP_DISPLAY_MEMC, false) && memDev > 0) {
        int value = 1;
        ioctl(memDev, MEMDEV_CONTRL, &value);
    }
    close(memDev);
}
