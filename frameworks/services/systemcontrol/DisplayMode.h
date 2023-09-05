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

#ifndef ANDROID_DISPLAY_MODE_H
#define ANDROID_DISPLAY_MODE_H

#include "SysWrite.h"
#include "common.h"
#include "HDCP/HDCPTxAuth.h"
#include "HDCP/HDCPRxAuth.h"
#include "HDCP/HDCPRx22ImgKey.h"
#include "HDCP/HDCPRxKey.h"
#include "FrameRateAutoAdaption.h"
#include <FormatColorDepth.h>
#include "UEventObserver.h"
#include <map>
#include <cmath>
#include <string>
#include <pthread.h>
#include <semaphore.h>
#include <utils/Mutex.h>
#include <vector>

#include "SceneProcess.h"
#include "ubootenv/Ubootenv.h"
#ifndef RECOVERY_MODE
#include "SystemControlNotify.h"
using namespace android;
#endif
#ifdef FRAMERATE_MODE
#include "PQ/include/CPQControl.h"
#endif
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

//frame rate auto adapter feature
#define FRAME_RATE_AUTO_ADAPTER

#define TEST_UBOOT_MODE

#define DEVICE_STR_MID                  "MID"
#define DEVICE_STR_MBOX                 "MBOX"
#define DEVICE_STR_TV                   "TV"

#define DENSITY_720P                     "160"
#define DENSITY_1080P                    "240"
#define DENSITY_2160P                    "480"

#define DEFAULT_EDID_CRCHEAD            "checkvalue: "

#define DISPLAY_CFG_FILE                "/vendor/etc/mesondisplay.cfg"
#define FILTER_EDID_CFG_FILE            "/vendor/etc/filteredid.cfg"

//when close freescale, will enable display axis, cut framebuffer output
//when open freescale, will enable window axis, scale framebuffer output
#define PROP_DISPLAY_SIZE               "vendor.display-size"
#define PROP_DISPLAY_ALLM               "vendor.allm.support"
#define PROP_DISPLAY_GAME               "vendor.contenttype_game.support"

/******************************************************/
#define DISPLAY_HDMI_HDCP14_STOP        "stop14" //stop HDCP1.4 authenticate
#define DISPLAY_HDMI_HDCP22_STOP        "stop22" //stop HDCP2.2 authenticate
#define DISPLAY_HDMI_HDCP_14            "1"
#define DISPLAY_HDMI_HDCP_22            "2"

#define HDMI_TX_PLUG_UEVENT             "DEVPATH=/devices/virtual/amhdmitx/amhdmitx0/hdmi"//hdmi hot plug event
#define HDMI_TX_POWER_UEVENT            "DEVPATH=/devices/virtual/amhdmitx/amhdmitx0/hdmi_power"
#define HDMI_TX_HDR_UEVENT              "DEVPATH=/devices/virtual/amhdmitx/amhdmitx0/hdmi_hdr"
#define HDMI_TX_HDCP_UEVENT             "DEVPATH=/devices/virtual/amhdmitx/amhdmitx0/hdcp"
#define HDMI_TX_HDCP14_LOG_UEVENT       "DEVPATH=/devices/virtual/amhdmitx/amhdmitx0/hdcp_log"
#define HDMI_TX_HDMI_AUDIO_UEVENT       "DEVPATH=/devices/virtual/amhdmitx/amhdmitx0/hdmi_audio"

/*
 * save user prefer fps set by hwc
 * 0:24hz/30hz/60hz
 * 1:23.97hz/29.97/59.94hz
 * 2:default value,hwc not change propert value
 */
#define HDMI_FRC_POLICY_PROP            "vendor.sys.frc_policy"

#define DOLBY_VISION_KO_DIR0                 "/odm/lib/modules/dovi.ko"
#define DOLBY_VISION_KO_DIR0_TV              "/odm/lib/modules/dovi_tv.ko"

