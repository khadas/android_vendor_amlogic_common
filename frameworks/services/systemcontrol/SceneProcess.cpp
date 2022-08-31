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
 *  @author   XiaoLiang.Wang
 *  @version  1.0
 *  @date     2016/06/13
 *  @par function description:
 *  - 1 3d set for player
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

#include "SceneProcess.h"

//sysfs define
#define DISPLAY_HDMI_DEEP_COLOR         "/sys/class/amhdmitx/amhdmitx0/dc_cap"//RX supoort deep color
#define DISPLAY_HDMI_VALID_MODE         "/sys/class/amhdmitx/amhdmitx0/valid_mode"//testing if tv support this displaymode and  deepcolor combination, then if cat result is 1: support, 0: not

#define DOLBY_VISION_LL_RGB             3
#define DOLBY_VISION_LL_YUV             2
#define DOLBY_VISION_STD_ENABLE         1
#define DOLBY_VISION_DISABLE            0

#define DV_MODE_720P50HZ                "720p50hz"
#define DV_MODE_720P                    "720p60hz"
#define DV_MODE_1080P24HZ               "1080p24hz"
#define DV_MODE_1080P50HZ               "1080p50hz"
#define DV_MODE_1080P                   "1080p60hz"
#define DV_MODE_4K2K24HZ                "2160p24hz"
#define DV_MODE_4K2K25HZ                "2160p25hz"
#define DV_MODE_4K2K30HZ                "2160p30hz"
#define DV_MODE_4K2K50HZ                "2160p50hz"
#define DV_MODE_4K2K60HZ                "2160p60hz"
#define DV_MODE_LIST_SIZE               10

#define MODE_480I                       "480i60hz"
#define MODE_480P                       "480p60hz"
#define MODE_480CVBS                    "480cvbs"
#define MODE_576I                       "576i50hz"
#define MODE_576P                       "576p50hz"
#define MODE_576CVBS                    "576cvbs"
#define MODE_720P50HZ                   "720p50hz"
#define MODE_720P                       "720p60hz"
#define MODE_768P                       "768p60hz"
#define MODE_1080P24HZ                  "1080p24hz"
#define MODE_1080I50HZ                  "1080i50hz"
#define MODE_1080P50HZ                  "1080p50hz"
#define MODE_1080I                      "1080i60hz"
#define MODE_1080P                      "1080p60hz"
#define MODE_4K2K24HZ                   "2160p24hz"
#define MODE_4K2K25HZ                   "2160p25hz"
#define MODE_4K2K30HZ                   "2160p30hz"
#define MODE_4K2K50HZ                   "2160p50hz"
#define MODE_4K2K60HZ                   "2160p60hz"
#define MODE_4K2KSMPTE                  "smpte24hz"
#define MODE_4K2KSMPTE30HZ              "smpte30hz"
#define MODE_4K2KSMPTE50HZ              "smpte50hz"
#define MODE_4K2KSMPTE60HZ              "smpte60hz"
#define MODE_PANEL                      "panel"
#define MODE_PAL_M                      "pal_m"
#define MODE_PAL_N                      "pal_n"
#define MODE_NTSC_M                     "ntsc_m"

#define DEFAULT_COLOR_FORMAT_4K         "420,8bit"
#define DEFAULT_COLOR_FORMAT            "444,8bit"

#define DEFAULT_HDMI_MODE               "480p60hz"


#define COLOR_YCBCR444_12BIT             "444,12bit"
#define COLOR_YCBCR444_10BIT             "444,10bit"
#define COLOR_YCBCR444_8BIT              "444,8bit"
#define COLOR_YCBCR422_12BIT             "422,12bit"
#define COLOR_YCBCR422_10BIT             "422,10bit"
#define COLOR_YCBCR422_8BIT              "422,8bit"
#define COLOR_YCBCR420_12BIT             "420,12bit"
#define COLOR_YCBCR420_10BIT             "420,10bit"
#define COLOR_YCBCR420_8BIT              "420,8bit"
#define COLOR_RGB_12BIT                  "rgb,12bit"
#define COLOR_RGB_10BIT                  "rgb,10bit"
#define COLOR_RGB_8BIT                   "rgb,8bit"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

