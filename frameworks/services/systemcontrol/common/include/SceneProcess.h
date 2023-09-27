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
 *  @author   kaifu.hu
 *  @version  1.0
 *  @date     2020/11/06
 *  @par function description:
 *  scene logic process
 */

#ifndef ANDROID_SCENEPROCESS_H
#define ANDROID_SCENEPROCESS_H

#include "common.h"
#include "SysWrite.h"

//mode definition
#define MODE_1024x600p                  "1024x600p60hz"
#define MODE_800x480p                   "800x480p60hz"
#define MODE_640x480P                   "640x480p60hz"
#define MODE_480I                       "480i60hz"
#define MODE_480P                       "480p60hz"
#define MODE_480CVBS                    "480cvbs"
#define MODE_576I                       "576i50hz"
#define MODE_576P                       "576p50hz"
#define MODE_576CVBS                    "576cvbs"
#define MODE_720P50HZ                   "720p50hz"
#define MODE_720P                       "720p60hz"
#define MODE_720P100HZ                  "1280x720p100hz"
#define MODE_720P120HZ                  "1280x720p120hz"
#define MODE_768P                       "768p60hz"
#define MODE_1080P24HZ                  "1080p24hz"
#define MODE_1080P25HZ                  "1080p25hz"
#define MODE_1080P30HZ                  "1080p30hz"
#define MODE_1080I50HZ                  "1080i50hz"
#define MODE_1080P50HZ                  "1080p50hz"
#define MODE_1080I                      "1080i60hz"
#define MODE_1080P                      "1080p60hz"
#define MODE_1080P100HZ                 "1920x1080p100hz"
#define MODE_1080P120HZ                 "1920x1080p120hz"
#define MODE_1440P50HZ                  "2560x1440p50hz"
#define MODE_1440P60HZ                  "2560x1440p60hz"
#define MODE_1440P100HZ                 "2560x1440p100hz"
#define MODE_1440P120HZ                 "2560x1440p120hz"
#define MODE_4K2K24HZ                   "2160p24hz"
#define MODE_4K2K25HZ                   "2160p25hz"
#define MODE_4K2K30HZ                   "2160p30hz"
#define MODE_4K2K50HZ                   "2160p50hz"
#define MODE_4K2K60HZ                   "2160p60hz"
#define MODE_4K2K100HZ                  "3840x2160p100hz"
#define MODE_4K2K120HZ                  "3840x2160p120hz"
#define MODE_4K2KSMPTE                  "smpte24hz"
#define MODE_4K2KSMPTE30HZ              "smpte30hz"
#define MODE_4K2KSMPTE50HZ              "smpte50hz"
#define MODE_4K2KSMPTE60HZ              "smpte60hz"
#define MODE_8K4K24HZ                   "7680x4320p24hz"
#define MODE_8K4K25HZ                   "7680x4320p25hz"
#define MODE_8K4K30HZ                   "7680x4320p30hz"
#define MODE_8K4K48HZ                   "7680x4320p48hz"
#define MODE_8K4K50HZ                   "7680x4320p50hz"
#define MODE_8K4K60HZ                   "7680x4320p60hz"
#define MODE_PANEL                      "panel"
#define MODE_PAL_M                      "pal_m"
#define MODE_PAL_N                      "pal_n"
#define MODE_NTSC_M                     "ntsc_m"

//default value
#define DEFAULT_COLOR_FORMAT_4K         "420,8bit"
#define DEFAULT_COLOR_FORMAT            "rgb,8bit"

#define DEFAULT_HDMI_MODE               "720p60hz"

