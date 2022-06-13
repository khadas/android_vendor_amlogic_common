/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef CONFIG_FILE_H_
#define CONFIG_FILE_H_

//pq file code dir path
#define PQ_DB_DEFAULT_PATH_0                "/mnt/vendor/odm_ext/etc/tvconfig/pq/pq.db"
#define OVERSCAN_DB_DEFAULT_PATH_0          "/mnt/vendor/odm_ext/etc/tvconfig/pq/overscan.db"
#define PQ_CONFIG_DEFAULT_PATH_0            "/mnt/vendor/odm_ext/etc/tvconfig/pq/pq_default.ini"
#define DOLBY_BIN_FILE_DEFAULT_PATH_0       "/mnt/vendor/odm_ext/etc/tvconfig/panel/dv_config.bin"
#define DOLBY_CFG_FILE_DEFAULT_PATH_0       "/mnt/vendor/odm_ext/etc/tvconfig/panel/Amlogic_dv.cfg"

#define PQ_DB_DEFAULT_PATH_1                "/odm/etc/tvconfig/pq/pq.db"
#define OVERSCAN_DB_DEFAULT_PATH_1          "/odm/etc/tvconfig/pq/overscan.db"
#define PQ_CONFIG_DEFAULT_PATH_1            "/odm/etc/tvconfig/pq/pq_default.ini"
#define DOLBY_BIN_FILE_DEFAULT_PATH_1       "/odm/etc/tvconfig/panel/dv_config.bin"
#define DOLBY_CFG_FILE_DEFAULT_PATH_1       "/odm/etc/tvconfig/panel/Amlogic_dv.cfg"

//pq file running dir path
#define PARAM_PQ_DB_PATH                    "/mnt/vendor/param/pq/pq.db"
#define PARAM_OVERSCAN_DB_PATH              "/mnt/vendor/param/pq/overscan.db"

#define PARAM_SSM_DATA_PATH                 "/mnt/vendor/param/pq/ssm_data"
#define PARAM_SSM_HANDLER_PATH              "/mnt/vendor/param/pq/SSMHandler"
#define WB_FILE_PATH                        "/dev/block/cri_data"

//for pq config
#define CFG_SECTION_PQ                          "PQ"
#define CFG_PQ_DB_PATH                          "pq_db_path"
#define CFG_PQ_OVERSCAN_DB_PATH                 "pq_overscan_db_path"
#define CFG_PQ_SSM_DATAT_PATH                   "pq_ssm_data_path"
#define CFG_PQ_SSM_DATA_HANDLER_PATH            "pq_ssm_data_handler_path"
#define CFG_PQ_WB_PATH                          "pq_wb_path"
#define CFG_PQ_DV_BIN_PATH                      "pq_dv_bin_path"
#define CFG_PQ_DV_CFG_PATH                      "pq_dv_cfg_path"

