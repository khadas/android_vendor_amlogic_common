/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef __VE_H
#define __VE_H

// ***************************************************************************
// *** enum definitions *********************************************
// ***************************************************************************

typedef enum ve_demo_pos_e {
    VE_DEMO_POS_TOP = 0,
    VE_DEMO_POS_BOTTOM,
    VE_DEMO_POS_LEFT,
    VE_DEMO_POS_RIGHT,
} ve_demo_pos_t;

typedef enum ve_dnlp_rt_e {
    VE_DNLP_RT_0S = 0,
    VE_DNLP_RT_1S = 6,
    VE_DNLP_RT_2S,
    VE_DNLP_RT_4S,
    VE_DNLP_RT_8S,
    VE_DNLP_RT_16S,
    VE_DNLP_RT_32S,
    VE_DNLP_RT_64S,
    VE_DNLP_RT_FREEZE,
} ve_dnlp_rt_t;

typedef enum ve_dnlp_state_e {
    VE_DNLP_STATE_OFF = 0,
    VE_DNLP_STATE_ON,
} ve_dnlp_state_t;

typedef enum pc_mode_e {
    PC_MOUDLE_OFF = 0,
    PC_MOUDLE_ON,
} pc_mode_t;

typedef enum ve_csc_type_e {
    VPP_MATRIX_NULL                  = 0,
    VPP_MATRIX_RGB_YUV601            = 0x1,
    VPP_MATRIX_RGB_YUV601F           = 0x2,
    VPP_MATRIX_RGB_YUV709            = 0x3,
    VPP_MATRIX_RGB_YUV709F           = 0x4,
    VPP_MATRIX_YUV601_RGB            = 0x10,
    VPP_MATRIX_YUV601_YUV601F        = 0x11,
    VPP_MATRIX_YUV601_YUV709         = 0x12,
    VPP_MATRIX_YUV601_YUV709F        = 0x13,
    VPP_MATRIX_YUV601F_RGB           = 0x14,
    VPP_MATRIX_YUV601F_YUV601        = 0x15,
    VPP_MATRIX_YUV601F_YUV709        = 0x16,
    VPP_MATRIX_YUV601F_YUV709F       = 0x17,
    VPP_MATRIX_YUV709_RGB            = 0x20,
    VPP_MATRIX_YUV709_YUV601         = 0x21,
    VPP_MATRIX_YUV709_YUV601F        = 0x22,
    VPP_MATRIX_YUV709_YUV709F        = 0x23,
    VPP_MATRIX_YUV709F_RGB           = 0x24,
    VPP_MATRIX_YUV709F_YUV601        = 0x25,
    VPP_MATRIX_YUV709F_YUV709        = 0x26,
    VPP_MATRIX_YUV601L_YUV709L       = 0x27,
    VPP_MATRIX_YUV709L_YUV601L       = 0x28,
    VPP_MATRIX_YUV709F_YUV601F       = 0x29,
    VPP_MATRIX_BT2020YUV_BT2020RGB   = 0x40,
    VPP_MATRIX_BT2020RGB_709RGB,
    VPP_MATRIX_BT2020RGB_CUSRGB,
    VPP_MATRIX_DEFAULT_CSCTYPE       = 0xffff,
} ve_csc_type_t;

typedef enum hdr_type_e {
    HDR_TYPE_NONE = 0,
    HDR_TYPE_HDR10 = 1,
    HDR_TYPE_HDR10PLUS = 2,
    HDR_TYPE_DOVI = 3,
    HDR_TYPE_PRIMESL = 4,
    HDR_TYPE_HLG = 5,
    HDR_TYPE_SDR = 6,
    HDR_TYPE_MVC = 7,
} hdr_type_t;

typedef enum meson_cpu_ver_e {
    MESON_CPU_VERSION_NULL = 0,
    MESON_CPU_VERSION_A,
    MESON_CPU_VERSION_B,
    MESON_CPU_VERSION_C,
    MESON_CPU_VERSION_MAX,
} meson_cpu_ver_t;

// ***************************************************************************
// *** struct definitions *********************************************
// ***************************************************************************

