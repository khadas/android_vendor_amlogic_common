/*
 * Copyright (C) 2009 The Android Open Source Project
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
 */

package com.droidlogic.app;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.lang.reflect.Method;
import java.lang.reflect.Constructor;
import java.util.UUID;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import android.content.Context;
import android.content.Intent;
import android.hardware.hdmi.HdmiControlManager;
import android.os.SystemProperties;
import android.os.UserHandle;
import android.provider.Settings;
import android.util.Log;
import android.media.AudioManager;
import android.media.AudioFormat;
import android.media.AudioTrack;
import android.media.audiofx.AudioEffect;
import android.content.ContentResolver;
import android.hardware.hdmi.HdmiControlManager;
import android.hardware.hdmi.HdmiTvClient;
import android.hardware.hdmi.HdmiTvClient.SelectCallback;


import com.droidlogic.app.AudioConfigManager;
import com.droidlogic.app.DroidLogicUtils;


public class OutputModeManager {
    private static final String TAG                         = "OutputModeManager";
   // private static final boolean DEBUG                      = false;
    /**
     * The saved value for Outputmode auto-detection.
     * One integer
     * @hide
     */
    public static final String DISPLAY_OUTPUTMODE_AUTO      = "display_outputmode_auto";

    /**
     *  broadcast of the current HDMI output mode changed.
     */
    public static final String ACTION_HDMI_MODE_CHANGED     = "droidlogic.intent.action.HDMI_MODE_CHANGED";

    /**
     * Extra in {@link #ACTION_HDMI_MODE_CHANGED} indicating the mode:
     */
    public static final String EXTRA_HDMI_MODE              = "mode";

    public static final String SYS_DIGITAL_RAW              = "/sys/class/audiodsp/digital_raw";
    public static final String SYS_AUDIO_CAP                = "/sys/class/amhdmitx/amhdmitx0/aud_cap";
    public static final String SYS_AUDIO_HDMI               = "/sys/class/amhdmitx/amhdmitx0/config";
    public static final String SYS_AUDIO_SPDIF              = "/sys/devices/platform/spdif-dit.0/spdif_mute";

    public static final String AUDIO_DSP_AC3_DRC            = "/sys/class/audiodsp/ac3_drc_control";
    public static final String AUDIO_DSP_DTS_DEC            = "/sys/class/audiodsp/dts_dec_control";

    public static final String HDMI_STATE                   = "/sys/class/amhdmitx/amhdmitx0/hpd_state";
    public static final String HDMI_SUPPORT_LIST            = "/sys/class/amhdmitx/amhdmitx0/disp_cap";

    public static final String COLOR_ATTRIBUTE              = "/sys/class/amhdmitx/amhdmitx0/attr";
    public static final String DISPLAY_HDMI_VALID_MODE      = "/sys/class/amhdmitx/amhdmitx0/valid_mode";//test if tv support this mode
    public static final String DOLBY_VISION_IS_SUPPORT2     = "/sys/class/amhdmitx/amhdmitx0/dv_cap2";

    public static final String DISPLAY_AXIS                 = "/sys/class/display/axis";

    public static final String VIDEO_AXIS                   = "/sys/class/video/axis";

    public static final String FB0_FREE_SCALE_AXIS          = "/sys/class/graphics/fb0/free_scale_axis";
    public static final String FB0_FREE_SCALE_MODE          = "/sys/class/graphics/fb0/freescale_mode";
    public static final String FB0_FREE_SCALE               = "/sys/class/graphics/fb0/free_scale";
    public static final String FB1_FREE_SCALE               = "/sys/class/graphics/fb1/free_scale";
    public static final String FB0_BLANK                    = "/sys/class/graphics/fb0/blank";

    public static final String ENV_CVBS_MODE                = "ubootenv.var.cvbsmode";
    public static final String ENV_HDMI_MODE                = "ubootenv.var.hdmimode";
    public static final String ENV_OUTPUT_MODE              = "ubootenv.var.outputmode";
    public static final String ENV_DIGIT_AUDIO              = "ubootenv.var.digitaudiooutput";
    public static final String ENV_IS_BEST_MODE             = "ubootenv.var.is.bestmode";
    public static final String ENV_IS_BEST_DOLBYVISION      = "ubootenv.var.bestdolbyvision";
    public static final String ENV_HDR_PRIORITY             = "ubootenv.var.hdr_priority";
    public static final String ENV_HDR_POLICY               = "ubootenv.var.hdr_policy";
    public static final String ENV_DOLBYSTATUS              = "ubootenv.var.dolby_status";
    public static final String ENV_FRAC_RATE_POLICY         = "ubootenv.var.frac_rate_policy";

    /*
     * cvbs mode extern
     *  null or cvbs:480cvbs and 576 cvbs
     *           pal:pal_n and pal_m
     *          ntsc:ntsc_m
     * ntsc_cvbs_pal:480cvbs/576 cvbs/pal_n/pal_m/ntsc_m
     */
    public static final String PROP_CVBS_MODE_EXTERN        = "ro.vendor.cvbs_extern";
    public static final String PROP_HDMI_ONLY               = "ro.vendor.platform.hdmionly";
    public static final String PROP_SUPPORT_4K              = "ro.vendor.platform.support.4k";
    public static final String PROP_SUPPORT_OVER_4K30       = "ro.platform.support.over.4k30";
    public static final String PROP_DEEPCOLOR               = "vendor.sys.open.deepcolor";
    public static final String PROP_SUPPORT_DOLBY_VISION    = "vendor.system.support.dolbyvision";
    public static final String PROP_ALWAYS_DOLBY_VISION     = "vendor.system.always.dolbyvision";
    public static final String PROP_DTSDRCSCALE             = "persist.vendor.sys.dtsdrcscale";
    public static final String PROP_DTSEDID                 = "persist.vendor.sys.dts.edid";
    public static final String DISPLAY_DEBUG_PROP            = "vendor.display.debug";
    public static final String PROP_LOG_LEVEL               = "persist.vendor.sc.log.level";

    public static final String FULL_WIDTH_480               = "720";
    public static final String FULL_HEIGHT_480              = "480";
    public static final String FULL_WIDTH_576               = "720";
    public static final String FULL_HEIGHT_576              = "576";
    public static final String FULL_WIDTH_720               = "1280";
    public static final String FULL_HEIGHT_720              = "720";
    public static final String FULL_WIDTH_1080              = "1920";
    public static final String FULL_HEIGHT_1080             = "1080";
    public static final String FULL_WIDTH_4K2K              = "3840";
    public static final String FULL_HEIGHT_4K2K             = "2160";
    public static final String FULL_WIDTH_4K2KSMPTE         = "4096";
    public static final String FULL_HEIGHT_4K2KSMPTE        = "2160";

    public static final String ALL_EXTERN_MODE = "ntsc_cvbs_pal";
    public static final String NTSC_MODE = "ntsc";
    public static final String PAL_MODE  = "pal";
    public static final String CVBS_MODE = "cvbs";
    public static final String HDMI_MODE = "hdmi";
    private static String mUiMode;

    //dolby vision mode
    private static final int DV_LL_RGB            = 3;
    private static final int DV_LL_YUV            = 2;
    private static final int DV_ENABLE            = 1;
    private static final int DV_DISABLE           = 0;

