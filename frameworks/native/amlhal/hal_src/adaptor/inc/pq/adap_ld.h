#ifndef _ADAP_LD_H_
#define _ADAP_LD_H_

#include <adap_common.h>



/**
*** definition
**/
/******************************************halpq itself define******************************************************/
typedef enum _adap_ld_level_e {
      ADAP_LD_LV_OFF,
      ADAP_LD_LV_LOW,
      ADAP_LD_LV_MID,
      ADAP_LD_LV_HIGH,
      ADAP_LD_LV_MAX,
} adap_ld_level_e;

typedef struct aml_path_s {
    CHAR string[256];
} aml_path_t;

typedef struct am_pq_bin_param_s {
    UINT32 table_index;
    UINT32 table_len;
    union {
        void *table_ptr;
        SINT64 l_table;
    };
}am_pq_bin_param_t;


/******************************************copy from ldm driver******************************************************/
typedef struct aml_ldim_pq_s {
    UINT32 func_en;
    UINT32 remapping_en;

    /* switch fw, use for custom fw. 0=aml_hw_fw, 1=aml_sw_fw */
    UINT32 fw_sel;

    /* fw parameters */
    UINT32 ldc_hist_mode;
    UINT32 ldc_hist_blend_mode;
    UINT32 ldc_hist_blend_alpha;
    UINT32 ldc_hist_adap_blend_max_gain;
    UINT32 ldc_hist_adap_blend_diff_th1;
    UINT32 ldc_hist_adap_blend_diff_th2;
    UINT32 ldc_hist_adap_blend_th0;
    UINT32 ldc_hist_adap_blend_thn;
    UINT32 ldc_hist_adap_blend_gain_0;
    UINT32 ldc_hist_adap_blend_gain_1;
    UINT32 ldc_init_bl_min;
    UINT32 ldc_init_bl_max;

    UINT32 ldc_sf_mode;
    UINT32 ldc_sf_gain_up;
    UINT32 ldc_sf_gain_dn;
    UINT32 ldc_sf_tsf_3x3;
    UINT32 ldc_sf_tsf_5x5;

    UINT32 ldc_bs_bl_mode;
    UINT32 ldc_bs_glb_apl_gain;
    UINT32 ldc_bs_dark_scene_bl_th;
    UINT32 ldc_bs_gain;
    UINT32 ldc_bs_limit_gain;
    UINT32 ldc_bs_loc_apl_gain;
    UINT32 ldc_bs_loc_max_min_gain;
    UINT32 ldc_bs_loc_dark_scene_bl_th;

    UINT32 ldc_tf_en;
    UINT32 ldc_tf_low_alpha;
    UINT32 ldc_tf_high_alpha;
    UINT32 ldc_tf_low_alpha_sc;
    UINT32 ldc_tf_high_alpha_sc;

    UINT32 ldc_dimming_curve_en;
    UINT32 ldc_sc_hist_diff_th;
    UINT32 ldc_sc_apl_diff_th;
    UINT32 bl_remap_curve[17];
    UINT32 post_bl_remap_curve[17];
    /* comp parameters */
    UINT32 ldc_bl_buf_diff;
    UINT32 ldc_glb_gain;
    UINT32 ldc_dth_en;
    UINT32 ldc_dth_bw;
    UINT32 ldc_gain_lut[16][64];
    UINT32 ldc_min_gain_lut[64];
} aml_ldim_pq_t;



/**
*** function
**/
ADAP_STATUS_T ADAP_LD_INIT(void);
ADAP_STATUS_T ADAP_LD_UNINIT(void);
ADAP_STATUS_T ADAP_LD_DevIoCtl(int request, ...);
ADAP_STATUS_T ADAP_LD_GetPqInitStatus(void);
ADAP_STATUS_T ADAP_LD_SetPqInit(void);
ADAP_STATUS_T ADAP_LD_GetLevelIdx(int *pLevelIdx);
ADAP_STATUS_T ADAP_LD_SetLevelIdx(int iLevelIdx);
ADAP_STATUS_T ADAP_LD_GetFuncEn(int *pFuncEn);
ADAP_STATUS_T ADAP_LD_SetFuncEn(int iFuncEn);
ADAP_STATUS_T ADAP_LD_GetRemapEn(int *pRemapEn);
ADAP_STATUS_T ADAP_LD_SetRemapEn(int iRemapEn);
ADAP_STATUS_T ADAP_LD_GetBlMatrix(int *pBlMatrix);
ADAP_STATUS_T ADAP_LD_SetBlMatrix(int iMatrix);
ADAP_STATUS_T ADAP_LD_GetDemoMode(int *pDemoMode);
ADAP_STATUS_T ADAP_LD_SetDemoMode(int iDemoMode);

ADAP_STATUS_T ADAP_LD_GetLdmInfo(aml_ldim_pq_t *pLdmInfo);
ADAP_STATUS_T ADAP_LD_SetLdmInfo(aml_ldim_pq_t *pLdmInfo);

ADAP_STATUS_T ADAP_LD_GetLdBLMappingPath(aml_path_t *Path);
ADAP_STATUS_T ADAP_LD_SetLdBLMapping(am_pq_bin_param_t *pData);
ADAP_STATUS_T ADAP_LD_GetLdBLProfilePath(aml_path_t *Path);
ADAP_STATUS_T ADAP_LD_SetLdBLProfile(am_pq_bin_param_t *pData);
ADAP_STATUS_T ADAP_LD_GetLdStructTable(am_pq_bin_param_t *pData);
ADAP_STATUS_T ADAP_LD_SetLdStructTable(am_pq_bin_param_t *pData);

#endif