typedef enum ve_dnlp_rl_e {
    VE_DNLP_RL_01 = 1, // max_contrast = 1.0625x
    VE_DNLP_RL_02,     // max_contrast = 1.1250x
    VE_DNLP_RL_03,     // max_contrast = 1.1875x
    VE_DNLP_RL_04,     // max_contrast = 1.2500x
    VE_DNLP_RL_05,     // max_contrast = 1.3125x
    VE_DNLP_RL_06,     // max_contrast = 1.3750x
    VE_DNLP_RL_07,     // max_contrast = 1.4375x
    VE_DNLP_RL_08,     // max_contrast = 1.5000x
    VE_DNLP_RL_09,     // max_contrast = 1.5625x
    VE_DNLP_RL_10,     // max_contrast = 1.6250x
    VE_DNLP_RL_11,     // max_contrast = 1.6875x
    VE_DNLP_RL_12,     // max_contrast = 1.7500x
    VE_DNLP_RL_13,     // max_contrast = 1.8125x
    VE_DNLP_RL_14,     // max_contrast = 1.8750x
    VE_DNLP_RL_15,     // max_contrast = 1.9375x
    VE_DNLP_RL_16,     // max_contrast = 2.0000x
} ve_dnlp_rl_t;

typedef enum ve_dnlp_ext_e {
    VE_DNLP_EXT_00 = 0, // weak
    VE_DNLP_EXT_01,
    VE_DNLP_EXT_02,
    VE_DNLP_EXT_03,
    VE_DNLP_EXT_04,
    VE_DNLP_EXT_05,
    VE_DNLP_EXT_06,
    VE_DNLP_EXT_07,
    VE_DNLP_EXT_08,
    VE_DNLP_EXT_09,
    VE_DNLP_EXT_10,
    VE_DNLP_EXT_11,
    VE_DNLP_EXT_12,
    VE_DNLP_EXT_13,
    VE_DNLP_EXT_14,
    VE_DNLP_EXT_15,
    VE_DNLP_EXT_16,     // strong
} ve_dnlp_ext_t;

typedef struct ve_bext_s {
    unsigned char en;
    unsigned char start;
    unsigned char slope1;
    unsigned char midpt;
    unsigned char slope2;
} ve_bext_t;

typedef struct ve_dnlp_s {
    unsigned int      en;
    enum ve_dnlp_rt_e rt;
    enum ve_dnlp_rl_e rl;
    enum ve_dnlp_ext_e black;
    enum ve_dnlp_ext_e white;
} ve_dnlp_t;
typedef struct ve_hist_s {
    unsigned int sum;
    int width;
    int height;
    int ave;
} ve_hist_t;

typedef struct vpp_hist_param_s {
    unsigned int vpp_hist_pow;
    unsigned int vpp_luma_sum;
    unsigned int vpp_pixel_sum;
    unsigned short vpp_histgram[64];
}vpp_hist_param_t;

