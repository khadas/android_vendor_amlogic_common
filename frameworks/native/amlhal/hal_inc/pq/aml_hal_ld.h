#ifndef _AML_HAL_LD_H_
#define _AML_HAL_LD_H_

#include <halcommon.h>

#ifdef  __cplusplus
extern "C"
{
#endif


/**
*** definition
**/
typedef struct aml_hal_ld_info_s {
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

    /* comp parameters */
    unsigned int ldc_bl_buf_diff;
    unsigned int ldc_glb_gain;
    unsigned int ldc_dth_en;
    unsigned int ldc_dth_bw;
    unsigned int ldc_gain_lut[16][64];
    unsigned int ldc_min_gain_lut[64];
    //unsigned int ldc_dither_lut[32][16];
} aml_hal_ld_info_t;


/*****************************************************base tvfuse videodev-ext.h*************************************************/
typedef enum _aml_hal_ldim_demo_type { //v4l2_ext_led_ldim_demo_type
    aml_hal_ldim_demo_type_linedemo = 0,
    aml_hal_ldim_demo_type_leftright,
    aml_hal_ldim_demo_type_topbottom,
    aml_hal_ldim_demo_type_max
} aml_hal_ldim_demo_type;

typedef struct aml_hal_ldim_demo_info_s { //v4l2_ext_led_ldim_demo_info
    aml_hal_ldim_demo_type eType;
    unsigned char bOnOff;
} aml_hal_ldim_demo_info_t;

typedef enum _aml_hal_panel_inch_e {
    AML_HAL_LED_INCH_32 = 0,
    AML_HAL_LED_INCH_39,
    AML_HAL_LED_INCH_42,
    AML_HAL_LED_INCH_47,
    AML_HAL_LED_INCH_49,
    AML_HAL_LED_INCH_50,
    AML_HAL_LED_INCH_55,
    AML_HAL_LED_INCH_58,
    AML_HAL_LED_INCH_60,
    AML_HAL_LED_INCH_65,
    AML_HAL_LED_INCH_70,
    AML_HAL_LED_INCH_77,
    AML_HAL_LED_INCH_79,
    AML_HAL_LED_INCH_84,
    AML_HAL_LED_INCH_98,
    AML_HAL_LED_INCH_105, // TV model
    AML_HAL_LED_INCH_23,
    AML_HAL_LED_INCH_24,
    AML_HAL_LED_INCH_26,
    AML_HAL_LED_INCH_27, // Smart Monitor TV
    AML_HAL_LED_INCH_22,
    AML_HAL_LED_INCH_28,
    AML_HAL_LED_INCH_40,
    AML_HAL_LED_INCH_43,
    AML_HAL_LED_INCH_86,
    AML_HAL_LED_INCH_BASE
} aml_hal_panel_inch_e;

typedef enum _aml_hal_led_backlight_e {
    AML_HAL_LED_BACKLIGHT_DIRECT_L = 0,
    AML_HAL_LED_BACKLIGHT_EDGE_LED,
    AML_HAL_LED_BACKLIGHT_OLED,
    AML_HAL_LED_BACKLIGHT_DIRECT_VI,
    AML_HAL_LED_BACKLIGHT_DIRECT_SKY,
    AML_HAL_LED_BACKLIGHT_MINI_LED,
    AML_HAL_LED_BACKLIGHT_END,
} aml_hal_led_backlight_e;

typedef enum _aml_hal_led_bar_e {
    AML_HAL_LED_BAR_6 = 0,
    AML_HAL_LED_BAR_12,
    AML_HAL_LED_BAR_32,
    AML_HAL_LED_BAR_36,
    AML_HAL_LED_BAR_40,
    AML_HAL_LED_BAR_48,
    AML_HAL_LED_BAR_50,
    AML_HAL_LED_BAR_90,
    AML_HAL_LED_BAR_96,
    AML_HAL_LED_BAR_120,
    AML_HAL_LED_BAR_60,
    AML_HAL_LED_BAR_144,
    AML_HAL_LED_BAR_108,
    AML_HAL_LED_BAR_160,
    AML_HAL_LED_BAR_720,
    AML_HAL_LED_BAR_960,
    AML_HAL_LED_BAR_1440,
    AML_HAL_LED_BAR_1800,
    AML_HAL_LED_BAR_1920,
    AML_HAL_LED_BAR_2400,
    AML_HAL_LED_BAR_1200,
    AML_HAL_LED_BAR_288,
    AML_HAL_LED_BAR_216,
    AML_HAL_LED_BAR_180,
    AML_HAL_LED_BAR_MAX,
    AML_HAL_LED_BAR_DEFAULT = AML_HAL_LED_BAR_MAX,
} aml_hal_led_bar_e;

typedef enum _aml_hal_led_module_maker_e {
    AML_HAL_LED_MODULE_LGD = 0,
    AML_HAL_LED_MODULE_CMI,
    AML_HAL_LED_MODULE_AUO,
    AML_HAL_LED_MODULE_SHARP,
    AML_HAL_LED_MODULE_IPS,
    AML_HAL_LED_MODULE_BOE,
    AML_HAL_LED_MODULE_CSOT,
    AML_HAL_LED_MODULE_INNOLUX,
    AML_HAL_LED_MODULE_LCD_END,
    AML_HAL_LED_MODULE_LGE = AML_HAL_LED_MODULE_LCD_END,
    AML_HAL_LED_MODULE_PANASONIC,
    AML_HAL_LED_MODULE_PDP_END,
    AML_HAL_LED_MODULE_BASE = AML_HAL_LED_MODULE_PDP_END,
} aml_hal_led_module_maker_e;

typedef enum _aml_hal_led_ldim_ic_e {
    AML_HAL_LED_LDIM_NONE = 0, // Not support Local dimming.
    AML_HAL_LED_LDIM_INTERNAL = 1, // Use internal Local dimming block.
    AML_HAL_LED_LDIM_EXTERNAL = 2, // Use external Local dimming IC.
} aml_hal_led_ldim_ic_e;

typedef enum _aml_hal_led_wcg_panel_e {
    AML_HAL_LED_WCG_PANEL_LED = 0,
    AML_HAL_LED_WCG_PANEL_LED_ULTRAHD = 1,
    AML_HAL_LED_WCG_PANEL_OLED  = 2,
    AML_HAL_LED_WCG_PANEL_OLED_ULTRAHD = 3
} aml_hal_led_wcg_panel_e;

typedef struct aml_hal_led_panel_info_s {
    aml_hal_panel_inch_e panel_inch; // panel size   ex) 47, 55
    aml_hal_led_backlight_e backlight_type; // led backlight type  ex) alef, edge
    aml_hal_led_bar_e bar_type; // led bar type   ex) h6,h12, v12
    aml_hal_led_module_maker_e module_maker; // panel maker   ex) lgd, auo
    aml_hal_led_ldim_ic_e local_dim_ic_type; // localdimming control type
    // ex) internal localdiming
    // block
    aml_hal_led_wcg_panel_e panel_type;
} aml_hal_led_panel_info_t;

typedef struct aml_hal_led_apl_info_s {
    unsigned short block_apl_min;
    unsigned short block_apl_max;
} aml_hal_led_apl_info_t;

typedef struct aml_hal_led_spi_ctrl_info_s {
    unsigned char bitMask; // see V4L2_EXT_LED_SPI_XXX for reference
    unsigned int ctrlValue;
} aml_hal_led_spi_ctrl_info_t;



/**
*** function
**/
HAL_STATUS_T AML_HAL_LD_INIT(void);
HAL_STATUS_T AML_HAL_LD_GetPqInitStatus(void);
HAL_STATUS_T AML_HAL_LD_SetPqInit(void);
HAL_STATUS_T AML_HAL_LD_GetLevelIdx(int *pLevelIdx);
HAL_STATUS_T AML_HAL_LD_SetLevelIdx(int iLevelIdx);
HAL_STATUS_T AML_HAL_LD_GetFuncEn(int *pFuncEn);
HAL_STATUS_T AML_HAL_LD_SetFuncEn(int iFuncEn);
HAL_STATUS_T AML_HAL_LD_GetRemapEn(int *pRemapEn);
HAL_STATUS_T AML_HAL_LD_SeRemapEn(int iRemapEn);
HAL_STATUS_T AML_HAL_LD_GetBlMatrix(int *pBlMatrix);
HAL_STATUS_T AML_HAL_LD_SeBlMatrix(int iMatrix);
HAL_STATUS_T AML_HAL_LD_GetDemoMode(aml_hal_ldim_demo_info_t *pDemoInfo);
HAL_STATUS_T AML_HAL_LD_SetDemoMode(aml_hal_ldim_demo_info_t *pDemoInfo);
HAL_STATUS_T AML_HAL_LD_GetLdmInfo(aml_hal_ld_info_t *pLdmInfo);
HAL_STATUS_T AML_HAL_LD_SetLocalDimming(aml_hal_ld_info_t *pLdmInfo);
HAL_STATUS_T AML_HAL_LD_SetInit(aml_hal_led_panel_info_t *pLedPanelInfo);
HAL_STATUS_T AML_HAL_LD_GetAplInfo(aml_hal_led_apl_info_t *pAplInfo);
HAL_STATUS_T AML_HAL_LD_SetDbIdx(int idx);
HAL_STATUS_T AML_HAL_LD_GetDbIdx(int *pIdx);
HAL_STATUS_T AML_HAL_LD_SetControlSpi(aml_hal_led_spi_ctrl_info_t *pLedControlSpi);
HAL_STATUS_T AML_HAL_LD_GetControlSpi(aml_hal_led_spi_ctrl_info_t *pLedControlSpi);

#ifdef  __cplusplus
}
#endif
#endif
