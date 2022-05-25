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
 *  @author   shuai zhang
 *  @version  2.0
 *  @date     2022/03/15
 *  @par function description:
 *  - 1 write provision key in daemon
 */

#ifndef SYS_PROVISIONKEY_H
#define SYS_PROVISIONKEY_H

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

#ifndef RECOVERY_MODE

#define PROVISION_KEY_TYPE_WIDEVINE                         0x11
#define PROVISION_KEY_TYPE_PLAYREADY_PRIVATE                0x21
#define PROVISION_KEY_TYPE_PLAYREADY_PUBLIC                 0x22
#define PROVISION_KEY_TYPE_HDCP_TX14                        0x31
#define PROVISION_KEY_TYPE_HDCP_TX22                        0x32
#define PROVISION_KEY_TYPE_HDCP_RX14                        0x33
#define PROVISION_KEY_TYPE_HDCP_RX22_WFD                    0x34
#define PROVISION_KEY_TYPE_HDCP_RX22_FW                     0x35
#define PROVISION_KEY_TYPE_HDCP_RX22_FW_PRIVATE             0x36
#define PROVISION_KEY_TYPE_KEYMASTER                        0x41
#define PROVISION_KEY_TYPE_KEYMASTER_3                      0x42
#define PROVISION_KEY_TYPE_KEYMASTER_3_ATTEST_DEV_ID_BOX    0x43
#define PROVISION_KEY_TYPE_EFUSE                            0x51
#define PROVISION_KEY_TYPE_CIPLUS                           0x61
#define PROVISION_KEY_TYPE_CIPLUS_ECP                       0x62
#define PROVISION_KEY_TYPE_NAGRA_DEV_UUID                   0x71
#define PROVISION_KEY_TYPE_NAGRA_DEV_SECRET                 0x72
#define PROVISION_KEY_TYPE_PFID                             0x81
#define PROVISION_KEY_TYPE_PFPK                             0x82
#define PROVISION_KEY_TYPE_YOUTUBE_SECRET                   0x91
#define PROVISION_KEY_TYPE_NETFLIX_MGKID                    0xA2
#define PROVISION_KEY_TYPE_WIDEVINE_CAS                     0xB1
#define PROVISION_KEY_TYPE_DOLBY_ID                         0xC1
#define PROVISION_KEY_TYPE_INVALID                          0xFFFFFFFF

#endif

#define PROVISION_KEY_CHECKSUM_LENGTH                       (32)

class ProvisionKey
{
public:
    ProvisionKey();
    ~ProvisionKey();

    bool getProperty(const char *key, char *value);
    bool getPropertyString(const char *key, char *value, const char *def);
    int32_t getPropertyInt(const char *key, int32_t def);
    int64_t getPropertyLong(const char *key, int64_t def);

    bool getPropertyBoolean(const char *key, bool def);
    void setProperty(const char *key, const char *value);

    //key start
    bool writePlayreadyKey(const char *value, const int size);
    bool writeNetflixKey(const char *value, const int size);
    bool writeWidevineKey(const char *value, const int size);
    bool writeAttestationKey(const char *value, const int size);
    bool writeHDCP14Key(const char *value, const int size);
    bool writeHdcpRX14Key(const char *value, const int size);
    bool writeHDCP22Key(const char *value, const int size);
    bool writeHdcpRX22Key(const char *value, const int size);
    bool writePFIDKey(const char *value, const int size);
    bool writePFPKKey(const char *value, const int size);

    bool readPlayreadyKey(const char *path, const uint32_t key_type, int size);
    bool readNetflixKey(const uint32_t key_type, int size);
    bool readWidevineKey(const uint32_t key_type, int size);
    bool readAttestationKey(const uint32_t key_type, int size);
    bool readHDCP14Key(const uint32_t key_type, int size);
    bool readHdcpRX14Key(const uint32_t key_type, int size);
    bool readHDCP22Key(const uint32_t key_type, int size);
    bool readHdcpRX22Key(const uint32_t key_type, int size);

    bool checkPlayreadyKey(const char *path, const char *value, const uint32_t key_type);
    bool checkNetflixKey(const char *value, const uint32_t key_type);
    bool checkWidevineKey(const char *value, const uint32_t key_type);
    bool checkAttestationKey(const char *value, const uint32_t key_type);
    bool checkHDCP14Key(const char *value, const uint32_t key_type);
    bool checkHDCP14KeyIsExist(const uint32_t key_type);
    bool checkHDCP22Key(const char *path, const char *value, const uint32_t key_type);
    bool checkHDCP22KeyIsExist(const uint32_t key_type_first, const uint32_t key_type_second);
    bool checkPFIDKeyIsExist(const uint32_t key_type);
    bool checkPFPKKeyIsExist(const uint32_t key_type);
    bool calcChecksumKey(const char *value, const int size, char *keyCheckSum);
    //key end

    void setLogLevel(int level);
private:

    //key start
    bool keyProvisionStore(const char *value, const int size);
    bool keyProvisionQuery(const uint32_t key_type, const int size);
    bool keyProvisionChecksum (const uint32_t key_type, const char *value);
    bool keyProvisionDelete (const uint32_t key_type);
    bool keyProvisionQueryV2 (const uint32_t ext_key_type, const int size);
    bool keyProvisionChecksumV2 (const char *value, const uint32_t ext_key_type);
    bool keyProvisionDeleteV2 (const uint32_t ext_key_type);
    bool keyProvisionGetPfid ();
    bool keyProvisionGetDac ();
    bool keyProvisionCalcChecksum (const char *value, const int size, char *keyCheckSum);
    //key end

    int getKernelReleaseVersion();

    int mLogLevel;

    const char *default_ext_ta_uuid = "11111111-2222-3333-4444-555555555555";
    uint32_t default_ext_key_type = 0x1001;
    uint32_t default_storage_location = 0;
    uint8_t default_dac_buf[DAC_SIZE] = { 0 };
    uint32_t default_dac_size = DAC_SIZE;
    uint8_t default_pfid_buf[PFID_SIZE] = { 0 };
    uint32_t default_id_size = PFID_SIZE;
};

#endif // SYS_PROVISIONKEY_H
