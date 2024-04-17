/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef __SSM_ACTION_H__
#define __SSM_ACTION_H__

#include "SSMHandler.h"
#include "PQType.h"

#define SSM_RGBOGO_FILE_OFFSET    (0)
#define CRI_DATE_RGBOGO_LEN                         (42) //sizeof(tcon_rgb_ogo_t) + 2
#define CRI_DATE_RGBOGO_INDEX_MAX                   (8)

#define SSM_CR_RGBOGO_LEN                           (256)
#define SSM_CR_RGBOGO_CHKSUM_LEN                    (2)
#define DEFAULT_BACKLIGHT_BRIGHTNESS                (10)

#define CRI_DATA_WB_GAMMA_OFFSET                    (SSM_RGBOGO_FILE_OFFSET + (CRI_DATE_RGBOGO_LEN * CRI_DATE_RGBOGO_INDEX_MAX))
#define CRI_DATE_WB_GAMMA_LEN                       ((MAX_WB_GAMMA_POINT * 3 * 4) + 2) //sizeof(WB_GAMMA_TABLE) + 2

//#define CRI_DATE_GAMMA_OFFSET                       (512)
//#define CRI_DATE_GAMMA_RGB_SIZE                     (257)
#define CRI_DATE_GAMMA_R_index                      (1)
#define CRI_DATE_GAMMA_G_index                      (2)
#define CRI_DATE_GAMMA_B_index                      (3)
#define CRI_DATE_GAMMA_COOL                         (1)
#define CRI_DATE_GAMMA_STANDARD                     (2)
#define CRI_DATE_GAMMA_WARM                         (3)

class SSMAction {
public:
    SSMAction();
    ~SSMAction();
    void init(const char *SsmDataPath, const char *SsmDataHandlerPath, const char *WhiteBalanceFilePath);
    int SaveBurnWriteCharacterChar(int rw_val);
    int ReadBurnWriteCharacterChar();
    int DeviceMarkCheck();
    int RestoreDeviceMarkValues();
    static SSMAction *getInstance();
    bool isFileExist(const char *file_name);
    int WriteBytes(int offset, int size, int *buf);
    int ReadBytes(int offset, int size, int *buf);
    int EraseAllData(void);
    int GetSSMActualAddr(int id);
    int GetSSMActualSize(int id);
    int GetSSMStatus(void);
    int SSMReadNTypes(int id, int data_len, int *data_buf, int offset = 0);
    int SSMWriteNTypes(int id, int data_len, int *data_buf, int offset = 0);

    bool SSMRecovery();

