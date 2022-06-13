/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "tvserver"
#define LOG_TV_TAG "tvutils"

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/prctl.h>
#include <stdlib.h>
#include <android/log.h>
#include <cutils/android_reboot.h>
#include <cutils/properties.h>
#include <dirent.h>

#include <utils/threads.h>
#include <binder/IServiceManager.h>
#include <systemcontrol/SystemControlClient.h>

#include "include/tvconfig.h"
#include "include/tvutils.h"
#include "include/CTvLog.h"

#include <vector>
#include <map>
#include <string>
using namespace android;

//static pthread_mutex_t file_attr_control_flag_mutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_t UserPet_ThreadId = 0;
static unsigned char is_turnon_user_pet_thread = false;
static unsigned char is_user_pet_thread_start = false;
static unsigned int user_counter = 0;
static unsigned int user_pet_terminal = 1;

static Mutex amLock;
static sp<SystemControlClient> sysctrlClient = nullptr;
static const sp<SystemControlClient> &getSystemControlService()
{
    Mutex::Autolock _l(amLock);
    if (sysctrlClient == nullptr) {
        sysctrlClient = SystemControlClient::getInstance();
    }
    ALOGE_IF(sysctrlClient == nullptr, "no System Control Service!?");

    return sysctrlClient;
}

int getBootEnv(const char *key, char *value, const char *def_val)
{
    const sp<SystemControlClient> sws = getSystemControlService();
    if (sws != nullptr) {
        std::string v;
        if (sws->getBootEnv(key, v)) {
            strcpy(value, v.c_str());
            return 0;
        }
    }

    strcpy(value, def_val);
    return -1;
}

void setBootEnv(const char *key, const char *value)
{
    const sp<SystemControlClient> &sws = getSystemControlService();
    if (sws != nullptr) {
        sws->setBootEnv(key, value);
    }
}

int writeSys(const char *path, const char *val) {
    int fd;

    if ((fd = open(path, O_RDWR)) < 0) {
        LOGE("writeSys, open %s error(%s)", path, strerror (errno));
        return -1;
    }

    LOGD("write %s, val:%s\n", path, val);

    int len = write(fd, val, strlen(val));
    close(fd);
    return len;
}

int readSys(const char *path, char *buf, int count) {
    int fd, len;

    if ( NULL == buf ) {
        LOGE("buf is NULL");
        return -1;
    }

    if ((fd = open(path, O_RDONLY)) < 0) {
        LOGE("readSys, open %s error(%s)", path, strerror (errno));
        return -1;
    }

    len = read(fd, buf, count);
    if (len < 0) {
        LOGE("read %s error, %s\n", path, strerror(errno));
        goto exit;
    }

    int i , j;
    for (i = 0, j = 0; i <= len -1; i++) {
        //change '\0' to 0x20(spacing), otherwise the string buffer will be cut off ,if the last char is '\0' should not replace it
        if (0x0 == buf[i] && i < len - 1) {
            buf[i] = 0x20;
        }
        /* delete all the character of '\n' */
        if (0x0a != buf[i]) {
            buf[j++] = buf[i];
        }
    }
    buf[j] = 0x0;

    //LOGI("read %s, result length:%d, val:%s\n", path, len, buf);

exit:
    close(fd);
    return len;
}

int tvReadSysfs(const char *path, char *value) {
#ifdef USE_SYSTEM_CONTROL
    const sp<SystemControlClient> &sws = getSystemControlService();
    if (sws != nullptr) {
        std::string v;
        if (sws->readSysfs(path, v)) {
            strcpy(value, v.c_str());
            return 0;
        }
    }
    return -1;
#else
    char buf[SYS_STR_LEN+1] = {0};
    int len = readSys(path, (char*)buf, SYS_STR_LEN);
    strcpy(value, buf);
    return len;
#endif
}

int tvWriteSysfs(const char *path, const char *value) {
#ifdef USE_SYSTEM_CONTROL
    const sp<SystemControlClient> &sws = getSystemControlService();
    if (sws != nullptr) {
        sws->writeSysfs(path, value);
    }
    return 0;
#else
    return writeSys(path, value);
#endif
}

int tvWriteSysfs(const char *path, int value, int base)
{
    char str_value[64] = {0};
    if (base == 16) {
        sprintf(str_value, "0x%-x", value);
    } else {
        sprintf(str_value, "%d", value);
    }
    LOGD("tvWriteSysfs, str_value = %s", str_value);
#ifdef USE_SYSTEM_CONTROL
    const sp<SystemControlClient> &sws = getSystemControlService();
    if (sws != nullptr) {
        sws->writeSysfs(path, str_value);
    }
    return 0;
#else
    return writeSys(path, str_value);
#endif
}