    private static final String PARA_AUDIO_DOLBY_MS12       = "dolby_ms12_enable";
    private static final String PARA_AUDIO_DOLBY_MS12_ENABLE= "dolby_ms12_enable=1";

    public static final String DIGITAL_AUDIO_FORMAT                     = "digital_audio_format";
    public static final String DIGITAL_AUDIO_SUBFORMAT                  = "digital_audio_subformat";
    public static final String PARAM_HAL_AUDIO_OUTPUT_FORMAT_PCM        = "hdmi_format=0";
    public static final String PARAM_HAL_AUDIO_OUTPUT_FORMAT_AUTO       = "hdmi_format=5";
    public static final String PARAM_HAL_AUDIO_OUTPUT_FORMAT_PASSTHROUGH= "hdmi_format=6";

    public static final int DIGITAL_AUDIO_FORMAT_PCM                    = 0;
    public static final int DIGITAL_AUDIO_FORMAT_AUTO                   = 1;
    public static final int DIGITAL_AUDIO_FORMAT_MANUAL                 = 2;
    public static final int DIGITAL_AUDIO_FORMAT_PASSTHROUGH            = 3;

    // DD/DD+/DTS
    public static final String DIGITAL_AUDIO_SUBFORMAT_SPDIF            = "5,6,7";

    private static final String NRDP_EXTERNAL_SURROUND                  = "nrdp_external_surround_sound_enabled";
    private static final int NRDP_ENABLE                                = 1;
    private static final int NRDP_DISABLE                               = 0;

    public static final String BOX_HDMI                                 = "box_hdmi";
    public static final String PARA_BOX_HDMI_OFF                        = "Audio hdmi-out mute=1";
    public static final String PARA_BOX_HDMI_ON                         = "Audio hdmi-out mute=0";
    public static final int BOX_HDMI_OFF                                = 0;
    public static final int BOX_HDMI_ON                                 = 1;

    public static final String DB_ID_AUDIO_OUTPUT_DEVICE_ARC_ENABLE     = "db_id_audio_output_device_arc_enable";

    public static final String TV_ARC_LATENCY                           = "tv_arc_latency";
    public static final String PROPERTY_LOCAL_ARC_LATENCY               = "vendor.media.amnuplayer.audio.delayus";
    public static final int TV_ARC_LATENCY_MIN                          = -200;
    public static final int TV_ARC_LATENCY_MAX                          = 200;
    public static final int TV_ARC_LATENCY_DEFAULT                      = -40;

    public static final String VIRTUAL_SURROUND                         = "virtual_surround";
    public static final String PARA_VIRTUAL_SURROUND_OFF                = "enable_virtual_surround=false";
    public static final String PARA_VIRTUAL_SURROUND_ON                 = "enable_virtual_surround=true";
    public static final int VIRTUAL_SURROUND_OFF                        = 0;
    public static final int VIRTUAL_SURROUND_ON                         = 1;

    public static final String DB_ID_SOUND_SPDIF_OUTPUT_ENABLE          = "db_id_sound_spdif_output_enable";
    public static final String HAL_PARAM_SPDIF_OUTPUT_ENABLE            = "hal_param_spdif_output_enable=";

    public static final String DB_ID_SOUND_AD_SWITCH                    = "ad_switch";
    public static final String HAL_PARAM_AD_SWITCH                      = "ad_switch_enable=";

    //surround sound formats, must sync with Settings.Global
    public static final String ENCODED_SURROUND_OUTPUT                  = "encoded_surround_output";
    public static final String ENCODED_SURROUND_OUTPUT_ENABLED_FORMATS  = "encoded_surround_output_enabled_formats";
    public static final int ENCODED_SURROUND_OUTPUT_AUTO                = 0;
    public static final int ENCODED_SURROUND_OUTPUT_NEVER               = 1;
    public static final int ENCODED_SURROUND_OUTPUT_ALWAYS              = 2;
    public static final int ENCODED_SURROUND_OUTPUT_MANUAL              = 3;

    public static final String DIGITAL_SOUND                = "digital_sound";
    public static final String PCM                          = "PCM";
    public static final String RAW                          = "RAW";
    public static final String HDMI                         = "HDMI";
    public static final String SPDIF                        = "SPDIF";
    public static final String HDMI_RAW                     = "HDMI passthrough";
    public static final String SPDIF_RAW                    = "SPDIF passthrough";
    public static final int IS_PCM                          = 0;
    public static final int IS_SPDIF_RAW                    = 1;
    public static final int IS_HDMI_RAW                     = 2;

    public static final String DRC_MODE                     = "drc_mode";
    public static final String DTSDRC_MODE                  = "dtsdrc_mode";
    public static final String CUSTOM_0_DRCMODE             = "0";
    public static final String CUSTOM_1_DRCMODE             = "1";
    public static final String LINE_DRCMODE                 = "2";
    public static final String RF_DRCMODE                   = "3";
    public static final String DEFAULT_DRCMODE              = LINE_DRCMODE;
    public static final String MIN_DRC_SCALE                = "0";
    public static final String MAX_DRC_SCALE                = "100";
    public static final String DEFAULT_DRC_SCALE            = MIN_DRC_SCALE;
    public static final int IS_DRC_OFF                      = 0;
    public static final int IS_DRC_LINE                     = 1;
    public static final int IS_DRC_RF                       = 2;

    public static final String REAL_OUTPUT_SOC              = "meson8,meson8b,meson8m2,meson9b";
    public static final String UI_720P                      = "720p";
    public static final String UI_1080P                     = "1080p";
    public static final String UI_2160P                     = "2160p";
    public static final String PAL_M                        = "pal_m";
    public static final String PAL_N                        = "pal_n";
    public static final String NTSC_M                       = "ntsc_m";
    public static final String HDMI_480                     = "480";
    public static final String HDMI_576                     = "576";
    public static final String HDMI_720                     = "720p";
    public static final String HDMI_1080                    = "1080";
    public static final String HDMI_4K2K                    = "2160p";
    public static final String HDMI_SMPTE                   = "smpte";
    public static final String HDMI_7680x4320               = "4320p";
    public static final String HDMI_2560X1440               = "1440p";

    private static final String HDR_POLICY_SOURCE           = "1";
    private static final String HDR_POLICY_SINK             = "0";

    private static final int DV_PRIORITY             =  0;
    private static final int HDR_PRIORITY            =  1;
    private static final int SDR_PRIORITY            =  2;
    private static final int MESON_G_DV_HDR10_HLG    = 0x10000000;
    private static final int MESON_G_DV_HDR10        = 0x10000040;
    private static final int MESON_G_DV_HLG          = 0x10000020;
    private static final int MESON_G_HDR10_HLG       = 0x10000010;
    private static final int MESON_G_DV              = 0x10000060;
    private static final int MESON_G_HDR10           = 0x10000050;
    private static final int MESON_G_HLG             = 0x10000030;
    private static final int MESON_G_SDR             = 0x10000070;

    private static final String HDMI_OFFSET_ENABLE           = "1";
    private static final String HDMI_OFFSET_DISABLE          = "0";

    private String DEFAULT_OUTPUT_MODE                      = "720p60hz";

    //ac4 enhancer
    public static final String AC4_DIALOGUE_ENHANCEMENT_VALUE   = "ac4_dialogue_enhancer_value";
    public static final String DIALOGUE_ENHANCEMENT_SWITCH      = "dialogue_enhancement";

