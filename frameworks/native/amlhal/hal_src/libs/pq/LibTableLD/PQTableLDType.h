#ifndef _PQ_TABLE_LD_TYPE_H_
#define _PQ_TABLE_LD_TYPE_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum pq_ld_level_e {
    VPP_PQ_LD_LV_OFF,
    VPP_PQ_LD_LV_LOW,
    VPP_PQ_LD_LV_MID,
    VPP_PQ_LD_LV_HIGH,
    VPP_PQ_LD_LV_MAX,
} pq_ld_level_t;

typedef enum LD_bin_table_index_e
{
    LD_BIN_BL_MAPPING = 0,
    LD_BIN_BL_PROFILE,
    LD_BIN_BL_MAX,
} LD_bin_table_index_t;

typedef struct _TABLE_VER_PQ_LD
{
    char ProjectVersion[64];
    char ChipVersion[64];
    char TableVersion[64];
    char oem_model[64];
    char PanelIndex[64];
    char reserved[64];
} TABLE_VER_PQ_LD;

typedef enum _PQ_TABLE_LD_TYPE {
    PQ_TABLE_LD_VERSION = 0,
    PQ_TABLE_LD_DATA,
    PQ_TABLE_LD_MAX,
} PQ_TABLE_LD_TYPE;

typedef struct ldim_pq_s {
    unsigned int func_en;
    unsigned int remapping_en;

    /* switch fw, use for custom fw. 0=aml_hw_fw, 1=aml_sw_fw */
    unsigned int fw_sel;

    /* fw parameters */
    unsigned int ldc_hist_mode;
    unsigned int ldc_hist_blend_mode;
    unsigned int ldc_hist_blend_alpha;
    unsigned int ldc_hist_adap_blend_max_gain;
    unsigned int ldc_hist_adap_blend_diff_th1;
    unsigned int ldc_hist_adap_blend_diff_th2;
    unsigned int ldc_hist_adap_blend_th0;
    unsigned int ldc_hist_adap_blend_thn;
    unsigned int ldc_hist_adap_blend_gain_0;
    unsigned int ldc_hist_adap_blend_gain_1;
    unsigned int ldc_init_bl_min;
    unsigned int ldc_init_bl_max;

    unsigned int ldc_sf_mode;
    unsigned int ldc_sf_gain_up;
    unsigned int ldc_sf_gain_dn;
    unsigned int ldc_sf_tsf_3x3;
    unsigned int ldc_sf_tsf_5x5;

    unsigned int ldc_bs_bl_mode;
    //unsigned int ldc_glb_apl; //read only
    unsigned int ldc_bs_glb_apl_gain;
    unsigned int ldc_bs_dark_scene_bl_th;
    unsigned int ldc_bs_gain;
    unsigned int ldc_bs_limit_gain;
    unsigned int ldc_bs_loc_apl_gain;
    unsigned int ldc_bs_loc_max_min_gain;
    unsigned int ldc_bs_loc_dark_scene_bl_th;

    unsigned int ldc_tf_en;
    //unsigned int ldc_tf_sc_flag; //read only
    unsigned int ldc_tf_low_alpha;
    unsigned int ldc_tf_high_alpha;
    unsigned int ldc_tf_low_alpha_sc;
    unsigned int ldc_tf_high_alpha_sc;

    unsigned int ldc_dimming_curve_en;
    unsigned int ldc_sc_hist_diff_th;
    unsigned int ldc_sc_apl_diff_th;
    unsigned int bl_remap_curve[17];
    unsigned int post_bl_remap_curve[17];

    /* comp parameters */
    unsigned int ldc_bl_buf_diff;
    unsigned int ldc_glb_gain;
    unsigned int ldc_dth_en;
    unsigned int ldc_dth_bw;
    unsigned int ldc_gain_lut[16][64];
    unsigned int ldc_min_gain_lut[64];
}ldim_pq_t;

typedef struct _TABLE_STRUCT_PQ_LD {
    ldim_pq_t LDData[VPP_PQ_LD_LV_MAX];
}TABLE_STRUCT_PQ_LD;

#ifdef __cplusplus
}
#endif
#endif
