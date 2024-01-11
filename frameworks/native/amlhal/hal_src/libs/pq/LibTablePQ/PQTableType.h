#ifndef _PQ_TABLE_TYPE_H_
#define _PQ_TABLE_TYPE_H_


#ifdef __cplusplus
extern "C" {
#endif

#define PQ_INDEX_TABLE_SIZE                 25

#define GAMMA_TABLE_NUM_MAX                 20
#define DNLP_TABLE_NUM_MAX                  20
#define HDR_TONEMAPPING_TABLE_NUM_MAX       20
#define AI_PQ_TABLE_NUM_MAX                 10
#define LC_TABLE_NUM_MAX                    10

/*-----------------------------------------------------------------------------*/
/* PQ Version*/
/*-----------------------------------------------------------------------------*/

typedef struct _TABLE_VER_PQ
{
    char ProjectVersion[64];
    char ChipVersion[64];
    char TableVersion[64];
    char oem_model[64];
    char PanelIndex[64];
    char reserved[64];
} TABLE_VER_PQ;

typedef struct gamma_table_s {
    unsigned short R[256];
    unsigned short G[256];
    unsigned short B[256];
} gamma_table_t;


typedef struct pq_tcon_gamma_table_s {
    gamma_table_t GammaData[GAMMA_TABLE_NUM_MAX];
} pq_tcon_gamma_table_t;

typedef enum pq_level_e {
    VPP_PQ_LV_OFF,
    VPP_PQ_LV_LOW,
    VPP_PQ_LV_MID,
    VPP_PQ_LV_HIGH,
    VPP_PQ_LV_MAX,
} pq_level_t;

typedef enum _PQ_SOURCE_TIMING {
    PQ_SRC_INDEX_VGA = 0,

    PQ_SRC_INDEX_ATV_NTSC,
    PQ_SRC_INDEX_ATV_PAL,
    PQ_SRC_INDEX_ATV_PAL_M,
    PQ_SRC_INDEX_ATV_SECAN,
    PQ_SRC_INDEX_ATV_NTSC443,
    PQ_SRC_INDEX_ATV_PAL60,
    PQ_SRC_INDEX_ATV_NTSC50,
    PQ_SRC_INDEX_ATV_PALN,

    PQ_SRC_INDEX_AV_NTSC,
    PQ_SRC_INDEX_AV_PAL,
    PQ_SRC_INDEX_AV_PAL_M,
    PQ_SRC_INDEX_AV_SECAN,
    PQ_SRC_INDEX_AV_NTSC443,
    PQ_SRC_INDEX_AV_PAL60,
    PQ_SRC_INDEX_AV_NTSC50,
    PQ_SRC_INDEX_AV_PALN,

    PQ_SRC_INDEX_SV_NTSC,
    PQ_SRC_INDEX_SV_PAL,
    PQ_SRC_INDEX_SV_PAL_M,
    PQ_SRC_INDEX_SV_SECAM,

    PQ_SRC_INDEX_YCbCr_480I,
    PQ_SRC_INDEX_YCbCr_576I,
    PQ_SRC_INDEX_YCbCr_480P,
    PQ_SRC_INDEX_YCbCr_576P,
    PQ_SRC_INDEX_YCbCr_720P,
    PQ_SRC_INDEX_YCbCr_1080I,
    PQ_SRC_INDEX_YCbCr_1080P,

    PQ_SRC_INDEX_HDMI_480I,
    PQ_SRC_INDEX_HDMI_576I,
    PQ_SRC_INDEX_HDMI_480P,
    PQ_SRC_INDEX_HDMI_576P,
    PQ_SRC_INDEX_HDMI_720P,
    PQ_SRC_INDEX_HDMI_1080I,
    PQ_SRC_INDEX_HDMI_1080P,
    PQ_SRC_INDEX_HDMI_4K2KI,
    PQ_SRC_INDEX_HDMI_4K2KP,
    PQ_SRC_INDEX_HDR10_HDMI_480I,
    PQ_SRC_INDEX_HDR10_HDMI_576I,
    PQ_SRC_INDEX_HDR10_HDMI_480P,
    PQ_SRC_INDEX_HDR10_HDMI_576P,
    PQ_SRC_INDEX_HDR10_HDMI_720P,
    PQ_SRC_INDEX_HDR10_HDMI_1080I,
    PQ_SRC_INDEX_HDR10_HDMI_1080P,
    PQ_SRC_INDEX_HDR10_HDMI_4K2KI,
    PQ_SRC_INDEX_HDR10_HDMI_4K2KP,
    PQ_SRC_INDEX_HLG_HDMI_480I,
    PQ_SRC_INDEX_HLG_HDMI_576I,
    PQ_SRC_INDEX_HLG_HDMI_480P,
    PQ_SRC_INDEX_HLG_HDMI_576P,
    PQ_SRC_INDEX_HLG_HDMI_720P,
    PQ_SRC_INDEX_HLG_HDMI_1080I,
    PQ_SRC_INDEX_HLG_HDMI_1080P,
    PQ_SRC_INDEX_HLG_HDMI_4K2KI,
    PQ_SRC_INDEX_HLG_HDMI_4K2KP,
    PQ_SRC_INDEX_DV_HDMI_480I,
    PQ_SRC_INDEX_DV_HDMI_576I,
    PQ_SRC_INDEX_DV_HDMI_480P,
    PQ_SRC_INDEX_DV_HDMI_576P,
    PQ_SRC_INDEX_DV_HDMI_720P,
    PQ_SRC_INDEX_DV_HDMI_1080I,
    PQ_SRC_INDEX_DV_HDMI_1080P,
    PQ_SRC_INDEX_DV_HDMI_4K2KI,
    PQ_SRC_INDEX_DV_HDMI_4K2KP,
    PQ_SRC_INDEX_HDR10P_HDMI_480I,
    PQ_SRC_INDEX_HDR10P_HDMI_576I,
    PQ_SRC_INDEX_HDR10P_HDMI_480P,
    PQ_SRC_INDEX_HDR10P_HDMI_576P,
    PQ_SRC_INDEX_HDR10P_HDMI_720P,
    PQ_SRC_INDEX_HDR10P_HDMI_1080I,
    PQ_SRC_INDEX_HDR10P_HDMI_1080P,
    PQ_SRC_INDEX_HDR10P_HDMI_4K2KI,
    PQ_SRC_INDEX_HDR10P_HDMI_4K2KP,

    PQ_SRC_INDEX_DTV_480I,
    PQ_SRC_INDEX_DTV_576I,
    PQ_SRC_INDEX_DTV_480P,
    PQ_SRC_INDEX_DTV_576P,
    PQ_SRC_INDEX_DTV_720P,
    PQ_SRC_INDEX_DTV_1080I,
    PQ_SRC_INDEX_DTV_1080P,
    PQ_SRC_INDEX_DTV_4k2kI,
    PQ_SRC_INDEX_DTV_4k2kP,

    PQ_SRC_INDEX_HDR10_DTV_480I,
    PQ_SRC_INDEX_HDR10_DTV_576I,
    PQ_SRC_INDEX_HDR10_DTV_480P,
    PQ_SRC_INDEX_HDR10_DTV_576P,
    PQ_SRC_INDEX_HDR10_DTV_720P,
    PQ_SRC_INDEX_HDR10_DTV_1080I,
    PQ_SRC_INDEX_HDR10_DTV_1080P,
    PQ_SRC_INDEX_HDR10_DTV_4K2KI,
    PQ_SRC_INDEX_HDR10_DTV_4K2KP,

    PQ_SRC_INDEX_HLG_DTV_480I,
    PQ_SRC_INDEX_HLG_DTV_576I,
    PQ_SRC_INDEX_HLG_DTV_480P,
    PQ_SRC_INDEX_HLG_DTV_576P,
    PQ_SRC_INDEX_HLG_DTV_720P,
    PQ_SRC_INDEX_HLG_DTV_1080I,
    PQ_SRC_INDEX_HLG_DTV_1080P,
    PQ_SRC_INDEX_HLG_DTV_4K2KI,
    PQ_SRC_INDEX_HLG_DTV_4K2KP,

    PQ_SRC_INDEX_HDR10P_DTV_480I,
    PQ_SRC_INDEX_HDR10P_DTV_576I,
    PQ_SRC_INDEX_HDR10P_DTV_480P,
    PQ_SRC_INDEX_HDR10P_DTV_576P,
    PQ_SRC_INDEX_HDR10P_DTV_720P,
    PQ_SRC_INDEX_HDR10P_DTV_1080I,
    PQ_SRC_INDEX_HDR10P_DTV_1080P,
    PQ_SRC_INDEX_HDR10P_DTV_4K2KI,
    PQ_SRC_INDEX_HDR10P_DTV_4K2KP,

    PQ_SRC_INDEX_DV_DTV_480I,
    PQ_SRC_INDEX_DV_DTV_576I,
    PQ_SRC_INDEX_DV_DTV_480P,
    PQ_SRC_INDEX_DV_DTV_576P,
    PQ_SRC_INDEX_DV_DTV_720P,
    PQ_SRC_INDEX_DV_DTV_1080I,
    PQ_SRC_INDEX_DV_DTV_1080P,
    PQ_SRC_INDEX_DV_DTV_4K2KI,
    PQ_SRC_INDEX_DV_DTV_4K2KP,

    PQ_SRC_INDEX_MPEG_480I,
    PQ_SRC_INDEX_MPEG_576I,
    PQ_SRC_INDEX_MPEG_480P,
    PQ_SRC_INDEX_MPEG_576P,
    PQ_SRC_INDEX_MPEG_720P,
    PQ_SRC_INDEX_MPEG_1080I,
    PQ_SRC_INDEX_MPEG_1080P,
    PQ_SRC_INDEX_MPEG_4K2KI,
    PQ_SRC_INDEX_MPEG_4K2KP,

    PQ_SRC_INDEX_HDR10_MPEG_480I,
    PQ_SRC_INDEX_HDR10_MPEG_576I,
    PQ_SRC_INDEX_HDR10_MPEG_480P,
    PQ_SRC_INDEX_HDR10_MPEG_576P,
    PQ_SRC_INDEX_HDR10_MPEG_720P,
    PQ_SRC_INDEX_HDR10_MPEG_1080I,
    PQ_SRC_INDEX_HDR10_MPEG_1080P,
    PQ_SRC_INDEX_HDR10_MPEG_4K2KI,
    PQ_SRC_INDEX_HDR10_MPEG_4K2KP,

    PQ_SRC_INDEX_HLG_MPEG_480I,
    PQ_SRC_INDEX_HLG_MPEG_576I,
    PQ_SRC_INDEX_HLG_MPEG_480P,
    PQ_SRC_INDEX_HLG_MPEG_576P,
    PQ_SRC_INDEX_HLG_MPEG_720P,
    PQ_SRC_INDEX_HLG_MPEG_1080I,
    PQ_SRC_INDEX_HLG_MPEG_1080P,
    PQ_SRC_INDEX_HLG_MPEG_4K2KI,
    PQ_SRC_INDEX_HLG_MPEG_4K2KP,

    PQ_SRC_INDEX_HDR10P_MPEG_480I,
    PQ_SRC_INDEX_HDR10P_MPEG_576I,
    PQ_SRC_INDEX_HDR10P_MPEG_480P,
    PQ_SRC_INDEX_HDR10P_MPEG_576P,
    PQ_SRC_INDEX_HDR10P_MPEG_720P,
    PQ_SRC_INDEX_HDR10P_MPEG_1080I,
    PQ_SRC_INDEX_HDR10P_MPEG_1080P,
    PQ_SRC_INDEX_HDR10P_MPEG_4K2KI,
    PQ_SRC_INDEX_HDR10P_MPEG_4K2KP,

    PQ_SRC_INDEX_DV_MPEG_480I,
    PQ_SRC_INDEX_DV_MPEG_576I,
    PQ_SRC_INDEX_DV_MPEG_480P,
    PQ_SRC_INDEX_DV_MPEG_576P,
    PQ_SRC_INDEX_DV_MPEG_720P,
    PQ_SRC_INDEX_DV_MPEG_1080I,
    PQ_SRC_INDEX_DV_MPEG_1080P,
    PQ_SRC_INDEX_DV_MPEG_4K2KI,
    PQ_SRC_INDEX_DV_MPEG_4K2KP,

    PQ_SRC_INDEX_MAX,
} PQ_SOURCE_TIMING;

typedef struct pq_dnlp_curve_param_s {
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
} pq_dnlp_curve_param_t;

typedef struct pq_ve_lc_curve_parm_s {
    unsigned int ve_lc_saturation[63];
    unsigned int ve_lc_yminval_lmt[16];
    unsigned int ve_lc_ypkbv_ymaxval_lmt[16];
    unsigned int ve_lc_ymaxval_lmt[16];
    unsigned int ve_lc_ypkbv_lmt[16];
    unsigned int ve_lc_ypkbv_ratio[4];
    unsigned int param[100];
} pq_ve_lc_curve_parm_t;

typedef struct pq_ve_lc_reg_parm_s {
    unsigned char lc_enable;
    unsigned char lc_blkblend_mode;
    unsigned char lc_blk_hnum;
    unsigned char lc_blk_vnum;
    unsigned char lc_curve_nodes_hlpf;
    unsigned char lc_curve_nodes_vlpf;
    unsigned char lc_hblank;
    unsigned char lcinput_ysel;
    unsigned char lcinput_csel;
    unsigned char lc_slope_min;
    unsigned char lc_slope_max;
    unsigned char lc_slope_max_face;
    unsigned char lmtrat_minmax;
    unsigned char lmtrat_valid;
    unsigned char lc_num_m_coring;
    unsigned short lc_cntst_gain_low;
    unsigned char lc_cntst_scale_low;
    unsigned char lc_cntstbvn_low;
    unsigned char lc_cntst_lmt_low_l;
    unsigned char lc_cntst_lmt_low_h;
    unsigned short lc_cntst_gain_hig;
    unsigned char lc_cntst_scale_hig;
    unsigned char lc_cntstbvn_hig;
    unsigned char lc_cntst_lmt_hig_l;
    unsigned char lc_cntst_lmt_hig_h;
} pq_ve_lc_reg_parm_t;

typedef struct pq_hdr_tmo_sw_s {
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
}pq_hdr_tmo_sw_t;

typedef enum ai_pq_rows_s {
    AI_PQ_ROWS_SKIN = 0,
    AI_PQ_ROWS_BLUESKY,
    AI_PQ_ROWS_FOODS,
    AI_PQ_ROWS_ARCHITECTURE,
    AI_PQ_ROWS_GRASS,
    AI_PQ_ROWS_NIGHT,
    AI_PQ_ROWS_DOCUMENT,
    AI_PQ_ROWS_MAX,
} ai_pq_rows_t;

typedef enum ai_pq_cols_s {
    AI_PQ_COLS_BLUE = 0,
    AI_PQ_COLS_GREEN,
    AI_PQ_COLS_SKINTONE,
    AI_PQ_COLS_PEAKING,
    AI_PQ_COLS_SAT,
    AI_PQ_COLS_CONTRAST,
    AI_PQ_COLS_MAX,
} ai_pq_cols_t;

typedef struct pq_ai_pq_size_s {
    unsigned int height;
    unsigned int width;
}pq_ai_pq_size_t;

typedef struct pq_ai_pq_para_s {
    pq_ai_pq_size_t aiSize;
    unsigned int ai_pq_table[AI_PQ_ROWS_MAX][AI_PQ_COLS_MAX];
}pq_ai_pq_para_t;

typedef struct _PQ_TABLE_PARAM {
    unsigned char PQ_Index_Table0[PQ_SRC_INDEX_MAX][PQ_INDEX_TABLE_SIZE];
    unsigned char PQ_Index_Table1[PQ_SRC_INDEX_MAX][PQ_INDEX_TABLE_SIZE];
    pq_dnlp_curve_param_t DNLPTable[DNLP_TABLE_NUM_MAX];
    pq_hdr_tmo_sw_t HDRToneMappingTable[HDR_TONEMAPPING_TABLE_NUM_MAX];
    pq_ai_pq_para_t AIPQ_Table[AI_PQ_TABLE_NUM_MAX];
    pq_ve_lc_curve_parm_t LC_NODE_Table[LC_TABLE_NUM_MAX][VPP_PQ_LV_MAX];
    pq_ve_lc_reg_parm_t LC_REG_Table[LC_TABLE_NUM_MAX][VPP_PQ_LV_MAX];
}PQ_TABLE_PARAM;


#ifdef __cplusplus
}
#endif
#endif
