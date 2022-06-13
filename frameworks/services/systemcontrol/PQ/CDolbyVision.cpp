/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "SystemControl"
#define LOG_TV_TAG "CDolbyVision"

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include "CPQLog.h"
#include "CDolbyVision.h"

CDolbyVision::CDolbyVision(const char *binFilePath, const char *cfgFilePath) {
    SetDolbyCfgFile(binFilePath, cfgFilePath);
}

CDolbyVision::~CDolbyVision() {

}

int CDolbyVision::DeviceIOCtl(int request, ...)
{
    int ret = -1;
    int dolbyDevFd = -1;
    va_list ap;
    void *arg;

    dolbyDevFd = open(DOLBY_DEV_PATH, O_RDWR);
    if (dolbyDevFd >= 0) {
        va_start(ap, request);
        arg = va_arg(ap, void *);
        va_end(ap);
        ret = ioctl(dolbyDevFd, request, arg);
        if (ret < 0) {
            SYS_LOGD("%s failed(%s)!\n", __FUNCTION__, strerror(errno));
        } else {
            SYS_LOGD("%s success!\n", __FUNCTION__);
        }
        close(dolbyDevFd);
    } else {
        SYS_LOGE("%s: Open dolby module error(%s).\n", __FUNCTION__, strerror(errno));
    }

    return ret;
}

int CDolbyVision::SetDolbyCfgFile(const char *binFilePath, const char *cfgFilePath){
    dolby_config_file_t dolbyCfgFile;
    memset(&dolbyCfgFile, 0, sizeof(dolby_config_file_t));
    if ((binFilePath == NULL) || (cfgFilePath == NULL)) {
        SYS_LOGD("%s: file path invalid\n", __FUNCTION__);
        return -1;
    } else {
        strncpy((char*)dolbyCfgFile.bin_name, binFilePath, 255);
        strncpy((char*)dolbyCfgFile.cfg_name, cfgFilePath, 255);
    }

    SYS_LOGD("%s: bin_name=%s, cfg_name=%s\n", __FUNCTION__, dolbyCfgFile.bin_name, dolbyCfgFile.cfg_name);
    int ret = DeviceIOCtl(DOLBY_IOC_SET_DV_CONFIG_FILE, &dolbyCfgFile);
    if (ret < 0) {
        SYS_LOGD("%s failed!\n", __FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n", __FUNCTION__);
    }

    return ret;
}

int CDolbyVision::SetDolbyPQMode(dolby_pq_mode_t mode) {
    int ret = DeviceIOCtl(DOLBY_IOC_SET_DV_PIC_MODE_ID, &mode);
    if (ret < 0) {
        SYS_LOGD("%s failed!\n", __FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n", __FUNCTION__);
    }

    return ret;

}

dolby_pq_mode_t CDolbyVision::GetDolbyPQMode(void) {
    dolby_pq_mode_t dolbyPQmode = DOLBY_PQ_MODE_BRIGHT_DV;
    int ret = DeviceIOCtl(DOLBY_IOC_GET_DV_PIC_MODE_ID, &dolbyPQmode);
    if (ret < 0) {
        SYS_LOGD("%s failed!\n", __FUNCTION__);
        dolbyPQmode = DOLBY_PQ_MODE_BRIGHT_DV;
    } else {
        SYS_LOGD("%s success, mode is %d!\n", __FUNCTION__, dolbyPQmode);
    }

    return dolbyPQmode;
}

int CDolbyVision::SetDolbyPQParam(dolby_pq_mode_t mode, dolby_pq_item_t iteamID, int value) {
    dolby_pq_info_t dolbyPQInfo;
    memset(&dolbyPQInfo, 0, sizeof(dolbyPQInfo));
    dolbyPQInfo.dolby_pic_mode_id = mode;
    dolbyPQInfo.item = iteamID;
    dolbyPQInfo.value = value;
    int ret = DeviceIOCtl(DOLBY_IOC_SET_DV_SINGLE_PQ_VALUE, &dolbyPQInfo);
    if (ret < 0) {
        SYS_LOGD("%s failed!\n", __FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n", __FUNCTION__);
    }

    return ret;
}

int CDolbyVision::GetDolbyPQParam(dolby_pq_mode_t mode, dolby_pq_item_t iteamID) {
    dolby_pq_info_t dolbyPQInfo;
    memset(&dolbyPQInfo, 0, sizeof(dolbyPQInfo));
    dolbyPQInfo.dolby_pic_mode_id = mode;
    dolbyPQInfo.item = iteamID;

    int ret = DeviceIOCtl(DOLBY_IOC_GET_DV_SINGLE_PQ_VALUE, &dolbyPQInfo);
    if (ret < 0) {
        SYS_LOGD("%s failed!\n", __FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n", __FUNCTION__);
    }

    return ret;
}

int CDolbyVision::SetDolbyPQFullParam(dolby_full_pq_info_t fullInfo) {
    int ret = DeviceIOCtl(DOLBY_IOC_GET_DV_FULL_PQ_VALUE, &fullInfo);
    if (ret < 0) {
        SYS_LOGD("%s failed!\n", __FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n", __FUNCTION__);
    }

    return ret;
}

int CDolbyVision::GetDolbyPQFullParam(dolby_full_pq_info_t *fullInfo) {
    int ret = 0;
    dolby_full_pq_info_t dolbyPQFullInfo;
    memset(&dolbyPQFullInfo, 0, sizeof(dolbyPQFullInfo));
    dolbyPQFullInfo.pic_mode_id = fullInfo->pic_mode_id;
    ret = DeviceIOCtl(DOLBY_IOC_GET_DV_FULL_PQ_VALUE, &dolbyPQFullInfo);
    if (ret < 0) {
        SYS_LOGD("%s failed!\n", __FUNCTION__);
    } else {
        SYS_LOGD("%s success, brightness:%d, contrast:%d, saturation:%d, hue:%d.\n",
                 __FUNCTION__,
                 dolbyPQFullInfo.brightness,
                 dolbyPQFullInfo.contrast,
                 dolbyPQFullInfo.saturation,
                 dolbyPQFullInfo.colorshift);
    }

    memcpy(fullInfo, &dolbyPQFullInfo, sizeof(dolbyPQFullInfo));
    return ret;
}