/*typedef struct ve_dnlp_table_s {
    unsigned int en;
    unsigned int method;
    unsigned int cliprate;
    unsigned int lowrange;
    unsigned int hghrange;
    unsigned int lowalpha;
    unsigned int midalpha;
    unsigned int hghalpha;
} ve_dnlp_table_t;*/
/*typedef struct ve_dnlp_table_s {
    unsigned int en;
    unsigned int method;
    unsigned int cliprate;
    unsigned int lowrange;
    unsigned int hghrange;
    unsigned int lowalpha;
    unsigned int midalpha;
    unsigned int hghalpha;
    unsigned int adj_level;
    unsigned int new_adj_level;
    unsigned int new_mvreflsh;
    unsigned int new_gmma_rate;
    unsigned int new_lowalpha;
    unsigned int new_hghalpha;
    unsigned int new_sbgnbnd;
    unsigned int new_sendbnd;
    unsigned int new_clashBgn;
    unsigned int new_clashEnd;
    unsigned int new_cliprate;
    unsigned int new_mtdbld_rate;
    unsigned int new_blkgma_rate;
} ve_dnlp_table_t;*/
typedef struct ve_dnlp_table_s {
    unsigned int en;
    unsigned int method;
    unsigned int cliprate;
    unsigned int lowrange;
    unsigned int hghrange;
    unsigned int lowalpha;
    unsigned int midalpha;
    unsigned int hghalpha;
    unsigned int adj_level;
    unsigned int new_adj_level;
    unsigned int new_mvreflsh;
    unsigned int new_gmma_rate;
    unsigned int new_lowalpha;
    unsigned int new_hghalpha;
    unsigned int new_sbgnbnd;
    unsigned int new_sendbnd;
    unsigned int new_clashBgn;
    unsigned int new_clashEnd;
    unsigned int new_cliprate;
    unsigned int new_mtdbld_rate;
    unsigned int new_dnlp_pst_gmarat;
    unsigned int dnlp_sel;
    unsigned int dnlp_blk_cctr;/*blk signal add brightness*/
    unsigned int dnlp_brgt_ctrl;
    unsigned int dnlp_brgt_range;
    unsigned int dnlp_brght_add;
    unsigned int dnlp_brght_max;
    unsigned int dnlp_almst_wht;
    unsigned int dnlp_hghbin;/*1*/
    unsigned int dnlp_hghnum;
    unsigned int dnlp_lowbin;
    unsigned int dnlp_lownum;
    unsigned int dnlp_bkgend;
    unsigned int dnlp_bkgert;
    unsigned int dnlp_blkext;
    unsigned int dnlp_whtext;
    unsigned int dnlp_bextmx;
    unsigned int dnlp_wextmx;
    unsigned int dnlp_smhist_ck;
    unsigned int dnlp_glb_crate;/*12*/

    unsigned int dnlp_pstgma_brghtrate;
    unsigned int dnlp_pstgma_brghtrat1;
    unsigned int dnlp_wext_autorat;
    unsigned int dnlp_cliprate_min;
    unsigned int dnlp_adpcrat_lbnd;
    unsigned int dnlp_adpcrat_hbnd;
    unsigned int dnlp_adpmtd_lbnd;
    unsigned int dnlp_adpmtd_hbnd;
    unsigned int dnlp_set_bext;
    unsigned int dnlp_set_wext;
    unsigned int dnlp_satur_rat;
    unsigned int dnlp_satur_max;
    unsigned int blk_prct_rng;
    unsigned int blk_prct_max;
    unsigned int dnlp_lowrange;
    unsigned int dnlp_hghrange;
    unsigned int dnlp_auto_rng;
    unsigned int dnlp_bin0_absmax;
    unsigned int dnlp_bin0_sbtmax;
    unsigned int dnlp_adpalpha_lrate;
    unsigned int dnlp_adpalpha_hrate;

    unsigned int dnlp_lrate00;/*0-64bin curve slope*/
    unsigned int dnlp_lrate02;
    unsigned int dnlp_lrate04;
    unsigned int dnlp_lrate06;
    unsigned int dnlp_lrate08;
    unsigned int dnlp_lrate10;
    unsigned int dnlp_lrate12;
    unsigned int dnlp_lrate14;
    unsigned int dnlp_lrate16;
    unsigned int dnlp_lrate18;
    unsigned int dnlp_lrate20;
    unsigned int dnlp_lrate22;
    unsigned int dnlp_lrate24;
    unsigned int dnlp_lrate26;
    unsigned int dnlp_lrate28;
    unsigned int dnlp_lrate30;
    unsigned int dnlp_lrate32;
    unsigned int dnlp_lrate34;
    unsigned int dnlp_lrate36;
    unsigned int dnlp_lrate38;
    unsigned int dnlp_lrate40;
    unsigned int dnlp_lrate42;
    unsigned int dnlp_lrate44;
    unsigned int dnlp_lrate46;
    unsigned int dnlp_lrate48;
    unsigned int dnlp_lrate50;
    unsigned int dnlp_lrate52;
    unsigned int dnlp_lrate54;
    unsigned int dnlp_lrate56;
    unsigned int dnlp_lrate58;
    unsigned int dnlp_lrate60;
    unsigned int dnlp_lrate62;
} ve_dnlp_table_t;
typedef struct ve_hsvs_s {
    unsigned char en;
    unsigned char peak_gain_h1;
    unsigned char peak_gain_h2;
    unsigned char peak_gain_h3;
    unsigned char peak_gain_h4;
    unsigned char peak_gain_h5;
    unsigned char peak_gain_v1;
    unsigned char peak_gain_v2;
    unsigned char peak_gain_v3;
    unsigned char peak_gain_v4;
    unsigned char peak_gain_v5;
    unsigned char peak_gain_v6;
    unsigned char hpeak_slope1;
    unsigned char hpeak_slope2;
    unsigned char hpeak_thr1;
    unsigned char hpeak_thr2;
    unsigned char hpeak_nlp_cor_thr;
    unsigned char hpeak_nlp_gain_pos;
    unsigned char hpeak_nlp_gain_neg;
    unsigned char vpeak_slope1;
    unsigned char vpeak_slope2;
    unsigned char vpeak_thr1;
    unsigned char vpeak_thr2;
    unsigned char vpeak_nlp_cor_thr;
    unsigned char vpeak_nlp_gain_pos;
    unsigned char vpeak_nlp_gain_neg;
    unsigned char speak_slope1;
    unsigned char speak_slope2;
    unsigned char speak_thr1;
    unsigned char speak_thr2;
    unsigned char speak_nlp_cor_thr;
    unsigned char speak_nlp_gain_pos;
    unsigned char speak_nlp_gain_neg;
    unsigned char peak_cor_gain;
    unsigned char peak_cor_thr_l;
    unsigned char peak_cor_thr_h;
    unsigned char vlti_step;
    unsigned char vlti_step2;
    unsigned char vlti_thr;
    unsigned char vlti_gain_pos;
    unsigned char vlti_gain_neg;
    unsigned char vlti_blend_factor;
    unsigned char hlti_step;
    unsigned char hlti_thr;
    unsigned char hlti_gain_pos;
    unsigned char hlti_gain_neg;
    unsigned char hlti_blend_factor;
    unsigned char vlimit_coef_h;
    unsigned char vlimit_coef_l;
    unsigned char hlimit_coef_h;
    unsigned char hlimit_coef_l;
    unsigned char cti_444_422_en;
    unsigned char cti_422_444_en;
    unsigned char cti_blend_factor;
    unsigned char vcti_buf_en;
    unsigned char vcti_buf_mode_c5l;
    unsigned char vcti_filter;
    unsigned char hcti_step;
    unsigned char hcti_step2;
    unsigned char hcti_thr;
    unsigned char hcti_gain;
    unsigned char hcti_mode_median;
} ve_hsvs_t;