int tvWriteDisplayMode(const char *mode)
{
    const sp<SystemControlClient> &sws = getSystemControlService();
    if (sws != nullptr) {
        sws->setSinkOutputMode(mode);
    }
    return 0;
}

//Add for PQ
int tvLoadPQSettings(source_input_param_t source_input_param)
{
    const sp<SystemControlClient> &sws = getSystemControlService();
    if (sws != nullptr) {
        sws->loadPQSettings(source_input_param);
    }
    return 0;
}

int tvSSMReadNTypes(int id, int data_len, int *data_buf, int offset)
{
    const sp<SystemControlClient> &sws = getSystemControlService();
    if (sws != nullptr) {
        int tmp_val = 0;
        tmp_val = sws->sysSSMReadNTypes(id, data_len, offset);
        *data_buf = tmp_val;
    }
    return 0;
}

int tvSSMWriteNTypes(int id, int data_len, int data_buf, int offset)
{
    const sp<SystemControlClient> &sws = getSystemControlService();
    if (sws != nullptr) {
        sws->sysSSMWriteNTypes(id, data_len, data_buf, offset);
    }
    return 0;

}

int tvGetActualAddr(int id)
{
    const sp<SystemControlClient> &sws = getSystemControlService();
    if (sws != nullptr) {
        sws->getActualAddr(id);
    }
    return 0;

}

int tvGetActualSize(int id)
{
    const sp<SystemControlClient> &sws = getSystemControlService();
    if (sws != nullptr) {
        sws->getActualSize(id);
    }
    return 0;

}

int tvSetPQMode ( vpp_picture_mode_t mode, int is_save, int is_autoswitch)
{
    const sp<SystemControlClient> &sws = getSystemControlService();

    if (sws != nullptr) {
        sws->setPQmode((int)mode, is_save, is_autoswitch);
    }

    return 0;
}

vpp_picture_mode_t tvGetPQMode ( void )
{
    const sp<SystemControlClient> &sws = getSystemControlService();
    if (sws != nullptr) {
        return (vpp_picture_mode_t)sws->getPQmode();
    }

    return VPP_PICTURE_MODE_MAX;
}

int tvSavePQMode ( vpp_picture_mode_t mode )
{
    const sp<SystemControlClient> &sws = getSystemControlService();

    if (sws != nullptr) {
        return sws->savePQmode(mode);
    }

    return -1;
}

vpp_picture_mode_t tvGetLastPQMode ( void )
{
    const sp<SystemControlClient> &sws = getSystemControlService();
    if (sws != nullptr) {
        //if want PQ mode set by user,
        //use return VPP_PICTURE_MODE_MAX;
        return (vpp_picture_mode_t)sws->getLastPQmode();
    }

    return VPP_PICTURE_MODE_MAX;
}

int tvSetCurrentSourceInfo(tv_source_input_t tv_source_input, tvin_sig_fmt_t sig_fmt, tvin_trans_fmt_t trans_fmt)
{
    int s32Ret;
    LOGD("%s: Start LoadPQ SwitchSourceTime = %f", __FUNCTION__, getUptimeSeconds());
    const sp<SystemControlClient> &sws = getSystemControlService();
    if (sws != nullptr) {
        s32Ret = sws->setCurrentSourceInfo((int)tv_source_input, (int)sig_fmt, (int)trans_fmt);
        LOGD("%s: End LoadPQ SwitchSourceTime = %f", __FUNCTION__, getUptimeSeconds());
        return s32Ret;
    }
    return -1;
}

int tvSetCVD2Values()
{
    const sp<SystemControlClient> &sws = getSystemControlService();
    if (sws != nullptr) {
        return sws->setCVD2Values();
    }
    return -1;
}

int tvSetCurrentHdrInfo(int hdr_info)
{
    int s32Ret;
    const sp<SystemControlClient> &sws = getSystemControlService();
    if (sws != nullptr) {
        s32Ret = sws->setCurrentHdrInfo(hdr_info);
        return s32Ret;
    }
    return -1;
}

int tvSetCurrentAspectRatioInfo(int AspectRatioInfo __unused)
{
    int s32Ret = -1;
    const sp<SystemControlClient> &sws = getSystemControlService();
    if (sws != nullptr) {
        //s32Ret = sws->setCurrentAspectRatioInfo(AspectRatioInfo);
        return s32Ret;
    }
    return -1;
}

int setVideoColor(int color)
{
    int s32Ret = -1;
    const sp<SystemControlClient> &sws = getSystemControlService();
    if (sws != nullptr) {
        s32Ret = sws->setVideoScreenColor(color);
        return s32Ret;
    }
    return -1;
}

