/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *  @author   tellen
 *  @version  1.0
 *  @date     2017/10/18
 *  @par function description:
 *  - 1 system control apis for other vendor process
 */

#ifndef ANDROID_SYSTEMCONTROLCLIENT_H
#define ANDROID_SYSTEMCONTROLCLIENT_H

#include <utils/Errors.h>
#include <string>
#include <vector>
#include "Rect.h"
#include "PQType.h"
#include <vendor/amlogic/hardware/systemcontrol/1.1/ISystemControl.h>
using ::vendor::amlogic::hardware::systemcontrol::V1_1::ISystemControl;
using ::vendor::amlogic::hardware::systemcontrol::V1_0::ISystemControlCallback;
using ::vendor::amlogic::hardware::systemcontrol::V1_0::Result;
using ::vendor::amlogic::hardware::systemcontrol::V1_0::DroidDisplayInfo;
using ::vendor::amlogic::hardware::systemcontrol::V1_0::SourceInputParam;
using ::vendor::amlogic::hardware::systemcontrol::V1_0::NolineParam;
using ::vendor::amlogic::hardware::systemcontrol::V1_0::OverScanParam;
using ::vendor::amlogic::hardware::systemcontrol::V1_0::WhiteBalanceParam;
using ::vendor::amlogic::hardware::systemcontrol::V1_0::PQDatabaseInfo;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_array;
using ::android::hardware::Return;
using ::android::hardware::Void;

namespace android {

class SysCtrlListener : virtual public RefBase {
public:
    virtual void notify(int event) = 0;
    virtual void notifyFBCUpgrade(int state, int param) = 0;
    virtual void onSetDisplayMode(int mode) = 0;
    virtual void onHdrInfoChange(int newHdrInfo) = 0;
    virtual void onAudioEvent(int param1, int param2, int param3, int param4) = 0;
};

class SystemControlClient  : virtual public RefBase {
private:
    SystemControlClient();

public:
    bool getProperty(const std::string& key, std::string& value);
    bool getPropertyString(const std::string& key, std::string& value, std::string& def);
    int32_t getPropertyInt(const std::string& key, int32_t def);
    int64_t getPropertyLong(const std::string& key, int64_t def);

    bool getPropertyBoolean(const std::string& key, bool def);
    void setProperty(const std::string& key, const std::string& value);

    bool writeHdcpRXImg(const std::string& path);
    bool updataLogoBmp(const std::string& path);

    bool readSysfs(const std::string& path, std::string& value);
    bool writeSysfs(const std::string& path, const std::string& value);
    bool writeSysfs(const std::string& path, const char *value, const int size);

    //Provision key start
    bool writeUnifyKey(const std::string& path, const std::string& value);
    bool writePlayreadyKey(const char *value, const int size);
    bool writeNetflixKey(const char *value, const int size);
    bool writeWidevineKey( const char *value, const int size);
    bool writeAttestationKey(const char *value, const int size);
    bool writeHDCP14Key( const char *value, const int size);
    bool writeHdcpRX14Key(const char *value, const int size);
    bool writeHDCP22Key(const char *value, const int size);
    bool writeHdcpRX22Key(const char *value, const int size);
    bool writePFIDKey(const char *value, const int size);
    bool writePFPKKey(const char *value, const int size);

    bool readUnifyKey(const std::string& path, std::string& value);
    bool readPlayreadyKey(const std::string& path, const uint32_t key_type, int size);
    bool readNetflixKey(const uint32_t key_type, int size);
    bool readWidevineKey(const uint32_t key_type, int size);
    bool readAttestationKey(const uint32_t key_type, int size);
    bool readHDCP14Key(const uint32_t key_type, int size);
    bool readHdcpRX14Key(const uint32_t key_type, int size);
    bool readHDCP22Key(const uint32_t key_type, int size);
    bool readHdcpRX22Key(const uint32_t key_type, int size);

