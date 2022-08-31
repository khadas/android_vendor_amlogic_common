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

#ifndef SYS_FS_H
#define SYS_FS_H

/*
#define SYS_VIDEO_HDR_FMT         "/sys/class/video_poll/primary_src_fmt"
#define SYSFS_VIDEO_EVENT_PATH    "/sys/class/video_poll/status_changed"
#define SYS_VIDEO_FRAME_HEIGHT    "/sys/class/video/frame_height"
#define CROP_PATH                 "/sys/class/video/crop"
#define SCREEN_MODE_PATH          "/sys/class/video/screen_mode"
#define NOLINER_FACTORY           "/sys/class/video/nonlinear_factor"
#define VIDEO_RGB_SCREEN          "/sys/class/video/rgb_screen"
#define TEST_SCREEN               "/sys/class/video/test_screen"

#define LDIM_CONTROL_PATH         "/sys/class/aml_ldim/func_en"
#define BACKLIGHT_PATH            "/sys/class/backlight/aml-bl/brightness"
#define SSC_PATH                  "/sys/class/lcd/ss"
#define SYSFS_VFM_MAP_PATH        "/sys/class/vfm/map"
#define AIPQ_RUNNING              "/sys/class/vdetect/aipq_enable"

#define PQ_SET_RW_INTERFACE       "/sys/class/amvecm/pq_reg_rw"
#define PQ_DNLIP_DEBUG            "/sys/class/amvecm/dnlp_debug"
#define PQ_USER_SET               "/sys/class/amvecm/pq_user_set"
#define PQ_CM2_SAT                "/sys/class/amvecm/cm2_sat"
#define PQ_CM2_HUE_BY_HS          "/sys/class/amvecm/cm2_hue_by_hs"
#define PQ_CM2_LUMA               "/sys/class/amvecm/cm2_luma"

#define TVAFE_TVAFE0_REG          "/sys/class/tvafe/tvafe0/reg"

#define SYS_DISPLAY_MODE_PATH     "/sys/class/display/mode"

#define DEMOSQUITO_MODULE_CONTROL_PATH   "/sys/module/di/parameters/dnr_dm_en"  //demosquito control
#define DEBLOCK_MODULE_CONTROL_PATH      "/sys/module/di/parameters/dnr_en"     //deblock control
#define NR2_MODULE_CONTROL_PATH          "/sys/module/di/parameters/nr2_en"      //noisereduction control
#define MCDI_MODULE_CONTROL_PATH         "/sys/module/di/parameters/mcen_mode"  //mcdi control
#define AIPQ_DEBUG_VDETECT               "/sys/module/decoder_common/parameters/debug_vdetect"
*/
#define HDMI_OUTPUT_CHECK_PATH    "/sys/class/amhdmitx"    //if this dir exist,is hdmi output

typedef enum {
    VIDEO_POLL_STATUS_CHANGE = 0,
    VIDEO_POLL_PRIMARY_SRC_FMT,
    VIDEO_CROP,
    VIDEO_SCREEN_MODE,
    VIDEO_NONLINEAR_FACTOR,
    VIDEO_RGB_SCREEN,
    VIDEO_TEST_SCREEN,
    VIDEO_FRAME_HEIGHT,
    AMVECM_PQ_REG_RW,
    AMVECM_PQ_DNLP_DEBUG,
    AMVECM_PQ_USER_SET,
    AMVECM_PQ_CM2_SAT,
    AMVECM_PQ_CM2_HUE_BY_HS,
    AMVECM_PQ_CM2_LUMA,
    AML_LDIM_FUNC_EN,
    BACKLIGHT_AML_BL_BRIGHTNESS,
    VFM_MAP,
    TVAFE_TVAFE0_REG,
    LCD_SS,
    DISPLAY_MODE,
    AMHDMITX,
    VDETECT_AIPQ_ENABLE,
    DI_PARAMETERS_DNR_DM_EN,
    DI_PARAMETERS_DNR_EN,
    DI_PARAMETERS_NR2_EN,
    DI_PARAMETERS_MCEN_MODE,
    DECODER_COMMON_PARAMETERS_DEBUG_VDETECT,
    VIDEO_BACKGROUND_COLOR,
    VIDEO_BLACKOUT_POLICY,
    VIDEO_DISABLE_VIDEO,
    VDIN_SNOW_FLAG,
    SysFsNodeIndexMax,
} ConstCharforSysFsNodeIndex;

class SysFs
{
public:
    SysFs();
    ~SysFs();
    static SysFs *GetInstance();
    const char *getSysNode(ConstCharforSysFsNodeIndex index);
    int readSysfs(ConstCharforSysFsNodeIndex index, char *value, int count);
    int readSysfsOriginal(ConstCharforSysFsNodeIndex index, char *value, int count);
    int writeSysfs(ConstCharforSysFsNodeIndex index, const char *value);
    void setLogLevel(int level);

private:
    static SysFs *mInstance;
    void initConstCharforSysNode();
    int writeSys(const char *path, const char *val);
    int readSys(const char *path, char *buf, int count, bool needOriginalData);
    int getKernelReleaseVersion();

    int mLogLevel;
    const char* mPathforSysNode[SysFsNodeIndexMax];
};

#endif // SysFs_H