    //PQ mode
    int SSMSavePictureMode(int offset, int rw_val);
    int SSMReadPictureMode(int offset, int *rw_val);
    int SSMSaveLastPictureMode(int offset, int rw_val);
    int SSMReadLastPictureMode(int offset, int *rw_val);
    //Color Temperature
    int SSMSaveColorTemperature(int offset, int rw_val);
    int SSMReadColorTemperature(int offset, int *rw_val);
    int SSMSaveColorDemoMode(unsigned char rw_val);
    int SSMReadColorDemoMode(unsigned char *rw_val);
    int SSMSaveColorBaseMode(unsigned char rw_val);
    int SSMReadColorBaseMode(unsigned char *rw_val);
    int SSMSaveRGBGainRStart(int offset, int rw_val);
    int SSMReadRGBGainRStart(int offset, int *rw_val);
    int SSMSaveRGBGainGStart(int offset, int rw_val);
    int SSMReadRGBGainGStart(int offset, int *rw_val);
    int SSMSaveRGBGainBStart(int offset, int rw_val);
    int SSMReadRGBGainBStart(int offset, int *rw_val);
    int SSMSaveRGBPostOffsetRStart(int offset, int rw_val);
    int SSMReadRGBPostOffsetRStart(int offset, int *rw_val);
    int SSMSaveRGBPostOffsetGStart(int offset, int rw_val);
    int SSMReadRGBPostOffsetGStart(int offset, int *rw_val);
    int SSMSaveRGBPostOffsetBStart(int offset, int rw_val);
    int SSMReadRGBPostOffsetBStart(int offset, int *rw_val);
    int SSMReadRGBOGOValue(int offset, int size, unsigned char data_buf[]);
    int SSMSaveRGBOGOValue(int offset, int size, unsigned char data_buf[]);
    int SSMSaveRGBValueStart(int offset, int8_t rw_val);
    int SSMReadRGBValueStart(int offset, int8_t *rw_val);
    int SSMSaveColorSpaceStart(unsigned char rw_val);
    int SSMReadColorSpaceStart(unsigned char *rw_val);
    int ReadDataFromFile(const char *file_name, int offset, int nsize, unsigned char data_buf[]);
    int SaveDataToFile(const char *file_name, int offset, int nsize, unsigned char data_buf[]);
    //Brightness
    int SSMSaveBrightness(int offset, int rw_val);
    int SSMReadBrightness(int offset, int *rw_val);
    //construct
    int SSMSaveContrast(int offset, int rw_val);
    int SSMReadContrast(int offset, int *rw_val);
    //saturation
    int SSMSaveSaturation(int offset, int rw_val);
    int SSMReadSaturation(int offset, int *rw_val);
    //hue
    int SSMSaveHue(int offset, int rw_val);
    int SSMReadHue(int offset, int *rw_val);
    //Sharpness
    int SSMSaveSharpness(int offset, int rw_val);
    int SSMReadSharpness(int offset, int *rw_val);
    //NoiseReduction
    int SSMSaveNoiseReduction(int offset, int rw_val);
    int SSMReadNoiseReduction(int offset, int *rw_val);
    //SmoothPlus
    int SSMSaveSmoothPlus(int offset, int rw_val);
    int SSMReadSmoothPlus(int offset, int *rw_val);
    //HDR TMO
    int SSMSaveHdrTmoVal(int offset, int rw_val);
    int SSMReadHdrTmoVal(int offset, int *rw_val);
    //Gamma
    int SSMSaveGammaValue(int offset, int rw_val);
    int SSMReadGammaValue(int offset, int *rw_val);
    //WhiteBalance
    bool SetWhitebalanceGammaData(WB_GAMMA_TABLE *pData, int src, int timming, int level);
    bool GetWhitebalanceGammaData(WB_GAMMA_TABLE *pData, int src, int timming, int level);
    bool CriDataGetWhitebalanceGammaData(WB_GAMMA_TABLE *pData, int level);
    bool CriDataSetWhitebalanceGammaData(WB_GAMMA_TABLE *pData, int level);
    //PQModuleDemoState
    int SSMSavePQModuleDemoState(int offset, int rw_val);
    int SSMReadPQModuleDemoState(int offset, int *rw_val);
    //Memc
    int SSMSaveMemcMode(int offset, int rw_val);
    int SSMReadMemcMode(int offset, int *rw_val);
    int SSMSaveMemcDeblurLevel(int offset, int rw_val);
    int SSMReadMemcDeblurLevel(int offset, int *rw_val);
    int SSMSaveMemcDeJudderLevel(int offset, int rw_val);
    int SSMReadMemcDeJudderLevel(int offset, int *rw_val);
    //Edge enhance
    int SSMSaveEdgeEnhanceStatus(int offset, int rw_val);
    int SSMReadEdgeEnhanceStatus(int offset, int *rw_val);
    //mpeg NR
    int SSMSaveMpegNoiseReduction(int offset, int rw_val);
    int SSMReadMpegNoiseReduction(int offset, int *rw_val);
    //Dynamic contrast
    int SSMSaveDynamicContrast(int offset, int rw_val);
    int SSMReadDynamicContrast(int offset, int *rw_val);
    //Dynamic Backlight
    int SSMSaveDynamicBacklightMode(int rw_val);
    int SSMReadDynamicBacklightMode(int *rw_val);
    //backlight
    int SSMReadBackLightVal(int offset, int *rw_val);
    int SSMSaveBackLightVal(int offset, int rw_val);
    int SSMSaveDnlpMode(int offset, int rw_val);
    int SSMReadDnlpMode(int offset, int *rw_val);
    int SSMSaveDnlpGainValue(int offset, int rw_val);
    int SSMReadDnlpGainValue(int offset, int *rw_val);
    int SSMSaveEyeProtectionMode(int rw_val);
    int SSMReadEyeProtectionMode(int *rw_val);
    int SSMSaveDDRSSC(unsigned char rw_val);
    int SSMReadDDRSSC(unsigned char *rw_val);
    int SSMSaveLVDSSSC(int *rw_val);
    int SSMReadLVDSSSC(int *rw_val);