    public static final int DIALOGUE_ENHANCEMENT_OFF       = 0;
    public static final int DIALOGUE_ENHANCEMENT_LOW       = 4;
    public static final int DIALOGUE_ENHANCEMENT_MEDIUM    = 8;
    public static final int DIALOGUE_ENHANCEMENT_HIGH      = 12;

    public static final String FORCE_DDP_SWITCH      = "force_ddp_enable";
    public static final int FORCE_DDP_OFF   = 0;
    public static final int FORCE_DDP_ON    = 1;

    //hdmi mode list
    public static final String[] HDMI_LIST = {
        "7680x4320p60hz",
        "7680x4320p50hz",
        "7680x4320p48hz",
        "7680x4320p30hz",
        "7680x4320p25hz",
        "7680x4320p24hz",
        "3840x2160p120hz",
        "3840x2160p100hz",
        "2160p60hz",
        "2160p50hz",
        "2160p30hz",
        "2160p25hz",
        "2160p24hz",
        "smpte24hz",
        "2560x1440p120hz",
        "2560x1440p100hz",
        "2560x1440p60hz",
        "2560x1440p50hz",
        "1920x1080p120hz",
        "1920x1080p100hz",
        "1080p60hz",
        "1080p50hz",
        "1080p24hz",
        "1280x720p120hz",
        "1280x720p100hz",
        "720p60hz",
        "720p50hz",
        "1080i60hz",
        "1080i50hz",
        "576p50hz",
        "480p60hz",
        "640x480p60hz",
        "576i50hz",
        "480i60hz"
    };

    public static final String[] HDMI_TITLE = {
        "7680x4320p 60hz",
        "7680x4320p 50hz",
        "7680x4320p 48hz",
        "7680x4320p 30hz",
        "7680x4320p 25hz",
        "7680x4320p 24hz",
        "3840x2160p 120hz",
        "3840x2160p 100hz",
        "3840x2160p 60hz",
        "3840x2160p 50hz",
        "3840x2160p 30hz",
        "3840x2160p 25hz",
        "3840x2160p 24hz",
        "4096x2160p 24hz",
        "2560x1440p 120hz",
        "2560x1440p 100hz",
        "2560x1440p 60hz",
        "2560x1440p 50hz",
        "1920x1080p 120hz",
        "1920x1080p 100hz",
        "1920x1080p 60hz",
        "1920x1080p 50hz",
        "1920x1080p 24hz",
        "1280x720p 120hz",
        "1280x720p 100hz",
        "1280x720p 60hz",
        "1280x720p 50hz",
        "1920x1080i 60hz",
        "1920x1080i 50hz",
        "720x576p 50hz",
        "720x480p 60hz",
        "640x480p 60hz",
        "720x576i 50hz",
        "720x480i 60hz"
    };

    //cvbs mode list
    public static final String[] CVBS_MODE_LIST = {
        "480cvbs",
        "576cvbs"
    };

    //pal mode list
    public static final String[] PAL_MODE_LIST = {
        "pal_m",
        "pal_n"
    };

    //ntsc mode list
    public static final String[] NTSC_MODE_LIST = {
        "ntsc_m"
    };

    //all cvbs extern mode list
    public static final String[] ALL_CVBS_MODE_EXTERN_LIST = {
        "480cvbs",
        "576cvbs",
        "pal_m",
        "pal_n",
        "ntsc_m"
    };

    public static final String[] HDMI_COLOR_LIST = {
        "444,12bit",
        "444,10bit",
        "444,8bit",
        "422,12bit",
        "422,10bit",
        "422,8bit",
        "420,12bit",
        "420,10bit",
        "420,8bit",
        "rgb,12bit",
        "rgb,10bit",
        "rgb,8bit"
    };

    public static final String[] HDMI_COLOR_LIST_8BIT = {
        "444,8bit",
        "422,8bit",
        "rgb,8bit"
    };

    public static final String[] HDMI_COLOR_TITLE_LIST = {
        "YCbCr 4:4:4 12-bit",
        "YCbCr 4:4:4 10-bit",
        "YCbCr 4:4:4 8-bit",
        "YCbCr 4:2:2 12-bit",
        "YCbCr 4:2:2 10-bit",
        "YCbCr 4:2:2 8-bit",
        "YCbCr 4:2:0 12-bit",
        "YCbCr 4:2:0 10-bit",
        "YCbCr 4:2:0 8-bit",
        "RGB 12-bit",
        "RGB 10-bit",
        "RGB 8-bit"
    };

  public static final String[] DOLBY_VISION_TYPE = {
        "DV_RGB_444_8BIT",
//         "DV_YCbCr_422_12BIT",  //box not support
        "LL_YCbCr_422_12BIT",
        "LL_RGB_444_12BIT",
        "LL_RGB_444_10BIT"
    };

    private static String currentColorAttribute = null;
    private static String currentOutputmode = null;
    private static String tvSupportDolbyVisionMode;
    private static String tvSupportDolbyVisionType;
    private List<String> mOutModeList = new ArrayList<String>();
    private List<String> mOutTitleList = new ArrayList<String>();
    private static String[] mHdmiSupportModeList;
    private static String[] mHdmiSupportTitleList;
    private volatile List<String> mHdmiModeList;
    private volatile List<String> mDolbyVisionModeList;
    private boolean ifModeSetting = false;
    private final Context mContext;
    private final ContentResolver mResolver;
    final Object mLock = new Object[0];

    private SystemControlManager mSystemControl;
    private DolbyVisionSettingManager mDolbyVisionSettingManager;
    private static OutputModeManager mOutputModeManager = null;
    private AudioManager mAudioManager;
    private HdmiTvClient mTvClient = null;

    public static OutputModeManager getInstance(Context context) {
        synchronized (OutputModeManager.class) {
            if (mOutputModeManager == null) {
                mOutputModeManager = new OutputModeManager(context);
            }
        }
        return mOutputModeManager;
    }

    public OutputModeManager(Context context) {
        mContext = context;

        mSystemControl = SystemControlManager.getInstance();
        mDolbyVisionSettingManager = new DolbyVisionSettingManager(mContext);
        mResolver = mContext.getContentResolver();
        currentOutputmode = getCurrentOutputMode();
        mAudioManager = (AudioManager) context.getSystemService(context.AUDIO_SERVICE);
        HdmiControlManager mHdmiControlManager = (HdmiControlManager)mContext.getSystemService(Context.HDMI_CONTROL_SERVICE);
        if (mHdmiControlManager != null) {
            mTvClient = mHdmiControlManager.getTvClient();
        }
    }

    public boolean isSupportNetflix() {
         return mContext.getPackageManager().hasSystemFeature("droidlogic.software.netflix");
    }

    public boolean isSupportDisplayDebug() {
         return mSystemControl.getPropertyBoolean(DISPLAY_DEBUG_PROP, false);
    }

    public boolean isDolbyVisionEnable() {
        return mDolbyVisionSettingManager.isDolbyVisionEnable();
    }