#define DOLBY_VISION_KO_DIR1                 "/oem/overlay/dovi.ko"
#define DOLBY_VISION_KO_DIR1_TV              "/oem/overlay/dovi_tv.ko"

#define DOLBY_VISION_SET_ENABLE_LL_RGB      3
#define DOLBY_VISION_SET_ENABLE_LL_YUV      2
#define DOLBY_VISION_SET_ENABLE             1
#define DOLBY_VISION_SET_DISABLE            0

#define DV_ENABLE                       "Y"
#define DV_DISABLE                      "N"

#define DV_POLICY_FOLLOW_SINK           "0"
#define DV_POLICY_FOLLOW_SOURCE         "1"
#define DV_POLICY_FORCE_MODE            "2"
#define DV_HDR10_POLICY                 "3"

#define DV_MODE_BYPASS                  "0x0"
#define DV_MODE_IPT_TUNNEL              "0x2"
#define DV_MODE_FORCE_HDR10             "0x3"
#define DV_MODE_FORCE_SDR10             "0x4"
#define DV_MODE_FORCE_SDR8              "0x5"

#define BYPASS_PROCESS                  "0"
#define SDR_PROCESS                     "1"
#define HDR_PROCESS                     "2"
#define DV_PROCESS                      "3"

#define HDMI_UEVENT_HDMI                "hdmi"
#define HDMI_UEVENT_HDMI_POWER          "hdmi_power"
#define HDMI_UEVENT_HDMI_HDR            "hdmi_hdr"
#define HDMI_UEVENT_HDCP                "hdcp"
#define HDMI_UEVENT_HDCP_LOG            "hdcp_log"
#define HDMI_UEVENT_HDMI_AUDIO          "hdmi_audio"

#define HDMI_TX_PLUG_OUT                "0"
#define HDMI_TX_PLUG_IN                 "1"
#define HDMI_TX_SUSPEND                 "0"
#define HDMI_TX_RESUME                  "1"
#define HDMI_TX_AUTH_FAIL               "0"
#define HDMI_TX_AUTH_SUCCESS            "1"

//HDCP RX
#define HDMI_RX_PLUG_UEVENT             "DEVPATH=/devices/platform/ffd26000.hdmirx/hdmirx/hdmirx0/rx22"               //"DEVPATH=/devices/virtual/switch/hdmirx_hpd"//1:plugin 0:plug out
#define HDMI_RX_AUTH_UEVENT             "DEVPATH=/devices/platform/ffd26000.hdmirx/hdmirx/hdmirx0/rp_auth"        //"DEVPATH=/devices/virtual/switch/hdmirx_hdcp_auth"//0:FAIL 1:HDCP14 2:HDCP22
#define HDMI_RX_PLUG_PATH               "sys/class/extcon/rx22"
#define HDMI_RX_AUTH_PATH               "sys/class/extcon/rp_auth"
#define HDMI_RX_UEVENT                  "change@/devices/platform/0.hdmirx"

#define HDMI_RX_PLUG_OUT                "0"
#define HDMI_RX_PLUG_IN                 "1"
#define HDMI_RX_AUTH_FAIL               "0"
#define HDMI_RX_AUTH_HDCP14             "1"
#define HDMI_RX_AUTH_HDCP22             "2"

#define HDMI_RX_HPD_STATE               "/sys/module/tvin_hdmirx/parameters/hpd_to_esm"
#define HDMI_RX_KEY_COMBINE             "/sys/module/tvin_hdmirx/parameters/hdcp22_firmware_ok_flag"

#define H265_DOUBLE_WRITE_MODE          "/sys/module/amvdec_h265/parameters/double_write_mode"

#define VIDEO_LAYER_ENABLE              "0"
#define VIDEO_LAYER_DISABLE             "1"
#define VIDEO_LAYER_AUTO_ENABLE         "2"//2:enable video layer when first frame data come

#define PROP_TVSOC_AS_MBOX              "vendor.tv.soc.as.mbox"
#define PROP_VMX                        "persist.vendor.sys.vmx"