enum {
    DISPLAY_MODE_480I                   = 0,
    DISPLAY_MODE_480P                   = 1,
    DISPLAY_MODE_480CVBS                = 2,
    DISPLAY_MODE_576I                   = 3,
    DISPLAY_MODE_576P                   = 4,
    DISPLAY_MODE_576CVBS                = 5,
    DISPLAY_MODE_720P50HZ               = 6,
    DISPLAY_MODE_720P                   = 7,
    DISPLAY_MODE_1080P24HZ              = 8,
    DISPLAY_MODE_1080I50HZ              = 9,
    DISPLAY_MODE_1080P50HZ              = 10,
    DISPLAY_MODE_1080I                  = 11,
    DISPLAY_MODE_1080P                  = 12,
    DISPLAY_MODE_4K2K24HZ               = 13,
    DISPLAY_MODE_4K2K25HZ               = 14,
    DISPLAY_MODE_4K2K30HZ               = 15,
    DISPLAY_MODE_4K2K50HZ               = 16,
    DISPLAY_MODE_4K2K60HZ               = 17,
    DISPLAY_MODE_4K2KSMPTE              = 18,
    DISPLAY_MODE_4K2KSMPTE30HZ          = 19,
    DISPLAY_MODE_4K2KSMPTE50HZ          = 20,
    DISPLAY_MODE_4K2KSMPTE60HZ          = 21,
    DISPLAY_MODE_768P                   = 22,
    DISPLAY_MODE_PANEL                  = 23,
    DISPLAY_MODE_PAL_M                  = 24,
    DISPLAY_MODE_PAL_N                  = 25,
    DISPLAY_MODE_NTSC_M                 = 26,
    DISPLAY_MODE_TOTAL                  = 27
};

static const char* DV_MODE_LIST[DV_MODE_LIST_SIZE] = {
    DV_MODE_720P,
    DV_MODE_720P50HZ,
    DV_MODE_1080P24HZ,
    DV_MODE_1080P50HZ,
    DV_MODE_1080P,
    DV_MODE_4K2K24HZ,
    DV_MODE_4K2K25HZ,
    DV_MODE_4K2K30HZ,
    DV_MODE_4K2K50HZ,
    DV_MODE_4K2K60HZ,
};

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

//this is prior selected list  of 4k2k50hz, 4k2k60hz smpte50hz, smpte60hz
static const char* COLOR_ATTRIBUTE_LIST1[] = {
    COLOR_YCBCR420_10BIT,
    COLOR_YCBCR422_12BIT,
    COLOR_YCBCR420_8BIT,
    COLOR_YCBCR444_8BIT,
    COLOR_RGB_8BIT,
};

//this is prior selected list  of other display mode
static const char* COLOR_ATTRIBUTE_LIST2[] = {
    COLOR_YCBCR444_10BIT,
    COLOR_YCBCR422_12BIT,
    COLOR_RGB_10BIT,
    COLOR_YCBCR444_8BIT,
    COLOR_RGB_8BIT,
};

//this is prior selected list  of Low Power Mode 4k2k50hz, 4k2k60hz smpte50hz, smpte60hz
static const char* COLOR_ATTRIBUTE_LIST3[] = {
    COLOR_YCBCR420_8BIT,
    COLOR_YCBCR420_10BIT,
    COLOR_YCBCR422_8BIT,
    COLOR_YCBCR422_10BIT,
    COLOR_YCBCR444_8BIT,
    COLOR_RGB_8BIT,
    COLOR_YCBCR420_12BIT,
    COLOR_YCBCR422_12BIT,
};

//this is prior selected list of Low Power Mode other display mode
static const char* COLOR_ATTRIBUTE_LIST4[] = {
    COLOR_YCBCR444_8BIT,
    COLOR_YCBCR422_8BIT,
    COLOR_RGB_8BIT,
    COLOR_YCBCR444_10BIT,
    COLOR_YCBCR422_10BIT,
    COLOR_RGB_10BIT,
    COLOR_YCBCR444_12BIT,
    COLOR_YCBCR422_12BIT,
    COLOR_RGB_12BIT,
};