    int SSMSaveDisplayMode(int offset, int rw_val);
    int SSMReadDisplayMode(int offset, int *rw_val);
    int SSMSaveAutoAspect(int offset, int rw_val);
    int SSMReadAutoAspect(int offset, int *rw_val);
    int SSMSave43Stretch(int offset, int rw_val);
    int SSMRead43Stretch(int offset, int *rw_val);
    int SSMEdidRestoreDefault(int rw_val);
    int SSMHdcpSwitcherRestoreDefault(int rw_val);
    int SSMSColorRangeModeRestoreDefault(int rw_val);
    int SSMSaveLocalContrastMode(int offset, int rw_val);
    int SSMReadLocalContrastMode(int offset, int *rw_val);
    int SSMSaveBlackExtensionMode(int offset, int rw_val);
    int SSMReadBlackExtensionMode(int offset, int *rw_val);
    int SSMSaveDeblockMode(int offset, int rw_val);
    int SSMReadDeblockMode(int offset, int *rw_val);
    int SSMSaveDemoSquitoMode(int offset, int rw_val);
    int SSMReadDemoSquitoMode(int offset, int *rw_val);
    int SSMSaveMcDiMode(int offset, int rw_val);
    int SSMReadMcDiMode(int offset, int *rw_val);
    int SSMReadAipqEnableVal(int *rw_val);
    int SSMSaveAipqEnableVal(int rw_val);
    int SSMReadAipqMode(int *rw_val);
    int SSMSaveAipqMode(int rw_val);
    int SSMReadAiSrEnable(int *rw_val);
    int SSMSaveAiSrEnable(int rw_val);
    int SSMReadAiSrMode(int *rw_val);
    int SSMSaveAiSrMode(int rw_val);
    int SSMReadAiColor(int *rw_val);
    int SSMSaveAiColor(int rw_val);
    int SSMReadDLGEnable(int *rw_val);
    int SSMSaveDLGEnable(int rw_val);
    int SSMSaveColorGamutMode(int offset, int rw_val);
    int SSMReadColorGamutMode(int offset, int *rw_val);
    int SSMSaveBlackStretch(int offset, int rw_val);
    int SSMReadBlackStretch(int offset, int *rw_val);
    int SSMSaveBlueStretch(int offset, int rw_val);
    int SSMReadBlueStretch(int offset, int *rw_val);
    int SSMSaveChromaCoring(int offset, int rw_val);
    int SSMReadChromaCoring(int offset, int *rw_val);
    int SSMSaveLocalDimming(int rw_val);
    int SSMReadLocalDimming(int *rw_val);
    int SSMSavePictureModeParamsFlag(int offset, int rw_val);
    int SSMReadPictureModeParamsFlag(int offset, int *rw_val);
    int SSMSavePictureModeParams(int offset, int size, int *rw_val);
    int SSMReadPictureModeParams(int offset, int size, int *rw_val);
    int SSMSaveDvApoPictureParams(int offset, int size, int *rw_val);
    int SSMReadDvApoPictureParams(int offset, int size, int *rw_val);

    int m_dev_fd;
    static SSMAction *mInstance;
    static SSMHandler *mSSMHandler;
    class ISSMActionObserver {
        public:
            ISSMActionObserver() {};
            virtual ~ISSMActionObserver() {};
            virtual void resetAllUserSettingParam() {};
    };

    void setObserver (ISSMActionObserver *pOb)
    {
        mpObserver = pOb;
    };
private:
    ISSMActionObserver *mpObserver;
    char mWhiteBalanceFilePath[128];
};
#endif