#define PROP_HDMIONLY                   "ro.vendor.platform.hdmionly"
#define PROP_SUPPORT_4K                 "ro.vendor.platform.support.4k"
#define PROP_SUPPORT_OVER_4K30          "ro.vendor.platform.support.over.4k30"
#define PROP_LCD_DENSITY                "ro.sf.lcd_density"
#define PROP_WINDOW_WIDTH               "const.window.w"
#define PROP_WINDOW_HEIGHT              "const.window.h"
#define PROP_HAS_CVBS_MODE              "ro.vendor.platform.has.cvbsmode"
#define PROP_BEST_OUTPUT_MODE           "ro.vendor.platform.best_outputmode"
#define PROP_BOOTVIDEO_SERVICE          "service.bootvideo"
#define PROP_DEEPCOLOR                  "vendor.sys.open.deepcolor" //default close this function, when reboot
#define PROP_DOLBY_VISION_FEATURE       "ro.vendor.platform.support.dolbyvision"
#define PROP_SUPPORT_DOLBY_VISION       "vendor.system.support.dolbyvision"
#define PROP_DOLBY_VISION_CERTIFICATION "persist.vendor.sys.dolbyvision.certification"
#define PROP_DOLBY_VISION_PRIORITY      "persist.vendor.sys.graphics.priority"
#define PROP_ALWAYS_DOLBY_VISION        "vendor.system.always.dolbyvision"
#define PROP_HDR_MODE_STATE             "persist.vendor.sys.hdr.state"
#define PROP_SDR_MODE_STATE             "persist.vendor.sys.sdr.state"
#define PROP_DISPLAY_SIZE_CHECK         "vendor.display-size.check"
#define PROP_ENABLE_SDR2HDR             "ro.vendor.sdr2hdr.enable"
#define PROP_HDMI_FRAMERATE_PRIORITY    "persist.vendor.sys.framerate.priority"
#define PROP_HDR_RESOLUTION_PRIORITY    "persist.vendor.hdr.resolution.priority"

#define HDR_MODE_OFF                    "0"
#define HDR_MODE_ON                     "1"
#define HDR_MODE_AUTO                   "2"

#define SDR_MODE_OFF                    "0"
#define SDR_MODE_AUTO                   "2"

#define SUFFIX_10BIT                    "10bit"
#define SUFFIX_12BIT                    "12bit"
#define SUFFIX_14BIT                    "14bit"
#define SUFFIX_RGB                      "rgb"

#define DEFAULT_DEEP_COLOR_ATTR         "444,8bit"
#define DEFAULT_420_DEEP_COLOR_ATTR     "420,8bit"


#define UBOOTENV_DIGITAUDIO             "ubootenv.var.digitaudiooutput"
/*
 * save user set color format
 */
#define UBOOTENV_USER_COLORATTRIBUTE   "ubootenv.var.user_colorattribute"

/*
 * save user set hdmi output resolution or tv prefer resolution
 */
#define UBOOTENV_HDMIMODE               "ubootenv.var.hdmimode"

/*
 * test for tv product
 */
#define UBOOTENV_TESTMODE               "ubootenv.var.testmode"

/*
 * save user prefer cvbs output resolution
 */
#define UBOOTENV_CVBSMODE               "ubootenv.var.cvbsmode"
/*
 * save device output resolution(hdmi/cvbs/panel) at uboot
 * and uboot transmit this value to kernel.
 */
#define UBOOTENV_OUTPUTMODE             "ubootenv.var.outputmode"
/*
 * best resolution policy
 * false:use user set resolution as hdmi output
 * true :choose the high and max fps resolution as hdmi output,
 *       resolution priority show as the table of MODE_FRAMERATE_FIRST[] in sceneprocess.cpp,
 *       ex:2160p60hz->216050hz->1080p60hz->1080p50hz->2160p30hz->2160p25hz->2160p24hz...
 */