/*
 * bit0-bit3 for hdr strategy1
 * 0 → original cap
 * 1 → disable dolby vision cap
 * 2 → disable dolby vision  and hdr cap
 * bit4-bit7 for hdr strategy2
 * bit4: 1 → disable dv, 0 → enable dv
 * bit5: 1 → disable hdr10/hdr10+, 0 → enable hdr10/hdr10+
 * bit6: 1→ disable hlg,  0 → enable hlg
 * bit7-bit27:reverse
 * bit28-bit31 choose strategy
 * 0：strategy1
 * 1：strategy2
*/
typedef enum hdr_priority {
    DOLBY_VISION_PRIORITY = 0,
    HDR10_PRIORITY        = 1,
    SDR_PRIORITY          = 2,
    MESON_G_DV_HDR10_HLG        = 0x10000000,
    MESON_G_DV_HDR10            = 0x10000040,
    MESON_G_DV_HLG              = 0x10000020,
    MESON_G_HDR10_HLG           = 0x10000010,
    MESON_G_DV                  = 0x10000060,
    MESON_G_HDR10               = 0x10000050,
    MESON_G_HLG                 = 0x10000030,
    MESON_G_SDR                 = 0x10000070,
}hdr_priority_e;

typedef enum {
    HDR_POLICY_SINK   = 0,
    HDR_POLICY_SOURCE = 1,
    HDR_POLICY_FORCE  = 2,
}hdr_policy_e;

typedef enum hdr_force_mode {
    MESON_HDR_FORCE_MODE_INVALID    = 0,
    MESON_HDR_FORCE_MODE_SDR        = 1,
    MESON_HDR_FORCE_MODE_DV         = 2,
    MESON_HDR_FORCE_MODE_HDR10      = 3,
    MESON_HDR_FORCE_MODE_HDR10PLUS  = 4,  //need to do
    MESON_HDR_FORCE_MODE_HLG        = 5,
} hdr_force_mode_e;

typedef enum {
    SCENE_STATE_INIT               = 0,//boot
    SCENE_STATE_POWER              = 1,//hot plug or suspend/resume
    SCENE_STATE_SWITCH             = 2,//user switch the mode
    SCENE_STATE_SWITCH_ADAPTER     = 3,//video auto switch the mode
    SCENE_STATE_RESERVE            = 4,
    SCENE_STATE_ADAPTER_END        = 5 //end hint video auto switch the mode
}scene_state;

typedef enum {
    SINK_TYPE_NONE                 = 0, //hdmi plug out,use cvbs
    SINK_TYPE_SINK                 = 1,
    SINK_TYPE_REPEATER             = 2,
    SINK_TYPE_RESERVE              = 3
}sink_type;

enum {
    RESOLUTION_PRIORITY = 0,
    FRAMERATE_PRIORITY  = 1,
};

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
    DISPLAY_MODE_1080P25HZ              = 9,
    DISPLAY_MODE_1080P30HZ              = 10,
    DISPLAY_MODE_1080I50HZ              = 11,
    DISPLAY_MODE_1080P50HZ              = 12,
    DISPLAY_MODE_1080I                  = 13,
    DISPLAY_MODE_1080P                  = 14,
    DISPLAY_MODE_4K2K24HZ               = 15,
    DISPLAY_MODE_4K2K25HZ               = 16,
    DISPLAY_MODE_4K2K30HZ               = 17,
    DISPLAY_MODE_4K2K50HZ               = 18,
    DISPLAY_MODE_4K2K60HZ               = 19,
    DISPLAY_MODE_4K2KSMPTE              = 20,
    DISPLAY_MODE_4K2KSMPTE30HZ          = 21,
    DISPLAY_MODE_4K2KSMPTE50HZ          = 22,
    DISPLAY_MODE_4K2KSMPTE60HZ          = 23,
    DISPLAY_MODE_768P                   = 24,
    DISPLAY_MODE_PANEL                  = 25,
    DISPLAY_MODE_PAL_M                  = 26,
    DISPLAY_MODE_PAL_N                  = 27,
    DISPLAY_MODE_NTSC_M                 = 28,
    DISPLAY_MODE_8K4K24HZ               = 29,
    DISPLAY_MODE_8K4K25HZ               = 30,
    DISPLAY_MODE_8K4K30HZ               = 31,
    DISPLAY_MODE_8K4K48HZ               = 32,
    DISPLAY_MODE_8K4K50HZ               = 33,
    DISPLAY_MODE_8K4K60HZ               = 34,
    DISPLAY_MODE_720P100HZ              = 35,
    DISPLAY_MODE_720P120HZ              = 36,
    DISPLAY_MODE_1080P100HZ             = 37,
    DISPLAY_MODE_1080P120HZ             = 38,
    DISPLAY_MODE_1440P50HZ              = 39,
    DISPLAY_MODE_1440P60HZ              = 40,
    DISPLAY_MODE_1440P100HZ             = 41,
    DISPLAY_MODE_1440P120HZ             = 42,
    DISPLAY_MODE_4K2K100HZ              = 43,
    DISPLAY_MODE_4K2K120HZ              = 44,
    DISPLAY_MODE_640x480p               = 45,
    DISPLAY_MODE_800x480p               = 46,
    DISPLAY_MODE_1024x600p              = 47,
    DISPLAY_MODE_TOTAL                  = 48
};