    bool checkPlayreadyKey(const std::string& path, const char *value, const uint32_t key_type, int size);
    bool checkNetflixKey(const char *value, const uint32_t key_type, int size);
    bool checkWidevineKey(const char *value, const uint32_t key_type, int size);
    bool checkAttestationKey(const char *value, const uint32_t key_type, int size);
    bool checkHDCP14Key(const char *value, const uint32_t key_type, int size);
    bool checkHDCP14KeyIsExist(const uint32_t key_type);
    bool checkHDCP22Key(const std::string& path, const char *value, const uint32_t key_type, int size);
    bool checkHDCP22KeyIsExist(const uint32_t key_type_first, const uint32_t key_type_second);
    bool checkPFIDKeyIsExist(const uint32_t key_type);
    bool checkPFPKKeyIsExist(const uint32_t key_type);
    bool calcChecksumKey(const char *value, const int size, std::string& keyCheckSum);
    //Provision key end

    void setBootEnv(const std::string& key, const std::string& value);
    bool getBootEnv(const std::string& key, std::string& value);
    void getDroidDisplayInfo(int &type, std::string& socType, std::string& defaultUI,
        int &fb0w, int &fb0h, int &fb0bits, int &fb0trip,
        int &fb1w, int &fb1h, int &fb1bits, int &fb1trip);

    void loopMountUnmount(int &isMount, const std::string& path);

    void setMboxOutputMode(const std::string& mode);
    void setSinkOutputMode(const std::string& mode);

    void setDigitalMode(const std::string& mode);
    void setListener(const sp<ISystemControlCallback> callback);
    void setOsdMouseMode(const std::string& mode);
    void setOsdMousePara(int x, int y, int w, int h);
    void setPosition(int left, int top, int width, int height);
    void getPosition(const std::string& mode, int &x, int &y, int &w, int &h);
    void getDeepColorAttr(const std::string& mode, std::string& value);
    void saveDeepColorAttr(const std::string& mode, const std::string& dcValue);
    int64_t resolveResolutionValue(const std::string& mode);
    void setDolbyVisionEnable(int state);
    bool isTvSupportDolbyVision(std::string& mode);
    int32_t getDolbyVisionType();
    void setGraphicsPriority(const std::string& mode);
    void getGraphicsPriority(std::string& mode);
    void setHdrMode(const std::string& mode);
    void setSdrMode(const std::string& mode);

    int32_t set3DMode(const std::string& mode3d);
    void init3DSetting(void);
    int getVideo3DFormat(void);
    int getDisplay3DTo2DFormat(void);
    bool setDisplay3DTo2DFormat(int format);
    bool setDisplay3DFormat(int format);
    int getDisplay3DFormat(void);
    bool setOsd3DFormat(int format);
    bool switch3DTo2D(int format);
    bool switch2DTo3D(int format);
    void autoDetect3DForMbox(void);
    bool getSupportDispModeList(std::vector<std::string>& supportDispModes);
    bool getActiveDispMode(std::string& activeDispMode);
    bool setActiveDispMode(std::string& activeDispMode);