#define UBOOTENV_ISBESTMODE             "ubootenv.var.is.bestmode"

/*
 * dv best policy
 * false:use user set dv mode as hdmi output
 * true :choose prefer dv mode as hdmi output,
 *       priority as std dv(444,8bit)-->low latency dv(422,12bit)
 */
#define UBOOTENV_BESTDOLBYVISION        "ubootenv.var.bestdolbyvision"

/*
 * save edid checksum
 */
#define UBOOTENV_EDIDCRCVALUE           "ubootenv.var.hdmichecksum"
/*
 * save user set hdmi output color space,ex444/422/420/rgb
 */
#define UBOOTENV_HDMICOLORSPACE         "ubootenv.var.hdmi_colorspace"
/*
 * save user set hdmi output color depth,ex:8bit/10bit/12bit
 */
#define UBOOTENV_HDMICOLORDEPTH         "ubootenv.var.hdmi_colordepth"
/*
 * dolby_status/dv_type/dv_enable maybe be merged to one variable
 * but for the backward compatible so keeping them.
 */
/*
 * save user set dv mode
 * uboot output dv signal or not base this value
 * 0:dv disable or match content hdr mode
 * 1:sink-led
 * 2:source-led
 */
#define UBOOTENV_DOLBYSTATUS            "ubootenv.var.dolby_status"
/*
 * save user set dv mode
 * systemcontrol output dv signal base this value
 * 0:disable dv
 * 1:sink-led
 * 2:source-led
 */
#define UBOOTENV_USER_DV_TYPE                "ubootenv.var.user_prefer_dv_type"
/*
 * save user prefer dv enable or disable
 * 0:disable
 * 1:enable
 */
#define UBOOTENV_DV_ENABLE              "ubootenv.var.dv_enable"
/*
 * save user prefer hdr policy
 * 0:always hdr(output signal base TV)
 * 1:adaptive hdr(output signal base tv and play content)
 */
#define UBOOTENV_HDR_POLICY             "ubootenv.var.hdr_policy"
/*
 * save user prefer fps
 * 0:24hz/30hz/60hz
 * 1:23.97hz/29.97/59.94hz
 */
#define UBOOTENV_FRAC_RATE_POLICY       "ubootenv.var.frac_rate_policy"
/*
 * save user prefer hdr priority
 * 0:keep tv dv/hdr/sdr capability
 * 1:disable tv dv capability
 * 2:disable tv dv and hdr capability
 */
#define UBOOTENV_HDR_PRIORITY           "ubootenv.var.hdr_priority"
/*
 * save user force hdr mode
 * 0:invalid hdr mode
 * 1:force sdr
 * 2:force dv
 * 3:force hdr10
 * 4:force hdr10+,need to do
 * 5:force hlg
 */
#define UBOOTENV_HDR_FORCE_MODE         "ubootenv.var.hdr_force_mode"

/*
 *save user prefer sdr to hdr enable or disable
 *sdr content force be converted to hdr content
 */
#define UBOOTENV_SDR2HDR                "ubootenv.var.sdr2hdr"
#define PROP_DEEPCOLOR_CTL              "persist.sys.open.deepcolor" // 8, 10, 12
#define PROP_PIXFMT                     "persist.sys.open.pixfmt" // rgb, ycbcr

//memc
#define PROP_DISPLAY_MEMC               "persist.vendor.sys.memc"
#define DISPLAY_MEMC_SYSFS              "/dev/frc"
#define MEMDEV_CONTRL                     _IOW('F', 0x06, unsigned int)