typedef struct ve_ccor_s {
    unsigned char en;
    unsigned char slope;
    unsigned char thr;
} ve_ccor_t;

typedef struct ve_benh_s {
    unsigned char en;
    unsigned char cb_inc;
    unsigned char cr_inc;
    unsigned char gain_cr;
    unsigned char gain_cb4cr;
    unsigned char luma_h;
    unsigned char err_crp;
    unsigned char err_crn;
    unsigned char err_cbp;
    unsigned char err_cbn;
} ve_benh_t;

typedef struct ve_cbar_s {
    unsigned char en;
    unsigned char wid;
    unsigned char cr;
    unsigned char cb;
    unsigned char y;
} ve_cbar_t;
typedef struct ve_demo_s {
    unsigned char bext;
    unsigned char dnlp;
    unsigned char hsvs;
    unsigned char ccor;
    unsigned char benh;
    enum  ve_demo_pos_e  pos;
    unsigned long wid;
    struct ve_cbar_s   cbar;
} ve_demo_t;

typedef struct vdo_meas_s {
    //...
} vdo_meas_t;

typedef struct ve_regmap_s {
    unsigned long reg[43];
} ve_regmap_t;

/*adjust for user*/
typedef struct hdr_tmo_sw_s {
    int tmo_en;              // 0 1
    int reg_highlight;       //u10: control overexposure level
    int reg_hist_th;         //u7
    int reg_light_th;
    int reg_highlight_th1;
    int reg_highlight_th2;
    int reg_display_e;       //u10
    int reg_middle_a;        //u7
    int reg_middle_a_adj;    //u10
    int reg_middle_b;        //u7
    int reg_middle_s;        //u7
    int reg_max_th1;          //u10
    int reg_middle_th;          //u10
    int reg_thold1;          //u10
    int reg_thold2;          //u10
    int reg_thold3;          //u10
    int reg_thold4;          //u10
    int reg_max_th2;          //u10
    int reg_pnum_th;          //u16
    int reg_hl0;
    int reg_hl1;             //u7
    int reg_hl2;             //u7
    int reg_hl3;             //u7
    int reg_display_adj;     //u7
    int reg_avg_th;
    int reg_avg_adj;
    int reg_low_adj;         //u7
    int reg_high_en;         //u3
    int reg_high_adj1;       //u7
    int reg_high_adj2;       //u7
    int reg_high_maxdiff;    //u7
    int reg_high_mindiff;    //u7
    unsigned int alpha;
}hdr_tmo_sw_t;