    void isHDCPTxAuthSuccess(int &status);
    bool getModeSupportDeepColorAttr(const std::string& mode, const std::string& color);
    void setHdrStrategy(const std::string& value);
    void setHdrPriority(const std::string& value);
    //PQ
    int loadPQSettings(source_input_param_t srcInputParam);
    int setPQmode(int mode, int isSave, int is_autoswitch);
    int getPQmode(void);
    int savePQmode(int mode);
    int getLastPQmode(void);
    int setColorTemperature(int mode, int isSave);
    int getColorTemperature(void);
    int saveColorTemperature(int mode);
    int setColorTemperatureUserParam(int mode, int isSave, int param_type, int value);
    tcon_rgb_ogo_t getColorTemperatureUserParam(void);
    int setBrightness(int value, int isSave);
    int getBrightness(void);
    int saveBrightness(int value);
    int setContrast(int value, int isSave);
    int getContrast(void);
    int saveContrast(int value);
    int setSaturation(int value, int isSave);
    int getSaturation(void);
    int saveSaturation(int value);
    int setHue(int value, int isSave );
    int getHue(void);
    int saveHue(int value);
    int setSharpness(int value, int is_enable, int isSave);
    int getSharpness(void);
    int saveSharpness(int value);
    int setNoiseReductionMode(int nr_mode, int isSave);
    int getNoiseReductionMode(void);
    int saveNoiseReductionMode(int nr_mode);
    int setSmoothPlusMode(int smoothplus_mode, int isSave);
    int getSmoothPlusMode(void);
    int setHDRTMOMode(int hdr_tmo_mode, int isSave);
    int getHDRTMOMode(void);
    int setEyeProtectionMode(int source_input, int enable, int isSave);
    int getEyeProtectionMode(int source_input);
    int setGammaValue(int gamma_curve, int isSave);
    int getGammaValue(void);
    bool hasMemcFunc();
    int setMemcMode(int memc_mode, int isSave);
    int getMemcMode(void);
    int setMemcDeBlurLevel(int level, int isSave);
    int getMemcDeBlurLevel(void);
    int setMemcDeJudderLevel(int level, int isSave);
    int getMemcDeJudderLevel(void);
    int setDisplayMode(int source_input, int mode, int isSave);
    int getDisplayMode(int source_input);
    int saveDisplayMode(int source_input, int mode);
    int setBacklight(int value, int isSave);
    int getBacklight(void);
    int saveBacklight(int value);
    int setDynamicBacklight(int mode, int isSave);
    int getDynamicBacklight(void);
    int setLocalContrastMode(int mode, int isSave);
    int getLocalContrastMode();
    int setBlackExtensionMode(int mode, int isSave);
    int getBlackExtensionMode();
    int setDeblockMode(int mode, int isSave);
    int getDeblockMode();
    int setDemoSquitoMode(int mode, int isSave);
    int getDemoSquitoMode();
    int setColorBaseMode(int mode, int isSave);
    int getColorBaseMode();
    int getSourceHdrType();
    bool checkLdimExist(void);
    int factoryResetPQMode(void);
    int factorySetPQMode_Brightness(int inputSrc, int sig_fmt, int trans_fmt, int pq_mode, int value);
    int factoryGetPQMode_Brightness(int inputSrc, int sig_fmt, int trans_fmt, int pq_mode);
    int factorySetPQMode_Contrast(int inputSrc, int sig_fmt, int trans_fmt, int pq_mode, int value);
    int factoryGetPQMode_Contrast(int inputSrc, int sig_fmt, int trans_fmt, int pq_mode);
    int factorySetPQMode_Saturation(int inputSrc, int sig_fmt, int trans_fmt, int pq_mode, int value);
    int factoryGetPQMode_Saturation(int inputSrc, int sig_fmt, int trans_fmt, int pq_mode);
    int factorySetPQMode_Hue(int inputSrc, int sig_fmt, int trans_fmt, int pq_mode, int value);
    int factoryGetPQMode_Hue(int inputSrc, int sig_fmt, int trans_fmt, int pq_mode);
    int factorySetPQMode_Sharpness(int inputSrc, int sig_fmt, int trans_fmt, int pq_mode, int value);
    int factoryGetPQMode_Sharpness(int inputSrc, int sig_fmt, int trans_fmt, int pq_mode);
    int factoryResetColorTemp(void);
    int factorySetOverscan(int inputSrc, int sigFmt, int transFmt, int he_value, int hs_value, int ve_value, int vs_value);
    tvin_cutwin_t factoryGetOverscan(int inputSrc, int sigFmt, int transFmt);
    int factorySetNolineParams(int inputSrc, int sigFmt, int transFmt, int type, int osd0_value, int osd25_value,
                                        int osd50_value, int osd75_value, int osd100_value);
    noline_params_t factoryGetNolineParams(int inputSrc, int sigFmt, int transFmt, int type);
    int factoryfactoryGetColorTemperatureParams(int colorTemp_mode);
    int factorySetParamsDefault(void);
    int factorySSMRestore(void);
    int factoryResetNonlinear(void);
    int factorySetGamma(int gamma_r, int gamma_g, int gamma_b);
    int sysSSMReadNTypes(int id, int data_len, int offset);
    int sysSSMWriteNTypes(int id, int data_len, int data_buf, int offset);
    int getActualAddr(int id);
    int getActualSize(int id);
    int SSMRecovery(void);
    int setPLLValues(source_input_param_t srcInputParam);
    int setCVD2Values(void);
    int getSSMStatus(void);
    int setCurrentHdrInfo(int32_t hdrInfo);
    int setCurrentAspectRatioInfo(int32_t aspectRatioInfo);
    int setCurrentSourceInfo(int32_t sourceInput, int32_t sigFmt, int32_t transFmt);
    void getCurrentSourceInfo(int32_t &sourceInput, int32_t &sigFmt, int32_t &transFmt);
    int setwhiteBalanceGainRed(int32_t inputSrc, int sig_fmt, int trans_fmt, int32_t colortemp_mode, int32_t value);
    int setwhiteBalanceGainGreen(int32_t inputSrc, int sig_fmt, int trans_fmt, int32_t colortemp_mode, int32_t value);
    int setwhiteBalanceGainBlue(int32_t inputSrc, int sig_fmt, int trans_fmt, int32_t colortemp_mode, int32_t value);
    int setwhiteBalanceOffsetRed(int32_t inputSrc, int sig_fmt, int trans_fmt, int32_t colortemp_mode, int32_t value);
    int setwhiteBalanceOffsetGreen(int32_t inputSrc, int sig_fmt, int trans_fmt, int32_t colortemp_mode, int32_t value);
    int setwhiteBalanceOffsetBlue(int32_t inputSrc, int sig_fmt, int trans_fmt, int32_t colortemp_mode, int32_t value);
    int getwhiteBalanceGainRed(int32_t inputSrc, int sig_fmt, int trans_fmt, int32_t colortemp_mode);
    int getwhiteBalanceGainGreen(int32_t inputSrc, int sig_fmt, int trans_fmt, int32_t colortemp_mode);
    int getwhiteBalanceGainBlue(int32_t inputSrc, int sig_fmt, int trans_fmt, int32_t colortemp_mode);
    int getwhiteBalanceOffsetRed(int32_t inputSrc, int sig_fmt, int trans_fmt, int32_t colortemp_mode);
    int getwhiteBalanceOffsetGreen(int32_t inputSrc, int sig_fmt, int trans_fmt, int32_t colortemp_mode);
    int getwhiteBalanceOffsetBlue(int32_t inputSrc, int sig_fmt, int trans_fmt, int32_t colortemp_mode);
    int saveWhiteBalancePara(int32_t sourceType, int sig_fmt, int trans_fmt, int32_t colorTemp_mode, int32_t r_gain, int32_t g_gain, int32_t b_gain, int32_t r_offset, int32_t g_offset, int32_t b_offset);
    int getRGBPattern();
    int setRGBPattern(int32_t r, int32_t g, int32_t b);
    int factorySetDDRSSC(int32_t step);
    int factoryGetDDRSSC();
    int factorySetLVDSSSC(int32_t step);
    int factoryGetLVDSSSC();
    int whiteBalanceGrayPatternClose();
    int whiteBalanceGrayPatternOpen();
    int whiteBalanceGrayPatternSet(int32_t value);
    int whiteBalanceGrayPatternGet();
    int factorySetHdrMode(int mode);
    int factoryGetHdrMode(void);
    int setDnlpParams(int inputSrc, int32_t sigFmt, int32_t transFmt, int level);
    int getDnlpParams(int inputSrc, int32_t sigFmt, int32_t transFmt);
    int factorySetDnlpParams(int inputSrc, int32_t sigFmt, int32_t transFmt, int level, int final_gain);
    int factoryGetDnlpParams(int inputSrc, int32_t sigFmt, int32_t transFmt, int level);
    int factorySetBlackExtRegParams(int inputSrc, int32_t sigFmt, int32_t transFmt, int val);
    int factoryGetBlackExtRegParams(int inputSrc, int32_t sigFmt, int32_t transFmt);
    int factorySetColorParams(int inputSrc, int32_t sigFmt, int32_t transFmt, int color_type, int color_param, int val);
    int factoryGetColorParams(int inputSrc, int32_t sigFmt, int32_t transFmt, int color_type, int color_param);
    int factorySetNoiseReductionParams(int inputSrc, int sig_fmt, int trans_fmt, int nr_mode, int param_type, int val);
    int factoryGetNoiseReductionParams(int inputSrc, int sig_fmt, int trans_fmt, int nr_mode, int param_type);
    int factorySetCTIParams(int inputSrc, int sig_fmt, int trans_fmt, int param_type, int val);
    int factoryGetCTIParams(int inputSrc, int sig_fmt, int trans_fmt, int param_type);
    int factorySetDecodeLumaParams(int inputSrc, int sig_fmt, int trans_fmt, int param_type, int val);
    int factoryGetDecodeLumaParams(int inputSrc, int sig_fmt, int trans_fmt, int param_type);
    int factorySetSharpnessParams(int inputSrc, int sig_fmt, int trans_fmt, int isHD, int param_type, int val);
    int factoryGetSharpnessParams(int inputSrc, int sig_fmt, int trans_fmt, int isHD,int param_type);
    void getChipVersionInfo(std::string& chipversion);
    PQDatabaseInfo getPQDatabaseInfo(int32_t dataBaseName);
    int setDtvKitSourceEnable(int isEnable);
    bool setAipqEnable(int isEnable);
    bool hasAipqFunc();
    bool getAipqEnable();
    bool readAiPqTable(std::string& aiPqTable);
    int setColorGamutMode(int mode, int is_save);
    int getColorGamutMode(void);
    //PQ end

