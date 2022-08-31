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

SysWrite::SysWrite()
    :mLogLevel(LOG_LEVEL_DEFAULT){
    initConstCharforSysNode();
}

SysWrite::~SysWrite() {
}

bool SysWrite::getProperty(const char *key, char *value){
    property_get(key, value, "");
    /*
    char buf[PROPERTY_VALUE_MAX] = {0};
    property_get(key, buf, "");
    value.setTo(String16(buf));
    */
    return true;
}

bool SysWrite::getPropertyString(const char *key, char *value,  const char *def){
    property_get(key, value, def);
    return true;
}

int32_t SysWrite::getPropertyInt(const char *key, int32_t def){
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

int64_t SysWrite::getPropertyLong(const char *key, int64_t def){

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

bool SysWrite::getPropertyBoolean(const char *key, bool def){

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

void SysWrite::setProperty(const char *key, const char *value){
    int err;
    err = property_set(key, value);
    if (err < 0) {
        SYS_LOGE("failed to set system property %s\n", key);
    }
}

bool SysWrite::readSysfs(const char *path, char *value){
    char buf[MAX_STR_LEN+1] = {0};
    readSys(path, (char*)buf, MAX_STR_LEN, false);
    strcpy(value, buf);
    return true;
}

bool SysWrite::readSysfs(ConstCharforSysNodeIndex index, char *value){
    readSysfs(mPathforSysNode[index], value);
    return true;
}

// get the original data from sysfs without any change.
bool SysWrite::readSysfsOriginal(const char *path, char *value){
    char buf[MAX_STR_LEN+1] = {0};
    readSys(path, (char*)buf, MAX_STR_LEN, true);
    strcpy(value, buf);
    return true;
}

bool SysWrite::writeSysfs(const char *path, const char *value){
    writeSys(path, value);
    return true;
}

bool SysWrite::writeSysfs(const char *path, const char *value, const int size){
    writeSys(path, value, size);
    return true;
}

bool SysWrite::writeSysfs(ConstCharforSysNodeIndex index, const char *value){
    writeSys(mPathforSysNode[index], value);
    return true;
}

//key start
bool SysWrite::writeUnifyKey(const char *path, const char *value){
    int ret;
    ret = writeUnifyKeyfs(path, value);
    if (ret == 0)
        return true;
    else
        return false;
}

bool SysWrite::writePlayreadyKey(const char *value, const int size){
    #ifndef RECOVERY_MODE
        return keyProvisionStore(value, size);
    #endif
    return false;
}

bool SysWrite::writeNetflixKey(const char *value, const int size){
    #ifndef RECOVERY_MODE
        return keyProvisionStore(value, size);
    #endif
    return false;
}

bool SysWrite::writeWidevineKey(const char *value, const int size){
    #ifndef RECOVERY_MODE
        return keyProvisionStore(value, size);
    #endif
    return false;
}

bool SysWrite::writeAttestationKey(const char *value, const int size) {
    #ifndef RECOVERY_MODE
        return keyProvisionStore(value, size);
    #endif
    return false;
}

bool SysWrite::writeHDCP14Key(const char *value, const int size){
    bool ret = false;
    ret = writeHDCP14Keyfs(value, size);
    if (ret) {
        system("/vendor/bin/tee_hdcp");
        writeSysfs("/sys/class/hdmirx/hdmirx0/debug", "load14key");
        return true;
    } else {
        return false;
    }
}

bool SysWrite::writeHDCP22Key(const char *value, const int size){
    return writeHDCP22Keyfs(value, size);
}

bool SysWrite::writePFIDKey(const char *value, const int size) {
    int ret = -1;
    SYS_LOGE("come to SysWrite::writePFIDKey size = %d\n", size);
    #ifndef RECOVERY_MODE
        ret = keyProvisionStore(value, size);
    #endif
    SYS_LOGI("writePFIDKey ret = %d\n", ret);
    if (ret == 0)
        return true;
    else
        return false;
}

bool SysWrite::writePFPKKey(const char *value, const int size) {
    int ret = -1;
    SYS_LOGE("come to SysWrite::writePFPKKey size = %d\n", size);
    #ifndef RECOVERY_MODE
        ret = keyProvisionStore(value, size);
    #endif
    SYS_LOGI("writePFPKKey ret = %d\n", ret);
    if (ret == 0)
        return true;
    else
        return false;
}


bool SysWrite::readUnifyKey(const char *path, char *value) {
    char buf[MAX_STR_LEN+1] = {0};
    int ret = readUnifyKeyfs(path, (char*)buf, MAX_STR_LEN);
    if (ret >= 1) {
        strcpy(value, buf);
        SYS_LOGI("readUnifyKey, value: %s", value);
        return true;
    } else
        return false;
}

bool SysWrite::readPlayreadyKey(const char *path, const uint32_t key_type, int size) {
    SYS_LOGI("readPlayreadyKey");
    bool ret = false;
    //int ret = readUnifyKeyfs(path, (char*)buf, size);
    #ifndef RECOVERY_MODE
        if (((strncmp("prpubkeybox", path, 12) == 0) && (PROVISION_KEY_TYPE_PLAYREADY_PUBLIC == key_type)) || 
            ((strncmp("prprivkeybox", path, 12)==0) && (PROVISION_KEY_TYPE_PLAYREADY_PRIVATE == key_type))) {
                SYS_LOGI("readPlayreadyKey key_type ERROR");
                return false;
        }
        ret = keyProvisionQuery(key_type,size);
    #endif
    return ret;
}

bool SysWrite::readNetflixKey(const uint32_t key_type, int size) {
    SYS_LOGI("readNetflixKey");
    bool ret = false;
    #ifndef RECOVERY_MODE
        //int ret = readUnifyKeyfs(path, (char*)buf, size);
        ret = keyProvisionQuery(key_type,size);
    #endif
    return ret;
}

bool SysWrite::readWidevineKey(const uint32_t key_type, int size) {
    SYS_LOGI("readWidevineKey");
    bool ret = false;
    #ifndef RECOVERY_MODE
        //int ret = readUnifyKeyfs(path, (char*)buf, size);
        ret = keyProvisionQuery(key_type, size);
    #endif
    return ret;
}


bool SysWrite::readAttestationKey(const uint32_t key_type, int size) {
    bool ret = false;
    #ifndef RECOVERY_MODE
        //int ret = readAttestationKeyfs(node, name, (char*)buf, size);
        ret = keyProvisionQuery(key_type, size);
    #endif
    return ret;

}

bool SysWrite::readHDCP14Key(const uint32_t key_type, int size) {
    SYS_LOGE("[%s, %d] not support", __FUNCTION__, __LINE__);
    return false;
}

bool SysWrite::readHDCP22Key(const uint32_t key_type, int size){
    SYS_LOGE("[%s, %d] not support", __FUNCTION__, __LINE__);
    return false;
}

bool SysWrite::checkPlayreadyKey(const char *path, const char *value, const uint32_t key_type) {
    bool ret = false;
    SYS_LOGI("checkPlayreadyKey");
    #ifndef RECOVERY_MODE
        if (((strncmp("prpubkeybox", path, 12) == 0) && (PROVISION_KEY_TYPE_PLAYREADY_PUBLIC == key_type)) ||
            ((strncmp("prprivkeybox", path, 12)==0) && (PROVISION_KEY_TYPE_PLAYREADY_PRIVATE == key_type))) {
            SYS_LOGI("checkPlayreadyKey key_type ERROR");
            return false;
        }
        ret = keyProvisionChecksum(key_type, value);
    #endif
    SYS_LOGI("checkPlayreadyKey ret = %d\n", ret);
    return ret;
}

bool SysWrite::checkNetflixKey(const char *value, const uint32_t key_type) {
    bool ret = false;
    SYS_LOGI("checkNetflixKey");
    #ifndef RECOVERY_MODE
        ret = keyProvisionChecksum(key_type, value);
    #endif
    SYS_LOGI("checkNetflixKey ret = %d\n", ret);
    return ret;
}

bool SysWrite::checkWidevineKey(const char *value, const uint32_t key_type) {
    SYS_LOGI("checkWidevineKey");
    bool ret = false;
    #ifndef RECOVERY_MODE
        ret = keyProvisionChecksum(key_type, value);
    #endif
    SYS_LOGI("checkWidevineKey ret = %d\n", ret);
    return ret;
}

bool SysWrite::checkAttestationKey(const char *value, const uint32_t key_type){
    bool ret = false;
    SYS_LOGI("checkAttestationKey");
    #ifndef RECOVERY_MODE
        ret = keyProvisionChecksum(key_type, value);
    #endif
    SYS_LOGI("writeAttestationKey ret = %d\n", ret);
    return ret;
}

bool SysWrite::checkHDCP14Key(const char *value, const uint32_t key_type) {
    bool ret = false;
    #ifndef RECOVERY_MODE
        ret = keyProvisionChecksum(key_type, value);
    #endif
    SYS_LOGI("[%s, %d] key is exist", __FUNCTION__, __LINE__);
    return ret;
}

bool SysWrite::checkHDCP14KeyIsExist(const uint32_t key_type) {
    bool ret = false;
    #ifndef RECOVERY_MODE
        ret = keyProvisionQuery(key_type, 0);
    #endif
    return ret;
}

bool SysWrite::checkHDCP22Key(const char *path, const char *value, const uint32_t key_type){
    bool ret = false;
    SYS_LOGI("checkHDCP22Key");
    #ifndef RECOVERY_MODE
        if (((strncmp("hdcp2_rx_fw", path, 11) == 0) && (PROVISION_KEY_TYPE_HDCP_RX22_FW == key_type)) || 
            ((strncmp("hdcp22_rx_private", path, 17)==0) && (PROVISION_KEY_TYPE_HDCP_RX22_FW_PRIVATE == key_type)) || 
            ((strncmp("hdcp22_rx", path, 8)==0) && (PROVISION_KEY_TYPE_HDCP_RX22_WFD == key_type))) {
            SYS_LOGI("checkHDCP22Key key_type ERROR");
            return false;
        }
        ret = keyProvisionChecksum(key_type, value);
    #endif
    SYS_LOGI("checkHDCP22Key ret = %d\n", ret);
    return ret;
}

bool SysWrite::checkHDCP22KeyIsExist(const uint32_t key_type_first, const uint32_t key_type_second) {
    int ret = -1;
    #ifndef RECOVERY_MODE
        ret = keyProvisionQuery(key_type_first, 0);
    #endif
    if(0 != ret) {
        SYS_LOGE("[%s, %d] RX22_FW is not exist", __FUNCTION__, __LINE__);
        return false;
    }

    #ifndef RECOVERY_MODE
        ret = keyProvisionQuery(key_type_second, 0);
    #endif
    if(0 != ret) {
        SYS_LOGE("[%s, %d] RX22_FW_PR is not exist", __FUNCTION__, __LINE__);
        return false;
    }

    SYS_LOGI("[%s, %d] key is exist", __FUNCTION__, __LINE__);

    return true;
}

bool SysWrite::checkPFIDKeyIsExist(const uint32_t key_type){
    int ret = -1;
    #ifndef RECOVERY_MODE
        ret = keyProvisionQuery(key_type, 0);
    #endif
    if (ret == 0)
        return true;
    else
        return false;
}

bool SysWrite::checkPFPKKeyIsExist(const uint32_t key_type){
    int ret = -1;
    #ifndef RECOVERY_MODE
        ret = keyProvisionQuery(key_type, 0);
    #endif
    if (ret == 0)
        return true;
    else
        return false;
}
//key end

void SysWrite::setLogLevel(int level){
    mLogLevel = level;
}

void SysWrite::writeSys(const char *path, const char *val){
    int fd;

    if((fd = open(path, O_RDWR)) < 0) {
        SYS_LOGE("writeSysFs, open %s fail.", path);
        goto exit;
    }

    if (mLogLevel > LOG_LEVEL_1)
        SYS_LOGI("write %s, val:%s\n", path, val);
    write(fd, val, strlen(val));

exit:
    close(fd);
}

int SysWrite::writeSys(const char *path, const char *val, const int size){
    int fd;

    if (mLogLevel > LOG_LEVEL_1)
        SYS_LOGI("writeSysFs, size = %d \n", size);

    if ((fd = open(path, O_WRONLY)) < 0) {
        SYS_LOGE("writeSysFs, open %s fail.", path);
        return -1;
    }

    if (write(fd, val, size) != size) {
        SYS_LOGE("write %s size:%d failed!\n", path, size);
        return -1;
    }

    close(fd);
    return 0;
}

bool SysWrite::keyProvisionStore(const char *value, const int size) {
    SYS_LOGI("keyProvisionStore keybox: %s key_size: %d\n", value,size);
    int ret = -1;
    #ifndef RECOVERY_MODE
        //ret = key_provision_store(NULL, 0, (uint8_t*)value, (uint32_t)size);
    #endif
    if (ret == 0)
        return true;
    else
        return false;
}

bool SysWrite::keyProvisionQuery (const uint32_t key_type, const int size) {
    uint32_t key_size = size;
    int ret = -1;
    SYS_LOGI("keyProvisionQuery key_type: %d default_storage_location: %d key_size: %d\n", key_type,default_storage_location,size);
    #ifndef RECOVERY_MODE
        //ret = key_provision_query(NULL, 0, key_type, &default_storage_location, &key_size);
    #endif
    if (ret == 0)
        return true;
    else
        return false;
}

bool SysWrite::keyProvisionChecksum (const uint32_t key_type, const char *value) {
    SYS_LOGI("keyProvisionChecksum key_type: %d checksum: %s \n", key_type,value);
    int ret = -1;
    #ifndef RECOVERY_MODE
        //ret = key_provision_checksum(key_type, NULL, 0, (uint8_t*)value);
    #endif
    if (ret == 0)
        return true;
    else
        return false;
}

bool SysWrite::keyProvisionDelete (const uint32_t key_type) {
    int ret = -1;
    #ifndef RECOVERY_MODE
        SYS_LOGI("keyProvisionDelete key_type: %d \n", key_type);
        //ret = key_provision_delete(key_type, NULL);
    #endif
    if (ret == 0)
        return true;
    else
        return false;
}

bool SysWrite::keyProvisionQueryV2 (const uint32_t ext_key_type, const int size) {
    uint32_t key_size = size;
    int ret = -1;
    SYS_LOGI("keyProvisionQueryV2 ext_key_type: %d default_storage_location: %d key_size: %d\n", ext_key_type,default_storage_location,size);
    #ifndef RECOVERY_MODE
        //ret = key_provision_query_v2(NULL, 0, ext_key_type, (uint8_t *)default_ext_ta_uuid, &default_storage_location, &key_size);
    #endif
    if (ret == 0)
        return true;
    else
        return false;
}

bool SysWrite::keyProvisionChecksumV2 (const char *value, const uint32_t ext_key_type) {
    int ret = -1;
    #ifndef RECOVERY_MODE
        SYS_LOGI("keyProvisionChecksumV2 ext_key_type: %d checksum: %s \n", ext_key_type,value);
        //ret = key_provision_checksum_v2(ext_key_type, NULL, 0, (uint8_t *)default_ext_ta_uuid, (uint8_t*)value);
    #endif
    if (ret == 0)
        return true;
    else
        return false;
}

bool SysWrite::keyProvisionDeleteV2 (const uint32_t ext_key_type) {
    int ret = -1;
    #ifndef RECOVERY_MODE
        SYS_LOGI("keyProvisionDeleteV2 ext_key_type: %d \n", ext_key_type);
        //ret = key_provision_delete(ext_key_type, (uint8_t *)default_ext_ta_uuid); // delete key
    #endif
    if (ret == 0)
        return true;
    else
        return false;
}

bool SysWrite::keyProvisionGetPfid () {
    int ret = -1;
    SYS_LOGI("keyProvisionGetPfid default_pfid_buf: %s default_pfid_buf: %d \n", default_pfid_buf,default_id_size);
    #ifndef RECOVERY_MODE
        //ret = key_provision_get_pfid(default_pfid_buf, &default_id_size);
    #endif
    if (ret == 0)
        return true;
    else
        return false;
}

bool SysWrite::keyProvisionGetDac () {
    int ret = -1;
    SYS_LOGI("keyProvisionGetDac default_dac_buf: %s default_dac_size: %d \n", default_dac_buf,default_dac_size);
    #ifndef RECOVERY_MODE
        //ret = key_provision_get_dac(default_dac_buf, &default_dac_size);
    #endif
    if (ret == 0)
        return true;
    else
        return false;
}


int SysWrite::readUnifyKeyfs(const char *path, char *value, int count) {
    int keyLen = 0;
    char existKey[10] = {0};

    writeSys(UNIFYKEY_ATTACH, "1");
    writeSys(UNIFYKEY_NAME, path);

    readSys(UNIFYKEY_EXIST, (char*)existKey, 10);
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
    char existKey[10] = {0};
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

    usleep(100*1000);

    readSys(UNIFYKEY_EXIST, (char*)existKey, 10);
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
    char existKey[10] = {0};
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

    usleep(100*1000);

    readSys(UNIFYKEY_EXIST, (char*)existKey, 10);
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
    char existKey[10] = {0};
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

    usleep(100*1000);

    readSys(UNIFYKEY_EXIST, (char*)existKey, 10);
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
    char existKey[10] = {0};
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

    usleep(100*1000);

    readSys(UNIFYKEY_EXIST, (char*)existKey, 10);
    if (0 == strcmp(existKey, "0")) {
        SYS_LOGE("do not write key to the storage");
        writeSys(UNIFYKEY_LOCK, "0");
        return -1;
    }
    SYS_LOGI("unify key write success\n");

    writeSys(UNIFYKEY_LOCK, "0");

    return ret;
}

int SysWrite::writeHDCP14Keyfs(const char *value, const int size) {
    SYS_LOGI("[%s, %d] size:%d", __FUNCTION__, __LINE__, size);
    return keyProvisionStore(value, size);
}

int SysWrite::writeHDCP22Keyfs(const char *value, const int size) {
    SYS_LOGI("[%s, %d] size:%d", __FUNCTION__, __LINE__, size);
    return keyProvisionStore(value, size);
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
    SYS_LOGI("%s() %d: key ioctl  KEYUNIFY_GET_INFO is %d\n", __func__, __LINE__, ret);
    if (ret < 0) {
        close(fp);
        return ret;
    }
    ppos = key_item_info.id;
    ret = lseek(fp, ppos, SEEK_SET);
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
    lseek(fp, ppos, SEEK_SET);
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


void SysWrite::readSys(const char *path, char *buf, int count, bool needOriginalData){
    int fd, len;

    if ( NULL == buf ) {
        SYS_LOGE("buf is NULL");
        return;
    }

    if ((fd = open(path, O_RDONLY)) < 0) {
        SYS_LOGE("readSysFs, open %s fail. Error info [%s]", path, strerror(errno));
        goto exit;
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

                if (mLogLevel > LOG_LEVEL_1)
                    SYS_LOGI("read buffer index:%d is a 0x0, replace to spacing \n", i);
            }

            /* delete all the character of '\n' */
            if (0x0a != buf[i]) {
                buf[j++] = buf[i];
            }
        }

        buf[j] = 0x0;
    }

    if (mLogLevel > LOG_LEVEL_1)
        SYS_LOGI("read %s, result length:%d, val:%s\n", path, len, buf);

exit:
    close(fd);
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
    if(reVersion == 5) {
        mPathforSysNode[DI_BYPASS_ALL] = "/sys/module/aml_media/parameters/bypass_all";
        mPathforSysNode[DI_BYPASS_POST] = "/sys/module/aml_media/parameters/bypass_post";
        mPathforSysNode[DET3D_MODE_SYSFS] = "/sys/module/aml_media/parameters/det3d_mode";
        mPathforSysNode[PROG_PROC_SYSFS] = "/sys/module/aml_media/parameters/prog_proc_config";
        mPathforSysNode[DISPLAY_HDMI_HDCP_AUTH] = "/sys/module/aml_media/parameters/hdmi_authenticated";
    } else {
        mPathforSysNode[DI_BYPASS_ALL] = "/sys/module/di/parameters/bypass_all";
        mPathforSysNode[DI_BYPASS_POST] = "/sys/module/di/parameters/bypass_post";
        mPathforSysNode[DET3D_MODE_SYSFS] = "/sys/module/di/parameters/det3d_mode";
        mPathforSysNode[PROG_PROC_SYSFS] = "/sys/module/di/parameters/prog_proc_config";
        mPathforSysNode[DISPLAY_HDMI_HDCP_AUTH] = "/sys/module/hdmitx20/parameters/hdmi_authenticated";
    }
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