#define CFG_BIG_SMALL_DB_ENABLE                 "pq.BigSmallDb.en"
#define CFG_ALL_PQ_MOUDLE_ENABLE                "pq.AllPQMoudle.en"
#define CFG_PQ_PARAM_CHECK_SOURCE_ENABLE        "pq.ParamCheckSource.en"
#define CFG_TVHAL_ENABLE                        "pq.tvhal.en"
//di moudle config
#define CFG_DI_ENABLE                           "pq.di.en"
#define CFG_MCDI_ENABLE                         "pq.mcdi.en"
#define CFG_DEBLOCK_ENABLE                      "pq.deblock.en"
#define CFG_NOISEREDUCTION_ENABLE               "pq.NoiseReduction.en"
#define CFG_DEMOSQUITO_ENABLE                   "pq.DemoSquito.en"
#define CFG_SMOOTHPLUS_ENABLE                   "pq.SmoothPlus.en"
//amvecm moudle config
#define CFG_SHARPNESS0_ENABLE                   "pq.sharpness0.en"
#define CFG_SHARPNESS1_ENABLE                   "pq.sharpness1.en"
#define CFG_DNLP_ENABLE                         "pq.dnlp.en"
#define CFG_CM2_ENABLE                          "pq.cm2.en"
#define CFG_AMVECM_BASCI_ENABLE                 "pq.amvecm.basic.en"
#define CFG_CONTRAST_RGB_ENABLE                 "pq.contrast.rgb.en"
#define CFG_AMVECM_BASCI_WITHOSD_ENABLE         "pq.amvecm.basic.withOSD.en"
#define CFG_CONTRAST_RGB_WITHOSD_ENABLE         "pq.contrast.rgb.withOSD.en"
#define CFG_WHITEBALANCE_ENABLE                 "pq.whitebalance.en"
#define CFG_GAMMA_ENABLE                        "pq.gamma.en"
#define CFG_LOCAL_CONTRAST_ENABLE               "pq.LocalContrast.en"
#define CFG_BLACKEXTENSION_ENABLE               "pq.BlackExtension.en"
#define CFG_CHROMA_COR_ENABLE                   "pq.chroma.cor.en"
#define CFG_XVYCC_ENABLE                        "pq.xvycc.en"
#define CFG_HDRTMO_ENABLE                       "pq.hdrtmo.en"
#define CFG_AI_ENABLE                           "pq.ai.en"
#define CFG_MEMC_ENABLE                         "pq.memc.en"
#define CFG_AAD_ENABLE                          "pq.aad.en"
#define CFG_CABC_ENABLE                         "pq.cabc.en"
//overscan moudle config
#define CFG_DISPLAY_OVERSCAN_ENABLE             "pq.DisplayOverscan.en"

//pq param default value
#define CFG_PICTUREMODE_DEF                     "pq.PictureMode.def"
#define CFG_COLORTEMPTUREMODE_DEF               "pq.ColorTemperature.def"
#define CFG_COLORDEMOMODE_DEF                   "pq.ColorDemoMode.def"
#define CFG_COLORBASEMODE_DEF                   "pq.ColorBaseMode.def"
#define CFG_COLORSPACE_DEF                      "pq.ColorSpace.def"
#define CFG_RGBGAIN_R_DEF                       "pq.RGBGainR.def"
#define CFG_RGBGAIN_G_DEF                       "pq.RGBGainG.def"
#define CFG_RGBGAIN_B_DEF                       "pq.RGBGainB.def"
#define CFG_RGBPOSTOFFSET_R_DEF_DEF             "pq.RGBPostOffsetR.def"
#define CFG_RGBPOSTOFFSET_G_DEF_DEF             "pq.RGBPostOffsetG.def"
#define CFG_RGBPOSTOFFSET_B_DEF_DEF             "pq.RGBPostOffsetB.def"
#define CFG_GAMMALEVEL_DEF                      "pq.GammaLevel.def"
#define CFG_DISPLAYMODE_DEF                     "pq.DisplayMode.def"
#define CFG_AUTOASPECT_DEF                      "pq.AutoAspect.def"
#define CFG_43STRETCH_DEF                       "pq.43Stretch.def"
#define CFG_DNLPLEVEL_DEF                       "pq.DnlpLevel.def"
#define CFG_DNLPGAIN_DEF                        "pq.DnlpGain.def"
#define CFG_EYEPROJECTMODE_DEF                  "pq.EyeProtectionMode.def"
#define CFG_DDRSSC_DEF                          "pq.DDRSSC.def"
#define CFG_LVDSSSC_DEF                         "pq.LVDSSSC.def"
#define CFG_LOCALCONTRASTMODE_DEF               "pq.LocalContrastMode.def"
#define CFG_MEMCMODE_DEF                        "pq.MemcMode.def"
#define CFG_MEMCDEBLURLEVEL_DEF                 "pq.MemcDeblurLevel.def"
#define CFG_MEMCDEJUDDERLEVEL_DEF               "pq.MemcDeJudderLevel.def"
#define CFG_DEBLOCKMODE_DEF                     "pq.DeblockMode.def"
#define CFG_DEMOSQUITOMODE_DEF                  "pq.DemoSquitoMode.def"
#define CFG_MCDI_DEF                            "pq.McDiMode.def"


