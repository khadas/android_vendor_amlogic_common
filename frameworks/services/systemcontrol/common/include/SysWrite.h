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

#define HDMI_OUTPUT_CHECK_PATH    "/sys/class/amhdmitx"    //if this dir exist,is hdmi output

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
    unsigned int flag;      /*bit 0: 1 exist, 0-none;*/
    unsigned int reserve;
};

typedef enum {
    DI_BYPASS_ALL = 0,
    DI_BYPASS_POST,
    DET3D_MODE_SYSFS,
    PROG_PROC_SYSFS,
    DISPLAY_HDMI_HDCP_AUTH,
    VIDEO_POLL_STATUS_CHANGE,
    VIDEO_POLL_PRIMARY_SRC_FMT,
    VIDEO_CROP,
    VIDEO_SCREEN_MODE,
    VIDEO_SCREEN_MODE_PIP,
    VIDEO_NONLINEAR_FACTOR,
    VIDEO_RGB_SCREEN,
    VIDEO_TEST_SCREEN,
    VIDEO_FRAME_HEIGHT,
    VIDEO_SR_ENABLE,
    VIDEO_AISR_ENABLE,
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
    AIPQ_PARAMETERS_UVM_OPEN,
    AISR_PARAMETERS_UVM_OPEN_NN,
    DECODER_COMMON_PARAMETERS_DEBUG_VDETECT,
    VIDEO_BACKGROUND_COLOR,
    VIDEO_BLACKOUT_POLICY,
    VIDEO_DISABLE_VIDEO,
    VDIN_SNOW_FLAG,
    VPP_AFD_MODULE_ASPECT_MODE,
    SYSFS_BOOT_TYPE,
    SYS_DISPLAY_RESOLUTION,
    DISPLAY_HDMI_HDCP_VER,    //RX support HDCP version
    DISPLAY_HDMI_HDCP_MODE,   //set HDCP mode
    DISPLAY_HDMI_HDCP_CONF,   //HDCP config
    DISPLAY_HDMI_HDCP_KEY,    //TX have 22 or 14 or none key
    DISPLAY_HDMI_HDCP_POWER,  //write to 1, force hdcp_tx22 quit safely
    DISPLAY_FB0_BLANK,
    DISPLAY_FB1_BLANK,
    DISPLAY_FB0_FREESCALE,
    DISPLAY_FB1_FREESCALE,
    DISPLAY_FB0_FREESCALE_AXIS,
    DISPLAY_FB0_WINDOW_AXIS,
    DISPLAY_HDMI_SYSCTRL_READY,
    DISPLAY_HPD_STATE,
    DISPLAY_HDMI_DISP_CAP,    //RX support display mode
    DISPLAY_HDMI_DISP_CAP_3D, //RX support display 3d mode
    DISPLAY_HDMI_DEEP_COLOR,  //RX support deep color
    DISPLAY_HDMI_HDR,
    DISPLAY_HDMI_HDR_CAP2,
    DISPLAY_HDMI_AUDIO,
    DISPLAY_HDMI_AUDIO_MUTE,
    DISPLAY_HDMI_VIDEO_MUTE,
    DISPLAY_MEDIA_VIDEO_MUTE,
    DISPLAY_HDMI_MODE_PREF,
    DISPLAY_HDMI_SINK_TYPE,
    DISPLAY_HDMI_USED,
    DISPLAY_HDMI_AVMUTE_SYSFS,
    DISPLAY_EDID_VALUE,
    DISPLAY_EDID_STATUS,
    DISPLAY_EDID_RAW,
    DISPLAY_HDMI_PHY,
    DISPLAY_HDMI_FRL_RATE,
    DISPLAY_HDMI_HDR_PRIORITY,
    AUDIO_DSP_DIGITAL_RAW,
    AV_HDMI_CONFIG,
    AV_HDMI_3D_SUPPORT,
    HDMI_TX_PLUG_STATE,
    HDMI_TX_SWITCH_HDR,
    AUTO_LOW_LATENCY_MODE_CAP,
    AUTO_LOW_LATENCY_MODE,
    HDMI_CONTENT_TYPE_CAP,
    HDMI_CONTENT_TYPE,
    DV_SUPPORT_INFO,
    PQ_DISPLAY_HDR_POLICY,
    AMDOLBY_VISION_HDR10_POLICY,
    VIDEO_AIFACE_ENABLE,
    AML_AUTO_NR_PARAMS,
    VIDEO_VD_PROC_STATE,
    AICOLOR_PARAMETERS_UVM_OPEN,
    PQ_MODULE_MEMC_DEMO_WIN,
    PQ_MODULE_AISR_DEMO_EN,
    PQ_MODULE_AISR_DEMO_AXIS,
    NodeIndexMax,
} ConstCharforSysNodeIndex;

class SysWrite
{
public:
    SysWrite();
    ~SysWrite();
    static SysWrite *GetInstance();

    bool getProperty(const char *key, char *value);
    bool getPropertyString(const char *key, char *value, const char *def);
    int32_t getPropertyInt(const char *key, int32_t def);
    int64_t getPropertyLong(const char *key, int64_t def);

    bool getPropertyBoolean(const char *key, bool def);
    void setProperty(const char *key, const char *value);

    bool readSysfs(const char *path, char *value);
    bool readSysfs(ConstCharforSysNodeIndex index, char *value);
    int readSysfs(ConstCharforSysNodeIndex index, char *buf, int count);
    bool readSysfsOriginal(const char *path, char *value);
    bool readSysfsOriginal(ConstCharforSysNodeIndex index, char *value);
    int readSysfsOriginal(ConstCharforSysNodeIndex index, char *value, int count);
    bool writeValidMode(const char *path, const char *outputmode);
    bool writeSysfs(const char *path, const char *value);
    bool writeSysfs(const char *path, const char *value, const int size);
    int writeSysfs(ConstCharforSysNodeIndex index, const char *value);

    //key start
    bool writeUnifyKey(const char *path, const char *value);
    bool readUnifyKey(const char *path, char *value);
    //key end

    void setLogLevel(int level);
    const char *getSysNode(ConstCharforSysNodeIndex index);
private:
    static SysWrite *mInstance;
    int writeSys(const char *path, const char *val);
    int writeSys(const char *path, const char *val, const int size);
    int readSys(const char *path, char *buf, int count, bool needOriginalData);
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