int getScreenColorSetting()
{
    int s32Ret = -1;
    const sp<SystemControlClient> &sws = getSystemControlService();
    if (sws != nullptr) {
        s32Ret = sws->getScreenColorForSignalChange();
        return s32Ret;
    }
    return -1;
}

//PQ end

int Tv_MiscRegs(const char *cmd)
{
    FILE *fp = NULL;
    fp = fopen("/sys/class/register/reg", "w");

    if (fp != NULL && cmd != NULL) {
        fprintf(fp, "%s", cmd);
        fclose(fp);
        return 0;
    } else {
        LOGE("Open /sys/class/register/reg ERROR(%s)!!\n", strerror(errno));
        if (fp != NULL) {
            fclose(fp);
        }
        return -1;
    }
}

int TvMisc_SetLVDSSSC(int val)
{
    FILE *fp;
    const char* path = NULL;
    int kernelVersion = getKernelMajorVersion();
    if (kernelVersion > 4) {
        path = PATH_LCD_SS_54;
    } else {
        path = PATH_LCD_SS_49;
    }
    fp = fopen(path, "w");
    if (fp != NULL) {
        fprintf(fp, "%d", val);
        fclose(fp);
        return 0;
    } else {
        LOGE("open %s ERROR(%s)!!\n", path, strerror(errno));
        return -1;
    }
}

int TvMisc_SetUserCounterTimeOut(int timeout)
{
    FILE *fp;

    fp = fopen("/sys/devices/platform/aml_wdt/user_pet_timeout", "w");
    if (fp != NULL) {
        fprintf(fp, "%d", timeout);
        fclose(fp);
        return 0;
    } else {
        LOGE("=OSD CPP=> open /sys/devices/platform/aml_wdt/user_pet_timeout ERROR(%s)!!\n", strerror(errno));
        return -1;
    }
}

int TvMisc_SetUserCounter(int count)
{
    FILE *fp;

    fp = fopen(PATH_WDT_USR_PET_49, "w");
    if (fp != NULL) {
        fprintf(fp, "%d", count);
        fclose(fp);
        return 0;
    } else {
        LOGE("%s: open %s ERROR(%s)!!\n", __FUNCTION__, PATH_WDT_USR_PET_49, strerror(errno));
        return -1;
    }
}

int TvMisc_SetUserPetResetEnable(int enable)
{
    FILE *fp;

    fp = fopen(PATH_WDT_USR_PET_RESET_EN_49, "w");

    if (fp != NULL) {
        fprintf(fp, "%d", enable);
        fclose(fp);
        return 0;
    } else {
        LOGE("%s: %s ERROR(%s)!!\n", __FUNCTION__, PATH_WDT_USR_PET_RESET_EN_49, strerror(errno));
        return -1;
    }
}

int TvMisc_SetSystemPetResetEnable(int enable)
{
    FILE *fp;

    fp = fopen("/sys/devices/platform/aml_wdt/reset_enable", "w");

    if (fp != NULL) {
        fprintf(fp, "%d", enable);
        fclose(fp);
        return 0;
    } else {
        LOGE("=OSD CPP=> open /sys/devices/platform/aml_wdt/reset_enable ERROR(%s)!!\n", strerror(errno));
        return -1;
    }
}

int TvMisc_SetSystemPetEnable(int enable)
{
    FILE *fp;

    fp = fopen("/sys/devices/platform/aml_wdt/ping_enable", "w");

    if (fp != NULL) {
        fprintf(fp, "%d", enable);
        fclose(fp);
        return 0;
    } else {
        LOGE("=OSD CPP=> open /sys/devices/platform/aml_wdt/ping_enable ERROR(%s)!!\n", strerror(errno));
        return -1;
    }
}

int SetAudioOutmode(int mode){
    char val[32] = {0};
    sprintf(val, "%d", mode);
    int len = tvWriteSysfs(AUDIO_OUTMODE_PATH, val);
    if (len > 0) {
        char temp[] = "audout_mode";
        tvWriteSysfs(ATVDEMODE_DEBUG_PATH,temp);
    }
    return 0;
}

int GetAudioOutmode(){
    char mode[32] = {0};
    tvReadSysfs(AUDIO_OUTMODE_PATH, mode);
    return atoi(mode);
}

int GetAudioStreamOutmode(){
    char mode[32] = {0};
    char std[32] = {0};
    char temp[] = "signal_audmode";
    tvWriteSysfs(ATVDEMODE_DEBUG_PATH,temp);
    tvReadSysfs(AUDIO_STREAM_OUTMODE_PATH, mode);
    tvReadSysfs(AUDIO_STD_PATH, std);
    return (atoi(mode) & 0xFF) | ((atoi(std) & 0xFF) << 8);
}

