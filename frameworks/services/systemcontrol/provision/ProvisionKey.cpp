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
#include <sys/utsname.h>

#include <sys/ioctl.h>
#include <sys/types.h>

#ifndef RECOVERY_MODE
//#include <../../../../provision/ca/include/provision_api.h>
#include <getopt.h>
#include <sys/stat.h>
//#include <tee_client_api.h>
//#include <ta.h>
#endif

#include "common.h"
#include "ProvisionKey.h"

ProvisionKey::ProvisionKey() {
}

ProvisionKey::~ProvisionKey() {
}

bool ProvisionKey::getProperty(const char *key, char *value){
    property_get(key, value, "");
    /*
    char buf[PROPERTY_VALUE_MAX] = {0};
    property_get(key, buf, "");
    value.setTo(String16(buf));
    */
    return true;
}

bool ProvisionKey::getPropertyString(const char *key, char *value,  const char *def){
    property_get(key, value, def);
    return true;
}

int32_t ProvisionKey::getPropertyInt(const char *key, int32_t def){
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

int64_t ProvisionKey::getPropertyLong(const char *key, int64_t def){

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

bool ProvisionKey::getPropertyBoolean(const char *key, bool def){

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

void ProvisionKey::setProperty(const char *key, const char *value){
    int err;
    err = property_set(key, value);
    if (err < 0) {
        SYS_LOGE("failed to set system property %s\n", key);
    }
}


//key start
bool ProvisionKey::writePlayreadyKey(const char *value, const int size){
    #ifndef RECOVERY_MODE
        return keyProvisionStore(value, size);
    #endif
    return false;
}

bool ProvisionKey::writeNetflixKey(const char *value, const int size){
    #ifndef RECOVERY_MODE
        return keyProvisionStore(value, size);
    #endif
    return false;
}

bool ProvisionKey::writeWidevineKey(const char *value, const int size){
    #ifndef RECOVERY_MODE
        return keyProvisionStore(value, size);
    #endif
    return false;
}

bool ProvisionKey::writeAttestationKey(const char *value, const int size) {
    #ifndef RECOVERY_MODE
        return keyProvisionStore(value, size);
    #endif
    return false;
}

bool ProvisionKey::writeHDCP14Key(const char *value, const int size){
    #ifndef RECOVERY_MODE
        return keyProvisionStore(value, size);
    #endif
    return false;
}

bool ProvisionKey::writeHdcpRX14Key(const char *value, const int size){
    #ifndef RECOVERY_MODE
        return keyProvisionStore(value, size);
    #endif
    return false;
}

bool ProvisionKey::writeHDCP22Key(const char *value, const int size){
    #ifndef RECOVERY_MODE
        return keyProvisionStore(value, size);
    #endif
    return false;
}

bool ProvisionKey::writeHdcpRX22Key(const char *value, const int size){
    #ifndef RECOVERY_MODE
        return keyProvisionStore(value, size);
    #endif
    return false;
}


bool ProvisionKey::writePFIDKey(const char *value, const int size) {
    int ret = -1;
    SYS_LOGE("come to ProvisionKey::writePFIDKey size = %d\n", size);
    #ifndef RECOVERY_MODE
        ret = keyProvisionStore(value, size);
    #endif
    SYS_LOGI("writePFIDKey ret = %d\n", ret);
    if (ret == 0)
        return true;
    else
        return false;
}

bool ProvisionKey::writePFPKKey(const char *value, const int size) {
    int ret = -1;
    SYS_LOGE("come to ProvisionKey::writePFPKKey size = %d\n", size);
    #ifndef RECOVERY_MODE
        ret = keyProvisionStore(value, size);
    #endif
    SYS_LOGI("writePFPKKey ret = %d\n", ret);
    if (ret == 0)
        return true;
    else
        return false;
}

bool ProvisionKey::readPlayreadyKey(const char *path, const uint32_t key_type, int size) {
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

bool ProvisionKey::readNetflixKey(const uint32_t key_type, int size) {
    SYS_LOGI("readNetflixKey");
    bool ret = false;
    #ifndef RECOVERY_MODE
        //int ret = readUnifyKeyfs(path, (char*)buf, size);
        ret = keyProvisionQuery(key_type,size);
    #endif
    return ret;
}

bool ProvisionKey::readWidevineKey(const uint32_t key_type, int size) {
    SYS_LOGI("readWidevineKey");
    bool ret = false;
    #ifndef RECOVERY_MODE
        //int ret = readUnifyKeyfs(path, (char*)buf, size);
        ret = keyProvisionQuery(key_type, size);
    #endif
    return ret;
}


bool ProvisionKey::readAttestationKey(const uint32_t key_type, int size) {
    bool ret = false;
    #ifndef RECOVERY_MODE
        //int ret = readAttestationKeyfs(node, name, (char*)buf, size);
        ret = keyProvisionQuery(key_type, size);
    #endif
    return ret;

}

bool ProvisionKey::readHDCP14Key(const uint32_t key_type, int size) {
    bool ret = false;
    #ifndef RECOVERY_MODE
        ret = keyProvisionQuery(key_type, size);
    #endif
    return ret;
}

bool ProvisionKey::readHdcpRX14Key(const uint32_t key_type, int size) {
    bool ret = false;
    #ifndef RECOVERY_MODE
        ret = keyProvisionQuery(key_type, size);
    #endif
    return ret;
}

bool ProvisionKey::readHDCP22Key(const uint32_t key_type, int size){
    bool ret = false;
    #ifndef RECOVERY_MODE
        ret = keyProvisionQuery(key_type, size);
    #endif
    return ret;
}

bool ProvisionKey::readHdcpRX22Key(const uint32_t key_type, int size) {
    bool ret = false;
    #ifndef RECOVERY_MODE
        ret = keyProvisionQuery(key_type, size);
    #endif
    return ret;
}


bool ProvisionKey::checkPlayreadyKey(const char *path, const char *value, const uint32_t key_type) {
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

bool ProvisionKey::checkNetflixKey(const char *value, const uint32_t key_type) {
    bool ret = false;
    SYS_LOGI("checkNetflixKey");
    #ifndef RECOVERY_MODE
        ret = keyProvisionChecksum(key_type, value);
    #endif
    SYS_LOGI("checkNetflixKey ret = %d\n", ret);
    return ret;
}

bool ProvisionKey::checkWidevineKey(const char *value, const uint32_t key_type) {
    SYS_LOGI("checkWidevineKey");
    bool ret = false;
    #ifndef RECOVERY_MODE
        ret = keyProvisionChecksum(key_type, value);
    #endif
    SYS_LOGI("checkWidevineKey ret = %d\n", ret);
    return ret;
}

bool ProvisionKey::checkAttestationKey(const char *value, const uint32_t key_type){
    bool ret = false;
    SYS_LOGI("checkAttestationKey");
    #ifndef RECOVERY_MODE
        ret = keyProvisionChecksum(key_type, value);
    #endif
    SYS_LOGI("writeAttestationKey ret = %d\n", ret);
    return ret;
}

bool ProvisionKey::checkHDCP14Key(const char *value, const uint32_t key_type) {
    bool ret = false;
    #ifndef RECOVERY_MODE
        ret = keyProvisionChecksum(key_type, value);
    #endif
    SYS_LOGI("[%s, %d] key is exist", __FUNCTION__, __LINE__);
    return ret;
}

bool ProvisionKey::checkHDCP14KeyIsExist(const uint32_t key_type) {
    bool ret = false;
    #ifndef RECOVERY_MODE
        ret = keyProvisionQuery(key_type, 0);
    #endif
    return ret;
}

bool ProvisionKey::checkHDCP22Key(const char *path, const char *value, const uint32_t key_type){
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

bool ProvisionKey::checkHDCP22KeyIsExist(const uint32_t key_type_first, const uint32_t key_type_second) {
    int ret = -1;
    #ifndef RECOVERY_MODE
        ret = keyProvisionQuery(key_type_first, 0);
    #endif
    if (0 != ret) {
        SYS_LOGE("[%s, %d] RX22_FW is not exist", __FUNCTION__, __LINE__);
        return false;
    }

    #ifndef RECOVERY_MODE
        ret = keyProvisionQuery(key_type_second, 0);
    #endif
    if (0 != ret) {
        SYS_LOGE("[%s, %d] RX22_FW_PR is not exist", __FUNCTION__, __LINE__);
        return false;
    }

    SYS_LOGI("[%s, %d] key is exist", __FUNCTION__, __LINE__);

    return true;
}

bool ProvisionKey::checkPFIDKeyIsExist(const uint32_t key_type){
    int ret = -1;
    #ifndef RECOVERY_MODE
        ret = keyProvisionQuery(key_type, 0);
    #endif
    if (ret == 0)
        return true;
    else
        return false;
}

bool ProvisionKey::checkPFPKKeyIsExist(const uint32_t key_type){
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

void ProvisionKey::setLogLevel(int level){
    mLogLevel = level;
}

bool ProvisionKey::keyProvisionStore(const char *value, const int size) {
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

bool ProvisionKey::keyProvisionQuery (const uint32_t key_type, const int size) {
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

bool ProvisionKey::keyProvisionChecksum (const uint32_t key_type, const char *value) {
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

bool ProvisionKey::keyProvisionDelete (const uint32_t key_type) {
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

bool ProvisionKey::keyProvisionQueryV2 (const uint32_t ext_key_type, const int size) {
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

bool ProvisionKey::keyProvisionChecksumV2 (const char *value, const uint32_t ext_key_type) {
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

bool ProvisionKey::keyProvisionDeleteV2 (const uint32_t ext_key_type) {
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

bool ProvisionKey::keyProvisionGetPfid () {
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

bool ProvisionKey::keyProvisionGetDac () {
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

int ProvisionKey::getKernelReleaseVersion() {
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

#if 0
status_t ProvisionKey::dump(int fd, const Vector<String16>& args){
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