SceneProcess::SceneProcess()
    :mSceneLock(PTHREAD_MUTEX_INITIALIZER) {
    mpSysWrite  = new SysWrite();
}

SceneProcess::~SceneProcess() {
    delete mpSysWrite;
}

int SceneProcess::updateDolbyVisionType(void) {
    char type[MODE_LEN];
    char dv_deepcolor[MODE_LEN];

    //1. read dolby vision mode from prop(maybe need to env)
    strcpy(type, mScene_Input_Info.dv_input_info.ubootenv_dv_type);
    strcpy(dv_deepcolor, mScene_Input_Info.dv_input_info.dv_deepcolor);
    SYS_LOGI("ubootenv_dv_type %s dv_deepcolor:%s\n", type, dv_deepcolor);

    //2. check tv support or not
    if ((strstr(type, "1") != NULL) && strstr(dv_deepcolor, "DV_RGB_444_8BIT") != NULL) {
        return DOLBY_VISION_STD_ENABLE;
    } else if ((strstr(type, "2") != NULL) && strstr(dv_deepcolor, "LL_YCbCr_422_12BIT") != NULL) {
        return DOLBY_VISION_LL_YUV;
    } else if ((strstr(type, "3") != NULL)
        && ((strstr(dv_deepcolor, "LL_RGB_444_12BIT") != NULL) ||
        (strstr(dv_deepcolor, "LL_RGB_444_10BIT") != NULL))) {
        return DOLBY_VISION_LL_RGB;
    } else if (strstr(type, "0") != NULL) {
        return DOLBY_VISION_DISABLE;
    }

    //3. dolby vision best policy:STD->LL_YUV->LL_RGB for netflix request
    //   dolby vision best policy:LL_YUV->STD->LL_RGB for dolby vision request
    if ((strstr(dv_deepcolor, "DV_RGB_444_8BIT") != NULL) ||
        (strstr(dv_deepcolor, "LL_YCbCr_422_12BIT") != NULL)) {
            if (strstr(dv_deepcolor, "DV_RGB_444_8BIT") != NULL) {
                return DOLBY_VISION_STD_ENABLE;
            } else if (strstr(dv_deepcolor, "LL_YCbCr_422_12BIT") != NULL) {
                return DOLBY_VISION_LL_YUV;
            }
    } else if ((strstr(dv_deepcolor, "LL_RGB_444_12BIT") != NULL) ||
        (strstr(dv_deepcolor, "LL_RGB_444_10BIT") != NULL)) {
        return DOLBY_VISION_LL_RGB;
    }

    return DOLBY_VISION_DISABLE;
}

void SceneProcess::updateDolbyVisionAttr(int dolbyvision_type, char * dv_attr) {
    int  dv_type = dolbyvision_type;

    switch (dv_type) {
        case DOLBY_VISION_STD_ENABLE:
            strcpy(dv_attr, "444,8bit");
            break;
        case DOLBY_VISION_LL_YUV:
            strcpy(dv_attr, "422,12bit");
            break;
        case DOLBY_VISION_LL_RGB:
            if (strstr(mScene_Input_Info.dv_input_info.dv_deepcolor, "LL_RGB_444_12BIT") != NULL) {
                strcpy(dv_attr, "444,12bit");
            } else if (strstr(mScene_Input_Info.dv_input_info.dv_deepcolor, "LL_RGB_444_10BIT") != NULL) {
                strcpy(dv_attr, "444,10bit");
            }
            break;
        default:
            strcpy(dv_attr, "444,8bit");
            break;
    }

    SYS_LOGI("dv_type :%d dv_attr:%s", dv_type, dv_attr);
}

bool SceneProcess::IsBestPolicy() {
    return mScene_Input_Info.isbestpolicy;
}

bool SceneProcess::IsFrameratePriority() {
    return mScene_Input_Info.hdmi_input_info.isframeratepriority;
}

