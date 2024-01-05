/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */


#ifndef _C_CPQCONTROL_H
#define _C_CPQCONTROL_H

#include "SSMAction.h"
#include "CDevicePollCheckThread.h"
#include "CPQdb.h"
#include "PQType.h"
#include "CPQColorData.h"
#include "CPQLog.h"
#include "CDynamicBackLight.h"
#include "CConfigFile.h"
#include "COverScandb.h"

#include "PqNotify.h"
#include "CHlgToneMapping.h"
#include "CDolbyVision.h"
#include "SysWrite.h"
#include "video_tunnel.h"

#define LDIM_PATH                 "/dev/aml_ldim"
#define VPP_DEV_PATH              "/dev/amvecm"
#define DI_DEV_PATH               "/dev/di0"
#define AFE_DEV_PATH              "/dev/tvafe0"

#define BOOTVIDEO_ENABLE_PROP     "persist.vendor.media.bootvideo"
#define BOOTVIDEO_EXIT_PROP       "service.bootvideo.exit"
#define STATIC_FRAME_ENABLE_PROP  "vendor.media.video.setvideoblackout"
#define PROP_MEDIA_AISR           "persist.vendor.sys.aisr"

#define UBOOTENV_HDR_POLICY       "ubootenv.var.hdr_policy"
#define FINAL_GAIN_REG_NUM        46

#define TVIN_IOC_MAGIC 'T'
#define TVIN_IOC_LOAD_REG           _IOW(TVIN_IOC_MAGIC, 0x20, struct am_regs_s)

//Backlight
#define VOUT_DEV                      "/dev/display"
#define VOUT_DEV2                     "/dev/display2"
#define VOUT_DEV3                     "/dev/display3"
#define VOUT_IOC_TYPE                 'C'
#define VOUT_IOC_NR_GET_BL_BRIGHTNESS 0x3
#define VOUT_IOC_NR_SET_BL_BRIGHTNESS 0x4

#define VOUT_IOC_CMD_GET_BL_BRIGHTNESS \
        _IOR(VOUT_IOC_TYPE, VOUT_IOC_NR_GET_BL_BRIGHTNESS, unsigned int)
#define VOUT_IOC_CMD_SET_BL_BRIGHTNESS \
        _IOW(VOUT_IOC_TYPE, VOUT_IOC_NR_SET_BL_BRIGHTNESS, unsigned int)


// screen mode index value
#define  SCREEN_MODE_NORMAL           0
#define  SCREEN_MODE_FULL_STRETCH     1
#define  SCREEN_MODE_4_3              2
#define  SCREEN_MODE_16_9             3
#define  SCREEN_MODE_NONLINEAR        4
#define  SCREEN_MODE_NORMAL_NOSCALEUP 5
#define  SCREEN_MODE_4_3_IGNORE       6
#define  SCREEN_MODE_4_3_LETTER_BOX   7
#define  SCREEN_MODE_4_3_PAN_SCAN     8
#define  SCREEN_MODE_4_3_COMBINED     9
#define  SCREEN_MODE_16_9_IGNORE      10
#define  SCREEN_MODE_16_9_LETTER_BOX  11
#define  SCREEN_MODE_16_9_PAN_SCAN    12
#define  SCREEN_MODE_16_9_COMBINED   13
#define  SCREEN_MODE_WIDEOPTION_CUSTOM 14
#define  SCREEN_MODE_WIDEOPTION_AFD   15

//NR Param
#define NR_3D_YGAIN_ADDR         (0X371C)
#define NR_3D_CGAIN_ADDR         (0X2DCE)
#define NR_2D_GAIN_ADDR          (0X2DAE)
#define VPP_BLACKEXT_CTRL        (0x1D80)

//Sharpness CTI
#define VPP_CTI_YC_DELAY         (0X7)
#define VPP_DECODE_CTI           (0XB5)
#define VPP_CTI_SR0_GAIN         (0X322F)
#define VPP_CTI_SR1_GAIN         (0X32AF)

#define YC_DELAY_REG_MASK        (0XF)
#define DECODE_CTI_REG_MASK      (0XFFFF)
#define SR0_GAIN0_REG_MASK       (0XFF000000)
#define SR0_GAIN1_REG_MASK       (0X00FF0000)
#define SR0_GAIN2_REG_MASK       (0X0000FF00)
#define SR0_GAIN3_REG_MASK       (0X000000FF)
#define SR1_GAIN0_REG_MASK       (0XFF000000)
#define SR1_GAIN1_REG_MASK       (0X00FF0000)
#define SR1_GAIN2_REG_MASK       (0X0000FF00)
#define SR1_GAIN3_REG_MASK       (0X000000FF)

//Video Decode Luma
#define DECODE_BRI_ADDR          (0X157)
#define DECODE_CON_ADDR          (0X157)
#define DECODE_SAT_ADDR          (0XA)
#define DECODE_BRI_REG_MASK      (0X3FF01FF)
#define DECODE_CON_REG_MASK      (0X3FF01FF)
#define DECODE_SAT_REG_MASK      (0XFF)