//////////for customer DNLP
typedef struct ve_dnlp_curve_param_s {
    unsigned int ve_dnlp_scurv_low[65];
    unsigned int ve_dnlp_scurv_mid1[65];
    unsigned int ve_dnlp_scurv_mid2[65];
    unsigned int ve_dnlp_scurv_hgh1[65];
    unsigned int ve_dnlp_scurv_hgh2[65];
    unsigned int ve_gain_var_lut49[49];
    unsigned int ve_wext_gain[48];
    unsigned int ve_adp_thrd[33];
    unsigned int ve_reg_blk_boost_12[13];
    unsigned int ve_reg_adp_ofset_20[20];
    unsigned int ve_reg_mono_protect[6];
    unsigned int ve_reg_trend_wht_expand_lut8[9];
    unsigned int ve_c_hist_gain[65];
    unsigned int ve_s_hist_gain[65];
    unsigned int param[100];
}ve_dnlp_curve_param_t;

typedef enum dnlp_curve_e {
    scurv_low = 1000,
    scurv_mid1,
    scurv_mid2,
    scurv_hgh1,
    scurv_hgh2,
    curv_var_lut49,
    curv_wext_gain,
    adp_thrd = 1013,
    reg_blk_boost_12,
    reg_adp_ofset_20,
    reg_mono_protect,
    reg_trend_wht_expand_lut8,
    c_hist_gain = 1019,
    s_hist_gain,
} dnlp_curve_t;

//local contrast
typedef struct ve_lc_curve_parm_s {
    unsigned int ve_lc_saturation[63];
    unsigned int ve_lc_yminval_lmt[16];
    unsigned int ve_lc_ypkbv_ymaxval_lmt[16];
    unsigned int ve_lc_ymaxval_lmt[16];
    unsigned int ve_lc_ypkbv_lmt[16];
    unsigned int ve_lc_ypkbv_ratio[4];
    unsigned int param[100];
} ve_lc_curve_parm_t;

typedef enum lc_curve_parm_num_e {
    LC_SATURATION_NUM = 1,
    LC_YMINVAL_LMT_NUM = 2,
    LC_YPKBV_YMAXVAL_LMT_NUM = 4,
    LC_YPKBV_RATIO_NUM = 8,
    LC_YMAXVAL_LMT_NUM = 16,
    LC_YPKBV_LMT_NUM = 32,
} lc_curve_parm_num_t;

typedef struct pq_ctrl_s {
    unsigned char sharpness0_en;
    unsigned char sharpness1_en;
    unsigned char dnlp_en;
    unsigned char cm_en;
    unsigned char vadj1_en;    //control video brightness contrast saturation hue
    unsigned char vd1_ctrst_en;
    unsigned char vadj2_en;    //control video+osd brightness contrast saturation hue
    unsigned char post_ctrst_en;
    unsigned char wb_en;
    unsigned char gamma_en;
    unsigned char lc_en;
    unsigned char black_ext_en;
    unsigned char chroma_cor_en;
    unsigned char reserved;
} pq_ctrl_t;

typedef struct vpp_pq_ctrl_s {
    unsigned int length;
    long long ptr;        /*point to pq_ctrl_s*/
} vpp_pq_ctrl_t;

struct db_cabc_aad_param_s {
    unsigned int length;
    union {
        void *cabc_aad_param_ptr;
        long long cabc_aad_param_ptr_len;
    };
};

typedef struct db_cabc_param_s {
    int cabc_param_cabc_en;
    int cabc_param_hist_mode;
    int cabc_param_tf_en;
    int cabc_param_sc_flag;
    int cabc_param_bl_map_mode;
    int cabc_param_bl_map_en;
    int cabc_param_temp_proc;
    int cabc_param_max95_ratio;
    int cabc_param_hist_blend_alpha;
    int cabc_param_init_bl_min;
    int cabc_param_init_bl_max;
    int cabc_param_tf_alpha;
    int cabc_param_sc_hist_diff_thd;
    int cabc_param_sc_apl_diff_thd;
    int cabc_param_patch_bl_th;
    int cabc_param_patch_on_alpha;
    int cabc_param_patch_bl_off_th;
    int cabc_param_patch_off_alpha;
    struct db_cabc_aad_param_s db_o_bl_cv;
    struct db_cabc_aad_param_s db_maxbin_bl_cv;
}db_cabc_param_t;