    public boolean isTvSupportDolbyVision() {
        String dv_cap = mDolbyVisionSettingManager.isTvSupportDolbyVision();
        tvSupportDolbyVisionType = null;
        if (!dv_cap.equals("")) {
            for (int i = 0;i < HDMI_LIST.length; i++) {
                if (dv_cap.contains(HDMI_LIST[i])) {
                    tvSupportDolbyVisionMode = HDMI_LIST[i];
                    break;
                }
            }
            for (int i = 0; i < DOLBY_VISION_TYPE.length; i++) {
                if (dv_cap.contains(DOLBY_VISION_TYPE[i])) {
                    tvSupportDolbyVisionType += DOLBY_VISION_TYPE[i];
                }
            }
        } else {
            tvSupportDolbyVisionMode = "";
            tvSupportDolbyVisionType = "";
        }

        return tvSupportDolbyVisionMode.equals("") ? false : true;
    }

    public boolean isDolbyVisionPreference() {
        int hdr_priority = getHdrPriority();

        return mDolbyVisionSettingManager.isDolbyVisionEnable()
               && isTvSupportDolbyVision()
               && (hdr_priority == DV_PRIORITY
               || hdr_priority == MESON_G_DV_HDR10_HLG
               || hdr_priority == MESON_G_DV_HDR10
               || hdr_priority == MESON_G_DV_HLG
               || hdr_priority == MESON_G_DV);
    }

    public String getCVBSModeExtern() {
         return mSystemControl.getPropertyString(PROP_CVBS_MODE_EXTERN, CVBS_MODE);
    }

    public long resolveResolutionValue(String mode) {
        return mSystemControl.resolveResolutionValue(mode);
    }

    public void setOutputMode(final String mode) {
        setOutputModeNowLocked(mode);
    }

    public void setBestMode(String mode) {
        if (mode == null) {
            mSystemControl.setBootenv(ENV_IS_BEST_MODE, "true");
            setOutputMode(getHighestMatchResolution());
        } else {
            mSystemControl.setBootenv(ENV_IS_BEST_MODE, "false");
            mSystemControl.setBootenv(ENV_IS_BEST_DOLBYVISION, "false");
            setOutputModeNowLocked(mode);
        }
    }

    public void setDeepColorMode() {
        if (isDeepColor()) {
            mSystemControl.setProperty(PROP_DEEPCOLOR, "false");
        } else {
            mSystemControl.setProperty(PROP_DEEPCOLOR, "true");
        }
        setOutputModeNowLocked(getCurrentOutputMode());
    }

    public void setDeepColorAttribute(final String colorValue) {
        mSystemControl.setColorSpace(colorValue);
    }

    public String getCurrentColorAttribute() {
       String colorValue = mSystemControl.getDeepColorAttr(getCurrentOutputMode());
       return colorValue;
    }

    public String getHdmiColorSupportList() {
        String colorlist = "";
        colorlist = mSystemControl.getColorSpaceList();

        if (isLogPrint(3))
            Log.d(TAG, "getHdmiColorSupportList: " + colorlist);
        return colorlist;
    }

    public boolean isModeSupportColor(final String curMode, final String curValue){
         return mSystemControl.GetModeSupportDeepColorAttr(curMode,curValue);
    }

    private void setOutputModeNowLocked(final String newMode){
        synchronized (mLock) {
            String oldMode = currentOutputmode;
            currentOutputmode = newMode;

            if (oldMode == null || oldMode.length() < 4) {
                Log.e(TAG, "get display mode error, oldMode:" + oldMode + " set to default " + DEFAULT_OUTPUT_MODE);
                oldMode = DEFAULT_OUTPUT_MODE;
            }

            if (isLogPrint(3))
                Log.d(TAG, "change mode from " + oldMode + " -> " + newMode);

            mSystemControl.setMboxOutputMode(newMode);

            Intent intent = new Intent(ACTION_HDMI_MODE_CHANGED);
            //intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT);
            intent.putExtra(EXTRA_HDMI_MODE, newMode);
            mContext.sendStickyBroadcastAsUser(intent, UserHandle.ALL);
        }
    }

    public void setOsdMouse(String curMode) {
        if (isLogPrint(3))
            Log.d(TAG, "set osd mouse curMode: " + curMode);
        mSystemControl.setOsdMouseMode(curMode);
    }

    public void setOsdMouse(int x, int y, int w, int h) {
        mSystemControl.setOsdMousePara(x, y, w, h);
    }

    public String getCurrentOutputMode() {
        return mSystemControl.getActiveDispMode();
    }

    public int getHdrPriority() {
        return mSystemControl.getHdrPriority();
    }

    public void setHdrPriority(int type) {
        mSystemControl.setHdrPriority(Integer.toString(type));
    }

    public int[] getPosition(String mode) {
        return mSystemControl.getPosition(mode);
    }

    public void savePosition(int left, int top, int width, int height) {
        mSystemControl.setPosition(left, top, width, height);
    }

    public List<String> getOutTitleList() {
        return mOutTitleList;
    }

    public List<String> getOutModeList() {
        RefreshOutModeList();
        return mOutModeList;
    }