int TvMisc_SetSystemPetCounterTimeOut(int timeout)
{
    FILE *fp;

    fp = fopen("/sys/devices/platform/aml_wdt/wdt_timeout", "w");

    if (fp != NULL) {
        fprintf(fp, "%d", timeout);
        fclose(fp);
        return 0;
    } else {
        LOGE("=OSD CPP=> open /sys/devices/platform/aml_wdt/wdt_timeout ERROR(%s)!!\n", strerror(errno));
        return -1;
    }
}

//0-turn off
//1-force non-standard
//2-force normal
int Set_Fixed_NonStandard(int value)
{
    int fd = -1, ret = -1;
    char set_vale[32] = {0};

    sprintf(set_vale, "%d", value);

    fd = open(PATH_AFE_FORCE_NOSTD_49, O_RDWR);
    if (fd >= 0) {
        ret = write(fd, set_vale, strlen(set_vale));
    }

    if (ret <= 0) {
        LOGE("%s: set %s error(%s)!\n", __FUNCTION__, PATH_AFE_FORCE_NOSTD_49, strerror(errno));
    }

    if (fd != -1) {
        close(fd);
        fd = -1;
    }
    return ret;
}

static void *UserPet_TreadRun(void *data __unused)
{
    while (is_turnon_user_pet_thread == true) {
        if (is_user_pet_thread_start == true) {
            usleep(1000 * 1000);
            if (++user_counter == 0xffffffff)
                user_counter = 1;
            TvMisc_SetUserCounter(user_counter);
        } else {
            usleep(10000 * 1000);
        }
    }
    if (user_pet_terminal == 1) {
        user_counter = 0;
    } else {
        user_counter = 1;
    }
    TvMisc_SetUserCounter(user_counter);
    return ((void *) 0);
}

static int UserPet_CreateThread(void)
{
    int ret = 0;
    pthread_attr_t attr;
    struct sched_param param;

    is_turnon_user_pet_thread = true;
    is_user_pet_thread_start = true;

    pthread_attr_init(&attr);
    pthread_attr_setschedpolicy(&attr, SCHED_RR);
    param.sched_priority = 1;
    pthread_attr_setschedparam(&attr, &param);
    ret = pthread_create(&UserPet_ThreadId, &attr, &UserPet_TreadRun, NULL);
    pthread_attr_destroy(&attr);
    return ret;
}

static void UserPet_KillThread(void)
{
    int i = 0, dly = 600;
    is_turnon_user_pet_thread = false;
    is_user_pet_thread_start = false;
    for (i = 0; i < 2; i++) {
        usleep(dly * 1000);
    }
    pthread_join(UserPet_ThreadId, NULL);
    UserPet_ThreadId = 0;
    LOGD("%s, done.", CFG_SECTION_TV);
}

void TvMisc_EnableWDT(bool kernelpet_disable, unsigned int userpet_enable,
    unsigned int kernelpet_timeout, unsigned int userpet_timeout, unsigned int userpet_reset)
{
    TvMisc_SetSystemPetCounterTimeOut(kernelpet_timeout);
    TvMisc_SetSystemPetEnable(1);
    if (kernelpet_disable) {
        TvMisc_SetSystemPetResetEnable(0);
    } else {
        TvMisc_SetSystemPetResetEnable(1);
    }
    if (userpet_enable) {
        TvMisc_SetUserCounterTimeOut(userpet_timeout);
        TvMisc_SetUserPetResetEnable(userpet_reset);
        UserPet_CreateThread();
    } else {
        TvMisc_SetUserCounter(0);
        TvMisc_SetUserPetResetEnable(0);
    }
}

void TvMisc_DisableWDT(unsigned int userpet_enable)
{
    if (userpet_enable) {
        user_pet_terminal = 0;
        UserPet_KillThread();
    }
}

/*---------------delete dir---------------*/
int TvMisc_DeleteDirFiles(const char *strPath, int flag)
{
    int status;
    char tmp[256];
    switch (flag) {
    case 0:
        sprintf(tmp, "rm -f %s", strPath);
        LOGE("%s", tmp);
        system(tmp);
        break;
    case 1:
        sprintf(tmp, "cd %s", strPath);
        LOGE("%s", tmp);
        status = system(tmp);
        if (status > 0 || status < 0)
            return -1;
        sprintf(tmp, "cd %s;rm -rf *", strPath);
        system(tmp);
        LOGE("%s", tmp);
        break;
    case 2:
        sprintf(tmp, "rm -rf %s", strPath);
        LOGE("%s", tmp);
        system(tmp);
        break;
    }
    return 0;
}

//check file exist or not
bool isFileExist(const char *file_name)
{
    struct stat tmp_st;

    return stat(file_name, &tmp_st) == 0;
}