//Sharpness Advanced
#define SHARPNESS_SD_GAIN                (0x3213)
#define SHARPNESS_SD_HP_DIAG_CORE        (0x320f)
#define SHARPNESS_SD_BP_DIAG_CORE        (0x3210)
#define SHARPNESS_SD_PKGAIN_VSLUMA       (0x327e)
#define SHARPNESS_HD_GAIN                (0x3293)
#define SHARPNESS_HD_HP_DIAG_CORE        (0x328f)
#define SHARPNESS_HD_BP_DIAG_CORE        (0x3290)
#define SHARPNESS_HD_PKGAIN_VSLUMA       (0x32fe)

//memc
#define PROP_CPQ_MEMC               "persist.vendor.sys.memc"
#define CPQ_MEMC_SYSFS              "/dev/frc"
#define MEMDEV_CONTRL               _IOW('F', 0x06, unsigned int)
#define FRC_IOC_SET_MEMC_LEVEL      _IOW('F', 0x07, unsigned int)
#define FRC_IOC_SET_MEMC_DEMO       _IOW('F', 0x08, unsigned int)

//lcd
#define MAX_TABLE_SIZE                            0x300000
#define CPQ_LCD_SYSFS                             "/dev/lcd0"
#define LCD_IOC_NR_GET_HDR_INFO                   _IOR('C', 0x0, struct lcd_optical_info_s)
#define LCD_IOC_NR_SET_HDR_INFO                   _IOW('C', 0x1, struct lcd_optical_info_s)
#define LCD_IOC_GET_TCON_BIN_MAX_CNT_INFO         _IOR('C', 0x2, unsigned int)
#define LCD_IOC_SET_TCON_DATA_INDEX_INFO          _IOW('C', 0x3, unsigned int)
#define LCD_IOC_GET_TCON_BIN_PATH_INFO            _IOR('C', 0x4, struct aml_path_s)
#define LCD_IOC_SET_TCON_BIN_DATA_INFO            _IOW('C', 0x5, struct am_pq_bin_param_s)

//pqmode para
#define MAX_PICTUREMODE_PARAM_SIZE                100
#define MAX_TEMPERATURE_PARAM_SIZE                48

typedef enum db_name_e {
    DB_NAME_PQ = 0,
    DB_NAME_OVERSCAN,
    DB_NAME_MAX,
} db_name_t;

typedef enum rgb_ogo_type_e{
    TYPE_INVALID = -1,
    R_GAIN = 0,
    G_GAIN,
    B_GAIN,
    R_POST_OFFSET,
    G_POST_OFFSET,
    B_POST_OFFSET,
    RGB_TYPE_MAX,
} rgb_ogo_type_t;

typedef enum video_layer_color_e{
    VIDEO_LAYER_COLOR_BLACK   = 0,
    VIDEO_LAYER_COLOR_BLUE    = 1,
    VIDEO_LAYER_COLOR_MAX,
} video_layer_color_t;

typedef enum video_color_frame {
    SET_BLACK,
    SET_BLUE,
} video_color_frame_t;

typedef enum video_color_frame_time {
   /*
     * only show one frame of solid color,
     * will recovery when receive new frame
     */
    SET_TIME_ONCE = 4,
    /*
     * Always show the solid color frame
     * until receive disable cmd or surface disconnect
     */
    SET_TIME_ALWAYS = 5,
    /*
     * disable color frame
     */
    SET_TIME_DISABLE = 6,
} video_color_frame_time_t;

typedef enum video__color_Window {
    RESERVED ,
    MAIN_WINDOW,
    SUB_WINDOW,
} video__color_Window_t;

