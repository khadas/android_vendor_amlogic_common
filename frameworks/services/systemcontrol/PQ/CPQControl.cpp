/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "SystemControl"
#define LOG_TV_TAG "CPQControl"

#include <cutils/properties.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <dlfcn.h>

#include "CPQControl.h"
#include "tconless/demura/CTconDemura.h"
#include "tconless/pgamma/CTconPGamma.h"

#include "pq/aml_hal_pq.h"
#include "panel/aml_hal_lcd.h"
#include "DV/aml_hal_dv.h"

#ifdef SUPPORT_TVSERVICE
#include "TvServerHidlClient.h"
#endif

#define PROP_DEMURA_AUTO_GEN "ro.vendor.demura.autogen"
#define PROP_PGAMMA_AUTO_GEN "ro.vendor.pgamma.autogen"

#define PI 3.14159265358979
CPQControl *CPQControl::mInstance = NULL;
CPQControl *CPQControl::GetInstance()
{
    if (NULL == mInstance)
        mInstance = new CPQControl();
    return mInstance;
}

CPQControl::CPQControl()
{
    mAmvideoFd  =-1;
    mDiFd = -1;
    mLdFd = -1;
    mMemcFd = -1;
    mLcdFd =  -1;
    mVideoTunelFd = -1;
    mCurrentNodeNumber = 0;
    mSourceInputForSaveParam = SOURCE_MPEG;
    mCurrentAfdInfo = TVIN_ASPECT_NULL;
    mInitialized   = false;
    mCurrentHdrStatus = false;
    mbDtvKitEnable = false;
    mbDatabaseMatchChipStatus = false;
    mbCpqCfg_whitebalance_enable = false;
    mbCpqCfg_dnlp_enable = false;
    mbCpqCfg_xvycc_enable = false;
    mbCpqCfg_display_overscan_enable=  false;
    mbCpqCfg_local_contrast_enable = false;
    mbCpqCfg_hdmi_out_with_fbc_enable = false;
    mbCpqCfg_pq_param_check_source_enable = false;
    mbCpqCfg_ai_enable = false;
    mbCpqCfg_aad_enable = false;
    mbCpqCfg_cabc_enable = false;
    mbCpqCfg_smoothplus_enable = false;
    mbCpqCfg_hdrtmo_enable = false;
    mbCpqCfg_memc_enable = false;
    mbCpqCfg_separate_black_blue_chorma_db_enable = false;
    mbCpqCfg_bluestretch_enable = false;
    mbCpqCfg_chroma_coring_enable = false;
    mbCpqCfg_LocalDimming_enable = false;
    mbCpqCfg_aisr_enable = false;
    mbCpqCfg_aicolor_enable = false;
    mbCpqCfg_new_picture_mode_enable = false;
    mbCpqCfg_separate_db_enable = false;
    mbCpqCfg_amvecm_basic_enable = false;
    mbCpqCfg_amvecm_basic_withOSD_enable = false;
    mbCpqCfg_contrast_rgb_enable = false;
    mbCpqCfg_contrast_rgb_withOSD_enable = false;
    mbCpqCfg_blackextension_enable = false;
    mbCpqCfg_sharpness0_enable = false;
    mbCpqCfg_sharpness1_enable = false;
    mbCpqCfg_sharpnesspi_enable = false;
    mbCpqCfg_di_enable = false;
    mbCpqCfg_mcdi_enable = false;
    mbCpqCfg_deblock_enable = false;
    mbCpqCfg_nr_enable = false;
    mbCpqCfg_demoSquito_enable = false;
    mbCpqCfg_gamma_enable = false;
    mbCpqCfg_cm2_enable = false;
    mPQdb = NULL;
    mpOverScandb = NULL;
    mSSMAction =  NULL;
    pqSysWrite = NULL;
    mPQConfigFile = NULL;
    mHlgToneMapping =  NULL;
    mDolbyVision = NULL;
    memset(rgbfrompq, 0, sizeof(tcon_rgb_ogo_t));
    memset(&mCurrentSourceInputInfo, 0, sizeof(source_input_param_t));
    memset(&mCurrentPqSource, 0, sizeof(pq_src_param_t));
    memset(&mCurrentOutputType, 0, sizeof(output_type_t));
}

CPQControl::~CPQControl()
{

}

void CPQControl::CPQControlInit()
{
    mInitialized   = false;
    mAmvideoFd     = -1;
    mDiFd          = -1;
    mMemcFd        = -1;
    mLdFd          = -1;
    mLcdFd         = -1;
    mbDtvKitEnable = false;
    mVideoTunelFd  = -1;

    SYS_LOGD("CPQControlInit start!\n");

    AML_HAL_PQ_INIT();
    AML_HAL_AMDOLBY_INIT();
    AML_HAL_LCD_INIT();

    //open vpp module
    mAmvideoFd = VPPOpenModule();
    if (mAmvideoFd < 0) {
        SYS_LOGE("Open PQ module failed\n");
    } else {
        SYS_LOGD("Open PQ module success\n");
    }
    //open DI module
    mDiFd = DIOpenModule();
    if (mDiFd < 0) {
        SYS_LOGE("Open DI module failed!\n");
    } else {
        SYS_LOGD("Open DI module success!\n");
    }
    //open MEMC module
    mMemcFd = MEMCOpenModule();
    if (mMemcFd < 0) {
        SYS_LOGE("Open MEMC module failed!\n");
    } else {
        SYS_LOGD("Open MEMC module success!\n");
    }
    //open LCD module
    mLcdFd = LCDOpenModule();
    if (mLcdFd < 0) {
        SYS_LOGE("Open LCD module failed!\n");
    } else {
        SYS_LOGD("Open LCD module success!\n");
    }
    //open VT module
    mVideoTunelFd = OpenVideotunnel();
    if (mVideoTunelFd < 0) {
        SYS_LOGE("Open VideoTunel module failed!\n");
    } else {
        SYS_LOGD("Open VideoTunel module success!\n");
    }

    //open Sys fs
    pqSysWrite = SysWrite::GetInstance();

    //Load config file
    mPQConfigFile = CConfigFile::GetInstance();
    SetFlagByCfg();

    //open DB
    int ret = -1;
    char dstPqDbPath[128] = {0};
    mPQdb = new CPQdb();
    mPQConfigFile->GetPqdbPath(dstPqDbPath);
    ret = mPQdb->openPqDB(dstPqDbPath);
    if (ret != 0) {
        mbDatabaseMatchChipStatus = false;
        SYS_LOGE("open pq DB failed!\n");
    } else {
        SYS_LOGD("open pq DB success!\n");
        mbDatabaseMatchChipStatus = isPqDatabaseMachChip();
    }

    //open overscan DB
    if (mbCpqCfg_separate_db_enable) {
        char dstOverscanDbPath[128] = {0};
        mpOverScandb = new COverScandb();
        mPQConfigFile->GetOverscandbPath(dstOverscanDbPath);
        ret = mpOverScandb->openOverScanDB(dstOverscanDbPath);
        if (ret != 0) {
            SYS_LOGE("open overscan DB failed!\n");
        } else {
            SYS_LOGD("open overscan DB success!\n");
        }
    }

    //SSM file check
    char SsmDataPath[128]       = {0};
    char SsmDataHandlerPath[128] = {0};
    char WBPath[128]            = {0};
    mPQConfigFile->GetSSMDataPath(SsmDataPath);
    mPQConfigFile->GetSSMDataHandlerPath(SsmDataHandlerPath);
    mPQConfigFile->GetWBFilePath(WBPath);
    mSSMAction = SSMAction::getInstance();
    mSSMAction->setObserver(this);
    mSSMAction->init(SsmDataPath, SsmDataHandlerPath, WBPath);
    //init source
    mCurrentSourceInputInfo.source_input = SOURCE_MPEG;
    mCurrentSourceInputInfo.sig_fmt      = TVIN_SIG_FMT_HDMI_1920X1080P_60HZ;
    mCurrentSourceInputInfo.trans_fmt    = TVIN_TFMT_2D;
    mSourceInputForSaveParam            = SOURCE_MPEG;
    mCurrentHdrStatus                   = false;
    mCurrentAfdInfo                      = TVIN_ASPECT_NULL;
    mCurrentPqSource.pq_source_input     = SOURCE_MPEG;
    mCurrentPqSource.pq_sig_fmt          = PQ_FMT_DEFAULT;
    mCurrentNodeNumber                  = 0;

    //check output mode
    mCurrentOutputType = CheckOutPutMode(SOURCE_MPEG);

    //load DV config file
    char dvbinpath[128] = {0};
    char dvcfgpath[128] = {0};
    mPQConfigFile->GetDvFilePath(dvbinpath, dvcfgpath);
    mDolbyVision = new CDolbyVision(dvbinpath, dvcfgpath);

    //HLG tone mapping init
    mHlgToneMapping = new CHlgToneMapping();
    int gainValue[149] = {0};
    mHlgToneMapping->hlg_sdr_process(350, gainValue);
    SetHDRTMData(gainValue);

    //Set DNLP
    if (mbCpqCfg_dnlp_enable) {
        Cpq_SetDNLPStatus(VE_DNLP_STATE_ON);
    } else {
        Cpq_SetDNLPStatus(VE_DNLP_STATE_OFF);
    }
    //screen color
    //SetScreenColorForSignalChange(GetScreenColorForSignalChange(), 0);
    //static frame
    SetStaticFrameEnable(GetStaticFrameEnable(), 0);
    //Load PQ
    if (LoadPQSettings() < 0) {
        SYS_LOGE("Load PQ failed!\n");
    } else {
        SYS_LOGD("Load PQ success!\n");
    }

    //set backlight
    BacklightInit();
    //auto backlight
    DynamicBackLightInit();
    //AI PQ
    AipqInit();
    //cabc pq
    SetCabc();
    //aad pq
    SetAad();
    //Vframe size
    mCDevicePollCheckThread = sp<CDevicePollCheckThread>::make();
    mCDevicePollCheckThread->setObserver(this);
    mCDevicePollCheckThread->StartCheck();
    mInitialized = true;
    InitAutoNr();
    //for tconless
    InitTconGamma();
    InitLocalDimmingBin();
    InitTconlessBin();
}

void CPQControl::CPQControlUnInit()
{
    //close moduel
    VPPCloseModule();
    //close DI module
    DICloseModule();
    //close VT module;
    CloseVideotunnel();

    if (mHlgToneMapping != NULL) {
        delete mHlgToneMapping;
        mHlgToneMapping = NULL;
    }

    if (mSSMAction!= NULL) {
        delete mSSMAction;
        mSSMAction = NULL;
    }

    if (mDolbyVision != NULL) {
        delete mDolbyVision;
        mDolbyVision = NULL;
    }

    if (mPQdb != NULL) {
        //closed DB
        mPQdb->closeDb();

        delete mPQdb;
        mPQdb = NULL;
    }

    if (mpOverScandb != NULL) {
        mpOverScandb->closeDb();

        delete mpOverScandb;
        mpOverScandb = NULL;
    }

    mCDevicePollCheckThread->requestExit();

    if (mPQConfigFile != NULL) {
        delete mPQConfigFile;
        mPQConfigFile = NULL;
    }

}

int CPQControl::pqWriteSys(ConstCharforSysNodeIndex index, const char *val)
{
    int len = -1;

    len = pqSysWrite->writeSysfs(index, val);

    return len;
}

int CPQControl::pqReadSys(ConstCharforSysNodeIndex index, char *buf, int count)
{
    int len = -1;

    len = pqSysWrite->readSysfs(index, buf, count);

    return len;
}

int CPQControl::VPPOpenModule(void)
{
    if (mAmvideoFd < 0) {
        mAmvideoFd = open(VPP_DEV_PATH, O_RDWR);
        if (mAmvideoFd < 0) {
            SYS_LOGE("Open amvecm module, error(%s)\n", strerror(errno));
            return -1;
        }
    } else {
        SYS_LOGD("vpp OpenModule has been opened before!\n");
    }

    return mAmvideoFd;
}

int CPQControl::VPPCloseModule(void)
{
    if (mAmvideoFd >= 0) {
        close ( mAmvideoFd);
        mAmvideoFd = -1;
    }
    return 0;
}

int CPQControl::VPPDeviceIOCtl(int request, ...)
{
    int ret = -1;
    va_list ap;
    void *arg;
    va_start(ap, request);
    arg = va_arg ( ap, void * );
    va_end(ap);
    ret = ioctl(mAmvideoFd, request, arg);
    return ret;
}

int CPQControl::DIOpenModule(void)
{
    if (mDiFd < 0) {
        mDiFd = open(DI_DEV_PATH, O_RDWR);

        SYS_LOGD("DI OpenModule path: %s", DI_DEV_PATH);

        if (mDiFd < 0) {
            SYS_LOGE("Open DI module, error(%s)\n", strerror(errno));
            return -1;
        }
    }

    return mDiFd;
}

int CPQControl::DICloseModule(void)
{
    if (mDiFd>= 0) {
        close ( mDiFd);
        mDiFd = -1;
    }
    return 0;
}

int CPQControl::DIDeviceIOCtl(int request, ...)
{
    int tmp_ret = -1;
    va_list ap;
    void *arg;
    va_start(ap, request);
    arg = va_arg ( ap, void * );
    va_end(ap);
    tmp_ret = ioctl(mDiFd, request, arg);
    return tmp_ret;
}

int CPQControl::AFEDeviceIOCtl ( int request, ... )
{
    int tmp_ret = -1;
    int afe_dev_fd = -1;
    va_list ap;
    void *arg;

    afe_dev_fd = open(AFE_DEV_PATH, O_RDWR );

    if ( afe_dev_fd >= 0 ) {
        va_start ( ap, request );
        arg = va_arg ( ap, void * );
        va_end ( ap );

        tmp_ret = ioctl ( afe_dev_fd, request, arg );

        close(afe_dev_fd);
        return tmp_ret;
    } else {
        SYS_LOGE ( "Open tvafe module error(%s).\n", strerror ( errno ));
        return -1;
    }
}

int CPQControl::LDOpenModule(void)
{
    if (!isFileExist(LDIM_PATH)) {
        SYS_LOGD("not support LocalDimming!\n");
        return -1;
    }

    if (mLdFd < 0) {
        mLdFd = open(LDIM_PATH, O_RDWR);
        if (mLdFd < 0) {
            SYS_LOGE("Open LocalDimming module, error(%s)!\n", strerror(errno));
            return -1;
        }
    } else {
        SYS_LOGD("LocalDimming OpenModule has been opened before!\n");
    }

    return mLdFd;
}

int CPQControl::LDCloseModule(void)
{
    if (mLdFd >= 0) {
        close ( mLdFd);
        mLdFd = -1;
    }
    return 0;
}

int CPQControl::LDDeviceIOCtl(int request, ...)
{
    int ret = -1;
    va_list ap;
    void *arg;
    va_start(ap, request);
    arg = va_arg ( ap, void * );
    va_end(ap);
    ret = ioctl(mLdFd, request, arg);
    return ret;
}

int CPQControl::MEMCOpenModule(void)
{
    if (mMemcFd < 0) {
        mMemcFd = open(CPQ_MEMC_SYSFS, O_RDWR);

        SYS_LOGD("MEMC OpenModule path: %s", CPQ_MEMC_SYSFS);

        if (mMemcFd < 0) {
            SYS_LOGE("Open MEMC module, error(%s)!\n", strerror(errno));
            return -1;
        }
    }

    return mMemcFd;
}

int CPQControl::MEMCCloseModule(void)
{
    if (mMemcFd>= 0) {
        close ( mMemcFd);
        mMemcFd = -1;
    }
    return 0;
}

int CPQControl::MEMCDeviceIOCtl(int request, ...)
{
    int tmp_ret = -1;
    va_list ap;
    void *arg;
    va_start(ap, request);
    arg = va_arg ( ap, void * );
    va_end(ap);
    tmp_ret = ioctl(mMemcFd, request, arg);
    return tmp_ret;
}

int CPQControl::LCDOpenModule(void)
{
    if (mLcdFd < 0) {
        mLcdFd = open(CPQ_LCD_SYSFS, O_RDWR);

        SYS_LOGD("LCD OpenModule path: %s", CPQ_LCD_SYSFS);

        if (mLcdFd < 0) {
            SYS_LOGE("Open LCD module, error(%s)!\n", strerror(errno));
            return -1;
        }
    }

    return mLcdFd;
}

int CPQControl::LCDCloseModule(void)
{
    if (mLcdFd>= 0) {
        close ( mLcdFd);
        mLcdFd = -1;
    }
    return 0;
}

int CPQControl::LCDDeviceIOCtl(int request, ...)
{
    int tmp_ret = -1;
    va_list ap;
    void *arg;
    va_start(ap, request);
    arg = va_arg ( ap, void * );
    va_end(ap);
    tmp_ret = ioctl(mLcdFd, request, arg);
    return tmp_ret;
}

void CPQControl::onVframeSizeChange()
{
    char temp[8];
    memset(temp, 0, sizeof(temp));
    int ret = pqReadSys(VIDEO_POLL_STATUS_CHANGE, temp, sizeof(temp));
    if (ret > 0) {
        int eventFlagValue = strtol(temp, NULL, 16);
        SYS_LOGD("%s: event value = %d(0x%x)\n", __FUNCTION__, eventFlagValue, eventFlagValue);
        int framesizeEventFlag      = (eventFlagValue & 0x1) >> 0;
        int hdrTypeEventFlag        = (eventFlagValue & 0x2) >> 1;
        int videoPlayStartEventFlag = (eventFlagValue & 0x4) >> 2;
        int videoPlayStopEventFlag  = (eventFlagValue & 0x8) >> 3;
        //int videoPlayAxisEventFlag = (eventFlagValue & 0x10) >> 4;
        /*SYS_LOGD("%s: framesizeEventFlag = %d,hdrTypeEventFlag = %d,videoPlayStartEventFlag = %d,videoPlayStopEventFlag = %d,videoPlayAxisEventFlag = %d!\n",
                 __FUNCTION__, framesizeEventFlag, hdrTypeEventFlag, videoPlayStartEventFlag, videoPlayStopEventFlag,
                 videoPlayAxisEventFlag);*/
        //check video play start or stop
        if ((videoPlayStartEventFlag == 1) && (videoPlayStopEventFlag == 0)) {
            mbVideoIsPlaying = true;
        } else if ((videoPlayStartEventFlag == 0) && (videoPlayStopEventFlag == 1)) {
            mbVideoIsPlaying = false;
        } else {
            SYS_LOGE("%s: invalid case\n", __FUNCTION__);
        }

        //
        source_input_param_t new_source_input_param;
        new_source_input_param = GetCurrentSourceInputInfo();
        if (((new_source_input_param.source_input == SOURCE_DTV) || (new_source_input_param.source_input == SOURCE_MPEG))
            && (framesizeEventFlag == 1)) {
            if (isBootvideoStopped()) {
                new_source_input_param.sig_fmt = getVideoResolutionToFmt();
                SYS_LOGD("%s: sig_fmt = 0x%x(%d)\n", __FUNCTION__, new_source_input_param.sig_fmt, new_source_input_param.sig_fmt);
                SetCurrentSourceInputInfo(new_source_input_param);
            } else {
                SYS_LOGD("%s: bootvideo don't stop\n", __FUNCTION__);
            }
        }

        if (hdrTypeEventFlag == 0x1) {
            //get hdr type
            hdr_type_t newHdrType = HDR_TYPE_NONE;
            newHdrType            = Cpq_GetSourceHDRType(mCurrentSourceInputInfo);

            //notify hdr event to framework
            if (mCurrentHdrType != newHdrType) {
                mCurrentHdrType = newHdrType;
                if (mNotifyListener != NULL) {
                    SYS_LOGD("%s: send hdr event, info is %d\n", __FUNCTION__, mCurrentHdrType);
                    mNotifyListener->onHdrInfoChange(mCurrentHdrType);
                } else {
                    SYS_LOGE("%s: mNotifyListener is NULL\n", __FUNCTION__);
                }
            }
        } else {
            SYS_LOGD("%s: not hdrInfo event\n", __FUNCTION__);
        }
    } else {
        SYS_LOGE("%s: read video event failed\n", __FUNCTION__);
    }
}

tvin_sig_fmt_t CPQControl::getVideoResolutionToFmt()
{
    int ret = -1;
    char buf[32] = {0};
    tvin_sig_fmt_t sig_fmt = TVIN_SIG_FMT_HDMI_1920X1080P_60HZ;

    ret = pqReadSys(VIDEO_FRAME_HEIGHT, buf, sizeof(buf));
    if (ret > 0) {
        int height = atoi(buf);
        if (height <= 480) {
            sig_fmt = TVIN_SIG_FMT_HDMI_720X480P_60HZ;
        } else if (height > 480 && height <= 576) {
            sig_fmt = TVIN_SIG_FMT_HDMI_720X576P_50HZ;
        } else if (height > 576 && height <= 720) {
            sig_fmt = TVIN_SIG_FMT_HDMI_1280X720P_60HZ;
        } else if (height > 720 && height <= 1088) {
            sig_fmt = TVIN_SIG_FMT_HDMI_1920X1080P_60HZ;
        } else {
            sig_fmt = TVIN_SIG_FMT_HDMI_3840_2160_00HZ;
        }
    } else {
        SYS_LOGE("[%s] read error!\n", __FUNCTION__);
    }

    return sig_fmt;
}

void CPQControl::onTXStatusChange()
{
    SYS_LOGI("%s!\n", __FUNCTION__);
    SetCurrentSourceInputInfo(mCurrentSourceInputInfo);
}

int CPQControl::isGameMode() {
    return property_get_int32("vendor.media.omx.gamemode.status", 0);
}

int CPQControl::LoadPQSettings()
{
    int ret = 0;
    const char *config_value;
    config_value = mPQConfigFile->GetString(CFG_SECTION_PQ, CFG_ALL_PQ_MODULE_ENABLE, "enable");
    if (strcmp(config_value, "disable") == 0) {
        SYS_LOGD("All PQ module disabled!\n");
        pq_ctrl_t pqControlVal;
        memset(&pqControlVal, 0, sizeof(pq_ctrl_t));
        vpp_pq_ctrl_t amvecmConfigVal;
        amvecmConfigVal.length = 14;//this is the count of pq_ctrl_s option
        amvecmConfigVal.ptr = (long long)&pqControlVal;
        ret = VPPDeviceIOCtl(AMVECM_IOC_S_PQ_CTRL, &amvecmConfigVal);
        if (ret < 0) {
            SYS_LOGE("%s error: %s!\n", __FUNCTION__, strerror(errno));
        }
    } else {
        SYS_LOGI("source_input: %d, sig_fmt: 0x%x(%d), trans_fmt: 0x%x\n", mCurrentSourceInputInfo.source_input,
                 mCurrentSourceInputInfo.sig_fmt, mCurrentSourceInputInfo.sig_fmt, mCurrentSourceInputInfo.trans_fmt);

        SYS_LOGI("pq_source_input: %d, timming: %d, \n", mCurrentPqSource.pq_source_input,
                 mCurrentPqSource.pq_sig_fmt);

        if (mbCpqCfg_new_picture_mode_enable) {
            ret |= LoadPQUISettings();
            ret |= LoadPQTableSettings();
            return ret;
        }

        ret |= Cpq_SetXVYCCMode(VPP_XVYCC_MODE_STANDARD, mCurrentSourceInputInfo);

        ret |= Cpq_SetDIModuleParam(mCurrentSourceInputInfo);

        int LDLevel = GetLocalDimming();
        ret |= SetLocalDimming(LDLevel, 1);

        di_deblock_mode_t DeblockMode =(di_deblock_mode_t)GetDeblockMode();
        ret |= Cpq_SetDeblockMode(DeblockMode, mCurrentSourceInputInfo);

        di_demosquito_mode_t DemoSquitoMode =(di_demosquito_mode_t)GetDemoSquitoMode();
        ret |= Cpq_SetDemoSquitoMode(DemoSquitoMode, mCurrentSourceInputInfo);

        vpp_mcdi_mode_t mcdimode =(vpp_mcdi_mode_t)GetMcDiMode();
        ret |= Cpq_SetMcDiMode(mcdimode, mCurrentSourceInputInfo);

        vpp_picture_mode_t pqmode = (vpp_picture_mode_t)GetPQMode();
        ret |= Cpq_SetPQMode(pqmode, mCurrentSourceInputInfo, PQ_MODE_SWITCH_TYPE_INIT);

        vpp_color_basemode_t baseMode = GetColorBaseMode();
        ret |= SetColorBaseMode(baseMode, 1);

		Cpq_CheckColorTemperatureParamAlldata(mCurrentSourceInputInfo);
        ret |= Cpq_SetColorTemperatureWithoutSave((vpp_color_temperature_mode_t)GetColorTemperature(), mCurrentSourceInputInfo.source_input);

        int DnlpLevel = GetDnlpMode();
        ret |= SetDnlpMode(DnlpLevel);

        int LocalContrastMode = GetLocalContrastMode();
        ret |= SetLocalContrastMode((local_contrast_mode_t)LocalContrastMode, 1);

        vpp_colorgamut_mode_t mode = (vpp_colorgamut_mode_t)GetColorGamutMode();
        ret |= SetColorGamutMode(mode, 1);

        vpp_display_mode_t display_mode = (vpp_display_mode_t)GetDisplayMode();
        ret |= SetDisplayMode(display_mode, 1);

        //load hdr tmo
        //int HdrTmoMode = GetHDRTMOMode();
        ret |= SetHDRTMOMode(HDR_TMO_DYNAMIC, 1);

        if (isGameMode()) {
            SetMemcMode(VPP_MEMC_MODE_OFF, 0);
            SetAiSrEnable(false);
        } else {
            int MemcMode = GetMemcMode();
            int aisr_enable = GetAiSrEnable();
            int aisr_mode = GetAiSrMode();
            int aipq_mode = GetAipqMode();
            int aicolor = GetAiColor();
            ret |= SetMemcMode(MemcMode, 1);
            ret |= SetAiSrEnable((aisr_enable > 0)? true : false);
            ret |= Cpq_SetAiSrMode((aisr_mode_e)aisr_mode, mCurrentSourceInputInfo);
            ret |= Cpq_SetAipqMode((aipq_mode_e)aipq_mode, mCurrentSourceInputInfo);
            ret |= Cpq_SetAiColor(aicolor);
       }

        vpp_smooth_plus_mode_t smoothplus_mode = (vpp_smooth_plus_mode_t)GetSmoothPlusMode();
        ret |= Cpq_SetSmoothPlusMode(smoothplus_mode, mCurrentSourceInputInfo);
    }
    return ret;
}

int CPQControl::LoadPQUISettings()
{
    int ret = 0;

    //picture mode
    vpp_picture_mode_t pqmode = (vpp_picture_mode_t)GetPQMode();
    ret = Set_PictureMode (pqmode, mCurrentPqSource, PQ_MODE_SWITCH_TYPE_INIT);

    return ret;
}

int CPQControl::LoadPQTableSettings()
{
    int ret = 0;

    ret |= Cpq_SetXVYCCMode(VPP_XVYCC_MODE_STANDARD, mCurrentSourceInputInfo);

    ret |= Cpq_SetDIModuleParam(mCurrentSourceInputInfo);

    int LDLevel = GetLocalDimming();
    ret |= SetLocalDimming(LDLevel, 1);

    vpp_mcdi_mode_t mcdimode =(vpp_mcdi_mode_t)GetMcDiMode();
    ret |= Cpq_SetMcDiMode(mcdimode, mCurrentSourceInputInfo);

    vpp_color_basemode_t baseMode = GetColorBaseMode();
    ret |= SetColorBaseMode(baseMode, 1);

    //display
    vpp_display_mode_t display_mode = (vpp_display_mode_t)GetDisplayMode();
    ret |= SetDisplayMode(display_mode, 1);

    //load hdr tmo
    //int HdrTmoMode = GetHDRTMOMode();
    ret |= SetHDRTMOMode(HDR_TMO_DYNAMIC, 1);

    if (isGameMode()) {
        SetMemcMode(VPP_MEMC_MODE_OFF, 0);
        SetAiSrEnable(false);
    } else {
        int MemcMode = GetMemcMode();
        int aisr_enable = GetAiSrEnable();
        int aisr_mode = GetAiSrMode();
        int aipq_mode = GetAipqMode();
        int aicolor = GetAiColor();
        ret |= SetMemcMode(MemcMode, 1);
        ret |= SetAiSrEnable((aisr_enable > 0)? true : false);
        ret |= Cpq_SetAiSrMode((aisr_mode_e)aisr_mode, mCurrentSourceInputInfo);
        ret |= Cpq_SetAipqMode((aipq_mode_e)aipq_mode, mCurrentSourceInputInfo);
        ret |= Cpq_SetAiColor(aicolor);
    }

    return ret;
}

int CPQControl::Cpq_LoadRegs(am_regs_t regs)
{
    if (regs.length == 0) {
        SYS_LOGE("%s--Regs is NULL!\n", __FUNCTION__);
        return -1;
    }

    int count_retry = 20;
    int ret = 0;
    while (count_retry) {
        ret = VPPDeviceIOCtl(AMVECM_IOC_LOAD_REG, &regs);
        if (ret < 0) {
            SYS_LOGE("%s, error(%s), errno(%d)\n", __FUNCTION__, strerror(errno), errno);
            if (errno == EBUSY) {
                SYS_LOGE("%s, %s, retry...\n", __FUNCTION__, strerror(errno));
                count_retry--;
                continue;
            }
        }
        break;
    }

    return ret;
}

int CPQControl::Cpq_LoadDisplayModeRegs(ve_pq_load_t regs)
{
    if (regs.length == 0) {
        SYS_LOGE("%s--Regs is NULL!\n", __FUNCTION__);
        return -1;
    }

    int count_retry = 20;
    int ret = 0;
    while (count_retry) {
        ret = VPPDeviceIOCtl(AMVECM_IOC_SET_OVERSCAN, &regs);
        if (ret < 0) {
            SYS_LOGE("%s, error(%s), errno(%d)\n", __FUNCTION__, strerror(errno), errno);
            if (errno == EBUSY) {
                SYS_LOGE("%s, %s, retry...\n", __FUNCTION__, strerror(errno));
                count_retry--;
                continue;
            }
        }
        break;
    }

    return ret;
}

int CPQControl::DI_LoadRegs(am_pq_param_t di_regs)
{
    int count_retry = 20;
    int ret = 0;
    while (count_retry) {
        ret = DIDeviceIOCtl(AMDI_IOC_SET_PQ_PARM, &di_regs);
        if (ret < 0) {
            SYS_LOGE("%s, error(%s), errno(%d)\n", __FUNCTION__, strerror(errno), errno);
            if (errno == EBUSY) {
                SYS_LOGE("%s, %s, retry...\n", __FUNCTION__, strerror(errno));
                count_retry--;
                continue;
            }
        }
        break;
    }

    return ret;
}

int CPQControl::LoadCpqLdimRegs()
{
    bool ret = 0;
    int ldFd = -1;

    if (!isFileExist(LDIM_PATH)) {
        SYS_LOGE("Don't have ldim module!\n");
    } else {
        ldFd = open(LDIM_PATH, O_RDWR);

        if (ldFd < 0) {
            SYS_LOGE("Open ldim module, error(%s)!\n", strerror(errno));
            ret = -1;
        } else {
            vpu_ldim_param_s *ldim_param_temp = new vpu_ldim_param_s();

            if (ldim_param_temp) {
                if (!mPQdb->PQ_GetLDIM_Regs(ldim_param_temp) || ioctl(ldFd, LDIM_IOC_PARA, ldim_param_temp) < 0) {
                   SYS_LOGE("LoadCpqLdimRegs, error(%s)!\n", strerror(errno));
                   ret = -1;
                }

                delete ldim_param_temp;
            }
                close (ldFd);
        }
    }

    return ret;
}

int CPQControl::Cpq_LoadBasicRegs(source_input_param_t source_input_param, vpp_picture_mode_t pqMode)
{
    int ret = 0;
    int level = 0;
    if (pqMode != VPP_PICTURE_MODE_MONITOR) {
        if (mbCpqCfg_separate_black_blue_chorma_db_enable) {
            level = GetBlackStretch();
            ret |= Cpq_BlackStretch(level, source_input_param);

            level = GetBlueStretch();
            ret |= Cpq_BlueStretch(level, source_input_param);

            level = GetChromaCoring();
            ret |= Cpq_ChromaCoring(level, source_input_param);
        } else {
            if (mbCpqCfg_blackextension_enable) {
                ret |= SetBlackExtensionParam(source_input_param);
            } else {
                SYS_LOGD("%s: BlackExtension module disabled!\n", __FUNCTION__);
            }
        }

        if (mbCpqCfg_sharpness0_enable) {
            ret |= Cpq_SetSharpness0FixedParam(source_input_param);
            ret |= Cpq_SetSharpness0VariableParam(source_input_param);
        } else {
            SYS_LOGD("%s: Sharpness0 module disabled!\n", __FUNCTION__);
        }

        if (mbCpqCfg_sharpness1_enable) {
            ret |= Cpq_SetSharpness1FixedParam(source_input_param);
            ret |= Cpq_SetSharpness1VariableParam(source_input_param);
        } else {
            SYS_LOGD("%s: Sharpness1 module disabled!\n", __FUNCTION__);
        }

        if (mbCpqCfg_sharpnesspi_enable) {
            ret |= Cpq_SetSharpnessPiFixedParam(source_input_param);
            ret |= Cpq_SetSharpnessPiVariableParam(source_input_param);
        } else {
            SYS_LOGD("%s: Sharpnesspi module disabled!\n", __FUNCTION__);
        }
    }

    if (mbCpqCfg_amvecm_basic_enable || mbCpqCfg_amvecm_basic_withOSD_enable) {
        ret |= Cpq_SetBrightnessBasicParam(source_input_param);
        ret |= Cpq_SetContrastBasicParam(source_input_param);
        ret |= Cpq_SetSaturationBasicParam(source_input_param);
        ret |= Cpq_SetHueBasicParam(source_input_param);
    } else {
        SYS_LOGD("%s: brightness contrast saturation hue module disabled!\n", __FUNCTION__);
    }

    return ret;
}

int CPQControl::SetFacColorParams(source_input_param_t source_input_param, vpp_picture_mode_t pqMode)
{
    int ret = 0;
    int level = 0;

    if (mbCpqCfg_sharpness0_enable) {
        ret |= Cpq_SetSharpness0FixedParam(source_input_param);
        ret |= Cpq_SetSharpness0VariableParam(source_input_param);
    } else {
        SYS_LOGD("%s: Sharpness0 module disabled!\n", __FUNCTION__);
    }

    if (mbCpqCfg_sharpness1_enable) {
        ret |= Cpq_SetSharpness1FixedParam(source_input_param);
        ret |= Cpq_SetSharpness1VariableParam(source_input_param);
    } else {
        SYS_LOGD("%s: Sharpness1 module disabled!\n", __FUNCTION__);
    }

    if (mbCpqCfg_sharpnesspi_enable) {
        ret |= Cpq_SetSharpnessPiFixedParam(source_input_param);
        ret |= Cpq_SetSharpnessPiVariableParam(source_input_param);
    } else {
        SYS_LOGD("%s: Sharpnesspi module disabled!\n", __FUNCTION__);
    }

    if (mbCpqCfg_amvecm_basic_enable || mbCpqCfg_amvecm_basic_withOSD_enable) {
        ret |= Cpq_SetBrightnessBasicParam(source_input_param);
        ret |= Cpq_SetContrastBasicParam(source_input_param);
        ret |= Cpq_SetSaturationBasicParam(source_input_param);
        ret |= Cpq_SetHueBasicParam(source_input_param);
    } else {
        SYS_LOGD("%s: brightness contrast saturation hue module disabled!\n", __FUNCTION__);
    }

    return ret;
}

int CPQControl::BacklightInit(void)
{
    int ret = 0;
    int backlight = 0;
    for (int i = 1; i < 4; i++) {
        backlight = GetBacklight(i);
        SYS_LOGD("%s i = %d, backlight = %d!\n", __FUNCTION__, i, backlight);
        ret = SetBacklight(backlight, i, 1);
        if (ret != 0) {
            SYS_LOGE("%s failed!\n", __FUNCTION__);
            return ret;
        }
    }

    return ret;
}

int CPQControl::Cpq_SetDIModuleParam(source_input_param_t source_input_param)
{
    int ret = -1;
    am_regs_t regs;
    am_pq_param_t di_regs;
    memset(&regs, 0x0, sizeof(am_regs_t));
    memset(&di_regs, 0x0, sizeof(am_pq_param_t));
    if (mbCpqCfg_di_enable) {
        if (mPQdb->PQ_GetDIParams(source_input_param, &regs) == 0) {
            di_regs.table_name |= TABLE_NAME_DI;
        } else {
            SYS_LOGE("%s GetDIParams failed!\n",__FUNCTION__);
        }
    } else {
        SYS_LOGD("DI module disabled!\n");
    }

    if (regs.length != 0) {
        di_regs.table_len = regs.length;
        am_reg_t tmp_buf[regs.length];
        for (unsigned int i=0;i<regs.length;i++) {
              tmp_buf[i].addr = regs.am_reg[i].addr;
              tmp_buf[i].mask = regs.am_reg[i].mask;
              tmp_buf[i].type = regs.am_reg[i].type;
              tmp_buf[i].val  = regs.am_reg[i].val;
        }

        di_regs.table_ptr = (long long)tmp_buf;

        ret = DI_LoadRegs(di_regs);
    } else {
        SYS_LOGE("%s: get DI Module Param failed!\n",__FUNCTION__);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }
    return ret;
}

int CPQControl::SetPQMode(int pq_mode, int is_save , int is_autoswitch)
{
    SYS_LOGI("%s, source: %d, timming: %d, pq_mode: %d\n", __FUNCTION__, mCurrentSourceInputInfo.source_input, mCurrentPqSource.pq_sig_fmt, pq_mode);
    int ret = -1;

    mLastPictureMode = (vpp_picture_mode_t)GetPQMode();
    if (mLastPictureMode == pq_mode) {
        SYS_LOGD("Same PQ mode,no need set again!\n");
        ret = 0;
        return ret;
    }

    if (is_autoswitch < PQ_MODE_SWITCH_TYPE_MANUAL || is_autoswitch >=PQ_MODE_SWITCH_TYPE_MAX) {
        is_autoswitch = PQ_MODE_SWITCH_TYPE_INIT;
    }

    if (is_save == 1) {
        SavePQMode(pq_mode);
    }

    if (mbCpqCfg_new_picture_mode_enable) {
        ret = Set_PictureMode((vpp_picture_mode_t)pq_mode, mCurrentPqSource, (pq_mode_switch_type_t)is_autoswitch);
    } else {
        ret = Cpq_SetPQMode((vpp_picture_mode_t)pq_mode, mCurrentSourceInputInfo, (pq_mode_switch_type_t)is_autoswitch);
    }

    if ((ret == 0) && (is_save == 1)) {
        if ((mCurrentSourceInputInfo.source_input >= SOURCE_HDMI1) &&
            (mCurrentSourceInputInfo.source_input <= SOURCE_HDMI4)) {
            vpp_display_mode_t display_mode = (vpp_display_mode_t)GetDisplayMode();
            ret = SetDisplayMode(display_mode, 1);
        }
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n", __FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n", __FUNCTION__);
    }

    return ret;
}

int CPQControl::GetPQMode(void)
{
    int mode = VPP_PICTURE_MODE_STANDARD;
    int offset = 0;
    if ( mbCpqCfg_new_picture_mode_enable) {
        offset =  mCurrentPqSource.pq_source_input * PQ_FMT_MAX + mCurrentPqSource.pq_sig_fmt;
    } else {
        offset = mSourceInputForSaveParam;
    }
    mSSMAction->SSMReadPictureMode(offset, &mode);

    if (mode < VPP_PICTURE_MODE_STANDARD || mode >= VPP_PICTURE_MODE_MAX) {
        mode = VPP_PICTURE_MODE_STANDARD;
    }

    //SYS_LOGD("%s, source: %d, timming: %d, mode: %d\n", __FUNCTION__, mSourceInputForSaveParam, mCurrentPqSource.pq_sig_fmt, mode);
    return mode;
}

int CPQControl::SavePQMode(int pq_mode)
{
    int ret = -1;
    SYS_LOGD("%s, source: %d, timming: %d, mode: %d\n", __FUNCTION__, mSourceInputForSaveParam, mCurrentPqSource.pq_sig_fmt, pq_mode);
    int offset = 0;
    if ( mbCpqCfg_new_picture_mode_enable) {
        offset =  mCurrentPqSource.pq_source_input * PQ_FMT_MAX + mCurrentPqSource.pq_sig_fmt;
    } else {
        offset = mSourceInputForSaveParam;
    }
    ret = mSSMAction->SSMSavePictureMode(offset, pq_mode);

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }
    return ret;
}

int CPQControl::GetLastPQMode(void)
{
    int mode = VPP_PICTURE_MODE_STANDARD;
    int offset = 0;
    if ( mbCpqCfg_new_picture_mode_enable) {
        offset =  mCurrentPqSource.pq_source_input * PQ_FMT_MAX + mCurrentPqSource.pq_sig_fmt;
    } else {
        offset = mSourceInputForSaveParam;
    }

    mSSMAction->SSMReadLastPictureMode(offset, &mode);
    if (mode < VPP_PICTURE_MODE_STANDARD || mode >= VPP_PICTURE_MODE_MAX) {
        mode = VPP_PICTURE_MODE_STANDARD;
    }

    SYS_LOGD("%s, source: %d, mode: %d\n", __FUNCTION__, mSourceInputForSaveParam, mode);
    return mode;

}

int CPQControl::SaveLastPQMode(int pq_mode)
{
    int ret = -1;
    SYS_LOGD("%s, source: %d, mode: %d\n", __FUNCTION__, mSourceInputForSaveParam, pq_mode);
    int offset = 0;
    if ( mbCpqCfg_new_picture_mode_enable) {
        offset =  mCurrentPqSource.pq_source_input * PQ_FMT_MAX + mCurrentPqSource.pq_sig_fmt;
    } else {
        offset = mSourceInputForSaveParam;
    }

    ret = mSSMAction->SSMSaveLastPictureMode(offset, pq_mode);
    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }
    return ret;

}

#ifdef SUPPORT_TVSERVICE
static Mutex amLock;
static sp<TvServerHidlClient> mTvService = nullptr;
static const sp<TvServerHidlClient> &getTvService()
{
    Mutex::Autolock _l(amLock);
    if (mTvService == nullptr) {
        mTvService = sp<TvServerHidlClient>::make(CONNECT_TYPE_HAL);
    }

    return mTvService;
}
#endif

int CPQControl::setPQModeByTvService(pq_status_update_e gameStatus, pq_status_update_e pcStatus, int autoSwitchMonitorModeFlag)
{
    SYS_LOGD("%s: gameStatus: %d, pcStatus: %d!\n", __FUNCTION__, gameStatus, pcStatus);

    int ret = -1;
#ifdef SUPPORT_TVSERVICE
    const sp<TvServerHidlClient> &TvService = getTvService();
    if ( TvService == NULL) {
        SYS_LOGE("%s: get tvservice failed!\n", __FUNCTION__);
    } else {
        ret = TvService->vdinUpdateForPQ(gameStatus, pcStatus, autoSwitchMonitorModeFlag);
    }
#else
    SYS_LOGD("%s: don't support tvservice!\n", __FUNCTION__);
    ret = 0;
#endif

    if (ret < 0) {
        SYS_LOGE("%s failed!\n", __FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n", __FUNCTION__);
    }
    return ret;
}

int CPQControl::Cpq_SetPQMode(vpp_picture_mode_t pq_mode, source_input_param_t source_input_param,
                                   pq_mode_switch_type_t switch_type)
{
    int ret = -1;
    vpp_pq_para_t pq_para;
    if ((mCurrentSourceInputInfo.source_input == SOURCE_HDMI1) ||
          (mCurrentSourceInputInfo.source_input == SOURCE_HDMI2) ||
          (mCurrentSourceInputInfo.source_input == SOURCE_HDMI3) ||
          (mCurrentSourceInputInfo.source_input == SOURCE_HDMI4)) {//HDMI source;

        if (mLastPictureMode == VPP_PICTURE_MODE_GAME) {
            if (pq_mode == VPP_PICTURE_MODE_GAME) {
                setPQModeByTvService(MODE_ON, MODE_OFF, switch_type);//game mode on and monitor mode off;
            } else if (pq_mode == VPP_PICTURE_MODE_MONITOR) {
                setPQModeByTvService(MODE_OFF, MODE_ON, switch_type);//game mode off and monitor mode on;
            } else {
                setPQModeByTvService(MODE_OFF, MODE_OFF, switch_type);//game mode off and monitor mode off;
            }
        } else if (mLastPictureMode == VPP_PICTURE_MODE_MONITOR) {
            if (pq_mode == VPP_PICTURE_MODE_MONITOR) {
                setPQModeByTvService(MODE_OFF, MODE_ON, switch_type);//game mode off and monitor mode on;
            } else if (pq_mode == VPP_PICTURE_MODE_GAME) {
                setPQModeByTvService(MODE_ON, MODE_OFF, switch_type);//game mode on and monitor mode off;
            } else {
                setPQModeByTvService(MODE_OFF, MODE_OFF, switch_type);//game mode off and monitor mode off;
            }
        } else {
            if (pq_mode == VPP_PICTURE_MODE_GAME) {
                ret = setPQModeByTvService(MODE_ON, MODE_OFF, switch_type);//game mode on and monitor mode off;
            } else if (pq_mode == VPP_PICTURE_MODE_MONITOR) {
                ret = setPQModeByTvService(MODE_OFF, MODE_ON, switch_type);//game mode off and monitor mode on;
            } else {
                setPQModeByTvService(MODE_OFF, MODE_OFF, PQ_MODE_SWITCH_TYPE_INIT);//game mode off and monitor mode off;
            }
        }
    } else {//other source;
        if (mInitialized) {
            setPQModeByTvService(MODE_OFF, MODE_OFF, PQ_MODE_SWITCH_TYPE_INIT);
        }
    }

    vpp_noise_reduction_mode_t nrMode = VPP_NOISE_REDUCTION_MODE_OFF;
    if (mbCpqCfg_nr_enable) {
        if ((pq_mode == VPP_PICTURE_MODE_MONITOR) || (pq_mode == VPP_PICTURE_MODE_GAME)) {
            nrMode = VPP_NOISE_REDUCTION_MODE_OFF;
        } else {
            nrMode = (vpp_noise_reduction_mode_t)GetNoiseReductionMode();
        }
        Cpq_SetNoiseReductionMode(nrMode, source_input_param);
    } else {
        SYS_LOGD("%s: nr2 module disabled!\n", __FUNCTION__);
    }

    Cpq_LoadBasicRegs(source_input_param, pq_mode);
    ret = GetPQParams(source_input_param, pq_mode, &pq_para);
    if (ret < 0) {
        SYS_LOGE("%s: Get PQ Params failed!\n", __FUNCTION__);
    } else {
        ret = SetPQParams(source_input_param, pq_mode, pq_para);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::SetPQParams(source_input_param_t source_input_param, vpp_picture_mode_t pq_mode, vpp_pq_para_t pq_para)
{
    int ret = 0;

    if (mbCpqCfg_amvecm_basic_enable || mbCpqCfg_amvecm_basic_withOSD_enable) {
        int hue_level = 0, hue = 50, saturation = 50;
        if (((source_input_param.source_input == SOURCE_TV) ||
              (source_input_param.source_input == SOURCE_AV1) ||
              (source_input_param.source_input == SOURCE_AV2)) &&
            ((source_input_param.sig_fmt == TVIN_SIG_FMT_CVBS_NTSC_M) ||
             (source_input_param.sig_fmt == TVIN_SIG_FMT_CVBS_NTSC_443))) {
            hue_level = 100 - pq_para.hue;
        } else {
            hue_level = 50;
        }

        ret = mPQdb->PQ_GetHueParams(source_input_param, hue_level, &hue);
        if (ret == 0) {
            ret = mPQdb->PQ_GetSaturationParams(source_input_param, pq_para.saturation, &saturation);
            if (ret == 0) {
                ret = Cpq_SetVideoSaturationHue(saturation, hue);
            } else {
                SYS_LOGE("%s: PQ_GetSaturationParams failed!\n", __FUNCTION__);
            }
        } else {
            SYS_LOGE("%s: PQ_GetHueParams failed!\n", __FUNCTION__);
        }
    }

    if (pq_mode != VPP_PICTURE_MODE_MONITOR) {
        ret |= Cpq_SetSharpness(pq_para.sharpness, source_input_param);
    }
    ret |= Cpq_SetBrightness(pq_para.brightness, source_input_param);
    ret |= Cpq_SetContrast(pq_para.contrast, source_input_param);

    return ret;
}

int CPQControl::GetPQParams(source_input_param_t source_input_param, vpp_picture_mode_t pq_mode, vpp_pq_para_t *pq_para)
{
    int ret = -1;
    if (pq_para == NULL) {
        SYS_LOGD("%s: pq_para is NULL!\n", __FUNCTION__);
        return ret;
    }

    mSSMAction->SSMReadBrightness(mSourceInputForSaveParam, &pq_para->brightness);
    mSSMAction->SSMReadContrast(mSourceInputForSaveParam, &pq_para->contrast);
    mSSMAction->SSMReadSaturation(mSourceInputForSaveParam, &pq_para->saturation);
    mSSMAction->SSMReadHue(mSourceInputForSaveParam, &pq_para->hue);
    mSSMAction->SSMReadSharpness(mSourceInputForSaveParam, &pq_para->sharpness);
    ret = 0;

    return ret;
}

//color temperature
int CPQControl::SetColorTemperature(int temp_mode, int is_save)
{
    int ret = -1;
    SYS_LOGI("%s: source:%d, mode: %d\n", __FUNCTION__, mCurrentSourceInputInfo.source_input, temp_mode);

	if (is_save == 1) {
        SaveColorTemperature(temp_mode);
    }

    ret = Cpq_SetColorTemperatureWithoutSave((vpp_color_temperature_mode_t)temp_mode, mCurrentSourceInputInfo.source_input);

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::GetColorTemperature(void)
{
    int mode = VPP_COLOR_TEMPERATURE_MODE_STANDARD;
    if (mbCpqCfg_new_picture_mode_enable) {
        vpp_picture_mode_t pq_mode = (vpp_picture_mode_t)GetPQMode();
        vpp_pictur_mode_para_t para;
        if (GetPictureModeData(mCurrentPqSource, pq_mode, &para) == 0) {
            mode = para.ColorTemperature;
        }
    } else {
        mSSMAction->SSMReadColorTemperature(mSourceInputForSaveParam, &mode);
    }

    if (mode < VPP_COLOR_TEMPERATURE_MODE_STANDARD || mode > VPP_COLOR_TEMPERATURE_MODE_USER) {
        mode = VPP_COLOR_TEMPERATURE_MODE_STANDARD;
    }

    SYS_LOGD("%s: source: %d, timming: %d mode: %d!\n",__FUNCTION__, mSourceInputForSaveParam, mCurrentPqSource.pq_sig_fmt, mode);
    return mode;
}

int CPQControl::SaveColorTemperature(int temp_mode)
{

    int ret = -1;
    SYS_LOGD("%s, source: %d,timming: %d, mode = %d\n", __FUNCTION__, mSourceInputForSaveParam, mCurrentPqSource.pq_sig_fmt, temp_mode);
    if (mbCpqCfg_new_picture_mode_enable) {
        vpp_pictur_mode_para_t para;
        vpp_picture_mode_t pq_mode = (vpp_picture_mode_t)GetPQMode();
        if (GetPictureModeData(mCurrentPqSource, pq_mode, &para) == 0) {
            para.ColorTemperature = temp_mode;
            ret = SetPictureModeData(mCurrentPqSource, pq_mode, &para);
        }
    } else {
        ret = mSSMAction->SSMSaveColorTemperature(mSourceInputForSaveParam, temp_mode);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::SetColorTemperatureUserParam(int temp_mode, int is_save, rgb_ogo_type_t rgb_ogo_type, int value)
{
    SYS_LOGD("%s: temp_mode:%d, rgb_ogo_type:%d  value %d, is_save: %d\n", __FUNCTION__, temp_mode, rgb_ogo_type, value, is_save);

    if (is_save == 1) {
        Cpq_SaveColorTemperatureUser((vpp_color_temperature_mode_t)temp_mode, rgb_ogo_type, value);
    }

    if (CPQ_SetColorTemperatureUserParam((vpp_color_temperature_mode_t)temp_mode, rgb_ogo_type, value) < 0) {
        SYS_LOGE("%s CPQ_SetColorTemperatureUserParam fail !\n",__FUNCTION__);
        return -1;
    }

    return 0;
}

int CPQControl::CPQ_SetColorTemperatureUserParam(vpp_color_temperature_mode_t temp_mode, rgb_ogo_type_t rgb_ogo_type, int value)
{
    //RGB GAIN OFFSET
    tcon_rgb_ogo_t rgbogo;
    memset(&rgbogo, 0, sizeof(tcon_rgb_ogo_t));
    if (GetColorTemperatureParams(temp_mode, &rgbogo) < 0) {
        SYS_LOGE("%s: GetColorTemperatureParams fail\n", __FUNCTION__);
        return -1;
    }
    rgbogo.en = 1;
    rgbogo.r_pre_offset = 0;
    rgbogo.g_pre_offset = 0;
    rgbogo.b_pre_offset = 0;

    //RGB UI GAIN OFFSET
    RGB_UI_OFFSET params;
    memset(&params, 0, sizeof(RGB_UI_OFFSET));
    if (Cpq_GetColorTemperatureUser(temp_mode, &params) == 0) {
        rgbogo.r_gain += params.r_gain_value;
        rgbogo.g_gain += params.g_gain_value;
        rgbogo.b_gain += params.b_gain_value;
        rgbogo.r_post_offset += params.r_offset_value;
        rgbogo.g_post_offset += params.g_offset_value;
        rgbogo.b_post_offset += params.b_offset_value;
    }

    if (GetEyeProtectionMode(mCurrentSourceInputInfo.source_input))//if eye protection mode is enable, b_gain / 2.
        rgbogo.b_gain /= 2;

    SYS_LOGD("%s: rgain:%d ggain:%d bgain:%d roffset:%d goffset:%d boffset:%d\n", __FUNCTION__, 
                                                                                rgbogo.r_gain, 
                                                                                rgbogo.g_gain, 
                                                                                rgbogo.b_gain, 
                                                                                rgbogo.r_post_offset,
                                                                                rgbogo.g_post_offset,
                                                                                rgbogo.b_post_offset);

    return Cpq_SetRGBOGO(&rgbogo);
}

tcon_rgb_ogo_t CPQControl::GetColorTemperatureUserParam(void)
{
    RGB_UI_OFFSET data;
    memset(&data, 0, sizeof(RGB_UI_OFFSET));

    if (Cpq_GetColorTemperatureUser((vpp_color_temperature_mode_t)GetColorTemperature(), &data) < 0) {
        SYS_LOGE("%s get colortemp user param failed!\n", __FUNCTION__);
    }

    tcon_rgb_ogo_t param;
    memset(&param, 0, sizeof(tcon_rgb_ogo_t));

    param.en = 1;
    param.r_pre_offset = 0;
    param.g_pre_offset = 0;
    param.b_pre_offset = 0;
    param.r_gain = data.r_gain_value;
    param.g_gain = data.g_gain_value;
    param.b_gain = data.b_gain_value;
    param.r_post_offset = data.r_offset_value;
    param.g_post_offset = data.g_offset_value;
    param.b_post_offset = data.b_offset_value;

    return param;
}

int CPQControl::Cpq_SetColorTemperatureWithoutSave(vpp_color_temperature_mode_t Tempmode, tv_source_input_t tv_source_input __unused)
{
    SYS_LOGD("%s: Tempmode = %d\n", __FUNCTION__, Tempmode);

    if (!mbCpqCfg_whitebalance_enable) {
        SYS_LOGD("whitebalance module disabled!\n");
        return 0;
    }

    if (mInitialized) {//don't load gamma in device turn on
        if (Cpq_LoadGamma((vpp_gamma_curve_t)GetGammaValue(), Tempmode) < 0) {
            SYS_LOGE("%s: Cpq_LoadGamma fail\n", __FUNCTION__);
        }
    }

    //RGB GAIN OFFSET
    tcon_rgb_ogo_t rgbogo;
    memset(&rgbogo, 0, sizeof(tcon_rgb_ogo_t));

    if (GetColorTemperatureParams(Tempmode, &rgbogo) < 0) {
        SYS_LOGE("%s: GetColorTemperatureParams fail\n", __FUNCTION__);
        return -1;
    }
    rgbogo.en = 1;
    rgbogo.r_pre_offset = 0;
    rgbogo.g_pre_offset = 0;
    rgbogo.b_pre_offset = 0;

    //RGB UI GAIN OFFSET
    RGB_UI_OFFSET params;
    memset(&params, 0, sizeof(RGB_UI_OFFSET));
    if (Cpq_GetColorTemperatureUser(Tempmode, &params) == 0) {
        rgbogo.r_gain += params.r_gain_value;
        rgbogo.g_gain += params.g_gain_value;
        rgbogo.b_gain += params.b_gain_value;
        rgbogo.r_post_offset += params.r_offset_value;
        rgbogo.g_post_offset += params.g_offset_value;
        rgbogo.b_post_offset += params.b_offset_value;
    }

    if (GetEyeProtectionMode(mCurrentSourceInputInfo.source_input))//if eye protection mode is enable, b_gain / 2.
        rgbogo.b_gain /= 2;

    SYS_LOGD("%s: rgain:%d ggain:%d bgain:%d roffset:%d goffset:%d boffset:%d\n", __FUNCTION__,rgbogo.r_gain, rgbogo.g_gain, rgbogo.b_gain, rgbogo.r_post_offset, rgbogo.g_post_offset, rgbogo.b_post_offset);
    return Cpq_SetRGBOGO(&rgbogo);
}

int CPQControl::Cpq_CheckColorTemperatureParamAlldata(source_input_param_t source_input_param)
{
    int ret= -1;
    unsigned short ret1 = 0, ret2 = 0;

    if (!mbCpqCfg_whitebalance_enable) {
        SYS_LOGD("%s, whitebalance module disabled! no need check data!\n",__FUNCTION__);
        return 0;
    }

    ret = Cpq_CheckTemperatureDataLabel();
    ret1 = Cpq_CalColorTemperatureParamsChecksum();
    ret2 = Cpq_GetColorTemperatureParamsChecksum();

    if (ret && (ret1 == ret2)) {
        SYS_LOGD("%s, color temperature param label & checksum ok.\n",__FUNCTION__);
        if (Cpq_CheckColorTemperatureParams() == 0) {
            SYS_LOGD("%s, color temperature params check failed.\n", __FUNCTION__);
            Cpq_RestoreColorTemperatureParamsFromDB(source_input_param);
         }
    } else {
        SYS_LOGD("%s, color temperature param data error.\n", __FUNCTION__);
        Cpq_SetTemperatureDataLabel();
        Cpq_RestoreColorTemperatureParamsFromDB(source_input_param);
    }

    return 0;
}

unsigned short CPQControl::Cpq_CalColorTemperatureParamsChecksum(void)
{
    unsigned char data_buf[SSM_CR_RGBOGO_LEN];
    unsigned short sum = 0;
    int cnt;

    mSSMAction->SSMReadRGBOGOValue(0, SSM_CR_RGBOGO_LEN, data_buf);

    for (cnt = 0; cnt < SSM_CR_RGBOGO_LEN; cnt++) {
        sum += data_buf[cnt];
    }

    SYS_LOGD("%s, sum = 0x%X.\n", __FUNCTION__, sum);

    return sum;
}

int CPQControl::Cpq_SetColorTemperatureParamsChecksum(void)
{
    int ret = 0;
    USUC usuc;

    usuc.s = Cpq_CalColorTemperatureParamsChecksum();

    SYS_LOGD("%s, sum = 0x%X.\n", __FUNCTION__, usuc.s);

    ret |= mSSMAction->SSMSaveRGBOGOValue(SSM_CR_RGBOGO_LEN, SSM_CR_RGBOGO_CHKSUM_LEN, usuc.c);

    return ret;
}

unsigned short CPQControl::Cpq_GetColorTemperatureParamsChecksum(void)
{
    USUC usuc;

    mSSMAction->SSMReadRGBOGOValue(SSM_CR_RGBOGO_LEN, SSM_CR_RGBOGO_CHKSUM_LEN, usuc.c);

    SYS_LOGD("%s, sum = 0x%X.\n", __FUNCTION__, usuc.s);

    return usuc.s;
}

int CPQControl::Cpq_SetColorTemperatureUser(tv_source_input_t source_input, tcon_rgb_ogo_t *pData)
{
    if (!mbCpqCfg_whitebalance_enable) {
        SYS_LOGD("whitebalance module disabled!\n");
        return 0;
    }

    if (GetEyeProtectionMode(source_input) == 1) {
        SYS_LOGD("eye protection mode is enable!\n");
        pData->b_gain /= 2;
    }

    if (Cpq_SetRGBOGO(pData) < 0) {
        SYS_LOGE("%s: Cpq_LoadGamma fail\n", __FUNCTION__);
        return -1;
    }

    SYS_LOGD("%s success!\n",__FUNCTION__);

    return 0;
}

int CPQControl::Cpq_GetColorTemperatureUser(vpp_color_temperature_mode_t mode, RGB_UI_OFFSET* pData)
{
    int ret = 0;
    int offset = mode * sizeof(int);
    ret |= mSSMAction->SSMReadRGBGainRStart(offset, &pData->r_gain_value);
    ret |= mSSMAction->SSMReadRGBGainGStart(offset, &pData->g_gain_value);
    ret |= mSSMAction->SSMReadRGBGainBStart(offset, &pData->b_gain_value);
    ret |= mSSMAction->SSMReadRGBPostOffsetRStart(offset, &pData->r_offset_value);
    ret |= mSSMAction->SSMReadRGBPostOffsetGStart(offset, &pData->g_offset_value);
    ret |= mSSMAction->SSMReadRGBPostOffsetBStart(offset, &pData->b_offset_value);

    SYS_LOGD("%s mode: %d, offset: %d RG:%d, GG:%d, BG:%d, RO:%d, GO:%d, BO:%d\n", __FUNCTION__, mode, offset,
                                                                           pData->r_gain_value,
                                                                           pData->g_gain_value,
                                                                           pData->b_gain_value,
                                                                           pData->r_offset_value,
                                                                           pData->g_offset_value,
                                                                           pData->b_offset_value);

    if (ret < 0) {
        SYS_LOGE("%s failed!\n", __FUNCTION__);
        ret = -1;
    }

    return ret;
}

int CPQControl::Cpq_SaveColorTemperatureUser(vpp_color_temperature_mode_t mode, rgb_ogo_type_t rgb_ogo_type, int value)
{
    int ret = 0;
    int offset = mode * sizeof(int);
    switch (rgb_ogo_type)
    {
        case R_GAIN:
            ret |= mSSMAction->SSMSaveRGBGainRStart(offset, value);
        break;
        case G_GAIN:
            ret |= mSSMAction->SSMSaveRGBGainGStart(offset, value);
        break;
        case B_GAIN:
            ret |= mSSMAction->SSMSaveRGBGainBStart(offset, value);
        break;
        case R_POST_OFFSET:
            ret |= mSSMAction->SSMSaveRGBPostOffsetRStart(offset, value);
        break;
        case G_POST_OFFSET:
            ret |= mSSMAction->SSMSaveRGBPostOffsetGStart(offset, value);
        break;
        case B_POST_OFFSET:
            ret |= mSSMAction->SSMSaveRGBPostOffsetBStart(offset, value);
        break;
        default:
            ret = -1;
        break;
    }

    if (ret < 0) {
        SYS_LOGE("%s rgb_ogo_type[%d]:[%d] failed!\n", __FUNCTION__, rgb_ogo_type, value);
    } else {
        SYS_LOGD("%s rgb_ogo_type[%d]:[%d] success!\n", __FUNCTION__, rgb_ogo_type, value);
    }

    return ret;
}


int CPQControl::Cpq_RestoreColorTemperatureParamsFromDB(source_input_param_t source_input_param)
{
    int i = 0;
    tcon_rgb_ogo_t rgbogo;

    for (i = VPP_COLOR_TEMPERATURE_MODE_STANDARD; i < VPP_COLOR_TEMPERATURE_MODE_MAX; i++) {
        mPQdb->PQ_GetColorTemperatureParams((vpp_color_temperature_mode_t) i, source_input_param, &rgbogo);
        SaveColorTemperatureParams((vpp_color_temperature_mode_t) i, rgbogo);
    }

    Cpq_SetColorTemperatureParamsChecksum();

    return 0;
}

int CPQControl::Cpq_CheckTemperatureDataLabel(void)
{
    USUC usuc;
    USUC ret;

    mSSMAction->SSMReadRGBOGOValue(SSM_CR_RGBOGO_LEN - 2, 2, ret.c);

    usuc.c[0] = 0x55;
    usuc.c[1] = 0xAA;

    if ((usuc.c[0] == ret.c[0]) && (usuc.c[1] == ret.c[1])) {
        SYS_LOGD("%s, label ok.\n", __FUNCTION__);
        return 1;
    } else {
        SYS_LOGE("%s, label error.\n", __FUNCTION__);
        return 0;
    }
}

int CPQControl::Cpq_SetTemperatureDataLabel(void)
{
    USUC usuc;
    int ret = 0;

    usuc.c[0] = 0x55;
    usuc.c[1] = 0xAA;

    ret = mSSMAction->SSMSaveRGBOGOValue(SSM_CR_RGBOGO_LEN - 2, 2, usuc.c);

    return ret;
}

int CPQControl::SetColorTemperatureParams(vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t params)
{
    SaveColorTemperatureParams(Tempmode, params);
    Cpq_SetColorTemperatureParamsChecksum();

    return 0;
}

int CPQControl::GetColorTemperatureParams(vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t *params)
{
    SUC suc;
    USUC usuc;
    int ret = 0;
    if (VPP_COLOR_TEMPERATURE_MODE_STANDARD == Tempmode) { //standard
        ret |= mSSMAction->SSMReadRGBOGOValue(0, 2, usuc.c);
        params->en = usuc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(2, 2, suc.c);
        params->r_pre_offset = suc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(4, 2, suc.c);
        params->g_pre_offset = suc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(6, 2, suc.c);
        params->b_pre_offset = suc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(8, 2, usuc.c);
        params->r_gain = usuc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(10, 2, usuc.c);
        params->g_gain = usuc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(12, 2, usuc.c);
        params->b_gain = usuc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(14, 2, suc.c);
        params->r_post_offset = suc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(16, 2, suc.c);
        params->g_post_offset = suc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(18, 2, suc.c);
        params->b_post_offset = suc.s;
    } else if (VPP_COLOR_TEMPERATURE_MODE_WARM == Tempmode) { //warm
        ret |= mSSMAction->SSMReadRGBOGOValue(20, 2, usuc.c);
        params->en = usuc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(22, 2, suc.c);
        params->r_pre_offset = suc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(24, 2, suc.c);
        params->g_pre_offset = suc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(26, 2, suc.c);
        params->b_pre_offset = suc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(28, 2, usuc.c);
        params->r_gain = usuc.s;
        ret |= mSSMAction->SSMReadRGBOGOValue(30, 2, usuc.c);
        params->g_gain = usuc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(32, 2, usuc.c);
        params->b_gain = usuc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(34, 2, suc.c);
        params->r_post_offset = suc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(36, 2, suc.c);
        params->g_post_offset = suc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(38, 2, suc.c);
        params->b_post_offset = suc.s;
    } else if (VPP_COLOR_TEMPERATURE_MODE_COLD == Tempmode) { //cool
        ret |= mSSMAction->SSMReadRGBOGOValue(40, 2, usuc.c);
        params->en = usuc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(42, 2, suc.c);
        params->r_pre_offset = suc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(44, 2, suc.c);
        params->g_pre_offset = suc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(46, 2, suc.c);
        params->b_pre_offset = suc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(48, 2, usuc.c);
        params->r_gain = usuc.s;
        ret |= mSSMAction->SSMReadRGBOGOValue(50, 2, usuc.c);
        params->g_gain = usuc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(52, 2, usuc.c);
        params->b_gain = usuc.s;
        ret |= mSSMAction->SSMReadRGBOGOValue(54, 2, suc.c);
        params->r_post_offset = suc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(56, 2, suc.c);
        params->g_post_offset = suc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(58, 2, suc.c);
        params->b_post_offset = suc.s;
    }else if (VPP_COLOR_TEMPERATURE_MODE_USER == Tempmode) {
        ret |= mSSMAction->SSMReadRGBOGOValue(60, 2, usuc.c);
        params->en = usuc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(62, 2, suc.c);
        params->r_pre_offset = suc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(64, 2, suc.c);
        params->g_pre_offset = suc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(66, 2, suc.c);
        params->b_pre_offset = suc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(68, 2, usuc.c);
        params->r_gain = usuc.s;
        ret |= mSSMAction->SSMReadRGBOGOValue(70, 2, usuc.c);
        params->g_gain = usuc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(72, 2, usuc.c);
        params->b_gain = usuc.s;
        ret |= mSSMAction->SSMReadRGBOGOValue(74, 2, suc.c);
        params->r_post_offset = suc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(76, 2, suc.c);
        params->g_post_offset = suc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(78, 2, suc.c);
        params->b_post_offset = suc.s;
    }

    SYS_LOGD("%s, Tempmode:%d rgain[%d], ggain[%d],bgain[%d],roffset[%d],goffset[%d],boffset[%d]\n", __FUNCTION__, Tempmode,
         params->r_gain, params->g_gain, params->b_gain, params->r_post_offset,
         params->g_post_offset, params->b_post_offset);

    return ret;
}

int CPQControl::SaveColorTemperatureParams(vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t params)
{
    SUC suc;
    USUC usuc;
    int ret = 0;

    if (VPP_COLOR_TEMPERATURE_MODE_STANDARD == Tempmode) { //standard
        usuc.s = params.en;
        ret |= mSSMAction->SSMSaveRGBOGOValue(0, 2, usuc.c);

        suc.s = params.r_pre_offset;
        ret |= mSSMAction->SSMSaveRGBOGOValue(2, 2, suc.c);

        suc.s = params.g_pre_offset;
        ret |= mSSMAction->SSMSaveRGBOGOValue(4, 2, suc.c);

        suc.s = params.b_pre_offset;
        ret |= mSSMAction->SSMSaveRGBOGOValue(6, 2, suc.c);

        usuc.s = params.r_gain;
        ret |= mSSMAction->SSMSaveRGBOGOValue(8, 2, usuc.c);

        usuc.s = params.g_gain;
        ret |= mSSMAction->SSMSaveRGBOGOValue(10, 2, usuc.c);

        usuc.s = params.b_gain;
        ret |= mSSMAction->SSMSaveRGBOGOValue(12, 2, usuc.c);

        suc.s = params.r_post_offset;
        ret |= mSSMAction->SSMSaveRGBOGOValue(14, 2, suc.c);

        suc.s = params.g_post_offset;
        ret |= mSSMAction->SSMSaveRGBOGOValue(16, 2, suc.c);

        suc.s = params.b_post_offset;
        ret |= mSSMAction->SSMSaveRGBOGOValue(18, 2, suc.c);
    } else if (VPP_COLOR_TEMPERATURE_MODE_WARM == Tempmode) { //warm
        usuc.s = params.en;
        ret |= mSSMAction->SSMSaveRGBOGOValue(20, 2, usuc.c);

        suc.s = params.r_pre_offset;
        ret |= mSSMAction->SSMSaveRGBOGOValue(22, 2, suc.c);

        suc.s = params.g_pre_offset;
        ret |= mSSMAction->SSMSaveRGBOGOValue(24, 2, suc.c);
        suc.s = params.b_pre_offset;
        ret |= mSSMAction->SSMSaveRGBOGOValue(26, 2, suc.c);

        usuc.s = params.r_gain;
        ret |= mSSMAction->SSMSaveRGBOGOValue(28, 2, usuc.c);

        usuc.s = params.g_gain;
        ret |= mSSMAction->SSMSaveRGBOGOValue(30, 2, usuc.c);

        usuc.s = params.b_gain;
        ret |= mSSMAction->SSMSaveRGBOGOValue(32, 2, usuc.c);

        suc.s = params.r_post_offset;
        ret |= mSSMAction->SSMSaveRGBOGOValue(34, 2, suc.c);

        suc.s = params.g_post_offset;
        ret |= mSSMAction->SSMSaveRGBOGOValue(36, 2, suc.c);

        suc.s = params.b_post_offset;
        ret |= mSSMAction->SSMSaveRGBOGOValue(38, 2, suc.c);
    } else if (VPP_COLOR_TEMPERATURE_MODE_COLD == Tempmode) { //cool
        usuc.s = params.en;
        ret |= mSSMAction->SSMSaveRGBOGOValue(40, 2, usuc.c);

        suc.s = params.r_pre_offset;
        ret |= mSSMAction->SSMSaveRGBOGOValue(42, 2, suc.c);

        suc.s = params.g_pre_offset;
        ret |= mSSMAction->SSMSaveRGBOGOValue(44, 2, suc.c);

        suc.s = params.b_pre_offset;
        ret |= mSSMAction->SSMSaveRGBOGOValue(46, 2, suc.c);

        usuc.s = params.r_gain;
        ret |= mSSMAction->SSMSaveRGBOGOValue(48, 2, usuc.c);

        usuc.s = params.g_gain;
        ret |= mSSMAction->SSMSaveRGBOGOValue(50, 2, usuc.c);

        usuc.s = params.b_gain;
        ret |= mSSMAction->SSMSaveRGBOGOValue(52, 2, usuc.c);

        suc.s = params.r_post_offset;
        ret |= mSSMAction->SSMSaveRGBOGOValue(54, 2, suc.c);

        suc.s = params.g_post_offset;
        ret |= mSSMAction->SSMSaveRGBOGOValue(56, 2, suc.c);

        suc.s = params.b_post_offset;
        ret |= mSSMAction->SSMSaveRGBOGOValue(58, 2, suc.c);
    }else if (VPP_COLOR_TEMPERATURE_MODE_USER == Tempmode) {
        usuc.s = params.en;
        ret |= mSSMAction->SSMSaveRGBOGOValue(60, 2, usuc.c);

        suc.s = params.r_pre_offset;
        ret |= mSSMAction->SSMSaveRGBOGOValue(62, 2, suc.c);

        suc.s = params.g_pre_offset;
        ret |= mSSMAction->SSMSaveRGBOGOValue(64, 2, suc.c);

        suc.s = params.b_pre_offset;
        ret |= mSSMAction->SSMSaveRGBOGOValue(66, 2, suc.c);

        usuc.s = params.r_gain;
        ret |= mSSMAction->SSMSaveRGBOGOValue(68, 2, usuc.c);

        usuc.s = params.g_gain;
        ret |= mSSMAction->SSMSaveRGBOGOValue(70, 2, usuc.c);

        usuc.s = params.b_gain;
        ret |= mSSMAction->SSMSaveRGBOGOValue(72, 2, usuc.c);

        suc.s = params.r_post_offset;
        ret |= mSSMAction->SSMSaveRGBOGOValue(74, 2, suc.c);

        suc.s = params.g_post_offset;
        ret |= mSSMAction->SSMSaveRGBOGOValue(76, 2, suc.c);

        suc.s = params.b_post_offset;
        ret |= mSSMAction->SSMSaveRGBOGOValue(78, 2, suc.c);
    }

    SYS_LOGD("%s, Tempmode:%d rgain[%d], ggain[%d],bgain[%d],roffset[%d],goffset[%d],boffset[%d]\n", __FUNCTION__, Tempmode,
         params.r_gain, params.g_gain, params.b_gain, params.r_post_offset,
         params.g_post_offset, params.b_post_offset);
    return ret;
}

int CPQControl::Cpq_CheckColorTemperatureParams(void)
{
    int i = 0;
    tcon_rgb_ogo_t rgbogo;
    memset (&rgbogo, 0, sizeof (rgbogo));
    for (i = VPP_COLOR_TEMPERATURE_MODE_STANDARD; i < VPP_COLOR_TEMPERATURE_MODE_MAX; i++) {
        GetColorTemperatureParams((vpp_color_temperature_mode_t) i, &rgbogo);

        if (rgbogo.r_gain > 2047 || rgbogo.b_gain > 2047 || rgbogo.g_gain > 2047) {
            if (rgbogo.r_post_offset > 1023 || rgbogo.g_post_offset > 1023 || rgbogo.b_post_offset > 1023 ||
                rgbogo.r_post_offset < -1024 || rgbogo.g_post_offset < -1024 || rgbogo.b_post_offset < -1024) {
                return 0;
            }
        }
    }

    return 1;
}

//Brightness
int CPQControl::SetBrightness(int value, int is_save)
{
    int ret = 0;
    SYS_LOGI("%s, source: %d, value = %d\n", __FUNCTION__, mSourceInputForSaveParam, value);

    ret = Cpq_SetBrightness(value, mCurrentSourceInputInfo);

    if ((ret == 0) && (is_save == 1)) {
        ret = SaveBrightness(value);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }
    return 0;
}

int CPQControl::GetBrightness(void)
{
    int data = 50;
    vpp_picture_mode_t pq_mode = (vpp_picture_mode_t)GetPQMode();
    vpp_pq_para_t pq_para;

    if (mbCpqCfg_new_picture_mode_enable) {
        vpp_pictur_mode_para_t para;
        if (GetPictureModeData(mCurrentPqSource, pq_mode, &para) == 0) {
            data = para.Brightness;
        }
    } else {
        if (GetPQParams(mCurrentSourceInputInfo, pq_mode, &pq_para) == 0) {
            data = pq_para.brightness;
        }
    }
    if (data < 0 || data > 100) {
        data = 50;
    }

    SYS_LOGI("%s, source: %d, timming: %d, value = %d\n", __FUNCTION__, mSourceInputForSaveParam, mCurrentPqSource.pq_sig_fmt, data);
    return data;
}

int CPQControl::SaveBrightness(int value)
{
    SYS_LOGD("%s, source: %d, timming: %d value = %d\n", __FUNCTION__, mSourceInputForSaveParam, mCurrentPqSource.pq_sig_fmt, value);
    int ret = -1;
    if (mbCpqCfg_new_picture_mode_enable) {
        vpp_pictur_mode_para_t para;
        vpp_picture_mode_t pq_mode = (vpp_picture_mode_t)GetPQMode();
        if (GetPictureModeData(mCurrentPqSource, pq_mode, &para) == 0) {
            para.Brightness = value;
            ret = SetPictureModeData(mCurrentPqSource, pq_mode, &para);
        }
    } else {
        ret = mSSMAction->SSMSaveBrightness(mSourceInputForSaveParam, value);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::Cpq_SetBrightnessBasicParam(source_input_param_t source_input_param)
{
    int ret = -1;
    ret = mPQdb->LoadVppBasicParam(TVPQ_DATA_BRIGHTNESS, source_input_param);
    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::Cpq_SetBrightness(int value, source_input_param_t source_input_param)
{
    int ret = -1;
    int params;
    int level;
    if (mbCpqCfg_amvecm_basic_enable || mbCpqCfg_amvecm_basic_withOSD_enable) {
        if (value >= 0 && value <= 100) {
            level = value;
            if (mPQdb->PQ_GetBrightnessParams(source_input_param, level, &params) == 0) {
                if (Cpq_SetVideoBrightness(params) == 0) {
                    return 0;
                }
            } else {
                SYS_LOGE("Vpp_SetBrightness, PQ_GetBrightnessParams failed!\n");
            }
        }
    } else {
        SYS_LOGD("%s: brightness contrast saturation hue module disabled!\n", __FUNCTION__);
        ret = 0;
    }
    return ret;
}

int CPQControl::Cpq_SetVideoBrightness(int value)
{
    SYS_LOGD("Cpq_SetVideoBrightness brightness : %d", value);
    am_pic_mode_t params;
    memset(&params, 0, sizeof(params));

    if (mbCpqCfg_amvecm_basic_enable) {
        params.flag |= 0x1;
        params.brightness = value;
    }

    if (mbCpqCfg_amvecm_basic_withOSD_enable) {
        params.flag |= (0x1<<1);
        params.brightness2 = value;
    }

    int ret = VPPDeviceIOCtl(AMVECM_IOC_S_PIC_MODE, &params);
    if (ret < 0) {
        SYS_LOGE("%s error: %s!\n", __FUNCTION__, strerror(errno));
    }

    return ret;
}

//Contrast
int CPQControl::SetContrast(int value, int is_save)
{
    int ret = 0;
    SYS_LOGI("%s, source: %d, value = %d\n", __FUNCTION__, mSourceInputForSaveParam, value);
    ret = Cpq_SetContrast(value, mCurrentSourceInputInfo);

    if ((ret == 0) && (is_save == 1)) {
        ret = SaveContrast(value);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::GetContrast(void)
{
    int data = 50;
    vpp_picture_mode_t pq_mode = (vpp_picture_mode_t)GetPQMode();
    vpp_pq_para_t pq_para;

    if (mbCpqCfg_new_picture_mode_enable) {
        vpp_pictur_mode_para_t para;
        if (GetPictureModeData(mCurrentPqSource, pq_mode, &para) == 0) {
            data = para.Contrast;
        }
    } else {
        if (GetPQParams(mCurrentSourceInputInfo, pq_mode, &pq_para) == 0) {
            data = pq_para.contrast;
        }
    }
    if (data < 0 || data > 100) {
        data = 50;
    }

    SYS_LOGD("%s, source: %d, value = %d\n", __FUNCTION__, mSourceInputForSaveParam, data);
    return data;
}

int CPQControl::SaveContrast(int value)
{
    SYS_LOGD("%s, source: %d, timming: %d value = %d\n", __FUNCTION__, mSourceInputForSaveParam, mCurrentPqSource.pq_sig_fmt, value);
    int ret = -1;
    if (mbCpqCfg_new_picture_mode_enable) {
        vpp_pictur_mode_para_t para;
        vpp_picture_mode_t pq_mode = (vpp_picture_mode_t)GetPQMode();
        if (GetPictureModeData(mCurrentPqSource, pq_mode, &para) == 0) {
            para.Contrast = value;
            ret = SetPictureModeData(mCurrentPqSource, pq_mode, &para);
        }
    } else {
        ret = mSSMAction->SSMSaveContrast(mSourceInputForSaveParam, value);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::Cpq_SetContrastBasicParam(source_input_param_t source_input_param)
{
    int ret = -1;
    ret = mPQdb->LoadVppBasicParam(TVPQ_DATA_CONTRAST, source_input_param);
    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::Cpq_SetContrast(int value, source_input_param_t source_input_param)
{
    int ret = -1;
    int params;
    int level;
    if (mbCpqCfg_amvecm_basic_enable || mbCpqCfg_amvecm_basic_withOSD_enable) {
        if (value >= 0 && value <= 100) {
            level = value;
            if (mPQdb->PQ_GetContrastParams(source_input_param, level, &params) == 0) {
                if (Cpq_SetVideoContrast(params) == 0) {
                    return 0;
                }
            } else {
                SYS_LOGE("%s: PQ_GetContrastParams failed!\n", __FUNCTION__);
            }
        }
    } else {
        SYS_LOGD("%s: brightness contrast saturation hue module disabled!\n", __FUNCTION__);
        ret = 0;
    }

    return ret;
}

int CPQControl::Cpq_SetVideoContrast(int value)
{
    SYS_LOGD("Cpq_SetVideoContrast: %d", value);
    am_pic_mode_t params;
    memset(&params, 0, sizeof(params));

    if (mbCpqCfg_amvecm_basic_enable) {
        params.flag |= (0x1<<4);
        params.contrast = value;
    }

    if (mbCpqCfg_amvecm_basic_withOSD_enable) {
        params.flag |= (0x1<<5);
        params.contrast2 = value;
    }

    int ret = VPPDeviceIOCtl(AMVECM_IOC_S_PIC_MODE, &params);
    if (ret < 0) {
        SYS_LOGE("%s error: %s!\n", __FUNCTION__, strerror(errno));
    }

    return ret;
}

//Saturation
int CPQControl::SetSaturation(int value, int is_save)
{
    int ret = 0;
    SYS_LOGI("%s, source: %d, value = %d\n", __FUNCTION__, mSourceInputForSaveParam, value);
    ret = Cpq_SetSaturation(value, mCurrentSourceInputInfo);

    if ((ret == 0) && (is_save == 1)) {
        ret = SaveSaturation(value);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::GetSaturation(void)
{
    int data = 50;
    vpp_picture_mode_t pq_mode = (vpp_picture_mode_t)GetPQMode();
    vpp_pq_para_t pq_para;

    if (mbCpqCfg_new_picture_mode_enable) {
        vpp_pictur_mode_para_t para;
        if (GetPictureModeData(mCurrentPqSource, pq_mode, &para) == 0) {
            data = para.Saturation;
        }
    } else {
        if (GetPQParams(mCurrentSourceInputInfo, pq_mode, &pq_para) == 0) {
            data = pq_para.saturation;
        }
    }

    if (data < 0 || data > 100) {
        data = 50;
    }

    SYS_LOGD("%s, source: %d, value = %d\n", __FUNCTION__, mSourceInputForSaveParam, data);
    return data;
}

int CPQControl::SaveSaturation(int value)
{
    SYS_LOGD("%s, source: %d, value = %d\n", __FUNCTION__, mSourceInputForSaveParam, value);
    int ret = -1;
    if (mbCpqCfg_new_picture_mode_enable) {
        vpp_pictur_mode_para_t para;
        vpp_picture_mode_t pq_mode = (vpp_picture_mode_t)GetPQMode();
        if (GetPictureModeData(mCurrentPqSource, pq_mode, &para) == 0) {
            para.Saturation = value;
            ret = SetPictureModeData(mCurrentPqSource, pq_mode, &para);
        }
    } else {
        ret = mSSMAction->SSMSaveSaturation(mSourceInputForSaveParam, value);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::Cpq_SetSaturationBasicParam(source_input_param_t source_input_param)
{
    int ret = -1;
    ret = mPQdb->LoadVppBasicParam(TVPQ_DATA_SATURATION, source_input_param);
    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::Cpq_SetSaturation(int value, source_input_param_t source_input_param)
{
    int ret = -1;
    int saturation = 0, hue = 0;
    int saturation_level = 0, hue_level = 0;
    if (mbCpqCfg_amvecm_basic_enable || mbCpqCfg_amvecm_basic_withOSD_enable) {
        if (value >= 0 && value <= 100) {
            saturation_level = value;
            if (((source_input_param.source_input == SOURCE_TV) ||
                  (source_input_param.source_input == SOURCE_AV1) ||
                  (source_input_param.source_input == SOURCE_AV2)) &&
                ((source_input_param.sig_fmt == TVIN_SIG_FMT_CVBS_NTSC_M) ||
                 (source_input_param.sig_fmt == TVIN_SIG_FMT_CVBS_NTSC_443))) {
                hue_level = 100 - GetHue();
            } else {
                hue_level = 50;
            }
            ret = mPQdb->PQ_GetHueParams(source_input_param, hue_level, &hue);
            if (ret == 0) {
                ret = mPQdb->PQ_GetSaturationParams(source_input_param, saturation_level, &saturation);
                if (ret == 0) {
                    ret = Cpq_SetVideoSaturationHue(saturation, hue);
                } else {
                    SYS_LOGE("%s: PQ_GetSaturationParams failed!\n", __FUNCTION__);
                }
            } else {
                SYS_LOGE("%s: PQ_GetHueParams failed!\n", __FUNCTION__);
            }
        }
    }else {
        SYS_LOGD("%s: brightness contrast saturation hue module disabled!\n", __FUNCTION__);
        ret = 0;
    }

    return ret;
}

//Hue
int CPQControl::SetHue(int value, int is_save)
{
    int ret = 0;
    SYS_LOGI("%s, source: %d, value = %d\n", __FUNCTION__, mSourceInputForSaveParam, value);
    ret = Cpq_SetHue(value, mCurrentSourceInputInfo);

    if ((ret == 0) && (is_save == 1)) {
        ret = SaveHue(value);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::GetHue(void)
{
    int data = 50;
    vpp_picture_mode_t pq_mode = (vpp_picture_mode_t)GetPQMode();
    vpp_pq_para_t pq_para;

    if (mbCpqCfg_new_picture_mode_enable) {
        vpp_pictur_mode_para_t para;
        if (GetPictureModeData(mCurrentPqSource, pq_mode, &para) == 0) {
            data = para.Hue;
        }
    } else {
        if (GetPQParams(mCurrentSourceInputInfo, pq_mode, &pq_para) == 0) {
            data = pq_para.hue;
        }
    }

    if (data < 0 || data > 100) {
        data = 50;
    }

    SYS_LOGD("%s, source: %d, timming: %d value = %d\n", __FUNCTION__, mSourceInputForSaveParam,mCurrentPqSource.pq_sig_fmt, data);
    return data;
}

int CPQControl::SaveHue(int value)
{
    SYS_LOGD("%s, source: %d, value = %d\n", __FUNCTION__, mSourceInputForSaveParam, value);
    int ret = -1;
    if (mbCpqCfg_new_picture_mode_enable) {
        vpp_pictur_mode_para_t para;
        vpp_picture_mode_t pq_mode = (vpp_picture_mode_t)GetPQMode();
        if (GetPictureModeData(mCurrentPqSource, pq_mode, &para) == 0) {
            para.Hue = value;
            ret = SetPictureModeData(mCurrentPqSource, pq_mode, &para);
        }
    } else {
        ret = mSSMAction->SSMSaveHue(mSourceInputForSaveParam, value);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::Cpq_SetHueBasicParam(source_input_param_t source_input_param)
{
    int ret = -1;
    ret = mPQdb->LoadVppBasicParam(TVPQ_DATA_HUE, source_input_param);
    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::Cpq_SetHue(int value, source_input_param_t source_input_param)
{
    int ret = -1;
    int hue_params = 0, saturation_params = 0;
    int hue_level = 0, saturation_level = 0;
    if (mbCpqCfg_amvecm_basic_enable || mbCpqCfg_amvecm_basic_withOSD_enable) {
        if (value >= 0 && value <= 100) {
            hue_level = 100 - value;
            ret = mPQdb->PQ_GetHueParams(source_input_param, hue_level, &hue_params);
            if (ret == 0) {
                saturation_level = GetSaturation();
                ret = mPQdb->PQ_GetSaturationParams(source_input_param, saturation_level, &saturation_params);
                if (ret == 0) {
                    ret = Cpq_SetVideoSaturationHue(saturation_params, hue_params);
                } else {
                    SYS_LOGE("PQ_GetSaturationParams failed!\n");
                }
            } else {
                SYS_LOGE("PQ_GetHueParams failed!\n");
            }
        }
    } else {
        SYS_LOGD("%s: brightness contrast saturation hue module disabled!\n", __FUNCTION__);
        ret = 0;
    }

    return ret;
}

int CPQControl::Cpq_SetVideoSaturationHue(int satVal, int hueVal)
{
    signed long temp;
    SYS_LOGD("Cpq_SetVideoSaturationHue: %d %d", satVal, hueVal);
    am_pic_mode_t params;
    memset(&params, 0, sizeof(params));
    video_set_saturation_hue(satVal, hueVal, &temp);

    if (mbCpqCfg_amvecm_basic_enable) {
        params.flag |= (0x1<<2);
        params.saturation_hue = temp;
    }

    if (mbCpqCfg_amvecm_basic_withOSD_enable) {
        params.flag |= (0x1<<3);
        params.saturation_hue_post = temp;
    }

    int ret = VPPDeviceIOCtl(AMVECM_IOC_S_PIC_MODE, &params);
    if (ret < 0) {
        SYS_LOGE("%s error: %s!\n", __FUNCTION__, strerror(errno));
    }

    return ret;
}

void CPQControl::video_set_saturation_hue(signed char saturation, signed char hue, signed long *mab)
{
    signed short ma = (signed short) (cos((float) hue * PI / 128.0) * ((float) saturation / 128.0
                                      + 1.0) * 256.0);
    signed short mb = (signed short) (sin((float) hue * PI / 128.0) * ((float) saturation / 128.0
                                      + 1.0) * 256.0);

    if (ma > 511) {
        ma = 511;
    }

    if (ma < -512) {
        ma = -512;
    }

    if (mb > 511) {
        mb = 511;
    }

    if (mb < -512) {
        mb = -512;
    }

    *mab = ((ma & 0x3ff) << 16) | (mb & 0x3ff);
}

void CPQControl::video_get_saturation_hue(signed char *sat, signed char *hue, signed long *mab)
{
    signed long temp = *mab;
    signed int ma = (signed int) ((temp << 6) >> 22);
    signed int mb = (signed int) ((temp << 22) >> 22);
    signed int sat16 = (signed int) ((sqrt(
                                          ((float) ma * (float) ma + (float) mb * (float) mb) / 65536.0) - 1.0) * 128.0);
    signed int hue16 = (signed int) (atan((float) mb / (float) ma) * 128.0 / PI);

    if (sat16 > 127) {
        sat16 = 127;
    }

    if (sat16 < -128) {
        sat16 = -128;
    }

    if (hue16 > 127) {
        hue16 = 127;
    }

    if (hue16 < -128) {
        hue16 = -128;
    }

    *sat = (signed char) sat16;
    *hue = (signed char) hue16;
}

//sharpness
int CPQControl::SetSharpness(int value, int is_enable __unused, int is_save)
{
    SYS_LOGI("%s, source: %d, value = %d\n", __FUNCTION__, mSourceInputForSaveParam, value);
    int ret = Cpq_SetSharpness(value, mCurrentSourceInputInfo);
    if ((ret== 0) && (is_save == 1)) {
        ret = SaveSharpness(value);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::GetSharpness(void)
{
    int data = 50;
    vpp_picture_mode_t pq_mode = (vpp_picture_mode_t)GetPQMode();
    vpp_pq_para_t pq_para;

    if (mbCpqCfg_new_picture_mode_enable) {
        vpp_pictur_mode_para_t para;
        if (GetPictureModeData(mCurrentPqSource, pq_mode, &para) == 0) {
            data = para.Sharpness;
        }
    } else {
        if (GetPQParams(mCurrentSourceInputInfo, pq_mode, &pq_para) == 0) {
            data = pq_para.sharpness;
        }
    }

    if (data < 0 || data > 100) {
        data = 50;
    }

    SYS_LOGD("%s, source: %d, value = %d\n", __FUNCTION__, mSourceInputForSaveParam, data);
    return data;
}

int CPQControl::SaveSharpness(int value)
{
    SYS_LOGD("%s, source: %d, value = %d\n", __FUNCTION__, mSourceInputForSaveParam, value);
    int ret = -1;
    if (mbCpqCfg_new_picture_mode_enable) {
        vpp_pictur_mode_para_t para;
        vpp_picture_mode_t pq_mode = (vpp_picture_mode_t)GetPQMode();
        if (GetPictureModeData(mCurrentPqSource, pq_mode, &para) == 0) {
            para.Sharpness = value;
            ret = SetPictureModeData(mCurrentPqSource, pq_mode, &para);
        }
    } else {
        ret = mSSMAction->SSMSaveSharpness(mSourceInputForSaveParam, value);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }
    return ret;
}

int CPQControl::Cpq_SetSharpness(int value, source_input_param_t source_input_param)
{
    int ret = -1;

    if (value >= 0 && value <= 100) {
        am_regs_t regs;
        memset(&regs, 0, sizeof(am_regs_t));
        if (mbCpqCfg_sharpness0_enable) {
            if (mbDatabaseMatchChipStatus) {
                ret = mPQdb->PQ_GetSharpness0Params(source_input_param, value, &regs);
                if (ret == 0) {
                    ret |= Cpq_LoadRegs(regs);
                } else {
                    SYS_LOGE("%s: PQ_GetSharpness0Params failed!\n", __FUNCTION__);
                }
            } else {
                SYS_LOGD("%s: pq.db don't match chip!\n", __FUNCTION__);
            }
        } else {
            SYS_LOGD("%s: sharpness0 module disabled!\n", __FUNCTION__);
        }

        if (mbCpqCfg_sharpness1_enable) {
            if (mbDatabaseMatchChipStatus) {
                ret = mPQdb->PQ_GetSharpness1Params(source_input_param, value, &regs);
                if (ret == 0) {
                    ret |= Cpq_LoadRegs(regs);
                } else {
                    SYS_LOGE("%s: PQ_GetSharpness1Params failed!\n", __FUNCTION__);
                }
            } else {
                SYS_LOGD("%s: pq.db don't match chip!\n", __FUNCTION__);
            }
        } else {
            SYS_LOGD("%s: sharpness1 module disabled!\n", __FUNCTION__);
        }

        if (mbCpqCfg_sharpnesspi_enable) {
            if (mbDatabaseMatchChipStatus) {
                ret = mPQdb->PQ_GetSharpnessPiParams(source_input_param, value, &regs);
                if (ret == 0) {
                    ret |= Cpq_LoadRegs(regs);
                } else {
                    SYS_LOGE("%s: PQ_GetSharpnessPiParams failed!\n", __FUNCTION__);
                }
            } else {
                SYS_LOGD("%s: pq.db don't match chip!\n", __FUNCTION__);
            }
        } else {
            SYS_LOGD("%s: sharpnesspi module disabled!\n", __FUNCTION__);
        }
    }else {
        SYS_LOGE("%s: invalid value!\n", __FUNCTION__);
    }

    return ret;
}

int CPQControl::Cpq_SetSharpness0FixedParam(source_input_param_t source_input_param)
{
    int ret = -1;

    if (mbDatabaseMatchChipStatus) {
        am_regs_t regs;
        memset(&regs, 0, sizeof(am_regs_t));
        ret = mPQdb->PQ_GetSharpness0FixedParams(source_input_param, &regs);
        if (ret < 0) {
            SYS_LOGE("%s: PQ_GetSharpness0FixedParams failed!\n", __FUNCTION__);
        } else {
            ret = Cpq_LoadRegs(regs);
        }
    } else {
        SYS_LOGD("%s: pq.db don't match chip!\n", __FUNCTION__);
        ret = 0;
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n", __FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n", __FUNCTION__);
    }

    return ret;
}

int CPQControl::Cpq_SetSharpness0VariableParam(source_input_param_t source_input_param)
{
    int ret = -1;

    if (mbDatabaseMatchChipStatus) {
        ret = mPQdb->PQ_SetSharpness0VariableParams(source_input_param);
    } else {
        SYS_LOGE("%s: pq.db don't match chip!\n", __FUNCTION__);
        ret = 0;
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n", __FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n", __FUNCTION__);
    }

    return ret;
}

int CPQControl::Cpq_SetSharpness1FixedParam(source_input_param_t source_input_param)
{
    int ret = -1;

    if (mbDatabaseMatchChipStatus) {
        am_regs_t regs;
        memset(&regs, 0, sizeof(am_regs_t));
        ret = mPQdb->PQ_GetSharpness1FixedParams(source_input_param, &regs);
        if (ret < 0) {
            SYS_LOGE("%s: PQ_GetSharpness1FixedParams failed!\n", __FUNCTION__);
        } else {
            ret = Cpq_LoadRegs(regs);
        }
    } else {
        SYS_LOGE("%s: pq.db don't match chip!\n", __FUNCTION__);
        ret = 0;
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n", __FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n", __FUNCTION__);
    }

    return ret;
}

int CPQControl::Cpq_SetSharpness1VariableParam(source_input_param_t source_input_param)
{
    int ret = -1;

    if (mbDatabaseMatchChipStatus) {
        ret = mPQdb->PQ_SetSharpness1VariableParams(source_input_param);
    } else {
        SYS_LOGE("%s: pq.db don't match chip!\n", __FUNCTION__);
        ret = 0;
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n", __FUNCTION__);
    } else {
        SYS_LOGI("%s success!\n", __FUNCTION__);
    }

    return ret;
}
void CPQControl::InitAutoNr(void)
{
    int ret = -1;
    const char *buff = NULL;
    int buf[128] = {0};

    buff = mPQConfigFile->GetString(CFG_SECTION_AUTO_NR, CFG_AUTO_NR_MOTION_TH, NULL);
    pqTransformStringToInt(buff, buf);
    if (buff != NULL) {
        char str1[128] = {0};
        sprintf(str1, "%s %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", "motion_th",
            buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9],
            buf[10], buf[11], buf[12], buf[13], buf[14]);
        SYS_LOGE("%s str1 = %s\n", __FUNCTION__, str1);
        ret = pqWriteSys(AML_AUTO_NR_PARAMS, str1);
    }

    buff = mPQConfigFile->GetString(CFG_SECTION_AUTO_NR, CFG_AUTO_NR_MOTION_LP_YGAIN, NULL);
    pqTransformStringToInt(buff, buf);
    if (buff != NULL) {
        char str2[128] = {0};
        sprintf(str2, "%s %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", "motion_lp_ygain",
            buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9],
            buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);
        SYS_LOGE("%s str2 = %s\n", __FUNCTION__, str2);
        ret |= pqWriteSys(AML_AUTO_NR_PARAMS, str2);
    }

    buff = mPQConfigFile->GetString(CFG_SECTION_AUTO_NR, CFG_AUTO_NR_MOTION_HP_YGAIN, NULL);
    pqTransformStringToInt(buff, buf);
    if (buff != NULL) {
        char str3[128] = {0};
        sprintf(str3, "%s %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", "motion_hp_ygain",
            buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9],
            buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);
        SYS_LOGE("%s str3 = %s\n", __FUNCTION__, str3);
        ret |= pqWriteSys(AML_AUTO_NR_PARAMS, str3);
    }

    buff = mPQConfigFile->GetString(CFG_SECTION_AUTO_NR, CFG_AUTO_NR_MOTION_LP_CGAIN, NULL);
    pqTransformStringToInt(buff, buf);
    if (buff != NULL) {
        char str4[128] = {0};
        sprintf(str4, "%s %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", "motion_lp_cgain",
            buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9],
            buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);
        SYS_LOGE("%s str4 = %s\n", __FUNCTION__, str4);

        ret |= pqWriteSys(AML_AUTO_NR_PARAMS, str4);
    }

    buff = mPQConfigFile->GetString(CFG_SECTION_AUTO_NR, CFG_AUTO_NR_MOTION_HP_CGAIN, NULL);
    pqTransformStringToInt(buff, buf);
    if (buff != NULL) {
        char str5[128] = {0};
        sprintf(str5, "%s %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", "motion_hp_cgain",
            buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9],
            buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);
        SYS_LOGE("%s str5 = %s\n", __FUNCTION__, str5);
        ret |= pqWriteSys(AML_AUTO_NR_PARAMS, str5);
    }

    buff = mPQConfigFile->GetString(CFG_SECTION_AUTO_NR, CFG_AUTO_NR_APL_GAIN, NULL);
    pqTransformStringToInt(buff, buf);
    if (buff != NULL) {
        char str6[128] = {0};
        sprintf(str6, "%s %d %d %d %d %d %d %d %d", "apl_gain",
            buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
        SYS_LOGE("%s str6 = %s\n", __FUNCTION__, str6);
        ret |= pqWriteSys(AML_AUTO_NR_PARAMS, str6);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed\n", __FUNCTION__);
    }

    return;
}

int CPQControl::Cpq_SetSharpnessPiFixedParam(source_input_param_t source_input_param)
{
    int ret = -1;

    if (mbDatabaseMatchChipStatus) {
        am_regs_t regs;
        memset(&regs, 0, sizeof(am_regs_t));
        ret = mPQdb->PQ_GetSharpnessPiFixedParams(source_input_param, &regs);
        if (ret < 0) {
            SYS_LOGE("%s: PQ_GetSharpnessPIFixedParams failed!\n", __FUNCTION__);
        } else {
            ret = Cpq_LoadRegs(regs);
        }
    } else {
        SYS_LOGD("%s: pq.db don't match chip!\n", __FUNCTION__);
        ret = 0;
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n", __FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n", __FUNCTION__);
    }

    return ret;
}

int CPQControl::Cpq_SetSharpnessPiVariableParam(source_input_param_t source_input_param)
{
    int ret = -1;

    if (mbDatabaseMatchChipStatus) {
        ret = mPQdb->PQ_SetSharpnessPiVariableParams(source_input_param);
    } else {
        SYS_LOGD("%s: pq.db don't match chip!\n", __FUNCTION__);
        ret = 0;
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n", __FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n", __FUNCTION__);
    }

    return ret;
}

//NoiseReductionMode
int CPQControl::SetNoiseReductionMode(int nr_mode, int is_save)
{
    SYS_LOGI("%s, source: %d, timming: %d, value = %d\n", __FUNCTION__, mSourceInputForSaveParam, mCurrentPqSource.pq_sig_fmt, nr_mode);
    int ret = Cpq_SetNoiseReductionMode((vpp_noise_reduction_mode_t)nr_mode, mCurrentSourceInputInfo);
    if ((ret ==0) && (is_save == 1)) {
        ret = SaveNoiseReductionMode((vpp_noise_reduction_mode_t)nr_mode);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::GetNoiseReductionMode(void)
{
    int mode = VPP_NOISE_REDUCTION_MODE_MID;
    if (mbCpqCfg_new_picture_mode_enable) {
        vpp_picture_mode_t pq_mode = (vpp_picture_mode_t)GetPQMode();
        vpp_pictur_mode_para_t para;
        if (GetPictureModeData(mCurrentPqSource, pq_mode, &para) == 0) {
            mode = para.Nr;
        }

    } else {
        mSSMAction->SSMReadNoiseReduction(mSourceInputForSaveParam, &mode);
    }

    if (mode < VPP_NOISE_REDUCTION_MODE_OFF || mode > VPP_NOISE_REDUCTION_MODE_AUTO) {
        mode = VPP_NOISE_REDUCTION_MODE_MID;
    }

    SYS_LOGD("%s, source: %d, value = %d\n", __FUNCTION__, mSourceInputForSaveParam, mode);
    return mode;
}

int CPQControl::SaveNoiseReductionMode(int nr_mode)
{
    SYS_LOGD("%s, source: %d, timming: %d value = %d\n", __FUNCTION__, mSourceInputForSaveParam,mCurrentPqSource.pq_sig_fmt, nr_mode);
    int ret = -1;
    if (mbCpqCfg_new_picture_mode_enable) {
        vpp_pictur_mode_para_t para;
        vpp_picture_mode_t pq_mode = (vpp_picture_mode_t)GetPQMode();
        if (GetPictureModeData(mCurrentPqSource, pq_mode, &para) == 0) {
            para.Nr = nr_mode;
            ret = SetPictureModeData(mCurrentPqSource, pq_mode, &para);
        }
    } else {
        ret = mSSMAction->SSMSaveNoiseReduction(mSourceInputForSaveParam, nr_mode);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::Cpq_SetNoiseReductionMode(vpp_noise_reduction_mode_t nr_mode, source_input_param_t source_input_param)
{
    int ret = -1;
    am_regs_t regs;
    am_pq_param_t di_regs;
    memset(&regs, 0x0, sizeof(am_regs_t));
    memset(&di_regs, 0x0,sizeof(am_pq_param_t));

    if (mbCpqCfg_nr_enable) {
        if (mPQdb->PQ_GetNR2Params((vpp_noise_reduction_mode_t)nr_mode, source_input_param, &regs) == 0) {
            di_regs.table_name = TABLE_NAME_NR;
            di_regs.table_len = regs.length;
            am_reg_t tmp_buf[regs.length];
            for (unsigned int i=0;i<regs.length;i++) {
                  tmp_buf[i].addr = regs.am_reg[i].addr;
                  tmp_buf[i].mask = regs.am_reg[i].mask;
                  tmp_buf[i].type = regs.am_reg[i].type;
                  tmp_buf[i].val  = regs.am_reg[i].val;
            }
            di_regs.table_ptr = (long long)tmp_buf;

            ret = DI_LoadRegs(di_regs);
        } else {
            SYS_LOGE("PQ_GetNR2Params failed!\n");
        }
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

//Gamma
int CPQControl::SetGammaValue(vpp_gamma_curve_t gamma_curve, int is_save)
{
    SYS_LOGD("%s, source: %d, value = %d\n", __FUNCTION__, mSourceInputForSaveParam, gamma_curve);
    int ret = -1;

    ret = Cpq_LoadGamma(gamma_curve, (vpp_color_temperature_mode_t)GetColorTemperature());

    if ((ret == 0) && (is_save == 1)) {
        ret = mSSMAction->SSMSaveGammaValue(mSourceInputForSaveParam, gamma_curve);    
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }
    return ret;
}

int CPQControl::GetGammaValue()
{
    int gammaValue = 0;
    if (mSSMAction->SSMReadGammaValue(mSourceInputForSaveParam, &gammaValue) < 0) {
        SYS_LOGE("%s, SSMReadGammaValue ERROR!!!\n", __FUNCTION__);
        return -1;
    }

    SYS_LOGD("%s, source: %d, value = %d\n", __FUNCTION__, mSourceInputForSaveParam, gammaValue);
    return gammaValue;
}

int CPQControl::Cpq_LoadGamma(vpp_gamma_curve_t gamma_curve, vpp_color_temperature_mode_t colortemp_mode)
{
    if (!mbCpqCfg_gamma_enable) {
        SYS_LOGD("Gamma module disabled!\n");
        return 0;
    }

    if (gamma_curve <= VPP_GAMMA_CURVE_DEFAULT || gamma_curve > VPP_GAMMA_CURVE_MAX) {
        SYS_LOGD("Gamma index out of range, as gamma disabled!\n");
        return 0;
    }

    int ret = 0;
    //gamma curve
    GAMMA_TABLE gamma_r, gamma_g, gamma_b;
    ret |= mPQdb->PQ_GetGammaSpecialTable(gamma_curve, "Red", &gamma_r);
    ret |= mPQdb->PQ_GetGammaSpecialTable(gamma_curve, "Green", &gamma_g);
    ret |= mPQdb->PQ_GetGammaSpecialTable(gamma_curve, "Blue", &gamma_b);
    if (ret < 0) {
        SYS_LOGE("%s: get PQ_GetGammaSpecialTable fail\n", __FUNCTION__);
        return -1;
    }

    //colortemp gamma curve
    tcon_gamma_table_t WB_GAMMA_R, WB_GAMMA_G, WB_GAMMA_B;
    ret |= mPQdb->PQ_GetWhiteBalanceGammaSpecialTable(colortemp_mode, "Red", &WB_GAMMA_R);
    ret |= mPQdb->PQ_GetWhiteBalanceGammaSpecialTable(colortemp_mode, "Green", &WB_GAMMA_G);
    ret |= mPQdb->PQ_GetWhiteBalanceGammaSpecialTable(colortemp_mode, "Blue", &WB_GAMMA_B);
    if (ret < 0) {
        SYS_LOGE("%s: get PQ_GetWhiteBalanceGammaSpecialTable fail\n", __FUNCTION__);
        return -1;
    }

    //blend gamma
    tcon_gamma_table_t blend_gamma_r, blend_gamma_g, blend_gamma_b;
    DBGammaBlend(&WB_GAMMA_R, &gamma_r, &blend_gamma_r);
    DBGammaBlend(&WB_GAMMA_G, &gamma_g, &blend_gamma_g);
    DBGammaBlend(&WB_GAMMA_B, &gamma_b, &blend_gamma_b);

    // set final gamma to driver
    ret |= Cpq_SetGammaTbl_R(blend_gamma_r.data);
    ret |= Cpq_SetGammaTbl_G(blend_gamma_g.data);
    ret |= Cpq_SetGammaTbl_B(blend_gamma_b.data);
    if (ret < 0) {
        SYS_LOGE("%s: Cpq_SetGammaTbl fail\n", __FUNCTION__);
        return -1;
    }

    return ret;
}

int CPQControl::DBGammaBlend(tcon_gamma_table_t *wb_gamma, GAMMA_TABLE *index_gamma, tcon_gamma_table_t *target_gamma)
{
    unsigned int i, final_value;
    unsigned int blend_alp, blend_bet;
    for (i = 1; i < (GAMMA_NUMBER - 1); i++) {
        blend_alp = index_gamma->data[i] / 1000;
        blend_bet = index_gamma->data[i] % 1000;
        if (blend_alp > 255) {
            SYS_LOGD("%s, blend_gamma->data[i] = %d\n", __FUNCTION__, i, index_gamma->data[i]);
            SYS_LOGD("%s, blend_alp = %d\n", __FUNCTION__, blend_alp);
            SYS_LOGD("%s, blend_bet = %d\n", __FUNCTION__, blend_bet);
            continue;
        }
        final_value = wb_gamma->data[blend_alp] + (wb_gamma->data[blend_alp + 1] - wb_gamma->data[blend_alp]) * blend_bet / 1000;
        target_gamma->data[i] = (unsigned short)final_value;
        //SYS_LOGD("%s, target_gamma->data[%d] = %d\n", __FUNCTION__, i, target_gamma->data[i]);
    }

    target_gamma->data[0] = wb_gamma->data[0];
    target_gamma->data[GAMMA_NUMBER - 1] = wb_gamma->data[GAMMA_NUMBER - 1];

    return 0;
}

int CPQControl::Cpq_SetGammaTbl_R(unsigned short red[GAMMA_NUMBER])
{
    struct tcon_gamma_table_s Redtbl;
    int ret = -1, i = 0;

    for (i = 0; i < GAMMA_NUMBER; i++) {
        Redtbl.data[i] = red[i];
    }

    ret = VPPDeviceIOCtl(AMVECM_IOC_GAMMA_TABLE_R, &Redtbl);
    if (ret < 0) {
        SYS_LOGE("%s error(%s)!\n", __FUNCTION__, strerror(errno));
    }
    return ret;
}

int CPQControl::Cpq_SetGammaTbl_G(unsigned short green[GAMMA_NUMBER])
{
    struct tcon_gamma_table_s Greentbl;
    int ret = -1, i = 0;

    for (i = 0; i < GAMMA_NUMBER; i++) {
        Greentbl.data[i] = green[i];
    }

    ret = VPPDeviceIOCtl(AMVECM_IOC_GAMMA_TABLE_G, &Greentbl);
    if (ret < 0) {
        SYS_LOGE("%s error(%s)!\n", __FUNCTION__, strerror(errno));
    }

    return ret;
}

int CPQControl::Cpq_SetGammaTbl_B(unsigned short blue[GAMMA_NUMBER])
{
    struct tcon_gamma_table_s Bluetbl;
    int ret = -1, i = 0;

    for (i = 0; i < GAMMA_NUMBER; i++) {
        Bluetbl.data[i] = blue[i];
    }

    ret = VPPDeviceIOCtl(AMVECM_IOC_GAMMA_TABLE_B, &Bluetbl);
    if (ret < 0) {
        SYS_LOGE("%s error(%s)!\n", __FUNCTION__, strerror(errno));
    }

    return ret;
}

//MEMC
bool CPQControl::hasMemcFunc() {
    if (mMemcFd > 0 && mbCpqCfg_memc_enable) {
        SYS_LOGD("%s, has memc\n", __FUNCTION__);
        return true;
    }
    SYS_LOGD("%s, has NO memc\n", __FUNCTION__);
    return false;
}

int CPQControl::initMemc(void) {
    int ret = -1;
    if (mbCpqCfg_memc_enable) {
        if (mMemcFd > 0) {
            Memc_enable(1);
            ret = 0;
        } else {
            ret = -1;
        }
    } else {
        SYS_LOGD("%s, memc disabled\n", __FUNCTION__);
        ret = 0;
    }

    if (ret >= 0) {
        SYS_LOGD("%s, success\n", __FUNCTION__);
    } else {
        SYS_LOGD("%s, fail\n", __FUNCTION__);
    }

    return ret;
}

int CPQControl::Memc_enable(int enable)
{
    int ret = -1;
    ret = MEMCDeviceIOCtl(MEMDEV_CONTRL, &enable);

    if (ret >= 0) {
        SYS_LOGD("%s, success\n", __FUNCTION__);
    } else {
        SYS_LOGD("%s, fail\n", __FUNCTION__);
    }
    return ret;
}

int CPQControl::SetMemcMode(int memc_mode, int is_save)
{
    SYS_LOGD("%s, mode = %d\n", __FUNCTION__, memc_mode);
    int ret = -1;

    ret = Cpq_SetMemcMode((vpp_memc_mode_t)memc_mode, mCurrentSourceInputInfo);

    if (ret == 0 && is_save == 1) {
        ret = SaveMemcMode((vpp_memc_mode_t)memc_mode);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::GetMemcMode()
{
    int level = VPP_MEMC_MODE_OFF;
    if (mSSMAction->SSMReadMemcMode(mSourceInputForSaveParam, &level) < 0) {
        SYS_LOGE("%s, SSMReadMemcDeblurLevel ERROR!!!\n", __FUNCTION__);
        return VPP_MEMC_MODE_OFF;
    } else {
        SYS_LOGD("%s, source: %d, level = %d\n", __FUNCTION__, mSourceInputForSaveParam, level);
    }

    if(level < VPP_MEMC_MODE_OFF || level >= VPP_MEMC_MODE_MAX) {
        level = VPP_MEMC_MODE_OFF;
    }

    return level;
}

int CPQControl::SaveMemcMode(vpp_memc_mode_t memc_mode)
{
    SYS_LOGD("%s, source: %d, mode = %d\n", __FUNCTION__, mSourceInputForSaveParam, memc_mode);
    int ret = mSSMAction->SSMSaveMemcMode(mSourceInputForSaveParam, memc_mode);

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::Cpq_SetMemcMode(vpp_memc_mode_t memc_mode, source_input_param_t source_input_param)
{
    int ret = -1;
    int DeJudder_level = 0;
    int DeBlur_Level = 0;
    int offset = source_input_param.source_input * VPP_MEMC_MODE_MAX + memc_mode;

    if (mMemcFd > 0 && mbCpqCfg_memc_enable) {
        if (mSSMAction->SSMReadMemcDeJudderLevel(offset, &DeJudder_level) < 0) {
            SYS_LOGE("%s, SSMReadMemcDeJudderLevel ERROR!!!\n", __FUNCTION__);
        }

        if (mSSMAction->SSMReadMemcDeblurLevel(offset, &DeBlur_Level) < 0) {
            SYS_LOGE("%s, SSMReadMemcDeblurLevel ERROR!!!\n", __FUNCTION__);
        }

        ret = Cpq_SetMemcDeJudderLevel(DeJudder_level, mCurrentSourceInputInfo);
        ret |= Cpq_SetMemcDeBlurLevel(DeBlur_Level, mCurrentSourceInputInfo);

        if (memc_mode == VPP_MEMC_MODE_OFF) {
            ret |= Memc_enable(0);
        } else {
            ret |= Memc_enable(1);
        }
    } else {
        SYS_LOGE("Memc module disabled!!!\n");
        ret = 0;
    }

    return ret;
}

int CPQControl::SetMemcDeBlurLevel(int level, int is_save)
{
    SYS_LOGD("%s, source: %d, level = %d\n", __FUNCTION__, mSourceInputForSaveParam, level);
    int ret = -1;

    ret = Cpq_SetMemcDeBlurLevel(level, mCurrentSourceInputInfo);
    if ((ret ==0) && (is_save == 1)) {
        ret = SaveMemcDeBlurLevel(level);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::GetMemcDeBlurLevel(void)
{
    int level = 0;
    int mode = GetMemcMode();
    int offset = mSourceInputForSaveParam * VPP_MEMC_MODE_MAX + mode;
    if (mSSMAction->SSMReadMemcDeblurLevel(offset, &level) < 0) {
        SYS_LOGE("%s, SSMReadMemcDeblurLevel ERROR!!!\n", __FUNCTION__);
        return -1;
    }

    SYS_LOGD("%s, source: %d, level = %d offset = %d\n", __FUNCTION__, mSourceInputForSaveParam, level, offset);
    return level;
}

int CPQControl::SaveMemcDeBlurLevel(int level)
{
    SYS_LOGD("%s, source: %d, level = %d\n", __FUNCTION__, mSourceInputForSaveParam, level);
    int mode = GetMemcMode();
    int offset = mSourceInputForSaveParam * VPP_MEMC_MODE_MAX + mode;
    int ret = mSSMAction->SSMSaveMemcDeblurLevel(offset, level);

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::Cpq_SetMemcDeBlurLevel(int level, source_input_param_t source_input_param)
{
    SYS_LOGD("%s, level = %d\n", __FUNCTION__, level);
    int ret = -1;

    if (mbCpqCfg_memc_enable) {
        //ret = MEMCDeviceIOCtl(FRC_IOC_SET_MEMC_LEVEL, &level);
        ret = 0;
    } else {
        SYS_LOGD("%s memc Disabled!\n",__FUNCTION__);
        ret = 0;
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::SetMemcDeJudderLevel(int level, int is_save)
{
    SYS_LOGD("%s, source: %d, level = %d\n", __FUNCTION__, mSourceInputForSaveParam, level);
    int ret = -1;
    ret = Cpq_SetMemcDeJudderLevel(level, mCurrentSourceInputInfo);
    if ((ret ==0) && (is_save == 1)) {
        ret = SaveMemcDeJudderLevel(level);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::GetMemcDeJudderLevel(void)
{
    int level = 0;
    int mode = GetMemcMode();
    int offset = mSourceInputForSaveParam * VPP_MEMC_MODE_MAX + mode;
    if (mSSMAction->SSMReadMemcDeJudderLevel(offset, &level) < 0) {
        SYS_LOGE("%s, SSMReadMemcDeJudderLevel ERROR!!!\n", __FUNCTION__);
        return -1;
    }

    SYS_LOGD("%s, source: %d, level = %d\n", __FUNCTION__, mSourceInputForSaveParam, level);
    return level;
}

int CPQControl::SaveMemcDeJudderLevel(int level)
{
    SYS_LOGD("%s, source: %d, level = %d\n", __FUNCTION__, mSourceInputForSaveParam, level);
    int mode = GetMemcMode();
    int offset = mSourceInputForSaveParam * VPP_MEMC_MODE_MAX + mode;
    int ret = mSSMAction->SSMSaveMemcDeJudderLevel(offset, level);

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::Cpq_SetMemcDeJudderLevel(int level, source_input_param_t source_input_param)
{
    SYS_LOGD("%s, level = %d\n", __FUNCTION__, level);
    int ret = -1;
    if (mbCpqCfg_memc_enable) {
        ret = MEMCDeviceIOCtl(FRC_IOC_SET_MEMC_LEVEL, &level);
    } else {
        SYS_LOGD("%s memc Disabled!\n",__FUNCTION__);
        ret = 0;
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

//Displaymode
int CPQControl::SetDisplayMode(vpp_display_mode_t display_mode, int is_save)
{
    SYS_LOGD("%s, source: %d, value = %d\n", __FUNCTION__, mSourceInputForSaveParam, display_mode);
    int ret = -1;

    //dtvkit process afd function,driver need output full
    if (display_mode == VPP_DISPLAY_MODE_NORMAL) {
        pqWriteSys(VPP_AFD_MODULE_ASPECT_MODE, "0 0");//set auto to afd before pq display
    }
    if (mbDtvKitEnable && (display_mode == VPP_DISPLAY_MODE_NORMAL)) {
        ret = Cpq_SetDisplayModeAllTiming(mCurrentSourceInputInfo.source_input, display_mode);
        ret = Cpq_SetDisplayModeScreenMode(mCurrentSourceInputInfo.source_input, display_mode);
    } else if ((mCurrentSourceInputInfo.source_input == SOURCE_DTV)
        || (mCurrentSourceInputInfo.source_input == SOURCE_TV)
        || (mCurrentSourceInputInfo.source_input == SOURCE_AV1)
        || (mCurrentSourceInputInfo.source_input == SOURCE_AV2)) {
        ret = Cpq_SetDisplayModeAllTiming(mCurrentSourceInputInfo.source_input, display_mode);
    } else {
        ret = Cpq_SetDisplayModeAllTiming(mCurrentSourceInputInfo.source_input, display_mode);
        ret = Cpq_SetDisplayModeOneTiming(mCurrentSourceInputInfo.source_input, display_mode);
    }
    if (display_mode != VPP_DISPLAY_MODE_NORMAL) {
        pqWriteSys(VPP_AFD_MODULE_ASPECT_MODE, "0 5");//set custom to afd after pq display
    }

    if ((ret == 0) && (is_save == 1)) {
        ret = SaveDisplayMode(display_mode);
    }

    return ret;
}

int CPQControl::GetDisplayMode()
{
    int mode = VPP_DISPLAY_MODE_169;
    mSSMAction->SSMReadDisplayMode(mSourceInputForSaveParam, &mode);
    if (mode < VPP_DISPLAY_MODE_169 || mode >= VPP_DISPLAY_MODE_MAX) {
        mode = VPP_DISPLAY_MODE_169;
    }

    SYS_LOGD("%s, source: %d, value = %d\n", __FUNCTION__, mSourceInputForSaveParam, mode);
    return mode;
}

int CPQControl::SaveDisplayMode(vpp_display_mode_t display_mode)
{
    SYS_LOGD("%s, source: %d, value = %d\n", __FUNCTION__, mSourceInputForSaveParam, display_mode);
    int ret = mSSMAction->SSMSaveDisplayMode(mSourceInputForSaveParam, (int)display_mode);
    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::Cpq_SetDisplayModeCrop(tv_source_input_t source_input, vpp_display_mode_t display_mode)
{
    int ret = -1;
    tvin_cutwin_t cutwin;
    if (mbCpqCfg_display_overscan_enable) {
        if (mbCpqCfg_separate_db_enable) {
            ret = mpOverScandb->PQ_GetOverscanParams(mCurrentSourceInputInfo, display_mode, &cutwin);
        }
    } else {
        SYS_LOGD("%s: Overscan module disabled\n", __FUNCTION__);
        ret = 0;
        cutwin.he = 0;
        cutwin.hs = 0;
        cutwin.ve = 0;
        cutwin.vs = 0;
    }

    if (ret == 0) {
       if (source_input == SOURCE_DTV) {//DTVKIT
            cutwin.vs = 0;
            cutwin.hs = 0;
            cutwin.ve = 0;
            cutwin.he = 0;
        } else if (source_input == SOURCE_MPEG) {//MPEG
            cutwin.vs = 0;
            cutwin.hs = 0;
            cutwin.ve = 0;
            cutwin.he = 0;
        } else if ((source_input >= SOURCE_HDMI1) && (source_input <= SOURCE_HDMI4)) {//hdmi source
            if (GetPQMode() == VPP_PICTURE_MODE_MONITOR) {//hdmi monitor mode
                cutwin.vs = 0;
                cutwin.hs = 0;
                cutwin.ve = 0;
                cutwin.he = 0;
            }
        }

        SYS_LOGD("%s: display_mode:%d hs:%d he:%d vs:%d ve:%d\n", __FUNCTION__, display_mode, cutwin.hs, cutwin.he, cutwin.vs, cutwin.ve);
        Cpq_SetVideoCrop(cutwin.vs, cutwin.hs, cutwin.ve, cutwin.he);
    } else {
        SYS_LOGD("PQ_GetOverscanParams failed\n");
    }

    return ret;
}

int CPQControl::Cpq_SetDisplayModeScreenMode(tv_source_input_t source_input, vpp_display_mode_t display_mode)
{
    int ret = 0;

    int ScreenModeValue = Cpq_GetScreenModeValue(display_mode);
    if (source_input == SOURCE_DTV) {//DTVKIT
        if ((mbDtvKitEnable && (ScreenModeValue == SCREEN_MODE_NORMAL))
            ||(!mbDtvKitEnable)) {
                ScreenModeValue = SCREEN_MODE_FULL_STRETCH;
        }
    } else if (source_input == SOURCE_MPEG) {//MPEG
        if ((mbDtvKitEnable && (ScreenModeValue == SCREEN_MODE_NORMAL))
            ||(!mbDtvKitEnable)) {
                ScreenModeValue = SCREEN_MODE_FULL_STRETCH;
        }
    } else if ((source_input >= SOURCE_HDMI1) && (source_input <= SOURCE_HDMI4)) {//hdmi source
        if (display_mode == VPP_DISPLAY_MODE_NORMAL) {//auto mode
            if (mCurrentAfdInfo == TVIN_ASPECT_4x3_FULL) {
                ScreenModeValue = SCREEN_MODE_4_3;
            } else if (mCurrentAfdInfo == TVIN_ASPECT_14x9_FULL) {
                ScreenModeValue = SCREEN_MODE_NORMAL;
            } else if (mCurrentAfdInfo == TVIN_ASPECT_16x9_FULL) {
                ScreenModeValue = SCREEN_MODE_16_9;
            } else {
                SYS_LOGD("%s: invalid AFD status\n", __FUNCTION__);
            }
        }
    }

    SYS_LOGD("%s: screenmode:%d\n", __FUNCTION__, ScreenModeValue);
    Cpq_SetVideoScreenMode(ScreenModeValue);

    return ret;
}

int CPQControl::Cpq_SetDisplayModeOneTiming(tv_source_input_t source_input, vpp_display_mode_t display_mode)
{
    int ret = -1;
    tvin_cutwin_t cutwin;
    if (mbCpqCfg_display_overscan_enable) {
        if (mbCpqCfg_separate_db_enable) {
            ret = mpOverScandb->PQ_GetOverscanParams(mCurrentSourceInputInfo, display_mode, &cutwin);
        }
    } else {
        SYS_LOGD("%s: Overscan module disabled!\n", __FUNCTION__);
        ret = 0;
        cutwin.he = 0;
        cutwin.hs = 0;
        cutwin.ve = 0;
        cutwin.vs = 0;
    }

    if (ret == 0) {
        int ScreenModeValue = Cpq_GetScreenModeValue(display_mode);
       if (source_input == SOURCE_DTV) {//DTVKIT
            cutwin.vs = 0;
            cutwin.hs = 0;
            cutwin.ve = 0;
            cutwin.he = 0;
            if ((mbDtvKitEnable && (ScreenModeValue == SCREEN_MODE_NORMAL))
                ||(!mbDtvKitEnable)) {
                    ScreenModeValue = SCREEN_MODE_FULL_STRETCH;
            }
        } else if (source_input == SOURCE_MPEG) {//MPEG
            cutwin.vs = 0;
            cutwin.hs = 0;
            cutwin.ve = 0;
            cutwin.he = 0;
            if ((mbDtvKitEnable && (ScreenModeValue == SCREEN_MODE_NORMAL))
                ||(!mbDtvKitEnable)) {
                    ScreenModeValue = SCREEN_MODE_FULL_STRETCH;
            }
        } else if ((source_input >= SOURCE_HDMI1) && (source_input <= SOURCE_HDMI4)) {//hdmi source
            if (GetPQMode() == VPP_PICTURE_MODE_MONITOR) {//hdmi monitor mode
                cutwin.vs = 0;
                cutwin.hs = 0;
                cutwin.ve = 0;
                cutwin.he = 0;
            }

            if (display_mode == VPP_DISPLAY_MODE_NORMAL) {//auto mode
                if (mCurrentAfdInfo == TVIN_ASPECT_4x3_FULL) {
                    ScreenModeValue = SCREEN_MODE_4_3;
                } else if (mCurrentAfdInfo == TVIN_ASPECT_14x9_FULL) {
                    ScreenModeValue = SCREEN_MODE_NORMAL;
                } else if (mCurrentAfdInfo == TVIN_ASPECT_16x9_FULL) {
                    ScreenModeValue = SCREEN_MODE_16_9;
                } else {
                    SYS_LOGE("%s: invalid AFD status.\n", __FUNCTION__);
                }
            }
        }

        SYS_LOGD("%s: screenmode:%d hs:%d he:%d vs:%d ve:%d\n", __FUNCTION__, ScreenModeValue, cutwin.hs, cutwin.he, cutwin.vs, cutwin.ve);
        Cpq_SetVideoCrop(cutwin.vs, cutwin.hs, cutwin.ve, cutwin.he);
        Cpq_SetVideoScreenMode(ScreenModeValue);
    } else {
        SYS_LOGE("PQ_GetOverscanParams failed!\n");
    }

    return ret;
}

int CPQControl::Cpq_SetDisplayModeAllTiming(tv_source_input_t source_input, vpp_display_mode_t display_mode)
{
    int i = 0, ScreenModeValue = 0, AFDFlag = 0, adapted_mode = 0;
    int ret = -1;
    ve_pq_load_t ve_pq_load_reg;
    memset(&ve_pq_load_reg, 0, sizeof(ve_pq_load_t));

    ve_pq_load_reg.param_id = TABLE_NAME_OVERSCAN;
    ve_pq_load_reg.length = SIG_TIMING_TYPE_MAX;

    ve_pq_table_t ve_pq_table[SIG_TIMING_TYPE_MAX];
    tvin_cutwin_t cutwin[SIG_TIMING_TYPE_MAX];
    memset(ve_pq_table, 0, sizeof(ve_pq_table));
    memset(cutwin, 0, sizeof(cutwin));

    tvin_sig_fmt_t sig_fmt[SIG_TIMING_TYPE_MAX];
    ve_pq_timing_type_t flag[SIG_TIMING_TYPE_MAX];
    sig_fmt[0] = TVIN_SIG_FMT_HDMI_720X480P_60HZ;
    sig_fmt[1] = TVIN_SIG_FMT_HDMI_720X576P_50HZ;
    sig_fmt[2] = TVIN_SIG_FMT_HDMI_1280X720P_60HZ;
    sig_fmt[3] = TVIN_SIG_FMT_HDMI_1920X1080P_60HZ;
    sig_fmt[4] = TVIN_SIG_FMT_HDMI_3840_2160_00HZ;
    sig_fmt[5] = TVIN_SIG_FMT_CVBS_NTSC_M;
    sig_fmt[6] = TVIN_SIG_FMT_CVBS_NTSC_443;
    sig_fmt[7] = TVIN_SIG_FMT_CVBS_PAL_I;
    sig_fmt[8] = TVIN_SIG_FMT_CVBS_PAL_M;
    sig_fmt[9] = TVIN_SIG_FMT_CVBS_PAL_60;
    sig_fmt[10] = TVIN_SIG_FMT_CVBS_PAL_CN;
    sig_fmt[11] = TVIN_SIG_FMT_CVBS_SECAM;
    sig_fmt[12] = TVIN_SIG_FMT_CVBS_NTSC_50;
    flag[0] = SIG_TIMING_TYPE_SD_480;
    flag[1] = SIG_TIMING_TYPE_SD_576;
    flag[2] = SIG_TIMING_TYPE_HD;
    flag[3] = SIG_TIMING_TYPE_FHD;
    flag[4] = SIG_TIMING_TYPE_UHD;
    flag[5] = SIG_TIMING_TYPE_NTSC_M;
    flag[6] = SIG_TIMING_TYPE_NTSC_443;
    flag[7] = SIG_TIMING_TYPE_PAL_I;
    flag[8] = SIG_TIMING_TYPE_PAL_M;
    flag[9] = SIG_TIMING_TYPE_PAL_60;
    flag[10] = SIG_TIMING_TYPE_PAL_CN;
    flag[11] = SIG_TIMING_TYPE_SECAM;
    flag[12] = SIG_TIMING_TYPE_NTSC_50;

    source_input_param_t source_input_param;
    source_input_param.source_input = source_input;
    source_input_param.trans_fmt = mCurrentSourceInputInfo.trans_fmt;
    ScreenModeValue = Cpq_GetScreenModeValue(display_mode);

    //non dtvkit,driver process afd
    if (display_mode == VPP_DISPLAY_MODE_NORMAL) { //auto mode
        AFDFlag = 1;
    } else {
        AFDFlag = 0;
    }

    //dtvkit process afd function,driver need output full
    if (mbDtvKitEnable && (ScreenModeValue == SCREEN_MODE_NORMAL)) {
        adapted_mode    = 0;
        ScreenModeValue = SCREEN_MODE_FULL_STRETCH;
        AFDFlag         = 0;

        ve_pq_table[0].src_timing = (adapted_mode << 31) |
                                    (AFDFlag << 30) |
                                    ((ScreenModeValue & 0x7f) << 24) |
                                    ((source_input & 0x7f) << 16 ) |
                                    (0x0);
        ve_pq_table[0].value1 = 0;
        ve_pq_table[0].value2 = 0;
        ve_pq_load_reg.param_ptr = (long long)ve_pq_table;

        ret = 0;
    } else if (source_input == SOURCE_DTV) {//DTV
        for (i=0;i<SIG_TIMING_TYPE_NTSC_M;i++) {
            adapted_mode = 1;
            ve_pq_table[i].src_timing = (adapted_mode << 31) |
                                        (AFDFlag << 30) |
                                        ((ScreenModeValue & 0x7f) << 24) |
                                        ((source_input & 0x7f) << 16 ) |
                                        (flag[i]);
            source_input_param.sig_fmt = sig_fmt[i];
            if  (display_mode >= VPP_DISPLAY_MODE_FULL_STRETCH && display_mode <= VPP_DISPLAY_MODE_169_COMBINED) {
                SYS_LOGI("%s: Project mode!\n", __FUNCTION__);
                ret = 0;
                cutwin[i].he = 0;
                cutwin[i].hs = 0;
                cutwin[i].ve = 0;
                cutwin[i].vs = 0;
            } else if (mbCpqCfg_display_overscan_enable) {
                if (mbCpqCfg_separate_db_enable) {
                    ret = mpOverScandb->PQ_GetOverscanParams(source_input_param, display_mode, cutwin+i);
                }
            } else {
                SYS_LOGD("%s: Overscan module disabled!\n", __FUNCTION__);
                ret = 0;
                cutwin[i].he = 0;
                cutwin[i].hs = 0;
                cutwin[i].ve = 0;
                cutwin[i].vs = 0;
            }

            //dtvkit process afd function,driver need output full
            if (mbDtvKitEnable && (ScreenModeValue == SCREEN_MODE_FULL_STRETCH)) {
                cutwin[i].he = 0;
                cutwin[i].hs = 0;
                cutwin[i].ve = 0;
                cutwin[i].vs = 0;
            }

            if (ret == 0) {
                SYS_LOGD("signal_fmt:0x%x, AFDFlag:%d,screen mode:%d he:%d hs:%d ve:%d vs:%d!\n", sig_fmt[i], AFDFlag, ScreenModeValue, cutwin[i].he, cutwin[i].hs, cutwin[i].ve, cutwin[i].vs);
                ve_pq_table[i].value1 = ((cutwin[i].he & 0xffff)<<16) | (cutwin[i].hs & 0xffff);
                ve_pq_table[i].value2 = ((cutwin[i].ve & 0xffff)<<16) | (cutwin[i].vs & 0xffff);
            } else {
                SYS_LOGE("PQ_GetOverscanParams failed!\n");
            }
        }
        ve_pq_load_reg.param_ptr = (long long)ve_pq_table;
    } else if ((source_input == SOURCE_TV)
        || (source_input == SOURCE_AV1)
        || (source_input == SOURCE_AV2)) {//ATV AV SOURCE
        for (i=SIG_TIMING_TYPE_NTSC_M;i<SIG_TIMING_TYPE_MAX;i++) {
            adapted_mode = 1;
            ve_pq_table[i].src_timing = (adapted_mode << 31) |
                                        (AFDFlag << 30) |
                                        ((ScreenModeValue & 0x7f) << 24) |
                                        ((source_input & 0x7f) << 16 ) |
                                        (flag[i]);
            source_input_param.sig_fmt = sig_fmt[i];

            if (mbCpqCfg_display_overscan_enable) {
                if (mbCpqCfg_separate_db_enable) {
                    ret = mpOverScandb->PQ_GetOverscanParams(source_input_param, display_mode, cutwin+i);
                }
            } else {
                SYS_LOGD("%s: Overscan module disabled!\n", __FUNCTION__);
                ret = 0;
                cutwin[i].he = 0;
                cutwin[i].hs = 0;
                cutwin[i].ve = 0;
                cutwin[i].vs = 0;
            }

            if (ret == 0) {
                SYS_LOGD("signal_fmt:0x%x, screen mode:%d hs:%d he:%d vs:%d ve:%d!\n", sig_fmt[i], ScreenModeValue, cutwin[i].he, cutwin[i].hs, cutwin[i].ve, cutwin[i].vs);
                ve_pq_table[i].value1 = ((cutwin[i].he & 0xffff)<<16) | (cutwin[i].hs & 0xffff);
                ve_pq_table[i].value2 = ((cutwin[i].ve & 0xffff)<<16) | (cutwin[i].vs & 0xffff);
            } else {
                SYS_LOGE("PQ_GetOverscanParams failed!\n");
            }
        }
        ve_pq_load_reg.param_ptr = (long long)ve_pq_table;
    } else {//HDMI && MPEG
        adapted_mode = 0;
        ve_pq_table[0].src_timing = (adapted_mode << 31) |
                                    (AFDFlag << 30) |
                                    ((ScreenModeValue & 0x7f) << 24) |
                                    ((source_input & 0x7f) << 16 ) |
                                    (0x0);
        ve_pq_table[0].value1 = 0;
        ve_pq_table[0].value2 = 0;
        ve_pq_load_reg.param_ptr = (long long)ve_pq_table;

        ret = 0;
    }

    if (ret == 0) {
        SYS_LOGD("source_input:%d, adapted_mode:%d, AFDFlag:%d,screen mode:%d\n", source_input, adapted_mode, AFDFlag, ScreenModeValue);
        ret = Cpq_LoadDisplayModeRegs(ve_pq_load_reg);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
        return -1;
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
        return 0;
    }

}

int CPQControl::Cpq_GetScreenModeValue(vpp_display_mode_t display_mode)
{
    int value = SCREEN_MODE_16_9;

    switch ( display_mode ) {
    case VPP_DISPLAY_MODE_169:
        value = SCREEN_MODE_16_9;
        break;
    case VPP_DISPLAY_MODE_MODE43:
        value = SCREEN_MODE_4_3;
        break;
    case VPP_DISPLAY_MODE_NORMAL:
        value = SCREEN_MODE_NORMAL;
        break;
    case VPP_DISPLAY_MODE_FULL:
        value = SCREEN_MODE_NONLINEAR;
        Cpq_SetNonLinearFactor(20);
        break;
    case VPP_DISPLAY_MODE_NOSCALEUP:
        value = SCREEN_MODE_NORMAL_NOSCALEUP;
        break;
    case VPP_DISPLAY_MODE_FULL_STRETCH:
        value = SCREEN_MODE_FULL_STRETCH;
        break;
    case VPP_DISPLAY_MODE_43_IGNORE:
        value = SCREEN_MODE_4_3_IGNORE;
        break;
    case VPP_DISPLAY_MODE_43_LETTER_BOX:
        value = SCREEN_MODE_4_3_LETTER_BOX;
        break;
    case VPP_DISPLAY_MODE_43_PAN_SCAN:
        value = SCREEN_MODE_4_3_PAN_SCAN;
        break;
    case VPP_DISPLAY_MODE_43_COMBINED:
        value = SCREEN_MODE_4_3_COMBINED;
        break;
    case VPP_DISPLAY_MODE_169_IGNORE:
        value = SCREEN_MODE_16_9_IGNORE;
        break;
    case VPP_DISPLAY_MODE_169_LETTER_BOX:
        value = SCREEN_MODE_16_9_LETTER_BOX;
        break;
    case VPP_DISPLAY_MODE_169_PAN_SCAN:
        value = SCREEN_MODE_16_9_PAN_SCAN;
        break;
    case VPP_DISPLAY_MODE_169_COMBINED:
        value = SCREEN_MODE_16_9_COMBINED;
        break;
    case VPP_DISPLAY_MODE_MOVIE:
    case VPP_DISPLAY_MODE_PERSON:
    case VPP_DISPLAY_MODE_CAPTION:
    case VPP_DISPLAY_MODE_CROP:
    case VPP_DISPLAY_MODE_CROP_FULL:
    case VPP_DISPLAY_MODE_ZOOM:
    default:
        value = SCREEN_MODE_FULL_STRETCH;
        break;
    }

    return value;
}

int CPQControl::Cpq_SetVideoScreenMode(int value)
{
    SYS_LOGD("%s: %d\n", __FUNCTION__, value);

    char val[64] = {0};
    sprintf(val, "%d", value);
    return pqWriteSys(VIDEO_SCREEN_MODE, val);
}

int CPQControl::Cpq_SetVideoCrop(int Voffset0, int Hoffset0, int Voffset1, int Hoffset1)
{
    SYS_LOGD("%s: %d %d %d %d\n", __FUNCTION__, Voffset0, Hoffset0, Voffset1, Hoffset1);

    char set_str[32];
    memset(set_str, 0, 32);
    sprintf(set_str, "%d %d %d %d", Voffset0, Hoffset0, Voffset1, Hoffset1);
    return pqWriteSys(VIDEO_CROP, set_str);
}

int CPQControl::Cpq_SetNonLinearFactor(int value)
{
    SYS_LOGD("%s: %d\n", __FUNCTION__, value);

    char val[64] = {0};
    sprintf(val, "%d", value);
    return pqWriteSys(VIDEO_NONLINEAR_FACTOR, val);
}

//Backlight
int CPQControl::SetBacklight(int value, int index, int is_save)
{
    int ret = -1;
    SYS_LOGD("%s: index = %d, value = %d\n", __FUNCTION__, index, value);
    if (value < 0 || value > 100) {
        value = DEFAULT_BACKLIGHT_BRIGHTNESS;
    }

    if (isFileExist(LDIM_PATH)) {//local diming
        int temp = (value * 255 / 100);
        ret = Cpq_SetBackLight(temp, index);
    }

    if (is_save == 1) {
        ret = SaveBacklight(value, index);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
        return -1;
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
        return 0;
    }

}

int CPQControl::GetBacklight(int index)
{
    int data = 0;
    mSSMAction->SSMReadBackLightVal(index*sizeof(int), &data);

    if (data < 0 || data > 100) {
        data = DEFAULT_BACKLIGHT_BRIGHTNESS;
    }

    return data;
}

int CPQControl::SaveBacklight(int value, int index)
{
    int ret = -1;
    SYS_LOGI("%s: index = %d, value = %d\n", __FUNCTION__, index, value);

    ret = mSSMAction->SSMSaveBackLightVal(index*sizeof(int), value);

    return ret;
}

int CPQControl::Cpq_SetBackLight(int value, int index)
{
    unsigned int temp = value;
    int ret = 0;
    if (index == 1)
        ret = write_backlight_value(&temp);
    else if (index == 2)
        ret = write_backlight2_value(&temp);
    else if (index == 3)
        ret = write_backlight3_value(&temp);

     if (ret == 0)
        SYS_LOGV("%s:succeed; index = %d, value = %d\n", __FUNCTION__, index, temp);
     else
        SYS_LOGV("%s:fail; index = %d, ret = %d\n", __FUNCTION__, index, ret);

     return ret;
}

void CPQControl::Cpq_GetBacklight(int *value, int index)
{
    int ret = 0;
    unsigned int temp = 0;
    if (index == 1)
        ret = read_backlight_value(&temp);
    else if (index == 2)
        ret =   read_backlight2_value(&temp);
    else if (index == 3)
        ret = read_backlight3_value(&temp);

    if (ret == 0) {
        SYS_LOGV("%s:succeed; index = %d, value = %d\n", __FUNCTION__, index, temp);
    } else {
        SYS_LOGV("%s:fail; index = %d, ret = %d\n", __FUNCTION__, index, ret);
    }

    *value = temp;
}

void CPQControl::Set_Backlight(int value)
{
    Cpq_SetBackLight(value, 1);
}

int CPQControl::read_backlight_value(unsigned int *temp)
{
    if (!temp)
        return -ENOBUFS;

    int bldev = open(VOUT_DEV, O_RDONLY);
    if (bldev < 0) {
        return -EBADFD;
    }

    if (ioctl(bldev, VOUT_IOC_CMD_GET_BL_BRIGHTNESS, (unsigned long)temp) != 0) {
        close(bldev);
        return -EINVAL;
    }

    close(bldev);
    return 0;
}

int CPQControl::read_backlight2_value(unsigned int *temp)
{
    if (!temp)
        return -ENOBUFS;

    int bldev = open(VOUT_DEV2, O_RDONLY);
    if (bldev < 0) {
        return -EBADFD;
    }

    if (ioctl(bldev, VOUT_IOC_CMD_GET_BL_BRIGHTNESS, (unsigned long)temp) != 0) {
        close(bldev);
        return -EINVAL;
    }

    close(bldev);
    return 0;
}

int CPQControl::read_backlight3_value(unsigned int *temp)
{
    if (!temp)
        return -ENOBUFS;

    int bldev = open(VOUT_DEV3, O_RDONLY);
    if (bldev < 0) {
        return -EBADFD;
    }

    if (ioctl(bldev, VOUT_IOC_CMD_GET_BL_BRIGHTNESS, (unsigned long)temp) != 0) {
        close(bldev);
        return -EINVAL;
    }

    close(bldev);
    return 0;
}

int CPQControl::write_backlight_value(unsigned int *temp)
{
    if (!temp)
        return -ENOBUFS;

    int bldev = open(VOUT_DEV, O_RDWR);
    if (bldev < 0) {
        return -EBADFD;
    }

    if (ioctl(bldev, VOUT_IOC_CMD_SET_BL_BRIGHTNESS, (unsigned long)temp) != 0) {
        close(bldev);
        return -EINVAL;
    }

    close(bldev);
    return 0;
}

int CPQControl::write_backlight2_value(unsigned int *temp)
{
    if (!temp)
        return -ENOBUFS;

    int bldev = open(VOUT_DEV2, O_RDWR);
    if (bldev < 0) {
        return -EBADFD;
    }

    if (ioctl(bldev, VOUT_IOC_CMD_SET_BL_BRIGHTNESS, (unsigned long)temp) != 0) {
        close(bldev);
        return -EINVAL;
    }

    close(bldev);
    return 0;
}

int CPQControl::write_backlight3_value(unsigned int *temp)
{
    if (!temp)
        return -ENOBUFS;

    int bldev = open(VOUT_DEV3, O_RDWR);
    if (bldev < 0) {
        return -EBADFD;
    }

    if (ioctl(bldev, VOUT_IOC_CMD_SET_BL_BRIGHTNESS, (unsigned long)temp) != 0) {
        close(bldev);
        return -EINVAL;
    }

    close(bldev);
    return 0;
}

//dynamic backlight
int CPQControl::SetDynamicBacklight(Dynamic_backlight_status_t mode, int is_save)
{
    SYS_LOGD("%s, mode = %d\n",__FUNCTION__, mode);
    int ret = -1;

    if (is_save == 1) {
        ret = mSSMAction->SSMSaveDynamicBacklightMode(mode);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::GetDynamicBacklight()
{
    int ret = -1;
    int mode = -1;
    ret = mSSMAction->SSMReadDynamicBacklightMode(&mode);
    if (0 == ret) {
        return mode;
    } else {
        return ret;
    }
    SYS_LOGD("%s: value is %d\n", __FUNCTION__, mode);
}

int CPQControl::DynamicBackLightInit(void)
{
    int ret = 0;
    Dynamic_backlight_status_t mode = (Dynamic_backlight_status_t)GetDynamicBacklight();
    ret = SetDynamicBacklight(mode, 1);

    if (!isFileExist(LDIM_PATH)) {
        if (isFileExist(pqSysWrite->getSysNode(BACKLIGHT_AML_BL_BRIGHTNESS))) {
            mDynamicBackLight = sp<CDynamicBackLight>::make();
            mDynamicBackLight->setObserver(this);
            mDynamicBackLight->startDected();
        } else {
            SYS_LOGD("No auto backlight module!\n");
        }
    }

    return ret;
}

int CPQControl::GetHistParam(ve_hist_t *hist)
{
    memset(hist, 0, sizeof(ve_hist_s));
    int ret = VPPDeviceIOCtl(AMVECM_IOC_G_HIST_AVG, hist);
    if (ret < 0) {
        //SYS_LOGE("GetAVGHistParam, error(%s)!\n", strerror(errno));
        hist->ave = -1;
    }
    return ret;
}

void CPQControl::GetDynamicBacklighConfig(int *thtf, int *lut_mode, int *height_param, int *low_param)
{
    *thtf = mPQConfigFile->GetInt(CFG_SECTION_BACKLIGHT, CFG_AUTOBACKLIGHT_THTF, 0);
    *lut_mode = mPQConfigFile->GetInt(CFG_SECTION_BACKLIGHT, CFG_AUTOBACKLIGHT_LUTMODE, 1);

    const char *buf = NULL;
    buf = mPQConfigFile->GetString(CFG_SECTION_BACKLIGHT, CFG_AUTOBACKLIGHT_LUTHIGH, NULL);
    pqTransformStringToInt(buf, height_param);

    buf = mPQConfigFile->GetString(CFG_SECTION_BACKLIGHT, CFG_AUTOBACKLIGHT_LUTLOW, NULL);
    pqTransformStringToInt(buf, low_param);
}

void CPQControl::GetDynamicBacklighParam(dynamic_backlight_Param_t *DynamicBacklightParam)
{
    int value = 0;
    ve_hist_t hist;
    memset(&hist, 0, sizeof(ve_hist_t));
    GetHistParam(&hist);
    DynamicBacklightParam->hist.ave = hist.ave;
    DynamicBacklightParam->hist.sum = hist.sum;
    DynamicBacklightParam->hist.width = hist.width;
    DynamicBacklightParam->hist.height = hist.height;

    Cpq_GetBacklight(&value, 1);
    DynamicBacklightParam->CurBacklightValue = value;
    DynamicBacklightParam->UiBackLightValue = GetBacklight(1);
    DynamicBacklightParam->CurDynamicBacklightMode = (Dynamic_backlight_status_t)GetDynamicBacklight();
    DynamicBacklightParam->VideoStatus = GetVideoPlayStatus();
}

int CPQControl::GetVideoPlayStatus(void)
{
    int curVideoState = 0;
    /*int offset = 0;
    char vframeMap[1024] = {0};
    char tmp[1024] = {0};
    char *findRet = NULL;
    char findStr1[20] = "provider";
    char findStr2[20] = "ionvideo";
    char findStr3[20] = "deinterlace(1)";
    int readRet =  pqReadSys(VFM_MAP, tmp, sizeof(tmp));
    strcpy(vframeMap, tmp);
    if (readRet > 0) {
        findRet = strstr(vframeMap, findStr1);
        if (findRet) {
            offset = findRet - vframeMap;
            memset(tmp, 0, sizeof(tmp));
            strncpy(tmp, vframeMap, offset);
            if (strstr(tmp, findStr2) || strstr(tmp, findStr3)) {
                curVideoState = 1;
            } else {
                curVideoState = 0;
            }
        }
    }*/

    if (mbVideoIsPlaying) {
        curVideoState = 1;//video playing
    } else {
        curVideoState = 0;//video stopping
    }

    //SYS_LOGD("%s: curVideoState = %d!\n",__FUNCTION__, curVideoState);
    return curVideoState;
}

int CPQControl::SetLocalContrastMode(local_contrast_mode_t mode, int is_save)
{
    SYS_LOGD("%s: mode is %d!\n",__FUNCTION__, mode);
    int ret = -1;
    if (mbCpqCfg_local_contrast_enable) {
        ret = Cpq_SetLocalContrastMode(mode);
        if ((ret == 0) && (is_save == 1)) {
            ret = SaveLocalContrastMode(mode);
        }
    } else {
        SYS_LOGD("%s: local contrast module disabled!\n",__FUNCTION__);
        ret = 0;
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }
    return ret;
}

int CPQControl::GetLocalContrastMode(void)
{
    int mode = LOCAL_CONTRAST_MODE_MID;
    if (mbCpqCfg_new_picture_mode_enable) {
        vpp_picture_mode_t pq_mode = (vpp_picture_mode_t)GetPQMode();
        vpp_pictur_mode_para_t para;
        if (GetPictureModeData(mCurrentPqSource, pq_mode, &para) == 0) {
            mode = para.LocalContrast;
        }
    } else {
        mSSMAction->SSMReadLocalContrastMode(mCurrentSourceInputInfo.source_input, &mode);
    }

    if (mode < LOCAL_CONTRAST_MODE_OFF || mode > LOCAL_CONTRAST_MODE_MAX) {
        mode = LOCAL_CONTRAST_MODE_MID;
    }

    SYS_LOGD("%s, source: %d, timming: %d, value = %d\n", __FUNCTION__, mSourceInputForSaveParam, mCurrentPqSource.pq_sig_fmt, mode);

    return mode;
}

int CPQControl::SaveLocalContrastMode(local_contrast_mode_t mode)
{
    int ret = -1;
    SYS_LOGD("%s: mode = %d\n", __FUNCTION__, mode);
    if (mbCpqCfg_new_picture_mode_enable) {
        vpp_pictur_mode_para_t para;
        vpp_picture_mode_t pq_mode = (vpp_picture_mode_t)GetPQMode();
        if (GetPictureModeData(mCurrentPqSource, pq_mode, &para) == 0) {
            para.LocalContrast = (int)mode;
            ret = SetPictureModeData(mCurrentPqSource, pq_mode, &para);
        }
    } else {
        ret = mSSMAction->SSMSaveLocalContrastMode(mCurrentSourceInputInfo.source_input, mode);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::Cpq_SetLocalContrastMode(local_contrast_mode_t mode)
{
    int ret = -1;

    if (mbDatabaseMatchChipStatus) {
        ve_lc_curve_parm_t lc_param;
        am_regs_t regs;
        memset(&lc_param, 0x0, sizeof(ve_lc_curve_parm_t));
        memset(&regs, 0x0, sizeof(am_regs_t));
        ret = mPQdb->PQ_GetLocalContrastNodeParams(mCurrentSourceInputInfo, mode, &lc_param);
        if (ret == 0 ) {
            ret = VPPDeviceIOCtl(AMVECM_IOC_S_LC_CURVE, &lc_param);
            if (ret == 0) {
                ret = mPQdb->PQ_GetLocalContrastRegParams(mCurrentSourceInputInfo, mode, &regs);
                if (ret == 0) {
                    ret = Cpq_LoadRegs(regs);
                } else {
                    SYS_LOGE("%s: PQ_GetLocalContrastRegParams failed!\n", __FUNCTION__ );
                }
            }
        } else {
            SYS_LOGE("%s: PQ_GetLocalContrastNodeParams failed!\n", __FUNCTION__ );
        }
    } else {
        SYS_LOGE("%s: pq.db don't match chip!\n", __FUNCTION__);
        ret = 0;
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::SetBlackExtensionMode(black_extension_mode_t mode, int is_save)
{
    SYS_LOGI("%s: mode is %d\n", __FUNCTION__, mode);
    int ret = -1;
    if (mbCpqCfg_new_picture_mode_enable)
    {
        ret = SetBlackStretch((int)mode, is_save);
        return ret;
    }

    SYS_LOGD("%s: mode is %d\n", __FUNCTION__, mode);
    if (mbCpqCfg_blackextension_enable) {
        ret = SetBlackExtensionParam(mCurrentSourceInputInfo);
        if ((ret == 0) && (is_save == 1)) {
            ret = SaveBlackExtensionMode(mode);
        }
    } else {
        SYS_LOGD("%s: black extension module disabled\n", __FUNCTION__);
        ret = 0;
    }

    if (ret < 0) {
        SYS_LOGE("%s failed\n", __FUNCTION__);
    } else {
        SYS_LOGD("%s success\n", __FUNCTION__);
    }
    return ret;
}

int CPQControl::GetBlackExtensionMode(void)
{
    int ret = -1;
    int mode = BLACK_EXTENSION_MODE_OFF;

    if (mbCpqCfg_new_picture_mode_enable)
    {
        mode = GetBlackStretch();
        return mode;
    }

    ret = mSSMAction->SSMReadBlackExtensionMode(mCurrentSourceInputInfo.source_input, &mode);
    if (0 == ret) {
        SYS_LOGD("%s: mode is %d\n", __FUNCTION__, mode);
    } else {
        SYS_LOGE("%s failed\n", __FUNCTION__);
    }

    return mode;
}

int CPQControl::SaveBlackExtensionMode(black_extension_mode_t mode)
{
    int ret = -1;
    SYS_LOGD("%s: mode = %d\n", __FUNCTION__, mode);

    ret = mSSMAction->SSMSaveBlackExtensionMode(mCurrentSourceInputInfo.source_input, mode);
    if (ret < 0) {
        SYS_LOGE("%s failed\n", __FUNCTION__);
    } else {
        SYS_LOGD("%s success\n", __FUNCTION__);
    }

    return ret;
}

int CPQControl::SetBlackExtensionParam(source_input_param_t source_input_param)
{
    am_regs_t regs;
    memset(&regs, 0, sizeof(am_regs_t));
    int ret = -1;

    ret = mPQdb->PQ_GetBlackExtensionParams(source_input_param, &regs);
    if (ret < 0) {
        SYS_LOGE("%s: PQ_GetBlackExtensionParams failed!\n", __FUNCTION__);
    } else {
        ret = Cpq_LoadRegs(regs);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n", __FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n", __FUNCTION__);
    }

    return ret;
}

int CPQControl::SetMpegNr(vpp_pq_level_t mode, int is_save)
{
    SYS_LOGD("%s: mode is %d\n", __FUNCTION__, mode);
    int ret = -1;

    ret = Cpq_SetMpegNr(mode, mCurrentSourceInputInfo);

    if ((ret == 0) && (is_save == 1)) {
        ret = SaveMpegNr(mode);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed\n", __FUNCTION__);
    } else {
        SYS_LOGD("%s success\n", __FUNCTION__);
    }
    return ret;
}

int CPQControl::GetMpegNr(void)
{
    int mode = VPP_PQ_LV_OFF;
    if (mbCpqCfg_new_picture_mode_enable) {
        vpp_picture_mode_t pq_mode = (vpp_picture_mode_t)GetPQMode();
        vpp_pictur_mode_para_t para;
        if (GetPictureModeData(mCurrentPqSource, pq_mode, &para) == 0) {
            mode = para.MpegNr;
        }
    } else {
        mSSMAction->SSMReadMpegNoiseReduction(mCurrentSourceInputInfo.source_input, &mode);
    }

    SYS_LOGD("%s: MpegNr = %d \n", __FUNCTION__, mode);
    return mode;
}

int CPQControl::SaveMpegNr(vpp_pq_level_t mode)
{
    int ret = -1;
    if (mbCpqCfg_new_picture_mode_enable) {
        vpp_pictur_mode_para_t para;
        vpp_picture_mode_t pq_mode = (vpp_picture_mode_t)GetPQMode();
        if (GetPictureModeData(mCurrentPqSource, pq_mode, &para) == 0) {
            para.MpegNr = (int)mode;
            ret = SetPictureModeData(mCurrentPqSource, pq_mode, &para);
        }
    } else {
        ret = mSSMAction->SSMSaveMpegNoiseReduction(mCurrentSourceInputInfo.source_input, mode);
    }

    if (ret < 0)
        SYS_LOGE("%s failed\n", __FUNCTION__);

    return ret;
}

int CPQControl::Cpq_SetMpegNr(vpp_pq_level_t mode, source_input_param_t source_input_param)
{
    int ret = -1;

    ret = Cpq_SetDeblockMode((di_deblock_mode_t) mode, source_input_param);
    ret |= Cpq_SetDemoSquitoMode((di_demosquito_mode_t) mode, source_input_param);

    if (ret < 0)
        SYS_LOGE("%s failed!\n",__FUNCTION__);

    return ret;
}

int CPQControl::SetDeblockMode(di_deblock_mode_t mode, int is_save)
{
    SYS_LOGD("%s: mode is %d\n", __FUNCTION__, mode);
    int ret = -1;
    if (mbCpqCfg_deblock_enable) {
        ret = Cpq_SetDeblockMode(mode, mCurrentSourceInputInfo);
        if ((ret == 0) && (is_save == 1)) {
            ret = SaveDeblockMode(mode);
        }
    } else {
        SYS_LOGD("%s: deblock disabled\n", __FUNCTION__);
        ret = 0;
    }

    if (ret < 0) {
        SYS_LOGE("%s failed\n", __FUNCTION__);
    } else {
        SYS_LOGD("%s success\n", __FUNCTION__);
    }
    return ret;
}

int CPQControl::GetDeblockMode(void)
{
    int ret = -1;
    int mode = DI_DEBLOCK_MODE_OFF;

    if (mbCpqCfg_new_picture_mode_enable) {
        vpp_picture_mode_t pq_mode = (vpp_picture_mode_t)GetPQMode();
        vpp_pictur_mode_para_t para;
        if (GetPictureModeData(mCurrentPqSource, pq_mode, &para) == 0) {
            mode = para.Deblock;
        }
    } else {
        ret = mSSMAction->SSMReadDeblockMode(mCurrentSourceInputInfo.source_input, &mode);
    }

    if (0 == ret) {
        SYS_LOGD("%s: mode is %d\n", __FUNCTION__, mode);
    } else {
        SYS_LOGE("%s failed\n", __FUNCTION__);
    }

    return mode;
}

int CPQControl::SaveDeblockMode(di_deblock_mode_t mode)
{
    int ret = -1;
    SYS_LOGD("%s: mode = %d\n", __FUNCTION__, mode);

    if (mbCpqCfg_new_picture_mode_enable) {
        vpp_pictur_mode_para_t para;
        vpp_picture_mode_t pq_mode = (vpp_picture_mode_t)GetPQMode();
        if (GetPictureModeData(mCurrentPqSource, pq_mode, &para) == 0) {
            para.Deblock = (int)mode;
            ret = SetPictureModeData(mCurrentPqSource, pq_mode, &para);
        }
    } else {
        ret = mSSMAction->SSMSaveDeblockMode(mCurrentSourceInputInfo.source_input, mode);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed\n", __FUNCTION__);
    } else {
        SYS_LOGD("%s success\n", __FUNCTION__);
    }

    return ret;
}

int CPQControl::Cpq_SetDeblockMode(di_deblock_mode_t deblock_mode, source_input_param_t source_input_param)
{
    int ret = -1;
    am_regs_t regs;
    am_pq_param_t di_regs;
    memset(&regs, 0x0, sizeof(am_regs_t));
    memset(&di_regs, 0x0,sizeof(am_pq_param_t));

    if (mbCpqCfg_deblock_enable) {
        if (mPQdb->PQ_GetDeblockParams((di_deblock_mode_t)deblock_mode, source_input_param, &regs) == 0) {
            di_regs.table_name = TABLE_NAME_DEBLOCK;
            di_regs.table_len = regs.length;
            am_reg_t tmp_buf[regs.length];
            for (unsigned int i=0;i<regs.length;i++) {
                  tmp_buf[i].addr = regs.am_reg[i].addr;
                  tmp_buf[i].mask = regs.am_reg[i].mask;
                  tmp_buf[i].type = regs.am_reg[i].type;
                  tmp_buf[i].val  = regs.am_reg[i].val;
            }
            di_regs.table_ptr = (long long)tmp_buf;

            ret = DI_LoadRegs(di_regs);
        } else {
            SYS_LOGE("PQ_GetDeblockParams failed!\n");
        }
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}


int CPQControl::SetDemoSquitoMode(di_demosquito_mode_t mode, int is_save)
{
    SYS_LOGD("%s: mode is %d\n", __FUNCTION__, mode);
    int ret = -1;
    if (mbCpqCfg_demoSquito_enable) {
        ret = Cpq_SetDemoSquitoMode(mode, mCurrentSourceInputInfo);
        if ((ret == 0) && (is_save == 1)) {
            ret = SaveDemoSquitoMode(mode);
        }
    } else {
        SYS_LOGD("%s: demosquito disabled\n", __FUNCTION__);
        ret = 0;
    }

    if (ret < 0) {
        SYS_LOGE("%s failed\n", __FUNCTION__);
    } else {
        SYS_LOGD("%s success\n", __FUNCTION__);
    }
    return ret;
}

int CPQControl::GetDemoSquitoMode(void)
{
    int ret = -1;
    int mode = DI_DEMOSQUITO_MODE_OFF;

    if (mbCpqCfg_new_picture_mode_enable) {
        vpp_picture_mode_t pq_mode = (vpp_picture_mode_t)GetPQMode();
        vpp_pictur_mode_para_t para;
        if (GetPictureModeData(mCurrentPqSource, pq_mode, &para) == 0) {
            mode = para.Demosquito;
        }
    } else {
        ret = mSSMAction->SSMReadDemoSquitoMode(mCurrentSourceInputInfo.source_input, &mode);
    }

    if (0 == ret) {
        SYS_LOGD("%s: mode is %d\n", __FUNCTION__, mode);
    } else {
        SYS_LOGE("%s failed\n", __FUNCTION__);
    }

    return mode;
}

int CPQControl::SaveDemoSquitoMode(di_demosquito_mode_t mode)
{
    int ret = -1;
    SYS_LOGD("%s: mode = %d\n", __FUNCTION__, mode);

    if (mbCpqCfg_new_picture_mode_enable) {
        vpp_pictur_mode_para_t para;
        vpp_picture_mode_t pq_mode = (vpp_picture_mode_t)GetPQMode();
        if (GetPictureModeData(mCurrentPqSource, pq_mode, &para) == 0) {
            para.Demosquito = (int)mode;
            ret = SetPictureModeData(mCurrentPqSource, pq_mode, &para);
        }
    } else {
        ret = mSSMAction->SSMSaveDemoSquitoMode(mCurrentSourceInputInfo.source_input, mode);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed\n", __FUNCTION__);
    } else {
        SYS_LOGD("%s success\n", __FUNCTION__);
    }

    return ret;
}

int CPQControl::Cpq_SetDemoSquitoMode(di_demosquito_mode_t DeMosquito_mode, source_input_param_t source_input_param)
{
    int ret = -1;
    am_regs_t regs;
    am_pq_param_t di_regs;
    memset(&regs, 0x0, sizeof(am_regs_t));
    memset(&di_regs, 0x0,sizeof(am_pq_param_t));

    if (mbCpqCfg_demoSquito_enable) {
        if (mPQdb->PQ_GetDemoSquitoParams(DeMosquito_mode, source_input_param, &regs) == 0) {
            di_regs.table_name = TABLE_NAME_DEMOSQUITO;
            di_regs.table_len = regs.length;
            am_reg_t tmp_buf[regs.length];
            for (unsigned int i=0;i<regs.length;i++) {
                  tmp_buf[i].addr = regs.am_reg[i].addr;
                  tmp_buf[i].mask = regs.am_reg[i].mask;
                  tmp_buf[i].type = regs.am_reg[i].type;
                  tmp_buf[i].val  = regs.am_reg[i].val;
            }
            di_regs.table_ptr = (long long)tmp_buf;

            ret = DI_LoadRegs(di_regs);
        } else {
            SYS_LOGE("DemoSquitoMode failed!\n");
        }
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::SetMcDiMode(vpp_mcdi_mode_e mode, int is_save)
{
    SYS_LOGD("%s: mode is %d\n", __FUNCTION__, mode);
    int ret = -1;
    if (mbCpqCfg_mcdi_enable) {
        ret = Cpq_SetMcDiMode(mode , mCurrentSourceInputInfo);
        if ((ret == 0) && (is_save == 1)) {
            ret = SaveMcDiMode(mode);
        }
    } else {
        SYS_LOGD("%s: McDi disabled\n", __FUNCTION__);
        ret = 0;
    }

    if (ret < 0) {
        SYS_LOGE("%s failed\n", __FUNCTION__);
    } else {
        SYS_LOGD("%s success\n", __FUNCTION__);
    }
    return ret;
}

int CPQControl::GetMcDiMode(void)
{
    int mode = VPP_MCDI_MODE_OFF;
    int ret = mSSMAction->SSMReadMcDiMode(mCurrentSourceInputInfo.source_input, &mode);
    if (0 == ret) {
        SYS_LOGD("%s: mode is %d\n", __FUNCTION__, mode);
    } else {
        SYS_LOGE("%s failed\n", __FUNCTION__);
    }

    return mode;
}

int CPQControl::SaveMcDiMode(vpp_mcdi_mode_e mode)
{
    int ret = -1;
    SYS_LOGD("%s: mode = %d\n", __FUNCTION__, mode);

    ret = mSSMAction->SSMSaveMcDiMode(mCurrentSourceInputInfo.source_input, mode);
    if (ret < 0) {
        SYS_LOGE("%s failed\n", __FUNCTION__);
    } else {
        SYS_LOGD("%s success\n", __FUNCTION__);
    }

    return ret;
}

int CPQControl::Cpq_SetMcDiMode(vpp_mcdi_mode_e McDi_mode, source_input_param_t source_input_param)
{
    int ret = -1;
    am_regs_t regs;
    am_pq_param_t di_regs;
    memset(&regs, 0x0, sizeof(am_regs_t));
    memset(&di_regs, 0x0,sizeof(am_pq_param_t));

    if (mbCpqCfg_mcdi_enable) {
        if (mPQdb->PQ_GetMCDIParams(McDi_mode, source_input_param, &regs) == 0) {
            di_regs.table_name = TABLE_NAME_MCDI;
            di_regs.table_len = regs.length;
            am_reg_t tmp_buf[regs.length];
            for (unsigned int i=0;i<regs.length;i++) {
                  tmp_buf[i].addr = regs.am_reg[i].addr;
                  tmp_buf[i].mask = regs.am_reg[i].mask;
                  tmp_buf[i].type = regs.am_reg[i].type;
                  tmp_buf[i].val  = regs.am_reg[i].val;
            }
            di_regs.table_ptr = (long long)tmp_buf;

            ret = DI_LoadRegs(di_regs);
        } else {
            SYS_LOGE("MCDI failed!\n");
        }
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

//static frame
int CPQControl::SetStaticFrameEnable(int enable, int isSave)
{
    SYS_LOGD("%s: StaticFrameEnable status is %d.\n", __FUNCTION__, enable);
    int ret = -1;
    if (enable == 1) {
        //ret = pqWriteSys(VIDEO_BLACKOUT_POLICY, "0");
        //enable static frame output
        ret = property_set(STATIC_FRAME_ENABLE_PROP, "1");
    } else {
        //ret = pqWriteSys(VIDEO_BLACKOUT_POLICY, "1");
        //disable static frame output
        ret = property_set(STATIC_FRAME_ENABLE_PROP, "0");
    }

    //if ((ret == 0) && (isSave == 1)) {
    if (isSave == 1) {
        Cpq_SSMWriteNTypes(SSM_RW_BLACKOUT_ENABLE_START, 1, enable, 0);
    }

    return 0;
}

int CPQControl::GetStaticFrameEnable()
{
    int ret = -1;
    int value = Cpq_SSMReadNTypes(SSM_RW_BLACKOUT_ENABLE_START, 1, 0);
    if (value < 0) {
        SYS_LOGE("%s failed.\n", __FUNCTION__);
        ret = 0;
    } else {
        ret = value;
    }
    SYS_LOGD("%s: StaticFrameEnable status is %d.\n", __FUNCTION__, ret);
    return ret;
}

int CPQControl::getSnowStatus()
{
    int ret = -1;
    char buf[8] = {0};

    ret = pqReadSys(VDIN_SNOW_FLAG, buf, sizeof(buf));
    if (ret > 0) {
        ret = strtol(buf, NULL, 10);
    } else {
        ret = 0;
    }

    return ret;
}

//screen color
int CPQControl::SetScreenColorForSignalChange(int screenColor, int isSave)
{
    SYS_LOGD("%s: screenColor = %s\n", __FUNCTION__, (screenColor==0)?"black":"blue");

    if (getSnowStatus() == 1) {
        if (screenColor == VIDEO_LAYER_COLOR_BLUE) {
            setVideoScreenColor(screenColor);
        } else {
            setVideoScreenColor(3);
        }
    } else if (screenColorEnable) {
        setVideoScreenColor(screenColor);
    }
    /*
    if (screenColor == 0) {//black screen
        SetVideoLayerColor(VIDEO_LAYER_COLOR_BLACK, VIDEO_LAYER_COLOR_BLACK);
    } else {//blue screen
        SetVideoLayerColor(VIDEO_LAYER_COLOR_BLACK, VIDEO_LAYER_COLOR_BLUE);
    }
    */
    if (isSave == 1) {
        Cpq_SSMWriteNTypes(CUSTOMER_DATA_POS_SCREEN_COLOR_START, 1, screenColor, 0);
    }

    return 0;
}

int CPQControl::GetScreenColorForSignalChange()
{
    int ret = -1;
    int value = Cpq_SSMReadNTypes(CUSTOMER_DATA_POS_SCREEN_COLOR_START, 1, 0);
    if (value < 0) {
        SYS_LOGE("%s failed.\n", __FUNCTION__);
        ret = 0;
    } else {
        ret = value;
    }
    SYS_LOGD("%s: status is %d.\n", __FUNCTION__, ret);
    return ret;
}

int CPQControl::SetVideoLayerColor(video_layer_color_t signalColor, video_layer_color_t nosignalColor)
{
    int ret = 0;
    char val[64] = {0};
    int value_y=0, value_u=0, value_v=0;
    switch (nosignalColor) {
        case VIDEO_LAYER_COLOR_BLUE:
            value_y = 41;
            value_u = 240;
            value_v = 110;
            break;
        case VIDEO_LAYER_COLOR_BLACK:
            value_y = 16;
            value_u = 128;
            value_v = 128;
            break;
        default:
            value_y = 16;
            value_u = 128;
            value_v = 128;
            break;
    }
    unsigned long signalColorValue = (1 << 24) | (16 << 16 ) | (128 << 8) | (128);//default black
    unsigned long nosignalColorValue = 1 << 24;
    nosignalColorValue |= (unsigned int)(value_y << 16) | (unsigned int) (value_u << 8) | (unsigned int)value_v;
    sprintf(val, "0x%lx 0x%lx", signalColorValue, nosignalColorValue);
    ret = pqWriteSys(VIDEO_BACKGROUND_COLOR, val);
    return ret;
}

int CPQControl::setVideoScreenColor (int vdin_blending_mask, int y, int u, int v)
{
    int ret = 0;
    unsigned long value = vdin_blending_mask << 24;
    value |= ( unsigned int ) ( y << 16 ) | ( unsigned int ) ( u << 8 ) | ( unsigned int ) ( v );

    char val[64] = {0};
    sprintf(val, "0x%lx", ( unsigned long ) value);
    ret = pqWriteSys(VIDEO_TEST_SCREEN, val);
    return ret;
}

int CPQControl::setVideoScreenColor (int color)
{
    SYS_LOGD("%s:  %d\n", __FUNCTION__, color);
    int ret = 0;
    switch (color) {
        case VIDEO_LAYER_COLOR_BLUE:
            pqWriteSys(VIDEO_DISABLE_VIDEO, "1");
            ret = setVideoScreenColor(0, 41, 240, 110);
            screenColorEnable = true;
            break;
        case VIDEO_LAYER_COLOR_BLACK:
            pqWriteSys(VIDEO_DISABLE_VIDEO, "1");
            ret = setVideoScreenColor(0, 16, 128, 128);
            screenColorEnable = true;
            break;
        default:
            ret = setVideoScreenColor(0, 16, 128, 128);
            pqWriteSys(VIDEO_DISABLE_VIDEO, "2");
            screenColorEnable = false;
            break;
    }
    return ret;
}

int CPQControl::setVideoScreenColorByVT( int window, int Color, int frequency)
{

    return SetVideotunnelSolidColor((video__color_Window)window, (video_color_frame)Color, (video_color_frame_time)frequency);
}

int CPQControl::OpenVideotunnel()
{
    int ret = -1;
    ret = meson_vt_open();
    if (ret < 0) {
        SYS_LOGE("%s: open meson_vt error!",__FUNCTION__);
        return ret;
    }
    mVideoTunelFd = ret;
    return mVideoTunelFd;
}

int CPQControl::CloseVideotunnel()
{
    int ret = -1;
    if (mVideoTunelFd >= 0) {
        ret = meson_vt_close(mVideoTunelFd);
        mVideoTunelFd = -1;
    } else {
        SYS_LOGD("%s: needn't close meson_vt!",__FUNCTION__);
    }
    return ret;
}

int CPQControl::SetVideotunnelSolidColor(video__color_Window window, video_color_frame cmd, video_color_frame_time cmd_data)
{
    int ret = -1;
    if (mVideoTunelFd < 0) {
        SYS_LOGD("%s: Video tunnel not yet opened!",__FUNCTION__);
        return ret;
    }
    SYS_LOGD("%s: window:%d, color:%d, times:%d", __FUNCTION__, window, cmd,cmd_data);
    ret = meson_vt_set_solid_color(mVideoTunelFd, window, (vt_color_cmd)cmd, (vt_color_data)cmd_data);
    return ret;
}

tvin_cutwin_t CPQControl::GetOverscanParams(vpp_display_mode_t display_mode)
{
    int ret = -1;
    tvin_cutwin_t cutwin_t;
    memset(&cutwin_t, 0, sizeof(cutwin_t));

    SYS_LOGD("%s:display_mode=%d source=%d,sigFmt=%d(0x%x)\n", __FUNCTION__,
                                                                 display_mode,
                                                                 mCurrentSourceInputInfo.source_input,
                                                                 mCurrentSourceInputInfo.sig_fmt,
                                                                 mCurrentSourceInputInfo.sig_fmt);

    if (mbCpqCfg_separate_db_enable) {
        ret = mpOverScandb->PQ_GetOverscanParams(mCurrentSourceInputInfo, display_mode, &cutwin_t);
    }

    if (ret != 0) {
        SYS_LOGE("%s failed\n", __FUNCTION__);
    } else {
        SYS_LOGD("%s success\n", __FUNCTION__);
    }

    SYS_LOGD("he:%d hs:%d ve:%d vs:%d\n", cutwin_t.he, cutwin_t.hs, cutwin_t.ve, cutwin_t.vs);

    return cutwin_t;
}

//PQ Factory
int CPQControl::FactoryResetPQMode(void)
{
    if (mbCpqCfg_separate_db_enable) {
        mpOverScandb->PQ_ResetAllPQModeParams();
    } else {
        mPQdb->PQ_ResetAllPQModeParams();
    }
    return 0;
}

int CPQControl::FactoryResetColorTemp(void)
{
    mPQdb->PQ_ResetAllColorTemperatureParams();
    return 0;
}

int CPQControl::FactorySetPQMode_Brightness(source_input_param_t source_input_param, int pq_mode, int brightness)
{
    int ret = -1;
    vpp_pq_para_t pq_para;
    if (mbCpqCfg_separate_db_enable) {
        if (mpOverScandb->PQ_GetPQModeParams(source_input_param.source_input, (vpp_picture_mode_t) pq_mode, &pq_para) == 0) {
            pq_para.brightness = brightness;
            if (mpOverScandb->PQ_SetPQModeParams(source_input_param.source_input, (vpp_picture_mode_t) pq_mode, &pq_para) == 0) {
                ret = 0;
            } else {
                ret = 1;
            }
        } else {
            ret = -1;
        }
    } else {
        if (mPQdb->PQ_GetPQModeParams(source_input_param.source_input, (vpp_picture_mode_t) pq_mode, &pq_para) == 0) {
            pq_para.brightness = brightness;
            if (mPQdb->PQ_SetPQModeParams(source_input_param.source_input, (vpp_picture_mode_t) pq_mode, &pq_para) == 0) {
                ret = 0;
            } else {
                ret = 1;
            }
        } else {
            ret = -1;
        }
    }

    return ret;
}

int CPQControl::FactoryGetPQMode_Brightness(source_input_param_t source_input_param, int pq_mode)
{
    vpp_pq_para_t pq_para;
    if (mbCpqCfg_separate_db_enable) {
        if (mpOverScandb->PQ_GetPQModeParams(source_input_param.source_input, (vpp_picture_mode_t) pq_mode, &pq_para) != 0) {
            pq_para.brightness = -1;
        }
    } else {
        if (mPQdb->PQ_GetPQModeParams(source_input_param.source_input, (vpp_picture_mode_t) pq_mode, &pq_para) != 0) {
            pq_para.brightness = -1;
        }
    }
    return pq_para.brightness;
}

int CPQControl::FactorySetPQMode_Contrast(source_input_param_t source_input_param, int pq_mode, int contrast)
{
    int ret = -1;
    vpp_pq_para_t pq_para;
    if (mbCpqCfg_separate_db_enable) {
        if (mpOverScandb->PQ_GetPQModeParams(source_input_param.source_input, (vpp_picture_mode_t) pq_mode, &pq_para) == 0) {
            pq_para.contrast = contrast;
            if (mpOverScandb->PQ_SetPQModeParams(source_input_param.source_input, (vpp_picture_mode_t) pq_mode, &pq_para) == 0) {
                ret = 0;
            } else {
                ret = 1;
            }
        } else {
            ret = -1;
        }
    } else {
        if (mPQdb->PQ_GetPQModeParams(source_input_param.source_input, (vpp_picture_mode_t) pq_mode, &pq_para) == 0) {
            pq_para.contrast = contrast;
            if (mPQdb->PQ_SetPQModeParams(source_input_param.source_input, (vpp_picture_mode_t) pq_mode, &pq_para) == 0) {
                ret = 0;
            } else {
                ret = 1;
            }
        } else {
            ret = -1;
        }
    }

    return ret;
}

int CPQControl::FactoryGetPQMode_Contrast(source_input_param_t source_input_param, int pq_mode)
{
    vpp_pq_para_t pq_para;
    if (mbCpqCfg_separate_db_enable) {
        if (mpOverScandb->PQ_GetPQModeParams(source_input_param.source_input, (vpp_picture_mode_t) pq_mode, &pq_para) != 0) {
            pq_para.contrast = -1;
        }
    } else {
        if (mPQdb->PQ_GetPQModeParams(source_input_param.source_input, (vpp_picture_mode_t) pq_mode, &pq_para) != 0) {
            pq_para.contrast = -1;
        }
    }

    return pq_para.contrast;
}

int CPQControl::FactorySetPQMode_Saturation(source_input_param_t source_input_param, int pq_mode, int saturation)
{
    int ret = -1;
    vpp_pq_para_t pq_para;
    if (mbCpqCfg_separate_db_enable) {
        if (mpOverScandb->PQ_GetPQModeParams(source_input_param.source_input, (vpp_picture_mode_t) pq_mode, &pq_para) == 0) {
            pq_para.saturation = saturation;
            if (mpOverScandb->PQ_SetPQModeParams(source_input_param.source_input, (vpp_picture_mode_t) pq_mode, &pq_para) == 0) {
                ret = 0;
            } else {
                ret = 1;
            }
        } else {
            ret = -1;
        }
    } else {
        if (mPQdb->PQ_GetPQModeParams(source_input_param.source_input, (vpp_picture_mode_t) pq_mode, &pq_para) == 0) {
            pq_para.saturation = saturation;
            if (mPQdb->PQ_SetPQModeParams(source_input_param.source_input, (vpp_picture_mode_t) pq_mode, &pq_para) == 0) {
                ret = 0;
            } else {
                ret = 1;
            }
        } else {
            ret = -1;
        }
    }

    return ret;
}

int CPQControl::FactoryGetPQMode_Saturation(source_input_param_t source_input_param, int pq_mode)
{
    vpp_pq_para_t pq_para;
    if (mbCpqCfg_separate_db_enable) {
        if (mpOverScandb->PQ_GetPQModeParams(source_input_param.source_input, (vpp_picture_mode_t) pq_mode, &pq_para) != 0) {
            pq_para.saturation = -1;
        }
    } else {
        if (mPQdb->PQ_GetPQModeParams(source_input_param.source_input, (vpp_picture_mode_t) pq_mode, &pq_para) != 0) {
            pq_para.saturation = -1;
        }
    }

    return pq_para.saturation;
}

int CPQControl::FactorySetPQMode_Hue(source_input_param_t source_input_param, int pq_mode, int hue)
{
    int ret = -1;
    vpp_pq_para_t pq_para;
    if (mbCpqCfg_separate_db_enable) {
        if (mpOverScandb->PQ_GetPQModeParams(source_input_param.source_input, (vpp_picture_mode_t) pq_mode, &pq_para) == 0) {
            pq_para.hue = hue;
            if (mpOverScandb->PQ_SetPQModeParams(source_input_param.source_input, (vpp_picture_mode_t) pq_mode, &pq_para) == 0) {
                ret = 0;
            } else {
                ret = 1;
            }
        } else {
            ret = -1;
        }
    } else {
        if (mPQdb->PQ_GetPQModeParams(source_input_param.source_input, (vpp_picture_mode_t) pq_mode, &pq_para) == 0) {
            pq_para.hue = hue;
            if (mPQdb->PQ_SetPQModeParams(source_input_param.source_input, (vpp_picture_mode_t) pq_mode, &pq_para) == 0) {
                ret = 0;
            } else {
                ret = 1;
            }
        } else {
            ret = -1;
        }
    }

    return ret;
}

int CPQControl::FactoryGetPQMode_Hue(source_input_param_t source_input_param, int pq_mode)
{
    vpp_pq_para_t pq_para;
    if (mbCpqCfg_separate_db_enable) {
        if (mpOverScandb->PQ_GetPQModeParams(source_input_param.source_input, (vpp_picture_mode_t) pq_mode, &pq_para) != 0) {
            pq_para.hue = -1;
        }
    } else {
        if (mPQdb->PQ_GetPQModeParams(source_input_param.source_input, (vpp_picture_mode_t) pq_mode, &pq_para) != 0) {
            pq_para.hue = -1;
        }
    }

    return pq_para.hue;
}

int CPQControl::FactorySetPQMode_Sharpness(source_input_param_t source_input_param, int pq_mode, int sharpness)
{
    int ret = -1;
    vpp_pq_para_t pq_para;
    if (mbCpqCfg_separate_db_enable) {
        if (mpOverScandb->PQ_GetPQModeParams(source_input_param.source_input, (vpp_picture_mode_t) pq_mode, &pq_para) == 0) {
            pq_para.sharpness = sharpness;
            if (mpOverScandb->PQ_SetPQModeParams(source_input_param.source_input, (vpp_picture_mode_t) pq_mode, &pq_para) == 0) {
                ret = 0;
            } else {
                ret = 1;
            }
        } else {
            ret = -1;
        }
    } else {
        if (mPQdb->PQ_GetPQModeParams(source_input_param.source_input, (vpp_picture_mode_t) pq_mode, &pq_para) == 0) {
            pq_para.sharpness = sharpness;
            if (mPQdb->PQ_SetPQModeParams(source_input_param.source_input, (vpp_picture_mode_t) pq_mode, &pq_para) == 0) {
                ret = 0;
            } else {
                ret = 1;
            }
        } else {
            ret = -1;
        }
    }
    return ret;
}

int CPQControl::FactoryGetPQMode_Sharpness(source_input_param_t source_input_param, int pq_mode)
{
    vpp_pq_para_t pq_para;
    if (mbCpqCfg_separate_db_enable) {
        if (mpOverScandb->PQ_GetPQModeParams(source_input_param.source_input,
                                         (vpp_picture_mode_t) pq_mode, &pq_para) != 0) {
            pq_para.sharpness = -1;
        }
    } else {
        if (mPQdb->PQ_GetPQModeParams(source_input_param.source_input, (vpp_picture_mode_t) pq_mode, &pq_para) != 0) {
             pq_para.sharpness = -1;
        }
    }
    return pq_para.sharpness;
}

int CPQControl::FactorySetColorTemp_Rgain(int source_input,int colortemp_mode, int rgain)
{
    tcon_rgb_ogo_t rgbogo;
    memset (&rgbogo, 0, sizeof (rgbogo));
    GetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, &rgbogo);
    rgbogo.r_gain = rgain;
    SYS_LOGD("%s, source[%d], colortemp_mode[%d], rgain[%d].", __FUNCTION__, source_input,
         colortemp_mode, rgain);
    rgbogo.en = 1;

    if (Cpq_SetRGBOGO(&rgbogo) == 0) {
        return 0;
    }

    return -1;
}

int CPQControl::FactorySaveColorTemp_Rgain(int source_input __unused, int colortemp_mode, int rgain)
{
    tcon_rgb_ogo_t rgbogo;
    memset (&rgbogo, 0, sizeof (rgbogo));
    if (0 == GetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, &rgbogo)) {
        rgbogo.r_gain = rgain;
        return SetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, rgbogo);
    } else {
        SYS_LOGE("FactorySaveColorTemp_Rgain error!\n");
        return -1;
    }
}

int CPQControl::FactoryGetColorTemp_Rgain(int source_input __unused, int colortemp_mode)
{
    tcon_rgb_ogo_t rgbogo;
    memset (&rgbogo, 0, sizeof (rgbogo));
    if (0 == GetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, &rgbogo)) {
        return rgbogo.r_gain;
    }

    SYS_LOGE("FactoryGetColorTemp_Rgain error!\n");
    return -1;
}

int CPQControl::FactorySetColorTemp_Ggain(int source_input, int colortemp_mode, int ggain)
{
    tcon_rgb_ogo_t rgbogo;
    memset (&rgbogo, 0, sizeof (rgbogo));
    GetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, &rgbogo);
    rgbogo.g_gain = ggain;
    SYS_LOGD("%s, source[%d], colortemp_mode[%d], ggain[%d].", __FUNCTION__, source_input,
         colortemp_mode, ggain);
    rgbogo.en = 1;

    if (Cpq_SetRGBOGO(&rgbogo) == 0) {
        return 0;
    }

    return -1;
}

int CPQControl::FactorySaveColorTemp_Ggain(int source_input __unused, int colortemp_mode, int ggain)
{
    tcon_rgb_ogo_t rgbogo;
    memset (&rgbogo, 0, sizeof (rgbogo));
    if (0 == GetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, &rgbogo)) {
        rgbogo.g_gain = ggain;
        return SetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, rgbogo);
    } else {
        SYS_LOGE("FactorySaveColorTemp_Ggain error!\n");
        return -1;
    }
}

int CPQControl::FactoryGetColorTemp_Ggain(int source_input __unused, int colortemp_mode)
{
    tcon_rgb_ogo_t rgbogo;
    memset (&rgbogo, 0, sizeof (rgbogo));
    if (0 == GetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, &rgbogo)) {
        return rgbogo.g_gain;
    }

    SYS_LOGE("FactoryGetColorTemp_Ggain error!\n");
    return -1;
}

int CPQControl::FactorySetColorTemp_Bgain(int source_input, int colortemp_mode, int bgain)
{
    tcon_rgb_ogo_t rgbogo;
    memset (&rgbogo, 0, sizeof (rgbogo));
    GetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, &rgbogo);
    rgbogo.b_gain = bgain;
    SYS_LOGD("%s, source[%d], colortemp_mode[%d], bgain[%d].", __FUNCTION__, source_input,
         colortemp_mode, bgain);
    rgbogo.en = 1;

    if (Cpq_SetRGBOGO(&rgbogo) == 0) {
        return 0;
    }

    return -1;
}

int CPQControl::FactorySaveColorTemp_Bgain(int source_input __unused, int colortemp_mode, int bgain)
{
    tcon_rgb_ogo_t rgbogo;
    memset (&rgbogo, 0, sizeof (rgbogo));
    if (0 == GetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, &rgbogo)) {
        rgbogo.b_gain = bgain;
        return SetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, rgbogo);
    } else {
        SYS_LOGE("FactorySaveColorTemp_Bgain error!\n");
        return -1;
    }
}

int CPQControl::FactoryGetColorTemp_Bgain(int source_input __unused, int colortemp_mode)
{
    tcon_rgb_ogo_t rgbogo;
    memset (&rgbogo, 0, sizeof (rgbogo));
    if (0 == GetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, &rgbogo)) {
        return rgbogo.b_gain;
    }

    SYS_LOGE("FactoryGetColorTemp_Bgain error!\n");
    return -1;
}

int CPQControl::FactorySetColorTemp_Roffset(int source_input, int colortemp_mode, int roffset)
{
    tcon_rgb_ogo_t rgbogo;
    memset (&rgbogo, 0, sizeof (rgbogo));
    GetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, &rgbogo);
    rgbogo.r_post_offset = roffset;
    SYS_LOGD("%s, source[%d], colortemp_mode[%d], r_post_offset[%d].", __FUNCTION__, source_input,
         colortemp_mode, roffset);
    rgbogo.en = 1;

    if (Cpq_SetRGBOGO(&rgbogo) == 0) {
        return 0;
    }

    return -1;
}

int CPQControl::FactorySaveColorTemp_Roffset(int source_input __unused, int colortemp_mode, int roffset)
{
    tcon_rgb_ogo_t rgbogo;
    memset (&rgbogo, 0, sizeof (rgbogo));
    if (0 == GetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, &rgbogo)) {
        rgbogo.r_post_offset = roffset;
        return SetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, rgbogo);
    } else {
        SYS_LOGE("FactorySaveColorTemp_Roffset error!\n");
        return -1;
    }
}

int CPQControl::FactoryGetColorTemp_Roffset(int source_input __unused, int colortemp_mode)
{
    tcon_rgb_ogo_t rgbogo;
    memset (&rgbogo, 0, sizeof (rgbogo));
    if (0 == GetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, &rgbogo)) {
        return rgbogo.r_post_offset;
    }

    SYS_LOGE("FactoryGetColorTemp_Roffset error!\n");
    return -1;
}

int CPQControl::FactorySetColorTemp_Goffset(int source_input, int colortemp_mode, int goffset)
{
    tcon_rgb_ogo_t rgbogo;
    memset (&rgbogo, 0, sizeof (rgbogo));
    GetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, &rgbogo);
    rgbogo.g_post_offset = goffset;
    SYS_LOGD("%s, source[%d], colortemp_mode[%d], g_post_offset[%d].", __FUNCTION__, source_input,
         colortemp_mode, goffset);
    rgbogo.en = 1;

    if (Cpq_SetRGBOGO(&rgbogo) == 0) {
        return 0;
    }

    return -1;
}

int CPQControl::FactorySaveColorTemp_Goffset(int source_input __unused, int colortemp_mode, int goffset)
{
    tcon_rgb_ogo_t rgbogo;
    memset (&rgbogo, 0, sizeof (rgbogo));
    if (0 == GetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, &rgbogo)) {
        rgbogo.g_post_offset = goffset;
        return SetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, rgbogo);
    } else {
        SYS_LOGE("FactorySaveColorTemp_Goffset error!\n");
        return -1;
    }
}

int CPQControl::FactoryGetColorTemp_Goffset(int source_input __unused, int colortemp_mode)
{
    tcon_rgb_ogo_t rgbogo;
    memset (&rgbogo, 0, sizeof (rgbogo));
    if (0 == GetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, &rgbogo)) {
        return rgbogo.g_post_offset;
    }

    SYS_LOGE("FactoryGetColorTemp_Goffset error!\n");
    return -1;
}

int CPQControl::FactorySetColorTemp_Boffset(int source_input, int colortemp_mode, int boffset)
{
    tcon_rgb_ogo_t rgbogo;
    memset (&rgbogo, 0, sizeof (rgbogo));
    GetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, &rgbogo);
    rgbogo.b_post_offset = boffset;
    SYS_LOGD("%s, source_input[%d], colortemp_mode[%d], b_post_offset[%d].", __FUNCTION__, source_input,
         colortemp_mode, boffset);
    rgbogo.en = 1;

    if (Cpq_SetRGBOGO(&rgbogo) == 0) {
        return 0;
    }

    return -1;
}

int CPQControl::FactorySaveColorTemp_Boffset(int source_input __unused, int colortemp_mode, int boffset)
{
    tcon_rgb_ogo_t rgbogo;
    memset (&rgbogo, 0, sizeof (rgbogo));
    if (0 == GetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, &rgbogo)) {
        rgbogo.b_post_offset = boffset;
        return SetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, rgbogo);
    } else {
        SYS_LOGE("FactorySaveColorTemp_Boffset error!\n");
        return -1;
    }
}

int CPQControl::FactoryGetColorTemp_Boffset(int source_input __unused, int colortemp_mode)
{
    tcon_rgb_ogo_t rgbogo;
    memset (&rgbogo, 0, sizeof (rgbogo));
    if (0 == GetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, &rgbogo)) {
        return rgbogo.b_post_offset;
    }

    SYS_LOGE("FactoryGetColorTemp_Boffset error!\n");
    return -1;
}

int CPQControl::FactoryResetNonlinear(void)
{
    return mPQdb->PQ_ResetAllNoLineParams();
}

int CPQControl::FactorySetParamsDefault(void)
{
    FactoryResetPQMode();
    FactoryResetNonlinear();
    FactoryResetColorTemp();
    if (mbCpqCfg_separate_db_enable) {
        int ret = -1;
        char dstOverscanDbPath[128] = {0};

        ret = mpOverScandb->closeOverScanDB();
        if (ret == 0) {
            ret = unlink(PARAM_OVERSCAN_DB_PATH);
            if (ret == 0) {
                int display_mode = GetDisplayMode();

                mPQConfigFile->GetOverscandbPath(dstOverscanDbPath);
                mpOverScandb->openOverScanDB(dstOverscanDbPath);
                ret = SetDisplayMode((vpp_display_mode_t)display_mode, 0);
            }else
                SYS_LOGE("unlink[%s] faile,ret=%d\n", PARAM_OVERSCAN_DB_PATH, ret);
        }else
            SYS_LOGE("closeOverScanDB err,ret=%d\n", ret);
    } else {
        int ret = -1;
        char dstPqDbPath[128] = {0};

        ret = mPQdb->closePqDB();
        if (ret == 0) {
            ret = unlink(PARAM_PQ_DB_PATH);
            if (ret == 0) {
                int display_mode = GetDisplayMode();

                mPQConfigFile->GetPqdbPath(dstPqDbPath);
                mPQdb->openPqDB(dstPqDbPath);
                ret = SetDisplayMode((vpp_display_mode_t)display_mode, 0);
            }else
                SYS_LOGE("unlink[%s] faile,ret=%d\n", PARAM_PQ_DB_PATH, ret);
        }else
            SYS_LOGE("closeOverScanDB err,ret=%d\n", ret);
    }
    return 0;
}

int CPQControl::FactorySetNolineParams(source_input_param_t source_input_param, int type, noline_params_t noline_params)
{
    int ret = -1;

    switch (type) {
    case NOLINE_PARAMS_TYPE_BRIGHTNESS:
        ret = mPQdb->PQ_SetNoLineAllBrightnessParams(source_input_param.source_input,
                noline_params.osd0, noline_params.osd25, noline_params.osd50, noline_params.osd75,
                noline_params.osd100);
        break;

    case NOLINE_PARAMS_TYPE_CONTRAST:
        ret = mPQdb->PQ_SetNoLineAllContrastParams(source_input_param.source_input,
                noline_params.osd0, noline_params.osd25, noline_params.osd50, noline_params.osd75,
                noline_params.osd100);
        break;

    case NOLINE_PARAMS_TYPE_SATURATION:
        ret = mPQdb->PQ_SetNoLineAllSaturationParams(source_input_param.source_input,
                noline_params.osd0, noline_params.osd25, noline_params.osd50, noline_params.osd75,
                noline_params.osd100);
        break;

    case NOLINE_PARAMS_TYPE_HUE:
        ret = mPQdb->PQ_SetNoLineAllHueParams(source_input_param.source_input,
                noline_params.osd0, noline_params.osd25, noline_params.osd50, noline_params.osd75,
                noline_params.osd100);
        break;

    case NOLINE_PARAMS_TYPE_SHARPNESS:
        ret = mPQdb->PQ_SetNoLineAllSharpnessParams(source_input_param.source_input,
                noline_params.osd0, noline_params.osd25, noline_params.osd50, noline_params.osd75,
                noline_params.osd100);
        break;

    case NOLINE_PARAMS_TYPE_VOLUME:
        ret = mPQdb->PQ_SetNoLineAllVolumeParams(source_input_param.source_input,
                noline_params.osd0, noline_params.osd25, noline_params.osd50, noline_params.osd75,
                noline_params.osd100);
        break;

    default:
        break;
    }

    return ret;
}

noline_params_t CPQControl::FactoryGetNolineParams(source_input_param_t source_input_param, int type)
{
    int ret = -1;
    noline_params_t noline_params;
    memset(&noline_params, 0, sizeof(noline_params_t));

    switch (type) {
    case NOLINE_PARAMS_TYPE_BRIGHTNESS:
        ret = mPQdb->PQ_GetNoLineAllBrightnessParams(source_input_param.source_input,
                &noline_params.osd0, &noline_params.osd25, &noline_params.osd50,
                &noline_params.osd75, &noline_params.osd100);
        break;

    case NOLINE_PARAMS_TYPE_CONTRAST:
        ret = mPQdb->PQ_GetNoLineAllContrastParams(source_input_param.source_input,
                &noline_params.osd0, &noline_params.osd25, &noline_params.osd50,
                &noline_params.osd75, &noline_params.osd100);
        break;

    case NOLINE_PARAMS_TYPE_SATURATION:
        ret = mPQdb->PQ_GetNoLineAllSaturationParams(source_input_param.source_input,
                &noline_params.osd0, &noline_params.osd25, &noline_params.osd50,
                &noline_params.osd75, &noline_params.osd100);
        break;

    case NOLINE_PARAMS_TYPE_HUE:
        ret = mPQdb->PQ_GetNoLineAllHueParams(source_input_param.source_input,
                &noline_params.osd0, &noline_params.osd25, &noline_params.osd50,
                &noline_params.osd75, &noline_params.osd100);
        break;

    case NOLINE_PARAMS_TYPE_SHARPNESS:
        ret = mPQdb->PQ_GetNoLineAllSharpnessParams(source_input_param.source_input,
                &noline_params.osd0, &noline_params.osd25, &noline_params.osd50,
                &noline_params.osd75, &noline_params.osd100);
        break;

    case NOLINE_PARAMS_TYPE_VOLUME:
        ret = mPQdb->PQ_GetNoLineAllVolumeParams(source_input_param.source_input,
                &noline_params.osd0, &noline_params.osd25, &noline_params.osd50,
                &noline_params.osd75, &noline_params.osd100);
        break;

    default:
        break;
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return noline_params;
}

int CPQControl::FactorySetHdrMode(int mode)
{
    return SetHDRMode(mode);
}

int CPQControl::FactoryGetHdrMode(void)
{
    return GetHDRMode();
}

int CPQControl::FactorySetOverscanParam(source_input_param_t source_input_param, vpp_display_mode_t dmode, tvin_cutwin_t cutwin_t)
{
    int ret = -1;
    if (mbCpqCfg_separate_db_enable) {
        ret = mpOverScandb->PQ_SetOverscanParams(source_input_param, dmode, cutwin_t);
    }

    if (ret != 0) {
        SYS_LOGE("%s failed.\n", __FUNCTION__);
    } else {
        SYS_LOGD("%s success.\n", __FUNCTION__);
    }

    return ret;
}

tvin_cutwin_t CPQControl::FactoryGetOverscanParam(source_input_param_t source_input_param, vpp_display_mode_t dmode)
{
    int ret = -1;
    tvin_cutwin_t cutwin_t;
    memset(&cutwin_t, 0, sizeof(cutwin_t));

    if (source_input_param.trans_fmt < TVIN_TFMT_2D || source_input_param.trans_fmt > TVIN_TFMT_3D_LDGD) {
        return cutwin_t;
    }
    if (mbCpqCfg_separate_db_enable) {
        ret = mpOverScandb->PQ_GetOverscanParams(source_input_param, dmode, &cutwin_t);
    }

    if (ret != 0) {
        SYS_LOGE("%s failed.\n", __FUNCTION__);
    } else {
        SYS_LOGD("%s success.\n", __FUNCTION__);
    }

    return cutwin_t;
}

int CPQControl::FactorySetGamma(int gamma_r_value, int gamma_g_value, int gamma_b_value)
{
    int ret = 0;
    tcon_gamma_table_t gamma_r, gamma_g, gamma_b;

    memset(gamma_r.data, (unsigned short)gamma_r_value, GAMMA_NUMBER);
    memset(gamma_g.data, (unsigned short)gamma_g_value, GAMMA_NUMBER);
    memset(gamma_b.data, (unsigned short)gamma_b_value, GAMMA_NUMBER);

    ret |= Cpq_SetGammaTbl_R((unsigned short *) gamma_r.data);
    ret |= Cpq_SetGammaTbl_G((unsigned short *) gamma_g.data);
    ret |= Cpq_SetGammaTbl_B((unsigned short *) gamma_b.data);

    return ret;
}

int CPQControl::FactorySSMRestore(void)
{
    resetAllUserSettingParam();
    return 0;
}

int CPQControl::Cpq_SetXVYCCMode(vpp_xvycc_mode_t xvycc_mode, source_input_param_t source_input_param)
{
    int ret = -1;
    am_regs_t regs, regs_1;
    memset(&regs, 0, sizeof(am_regs_t));
    memset(&regs_1, 0, sizeof(am_regs_t));

    if (mbCpqCfg_xvycc_enable) {
        if (mPQdb->PQ_GetXVYCCParams((vpp_xvycc_mode_t) xvycc_mode, source_input_param, &regs, &regs_1) == 0) {
            ret = Cpq_LoadRegs(regs);
            ret |= Cpq_LoadRegs(regs_1);
        } else {
            SYS_LOGE("PQ_GetXVYCCParams failed!\n");
        }
    } else {
        SYS_LOGD("XVYCC module disabled!\n");
        ret = 0;
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::SetColorDemoMode(vpp_color_demomode_t demomode)
{
    SYS_LOGD("%s: mode is %d\n", __FUNCTION__, demomode);
    int ret = -1;
    cm_regmap_t regmap;
    unsigned long *temp_regmap;
    int i = 0;
    vpp_display_mode_t displaymode = VPP_DISPLAY_MODE_MODE43;

    switch (demomode) {
    case VPP_COLOR_DEMO_MODE_YOFF:
        temp_regmap = DemoColorYOffRegMap;
        break;

    case VPP_COLOR_DEMO_MODE_COFF:
        temp_regmap = DemoColorCOffRegMap;
        break;

    case VPP_COLOR_DEMO_MODE_GOFF:
        temp_regmap = DemoColorGOffRegMap;
        break;

    case VPP_COLOR_DEMO_MODE_MOFF:
        temp_regmap = DemoColorMOffRegMap;
        break;

    case VPP_COLOR_DEMO_MODE_ROFF:
        temp_regmap = DemoColorROffRegMap;
        break;

    case VPP_COLOR_DEMO_MODE_BOFF:
        temp_regmap = DemoColorBOffRegMap;
        break;

    case VPP_COLOR_DEMO_MODE_RGBOFF:
        temp_regmap = DemoColorRGBOffRegMap;
        break;

    case VPP_COLOR_DEMO_MODE_YMCOFF:
        temp_regmap = DemoColorYMCOffRegMap;
        break;

    case VPP_COLOR_DEMO_MODE_ALLOFF:
        temp_regmap = DemoColorALLOffRegMap;
        break;

    case VPP_COLOR_DEMO_MODE_ALLON:
    default:
        if (displaymode == VPP_DISPLAY_MODE_MODE43) {
            temp_regmap = DemoColorSplit4_3RegMap;
        }/* else {
            temp_regmap = DemoColorSplitRegMap;
        }*/

        break;
    }

    for (i = 0; i < CM_REG_NUM; i++) {
        regmap.reg[i] = temp_regmap[i];
    }

    ret = VPPDeviceIOCtl(AMSTREAM_IOC_CM_REGMAP, regmap);
    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::SetColorBaseMode(vpp_color_basemode_t basemode, int isSave)
{
    SYS_LOGI("%s: mode is %d\n", __FUNCTION__, basemode);
    int ret = Cpq_SetColorBaseMode(basemode, mCurrentSourceInputInfo);
    if (ret < 0) {
        SYS_LOGE("Cpq_SetColorBaseMode Failed!!!");
    } else {
        if (isSave == 1) {
            ret = SaveColorBaseMode(basemode);
        } else {
            SYS_LOGD("%s: No need save!\n", __FUNCTION__);
        }
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

vpp_color_basemode_t CPQControl::GetColorBaseMode(void)
{
    vpp_color_basemode_t data = VPP_COLOR_BASE_MODE_OFF;
    unsigned char tmp_base_mode = 0;
    mSSMAction->SSMReadColorBaseMode(&tmp_base_mode);
    data = (vpp_color_basemode_t) tmp_base_mode;
    if (data < VPP_COLOR_BASE_MODE_OFF || data >= VPP_COLOR_BASE_MODE_MAX) {
        data = VPP_COLOR_BASE_MODE_OPTIMIZE;
    }
    SYS_LOGD("%s: mode is %d\n", __FUNCTION__, data);
    return data;
}

int CPQControl::SaveColorBaseMode(vpp_color_basemode_t basemode)
{
    SYS_LOGD("%s: mode is %d\n", __FUNCTION__, basemode);
    int ret = -1;
    if (basemode == VPP_COLOR_BASE_MODE_DEMO) {
        ret = 0;
    } else {
        ret = mSSMAction->SSMSaveColorBaseMode(basemode);
    }

    return ret;
}

int CPQControl::Cpq_SetColorBaseMode(vpp_color_basemode_t basemode, source_input_param_t source_input_param)
{
    int ret = -1;
    am_regs_t regs;
    memset(&regs, 0, sizeof(am_regs_t));

    if (mbCpqCfg_cm2_enable) {
        if (mPQdb->PQ_GetCM2Params((vpp_color_management2_t)basemode, source_input_param, &regs) == 0) {
            ret = Cpq_LoadRegs(regs);
        } else {
            SYS_LOGE("PQ_GetCM2Params failed!\n");
        }
    } else {
        SYS_LOGD("CM module disabled!\n");
        ret = 0;
    }

    return ret;
}


int CPQControl::Cpq_SetRGBOGO(const struct tcon_rgb_ogo_s *rgbogo)
{
    int ret = VPPDeviceIOCtl(AMVECM_IOC_S_RGB_OGO, rgbogo);
    if (ret < 0) {
        SYS_LOGE("%s failed(%s)!\n", __FUNCTION__, strerror(errno));
    }

    return ret;
}

int CPQControl::Cpq_GetRGBOGO(const struct tcon_rgb_ogo_s *rgbogo)
{
    int ret = VPPDeviceIOCtl(AMVECM_IOC_G_RGB_OGO, rgbogo);
    if (ret < 0) {
        SYS_LOGE("%s failed(%s)!\n", __FUNCTION__, strerror(errno));
    }

    return ret;
}

int CPQControl::Cpq_SetGammaOnOff(int onoff)
{
    int ret = -1;

    if (onoff == 1) {
        SYS_LOGD("%s: enable gamma!\n", __FUNCTION__);
        ret = VPPDeviceIOCtl(AMVECM_IOC_GAMMA_TABLE_EN);
    } else {
        SYS_LOGD("%s: disable gamma!\n", __FUNCTION__);
        ret = VPPDeviceIOCtl(AMVECM_IOC_GAMMA_TABLE_DIS);
    }

    if (ret < 0) {
        SYS_LOGE("%s error(%s)!\n", __FUNCTION__, strerror(errno));
    }

    return ret;
}

//load aad pq
int CPQControl::Cpq_SetAAD(const db_aad_param_t *pAAD)
{
    int ret = VPPDeviceIOCtl(AMVECM_IOC_S_AAD_PARAM, pAAD);
    if (ret < 0) {
        SYS_LOGE("%s error(%s)\n", __FUNCTION__, strerror(errno));
    }

    return ret;
}

int CPQControl::SetAad(void)
{
    int ret = -1;

    if (mbCpqCfg_aad_enable) {
        aad_param_t newaad;
        if (mPQdb->PQ_GetAADParams(mCurrentSourceInputInfo, &newaad) == 0) {
            db_aad_param_t db_newaad;
            db_newaad.aad_param_cabc_aad_en   = newaad.aad_param_cabc_aad_en;
            db_newaad.aad_param_aad_en        = newaad.aad_param_aad_en;
            db_newaad.aad_param_tf_en         = newaad.aad_param_tf_en;
            db_newaad.aad_param_force_gain_en = newaad.aad_param_force_gain_en;
            db_newaad.aad_param_sensor_mode   = newaad.aad_param_sensor_mode;
            db_newaad.aad_param_mode          = newaad.aad_param_mode;
            db_newaad.aad_param_dist_mode     = newaad.aad_param_dist_mode;
            db_newaad.aad_param_tf_alpha      = newaad.aad_param_tf_alpha;
            db_newaad.aad_param_sensor_input[0] = newaad.aad_param_sensor_input[0];
            db_newaad.aad_param_sensor_input[1] = newaad.aad_param_sensor_input[1];
            db_newaad.aad_param_sensor_input[2] = newaad.aad_param_sensor_input[2];
            db_newaad.db_LUT_Y_gain.length                   = newaad.aad_param_LUT_Y_gain_len;
            db_newaad.db_LUT_Y_gain.cabc_aad_param_ptr_len   = (long long)&(newaad.aad_param_LUT_Y_gain);
            db_newaad.db_LUT_RG_gain.length                  = newaad.aad_param_LUT_RG_gain_len;
            db_newaad.db_LUT_RG_gain.cabc_aad_param_ptr_len  = (long long)&(newaad.aad_param_LUT_RG_gain);
            db_newaad.db_LUT_BG_gain.length                  = newaad.aad_param_LUT_BG_gain_len;
            db_newaad.db_LUT_BG_gain.cabc_aad_param_ptr_len  = (long long)&(newaad.aad_param_LUT_BG_gain);
            db_newaad.db_gain_lut.length                     = newaad.aad_param_gain_lut_len;
            db_newaad.db_gain_lut.cabc_aad_param_ptr_len     = (long long)&(newaad.aad_param_gain_lut);
            db_newaad.db_xy_lut.length                       = newaad.aad_param_xy_lut_len;
            db_newaad.db_xy_lut.cabc_aad_param_ptr_len       = (long long)&(newaad.aad_param_xy_lut);

            ret = Cpq_SetAAD(&db_newaad);
        } else {
            SYS_LOGE("mPQdb->PQ_GetAADParams failed\n");
        }
    } else {
        SYS_LOGD("AAD module disabled\n");
        ret = 0;
    }

    if (ret < 0) {
        SYS_LOGE("%s failed\n",__FUNCTION__);
    } else {
        SYS_LOGI("%s success\n",__FUNCTION__);
    }

    return ret;
}

//load aad pq
int CPQControl::Cpq_SetCABC(const db_cabc_param_t *pCABC)
{
    int ret = VPPDeviceIOCtl(AMVECM_IOC_S_CABC_PARAM, pCABC);
    if (ret < 0) {
        SYS_LOGE("%s error(%s)\n", __FUNCTION__, strerror(errno));
    }

    return ret;
}

int CPQControl::SetCabc(void)
{
    int ret = -1;

    if (mbCpqCfg_cabc_enable) {
        cabc_param_t newcabc;
        if (mPQdb->PQ_GetCABCParams(mCurrentSourceInputInfo, &newcabc) == 0) {
            db_cabc_param_t db_newcabc;
            db_newcabc.cabc_param_cabc_en          = newcabc.cabc_param_cabc_en;
            db_newcabc.cabc_param_hist_mode        = newcabc.cabc_param_hist_mode;
            db_newcabc.cabc_param_tf_en            = newcabc.cabc_param_tf_en;
            db_newcabc.cabc_param_sc_flag          = newcabc.cabc_param_sc_flag;
            db_newcabc.cabc_param_bl_map_mode      = newcabc.cabc_param_bl_map_mode;
            db_newcabc.cabc_param_bl_map_en        = newcabc.cabc_param_bl_map_en;
            db_newcabc.cabc_param_temp_proc        = newcabc.cabc_param_temp_proc;
            db_newcabc.cabc_param_max95_ratio      = newcabc.cabc_param_max95_ratio;
            db_newcabc.cabc_param_hist_blend_alpha = newcabc.cabc_param_hist_blend_alpha;
            db_newcabc.cabc_param_init_bl_min      = newcabc.cabc_param_init_bl_min;
            db_newcabc.cabc_param_init_bl_max      = newcabc.cabc_param_init_bl_max;
            db_newcabc.cabc_param_tf_alpha         = newcabc.cabc_param_tf_alpha;
            db_newcabc.cabc_param_sc_hist_diff_thd = newcabc.cabc_param_sc_hist_diff_thd;
            db_newcabc.cabc_param_sc_apl_diff_thd  = newcabc.cabc_param_sc_apl_diff_thd;
            db_newcabc.cabc_param_patch_bl_th      = newcabc.cabc_param_patch_bl_th;
            db_newcabc.cabc_param_patch_on_alpha   = newcabc.cabc_param_patch_on_alpha;
            db_newcabc.cabc_param_patch_bl_off_th  = newcabc.cabc_param_patch_bl_off_th;
            db_newcabc.cabc_param_patch_off_alpha  = newcabc.cabc_param_patch_off_alpha;
            db_newcabc.db_o_bl_cv.length                      = newcabc.cabc_param_o_bl_cv_len;
            db_newcabc.db_o_bl_cv.cabc_aad_param_ptr_len      = (long long)&(newcabc.cabc_param_o_bl_cv);
            db_newcabc.db_maxbin_bl_cv.length                 = newcabc.cabc_param_maxbin_bl_cv_len;
            db_newcabc.db_maxbin_bl_cv.cabc_aad_param_ptr_len = (long long)&(newcabc.cabc_param_maxbin_bl_cv);

            ret = Cpq_SetCABC(&db_newcabc);
        } else {
            SYS_LOGE("mPQdb->PQ_GetCABCParams failed\n");
        }
    } else {
        SYS_LOGD("CABC module disabled\n");
        ret = 0;
    }

    if (ret < 0) {
        SYS_LOGE("%s failed\n",__FUNCTION__);
    } else {
        SYS_LOGI("%s success\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::SetDnlpMode(int level)
{
    int ret = -1;
    if (mbCpqCfg_dnlp_enable) {
        ret = Cpq_SetDnlpMode((Dynamic_contrast_status_t)level, mCurrentSourceInputInfo);
        if (ret == 0) {
            ret = SaveDnlpMode((Dynamic_contrast_status_t)level);
        }
    } else {
        SYS_LOGI("DNLP module disabled!\n");
        ret = 0;
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGI("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::GetDnlpMode()
{
    int ret = -1, level = DYNAMIC_CONTRAST_MID;
    if (mbCpqCfg_new_picture_mode_enable) {
        vpp_picture_mode_t pq_mode = (vpp_picture_mode_t)GetPQMode();
        vpp_pictur_mode_para_t para;
        if (GetPictureModeData(mCurrentPqSource, pq_mode, &para) == 0) {
            level = para.DynamicContrast;
            ret = 0;
        }
    } else {
        ret = mSSMAction->SSMReadDnlpMode(mCurrentSourceInputInfo.source_input, &level);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGI("%s success!\n",__FUNCTION__);
    }

    SYS_LOGI("%s, source_input = %d, mode is %d\n",__FUNCTION__, mCurrentSourceInputInfo.source_input, level);
    return level;
}

int CPQControl::SaveDnlpMode(Dynamic_contrast_status_t level)
{
    SYS_LOGD("%s: level is %d\n", __FUNCTION__, level);
    int ret = -1;
    if (mbCpqCfg_new_picture_mode_enable) {
         vpp_pictur_mode_para_t para;
         vpp_picture_mode_t pq_mode = (vpp_picture_mode_t)GetPQMode();
         if (GetPictureModeData(mCurrentPqSource, pq_mode, &para) == 0) {
             para.DynamicContrast = (int)level;
             ret = SetPictureModeData(mCurrentPqSource, pq_mode, &para);
         }
     } else {
        ret = mSSMAction->SSMSaveDnlpMode(mSourceInputForSaveParam, level);
    }

    return ret;
}

int CPQControl::Cpq_SetVENewDNLP(const ve_dnlp_curve_param_t *pDNLP)
{
    int ret = VPPDeviceIOCtl(AMVECM_IOC_VE_NEW_DNLP, pDNLP);
    if (ret < 0) {
        SYS_LOGE("%s error(%s)!\n", __FUNCTION__, strerror(errno));
    }

    return ret;
}

int CPQControl::Cpq_SetDnlpMode(Dynamic_contrast_status_t level, source_input_param_t source_input_param)
{
    int ret = -1;
    ve_dnlp_curve_param_t newdnlp;

    if (mPQdb->PQ_GetDNLPParams(mCurrentSourceInputInfo, level, &newdnlp) == 0) {
        ret = Cpq_SetVENewDNLP(&newdnlp);
    } else {
        SYS_LOGE("mPQdb->PQ_GetDNLPParams failed!\n");
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGI("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::Cpq_SetDNLPStatus(ve_dnlp_state_t status)
{
    int ret = VPPDeviceIOCtl(AMVECM_IOC_S_DNLP_STATE, &status);
    if (ret < 0) {
        SYS_LOGE("%s error(%s)!\n", __FUNCTION__, strerror(errno));
    }

    return ret;
}

int CPQControl::FactorySetDNLPCurveParams(source_input_param_t source_input_param, int level, int final_gain)
{
    int ret = -1;
    int cur_final_gain = -1;
    char tmp_buf[128];

    cur_final_gain = mPQdb->PQ_GetDNLPGains(source_input_param, (Dynamic_contrast_status_t)level);
    if (cur_final_gain == final_gain) {
        SYS_LOGI("FactorySetDNLPCurveParams, same value, no need to update!");
        return ret;
    } else {
        SYS_LOGI("%s final_gain = %d \n", __FUNCTION__, final_gain);
        sprintf(tmp_buf, "%s %s %d", "w", "final_gain", final_gain);
        pqWriteSys(AMVECM_PQ_DNLP_DEBUG, tmp_buf);
        ret |= mPQdb->PQ_SetDNLPGains(source_input_param, (Dynamic_contrast_status_t)level, final_gain);

    }
    return ret;
}

int CPQControl::FactoryGetDNLPCurveParams(source_input_param_t source_input_param, int level)
{
    return mPQdb->PQ_GetDNLPGains(source_input_param, (Dynamic_contrast_status_t)level);
}

int CPQControl::FactorySetBlackExtRegParams(source_input_param_t source_input_param, int val)
{
    int rt = -1;
    unsigned int reg_val = 0,tmp = 0;
    unsigned int start_val = 0;
    char tmp_buf[128];

    SYS_LOGI("%s, input BE value: %d!\n", __FUNCTION__, val);

    //read data from pq.db in condition of RegAddr = 0x1D80
    reg_val = FactoryGetBEValFromDB(source_input_param, VPP_BLACKEXT_CTRL);
    SYS_LOGD("%s, read reg_val from DB: reg_val: 0x%x!\n", __FUNCTION__, reg_val);

    // blackext_start(31~24 bit) get value
    tmp = reg_val;
    for (int i = 31; i >= 24; i--) {
        start_val |= ((tmp & (1<<i)) >> 24);
    }
    SYS_LOGD("%s, blackext_start val: 0x%x\n",__FUNCTION__, start_val);

    //Re assign for target reg_addr. (23~16bit)  reset value
    for (int i = 16;i <= 23;i++) {
        reg_val = (reg_val & ~(1<<i));
    }
    reg_val = reg_val | (val << 16);
    SYS_LOGD("%s, reg_val: 0x%x val: 0x%x\n",__FUNCTION__, reg_val, val);

    rt = FactorySetBERegDBVal(source_input_param, VPP_BLACKEXT_CTRL, reg_val);

    sprintf(tmp_buf, "%s", "blk_ext_en");
    pqWriteSys(AMVECM_PQ_USER_SET, tmp_buf);

    sprintf(tmp_buf, "%s %d", "blk_start",start_val);
    pqWriteSys(AMVECM_PQ_USER_SET, tmp_buf);

    sprintf(tmp_buf, "%s %d", "blk_slope",val);
    pqWriteSys(AMVECM_PQ_USER_SET, tmp_buf);

    return rt;
}

int CPQControl::FactoryGetBlackExtRegParams(source_input_param_t source_input_param)
{
    int rt = -1;
    unsigned int reg_val = 0;
    unsigned int start_val = 0, slope_val = 0;

    //read data from pq.db in condition of RegAddr = iaddr
    reg_val = FactoryGetBEValFromDB(source_input_param, VPP_BLACKEXT_CTRL);
    SYS_LOGD("%s, read reg_val from DB: reg_val: 0x%x!\n", __FUNCTION__,reg_val);

    //analysis val read from db, Bit 31~24 blackext_start, Bit 23~16 blackext_slope1;
    // blackext_start(31~24 bit) get value
    for (int i = 31; i >= 24; i--) {
        start_val |= ((reg_val & (1<<i)) >> 24);
    }
    SYS_LOGD("%s, start_val: 0x%x\n",__FUNCTION__, start_val);
    // blackext_slope1(23~16 bit) get value
    for (int i = 23; i >= 16; i--) {
        slope_val |= ((reg_val & (1<<i)) >> 16);
    }
    SYS_LOGD("%s, slope_val: 0x%x\n",__FUNCTION__, slope_val);
    rt = slope_val;

    return rt;
}

int CPQControl::FactoryGetBEValFromDB(source_input_param_t source_input_param, int addr)
{
    unsigned int ret = 0;
    am_regs_t regs;

    SYS_LOGD("%s, input BE addr: 0x%x!\n", __FUNCTION__,addr);
    mPQdb->PQ_GetBEParams(source_input_param, addr, &regs);

    SYS_LOGD("%s - get addr - 0x%x & value - 0x%x", __FUNCTION__,
          regs.am_reg[0].addr,regs.am_reg[0].val);

    ret = regs.am_reg[0].val;

    return ret;
}

int CPQControl::FactorySetBERegDBVal(source_input_param_t source_input_param, int addr, unsigned int reg_val)
{
    int ret = -1;
    am_regs_t regs;

    mPQdb->PQ_GetBEParams(source_input_param, addr, &regs);
    regs.am_reg[0].val = reg_val;
    ret = mPQdb->PQ_SetBEParams(source_input_param, addr, regs.am_reg[0].val);
    SYS_LOGD ("%s - get addr - 0x%x & value - 0x%x", __FUNCTION__,
          regs.am_reg[0].addr,regs.am_reg[0].val);

    return ret;
}

int CPQControl::FactorySetRGBCMYFcolorParams(source_input_param_t source_input_param, int color_type,int color_param,int val)
{
    int data_Rank = -1;
    char tmp_buf[128];

    if (color_param == COLOR_SATURATION) {
        if (val < -100 || val > 127)
            return -1;
    } else if (color_param == COLOR_HUE) {
        if (val < -127 || val > 127)
            return -1;
    } else if (color_param == COLOR_LUMA) {
        if (val < -15 || val > 15)
            return -1;
    }

    switch (color_type) {
        case COLOR_RED:
            if (color_param == COLOR_SATURATION) {
                data_Rank = CMS_sat_red;
            } else if (color_param == COLOR_HUE) {
                data_Rank = CMS_hue_red;
            } else if (color_param == COLOR_LUMA) {
                data_Rank = CMS_luma_red;
            }
            break;
        case COLOR_GREEN:
            if (color_param == COLOR_SATURATION) {
                data_Rank = CMS_sat_green;
            } else if (color_param == COLOR_HUE) {
                data_Rank = CMS_hue_green;
            } else if (color_param == COLOR_LUMA) {
                data_Rank = CMS_luma_green;
            }
            break;
        case COLOR_BLUE:
            if (color_param == COLOR_SATURATION) {
                data_Rank = CMS_sat_blue;
            } else if (color_param == COLOR_HUE) {
                data_Rank = CMS_hue_blue;
            } else if (color_param == COLOR_LUMA) {
                data_Rank = CMS_luma_blue;
            }
            break;
        case COLOR_GRAY:
            if (color_param == COLOR_SATURATION) {
                data_Rank = CMS_sat_cyan;
            } else if (color_param == COLOR_HUE) {
                data_Rank = CMS_hue_cyan;
            } else if (color_param == COLOR_LUMA) {
                data_Rank = CMS_luma_cyan;
            }
            break;
        case COLOR_MAGENTA:
            if (color_param == COLOR_SATURATION) {
                data_Rank = CMS_sat_purple;
            } else if (color_param == COLOR_HUE) {
                data_Rank = CMS_hue_purple;
            } else if (color_param == COLOR_LUMA) {
                data_Rank = CMS_luma_purple;
            }
            break;
        case COLOR_YELLOW:
            if (color_param == COLOR_SATURATION) {
                data_Rank = CMS_sat_yellow;
            } else if (color_param == COLOR_HUE) {
                data_Rank = CMS_hue_yellow;
            } else if (color_param == COLOR_LUMA) {
                data_Rank = CMS_luma_yellow;
            }
            break;
        case COLOR_FLESHTONE:
            if (color_param == COLOR_SATURATION) {
                data_Rank = CMS_sat_skin;
            } else if (color_param == COLOR_HUE) {
                data_Rank = CMS_hue_skin;
            } else if (color_param == COLOR_LUMA) {
                data_Rank = CMS_luma_skin;
            }
            break;
        default:
            break;
    }

    if (color_param == COLOR_SATURATION) {
        sprintf(tmp_buf, "%s %d %d %d", "cm2_sat", color_type, val, 0);
        pqWriteSys(AMVECM_PQ_CM2_SAT, tmp_buf);
    } else if (color_param == COLOR_HUE) {
        sprintf(tmp_buf, "%s %d %d %d", "cm2_hue", color_type, val, 0);
        pqWriteSys(AMVECM_PQ_CM2_HUE_BY_HS, tmp_buf);
    } else if (color_param == COLOR_LUMA) {
        sprintf(tmp_buf, "%s %d %d %d", "cm2_luma", color_type, val, 0);
        pqWriteSys(AMVECM_PQ_CM2_LUMA, tmp_buf);
    }

    return mPQdb->PQ_SetRGBCMYFcolor(source_input_param, data_Rank, val);
}

int CPQControl::FactoryGetRGBCMYFcolorParams(source_input_param_t source_input_param, int color_type,int color_param)
{
    int data_Rank = -1;

    switch (color_type) {
        case COLOR_RED:
            if (color_param == COLOR_SATURATION) {
                data_Rank = CMS_sat_red;
            } else if (color_param == COLOR_HUE) {
                data_Rank = CMS_hue_red;
            } else if (color_param == COLOR_LUMA) {
                data_Rank = CMS_luma_red;
            }
            break;
        case COLOR_GREEN:
            if (color_param == COLOR_SATURATION) {
                data_Rank = CMS_sat_green;
            } else if (color_param == COLOR_HUE) {
                data_Rank = CMS_hue_green;
            } else if (color_param == COLOR_LUMA) {
                data_Rank = CMS_luma_green;
            }
            break;
        case COLOR_BLUE:
            if (color_param == COLOR_SATURATION) {
                data_Rank = CMS_sat_blue;
            } else if (color_param == COLOR_HUE) {
                data_Rank = CMS_hue_blue;
            } else if (color_param == COLOR_LUMA) {
                data_Rank = CMS_luma_blue;
            }
            break;
        case COLOR_GRAY:
            if (color_param == COLOR_SATURATION) {
                data_Rank = CMS_sat_cyan;
            } else if (color_param == COLOR_HUE) {
                data_Rank = CMS_hue_cyan;
            } else if (color_param == COLOR_LUMA) {
                data_Rank = CMS_luma_cyan;
            }
            break;
        case COLOR_MAGENTA:
            if (color_param == COLOR_SATURATION) {
                data_Rank = CMS_sat_purple;
            } else if (color_param == COLOR_HUE) {
                data_Rank = CMS_hue_purple;
            } else if (color_param == COLOR_LUMA) {
                data_Rank = CMS_luma_purple;
            }
            break;
        case COLOR_YELLOW:
            if (color_param == COLOR_SATURATION) {
                data_Rank = CMS_sat_yellow;
            } else if (color_param == COLOR_HUE) {
                data_Rank = CMS_hue_yellow;
            } else if (color_param == COLOR_LUMA) {
                data_Rank = CMS_luma_yellow;
            }
            break;
        case COLOR_FLESHTONE:
            if (color_param == COLOR_SATURATION) {
                data_Rank = CMS_sat_skin;
            } else if (color_param == COLOR_HUE) {
                data_Rank = CMS_hue_skin;
            } else if (color_param == COLOR_LUMA) {
                data_Rank = CMS_luma_skin;
            }
            break;
        default:
            break;
    }

    return mPQdb->PQ_GetRGBCMYFcolor(source_input_param, data_Rank);
}

int CPQControl::FactorySetNoiseReductionParams(source_input_param_t source_input_param, vpp_noise_reduction_mode_t nr_mode, int addr, int val)
{
    return mPQdb->PQ_SetNoiseReductionParams(nr_mode, source_input_param, addr, val);
}

int CPQControl::FactoryGetNoiseReductionParams(source_input_param_t source_input_param, vpp_noise_reduction_mode_t nr_mode, int addr)
{
    return mPQdb->PQ_GetNoiseReductionParams(nr_mode, source_input_param, addr);
}

int CPQControl::SetCTIParamsCheckVal(int param_type, int val)
{
    switch (param_type) {
        case CVD_YC_DELAY: {
            if (val < 0 || val > 15) {
                return -1;
            }
            break;
        }
        case DECODE_CTI: {
            if (val < 0 || val > 3) {
                return -1;
            }
            break;
        }
        case SR0_CTI_GAIN0:
        case SR0_CTI_GAIN1:
        case SR0_CTI_GAIN2:
        case SR0_CTI_GAIN3:
        case SR1_CTI_GAIN0:
        case SR1_CTI_GAIN1:
        case SR1_CTI_GAIN2:
        case SR1_CTI_GAIN3:{
            if (val < 0 || val > 255) {
                return -1;
            }
            break;
        }
    }

    return 0;
}

int CPQControl::MatchCTIRegMask(int param_type)
{
    switch (param_type) {
        case CVD_YC_DELAY:
            return YC_DELAY_REG_MASK;
        case DECODE_CTI:
            return DECODE_CTI_REG_MASK;
        case SR0_CTI_GAIN0:
            return SR0_GAIN0_REG_MASK;
        case SR0_CTI_GAIN1:
            return SR0_GAIN1_REG_MASK;
        case SR0_CTI_GAIN2:
            return SR0_GAIN2_REG_MASK;
        case SR0_CTI_GAIN3:
            return SR0_GAIN3_REG_MASK;
        case SR1_CTI_GAIN0:
            return SR1_GAIN0_REG_MASK;
        case SR1_CTI_GAIN1:
            return SR1_GAIN1_REG_MASK;
        case SR1_CTI_GAIN2:
            return SR1_GAIN2_REG_MASK;
        case SR1_CTI_GAIN3:
            return SR1_GAIN3_REG_MASK;
    }
    return -1;
}

int CPQControl::MatchCTIRegAddr(int param_type)
{
    if (param_type == CVD_YC_DELAY) {
        return VPP_CTI_YC_DELAY;
    } else if (param_type == DECODE_CTI) {
        return VPP_DECODE_CTI;
    } else if (param_type >= SR0_CTI_GAIN0 && param_type <= SR0_CTI_GAIN3) {
        return VPP_CTI_SR0_GAIN;
    } else if (param_type >= SR1_CTI_GAIN0 && param_type <= SR1_CTI_GAIN3) {
        return VPP_CTI_SR1_GAIN;
    } else {
        SYS_LOGE("%s, error: param_type[%d] is not supported", __FUNCTION__, param_type);
        return -1;
    }
    return -1;
}

int CPQControl::FactorySetCTIParams(source_input_param_t source_input_param, int param_type, int val)
{
    int tmp_val;
    char tmp_buf[128];
    int addr, reg_mask;

    addr = MatchCTIRegAddr(param_type);
    reg_mask = MatchCTIRegMask(param_type);

    if (SetCTIParamsCheckVal(param_type, val) < 0) {
        SYS_LOGE("%s, error: val[%d] is out of range", __FUNCTION__, val);
        return -1;
    }

    tmp_val = mPQdb->PQ_GetSharpnessCTIParams(source_input_param, addr, param_type, reg_mask);
    SYS_LOGI("%s, get value is %d, try to set value: %d\n", __FUNCTION__, tmp_val, val);

    switch (param_type) {
        case CVD_YC_DELAY: {
            tmp_val &= ~0xf;
            tmp_val |= val;
            sprintf(tmp_buf, "%s %x %x", "wv", tmp_val, VPP_CTI_YC_DELAY);
            pqWriteSys(TVAFE_TVAFE0_REG, tmp_buf);
            break;
        }
        case DECODE_CTI: {
            tmp_val &= ~(0x3 << 6);
            tmp_val |= (val << 6);
            sprintf(tmp_buf, "%s %x %x", "wv", tmp_val, VPP_DECODE_CTI);
            pqWriteSys(TVAFE_TVAFE0_REG, tmp_buf);
            break;
        }
        case SR0_CTI_GAIN0: {
            tmp_val = val << 24;
            sprintf(tmp_buf, "%s %x %x %x %x", "bw", VPP_CTI_SR0_GAIN, val, 24, 8);
            pqWriteSys(AMVECM_PQ_REG_RW, tmp_buf);
            break;
        }
        case SR0_CTI_GAIN1: {
            tmp_val = val << 16;
            sprintf(tmp_buf, "%s %x %x %x %x", "bw", VPP_CTI_SR0_GAIN, val, 16, 8);
            pqWriteSys(AMVECM_PQ_REG_RW, tmp_buf);
            break;
        }
        case SR0_CTI_GAIN2: {
            tmp_val = val << 8;
            sprintf(tmp_buf, "%s %x %x %x %x", "bw", VPP_CTI_SR0_GAIN, val, 8, 8);
            pqWriteSys(AMVECM_PQ_REG_RW, tmp_buf);
            break;
        }
        case SR0_CTI_GAIN3: {
            tmp_val = val;
            sprintf(tmp_buf, "%s %x %x %x %x", "bw", VPP_CTI_SR0_GAIN, val, 0, 8);
            pqWriteSys(AMVECM_PQ_REG_RW, tmp_buf);
            break;
        }
        case SR1_CTI_GAIN0: {
            tmp_val = val << 24;
            sprintf(tmp_buf, "%s %x %x %x %x", "bw", VPP_CTI_SR1_GAIN, val, 24, 8);
            pqWriteSys(AMVECM_PQ_REG_RW, tmp_buf);
            break;
        }
        case SR1_CTI_GAIN1: {
            tmp_val = val << 16;
            sprintf(tmp_buf, "%s %x %x %x %x", "bw", VPP_CTI_SR1_GAIN, val, 16, 8);
            pqWriteSys(AMVECM_PQ_REG_RW, tmp_buf);
            break;
        }
        case SR1_CTI_GAIN2: {
            tmp_val = val << 8;
            sprintf(tmp_buf, "%s %x %x %x %x", "bw", VPP_CTI_SR1_GAIN, val, 8, 8);
            pqWriteSys(AMVECM_PQ_REG_RW, tmp_buf);
            break;
        }
        case SR1_CTI_GAIN3: {
            tmp_val = val;
            sprintf(tmp_buf, "%s %x %x %x %x", "bw", VPP_CTI_SR1_GAIN, val, 0, 8);
            pqWriteSys(AMVECM_PQ_REG_RW, tmp_buf);
            break;
        }
        default:
            break;
    }

    mPQdb->PQ_SetSharpnessCTIParams(source_input_param, addr, tmp_val, param_type, reg_mask);

    return 0;
}

int CPQControl::FactoryGetCTIParams(source_input_param_t source_input_param, int param_type)
{
    unsigned int rval, reg_val;
    int addr, reg_mask;

    addr = MatchCTIRegAddr(param_type);
    reg_mask = MatchCTIRegMask(param_type);

    reg_val = mPQdb->PQ_GetSharpnessCTIParams(source_input_param, addr, param_type, reg_mask);

    switch (param_type) {
        case CVD_YC_DELAY: {
            SYS_LOGD("%s, type [%d], get val: %u\n", __FUNCTION__, param_type, reg_val);
            SYS_LOGD("%s, type [%d], get val: %d\n", __FUNCTION__, param_type, reg_val);
            rval = reg_val & 0xf;
            SYS_LOGD("%s, type [%d], get val: %d\n", __FUNCTION__, param_type, rval);
            break;
        }
        case DECODE_CTI: {
            rval = (reg_val >> 6) & 0x3;
            break;
        }
        case SR0_CTI_GAIN0: {
            rval = (reg_val >> 24) & 0xff;
            break;
        }
        case SR0_CTI_GAIN1: {
            rval = (reg_val >> 16) & 0xff;
            break;
        }
        case SR0_CTI_GAIN2: {
            rval = (reg_val >> 8) & 0xff;
            break;
        }
        case SR0_CTI_GAIN3: {
            rval = reg_val & 0xff;
            break;
        }
        case SR1_CTI_GAIN0: {
            rval = (reg_val >> 24) & 0xff;
            break;
        }
        case SR1_CTI_GAIN1: {
            rval = (reg_val >> 16) & 0xff;
            break;
        }
        case SR1_CTI_GAIN2: {
            rval = (reg_val >> 8) & 0xff;
            break;
        }
        case SR1_CTI_GAIN3: {
            rval = reg_val & 0xff;
            break;
        }
        default: {
            rval = 0;
            break;
        }
    }

    SYS_LOGD("%s, type [%d], get reg_val: %u, ret val: %u\n", __FUNCTION__, param_type, reg_val, rval);
    return rval;
}

int CPQControl::SetDecodeLumaParamsCheckVal(int param_type, int val)
{
    switch (param_type) {
        case VIDEO_DECODE_BRIGHTNESS: {
            if (val < 0 || val > 511) {
                return -1;
            }
            break;
        }
        case VIDEO_DECODE_CONTRAST: {
            if (val < 0 || val > 1023) {
                return -1;
            }
            break;
        }
        case VIDEO_DECODE_SATURATION: {
            if (val < 0 || val > 255) {
                return -1;
            }
            break;
        }
        default: {
            break;
        }
    }
    return 0;
}

int CPQControl::FactorySetDecodeLumaParams(source_input_param_t source_input_param, int param_type, int val)
{
    unsigned int tmp_val = 0;
    char tmp_buf[128] = {0};
    int addr = 0, reg_mask = 0;
    int reg_set_val = 0;

    if (SetDecodeLumaParamsCheckVal(param_type, val) < 0) {
        SYS_LOGE("%s, error: val[%d] is out of range", __FUNCTION__, val);
        return -1;
    }

    if (param_type == VIDEO_DECODE_BRIGHTNESS) {
        addr = DECODE_BRI_ADDR; // 0~8 bit
        reg_mask = DECODE_BRI_REG_MASK;
    } else if (param_type == VIDEO_DECODE_CONTRAST) {
        addr = DECODE_CON_ADDR; //16~25 bit
        reg_mask = DECODE_CON_REG_MASK;
    } else if (param_type == VIDEO_DECODE_SATURATION) {
        addr = DECODE_SAT_ADDR; // 0~8 bit
        reg_mask = DECODE_SAT_REG_MASK;
    } else {
        SYS_LOGE("%s, error: param_type[%d] is not supported", __FUNCTION__, param_type);
        return -1;
    }

    tmp_val = mPQdb->PQ_GetCVD2Param(source_input_param, addr, param_type, reg_mask);
    SYS_LOGI("%s, get value is %d, try to set value: %d\n", __FUNCTION__, tmp_val, val);

    switch (param_type) {
        case VIDEO_DECODE_BRIGHTNESS: {
            tmp_val &=  ~0x1ff;
            tmp_val |= val;

            reg_set_val = tmp_val;
            reg_set_val |= (0x8 << 12);
            reg_set_val |= (0x8 << 28);
            SYS_LOGD("%s: setting val is %d", __FUNCTION__, reg_set_val);
            sprintf(tmp_buf, "%s %x %x", "wv", reg_set_val, DECODE_BRI_ADDR);
            pqWriteSys(TVAFE_TVAFE0_REG, tmp_buf);
            break;
        }
        case VIDEO_DECODE_CONTRAST: {
            tmp_val &= ~0x3ff0000;
            tmp_val |= (val << 16);

            reg_set_val = tmp_val;
            reg_set_val |= (0x8 << 12);
            reg_set_val |= (0x8 << 28);
            sprintf(tmp_buf, "%s %x %x", "wv", reg_set_val, DECODE_CON_ADDR);
            pqWriteSys(TVAFE_TVAFE0_REG, tmp_buf);
            break;
        }
        case VIDEO_DECODE_SATURATION: {
            tmp_val &= ~0xff;
            tmp_val |= val;
            sprintf(tmp_buf, "%s %x %x", "wv", tmp_val, DECODE_SAT_ADDR);
            pqWriteSys(TVAFE_TVAFE0_REG, tmp_buf);
            break;
        }
    }

    mPQdb->PQ_SetCVD2Param(source_input_param, addr, tmp_val, param_type, reg_mask);

    return 0;
}

int CPQControl::FactoryGetDecodeLumaParams(source_input_param_t source_input_param, int param_type)
{
    unsigned int rval, reg_val;
    int addr, reg_mask;

    if (param_type == VIDEO_DECODE_BRIGHTNESS) {
        addr = 0x157;
        reg_mask = 0x1ff;
    }else if (param_type == VIDEO_DECODE_CONTRAST) {
        addr = 0x157;
        reg_mask = 0x3ff0000;
    }else if (param_type == VIDEO_DECODE_SATURATION) {
        addr = DECODE_SAT_ADDR;
        reg_mask = 0xff;
    } else {
        SYS_LOGE("%s, error: param_type[%d] is not supported", __FUNCTION__, param_type);
        return -1;
    }

    reg_val = mPQdb->PQ_GetCVD2Param(source_input_param, addr, param_type, reg_mask);

    switch (param_type) {
        case VIDEO_DECODE_BRIGHTNESS: {
            rval = reg_val & 0x1ff;
            break;
        }
        case VIDEO_DECODE_CONTRAST: {
            rval = (reg_val >> 16) & 0x3ff;
            break;
        }
        case VIDEO_DECODE_SATURATION: {
            rval = reg_val & 0xff;
            break;
        }
        /*default: {
            rval = 0;
            break;
        }*/
    }

    SYS_LOGI("%s, type [%d], get reg_val: %u, ret val: %u\n", __FUNCTION__, param_type, reg_val, rval);
    return rval;
}

int CPQControl::SetSharpnessParamsCheckVal(int param_type, int val)
{
    switch (param_type) {
        case H_GAIN_HIGH:
        case H_GAIN_LOW:
        case V_GAIN_HIGH:
        case V_GAIN_LOW:
        case D_GAIN_HIGH:
        case D_GAIN_LOW:
        case PKGAIN_VSLUMALUT7:
        case PKGAIN_VSLUMALUT6:
        case PKGAIN_VSLUMALUT5:
        case PKGAIN_VSLUMALUT4:
        case PKGAIN_VSLUMALUT3:
        case PKGAIN_VSLUMALUT2:
        case PKGAIN_VSLUMALUT1:
        case PKGAIN_VSLUMALUT0: {
            if (val < 0 || val > 15) {
                return -1;
            }
            break;
        }
        case HP_DIAG_CORE:
        case BP_DIAG_CORE: {
            if (val < 0 || val > 63) {
                return -1;
            }
            break;
        }
        default: {
            break;
        }
    }

    return 0;
}

int CPQControl::MatchSharpnessRegAddr(int param_type, int isHd)
{
    if (isHd) {
        if (param_type >= H_GAIN_HIGH && param_type <= D_GAIN_LOW) {
            return SHARPNESS_HD_GAIN;
        } else if (param_type == HP_DIAG_CORE) {
            return SHARPNESS_HD_HP_DIAG_CORE;
        } else if (param_type == BP_DIAG_CORE) {
            return SHARPNESS_HD_BP_DIAG_CORE;
        } else if (param_type >= PKGAIN_VSLUMALUT7 && param_type <=PKGAIN_VSLUMALUT0) {
            return SHARPNESS_HD_PKGAIN_VSLUMA;
        } else {
            SYS_LOGE("%s, error: param_type[%d] is not supported", __FUNCTION__, param_type);
            return -1;
        }
    }

    if (param_type >= H_GAIN_HIGH && param_type <= D_GAIN_LOW) {
        return SHARPNESS_SD_GAIN;
    } else if (param_type == HP_DIAG_CORE) {
        return SHARPNESS_SD_HP_DIAG_CORE;
    } else if (param_type == BP_DIAG_CORE) {
        return SHARPNESS_SD_BP_DIAG_CORE;
    } else if (param_type >= PKGAIN_VSLUMALUT7 && param_type <=PKGAIN_VSLUMALUT0) {
        return SHARPNESS_SD_PKGAIN_VSLUMA;
    } else {
        SYS_LOGE("%s, error: param_type[%d] is not supported", __FUNCTION__, param_type);
        return -1;
    }
    return -1;
}

unsigned int CPQControl::GetSharpnessRegVal(int addr)
{
    char tmp_buf[128] = {0};
    char rval[32] = {0};

    sprintf(tmp_buf, "%s %x", "r", addr);
    pqWriteSys(AMVECM_PQ_REG_RW, tmp_buf);
    pqReadSys(AMVECM_PQ_REG_RW, rval, sizeof(rval));
    return atoi(rval);
}

int CPQControl::FactorySetSharpnessParams(source_input_param_t source_input_param, Sharpness_timing_e source_timing, int param_type, int val)
{
    int tmp_val;
    char tmp_buf[128];
    int addr;

    if (SetSharpnessParamsCheckVal(param_type, val) < 0) {
        SYS_LOGE("%s, error: val[%d] is out of range", __FUNCTION__, val);
        return -1;
    }

    addr = MatchSharpnessRegAddr(param_type, source_timing);
    tmp_val = mPQdb->PQ_GetSharpnessAdvancedParams(source_input_param, addr, source_timing);
    if (tmp_val == 0) {
        tmp_val = GetSharpnessRegVal(addr);
    }

    SYS_LOGD("%s, get value is %d, try to set value: %d\n", __FUNCTION__, tmp_val, val);

    switch (param_type) {
        case H_GAIN_HIGH: {
            SYS_LOGD("%s, type [%d], get val: %u\n", __FUNCTION__, param_type, tmp_val);
            tmp_val = tmp_val & (~(0xf << 28));
            SYS_LOGD("%s, type [%d], get val: %u\n", __FUNCTION__, param_type, tmp_val);
            tmp_val |= (val << 28);
            SYS_LOGD("%s, setting value : %u", __FUNCTION__, tmp_val);
            sprintf(tmp_buf, "%s %x %x", "w", SHARPNESS_SD_GAIN, tmp_val);
            pqWriteSys(AMVECM_PQ_REG_RW, tmp_buf);
            break;
        }
        case H_GAIN_LOW: {
            SYS_LOGD("%s, type [%d], get val: %d\n", __FUNCTION__, param_type, tmp_val);
            tmp_val &= (~(0xf << 12));
            tmp_val |= (val << 12);
            SYS_LOGD("%s, setting value : %d", __FUNCTION__, tmp_val);
            sprintf(tmp_buf, "%s %x %x", "w", SHARPNESS_SD_GAIN, tmp_val);
            pqWriteSys(AMVECM_PQ_REG_RW, tmp_buf);
            break;
        }
        case V_GAIN_HIGH: {
            tmp_val &= (~(0xf << 24));
            tmp_val |= (val << 24);
            sprintf(tmp_buf, "%s %x %x", "w", SHARPNESS_SD_GAIN, tmp_val);
            pqWriteSys(AMVECM_PQ_REG_RW, tmp_buf);
            break;
        }
        case V_GAIN_LOW: {
            tmp_val &= ~(0xf << 8);
            tmp_val |= (val << 8);
            sprintf(tmp_buf, "%s %x %x", "w", SHARPNESS_SD_GAIN, tmp_val);
            pqWriteSys(AMVECM_PQ_REG_RW, tmp_buf);
            break;
        }
        case D_GAIN_HIGH: {
            tmp_val &= ~(0xf << 20);
            tmp_val |= (val << 20);
            sprintf(tmp_buf, "%s %x %x", "w", SHARPNESS_SD_GAIN, tmp_val);
            pqWriteSys(AMVECM_PQ_REG_RW, tmp_buf);
            break;
        }
        case D_GAIN_LOW: {
            tmp_val &= ~(0xf << 4);
            tmp_val |= (val << 4);
            sprintf(tmp_buf, "%s %x %x", "w", SHARPNESS_SD_GAIN, tmp_val);
            pqWriteSys(AMVECM_PQ_REG_RW, tmp_buf);
            break;
        }
        case HP_DIAG_CORE: {
            tmp_val &= ~0x3f;
            tmp_val |= val;
            sprintf(tmp_buf, "%s %x %x", "w", SHARPNESS_SD_HP_DIAG_CORE, tmp_val);
            pqWriteSys(AMVECM_PQ_REG_RW, tmp_buf);
            break;
        }
        case BP_DIAG_CORE: {
            tmp_val &= ~0x3f;
            tmp_val |= val;
            sprintf(tmp_buf, "%s %x %x", "w", SHARPNESS_SD_BP_DIAG_CORE, tmp_val);
            pqWriteSys(AMVECM_PQ_REG_RW, tmp_buf);
            break;
        }
        case PKGAIN_VSLUMALUT7: {
            tmp_val &= ~(0xf << 28);
            tmp_val |= (val << 28);
            sprintf(tmp_buf, "%s %x %x", "w", SHARPNESS_SD_PKGAIN_VSLUMA, tmp_val);
            pqWriteSys(AMVECM_PQ_REG_RW, tmp_buf);
            break;
        }
        case PKGAIN_VSLUMALUT6: {
            tmp_val &= ~(0xf << 24);
            tmp_val |= (val << 24);
            sprintf(tmp_buf, "%s %x %x", "w", SHARPNESS_SD_PKGAIN_VSLUMA, tmp_val);
            pqWriteSys(AMVECM_PQ_REG_RW, tmp_buf);
            break;
        }
        case PKGAIN_VSLUMALUT5: {
            tmp_val &= ~(0xf << 20);
            tmp_val |= (val << 20);
            sprintf(tmp_buf, "%s %x %x", "w", SHARPNESS_SD_PKGAIN_VSLUMA, tmp_val);
            pqWriteSys(AMVECM_PQ_REG_RW, tmp_buf);
            break;
        }
        case PKGAIN_VSLUMALUT4: {
            tmp_val &= ~(0xf << 16);
            tmp_val |= (val << 16);
            sprintf(tmp_buf, "%s %x %x", "w", SHARPNESS_SD_PKGAIN_VSLUMA, tmp_val);
            pqWriteSys(AMVECM_PQ_REG_RW, tmp_buf);
            break;
        }
        case PKGAIN_VSLUMALUT3: {
            tmp_val &= ~(0xf << 12);
            tmp_val |= (val << 12);
            sprintf(tmp_buf, "%s %x %x", "w", SHARPNESS_SD_PKGAIN_VSLUMA, tmp_val);
            pqWriteSys(AMVECM_PQ_REG_RW, tmp_buf);
            break;
        }
        case PKGAIN_VSLUMALUT2: {
            tmp_val &= ~(0xf << 8);
            tmp_val |= (val << 8);
            sprintf(tmp_buf, "%s %x %x", "w", SHARPNESS_SD_PKGAIN_VSLUMA, tmp_val);
            pqWriteSys(AMVECM_PQ_REG_RW, tmp_buf);
            break;
        }
        case PKGAIN_VSLUMALUT1: {
            tmp_val &= ~(0xf << 4);
            tmp_val |= (val << 4);
            sprintf(tmp_buf, "%s %x %x", "w", SHARPNESS_SD_PKGAIN_VSLUMA, tmp_val);
            pqWriteSys(AMVECM_PQ_REG_RW, tmp_buf);
            break;
        }
        case PKGAIN_VSLUMALUT0: {
            tmp_val &= ~0xf;
            tmp_val |= val;
            sprintf(tmp_buf, "%s %x %x", "w", SHARPNESS_SD_PKGAIN_VSLUMA, tmp_val);
            pqWriteSys(AMVECM_PQ_REG_RW, tmp_buf);
            break;
        }
        default:
            break;
    }

    mPQdb->PQ_SetSharpnessAdvancedParams(source_input_param, addr, tmp_val, source_timing);

    return 0;
}

int CPQControl::FactoryGetSharpnessParams(source_input_param_t source_input_param, Sharpness_timing_e source_timing, int param_type)
{
    unsigned int rval, reg_val;
    int addr;

    addr = MatchSharpnessRegAddr(param_type, source_timing);
    reg_val = mPQdb->PQ_GetSharpnessAdvancedParams(source_input_param, addr, source_timing);
    if (reg_val == 0)
    {
        reg_val = GetSharpnessRegVal(addr);
    }

    switch (param_type) {
        case H_GAIN_HIGH: {
            SYS_LOGI("%s, type [%d], get val: %u\n", __FUNCTION__, param_type, reg_val);
            rval = (reg_val >> 28) & 0xf;
            SYS_LOGI("%s, type [%d], get val: %u\n", __FUNCTION__, param_type, rval);
            break;
        }
        case H_GAIN_LOW: {
            rval = (reg_val >> 12) & 0xf;
            break;
        }
        case V_GAIN_HIGH: {
            rval = (reg_val >> 24) & 0xf;
            break;
        }
        case V_GAIN_LOW: {
            rval = (reg_val >> 8) & 0xf;
            break;
        }
        case D_GAIN_HIGH: {
            rval = (reg_val >> 20) & 0xf;
            break;
        }
        case D_GAIN_LOW: {
            rval = (reg_val >> 4) & 0xf;
            break;
        }
        case HP_DIAG_CORE: {
            rval = reg_val & 0x3f;
            break;
        }
        case BP_DIAG_CORE: {
            rval = reg_val & 0x3f;
            break;
        }
        case PKGAIN_VSLUMALUT7: {
            rval = (reg_val >> 28) & 0xf;
            break;
        }
        case PKGAIN_VSLUMALUT6: {
            rval = (reg_val >> 24) & 0xf;
            break;
        }
        case PKGAIN_VSLUMALUT5: {
            rval = (reg_val >> 20) & 0xf;
            break;
        }
        case PKGAIN_VSLUMALUT4: {
            rval = (reg_val >> 16) & 0xf;
            break;
        }
        case PKGAIN_VSLUMALUT3: {
            rval = (reg_val >> 12) & 0xf;
            break;
        }
        case PKGAIN_VSLUMALUT2: {
            rval = (reg_val >> 8) & 0xf;
            break;
        }
        case PKGAIN_VSLUMALUT1: {
            rval = (reg_val >> 4) & 0xf;
            break;
        }
        case PKGAIN_VSLUMALUT0: {
            rval = reg_val & 0xf;
            break;
        }
        default: {
            rval = 0;
            break;
        }
    }

    SYS_LOGI("%s, type [%d], get reg_val: %u, ret val: %u\n", __FUNCTION__, param_type, reg_val, rval);
    return rval;
}

int CPQControl::SetEyeProtectionMode(tv_source_input_t source_input __unused, int enable, int is_save __unused)
{
    SYS_LOGI("%s: mode:%d!\n", __FUNCTION__, enable);
    int ret = -1;
    vpp_color_temperature_mode_t TempMode = (vpp_color_temperature_mode_t)GetColorTemperature();
    tcon_rgb_ogo_t param;
    memset(&param, 0, sizeof(tcon_rgb_ogo_t));
	ret = GetColorTemperatureParams(TempMode, &param);

    //RGB UI GAIN OFFSET
    RGB_UI_OFFSET params;
    memset(&params, 0, sizeof(RGB_UI_OFFSET));
    if (Cpq_GetColorTemperatureUser(TempMode, &params) == 0) {
        param.r_gain += params.r_gain_value;
        param.g_gain += params.g_gain_value;
        param.b_gain += params.b_gain_value;
        param.r_post_offset += params.r_offset_value;
        param.g_post_offset += params.g_offset_value;
        param.b_post_offset += params.b_offset_value;
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        if (enable) {
            param.b_gain /= 2;
        }
        ret = Cpq_SetRGBOGO(&param);
        mSSMAction->SSMSaveEyeProtectionMode(enable);
        SYS_LOGI("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::GetEyeProtectionMode(tv_source_input_t source_input __unused)
{
    int mode = -1;

    if (mSSMAction->SSMReadEyeProtectionMode(&mode) < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
        return -1;
    } else {
        SYS_LOGI("%s: mode is %d!\n",__FUNCTION__, mode);
        return mode;
    }
}

int CPQControl::SetFlagByCfg(void)
{
    pq_ctrl_t pqControlVal;
    memset(&pqControlVal, 0x0, sizeof(pq_ctrl_t));
    const char *config_value;

    config_value = mPQConfigFile->GetString(CFG_SECTION_PQ, CFG_BIG_SMALL_DB_ENABLE, "enable");
    if (strcmp(config_value, "enable") == 0) {
        mbCpqCfg_separate_db_enable = true;
    } else {
        mbCpqCfg_separate_db_enable = false;
    }

    config_value = mPQConfigFile->GetString(CFG_SECTION_PQ, CFG_DI_ENABLE, "enable");
    if (strcmp(config_value, "enable") == 0) {
        mbCpqCfg_di_enable = true;
    } else {
        mbCpqCfg_di_enable = false;
    }

    config_value = mPQConfigFile->GetString(CFG_SECTION_PQ, CFG_MCDI_ENABLE, "enable");
    if (strcmp(config_value, "enable") == 0) {
        mbCpqCfg_mcdi_enable = true;
        pqWriteSys(DI_PARAMETERS_MCEN_MODE, "1");
    } else {
        mbCpqCfg_mcdi_enable = false;
        pqWriteSys(DI_PARAMETERS_MCEN_MODE, "0");
    }

    config_value = mPQConfigFile->GetString(CFG_SECTION_PQ, CFG_DEBLOCK_ENABLE, "enable");
    if (strcmp(config_value, "enable") == 0) {
        mbCpqCfg_deblock_enable = true;
        pqWriteSys(DI_PARAMETERS_DNR_EN, "13");//bit2~bit3
    } else {
        mbCpqCfg_deblock_enable = false;
        pqWriteSys(DI_PARAMETERS_DNR_EN, "1");
    }

    config_value = mPQConfigFile->GetString(CFG_SECTION_PQ, CFG_DEMOSQUITO_ENABLE, "enable");
    if (strcmp(config_value, "enable") == 0) {
        mbCpqCfg_demoSquito_enable = true;
        pqWriteSys(DI_PARAMETERS_DNR_DM_EN, "1");//bit0
    } else {
        mbCpqCfg_demoSquito_enable = false;
        pqWriteSys(DI_PARAMETERS_DNR_DM_EN, "0");
    }

    config_value = mPQConfigFile->GetString(CFG_SECTION_PQ, CFG_NOISEREDUCTION_ENABLE, "enable");
    if (strcmp(config_value, "enable") == 0) {
        mbCpqCfg_nr_enable = true;
        pqWriteSys(DI_PARAMETERS_NR2_EN, "1");
    } else {
        mbCpqCfg_nr_enable = false;
        pqWriteSys(DI_PARAMETERS_NR2_EN, "0");
    }

    config_value = mPQConfigFile->GetString(CFG_SECTION_PQ, CFG_SHARPNESS0_ENABLE, "disable");
    if (strcmp(config_value, "enable") == 0) {
        mbCpqCfg_sharpness0_enable = true;
        pqControlVal.sharpness0_en = 1;
    } else {
        mbCpqCfg_sharpness0_enable = false;
        pqControlVal.sharpness0_en = 0;
    }

    config_value = mPQConfigFile->GetString(CFG_SECTION_PQ, CFG_SHARPNESS1_ENABLE, "disable");
    if (strcmp(config_value, "enable") == 0) {
        mbCpqCfg_sharpness1_enable = true;
        pqControlVal.sharpness1_en = 1;
    } else {
        mbCpqCfg_sharpness1_enable = false;
        pqControlVal.sharpness1_en = 0;
    }

    config_value = mPQConfigFile->GetString(CFG_SECTION_PQ, CFG_SHARPNESSPI_ENABLE, "disable");
    if (strcmp(config_value, "enable") == 0) {
        mbCpqCfg_sharpnesspi_enable = true;
        //pqControlVal.sharpnesspi_en = 1;
    } else {
        mbCpqCfg_sharpnesspi_enable = false;
        //pqControlVal.sharpnesspi_en = 0;
    }

    config_value = mPQConfigFile->GetString(CFG_SECTION_PQ, CFG_DNLP_ENABLE, "enable");
    if (strcmp(config_value, "enable") == 0) {
        mbCpqCfg_dnlp_enable = true;
        pqControlVal.dnlp_en = 1;
    } else {
        mbCpqCfg_dnlp_enable = false;
        pqControlVal.dnlp_en = 0;
    }

    config_value = mPQConfigFile->GetString(CFG_SECTION_PQ, CFG_CM2_ENABLE, "enable");
    if (strcmp(config_value, "enable") == 0) {
        mbCpqCfg_cm2_enable = true;
        pqControlVal.cm_en = 1;
    } else {
        mbCpqCfg_cm2_enable = false;
        pqControlVal.cm_en = 0;
    }

    config_value = mPQConfigFile->GetString(CFG_SECTION_PQ, CFG_AMVECM_BASCI_ENABLE, "enable");
    if (strcmp(config_value, "enable") == 0) {
        mbCpqCfg_amvecm_basic_enable = true;
        pqControlVal.vadj1_en = 1;
    } else {
        mbCpqCfg_amvecm_basic_enable = false;
        pqControlVal.vadj1_en = 0;
    }

    config_value = mPQConfigFile->GetString(CFG_SECTION_PQ, CFG_AMVECM_BASCI_WITHOSD_ENABLE, "enable");
    if (strcmp(config_value, "enable") == 0) {
        mbCpqCfg_amvecm_basic_withOSD_enable = true;
        pqControlVal.vadj2_en = 1;
    } else {
        mbCpqCfg_amvecm_basic_withOSD_enable = false;
        pqControlVal.vadj2_en = 0;
    }

    config_value = mPQConfigFile->GetString(CFG_SECTION_PQ, CFG_CONTRAST_RGB_ENABLE, "enable");
    if (strcmp(config_value, "enable") == 0) {
        mbCpqCfg_contrast_rgb_enable = true;
        pqControlVal.vd1_ctrst_en = 1;
    } else {
        mbCpqCfg_contrast_rgb_enable = false;
        pqControlVal.vd1_ctrst_en = 0;
    }

    config_value = mPQConfigFile->GetString(CFG_SECTION_PQ, CFG_CONTRAST_RGB_WITHOSD_ENABLE, "enable");
    if (strcmp(config_value, "enable") == 0) {
        mbCpqCfg_contrast_rgb_withOSD_enable = true;
        pqControlVal.post_ctrst_en = 1;
    } else {
        mbCpqCfg_contrast_rgb_withOSD_enable = false;
        pqControlVal.post_ctrst_en = 0;
    }

    config_value = mPQConfigFile->GetString(CFG_SECTION_PQ, CFG_WHITEBALANCE_ENABLE, "enable");
    if (strcmp(config_value, "enable") == 0) {
        mbCpqCfg_whitebalance_enable = true;
        pqControlVal.wb_en = 1;
    } else {
        mbCpqCfg_whitebalance_enable = false;
        pqControlVal.wb_en = 0;
    }

    config_value = mPQConfigFile->GetString(CFG_SECTION_PQ, CFG_GAMMA_ENABLE, "enable");
    if (strcmp(config_value, "enable") == 0) {
        mbCpqCfg_gamma_enable = true;
        pqControlVal.gamma_en = 1;
    } else {
        mbCpqCfg_gamma_enable = false;
        pqControlVal.gamma_en = 0;
    }

    config_value = mPQConfigFile->GetString(CFG_SECTION_PQ, CFG_LOCAL_CONTRAST_ENABLE, "enable");
    if (strcmp(config_value, "enable") == 0) {
        mbCpqCfg_local_contrast_enable = true;
        pqControlVal.lc_en = 1;
    } else {
        mbCpqCfg_local_contrast_enable = false;
        pqControlVal.lc_en = 0;
    }

    config_value = mPQConfigFile->GetString(CFG_SECTION_PQ, CFG_BLACKEXTENSION_ENABLE, "enable");
    if (strcmp(config_value, "enable") == 0) {
        mbCpqCfg_blackextension_enable = true;
        pqControlVal.black_ext_en = 1;
    } else {
        mbCpqCfg_blackextension_enable = false;
        pqControlVal.black_ext_en = 0;
    }

    config_value = mPQConfigFile->GetString(CFG_SECTION_PQ, CFG_XVYCC_ENABLE, "disable");
    if (strcmp(config_value, "enable") == 0) {
        mbCpqCfg_xvycc_enable = true;
    } else {
        mbCpqCfg_xvycc_enable = false;
    }

    config_value = mPQConfigFile->GetString(CFG_SECTION_PQ, CFG_DISPLAY_OVERSCAN_ENABLE, "enable");
    if (strcmp(config_value, "enable") == 0) {
        mbCpqCfg_display_overscan_enable = true;
    } else {
        mbCpqCfg_display_overscan_enable = false;
    }

    config_value = mPQConfigFile->GetString(CFG_SECTION_HDMI, CFG_HDMI_OUT_WITH_FBC_ENABLE, "disable");
    if (strcmp(config_value, "enable") == 0) {
        mbCpqCfg_hdmi_out_with_fbc_enable = true;
    } else {
        mbCpqCfg_hdmi_out_with_fbc_enable = false;
    }

    config_value = mPQConfigFile->GetString(CFG_SECTION_PQ, CFG_PQ_PARAM_CHECK_SOURCE_ENABLE, "enable");
    if (strcmp(config_value, "enable") == 0) {
        mbCpqCfg_pq_param_check_source_enable = true;
    } else {
        mbCpqCfg_pq_param_check_source_enable = false;
    }

    config_value = mPQConfigFile->GetString(CFG_SECTION_PQ, CFG_AI_ENABLE, "disable");
    if (strcmp(config_value, "enable") == 0) {
        mbCpqCfg_ai_enable = true;
    } else {
        mbCpqCfg_ai_enable = false;
    }

    config_value = mPQConfigFile->GetString(CFG_SECTION_PQ, CFG_SMOOTHPLUS_ENABLE, "disable");
    if (strcmp(config_value, "enable") == 0) {
        mbCpqCfg_smoothplus_enable = true;
    } else {
        mbCpqCfg_smoothplus_enable = false;
    }

    config_value = mPQConfigFile->GetString(CFG_SECTION_PQ, CFG_HDRTMO_ENABLE, "disable");
    if (strcmp(config_value, "enable") == 0) {
        mbCpqCfg_hdrtmo_enable = true;
    } else {
        mbCpqCfg_hdrtmo_enable = false;
    }

    config_value = mPQConfigFile->GetString(CFG_SECTION_PQ, CFG_MEMC_ENABLE, "enable");
    if (strcmp(config_value, "enable") == 0) {
        mbCpqCfg_memc_enable = true;
    } else {
        mbCpqCfg_memc_enable = false;
    }

    config_value = mPQConfigFile->GetString(CFG_SECTION_PQ, CFG_AAD_ENABLE, "disable");
    if (strcmp(config_value, "enable") == 0) {
        mbCpqCfg_aad_enable = true;
    } else {
        mbCpqCfg_aad_enable = false;
    }

    config_value = mPQConfigFile->GetString(CFG_SECTION_PQ, CFG_CABC_ENABLE, "disable");
    if (strcmp(config_value, "enable") == 0) {
        mbCpqCfg_cabc_enable = true;
    } else {
        mbCpqCfg_cabc_enable = false;
    }

    config_value = mPQConfigFile->GetString(CFG_SECTION_PQ, CFG_BLACK_BLUE_CHROMA_DB_ENABLE, "enable");
    if (strcmp(config_value, "enable") == 0) {
        mbCpqCfg_separate_black_blue_chorma_db_enable = true;
    } else {
        mbCpqCfg_separate_black_blue_chorma_db_enable = false;
    }

    config_value = mPQConfigFile->GetString(CFG_SECTION_PQ, CFG_BLUESTRETCH_ENABLE, "enable");
    if (strcmp(config_value, "enable") == 0) {
        mbCpqCfg_bluestretch_enable = true;
    } else {
        mbCpqCfg_bluestretch_enable = false;
    }

    config_value = mPQConfigFile->GetString(CFG_SECTION_PQ, CFG_CHROMACORING_ENABLE, "enable");
    if (strcmp(config_value, "enable") == 0) {
        mbCpqCfg_chroma_coring_enable = true;
    } else {
        mbCpqCfg_chroma_coring_enable = false;
    }

    config_value = mPQConfigFile->GetString(CFG_SECTION_PQ, CFG_LOCALDIMMING_ENABLE, "enable");
    if (strcmp(config_value, "enable") == 0) {
        mbCpqCfg_LocalDimming_enable = true;
    } else {
        mbCpqCfg_LocalDimming_enable = false;
    }

    config_value = mPQConfigFile->GetString(CFG_SECTION_PQ, CFG_AISR_ENABLE, "disable");
    if (strcmp(config_value, "enable") == 0) {
        mbCpqCfg_aisr_enable = true;
    } else {
        mbCpqCfg_aisr_enable = false;
    }

    config_value = mPQConfigFile->GetString(CFG_SECTION_PQ, CFG_AICOLOR_ENABLE, "disable");
    if (strcmp(config_value, "enable") == 0) {
        mbCpqCfg_aicolor_enable = true;
    } else {
        mbCpqCfg_aicolor_enable = false;
    }

    config_value = mPQConfigFile->GetString(CFG_SECTION_PQ, CFG_NEW_PICTURE_MODE_ENABLE, "disable");
    if (strcmp(config_value, "enable") == 0) {
        mbCpqCfg_new_picture_mode_enable = true;
    } else {
        mbCpqCfg_new_picture_mode_enable = false;
    }

    vpp_pq_ctrl_t amvecmConfigVal;
    amvecmConfigVal.length = 14;//this is the count of pq_ctrl_s option
    amvecmConfigVal.ptr = (long long)&pqControlVal;
    int ret = VPPDeviceIOCtl(AMVECM_IOC_S_PQ_CTRL, &amvecmConfigVal);
    if (ret < 0) {
        SYS_LOGE("%s error: %s!\n", __FUNCTION__, strerror(errno));
    }

    Cpq_SetVadjEnableStatus(pqControlVal.vadj1_en, pqControlVal.vadj2_en);

    return 0;
}

int CPQControl::SetPLLValues(source_input_param_t source_input_param)
{
    am_regs_t regs;
    int ret = 0;
    if (mPQdb->PQ_GetPLLParams (source_input_param, &regs ) == 0 ) {
        ret = AFEDeviceIOCtl(TVIN_IOC_LOAD_REG, &regs);
        if ( ret < 0 ) {
            SYS_LOGE ( "%s error(%s)!\n", __FUNCTION__, strerror(errno));
            return -1;
        }
    } else {
        SYS_LOGE ( "%s, PQ_GetPLLParams failed!\n", __FUNCTION__ );
        return -1;
    }

    return 0;
}

int CPQControl::SetCVD2Values(void)
{
    am_regs_t regs;
    int ret = mPQdb->PQ_GetCVD2Params ( mCurrentSourceInputInfo, &regs);
    if (ret < 0) {
        SYS_LOGE ( "%s, PQ_GetCVD2Params failed!\n", __FUNCTION__);
    } else {
        ret = AFEDeviceIOCtl(TVIN_IOC_LOAD_REG, &regs);
        if ( ret < 0 ) {
            SYS_LOGE ( "%s: ioctl failed!\n", __FUNCTION__);
        }
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n", __FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n", __FUNCTION__);
    }

    return ret;
}

int CPQControl::Cpq_SSMReadNTypes(int id, int data_len, int offset)
{
    int value = 0;
    int ret = 0;

    ret = mSSMAction->SSMReadNTypes(id, data_len, &value, offset);

    if (ret < 0) {
        SYS_LOGE("Cpq_SSMReadNTypes, error(%s).\n", strerror ( errno ) );
        return -1;
    } else {
        return value;
    }
}

int CPQControl::Cpq_SSMWriteNTypes(int id, int data_len, int data_buf, int offset)
{
    int ret = 0;
    ret = mSSMAction->SSMWriteNTypes(id, data_len, &data_buf, offset);

    if (ret < 0) {
        SYS_LOGE("Cpq_SSMWriteNTypes, error(%s).\n", strerror ( errno ) );
    }

    return ret;
}

int CPQControl::Cpq_GetSSMActualAddr(int id)
{
    return mSSMAction->GetSSMActualAddr(id);
}

int CPQControl::Cpq_GetSSMActualSize(int id)
{
    return mSSMAction->GetSSMActualSize(id);
}

int CPQControl::Cpq_SSMRecovery(void)
{
    return mSSMAction->SSMRecovery();
}

int CPQControl::Cpq_GetSSMStatus()
{
    return mSSMAction->GetSSMStatus();
}

hdr_type_t CPQControl::Cpq_GetSourceHDRType(source_input_param_t source_input_param)
{
    hdr_type_t newHdrType = HDR_TYPE_NONE;
    if ((source_input_param.source_input == SOURCE_MPEG)
        ||(source_input_param.source_input == SOURCE_DTV)) {
        if (!mbVideoIsPlaying) {
            newHdrType = HDR_TYPE_SDR;
        } else {
            char buf[32] = {0};
            int ret = pqReadSys(VIDEO_POLL_PRIMARY_SRC_FMT, buf, sizeof(buf));
            if (ret < 0) {
                newHdrType = HDR_TYPE_SDR;
                SYS_LOGE("%s error: %s\n", __FUNCTION__, strerror(errno));
            } else {
                if (0 == strcmp(buf, "src_fmt = SDR")) {
                    newHdrType = HDR_TYPE_SDR;
                } else if (0 == strcmp(buf, "src_fmt = HDR10")) {
                    newHdrType = HDR_TYPE_HDR10;
                } else if (0 == strcmp(buf, "src_fmt = HDR10+")) {
                    newHdrType = HDR_TYPE_HDR10PLUS;
                } else if (0 == strcmp(buf, "src_fmt = HDR10 prime")) {
                    newHdrType = HDR_TYPE_PRIMESL;
                } else if (0 == strcmp(buf, "src_fmt = HLG")) {
                    newHdrType = HDR_TYPE_HLG;
                } else if (0 == strcmp(buf, "src_fmt = Dolby Vison")) {
                    newHdrType = HDR_TYPE_DOVI;
                } else if (0 == strcmp(buf, "src_fmt = MVC")) {
                    newHdrType = HDR_TYPE_MVC;
                } else {
                    SYS_LOGE("%s: invalid hdr type:%s\n", __FUNCTION__, buf);
                    newHdrType = HDR_TYPE_SDR;
                }
            }
        }
    } else if ((source_input_param.source_input == SOURCE_HDMI1)
             || (source_input_param.source_input == SOURCE_HDMI2)
             || (source_input_param.source_input == SOURCE_HDMI3)
             || (source_input_param.source_input == SOURCE_HDMI4)) {
        int signalRange                  = (mHdmiHdrInfo >> 29) & 0x1;
        int signalColorPrimaries         = (mHdmiHdrInfo >> 16) & 0xff;
        int signalTransferCharacteristic = (mHdmiHdrInfo >> 8)  & 0xff;
        int dvFlag                       = (mHdmiHdrInfo >> 30) & 0x1;
        SYS_LOGD("%s: signalRange= 0x%x, signalColorPrimaries = 0x%x, signalTransferCharacteristic = 0x%x, dvFlag = 0x%x\n",
                __FUNCTION__, signalRange, signalColorPrimaries, signalTransferCharacteristic, dvFlag);
        if (((signalTransferCharacteristic == 0xe) || (signalTransferCharacteristic == 0x12))
            && (signalColorPrimaries == 0x9)) {
            newHdrType = HDR_TYPE_HLG;
        } else if ((signalTransferCharacteristic == 0x30) && (signalColorPrimaries == 0x9)) {
            newHdrType = HDR_TYPE_HDR10PLUS;
        } else if ((signalTransferCharacteristic == 0x10) || (signalColorPrimaries == 0x9)) {
            newHdrType = HDR_TYPE_HDR10;
        } else if (dvFlag == 0x1) {
            newHdrType = HDR_TYPE_DOVI;
        } else {
            newHdrType = HDR_TYPE_SDR;
        }
    } else if ((source_input_param.source_input == SOURCE_TV)
             || (source_input_param.source_input == SOURCE_AV1)
             || (source_input_param.source_input == SOURCE_AV2)) {
        if (source_input_param.sig_fmt != TVIN_SIG_FMT_NULL) {
            newHdrType = HDR_TYPE_SDR;
        } else {
           newHdrType = HDR_TYPE_NONE;
        }
    } else {
        newHdrType = HDR_TYPE_NONE;
    }

    SYS_LOGD("%s: newHdrType:%d, source_input:%d\n", __FUNCTION__, newHdrType, source_input_param.source_input);

    return newHdrType;
}

int CPQControl::SetCurrentSourceInputInfo(source_input_param_t source_input_param)
{
    AutoMutex _l( mLock );
    SYS_LOGD("%s: param_check_source_enable = %d\n", __FUNCTION__, mbCpqCfg_pq_param_check_source_enable);

    SYS_LOGD("%s:new source info: source=%d,sigFmt=%d(0x%x)\n", __FUNCTION__,
                                                                 source_input_param.source_input,
                                                                 source_input_param.sig_fmt,
                                                                 source_input_param.sig_fmt);


    //get hdr type
    hdr_type_t newHdrType = HDR_TYPE_NONE;
    newHdrType = Cpq_GetSourceHDRType(source_input_param);

    //notify hdr event to framework
    if (mCurrentHdrType != newHdrType) {
        mCurrentHdrType = newHdrType;
        if (mNotifyListener != NULL) {
            SYS_LOGD("%s: send hdr event, info is %d\n", __FUNCTION__, mCurrentHdrType);
            mNotifyListener->onHdrInfoChange(mCurrentHdrType);
        } else {
            SYS_LOGE("%s: mNotifyListener is NULL\n", __FUNCTION__);
        }
    }

    if ((newHdrType == HDR_TYPE_SDR) || (newHdrType == HDR_TYPE_NONE)) {
        mPQdb->mHdrStatus = false;
    } else {
        mPQdb->mHdrStatus = true;
    }
    SYS_LOGD("%s: mCurrentHdrType is %d, hdrStatus is %d!\n", __FUNCTION__, mCurrentHdrType, mPQdb->mHdrStatus);

    //check pq src timming
    pq_src_param_t PqSrcTim;
    PqSrcTim.pq_source_input = source_input_param.source_input;
    PqSrcTim.pq_sig_fmt = CheckPQTimming(newHdrType);
    SYS_LOGD("%s:PqSrcTim.pq_source_input is %d  PqSrcTim.pq_sig_fmt is %d\n", __FUNCTION__, PqSrcTim.pq_source_input, PqSrcTim.pq_sig_fmt);

    CheckOutPutMode(source_input_param.source_input);

    //check env hdr policy (always hdr or adaptive hdr)
    getHdrPolicy();

    if ((mCurrentSourceInputInfo.source_input != source_input_param.source_input) ||
         (mCurrentSourceInputInfo.sig_fmt != source_input_param.sig_fmt) ||
         (mCurrentSourceInputInfo.trans_fmt != source_input_param.trans_fmt) ||
         (mCurrentHdrStatus != mPQdb->mHdrStatus) ||
         (mCurrentOutputType != mPQdb->mOutPutType) ||
         (mCurrentPqSource.pq_source_input != PqSrcTim.pq_source_input) ||
         (mCurrentPqSource.pq_sig_fmt != PqSrcTim.pq_sig_fmt) ||
         (mCurrentNodeNumber != mPQdb->node_number)) {
        mCurrentSourceInputInfo.source_input = source_input_param.source_input;
        mCurrentSourceInputInfo.sig_fmt = source_input_param.sig_fmt;
        mCurrentSourceInputInfo.trans_fmt = source_input_param.trans_fmt;
        mCurrentHdrStatus = mPQdb->mHdrStatus;
        mCurrentOutputType = mPQdb->mOutPutType;
        mCurrentPqSource.pq_source_input = PqSrcTim.pq_source_input;
        mCurrentPqSource.pq_sig_fmt = PqSrcTim.pq_sig_fmt;
        mCurrentNodeNumber = mPQdb->node_number;

        SYS_LOGD("%s:mCurrentPqSource is %d  mCurrentPqTimming is %d\n", __FUNCTION__, mCurrentPqSource.pq_source_input, mCurrentPqSource.pq_sig_fmt);

        if (mbCpqCfg_pq_param_check_source_enable) {
            mSourceInputForSaveParam = mCurrentSourceInputInfo.source_input;
        } else {
            mSourceInputForSaveParam = SOURCE_MPEG;
        }

        if (mCurrentSourceInputInfo.sig_fmt != TVIN_SIG_FMT_NULL) {
            LoadPQSettings();
        } else {
            vpp_display_mode_t display_mode = (vpp_display_mode_t)GetDisplayMode();
            SetDisplayMode(display_mode, 1);
        }
    } else {
        SYS_LOGD("%s: same signal, no need set!\n", __FUNCTION__);
    }
    return 0;
}

source_input_param_t CPQControl::GetCurrentSourceInputInfo()
{
    AutoMutex _l( mLock );
    return mCurrentSourceInputInfo;
}

int CPQControl::GetRGBPattern() {
    char value[33] = {0};
    pqReadSys(VIDEO_RGB_SCREEN, value, (sizeof(value)-1));
    value[32] = '\0';
    return strtol(value, NULL, 10);
}

int CPQControl::SetRGBPattern(int r, int g, int b) {
    int value = ((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff);
    char str[32] = {0};
    sprintf(str, "%d", value);
    int ret = pqWriteSys(VIDEO_RGB_SCREEN, str);
    return ret;
}

int CPQControl::FactorySetDDRSSC(int step) {
    if (step < 0 || step > 5) {
        SYS_LOGE ("%s, step = %d is too long", __FUNCTION__, step);
        return -1;
    }

    return mSSMAction->SSMSaveDDRSSC(step);
}

int CPQControl::FactoryGetDDRSSC() {
    unsigned char data = 0;
    mSSMAction->SSMReadDDRSSC(&data);
    return data;
}

int CPQControl::SetLVDSSSC(int step) {

    SYS_LOGI("%s: %d\n", __FUNCTION__, step);

    char buf[32] = {0};
    sprintf(buf, "%d", step);
    int ret = pqWriteSys(LCD_SS, buf);
    return ret;
}

int CPQControl::FactorySetLVDSSSC(int step)
{
    if (step > 4)
        step = 4;

    aml_lcd_ss_ctl_t pData;
    memset(&pData, 0, sizeof(aml_lcd_ss_ctl_t));

    pData.level = step;

    int data[3] = {0, 0, 0};// level frep mode
    data[0] = pData.level;
    data[1] = pData.freq;
    data[2] = pData.mode;

    mSSMAction ->SSMSaveLVDSSSC(data);

    if (AML_HAL_LCD_SetSS((HAL_lcd_ss_ctl_t *)&pData) != API_OK) {
        SYS_LOGE("%s: fail\n", __FUNCTION__);
        return -1;
    }

    return 0;
}

int CPQControl::FactoryGetLVDSSSC()
{
    int data[3] = {0, 0, 0};

    mSSMAction ->SSMReadLVDSSSC(data);

    int level = data[0];

    return level;
}

int CPQControl::SetLCDPowerCtrl(int state)
{
    unsigned int onoff = state;

    if (AML_HAL_LCD_SetPowerCtrl(onoff) != API_OK) {
        SYS_LOGE("%s: fail\n", __FUNCTION__);
        return -1;
    }

    return 0;
}

int CPQControl::SetLCDMuteCtrl(int state)
{
    unsigned int onoff = state;

    if (AML_HAL_LCD_SetMuteCtrl(onoff) != API_OK) {
        SYS_LOGE("%s: fail\n", __FUNCTION__);
        return -1;
    }

    return 0;
}

int CPQControl::SetGrayPattern(int value) {
    if (value < 0) {
        value = 0;
    } else if (value > 255) {
        value = 255;
    }
    value = value << 16 | 0x8080;

    SYS_LOGI("%s: %d\n", __FUNCTION__, value);

    char val[64] = {0};
    sprintf(val, "%d", value);
    return pqWriteSys(VIDEO_TEST_SCREEN, val);
}

int CPQControl::GetGrayPattern() {
    int value = 0;
    char temp[9];
    memset(temp, 0, sizeof(temp));
    int ret = pqReadSys(VIDEO_TEST_SCREEN, temp, (sizeof(temp)-1));
    temp[8] = '\0';
    value = strtol(temp, NULL, 16);

    if (value < 0) {
        return 0;
    } else {
        value = value >> 16;
        if (value > 255) {
            value = 255;
        }
        return value;
    }
}

int CPQControl::SetHDRMode(int mode)
{
    int ret = -1;
    if ((mCurrentSourceInputInfo.source_input == SOURCE_MPEG) ||
       ((mCurrentSourceInputInfo.source_input >= SOURCE_HDMI1) && mCurrentSourceInputInfo.source_input <= SOURCE_HDMI4)) {
        ret = VPPDeviceIOCtl(AMVECM_IOC_S_CSCTYPE, &mode);
        if (ret < 0) {
            SYS_LOGE("%s error: %s!\n", __FUNCTION__, strerror(errno));
        }
    } else {
        SYS_LOGE("%s: Current source no hdr status!\n", __FUNCTION__);
    }

    return ret;
}

int CPQControl::GetHDRMode()
{
    ve_csc_type_t mode = VPP_MATRIX_NULL;
    if ((mCurrentSourceInputInfo.source_input == SOURCE_MPEG) ||
       ((mCurrentSourceInputInfo.source_input >= SOURCE_HDMI1) && mCurrentSourceInputInfo.source_input <= SOURCE_HDMI4)) {
        int ret = VPPDeviceIOCtl(AMVECM_IOC_G_CSCTYPE, &mode);
        if (ret < 0) {
            SYS_LOGE("%s error: %s!\n", __FUNCTION__, strerror(errno));
            mode = VPP_MATRIX_NULL;
        } else {
            SYS_LOGI("%s: mode is %d\n", __FUNCTION__, mode);
        }
    } else {
        SYS_LOGI("%s: Current source no hdr status!\n", __FUNCTION__);
    }

    return mode;
}

int CPQControl::GetSourceHDRType()
{
    SYS_LOGI("%s: type is %d\n", __FUNCTION__, mCurrentHdrType);
    return mCurrentHdrType;
}

void CPQControl::setHdrInfoListener(const sp<PqNotify>& listener) {
    mNotifyListener = listener;
}

void CPQControl::GetChipVersionInfo(char* chip_version) {
    database_attribute_t dbAttribute;
    mPQdb->PQ_GetDataBaseAttribute(&dbAttribute);
    if ((dbAttribute.ChipVersion.c_str() == NULL) || (dbAttribute.ChipVersion.length() == 0)) {
        SYS_LOGI("%s: ChipVersion is null\n", __FUNCTION__);
        std::strcpy(chip_version, " ");
    } else {
        std::string TempString = std::string(dbAttribute.ChipVersion.c_str());
        char* tempstr = new char[TempString.length() + 1];
        std::strcpy(tempstr, TempString.c_str());
        chip_version = strtok(tempstr, "_");

        SYS_LOGI("%s: versionStr is %s\n", __FUNCTION__, chip_version);
        delete []tempstr;
    }
}

tvpq_databaseinfo_t CPQControl::GetDBVersionInfo(db_name_t name) {
    bool val = false;
    String8 tmpToolVersion, tmpProjectVersion, tmpGenerateTime;
    tvpq_databaseinfo_t pqdatabaseinfo_t;
    memset(&pqdatabaseinfo_t, 0, sizeof(pqdatabaseinfo_t));
    switch (name) {
        case DB_NAME_PQ:
            val = mPQdb->PQ_GetPqVersion(tmpToolVersion, tmpProjectVersion, tmpGenerateTime);
            break;
        case DB_NAME_OVERSCAN:
            val = mpOverScandb->GetOverScanDbVersion(tmpToolVersion, tmpProjectVersion, tmpGenerateTime);
            break;
        default:
            val = mPQdb->PQ_GetPqVersion(tmpToolVersion, tmpProjectVersion, tmpGenerateTime);
            break;
    }

    if (val) {
        if (strlen(tmpToolVersion.c_str()) < sizeof(pqdatabaseinfo_t.ToolVersion)/sizeof(char)) {
            strcpy(pqdatabaseinfo_t.ToolVersion, tmpToolVersion.c_str());
        }
        if (strlen(tmpProjectVersion.c_str()) < sizeof(pqdatabaseinfo_t.ProjectVersion)/sizeof(char)) {
            strcpy(pqdatabaseinfo_t.ProjectVersion, tmpProjectVersion.c_str());
        }
        if (strlen(tmpGenerateTime.c_str()) < sizeof(pqdatabaseinfo_t.GenerateTime)/sizeof(char)) {
            strcpy(pqdatabaseinfo_t.GenerateTime, tmpGenerateTime.c_str());
        }
    }

    return pqdatabaseinfo_t;
}

int CPQControl::SetCurrentHdrInfo (int hdrInfo)
{
    int ret = 0;
    SYS_LOGI("%s: mHdmiHdrInfo:%d new hdrInfo:%d\n", __FUNCTION__, mHdmiHdrInfo, hdrInfo);

    if (mHdmiHdrInfo != (unsigned int)hdrInfo) {
        mHdmiHdrInfo = (unsigned int)hdrInfo;
        //get hdr type
        hdr_type_t newHdrType = HDR_TYPE_NONE;
        newHdrType            = Cpq_GetSourceHDRType(mCurrentSourceInputInfo);

        //notify hdr event to framework
        if (mCurrentHdrType != newHdrType) {
            mCurrentHdrType = newHdrType;
            if (mNotifyListener != NULL) {
                SYS_LOGI("%s: send hdr event, info is %d\n", __FUNCTION__, mCurrentHdrType);
                mNotifyListener->onHdrInfoChange(mCurrentHdrType);
            } else {
                SYS_LOGE("%s: mNotifyListener is NULL\n", __FUNCTION__);
            }
        }
    } else {
        SYS_LOGI("%s: same HDR info\n", __FUNCTION__);
    }

    return ret;
}

int CPQControl::SetCurrentAspectRatioInfo(tvin_aspect_ratio_e aspectRatioInfo)
{
    int ret = 0;
    if (mCurrentAfdInfo != aspectRatioInfo) {
        mCurrentAfdInfo = aspectRatioInfo;
        SYS_LOGD("%s mCurrentAfdInfo:%d\n", __FUNCTION__, mCurrentAfdInfo);
        vpp_display_mode_t display_mode = (vpp_display_mode_t)GetDisplayMode();
        ret = SetDisplayMode(display_mode, 1);
    } else {
        SYS_LOGI("%s: same AFD info\n", __FUNCTION__);
    }

    return ret;
}

int CPQControl::SetDtvKitSourceEnable(bool isEnable)
{
    SYS_LOGI("%s: isEnable:%d\n", __FUNCTION__, isEnable);

    mbDtvKitEnable = isEnable;
    return 0;
}

//AI
void CPQControl::AipqInit(void)
{
    SYS_LOGI("%s\n", __FUNCTION__);

    if (GetAipqEnable() == 1) {
        enableAipq(true);
    }
}

bool CPQControl::hasAipqFunc(void)
{
    int ret = -1;
    SYS_LOGI("%s, hasAipqFunc\n", __FUNCTION__);
    if (mbCpqCfg_ai_enable && isFileExist(pqSysWrite->getSysNode(AIPQ_PARAMETERS_UVM_OPEN))) {
        ret = true;
    } else {
        ret = false;
    }

    SYS_LOGI("%s, has aipq or not:%d\n", __FUNCTION__, ret);
    return ret;
}

int CPQControl::SetAipqEnable(bool isEnable)
{

    SYS_LOGI("%s, SetAipqEnable isEnable:%d\n", __FUNCTION__, isEnable);
    enableAipq(isEnable);
    mSSMAction->SSMSaveAipqEnableVal(isEnable ? 1 : 0);
    return 0;
}

int CPQControl::GetAipqEnable(void)
{
    int data = 0;
    SYS_LOGI("%s, GetAipqEnable\n", __FUNCTION__);
    mSSMAction->SSMReadAipqEnableVal(&data);

    if (data < 0 || data > 1) {
        data = 0;
    }
    return data;
}

void CPQControl::enableAipq(bool isEnable)
{
    SYS_LOGI("%s, enableAipq\n", __FUNCTION__);
    pqWriteSys(DECODER_COMMON_PARAMETERS_DEBUG_VDETECT,  isEnable ? "1" : "0");
    pqWriteSys(VDETECT_AIPQ_ENABLE,  isEnable ? "1" : "0");
    pqWriteSys(AIPQ_PARAMETERS_UVM_OPEN,  isEnable ? "1" : "0");
}

int CPQControl::SetAipqMode(aipq_mode_e mode, int is_save)
{
    SYS_LOGI("%s mode:%d is_save:%d\n", __FUNCTION__, mode, is_save);
    int ret = -1;

    ret = Cpq_SetAipqMode(mode, mCurrentSourceInputInfo);

    if ((ret == 0) && (is_save == 1)) {
        ret = SaveAipqMode((int)mode);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed\n", __FUNCTION__);
    } else {
        SYS_LOGI("%s success\n", __FUNCTION__);
    }
    return ret;
}

int CPQControl::GetAipqMode(void)
{
    int data = 0;

    mSSMAction->SSMReadAipqMode(&data);
    SYS_LOGI("%s, data:%d\n", __FUNCTION__, data);

    return data;
}

int CPQControl::SaveAipqMode(int mode)
{
    int ret = mSSMAction->SSMSaveAipqMode(mode);

    if (ret < 0) {
        SYS_LOGE("%s failed\n", __FUNCTION__);
    } else {
        SYS_LOGI("%s success\n", __FUNCTION__);
    }

    return ret;
}

int CPQControl::Cpq_SetAipqMode(aipq_mode_e mode, source_input_param_t source_input_param)
{
    SYS_LOGI("%s mode: %d\n", __FUNCTION__, mode);
    int ret = -1;

    if (mbCpqCfg_ai_enable) {
        ai_pic_table_t aiRegs;
        memset(&aiRegs, 0, sizeof(ai_pic_table_t));
        ret = mPQdb->PQ_GetAIParams(mode, mCurrentSourceInputInfo, &aiRegs);
        if (ret >= 0) {
            SYS_LOGI("%s: width:%d, height:%d, array:%s.\n", __FUNCTION__, aiRegs.width, aiRegs.height, (char *)aiRegs.table_ptr);
            ret = VPPDeviceIOCtl(AMVECM_IOC_S_AIPQ_TABLE, &aiRegs);
            if (ret < 0) {
                SYS_LOGE("%s: iocontrol failed\n", __FUNCTION__);
            }
        } else {
            SYS_LOGE("%s: get AI pq params failed\n", __FUNCTION__);
        }
    } else {
        SYS_LOGE("%s: ai is disable\n", __FUNCTION__);
        ret = 0;
    }

    if (ret < 0) {
        SYS_LOGE("%s failed\n", __FUNCTION__);
    } else {
        SYS_LOGI("%s success\n", __FUNCTION__);
    }

    return ret;
}

 bool CPQControl::hasAisrFunc(void)
 {
    bool hasAisr = false;

    if (mbCpqCfg_aisr_enable && isFileExist(pqSysWrite->getSysNode(AISR_PARAMETERS_UVM_OPEN_NN))) {
        hasAisr = true;
    }

    SYS_LOGI("%s, has aisr or not:%d\n", __FUNCTION__, hasAisr);
    return hasAisr;
 }

int CPQControl::SetAiSrEnable(bool isEnable)
{
    SYS_LOGI("%s isEnable:%d\n", __FUNCTION__, isEnable);
    int ret = -1;

    ret = Cpq_SetAiSrEnable(isEnable);
    if (ret < 0) {
        SYS_LOGE("%s Cpq_SetAiSrEnable failed\n", __FUNCTION__);
        return ret;
    }

    ret = SaveAiSrEnable(isEnable);
    property_set(PROP_MEDIA_AISR, (isEnable > 0) ? "true" : "false");

    if (ret < 0) {
        SYS_LOGE("%s failed\n", __FUNCTION__);
    } else {
        SYS_LOGI("%s success\n", __FUNCTION__);
    }
    return ret;
}

int CPQControl::GetAiSrEnable(void)
{
    int data = 0;

    mSSMAction->SSMReadAiSrEnable(&data);
    SYS_LOGI(" %s, data:%d\n", __FUNCTION__, data);

    if (data < 0 || data > 1) {
        data = 0;
    }
    return data;
}

int CPQControl::SaveAiSrEnable(bool enable)
{
    int ret = mSSMAction->SSMSaveAiSrEnable(enable ? 1 : 0);

    if (ret < 0) {
        SYS_LOGE("%s failed\n", __FUNCTION__);
    } else {
        SYS_LOGI("%s success\n", __FUNCTION__);
    }
    return ret;
}

int CPQControl::Cpq_SetAiSrEnable(bool enable)
{
    SYS_LOGI("%s, Cpq_SetAiSrEnable\n", __FUNCTION__);
    int ret = -1;

    if (mbCpqCfg_aisr_enable) {
        ret = pqWriteSys(VIDEO_AISR_ENABLE, enable ? "1" : "0");
    } else {
        SYS_LOGE("%s disabled\n", __FUNCTION__);
    }

    return ret;
}

int CPQControl::SetAiSrMode(aisr_mode_e mode, int is_save)
{
    SYS_LOGI("%s mode:%d is_save:%d\n", __FUNCTION__, mode, is_save);
    int ret = -1;

    ret = Cpq_SetAiSrMode(mode, mCurrentSourceInputInfo);

    if ((ret == 0) && (is_save == 1)) {
        ret = SaveAiSrMode((int)mode);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed\n", __FUNCTION__);
    } else {
        SYS_LOGI("%s success\n", __FUNCTION__);
    }
    return ret;
}

int CPQControl::GetAiSrMode(void)
{
    int data = 0;

    mSSMAction->SSMReadAiSrMode(&data);
    SYS_LOGI("%s, data:%d\n", __FUNCTION__, data);

    return data;
}

int CPQControl::SaveAiSrMode(int mode)
{
    int ret = mSSMAction->SSMSaveAiSrMode(mode);

    if (ret < 0) {
        SYS_LOGE("%s failed\n", __FUNCTION__);
    } else {
        SYS_LOGI("%s success\n", __FUNCTION__);
    }
    return ret;
}

int CPQControl::Cpq_SetAiSrMode(aisr_mode_e mode, source_input_param_t source_input_param)
{
    SYS_LOGI("%s mode:%d\n", __FUNCTION__, mode);
    int ret = -1;
    am_regs_t regs;

    memset(&regs, 0, sizeof(am_regs_t));

    if (mbCpqCfg_aisr_enable) {
        ret = mPQdb->PQ_GetAiSrParams(mode, source_input_param, &regs);
        if (ret < 0) {
            SYS_LOGE("%s PQ_GetAiSrParams failed\n", __FUNCTION__);
        } else {
            ret = Cpq_LoadRegs(regs);
        }
    } else {
        SYS_LOGE("%s: AiSr disabled\n", __FUNCTION__);
        ret = 0;
    }

    if (ret < 0) {
        SYS_LOGE("%s failed\n", __FUNCTION__);
    } else {
        SYS_LOGI("%s success\n", __FUNCTION__);
    }

    return ret;
}

int CPQControl::SetAiColor(int value, int is_save)
{
    SYS_LOGI("%s value = %d\n", __FUNCTION__, value);
    int ret = -1;

    ret = Cpq_SetAiColor(value);

    if ((ret == 0) && (is_save == 1)) {
        ret = SaveAiColor(value);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed\n", __FUNCTION__);
    } else {
        SYS_LOGI("%s success\n", __FUNCTION__);
    }
    return ret;
}

int CPQControl::GetAiColor(void)
{
    int data = 0;

    mSSMAction->SSMReadAiColor(&data);
    SYS_LOGI("%s, data = %d\n", __FUNCTION__, data);

    return data;
}

int CPQControl::SaveAiColor(int value)
{
    SYS_LOGI(" %s, value = %d\n", __FUNCTION__, value);
    int ret = -1;

    ret = mSSMAction->SSMSaveAiColor(value);

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGI("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::Cpq_SetAiColor(int value)
{
    SYS_LOGI("%s value = %d\n", __FUNCTION__, value);
    int ret = -1;

    if (mbCpqCfg_aicolor_enable) {
        ret = VPPDeviceIOCtl(AMVECM_IOC_AI_COLOR_EN, &value);
        pqWriteSys(AICOLOR_PARAMETERS_UVM_OPEN,  value ? "1" : "0");
    } else {
        SYS_LOGE("%s: AiColor disabled!\n", __FUNCTION__);
        ret = 0;
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGI("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::HasAiFace(void)
{
    char buf[32] = {0};

    if (pqReadSys(VIDEO_AIFACE_ENABLE, buf, sizeof(buf)) > 0) {
        SYS_LOGI("%s has aiface\n", __FUNCTION__);
        return 0;
    } else {
        SYS_LOGE("%s read VIDEO_AIFACE_ENABLE failed\n", __FUNCTION__);
        return -1;
    }
}

int CPQControl::SetAiFaceEnable(bool isEnable)
{
    SYS_LOGI("%s isEnable:%d\n", __FUNCTION__, isEnable);
    int ret = -1;

    ret = pqWriteSys(VIDEO_AIFACE_ENABLE, isEnable ? "1" : "0");

    if (ret < 0) {
        SYS_LOGE("%s failed\n", __FUNCTION__);
    } else {
        SYS_LOGI("%s success\n", __FUNCTION__);
    }

    return ret;
}

int CPQControl::GetAiFaceEnable(void)
{
    int enable = 0;
    char buf[32] = {0};

    if (pqReadSys(VIDEO_AIFACE_ENABLE, buf, sizeof(buf)) > 0) {
        enable = atoi(buf);
    } else {
        SYS_LOGE("%s read VIDEO_AIFACE_ENABLE failed!\n", __FUNCTION__);
    }

    SYS_LOGI("%s enable:%d\n", __FUNCTION__, enable);

    return enable;
}

//DLG
int CPQControl::SetDLGEnable(int enable, int is_save)
{
    int ret =0;
    SYS_LOGI("%s, source:%d, enable:%d\n",
        __FUNCTION__, mSourceInputForSaveParam, enable);



    if ((ret == 0) && (is_save == 1)) {
        ret = SaveDLGEnable(enable);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed\n",__FUNCTION__);
    } else {
        SYS_LOGI("%s success\n",__FUNCTION__);
    }
    return ret;
}

int CPQControl::GetDLGEnable()
{
    int data = 0;
    mSSMAction->SSMReadDLGEnable(&data);
    SYS_LOGI(" %s, data = %d\n", __FUNCTION__, data);

    if (data < 0 || data > 1) {
        data = 0;
    }
    return data;
}

int CPQControl::SaveDLGEnable(int enable)
{
    SYS_LOGI(" %s, enable = %d\n", __FUNCTION__, enable);
    int ret = mSSMAction->SSMSaveDLGEnable(enable);

    if (ret < 0) {
        SYS_LOGE("%s failed\n",__FUNCTION__);
    } else {
        SYS_LOGI("%s success\n",__FUNCTION__);
    }

    return ret;
}


//color space
int CPQControl::SetColorGamutMode(vpp_colorgamut_mode_t value, int is_save)
{
    int ret =0;
    SYS_LOGI("%s, source:%d, value:%d\n",
        __FUNCTION__, mSourceInputForSaveParam, value);
    ret = Cpq_SetColorGamutMode(value, mCurrentSourceInputInfo);

    if ((ret == 0) && (is_save == 1)) {
        ret = SaveColorGamutMode(value);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed\n",__FUNCTION__);
    } else {
        SYS_LOGI("%s success\n",__FUNCTION__);
    }
    return 0;
}

int CPQControl::GetColorGamutMode(void)
{
    int data = VPP_COLORGAMUT_MODE_AUTO;
    if (mbCpqCfg_new_picture_mode_enable) {
        vpp_picture_mode_t pq_mode = (vpp_picture_mode_t)GetPQMode();
        vpp_pictur_mode_para_t para;
        if (GetPictureModeData(mCurrentPqSource, pq_mode, &para) == 0) {
            data = para.ColorGamut;
        }
    } else {
        mSSMAction->SSMReadColorGamutMode(mSourceInputForSaveParam, &data);
    }

    SYS_LOGI("%s:source:%d, timming:%d, value:%d\n",
        __FUNCTION__, mSourceInputForSaveParam, mCurrentPqSource.pq_sig_fmt, data);

    if (data < VPP_COLORGAMUT_MODE_SRC || data > VPP_COLORGAMUT_MODE_NATIVE) {
        data = VPP_COLORGAMUT_MODE_AUTO;
    }

    return data;
}

int CPQControl::SaveColorGamutMode(vpp_colorgamut_mode_t value)
{
    int ret    = 1;
    SYS_LOGI("%s:source:%d, timming:%d, value:%d\n",__FUNCTION__, mSourceInputForSaveParam, mCurrentPqSource.pq_sig_fmt, value);
    if (mbCpqCfg_new_picture_mode_enable) {
        vpp_pictur_mode_para_t para;
        vpp_picture_mode_t pq_mode = (vpp_picture_mode_t)GetPQMode();
        if (GetPictureModeData(mCurrentPqSource, pq_mode, &para) == 0) {
            para.ColorGamut = (int)value;
            ret = SetPictureModeData(mCurrentPqSource, pq_mode, &para);
        }
    } else {
        ret = mSSMAction->SSMSaveColorGamutMode(mSourceInputForSaveParam, value);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed\n",__FUNCTION__);
    } else {
        SYS_LOGI("%s success\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::Cpq_SetColorGamutMode(vpp_colorgamut_mode_t value, source_input_param_t source_input_param)
{
    char val[64] = {0};
    sprintf(val, "%d", value);
    //need driver support
    return 0;
}

//SmoothPlus mode
int CPQControl::SetSmoothPlusMode(int smoothplus_mode, int is_save)
{
    SYS_LOGI("%s, source: %d, value = %d\n", __FUNCTION__, mSourceInputForSaveParam, smoothplus_mode);
    int ret = Cpq_SetSmoothPlusMode((vpp_smooth_plus_mode_t)smoothplus_mode, mCurrentSourceInputInfo);
    if ((ret ==0) && (is_save == 1)) {
        ret = SaveSmoothPlusMode((vpp_smooth_plus_mode_t)smoothplus_mode);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGI("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::GetSmoothPlusMode(void)
{
    int mode = VPP_SMOOTH_PLUS_MODE_MID;

    if (mbCpqCfg_new_picture_mode_enable) {
        vpp_picture_mode_t pq_mode = (vpp_picture_mode_t)GetPQMode();
        vpp_pictur_mode_para_t para;
        if (GetPictureModeData(mCurrentPqSource, pq_mode, &para) == 0) {
            mode = para.SmoothPlus;
        }
    } else {
        mSSMAction->SSMReadSmoothPlus(mSourceInputForSaveParam, &mode);
    }

    if (mode < VPP_SMOOTH_PLUS_MODE_OFF || mode > VPP_SMOOTH_PLUS_MODE_AUTO) {
        mode = VPP_SMOOTH_PLUS_MODE_MID;
    }

    SYS_LOGI("%s, source: %d, value = %d\n", __FUNCTION__, mSourceInputForSaveParam, mode);
    return mode;
}

int CPQControl::SaveSmoothPlusMode(int smoothplus_mode)
{
    int ret = -1;

    if (mbCpqCfg_new_picture_mode_enable) {
        vpp_pictur_mode_para_t para;
        vpp_picture_mode_t pq_mode = (vpp_picture_mode_t)GetPQMode();
        if (GetPictureModeData(mCurrentPqSource, pq_mode, &para) == 0) {
            para.SmoothPlus = (int)smoothplus_mode;
            ret = SetPictureModeData(mCurrentPqSource, pq_mode, &para);
        }
    } else {
        ret = mSSMAction->SSMSaveSmoothPlus(mSourceInputForSaveParam, smoothplus_mode);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGI("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::Cpq_SetSmoothPlusMode(vpp_smooth_plus_mode_t smoothplus_mode, source_input_param_t source_input_param)
{
    int ret = -1;
    am_regs_t regs;
    am_pq_param_t di_regs;
    memset(&regs, 0x0, sizeof(am_regs_t));
    memset(&di_regs, 0x0,sizeof(am_pq_param_t));

    if (mbCpqCfg_smoothplus_enable) {
        if (mPQdb->PQ_GetSmoothPlusParams(smoothplus_mode, source_input_param, &regs) == 0) {
            di_regs.table_name = TABLE_NAME_SMOOTHPLUS;
            di_regs.table_len = regs.length;
            am_reg_t tmp_buf[regs.length];
            for (unsigned int i=0;i<regs.length;i++) {
                  tmp_buf[i].addr = regs.am_reg[i].addr;
                  tmp_buf[i].mask = regs.am_reg[i].mask;
                  tmp_buf[i].type = regs.am_reg[i].type;
                  tmp_buf[i].val  = regs.am_reg[i].val;
            }
            di_regs.table_ptr = (long long)tmp_buf;

            ret = DI_LoadRegs(di_regs);
        } else {
            SYS_LOGE("PQ_GetSmoothPlusParams failed!\n");
        }
    } else {
        SYS_LOGI("Smooth Plus disabled\n");
        ret = 0;
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGI("%s success!\n",__FUNCTION__);
    }

    return ret;
}

bool CPQControl::hasSmoothPlusFunc(void)
{
    return mbCpqCfg_smoothplus_enable;
}

int CPQControl::SetHDRTMData(int *reGain)
{
    int ret = -1;
    if (reGain == NULL) {
        SYS_LOGE("%s: reGain is NULL.\n", __FUNCTION__);
    } else {
        int i = 0;
        vpp_hdr_tone_mapping_t hdrToneMapping;
        memset(&hdrToneMapping, 0, sizeof(vpp_hdr_tone_mapping_t));
        hdrToneMapping.lut_type = LUT_TYPE_HLG;
        hdrToneMapping.lutlength = 149;
        hdrToneMapping.tm_lut = reGain;

        SYS_LOGI("hdrToneMapping.lut_type = %d\n", hdrToneMapping.lut_type);
        SYS_LOGI("hdrToneMapping.lutlength = %d\n", hdrToneMapping.lutlength);
        //SYS_LOGV("hdrToneMapping.tm_lut = %s\n", hdrToneMapping.tm_lut);

        ret = VPPDeviceIOCtl(AMVECM_IOC_S_HDR_TM, &hdrToneMapping);
    }

    if (ret < 0) {
        SYS_LOGE("%s error(%s)!\n", __FUNCTION__, strerror(errno));
    } else {
        SYS_LOGI("%s success!\n", __FUNCTION__);
    }

    return 0;
}

//HDR TMO
int CPQControl::Cpq_SetHDRTMOParams(const hdr_tmo_sw_s *phdrtmo)
{
    int ret = 0;

    ret = VPPDeviceIOCtl(AMVECM_IOC_S_HDR_TMO, phdrtmo);
    if (ret < 0) {
        SYS_LOGE("%s error(%s)!\n", __FUNCTION__, strerror(errno));
    }

    return ret;
}

int CPQControl::SetHDRTMOMode(hdr_tmo_t mode, int is_save)
{
    int ret = -1;
    hdr_tmo_sw_s hdrtmo_param;

    SYS_LOGI("%s, source: %d, mode = %d\n", __FUNCTION__, mSourceInputForSaveParam, mode);

    if (mbCpqCfg_hdrtmo_enable) {
        if (mPQdb->PQ_GetHDRTMOParams(mCurrentSourceInputInfo, mode, &hdrtmo_param) == 0) {
            ret = Cpq_SetHDRTMOParams(&hdrtmo_param);
            if ((ret ==0) && (is_save == 1)) {
                ret = SaveHDRTMOMode(mode);
            }
        } else {
            SYS_LOGE("mPQdb->PQ_GetHDRTMOParams failed!\n");
        }
    } else {
        SYS_LOGI("hdr tmo disabled\n");
        ret = 0;
    }

    if (ret < 0) {
        SYS_LOGE("%s failed\n",__FUNCTION__);
    } else {
        SYS_LOGI("%s success\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::GetHDRTMOMode()
{
    int data = HDR_TMO_DYNAMIC;
    mSSMAction->SSMReadHdrTmoVal(mSourceInputForSaveParam, &data);

    if (data < HDR_TMO_OFF || data > HDR_TMO_STATIC) {
        data = HDR_TMO_DYNAMIC;
    }

    SYS_LOGI("%s, source: %d, value = %d\n", __FUNCTION__, mSourceInputForSaveParam, data);

    return data;
}

int CPQControl::SaveHDRTMOMode(hdr_tmo_t mode)
{
    SYS_LOGI("%s, source: %d, value = %d\n", __FUNCTION__, mSourceInputForSaveParam, mode);

    int ret = mSSMAction->SSMSaveHdrTmoVal(mSourceInputForSaveParam, mode);
    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGI("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::SetBlackStretch(int level, int is_save)
{
    SYS_LOGI("%s, level = %d\n", __FUNCTION__, level);
    int ret = -1;
    ret = Cpq_BlackStretch(level, mCurrentSourceInputInfo);

    if (ret == 0 && is_save == 1) {
        SaveBlackStretch(level);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGI("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::GetBlackStretch(void)
{
    int level = VPP_PQ_LV_OFF;
    if (mbCpqCfg_new_picture_mode_enable) {
        vpp_picture_mode_t pq_mode = (vpp_picture_mode_t)GetPQMode();
        vpp_pictur_mode_para_t para;

        if (GetPictureModeData(mCurrentPqSource, pq_mode, &para) == 0) {
            level = para.BlackStretch;
        }
    } else {
        mSSMAction->SSMReadBlackStretch(mCurrentSourceInputInfo.source_input, &level);
    }

    if (level < VPP_PQ_LV_OFF || level >= VPP_PQ_LV_MAX) {
        level = VPP_PQ_LV_OFF;
    }

    SYS_LOGI("%s:source:%d, timming:%d, value:%d\n", __FUNCTION__, mSourceInputForSaveParam, mCurrentPqSource.pq_sig_fmt, level);

    return level;
}

int CPQControl::SaveBlackStretch(int level)
{
    int ret = -1;
    if (mbCpqCfg_new_picture_mode_enable) {
        vpp_pictur_mode_para_t para;
        vpp_picture_mode_t pq_mode = (vpp_picture_mode_t)GetPQMode();

        if (GetPictureModeData(mCurrentPqSource, pq_mode, &para) == 0) {
            para.BlackStretch = level;
            ret = SetPictureModeData(mCurrentPqSource, pq_mode, &para);
        }
    } else {
        mSSMAction->SSMSaveBlackStretch(mCurrentSourceInputInfo.source_input, level);
    }

    if (ret < 0)
        SYS_LOGE("%s %d failed!\n",__FUNCTION__, level);

    return ret;
}

int CPQControl::Cpq_BlackStretch(int level,source_input_param_t source_input_param)
{
    int ret = -1;
    am_regs_t regs;
    memset(&regs, 0, sizeof(am_regs_t));

    if (mbCpqCfg_blackextension_enable) {
        ret = mPQdb->PQ_GetBlackStretchParams(level,source_input_param, &regs);

        if (ret < 0) {
            SYS_LOGE("%s: PQ_GetBlackStretchParams failed!\n", __FUNCTION__);
        } else {
            ret = Cpq_LoadRegs(regs);
        }
    } else {
        SYS_LOGE("%s: BlackStretch disabled!\n", __FUNCTION__);
        ret = 0;
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGI("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::SetBlueStretch(int level, int is_save)
{
    SYS_LOGI("%s, level = %d\n", __FUNCTION__, level);
    int ret = -1;
    ret = Cpq_BlueStretch(level, mCurrentSourceInputInfo);

    if (ret == 0 && is_save == 1) {
        SaveBlueStretch(level);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGI("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::GetBlueStretch(void)
{
    int level = VPP_PQ_LV_OFF;
    if (mbCpqCfg_new_picture_mode_enable) {
        vpp_picture_mode_t pq_mode = (vpp_picture_mode_t)GetPQMode();
        vpp_pictur_mode_para_t para;
        if (GetPictureModeData(mCurrentPqSource, pq_mode, &para) == 0) {
            level = para.BlueStretch;
        }
    } else {
        mSSMAction->SSMReadBlueStretch(mSourceInputForSaveParam, &level);
    }

    if (level < VPP_PQ_LV_OFF || level >= VPP_PQ_LV_MAX) {
        level = VPP_PQ_LV_OFF;
    }

    return level;
}

int CPQControl::SaveBlueStretch(int level)
{
    int ret = -1;
    SYS_LOGI("%s, source: %d, level = %d\n", __FUNCTION__, mSourceInputForSaveParam, level);
    if (mbCpqCfg_new_picture_mode_enable) {
        vpp_pictur_mode_para_t para;
        vpp_picture_mode_t pq_mode = (vpp_picture_mode_t)GetPQMode();
        if (GetPictureModeData(mCurrentPqSource, pq_mode, &para) == 0) {
            para.BlackStretch = level;
            ret = SetPictureModeData(mCurrentPqSource, pq_mode, &para);
        }
    } else {
        ret = mSSMAction->SSMSaveBlueStretch(mSourceInputForSaveParam, level);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGI("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::Cpq_BlueStretch(int level,source_input_param_t source_input_param)
{
    int ret = -1;
    am_regs_t regs;
    memset(&regs, 0, sizeof(am_regs_t));
    if (mbCpqCfg_bluestretch_enable) {
        ret = mPQdb->PQ_GetBlueStretchParams(level,source_input_param, &regs);

        if (ret < 0) {
            SYS_LOGE("%s: PQ_GetBlueStretchParams failed!\n", __FUNCTION__);
        } else {
            ret = Cpq_LoadRegs(regs);
        }
    } else {
        SYS_LOGE("%s: BlueStretch disabled!\n", __FUNCTION__);
        ret = 0;
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGI("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::SetChromaCoring(int level, int is_save)
{
    SYS_LOGI("%s, level = %d\n", __FUNCTION__, level);
    int ret = -1;
    ret = Cpq_ChromaCoring(level, mCurrentSourceInputInfo);

    if (ret == 0 && is_save == 1) {
        SaveChromaCoring(level);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGI("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::GetChromaCoring(void)
{
    int level = VPP_PQ_LV_OFF;
    if (mbCpqCfg_new_picture_mode_enable) {
        vpp_picture_mode_t pq_mode = (vpp_picture_mode_t)GetPQMode();
        vpp_pictur_mode_para_t para;
        if (GetPictureModeData(mCurrentPqSource, pq_mode, &para) == 0) {
            level = para.ChromaCoring;
        }
    } else {
        mSSMAction->SSMReadChromaCoring(mSourceInputForSaveParam, &level);
    }

    if (level < VPP_PQ_LV_OFF || level >= VPP_PQ_LV_MAX) {
        level = VPP_PQ_LV_OFF;
    }

    return level;
}

int CPQControl::SaveChromaCoring(int level)
{
    int ret = -1;
    SYS_LOGI("%s, source: %d, level = %d\n", __FUNCTION__, mSourceInputForSaveParam, level);
    if (mbCpqCfg_new_picture_mode_enable) {
        vpp_pictur_mode_para_t para;
        vpp_picture_mode_t pq_mode = (vpp_picture_mode_t)GetPQMode();
        if (GetPictureModeData(mCurrentPqSource, pq_mode, &para) == 0) {
            para.ChromaCoring = level;
            ret = SetPictureModeData(mCurrentPqSource, pq_mode, &para);
        }
    } else {
        ret = mSSMAction->SSMSaveChromaCoring(mSourceInputForSaveParam, level);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGI("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::Cpq_ChromaCoring(int level,source_input_param_t source_input_param)
{
    int ret = -1;
    am_regs_t regs;
    memset(&regs, 0, sizeof(am_regs_t));
    if (mbCpqCfg_chroma_coring_enable) {
        ret = mPQdb->PQ_GetChromaCoringParams(level,source_input_param, &regs);

        if (ret < 0) {
            SYS_LOGE("%s: PQ_GetChromaCoringParams failed!\n", __FUNCTION__);
        } else {
            ret = Cpq_LoadRegs(regs);
        }
    } else {
        SYS_LOGE("%s: ChromaCoring disabled!\n", __FUNCTION__);
        ret = 0;
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGI("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::SetLocalDimming(int level, int is_save)
{
    SYS_LOGI("%s, level = %d\n", __FUNCTION__, level);
    int ret = -1;

    ret = Cpq_LocalDimming((vpp_pq_level_t)level);

    if (ret == 0 && is_save == 1) {
        ret = mSSMAction->SSMSaveLocalDimming(level);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed\n",__FUNCTION__);
    } else {
        SYS_LOGI("%s success\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::GetLocalDimming(void)
{
    int level = VPP_PQ_LV_OFF;
    if (mSSMAction->SSMReadLocalDimming(&level) < 0) {
        SYS_LOGE("%s, SSMReadLocalDimming ERROR!!!\n", __FUNCTION__);
        return VPP_PQ_LV_OFF;
    } else {
        SYS_LOGI("%s, source: %d, level = %d\n", __FUNCTION__, mSourceInputForSaveParam, level);
    }

    if (level < VPP_PQ_LV_OFF || level >= VPP_PQ_LV_MAX) {
        level = VPP_PQ_LV_OFF;
    }

    return level;
}

int CPQControl::Cpq_LocalDimming(vpp_pq_level_t level)
{
    if (!mbCpqCfg_LocalDimming_enable) {
        SYS_LOGD("%s: LocalDimming disabled!\n", __FUNCTION__);
        return 0;
    }

    if (AML_HAL_PQ_LD_SetLevelIdx((int)level) != API_OK) {
        SYS_LOGE("%s: AML_HAL_LD_SetLevelIdx failed!\n", __FUNCTION__);
        return -1;
    }

    return 0;
}

int CPQControl::SetPQModuleDemoState(pq_module_demo_t modules, pq_module_demo_state_t state)
{
    SYS_LOGD("%s, modules:%d, state:%d\n",__FUNCTION__, modules, state);
    int ret = -1;
    switch (modules) {
        case PQ_DEMO_MEMC://left memc on, right memc off
            if (hasMemcFunc()) {
                ret = pqWriteSys(PQ_MODULE_MEMC_DEMO_WIN, (state > 0) ? "demo_win 1" : "demo_win 0");
            } else {
                SYS_LOGE("%s MEMC Module disabled\n",__FUNCTION__);
                ret = -1;
            }
            break;
        case PQ_DEMO_AISR:
            if (mbCpqCfg_aisr_enable) {
                ret = pqWriteSys(PQ_MODULE_AISR_DEMO_EN, (state > 0) ? "1" : "0");
                    if (state == PQ_DEMO_STATE_4K) {
                        ret = pqWriteSys(PQ_MODULE_AISR_DEMO_AXIS, "0 0 1919 2159");
                    } else if(state == PQ_DEMO_STATE_8K){
                        ret = pqWriteSys(PQ_MODULE_AISR_DEMO_AXIS, "0 0 3839 4319");
                    } else if(state == PQ_DEMO_STATE_1080P){
                        ret = pqWriteSys(PQ_MODULE_AISR_DEMO_AXIS, "0 0 960 1079");
                    } else if(state < PQ_DEMO_STATE_OFF || state >= PQ_DEMO_STATE_MAX){
                        SYS_LOGE("%s state:%d out of range\n", __FUNCTION__, state);
                        state = PQ_DEMO_STATE_4K;
                        ret = pqWriteSys(PQ_MODULE_AISR_DEMO_AXIS, "0 0 1919 2159");
                    }
            } else {
                SYS_LOGE("%s AISR Module disabled\n",__FUNCTION__);
                ret = -1;
            }
            break;
        default:
            SYS_LOGE("%s This Module ：%d is missing \n",__FUNCTION__, modules);
            ret = -1;
            break;
    }

    if (ret != -1) {
        if (mSSMAction->SSMSavePQModuleDemoState((int)modules, (int)state) < 0) {
            SYS_LOGE("%s, SSMSavePQModuleDemoState ERROR!!!\n", __FUNCTION__);
            ret = -1;
        }
    }

    if (ret < 0) {
        SYS_LOGE("%s failed\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success\n",__FUNCTION__);
    }
    return ret;
}

int CPQControl::GetPQModuleDemoState(int modules)
{
    int state = 0;
    if (mSSMAction->SSMReadPQModuleDemoState(modules, &state) < 0) {
        SYS_LOGE("%s, SSMReadPQModuleDemoState ERROR!!!\n", __FUNCTION__);
        return -1;
    } else {
        SYS_LOGD("%s, modules:%d, state:%d\n",__FUNCTION__, modules, state);
    }

    if (state < PQ_DEMO_STATE_OFF || state >= PQ_DEMO_STATE_MAX) {
        SYS_LOGE("%s state:%d out of range\n", __FUNCTION__, state);
        state = PQ_DEMO_STATE_4K;
    }

    return state;
}

void CPQControl::resetAllUserSettingParam()
{
    int ret = 0, i = 0, config_val = 0;
    vpp_pq_para_t pq_para;
    const char *buf = NULL;

    if (mbCpqCfg_new_picture_mode_enable) {
        resetPQUiSetting();
        resetPQTableSetting();
        mSSMAction->SSMSaveBackLightVal(1*sizeof(int), 100);
        mSSMAction->SSMSaveBackLightVal(2*sizeof(int), 100);
        mSSMAction->SSMSaveBackLightVal(3*sizeof(int), 100);
        return;
    }

    for (i=SOURCE_TV;i<SOURCE_MAX;i++) {
        if (mbCpqCfg_separate_db_enable) {
            ret = mpOverScandb->PQ_GetPQModeParams((tv_source_input_t)i, VPP_PICTURE_MODE_USER, &pq_para);
        } else {
            ret = mPQdb->PQ_GetPQModeParams((tv_source_input_t)i, VPP_PICTURE_MODE_USER, &pq_para);
        }
        /*SYS_LOGD("%s: brightness=%d, contrast=%d, saturation=%d, hue=%d, sharpness=%d, backlight=%d, nr=%d\n",
                 __FUNCTION__, pq_para.brightness, pq_para.contrast, pq_para.saturation, pq_para.hue,
                 pq_para.sharpness, pq_para.backlight, pq_para.nr);*/
        mSSMAction->SSMSaveBrightness((tv_source_input_t)i, pq_para.brightness);
        mSSMAction->SSMSaveContrast((tv_source_input_t)i, pq_para.contrast);
        mSSMAction->SSMSaveSaturation((tv_source_input_t)i, pq_para.saturation);
        mSSMAction->SSMSaveHue((tv_source_input_t)i, pq_para.hue);
        mSSMAction->SSMSaveSharpness((tv_source_input_t)i, pq_para.sharpness);
        mSSMAction->SSMSaveBackLightVal(1*sizeof(int), pq_para.backlight);
        mSSMAction->SSMSaveBackLightVal(2*sizeof(int), pq_para.backlight);
        mSSMAction->SSMSaveBackLightVal(3*sizeof(int), pq_para.backlight);
        mSSMAction->SSMSaveNoiseReduction((tv_source_input_t)i, pq_para.nr);
        mSSMAction->SSMSaveColorGamutMode((tv_source_input_t)i, 0);

        config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_PICTUREMODE_DEF, VPP_PICTURE_MODE_STANDARD);
        mSSMAction->SSMSavePictureMode(i, config_val);
        mSSMAction->SSMSaveLastPictureMode(i, config_val);

        config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_COLORTEMPTUREMODE_DEF, VPP_COLOR_TEMPERATURE_MODE_STANDARD);
        mSSMAction->SSMSaveColorTemperature(i, config_val);

        config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_DISPLAYMODE_DEF, VPP_DISPLAY_MODE_NORMAL);
        mSSMAction->SSMSaveDisplayMode(i, config_val);

        config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_GAMMALEVEL_DEF, VPP_GAMMA_CURVE_DEFAULT);
        mSSMAction->SSMSaveGammaValue(i, config_val);

        config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_AUTOASPECT_DEF, 1);
        mSSMAction->SSMSaveAutoAspect(i, config_val);

        config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_43STRETCH_DEF, 0);
        mSSMAction->SSMSave43Stretch(i, config_val);

        config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_DNLPLEVEL_DEF, DYNAMIC_CONTRAST_MID);
        mSSMAction->SSMSaveDnlpMode(i, config_val);

        config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_DNLPGAIN_DEF, 0);
        mSSMAction->SSMSaveDnlpGainValue(i, config_val);

        config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_LOCALCONTRASTMODE_DEF, 2);
        mSSMAction->SSMSaveLocalContrastMode(i, config_val);

        config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_DEBLOCKMODE_DEF, DI_DEMOSQUITO_MODE_OFF);
        mSSMAction->SSMSaveDeblockMode(i, config_val);

        config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_DEMOSQUITOMODE_DEF, DI_DEMOSQUITO_MODE_OFF);
        mSSMAction->SSMSaveDemoSquitoMode(i, config_val);

        config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_MCDI_DEF, VPP_MCDI_MODE_STANDARD);
        mSSMAction->SSMSaveMcDiMode(i, config_val);

        config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_MEMCMODE_DEF, VPP_MEMC_MODE_HIGH);
        mSSMAction->SSMSaveMemcMode(i, config_val);

        buf = mPQConfigFile->GetString(CFG_SECTION_PQ, CFG_MEMCDEBLURLEVEL_DEF, NULL);
        int Deblur_para[VPP_MEMC_MODE_MAX] = { 0, 3, 6, 10 };
        pqTransformStringToInt(buf, Deblur_para);
        for (int j = VPP_MEMC_MODE_OFF; j < VPP_MEMC_MODE_MAX; j++) {
            mSSMAction->SSMSaveMemcDeblurLevel(i * VPP_MEMC_MODE_MAX + j, Deblur_para[j]);
        }

        buf = mPQConfigFile->GetString(CFG_SECTION_PQ, CFG_MEMCDEJUDDERLEVEL_DEF, NULL);
        int DeJudder_para[VPP_MEMC_MODE_MAX] = { 0, 3, 6, 10 };
        pqTransformStringToInt(buf, DeJudder_para);
        for (int j = VPP_MEMC_MODE_OFF; j < VPP_MEMC_MODE_MAX; j++) {
            mSSMAction->SSMSaveMemcDeJudderLevel(i * VPP_MEMC_MODE_MAX + j, DeJudder_para[j]);
        }

        config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_BLACKSTRETCH_DEF, VPP_PQ_LV_OFF);
        mSSMAction->SSMSaveBlackStretch(i, config_val);

        config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_BLUESTRETCH_DEF, VPP_PQ_LV_OFF);
        mSSMAction->SSMSaveBlueStretch(i, config_val);

        config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_CHMROMACORING_DEF, VPP_PQ_LV_OFF);
        mSSMAction->SSMSaveChromaCoring(i, config_val);
    }

    config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_LOCALDIMMING_DEF, VPP_PQ_LV_OFF);
    mSSMAction->SSMSaveLocalDimming(config_val);

    config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_COLORDEMOMODE_DEF, VPP_COLOR_DEMO_MODE_ALLON);
    mSSMAction->SSMSaveColorDemoMode(config_val);

    config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_COLORBASEMODE_DEF, VPP_COLOR_DEMO_MODE_ALLON);
    mSSMAction->SSMSaveColorBaseMode ( VPP_COLOR_BASE_MODE_OPTIMIZE);

    config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_RGBGAIN_R_DEF, 0);
    mSSMAction->SSMSaveRGBGainRStart(0, config_val);

    config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_RGBGAIN_G_DEF, 0);
    mSSMAction->SSMSaveRGBGainGStart(0, config_val);

    config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_RGBGAIN_B_DEF, 0);
    mSSMAction->SSMSaveRGBGainBStart(0, config_val);

    config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_RGBPOSTOFFSET_R_DEF_DEF, 1024);
    mSSMAction->SSMSaveRGBPostOffsetRStart(0, config_val);

    config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_RGBPOSTOFFSET_G_DEF_DEF, 1024);
    mSSMAction->SSMSaveRGBPostOffsetGStart(0, config_val);

    config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_RGBPOSTOFFSET_B_DEF_DEF, 1024);
    mSSMAction->SSMSaveRGBPostOffsetBStart(0, config_val);

    int8_t std_buf[6] = { 0, 0, 0, 0, 0, 0 };
    int8_t warm_buf[6] = { 0, 0, -8, 0, 0, 0 };
    int8_t cold_buf[6] = { -8, 0, 0, 0, 0, 0 };
    for (i = 0; i < 6; i++) {
        mSSMAction->SSMSaveRGBValueStart(i + VPP_COLOR_TEMPERATURE_MODE_STANDARD * 6, std_buf[i]); //0~5
        mSSMAction->SSMSaveRGBValueStart(i + VPP_COLOR_TEMPERATURE_MODE_WARM * 6, warm_buf[i]); //6~11
        mSSMAction->SSMSaveRGBValueStart(i + VPP_COLOR_TEMPERATURE_MODE_COLD * 6, cold_buf[i]); //12~17
    }

    config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_COLORSPACE_DEF, VPP_COLOR_SPACE_AUTO);
    mSSMAction->SSMSaveColorSpaceStart(config_val);

    config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_DDRSSC_DEF, 0);
    mSSMAction->SSMSaveDDRSSC(config_val);

    buf = mPQConfigFile->GetString(CFG_SECTION_PQ, CFG_LVDSSSC_DEF, NULL);
    int tmp[3] = {0, 0, 0};
    pqTransformStringToInt(buf, tmp);
    mSSMAction->SSMSaveLVDSSSC(tmp);

    config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_EYEPROJECTMODE_DEF, 0);
    mSSMAction->SSMSaveEyeProtectionMode(config_val);

    config_val = mPQConfigFile->GetInt(CFG_SECTION_HDMI, CFG_EDID_VERSION_DEF, 0);
    mSSMAction->SSMEdidRestoreDefault(config_val);

    config_val = mPQConfigFile->GetInt(CFG_SECTION_HDMI, CFG_HDCP_SWITCHER_DEF, 0);
    mSSMAction->SSMHdcpSwitcherRestoreDefault(0);

    buf = mPQConfigFile->GetString(CFG_SECTION_HDMI, CFG_COLOR_RANGE_MODE_DEF, "default");
    if (strcmp(buf, "full") == 0) {
        mSSMAction->SSMSColorRangeModeRestoreDefault(1);
    } else if (strcmp(buf, "limit") == 0) {
        mSSMAction->SSMSColorRangeModeRestoreDefault(2);
    } else {
        mSSMAction->SSMSColorRangeModeRestoreDefault(0);
    }
    //static frame
    Cpq_SSMWriteNTypes(SSM_RW_BLACKOUT_ENABLE_START, 1, 1, 0);
    //screen color for signal
    Cpq_SSMWriteNTypes(CUSTOMER_DATA_POS_SCREEN_COLOR_START, 1, 0, 0);

    mSSMAction->SSMSaveAipqEnableVal(0);
    mSSMAction->SSMSaveAiSrEnable(1);
    config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_AISRMODE_DEF, 3);
    mSSMAction->SSMSaveAiSrMode(config_val);
    config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_AIPQMODE_DEF, 2);
    mSSMAction->SSMSaveAipqMode(config_val);

    //PQ Module Demo State
    for (i = PQ_DEMO_MEMC; i < PQ_DEMO_MAX; i++) {
        mSSMAction->SSMSavePQModuleDemoState(i, 0);
    }

    return;
}

void CPQControl::pqTransformStringToInt(const char *buf, int *val)
{
    if (buf != NULL) {
        //SYS_LOGD("%s: %s\n", __FUNCTION__, buf);
        char temp_buf[256];
        char *p = NULL;
        int i = 0;
        strncpy(temp_buf, buf, strlen(buf)+1);
        p = strtok(temp_buf, ",");
        while (NULL != p) {
           val[i++] = atoi(p);
           p = strtok(NULL,  ",");
        }
    } else {
        SYS_LOGE("%s:Invalid param!\n", __FUNCTION__);
    }
    return;
}

bool CPQControl::isFileExist(const char *file_name)
{
    struct stat tmp_st;
    int ret = -1;

    ret = stat(file_name, &tmp_st);
    if (ret != 0 ) {
       SYS_LOGE("%s don't exist!\n",file_name);
       return false;
    } else {
       return true;
    }
}

int CPQControl::Cpq_GetInputVideoFrameHeight(tv_source_input_t source_input)
{
    int inputFrameHeight = 0;

    if ((source_input == SOURCE_MPEG)
        || (source_input == SOURCE_DTV)) {//decoder
        char inputModeBuf[32] = {0};
        if (pqReadSys(VIDEO_FRAME_HEIGHT, inputModeBuf, sizeof(inputModeBuf)) > 0) {
            inputFrameHeight = atoi(inputModeBuf);
        } else {
            SYS_LOGE("Read VIDEO_FRAME_HEIGHT failed!\n");
        }
    } else {//vdin
#ifdef SUPPORT_TVSERVICE
        const sp<TvServerHidlClient> &TvService = getTvService();
        if (TvService == NULL) {
            SYS_LOGE("%s: get tvservice failed!\n", __FUNCTION__);
        } else {
            FormatInfo info = TvService->getHdmiFormatInfo();
            inputFrameHeight = info.height;
        }
#else
        SYS_LOGI("%s: don't support tvservice!\n", __FUNCTION__);
#endif
    }

    if (inputFrameHeight <= 0) {
        SYS_LOGE("%s: inputFrameHeight is invalid, return default value!\n", __FUNCTION__);
        inputFrameHeight = 1080;
    }

    SYS_LOGI("%s: inputFrameHeight is %d!\n", __FUNCTION__, inputFrameHeight);
    return inputFrameHeight;
}

//table about db tvout with input/output resolution
int Table_TvoutWithIOResolution[TABLE_TYPE_MAX][RESOLUTION_MAX][RESOLUTION_MAX] = {
                /*480                      576                       720                      1080                       2160                       4320*/
    { //SDR
        /*480*/ {OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_480_720, OUTPUT_TYPE_HDMI_480_1080, OUTPUT_TYPE_HDMI_480_480,  OUTPUT_TYPE_HDMI_480_480},
        /*576*/ {OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_576_720, OUTPUT_TYPE_HDMI_576_1080, OUTPUT_TYPE_HDMI_576_576,  OUTPUT_TYPE_HDMI_576_576},
        /*720*/ {OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_720_1080, OUTPUT_TYPE_HDMI_720_720,  OUTPUT_TYPE_HDMI_720_720},
        /*1080*/{OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE,  OUTPUT_TYPE_HDMI_1080_1080,OUTPUT_TYPE_HDMI_1080_1080},
        /*2160*/{OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE,  OUTPUT_TYPE_HDMI_4K,       OUTPUT_TYPE_HDMI_4K},
        /*4320*/{OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE,  OUTPUT_TYPE_HDMI_NOSCALE,  OUTPUT_TYPE_HDMI_NOSCALE}
    },
    { //HDR
        /*480*/ {OUTPUT_TYPE_HDMI_NOSCALE_HDR, OUTPUT_TYPE_HDMI_NOSCALE_HDR, OUTPUT_TYPE_HDMI_480_720_HDR, OUTPUT_TYPE_HDMI_480_1080_HDR, OUTPUT_TYPE_HDMI_480_480_HDR,  OUTPUT_TYPE_HDMI_480_480_HDR},
        /*576*/ {OUTPUT_TYPE_HDMI_NOSCALE_HDR, OUTPUT_TYPE_HDMI_NOSCALE_HDR, OUTPUT_TYPE_HDMI_576_720_HDR, OUTPUT_TYPE_HDMI_576_1080_HDR, OUTPUT_TYPE_HDMI_576_576_HDR,  OUTPUT_TYPE_HDMI_576_576_HDR},
        /*720*/ {OUTPUT_TYPE_HDMI_NOSCALE_HDR, OUTPUT_TYPE_HDMI_NOSCALE_HDR, OUTPUT_TYPE_HDMI_NOSCALE_HDR, OUTPUT_TYPE_HDMI_720_1080_HDR, OUTPUT_TYPE_HDMI_720_720_HDR,  OUTPUT_TYPE_HDMI_720_720_HDR},
        /*1080*/{OUTPUT_TYPE_HDMI_NOSCALE_HDR, OUTPUT_TYPE_HDMI_NOSCALE_HDR, OUTPUT_TYPE_HDMI_NOSCALE_HDR, OUTPUT_TYPE_HDMI_NOSCALE_HDR,  OUTPUT_TYPE_HDMI_1080_1080_HDR,OUTPUT_TYPE_HDMI_1080_1080_HDR},
        /*2160*/{OUTPUT_TYPE_HDMI_NOSCALE_HDR, OUTPUT_TYPE_HDMI_NOSCALE_HDR, OUTPUT_TYPE_HDMI_NOSCALE_HDR, OUTPUT_TYPE_HDMI_NOSCALE_HDR,  OUTPUT_TYPE_HDMI_4K_HDR,       OUTPUT_TYPE_HDMI_4K_HDR},
        /*4320*/{OUTPUT_TYPE_HDMI_NOSCALE_HDR, OUTPUT_TYPE_HDMI_NOSCALE_HDR, OUTPUT_TYPE_HDMI_NOSCALE_HDR, OUTPUT_TYPE_HDMI_NOSCALE_HDR,  OUTPUT_TYPE_HDMI_NOSCALE_HDR,  OUTPUT_TYPE_HDMI_NOSCALE_HDR}
    },
    { //4K 120HZ
        /*480*/ {OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_480_4K120,  OUTPUT_TYPE_HDMI_NOSCALE},
        /*576*/ {OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_576_4K120,  OUTPUT_TYPE_HDMI_NOSCALE},
        /*720*/ {OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_720_4K120,  OUTPUT_TYPE_HDMI_NOSCALE},
        /*1080*/{OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_1080_4K120, OUTPUT_TYPE_HDMI_NOSCALE},
        /*2160*/{OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_4K_4K120,   OUTPUT_TYPE_HDMI_NOSCALE},
        /*4320*/{OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE,    OUTPUT_TYPE_HDMI_NOSCALE}
    },
    { //4K 120HZ HDR
        /*480*/ {OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_480_4K120_HDR,  OUTPUT_TYPE_HDMI_NOSCALE},
        /*576*/ {OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_576_4K120_HDR,  OUTPUT_TYPE_HDMI_NOSCALE},
        /*720*/ {OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_720_4K120_HDR,  OUTPUT_TYPE_HDMI_NOSCALE},
        /*1080*/{OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_1080_4K120_HDR, OUTPUT_TYPE_HDMI_NOSCALE},
        /*2160*/{OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_4K_4K120_HDR,   OUTPUT_TYPE_HDMI_NOSCALE},
        /*4320*/{OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE, OUTPUT_TYPE_HDMI_NOSCALE,        OUTPUT_TYPE_HDMI_NOSCALE}
    }
};

//table about resolution height thread value
int Table_ResolutionHeightThread[RESOLUTION_MAX][2] = {
    //type            value
    {SD_HEIGHT_480,   480},
    {SD_HEIGHT_576,   576},
    {HD_HEIGHT_720,   720},
    {FHD_HEIGHT_1080, 1080},
    {UHD_HEIGHT_2160, 2160},
    {UHD_HEIGHT_4320, 4320}
};

output_type_t CPQControl::MapDbTvoutWithIOResolution(int inputFrameHeight, int outputFrameHeight)
{
    output_type_t OutPutType = OUTPUT_TYPE_LVDS;
    SYS_LOGD("%s inputFrameHeight %d outputFrameHeight %d\n", __FUNCTION__, inputFrameHeight, outputFrameHeight);

    if (mPQdb->mDbMatchType == MATCH_TYPE_MBOX_S5) {
        int index_in = 0, index_out = 0, table_type = 0;

        for (int i = 0; i < RESOLUTION_MAX; i++) { //pick up input index
            if (inputFrameHeight < Table_ResolutionHeightThread[i][1]) {
                index_in = i - 1;
                break;
            }
            if (inputFrameHeight >= Table_ResolutionHeightThread[UHD_HEIGHT_4320][1]) { //input 8k
                index_in = 5;
            }
        }

        for (int j = 0; j < RESOLUTION_MAX; j++) { //pick up output index
            if (outputFrameHeight < Table_ResolutionHeightThread[j][1]) {
                index_out = j - 1;
                break;
            }
            if (outputFrameHeight >= Table_ResolutionHeightThread[UHD_HEIGHT_4320][1]) { //8k output
                index_out = 5;
            }
        }

        if (mPQdb->mHdrStatus == true) {
            if (mDisplayMode4k120 == true || mDisplayMode4k100 ==  true) {
                table_type = 3;
            } else {
                table_type = 1;
            }
        } else {
            if (mDisplayMode4k120 == true || mDisplayMode4k100 == true) {
                table_type = 2;
            } else {
                table_type = 0;
            }
        }

        SYS_LOGD("%s table_type %d index_in %d index_out %d\n", __FUNCTION__, table_type, index_in, index_out);
        OutPutType = (output_type_t)Table_TvoutWithIOResolution[table_type][index_in][index_out];
    } else { //old project logic
        if (inputFrameHeight > 1088) {//inputsource is 4k
            OutPutType = OUTPUT_TYPE_HDMI_4K;
        } else {
            if (inputFrameHeight >= outputFrameHeight) {//input height >= output height
                OutPutType = OUTPUT_TYPE_HDMI_NOSCALE;
            } else {//input height < output height
                if (inputFrameHeight > 720 && inputFrameHeight <= 1088) {//inputsource is 1080
                    OutPutType = OUTPUT_TYPE_HDMI_NOSCALE;
                } else if (inputFrameHeight > 576 && inputFrameHeight <= 720) {//inputsource is 720
                    if (outputFrameHeight == 4096) {
                        OutPutType = OUTPUT_TYPE_HDMI_HD_4096;
                    } else if (outputFrameHeight >= inputFrameHeight * 2) {
                        OutPutType = OUTPUT_TYPE_HDMI_HD_UPSCALE;
                    } else {
                        OutPutType = OUTPUT_TYPE_HDMI_NOSCALE;
                    }
                } else {//inputsource is 480
                    if (outputFrameHeight == 4096) {
                        OutPutType = OUTPUT_TYPE_HDMI_SD_4096;
                    } else if ((outputFrameHeight * 8) >= (inputFrameHeight * 15)) {
                        OutPutType = OUTPUT_TYPE_HDMI_SD_UPSCALE;
                    } else {
                        OutPutType = OUTPUT_TYPE_HDMI_NOSCALE;
                    }
                }
            }
        }
    }

    SYS_LOGD("%s OutPutType %d\n", __FUNCTION__, OutPutType);
    return OutPutType;
}

output_type_t CPQControl::CheckOutPutMode(tv_source_input_t source_input)
{
    output_type_t OutPutType = OUTPUT_TYPE_LVDS;
    if (!isFileExist(HDMI_OUTPUT_CHECK_PATH)) {//LVDS output
        OutPutType = OUTPUT_TYPE_LVDS;
    } else {
        int outputFrameHeight = 1080;
        char outputModeBuf[32] = {0};
        if ((pqReadSys(DISPLAY_MODE, outputModeBuf, sizeof(outputModeBuf)) < 0) || (strlen(outputModeBuf) == 0)) {
            SYS_LOGD("Read DISPLAY_MODE failed!\n");
        } else {
            SYS_LOGD( "%s: current output mode is %s!\n", __FUNCTION__, outputModeBuf);
            if (strstr(outputModeBuf, "null")) {
                return OUTPUT_TYPE_MAX;
            } else if (strstr(outputModeBuf, "480cvbs")) {//NTSC output
                OutPutType = OUTPUT_TYPE_NTSC;
            } else if(strstr(outputModeBuf, "576cvbs")) {//PAL output
                OutPutType = OUTPUT_TYPE_PAL;
            } else {//HDMI output
                char tempBuf[32] = {0};
                int outputModeStrSize = strlen(outputModeBuf);
                strncpy(tempBuf, outputModeBuf, (outputModeStrSize-4));//delete "xxhz"
                SYS_LOGD( "%s: size is %d, str is : %s!\n", __FUNCTION__, outputModeStrSize, tempBuf);
                if (strstr(tempBuf, "smpte")) {
                    outputFrameHeight = 4096;
                } else {
                    memset(tempBuf,0, sizeof(tempBuf));
                    strncpy(tempBuf, outputModeBuf, (outputModeStrSize - 5));//delete "pxxhz"
                    outputFrameHeight = atoi(tempBuf);
                }
                SYS_LOGD("%s: outputFrameHeight: %d!\n", __FUNCTION__, outputFrameHeight);

                if (strstr(outputModeBuf, "120hz")) {
                    mDisplayMode4k120 = true;
                } else if (strstr(outputModeBuf, "100hz")) {
                    mDisplayMode4k100 = true;
                } else {
                    mDisplayMode4k120 = false;
                    mDisplayMode4k100= false;
                }
                SYS_LOGD("%s: mDisplayMode4k120:%d mDisplayMode4k100:%d!\n", __FUNCTION__, mDisplayMode4k120, mDisplayMode4k100);

                //check outputmode
                if ((source_input == SOURCE_MPEG)
                    || (source_input == SOURCE_DTV)
                    || (source_input == SOURCE_HDMI1)
                    || (source_input == SOURCE_HDMI2)
                    || (source_input == SOURCE_HDMI3)
                    || (source_input == SOURCE_HDMI4)) {//hdmi/dtv/mpeg input
                    int inputFrameHeight = Cpq_GetInputVideoFrameHeight(source_input);
                    OutPutType = MapDbTvoutWithIOResolution(inputFrameHeight, outputFrameHeight);
                } else {//atv/av input
                    if (outputFrameHeight >= 720) {
                        OutPutType = OUTPUT_TYPE_HDMI_SD_UPSCALE;
                    } else {
                        OutPutType = OUTPUT_TYPE_HDMI_NOSCALE;
                    }
                }
            }
        }
    }

    SYS_LOGD("%s: output mode is %d!\n", __FUNCTION__, OutPutType);
    mPQdb->mOutPutType = OutPutType;
    return OutPutType;
}

bool CPQControl::isCVBSParamValid(void)
{
    bool ret = mPQdb->CheckCVBSParamValidStatus();
    if (ret) {
        SYS_LOGI("cvbs param exist!\n");
    } else {
        SYS_LOGI("cvbs param don't exist!\n");
    }
    return ret;
}

bool CPQControl::isPqDatabaseMachChip()
{
    bool matchStatus = false;
    meson_cpu_ver_e chipVersion = MESON_CPU_VERSION_NULL;
    database_attribute_t dbAttribute;
    mPQdb->PQ_GetDataBaseAttribute(&dbAttribute);
    if ((dbAttribute.ChipVersion.c_str() == NULL) || (dbAttribute.ChipVersion.length() == 0)) {
        SYS_LOGI("%s: ChipVersion is null!\n", __FUNCTION__);
        chipVersion = MESON_CPU_VERSION_NULL;
    } else {
        std::string TempStr = std::string(dbAttribute.ChipVersion.c_str());
        int flagPosition = TempStr.find("_");
        std::string versionStr = TempStr.substr(flagPosition+1, 1);
        SYS_LOGI("%s: versionStr is %s!\n", __FUNCTION__, versionStr.c_str());
        if (versionStr == "A") {
            chipVersion = MESON_CPU_VERSION_A;
        } else if (versionStr ==  "B") {
            chipVersion = MESON_CPU_VERSION_B;
        } else if (versionStr == "C") {
            chipVersion = MESON_CPU_VERSION_C;
        } else {
            chipVersion = MESON_CPU_VERSION_NULL;
        }
    }

    if (chipVersion == MESON_CPU_VERSION_NULL) {
        SYS_LOGI("%s: database don't have chipversion!\n", __FUNCTION__);
        matchStatus = true;
    } else {
        int ret = VPPDeviceIOCtl(AMVECM_IOC_S_MESON_CPU_VER, &chipVersion);
        if (ret < 0) {
            SYS_LOGE("%s: database don't match chip!\n", __FUNCTION__);
            matchStatus = false;
        } else {
            SYS_LOGI("%s: database is match chip!\n", __FUNCTION__);
            matchStatus = true;
        }
    }

    return matchStatus;
}

int CPQControl::Cpq_SetVadjEnableStatus(int isvadj1Enable, int isvadj2Enable)
{
    SYS_LOGD("%s: isvadj1Enable = %d, isvadj2Enable = %d.\n", __FUNCTION__, isvadj1Enable, isvadj2Enable);
    int ret = -1;
    if ((!mbCpqCfg_amvecm_basic_enable) && (!mbCpqCfg_amvecm_basic_withOSD_enable)) {
        ret = 0;
        SYS_LOGD("%s: all vadj module disabled.\n", __FUNCTION__);
    } else {
        am_pic_mode_t params;
        memset(&params, 0, sizeof(params));
        params.flag |= (0x1<<6);
        params.vadj1_en = isvadj1Enable;
        params.vadj2_en = isvadj2Enable;
        ret = VPPDeviceIOCtl(AMVECM_IOC_S_PIC_MODE, &params);
        if (ret < 0) {
            SYS_LOGE("%s error: %s!\n", __FUNCTION__, strerror(errno));
        }
    }

    return ret;
}

bool CPQControl::isBootvideoStopped()
{
    int readLength = 0;
    char* end = NULL;
    char buf[PROPERTY_VALUE_MAX] = {0};
    bool ret = true;

    readLength = property_get(BOOTVIDEO_ENABLE_PROP, buf, "3050");
    SYS_LOGD("%s: bootvideo enable value is %s!\n", __FUNCTION__, buf);
    if (readLength > 0) {
        int bootVideoEnable = (strtol(buf, &end, 0)) / 1000;
        if (bootVideoEnable == 3) {
            memset(buf, 0, sizeof(buf));
            readLength = property_get(BOOTVIDEO_EXIT_PROP, buf, "0");
            if (readLength > 0) {
                if (strcmp(buf, "1") == 0) {
                    ret = false;
                } else {
                    ret = true;
                }
            } else {
                SYS_LOGE("%s: getprop %s error!\n", __FUNCTION__, BOOTVIDEO_EXIT_PROP);
                ret = true;
            }
        } else {
            SYS_LOGD("%s: bootvideo don't enable!\n", __FUNCTION__);
            ret = true;
        }
    } else {
        SYS_LOGE("%s: getprop %s error!\n", __FUNCTION__, BOOTVIDEO_ENABLE_PROP);
        ret = true;
    }

    return ret;
}

void CPQControl::resetPQUiSetting(void)
{
    int ret = 0, i = 0, j = 0, k = 0, config_val = 0;
    vpp_pictur_mode_para_t picture;
    const char *buf = NULL;
    pq_src_param_t src;

    for (i = SOURCE_TV; i < SOURCE_MAX; i++) {
        for (j = PQ_FMT_DEFAULT; j < PQ_FMT_MAX; j++) {
            src.pq_source_input = (tv_source_input_t)i;
            src.pq_sig_fmt = (pq_sig_fmt_t)j;

            //picture
            if (j == PQ_FMT_DOLBY) {
                config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_DV_PICTUREMODE_DEF, VPP_PICTURE_MODE_AMDV_BRIGHT);
            } else {
                config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_PICTUREMODE_DEF, VPP_PICTURE_MODE_STANDARD);
            }
            mSSMAction->SSMSavePictureMode(i * PQ_FMT_MAX + j, config_val);
            mSSMAction->SSMSaveLastPictureMode(i * PQ_FMT_MAX + j, config_val);

            //picture mode params
            for (k = VPP_PICTURE_MODE_STANDARD; k < VPP_PICTURE_MODE_MAX; k++) {
                if (mPQdb->PQ_GetPictureModeParams(src, vpp_picture_mode_t(k), &picture) == 0) {
                    //patch start: for PM5 MpegNr level manage deblock&demosquito two ui default value
                    picture.Deblock = picture.MpegNr;
                    picture.Demosquito = picture.MpegNr;
                    //patch end

                    ret = SetPictureModeData(src, vpp_picture_mode_t(k), &picture);
                } else {
                    ret = RsetPictureModeData(src, vpp_picture_mode_t(k));
                }
            }
        }
    }
    
    for (int i = VPP_COLOR_TEMPERATURE_MODE_STANDARD; i < VPP_COLOR_TEMPERATURE_MODE_MAX; i++) {
        //user colortemp offset
        int offset = i * sizeof(int);
        config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_RGBGAIN_R_DEF, 0);
        mSSMAction->SSMSaveRGBGainRStart(offset, 0);

        config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_RGBGAIN_G_DEF, 0);
        mSSMAction->SSMSaveRGBGainGStart(offset, 0);

        config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_RGBGAIN_B_DEF, 0);
        mSSMAction->SSMSaveRGBGainBStart(offset, 0);

        config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_RGBPOSTOFFSET_R_DEF_DEF, 0);
        mSSMAction->SSMSaveRGBPostOffsetRStart(offset, 0);

        config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_RGBPOSTOFFSET_G_DEF_DEF, 0);
        mSSMAction->SSMSaveRGBPostOffsetGStart(offset, 0);

        config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_RGBPOSTOFFSET_B_DEF_DEF, 0);
        mSSMAction->SSMSaveRGBPostOffsetBStart(offset, 0);
    }

    //PQ Module Demo State
    for (int i = PQ_DEMO_MEMC; i < PQ_DEMO_MAX; i++) {
        mSSMAction->SSMSavePQModuleDemoState(i, 0);
    }

    return;
}

void CPQControl::resetPQTableSetting(void)
{
    int ret = 0, i = 0, j = 0, config_val = 0;
    const char *buf = NULL;

    for (i = SOURCE_TV; i < SOURCE_MAX; i++) {

		config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_GAMMALEVEL_DEF, VPP_GAMMA_CURVE_6);
        mSSMAction->SSMSaveGammaValue(i, VPP_GAMMA_CURVE_6);

        config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_MCDI_DEF, VPP_MCDI_MODE_STANDARD);
        mSSMAction->SSMSaveMcDiMode(i, config_val);

        //MEMC
        config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_MEMCMODE_DEF, VPP_MEMC_MODE_HIGH);
        mSSMAction->SSMSaveMemcMode(i, config_val);

        buf = mPQConfigFile->GetString(CFG_SECTION_PQ, CFG_MEMCDEBLURLEVEL_DEF, NULL);
        int Deblur_para[VPP_MEMC_MODE_MAX] = { 0, 3, 6, 10 };
        pqTransformStringToInt(buf, Deblur_para);
        for (j = VPP_MEMC_MODE_OFF; j < VPP_MEMC_MODE_MAX; j++) {
            mSSMAction->SSMSaveMemcDeblurLevel(i * VPP_MEMC_MODE_MAX + j, Deblur_para[j]);
        }

        buf = mPQConfigFile->GetString(CFG_SECTION_PQ, CFG_MEMCDEJUDDERLEVEL_DEF, NULL);
        int DeJudder_para[VPP_MEMC_MODE_MAX] = { 0, 3, 6, 10 };
        pqTransformStringToInt(buf, DeJudder_para);
        for (j = VPP_MEMC_MODE_OFF; j < VPP_MEMC_MODE_MAX; j++) {
            mSSMAction->SSMSaveMemcDeJudderLevel(i * VPP_MEMC_MODE_MAX + j, DeJudder_para[j]);
        }

        //DISPLAY MODE
        config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_DISPLAYMODE_DEF, VPP_DISPLAY_MODE_NORMAL);
        mSSMAction->SSMSaveDisplayMode(i, config_val);
    }

    config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_LOCALDIMMING_DEF, VPP_PQ_LV_OFF);
    mSSMAction->SSMSaveLocalDimming(config_val);

    config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_COLORDEMOMODE_DEF, VPP_COLOR_DEMO_MODE_ALLON);
    mSSMAction->SSMSaveColorDemoMode(config_val);

    config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_COLORBASEMODE_DEF, VPP_COLOR_BASE_MODE_ENHANCE);
    mSSMAction->SSMSaveColorBaseMode ( VPP_COLOR_BASE_MODE_OPTIMIZE);

    config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_DDRSSC_DEF, 0);
    mSSMAction->SSMSaveDDRSSC(config_val);

    buf = mPQConfigFile->GetString(CFG_SECTION_PQ, CFG_LVDSSSC_DEF, NULL);
    int tmp[3] = {0, 0, 0};
    pqTransformStringToInt(buf, tmp);
    mSSMAction->SSMSaveLVDSSSC(tmp);

    config_val = mPQConfigFile->GetInt(CFG_SECTION_HDMI, CFG_EDID_VERSION_DEF, 0);
    mSSMAction->SSMEdidRestoreDefault(config_val);

    config_val = mPQConfigFile->GetInt(CFG_SECTION_HDMI, CFG_HDCP_SWITCHER_DEF, 0);
    mSSMAction->SSMHdcpSwitcherRestoreDefault(0);

    buf = mPQConfigFile->GetString(CFG_SECTION_HDMI, CFG_COLOR_RANGE_MODE_DEF, "default");
    if (strcmp(buf, "full") == 0) {
        mSSMAction->SSMSColorRangeModeRestoreDefault(1);
    } else if (strcmp(buf, "limit") == 0) {
        mSSMAction->SSMSColorRangeModeRestoreDefault(2);
    } else {
        mSSMAction->SSMSColorRangeModeRestoreDefault(0);
    }
    //static frame
    Cpq_SSMWriteNTypes(SSM_RW_BLACKOUT_ENABLE_START, 1, 1, 0);
    //screen color for signal
    Cpq_SSMWriteNTypes(CUSTOMER_DATA_POS_SCREEN_COLOR_START, 1, 0, 0);

    //EyeProtection
    config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_EYEPROJECTMODE_DEF, 0);
    mSSMAction->SSMSaveEyeProtectionMode(config_val);

    //ai pq/sr/color
    mSSMAction->SSMSaveAipqEnableVal(0);
    mSSMAction->SSMSaveAiSrEnable(1);
    config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_AISRMODE_DEF, 3);
    mSSMAction->SSMSaveAiSrMode(config_val);
    config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_AIPQMODE_DEF, 2);
    mSSMAction->SSMSaveAipqMode(config_val);
    config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_AICOLOR_DEF, 0);
    mSSMAction->SSMSaveAiColor(config_val);

    return;
}

int CPQControl::SetPictureModeData(pq_src_param_t pq_source_input, vpp_picture_mode_t picmode, vpp_pictur_mode_para_t *params)
{
    int ret = -1;
    int isValid = -1;
    int size = sizeof(vpp_pictur_mode_para_t);
    int offset = 0;
    if (size > MAX_PICTUREMODE_PARAM_SIZE) {
        SYS_LOGE("%s error size: %d > ssmdata param len : %d!\n", __FUNCTION__, size, MAX_PICTUREMODE_PARAM_SIZE);
        return -1;
    }

    offset = pq_source_input.pq_source_input * PQ_FMT_MAX * VPP_PICTURE_MODE_MAX + pq_source_input.pq_sig_fmt * VPP_PICTURE_MODE_MAX + picmode;

    ret = mSSMAction->SSMSavePictureModeParams(offset * MAX_PICTUREMODE_PARAM_SIZE, size, (int *)params);
    isValid = 0;
    mSSMAction->SSMReadPictureModeParamsFlag(offset, &isValid);
    if (isValid != 1) {
        isValid = 1;
        ret |= mSSMAction->SSMSavePictureModeParamsFlag(offset, isValid);
    }

    return ret;
}

int CPQControl::RsetPictureModeData(pq_src_param_t pq_source_input, vpp_picture_mode_t picmode)
{
    int ret = -1;
    int isValid = -1;
    int offset = 0;

    offset = pq_source_input.pq_source_input * PQ_FMT_MAX * VPP_PICTURE_MODE_MAX + pq_source_input.pq_sig_fmt * VPP_PICTURE_MODE_MAX + picmode;
    ret = mSSMAction->SSMReadPictureModeParamsFlag(offset, &isValid);
    if (isValid != 0) {
        isValid = 0;
        ret = mSSMAction->SSMSavePictureModeParamsFlag(offset, isValid);
    }

    return ret;
}

int CPQControl::GetPictureModeData(pq_src_param_t pq_source_input, vpp_picture_mode_t picmode, vpp_pictur_mode_para_t *params)
{
    int ret = 0;
    int isValid = -1;;
    int size = sizeof(vpp_pictur_mode_para_t);
    int Offset = 0, OffsetRetry = 0, OffsetDef = 0;
    if (size > MAX_PICTUREMODE_PARAM_SIZE) {
        SYS_LOGE("%s error size: %d > ssmdata param len : %d!\n", __FUNCTION__, size, MAX_PICTUREMODE_PARAM_SIZE);
        return -1;
    }

    Offset =      pq_source_input.pq_source_input * PQ_FMT_MAX * VPP_PICTURE_MODE_MAX + pq_source_input.pq_sig_fmt * VPP_PICTURE_MODE_MAX + picmode;

    OffsetRetry = pq_source_input.pq_source_input * PQ_FMT_MAX * VPP_PICTURE_MODE_MAX + PQ_FMT_DEFAULT * VPP_PICTURE_MODE_MAX + picmode;

    OffsetDef = SOURCE_TV * PQ_FMT_MAX * VPP_PICTURE_MODE_MAX + PQ_FMT_DEFAULT * VPP_PICTURE_MODE_MAX + picmode;

    if (mSSMAction->SSMReadPictureModeParamsFlag(Offset, &isValid) == 0) {
        if (isValid == 1) {
            ret = mSSMAction->SSMReadPictureModeParams(Offset * MAX_PICTUREMODE_PARAM_SIZE, size, (int *)params);
        } else {
            if (mSSMAction->SSMReadPictureModeParamsFlag(OffsetRetry, &isValid) == 0) {
                if (isValid == 1) {
                    ret = mSSMAction->SSMReadPictureModeParams(OffsetRetry * MAX_PICTUREMODE_PARAM_SIZE, size, (int *)params);
                } else {
                    if (mSSMAction->SSMReadPictureModeParamsFlag(OffsetDef, &isValid) == 0) {
                        if (isValid == 1) {
                            ret = mSSMAction->SSMReadPictureModeParams(OffsetDef * MAX_PICTUREMODE_PARAM_SIZE, size, (int *)params);
                        } else {
                            SYS_LOGD("%s  Offset:%d  OffsetRetry: %d, OffsetDef: %d, all NULL!!! check XML DB\n", __FUNCTION__, Offset, OffsetRetry, OffsetDef);
                            ret = -1;
                            return ret;
                        }
                    }
                }
            }
        }
    }

    return ret;
}

int CPQControl::Set_PictureMode(vpp_picture_mode_t pq_mode, pq_src_param_t source_input_param, pq_mode_switch_type_t switch_type)
{
    int ret = -1;
    vpp_pictur_mode_para_t pq_para;

    SetPcGameMode(pq_mode, switch_type);

    SetFacColorParams(mCurrentSourceInputInfo, pq_mode);

    ret = GetPictureModeData(source_input_param, pq_mode, &pq_para);

    if (ret == 0) {
        ret |= Cpq_SetBrightness(pq_para.Brightness, mCurrentSourceInputInfo);
        ret |= Cpq_SetContrast(pq_para.Contrast, mCurrentSourceInputInfo);
        ret |= Cpq_SetSaturation(pq_para.Saturation, mCurrentSourceInputInfo);
        ret |= Cpq_SetHue(pq_para.Hue, mCurrentSourceInputInfo);
        ret |= Cpq_SetSharpness(pq_para.Sharpness, mCurrentSourceInputInfo);
        ret |= Cpq_SetNoiseReductionMode((vpp_noise_reduction_mode_t)pq_para.Nr, mCurrentSourceInputInfo);
        ret |= Cpq_SetDnlpMode((Dynamic_contrast_status_t)pq_para.DynamicContrast, mCurrentSourceInputInfo);
        ret |= Cpq_SetLocalContrastMode((local_contrast_mode_t)pq_para.LocalContrast);
        ret |= Cpq_SetColorGamutMode((vpp_colorgamut_mode_t)pq_para.ColorGamut, mCurrentSourceInputInfo);
        ret |= Cpq_BlackStretch(pq_para.BlackStretch, mCurrentSourceInputInfo);
        ret |= Cpq_BlueStretch(pq_para.BlueStretch, mCurrentSourceInputInfo);
        ret |= Cpq_ChromaCoring(pq_para.ChromaCoring, mCurrentSourceInputInfo);
        ret |= Cpq_SetMpegNr((vpp_pq_level_t)pq_para.MpegNr, mCurrentSourceInputInfo);
        ret |= Cpq_SetSmoothPlusMode((vpp_smooth_plus_mode_t)pq_para.SmoothPlus, mCurrentSourceInputInfo);

        //dobly mode
        if (pq_para.DvMode >= 0) {
            ret |= mDolbyVision->SetDolbyPQMode((dolby_pq_mode_t) pq_para.DvMode);
        }
        //dolby dark Detail
        if (pq_para.DvDarkDetail >= 0) {
            ret |= Cpq_SetDolbyDarkDetail(pq_para.DvDarkDetail);
        }

		//colortemp
        Cpq_CheckColorTemperatureParamAlldata(mCurrentSourceInputInfo);
        ret |= Cpq_SetColorTemperatureWithoutSave((vpp_color_temperature_mode_t)pq_para.ColorTemperature, mCurrentSourceInputInfo.source_input);

    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

void CPQControl::SetPcGameMode(vpp_picture_mode_t pq_mode, pq_mode_switch_type_t switch_type)
{
    if (switch_type >= PQ_MODE_SWITCH_TYPE_MAX) {
        return;
    }

    if ((mCurrentSourceInputInfo.source_input == SOURCE_HDMI1) ||
          (mCurrentSourceInputInfo.source_input == SOURCE_HDMI2) ||
          (mCurrentSourceInputInfo.source_input == SOURCE_HDMI3) ||
          (mCurrentSourceInputInfo.source_input == SOURCE_HDMI4)) {//HDMI source;

        if (mLastPictureMode == VPP_PICTURE_MODE_GAME) {
            if (pq_mode == VPP_PICTURE_MODE_GAME) {
                setPQModeByTvService(MODE_ON, MODE_OFF, switch_type);//game mode on and monitor mode off;
            } else if (pq_mode == VPP_PICTURE_MODE_MONITOR) {
                setPQModeByTvService(MODE_OFF, MODE_ON, switch_type);//game mode off and monitor mode on;
            } else {
                setPQModeByTvService(MODE_OFF, MODE_OFF, switch_type);//game mode off and monitor mode off;
            }
        } else if (mLastPictureMode == VPP_PICTURE_MODE_MONITOR) {
            if (pq_mode == VPP_PICTURE_MODE_MONITOR) {
                setPQModeByTvService(MODE_OFF, MODE_ON, switch_type);//game mode off and monitor mode on;
            } else if (pq_mode == VPP_PICTURE_MODE_GAME) {
                setPQModeByTvService(MODE_ON, MODE_OFF, switch_type);//game mode on and monitor mode off;
            } else {
                setPQModeByTvService(MODE_OFF, MODE_OFF, switch_type);//game mode off and monitor mode off;
            }
        } else {
            if (pq_mode == VPP_PICTURE_MODE_GAME) {
                setPQModeByTvService(MODE_ON, MODE_OFF, switch_type);//game mode on and monitor mode off;
            } else if (pq_mode == VPP_PICTURE_MODE_MONITOR) {
                setPQModeByTvService(MODE_OFF, MODE_ON, switch_type);//game mode off and monitor mode on;
            } else {
                setPQModeByTvService(MODE_OFF, MODE_OFF, PQ_MODE_SWITCH_TYPE_INIT);//game mode off and monitor mode off;
            }
        }
    } else {//other source;
        if (mInitialized) {
            setPQModeByTvService(MODE_OFF, MODE_OFF, PQ_MODE_SWITCH_TYPE_INIT);
        }
    }

    return;
}

pq_sig_fmt_t CPQControl::CheckPQTimming(hdr_type_t hdr_type)
{
    pq_sig_fmt_t timming = PQ_FMT_DEFAULT;
    switch (hdr_type) {
        case HDR_TYPE_HDR10:
            timming = PQ_FMT_HDR;
            break;
        case HDR_TYPE_HDR10PLUS:
            timming = PQ_FMT_HDRP;
            break;
        case HDR_TYPE_DOVI:
            timming = PQ_FMT_DOLBY;
            break;
        case HDR_TYPE_HLG:
            timming = PQ_FMT_HLG;
            break;
        case HDR_TYPE_SDR:
            timming = PQ_FMT_SDR;
            break;
        case HDR_TYPE_NONE:
        case HDR_TYPE_PRIMESL:
        case HDR_TYPE_MVC:
        default:
            timming = PQ_FMT_DEFAULT;
            break;
    }

    return timming;
}

void CPQControl::InitLocalDimmingBin(void)
{
    int i;
    for (i = 0; i < LD_BIN_BL_MAX; i++) {
        if (LoadLdBin((LD_bin_table_index_t)i) < 0) {
            SYS_LOGE("%s table is not exist %d\n", __FUNCTION__, i);
        } else {
            SYS_LOGI("%s LOAD LD bin index: %d  \n", __FUNCTION__, i);
        }
    }
}

int CPQControl::LoadLdBin(LD_bin_table_index_t index)
{
    int ret = -1;
    int fd = -1;
    int dataSize = 0;
    unsigned char *dataBuff = NULL;
    unsigned char buf[MAX_TABLE_SIZE/sizeof(unsigned char)] = {0};
    char path[256] = {0};
    int count_retry = 20;

    am_pq_bin_param_s para;
    memset(&para, 0x0, sizeof(am_pq_bin_param_s));

    // get bin path
    if (AMHal_VPQ_Get_LDBinPath(path, index) < 0) {
        SYS_LOGE("%s AMHal_VPQ_Get_LDBinPath fail \n", __FUNCTION__);
        return -1;
    }

    // read bin data
    if (isFileExist(path) == false) {
        return -1;
    }
    if ((fd = open(path, O_RDONLY)) < 0) {
        SYS_LOGE("Open %s error(%s)!\n", path, strerror(errno));
        return -1;
    } else {
        dataSize = read(fd, buf, sizeof(buf));
        if (dataSize > 0) {
            dataBuff = (unsigned char *)malloc(dataSize);
            if (dataBuff != NULL) {
                memset(dataBuff, 0x0, dataSize);
            } else {
                SYS_LOGE("%s malloc memory fail \n", __FUNCTION__);
                ret = -1;
                goto exit;
            }
            memcpy((void *)dataBuff, buf, dataSize);
            para.table_index = index;
            para.table_len = dataSize;
            para.table_ptr = (long long)dataBuff;
            ret = 0;
        } else {
            SYS_LOGE("%s size is NULL \n", __FUNCTION__);
            ret = -1;
            goto exit;
        }
    }

    //set bin data to driver
    while (count_retry) {
        ret = AMHal_VPQ_Set_LDBinData(&para, index);
        if (ret < 0) {
            SYS_LOGE("%s, error(%s), errno(%d)\n", __FUNCTION__, strerror(errno), errno);
            if (errno == EBUSY) {
                SYS_LOGE("%s, %s, retry...\n", __FUNCTION__, strerror(errno));
                count_retry--;
                continue;
            }
        }
        break;
    }

    close(fd);
    free(dataBuff);
    return ret;

    exit:
        close(fd);
        return ret;
}


void CPQControl::InitTconGamma(void)
{
    int ret = -1;
    gm_tbl_t tconGmTbl;
    memset(&tconGmTbl, 0, sizeof(gm_tbl_t));

    for (int i = 0; i < 10; i++) {
        ret  = mPQdb->PQ_GetTconGammaTable(i, &tconGmTbl);

        if (ret < 0) {
            SYS_LOGE("%s, PQ_GetTconGammaTable %d file...\n", __FUNCTION__, i);
            return;
        }
    }

    ret = VPPDeviceIOCtl(AMVECM_IOC_GAMMA_SET, &tconGmTbl);

    if (ret < 0) {
        SYS_LOGE("%s error: %s!\n", __FUNCTION__, strerror(errno));
    }

    return;
}

void CPQControl::InitPGammaBin()
{
    char propbuf[PROPERTY_VALUE_MAX] = {0};
    bool autogen = false;

    if (property_get(PROP_PGAMMA_AUTO_GEN, propbuf, "0") > 0) {
        SYS_LOGE("Prop [%s]=%s\n", PROP_PGAMMA_AUTO_GEN, propbuf);
        if (!strcasecmp(propbuf, "true") || !strcmp(propbuf, "1"))
            autogen = true;
    } else {
        SYS_LOGE("getprop [%s] fail\n", PROP_PGAMMA_AUTO_GEN);
    }

    if (autogen) {
        CTconPGamma *pgammaDev = CTconPGamma::GetInstance();
        if (pgammaDev && !pgammaDev->Init(NULL)) {
            pgammaDev->PrintInfo(-1);
            if (pgammaDev->GenerateBin((char *)"default") < 0)
                SYS_LOGE("Gen pgamma bin failed, exit...\n");
            else
                SYS_LOGD("Gen pgamma bin Ok\n");
        }

        if (pgammaDev)
        pgammaDev->UnInit();
    }
}

void CPQControl::InitTconlessBin(void)
{
    int ret = -1;
    unsigned int i;
    unsigned int max_cnt = 0;

    if (AML_HAL_LCD_GetTconBinMaxCnt(&max_cnt) != API_OK) {
        SYS_LOGE("%s AMHal_VPQ_Get_TconlessBinMax FAIL max_cnt = %d \n", __FUNCTION__, max_cnt);
        return;
    }

    for (i = 0; i < max_cnt; i++) {
        if (LoadTconlessBin(i) < 0) {
            SYS_LOGE("%s table %d is not exist  \n", __FUNCTION__, i);
        } else {
            SYS_LOGD("%s LOAD tcon bin index: %d  \n", __FUNCTION__, i);
        }
    }

    // handle pgamma bin
    InitPGammaBin();
}

int CPQControl::LoadTconlessBin(unsigned int index)
{
    int ret = 0;
    int fd = -1;
    int sizeHeader = 0;
    int sizeData = 0;
    unsigned char * dataBuff = NULL;
    unsigned char buff[MAX_TABLE_SIZE/sizeof(unsigned char)] = {0};
    int count_retry = 20;

    am_pq_bin_param_t param;
    aml_path_t path;
    memset(&param, 0, sizeof(am_pq_bin_param_t));
    memset(&path, 0, sizeof(aml_path_t));

    //set index to driver
    if (AML_HAL_LCD_SetTconDataIndex(index) != API_OK) {
        SYS_LOGE("%s AMHal_VPQ_Set_TconlessBinIndex fail \n", __FUNCTION__);
        return -1;
    }

    // get bin path
    if (AML_HAL_LCD_GetTconBinPath((HAL_aml_path_s *)&path) != API_OK) {
        SYS_LOGE("%s AMHal_VPQ_Get_TconlessBinPath fail \n", __FUNCTION__);
        return -1;
    } else {
        SYS_LOGD("%s INDEX:{ %d } ====> GetBinPath = %s!\n",__FUNCTION__, index, path.string);
    }

    if (strstr(path.string, "demura")) {
        char propbuf[PROPERTY_VALUE_MAX] = {0};
        bool autogen = false;
        if (property_get(PROP_DEMURA_AUTO_GEN, propbuf, "0") > 0) {
            SYS_LOGD("Prop [%s]=%s\n", PROP_DEMURA_AUTO_GEN, propbuf);
            if (!strcasecmp(propbuf, "true") || !strcmp(propbuf, "1"))
                autogen = true;
        } else {
            SYS_LOGE("getprop [%s] fail\n", PROP_DEMURA_AUTO_GEN);
        }
        if (autogen) {
            SYS_LOGD("Detect demura binary, try to gen %s...\n", path.string);
            CTconDemura *demuraDev = CTconDemura::GetInstance();
            if (demuraDev && !demuraDev->Init(NULL)) {
                demuraDev->PrintInfo(-1);
                if (demuraDev->GenerateBin(path.string) < 0)
                    SYS_LOGE("Gen %s failed, exit...\n", path.string);
                else
                    SYS_LOGD("Gen %s Ok\n", path.string);
            }
            if (demuraDev)
                demuraDev->UnInit();
        }
    }

    // read bin data
    if (isFileExist(path.string) == false) {
        return -1;
    }
    if ((fd = open(path.string, O_RDONLY)) < 0) {
        SYS_LOGE("Open %s error(%s)!\n", path.string, strerror(errno));
        return -1;
    }
    sizeData = read(fd, buff, sizeof(buff));
    if (sizeData <= 0) {
        SYS_LOGE("%s ERROR !!!data size[%d]\n", __FUNCTION__,sizeData);
        ret = -1;
        goto exit;
    } else {
        dataBuff = (unsigned char *)malloc(sizeData);
        if (dataBuff != NULL) {
            memset(dataBuff, 0x0, sizeData);
        } else {
            SYS_LOGE("%s malloc memory fail \n", __FUNCTION__);
            ret = -1;
            goto exit;
        }
        memcpy((void *)dataBuff, buff, sizeData);
        param.table_index= index;
        param.table_len= sizeData;
        param.table_ptr = (long long) dataBuff;
        ret = 0;
    }

    //set bin data to driver
    while (count_retry) {
        if (AML_HAL_LCD_SetTconBinData((HAL_am_pq_bin_param_s *)&param) !=  API_OK) {
            SYS_LOGE("%s, error(%s), errno(%d)\n", __FUNCTION__, strerror(errno), errno);
            if (errno == EBUSY) {
                SYS_LOGE("%s, %s, retry...\n", __FUNCTION__, strerror(errno));
                count_retry--;
                continue;
            }
        }
        break;
    }

    close(fd);
    free(dataBuff);
    return ret;

    exit:
        close(fd);
        return ret;
}



int CPQControl::AMHal_VPQ_Get_LDBinPath(char *path, LD_bin_table_index_t index)
{
    int ret = -1;
    aml_path_s driverPath;
    memset(&driverPath, 0, sizeof(aml_path_s));

    switch (index) {
    case LD_BIN_BL_MAPPING:
        if (LDDeviceIOCtl(AML_LDIM_IOC_CMD_GET_BL_MAPPING_PATH, &driverPath) < 0) {
            SYS_LOGE("%s GET BL_MAPPING PATH error(%s)!\n", __FUNCTION__, strerror(errno));
            ret = -1;
        } else {
            strncpy(path, driverPath.string, strlen(driverPath.string) + 1);
            SYS_LOGD("%s BL MAPPING PATH = %s!\n",__FUNCTION__, path);
            ret = 0;
        }
        break;
    case LD_BIN_BL_PROFILE:
        if (LDDeviceIOCtl(AML_LDIM_IOC_CMD_GET_BL_PROFILE_PATH, &driverPath) < 0) {
            SYS_LOGE("%s GET BL_PROFILE PATH error(%s)!\n", __FUNCTION__, strerror(errno));
            ret = -1;
        } else {
            strncpy(path, driverPath.string, strlen(driverPath.string) + 1);
            SYS_LOGD("%s BL PROFILE PATH = %s!\n",__FUNCTION__, path);
            ret = 0;
        }
        break;
    default:
        break;
    }

    return ret;
}

int CPQControl::AMHal_VPQ_Set_LDBinData(am_pq_bin_param_s *buff, LD_bin_table_index_t index)
{
    int ret = -1;

    switch (index) {
        case LD_BIN_BL_MAPPING:
            ret = LDDeviceIOCtl(AML_LDIM_IOC_CMD_SET_BL_MAPPING, buff);
            break;
        case LD_BIN_BL_PROFILE:
            ret = LDDeviceIOCtl(AML_LDIM_IOC_CMD_SET_BL_PROFILE, buff);
            break;
        default:
            break;
        }

    return ret;
}

int CPQControl::AMHal_VPQ_Get_TconlessBinMax(unsigned int *cnt)
{
    int ret = -1;
    ret = LCDDeviceIOCtl(LCD_IOC_GET_TCON_BIN_MAX_CNT_INFO, cnt);

    return ret;
}

int CPQControl::AMHal_VPQ_Get_TconlessBinPath(aml_path_t *param)
{
    int ret = -1;
    ret = LCDDeviceIOCtl(LCD_IOC_GET_TCON_BIN_PATH_INFO, param);

    return ret;
}

int CPQControl::AMHal_VPQ_Set_TconlessBinIndex(unsigned int index)
{
    int ret = -1;
    ret = LCDDeviceIOCtl(LCD_IOC_SET_TCON_DATA_INDEX_INFO, &index);

    return ret;
}

int CPQControl::AMHal_VPQ_Set_TconlessBinData(am_pq_bin_param_t *param)
{
    int ret = -1;
    ret = LCDDeviceIOCtl(LCD_IOC_SET_TCON_BIN_DATA_INFO, param);

    return ret;
}

int CPQControl::SetDolbyDarkDetail(int mode, int is_save)
{
    int ret =0;
    SYS_LOGD("%s, mode = %d\n", __FUNCTION__, mode);
    ret = Cpq_SetDolbyDarkDetail(mode);

    if ((ret == 0) && (is_save == 1)) {
        ret = SaveDolbyDarkDetail(mode);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }
    return 0;
}

int CPQControl::GetDolbyDarkDetail(void)
{
    int mode = -1;

    if (mbCpqCfg_new_picture_mode_enable) {
        vpp_picture_mode_t pq_mode = (vpp_picture_mode_t)GetPQMode();
        vpp_pictur_mode_para_t para;
        if (GetPictureModeData(mCurrentPqSource, pq_mode, &para) == 0) {
            mode = para.DvDarkDetail;
        }
    }

    SYS_LOGD("%s, source: %d, timming: %d, mode = %d\n", __FUNCTION__, mSourceInputForSaveParam, mCurrentPqSource.pq_sig_fmt, mode);
    return mode;
}

int CPQControl::SaveDolbyDarkDetail(int value)
{
    SYS_LOGD("%s, source: %d, timming: %d value = %d\n", __FUNCTION__, mSourceInputForSaveParam, mCurrentPqSource.pq_sig_fmt, value);
    int ret = -1;
    if (mbCpqCfg_new_picture_mode_enable) {
        vpp_pictur_mode_para_t para;
        vpp_picture_mode_t pq_mode = (vpp_picture_mode_t)GetPQMode();
        if (GetPictureModeData(mCurrentPqSource, pq_mode, &para) == 0) {
            para.DvDarkDetail = value;
            ret = SetPictureModeData(mCurrentPqSource, pq_mode, &para);
        }
    }

    if (ret < 0)
        SYS_LOGE("%s failed!\n",__FUNCTION__);

    return ret;
}


int CPQControl::Cpq_SetDolbyDarkDetail(int mode)
{
    int ret = -1;

    ret = mDolbyVision->SetDolbyPQDarkDetail(mode);

    if (ret < 0)
        SYS_LOGE("%s failed!\n",__FUNCTION__);

    return ret;
}

#ifdef DIFFERENTIAL_COMPRESS_PQ_DB
char* CPQControl::CalculateFileSha1(const char* filePath)
{
    SHA_CTX c;
    unsigned char md[SHA_DIGEST_LENGTH];
    int fd;
    int size;
    unsigned char buf[BUFSIZE];
    static char strMd[SHA_DIGEST_LENGTH*2+1];

    if ((fd = open(filePath, O_RDONLY)) < 0) {
        SYS_LOGE("Open %s error(%s)!\n", filePath, strerror(errno));
        return NULL;
    }
    SHA1_Init(&c);
    for (;;)
    {
        size = read(fd, buf, BUFSIZE);
        if (size <= 0) break;
        SHA1_Update(&c, buf, (unsigned long)size);
    }
    SHA1_Final(md, &c);
    close(fd);

    for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
        sprintf(strMd + i*2, "%02x", md[i]);
    strMd[SHA_DIGEST_LENGTH*2] = '\0';
    return strMd;
}

int CPQControl::GenerateTargetPQ()
{
    char basePQPath[128] = {0};
    char diffPQPath[128] = {0};
    char targetPQSha1[128] = {0};
    char basePQSha1[128] = {0};

    const char *config_value = NULL;
    config_value = mPQConfigFile->GetString(CFG_SECTION_PQ_DIFF, CFG_BASE_PQ_PATH, NULL);
    if (!config_value) {
        SYS_LOGE("Can not get basePQPath!\n");
        return -1;
    }
    strcpy(basePQPath, config_value);

    config_value = mPQConfigFile->GetString(CFG_SECTION_PQ_DIFF, CFG_DIFF_PQ_PATH, NULL);
    if (!config_value) {
        SYS_LOGE("Can not get diffPQPath!\n");
        return -1;
    }
    strcpy(diffPQPath, config_value);

    config_value = mPQConfigFile->GetString(CFG_SECTION_PQ_DIFF, CFG_TARGET_PQ_SHA1, NULL);
    if (!config_value) {
        SYS_LOGE("Can not get targetPQSha1!\n");
        return -1;
    }
    strcpy(targetPQSha1, config_value);

    config_value = mPQConfigFile->GetString(CFG_SECTION_PQ_DIFF, CFG_BASE_PQ_SHA1, NULL);
    if (!config_value) {
        SYS_LOGE("Can not get basePQSha1!\n");
        return -1;
    }
    strcpy(basePQSha1, config_value);

    if (!isFileExist(basePQPath) && (!strcmp(basePQSha1, CalculateFileSha1(basePQPath)))) {
        SYS_LOGE("Base pq.db is not exist or sha1 value [%s] is error!\n", CalculateFileSha1(basePQPath));
        return -1;
    }

    if (!isFileExist(diffPQPath)) {
        SYS_LOGE("%s is not exist!\n", CalculateFileSha1(diffPQPath));
        return -1;
    }

    if (pqbspatch(basePQPath, PARAM_PQ_DB_PATH, diffPQPath) != 0) {
        SYS_LOGE("Doing bspatch pq is failed\n");
        return -1;
    }

    if (strcmp(targetPQSha1, CalculateFileSha1(PARAM_PQ_DB_PATH))) {
        SYS_LOGE("Target pq.db sha1 value [%s] is not equal targetPQSha1[%s]!\n", CalculateFileSha1(PARAM_PQ_DB_PATH), targetPQSha1);
        return -1;
    }
    return 0;
}
#endif

bool CPQControl::getBootEnv(const char *name, char *value)
{
    bool result = false;
    char env_name_buffer[100] = {'\0'};

    memset(env_name_buffer, '\0', sizeof(env_name_buffer));
    if (strstr(name, "ubootenv.var.") == NULL) {
        SYS_LOGE("%s uboot_env_name does not include \"ubootenv.var.\" prefix, now add it!", __FUNCTION__);
        sprintf(env_name_buffer, "ubootenv.var.%s", name);
    } else {
        sprintf(env_name_buffer, "%s", name);
    }
/*
    if (pqUbootenv != NULL) {
        const char* p_value = pqUbootenv->getValue(env_name_buffer);
        if (p_value) {
            strcpy(value, p_value);
            SYS_LOGD("%s read [%s]=%s", __FUNCTION__, name, value);
            result = true;
        } else {
            SYS_LOGE("%s get %s failed!\n ", __FUNCTION__, name);
        }
    } else {
        SYS_LOGE("%s get [%s] fail", __FUNCTION__, name);
    }
*/
    return result;
}

int CPQControl::getHdrPolicy(void)
{
    int ret = -1;
    char hdr_policy[9] = {0};

    memset(hdr_policy, 0, sizeof(hdr_policy));
    ret = pqReadSys(PQ_DISPLAY_HDR_POLICY, hdr_policy, (sizeof(hdr_policy)-1));
    if (ret > 0) {
        hdr_policy[ret] = 0;
    } else {
        memset(hdr_policy, 0, sizeof(hdr_policy));
    }
    SYS_LOGD("%s ret %d hdr_policy %s\n", __FUNCTION__, ret, hdr_policy);

    if (strcmp(hdr_policy, "1") == 0) { //adaptive Hdr
        SYS_LOGD("%s adaptive Hdr\n", __FUNCTION__);
        mPQdb->node_number = 1;
    } else if (strcmp(hdr_policy, "0") == 0) { //always hdr
        SYS_LOGD("%s always Hdr\n", __FUNCTION__);
        mPQdb->node_number = 2;
    } else {
        SYS_LOGE("%s hdr policy setting out of range\n", __FUNCTION__);
    }

    return ((ret == true) ? 1 : 0);
}