int GetPlatformHaveDDFlag()
{
    const char *config_value;

    config_value = config_get_str(CFG_SECTION_TV, "platform.havedd", "null");
    if (strcmp(config_value, "true") == 0 || strcmp(config_value, "1") == 0) {
        return 1;
    }

    return 0;
}

int GetFileAttrIntValue(const char *fp, int flag)
{
    int fd = -1;
    int temp = -1;
    char temp_str[32];

    memset(temp_str, 0, 32);

    fd = open(fp, flag);

    if (fd < 0) {
        LOGE("open %s ERROR(%s)!!\n", fp, strerror(errno));
        return -1;
    }

    if (read(fd, temp_str, sizeof(temp_str)) > 0) {
        if (sscanf(temp_str, "%d", &temp) >= 0) {
            LOGD("%s -> get %s value =%d!\n", "TV", fp, temp);
            close(fd);
            return temp;
        } else {
            LOGE("%s -> get %s value error(%s)\n", "TV", fp, strerror(errno));
            close(fd);
            return -1;
        }
    }

    close(fd);
    return -1;
}



/*
void split(const std::string &s, std::vector<std::string> &v, const std::string &c)
{
    std::string::size_type pos1, pos2;
    pos2 = s.find(c);
    pos1 = 0;
    while (std::string::npos != pos2)
    {
        v.push_back(s.substr(pos1, pos2-pos1));

        pos1 = pos2 + c.size();
        pos2 = s.find(c, pos1);
    }
    if (pos1 != s.length())
        v.push_back(s.substr(pos1));
}
void stringToMap(std::map<std::string, std::string> &m, const std::string &s)
{
    std::vector<std::string> nvpairs;
    split(s, nvpairs, "&");
    for (std::vector<std::string>::iterator it = nvpairs.begin(); it != nvpairs.end(); it++) {
        std::vector<std::string> nv;
        split(*it, nv, "=");
        if (nv.size() == 2)
            m.insert(std::map<std::string, std::string>::value_type(nv[0], nv[1]));
    }
}

void mapToString(std::string &s, const std::map<std::string, std::string> &m)
{
    for (std::map<std::string, std::string>::const_iterator it = m.begin(); it != m.end(); ++it) {
        if (!s.empty())
            s.append("&");
        s.append(it->first);
        s.append("=");
        s.append(it->second);
    }
}
*/

#include "json/json.h"

void jsonToMap(STR_MAP &m, const std::string &j)
{
    Json::Reader reader;
    Json::Value root;
    Json::FastWriter writer;

    if (!reader.parse(j, root))
        return;

    for (Json::Value::iterator it = root.begin(); it != root.end(); it++) {
        std::string v(writer.write(*it));
        if (v.compare(v.size()-1, 1, "\n") == 0)
            v.erase(v.size()-1);
        if (v[0] == '\"')
            m.insert(STR_MAP::value_type(it.key().asString(), v.substr(1, v.size()-2)));
        else
            m.insert(STR_MAP::value_type(it.key().asString(), v));
    }
}

void mapToJson(std::string &j, const STR_MAP &m)
{
    int has_member = 0;

    if (m.empty())
        return;

    j.append("{");
    for (STR_MAP::const_iterator it = m.begin(); it != m.end(); ++it) {
        if (has_member)
            j.append(",");
        j.append("\"").append(it->first).append("\":").append(it->second);
        has_member = 1;
    }
    j.append("}");
}

void mapToJson(std::string &j, const STR_MAP &m, const std::string &k)
{
    if (m.empty())
        return;

    if (k.size())
        j.append("\"").append(k).append("\":");

    mapToJson(j, m);
}

void mapToJsonAppend(std::string &j, const STR_MAP &m, const std::string &k)
{
    if (m.empty())
        return;

    int append = 0;

    if (!j.empty()) {
        append = 1;
        j.replace(j.size()-1, 1, 1, ',');
    }

    mapToJson(j, m, k);

    if (append)
        j.append("}");
}

Paras Paras::operator + (const Paras &p)
{
    Paras pnew(*this);
    for (STR_MAP::const_iterator it = p.mparas.begin(); it != p.mparas.end(); ++it)
        pnew.mparas.insert(*it);
    return pnew;
}

int Paras::getInt(const char *key, int def) const
{
    STR_MAP::const_iterator it = mparas.find(std::string(key));
    if (it == mparas.end())
        return def;
    return atoi(it->second.c_str());
}

void Paras::setInt(const char *key, int v)
{
    char cs[64] = {0};
    snprintf(cs, sizeof(cs), "%d", v);
    STR_MAP::iterator it = mparas.find(std::string(key));
    if (it != mparas.end()) {
        it->second.assign(cs);
    } else {
        std::pair<std::map<std::string, std::string>::iterator, bool> ret;
        ret = mparas.insert(std::pair<std::string, std::string>(std::string(key), std::string(cs)));
        if (ret.second == false) {
            LOGE("error: map can not insert");
        }
    }
}

