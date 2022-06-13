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
#ifdef SUPPORT_TVSERVICE
#include "TvServerHidlClient.h"
#endif

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

}

CPQControl::~CPQControl()
{

}

void CPQControl::CPQControlInit()
{
    mInitialized   = false;
    mAmvideoFd     = -1;
    mDiFd          = -1;
    mbDtvKitEnable = false;

    //open vpp module
    mAmvideoFd = VPPOpenModule();
    if (mAmvideoFd < 0) {
        SYS_LOGE("Open PQ module failed!\n");
    } else {
        SYS_LOGD("Open PQ module success!\n");
    }
    //open DI module
    mDiFd = DIOpenModule();
    if (mDiFd < 0) {
        SYS_LOGE("Open DI module failed!\n");
    } else {
        SYS_LOGD("Open DI module success!\n");
    }

    //open Sys fs
    mSysFs = SysFs::GetInstance();

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
    if (mbCpqCfg_seperate_db_enable) {
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
    mCurentSourceInputInfo.source_input = SOURCE_MPEG;
    mCurentSourceInputInfo.sig_fmt      = TVIN_SIG_FMT_HDMI_1920X1080P_60HZ;
    mCurentSourceInputInfo.trans_fmt    = TVIN_TFMT_2D;
    mSourceInputForSaveParam            = SOURCE_MPEG;
    mCurrentHdrStatus                   = false;
    mCurentAfdInfo                      = TVIN_ASPECT_NULL;

    //check output mode
    mCurentOutputType = CheckOutPutMode(SOURCE_MPEG);

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
    //AI PQ
    AipqInit();
    //cabc pq
    SetCabc();
    //aad pq
    SetAad();
    //Vframe size
    mCDevicePollCheckThread.setObserver(this);
    mCDevicePollCheckThread.StartCheck();
    mInitialized = true;
    //auto backlight
    if (isFileExist(LDIM_PATH)) {
        SetDynamicBacklight((Dynamic_backlight_status_t)GetDynamicBacklight(), 1);
    } else if (isFileExist(mSysFs->getSysNode(BACKLIGHT_AML_BL_BRIGHTNESS))) {//local diming or pwm
        mDynamicBackLight.setObserver(this);
        mDynamicBackLight.startDected();
    } else {
        SYS_LOGD("No auto backlight moudle!\n");
    }
}

void CPQControl::CPQControlUnInit()
{
    //close moduel
    VPPCloseModule();
    //close DI module
    DICloseModule();

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

    mCDevicePollCheckThread.requestExit();

    if (mPQConfigFile != NULL) {
        delete mPQConfigFile;
        mPQConfigFile = NULL;
    }

}

int CPQControl::pqWriteSys(ConstCharforSysFsNodeIndex index, const char *val)
{
    int len = -1;

    len = mSysFs->writeSysfs(index, val);

    return len;
}

int CPQControl::pqReadSys(ConstCharforSysFsNodeIndex index, char *buf, int count)
{
    int len = -1;

    len = mSysFs->readSysfs(index, buf, count);

    return len;
}

int CPQControl::VPPOpenModule(void)
{
    if (mAmvideoFd < 0) {
        mAmvideoFd = open(VPP_DEV_PATH, O_RDWR);
        if (mAmvideoFd < 0) {
            SYS_LOGE("Open amvecm module, error(%s)!\n", strerror(errno));
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
            SYS_LOGE("Open DI module, error(%s)!\n", strerror(errno));
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

void CPQControl::onVframeSizeChange()
{
    char temp[8];
    memset(temp, 0, sizeof(temp));
    int ret = pqReadSys(VIDEO_POLL_STATUS_CHANGE, temp, sizeof(temp));
    if (ret > 0) {
        int eventFlagValue = strtol(temp, NULL, 16);
        SYS_LOGD("%s: event value = %d(0x%x)\n", __FUNCTION__, eventFlagValue, eventFlagValue);
        int frameszieEventFlag      = (eventFlagValue & 0x1) >> 0;
        int hdrTypeEventFlag        = (eventFlagValue & 0x2) >> 1;
        int videoPlayStartEventFlag = (eventFlagValue & 0x4) >> 2;
        int videoPlayStopEventFlag  = (eventFlagValue & 0x8) >> 3;
        //int videoPlayAxisEventFlag = (eventFlagValue & 0x10) >> 4;
        /*SYS_LOGD("%s: frameszieEventFlag = %d,hdrTypeEventFlag = %d,videoPlayStartEventFlag = %d,videoPlayStopEventFlag = %d,videoPlayAxisEventFlag = %d!\n",
                 __FUNCTION__, frameszieEventFlag, hdrTypeEventFlag, videoPlayStartEventFlag, videoPlayStopEventFlag,
                 videoPlayAxisEventFlag);*/
        //check video play start or stop
        if ((videoPlayStartEventFlag == 1) && (videoPlayStopEventFlag == 0)) {
            mbVideoIsPlaying = true;
        } else if ((videoPlayStartEventFlag == 0) && (videoPlayStopEventFlag == 1)) {
            mbVideoIsPlaying = false;
        } else {
            SYS_LOGD("%s: invalid case\n", __FUNCTION__);
        }

        //
        source_input_param_t new_source_input_param;
        if (((mCurentSourceInputInfo.source_input == SOURCE_DTV) || (mCurentSourceInputInfo.source_input == SOURCE_MPEG))
            && (frameszieEventFlag == 1)) {
            if (isBootvideoStopped()) {
                new_source_input_param.sig_fmt = getVideoResolutionToFmt();
                SYS_LOGD("%s: sig_fmt = 0x%x(%d)\n", __FUNCTION__, new_source_input_param.sig_fmt, new_source_input_param.sig_fmt);
                new_source_input_param.source_input = mCurentSourceInputInfo.source_input;
                new_source_input_param.trans_fmt    = mCurentSourceInputInfo.trans_fmt;
                SetCurrentSourceInputInfo(new_source_input_param);
            } else {
                SYS_LOGD("%s: bootvideo don't stop\n", __FUNCTION__);
            }
        }

        if (hdrTypeEventFlag == 0x1) {
            //get hdr type
            hdr_type_t newHdrType = HDR_TYPE_NONE;
            newHdrType            = Cpq_GetSourceHDRType(mCurentSourceInputInfo.source_input);

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
        if (height <= 576) {
            sig_fmt = TVIN_SIG_FMT_HDMI_720X480P_60HZ;
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
    SYS_LOGD("%s!\n", __FUNCTION__);
    SetCurrentSourceInputInfo(mCurentSourceInputInfo);
}

int CPQControl::LoadPQSettings()
{
    int ret = 0;
    const char *config_value;
    config_value = mPQConfigFile->GetString(CFG_SECTION_PQ, CFG_ALL_PQ_MOUDLE_ENABLE, "enable");
    if (strcmp(config_value, "disable") == 0) {
        SYS_LOGD("All PQ moudle disabled!\n");
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
        SYS_LOGD("source_input: %d, sig_fmt: 0x%x(%d), trans_fmt: 0x%x\n", mCurentSourceInputInfo.source_input,
                 mCurentSourceInputInfo.sig_fmt, mCurentSourceInputInfo.sig_fmt, mCurentSourceInputInfo.trans_fmt);

        ret |= Cpq_SetXVYCCMode(VPP_XVYCC_MODE_STANDARD, mCurentSourceInputInfo);

        ret |= Cpq_SetDIModuleParam(mCurentSourceInputInfo);

        di_deblock_mode_t DeblockMode =(di_deblock_mode_t)GetDeblockMode();
        ret |= Cpq_SetDeblockMode(DeblockMode, mCurentSourceInputInfo);

        di_demosquito_mode_t DemoSquitoMode =(di_demosquito_mode_t)GetDemoSquitoMode();
        ret |= Cpq_SetDemoSquitoMode(DemoSquitoMode, mCurentSourceInputInfo);

        vpp_mcdi_mode_t mcdimode =(vpp_mcdi_mode_t)GetMcDiMode();
        ret |= Cpq_SetMcDiMode(mcdimode, mCurentSourceInputInfo);

        vpp_picture_mode_t pqmode = (vpp_picture_mode_t)GetPQMode();
        ret |= Cpq_SetPQMode(pqmode, mCurentSourceInputInfo, PQ_MODE_SWITCH_TYPE_INIT);

        if (mInitialized) {//don't load gamma in device turn on
            vpp_gamma_curve_t GammaLevel = (vpp_gamma_curve_t)GetGammaValue();
            ret |= SetGammaValue(GammaLevel, 1);
        }

        vpp_color_basemode_t baseMode = GetColorBaseMode();
        ret |= SetColorBaseMode(baseMode, 1);

        vpp_color_temperature_mode_t temp_mode = (vpp_color_temperature_mode_t)GetColorTemperature();
        if (mbCpqCfg_whitebalance_enable) {
            if (temp_mode != VPP_COLOR_TEMPERATURE_MODE_USER) {
                Cpq_CheckColorTemperatureParamAlldata(mCurentSourceInputInfo);
                ret |= SetColorTemperature((int)temp_mode, 1);
            } else {
                tcon_rgb_ogo_t param;
                memset(&param, 0, sizeof(tcon_rgb_ogo_t));
                if (Cpq_GetColorTemperatureUser(mCurentSourceInputInfo.source_input, &param) == 0) {
                    ret |= Cpq_SetColorTemperatureUser(mCurentSourceInputInfo.source_input, R_GAIN, 1, param.r_gain);
                    ret |= Cpq_SetColorTemperatureUser(mCurentSourceInputInfo.source_input, G_GAIN, 1, param.g_gain);
                    ret |= Cpq_SetColorTemperatureUser(mCurentSourceInputInfo.source_input, B_GAIN, 1, param.b_gain);
                    ret |= Cpq_SetColorTemperatureUser(mCurentSourceInputInfo.source_input, R_POST_OFFSET, 1, param.r_post_offset);
                    ret |= Cpq_SetColorTemperatureUser(mCurentSourceInputInfo.source_input, G_POST_OFFSET, 1, param.g_post_offset);
                    ret |= Cpq_SetColorTemperatureUser(mCurentSourceInputInfo.source_input, B_POST_OFFSET, 1, param.b_post_offset);
                }
            }
        } else {
            SYS_LOGD("%s: whitebalance moudle disabled!\n", __FUNCTION__);
        }

        int DnlpLevel = GetDnlpMode();
        ret |= SetDnlpMode(DnlpLevel);

        int LocalContrastMode = GetLocalContrastMode();
        ret |= SetLocalContrastMode((local_contrast_mode_t)LocalContrastMode, 1);

        int MemcMode = GetMemcMode();
        ret |= SetMemcMode(MemcMode, 1);

        int DeBlurLevel = GetMemcDeBlurLevel();
        ret |= SetMemcDeBlurLevel(DeBlurLevel, 1);

        int DeJudderLevel = GetMemcDeJudderLevel();
        ret |= SetMemcDeJudderLevel(DeJudderLevel, 1);

        vpp_colorgamut_mode_t mode = (vpp_colorgamut_mode_t)GetColorGamutMode();
        ret |= SetColorGamutMode(mode, 1);

        vpp_display_mode_t display_mode = (vpp_display_mode_t)GetDisplayMode();
        ret |= SetDisplayMode(display_mode, 1);

        //load hdr tmo
        //int HdrTmoMode = GetHDRTMOMode();
        ret |= SetHDRTMOMode(HDR_TMO_DYNAMIC, 1);

        ret |= AiParamLoad();

        vpp_smooth_plus_mode_t smoothplus_mode = VPP_SMOOTH_PLUS_MODE_OFF;
        ret |= Cpq_SetSmoothPlusMode(smoothplus_mode, mCurentSourceInputInfo);
    }
    return ret;
}

int CPQControl::Cpq_LoadRegs(am_regs_t regs)
{
    if (regs.length == 0) {
        SYS_LOGD("%s--Regs is NULL!\n", __FUNCTION__);
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
        SYS_LOGD("%s--Regs is NULL!\n", __FUNCTION__);
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
        SYS_LOGD("Don't have ldim module!\n");
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
    if (pqMode != VPP_PICTURE_MODE_MONITOR) {
        if (mbCpqCfg_blackextension_enable) {
            ret |= SetBlackExtensionParam(source_input_param);
        } else {
            SYS_LOGD("%s: BlackExtension moudle disabled!\n", __FUNCTION__);
        }

        if (mbCpqCfg_sharpness0_enable) {
            ret |= Cpq_SetSharpness0FixedParam(source_input_param);
            ret |= Cpq_SetSharpness0VariableParam(source_input_param);
        } else {
            SYS_LOGD("%s: Sharpness0 moudle disabled!\n", __FUNCTION__);
        }

        if (mbCpqCfg_sharpness1_enable) {
            ret |= Cpq_SetSharpness1FixedParam(source_input_param);
            ret |= Cpq_SetSharpness1VariableParam(source_input_param);
        } else {
            SYS_LOGD("%s: Sharpness1 moudle disabled!\n", __FUNCTION__);
        }
    }

    if (mbCpqCfg_amvecm_basic_enable || mbCpqCfg_amvecm_basic_withOSD_enable) {
        ret |= Cpq_SetBrightnessBasicParam(source_input_param);
        ret |= Cpq_SetContrastBasicParam(source_input_param);
        ret |= Cpq_SetSaturationBasicParam(source_input_param);
        ret |= Cpq_SetHueBasicParam(source_input_param);
    } else {
        SYS_LOGD("%s: brightness contrast saturation hue moudle disabled!\n", __FUNCTION__);
    }

    return ret;
}

int CPQControl::BacklightInit(void)
{
    int ret = 0;
    int backlight = GetBacklight();
    ret = SetBacklight(backlight, 1);

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
        SYS_LOGD("DI moudle disabled!\n");
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
    SYS_LOGD("%s, source: %d, pq_mode: %d\n", __FUNCTION__, mCurentSourceInputInfo.source_input, pq_mode);
    int ret = -1;

    int cur_mode = GetPQMode();
    if (cur_mode == pq_mode) {
        SYS_LOGD("Same PQ mode,no need set again!\n");
        ret = 0;
    } else {
        if (is_autoswitch == PQ_MODE_SWITCH_TYPE_AUTO) {
            ret = Cpq_SetPQMode((vpp_picture_mode_t)pq_mode, mCurentSourceInputInfo, PQ_MODE_SWITCH_TYPE_AUTO);
        } else if (is_autoswitch == PQ_MODE_SWITCH_TYPE_MANUAL) {
            ret = Cpq_SetPQMode((vpp_picture_mode_t)pq_mode, mCurentSourceInputInfo, PQ_MODE_SWITCH_TYPE_MANUAL);
        } else {
            ret = Cpq_SetPQMode((vpp_picture_mode_t)pq_mode, mCurentSourceInputInfo, PQ_MODE_SWITCH_TYPE_INIT);
        }
    }

    if ((ret == 0) && (is_save == 1)) {
        if (cur_mode != pq_mode) {
            SaveLastPQMode(cur_mode);
        }
        SavePQMode(pq_mode);
        if ((mCurentSourceInputInfo.source_input >= SOURCE_HDMI1) &&
            (mCurentSourceInputInfo.source_input <= SOURCE_HDMI4)) {
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
    mSSMAction->SSMReadPictureMode(mSourceInputForSaveParam, &mode);
    if (mode < VPP_PICTURE_MODE_STANDARD || mode >= VPP_PICTURE_MODE_MAX) {
        mode = VPP_PICTURE_MODE_STANDARD;
    }

    SYS_LOGD("%s, source: %d, mode: %d\n", __FUNCTION__, mSourceInputForSaveParam, mode);
    return mode;
}

int CPQControl::SavePQMode(int pq_mode)
{
    int ret = -1;
    SYS_LOGD("%s, source: %d, mode: %d\n", __FUNCTION__, mSourceInputForSaveParam, pq_mode);
    ret = mSSMAction->SSMSavePictureMode(mSourceInputForSaveParam, pq_mode);
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
    mSSMAction->SSMReadLastPictureMode(mSourceInputForSaveParam, &mode);
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
    ret = mSSMAction->SSMSaveLastPictureMode(mSourceInputForSaveParam, pq_mode);
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
        mTvService = new TvServerHidlClient(CONNECT_TYPE_HAL);
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
    if ((mCurentSourceInputInfo.source_input == SOURCE_HDMI1) ||
          (mCurentSourceInputInfo.source_input == SOURCE_HDMI2) ||
          (mCurentSourceInputInfo.source_input == SOURCE_HDMI3) ||
          (mCurentSourceInputInfo.source_input == SOURCE_HDMI4)) {//HDMI source;

        int cur_mode = GetPQMode();
        if (cur_mode == VPP_PICTURE_MODE_GAME) {
            if (pq_mode == VPP_PICTURE_MODE_GAME) {
                setPQModeByTvService(MODE_ON, MODE_OFF, switch_type);//game mode on and monitor mode off;
            } else if (pq_mode == VPP_PICTURE_MODE_MONITOR) {
                setPQModeByTvService(MODE_OFF, MODE_ON, switch_type);//game mode off and monitor mode on;
            } else {
                setPQModeByTvService(MODE_OFF, MODE_OFF, switch_type);//game mode off and monitor mode off;
            }
        } else if (cur_mode == VPP_PICTURE_MODE_MONITOR) {
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
        SYS_LOGD("%s: nr2 moudle disabled!\n", __FUNCTION__);
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
    } else {
        if (pq_mode == VPP_PICTURE_MODE_USER) {
            mSSMAction->SSMReadBrightness(mSourceInputForSaveParam, &pq_para->brightness);
            mSSMAction->SSMReadContrast(mSourceInputForSaveParam, &pq_para->contrast);
            mSSMAction->SSMReadSaturation(mSourceInputForSaveParam, &pq_para->saturation);
            mSSMAction->SSMReadHue(mSourceInputForSaveParam, &pq_para->hue);
            mSSMAction->SSMReadSharpness(mSourceInputForSaveParam, &pq_para->sharpness);
            ret = 0;
        } else {
            if (mbCpqCfg_seperate_db_enable) {
                ret = mpOverScandb->PQ_GetPQModeParams(source_input_param.source_input, pq_mode, pq_para);
            } else {
                ret = mPQdb->PQ_GetPQModeParams(source_input_param.source_input, pq_mode, pq_para);
            }
        }
    }

    if (ret != 0) {
        SYS_LOGE("%s failed!\n", __FUNCTION__);
        return -1;
    } else {
        return 0;
    }
}

//color temperature
int CPQControl::SetColorTemperature(int temp_mode, int is_save, rgb_ogo_type_t rgb_ogo_type, int value)
{
    int ret = -1;
    SYS_LOGD("%s: source:%d, mode: %d\n", __FUNCTION__, mCurentSourceInputInfo.source_input, temp_mode);
    if (mbCpqCfg_whitebalance_enable) {
        if (temp_mode == VPP_COLOR_TEMPERATURE_MODE_USER) {
            ret = Cpq_SetColorTemperatureUser(mCurentSourceInputInfo.source_input, rgb_ogo_type, is_save, value);
        } else {
            ret = Cpq_SetColorTemperatureWithoutSave((vpp_color_temperature_mode_t)temp_mode, mCurentSourceInputInfo.source_input);
        }

        if ((ret == 0) && (is_save == 1)) {
            ret = SaveColorTemperature((vpp_color_temperature_mode_t)temp_mode);
        }
    } else {
        SYS_LOGD("whitebalance moudle disabled!\n");
        ret = 0;
    }

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
    mSSMAction->SSMReadColorTemperature(mSourceInputForSaveParam, &mode);
    if (mode < VPP_COLOR_TEMPERATURE_MODE_STANDARD || mode > VPP_COLOR_TEMPERATURE_MODE_USER) {
        mode = VPP_COLOR_TEMPERATURE_MODE_STANDARD;
    }

    SYS_LOGD("%s: source: %d, mode: %d!\n",__FUNCTION__, mSourceInputForSaveParam, mode);
    return mode;
}

int CPQControl::SaveColorTemperature(int temp_mode)
{
    SYS_LOGD("%s, source: %d, mode = %d\n", __FUNCTION__, mSourceInputForSaveParam, temp_mode);
    int ret = mSSMAction->SSMSaveColorTemperature(mSourceInputForSaveParam, temp_mode);
    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

tcon_rgb_ogo_t CPQControl::GetColorTemperatureUserParam(void) {
    tcon_rgb_ogo_t param;
    memset(&param, 0, sizeof(tcon_rgb_ogo_t));
    Cpq_GetColorTemperatureUser(mCurentSourceInputInfo.source_input, &param);
    return param;
}

int CPQControl::Cpq_SetColorTemperatureWithoutSave(vpp_color_temperature_mode_t Tempmode, tv_source_input_t tv_source_input __unused)
{
    tcon_rgb_ogo_t rgbogo;

    GetColorTemperatureParams(Tempmode, &rgbogo);

    if (GetEyeProtectionMode(mCurentSourceInputInfo.source_input))//if eye protection mode is enable, b_gain / 2.
        rgbogo.b_gain /= 2;

    return Cpq_SetRGBOGO(&rgbogo);
}

int CPQControl::Cpq_CheckColorTemperatureParamAlldata(source_input_param_t source_input_param)
{
    int ret= -1;
    unsigned short ret1 = 0, ret2 = 0;

    ret = Cpq_CheckTemperatureDataLable();
    ret1 = Cpq_CalColorTemperatureParamsChecksum();
    ret2 = Cpq_GetColorTemperatureParamsChecksum();

    if (ret && (ret1 == ret2)) {
        SYS_LOGD("%s, color temperature param lable & checksum ok.\n",__FUNCTION__);
        if (Cpq_CheckColorTemperatureParams() == 0) {
            SYS_LOGD("%s, color temperature params check failed.\n", __FUNCTION__);
            Cpq_RestoreColorTemperatureParamsFromDB(source_input_param);
         }
    } else {
        SYS_LOGD("%s, color temperature param data error.\n", __FUNCTION__);
        Cpq_SetTemperatureDataLable();
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

int CPQControl::Cpq_SetColorTemperatureUser(tv_source_input_t source_input, rgb_ogo_type_t rgb_ogo_type, int is_save, int value)
{
    SYS_LOGD("%s: type: %d, value: %u!\n", __FUNCTION__, rgb_ogo_type, value);
    int ret = -1;
    tcon_rgb_ogo_t rgbogo;
    memset(&rgbogo, 0, sizeof(tcon_rgb_ogo_t));
    ret = Cpq_GetColorTemperatureUser(source_input, &rgbogo);

    switch (rgb_ogo_type)
    {
        case R_GAIN:
            rgbogo.r_gain = (unsigned)value;
        break;
        case G_GAIN:
            rgbogo.g_gain = (unsigned)value;
        break;
        case B_GAIN:
            rgbogo.b_gain = (unsigned)value;
        break;
        case R_POST_OFFSET:
            rgbogo.r_post_offset = value;
        break;
        case G_POST_OFFSET:
            rgbogo.g_post_offset = value;
        break;
        case B_POST_OFFSET:
            rgbogo.b_post_offset = value;
        break;
        default:
            ret = -1;
        break;
    }

    if (GetEyeProtectionMode(source_input) == 1) {
        SYS_LOGD("eye protection mode is enable!\n");
        rgbogo.b_gain /= 2;
    }

    if (ret != -1) {
       ret = Cpq_SetRGBOGO(&rgbogo);
    }

    if ((ret != -1) && (is_save == 1)) {
        ret = Cpq_SaveColorTemperatureUser(source_input, rgb_ogo_type, value);
    }

    if (ret < 0) {
        SYS_LOGD("%s failed!\n", __FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n", __FUNCTION__);
    }
    return ret;
}

int CPQControl::Cpq_GetColorTemperatureUser(tv_source_input_t source_input __unused, tcon_rgb_ogo_t* p_tcon_rgb_ogo)
{
    int ret = 0;
    if (p_tcon_rgb_ogo != NULL) {
        p_tcon_rgb_ogo->en = 1;
        p_tcon_rgb_ogo->r_pre_offset = 0;
        p_tcon_rgb_ogo->g_pre_offset = 0;
        p_tcon_rgb_ogo->b_pre_offset = 0;
        ret |= mSSMAction->SSMReadRGBGainRStart(0, &p_tcon_rgb_ogo->r_gain);
        ret |= mSSMAction->SSMReadRGBGainGStart(0, &p_tcon_rgb_ogo->g_gain);
        ret |= mSSMAction->SSMReadRGBGainBStart(0, &p_tcon_rgb_ogo->b_gain);
        ret |= mSSMAction->SSMReadRGBPostOffsetRStart(0, &p_tcon_rgb_ogo->r_post_offset);
        ret |= mSSMAction->SSMReadRGBPostOffsetGStart(0, &p_tcon_rgb_ogo->g_post_offset);
        ret |= mSSMAction->SSMReadRGBPostOffsetBStart(0, &p_tcon_rgb_ogo->b_post_offset);
    } else {
        SYS_LOGD("%s: buf is null!\n", __FUNCTION__);
        ret = -1;
    }

    if (ret < 0) {
        SYS_LOGD("%s failed!\n", __FUNCTION__);
        ret = -1;
    }

    return ret;
}

int CPQControl::Cpq_SaveColorTemperatureUser(tv_source_input_t source_input __unused, rgb_ogo_type_t rgb_ogo_type, int value)
{
    SYS_LOGD("%s: rgb_ogo_type[%d]:[%d]", __FUNCTION__, rgb_ogo_type, value);

    int ret = 0;
    switch (rgb_ogo_type)
    {
        case R_GAIN:
            ret |= mSSMAction->SSMSaveRGBGainRStart(0, (unsigned)value);
        break;
        case G_GAIN:
            ret |= mSSMAction->SSMSaveRGBGainGStart(0, (unsigned)value);
        break;
        case B_GAIN:
            ret |= mSSMAction->SSMSaveRGBGainBStart(0, (unsigned)value);
        break;
        case R_POST_OFFSET:
            ret |= mSSMAction->SSMSaveRGBPostOffsetRStart(0, value);
        break;
        case G_POST_OFFSET:
            ret |= mSSMAction->SSMSaveRGBPostOffsetGStart(0, value);
        break;
        case B_POST_OFFSET:
            ret |= mSSMAction->SSMSaveRGBPostOffsetBStart(0, value);
        break;
        default:
            ret = -1;
        break;
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n", __FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n", __FUNCTION__);
    }

    return ret;
}

int CPQControl::Cpq_RestoreColorTemperatureParamsFromDB(source_input_param_t source_input_param)
{
    int i = 0;
    tcon_rgb_ogo_t rgbogo;

    for (i = 0; i < 3; i++) {
        mPQdb->PQ_GetColorTemperatureParams((vpp_color_temperature_mode_t) i, source_input_param, &rgbogo);
        SaveColorTemperatureParams((vpp_color_temperature_mode_t) i, rgbogo);
    }

    Cpq_SetColorTemperatureParamsChecksum();

    return 0;
}

int CPQControl::Cpq_CheckTemperatureDataLable(void)
{
    USUC usuc;
    USUC ret;

    mSSMAction->SSMReadRGBOGOValue(SSM_CR_RGBOGO_LEN - 2, 2, ret.c);

    usuc.c[0] = 0x55;
    usuc.c[1] = 0xAA;

    if ((usuc.c[0] == ret.c[0]) && (usuc.c[1] == ret.c[1])) {
        SYS_LOGD("%s, lable ok.\n", __FUNCTION__);
        return 1;
    } else {
        SYS_LOGE("%s, lable error.\n", __FUNCTION__);
        return 0;
    }
}

int CPQControl::Cpq_SetTemperatureDataLable(void)
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
    }

    SYS_LOGD("%s, rgain[%d], ggain[%d],bgain[%d],roffset[%d],goffset[%d],boffset[%d]\n", __FUNCTION__,
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
    }

    SYS_LOGD("%s, rgain[%d], ggain[%d],bgain[%d],roffset[%d],goffset[%d],boffset[%d]\n", __FUNCTION__,
         params.r_gain, params.g_gain, params.b_gain, params.r_post_offset,
         params.g_post_offset, params.b_post_offset);
    return ret;
}

int CPQControl::Cpq_CheckColorTemperatureParams(void)
{
    int i = 0;
    tcon_rgb_ogo_t rgbogo;
    memset (&rgbogo, 0, sizeof (rgbogo));
    for (i = 0; i < 3; i++) {
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
    int ret =0;
    SYS_LOGD("%s, source: %d, value = %d\n", __FUNCTION__, mSourceInputForSaveParam, value);
    ret = Cpq_SetBrightness(value, mCurentSourceInputInfo);

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
    if (GetPQParams(mCurentSourceInputInfo, pq_mode, &pq_para) == 0) {
        data = pq_para.brightness;
    }

    if (data < 0 || data > 100) {
        data = 50;
    }

    SYS_LOGD("%s, source: %d, value = %d\n", __FUNCTION__, mSourceInputForSaveParam, data);
    return data;
}

int CPQControl::SaveBrightness(int value)
{
    SYS_LOGD("%s, source: %d, value = %d\n", __FUNCTION__, mSourceInputForSaveParam, value);
    int ret = mSSMAction->SSMSaveBrightness(mSourceInputForSaveParam, value);
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
        SYS_LOGD("%s: brightness contrast saturation hue moudle disabled!\n", __FUNCTION__);
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
    SYS_LOGD("%s, source: %d, value = %d\n", __FUNCTION__, mSourceInputForSaveParam, value);
    int ret = Cpq_SetContrast(value, mCurentSourceInputInfo);
    if ((ret == 0) && (is_save == 1)) {
        ret = mSSMAction->SSMSaveContrast(mSourceInputForSaveParam, value);
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
    if (GetPQParams(mCurentSourceInputInfo, pq_mode, &pq_para) == 0) {
        data = pq_para.contrast;
    }

    if (data < 0 || data > 100) {
        data = 50;
    }

    SYS_LOGD("%s, source: %d, value = %d\n", __FUNCTION__, mSourceInputForSaveParam, data);
    return data;
}

int CPQControl::SaveContrast(int value)
{
    SYS_LOGD("%s, source: %d, value = %d\n", __FUNCTION__, mSourceInputForSaveParam, value);
    int ret = mSSMAction->SSMSaveContrast(mSourceInputForSaveParam, value);
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
        SYS_LOGD("%s: brightness contrast saturation hue moudle disabled!\n", __FUNCTION__);
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
    SYS_LOGD("%s, source: %d, value = %d\n", __FUNCTION__, mSourceInputForSaveParam, value);
    int ret = Cpq_SetSaturation(value, mCurentSourceInputInfo);
    if ((ret == 0) && (is_save == 1)) {
        ret = mSSMAction->SSMSaveSaturation(mSourceInputForSaveParam, value);
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
    if (GetPQParams(mCurentSourceInputInfo, pq_mode, &pq_para) == 0) {
        data = pq_para.saturation;
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
    int ret = mSSMAction->SSMSaveSaturation(mSourceInputForSaveParam, value);
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
    int satuation_level = 0, hue_level = 0;
    if (mbCpqCfg_amvecm_basic_enable || mbCpqCfg_amvecm_basic_withOSD_enable) {
        if (value >= 0 && value <= 100) {
            satuation_level = value;
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
                ret = mPQdb->PQ_GetSaturationParams(source_input_param, satuation_level, &saturation);
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
        SYS_LOGD("%s: brightness contrast saturation hue moudle disabled!\n", __FUNCTION__);
        ret = 0;
    }

    return ret;
}

//Hue
int CPQControl::SetHue(int value, int is_save)
{
    SYS_LOGD("%s, source: %d, value = %d\n", __FUNCTION__, mSourceInputForSaveParam, value);
    int ret = Cpq_SetHue(value, mCurentSourceInputInfo);
    if ((ret == 0) && (is_save == 1)) {
        ret = mSSMAction->SSMSaveHue(mSourceInputForSaveParam, value);
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
    if (GetPQParams(mCurentSourceInputInfo, pq_mode, &pq_para) == 0) {
        data = pq_para.hue;
    }

    if (data < 0 || data > 100) {
        data = 50;
    }

    SYS_LOGD("%s, source: %d, value = %d\n", __FUNCTION__, mSourceInputForSaveParam, data);
    return data;
}

int CPQControl::SaveHue(int value)
{
    SYS_LOGD("%s, source: %d, value = %d\n", __FUNCTION__, mSourceInputForSaveParam, value);
    int ret = mSSMAction->SSMSaveHue(mSourceInputForSaveParam, value);
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
        SYS_LOGD("%s: brightness contrast saturation hue moudle disabled!\n", __FUNCTION__);
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
    SYS_LOGD("%s, source: %d, value = %d\n", __FUNCTION__, mSourceInputForSaveParam, value);
    int ret = Cpq_SetSharpness(value, mCurentSourceInputInfo);
    if ((ret== 0) && (is_save == 1)) {
        ret = mSSMAction->SSMSaveSharpness(mSourceInputForSaveParam, value);
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
    if (GetPQParams(mCurentSourceInputInfo, pq_mode, &pq_para) == 0) {
        data = pq_para.sharpness;
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
    int ret = mSSMAction->SSMSaveSharpness(mSourceInputForSaveParam, value);
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
                    ret = Cpq_LoadRegs(regs);
                } else {
                    SYS_LOGE("%s: PQ_GetSharpness0Params failed!\n", __FUNCTION__);
                }
            } else {
                SYS_LOGD("%s: pq.db don't match chip!\n", __FUNCTION__);
                ret = 0;
            }
        } else {
            SYS_LOGD("%s: sharpness0 moudle disabled!\n", __FUNCTION__);
            ret = 0;
        }

        if (mbCpqCfg_sharpness1_enable) {
            if (mbDatabaseMatchChipStatus) {
                ret = mPQdb->PQ_GetSharpness1Params(source_input_param, value, &regs);
                if (ret == 0) {
                    ret = Cpq_LoadRegs(regs);
                } else {
                    SYS_LOGE("%s: PQ_GetSharpness1Params failed!\n", __FUNCTION__);
                }
            } else {
                SYS_LOGD("%s: pq.db don't match chip!\n", __FUNCTION__);
                ret = 0;
            }
        } else {
            SYS_LOGD("%s: sharpness1 moudle disabled!\n", __FUNCTION__);
            ret = 0;
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

int CPQControl::Cpq_SetSharpness1VariableParam(source_input_param_t source_input_param)
{
    int ret = -1;

    if (mbDatabaseMatchChipStatus) {
        ret = mPQdb->PQ_SetSharpness1VariableParams(source_input_param);
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
    SYS_LOGD("%s, source: %d, value = %d\n", __FUNCTION__, mSourceInputForSaveParam, nr_mode);
    int ret = Cpq_SetNoiseReductionMode((vpp_noise_reduction_mode_t)nr_mode, mCurentSourceInputInfo);
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
    mSSMAction->SSMReadNoiseReduction(mSourceInputForSaveParam, &mode);
    if (mode < VPP_NOISE_REDUCTION_MODE_OFF || mode > VPP_NOISE_REDUCTION_MODE_AUTO) {
        mode = VPP_NOISE_REDUCTION_MODE_MID;
    }

    SYS_LOGD("%s, source: %d, value = %d\n", __FUNCTION__, mSourceInputForSaveParam, mode);
    return mode;
}

int CPQControl::SaveNoiseReductionMode(int nr_mode)
{
    SYS_LOGD("%s, source: %d, value = %d\n", __FUNCTION__, mSourceInputForSaveParam, nr_mode);
    int ret = mSSMAction->SSMSaveNoiseReduction(mSourceInputForSaveParam, nr_mode);
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
    if (mbCpqCfg_gamma_enable) {
        ret = Cpq_LoadGamma(gamma_curve);
        if ((ret == 0) && (is_save == 1)) {
            ret = mSSMAction->SSMSaveGammaValue(mSourceInputForSaveParam, gamma_curve);
        }
    } else {
        SYS_LOGD("Gamma moudle disabled!\n");
        ret = 0;
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

int CPQControl::Cpq_LoadGamma(vpp_gamma_curve_t gamma_curve)
{
    int ret = 0;
    tcon_gamma_table_t gamma_r, gamma_g, gamma_b;

    ret |= mPQdb->PQ_GetGammaSpecialTable(gamma_curve, "Red", &gamma_r);
    ret |= mPQdb->PQ_GetGammaSpecialTable(gamma_curve, "Green", &gamma_g);
    ret |= mPQdb->PQ_GetGammaSpecialTable(gamma_curve, "Blue", &gamma_b);

    if (ret < 0) {
        SYS_LOGE("%s, PQ_GetGammaSpecialTable failed!", __FUNCTION__);
    } else {
        Cpq_SetGammaTbl_R((unsigned short *) gamma_r.data);
        Cpq_SetGammaTbl_G((unsigned short *) gamma_g.data);
        Cpq_SetGammaTbl_B((unsigned short *) gamma_b.data);
    }

    return ret;
}

int CPQControl::Cpq_SetGammaTbl_R(unsigned short red[256])
{
    struct tcon_gamma_table_s Redtbl;
    int ret = -1, i = 0;

    for (i = 0; i < 256; i++) {
        Redtbl.data[i] = red[i];
    }

    ret = VPPDeviceIOCtl(AMVECM_IOC_GAMMA_TABLE_R, &Redtbl);
    if (ret < 0) {
        SYS_LOGE("%s error(%s)!\n", __FUNCTION__, strerror(errno));
    }
    return ret;
}

int CPQControl::Cpq_SetGammaTbl_G(unsigned short green[256])
{
    struct tcon_gamma_table_s Greentbl;
    int ret = -1, i = 0;

    for (i = 0; i < 256; i++) {
        Greentbl.data[i] = green[i];
    }

    ret = VPPDeviceIOCtl(AMVECM_IOC_GAMMA_TABLE_G, &Greentbl);
    if (ret < 0) {
        SYS_LOGE("%s error(%s)!\n", __FUNCTION__, strerror(errno));
    }

    return ret;
}

int CPQControl::Cpq_SetGammaTbl_B(unsigned short blue[256])
{
    struct tcon_gamma_table_s Bluetbl;
    int ret = -1, i = 0;

    for (i = 0; i < 256; i++) {
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
      int memDev = open(CPQ_MEMC_SYSFS, O_WRONLY);
      if (memDev > 0 && mbCpqCfg_memc_enable) {
          SYS_LOGD("%s, has memc\n", __FUNCTION__);
          return true;
      }
      SYS_LOGD("%s, has NO memc\n", __FUNCTION__);
      return false;
}

int CPQControl::SetMemcMode(int memc_mode, int is_save)
{
    SYS_LOGD("%s, mode = %d\n", __FUNCTION__, memc_mode);
    int ret = -1;
    ret = Cpq_SetMemcMode((vpp_memc_mode_t)memc_mode, mCurentSourceInputInfo);

    if (ret == 0 && is_save == 1) {
        SaveMemcMode((vpp_memc_mode_t)memc_mode);
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
    int memDev = open(CPQ_MEMC_SYSFS, O_WRONLY);
    int enable, level;
    if(memDev > 0 && mbCpqCfg_memc_enable) {
        switch (memc_mode) {
            case VPP_MEMC_MODE_OFF:
                enable = 0;
                level = 0;
            break;
            case VPP_MEMC_MODE_LOW:
                enable = 1;
                level = 3;
            break;
            case VPP_MEMC_MODE_MID:
                enable = 1;
                level = 6;
            break;
            case VPP_MEMC_MODE_HIGH:
                enable = 1;
                level = 10;
            break;
            default:
                enable = 0;
                level = 0;
            break;
        }
        ioctl(memDev, MEMDEV_CONTRL, &enable);
        ioctl(memDev, FRC_IOC_SET_MEMC_LEVEL, &level);
        ret = 0;
    }else {
        SYS_LOGE("Memc moudle disabled!!!\n");
    }
    return ret;
}

int CPQControl::SetMemcDeBlurLevel(int level, int is_save)
{
    SYS_LOGD("%s, source: %d, level = %d\n", __FUNCTION__, mSourceInputForSaveParam, level);
    int ret = -1;
    if(mbCpqCfg_memc_enable) {
        ret = Cpq_SetMemcDeBlurLevel(level, mCurentSourceInputInfo);
        if ((ret ==0) && (is_save == 1)) {
            ret = SaveMemcDeBlurLevel(level);
        }
    } else {
        SYS_LOGD("Memc moudle disabled!!! DeBlurLevel invalid\n");
        ret = 0;
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::GetMemcDeBlurLevel()
{
    int level = 0;
    if (mSSMAction->SSMReadMemcDeblurLevel(mSourceInputForSaveParam, &level) < 0) {
        SYS_LOGE("%s, SSMReadMemcDeblurLevel ERROR!!!\n", __FUNCTION__);
        return -1;
    }

    SYS_LOGD("%s, source: %d, level = %d\n", __FUNCTION__, mSourceInputForSaveParam, level);
    return level;
}

int CPQControl::SaveMemcDeBlurLevel(int level)
{
    SYS_LOGD("%s, source: %d, level = %d\n", __FUNCTION__, mSourceInputForSaveParam, level);
    int ret = mSSMAction->SSMSaveMemcDeblurLevel(mSourceInputForSaveParam, level);
    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::Cpq_SetMemcDeBlurLevel(int level, source_input_param_t source_input_param)
{
    int ret = 0;
    //need xml data
    return ret;
}

int CPQControl::SetMemcDeJudderLevel(int level, int is_save)
{
    SYS_LOGD("%s, source: %d, level = %d\n", __FUNCTION__, mSourceInputForSaveParam, level);
    int ret = -1;
    if(mbCpqCfg_memc_enable) {
        ret = Cpq_SetMemcDeJudderLevel(level, mCurentSourceInputInfo);
        if ((ret ==0) && (is_save == 1)) {
            ret = SaveMemcDeJudderLevel(level);
        }
    } else {
        SYS_LOGD("Memc moudle disabled!!! DeJudderLevel invalid\n");
        ret = 0;
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::GetMemcDeJudderLevel()
{
    int level = 0;
    if (mSSMAction->SSMReadMemcDeJudderLevel(mSourceInputForSaveParam, &level) < 0) {
        SYS_LOGE("%s, SSMReadMemcDeJudderLevel ERROR!!!\n", __FUNCTION__);
        return -1;
    }

    SYS_LOGD("%s, source: %d, level = %d\n", __FUNCTION__, mSourceInputForSaveParam, level);
    return level;
}

int CPQControl::SaveMemcDeJudderLevel(int level)
{
    SYS_LOGD("%s, source: %d, level = %d\n", __FUNCTION__, mSourceInputForSaveParam, level);
    int ret = mSSMAction->SSMSaveMemcDeJudderLevel(mSourceInputForSaveParam, level);
    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::Cpq_SetMemcDeJudderLevel(int level, source_input_param_t source_input_param)
{
    int ret = 0;
    //need xml data
    return ret;
}

//Displaymode
int CPQControl::SetDisplayMode(vpp_display_mode_t display_mode, int is_save)
{
    SYS_LOGD("%s, source: %d, value = %d\n", __FUNCTION__, mSourceInputForSaveParam, display_mode);
    int ret = -1;

    //dtvkit process afd function,driver need output full
    if (mbDtvKitEnable && (display_mode == VPP_DISPLAY_MODE_NORMAL)) {
        ret = Cpq_SetDisplayModeAllTiming(mCurentSourceInputInfo.source_input, display_mode);
        ret = Cpq_SetDisplayModeScreenMode(mCurentSourceInputInfo.source_input, display_mode);
    } else if ((mCurentSourceInputInfo.source_input == SOURCE_DTV)
        || (mCurentSourceInputInfo.source_input == SOURCE_TV)
        || (mCurentSourceInputInfo.source_input == SOURCE_AV1)
        || (mCurentSourceInputInfo.source_input == SOURCE_AV2)) {
        ret = Cpq_SetDisplayModeAllTiming(mCurentSourceInputInfo.source_input, display_mode);
    } else {
        ret = Cpq_SetDisplayModeAllTiming(mCurentSourceInputInfo.source_input, display_mode);
        ret = Cpq_SetDisplayModeOneTiming(mCurentSourceInputInfo.source_input, display_mode);
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
        if (mbCpqCfg_seperate_db_enable) {
            ret = mpOverScandb->PQ_GetOverscanParams(mCurentSourceInputInfo, display_mode, &cutwin);
        } else {
            ret = mPQdb->PQ_GetOverscanParams(mCurentSourceInputInfo, display_mode, &cutwin);
        }
    } else {
        SYS_LOGD("%s: Overscan moudle disabled\n", __FUNCTION__);
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
            if (mCurentAfdInfo == TVIN_ASPECT_4x3_FULL) {
                ScreenModeValue = SCREEN_MODE_4_3;
            } else if (mCurentAfdInfo == TVIN_ASPECT_14x9_FULL) {
                ScreenModeValue = SCREEN_MODE_NORMAL;
            } else if (mCurentAfdInfo == TVIN_ASPECT_16x9_FULL) {
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
        if (mbCpqCfg_seperate_db_enable) {
            ret = mpOverScandb->PQ_GetOverscanParams(mCurentSourceInputInfo, display_mode, &cutwin);
        } else {
            ret = mPQdb->PQ_GetOverscanParams(mCurentSourceInputInfo, display_mode, &cutwin);
        }
    } else {
        SYS_LOGD("%s: Overscan moudle disabled!\n", __FUNCTION__);
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
                if (mCurentAfdInfo == TVIN_ASPECT_4x3_FULL) {
                    ScreenModeValue = SCREEN_MODE_4_3;
                } else if (mCurentAfdInfo == TVIN_ASPECT_14x9_FULL) {
                    ScreenModeValue = SCREEN_MODE_NORMAL;
                } else if (mCurentAfdInfo == TVIN_ASPECT_16x9_FULL) {
                    ScreenModeValue = SCREEN_MODE_16_9;
                } else {
                    SYS_LOGD("%s: invalid AFD status.\n", __FUNCTION__);
                }
            }
        }

        SYS_LOGD("%s: screenmode:%d hs:%d he:%d vs:%d ve:%d\n", __FUNCTION__, ScreenModeValue, cutwin.hs, cutwin.he, cutwin.vs, cutwin.ve);
        Cpq_SetVideoCrop(cutwin.vs, cutwin.hs, cutwin.ve, cutwin.he);
        Cpq_SetVideoScreenMode(ScreenModeValue);
    } else {
        SYS_LOGD("PQ_GetOverscanParams failed!\n");
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
    sig_fmt[1] = TVIN_SIG_FMT_HDMI_1280X720P_60HZ;
    sig_fmt[2] = TVIN_SIG_FMT_HDMI_1920X1080P_60HZ;
    sig_fmt[3] = TVIN_SIG_FMT_HDMI_3840_2160_00HZ;
    sig_fmt[4] = TVIN_SIG_FMT_CVBS_NTSC_M;
    sig_fmt[5] = TVIN_SIG_FMT_CVBS_NTSC_443;
    sig_fmt[6] = TVIN_SIG_FMT_CVBS_PAL_I;
    sig_fmt[7] = TVIN_SIG_FMT_CVBS_PAL_M;
    sig_fmt[8] = TVIN_SIG_FMT_CVBS_PAL_60;
    sig_fmt[9] = TVIN_SIG_FMT_CVBS_PAL_CN;
    sig_fmt[10] = TVIN_SIG_FMT_CVBS_SECAM;
    sig_fmt[11] = TVIN_SIG_FMT_CVBS_NTSC_50;
    flag[0] = SIG_TIMING_TYPE_SD;
    flag[1] = SIG_TIMING_TYPE_HD;
    flag[2] = SIG_TIMING_TYPE_FHD;
    flag[3] = SIG_TIMING_TYPE_UHD;
    flag[4] = SIG_TIMING_TYPE_NTSC_M;
    flag[5] = SIG_TIMING_TYPE_NTSC_443;
    flag[6] = SIG_TIMING_TYPE_PAL_I;
    flag[7] = SIG_TIMING_TYPE_PAL_M;
    flag[8] = SIG_TIMING_TYPE_PAL_60;
    flag[9] = SIG_TIMING_TYPE_PAL_CN;
    flag[10] = SIG_TIMING_TYPE_SECAM;
    flag[11] = SIG_TIMING_TYPE_NTSC_50;

    source_input_param_t source_input_param;
    source_input_param.source_input = source_input;
    source_input_param.trans_fmt = mCurentSourceInputInfo.trans_fmt;
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
            if (mbCpqCfg_display_overscan_enable) {
                if (mbCpqCfg_seperate_db_enable) {
                    ret = mpOverScandb->PQ_GetOverscanParams(source_input_param, display_mode, cutwin+i);
                } else {
                    ret = mPQdb->PQ_GetOverscanParams(source_input_param, display_mode, cutwin+i);
                }
            } else {
                SYS_LOGD("%s: Overscan moudle disabled!\n", __FUNCTION__);
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
                SYS_LOGD("signal_fmt:0x%x, AFDFlag:%d,screen mode:%d hs:%d he:%d vs:%d ve:%d!\n", sig_fmt[i], AFDFlag, ScreenModeValue, cutwin[i].he, cutwin[i].hs, cutwin[i].ve, cutwin[i].vs);
                ve_pq_table[i].value1 = ((cutwin[i].he & 0xffff)<<16) | (cutwin[i].hs & 0xffff);
                ve_pq_table[i].value2 = ((cutwin[i].ve & 0xffff)<<16) | (cutwin[i].vs & 0xffff);
            } else {
                SYS_LOGD("PQ_GetOverscanParams failed!\n");
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
                if (mbCpqCfg_seperate_db_enable) {
                    ret = mpOverScandb->PQ_GetOverscanParams(source_input_param, display_mode, cutwin+i);
                } else {
                    ret = mPQdb->PQ_GetOverscanParams(source_input_param, display_mode, cutwin+i);
                }
            } else {
                SYS_LOGD("%s: Overscan moudle disabled!\n", __FUNCTION__);
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
                SYS_LOGD("PQ_GetOverscanParams failed!\n");
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
int CPQControl::SetBacklight(int value, int is_save)
{
    int ret = -1;
    SYS_LOGD("%s: value = %d\n", __FUNCTION__, value);
    if (value < 0 || value > 100) {
        value = DEFAULT_BACKLIGHT_BRIGHTNESS;
    }

    if (isFileExist(LDIM_PATH)) {//local diming
        int temp = (value * 255 / 100);
        Cpq_SetBackLight(temp);
    }

    if (is_save == 1) {
        ret = SaveBacklight(value);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
        return -1;
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
        return 0;
    }

}

int CPQControl::GetBacklight(void)
{
    int data = 0;
    mSSMAction->SSMReadBackLightVal(&data);

    if (data < 0 || data > 100) {
        data = DEFAULT_BACKLIGHT_BRIGHTNESS;
    }

    return data;
}

int CPQControl::SaveBacklight(int value)
{
    int ret = -1;
    SYS_LOGD("%s: value = %d\n", __FUNCTION__, value);

    ret = mSSMAction->SSMSaveBackLightVal(value);

    return ret;
}

int CPQControl::Cpq_SetBackLight(int value)
{
    SYS_LOGD("%s: %d\n",__FUNCTION__, value);

    char val[64] = {0};
    sprintf(val, "%d", value);
    return pqWriteSys(BACKLIGHT_AML_BL_BRIGHTNESS, val);
}

void CPQControl::Cpq_GetBacklight(int *value)
{
    int ret = 0;
    char buf[64] = {0};

    ret = pqReadSys(BACKLIGHT_AML_BL_BRIGHTNESS, buf, sizeof(buf));
    if (ret > 0) {
        ret = strtol(buf, NULL, 10);
    } else {
        ret = 0;
    }

    *value = ret;
}

void CPQControl::Set_Backlight(int value)
{
    Cpq_SetBackLight(value);
}

//dynamic backlight
int CPQControl::SetDynamicBacklight(Dynamic_backlight_status_t mode, int is_save)
{
    SYS_LOGD("%s, mode = %d\n",__FUNCTION__, mode);
    int ret = -1;
    if (isFileExist(LDIM_PATH)) {//local diming
        char val[64] = {0};
        sprintf(val, "%d", mode);
        pqWriteSys(AML_LDIM_FUNC_EN, val);
    }

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

void CPQControl::GetDynamicBacklighConfig(int *thtf, int *lut_mode, int *heigh_param, int *low_param)
{
    *thtf = mPQConfigFile->GetInt(CFG_SECTION_BACKLIGHT, CFG_AUTOBACKLIGHT_THTF, 0);
    *lut_mode = mPQConfigFile->GetInt(CFG_SECTION_BACKLIGHT, CFG_AUTOBACKLIGHT_LUTMODE, 1);

    const char *buf = NULL;
    buf = mPQConfigFile->GetString(CFG_SECTION_BACKLIGHT, CFG_AUTOBACKLIGHT_LUTHIGH, NULL);
    pqTransformStringToInt(buf, heigh_param);

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

    Cpq_GetBacklight(&value);
    DynamicBacklightParam->CurBacklightValue = value;
    DynamicBacklightParam->UiBackLightValue = GetBacklight();
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
        curVideoState = 0;//video stoping
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
        SYS_LOGD("%s: local contrast moudle disabled!\n",__FUNCTION__);
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
    int ret = mSSMAction->SSMReadLocalContrastMode(mCurentSourceInputInfo.source_input, &mode);
    if (0 == ret) {
        SYS_LOGD("%s: mode is %d\n", __FUNCTION__, mode);
    } else {
        SYS_LOGE("%s failed!\n", __FUNCTION__);
    }

    return mode;
}

int CPQControl::SaveLocalContrastMode(local_contrast_mode_t mode)
{
    int ret = -1;
    SYS_LOGD("%s: mode = %d\n", __FUNCTION__, mode);

    ret = mSSMAction->SSMSaveLocalContrastMode(mCurentSourceInputInfo.source_input, mode);
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
        ret = mPQdb->PQ_GetLocalContrastNodeParams(mCurentSourceInputInfo, mode, &lc_param);
        if (ret == 0 ) {
            ret = VPPDeviceIOCtl(AMVECM_IOC_S_LC_CURVE, &lc_param);
            if (ret == 0) {
                ret = mPQdb->PQ_GetLocalContrastRegParams(mCurentSourceInputInfo, mode, &regs);
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
        SYS_LOGD("%s: pq.db don't match chip!\n", __FUNCTION__);
        ret = 0;
    }
    return ret;
}

int CPQControl::SetBlackExtensionMode(black_extension_mode_t mode, int is_save)
{
    SYS_LOGD("%s: mode is %d\n", __FUNCTION__, mode);
    int ret = -1;
    if (mbCpqCfg_blackextension_enable) {
        ret = SetBlackExtensionParam(mCurentSourceInputInfo);
        if ((ret == 0) && (is_save == 1)) {
            ret = SaveBlackExtensionMode(mode);
        }
    } else {
        SYS_LOGD("%s: black extension moudle disabled\n", __FUNCTION__);
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
    int mode = BLACK_EXTENSION_MODE_OFF;
    int ret = mSSMAction->SSMReadBlackExtensionMode(mCurentSourceInputInfo.source_input, &mode);
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

    ret = mSSMAction->SSMSaveBlackExtensionMode(mCurentSourceInputInfo.source_input, mode);
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

int CPQControl::SetDeblockMode(di_deblock_mode_t mode, int is_save)
{
    SYS_LOGD("%s: mode is %d\n", __FUNCTION__, mode);
    int ret = -1;
    if (mbCpqCfg_deblock_enable) {
        ret = Cpq_SetDeblockMode(mode, mCurentSourceInputInfo);
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
    int mode = DI_DEBLOCK_MODE_OFF;
    int ret = mSSMAction->SSMReadDeblockMode(mCurentSourceInputInfo.source_input, &mode);
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

    ret = mSSMAction->SSMSaveDeblockMode(mCurentSourceInputInfo.source_input, mode);
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
        ret = Cpq_SetDemoSquitoMode(mode, mCurentSourceInputInfo);
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
    int mode = DI_DEMOSQUITO_MODE_OFF;
    int ret = mSSMAction->SSMReadDemoSquitoMode(mCurentSourceInputInfo.source_input, &mode);
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

    ret = mSSMAction->SSMSaveDemoSquitoMode(mCurentSourceInputInfo.source_input, mode);
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
        ret = Cpq_SetMcDiMode(mode , mCurentSourceInputInfo);
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
    int ret = mSSMAction->SSMReadMcDiMode(mCurentSourceInputInfo.source_input, &mode);
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

    ret = mSSMAction->SSMSaveMcDiMode(mCurentSourceInputInfo.source_input, mode);
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

    if ((ret == 0) && (isSave == 1)) {
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

//PQ Factory
int CPQControl::FactoryResetPQMode(void)
{
    if (mbCpqCfg_seperate_db_enable) {
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
    if (mbCpqCfg_seperate_db_enable) {
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
    if (mbCpqCfg_seperate_db_enable) {
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
    if (mbCpqCfg_seperate_db_enable) {
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
    if (mbCpqCfg_seperate_db_enable) {
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
    if (mbCpqCfg_seperate_db_enable) {
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
    if (mbCpqCfg_seperate_db_enable) {
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
    if (mbCpqCfg_seperate_db_enable) {
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
    if (mbCpqCfg_seperate_db_enable) {
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
    if (mbCpqCfg_seperate_db_enable) {
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
    if (mbCpqCfg_seperate_db_enable) {
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
    if (mbCpqCfg_seperate_db_enable) {
        mpOverScandb->PQ_ResetAllOverscanParams();
    } else {
        mPQdb->PQ_ResetAllOverscanParams();
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

int CPQControl::FactorySetOverscanParam(source_input_param_t source_input_param, tvin_cutwin_t cutwin_t)
{
    int ret = -1;
    if (mbCpqCfg_seperate_db_enable) {
        ret = mpOverScandb->PQ_SetOverscanParams(source_input_param, cutwin_t);
    } else {
        ret = mPQdb->PQ_SetOverscanParams(source_input_param, cutwin_t);
    }
    if (ret != 0) {
        SYS_LOGE("%s failed.\n", __FUNCTION__);
    } else {
        SYS_LOGD("%s success.\n", __FUNCTION__);
    }

    return ret;
}

tvin_cutwin_t CPQControl::FactoryGetOverscanParam(source_input_param_t source_input_param)
{
    int ret = -1;
    tvin_cutwin_t cutwin_t;
    memset(&cutwin_t, 0, sizeof(cutwin_t));

    if (source_input_param.trans_fmt < TVIN_TFMT_2D || source_input_param.trans_fmt > TVIN_TFMT_3D_LDGD) {
        return cutwin_t;
    }
    if (mbCpqCfg_seperate_db_enable) {
        ret = mpOverScandb->PQ_GetOverscanParams(source_input_param, VPP_DISPLAY_MODE_169, &cutwin_t);
    } else {
        ret = mPQdb->PQ_GetOverscanParams(source_input_param, VPP_DISPLAY_MODE_169, &cutwin_t);
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

    memset(gamma_r.data, (unsigned short)gamma_r_value, 256);
    memset(gamma_g.data, (unsigned short)gamma_g_value, 256);
    memset(gamma_b.data, (unsigned short)gamma_b_value, 256);

    ret |= Cpq_SetGammaTbl_R((unsigned short *) gamma_r.data);
    ret |= Cpq_SetGammaTbl_G((unsigned short *) gamma_g.data);
    ret |= Cpq_SetGammaTbl_B((unsigned short *) gamma_b.data);

    return ret;
}

int CPQControl::FcatorySSMRestore(void)
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
        SYS_LOGD("XVYCC Moudle disabled!\n");
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
        } else {
            temp_regmap = DemoColorSplitRegMap;
        }

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
    SYS_LOGD("%s: mode is %d\n", __FUNCTION__, basemode);
    int ret = Cpq_SetColorBaseMode(basemode, mCurentSourceInputInfo);
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
        SYS_LOGD("CM moudle disabled!\n");
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
        if (mPQdb->PQ_GetAADParams(mCurentSourceInputInfo, &newaad) == 0) {
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
        SYS_LOGD("AAD moudle disabled\n");
        ret = 0;
    }

    if (ret < 0) {
        SYS_LOGE("%s failed\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s succes\n",__FUNCTION__);
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
        if (mPQdb->PQ_GetCABCParams(mCurentSourceInputInfo, &newcabc) == 0) {
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
        SYS_LOGD("CABC moudle disabled\n");
        ret = 0;
    }

    if (ret < 0) {
        SYS_LOGE("%s failed\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s succes\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::SetDnlpMode(int level)
{
    int ret = -1;
    ve_dnlp_curve_param_t newdnlp;
    if (mbCpqCfg_dnlp_enable) {
        if (mPQdb->PQ_GetDNLPParams(mCurentSourceInputInfo, (Dynamic_contrst_status_t)level, &newdnlp) == 0) {
            ret = Cpq_SetVENewDNLP(&newdnlp);
            if (ret == 0) {
                mSSMAction->SSMSaveDnlpMode(mCurentSourceInputInfo.source_input, level);
            }
        } else {
            SYS_LOGE("mPQdb->PQ_GetDNLPParams failed!\n");
        }
    } else {
        SYS_LOGD("DNLP moudle disabled!\n");
        ret = 0;
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::GetDnlpMode()
{
    int ret = -1, level = 0;
    ret = mSSMAction->SSMReadDnlpMode(mCurentSourceInputInfo.source_input, &level);
    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    SYS_LOGD("%s, source_input = %d, mode is %d\n",__FUNCTION__, mCurentSourceInputInfo.source_input, level);
    return level;
}

int CPQControl::Cpq_SetVENewDNLP(const ve_dnlp_curve_param_t *pDNLP)
{
    int ret = VPPDeviceIOCtl(AMVECM_IOC_VE_NEW_DNLP, pDNLP);
    if (ret < 0) {
        SYS_LOGE("%s error(%s)!\n", __FUNCTION__, strerror(errno));
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

    cur_final_gain = mPQdb->PQ_GetDNLPGains(source_input_param, (Dynamic_contrst_status_t)level);
    if (cur_final_gain == final_gain) {
        SYS_LOGD("FactorySetDNLPCurveParams, same value, no need to update!");
        return ret;
    } else {
        SYS_LOGD("%s final_gain = %d \n", __FUNCTION__, final_gain);
        sprintf(tmp_buf, "%s %s %d", "w", "final_gain", final_gain);
        pqWriteSys(AMVECM_PQ_DNLP_DEBUG, tmp_buf);
        ret |= mPQdb->PQ_SetDNLPGains(source_input_param, (Dynamic_contrst_status_t)level, final_gain);

    }
    return ret;
}

int CPQControl::FactoryGetDNLPCurveParams(source_input_param_t source_input_param, int level)
{
    return mPQdb->PQ_GetDNLPGains(source_input_param, (Dynamic_contrst_status_t)level);
}

int CPQControl::FactorySetBlackExtRegParams(source_input_param_t source_input_param, int val)
{
    int rt = -1;
    unsigned int reg_val = 0,tmp = 0;
    unsigned int start_val = 0;
    char tmp_buf[128];

    SYS_LOGD("%s, input BE value: %d!\n", __FUNCTION__, val);

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
    SYS_LOGD("%s, get value is %d, try to set value: %d\n", __FUNCTION__, tmp_val, val);

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
    SYS_LOGD("%s, get value is %d, try to set value: %d\n", __FUNCTION__, tmp_val, val);

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
        default: {
            rval = 0;
            break;
        }
    }

    SYS_LOGD("%s, type [%d], get reg_val: %u, ret val: %u\n", __FUNCTION__, param_type, reg_val, rval);
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
            SYS_LOGD("%s, type [%d], get val: %u\n", __FUNCTION__, param_type, reg_val);
            rval = (reg_val >> 28) & 0xf;
            SYS_LOGD("%s, type [%d], get val: %u\n", __FUNCTION__, param_type, rval);
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

    SYS_LOGD("%s, type [%d], get reg_val: %u, ret val: %u\n", __FUNCTION__, param_type, reg_val, rval);
    return rval;
}

int CPQControl::SetEyeProtectionMode(tv_source_input_t source_input __unused, int enable, int is_save __unused)
{
    SYS_LOGD("%s: mode:%d!\n", __FUNCTION__, enable);
    int ret = -1;
    vpp_color_temperature_mode_t TempMode = (vpp_color_temperature_mode_t)GetColorTemperature();
    tcon_rgb_ogo_t param;
    memset(&param, 0, sizeof(tcon_rgb_ogo_t));
    if (TempMode == VPP_COLOR_TEMPERATURE_MODE_USER) {
        ret = Cpq_GetColorTemperatureUser(mCurentSourceInputInfo.source_input, &param);
    } else {
        ret = GetColorTemperatureParams(TempMode, &param);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        if (enable) {
            param.b_gain /= 2;
        }
        ret = Cpq_SetRGBOGO(&param);
        mSSMAction->SSMSaveEyeProtectionMode(enable);
        SYS_LOGD("%s success!\n",__FUNCTION__);
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
        SYS_LOGD("%s: mode is %d!\n",__FUNCTION__, mode);
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
        mbCpqCfg_seperate_db_enable = true;
    } else {
        mbCpqCfg_seperate_db_enable = false;
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
        pqControlVal.lc_en = 0;
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
    int ret = mPQdb->PQ_GetCVD2Params ( mCurentSourceInputInfo, &regs);
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

hdr_type_t CPQControl::Cpq_GetSourceHDRType(tv_source_input_t source_input)
{
    hdr_type_t newHdrType = HDR_TYPE_NONE;
    if ((source_input == SOURCE_MPEG)
        ||(source_input == SOURCE_DTV)) {
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
                    SYS_LOGD("%s: invalid hdr type:%s\n", __FUNCTION__, buf);
                    newHdrType = HDR_TYPE_SDR;
                }
            }
        }
    } else if ((source_input == SOURCE_HDMI1)
             || (source_input == SOURCE_HDMI2)
             || (source_input == SOURCE_HDMI3)
             || (source_input == SOURCE_HDMI4)) {
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
    } else {
        newHdrType = HDR_TYPE_NONE;
    }

    SYS_LOGD("%s: newHdrType:%d\n", __FUNCTION__, newHdrType);

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
    newHdrType = Cpq_GetSourceHDRType(source_input_param.source_input);

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

    CheckOutPutMode(source_input_param.source_input);

    if ((mCurentSourceInputInfo.source_input != source_input_param.source_input) ||
         (mCurentSourceInputInfo.sig_fmt != source_input_param.sig_fmt) ||
         (mCurentSourceInputInfo.trans_fmt != source_input_param.trans_fmt) ||
         (mCurrentHdrStatus != mPQdb->mHdrStatus) ||
         (mCurentOutputType != mPQdb->mOutPutType)) {
        mCurentSourceInputInfo.source_input = source_input_param.source_input;
        mCurentSourceInputInfo.sig_fmt = source_input_param.sig_fmt;
        mCurentSourceInputInfo.trans_fmt = source_input_param.trans_fmt;
        mCurrentHdrStatus = mPQdb->mHdrStatus;
        mCurentOutputType = mPQdb->mOutPutType;

        if (mbCpqCfg_pq_param_check_source_enable) {
            mSourceInputForSaveParam = mCurentSourceInputInfo.source_input;
        } else {
            mSourceInputForSaveParam = SOURCE_MPEG;
        }

        if (mCurentSourceInputInfo.sig_fmt != TVIN_SIG_FMT_NULL) {
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
    return mCurentSourceInputInfo;
}

int CPQControl::GetRGBPattern() {
    char value[32] = {0};
    pqReadSys(VIDEO_RGB_SCREEN, value, sizeof(value));
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
    if (step > 4) {
        step = 4;
    }

    SYS_LOGD("%s: %d\n", __FUNCTION__, step);

    char buf[32] = {0};
    sprintf(buf, "%d", step);
    int ret = pqWriteSys(LCD_SS, buf);
    return ret;
}

int CPQControl::FactorySetLVDSSSC(int step) {
    int data[2] = {0, 0};
    int value = 0, panel_idx = 0, tmp = 0;
    const char *PanelIdx;
    if (step > 4)
        step = 4;
    PanelIdx = "0";//config_get_str ( CFG_SECTION_TV, "get.panel.index", "0" ); can't parse ini file in systemcontrol
    panel_idx = strtoul(PanelIdx, NULL, 10);
    SYS_LOGD ("%s, panel_idx = %x", __FUNCTION__, panel_idx);
    mSSMAction->SSMReadLVDSSSC(data);

    //every 2 bits represent one panel, use 2 byte store 8 panels
    value = (data[1] << 8) | data[0];
    step = step & 0x03;
    panel_idx = panel_idx * 2;
    tmp = 3 << panel_idx;
    value = (value & (~tmp)) | (step << panel_idx);
    data[0] = value & 0xFF;
    data[1] = (value >> 8) & 0xFF;
    SYS_LOGD ("%s, tmp = %x, save value = %x", __FUNCTION__, tmp, value);

    SetLVDSSSC(step);
    return mSSMAction->SSMSaveLVDSSSC(data);
}

int CPQControl::FactoryGetLVDSSSC() {
    int data[2] = {0, 0};
    int value = 0, panel_idx = 0;
    const char *PanelIdx = "0";//config_get_str ( CFG_SECTION_TV, "get.panel.index", "0" );can't parse ini file in systemcontrol

    panel_idx = strtoul(PanelIdx, NULL, 10);
    mSSMAction->SSMReadLVDSSSC(data);
    value = (data[1] << 8) | data[0];
    value = (value >> (2 * panel_idx)) & 0x03;
    SYS_LOGD ("%s, panel_idx = %x, value= %d", __FUNCTION__, panel_idx, value);
    return value;
}

int CPQControl::SetGrayPattern(int value) {
    if (value < 0) {
        value = 0;
    } else if (value > 255) {
        value = 255;
    }
    value = value << 16 | 0x8080;

    SYS_LOGD("%s: %d\n", __FUNCTION__, value);

    char val[64] = {0};
    sprintf(val, "%d", value);
    return pqWriteSys(VIDEO_TEST_SCREEN, val);
}

int CPQControl::GetGrayPattern() {
    int value = 0;
    char temp[8];
    memset(temp, 0, sizeof(temp));
    int ret = pqReadSys(VIDEO_TEST_SCREEN, temp, sizeof(temp));
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
    if ((mCurentSourceInputInfo.source_input == SOURCE_MPEG) ||
       ((mCurentSourceInputInfo.source_input >= SOURCE_HDMI1) && mCurentSourceInputInfo.source_input <= SOURCE_HDMI4)) {
        ret = VPPDeviceIOCtl(AMVECM_IOC_S_CSCTYPE, &mode);
        if (ret < 0) {
            SYS_LOGE("%s error: %s!\n", __FUNCTION__, strerror(errno));
        }
    } else {
        SYS_LOGE("%s: Curent source no hdr status!\n", __FUNCTION__);
    }

    return ret;
}

int CPQControl::GetHDRMode()
{
    ve_csc_type_t mode = VPP_MATRIX_NULL;
    if ((mCurentSourceInputInfo.source_input == SOURCE_MPEG) ||
       ((mCurentSourceInputInfo.source_input >= SOURCE_HDMI1) && mCurentSourceInputInfo.source_input <= SOURCE_HDMI4)) {
        int ret = VPPDeviceIOCtl(AMVECM_IOC_G_CSCTYPE, &mode);
        if (ret < 0) {
            SYS_LOGE("%s error: %s!\n", __FUNCTION__, strerror(errno));
            mode = VPP_MATRIX_NULL;
        } else {
            SYS_LOGD("%s: mode is %d\n", __FUNCTION__, mode);
        }
    } else {
        SYS_LOGD("%s: Curent source no hdr status!\n", __FUNCTION__);
    }

    return mode;
}

int CPQControl::GetSourceHDRType()
{
    SYS_LOGD("%s: type is %d\n", __FUNCTION__, mCurrentHdrType);
    return mCurrentHdrType;
}

void CPQControl::setHdrInfoListener(const sp<PqNotify>& listener) {
    mNotifyListener = listener;
}

void CPQControl::GetChipVersionInfo(char* chip_version) {
    database_attribute_t dbAttribute;
    mPQdb->PQ_GetDataBaseAttribute(&dbAttribute);
    if ((dbAttribute.ChipVersion.string() == NULL) || (dbAttribute.ChipVersion.length() == 0)) {
        SYS_LOGD("%s: ChipVersion is null\n", __FUNCTION__);
        std::strcpy(chip_version, " ");
    } else {
        std::string TempString = std::string(dbAttribute.ChipVersion.string());
        char* tempstr = new char[TempString.length() + 1];
        std::strcpy(tempstr, TempString.c_str());
        chip_version = strtok(tempstr, "_");

        SYS_LOGD("%s: versionStr is %s\n", __FUNCTION__, chip_version);
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
        strcpy(pqdatabaseinfo_t.ToolVersion, tmpToolVersion.string());
        strcpy(pqdatabaseinfo_t.ProjectVersion, tmpProjectVersion.string());
        strcpy(pqdatabaseinfo_t.GenerateTime, tmpGenerateTime.string());
    }

    return pqdatabaseinfo_t;
}

int CPQControl::SetCurrentHdrInfo (int hdrInfo)
{
    int ret = 0;
    SYS_LOGD("%s: mHdmiHdrInfo:%d new hdrInfo:%d\n", __FUNCTION__, mHdmiHdrInfo, hdrInfo);

    if (mHdmiHdrInfo != (unsigned int)hdrInfo) {
        mHdmiHdrInfo = (unsigned int)hdrInfo;
        //get hdr type
        hdr_type_t newHdrType = HDR_TYPE_NONE;
        newHdrType            = Cpq_GetSourceHDRType(mCurentSourceInputInfo.source_input);

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
        SYS_LOGD("%s: same HDR info\n", __FUNCTION__);
    }

    return ret;
}

int CPQControl::SetCurrentAspectRatioInfo(tvin_aspect_ratio_e aspectRatioInfo)
{
    int ret = 0;
    if (mCurentAfdInfo != aspectRatioInfo) {
        mCurentAfdInfo = aspectRatioInfo;
        vpp_display_mode_t display_mode = (vpp_display_mode_t)GetDisplayMode();
        ret = SetDisplayMode(display_mode, 1);
    } else {
        SYS_LOGD("%s: same AFD info!\n", __FUNCTION__);
    }

    return ret;
}

int CPQControl::SetDtvKitSourceEnable(bool isEnable)
{
    SYS_LOGD("%s: isEnable:%d\n", __FUNCTION__, isEnable);

    mbDtvKitEnable = isEnable;
    return 0;
}

//AI
void CPQControl::AipqInit()
{
    if (GetAipqEnable() == 1) {
        enableAipq(true);
    }
}

int CPQControl::SetAipqEnable(bool isEnable)
{
    enableAipq(isEnable);
    mSSMAction->SSMSaveAipqEnableVal(isEnable ? 1 : 0);
    return 0;
}

int CPQControl::GetAipqEnable()
{
    int data = 0;
    mSSMAction->SSMReadAipqEnableVal(&data);

    if (data < 0 || data > 1) {
        data = 0;
    }
    return data;
}

void CPQControl::enableAipq(bool isEnable)
{
    pqWriteSys(DECODER_COMMON_PARAMETERS_DEBUG_VDETECT,  isEnable ? "1" : "0");
    pqWriteSys(VDETECT_AIPQ_ENABLE,  isEnable ? "1" : "0");
}

int CPQControl::AiParamLoad(void)
{
    int ret = -1;

    if (mbCpqCfg_ai_enable) {
        ai_pic_table_t aiRegs;
        memset(&aiRegs, 0, sizeof(ai_pic_table_t));
        ret = mPQdb->PQ_GetAIParams(mCurentSourceInputInfo, &aiRegs);
        if (ret >= 0) {
            SYS_LOGD("%s: width: %d, height: %d, array: %s.\n", __FUNCTION__, aiRegs.width, aiRegs.height, aiRegs.table_ptr);
            ret = VPPDeviceIOCtl(AMVECM_IOC_S_AIPQ_TABLE, &aiRegs);
            if (ret < 0) {
                SYS_LOGE("%s: iocontrol failed\n", __FUNCTION__);
            }
        } else {
            SYS_LOGE("%s: get AI pq params failed\n", __FUNCTION__);
        }
    } else {
        SYS_LOGE("ai is disable\n", __FUNCTION__);
        ret = 0;
    }

    if (ret < 0) {
        SYS_LOGE("%s failed\n", __FUNCTION__);
    } else {
        SYS_LOGD("%s success\n", __FUNCTION__);
    }

    return ret;
}

//color space
int CPQControl::SetColorGamutMode(vpp_colorgamut_mode_t value, int is_save)
{
    int ret =0;
    SYS_LOGD("%s, source:%d, value:%d\n",
        __FUNCTION__, mSourceInputForSaveParam, value);
    ret = Cpq_SetColorGamutMode(value, mCurentSourceInputInfo);

    if ((ret == 0) && (is_save == 1)) {
        ret = SaveColorGamutMode(value);
    }

    if (ret < 0) {
        SYS_LOGD("%s failed\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success\n",__FUNCTION__);
    }
    return 0;
}

int CPQControl::GetColorGamutMode(void)
{
    int data = VPP_COLORGAMUT_MODE_AUTO;
    mSSMAction->SSMReadColorGamutMode(mSourceInputForSaveParam, &data);
    SYS_LOGD("%s:source:%d, offset:%d, value:%d\n",
        __FUNCTION__, mSourceInputForSaveParam, mSourceInputForSaveParam, data);

    if (data < VPP_COLORGAMUT_MODE_SRC || data > VPP_COLORGAMUT_MODE_NATIVE) {
        data = VPP_COLORGAMUT_MODE_AUTO;
    }

    return data;
}

int CPQControl::SaveColorGamutMode(vpp_colorgamut_mode_t value)
{
    int ret    = 1;
    SYS_LOGD("%s:source:%d, offset:%d, value:%d\n",
        __FUNCTION__, mSourceInputForSaveParam, mSourceInputForSaveParam, value);

    ret = mSSMAction->SSMSaveColorGamutMode(mSourceInputForSaveParam, value);
    if (ret < 0) {
        SYS_LOGD("%s failed\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success\n",__FUNCTION__);
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
    SYS_LOGD("%s, source: %d, value = %d\n", __FUNCTION__, mSourceInputForSaveParam, smoothplus_mode);
    int ret = Cpq_SetSmoothPlusMode((vpp_smooth_plus_mode_t)smoothplus_mode, mCurentSourceInputInfo);
    if ((ret ==0) && (is_save == 1)) {
        ret = SaveSmoothPlusMode((vpp_smooth_plus_mode_t)smoothplus_mode);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::GetSmoothPlusMode(void)
{
    int mode = VPP_SMOOTH_PLUS_MODE_MID;
    mSSMAction->SSMReadSmoothPlus(mSourceInputForSaveParam, &mode);
    if (mode < VPP_SMOOTH_PLUS_MODE_OFF || mode > VPP_SMOOTH_PLUS_MODE_AUTO) {
        mode = VPP_SMOOTH_PLUS_MODE_MID;
    }

    SYS_LOGD("%s, source: %d, value = %d\n", __FUNCTION__, mSourceInputForSaveParam, mode);
    return mode;
}

int CPQControl::SaveSmoothPlusMode(int smoothplus_mode)
{
    SYS_LOGD("%s, source: %d, value = %d\n", __FUNCTION__, mSourceInputForSaveParam, smoothplus_mode);
    int ret = mSSMAction->SSMSaveSmoothPlus(mSourceInputForSaveParam, smoothplus_mode);
    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
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
        SYS_LOGD("Smooth Plus disabled\n");
        ret = 0;
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::SetHDRTMData(int *reGain)
{
    int ret = -1;
    if (reGain == NULL) {
        SYS_LOGD("%s: reGain is NULL.\n", __FUNCTION__);
    } else {
        int i = 0;
        vpp_hdr_tone_mapping_t hdrToneMapping;
        memset(&hdrToneMapping, 0, sizeof(vpp_hdr_tone_mapping_t));
        hdrToneMapping.lut_type = LUT_TYPE_HLG;
        hdrToneMapping.lutlength = 149;
        hdrToneMapping.tm_lut = reGain;

        SYS_LOGD("hdrToneMapping.lut_type = %d\n", hdrToneMapping.lut_type);
        SYS_LOGD("hdrToneMapping.lutlength = %d\n", hdrToneMapping.lutlength);
        //SYS_LOGD("hdrToneMapping.tm_lut = %s\n", hdrToneMapping.tm_lut);

        int ret = VPPDeviceIOCtl(AMVECM_IOC_S_HDR_TM, &hdrToneMapping);
    }

    if (ret < 0) {
        SYS_LOGE("%s error(%s)!\n", __FUNCTION__, strerror(errno));
    } else {
        SYS_LOGD("%s success!\n", __FUNCTION__);
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

    SYS_LOGD("%s, source: %d, mode = %d\n", __FUNCTION__, mSourceInputForSaveParam, mode);

    if (mbCpqCfg_hdrtmo_enable) {
        if (mPQdb->PQ_GetHDRTMOParams(mCurentSourceInputInfo, mode, &hdrtmo_param) == 0) {
            ret = Cpq_SetHDRTMOParams(&hdrtmo_param);
            if ((ret ==0) && (is_save == 1)) {
                ret = SaveHDRTMOMode(mode);
            }
        } else {
            SYS_LOGE("mPQdb->PQ_GetHDRTMOParams failed!\n");
        }
    } else {
        SYS_LOGD("hdr tmo disabled\n");
        ret = 0;
    }

    if (ret < 0) {
        SYS_LOGE("%s failed\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success\n",__FUNCTION__);
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

    SYS_LOGD("%s, source: %d, value = %d\n", __FUNCTION__, mSourceInputForSaveParam, data);

    return data;
}

int CPQControl::SaveHDRTMOMode(hdr_tmo_t mode)
{
    SYS_LOGD("%s, source: %d, value = %d\n", __FUNCTION__, mSourceInputForSaveParam, mode);

    int ret = mSSMAction->SSMSaveHdrTmoVal(mSourceInputForSaveParam, mode);
    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

void CPQControl::resetAllUserSettingParam()
{
    int ret = 0, i = 0, config_val = 0;
    vpp_pq_para_t pq_para;
    const char *buf = NULL;
    for (i=SOURCE_TV;i<SOURCE_MAX;i++) {
        if (mbCpqCfg_seperate_db_enable) {
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
        mSSMAction->SSMSaveBackLightVal(pq_para.backlight);
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

        config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_MEMCMODE_DEF, VPP_MEMC_MODE_OFF);
        mSSMAction->SSMSaveMemcMode(i, config_val);

        config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_MEMCDEBLURLEVEL_DEF, 10);
        mSSMAction->SSMSaveMemcDeblurLevel(i, config_val);

        config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_MEMCDEJUDDERLEVEL_DEF, 10);
        mSSMAction->SSMSaveMemcDeJudderLevel(i, config_val);

        config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_DEBLOCKMODE_DEF, DI_DEMOSQUITO_MODE_OFF);
        mSSMAction->SSMSaveDeblockMode(i, config_val);

        config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_DEMOSQUITOMODE_DEF, DI_DEMOSQUITO_MODE_OFF);
        mSSMAction->SSMSaveDemoSquitoMode(i, config_val);

        config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_MCDI_DEF, VPP_MCDI_MODE_STANDARD);
        mSSMAction->SSMSaveMcDiMode(i, config_val);
    }

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
    int tmp[2] = {0, 0};
    pqTransformStringToInt(buf, tmp);
    mSSMAction->SSMSaveLVDSSSC(tmp);

    config_val = mPQConfigFile->GetInt(CFG_SECTION_PQ, CFG_EYEPROJECTMODE_DEF, 0);
    mSSMAction->SSMSaveEyeProtectionMode(config_val);

    config_val = mPQConfigFile->GetInt(CFG_SECTION_HDMI, CFG_EDID_VERSION_DEF, 0);
    mSSMAction->SSMEdidRestoreDefault(config_val);

    config_val = mPQConfigFile->GetInt(CFG_SECTION_HDMI, CFG_HDCP_SWITCHER_DEF, 0);
    mSSMAction->SSMHdcpSwitcherRestoreDefault(0);

    buf = mPQConfigFile->GetString(CFG_SECTION_HDMI, CFG_COLOR_RANGE_MODE_DEF, NULL);
    if (strcmp(buf, "full") == 0) {
        mSSMAction->SSMSColorRangeModeRestoreDefault(1);
    } else if (strcmp(buf, "limit") == 0) {
        mSSMAction->SSMSColorRangeModeRestoreDefault(2);
    } else {
        mSSMAction->SSMSColorRangeModeRestoreDefault(0);
    }
    //static frame
    Cpq_SSMWriteNTypes(SSM_RW_BLACKOUT_ENABLE_START, 1, 0, 0);
    //screen color for signal
    Cpq_SSMWriteNTypes(CUSTOMER_DATA_POS_SCREEN_COLOR_START, 1, 0, 0);

    mSSMAction->SSMSaveAipqEnableVal(0);
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
            SYS_LOGD("Read VIDEO_FRAME_HEIGHT failed!\n");
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
        SYS_LOGD("%s: don't support tvservice!\n", __FUNCTION__);
#endif
    }

    if (inputFrameHeight <= 0) {
        SYS_LOGD("%s: inputFrameHeight is invalid, return default value!\n", __FUNCTION__);
        inputFrameHeight = 1080;
    }

    SYS_LOGD("%s: inputFrameHeight is %d!\n", __FUNCTION__, inputFrameHeight);
    return inputFrameHeight;
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
                //SYS_LOGD( "%s: size is %d, str is : %s!\n", __FUNCTION__, outputModeStrSize, tempBuf);
                if (strstr(tempBuf, "smpte")) {
                    outputFrameHeight = 4096;
                } else {
                    memset(tempBuf,0, sizeof(tempBuf));
                    strncpy(tempBuf, outputModeBuf, (outputModeStrSize - 5));//delete "pxxhz"
                    outputFrameHeight = atoi(tempBuf);
                }
                SYS_LOGD("%s: outputFrameHeight: %d!\n", __FUNCTION__, outputFrameHeight);
                //check outputmode
                if ((source_input == SOURCE_MPEG)
                    || (source_input == SOURCE_DTV)
                    || (source_input == SOURCE_HDMI1)
                    || (source_input == SOURCE_HDMI2)
                    || (source_input == SOURCE_HDMI3)
                    || (source_input == SOURCE_HDMI4)) {//hdmi/dtv/mpeg input
                    int inputFrameHeight = Cpq_GetInputVideoFrameHeight(source_input);
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
        SYS_LOGD("cvbs param exist!\n");
    } else {
        SYS_LOGD("cvbs param don't exist!\n");
    }
    return ret;
}

bool CPQControl::isPqDatabaseMachChip()
{
    bool matchStatus = false;
    meson_cpu_ver_e chipVersion = MESON_CPU_VERSION_NULL;
    database_attribute_t dbAttribute;
    mPQdb->PQ_GetDataBaseAttribute(&dbAttribute);
    if ((dbAttribute.ChipVersion.string() == NULL) || (dbAttribute.ChipVersion.length() == 0)) {
        SYS_LOGD("%s: ChipVersion is null!\n", __FUNCTION__);
        chipVersion = MESON_CPU_VERSION_NULL;
    } else {
        std::string TempStr = std::string(dbAttribute.ChipVersion.string());
        int flagPosition = TempStr.find("_");
        std::string versionStr = TempStr.substr(flagPosition+1, 1);
        SYS_LOGD("%s: versionStr is %s!\n", __FUNCTION__, versionStr.c_str());
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
        SYS_LOGD("%s: database don't have chipversion!\n", __FUNCTION__);
        matchStatus = true;
    } else {
        int ret = VPPDeviceIOCtl(AMVECM_IOC_S_MESON_CPU_VER, &chipVersion);
        if (ret < 0) {
            SYS_LOGD("%s: database don't match chip!\n", __FUNCTION__);
            matchStatus = false;
        } else {
            SYS_LOGD("%s: database is match chip!\n", __FUNCTION__);
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
    int readLenth = 0;
    char* end = NULL;
    char buf[PROPERTY_VALUE_MAX] = {0};
    bool ret = true;

    readLenth = property_get(BOOTVIDEO_ENABLE_PROP, buf, "3050");
    SYS_LOGD("%s: bootvideo enable value is %s!\n", __FUNCTION__, buf);
    if (readLenth > 0) {
        int bootVideoEnable = (strtol(buf, &end, 0)) / 1000;
        if (bootVideoEnable == 3) {
            memset(buf, 0, sizeof(buf));
            readLenth = property_get(BOOTVIDEO_EXIT_PROP, buf, "0");
            if (readLenth > 0) {
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
