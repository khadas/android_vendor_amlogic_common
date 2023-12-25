#ifndef _AML_HAL_VPQ_H_
#define _AML_HAL_VPQ_H_

#include <halcommon.h>

#ifdef  __cplusplus
extern "C"
{
#endif


/**
*** definition.
**/
#define AML_HAL_PRE_GAMMA_TABLE_LEN    65
#define AML_HAL_HIST_BIN_COUNT         64
#define AML_HAL_COLOR_HIST_BIN_COUNT   32
#define AML_HAL_MTRX_OFFSET_LEN        3
#define AML_HAL_MTRX_COEF_LEN          9

typedef struct _HAL_MAGIC_COLORTEMP_OFFSET_DATA {
    short R_offset[11];
    short G_offset[11];
    short B_offset[11];
} HAL_MAGIC_COLORTEMP_OFFSET_DATA;

typedef struct aml_hal_white_balance_s {
    int R_val;
    int G_val;
    int B_val;
    int R_offset_val;
    int G_offset_val;
    int B_offset_val;
    HAL_MAGIC_COLORTEMP_OFFSET_DATA magicOffset;
    unsigned char  gamma_curve_index;
} aml_hal_white_balance_t;

typedef struct aml_hal_pre_gamma_table_s {
    UINT32 r_data[AML_HAL_PRE_GAMMA_TABLE_LEN];
    UINT32 g_data[AML_HAL_PRE_GAMMA_TABLE_LEN];
    UINT32 b_data[AML_HAL_PRE_GAMMA_TABLE_LEN];
} aml_hal_pre_gamma_table_t;

typedef enum _aml_hal_gamma_curve_e {
    AML_HAL_GAMMA_CURVE_DEFAULT,
    AML_HAL_GAMMA_CURVE_1,
    AML_HAL_GAMMA_CURVE_2,
    AML_HAL_GAMMA_CURVE_3,
    AML_HAL_GAMMA_CURVE_4,
    AML_HAL_GAMMA_CURVE_5,
    AML_HAL_GAMMA_CURVE_6,
    AML_HAL_GAMMA_CURVE_7,
    AML_HAL_GAMMA_CURVE_8,
    AML_HAL_GAMMA_CURVE_9,
    AML_HAL_GAMMA_CURVE_10,
    AML_HAL_GAMMA_CURVE_11,
    AML_HAL_GAMMA_CURVE_MAX,
} aml_hal_gamma_curve_e;

typedef enum _aml_hal_module_e {
    AML_HAL_MODULE_VADJ1 = 0,
    AML_HAL_MODULE_VADJ2,
    AML_HAL_MODULE_PREGAMMA,
    AML_HAL_MODULE_GAMMA,
    AML_HAL_MODULE_WB,
    AML_HAL_MODULE_DNLP,
    AML_HAL_MODULE_CCORING,
    AML_HAL_MODULE_SR0,
    AML_HAL_MODULE_SR0_DNLP,
    AML_HAL_MODULE_SR1,
    AML_HAL_MODULE_SR1_DNLP,
    AML_HAL_MODULE_LC,
    AML_HAL_MODULE_CM,
    AML_HAL_MODULE_BLE,
    AML_HAL_MODULE_BLS,
    AML_HAL_MODULE_LUT3D,
    AML_HAL_MODULE_ALL,
} aml_hal_module_e;

typedef struct aml_hal_module_ctrl_s {
    aml_hal_module_e module_type;
    SINT32 status;
} aml_hal_module_ctrl_t;

typedef struct aml_hal_pq_ctrl_s {
    UINT8 vadj1_en;    /*control video brightness contrast saturation hue*/
    UINT8 vd1_ctrst_en;
    UINT8 vadj2_en;    /*control video+osd brightness contrast saturation hue*/
    UINT8 post_ctrst_en;
    UINT8 pregamma_en;
    UINT8 gamma_en;
    UINT8 wb_en;
    UINT8 dnlp_en;
    UINT8 lc_en;
    UINT8 black_ext_en;
    UINT8 chroma_cor_en;
    UINT8 sharpness0_en;
    UINT8 sharpness1_en;
    UINT8 cm_en;
    UINT8 reserved;
} aml_hal_pq_ctrl_t;

typedef struct aml_hal_pq_state_s {
    SINT32 pq_en;
    aml_hal_pq_ctrl_t pq_cfg;
} aml_hal_pq_state_t;

typedef enum _aml_hal_pc_mode_e {
    AML_HAL_PC_MODE_OFF = 0,
    AML_HAL_PC_MODE_ON,
} aml_hal_pc_mode_e;

typedef enum _aml_hal_pq_dc_mode_e {
    AML_HAL_DC_OFF = 0,
    AML_HAL_DC_LOW,
    AML_HAL_DC_MID,
    AML_HAL_DC_HIGH,
} aml_hal_pq_dc_mode_e;

typedef enum _aml_hal_pq_lc_mode_e {
    AML_HAL_LC_OFF = 0,
    AML_HAL_LC_LOW,
    AML_HAL_LC_MID,
    AML_HAL_LC_HIGH,
    AML_HAL_LC_MAX,
} aml_hal_pq_lc_mode_e;

typedef enum _aml_hal_csc_type_e {
    AML_HAL_CSC_MATRIX_NULL                = 0,
    AML_HAL_CSC_MATRIX_RGB_YUV601          = 0x1,
    AML_HAL_CSC_MATRIX_RGB_YUV601F         = 0x2,
    AML_HAL_CSC_MATRIX_RGB_YUV709          = 0x3,
    AML_HAL_CSC_MATRIX_RGB_YUV709F         = 0x4,
    AML_HAL_CSC_MATRIX_YUV601_RGB          = 0x10,
    AML_HAL_CSC_MATRIX_YUV601_YUV601F      = 0x11,
    AML_HAL_CSC_MATRIX_YUV601_YUV709       = 0x12,
    AML_HAL_CSC_MATRIX_YUV601_YUV709F      = 0x13,
    AML_HAL_CSC_MATRIX_YUV601F_RGB         = 0x14,
    AML_HAL_CSC_MATRIX_YUV601F_YUV601      = 0x15,
    AML_HAL_CSC_MATRIX_YUV601F_YUV709      = 0x16,
    AML_HAL_CSC_MATRIX_YUV601F_YUV709F     = 0x17,
    AML_HAL_CSC_MATRIX_YUV709_RGB          = 0x20,
    AML_HAL_CSC_MATRIX_YUV709_YUV601       = 0x21,
    AML_HAL_CSC_MATRIX_YUV709_YUV601F      = 0x22,
    AML_HAL_CSC_MATRIX_YUV709_YUV709F      = 0x23,
    AML_HAL_CSC_MATRIX_YUV709F_RGB         = 0x24,
    AML_HAL_CSC_MATRIX_YUV709F_YUV601      = 0x25,
    AML_HAL_CSC_MATRIX_YUV709F_YUV709      = 0x26,
    AML_HAL_CSC_MATRIX_YUV601L_YUV709L     = 0x27,
    AML_HAL_CSC_MATRIX_YUV709L_YUV601L     = 0x28,
    AML_HAL_CSC_MATRIX_YUV709F_YUV601F     = 0x29,
    AML_HAL_CSC_MATRIX_BT2020YUV_BT2020RGB = 0x40,
    AML_HAL_CSC_MATRIX_BT2020RGB_709RGB,
    AML_HAL_CSC_MATRIX_BT2020RGB_CUSRGB,
    AML_HAL_CSC_MATRIX_DEFAULT_CSCTYPE     = 0xffff,
} aml_hal_csc_type_e;

typedef enum _aml_hal_hdr_type_e {
    AML_HAL_TYPE_NONE = 0,
    AML_HAL_TYPE_SDR,
    AML_HAL_TYPE_HDR10,
    AML_HAL_TYPE_HLG,
    AML_HAL_TYPE_HDR10PLUS,
    AML_HAL_TYPE_DOBVI,
    AML_HAL_TYPE_MVC,
    AML_HAL_TYPE_CUVA_HDR,
    AML_HAL_TYPE_CUVA_HLG,
} aml_hal_hdr_type_e;

typedef enum _aml_hal_color_primary_e {
    AML_HAL_COLOR_PRI_NULL = 0,
    AML_HAL_COLOR_PRI_BT601,
    AML_HAL_COLOR_PRI_BT709,
    AML_HAL_COLOR_PRI_BT2020,
    AML_HAL_COLOR_PRI_MAX,
} aml_hal_color_primary_e;

/*master_display_info for display device*/
typedef struct aml_hal_hdr_metadata_s {
    UINT32 primaries[3][2]; /*normalized 50000 in G,B,R order*/
    UINT32 white_point[2];  /*normalized 50000*/
    UINT32 luminance[2];    /*max/min luminance, normalized 10000*/
} aml_hal_hdr_metadata_t;

typedef struct aml_hal_histgm_ave_s {
    UINT32 sum;
    SINT32 width;
    SINT32 height;
    SINT32 ave;
} aml_hal_histgm_ave_t;

typedef struct aml_hal_histgm_param_s {
    UINT32 hist_pow;
    UINT32 luma_sum;
    UINT32 pixel_sum;
    UINT32 histgm[AML_HAL_HIST_BIN_COUNT];
    UINT32 hue_histgm[AML_HAL_COLOR_HIST_BIN_COUNT];
    UINT32 sat_histgm[AML_HAL_COLOR_HIST_BIN_COUNT];
} aml_hal_histgm_param_t;

typedef enum _adm_hal_mtrx_type_e {
    AML_HAL_MTRX_VD1 = 0,
    AML_HAL_MTRX_POST,
    AML_HAL_MTRX_POST2,
    AML_HAL_MTRX_MAX,
} adm_hal_mtrx_type_e;

typedef struct aml_hal_mtrx_param_s {
    UINT32 pre_offset[AML_HAL_MTRX_OFFSET_LEN];
    UINT32 matrix_coef[AML_HAL_MTRX_COEF_LEN];
    UINT32 post_offset[AML_HAL_MTRX_OFFSET_LEN];
    UINT32 right_shift;
} aml_hal_mtrx_param_t;

typedef struct aml_hal_mtrx_info_s {
    adm_hal_mtrx_type_e mtrx_sel;
    aml_hal_mtrx_param_t mtrx_param;
} aml_hal_mtrx_info_t;

typedef enum _aml_hal_pq_source_timing_e {
    AML_HAL_SRC_INDEX_VGA = 0,

    AML_HAL_SRC_INDEX_ATV_NTSC,
    AML_HAL_SRC_INDEX_ATV_PAL,
    AML_HAL_SRC_INDEX_ATV_PAL_M,
    AML_HAL_SRC_INDEX_ATV_SECAN,
    AML_HAL_SRC_INDEX_ATV_NTSC443,
    AML_HAL_SRC_INDEX_ATV_PAL60,
    AML_HAL_SRC_INDEX_ATV_NTSC50,
    AML_HAL_SRC_INDEX_ATV_PALN,

    AML_HAL_SRC_INDEX_AV_NTSC,
    AML_HAL_SRC_INDEX_AV_PAL,
    AML_HAL_SRC_INDEX_AV_PAL_M,
    AML_HAL_SRC_INDEX_AV_SECAN,
    AML_HAL_SRC_INDEX_AV_NTSC443,
    AML_HAL_SRC_INDEX_AV_PAL60,
    AML_HAL_SRC_INDEX_AV_NTSC50,
    AML_HAL_SRC_INDEX_AV_PALN,

    AML_HAL_SRC_INDEX_SV_NTSC,
    AML_HAL_SRC_INDEX_SV_PAL,
    AML_HAL_SRC_INDEX_SV_PAL_M,
    AML_HAL_SRC_INDEX_SV_SECAM,

    AML_HAL_SRC_INDEX_YCbCr_480I,
    AML_HAL_SRC_INDEX_YCbCr_576I,
    AML_HAL_SRC_INDEX_YCbCr_480P,
    AML_HAL_SRC_INDEX_YCbCr_576P,
    AML_HAL_SRC_INDEX_YCbCr_720P,
    AML_HAL_SRC_INDEX_YCbCr_1080I,
    AML_HAL_SRC_INDEX_YCbCr_1080P,

    AML_HAL_SRC_INDEX_HDMI_480I,
    AML_HAL_SRC_INDEX_HDMI_576I,
    AML_HAL_SRC_INDEX_HDMI_480P,
    AML_HAL_SRC_INDEX_HDMI_576P,
    AML_HAL_SRC_INDEX_HDMI_720P,
    AML_HAL_SRC_INDEX_HDMI_1080I,
    AML_HAL_SRC_INDEX_HDMI_1080P,
    AML_HAL_SRC_INDEX_HDMI_4K2KI,
    AML_HAL_SRC_INDEX_HDMI_4K2KP,
    AML_HAL_SRC_INDEX_HDR10_HDMI_480I,
    AML_HAL_SRC_INDEX_HDR10_HDMI_576I,
    AML_HAL_SRC_INDEX_HDR10_HDMI_480P,
    AML_HAL_SRC_INDEX_HDR10_HDMI_576P,
    AML_HAL_SRC_INDEX_HDR10_HDMI_720P,
    AML_HAL_SRC_INDEX_HDR10_HDMI_1080I,
    AML_HAL_SRC_INDEX_HDR10_HDMI_1080P,
    AML_HAL_SRC_INDEX_HDR10_HDMI_4K2KI,
    AML_HAL_SRC_INDEX_HDR10_HDMI_4K2KP,
    AML_HAL_SRC_INDEX_HLG_HDMI_480I,
    AML_HAL_SRC_INDEX_HLG_HDMI_576I,
    AML_HAL_SRC_INDEX_HLG_HDMI_480P,
    AML_HAL_SRC_INDEX_HLG_HDMI_576P,
    AML_HAL_SRC_INDEX_HLG_HDMI_720P,
    AML_HAL_SRC_INDEX_HLG_HDMI_1080I,
    AML_HAL_SRC_INDEX_HLG_HDMI_1080P,
    AML_HAL_SRC_INDEX_HLG_HDMI_4K2KI,
    AML_HAL_SRC_INDEX_HLG_HDMI_4K2KP,
    AML_HAL_SRC_INDEX_DV_HDMI_480I,
    AML_HAL_SRC_INDEX_DV_HDMI_576I,
    AML_HAL_SRC_INDEX_DV_HDMI_480P,
    AML_HAL_SRC_INDEX_DV_HDMI_576P,
    AML_HAL_SRC_INDEX_DV_HDMI_720P,
    AML_HAL_SRC_INDEX_DV_HDMI_1080I,
    AML_HAL_SRC_INDEX_DV_HDMI_1080P,
    AML_HAL_SRC_INDEX_DV_HDMI_4K2KI,
    AML_HAL_SRC_INDEX_DV_HDMI_4K2KP,
    AML_HAL_SRC_INDEX_HDR10P_HDMI_480I,
    AML_HAL_SRC_INDEX_HDR10P_HDMI_576I,
    AML_HAL_SRC_INDEX_HDR10P_HDMI_480P,
    AML_HAL_SRC_INDEX_HDR10P_HDMI_576P,
    AML_HAL_SRC_INDEX_HDR10P_HDMI_720P,
    AML_HAL_SRC_INDEX_HDR10P_HDMI_1080I,
    AML_HAL_SRC_INDEX_HDR10P_HDMI_1080P,
    AML_HAL_SRC_INDEX_HDR10P_HDMI_4K2KI,
    AML_HAL_SRC_INDEX_HDR10P_HDMI_4K2KP,

    AML_HAL_SRC_INDEX_DTV_480I,
    AML_HAL_SRC_INDEX_DTV_576I,
    AML_HAL_SRC_INDEX_DTV_480P,
    AML_HAL_SRC_INDEX_DTV_576P,
    AML_HAL_SRC_INDEX_DTV_720P,
    AML_HAL_SRC_INDEX_DTV_1080I,
    AML_HAL_SRC_INDEX_DTV_1080P,
    AML_HAL_SRC_INDEX_DTV_4K2KI,
    AML_HAL_SRC_INDEX_DTV_4K2KP,

    AML_HAL_SRC_INDEX_HDR10_DTV_480I,
    AML_HAL_SRC_INDEX_HDR10_DTV_576I,
    AML_HAL_SRC_INDEX_HDR10_DTV_480P,
    AML_HAL_SRC_INDEX_HDR10_DTV_576P,
    AML_HAL_SRC_INDEX_HDR10_DTV_720P,
    AML_HAL_SRC_INDEX_HDR10_DTV_1080I,
    AML_HAL_SRC_INDEX_HDR10_DTV_1080P,
    AML_HAL_SRC_INDEX_HDR10_DTV_4K2KI,
    AML_HAL_SRC_INDEX_HDR10_DTV_4K2KP,

    AML_HAL_SRC_INDEX_HLG_DTV_480I,
    AML_HAL_SRC_INDEX_HLG_DTV_576I,
    AML_HAL_SRC_INDEX_HLG_DTV_480P,
    AML_HAL_SRC_INDEX_HLG_DTV_576P,
    AML_HAL_SRC_INDEX_HLG_DTV_720P,
    AML_HAL_SRC_INDEX_HLG_DTV_1080I,
    AML_HAL_SRC_INDEX_HLG_DTV_1080P,
    AML_HAL_SRC_INDEX_HLG_DTV_4K2KI,
    AML_HAL_SRC_INDEX_HLG_DTV_4K2KP,

    AML_HAL_SRC_INDEX_HDR10P_DTV_480I,
    AML_HAL_SRC_INDEX_HDR10P_DTV_576I,
    AML_HAL_SRC_INDEX_HDR10P_DTV_480P,
    AML_HAL_SRC_INDEX_HDR10P_DTV_576P,
    AML_HAL_SRC_INDEX_HDR10P_DTV_720P,
    AML_HAL_SRC_INDEX_HDR10P_DTV_1080I,
    AML_HAL_SRC_INDEX_HDR10P_DTV_1080P,
    AML_HAL_SRC_INDEX_HDR10P_DTV_4K2KI,
    AML_HAL_SRC_INDEX_HDR10P_DTV_4K2KP,

    AML_HAL_SRC_INDEX_DV_DTV_480I,
    AML_HAL_SRC_INDEX_DV_DTV_576I,
    AML_HAL_SRC_INDEX_DV_DTV_480P,
    AML_HAL_SRC_INDEX_DV_DTV_576P,
    AML_HAL_SRC_INDEX_DV_DTV_720P,
    AML_HAL_SRC_INDEX_DV_DTV_1080I,
    AML_HAL_SRC_INDEX_DV_DTV_1080P,
    AML_HAL_SRC_INDEX_DV_DTV_4K2KI,
    AML_HAL_SRC_INDEX_DV_DTV_4K2KP,

    AML_HAL_SRC_INDEX_MPEG_480I,
    AML_HAL_SRC_INDEX_MPEG_576I,
    AML_HAL_SRC_INDEX_MPEG_480P,
    AML_HAL_SRC_INDEX_MPEG_576P,
    AML_HAL_SRC_INDEX_MPEG_720P,
    AML_HAL_SRC_INDEX_MPEG_1080I,
    AML_HAL_SRC_INDEX_MPEG_1080P,
    AML_HAL_SRC_INDEX_MPEG_4K2KI,
    AML_HAL_SRC_INDEX_MPEG_4K2KP,

    AML_HAL_SRC_INDEX_HDR10_MPEG_480I,
    AML_HAL_SRC_INDEX_HDR10_MPEG_576I,
    AML_HAL_SRC_INDEX_HDR10_MPEG_480P,
    AML_HAL_SRC_INDEX_HDR10_MPEG_576P,
    AML_HAL_SRC_INDEX_HDR10_MPEG_720P,
    AML_HAL_SRC_INDEX_HDR10_MPEG_1080I,
    AML_HAL_SRC_INDEX_HDR10_MPEG_1080P,
    AML_HAL_SRC_INDEX_HDR10_MPEG_4K2KI,
    AML_HAL_SRC_INDEX_HDR10_MPEG_4K2KP,

    AML_HAL_SRC_INDEX_HLG_MPEG_480I,
    AML_HAL_SRC_INDEX_HLG_MPEG_576I,
    AML_HAL_SRC_INDEX_HLG_MPEG_480P,
    AML_HAL_SRC_INDEX_HLG_MPEG_576P,
    AML_HAL_SRC_INDEX_HLG_MPEG_720P,
    AML_HAL_SRC_INDEX_HLG_MPEG_1080I,
    AML_HAL_SRC_INDEX_HLG_MPEG_1080P,
    AML_HAL_SRC_INDEX_HLG_MPEG_4K2KI,
    AML_HAL_SRC_INDEX_HLG_MPEG_4K2KP,

    AML_HAL_SRC_INDEX_HDR10P_MPEG_480I,
    AML_HAL_SRC_INDEX_HDR10P_MPEG_576I,
    AML_HAL_SRC_INDEX_HDR10P_MPEG_480P,
    AML_HAL_SRC_INDEX_HDR10P_MPEG_576P,
    AML_HAL_SRC_INDEX_HDR10P_MPEG_720P,
    AML_HAL_SRC_INDEX_HDR10P_MPEG_1080I,
    AML_HAL_SRC_INDEX_HDR10P_MPEG_1080P,
    AML_HAL_SRC_INDEX_HDR10P_MPEG_4K2KI,
    AML_HAL_SRC_INDEX_HDR10P_MPEG_4K2KP,

    AML_HAL_SRC_INDEX_DV_MPEG_480I,
    AML_HAL_SRC_INDEX_DV_MPEG_576I,
    AML_HAL_SRC_INDEX_DV_MPEG_480P,
    AML_HAL_SRC_INDEX_DV_MPEG_576P,
    AML_HAL_SRC_INDEX_DV_MPEG_720P,
    AML_HAL_SRC_INDEX_DV_MPEG_1080I,
    AML_HAL_SRC_INDEX_DV_MPEG_1080P,
    AML_HAL_SRC_INDEX_DV_MPEG_4K2KI,
    AML_HAL_SRC_INDEX_DV_MPEG_4K2KP,

    AML_HAL_SRC_INDEX_MAX,
} aml_hal_pq_source_timing_e;

/*****************************************************base tvfuse videodev-ext.h*************************************************/
#define V4L2_EXT_GAMUT_MATRIX_PRE_SIZE  9
#define MAX_EXT_PATTERN_GRADATION_LINE  4 //it may depends on chip limitation
#define GAMMA_LUT_TABLE_NUM             1024
#define MAX_EXT_PATTERN_WINBOX          10 //it may depends on chip limitation

typedef enum _aml_hal_cm_perferred_color_type { //v4l2_ext_cm_perferred_color_type
    AML_HAL_CM_PREFERRED_COLOR_SKIN = 0,
    AML_HAL_CM_PREFERRED_COLOR_GRASS,
    AML_HAL_CM_PREFERRED_COLOR_SKYBLUE,
    AML_HAL_CM_PREFERRED_COLOR_MAX,
} aml_hal_cm_perferred_color_type;

typedef enum _aml_hal_cm_dc_color_level_e { //v4l2_ext_cm_dynamic_color_level
    AML_HAL_CM_DYNAMIC_COLOR_OFF = 0,
    AML_HAL_CM_DYNAMIC_COLOR_LOW,
    AML_HAL_CM_DYNAMIC_COLOR_MEDIUM,
    AML_HAL_CM_DYNAMIC_COLOR_HIGH,
    AML_HAL_CM_DYNAMIC_COLOR_INVALID,
} aml_hal_cm_dc_color_level_e;

typedef enum _aml_hal_cm_cmd_color_type { // v4l2_ext_cm_cms_color_type
    AML_HAL_CM_CMS_RED = 0,
    AML_HAL_CM_CMS_GREEN,
    AML_HAL_CM_CMS_BLUE,
    AML_HAL_CM_CMS_CYAN,
    AML_HAL_CM_CMS_MAGENTA,
    AML_HAL_CM_CMS_YELLOW,
    AML_HAL_CM_CMS_MAX,
} aml_hal_cm_cmd_color_type;

typedef struct amd_hal_dc_color_ui_s { //v4l2_ext_cm_dynamic_color_ui
    UINT8 enable; // 0: don't reference preferred_value(expert_control),
    // 1: reference preferred_value(not_use_cm_db==1 &&
    // advanced_control)
    aml_hal_cm_dc_color_level_e value; // ui value
} amd_hal_dc_color_ui_t;

typedef struct aml_hal_cm_perferred_color_ui_s { //v4l2_ext_cm_perferred_color_ui
    UINT8 enable; // 0: don't reference preferred_value(expert_control),
                          // 1: reference preferred_value(advanced_control)
    SINT8 value[AML_HAL_CM_PREFERRED_COLOR_MAX]; // ui value per each color_type
} aml_hal_cm_perferred_color_ui_t;

typedef struct aml_hal_cm_cms_ui_s { //v4l2_ext_cm_cms_ui
    UINT8 enable;   // 0: don't reference value(advanced_control),  1:
    // reference value(not_use_cm_db==1 && expert_control)
    SINT8 gain_saturation[AML_HAL_CM_CMS_MAX];  // ui value per each color_type
    SINT8 gain_hue[AML_HAL_CM_CMS_MAX]; // ui value per each color_type
    SINT8 gain_luminance[AML_HAL_CM_CMS_MAX]; // ui value per each color_type
} aml_hal_cm_cms_ui_t;

typedef struct amd_hal_cm_ui_status_s { //v4l2_ext_cm_ui_status
    amd_hal_dc_color_ui_t dynamic;
    aml_hal_cm_perferred_color_ui_t preferred;
    aml_hal_cm_cms_ui_t cms;
} amd_hal_cm_ui_status_t;

typedef struct aml_hal_cmd_info_s { //v4l2_ext_cm_info
    UINT8
    use_internal_cm_db; // 0: use dbInfo, 1: use driver internal db
    amd_hal_cm_ui_status_t uiInfo;
    union {
        UINT8 *dbInfo; ///< p_data
        UINT32 compat_data;
        UINT64 sizer;
    };
} aml_hal_cmd_info_t;

typedef enum _aml_hal_black_level_type { //v4l2_ext_vpq_black_level_type
    AML_HAL_BLACKLEVEL_Y709_LINEAR_LIMIT_HIGH,
    AML_HAL_BLACKLEVEL_Y709_BYPASS,
    AML_HAL_BLACKLEVEL_RGB_Y709_LINEAR_LOW,
    AML_HAL_BLACKLEVEL_RGB_Y709_LIMIT_HIGH,
    AML_HAL_BLACKLEVEL_Y709_COMP_LOW,
    AML_HAL_BLACKLEVEL_AV_RF_EXTENSION,
    AML_HAL_BLACKLEVEL_RGB_BT2020_LINEAR_LOW,
    AML_HAL_BLACKLEVEL_RGB_BT2020_LIMIT_HIGH,
    AML_HAL_BLACKLEVEL_RGB_Y601_LINEAR_LOW,
    AML_HAL_BLACKLEVEL_RGB_Y601_LIMIT_HIGH,
} aml_hal_black_level_type;

typedef struct aml_hal_black_level_info_s { //v4l2_ext_vpq_black_level_info
    UINT8 ui_value;
    UINT8 curr_input;
    UINT8 color_space;
    aml_hal_black_level_type black_level_type;
} aml_hal_black_level_info_t;

typedef struct aml_hal_gamma_lut_s { //v4l2_ext_gamma_lut
    UINT32 table_num; // number of table elements
    UINT32 table_read[GAMMA_LUT_TABLE_NUM];
    UINT32 table_green[GAMMA_LUT_TABLE_NUM];
    UINT32 table_blue[GAMMA_LUT_TABLE_NUM];
} aml_hal_gamma_lut_t;

#define CSC_MUX_LUT_SIZE 4
typedef struct aml_hal_csc_mux_lut_s { //v4l2_ext_csc_mux_lut
    UINT32 mux_l3d_in;
    UINT32 mux_blend_in;
    UINT32 mux_4p_lut_in;
    UINT32 mux_oetf_out;
    UINT32 b4p_lut_x[CSC_MUX_LUT_SIZE];
    UINT32 b4p_lut_y[CSC_MUX_LUT_SIZE];
} aml_hal_csc_mux_lut_t;

typedef struct aml_hal_gamut_pre_s { //gamut_matrix_pre_gain
    SINT16 gamut_matrix_pre_gain[V4L2_EXT_GAMUT_MATRIX_PRE_SIZE];
} aml_hal_gamut_pre_t;

typedef struct aml_hal_gamut_post_s { //v4l2_ext_gamut_post
    UINT8 gamma;
    UINT8 degamma;
    SINT16 matrix[9];
    aml_hal_csc_mux_lut_t mux_blend;
    union {
        UINT8 *pst_chip_data;
        UINT32 compat_data;
        UINT64 sizer;
    };
} aml_hal_gamut_post_t;

typedef struct aml_hal_pqmode_info_s { //v4l2_ext_pq_mode_info
    UINT32 hdrStatus; //see v4l2_ext_hdr_mode
    UINT32 colorimetry; //see v4l2_ext_colorimetry_info
    UINT32 peakLuminance; //peakLuminance of panel
    UINT32 supportPrime; //support Prime
    UINT32 reserved;
} aml_hal_pqmode_info_t;

typedef enum _aml_hal_ptn_mode_e { //V4L2_VPQ_EXT_PATTERN_MODE
    AML_HAL_PATTERN_WINBOX = 0,
    AML_HAL_PATTERN_GRADATION,
    AML_HAL_PATTERN_MAX,
} aml_hal_ptn_mode_e;

typedef enum _aml_hal_ptn_gradation_direction_e { //V4L2_VPQ_EXT_PATTERN_GRADATION_DIRECTION
    AML_HAL_PATTERN_GRADATION_DIRECTION_HORIZONTAL = 0,
    AML_HAL_PATTERN_GRADATION_DIRECTION_VERTICAL,
    AML_HAL_PATTERN_GRADATION_DIRECTION_MAX,
} aml_hal_ptn_gradation_direction_e;

typedef struct aml_hal_ptn_gradation_line_attr_s { //v4l2_vpq_ext_pattern_gradation_line_attr
    UINT8 lineIdx; //gradation line index
    UINT16 start_R; //1st gradation block's red level as a 10bit resolution
    UINT16 start_G; //1st gradation block's green level as a 10bit resolution
    UINT16 start_B; //1st gradation block's blue level as a 10bit resolution
    UINT16 step_R; //step size for next gradation block
    UINT16 step_G; //step size for next gradation block
    UINT16 step_B; //step size for next gradation block
    UINT16 strideSize; // gradation block's width(horizontal mode)/height(vertical mode)
} aml_hal_ptn_gradation_line_attr_t;

typedef struct aml_hal_ptn_gradation_info_s { //v4l2_vpq_ext_pattern_gradation_info
    UINT8 numGrad; //number of gradation lines in a screen
    aml_hal_ptn_gradation_direction_e eGradMode;
    aml_hal_ptn_gradation_line_attr_t stLineAttr[MAX_EXT_PATTERN_GRADATION_LINE];
} aml_hal_ptn_gradation_info_t;

typedef struct aml_hal_ptn_winbox_win_attr_s { //v4l2_vpq_ext_pattern_winbox_win_attr
    UINT16 winIdx; //window layer index. 0:background
    UINT16 x;
    UINT16 y;
    UINT16 w;
    UINT16 h;
    UINT16 fill_R; //10 bit resolution
    UINT16 fill_G; //10 bit resolution
    UINT16 fill_B; //10 bit resolution
} aml_hal_ptn_winbox_win_attr_t;

typedef struct aml_hal_ptn_winbox_info_s { //v4l2_vpq_ext_pattern_winbox_info
    UINT8 u8NumWin; //number of windows in a screen(including background window)
    aml_hal_ptn_winbox_win_attr_t stWinBoxAttr[MAX_EXT_PATTERN_WINBOX];
} aml_hal_ptn_winbox_info_t;

typedef struct aml_hal_ptn_info_v2_s { //v4l2_vpq_ext_pattern_info_v2
    UINT8 bOnOff;
    aml_hal_ptn_mode_e eMode;
    aml_hal_ptn_gradation_info_t stGradInfo;
    aml_hal_ptn_winbox_info_t stWinboxInfo;
} aml_hal_ptn_info_v2_t;

typedef struct {
    UINT8 Brightness_0;
    UINT8 Brightness_25;
    UINT8 Brightness_50;
    UINT8 Brightness_75;
    UINT8 Brightness_100;
    UINT8 Contrast_0;
    UINT8 Contrast_25;
    UINT8 Contrast_50;
    UINT8 Contrast_75;
    UINT8 Contrast_100;
    UINT8 Saturation_0;
    UINT8 Saturation_25;
    UINT8 Saturation_50;
    UINT8 Saturation_75;
    UINT8 Saturation_100;
    UINT8 Hue_0;
    UINT8 Hue_25;
    UINT8 Hue_50;
    UINT8 Hue_75;
    UINT8 Hue_100;
    UINT8 Sharpness_0;
    UINT8 Sharpness_25;
    UINT8 Sharpness_50;
    UINT8 Sharpness_75;
    UINT8 Sharpness_100;
    UINT8 Backlight_0;
    UINT8 Backlight_25;
    UINT8 Backlight_50;
    UINT8 Backlight_75;
    UINT8 Backlight_100;
} Hal_NonlinearModeType;

/**
*** function
**/
HAL_STATUS_T AML_HAL_PQ_INIT(void);
HAL_STATUS_T AML_HAL_PQ_UNINIT(void);
HAL_STATUS_T AML_HAL_PQ_SetBrightness(SINT32 value);
HAL_STATUS_T AML_HAL_PQ_GetBrightness(SINT32 *pValue);
HAL_STATUS_T AML_HAL_PQ_SetContrast(SINT32 value);
HAL_STATUS_T AML_HAL_PQ_GetContrast(SINT32 *pValue);
HAL_STATUS_T AML_HAL_PQ_SetSaturation(SINT32 value);
HAL_STATUS_T AML_HAL_PQ_GetSaturation(SINT32 *pValue);
HAL_STATUS_T AML_HAL_PQ_SetHue(SINT32 value);
HAL_STATUS_T AML_HAL_PQ_GetHue(SINT32 *pValue);
HAL_STATUS_T AML_HAL_PQ_SetSharpness(SINT32 value);
HAL_STATUS_T AML_HAL_PQ_GetSharpness(SINT32 *pValue);
HAL_STATUS_T AML_HAL_PQ_SetBacklight(SINT32 value);
HAL_STATUS_T AML_HAL_PQ_GetBacklight(SINT32 *pValue);
HAL_STATUS_T AML_HAL_PQ_SetColorTempData(aml_hal_white_balance_t *pamlHalWb);
HAL_STATUS_T AML_HAL_PQ_GetColorTempData(aml_hal_white_balance_t *pamlHalWb);

HAL_STATUS_T AML_HAL_PQ_SetColorTemp_RGain(UINT32 r_gain);
HAL_STATUS_T AML_HAL_PQ_SetColorTemp_GGain(UINT32 g_gain);
HAL_STATUS_T AML_HAL_PQ_SetColorTemp_BGain(UINT32 b_gain);
HAL_STATUS_T AML_HAL_PQ_SetColorTemp_ROffSet(SINT32 r_offset);
HAL_STATUS_T AML_HAL_PQ_SetColorTemp_GOffSet(SINT32 g_offset);
HAL_STATUS_T AML_HAL_PQ_SetColorTemp_BOffSet(SINT32 b_offset);

HAL_STATUS_T AML_HAL_PQ_SetPreGamma(aml_hal_pre_gamma_table_t *pamlHalPreGamma);
HAL_STATUS_T AML_HAL_PQ_GetPreGamma(void);
HAL_STATUS_T AML_HAL_PQ_SetGammaCurve(aml_hal_gamma_curve_e eGammaCurve);
HAL_STATUS_T AML_HAL_PQ_GetGammaCurve(void);
HAL_STATUS_T AML_HAL_PQ_SetModuleCtrl(aml_hal_module_ctrl_t *pModuleCtrl);
HAL_STATUS_T AML_HAL_PQ_GetModuleCtrl(aml_hal_module_ctrl_t *pModuleCtrl);
HAL_STATUS_T AML_HAL_PQ_SetMatrixParam(aml_hal_mtrx_info_t *pMatrixInfo);
HAL_STATUS_T AML_HAL_PQ_SetPqState(aml_hal_pq_state_t *pPqState);
HAL_STATUS_T AML_HAL_PQ_GetPqState(aml_hal_pq_state_t *pPqState);
HAL_STATUS_T AML_HAL_PQ_SetPcMode(aml_hal_pc_mode_e ePcMode);
HAL_STATUS_T AML_HAL_PQ_GetPcMode(aml_hal_pc_mode_e *pPcMode);
HAL_STATUS_T AML_HAL_PQ_SetDc(aml_hal_pq_dc_mode_e eDcMode);
HAL_STATUS_T AML_HAL_PQ_GetDc(SINT32 *pValue);
HAL_STATUS_T AML_HAL_PQ_SetLcMode(aml_hal_pq_lc_mode_e eLcMode);
HAL_STATUS_T AML_HAL_PQ_SetCscType(aml_hal_csc_type_e eCscType);
HAL_STATUS_T AML_HAL_PQ_GetCscType(aml_hal_csc_type_e *pCscType);
HAL_STATUS_T AML_HAL_PQ_Set3DLutData(SINT32 value);
HAL_STATUS_T AML_HAL_PQ_Get3DLutData(void);
HAL_STATUS_T AML_HAL_PQ_GetHdrType(aml_hal_hdr_type_e *pHdrType);
HAL_STATUS_T AML_HAL_PQ_GetColorPrim(aml_hal_color_primary_e *pColorPrim);
HAL_STATUS_T AML_HAL_PQ_GetHdrMetadata(aml_hal_hdr_metadata_t *pHdrMetadata);
HAL_STATUS_T AML_HAL_PQ_GetHistAvg(aml_hal_histgm_ave_t *pHistAve);
HAL_STATUS_T AML_HAL_PQ_GetHistParam(aml_hal_histgm_param_t *pHistParam);
HAL_STATUS_T AML_HAL_PQ_SetPQSrcTiming(aml_hal_pq_source_timing_e eSrcTiming);
HAL_STATUS_T AML_HAL_PQ_SetPQOSDSrcTiming(aml_hal_pq_source_timing_e eSrcTiming);


HAL_STATUS_T AML_HAL_PQ_SetDcColorGain(SINT32 value);
HAL_STATUS_T AML_HAL_PQ_GetDcColorGain(SINT32 *pValue);
HAL_STATUS_T AML_HAL_PQ_SetInnerPattern(SINT32 value);
HAL_STATUS_T AML_HAL_PQ_SetRealCinema(SINT32 value);
HAL_STATUS_T AML_HAL_PQ_GetRealCinema(SINT32 *pValue);
HAL_STATUS_T AML_HAL_PQ_SetCmDbData(aml_hal_cmd_info_t *pCmInfo);
HAL_STATUS_T AML_HAL_PQ_GetCmDbData(aml_hal_cmd_info_t *pCmInfo);
HAL_STATUS_T AML_HAL_PQ_SetBlackLevel(aml_hal_black_level_info_t *pBlackInfo);
HAL_STATUS_T AML_HAL_PQ_GetBlackLevel(aml_hal_black_level_info_t *pBlackInfo);
HAL_STATUS_T AML_HAL_PQ_SetGammaData(aml_hal_gamma_lut_t *pGammaLut);
HAL_STATUS_T AML_HAL_PQ_GetGammaData(aml_hal_gamma_lut_t *pGammaLut);
HAL_STATUS_T AML_HAL_PQ_SetGamutMatrixPre(aml_hal_gamut_pre_t *pGamutPre);
HAL_STATUS_T AML_HAL_PQ_GetGamutMatrixPre(aml_hal_gamut_pre_t *pGamutPre);
HAL_STATUS_T AML_HAL_PQ_SetGamutMatrixPost(aml_hal_gamut_post_t *pGamutPost);
HAL_STATUS_T AML_HAL_PQ_GetGamutMatrixPost(aml_hal_gamut_post_t *pGamutPost);
HAL_STATUS_T AML_HAL_PQ_SetPqModeInfo(aml_hal_pqmode_info_t *pPqModeInfo);
HAL_STATUS_T AML_HAL_PQ_GetPqModeInfo(aml_hal_pqmode_info_t *pPqModeInfo);
HAL_STATUS_T AML_HAL_PQ_SetExtraPattern(aml_hal_ptn_info_v2_t *pPtnInfo);
HAL_STATUS_T AML_HAL_PQ_SetNr(SINT32 value);
HAL_STATUS_T AML_HAL_PQ_GetNr(SINT32 *pValue);
HAL_STATUS_T AML_HAL_PQ_SetMpegNr(SINT32 value);
HAL_STATUS_T AML_HAL_PQ_GetMpegNr(SINT32 *pValue);
HAL_STATUS_T AML_HAL_PQ_SetDecontour(SINT32 value);
HAL_STATUS_T AML_HAL_PQ_GetDecontour(SINT32 *pValue);

HAL_STATUS_T AML_HAL_PQ_SetPictureModeFlag(int Mode);
HAL_STATUS_T AML_HAL_PQ_GetPictureModeFlag(int *Mode);

HAL_STATUS_T AML_HAL_PQ_SetStructNonlinearModeType(Hal_NonlinearModeType *pData);

HAL_STATUS_T AML_HAL_PQ_SetBrightness_OSD(SINT32 value);
HAL_STATUS_T AML_HAL_PQ_GetBrightness_OSD(SINT32 *pValue);
HAL_STATUS_T AML_HAL_PQ_SetContrast_OSD(SINT32 value);
HAL_STATUS_T AML_HAL_PQ_GetContrast_OSD(SINT32 *pValue);
HAL_STATUS_T AML_HAL_PQ_SetSaturation_OSD(SINT32 value);
HAL_STATUS_T AML_HAL_PQ_GetSaturation_OSD(SINT32 *pValue);
HAL_STATUS_T AML_HAL_PQ_SetHue_OSD(SINT32 value);
HAL_STATUS_T AML_HAL_PQ_GetHue_OSD(SINT32 *pValue);

HAL_STATUS_T AML_HAL_PQ_LD_SetLevelIdx(int iLevelIdx);

//PQ OSD Params
void* AML_HAL_PQ_GetPQOsdVerData(void);
void* AML_HAL_PQ_GetPQOsdNonlinearData(UINT32 *TableNum);
void* AML_HAL_PQ_GetPQOsdPictureData(UINT32 *TableNum);
void* AML_HAL_PQ_GetPQOsdColorTempData(UINT32 *TableNum);

#ifdef  __cplusplus
}
#endif
#endif
