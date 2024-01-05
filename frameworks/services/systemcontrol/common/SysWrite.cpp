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
#include <SysWrite.h>
#include <common.h>
#include <sys/utsname.h>

#include <sys/ioctl.h>
#include <sys/types.h>

#ifndef RECOVERY_MODE
//#include <../../../provision/ca/include/provision_api.h>
#include <getopt.h>
#include <sys/stat.h>
//#include <tee_client_api.h>
//#include <ta.h>
#endif

SysWrite *SysWrite::mInstance = NULL;
SysWrite *SysWrite::GetInstance() {
    if (NULL == mInstance) {
        mInstance = new SysWrite();
    }

    return mInstance;
}


SysWrite::SysWrite()
    :mLogLevel(LOG_LEVEL_DEFAULT) {
    initConstCharforSysNode();
}

SysWrite::~SysWrite() {
}

bool SysWrite::getProperty(const char *key, char *value) {
    property_get(key, value, "");
    /*
    char buf[PROPERTY_VALUE_MAX] = {0};
    property_get(key, buf, "");
    value.setTo(String16(buf));
    */
    return true;
}

bool SysWrite::getPropertyString(const char *key, char *value,  const char *def) {
    property_get(key, value, def);
    return true;
}

int32_t SysWrite::getPropertyInt(const char *key, int32_t def) {
    int len;
    char* end;
    char buf[PROPERTY_VALUE_MAX] = {0};
    int32_t result = def;

    len = property_get(key, buf, "");
    if (len > 0) {
        result = strtol(buf, &end, 0);
        if (end == buf) {
            result = def;
        }
    }

    return result;
}

int64_t SysWrite::getPropertyLong(const char *key, int64_t def) {

    int len;
    char buf[PROPERTY_VALUE_MAX] = {0};
    char* end;
    int64_t result = def;

    len = property_get(key, buf, "");
    if (len > 0) {
        result = strtoll(buf, &end, 0);
        if (end == buf) {
            result = def;
        }
    }

    return result;
}

bool SysWrite::getPropertyBoolean(const char *key, bool def) {

    int len;
    char buf[PROPERTY_VALUE_MAX] = {0};
    bool result = def;

    len = property_get(key, buf, "");
    if (len == 1) {
        char ch = buf[0];
        if (ch == '0' || ch == 'n')
            result = false;
        else if (ch == '1' || ch == 'y')
            result = true;
    } else if (len > 1) {
         if (!strcmp(buf, "no") || !strcmp(buf, "false") || !strcmp(buf, "off")) {
            result = false;
        } else if (!strcmp(buf, "yes") || !strcmp(buf, "true") || !strcmp(buf, "on")) {
            result = true;
        }
    }

    return result;
}

void SysWrite::setProperty(const char *key, const char *value) {
    int err;
    err = property_set(key, value);
    if (err < 0) {
        SYS_LOGE("failed to set system property %s\n", key);
    }
}

bool SysWrite::readSysfs(const char *path, char *value) {
    char buf[MAX_STR_LEN+1] = {0};
    readSys(path, (char*)buf, MAX_STR_LEN, false);
    buf[MAX_STR_LEN] = '\0';
    strcpy(value, buf);
    return true;
}

bool SysWrite::readSysfs(ConstCharforSysNodeIndex index, char *value) {
    readSysfs(mPathforSysNode[index], value);
    return true;
}

int SysWrite::readSysfs(ConstCharforSysNodeIndex index, char *buf, int count) {
    int len = -1;

    SYS_LOGD("index %d path:%s count:%d\n", index, mPathforSysNode[index], count);

    len = readSys(mPathforSysNode[index], (char*)buf, count, false);

    return len;
}

// get the original data from sysfs without any change.
bool SysWrite::readSysfsOriginal(const char *path, char *value) {
    char buf[MAX_STR_LEN+1] = {0};
    readSys(path, (char*)buf, MAX_STR_LEN, true);
    buf[MAX_STR_LEN] = '\0';
    strcpy(value, buf);
    return true;
}

bool SysWrite::readSysfsOriginal(ConstCharforSysNodeIndex index, char *value) {
    readSysfsOriginal(mPathforSysNode[index], value);
    return true;
}

int SysWrite::readSysfsOriginal(ConstCharforSysNodeIndex index, char *buf, int count) {
    int len = -1;

    SYS_LOGD("index:%d path:%s count:%d\n", index, mPathforSysNode[index], count);

    len = readSys(mPathforSysNode[index], (char*)buf, count, true);

    return len;
}

