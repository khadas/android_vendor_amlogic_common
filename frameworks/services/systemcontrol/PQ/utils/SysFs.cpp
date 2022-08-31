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

#define LOG_TAG "SystemControl"
#define LOG_TV_TAG "SysFs"
//#define LOG_NDEBUG 0
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <cutils/properties.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include "CPQLog.h"
#include "SysFs.h"

SysFs *SysFs::mInstance = NULL;
SysFs *SysFs::GetInstance()
{
    if (NULL == mInstance) {
        mInstance = new SysFs();
    }

    return mInstance;
}

SysFs::SysFs()
{
    initConstCharforSysNode();
}

SysFs::~SysFs()
{

}

int SysFs::readSys(const char *path, char *buf, int count, bool needOriginalData)
{
    int fd;
    int len = 0;

    if ( NULL == buf ) {
        SYS_LOGE("buf is NULL");
        return -1;
    }

    if ((fd = open(path, O_RDONLY)) < 0) {
        SYS_LOGE("readSys, open %s fail. Error info [%s]", path, strerror(errno));
        len = -1;
        goto exit;
    }

    len = read(fd, buf, count);
    if (len < 0) {
        SYS_LOGE("readSys, read error: %s, %s\n", path, strerror(errno));
        goto exit;
    }

    if (!needOriginalData) {
        int i , j;
        for (i = 0, j = 0; i <= len -1; i++) {
            /*change '\0' to 0x20(spacing), otherwise the string buffer will be cut off
             * if the last char is '\0' should not replace it
             */
            if (0x0 == buf[i] && i < len - 1) {
                buf[i] = 0x20;

                //SYS_LOGI("read buffer index:%d is a 0x0, replace to spacing \n", i);
            }

            /* delete all the character of '\n' */
            if (0x0a != buf[i]) {
                buf[j++] = buf[i];
            }
        }

        buf[j] = 0x0;
    }

    //SYS_LOGI("%s, path:%s, len:%d, buf:%s\n", __FUNCTION__, path, len, buf);

exit:
    close(fd);
    return len;
}

int SysFs::readSysfs(ConstCharforSysFsNodeIndex index, char *buf, int count)
{
    int len = -1;

    //SYS_LOGD("%s, index %d path:%s count:%d\n", __FUNCTION__, index, mPathforSysNode[index], count);

    len = readSys(mPathforSysNode[index], (char*)buf, count, false);

    return len;
}

// get the original data from sysfs without any change.
int SysFs::readSysfsOriginal(ConstCharforSysFsNodeIndex index, char *buf, int count)
{
    int len = -1;

    //SYS_LOGD("%s, index:%d path:%s count:%d\n", __FUNCTION__, index, mPathforSysNode[index], count);

    len = readSys(mPathforSysNode[index], (char*)buf, count, true);

    return len;
}

int SysFs::writeSys(const char *path, const char *val)
{
    int fd;
    int len = -1;

    if ((fd = open(path, O_RDWR)) < 0) {
        SYS_LOGE("writeSysFs, open %s fail.", path);
        len = -1;
        goto exit;
    }

    //SYS_LOGI("%s,path:%s, val:%s\n", __FUNCTION__, path, val);

    len = write(fd, val, strlen(val));

exit:
    close(fd);
    return len;
}

int SysFs::writeSysfs(ConstCharforSysFsNodeIndex index, const char *value)
{
    int len = -1;

    //SYS_LOGD("%s, index %d path:%s value:%s\n", __FUNCTION__, index, mPathforSysNode[index], value);

    len = writeSys(mPathforSysNode[index], value);
    return len;
}

void SysFs::setLogLevel(int level)
{
    mLogLevel = level;
}

const char *SysFs::getSysNode(ConstCharforSysFsNodeIndex index)
{
    return mPathforSysNode[index];
}

