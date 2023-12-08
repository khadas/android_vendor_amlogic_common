/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef CPQDB_H_
#define CPQDB_H_

#include <cutils/properties.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "CSqlite.h"
#include "PQType.h"

using namespace android;

//pq db table name
#define PQ_DB_HDRTMO_TABLE_NAME        "HDR_HDMI_NODE"
#define PQ_DB_CABC_TABLE_NAME          "CABC_AAD_HDMI"

typedef enum initial_type_e {
    TYPE_PMode = 0,
    TYPE_PMode_Default,
    TYPE_Nonlinear,
    TYPE_NonLinear_Default,
    TYPE_VGA_AUTO,
    TYPE_OVERSCAN,
} initial_type_t;

typedef enum output_type_e {
    OUTPUT_TYPE_LVDS = -1,
    OUTPUT_TYPE_PAL,
    OUTPUT_TYPE_NTSC,

    OUTPUT_TYPE_HDMI_4K = 10,
    OUTPUT_TYPE_HDMI_HD_UPSCALE,
    OUTPUT_TYPE_HDMI_SD_UPSCALE,
    OUTPUT_TYPE_HDMI_NOSCALE,
    OUTPUT_TYPE_HDMI_SD_4096,
    OUTPUT_TYPE_HDMI_HD_4096,

    OUTPUT_TYPE_HDMI_1080_1080 = 21, //1080 to 1080
    OUTPUT_TYPE_HDMI_720_720,
    OUTPUT_TYPE_HDMI_576_576,
    OUTPUT_TYPE_HDMI_480_480,
    OUTPUT_TYPE_HDMI_720_1080,
    OUTPUT_TYPE_HDMI_576_1080,
    OUTPUT_TYPE_HDMI_480_1080,
    OUTPUT_TYPE_HDMI_576_720,
    OUTPUT_TYPE_HDMI_480_720,

    OUTPUT_TYPE_HDMI_4K_HDR = 30,
    OUTPUT_TYPE_HDMI_1080_1080_HDR,
    OUTPUT_TYPE_HDMI_720_720_HDR,
    OUTPUT_TYPE_HDMI_576_576_HDR,
    OUTPUT_TYPE_HDMI_480_480_HDR,
    OUTPUT_TYPE_HDMI_720_1080_HDR,
    OUTPUT_TYPE_HDMI_576_1080_HDR,
    OUTPUT_TYPE_HDMI_480_1080_HDR,
    OUTPUT_TYPE_HDMI_576_720_HDR,
    OUTPUT_TYPE_HDMI_480_720_HDR,
    OUTPUT_TYPE_HDMI_NOSCALE_HDR,

    OUTPUT_TYPE_HDMI_4K_4K120 = 41,
    OUTPUT_TYPE_HDMI_1080_4K120,
    OUTPUT_TYPE_HDMI_720_4K120,
    OUTPUT_TYPE_HDMI_576_4K120,
    OUTPUT_TYPE_HDMI_480_4K120,

    OUTPUT_TYPE_HDMI_4K_4K120_HDR,
    OUTPUT_TYPE_HDMI_1080_4K120_HDR,
    OUTPUT_TYPE_HDMI_720_4K120_HDR,
    OUTPUT_TYPE_HDMI_576_4K120_HDR,
    OUTPUT_TYPE_HDMI_480_4K120_HDR,

    OUTPUT_TYPE_MAX,
} output_type_t;

typedef enum resolution_height_type_e {
    SD_HEIGHT_480 = 0, //input
    SD_HEIGHT_576, //input
    HD_HEIGHT_720, //input + output
    FHD_HEIGHT_1080, //input + output
    UHD_HEIGHT_2160, //input + output
    UHD_HEIGHT_4320, //input + output
    RESOLUTION_MAX,
} resolution_height_type_t;