#define FULL_WIDTH_1024x600             1024
#define FULL_HEIGHT_1024x600            600
#define FULL_WIDTH_800x480              800
#define FULL_HEIGHT_800x480             480
#define FULL_WIDTH_640x480              640
#define FULL_HEIGHT_640x480             480
#define FULL_WIDTH_480                  720
#define FULL_HEIGHT_480                 480
#define FULL_WIDTH_576                  720
#define FULL_HEIGHT_576                 576
#define FULL_WIDTH_720                  1280
#define FULL_HEIGHT_720                 720
#define FULL_WIDTH_768                  1366
#define FULL_HEIGHT_768                 768
#define FULL_WIDTH_1080                 1920
#define FULL_HEIGHT_1080                1080
#define FULL_WIDTH_1440                 2560
#define FULL_HEIGHT_1440                1440
#define FULL_WIDTH_4K2K                 3840
#define FULL_HEIGHT_4K2K                2160
#define FULL_WIDTH_4K2KSMPTE            4096
#define FULL_HEIGHT_4K2KSMPTE           2160
#define FULL_WIDTH_8K4K                 7680
#define FULL_HEIGHT_8K4K                4320
#define FULL_WIDTH_PANEL                1024
#define FULL_HEIGHT_PANEL               600

/* In HDMI 2.1 CTS, the blocks of EDID data will increase to 8 blocks,
 * and the binary data size will be 128 x 8 = 1024bytes
 * So cat /sys/class/amhdmitx/amhdmitx0/rawedid will be double
 * to 2048, plus the '\0' char.
 */
#define EDID_MAX_SIZE                   2049

enum {
    EVENT_OUTPUT_MODE_CHANGE            = 0,
    EVENT_DIGITAL_MODE_CHANGE           = 1,
    EVENT_HDMI_PLUG_OUT                 = 2,
    EVENT_HDMI_PLUG_IN                  = 3,
    EVENT_HDMI_AUDIO_OUT                = 4,
    EVENT_HDMI_AUDIO_IN                 = 5,
    EVENT_HDMI_TX_AUTH_FAIL             = 6,
    EVENT_HDMI_TX_AUTH_SUCCESS          = 7,
};

enum {
    DISPLAY_TYPE_NONE                   = 0,
    DISPLAY_TYPE_TABLET                 = 1,
    DISPLAY_TYPE_MBOX                   = 2,
    DISPLAY_TYPE_TV                     = 3,
    DISPLAY_TYPE_REPEATER               = 4
};

#define MODE_480I_PREFIX                "480i"
#define MODE_480P_PREFIX                "480p"
#define MODE_576I_PREFIX                "576i"
#define MODE_576P_PREFIX                "576p"
#define MODE_720P_PREFIX                "720p"
#define MODE_768P_PREFIX                "768p"
#define MODE_1080I_PREFIX               "1080i"
#define MODE_1080P_PREFIX               "1080p"
#define MODE_1440P_PREFIX               "1440p"
#define MODE_4K2K_PREFIX                "2160p"
#define MODE_4K2KSMPTE_PREFIX           "smpte"
#define MODE_8K4K_PREFIX                "4320p"
#define MODE_4K1K_PREFIX                "3840x1080"

#define DV_HDR_SINK_SOURCE_BYPASS       "0"
#define DV_HDR_SINK_PROCESS             "1"
#define DV_HDR_SOURCE_PROCESS           "2"
#define DV_HDR_SINK_SOURCE_PROCESS      "3"
#define DV_ENABLE_FORCE_SDR_10BIT       "4"
#define DV_ENABLE_FORCE_SDR_8BIT        "5"

#define HDR_POLICY_SINK                 "0"
#define HDR_POLICY_SOURCE               "1"
#define HDR_POLICY_FORCE                "2"

#define FORCE_SDR                       "1"
#define FORCE_DV                        "2"
#define FORCE_HDR10                     "3"
#define FORCE_HLG                       "5"


/* foce mode type in uboot env
 * 0: invalid type
 * 1: force sdr
 * 2: force dv
 * 3: force hdr10
 * 4: force hdr10plus (need to do)
 * 5: force hlg
 * */
static const char* FORCE_MODE_TYPE[] = {
    "0",
    "1",
    "2",
    "3",
    "4",
    "5"
};