float getUptimeSeconds() {
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (float)(ts.tv_sec +(float)ts.tv_nsec / 1000000000);
}

bool jsonisMember(const char *json, const char* key)
{
    Json::Reader reader;
    Json::Value root;

    if (!reader.parse(json, json + strlen(json), root)) {
        LOGD("parse fail:(%s)", json);
        return false;
    }

    return root.isMember(key);
}

int jsonGetInt(const char *json, const char *obj, const char *value, int def)
{
    Json::Reader reader;
    Json::Value root;

    if (!reader.parse(json, json + strlen(json), root)) {
        LOGD("parse fail:(%s)", json);
        return def;
    }
    if (obj && strlen(obj))
        return root.get(obj, Json::Value()).get(value, def).asInt();
    return root.get(value, def).asInt();
}

int64_t jsonGetInt64(const char *json, const char *obj, const char *value, int64_t def)
{
    Json::Reader reader;
    Json::Value root;

    if (!reader.parse(json, json + strlen(json), root)) {
        LOGD("parse fail:(%s)", json);
        return def;
    }
    if (obj && strlen(obj))
        return root.get(obj, Json::Value()).get(value, (Json::Int64)def).asInt64();
    return root.get(value, (Json::Int64)def).asInt64();
}

const std::string jsonToString(Json::Value v)
{
    if (!v.isObject())
        return v.asString();

    Json::FastWriter writer;
    writer.omitEndingLineFeed();
    return writer.write(v);
}

const std::string jsonGetString(const char *json, const char *obj, const char *value, const char *def)
{
    Json::Reader reader;
    Json::Value root;

    if (!reader.parse(json, json + strlen(json), root)) {
        LOGD("parse fail:(%s)", json);
        return def;
    }
    if (obj && strlen(obj))
        return jsonToString(root.get(obj, Json::Value()).get(value, def));
    return jsonToString(root.get(value, def));
}

int paramGetInt(const char *param, const char *section, const char *value, int def) {
    if (!param || !strlen(param))
        return def;
    return jsonGetInt(param, section, value, def);
}

int64_t paramGetInt64(const char *param, const char *section, const char *value, int64_t def) {
    if (!param || !strlen(param))
        return def;
    return jsonGetInt64(param, section, value, def);
}

const std::string paramGetString(const char *param, const char *section, const char *value, const char *def) {
    if (!param || !strlen(param))
        return def;
    return jsonGetString(param, section, value, def);
}

bool propertyGetBool(const char *prop, bool def) {
    return property_get_bool(prop, def ? 1 : 0) ? true : false;
}

bool isVideoInuse() {
    char buf[32] = {0}, value[PROPERTY_VALUE_MAX] = {0};
    int video_inuse = 0, tmp_read = 0, wait_number = 0;
    tvReadSysfs(SYS_VIDEO_INUSE_PATH, buf);
    video_inuse = atoi(buf);
    property_get("media.video.waitnumber", value, "200");
    wait_number = atoi(value);
    tmp_read = wait_number;
    LOGD("start CTvin video in_use = %d,wait_number=%d\n", video_inuse,wait_number);

    if (video_inuse != 0) {
        while (video_inuse && wait_number) {
            tvReadSysfs( SYS_VIDEO_INUSE_PATH, buf);
            video_inuse = atoi(buf);
            wait_number--;
            LOGD("waiting resource =%d\n", wait_number);
            usleep (50*1000);
        }
    }
    if (wait_number == 0 && video_inuse != 0) {
        LOGD("have waited %d ms,return -1,CTvin start with video in use\n", tmp_read*50);
        return false;
    } else {
        return true;
    }
}

static pthread_mutex_t ukey_write_mutex = PTHREAD_MUTEX_INITIALIZER;

static const char *GetKeyDataWriteDevPath() {
    return "/sys/class/unifykeys/write";
}

static const char *GetKeyDataReadDevPath() {
    return "/sys/class/unifykeys/read";
}

static int checkKeyNameInList(const char *key_name) {
    FILE *dev_fp = NULL;
    char *tmp_ptr = NULL;
    char lineStr[1024];

    dev_fp = fopen(CS_KEY_DATA_LIST_DEV_PATH, "r");
    if (dev_fp == NULL) {
        ALOGE("%s, open %s ERROR(%s)!!\n", __FUNCTION__,
                CS_KEY_DATA_LIST_DEV_PATH, strerror(errno));
        return -1;
    }

    while (fgets(lineStr, 1024, dev_fp) != NULL) {
        tmp_ptr = strstr(lineStr, key_name);
        if (tmp_ptr != NULL) {
            break;
        }

        tmp_ptr = NULL;
    }

    fclose(dev_fp);
    dev_fp = NULL;

    if (tmp_ptr == NULL) {
        return -1;
    }

    return 0;
}