    public void filterHdmiSupportModeList() {
        //init mode and title list
        mHdmiSupportModeList  = new String[0];
        mHdmiSupportTitleList = new String[0];

        //If reading hdmi support mode returns failure, stop and return empty list
        ArrayList<String> mSupportDispModeList = new ArrayList<String>();
        mSystemControl.getSupportDispModeList(mSupportDispModeList);
        if (mSupportDispModeList.size() <= 0) {
            Log.w(TAG, "read hdmi support mode fail");
            return;
        }

        //1. update title for 59.94/29.97/23.976
        //mHdmiSupportModeList-->listMode
        //mHdmiSupportTitleList-->listTitle
        List<String> listMode = new ArrayList<String>();
        List<String> listTitle = new ArrayList<String>();
        String frac_rate_policy = getFrameRateOffset();
        for (int i = 0; i < HDMI_LIST.length; i++) {
            if (HDMI_LIST[i] != null) {
                listMode.add(HDMI_LIST[i]);
                if (frac_rate_policy.contains(HDMI_OFFSET_ENABLE)) {
                    if (HDMI_TITLE[i].contains("60hz")) {
                        listTitle.add(HDMI_TITLE[i].replace("60hz", "59.94hz"));
                    } else if (HDMI_TITLE[i].contains("30hz")) {
                        listTitle.add(HDMI_TITLE[i].replace("30hz", "29.97hz"));
                    } else if (HDMI_TITLE[i].contains("24hz")) {
                        listTitle.add(HDMI_TITLE[i].replace("24hz", "23.976hz"));
                    } else {
                        listTitle.add(HDMI_TITLE[i]);
                    }
                } else {
                    listTitle.add(HDMI_TITLE[i]);
                }
            }
        }

        //2. check hdmi edid support mode
        //2.1 filter hdmi edid mode list
        //listMode-->listHdmiMode
        //listTitle-->listHdmiTitle
        List<String> listHdmiMode  = new ArrayList<String>();
        List<String> listHdmiTitle = new ArrayList<String>();
        for (int i = 0; i < listMode.size(); i++) {
            if (mSupportDispModeList.contains(listMode.get(i))) {
                listHdmiMode.add(listMode.get(i));
                listHdmiTitle.add(listTitle.get(i));
            }
        }

        if (isLogPrint(2)) {
            Log.d(TAG, "listHdmiMode: " + listHdmiMode);
        }

        //2.2 filter dolby vision support mode list
        //listHdmiMode-->listHdmiDVMode
        //listHdmiTitle-->listHdmiDVTitle
        List<String> listHdmiDVMode  = new ArrayList<String>();
        List<String> listHdmiDVTitle = new ArrayList<String>();
        if (isDolbyVisionPreference()) {
            //get current dolby vision mode
            int type = mDolbyVisionSettingManager.getDolbyVisionType();
            for (int i = 0; i < listHdmiMode.size(); i++) {
                if (resolveResolutionValue(listHdmiMode.get(i))
                        > resolveResolutionValue(tvSupportDolbyVisionMode)) {
                    Log.w(TAG, "This TV not Support Dolby Vision: " + listHdmiMode.get(i));
                } else {
                    if (listHdmiMode.get(i).contains("smpte")
                        || listHdmiMode.get(i).contains("i")
                        || listHdmiMode.get(i).contains("576p")
                        || listHdmiMode.get(i).contains("480p"))  {
                        Log.w(TAG, "This hdmi mode is not support Dolby Vision: " + listHdmiMode.get(i));
                        continue;
                    }

                    switch (type) {
                        case DV_ENABLE:
                            if (isModeSupportColor(listHdmiMode.get(i), "444,8bit")) {
                                listHdmiDVMode.add(listHdmiMode.get(i));
                                listHdmiDVTitle.add(listHdmiTitle.get(i));
                            }
                            break;
                        case DV_LL_YUV:
                            if (isModeSupportColor(listHdmiMode.get(i), "422,12bit")
                                    || isModeSupportColor(listHdmiMode.get(i), "422,10bit")) {
                                listHdmiDVMode.add(listHdmiMode.get(i));
                                listHdmiDVTitle.add(listHdmiTitle.get(i));
                            }
                            break;
                        case DV_LL_RGB:
                            if (resolveResolutionValue(listHdmiMode.get(i))
                                    > resolveResolutionValue("1080p60hz")) {
                                Log.e(TAG, "This mode is not support dv LL RGB: " + listHdmiMode.get(i));
                                continue;
                            }

                            if (isModeSupportColor(listHdmiMode.get(i), "444,12bit")
                                || isModeSupportColor(listHdmiMode.get(i), "444,10bit")) {
                                listHdmiDVMode.add(listHdmiMode.get(i));
                                listHdmiDVTitle.add(listHdmiTitle.get(i));
                            }
                            break;
                    }
                }
            }

            if (isLogPrint(2)) {
                for (int i =  0; i < listHdmiDVMode.size(); i++) {
                    Log.d(TAG, "listHdmiDVMode:"+ listHdmiDVMode.get(i));
                }
            }
            mHdmiSupportModeList  = listHdmiDVMode.toArray(new String[listHdmiDVMode.size()]);
            mHdmiSupportTitleList = listHdmiDVTitle.toArray(new String[listHdmiDVTitle.size()]);
        } else {
            mHdmiSupportModeList  = listHdmiMode.toArray(new String[listHdmiMode.size()]);
            mHdmiSupportTitleList = listHdmiTitle.toArray(new String[listHdmiTitle.size()]);
        }
    }

    public String getUiMode() {
        String currentMode = getCurrentOutputMode();
        Log.d(TAG,"getUiMode currentMode = " + currentMode);
        if (currentMode.contains(CVBS_MODE)
            || currentMode.contains(PAL_MODE)
            || currentMode.contains(NTSC_MODE)) {
            mUiMode = CVBS_MODE;
        } else {
            mUiMode = HDMI_MODE;
        }
        return mUiMode;
    }

    private void RefreshOutModeList() {
        filterHdmiSupportModeList();
        mOutModeList.clear();
        mOutTitleList.clear();

        String currentUiMode = getUiMode();
        Log.d(TAG,"getOutModeList currentUiMode:" + currentUiMode);

        if (currentUiMode.equalsIgnoreCase(HDMI_MODE)) {
            for (int i=0 ; i< mHdmiSupportTitleList.length; i++) {
                if (mHdmiSupportTitleList[i] != null && mHdmiSupportTitleList[i].length() != 0) {
                    mOutTitleList.add(mHdmiSupportTitleList[i]);
                    mOutModeList.add(mHdmiSupportModeList[i]);
                }
            }
        } else {
           String cvbs_extern_mode = getCVBSModeExtern();
           if (cvbs_extern_mode.equalsIgnoreCase(CVBS_MODE)) {
               for (int i = 0 ; i< CVBS_MODE_LIST.length; i++) {
                   mOutTitleList.add(CVBS_MODE_LIST[i]);
                   mOutModeList.add(CVBS_MODE_LIST[i]);
               }
           } else if (cvbs_extern_mode.equalsIgnoreCase(PAL_MODE)) {
               for (int i = 0 ; i< PAL_MODE_LIST.length; i++) {
                   mOutTitleList.add(PAL_MODE_LIST[i]);
                   mOutModeList.add(PAL_MODE_LIST[i]);
               }
           } else if (cvbs_extern_mode.equalsIgnoreCase(NTSC_MODE)) {
               for (int i = 0 ; i< NTSC_MODE_LIST.length; i++) {
                   mOutTitleList.add(NTSC_MODE_LIST[i]);
                   mOutModeList.add(NTSC_MODE_LIST[i]);
               }
           } else if (cvbs_extern_mode.equalsIgnoreCase(ALL_EXTERN_MODE)) {
               for (int i = 0 ; i< ALL_CVBS_MODE_EXTERN_LIST.length; i++) {
                   mOutTitleList.add(ALL_CVBS_MODE_EXTERN_LIST[i]);
                   mOutModeList.add(ALL_CVBS_MODE_EXTERN_LIST[i]);
               }
           }
        }

        if (isLogPrint(2)) {
            Log.v(TAG, "mOutModeList: " + mOutModeList);
            Log.v(TAG, "mOutTitleList: " + mOutTitleList);
        }
    }

    public String getHdmiSupportList() {
        RefreshOutModeList();
        String list = "";
        if (mOutModeList != null && !mOutModeList.isEmpty()) {
            list = mOutModeList.toString();
        }

        if (isLogPrint(3))
            Log.d(TAG, "getHdmiSupportList: " + list);
        return list;
    }

    public String getHighestMatchResolution() {
        return mSystemControl.getPrefHdmiDispMode();
    }


    public boolean isAudioSupportMs12System() {
        return mAudioManager.getParameters(PARA_AUDIO_DOLBY_MS12).contains(PARA_AUDIO_DOLBY_MS12_ENABLE);
    }

    public String getSupportedResolution() {
        String curMode = getBootenv(ENV_HDMI_MODE, DEFAULT_OUTPUT_MODE);

        if (isLogPrint(3))
            Log.d(TAG, "get supported resolution curMode: " + curMode);

        ArrayList<String> HdmiSupportModeList = new ArrayList<String>();
        mSystemControl.getSupportDispModeList(HdmiSupportModeList);

        if (HdmiSupportModeList.contains(curMode)) {
            return curMode;
        }

        return getHighestMatchResolution();
    }