int SysFs::getKernelReleaseVersion()
{
    int major = 4;
    int minor = 9;
    struct utsname uts;

    if (uname(&uts) == -1) {
        return major;
    }

    if (sscanf(uts.release, "%d.%d", &major, &minor) != 2) {
        return major;
    }

    SYS_LOGI("getKernelReleaseVersion: %d.%d\n", major,minor);
    return major;
}

void SysFs::initConstCharforSysNode()
{
    int reVersion = getKernelReleaseVersion();

    if (reVersion == 5) {
        //sysfs point
        mPathforSysNode[VIDEO_POLL_STATUS_CHANGE]   = "/sys/class/video_poll/status_changed";
        mPathforSysNode[VIDEO_POLL_PRIMARY_SRC_FMT] = "/sys/class/video_poll/primary_src_fmt";
        mPathforSysNode[VIDEO_CROP]                 = "/sys/class/video/crop";
        mPathforSysNode[VIDEO_SCREEN_MODE]          = "/sys/class/video/screen_mode";
        mPathforSysNode[VIDEO_NONLINEAR_FACTOR]     = "/sys/class/video/nonlinear_factor";
        mPathforSysNode[VIDEO_RGB_SCREEN]           = "/sys/class/video/rgb_screen";
        mPathforSysNode[VIDEO_TEST_SCREEN]          = "/sys/class/video/test_screen";
        mPathforSysNode[VIDEO_FRAME_HEIGHT]         = "/sys/class/video/frame_height";
        mPathforSysNode[AMVECM_PQ_REG_RW]           = "/sys/class/amvecm/pq_reg_rw";
        mPathforSysNode[AMVECM_PQ_DNLP_DEBUG]       = "/sys/class/amvecm/dnlp_debug";
        mPathforSysNode[AMVECM_PQ_USER_SET]         = "/sys/class/amvecm/pq_user_set";
        mPathforSysNode[AMVECM_PQ_CM2_SAT]          = "/sys/class/amvecm/cm2_sat";
        mPathforSysNode[AMVECM_PQ_CM2_HUE_BY_HS]    = "/sys/class/amvecm/cm2_hue_by_hs";
        mPathforSysNode[AMVECM_PQ_CM2_LUMA]         = "/sys/class/amvecm/cm2_luma";
        mPathforSysNode[AML_LDIM_FUNC_EN]           = "/sys/class/aml_ldim/func_en";
        mPathforSysNode[BACKLIGHT_AML_BL_BRIGHTNESS] = "/sys/class/backlight/aml-bl/brightness";
        mPathforSysNode[VFM_MAP]                     = "/sys/class/vfm/map";
        mPathforSysNode[TVAFE_TVAFE0_REG]            = "/sys/class/tvafe/tvafe0/reg";
        mPathforSysNode[LCD_SS]                      = "/sys/class/aml_lcd/ss";
        mPathforSysNode[DISPLAY_MODE]                = "/sys/class/display/mode";
        mPathforSysNode[VDETECT_AIPQ_ENABLE]         = "/sys/class/vdetect/aipq_enable";
        //parameter
        mPathforSysNode[DI_PARAMETERS_DNR_DM_EN]     = "/sys/module/aml_media/parameters/dnr_dm_en";
        mPathforSysNode[DI_PARAMETERS_DNR_EN]        = "/sys/module/aml_media/parameters/dnr_en";
        mPathforSysNode[DI_PARAMETERS_NR2_EN]        = "/sys/module/aml_media/parameters/nr2_en";
        mPathforSysNode[DI_PARAMETERS_MCEN_MODE]     = "/sys/module/aml_media/parameters/mcen_mode";
        mPathforSysNode[DECODER_COMMON_PARAMETERS_DEBUG_VDETECT] = "/sys/module/decoder_common/parameters/debug_vdetect";
        mPathforSysNode[VIDEO_BACKGROUND_COLOR]      = "/sys/class/video/video_background";
        mPathforSysNode[VIDEO_BLACKOUT_POLICY]       = "/sys/class/video/blackout_policy";
        mPathforSysNode[VIDEO_DISABLE_VIDEO]         = "/sys/class/video/disable_video";
        mPathforSysNode[VDIN_SNOW_FLAG]              = "/sys/class/vdin/vdin0/snow_flag";
    } else {
        //sysfs point
        mPathforSysNode[VIDEO_POLL_STATUS_CHANGE]   = "/sys/class/video_poll/status_changed";
        mPathforSysNode[VIDEO_POLL_PRIMARY_SRC_FMT] = "/sys/class/video_poll/primary_src_fmt";
        mPathforSysNode[VIDEO_CROP]                 = "/sys/class/video/crop";
        mPathforSysNode[VIDEO_SCREEN_MODE]          = "/sys/class/video/screen_mode";
        mPathforSysNode[VIDEO_NONLINEAR_FACTOR]     = "/sys/class/video/nonlinear_factor";
        mPathforSysNode[VIDEO_RGB_SCREEN]           = "/sys/class/video/rgb_screen";
        mPathforSysNode[VIDEO_TEST_SCREEN]          = "/sys/class/video/test_screen";
        mPathforSysNode[VIDEO_FRAME_HEIGHT]         = "/sys/class/video/frame_height";
        mPathforSysNode[AMVECM_PQ_REG_RW]           = "/sys/class/amvecm/pq_reg_rw";
        mPathforSysNode[AMVECM_PQ_DNLP_DEBUG]       = "/sys/class/amvecm/dnlp_debug";
        mPathforSysNode[AMVECM_PQ_USER_SET]         = "/sys/class/amvecm/pq_user_set";
        mPathforSysNode[AMVECM_PQ_CM2_SAT]          = "/sys/class/amvecm/cm2_sat";
        mPathforSysNode[AMVECM_PQ_CM2_HUE_BY_HS]    = "/sys/class/amvecm/cm2_hue_by_hs";
        mPathforSysNode[AMVECM_PQ_CM2_LUMA]         = "/sys/class/amvecm/cm2_luma";
        mPathforSysNode[AML_LDIM_FUNC_EN]           = "/sys/class/aml_ldim/func_en";
        mPathforSysNode[BACKLIGHT_AML_BL_BRIGHTNESS] = "/sys/class/backlight/aml-bl/brightness";
        mPathforSysNode[VFM_MAP]                     = "/sys/class/vfm/map";
        mPathforSysNode[TVAFE_TVAFE0_REG]            = "/sys/class/tvafe/tvafe0/reg";
        mPathforSysNode[LCD_SS]                      = "/sys/class/lcd/ss";
        mPathforSysNode[DISPLAY_MODE]                = "/sys/class/display/mode";
        mPathforSysNode[VDETECT_AIPQ_ENABLE]         = "/sys/class/vdetect/aipq_enable";
        //parameter
        mPathforSysNode[DI_PARAMETERS_DNR_DM_EN]     = "/sys/module/di/parameters/dnr_dm_en";
        mPathforSysNode[DI_PARAMETERS_DNR_EN]        = "/sys/module/di/parameters/dnr_en";
        mPathforSysNode[DI_PARAMETERS_NR2_EN]        = "/sys/module/di/parameters/nr2_en";
        mPathforSysNode[DI_PARAMETERS_MCEN_MODE]     = "/sys/module/di/parameters/mcen_mode";
        mPathforSysNode[DECODER_COMMON_PARAMETERS_DEBUG_VDETECT] = "/sys/module/decoder_common/parameters/debug_vdetect";
        mPathforSysNode[VIDEO_BACKGROUND_COLOR]      = "/sys/class/video/video_background";
        mPathforSysNode[VIDEO_BLACKOUT_POLICY]       = "/sys/class/video/blackout_policy";
        mPathforSysNode[VIDEO_DISABLE_VIDEO]         = "/sys/class/video/disable_video";
        mPathforSysNode[VDIN_SNOW_FLAG]              = "/sys/class/vdin/vdin0/snow_flag";
    }
}
