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

#ifndef SYS_WRITE_H
#define SYS_WRITE_H

#define UNIFYKEY_ATTACH      "/sys/class/unifykeys/attach"
#define UNIFYKEY_NAME        "/sys/class/unifykeys/name"
#define UNIFYKEY_WRITE       "/sys/class/unifykeys/write"
#define UNIFYKEY_READ        "/sys/class/unifykeys/read"
#define UNIFYKEY_EXIST       "/sys/class/unifykeys/exist"
#define UNIFYKEY_LOCK        "/sys/class/unifykeys/lock"

#define KEYUNIFY_ATTACH      _IO('f', 0x60)
#define KEYUNIFY_GET_INFO    _IO('f', 0x62)
#define KEY_UNIFY_NAME_LEN   (48)
#define ATTESTATION_KEY_LEN  10240

#define KEYBOX_MAX_SIZE        (16 * 1024)
#define PFID_SIZE              (16)
#define DAC_SIZE               (32)

struct key_item_info_t {
    unsigned int id;
    char name[KEY_UNIFY_NAME_LEN];
    unsigned int size;
    unsigned int permit;
    unsigned int flag;      /*bit 0: 1 exsit, 0-none;*/
    unsigned int reserve;
};

typedef enum {
    DI_BYPASS_ALL = 0,
    DI_BYPASS_POST,
    DET3D_MODE_SYSFS,
    PROG_PROC_SYSFS,
    DISPLAY_HDMI_HDCP_AUTH,
    NodeIndexMax,
} ConstCharforSysNodeIndex;

class SysWrite
{
public:
    SysWrite();
    ~SysWrite();

    bool getProperty(const char *key, char *value);
    bool getPropertyString(const char *key, char *value, const char *def);
    int32_t getPropertyInt(const char *key, int32_t def);
    int64_t getPropertyLong(const char *key, int64_t def);

    bool getPropertyBoolean(const char *key, bool def);
    void setProperty(const char *key, const char *value);

    bool readSysfs(const char *path, char *value);
    bool readSysfs(ConstCharforSysNodeIndex index, char *value);
    bool readSysfsOriginal(const char *path, char *value);
    bool writeSysfs(const char *path, const char *value);
    bool writeSysfs(const char *path, const char *value, const int size);
    bool writeSysfs(ConstCharforSysNodeIndex index, const char *value);

    //key start
    bool writeUnifyKey(const char *path, const char *value);
    bool readUnifyKey(const char *path, char *value);
    //key end

    void setLogLevel(int level);
private:
    void writeSys(const char *path, const char *val);
    int writeSys(const char *path, const char *val, const int size);
    void readSys(const char *path, char *buf, int count, bool needOriginalData);
    int readSys(const char *path, char *buf, int count);

    //key start
    int readUnifyKeyfs(const char *path, char *value, int count);
    int writeUnifyKeyfs(const char *path, const char *value);
    int writePlayreadyKeyfs(const char *path, const char *value, const int size);
    bool writeNetflixKeyfs(const char *path, const char *value, const int size);
    bool writeWidevineKeyfs(const char *path, const char *value, const int size);
    int readAttestationKeyfs(const char * node, const char *name, char *value, int size);
    int writeAttestationKeyfs(const char * node, const char *name, const char *buff, const int size);
    //key end

    int getKernelReleaseVersion();
    void initConstCharforSysNode();
    void dump_keyitem_info(struct key_item_info_t *info);

    int mLogLevel;
    const char* mPathforSysNode[NodeIndexMax];

    const char *default_ext_ta_uuid = "11111111-2222-3333-4444-555555555555";
    uint32_t default_ext_key_type = 0x1001;
    uint32_t default_storage_location = 0;
    uint8_t default_dac_buf[DAC_SIZE] = { 0 };
    uint32_t default_dac_size = DAC_SIZE;
    uint8_t default_pfid_buf[PFID_SIZE] = { 0 };
    uint32_t default_id_size = PFID_SIZE;
};

#endif // SYS_WRITE_H