static const char* DISPLAY_MODE_LIST[] = {
    MODE_800x480p,
    MODE_1024x600p,
    MODE_640x480P,
    MODE_480I,
    MODE_480P,
    MODE_480CVBS,
    MODE_576I,
    MODE_576P,
    MODE_576CVBS,
    MODE_720P,
    MODE_720P50HZ,
    MODE_720P100HZ,
    MODE_720P120HZ,
    MODE_1080P24HZ,
    MODE_1080P25HZ,
    MODE_1080P30HZ,
    MODE_1080I50HZ,
    MODE_1080P50HZ,
    MODE_1080I,
    MODE_1080P,
    MODE_1080P100HZ,
    MODE_1080P120HZ,
    MODE_1440P50HZ,
    MODE_1440P60HZ,
    MODE_1440P100HZ,
    MODE_1440P120HZ,
    MODE_4K2K24HZ,
    MODE_4K2K25HZ,
    MODE_4K2K30HZ,
    MODE_4K2K50HZ,
    MODE_4K2K60HZ,
    MODE_4K2KSMPTE,
    MODE_4K2KSMPTE30HZ,
    MODE_4K2KSMPTE50HZ,
    MODE_4K2KSMPTE60HZ,
    MODE_4K2K100HZ,
    MODE_4K2K120HZ,
    MODE_8K4K24HZ,
    MODE_8K4K25HZ,
    MODE_8K4K30HZ,
    MODE_8K4K48HZ,
    MODE_8K4K50HZ,
    MODE_8K4K60HZ,
    MODE_768P,
    MODE_PANEL,
    MODE_PAL_M,
    MODE_PAL_N,
    MODE_NTSC_M,
};

typedef struct dv_input_info {
    char ubootenv_dv_type[MODE_LEN];  //the env of dolby vision type
    char dv_cap[MAX_STR_LEN];         //tv dolby vision cap
    char dv_displaymode[MODE_LEN];    //tv dolby vision max resolution
    char dv_deepcolor[DV_MODE_LEN];   //tv dolby vision type
}dv_input_info_t;

typedef struct hdmi_input_info {
    bool isSupport4K30Hz; // soc is support 4k30 or not, false:not support true:support
    bool isSupport4K;     // soc is support 4k or not, false:not support true:support
    bool isDeepColor;     // deepcolor feature enable or not, false:disable true:enable
    bool isframeratepriority;//frame priority feature enable or not, false:disable true:enable
    bool isLowPowerMode;//low power feature enable or not, false:disable true:enable
    int  sinkType; //sink type,0:not hdmi sink 1:hdmi sink,2:repeater sink
    char edidParsing[MODE_LEN];//edid parse ok or not, ok:parse ok,ng:parse ng
    char dc_cap[MAX_STR_LEN];  //device colorspace cap
    char disp_cap[MAX_STR_LEN]; //device resolution cap
    char ubootenv_cvbsmode[MODE_LEN]; //the env of cvbsmode
    char ubootenv_colorattribute[MODE_LEN];//the env of colospace for UI change colorspace or boot
}hdmi_input_info_t;

typedef struct dv_output_info {
    char dv_final_displaymode[MODE_LEN];
    char dv_final_deepcolor[MODE_LEN];
}dv_output_info_t;