int readUKeyData(const char *key_name, unsigned char data_buf[], int rd_size __unused) {
    int rd_cnt = 0;
    FILE *dev_fp = NULL;

    if (checkKeyNameInList(key_name) < 0) {
        LOGD("%s, key \"%s\" isn't exist in unifykeys list\n", __FUNCTION__, key_name);
        return -1;
    }

    dev_fp = fopen(CS_KEY_DATA_NAME_DEV_PATH, "w");
    if (dev_fp == NULL) {
        LOGD("%s, open %s ERROR(%s)!!\n", __FUNCTION__,
                CS_KEY_DATA_NAME_DEV_PATH, strerror(errno));
        return -1;
    }

    fprintf(dev_fp, "%s", key_name);

    fclose(dev_fp);
    dev_fp = NULL;

    int mode = 1;

    if (mode == 0) {
        dev_fp = fopen(GetKeyDataReadDevPath(), "r");
        if (dev_fp == NULL) {
            LOGD("%s, open %s ERROR(%s)!!\n", __FUNCTION__,
                    GetKeyDataReadDevPath(), strerror(errno));
            return -1;
        }

        fscanf(dev_fp, "%s", data_buf);
        rd_cnt = strlen((char *) data_buf);
    } else {
        dev_fp = fopen(GetKeyDataReadDevPath(), "rb");
        if (dev_fp == NULL) {
            LOGD("%s, open %s ERROR(%s)!!\n", __FUNCTION__,
                    GetKeyDataReadDevPath(), strerror(errno));
            return -1;
        }

        rd_cnt = fread(data_buf, 1, CC_ONE_SECTION_SIZE, dev_fp);
    }

    fclose(dev_fp);
    dev_fp = NULL;

    return rd_cnt;
}

int writeUKeyData(const char *key_name, unsigned char data_buf[], int wr_size) {
    int wr_cnt = 0;
    int dev_fd = -1;
    FILE *dev_fp = NULL;

    if (checkKeyNameInList(key_name) < 0) {
        LOGD("%s, key \"%s\" isn't exist in unifykeys list\n", __FUNCTION__, key_name);
        return -1;
    }

    pthread_mutex_lock(&ukey_write_mutex);
    dev_fp = fopen(CS_KEY_DATA_NAME_DEV_PATH, "w");
    if (dev_fp == NULL) {
        LOGD("%s, open %s ERROR(%s)!!\n", __FUNCTION__,
                CS_KEY_DATA_NAME_DEV_PATH, strerror(errno));
        pthread_mutex_unlock(&ukey_write_mutex);
        return -1;
    }

    fprintf(dev_fp, "%s", key_name);

    fclose(dev_fp);
    dev_fp = NULL;

    dev_fd = open(GetKeyDataWriteDevPath(), O_WRONLY | O_SYNC | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);

    if (dev_fd < 0) {
        LOGD("%s, open %s ERROR(%s)!!\n", __FUNCTION__,
                GetKeyDataWriteDevPath(), strerror(errno));
        pthread_mutex_unlock(&ukey_write_mutex);
        return -1;
    }

    wr_cnt = write(dev_fd, data_buf, wr_size);

    fsync(dev_fd);

    close(dev_fd);
    dev_fd = -1;
    pthread_mutex_unlock(&ukey_write_mutex);

    return wr_cnt;
}

//spi interface start
#define SPI_DEVICE "/dev/mtd/mtd0"
int tvSpiOpen(void) {
    int fd = -1;
    fd = open(SPI_DEVICE, O_RDWR);
    if (fd < 0) {
        LOGD("%s: spi device is not!\n", __FUNCTION__);
    return -1;
    }

    return fd;
}
int tvSpiRead(unsigned int offset, unsigned int ulength, unsigned char *pubuf) {
    int err = -1;
    int fd = tvSpiOpen();

    if (fd < 0) {
        LOGE("%s:fd < 0!\n", __FUNCTION__);
        return -1;
    }
    if (NULL == pubuf) {
        LOGE("%s:pbuf is NULL!\n", __FUNCTION__);
        return -1;
    }
    if (offset != (unsigned int)lseek (fd, offset, SEEK_SET)) {
        LOGE("%s lseek error(%s)!\n", __FUNCTION__, strerror(errno));
        return -1;
    }

    err = read (fd, pubuf, ulength);
    if (err < 0) {
        LOGE("%s read error(%s)!\n", __FUNCTION__, strerror(errno));
        return -1;
    }

    LOGD("%s:read success!\n", __FUNCTION__);
    close(fd);
    return err;

}