bool SysWrite::writeValidMode(const char *path, const char *outputmode)
{
    int fd;

    SYS_LOGD("write %s, outputmode:%s\n", path, outputmode);

    if ((fd = open(path, O_WRONLY)) < 0) {
        SYS_LOGE("writeSysFs, open %s fail.", path);
        return false;
    }

    if (write(fd, outputmode, strlen(outputmode)) != strlen(outputmode)) {
        SYS_LOGD("valid mode is false!\n");
        close(fd);
        return false;
    }

    SYS_LOGD("valid mode is true!\n");
    close(fd);
    return true;
}

bool SysWrite::writeSysfs(const char *path, const char *value) {
    writeSys(path, value);
    return true;
}

bool SysWrite::writeSysfs(const char *path, const char *value, const int size) {
    int ret;
    ret = writeSys(path, value, size);

    if (ret == 0)
        return true;
    else
        return false;
}

int SysWrite::writeSysfs(ConstCharforSysNodeIndex index, const char *value) {
    int len = -1;

    SYS_LOGD("index %d path:%s value:%s\n", index, mPathforSysNode[index], value);

    len = writeSys(mPathforSysNode[index], value);
    return len;
}

//key start
bool SysWrite::writeUnifyKey(const char *path, const char *value) {
    int ret;
    ret = writeUnifyKeyfs(path, value);
    if (ret == 0)
        return true;
    else
        return false;
}


bool SysWrite::readUnifyKey(const char *path, char *value) {
    char buf[MAX_STR_LEN+1] = {0};
    int ret = readUnifyKeyfs(path, (char*)buf, MAX_STR_LEN);

    if (ret >= 1) {
        buf[ret] = '\0';
        strcpy(value, buf);
        SYS_LOGI("readUnifyKey, value: %s", value);
        return true;
    } else
        return false;
}
//key end

void SysWrite::setLogLevel(int level) {
    mLogLevel = level;
}

const char *SysWrite::getSysNode(ConstCharforSysNodeIndex index) {
    return mPathforSysNode[index];
}

int  SysWrite::writeSys(const char *path, const char *val) {
    int fd;
    int len = -1;

    if ((fd = open(path, O_RDWR)) < 0) {
        SYS_LOGE("writeSysFs, open %s fail.", path);
        return -1;
    }

    SYS_LOGD("write %s, val:%s\n", path, val);

    len = write(fd, val, strlen(val));
    if (len != strlen(val))
        SYS_LOGE("write %s failed!\n", path);

    close(fd);
    return len;
}