bool SceneProcess::IsSupport4K() {
    return mScene_Input_Info.hdmi_input_info.isSupport4K;
}

bool SceneProcess::IsSupport4K30Hz() {
    return mScene_Input_Info.hdmi_input_info.isSupport4K30Hz;
}

bool SceneProcess::IsSupportDeepColor() {
    return mScene_Input_Info.hdmi_input_info.isDeepColor;
}

bool SceneProcess::isLowPowerMode() {
    return mScene_Input_Info.hdmi_input_info.isLowPowerMode;
}

void SceneProcess::updateDolbyVisionDisplayMode(char * cur_outputmode, int dv_type, char * final_displaymode) {
    char dv_displaymode[MODE_LEN] = {0};

    //1. read tv dv_mode
    for (int i = DV_MODE_LIST_SIZE - 1; i >= 0; i--) {
        if (strstr(mScene_Input_Info.dv_input_info.dv_displaymode, DV_MODE_LIST[i]) != NULL) {
            strcpy(dv_displaymode, DV_MODE_LIST[i]);
        }
    }

    if (IsBestPolicy()) {
        if (!strcmp(dv_displaymode, DV_MODE_4K2K60HZ)) {
            if (dv_type == DOLBY_VISION_LL_RGB) {
                strcpy(final_displaymode, DV_MODE_1080P);
            } else {
                strcpy(final_displaymode, DV_MODE_4K2K60HZ);
            }
        } else {
            if (!strcmp(dv_displaymode, DV_MODE_4K2K30HZ)
                || !strcmp(dv_displaymode, DV_MODE_4K2K25HZ) || !strcmp(dv_displaymode, DV_MODE_4K2K24HZ)) {
                strcpy(final_displaymode, DV_MODE_1080P);
            } else {
                strcpy(final_displaymode, dv_displaymode);
            }
        }
    } else {
        if ((resolveResolutionValue(cur_outputmode) > resolveResolutionValue(dv_displaymode))
            || (strstr(cur_outputmode, "smpte") != NULL) || (strstr(cur_outputmode, "i") != NULL)) {
            strcpy(final_displaymode, dv_displaymode);
        } else {
            strcpy(final_displaymode, cur_outputmode);
        }
    }

    SYS_LOGI("final_displaymode:%s, cur_outputmode:%s, dv_displaymode:%s", final_displaymode, cur_outputmode, dv_displaymode);
}