typedef enum tvout_with_io_table_type_t {
    TABLE_TYPE_SDR = 0,
    TABLE_TYPE_HDR,
    TABLE_TYPE_4K120,
    TABLE_TYPE_4K120_HDR,
    TABLE_TYPE_MAX,
} tvout_with_io_table_type_t;

typedef enum code_db_match_type_e {
    MATCH_TYPE_NO_DBVERSION,
    MATCH_TYPE_NEWCODE_OLDDB,
    MATCH_TYPE_MATCH,
    MATCH_TYPE_OLDCODE_NEWDB,
    MATCH_TYPE_MBOX_S5, //s928x
    MATCH_TYPE_MAX,
} code_db_match_type_t;

typedef struct database_attribute_s {
    String8 ToolVersion;
    String8 ProjectVersion; //project bringup date
    String8 GenerateTime;
    String8 ChipVersion; //chip information
    String8 dbversion; //each time db version
    String8 reserved;
} database_attribute_t;


#ifdef getSqlParams
#undef getSqlParams
#endif
#define getSqlParams(func, buffer, args...) \
    do{\
        sprintf(buffer, ##args);\
        SYS_LOGV("getSqlParams for %s\n", func);\
        SYS_LOGV("%s = %s\n",#buffer, buffer);\
    }while(0)

#define PQ_DB_CODE_MATCH_MASK        20191113

class CPQdb: public CSqlite {
public:
    CPQdb();
    ~CPQdb();
    int PQ_GetBlackExtensionParams(source_input_param_t source_input_param, am_regs_t *regs);
    int PQ_GetSharpness0FixedParams(source_input_param_t source_input_param, am_regs_t *regs);
    int PQ_GetSharpness1FixedParams(source_input_param_t source_input_param, am_regs_t *regs);
    int PQ_GetSharpnessPiFixedParams(source_input_param_t source_input_param, am_regs_t *regs);
    int PQ_SetSharpness0VariableParams(source_input_param_t source_input_param);
    int PQ_SetSharpness1VariableParams(source_input_param_t source_input_param);
    int PQ_SetSharpnessPiVariableParams(source_input_param_t source_input_param);
    int LoadVppBasicParam(tvpq_data_type_t data_type, source_input_param_t source_input_param);
    int PQ_GetCM2Params(vpp_color_management2_t basemode, source_input_param_t source_input_param, am_regs_t *regs);
    int PQ_GetXVYCCParams(vpp_xvycc_mode_t xvycc_mode, source_input_param_t source_input_param, am_regs_t *regs, am_regs_t *regs_1);
    int PQ_GetDIParams(source_input_param_t source_input_param, am_regs_t *regs);
    int PQ_GetMCDIParams(vpp_mcdi_mode_t mcdi_mode, source_input_param_t source_input_param, am_regs_t *regs);
    int PQ_GetDeblockParams(di_deblock_mode_t mode, source_input_param_t source_input_param, am_regs_t *regs);
    int PQ_GetNR2Params(vpp_noise_reduction_mode_t nr_mode, source_input_param_t source_input_param, am_regs_t *regs);
    int PQ_GetDemoSquitoParams(di_demosquito_mode_e demosquito, source_input_param_t source_input_param, am_regs_t *regs);
    int getDIRegValuesByValue(const char *name, const char *f_name, const char *f2_name, const int val, const int val2, am_regs_t *regs);
    int PQ_GetColorTemperatureParams(vpp_color_temperature_mode_t Tempmode, source_input_param_t source_input_param, tcon_rgb_ogo_t *params);
    int PQ_SetColorTemperatureParams(vpp_color_temperature_mode_t Tempmode, source_input_param_t source_input_param, tcon_rgb_ogo_t params);
    int PQ_ResetAllColorTemperatureParams(void);
    int PQ_SetNoLineAllBrightnessParams(tv_source_input_t source_input, int osd0, int osd25, int osd50, int osd75, int osd100);
    int PQ_GetNoLineAllBrightnessParams(tv_source_input_t source_input, int *osd0, int *osd25, int *osd50, int *osd75, int *osd100);
    int PQ_GetBrightnessParams(source_input_param_t source_input_param, int level, int *params);
    int PQ_SetBrightnessParams(source_input_param_t source_input_param, int level, int params);
    int PQ_SetNoLineAllContrastParams(tv_source_input_t source_input, int osd0, int osd25, int osd50, int osd75, int osd100);
    int PQ_GetNoLineAllContrastParams(tv_source_input_t source_input, int *osd0, int *osd25, int *osd50, int *osd75, int *osd100);
    int PQ_GetContrastParams(source_input_param_t source_input_param, int level, int *params);
    int PQ_SetContrastParams(source_input_param_t source_input_param, int level, int params);
    int PQ_SetNoLineAllSaturationParams(tv_source_input_t source_input, int osd0, int osd25, int osd50, int osd75, int osd100);
    int PQ_GetNoLineAllSaturationParams(tv_source_input_t source_input, int *osd0, int *osd25, int *osd50, int *osd75, int *osd100);
    int PQ_GetSaturationParams(source_input_param_t source_input_param, int level, int *params);
    int PQ_SetSaturationParams(source_input_param_t source_input_param, int level, int params);
    int PQ_SetNoLineAllHueParams(tv_source_input_t source_input, int osd0, int osd25, int osd50, int osd75, int osd100);
    int PQ_GetNoLineAllHueParams(tv_source_input_t source_input, int *osd0, int *osd25, int *osd50, int *osd75, int *osd100);
    int PQ_GetHueParams(source_input_param_t source_input_param, int level, int *params);
    int PQ_SetHueParams(source_input_param_t source_input_param, int level, int params);
    int PQ_SetNoLineAllSharpnessParams(tv_source_input_t source_input, int osd0, int osd25, int osd50, int osd75, int osd100);
    int PQ_GetNoLineAllSharpnessParams(tv_source_input_t source_input, int *osd0, int *osd25, int *osd50, int *osd75, int *osd100);
    int PQ_GetSharpness0Params(source_input_param_t source_input_param, int level, am_regs_t *regs);
    int PQ_GetSharpness1Params(source_input_param_t source_input_param, int level, am_regs_t *regs);
    int PQ_GetSharpnessPiParams(source_input_param_t source_input_param, int level, am_regs_t *regs);
    int PQ_SetSharpnessParams(source_input_param_t source_input_param, int level, am_regs_t regs);
    int PQ_SetNoLineAllVolumeParams(tv_source_input_t source_input, int osd0, int osd25, int osd50, int osd75, int osd100);
    int PQ_GetNoLineAllVolumeParams(tv_source_input_t source_input, int *osd0, int *osd25, int *osd50, int *osd75, int *osd100);
    int PQ_ResetAllNoLineParams(void);
    int PQ_GetNoiseReductionParams(vpp_noise_reduction_mode_t nr_mode, source_input_param_t source_input_param, int reg_addr);
    int PQ_SetNoiseReductionParams(vpp_noise_reduction_mode_t nr_mode, source_input_param_t source_input_param, int reg_addr, int value);
    int PQ_GetSharpnessCTIParams(source_input_param_t source_input_param, int reg_addr, int param_type, int reg_mask);
    int PQ_SetSharpnessCTIParams(source_input_param_t source_input_param, int reg_addr, int value, int param_type, int reg_mask);
    int PQ_GetCVD2Param(source_input_param_t source_input_param, int reg_addr, int param_type, int reg_mask);
    int PQ_SetCVD2Param(source_input_param_t source_input_param, int reg_addr, int value, int param_type, int reg_mask);
    int PQ_GetCVD2Params(source_input_param_t source_input_param, am_regs_t *regs);
    int PQ_GetSharpnessAdvancedParams(source_input_param_t source_input_param, int reg_addr, int isHd);
    int PQ_SetSharpnessAdvancedParams(source_input_param_t source_input_param, int reg_addr, int value, int isHd);
    int getSharpnessRegValues(const char *table_name, source_input_param_t source_input_param, am_regs_t *regs, int reg_addr, int isHd);
    int PQ_GetDNLPParams(source_input_param_t source_input_param, Dynamic_contrast_status_t mode, ve_dnlp_curve_param_t *newParams);
    int PQ_SetDNLPGains(source_input_param_t source_input_param, Dynamic_contrast_status_t level, int final_gain);
    int PQ_GetDNLPGains(source_input_param_t source_input_param, Dynamic_contrast_status_t level);
    int PQ_GetLocalContrastNodeParams(source_input_param_t source_input_param, local_contrast_mode_t mode, ve_lc_curve_parm_t *Params);
    int PQ_GetLocalContrastRegParams(source_input_param_t source_input_param, local_contrast_mode_t mode, am_regs_t *regs);
    int PQ_GetBEParams(source_input_param_t source_input_param, int addr, am_regs_t *regs);
    int PQ_SetBEParams(source_input_param_t source_input_param, int addr, unsigned int reg_val);
    int PQ_SetRGBCMYFcolor(source_input_param_t source_input_param, int data_Rank ,int val);
    int PQ_GetRGBCMYFcolor(source_input_param_t source_input_param, int data_Rank);
    int PQ_ResetAllOverscanParams(void);
    bool PQ_GetPqVersion(String8& ToolVersion, String8& ProjectVersion, String8& GenerateTime);
    bool PQ_GetDataBaseAttribute(database_attribute_t *DbAttribute);
    int PQ_GetPQModeParams(tv_source_input_t source_input, vpp_picture_mode_t pq_mode, vpp_pq_para_t *params);
    int PQ_SetPQModeParams(tv_source_input_t source_input, vpp_picture_mode_t pq_mode, vpp_pq_para_t *params);
    int PQ_ResetAllPQModeParams(void);

    int PQ_GetPictureModeParams(pq_src_param_t source_input, vpp_picture_mode_t pq_mode, vpp_pictur_mode_para_t *params);

    int PQ_GetTconGammaTable(int gamma_curve, gm_tbl_t *gamma_value);
    int PQ_GetGammaTableR(int panel_id, source_input_param_t source_input_param, tcon_gamma_table_t *gamma_r);
    int PQ_GetGammaTableG(int panel_id, source_input_param_t source_input_param, tcon_gamma_table_t *gamma_g);
    int PQ_GetGammaTableB(int panel_id, source_input_param_t source_input_param, tcon_gamma_table_t *gamma_b);
    int PQ_GetGammaSpecialTable(vpp_gamma_curve_t gamma_curve, const char *f_name, GAMMA_TABLE *gamma_value);
    int PQ_GetWhiteBalanceGammaSpecialTable(vpp_color_temperature_mode_t mode, const char *f_name, tcon_gamma_table_t *gamma_value);
	int PQ_GetVGAAdjustPara(tvin_sig_fmt_t vga_fmt, tvafe_vga_parm_t *adjparam);
    int PQ_SetVGAAdjustPara(tvin_sig_fmt_t vga_fmt, tvafe_vga_parm_t adjparam);
    int PQ_GetPhaseArray(am_phase_t *am_phase);
    int PQ_GetPLLParams(source_input_param_t source_input_param, am_regs_t *regs);
    int PQ_GetAIParams(aipq_mode_e mode, source_input_param_t source_input_param, ai_pic_table_t *regs);
    int PQ_GetHDRTMOParams(source_input_param_t source_input_param, hdr_tmo_t mode, hdr_tmo_sw_s *newParams);
    int PQ_GetSmoothPlusParams(vpp_smooth_plus_mode_t smoothplus_mode, source_input_param_t source_input_param, am_regs_t *regs);
    int PQ_GetAADParams(source_input_param_t source_input_param, aad_param_t *newParams);
    int PQ_GetCABCParams(source_input_param_t source_input_param, cabc_param_t *newParams);
    int openPqDB(const char *db_path);
    int closePqDB(void);
    int reopenDB(const char *db_path);
    int getRegValues(const char *table_name, am_regs_t *regs);
    int getRegValuesByValue(const char *name, const char *f_name, const char *f2_name, const int val, const int val2, am_regs_t *regs);
    int getRegValuesByValue_long(const char *name, const char *f_name, const char *f2_name, const int val, const int val2, am_regs_t *regs, am_regs_t *regs_1);
    void initialTable(int type);
    bool PQ_GetLDIM_Regs(vpu_ldim_param_s *vpu_ldim_param);
    int GetFileAttrIntValue(const char *fp, int flag);
    bool CheckHdrStatus(const char *tableName);
    bool CheckCVBSParamValidStatus(void);
    bool CheckIdExistInDb(const char *Id, const char *TableName);
    int PQ_GetBlackStretchParams(int level, source_input_param_t source_input_param, am_regs_t *regs);
    int PQ_GetBlueStretchParams(int level, source_input_param_t source_input_param, am_regs_t *regs);
    int PQ_GetChromaCoringParams(int level, source_input_param_t source_input_param, am_regs_t *regs);
    int PQ_GetLocalDimmingParams(int level, source_input_param_t source_input_param, aml_ldim_pq_s *newParams);
    int PQ_GetAiSrParams(aisr_mode_e mode, source_input_param_t source_input_param, am_regs_t *regs);

private:
    String8 GetTableName(const char *GeneralTableName, source_input_param_t source_input_param);
    String8 GetPqOsdTableName(const char *GeneralTableName, pq_src_param_t source_input_param);
    int CalculateLevelParam(tvpq_data_t *pq_data, int nodes, int level);
    am_regs_t CalculateLevelRegsParam(tvpq_sharpness_regs_t *pq_regs, int level, int sharpness_number);
    int GetNonlinearMapping(tvpq_data_type_t data_type, tv_source_input_t source_input, int level, int *params);
    int GetNonlinearMappingByOSDFac(tvpq_data_type_t data_type, tv_source_input_t source_input, int *params);
    int SetNonlinearMapping(tvpq_data_type_t data_type, tv_source_input_t source_input, int osd0, int osd25, int osd50, int osd75, int osd100);
    int loadSharpnessData(const char *table_name, int sharpness_number);
    int PQ_GetGammaTable(int panel_id, source_input_param_t source_input_param, const char *f_name, tcon_gamma_table_t *val);
    int SetNonlinearMappingByName(const char *name, tvpq_data_type_t data_type, tv_source_input_t source_input, int osd0, int osd25, int osd50, int osd75, int osd100);
    int PQ_SetPQModeParamsByName(const char *name, tv_source_input_t source_input, vpp_picture_mode_t pq_mode, vpp_pq_para_t *params);
    const char *getSharpnessTableName(source_input_param_t source_input_param, int isHd);
    am_regs_t MergeSameAddrVal(am_regs_t regs);
    void PQ_GetPqDbMatchType(database_attribute_t *DbAttribute);

    tvpq_data_t pq_bri_data[15];
    tvpq_data_t pq_con_data[15];
    tvpq_data_t pq_sat_data[15];
    tvpq_data_t pq_hue_data[15];
    tvpq_sharpness_regs_t pq_sharpness0_reg_data[10];
    tvpq_sharpness_regs_t pq_sharpness1_reg_data[10];
    tvpq_sharpness_regs_t pq_sharpnesspi_reg_data[10];
    int bri_nodes;
    int con_nodes;
    int hue_nodes;
    int sat_nodes;
    int sha0_nodes;
    int sha1_nodes;
    int sha2_nodes;
public:
    bool mHdrStatus = false;
    output_type_t mOutPutType = OUTPUT_TYPE_LVDS;
    code_db_match_type_t mDbMatchType = MATCH_TYPE_MATCH;
    int node_number = 0;
};
#endif