typedef enum {
    OUTPUT_MODE_STATE_INIT               = 0,
    OUTPUT_MODE_STATE_POWER              = 1,//hot plug
    OUTPUT_MODE_STATE_SWITCH             = 2,//user switch the mode
    OUTPUT_MODE_STATE_SWITCH_ADAPTER     = 3,//video auto switch the mode
    OUTPUT_MODE_STATE_RESERVE            = 4,
    OUTPUT_MODE_STATE_ADAPTER_END        = 5 //end hint video auto switch the mode
}output_mode_state;

typedef enum {
    OUTPUT_CHANGE_BY_INIT               = 0,
    OUTPUT_CHANGE_BY_USER               = 1,
    OUTPUT_CHANGE_BY_PLUG               = 2,
    OUTPUT_CHANGE_BY_HWC                = 3
}output_change_reason;
typedef enum {
    HDMI_SINK_TYPE_NONE                 = 0,
    HDMI_SINK_TYPE_SINK                 = 1,
    HDMI_SINK_TYPE_REPEATER             = 2,
    HDMI_SINK_TYPE_RESERVE              = 3
}hdmi_sink_type;

typedef struct hdmi_dv_info {
    char ubootenv_dv_type[MODE_LEN];
    char dv_cap[MAX_STR_LEN];
    char dv_displaymode[MODE_LEN];
    char dv_deepcolor[DV_MODE_LEN];
    int  dv_type;
    char dv_enable[MODE_LEN];
    char dv_cur_displaymode[MODE_LEN];
    char dv_final_displaymode[MODE_LEN];
    char dv_final_deepcolor[MODE_LEN];
}hdmi_dv_info_t;

typedef struct hdmi_data {
    output_mode_state state;
    bool isbestcolorspace;       //hdmi best colorspace,false:disable true:enable
    bool isbestpolicy;           //hdmi resolution best policy,false:disable true:enable
    hdr_priority_e hdr_priority; //dynamic range fromat preference,0:dolby vision,1:hdr,2:sdr
    hdr_policy_e   hdr_policy;   //dynamic range policy,0 :follow sink, 1: match content
    hdr_force_mode_e hdr_force_mode;  /* hdr force mode,1 :force sdr, 2: force dv, 3: force hdr10, 5:force hlg*/
    char edidParsing[MODE_LEN];
    char dc_cap[MAX_STR_LEN];  //device colorspace cap
    char disp_cap[MAX_STR_LEN];
    int  sinkType;
    char ui_hdmimode[MODE_LEN];
    char ubootenv_cvbsmode[MODE_LEN];
    char ubootenv_hdmimode[MODE_LEN];
    char hdmi_current_mode[MODE_LEN];
    char ubootenv_colorattribute[MODE_LEN];
    char hdmi_current_attr[MODE_LEN];
    bool iscvbsMode;
    char hdmi_final_displaymode[MODE_LEN];
    char hdmi_final_deepcolor[MODE_LEN];
    char final_displaymode[MODE_LEN];
    char final_deepcolor[MODE_LEN];
    hdmi_dv_info dv_info;
    output_change_reason reason;
}hdmi_data_t;

typedef struct hdmi_output_info {
    output_mode_state reason;          //change driver setting reason
    char final_displaymode[MODE_LEN];  //hdmi final resolution
    char final_deepcolor[MODE_LEN];    //hdmi final colorspace
    int  dv_type;                      //hdmi final dolby vision type
}hdmi_output_info_t;

typedef struct axis_s {
    int x;
    int y;
    int w;
    int h;
} axis_t;

typedef struct resolution {
//       resolution       standard frequency deepcolor
//          2160             p       50hz      420   //2160p50hz420
//          1080             p       60hz        0   //1080p60hz
//0x00 0000 0000 0000 0000 | 0 | 0000 0000 | 0 0000 0000 //resolution_num
    int resolution;
    char standard;
    int frequency;
    int deepcolor;
    int64_t resolution_num;
} resolution_t;

// ----------------------------------------------------------------------------
namespace meson {
    class DisplayAdapter;
}