#define CABC_O_BL_CV_MAX       15
#define CABC_MAXBIN_BL_CV_MAX  15

typedef struct cabc_param_s {
    int cabc_param_cabc_en;
    int cabc_param_hist_mode;
    int cabc_param_tf_en;
    int cabc_param_sc_flag;
    int cabc_param_bl_map_mode;
    int cabc_param_bl_map_en;
    int cabc_param_temp_proc;
    int cabc_param_max95_ratio;
    int cabc_param_hist_blend_alpha;
    int cabc_param_init_bl_min;
    int cabc_param_init_bl_max;
    int cabc_param_tf_alpha;
    int cabc_param_sc_hist_diff_thd;
    int cabc_param_sc_apl_diff_thd;
    int cabc_param_patch_bl_th;
    int cabc_param_patch_on_alpha;
    int cabc_param_patch_bl_off_th;
    int cabc_param_patch_off_alpha;
    int cabc_param_o_bl_cv_len;
    int cabc_param_o_bl_cv[CABC_O_BL_CV_MAX];
    int cabc_param_maxbin_bl_cv_len;
    int cabc_param_maxbin_bl_cv[CABC_MAXBIN_BL_CV_MAX];
}cabc_param_t;

typedef enum aad_curve_e {
    LUT_Y_gain = 400,
    LUT_RG_gain,
    LUT_BG_gain,
    gain_lut,
    xy_lut,
    sensor_input,
} add_curve_t;

typedef struct db_aad_param_s {
    int aad_param_cabc_aad_en;
    int aad_param_aad_en;
    int aad_param_tf_en;
    int aad_param_force_gain_en;
    int aad_param_sensor_mode;
    int aad_param_mode;
    int aad_param_dist_mode;
    int aad_param_tf_alpha;
    int aad_param_sensor_input[3];
    struct db_cabc_aad_param_s db_LUT_Y_gain;
    struct db_cabc_aad_param_s db_LUT_RG_gain;
    struct db_cabc_aad_param_s db_LUT_BG_gain;
    struct db_cabc_aad_param_s db_gain_lut;
    struct db_cabc_aad_param_s db_xy_lut;
}db_aad_param_t;

#define CAAD_SENSOR_INPUT_MAX  3
#define CAAD_LUT_Y_GAIN_MAX    20
#define CAAD_LUT_RG_GAIN_MAX   20
#define CAAD_LUT_BG_GAIN_MAX   20
#define CAAD_GAIN_LUT_MAX      50
#define CAAD_XY_LUT_MAX        40


typedef struct aad_param_s {
    int aad_param_cabc_aad_en;
    int aad_param_aad_en;
    int aad_param_tf_en;
    int aad_param_force_gain_en;
    int aad_param_sensor_mode;
    int aad_param_mode;
    int aad_param_dist_mode;
    int aad_param_tf_alpha;
    int aad_param_sensor_input_len;
    int aad_param_sensor_input[CAAD_SENSOR_INPUT_MAX];
    int aad_param_LUT_Y_gain_len;
    int aad_param_LUT_Y_gain[CAAD_LUT_Y_GAIN_MAX];
    int aad_param_LUT_RG_gain_len;
    int aad_param_LUT_RG_gain[CAAD_LUT_RG_GAIN_MAX];
    int aad_param_LUT_BG_gain_len;
    int aad_param_LUT_BG_gain[CAAD_LUT_BG_GAIN_MAX];
    int aad_param_gain_lut_len;
    int aad_param_gain_lut[CAAD_GAIN_LUT_MAX];
    int aad_param_xy_lut_len;
    int aad_param_xy_lut[CAAD_XY_LUT_MAX];
}aad_param_t;

typedef enum lut_type_e {
    LUT_TYPE_HLG = 1,
    LUT_TYPE_HDR = 2,
    LUT_TYPE_MAX
} lut_type_t;

/*tone mapping struct*/
typedef struct hdr_tone_mapping_s {
    lut_type_t lut_type;
    unsigned int lutlength;
    union {
    void *tm_lut;
    long long tm_lut_len;
    };
} vpp_hdr_tone_mapping_t;

// ***************************************************************************
// *** MACRO definitions **********
// ***************************************************************************

// ***************************************************************************
// *** FUNCTION definitions **********
// ***************************************************************************

#endif  // _VE_H