int SysWrite::writeSys(const char *path, const char *val, const int size) {
    int fd;

    SYS_LOGD("writeSysFs, size = %d \n", size);

    if ((fd = open(path, O_WRONLY)) < 0) {
        SYS_LOGE("writeSysFs, open %s fail.", path);
        return -1;
    }

    if (write(fd, val, size) != size) {
        SYS_LOGE("write %s size:%d failed!\n", path, size);
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}


int SysWrite::readUnifyKeyfs(const char *path, char *value, int count) {
    int keyLen = 0;
    char existKey[11] = {0};

    writeSys(UNIFYKEY_ATTACH, "1");
    writeSys(UNIFYKEY_NAME, path);

    int len = readSys(UNIFYKEY_EXIST, (char*)existKey, 10);
    if (len >= 1)
        existKey[len] = '\0';
    else
        memset(existKey, '\0', sizeof(existKey));

    if (0 == strcmp(existKey, "0")) {
        SYS_LOGE("do not write key to the storage");
        goto _exit;
    }

    keyLen = readSys(UNIFYKEY_READ, value, count);
    if (keyLen < 1) {
        SYS_LOGE("read key length fail, at least 1 bytes, but read len = %d\n", keyLen);
        goto _exit;
    }

    SYS_LOGI("read success, read len = %d\n", keyLen);
_exit:
    return keyLen;
}

int SysWrite::writeUnifyKeyfs(const char *path, const char *value) {
    int keyLen;
    char existKey[11] = {0};
    int ret;
    char lock_str[10] = {0};
    int size = 0;
    writeSys(UNIFYKEY_ATTACH, "1");
    writeSys(UNIFYKEY_NAME, path);
    size = strlen(value);

    do {
        readSys(UNIFYKEY_EXIST, (char*)lock_str, 10);
        ret = atoi(lock_str);
        SYS_LOGE("ret = %d\n", ret);
    }while(ret != 0);

    writeSys(UNIFYKEY_LOCK, "1");

    keyLen = writeSys(UNIFYKEY_WRITE, value, size);
    if (keyLen != 0) {
        SYS_LOGE("write key length fail\n");
        writeSys(UNIFYKEY_LOCK, "0");
        return -1;
    }

    if (usleep(100*1000) < 0)
        SYS_LOGE("usleep interrupt!\n");

    int len = readSys(UNIFYKEY_EXIST, (char*)existKey, 10);
    if (len >= 1)
        existKey[len] = '\0';
    else
        memset(existKey, '\0', sizeof(existKey));

    if (0 == strcmp(existKey, "0")) {
        SYS_LOGE("do not write key to the storage");
        writeSys(UNIFYKEY_LOCK, "0");
        return -1;
    }
    SYS_LOGI("unify key write success\n");

    writeSys(UNIFYKEY_LOCK, "0");
    return 0;
}

int SysWrite::writePlayreadyKeyfs(const char *path, const char *value, const int size) {
    int keyLen;
    char existKey[11] = {0};
    int ret;
    char lock_str[10] = {0};
    writeSys(UNIFYKEY_ATTACH, "1");
    writeSys(UNIFYKEY_NAME, path);

    do {
        readSys(UNIFYKEY_EXIST, (char*)lock_str, 10);
        ret = atoi(lock_str);
        SYS_LOGE("ret = %d\n", ret);
    }while(ret != 0);

    writeSys(UNIFYKEY_LOCK, "1");

    keyLen = writeSys(UNIFYKEY_WRITE, value, size);
    if (keyLen != 0) {
        SYS_LOGE("write key length fail\n");
        writeSys(UNIFYKEY_LOCK, "0");
        return -1;
    }

    if (usleep(100*1000) < 0)
        SYS_LOGE("usleep interrupt!\n");

    int len = readSys(UNIFYKEY_EXIST, (char*)existKey, 10);
    if (len >= 1)
        existKey[len] = '\0';
    else
        memset(existKey, '\0', sizeof(existKey));

    if (0 == strcmp(existKey, "0")) {
        SYS_LOGE("do not write key to the storage");
        writeSys(UNIFYKEY_LOCK, "0");
        return -1;
    }
    SYS_LOGI("unify key write success\n");

    writeSys(UNIFYKEY_LOCK, "0");
    return ret;
}

bool SysWrite::writeNetflixKeyfs(const char *path, const char *value, const int size) {
    SYS_LOGI("writeNetflixKeyfs");
    int keyLen;
    char existKey[11] = {0};
    int ret;
    char lock_str[10] = {0};
    writeSys(UNIFYKEY_ATTACH, "1");
    writeSys(UNIFYKEY_NAME, path);

    do {
        readSys(UNIFYKEY_EXIST, (char*)lock_str, 10);
        ret = atoi(lock_str);
        SYS_LOGE("ret = %d\n", ret);
    }while(ret != 0);

    writeSys(UNIFYKEY_LOCK, "1");

    keyLen = writeSys(UNIFYKEY_WRITE, value, size);
    if (keyLen != 0) {
        SYS_LOGE("write key length fail\n");
        writeSys(UNIFYKEY_LOCK, "0");
        return -1;
    }

    if (usleep(100*1000) < 0)
        SYS_LOGE("usleep interrupt!\n");

    int len = readSys(UNIFYKEY_EXIST, (char*)existKey, 10);
    if (len >= 1)
        existKey[len] = '\0';
    else
        memset(existKey, '\0', sizeof(existKey));

    if (0 == strcmp(existKey, "0")) {
        SYS_LOGE("do not write key to the storage");
        writeSys(UNIFYKEY_LOCK, "0");
        return -1;
    }
    SYS_LOGI("unify key write success\n");

    writeSys(UNIFYKEY_LOCK, "0");

    return ret;
}

bool SysWrite::writeWidevineKeyfs(const char *path, const char *value, const int size) {
    SYS_LOGI("writeWidevineKeyfs");
    int keyLen;
    char existKey[11] = {0};
    int ret;
    char lock_str[10] = {0};
    writeSys(UNIFYKEY_ATTACH, "1");
    writeSys(UNIFYKEY_NAME, path);

    do {
        readSys(UNIFYKEY_EXIST, (char*)lock_str, 10);
        ret = atoi(lock_str);
        SYS_LOGE("ret = %d\n", ret);
    }while(ret != 0);

    writeSys(UNIFYKEY_LOCK, "1");

    keyLen = writeSys(UNIFYKEY_WRITE, value, size);
    if (keyLen != 0) {
        SYS_LOGE("write key length fail\n");
        writeSys(UNIFYKEY_LOCK, "0");
        return -1;
    }

    if (usleep(100*1000) < 0)
        SYS_LOGE("usleep interrupt!\n");

    int len = readSys(UNIFYKEY_EXIST, (char*)existKey, 10);
    if (len >= 1)
        existKey[len] = '\0';
    else
        memset(existKey, '\0', sizeof(existKey));

    if (0 == strcmp(existKey, "0")) {
        SYS_LOGE("do not write key to the storage");
        writeSys(UNIFYKEY_LOCK, "0");
        return -1;
    }
    SYS_LOGI("unify key write success\n");

    writeSys(UNIFYKEY_LOCK, "0");

    return ret;
}

void SysWrite::dump_keyitem_info(struct key_item_info_t *info) {
    if (info == NULL)
        return;
    SYS_LOGI("id: %d\n", info->id);
    SYS_LOGI("name: %s\n", info->name);
    SYS_LOGI("size: %d\n", info->size);
    SYS_LOGI("permit: 0x%x\n", info->permit);
    SYS_LOGI("flag: 0x%x\n", info->flag);
    return;
}

int SysWrite::readAttestationKeyfs(const char * node, const char *name, char *value, int size) {
    SYS_LOGI("readAttestationKeyfs");
    int ret = 0;
    unsigned long ppos;
    int readsize = 0;
    int fp;
    int seek_num;
    struct key_item_info_t key_item_info;
    if ((NULL == node) || (NULL == name)) {
        SYS_LOGE("%s() %d: invalid param!\n", __func__, __LINE__);
        return -1;
    }
    SYS_LOGI("path=%s\n", node);
    fp  = open(node, O_RDWR);
    if (fp < 0) {
        SYS_LOGE("no %s found\n", node);
        return -1;
    }
    strcpy(key_item_info.name, name);
    ret = ioctl(fp, KEYUNIFY_GET_INFO, &key_item_info);
    ppos = key_item_info.id;
    seek_num = lseek(fp, ppos, SEEK_SET);
    if (seek_num != ppos) {
        SYS_LOGI("%s() %d: Can not set offset to %d\n", __func__, __LINE__, seek_num);
    }
    SYS_LOGI("%s() %d: key ioctl  KEYUNIFY_GET_INFO is %d\n", __func__, __LINE__, ret);
    if (ret < 0) {
        close(fp);
        return ret;
    }
    dump_keyitem_info(&key_item_info);
    SYS_LOGI("size =  %d", size);
    if (key_item_info.flag) {
        readsize = read(fp, value, key_item_info.size);
        SYS_LOGI("readsize =  %d", readsize);
    }

    close(fp);
    return readsize;
}

int SysWrite::writeAttestationKeyfs(const char * node, const char *name, const char *buff, const int size) {
    int ret = 0;
    unsigned long ppos;
    int writesize;
    int fp;
    int seek_num;
    struct key_item_info_t key_item_info;
    if ((NULL == node) || (NULL == buff) || (NULL == name)) {
        SYS_LOGE("%s() %d: invalid param!\n", __func__, __LINE__);
        return -1;
    }

    SYS_LOGI("path=%s\n", node);
    fp  = open(node, O_RDWR);
    if (fp < 0) {
        SYS_LOGE("no %s found\n", node);
        return -1;
    }// seek the key index need operate.
    strcpy(key_item_info.name, name);
    ret = ioctl(fp, KEYUNIFY_GET_INFO, &key_item_info);
    ppos = key_item_info.id;
    seek_num = lseek(fp, ppos, SEEK_SET);
    if (seek_num != ppos) {
        SYS_LOGI("%s() %d: Can not set offset to %d\n", __func__, __LINE__, seek_num);
    }

    SYS_LOGI("%s() %d: ret is %d\n", __func__, __LINE__, ret);
    if (ret < 0) {
        close(fp);
        return ret;
    }
    dump_keyitem_info(&key_item_info);

    writesize = write(fp, buff, size);
    if (writesize != size) {
        SYS_LOGI("%s() %d: write %s failed!\n", __func__, __LINE__, key_item_info.name);
    }
    SYS_LOGI("%s() %d, write %d down!\n", __func__, __LINE__, writesize);
    close(fp);
    SYS_LOGI("ret = %d \n", ret);
    return ret;
    SYS_LOGI("writeAttestationKeyfs");
}


int SysWrite::readSys(const char *path, char *buf, int count) {
    int fd, len = -1;

    if ( NULL == buf ) {
        SYS_LOGE("buf is NULL");
        return len;
    }

    if ((fd = open(path, O_RDONLY)) < 0) {
        SYS_LOGE("readSys, open %s fail. Error info [%s]", path, strerror(errno));
        return len;
    }

    len = read(fd, buf, count);
    if (len < 0) {
        SYS_LOGE("read error: %s, %s\n", path, strerror(errno));
    }

    close(fd);
    return len;
}


int SysWrite::readSys(const char *path, char *buf, int count, bool needOriginalData) {
    int fd, len;

    if ( NULL == buf ) {
        SYS_LOGE("buf is NULL");
        return -1;
    }

    if ((fd = open(path, O_RDONLY)) < 0) {
        SYS_LOGE("readSysFs, open %s fail. Error info [%s]", path, strerror(errno));
        return -1;
    }

    len = read(fd, buf, count);
    if (len < 0) {
        SYS_LOGE("read error: %s, %s\n", path, strerror(errno));
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
                SYS_LOGD("read buffer index:%d is a 0x0, replace to spacing \n", i);
            }

            /* delete all the character of '\n' */
            if (0x0a != buf[i]) {
                buf[j++] = buf[i];
            }
        }

        buf[j] = 0x0;
    }

    SYS_LOGD("read %s, result length:%d, val:%s\n", path, len, buf);