    //memc
    bool memcContrl(int isEnable);
    bool scCpyFile(const std::string& src,const std::string& dest, bool usethread);

    //aisr
    bool aisrContrl(int isEnable);
    bool hasAisrFunc();
    bool getAisr();

    //static frame
    int setStaticFrameEnable(int enable, int isSave);
    int getStaticFrameEnable();
    //screen color
    int setScreenColorForSignalChange(int screenColor, int isSave);
    int getScreenColorForSignalChange();
    int setVideoScreenColor(int color);
    //FBC
    int StartUpgradeFBC(const std::string&file_name, int mode, int upgrade_blk_size);
    int UpdateFBCUpgradeStatus(int state, int param);
    int setAudioParam(int param1, int param2, int param3, int param4 = -1);

    void setListener(const sp<SysCtrlListener> &listener);
    static SystemControlClient * getInstance();
    bool frameRateDisplay(bool on,droidlogic::Rect rect);
 private:
     class SystemControlHidlCallback : public ISystemControlCallback {
     public:
         SystemControlHidlCallback(SystemControlClient *client): SysCtrlClient(client) {};
         Return<void> notifyCallback(const int event) override;
         Return<void> notifyFBCUpgradeCallback(int state, int param) override;
         Return<void> notifySetDisplayModeCallback(int mode) override;
         Return<void> notifyHdrInfoChangedCallback(int newHdrInfo) override;
         Return<void> notifyAudioCallback(int param1, int param2, int param3, int param4) override;
     private:
         SystemControlClient *SysCtrlClient;
     };
    struct SystemControlDeathRecipient : public android::hardware::hidl_death_recipient  {
        // hidl_death_recipient interface
        virtual void serviceDied(uint64_t cookie,
            const ::android::wp<::android::hidl::base::V1_0::IBase>& who) override;
    };
    sp<SystemControlDeathRecipient> mDeathRecipient = nullptr;
    static SystemControlClient *mInstance;

    sp<ISystemControl> mSysCtrl;
    sp<SysCtrlListener> mListener;
    sp<SystemControlHidlCallback> mSystemControlHidlCallback = nullptr;

};

}; // namespace android

#endif // ANDROID_SYSTEMCONTROLCLIENT_H