    private boolean isSupportHdmiMode(String hdmi_mode) {
        String curMode        = null;
        String colorvalue      = null;
        curMode = hdmi_mode.replaceAll("[*]", "");
        if (curMode.contains("2160p60hz") || curMode.contains("2160p50hz")
            || curMode.contains("smpte60hz") || curMode.contains("smpte50hz")) {
            for (int j = 0; j < HDMI_COLOR_LIST.length; j++) {
                colorvalue = HDMI_COLOR_LIST[j];
                if (colorvalue.contains("8bit"))  {
                    if (isModeSupportColor(curMode, colorvalue)) {
                        return true ;
                    }
                }
            }
            return false ;
        } else {
            for (int i = 0; i < HDMI_COLOR_LIST_8BIT.length; i++) {
                colorvalue = HDMI_COLOR_LIST_8BIT[i];
                if (colorvalue.contains("8bit"))  {
                    if (isModeSupportColor(curMode, colorvalue)) {
                        return true ;
                    }
                }
            }
            return false ;
        }
    }

    private String readSupportList(String path) {
        String fullStr = mSystemControl.readSysFsOri(path).replaceAll("\n", "#");
        Log.d(TAG, "TV support list is :" + fullStr);
        return fullStr;
    }

    public void initOutputMode(){
        if (isHDMIPlugged()) {
            setHdmiPlugged();
        } else {
            if (!currentOutputmode.contains("cvbs"))
                setHdmiUnPlugged();
        }

        //there can not set osd mouse parameter, otherwise bootanimation logo will shake
        //because set osd1 scaler will shake
    }

    public void setHdmiUnPlugged(){
        Log.d(TAG, "setHdmiUnPlugged");

        if (getPropertyBoolean(PROP_HDMI_ONLY, true)) {
            String cvbsmode = getBootenv(ENV_CVBS_MODE, "576cvbs");
            setOutputMode(cvbsmode);
        }
    }

    public void setHdmiPlugged() {
        boolean isAutoMode = isBestOutputmode();

        Log.d(TAG, "setHdmiPlugged auto mode: " + isAutoMode);
        if (getPropertyBoolean(PROP_HDMI_ONLY, true)) {
            if (isAutoMode) {
                setOutputMode(getHighestMatchResolution());
            } else {
                String mode = getSupportedResolution();
                setOutputMode(mode);
            }
        }
    }

    public String getFrameRateOffset() {
        String frac_rate_policy = getBootenv(ENV_FRAC_RATE_POLICY, HDMI_OFFSET_ENABLE);
        return frac_rate_policy;
    }

    public void setFrameRateOffset(String mode) {
        String frac_rate_policy = getBootenv(ENV_FRAC_RATE_POLICY, HDMI_OFFSET_ENABLE);

        if (!frac_rate_policy.contains(mode)) {
            setBootenv(ENV_FRAC_RATE_POLICY, mode);
            setOutputModeNowLocked(getCurrentOutputMode());
        }
    }

    public boolean isBestOutputmode() {
        String isBestOutputmode = mSystemControl.getBootenv(ENV_IS_BEST_MODE, "true");
        return Boolean.parseBoolean(isBestOutputmode.equals("") ? "true" : isBestOutputmode);
    }

    public void setBestDolbyVision(boolean enable) {
        mSystemControl.setBootenv(ENV_IS_BEST_DOLBYVISION, enable ? "true" : "false");
    }

    public boolean isBestDolbyVsion() {
        String isBestDolbyVsion = mSystemControl.getBootenv(ENV_IS_BEST_DOLBYVISION, "true");
        Log.e("TEST", "isBestDolbyVsion:" + isBestDolbyVsion);
        return Boolean.parseBoolean(isBestDolbyVsion.equals("") ? "true" : isBestDolbyVsion);
    }
    public void setHdrStrategy(final String HdrStrategy) {
          mSystemControl.setBootenv(ENV_HDR_POLICY, HdrStrategy);
          mSystemControl.setHdrStrategy(HdrStrategy);
    }

    public String getHdrStrategy() {
        return mSystemControl.getHdrStrategy();
    }
    public boolean isDeepColor() {
        return getPropertyBoolean(PROP_DEEPCOLOR, false);
    }

    public boolean isHDMIPlugged() {
        String status = readSysfs(HDMI_STATE);
        if ("1".equals(status))
            return true;
        else
            return false;
    }

    public boolean ifModeIsSetting() {
        return ifModeSetting;
    }