int64_t SceneProcess::resolveResolutionValue(const char *mode) {
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

    if (IsFrameratePriority()) {
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

//get the highest hdmi mode by edid
void SceneProcess::getHighestHdmiMode(char* mode) {
    char value[MODE_LEN] = {0};
    char tempMode[MODE_LEN] = {0};

    char* startpos;
    char* destpos;

    startpos = mScene_Input_Info.hdmi_input_info.disp_cap;
    strcpy(value, DEFAULT_HDMI_MODE);

    while (strlen(startpos) > 0) {
        //get edid resolution to tempMode in order.
        destpos = strstr(startpos, "\n");
        if (NULL == destpos)
            break;
        memset(tempMode, 0, MODE_LEN);
        strncpy(tempMode, startpos, destpos - startpos);
        startpos = destpos + 1;
        if ((IsSupport4K() == false)
            &&(strstr(tempMode, "2160") || strstr(tempMode, "smpte"))) {
            SYS_LOGE("This platform not support : %s\n", tempMode);
            continue;
        }

        if (tempMode[strlen(tempMode) - 1] == '*') {
            tempMode[strlen(tempMode) - 1] = '\0';
        }

        if (resolveResolutionValue(tempMode) > resolveResolutionValue(value)) {
            memset(value, 0, MODE_LEN);
            strcpy(value, tempMode);
        }
    }

    strcpy(mode, value);
    SYS_LOGI("set HDMI to highest edid mode: %s\n", mode);
}

//check if the edid support current hdmi mode
void SceneProcess::filterHdmiMode(char* mode) {
    char *pCmp = mScene_Input_Info.hdmi_input_info.disp_cap;
    while ((pCmp - mScene_Input_Info.hdmi_input_info.disp_cap) < (int)strlen(mScene_Input_Info.hdmi_input_info.disp_cap)) {
        char *pos = strchr(pCmp, 0x0a);
        if (NULL == pos)
            break;

        int step = 1;
        if (*(pos - 1) == '*') {
            pos -= 1;
            step += 1;
        }
        if (!strncmp(pCmp, mScene_Input_Info.cur_displaymode, pos - pCmp)) {
            strncpy(mode, pCmp, pos - pCmp);
            return;
        }
        pCmp = pos + step;
    }

    //old mode is not support in this TV, so switch to best mode.
    getHighestHdmiMode(mode);
}

void SceneProcess::getHdmiOutputMode(char* mode) {
    /* Fall back to 480p if EDID can't be parsed */
    if (strcmp(mScene_Input_Info.hdmi_input_info.edidParsing, "ok")) {
        strcpy(mode, DEFAULT_HDMI_MODE);
        SYS_LOGE("EDID parsing error detected\n");
        return;
    }

    if (IsBestPolicy()) {
        getHighestHdmiMode(mode);
    } else {
        filterHdmiMode(mode);
    }
    SYS_LOGI("set HDMI mode to %s\n", mode);
}

bool SceneProcess::initColorAttribute(char* supportedColorList, int len) {
    int count = 0;
    bool result = false;

    if (supportedColorList != NULL)
        memset(supportedColorList, 0, len);

    while (true) {
        //mSysWrite.readSysfsOriginal(DISPLAY_HDMI_DEEP_COLOR, supportedColorList);
        mpSysWrite->readSysfs(DISPLAY_HDMI_DEEP_COLOR, supportedColorList);
        if (strlen(supportedColorList) > 0) {
            result = true;
            break;
        }

        if (count++ >= 5) {
            break;
        }
        usleep(500000);
    }

    return result;
}

bool SceneProcess::isModeSupportDeepColorAttr(const char *mode, const char * color) {
    char valueStr[10] = {0};
    char outputmode[MODE_LEN] = {0};

    strcpy(outputmode, mode);
    strcat(outputmode, color);

    //try support or not
    mpSysWrite->writeSysfs(DISPLAY_HDMI_VALID_MODE, outputmode);
    mpSysWrite->readSysfs(DISPLAY_HDMI_VALID_MODE, valueStr);

    return atoi(valueStr) ? true : false;
}

void SceneProcess::getBestHdmiDeepColorAttr(const char *outputmode, char* colorAttribute) {
    char *pos = NULL;
    int length = 0;
    const char **colorList = NULL;
    char supportedColorList[MAX_STR_LEN];
    if (!initColorAttribute(supportedColorList, MAX_STR_LEN)) {
        SYS_LOGE("initColorAttribute fail\n");
        return;
    }

    //filter some color value options, aimed at some modes.
    if (!strcmp(outputmode, MODE_4K2K60HZ) || !strcmp(outputmode, MODE_4K2K50HZ)
        || !strcmp(outputmode, MODE_4K2KSMPTE60HZ) || !strcmp(outputmode, MODE_4K2KSMPTE50HZ)) {
        if (isLowPowerMode()) {
            colorList = COLOR_ATTRIBUTE_LIST3;
            length = ARRAY_SIZE(COLOR_ATTRIBUTE_LIST3);
        } else {
            colorList = COLOR_ATTRIBUTE_LIST1;
            length = ARRAY_SIZE(COLOR_ATTRIBUTE_LIST1);
        }
    } else {
        if (isLowPowerMode()) {
            colorList = COLOR_ATTRIBUTE_LIST4;
            length = ARRAY_SIZE(COLOR_ATTRIBUTE_LIST4);
        } else {
            colorList = COLOR_ATTRIBUTE_LIST2;
            length = ARRAY_SIZE(COLOR_ATTRIBUTE_LIST2);
        }
    }

    for (int i = 0; i < length; i++) {
        if ((pos = strstr(supportedColorList, colorList[i])) != NULL) {
            if (isModeSupportDeepColorAttr(outputmode, colorList[i])) {
                SYS_LOGI("support current mode:[%s], deep color:[%s]\n", outputmode, colorList[i]);

                strcpy(colorAttribute, colorList[i]);
                break;
            }
        }
    }
}

void SceneProcess::getHdmiColorAttribute(const char* outputmode, char* colorAttribute, int state) {
    char supportedColorList[MAX_STR_LEN];

    //if read /sys/class/amhdmitx/amhdmitx0/dc_cap is null.
    //use default color format
    if (!initColorAttribute(supportedColorList, MAX_STR_LEN)) {
        if (!strcmp(outputmode, MODE_4K2K60HZ) || !strcmp(outputmode, MODE_4K2K50HZ)
            || !strcmp(outputmode, MODE_4K2KSMPTE60HZ) || !strcmp(outputmode, MODE_4K2KSMPTE50HZ)) {
            strcpy(colorAttribute, DEFAULT_COLOR_FORMAT_4K);
        } else {
            strcpy(colorAttribute, DEFAULT_COLOR_FORMAT);
        }

        SYS_LOGE("Error!!! Do not find sink color list, use default color attribute:%s\n", colorAttribute);
        return;
    }

    //if bestpolicy is disable use ubootenv.var.colorattribute
    if (IsBestPolicy() == false) {
        char colorTemp[MODE_LEN] = {0};
        strcpy(colorTemp, mScene_Input_Info.hdmi_input_info.ubootenv_colorattribute);
        if (isModeSupportDeepColorAttr(outputmode, colorTemp)) {
            strcpy(colorAttribute, mScene_Input_Info.hdmi_input_info.ubootenv_colorattribute);
        } else {
            getBestHdmiDeepColorAttr(outputmode,  colorAttribute);
        }
    } else {
        getBestHdmiDeepColorAttr(outputmode,  colorAttribute);
    }

    //if colorAttr is null above steps, will defines a initial value to it
    if (!strstr(colorAttribute, "bit")) {
        strcpy(colorAttribute, DEFAULT_COLOR_FORMAT);
    }

    SYS_LOGI("get hdmi color attribute : [%s], outputmode is: [%s] , and support color list is: [%s]\n",
        colorAttribute, outputmode, supportedColorList);
}

void SceneProcess::updateHdmiDeepColor(scene_state state, const char* outputmode, char* colorAttribute) {
    if (IsSupportDeepColor()) {
        getHdmiColorAttribute(outputmode, colorAttribute, (int)state);
    } else {
        strcpy(colorAttribute, "default");
    }

    SYS_LOGI("colorAttribute = %s\n", colorAttribute);
}

void SceneProcess::SetSceneInputInfo(scene_input_info_t* input_info) {
    pthread_mutex_lock(&mSceneLock);

    memcpy(&mScene_Input_Info, input_info, sizeof(scene_input_info_t));
    //common info
    SYS_LOGI("state:%d, isbestpolicy:%d, cur_displaymode:%s",
        mScene_Input_Info.state,
        mScene_Input_Info.isbestpolicy,
        mScene_Input_Info.cur_displaymode);
    SYS_LOGI("isframeratepriority:%d, isLowPowerMode:%d, isDeepColor:%d",
        mScene_Input_Info.hdmi_input_info.isframeratepriority,
        mScene_Input_Info.hdmi_input_info.isLowPowerMode,
        mScene_Input_Info.hdmi_input_info.isDeepColor);

    //dolby vision info
    SYS_LOGI("dv_cap:%s, ubootenv_dv_type:%s, dv_deepcolor:%s, dv_displaymode:%s",
        mScene_Input_Info.dv_input_info.dv_cap,
        mScene_Input_Info.dv_input_info.ubootenv_dv_type,
        mScene_Input_Info.dv_input_info.dv_deepcolor,
        mScene_Input_Info.dv_input_info.dv_displaymode);
    //hdmi info
    SYS_LOGI("sinkType:%d, edidParsing:%s",
        mScene_Input_Info.hdmi_input_info.sinkType,
        mScene_Input_Info.hdmi_input_info.edidParsing);
    SYS_LOGI("ubootenv_cvbsmode:%s, ubootenv_colorattribute:%s",
        mScene_Input_Info.hdmi_input_info.ubootenv_cvbsmode,
        mScene_Input_Info.hdmi_input_info.ubootenv_colorattribute);

    pthread_mutex_unlock(&mSceneLock);

}

void SceneProcess::DolbyVisionSceneProcess(scene_output_info_t* output_info) {
    pthread_mutex_lock(&mSceneLock);

    //1. update dolby vision output type
    int dv_type = DOLBY_VISION_DISABLE;
    dv_type = updateDolbyVisionType();
    mScene_output_info.dv_type = dv_type;
    SYS_LOGI("dv type:%d", mScene_output_info.dv_type);

    //2. update dolby vision output output mode and colorspace
    //2.1 update dolby vision deepcolor
    char dv_attr[MODE_LEN] = {0};
    updateDolbyVisionAttr(dv_type, dv_attr);
    strcpy(mScene_output_info.final_deepcolor, dv_attr);
    SYS_LOGI("dv final_deepcolor:%s", mScene_output_info.final_deepcolor);

    //2.2 update dolby vision output mode
    char final_displaymode[MODE_LEN] = {0};
    char cur_displaymode[MODE_LEN] = {0};
    strcpy(cur_displaymode, mScene_Input_Info.cur_displaymode);

    updateDolbyVisionDisplayMode(cur_displaymode, dv_type, final_displaymode);
    strcpy(mScene_output_info.final_displaymode, final_displaymode);
    SYS_LOGI("dv final_displaymode:%s", mScene_output_info.final_displaymode);

    //2.3 return output info
    strcpy(output_info->final_displaymode, mScene_output_info.final_displaymode);
    strcpy(output_info->final_deepcolor, mScene_output_info.final_deepcolor);
    output_info->dv_type = mScene_output_info.dv_type;

    pthread_mutex_unlock(&mSceneLock);
}

void SceneProcess::SDRSceneProcess(scene_output_info_t* output_info) {
    pthread_mutex_lock(&mSceneLock);

    if ((mScene_Input_Info.state == SCENE_STATE_INIT) ||
        (mScene_Input_Info.state == SCENE_STATE_POWER)) {
        //1. choose resolution, frame rate
        char outputmode[MODE_LEN] = {0};

        if (SINK_TYPE_NONE != mScene_Input_Info.hdmi_input_info.sinkType) {
            getHdmiOutputMode(outputmode);
        } else {
            strcpy(outputmode, mScene_Input_Info.hdmi_input_info.ubootenv_cvbsmode);
        }

        if (strlen(outputmode) == 0) {
            strcpy(outputmode, DEFAULT_HDMI_MODE);
        }

        strcpy(mScene_output_info.final_displaymode, outputmode);
        SYS_LOGI("final_displaymode:%s\n", mScene_output_info.final_displaymode);
    } else if (mScene_Input_Info.state == SCENE_STATE_SWITCH) {
        //1. doesn't read hdmi info for ui switch scene
        //   choose resolution, frame rate
        strcpy(mScene_output_info.final_displaymode, mScene_Input_Info.cur_displaymode);
        SYS_LOGI("final_displaymode:%s\n", mScene_output_info.final_displaymode);
    }

    //2. choose color format, bit-depth
    char colorAttribute[MODE_LEN] = {0};
    updateHdmiDeepColor(mScene_Input_Info.state, mScene_output_info.final_displaymode, colorAttribute);
    strcpy(mScene_output_info.final_deepcolor, colorAttribute);
    SYS_LOGI("final_deepcolor = %s\n", mScene_output_info.final_deepcolor);

    strcpy(output_info->final_displaymode, mScene_output_info.final_displaymode);
    strcpy(output_info->final_deepcolor, mScene_output_info.final_deepcolor);

    pthread_mutex_unlock(&mSceneLock);
}