exit:
    close(fd);
    return len;
}

int SysWrite::getKernelReleaseVersion() {
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

void SysWrite::initConstCharforSysNode() {
    int reVersion = getKernelReleaseVersion();
    if (reVersion == 5) {
        mPathforSysNode[DI_BYPASS_ALL] = "/sys/module/aml_media/parameters/bypass_all";
        mPathforSysNode[DI_BYPASS_POST] = "/sys/module/aml_media/parameters/bypass_post";
        mPathforSysNode[DET3D_MODE_SYSFS] = "/sys/module/aml_media/parameters/det3d_mode";
        mPathforSysNode[PROG_PROC_SYSFS] = "/sys/module/aml_media/parameters/prog_proc_config";
        mPathforSysNode[DISPLAY_HDMI_HDCP_AUTH] = "/sys/module/aml_media/parameters/hdmi_authenticated";
        mPathforSysNode[DISPLAY_MEDIA_VIDEO_MUTE]   = "/sys/module/aml_media/parameters/video_mute_on";
        mPathforSysNode[PQ_DISPLAY_HDR_POLICY]         = "/sys/module/aml_media/parameters/hdr_policy";
        //sysfs point
        mPathforSysNode[VIDEO_POLL_STATUS_CHANGE]   = "/sys/class/video_poll/status_changed";
        mPathforSysNode[VIDEO_POLL_PRIMARY_SRC_FMT] = "/sys/class/video_poll/primary_src_fmt";
        mPathforSysNode[VIDEO_CROP]                 = "/sys/class/video/crop";
        mPathforSysNode[VIDEO_SCREEN_MODE]          = "/sys/class/video/screen_mode";
        mPathforSysNode[VIDEO_NONLINEAR_FACTOR]     = "/sys/class/video/nonlinear_factor";
        mPathforSysNode[VIDEO_RGB_SCREEN]           = "/sys/class/video/rgb_screen";
        mPathforSysNode[VIDEO_TEST_SCREEN]          = "/sys/class/video/test_screen";
        mPathforSysNode[VIDEO_FRAME_HEIGHT]         = "/sys/class/video/frame_height";
        mPathforSysNode[VIDEO_AISR_ENABLE]          = "/sys/module/aml_media/parameters/uvm_open_nn";
        mPathforSysNode[VIDEO_SR_ENABLE]            = "/sys/class/video/sr";
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
        mPathforSysNode[AMDOLBY_VISION_HDR10_POLICY] = "/sys/module/aml_media/parameters/dolby_vision_hdr10_policy";
        mPathforSysNode[AML_AUTO_NR_PARAMS]          = "/sys/class/deinterlace/di0/autonr_param";
        //parameter
        mPathforSysNode[DI_PARAMETERS_DNR_DM_EN]     = "/sys/module/aml_media/parameters/dnr_dm_en";
        mPathforSysNode[DI_PARAMETERS_DNR_EN]        = "/sys/module/aml_media/parameters/dnr_en";
        mPathforSysNode[DI_PARAMETERS_NR2_EN]        = "/sys/module/aml_media/parameters/nr2_en";
        mPathforSysNode[DI_PARAMETERS_MCEN_MODE]     = "/sys/module/aml_media/parameters/mcen_mode";
        mPathforSysNode[AIPQ_PARAMETERS_UVM_OPEN]    = "/sys/module/aml_media/parameters/uvm_open_aipq";
        mPathforSysNode[AISR_PARAMETERS_UVM_OPEN_NN] = "/sys/module/aml_media/parameters/uvm_open_nn";
        mPathforSysNode[DECODER_COMMON_PARAMETERS_DEBUG_VDETECT] = "/sys/module/decoder_common/parameters/debug_vdetect";
        mPathforSysNode[VIDEO_BACKGROUND_COLOR]      = "/sys/class/video/video_background";
        mPathforSysNode[VIDEO_BLACKOUT_POLICY]       = "/sys/class/video/blackout_policy";
        mPathforSysNode[VIDEO_DISABLE_VIDEO]         = "/sys/class/video/disable_video";
        mPathforSysNode[VDIN_SNOW_FLAG]              = "/sys/class/vdin/vdin0/snow_flag";
        mPathforSysNode[VPP_AFD_MODULE_ASPECT_MODE]  = "/sys/class/afd_module/aspect_mode";
    } else {
        mPathforSysNode[DI_BYPASS_ALL] = "/sys/module/di/parameters/bypass_all";
        mPathforSysNode[DI_BYPASS_POST] = "/sys/module/di/parameters/bypass_post";
        mPathforSysNode[DET3D_MODE_SYSFS] = "/sys/module/di/parameters/det3d_mode";
        mPathforSysNode[PROG_PROC_SYSFS] = "/sys/module/di/parameters/prog_proc_config";
        mPathforSysNode[DISPLAY_HDMI_HDCP_AUTH] = "/sys/module/hdmitx20/parameters/hdmi_authenticated";
        mPathforSysNode[DISPLAY_MEDIA_VIDEO_MUTE]   = "/sys/module/aml_media/parameters/video_mute_on";
        mPathforSysNode[PQ_DISPLAY_HDR_POLICY]         = "/sys/module/am_vecm/parameters/hdr_policy";
        //sysfs point
        mPathforSysNode[VIDEO_POLL_STATUS_CHANGE]   = "/sys/class/video_poll/status_changed";
        mPathforSysNode[VIDEO_POLL_PRIMARY_SRC_FMT] = "/sys/class/video_poll/primary_src_fmt";
        mPathforSysNode[VIDEO_CROP]                 = "/sys/class/video/crop";
        mPathforSysNode[VIDEO_SCREEN_MODE]          = "/sys/class/video/screen_mode";
        mPathforSysNode[VIDEO_NONLINEAR_FACTOR]     = "/sys/class/video/nonlinear_factor";
        mPathforSysNode[VIDEO_RGB_SCREEN]           = "/sys/class/video/rgb_screen";
        mPathforSysNode[VIDEO_TEST_SCREEN]          = "/sys/class/video/test_screen";
        mPathforSysNode[VIDEO_FRAME_HEIGHT]         = "/sys/class/video/frame_height";
        mPathforSysNode[VIDEO_AISR_ENABLE]          = "/sys/module/aml_media/parameters/uvm_open_nn";
        mPathforSysNode[VIDEO_SR_ENABLE]            = "/sys/class/video/sr";
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
        mPathforSysNode[AML_AUTO_NR_PARAMS]          = "/sys/class/deinterlace/di0/autonr_param";
        //parameter
        mPathforSysNode[DI_PARAMETERS_DNR_DM_EN]     = "/sys/module/di/parameters/dnr_dm_en";
        mPathforSysNode[DI_PARAMETERS_DNR_EN]        = "/sys/module/di/parameters/dnr_en";
        mPathforSysNode[DI_PARAMETERS_NR2_EN]        = "/sys/module/di/parameters/nr2_en";
        mPathforSysNode[DI_PARAMETERS_MCEN_MODE]     = "/sys/module/di/parameters/mcen_mode";
        mPathforSysNode[AIPQ_PARAMETERS_UVM_OPEN]    = "/sys/module/aml_media/parameters/uvm_open_aipq";
        mPathforSysNode[AISR_PARAMETERS_UVM_OPEN_NN] = "/sys/module/aml_media/parameters/uvm_open_nn";
        mPathforSysNode[DECODER_COMMON_PARAMETERS_DEBUG_VDETECT] = "/sys/module/decoder_common/parameters/debug_vdetect";
        mPathforSysNode[VIDEO_BACKGROUND_COLOR]      = "/sys/class/video/video_background";
        mPathforSysNode[VIDEO_BLACKOUT_POLICY]       = "/sys/class/video/blackout_policy";
        mPathforSysNode[VIDEO_DISABLE_VIDEO]         = "/sys/class/video/disable_video";
        mPathforSysNode[VDIN_SNOW_FLAG]              = "/sys/class/vdin/vdin0/snow_flag";
        mPathforSysNode[VPP_AFD_MODULE_ASPECT_MODE]  = "/sys/class/afd_module/aspect_mode";
    }
    mPathforSysNode[SYSFS_BOOT_TYPE]            = "/sys/power/boot_type";
    mPathforSysNode[SYS_DISPLAY_RESOLUTION]     = "/sys/class/video/device_resolution";
    mPathforSysNode[DISPLAY_HDMI_HDCP_VER]      = "/sys/class/amhdmitx/amhdmitx0/hdcp_ver";
    mPathforSysNode[DISPLAY_HDMI_HDCP_MODE]     = "/sys/class/amhdmitx/amhdmitx0/hdcp_mode";
    mPathforSysNode[DISPLAY_HDMI_HDCP_CONF]     = "/sys/class/amhdmitx/amhdmitx0/hdcp_ctrl";
    mPathforSysNode[DISPLAY_HDMI_HDCP_KEY]      = "/sys/class/amhdmitx/amhdmitx0/hdcp_lstore";
    mPathforSysNode[DISPLAY_HDMI_HDCP_POWER]    = "/sys/class/amhdmitx/amhdmitx0/hdcp_pwr";
    mPathforSysNode[DISPLAY_FB0_BLANK]          = "/sys/class/graphics/fb0/blank";
    mPathforSysNode[DISPLAY_FB1_BLANK]          = "/sys/class/graphics/fb1/blank";
    mPathforSysNode[DISPLAY_FB0_FREESCALE]      = "/sys/class/graphics/fb0/free_scale";
    mPathforSysNode[DISPLAY_FB1_FREESCALE]      = "/sys/class/graphics/fb1/free_scale";
    mPathforSysNode[DISPLAY_FB0_FREESCALE_AXIS] = "/sys/class/graphics/fb0/free_scale_axis";
    mPathforSysNode[DISPLAY_FB0_WINDOW_AXIS]    = "/sys/class/graphics/fb0/window_axis";
    mPathforSysNode[DISPLAY_HDMI_SYSCTRL_READY] = "/sys/class/amhdmitx/amhdmitx0/sysctrl_enable";
    mPathforSysNode[DISPLAY_HPD_STATE]          = "/sys/class/amhdmitx/amhdmitx0/hpd_state";
    mPathforSysNode[DISPLAY_HDMI_DISP_CAP]      = "/sys/class/amhdmitx/amhdmitx0/disp_cap";
    mPathforSysNode[DISPLAY_HDMI_DISP_CAP_3D]   = "/sys/class/amhdmitx/amhdmitx0/disp_cap_3d";
    mPathforSysNode[DISPLAY_HDMI_DEEP_COLOR]    = "/sys/class/amhdmitx/amhdmitx0/dc_cap";
    mPathforSysNode[DISPLAY_HDMI_HDR]           = "/sys/class/amhdmitx/amhdmitx0/hdr_cap";
    mPathforSysNode[DISPLAY_HDMI_HDR_CAP2]      = "/sys/class/amhdmitx/amhdmitx0/hdr_cap2";
    mPathforSysNode[DISPLAY_HDMI_AUDIO]         = "/sys/class/amhdmitx/amhdmitx0/aud_cap";
    mPathforSysNode[DISPLAY_HDMI_AUDIO_MUTE]    = "/sys/class/amhdmitx/amhdmitx0/aud_mute";
    mPathforSysNode[DISPLAY_HDMI_VIDEO_MUTE]    = "/sys/class/amhdmitx/amhdmitx0/vid_mute";
    mPathforSysNode[DISPLAY_HDMI_MODE_PREF]     = "/sys/class/amhdmitx/amhdmitx0/preferred_mode";
    mPathforSysNode[DISPLAY_HDMI_SINK_TYPE]     = "/sys/class/amhdmitx/amhdmitx0/sink_type";
    mPathforSysNode[DISPLAY_HDMI_USED]          = "/sys/class/amhdmitx/amhdmitx0/hdmi_used";
    mPathforSysNode[DISPLAY_HDMI_AVMUTE_SYSFS]  = "/sys/devices/virtual/amhdmitx/amhdmitx0/avmute";
    mPathforSysNode[DISPLAY_EDID_VALUE]         = "/sys/class/amhdmitx/amhdmitx0/edid";
    mPathforSysNode[DISPLAY_EDID_STATUS]        = "/sys/class/amhdmitx/amhdmitx0/edid_parsing";
    mPathforSysNode[DISPLAY_EDID_RAW]           = "/sys/class/amhdmitx/amhdmitx0/rawedid";
    mPathforSysNode[DISPLAY_HDMI_PHY]           = "/sys/class/amhdmitx/amhdmitx0/phy";
    mPathforSysNode[DISPLAY_HDMI_FRL_RATE]      = "/sys/class/amhdmitx/amhdmitx0/frl_rate";
    mPathforSysNode[DISPLAY_HDMI_HDR_PRIORITY]  = "/sys/class/amhdmitx/amhdmitx0/hdr_priority";
    mPathforSysNode[AUDIO_DSP_DIGITAL_RAW]      = "/sys/class/audiodsp/digital_raw";
    mPathforSysNode[AV_HDMI_CONFIG]             = "/sys/class/amhdmitx/amhdmitx0/config";
    mPathforSysNode[AV_HDMI_3D_SUPPORT]         = "/sys/class/amhdmitx/amhdmitx0/support_3d";
    mPathforSysNode[HDMI_TX_PLUG_STATE]         = "/sys/class/extcon/hdmi/state";
    mPathforSysNode[HDMI_TX_SWITCH_HDR]         = "/sys/class/extcon/hdmi_hdr/state";
    //auto low latency mode
    mPathforSysNode[AUTO_LOW_LATENCY_MODE_CAP]  = "/sys/class/amhdmitx/amhdmitx0/allm_cap";
    mPathforSysNode[AUTO_LOW_LATENCY_MODE]      = "/sys/class/amhdmitx/amhdmitx0/allm_mode";
    mPathforSysNode[HDMI_CONTENT_TYPE_CAP]      = "/sys/class/amhdmitx/amhdmitx0/contenttype_cap";
    mPathforSysNode[HDMI_CONTENT_TYPE]          = "/sys/class/amhdmitx/amhdmitx0/contenttype_mode";
    mPathforSysNode[DV_SUPPORT_INFO]            = "/sys/class/amdolby_vision/support_info";
    mPathforSysNode[VIDEO_AIFACE_ENABLE]        = "/sys/module/aml_media/parameters/uvm_open_aiface";
    mPathforSysNode[AICOLOR_PARAMETERS_UVM_OPEN] = "/sys/module/aml_media/parameters/uvm_open_aicolor";
    mPathforSysNode[PQ_MODULE_MEMC_DEMO_WIN] = "/sys/class/frc/param";
    mPathforSysNode[PQ_MODULE_AISR_DEMO_EN] = "/sys/class/video/aisr_demo_en";
    mPathforSysNode[PQ_MODULE_AISR_DEMO_AXIS] = "/sys/class/video/aisr_demo_axis";
}
#if 0
status_t SysWrite::dump(int fd, const Vector<String16>& args){
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;
    if (checkCallingPermission(String16("android.permission.DUMP")) == false) {
        snprintf(buffer, SIZE, "Permission Denial: "
                "can't dump sys.write from pid=%d, uid=%d\n",
                IPCThreadState::self()->getCallingPid(),
                IPCThreadState::self()->getCallingUid());
        result.append(buffer);
    } else {
        Mutex::Autolock lock(mLock);

        result.appendFormat("sys write service wrote by multi-user mode, normal process will have not system privilege\n");
        /*
        int n = args.size();
        for (int i = 0; i + 1 < n; i++) {
            String16 verboseOption("-v");
            if (args[i] == verboseOption) {
                String8 levelStr(args[i+1]);
                int level = atoi(levelStr.string());
                result = String8::format("\nSetting log level to %d.\n", level);
                setLogLevel(level);
                write(fd, result.string(), result.size());
            }
        }*/
    }
    write(fd, result.string(), result.size());
    return NO_ERROR;
}
#endif