    private void shadowScreen() {
        writeSysfs(FB0_BLANK, "1");
        Thread task = new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    ifModeSetting = true;
                    Thread.sleep(1000);
                    writeSysfs(FB0_BLANK, "0");
                    ifModeSetting = false;
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        });
        task.start();
    }

    public String getDigitalVoiceMode(){
        return getBootenv(ENV_DIGIT_AUDIO, PCM);
    }

    public int autoSwitchHdmiPassthough () {
        String mAudioCapInfo = readSysfsTotal(SYS_AUDIO_CAP);
        if (mAudioCapInfo.contains("Dobly_Digital+")) {
            setDigitalMode(HDMI_RAW);
            return IS_HDMI_RAW;
        } else if (mAudioCapInfo.contains("AC-3")
                || (getPropertyBoolean(PROP_DTSEDID, false) && mAudioCapInfo.contains("DTS"))) {
            setDigitalMode(SPDIF_RAW);
            return IS_SPDIF_RAW;
        } else {
            setDigitalMode(PCM);
            return IS_PCM;
        }
    }

    public void setDigitalMode(String mode) {
        // value : "PCM" ,"RAW","SPDIF passthrough","HDMI passthrough"
        setBootenv(ENV_DIGIT_AUDIO, mode);
        mSystemControl.setDigitalMode(mode);
    }

    public void enableDobly_DRC (boolean enable) {
        if (enable) {       //open DRC
            writeSysfs(AUDIO_DSP_AC3_DRC, "drchighcutscale 0x64");
            writeSysfs(AUDIO_DSP_AC3_DRC, "drclowboostscale 0x64");
        } else {           //close DRC
            writeSysfs(AUDIO_DSP_AC3_DRC, "drchighcutscale 0");
            writeSysfs(AUDIO_DSP_AC3_DRC, "drclowboostscale 0");
        }
    }

    public void setDoblyMode (String mode) {
        //"CUSTOM_0","CUSTOM_1","LINE","RF"; default use "LINE"
        int i = Integer.parseInt(mode);
        if (i >= 0 && i <= 3) {
            writeSysfs(AUDIO_DSP_AC3_DRC, "drcmode" + " " + mode);
        } else {
            writeSysfs(AUDIO_DSP_AC3_DRC, "drcmode" + " " + DEFAULT_DRCMODE);
        }
    }

    public void setDtsDrcScale (String drcscale) {
        //10 one step,100 highest; default use "0"
        int i = Integer.parseInt(drcscale);
        if (i >= 0 && i <= 100) {
            setProperty(PROP_DTSDRCSCALE, drcscale);
        } else {
            setProperty(PROP_DTSDRCSCALE, DEFAULT_DRC_SCALE);
        }
        setDtsDrcScaleSysfs();
    }

    public void setDtsDrcScaleSysfs() {
        String prop = getPropertyString(PROP_DTSDRCSCALE, DEFAULT_DRC_SCALE);
        int val = Integer.parseInt(prop);
        writeSysfs(AUDIO_DSP_DTS_DEC, String.format("0x%02x", val));
    }
    /**
    * @Deprecated
    **/
    public void setDTS_DownmixMode(String mode) {
        // 0: Lo/Ro;   1: Lt/Rt;  default 0
        int i = Integer.parseInt(mode);
        if (i >= 0 && i <= 1) {
            writeSysfs(AUDIO_DSP_DTS_DEC, "dtsdmxmode" + " " + mode);
        } else {
            writeSysfs(AUDIO_DSP_DTS_DEC, "dtsdmxmode" + " " + "0");
        }
    }
    /**
    * @Deprecated
    **/
    public void enableDTS_DRC_scale_control (boolean enable) {
        if (enable) {
            writeSysfs(AUDIO_DSP_DTS_DEC, "dtsdrcscale 0x64");
        } else {
            writeSysfs(AUDIO_DSP_DTS_DEC, "dtsdrcscale 0");
        }
    }
    /**
    * @Deprecated
    **/
    public void enableDTS_Dial_Norm_control (boolean enable) {
        if (enable) {
            writeSysfs(AUDIO_DSP_DTS_DEC, "dtsdialnorm 1");
        } else {
            writeSysfs(AUDIO_DSP_DTS_DEC, "dtsdialnorm 0");
        }
    }

    private boolean isLogPrint(int prio) {
        int log_level = mSystemControl.getPropertyInt(PROP_LOG_LEVEL, 4);
        if (prio < log_level) {
            return false;
        } else {
            return true;
        }
    }

    private String getProperty(String key) {
        if (isLogPrint(4))
            Log.i(TAG, "getProperty key:" + key);
        return mSystemControl.getProperty(key);
    }

    private String getPropertyString(String key, String def) {
        if (isLogPrint(4))
            Log.i(TAG, "getPropertyString key:" + key + " def:" + def);
        return mSystemControl.getPropertyString(key, def);
    }

    private int getPropertyInt(String key,int def) {
        if (isLogPrint(4))
            Log.i(TAG, "getPropertyInt key:" + key + " def:" + def);
        return mSystemControl.getPropertyInt(key, def);
    }

    private long getPropertyLong(String key,long def) {
        if (isLogPrint(4))
            Log.i(TAG, "getPropertyLong key:" + key + " def:" + def);
        return mSystemControl.getPropertyLong(key, def);
    }

    private boolean getPropertyBoolean(String key,boolean def) {
        if (isLogPrint(4))
            Log.i(TAG, "getPropertyBoolean key:" + key + " def:" + def);
        return mSystemControl.getPropertyBoolean(key, def);
    }

    private void setProperty(String key, String value) {
        if (isLogPrint(4))
            Log.i(TAG, "setProperty key:" + key + " value:" + value);
        mSystemControl.setProperty(key, value);
    }

    private String getBootenv(String key, String value) {
        if (isLogPrint(4))
            Log.i(TAG, "getBootenv key:" + key + " def value:" + value);
        return mSystemControl.getBootenv(key, value);
    }

    private int getBootenvInt(String key, String value) {
        if (isLogPrint(4))
            Log.i(TAG, "getBootenvInt key:" + key + " def value:" + value);
        return Integer.parseInt(mSystemControl.getBootenv(key, value));
    }

    private void setBootenv(String key, String value) {
        if (isLogPrint(4))
            Log.i(TAG, "setBootenv key:" + key + " value:" + value);
        mSystemControl.setBootenv(key, value);
    }

    private String readSysfsTotal(String path) {
        return mSystemControl.readSysFs(path).replaceAll("\n", "");
    }

    private String readSysfs(String path) {

        return mSystemControl.readSysFs(path).replaceAll("\n", "");
        /*
        if (!new File(path).exists()) {
            Log.e(TAG, "File not found: " + path);
            return null;
        }

        String str = null;
        StringBuilder value = new StringBuilder();

        if (isLogPrint(4))
            Log.i(TAG, "readSysfs path:" + path);

        try {
            FileReader fr = new FileReader(path);
            BufferedReader br = new BufferedReader(fr);
            try {
                while ((str = br.readLine()) != null) {
                    if (str != null)
                        value.append(str);
                };
                fr.close();
                br.close();
                if (value != null)
                    return value.toString();
                else
                    return null;
            } catch (IOException e) {
                e.printStackTrace();
                return null;
            }
        } catch (FileNotFoundException e) {
            e.printStackTrace();
            return null;
        }
        */
    }

    private boolean writeSysfs(String path, String value) {
        if (isLogPrint(4))
            Log.i(TAG, "writeSysfs path:" + path + " value:" + value);

        return mSystemControl.writeSysFs(path, value);
        /*
        if (!new File(path).exists()) {
            Log.e(TAG, "File not found: " + path);
            return false;
        }

        try {
            BufferedWriter writer = new BufferedWriter(new FileWriter(path), 64);
            try {
                writer.write(value);
            } finally {
                writer.close();
            }
            return true;

        } catch (IOException e) {
            Log.e(TAG, "IO Exception when write: " + path, e);
            return false;
        }
        */
    }

    public void saveDigitalAudioFormatToHal(int mode, String submode) {
        boolean isTv = DroidLogicUtils.isTv();
        int nrdpStatus = NRDP_DISABLE;
        switch (mode) {
            case DIGITAL_AUDIO_FORMAT_MANUAL:
                if (isTv) {
                    mode = DIGITAL_AUDIO_FORMAT_AUTO;
                } else {
                    Settings.Global.putString(mResolver, DIGITAL_AUDIO_SUBFORMAT, submode);
                }
                mAudioManager.setParameters(PARAM_HAL_AUDIO_OUTPUT_FORMAT_AUTO);
                break;
            case DIGITAL_AUDIO_FORMAT_AUTO:
            case DIGITAL_AUDIO_FORMAT_PASSTHROUGH:
                if (isTv && isAudioSupportMs12System()) {
                    nrdpStatus = NRDP_ENABLE;
                }
                if (mode == DIGITAL_AUDIO_FORMAT_AUTO) {
                    mAudioManager.setParameters(PARAM_HAL_AUDIO_OUTPUT_FORMAT_AUTO);
                } else {
                    mAudioManager.setParameters(PARAM_HAL_AUDIO_OUTPUT_FORMAT_PASSTHROUGH);
                }
                break;
            case DIGITAL_AUDIO_FORMAT_PCM:
            default:
                mode = DIGITAL_AUDIO_FORMAT_PCM;
                mAudioManager.setParameters(PARAM_HAL_AUDIO_OUTPUT_FORMAT_PCM);
                break;
        }
        Settings.Global.putInt(mResolver, NRDP_EXTERNAL_SURROUND, nrdpStatus);
        Settings.Global.putInt(mResolver, DIGITAL_AUDIO_FORMAT, mode);
    }

    public void saveDigitalAudioFormatToAndroid(int mode, String submode) {
        String tmp;
        // trigger AudioService retrieve support audio format value. Settings.Global.ENCODED_SURROUND_OUTPUT */
        Settings.Global.putInt(mResolver, ENCODED_SURROUND_OUTPUT, -1);
        int surround = -1;
        switch (mode) {
            case DIGITAL_AUDIO_FORMAT_MANUAL:
                if (DroidLogicUtils.isTv()) {
                    break;
                }
                /* Settings.Global.ENCODED_SURROUND_OUTPUT, Settings.Global.ENCODED_SURROUND_OUTPUT_MANUAL */
                Settings.Global.putInt(mResolver, ENCODED_SURROUND_OUTPUT, ENCODED_SURROUND_OUTPUT_MANUAL);
                tmp = Settings.Global.getString(mResolver, OutputModeManager.ENCODED_SURROUND_OUTPUT_ENABLED_FORMATS);
                if (!submode.equals(tmp)) {
                    Settings.Global.putString(mResolver, ENCODED_SURROUND_OUTPUT_ENABLED_FORMATS, submode);
                }
                break;
            case DIGITAL_AUDIO_FORMAT_PASSTHROUGH:
            case DIGITAL_AUDIO_FORMAT_AUTO:
                /* Settings.Global.ENCODED_SURROUND_OUTPUT, Settings.Global.ENCODED_SURROUND_OUTPUT_AUTO */
                Settings.Global.putInt(mResolver, ENCODED_SURROUND_OUTPUT, ENCODED_SURROUND_OUTPUT_AUTO);
                break;
            case DIGITAL_AUDIO_FORMAT_PCM:
            default:
                /* Settings.Global.ENCODED_SURROUND_OUTPUT, Settings.Global.ENCODED_SURROUND_OUTPUT_NEVER */
                Settings.Global.putInt(mResolver, ENCODED_SURROUND_OUTPUT, ENCODED_SURROUND_OUTPUT_NEVER);
                break;
        }
    }

    public void setDigitalAudioFormatOut(int mode) {
        setDigitalAudioFormatOut(mode, "");
    }

    public void setDigitalAudioFormatOut(int mode, String submode) {
        Log.d(TAG, "setDigitalAudioFormatOut mode:" + mode + ", submode:" + submode);
        if (DIGITAL_AUDIO_FORMAT_MANUAL == mode && submode == null) {
            submode = "";
            Log.i(TAG, "setDigitalAudioFormatOut manual mode, submode is null.");
        }
        saveDigitalAudioFormatToHal(mode, submode);
        saveDigitalAudioFormatToAndroid(mode, submode);
    }

    public int getDigitalAudioFormatOut() {
        return Settings.Global.getInt(mResolver, DIGITAL_AUDIO_FORMAT, DIGITAL_AUDIO_FORMAT_AUTO);
    }

    private final SelectCallback mSelectCallback = new SelectCallback() {
        @Override
        public void onComplete(int result) {
            Log.d(TAG, "setSystemAudioMode onComplete result:" + result);
        }
    };

    public void setARCLatency(int value) {
        if (value > TV_ARC_LATENCY_MAX)
            value = TV_ARC_LATENCY_MAX;
        else if (value < TV_ARC_LATENCY_MIN)
            value = TV_ARC_LATENCY_MIN;
        mSystemControl.setProperty(PROPERTY_LOCAL_ARC_LATENCY, ""+(value*1000));
    }

    public void setVirtualSurround (int value) {
        if (value == VIRTUAL_SURROUND_ON) {
            mAudioManager.setParameters(PARA_VIRTUAL_SURROUND_ON);
        } else {
            mAudioManager.setParameters(PARA_VIRTUAL_SURROUND_OFF);
        }
    }

    public void setAc4DialogEnhancer(int newVal) {
        Log.d(TAG, "setAc4DialogEnhancer: " + newVal);
        switch (newVal) {
            case DIALOGUE_ENHANCEMENT_OFF:
                Settings.Global.putInt(mResolver, DIALOGUE_ENHANCEMENT_SWITCH, 0);
                mAudioManager.setParameters("dialogue_enhancement=0");
                break;
            case DIALOGUE_ENHANCEMENT_LOW:
                Settings.Global.putInt(mResolver, DIALOGUE_ENHANCEMENT_SWITCH, 4);
                mAudioManager.setParameters("dialogue_enhancement=4");
                break;
            case DIALOGUE_ENHANCEMENT_MEDIUM:
                Settings.Global.putInt(mResolver, DIALOGUE_ENHANCEMENT_SWITCH, 8);
                mAudioManager.setParameters("dialogue_enhancement=8");
                break;
            case DIALOGUE_ENHANCEMENT_HIGH:
                Settings.Global.putInt(mResolver, DIALOGUE_ENHANCEMENT_SWITCH, 12);
                mAudioManager.setParameters("dialogue_enhancement=12");
                break;
        }
    }


    public void setForceDDPEnable(boolean newVal) {
        Log.d(TAG, "setForceDDPEnable: " + newVal);
        if (newVal) {
           Settings.Global.putInt(mResolver, FORCE_DDP_SWITCH, FORCE_DDP_ON);
           mAudioManager.setParameters("hal_param_force_ddp=1");
        } else {
           Settings.Global.putInt(mResolver, FORCE_DDP_SWITCH, FORCE_DDP_OFF);
           mAudioManager.setParameters("hal_param_force_ddp=0");
        }
    }

    public int getAc4DialogEnhancer() {
        return Settings.Global.getInt(mResolver, DIALOGUE_ENHANCEMENT_SWITCH, DIALOGUE_ENHANCEMENT_OFF);
    }

    public boolean getForceDDPEnable() {
        return Settings.Global.getInt(mResolver, FORCE_DDP_SWITCH,
                FORCE_DDP_OFF) == FORCE_DDP_ON;
    }


    public void setSoundSpdifEnable(boolean enable) {
        Settings.Global.putInt(mResolver, DB_ID_SOUND_SPDIF_OUTPUT_ENABLE, enable ? 1 : 0);
        mAudioManager.setParameters(HAL_PARAM_SPDIF_OUTPUT_ENABLE + (enable ? 1 : 0));
    }

    public boolean getSoundSpdifEnable() {
        return Settings.Global.getInt(mResolver, DB_ID_SOUND_SPDIF_OUTPUT_ENABLE, 1) != 0;
    }

    public void setAdSupportEnable(boolean newVal) {
        DataProviderManager.putIntValue(mContext, DB_ID_SOUND_AD_SWITCH, newVal ? 1 : 0);
        mAudioManager.setParameters(HAL_PARAM_AD_SWITCH + (newVal ? 1 : 0));
    }

    public boolean getAdSupportEnable() {
        DataProviderManager.getIntValue(mContext, DB_ID_SOUND_AD_SWITCH, 0) ;
        return DataProviderManager.getIntValue(mContext, DB_ID_SOUND_AD_SWITCH, 0) != 0;
    }

    public void initSoundParametersAfterBoot() {
        if (DroidLogicUtils.isTv()) {
            Log.d(TAG, "initSoundParametersAfterBoot start");
            //Settings.Global.putInt(mContext.getContentResolver(), DB_ID_AUDIO_OUTPUT_DEVICE_ARC_ENABLE, 0);
            final int virtualsurround = Settings.Global.getInt(mResolver, VIRTUAL_SURROUND, VIRTUAL_SURROUND_OFF);
            setVirtualSurround(virtualsurround);
        }
        setSoundSpdifEnable(getSoundSpdifEnable());
        setAdSupportEnable(getAdSupportEnable());
        setAc4DialogEnhancer(getAc4DialogEnhancer());
        setForceDDPEnable(getForceDDPEnable());
        AudioConfigManager.getInstance(mContext).initAudioConfigSettings();
    }

    public void resetSoundParameters() {
        if (DroidLogicUtils.isTv()) {
            setVirtualSurround(VIRTUAL_SURROUND_OFF);
        }
    }
}