class CPQControl: public CDevicePollCheckThread::IDevicePollCheckObserver,
                         public CDynamicBackLight::IDynamicBackLightObserver,
                         public SSMAction::ISSMActionObserver {
public:
    CPQControl();
    ~CPQControl();
    static CPQControl *GetInstance();
    void CPQControlInit(void);
    void CPQControlUnInit(void);
    virtual void onVframeSizeChange();
    virtual void onTXStatusChange();
    int SetPQModuleDemoState(pq_module_demo_t modules, pq_module_demo_state_t state);
    int GetPQModuleDemoState(int modules);
    virtual void resetAllUserSettingParam();
    virtual void resetPQUiSetting(void);
    virtual void resetPQTableSetting(void);
    virtual void Set_Backlight(int value);
    virtual void GetDynamicBacklighConfig(int *thtf, int *lut_mode, int *height_param, int *low_param);
    virtual void GetDynamicBacklighParam(dynamic_backlight_Param_t *DynamicBacklightParam);
    int isGameMode();
    int LoadPQSettings();
    int LoadPQUISettings();
    int LoadPQTableSettings(void);
    int LoadCpqLdimRegs(void);
    int Cpq_LoadRegs(am_regs_t regs);
    int Cpq_LoadDisplayModeRegs(ve_pq_load_t regs);
    int DI_LoadRegs(am_pq_param_t di_regs );
    int Cpq_LoadBasicRegs(source_input_param_t source_input_param, vpp_picture_mode_t pqMode);
    int Cpq_SetDIModuleParam(source_input_param_t source_input_param);
    int ResetLastPQSettingsSourceType(void);
    int BacklightInit(void);
    //PQ mode
    int SetPQMode(int pq_mode, int is_save, int is_autoswitch);
    int GetPQMode(void);
    int GetLastPQMode(void);
    int SavePQMode(int pq_mode);
    int SaveLastPQMode(int pq_mode);
    int setPQModeByTvService(pq_status_update_e gameStatus, pq_status_update_e pcStatus, int autoSwitchMonitorModeFlag);
    int Cpq_SetPQMode(vpp_picture_mode_t pq_mode, source_input_param_t source_input_param, pq_mode_switch_type_t switch_type);
    int SetPQParams(source_input_param_t source_input_param, vpp_picture_mode_t pq_mode, vpp_pq_para_t pq_para);
    int GetPQParams(source_input_param_t source_input_param, vpp_picture_mode_t pq_mode, vpp_pq_para_t *pq_para);
    int SetPictureModeData(pq_src_param_t source_input, vpp_picture_mode_t picmode, vpp_pictur_mode_para_t *params);
    int GetPictureModeData(pq_src_param_t source_input, vpp_picture_mode_t picmode, vpp_pictur_mode_para_t *params);
    int RsetPictureModeData(pq_src_param_t source_input, vpp_picture_mode_t picmode);
    void SetPcGameMode(vpp_picture_mode_t pq_mode, pq_mode_switch_type_t switch_type);
    int Set_PictureMode(vpp_picture_mode_t pq_mode, pq_src_param_t source_input_param, pq_mode_switch_type_t switch_type);
    int SetFacColorParams(source_input_param_t source_input_param, vpp_picture_mode_t pqMode);

    //color Temperature
    int SetColorTemperature(int temp_mode, int is_save);
    int GetColorTemperature(void);
    int SaveColorTemperature(int temp_mode);
	int SetColorTemperatureUserParam(int temp_mode, int is_save, rgb_ogo_type_t rgb_ogo_type = TYPE_INVALID, int value = -1);
	tcon_rgb_ogo_t GetColorTemperatureUserParam(void);
    int Cpq_SetColorTemperatureWithoutSave(vpp_color_temperature_mode_t Tempmode, tv_source_input_t tv_source_input __unused);
    int Cpq_CheckColorTemperatureParamAlldata(source_input_param_t source_input_param);
    unsigned short Cpq_CalColorTemperatureParamsChecksum(void);
    int Cpq_SetColorTemperatureParamsChecksum(void);
    unsigned short Cpq_GetColorTemperatureParamsChecksum(void);
    int Cpq_SetColorTemperatureUser(tv_source_input_t source_input, tcon_rgb_ogo_t *pData);
    int Cpq_GetColorTemperatureUser(vpp_color_temperature_mode_t mode, RGB_UI_OFFSET* pData);
    int Cpq_SaveColorTemperatureUser(vpp_color_temperature_mode_t mode, rgb_ogo_type_t rgb_ogo_type, int value);
    int CPQ_SetColorTemperatureUserParam(vpp_color_temperature_mode_t temp_mode, rgb_ogo_type_t rgb_ogo_type, int value);
    int Cpq_RestoreColorTemperatureParamsFromDB(source_input_param_t source_input_param);
    int Cpq_CheckTemperatureDataLabel(void);
    int Cpq_SetTemperatureDataLabel(void);
    int SetColorTemperatureParams(vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t params);
    int GetColorTemperatureParams(vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t *params);
    int SaveColorTemperatureParams(vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t params);
    int Cpq_CheckColorTemperatureParams(void);
    //Brightness
    int SetBrightness(int value, int is_save);
    int GetBrightness(void);
    int SaveBrightness(int value);
    int Cpq_SetBrightnessBasicParam(source_input_param_t source_input_param);
    int Cpq_SetBrightness(int value, source_input_param_t source_input_param);
    int Cpq_SetVideoBrightness(int value);
    //Contrast
    int SetContrast(int value, int is_save);
    int GetContrast(void);
    int SaveContrast(int value);
    int Cpq_SetContrastBasicParam(source_input_param_t source_input_param);
    int Cpq_SetContrast(int value, source_input_param_t source_input_param);
    int Cpq_SetVideoContrast(int value);
    //Saturation
    int SetSaturation(int value, int is_save);
    int GetSaturation(void);
    int SaveSaturation(int value);
    int Cpq_SetSaturationBasicParam(source_input_param_t source_input_param);
    int Cpq_SetSaturation(int value, source_input_param_t source_input_param);
    //Hue
    int SetHue(int value, int is_save);
    int GetHue(void);
    int SaveHue(int value);
    int Cpq_SetHueBasicParam(source_input_param_t source_input_param);
    int Cpq_SetHue(int value, source_input_param_t source_input_param);
    int Cpq_SetVideoSaturationHue(int satVal, int hueVal);
    void video_set_saturation_hue(signed char saturation, signed char hue, signed long *mab);
    void video_get_saturation_hue(signed char *sat, signed char *hue, signed long *mab);
    //Sharpness
    int SetSharpness(int value, int is_enable, int is_save);
    int GetSharpness(void);
    int SaveSharpness(int value);
    int Cpq_SetSharpness(int value, source_input_param_t source_input_param);
    int Cpq_SetSharpness0FixedParam(source_input_param_t source_input_param);
    int Cpq_SetSharpness0VariableParam(source_input_param_t source_input_param);
    int Cpq_SetSharpness1FixedParam(source_input_param_t source_input_param);
    int Cpq_SetSharpness1VariableParam(source_input_param_t source_input_param);
    int Cpq_SetSharpnessPiFixedParam(source_input_param_t source_input_param);
    int Cpq_SetSharpnessPiVariableParam(source_input_param_t source_input_param);
    //NoiseReductionMode
    void InitAutoNr(void);
    int SetNoiseReductionMode(int nr_mode, int is_save);
    int GetNoiseReductionMode(void);
    int SaveNoiseReductionMode(int nr_mode);
    int Cpq_SetNoiseReductionMode(vpp_noise_reduction_mode_t nr_mode, source_input_param_t source_input_param);
    //GammaValue
    int SetGammaValue(vpp_gamma_curve_t gamma_curve, int is_save);
    int GetGammaValue();
    //Memc
    bool hasMemcFunc();
    int initMemc(void);
    int Memc_enable(int enable);
    int SetMemcMode(int memc_mode, int is_save);
    int GetMemcMode(void);
    int SaveMemcMode(vpp_memc_mode_t memc_mode);
    int Cpq_SetMemcMode(vpp_memc_mode_t memc_mode, source_input_param_t source_input_param);
    int SetMemcDeBlurLevel(int level, int is_save);
    int GetMemcDeBlurLevel(void);
    int SaveMemcDeBlurLevel(int level);
    int Cpq_SetMemcDeBlurLevel(int level, source_input_param_t source_input_param);
    int SetMemcDeJudderLevel(int level, int is_save);
    int GetMemcDeJudderLevel(void);
    int SaveMemcDeJudderLevel(int level);
    int Cpq_SetMemcDeJudderLevel(int level, source_input_param_t source_input_param);

    //Displaymode
    int SetDisplayMode(vpp_display_mode_t display_mode, int is_save);
    int GetDisplayMode(void);
    int SaveDisplayMode(vpp_display_mode_t mode);
    int Cpq_SetDisplayModeAllTiming(tv_source_input_t source_input, vpp_display_mode_t display_mode);
    int Cpq_SetDisplayModeOneTiming(tv_source_input_t source_input, vpp_display_mode_t display_mode);
    int Cpq_SetDisplayModeCrop(tv_source_input_t source_input, vpp_display_mode_t display_mode);
    int Cpq_SetDisplayModeScreenMode(tv_source_input_t source_input, vpp_display_mode_t display_mode);
    int Cpq_SetVideoScreenMode(int value);
    int Cpq_GetScreenModeValue(vpp_display_mode_t display_mode);
    int Cpq_SetVideoCrop(int Voffset0, int Hoffset0, int Voffset1, int Hoffset1);
    int Cpq_SetNonLinearFactor(int value);
    //Backlight
    int read_backlight_value(unsigned int *temp);
    int read_backlight2_value(unsigned int *temp);
    int read_backlight3_value(unsigned int *temp);
    int write_backlight_value(unsigned int *temp);
    int write_backlight2_value(unsigned int *temp);
    int write_backlight3_value(unsigned int *temp);
    int SetBacklight(int value, int index, int is_save);
    int GetBacklight(int index);
    int SaveBacklight(int value, int index);
    int Cpq_SetBackLight(int value, int index);
    void Cpq_GetBacklight(int *value, int index);
    int SetDynamicBacklight(Dynamic_backlight_status_t mode, int is_save);
    int GetDynamicBacklight(void);
    int DynamicBackLightInit(void);
    int GetVideoPlayStatus(void);
    //smooth plus
    int SetSmoothPlusMode(int smoothplus_mode, int is_save);
    int GetSmoothPlusMode(void);
    int SaveSmoothPlusMode(int smoothplus_mode);
    int Cpq_SetSmoothPlusMode(vpp_smooth_plus_mode_t smoothplus_mode, source_input_param_t source_input_param);
    bool hasSmoothPlusFunc(void);
    //DLG
    int SetDLGEnable(int enable, int is_save);
    int GetDLGEnable(void);
    int SaveDLGEnable(int enable);
    //local contrast
    int SetLocalContrastMode(local_contrast_mode_t mode, int is_save);
    int GetLocalContrastMode(void);
    int SaveLocalContrastMode(local_contrast_mode_t mode);
    //BlackExtension
    int SetBlackExtensionMode(black_extension_mode_t mode, int is_save);
    int GetBlackExtensionMode(void);
    int SaveBlackExtensionMode(black_extension_mode_t mode);
    int SetBlackExtensionParam(source_input_param_t source_input_param);
    //MpegNr
    int SetMpegNr(vpp_pq_level_t mode, int is_save);
    int GetMpegNr(void);
    int SaveMpegNr(vpp_pq_level_t mode);
    int Cpq_SetMpegNr(vpp_pq_level_t mode, source_input_param_t source_input_param);
    //DI deblock
    int SetDeblockMode(di_deblock_mode_t mode, int is_save);
    int GetDeblockMode(void);
    int SaveDeblockMode(di_deblock_mode_t mode);
    int Cpq_SetDeblockMode(di_deblock_mode_t deblock_mode, source_input_param_t source_input_param);
    //DI demosquito
    int SetDemoSquitoMode(di_demosquito_mode_t mode, int is_save);
    int GetDemoSquitoMode(void);
    int SaveDemoSquitoMode(di_demosquito_mode_t mode);
    int Cpq_SetDemoSquitoMode(di_demosquito_mode_t DeMosquito_mode, source_input_param_t source_input_param);
    //DI MCDI
    int SetMcDiMode(vpp_mcdi_mode_e mode, int is_save);
    int GetMcDiMode(void);
    int SaveMcDiMode(vpp_mcdi_mode_e mode);
    int Cpq_SetMcDiMode(vpp_mcdi_mode_e McDi_mode, source_input_param_t source_input_param);
    //static frame
    int SetStaticFrameEnable(int enable, int isSave);
    int GetStaticFrameEnable();
    //screen color
    int SetScreenColorForSignalChange(int screenColor, int isSave);
    int GetScreenColorForSignalChange();
    int setVideoScreenColor (int color);
    int setVideoScreenColorByVT(int Color, int frequency, int window);//for new path
    //get overscan
    tvin_cutwin_t GetOverscanParams(vpp_display_mode_t display_mode);
    //Factory
    int FactoryResetPQMode(void);
    int FactoryResetColorTemp(void);
    int FactorySetPQMode_Brightness(source_input_param_t source_input_param, int pq_mode, int brightness );
    int FactoryGetPQMode_Brightness(source_input_param_t source_input_param, int pq_mode );
    int FactorySetPQMode_Contrast(source_input_param_t source_input_param, int pq_mode, int contrast );
    int FactoryGetPQMode_Contrast(source_input_param_t source_input_param, int pq_mode );
    int FactorySetPQMode_Saturation(source_input_param_t source_input_param, int pq_mode, int saturation );
    int FactoryGetPQMode_Saturation(source_input_param_t source_input_param, int pq_mode );
    int FactorySetPQMode_Hue(source_input_param_t source_input_param, int pq_mode, int hue );
    int FactoryGetPQMode_Hue(source_input_param_t source_input_param, int pq_mode );
    int FactorySetPQMode_Sharpness(source_input_param_t source_input_param, int pq_mode, int sharpness );
    int FactoryGetPQMode_Sharpness(source_input_param_t source_input_param, int pq_mode );
    int FactorySetColorTemp_Rgain ( int source_input, int colortemp_mode, int rgain );
    int FactorySaveColorTemp_Rgain ( int source_input, int colortemp_mode, int rgain );
    int FactoryGetColorTemp_Rgain ( int source_input, int colortemp_mode );
    int FactorySetColorTemp_Ggain ( int source_input, int colortemp_mode, int ggain );
    int FactorySaveColorTemp_Ggain ( int source_input, int colortemp_mode, int ggain );
    int FactoryGetColorTemp_Ggain ( int source_input, int colortemp_mode );
    int FactorySetColorTemp_Bgain ( int source_input, int colortemp_mode, int bgain );
    int FactorySaveColorTemp_Bgain ( int source_input, int colortemp_mode, int bgain );
    int FactoryGetColorTemp_Bgain ( int source_input, int colortemp_mode );
    int FactorySetColorTemp_Roffset ( int source_input, int colortemp_mode, int roffset );
    int FactorySaveColorTemp_Roffset ( int source_input, int colortemp_mode, int roffset );
    int FactoryGetColorTemp_Roffset ( int source_input, int colortemp_mode );
    int FactorySetColorTemp_Goffset ( int source_input, int colortemp_mode, int goffset );
    int FactorySaveColorTemp_Goffset ( int source_input, int colortemp_mode, int goffset );
    int FactoryGetColorTemp_Goffset ( int source_input, int colortemp_mode );
    int FactorySetColorTemp_Boffset ( int source_input, int colortemp_mode, int boffset );
    int FactorySaveColorTemp_Boffset ( int source_input, int colortemp_mode, int boffset );
    int FactoryGetColorTemp_Boffset ( int source_input, int colortemp_mode );
    int FactoryResetNonlinear(void);
    int FactorySetParamsDefault(void);
    int FactorySetNolineParams(source_input_param_t source_input_param, int type, noline_params_t noline_params);
    noline_params_t FactoryGetNolineParams(source_input_param_t source_input_param,          int type);
    int FactorySetHdrMode(int mode);
    int FactoryGetHdrMode(void);
    int FactorySetOverscanParam(source_input_param_t source_input_param, vpp_display_mode_t dmode, tvin_cutwin_t cutwin_t);
    tvin_cutwin_t FactoryGetOverscanParam(source_input_param_t source_input_param, vpp_display_mode_t dmode);
    int FactorySetGamma(int gamma_r_value, int gamma_g_value, int gamma_b_value);
    int FactorySSMRestore(void);

    int SetColorDemoMode(vpp_color_demomode_t demomode);
    int SetColorBaseMode(vpp_color_basemode_t basemode, int isSave);
    vpp_color_basemode_t GetColorBaseMode(void);
    int SaveColorBaseMode(vpp_color_basemode_t basemode);
    int Cpq_SetColorBaseMode(vpp_color_basemode_t basemode, source_input_param_t source_input_param);
    int Cpq_SetRGBOGO(const struct tcon_rgb_ogo_s *rgbogo);
    int Cpq_GetRGBOGO(const struct tcon_rgb_ogo_s *rgbogo);
    int Cpq_LoadGamma(vpp_gamma_curve_t gamma_curve, vpp_color_temperature_mode_t colortemp_mode);
    int DBGammaBlend(tcon_gamma_table_t *wb_gamma, GAMMA_TABLE *index_gamma, tcon_gamma_table_t *target_gamma);
    int Cpq_SetGammaTbl_R(unsigned short red[GAMMA_NUMBER]);
    int Cpq_SetGammaTbl_G(unsigned short green[GAMMA_NUMBER]);
    int Cpq_SetGammaTbl_B(unsigned short blue[GAMMA_NUMBER]);
    int Cpq_SetGammaOnOff(int onoff);
    int Cpq_SetCABC(const db_cabc_param_t *pCABC);
    int SetCabc(void);
    int Cpq_SetAAD(const db_aad_param_t *pAAD);
    int SetAad(void);
    int SetDnlpMode(int level);
    int GetDnlpMode();
    int Cpq_SetVENewDNLP(const ve_dnlp_curve_param_t *pDNLP);
    int SaveDnlpMode(Dynamic_contrast_status_t level);
    int Cpq_SetDnlpMode(Dynamic_contrast_status_t level, source_input_param_t source_input_param);
    int Cpq_SetDNLPStatus(ve_dnlp_state_t status);
    int FactorySetDNLPCurveParams(source_input_param_t source_input_param, int level, int final_gain);
    int FactoryGetDNLPCurveParams(source_input_param_t source_input_param, int level);
    int FactorySetBlackExtRegParams(source_input_param_t source_input_param, int val);
    int FactoryGetBlackExtRegParams(source_input_param_t source_input_param);
    int FactoryGetBEValFromDB(source_input_param_t source_input_param, int addr);
    int FactorySetBERegDBVal(source_input_param_t source_input_param, int addr, unsigned int reg_val);
    int FactorySetRGBCMYFcolorParams(source_input_param_t source_input_param, int color_type,int color_param,int val);
    int FactoryGetRGBCMYFcolorParams(source_input_param_t source_input_param, int color_type,int color_param);
    int FactorySetNoiseReductionParams(source_input_param_t source_input_param, vpp_noise_reduction_mode_t nr_mode, int addr, int val);
    int FactoryGetNoiseReductionParams(source_input_param_t source_input_param, vpp_noise_reduction_mode_t nr_mode, int addr);
    int FactorySetCTIParams(source_input_param_t source_input_param, int param_type, int val);
    int FactoryGetCTIParams(source_input_param_t source_input_param, int param_type);
    int SetCTIParamsCheckVal(int param_type, int val);
    int MatchCTIRegMask(int param_type);
    int MatchCTIRegAddr(int param_type);
    int FactorySetDecodeLumaParams(source_input_param_t source_input_param, int param_type, int val);
    int FactoryGetDecodeLumaParams(source_input_param_t source_input_param, int param_type);
    int SetDecodeLumaParamsCheckVal(int param_type, int val);
    int SetSharpnessParamsCheckVal(int param_type, int val);
    int MatchSharpnessRegAddr(int param_type, int isHd);
    int FactorySetSharpnessParams(source_input_param_t source_input_param, Sharpness_timing_e source_timing, int param_type, int val);
    int FactoryGetSharpnessParams(source_input_param_t source_input_param, Sharpness_timing_e source_timing, int param_type);
    int SetEyeProtectionMode(tv_source_input_t source_input, int enable, int is_save);
    int GetEyeProtectionMode(tv_source_input_t source_input);
    int Cpq_SSMReadNTypes(int id, int data_len, int offset);
    int Cpq_SSMWriteNTypes(int id, int data_len, int data_buf, int offset);
    int Cpq_GetSSMActualAddr(int id);
    int Cpq_GetSSMActualSize(int id);
    int Cpq_SSMRecovery(void);
    int Cpq_GetSSMStatus();
    int SetFlagByCfg(void);
    int SetPLLValues(source_input_param_t source_input_param);
    int SetCVD2Values(void);
    int SetCurrentSourceInputInfo(source_input_param_t source_input_param);
    source_input_param_t GetCurrentSourceInputInfo();
    int GetHistParam(ve_hist_t *hist);
    bool isFileExist(const char *file_name);

    int GetRGBPattern();
    int SetRGBPattern(int r, int g, int b);
    int FactorySetDDRSSC (int step);
    int FactoryGetDDRSSC(void);
    int FactorySetLVDSSSC (int step);
    int FactoryGetLVDSSSC(void);
    int SetLVDSSSC(int step);
    int SetLCDPowerCtrl(int state);
    int SetLCDMuteCtrl(int state);
    int SetGrayPattern(int value);
    int GetGrayPattern();
    int SetLCDPowerCtrl(unsigned int state);
    int SetLCDMuteCtrl(unsigned int state);

    //HDR
    int SetHDRMode(int mode);
    int GetHDRMode(void);
    int GetSourceHDRType(void);
    void GetChipVersionInfo(char* chip_version);
    tvpq_databaseinfo_t GetDBVersionInfo(db_name_t name);
    void setHdrInfoListener(const sp<PqNotify>& listener);
    int SetCurrentHdrInfo (int hdrInfo);
    int SetCurrentAspectRatioInfo(tvin_aspect_ratio_e aspectRatioInfo);
    int SetDtvKitSourceEnable(bool isEnable);
    //AI
    bool hasAipqFunc(void);
    int SetAipqEnable(bool isEnable);
    int GetAipqEnable(void);
    int SetAipqMode(aipq_mode_e mode, int is_save);
    int GetAipqMode(void);
    int HasAiFace(void);
    int SetAiFaceEnable(bool isEnable);
    int GetAiFaceEnable(void);
    bool hasAisrFunc(void);
    int SetAiSrEnable(bool isEnable);
    int GetAiSrEnable(void);
    int SetAiSrMode(aisr_mode_e mode, int is_save);
    int GetAiSrMode(void);

    //aicolor
    int SetAiColor(int value, int is_save);
    int GetAiColor(void);
    int SaveAiColor(int value);
    int Cpq_SetAiColor(int value);

    //COLOR SPACE
    int SetColorGamutMode(vpp_colorgamut_mode_t value, int is_save);
    int GetColorGamutMode(void);
    int SaveColorGamutMode(vpp_colorgamut_mode_t value);
    int Cpq_SetColorGamutMode(vpp_colorgamut_mode_t value, source_input_param_t source_input_param);

    //HDR tone mapping
    int SetHDRTMData(int *reGain);
    //HDR TMO
    int Cpq_SetHDRTMOParams(const hdr_tmo_sw_s *phdrtmo);
    int SetHDRTMOMode(hdr_tmo_t mode, int is_save);
    int GetHDRTMOMode();
    int SaveHDRTMOMode(hdr_tmo_t mode);
    //PQ Diff
    char* CalculateFileSha1(const char* filePath);
    int GenerateTargetPQ();

    //black/bule/chroma stretch
    int SetBlackStretch(int level, int is_save);
    int GetBlackStretch(void);
    int SaveBlackStretch(int level);
    int Cpq_BlackStretch(int level, source_input_param_t source_input_param);

    int SetBlueStretch(int level, int is_save);
    int GetBlueStretch(void);
    int SaveBlueStretch(int level);
    int Cpq_BlueStretch(int level, source_input_param_t source_input_param);

    int SetChromaCoring(int level, int is_save);
    int GetChromaCoring(void);
    int SaveChromaCoring(int level);
    int Cpq_ChromaCoring(int level, source_input_param_t source_input_param);

    int SetLocalDimming(int level, int is_save);
    int GetLocalDimming(void);
    int Cpq_LocalDimming(vpp_pq_level_t level);

    int SetDolbyDarkDetail(int mode, int is_save);
    int GetDolbyDarkDetail(void);
    int SaveDolbyDarkDetail(int value);
    int Cpq_SetDolbyDarkDetail(int mode);

    void InitTconGamma(void);
    void InitLocalDimmingBin(void);
    int LoadLdBin(LD_bin_table_index_t index);
    void InitTconlessBin(void);
    int LoadTconlessBin(unsigned int index);

    //AMHAL
    int AMHal_VPQ_Get_LDBinPath(char *path, LD_bin_table_index_t index);
    int AMHal_VPQ_Set_LDBinData(am_pq_bin_param_s *buff, LD_bin_table_index_t index);

    int AMHal_VPQ_Get_TconlessBinMax(unsigned int *cnt);
    int AMHal_VPQ_Get_TconlessBinPath(aml_path_t *param);
    int AMHal_VPQ_Set_TconlessBinIndex(unsigned int index);
    int AMHal_VPQ_Set_TconlessBinData(am_pq_bin_param_t *param);

private:
    int VPPOpenModule(void);
    int VPPCloseModule(void );
    int VPPDeviceIOCtl(int request, ...);
    int DIOpenModule(void);
    int DICloseModule(void);
    int DIDeviceIOCtl(int request, ...);
    int AFEDeviceIOCtl ( int request, ... );
    int LDOpenModule(void);
    int LDCloseModule(void);
    int LDDeviceIOCtl(int request, ...);
    int MEMCOpenModule(void);
    int MEMCCloseModule(void);
    int MEMCDeviceIOCtl(int request, ...);
    int LCDOpenModule(void);
    int LCDCloseModule(void);
    int LCDDeviceIOCtl(int request, ...);
    tvin_sig_fmt_t getVideoResolutionToFmt();
    int Cpq_SetXVYCCMode(vpp_xvycc_mode_t xvycc_mode, source_input_param_t source_input_param);
    int pqWriteSys(ConstCharforSysNodeIndex index, const char *val);
    int pqReadSys(ConstCharforSysNodeIndex index, char *buf, int count);
    void pqTransformStringToInt(const char *buf, int *val);
    unsigned int GetSharpnessRegVal(int addr);
    int Cpq_SetLocalContrastMode(local_contrast_mode_t mode);
    output_type_t MapDbTvoutWithIOResolution(int inputFrameHeight, int outputFrameHeight);
    output_type_t CheckOutPutMode(tv_source_input_t source_input);
    pq_sig_fmt_t CheckPQTimming(hdr_type_t hdr_type);
    hdr_type_t Cpq_GetSourceHDRType(source_input_param_t source_input_param);
    bool isCVBSParamValid(void);
    bool isPqDatabaseMachChip();
    int Cpq_GetInputVideoFrameHeight(tv_source_input_t source_input);
    int Cpq_SetVadjEnableStatus(int isvadj1Enable, int isvadj2Enable);
    bool isBootvideoStopped();
    int SetVideoLayerColor(video_layer_color_t signalColor, video_layer_color_t nosignalColor);
    int setVideoScreenColor (int vdin_blending_mask, int y, int u, int v );
    int getSnowStatus();
    void InitPGammaBin();
    int getHdrPolicy();
    bool mInitialized;
    bool getBootEnv(const char *name, char *value);
    //for new path set background color
    int OpenVideotunnel();
    int CloseVideotunnel();
    int SetVideotunnelSolidColor(video__color_Window window, video_color_frame cmd, video_color_frame_time cmd_data);

    //AI
    void AipqInit(void);
    void enableAipq(bool isEnable);
    int SaveAipqMode(int mode);
    int Cpq_SetAipqMode(aipq_mode_e mode, source_input_param_t source_input_param);
    int SaveAiSrEnable(bool enable);
    int Cpq_SetAiSrEnable(bool enable);
    int SaveAiSrMode(int mode);
    int Cpq_SetAiSrMode(aisr_mode_e mode, source_input_param_t source_input_param);

    //cfg
    bool mbCpqCfg_separate_db_enable;
    bool mbCpqCfg_amvecm_basic_enable;
    bool mbCpqCfg_amvecm_basic_withOSD_enable;
    bool mbCpqCfg_contrast_rgb_enable;
    bool mbCpqCfg_contrast_rgb_withOSD_enable;
    bool mbCpqCfg_blackextension_enable;
    bool mbCpqCfg_sharpness0_enable;
    bool mbCpqCfg_sharpness1_enable;
    bool mbCpqCfg_sharpnesspi_enable;
    bool mbCpqCfg_di_enable;
    bool mbCpqCfg_mcdi_enable;
    bool mbCpqCfg_deblock_enable;
    bool mbCpqCfg_nr_enable;
    bool mbCpqCfg_demoSquito_enable;
    bool mbCpqCfg_gamma_enable;
    bool mbCpqCfg_cm2_enable;
    bool mbCpqCfg_whitebalance_enable;
    bool mbCpqCfg_dnlp_enable;
    bool mbCpqCfg_xvycc_enable;
    bool mbCpqCfg_display_overscan_enable;
    bool mbCpqCfg_local_contrast_enable;
    bool mbCpqCfg_hdmi_out_with_fbc_enable;
    bool mbCpqCfg_pq_param_check_source_enable;
    bool mbCpqCfg_ai_enable;
    bool mbCpqCfg_aisr_enable;
    bool mbCpqCfg_aicolor_enable;
    bool mbCpqCfg_aad_enable;
    bool mbCpqCfg_cabc_enable;
    bool mbCpqCfg_smoothplus_enable;
    bool mbCpqCfg_hdrtmo_enable;
    bool mbCpqCfg_memc_enable;
    bool mbCpqCfg_separate_black_blue_chorma_db_enable;
    bool mbCpqCfg_bluestretch_enable;
    bool mbCpqCfg_chroma_coring_enable;
    bool mbCpqCfg_LocalDimming_enable;
    bool mbCpqCfg_new_picture_mode_enable;

    CPQdb *mPQdb;
    COverScandb *mpOverScandb;
    SSMAction *mSSMAction;
    SysWrite *pqSysWrite;
    static CPQControl *mInstance;
    sp<CDevicePollCheckThread> mCDevicePollCheckThread;
    sp<CDynamicBackLight> mDynamicBackLight;
    CConfigFile *mPQConfigFile;

    CHlgToneMapping *mHlgToneMapping;
    sp<PqNotify> mNotifyListener;
    CDolbyVision *mDolbyVision;

    int mAmvideoFd;
    int mDiFd;
    int mLdFd;
    int mMemcFd;
    int mLcdFd;
    int mVideoTunelFd;

    tcon_rgb_ogo_t rgbfrompq[3];
    source_input_param_t mCurrentSourceInputInfo;
    tv_source_input_t mSourceInputForSaveParam;
    pq_src_param_t mCurrentPqSource;
    bool mCurrentHdrStatus;
    unsigned int mHdmiHdrInfo = 0;
    bool mbDtvKitEnable;
    bool mbDatabaseMatchChipStatus;
    mutable Mutex mLock;
    output_type_t mCurrentOutputType;
    tvin_aspect_ratio_e mCurrentAfdInfo;
    bool mbVideoIsPlaying = false;//video don't playing
    hdr_type_t mCurrentHdrType = HDR_TYPE_NONE;
    bool screenColorEnable = false;
    vpp_picture_mode_t mLastPictureMode = VPP_PICTURE_MODE_STANDARD;

    int mCurrentNodeNumber;
    bool mDisplayMode4k120 = false;
    bool mDisplayMode4k100 = false;
};
#endif