typedef struct scene_input_info {
    scene_state state; //scene state
    bool isbestcolorspace; //hdmi best colorspace,false:disable true:enable
    bool isbestpolicy; //hdmi best policy,false:disable true:enable
    bool isDvEnable;   //dolby vision enable or not,false:disable true:enable
    bool isTvSupportHDR;//tv is support HDR or not, false:not support true:support
    bool isTvSupportDv;//tv is support dolby vision or not, false:not support true:support
    bool isHdrResolutionPriority;//Hdr Resolution Priority enable or not, false:disable true:enable
    hdr_priority_e hdr_priority; //dynamic range fromat preference,0:dolby vision,1:hdr,2:sdr
    hdr_policy_e hdr_policy;     //dynamic range policy,0 :follow sink, 1: match content
    hdr_force_mode_e hdr_force_mode;  //hdr force mode,1 :force sdr, 2: force dv, 3: force hdr10, 5:force hlg
    char cur_displaymode[MODE_LEN]; //hdmi current output mode
    dv_input_info_t dv_input_info;
    hdmi_input_info_t hdmi_input_info;
}scene_input_info_t;

typedef struct scene_output_info {
    char final_displaymode[MODE_LEN];  //final resolution
    char final_deepcolor[MODE_LEN];    //final colorspace
    int  dv_type;                      //final dolby vision type
}scene_output_info_t;

class SceneProcess
{
public:
    SceneProcess();
    ~SceneProcess();
    //update scene input info api
    void setSceneState(const scene_state state);
    void setBestPolicy(const bool isEnable);
    void setDvEnable(const bool isEnable);
    void setTvSupportHDR(const bool isEnable);
    void setTvSupportDV(const bool isEnable);
    void setHDRPriority(const hdr_priority_e value);
    void setHDRPolicy(const hdr_policy_e value);
    void setCurrentDisplayMode(const char* value);
    void setIsSupport4K(const bool isEnable);
    void setIsSupport4K30(const bool isEnable);
    void setIsDeepColor(const bool isEnable);
    void setIsLowPowerMode(const bool isEnable);
    void setFrameRatePriority(const bool isEnable);
    void setSinkType(const int value);
    void setdccap(const char* value);
    void setdispcap(const char* value);
    void setcvbsmode(const char* value);
    void setcolorattribute(const char* value);
    void setdvtype(const char* value);
    void setdvcap(const char* value);
    void setdvdisplaymode(const char* value);
    void setdvdeepcolor(const char* value);
    bool isHDRSupportMode(const char *mode);
    void UpdateSceneInputInfo(scene_input_info_t * input_info);
    int64_t resolveResolutionValue(const char *mode, int flag = FRAMERATE_PRIORITY);

    void Process(scene_output_info_t* output_info);

private:
    void DolbyVisionSceneProcess(scene_output_info_t* output_info);
    void HDRSceneProcess(scene_output_info_t* output_info);
    void SDRSceneProcess(scene_output_info_t* output_info);
    bool isSupport4KHDR(scene_output_info_t *output_info);
    bool isSupportnon4KHDR(scene_output_info_t *output_info);
    bool findHDRpreferMode(scene_output_info_t *output_info);
    int updateDolbyVisionType(void);
    void updateDolbyVisionAttr(int dolbyvision_type, char * dv_attr);
    void updateDolbyVisionDisplayMode(char * cur_outputmode, int dv_type, char * final_displaymode);
    bool isHDRPreference();
    bool isDolbyVisionPreference();
    bool isBestPolicy();
    bool isBestColorSpace();
    bool isFrameratePriority();
    bool isSupport4K();
    bool isSupport4K30Hz();
    bool isSupportDeepColor();
    bool isLowPowerMode();
    bool isDVSupportMode(char *mode);
    scene_state getSceneState();
    bool isModeSupportDeepColorAttr(const char *mode, const char * color);
    void getBestHdmiDeepColorAttr(const char *outputmode, char* colorAttribute);
    void getHdmiColorAttribute(const char* outputmode, char* colorAttribute, int state);
    void updateHdmiDeepColor(scene_state state, const char* outputmode, char* colorAttribute);
    void getHighestHdmiMode(char* mode);
    void filterHdmiMode(char* mode);
    bool isSupportHdmiMode(char* mode);
    void getHdmiOutputMode(char* mode);

    scene_input_info_t    mScene_Input_Info;
    scene_output_info_t   mScene_output_info;

    SysWrite*             mpSysWrite  = NULL;

    pthread_mutex_t       mSceneLock;
};
#endif