int tvSpiRead(int fd, unsigned int offset, unsigned int ulength, unsigned char *pubuf) {
    int err = -1;

    if (fd < 0) {
        LOGE("%s:fd < 0!\n", __FUNCTION__);
        return -1;
    }
    if (NULL == pubuf) {
        LOGE("%s:pbuf is NULL!\n", __FUNCTION__);
        return -1;
    }
    if (offset != (unsigned int)lseek (fd, offset, SEEK_SET)) {
        LOGE("%s lseek error(%s)!\n", __FUNCTION__, strerror(errno));
        return -1;
    }

    err = read (fd, pubuf, ulength);
    if (err < 0) {
        LOGE("%s read error(%s)!\n", __FUNCTION__, strerror(errno));
        return -1;
    }

    LOGD("%s:read success!\n", __FUNCTION__);
    return err;
}

int tvSpiwrite(int fd, unsigned int offset, unsigned int ulength, unsigned char *pubuf) {
    int err = -1;

    if (fd < 0) {
        LOGE("%s:fd < 0!\n", __FUNCTION__);
        return -1;
    }
    if (NULL == pubuf) {
        LOGE("%s:pbuf is NULL!\n", __FUNCTION__);
        return -1;
    }
    if (offset != (unsigned int)lseek (fd, offset, SEEK_SET)) {
        LOGE("%s lseek error(%s)!\n", __FUNCTION__, strerror(errno));
        return -1;
    }

    err = write (fd, pubuf, ulength);
    if (err < 0) {
        LOGE("%s write error(%s)!\n", __FUNCTION__, strerror(errno));
        return -1;
    }

    LOGD("%s:write success!\n", __FUNCTION__);
    return err;
}

int tvFlashRead(const char *partition_name, long offset,unsigned char *buffer, unsigned int length) {
    int fd = -1;
    if (partition_name == NULL || buffer == NULL) {
        LOGE("%s, illegal parameter, null pointer.", __FUNCTION__);
        return -1;
    }
    if (isSysfsExist(partition_name) != 0) {
        LOGE("%s, file (%s) does not exist.\n", __FUNCTION__, partition_name);
        return -1;
    }
    LOGD("%s, partition_name = %s, offset = %ld, read_length = %d", __FUNCTION__, partition_name, offset, length);
    errno = 0;
    fd = open(partition_name, O_RDONLY);
    if (fd < 0) {
        LOGE("%s, open file %s error(%s)", __FUNCTION__, partition_name, strerror(errno));
        return -1;
    }
    if (lseek(fd, offset, SEEK_SET) == -1) {
        LOGE("%s, lseek error(%s)", __FUNCTION__, strerror(errno));
        close(fd);
        return -1;
    }
    int ret = read(fd, buffer, length);
    if (ret == -1) {
        LOGE("%s, read error(%s)", __FUNCTION__, strerror(errno));
        close(fd);
        return -1;
    } else if (ret == 0) {
        LOGD("%s, end of file", __FUNCTION__);
        close(fd);
        return -1;
    } else {
        LOGD("%s, read success", __FUNCTION__);
        close(fd);
        return 0;
    }
}

int tvFlashWrite(const char *partition_name, long offset,unsigned char *buffer, unsigned int length) {
    int fd = -1;
    if (partition_name == NULL || buffer == NULL) {
        LOGE("%s, illegal parameter, null pointer.", __FUNCTION__);
        return -1;
    }
    LOGD("%s, partition_name = %s, offset = %ld, write_length = %d", __FUNCTION__, partition_name, offset, length);
    errno = 0;
    fd = open(partition_name, O_RDWR | O_SYNC | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        LOGE("%s, open file %s error(%s)", __FUNCTION__, partition_name, strerror(errno));
        return -1;
    }
    if (lseek(fd, offset, SEEK_SET) == -1) {
        LOGE("%s, lseek error(%s)", __FUNCTION__, strerror(errno));
        close(fd);
        return -1;
    }
    if (write(fd, buffer, length) == -1) {
        LOGE("%s, write error(%s)", __FUNCTION__, strerror(errno));
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

int isSysfsExist(const char *path) {
    return access(path, F_OK);
}

int getKernelMajorVersion(void)
{
    int retVal = 0;
    char buf[16] = {0};
    int readRet = readSys("/proc/version", buf, sizeof(buf)-1);
    if (readRet < 0) {
        retVal = 0;
    } else {
        retVal = (int)(buf[14] - '0');
    }
    //LOGD("%s: kernel version is %d.", __FUNCTION__, retVal);
    return retVal;
}