class DisplayMode : public UEventObserver::HDMITxUevntCallback,
                                      private FrameRateAutoAdaption::Callback,
                                      public HDCPTxAuth::HDCPTxAuthCallback
{
public:
    DisplayMode(const char *path);
    DisplayMode(const char *path, Ubootenv *ubootenv);
    ~DisplayMode();

    void init();
    void reInit();

    void setRecoveryMode(bool isRecovery);
    void setTvModelName();
    void setLogLevel(int level);
    int dump(char *result);
    void setTvRecoveryDisplay();
    void setSourceOutputMode(const char* outputmode);
    void setSinkOutputMode(const char* outputmode);
    void setSinkOutputMode(const char* outputmode, bool initState);
    void setDigitalMode(const char* mode);
    void setPosition(const char* curMode, int left, int top, int width, int height);
    void getPosition(const char* curMode, int *position);
    bool getDisplayMode(char* mode);
    void setDisplayMode(std::string mode);
    void setFrameRate(float frameRate);
    void setPerferredMode(const char* mode);
    bool isExitDovi();
    bool isLoadDovi();
    void setDolbyVisionSupport();
    void setDolbyVisionEnable(int state, output_mode_state mode_state);
    void setTvDolbyVisionEnable(void);
    void setTvDolbyVisionDisable(void);
    void enableDolbyVision(int DvMode);
    void disableDolbyVision(int DvMode);
    int  getDolbyVisionType();
    bool isDolbyVisionEnable();
    bool isTvDolbyVisionEnable();
    bool isMboxSupportDolbyVision();
    bool isTvSupportDolbyVision(char *mode);
    void setGraphicsPriority(const char* mode);
    void getGraphicsPriority(char* mode);
    bool isTvSupportHDR();
    bool setColorSpace(const char* colorspace);
    void getDeepColorAttr(const char* mode, char *value);
    void saveDeepColorAttr(const char* mode, const char* dcValue);
    int64_t resolveResolutionValue(const char *mode);
    void setHdrMode(const char* mode);
    void setSdrMode(const char* mode);
    void isHDCPTxAuthSuccess( int *status);
    static void* bootanimDetect(void *data);
    void setSourceDisplay(output_mode_state state);
    void clearUserDisplayConfig();
    void clearBootDisplayConfig(const char*value);
    void setBootDisplayConfig(const char*value);
    bool getPreferredDisplayConfig(char* mode);
    bool isHdmiEdidParseOK(void);
    bool isHdmiHpd(void);
    bool isHdmiUsed(void);
    bool isVMXCertification(void);
    int  getHdmiSinkType(void);
    void getHdmiEdidStatus(char* edidstatus);
    void getHdmiDispCap(char* disp_cap);
    void getHdmiDcCap(char* dc_cap);
    void getHdmiDvCap(hdmi_data_t* data);
    void getCommonData(hdmi_data_t* data);
    void getHdmiData(hdmi_data_t* data);
    void getSupportDispModeList(char * modelist);
    void setActiveDispMode(const char*value);
    void notifyPlugin();
    int readHdcpRX22Key(char *value, int size);
    bool writeHdcpRX22Key(const char *value, const int size);
    int readHdcpRX14Key(char *value, int size);
    bool writeHdcpRX14Key(const char *value, const int size);
    bool writeHdcpRXImg(const char *path);
    bool updataLogoBmp(const char *path);
    void setALLMMode(int state);
    bool isTvSupportALLM();
    bool frameRateDisplay(bool on);

    void sendHDMIContentType(int state);
    bool getGameContentTypeSupport();

    bool getSupportALLMContentTypeList(std::vector<std::string> *supportModes);

    HDCPTxAuth *geTxAuth();
#ifndef RECOVERY_MODE
    void notifyEvent(int event);
    void setListener(const sp<SystemControlNotify>& listener);
#endif
#ifdef FRAMERATE_MODE
    void setPQHandle(CPQControl* handle);
#endif

    virtual void onHdcpTxAuthEvent (const char* status);
    virtual void onTxEvent (char* switchName, char* hpdstate, int outputState);
    virtual void onDispModeSyncEvent (const char* outputmode, int state);
    virtual void setDisplayModeinner(const char* outputmode);
    void hdcpSwitch();

    void getBootanimStatus(int *status);
    bool getModeSupportDeepColorAttr(const char* outputmode,const char * color);
    bool getPrefHdmiDispMode(char* mode);
    void getHdrStrategy(char* value);
    void setHdrStrategy(const char* type);
    int getCurrentHdrPriority(void);
    int getHdrPriority(void);
    void setHdrPriority(const char* type);
    void gethdrforcemode(char* value);
    bool memcContrl(bool on);
private:

    bool getBootEnv(const char* key, char* value);
    void setBootEnv(const char* key, const char* value);

    void getHdmiData_cached(hdmi_data_t* data);
    int parseConfigFile();
    int parseFilterEdidConfigFile();
    void getHighestPriorityMode(char* mode, hdmi_data_t* data);
    bool isMatchMode(char* curmode, const char* outputmode);
    void filterHdmiDispcap(hdmi_data_t* data);
    void applyDisplaySetting(hdmi_output_info_t* output_info);
    void sceneProcess(hdmi_data_t* data);
    void setAutoSwitchFrameRate(int state);
    void updateDefaultUI();
    void startBootanimDetectThread();
    void updateFreeScaleAxis();
    void updateWindowAxis(const char* outputmode);
    void initGraphicsPriority();
    void initHdrSdrMode();
    bool isEdidChange();
    bool isHWCProcess();
    bool isSupport4K30Hz();
    bool isSupport4K();
    bool isSupportDeepColor();
    bool isFrameratePriority();
    bool isHdrResolutionPriority();
    bool isLowPowerMode();
    bool isBestOutputmode();
    bool isBestColorSpace();
    bool modeSupport(char *mode, int sinkType);
    void setDvHdrPolicy(const char* policy);
    void setDefaultMode();
    int64_t resolveResolutionValue(const char *mode, int flag);
    void startHdmiPlugDetectThread();
    void startBootvideoDetectThread();
    static void* HdmiUenventThreadLoop(void* data);
    void setSinkDisplay(bool initState);
    int getBootenvInt(const char* key, int defaultVal);
    void dumpCap(const ConstCharforSysNodeIndex index, const char * hint, char *result);
    void dumpCaps(char *result=NULL);
    void saveHdmiParamToEnv();
    bool checkDolbyVisionStatusChanged(int state);
    /*
     * for parse dv mode type
     */
    const char *dvModeTypeToString(const char *dvmode);
    void resetMemc();

    bool getContentTypeSupport(const char* type);

    const char* pConfigPath;
    int mDisplayType;
    bool mIsRecovery = false;

    pthread_mutex_t mEnvLock;

    int mDisplayWidth;
    int mDisplayHeight;

    char mSocType[64];
    char mDefaultUI[64];//this used for mbox
    int mLogLevel;

    SceneProcess *mpSceneProcess = NULL;
    SysWrite *pSysWrite = NULL;
    Ubootenv *mUbootenv = NULL;
    FrameRateAutoAdaption *pFrameRateAutoAdaption = NULL;

    HDCPTxAuth *pTxAuth = NULL;
    HDCPRxAuth *pRxAuth = NULL;
    UEventObserver *pUEventObserver = NULL;
    FormatColorDepth *pmDeepColor = NULL;
    // bootAnimation flag
    bool setDolbyVisionState = true;
    char mEdid[EDID_MAX_SIZE] = {0};
    hdmi_data_t mHdmidata;
    scene_output_info_t mScene_output_info;
#ifndef RECOVERY_MODE
    mutable Mutex mLock;
    sp<SystemControlNotify> mNotifyListener;
#endif
};

#endif // ANDROID_DISPLAY_MODE_H