//for backlight
#define CFG_SECTION_BACKLIGHT                   "BACKLIGHT"
#define CFG_AUTOBACKLIGHT_THTF                  "auto.backlight.thtf"
#define CFG_AUTOBACKLIGHT_LUTMODE               "auto.backlight.lutmode"
#define CFG_AUTOBACKLIGHT_LUTHIGH               "auto.backlight.lutvalue_high"
#define CFG_AUTOBACKLIGHT_LUTLOW                "auto.backlight.lutvalue_low"

//For HDMI
#define CFG_SECTION_HDMI                        "HDMI"
#define CFG_EDID_VERSION_DEF                    "hdmi.edid.version.def"
#define CFG_HDCP_SWITCHER_DEF                   "hdmi.hdcp.switcher.def"
#define CFG_COLOR_RANGE_MODE_DEF                "hdmi.color.range.mode.def"
#define CFG_HDMI_OUT_WITH_FBC_ENABLE            "hdmi.out.with.fbc.en"

//For PQ Diff
#define CFG_SECTION_PQ_DIFF                     "PQ_DIFF"
#define CFG_BASE_PQ_PATH                        "pq.diff.base.pq.path"
#define CFG_DIFF_PQ_PATH                        "pq.diff.diff.pq.path"
#define CFG_TARGET_PQ_SHA1                      "pq.diff.target.pq.sha1"
#define CFG_BASE_PQ_SHA1                        "pq.diff.base.pq.sha1"

static const int MAX_CONFIG_FILE_LINE_LEN = 512;

typedef enum _LINE_TYPE {
    LINE_TYPE_SECTION = 0,
    LINE_TYPE_KEY,
    LINE_TYPE_COMMENT,
} LINE_TYPE;

typedef struct _LINE {
    LINE_TYPE type;
    char  Text[MAX_CONFIG_FILE_LINE_LEN];
    int LineLen;
    char *pKeyStart;
    char *pKeyEnd;
    char *pValueStart;
    char *pValueEnd;
    struct _LINE *pNext;
} LINE;

typedef struct _SECTION {
    LINE *pLine;
    struct _SECTION *pNext;
} SECTION;


class CConfigFile {
public:
    CConfigFile();
    ~CConfigFile();
    static CConfigFile *GetInstance();
    bool isFileExist(const char *file_name);
    void GetPqdbPath(char *file_path);
    void GetOverscandbPath(char *file_path);
    void GetSSMDataPath(char *file_path);
    void GetSSMDataHandlerPath(char *file_path);
    void GetWBFilePath(char *file_path);
    void GetDvFilePath(char *bin_file_path, char *cfg_file_path);
    int LoadFromFile(const char *filename);
    int SaveToFile(const char *filename = NULL);
    int SetString(const char *section, const char *key, const char *value);
    int SetInt(const char *section, const char *key, int value);
    int SetFloat(const char *section, const char *key, float value);
    const char *GetString(const char *section, const char *key, const char *def_value);
    int GetInt(const char *section, const char *key, int def_value);
    float GetFloat(const char *section, const char *key, float def_value);

private:
    LINE_TYPE getLineType(char *Str);
    void allTrim(char *Str);
    SECTION *getSection(const char *section);
    LINE *getKeyLineAtSec(SECTION *pSec, const char *key);
    void FreeAllMem();
    int InsertSection(SECTION *pSec);
    int InsertKeyLine(SECTION *pSec, LINE *line);

    static CConfigFile *mInstance;
    char mpFileName[256];
    FILE *mpConfigFile;
    LINE *mpFirstLine;
    SECTION *mpFirstSection;
};
#endif //end of CONFIG_FILE_H_
