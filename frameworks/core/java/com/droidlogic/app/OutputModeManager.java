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

import android.content.Context;
import android.content.Intent;
import android.os.SystemProperties;
import android.os.UserHandle;
import android.provider.Settings;
import android.util.Log;
import android.media.AudioManager;
import android.content.ContentResolver;
import android.hardware.hdmi.HdmiControlManager;
import android.hardware.hdmi.HdmiTvClient;
import android.hardware.hdmi.HdmiTvClient.SelectCallback;


import com.droidlogic.app.AudioConfigManager;
import com.droidlogic.app.DroidLogicUtils;


public class OutputModeManager {
    private static final String TAG                         = "OutputModeManager";
    private static final boolean DEBUG                      = false;
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
    public static final String SYS_AUIDO_HDMI               = "/sys/class/amhdmitx/amhdmitx0/config";
    public static final String SYS_AUIDO_SPDIF              = "/sys/devices/platform/spdif-dit.0/spdif_mute";

    public static final String AUIDO_DSP_AC3_DRC            = "/sys/class/audiodsp/ac3_drc_control";
    public static final String AUIDO_DSP_DTS_DEC            = "/sys/class/audiodsp/dts_dec_control";

    public static final String HDMI_STATE                   = "/sys/class/amhdmitx/amhdmitx0/hpd_state";
    public static final String HDMI_SUPPORT_LIST            = "/sys/class/amhdmitx/amhdmitx0/disp_cap";
    public static final String HDMI_COLOR_SUPPORT_LIST      = "/sys/class/amhdmitx/amhdmitx0/dc_cap";

    public static final String HDMI_VESA_SUPPORT_LIST       = "/sys/class/amhdmitx/amhdmitx0/vesa_cap";

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
    public static final String ENV_COLORATTRIBUTE           = "ubootenv.var.colorattribute";
    public static final String ENV_HDR_PRIORITY             = "ubootenv.var.hdr_priority";
    public static final String ENV_HDR_POLICY               = "ubootenv.var.hdr_policy";
    public static final String ENV_DOLBYSTATUS              = "ubootenv.var.dolby_status";
    public static final String ENV_FRAC_RATE_POLICY         = "ubootenv.var.frac_rate_policy";

    public static final String PROP_BEST_OUTPUT_MODE        = "ro.vendor.platform.best_outputmode";
    public static final String PROP_HDMI_ONLY               = "ro.vendor.platform.hdmionly";
    public static final String PROP_SUPPORT_4K              = "ro.vendor.platform.support.4k";
    public static final String PROP_SUPPORT_OVER_4K30       = "ro.platform.support.over.4k30";
    public static final String PROP_DEEPCOLOR               = "vendor.sys.open.deepcolor";
    public static final String PROP_SUPPORT_DOLBY_VISION    = "vendor.system.support.dolbyvision";
    public static final String PROP_ALWAYS_DOLBY_VISION     = "vendor.system.always.dolbyvision";
    public static final String PROP_DTSDRCSCALE             = "persist.vendor.sys.dtsdrcscale";
    public static final String PROP_DTSEDID                 = "persist.vendor.sys.dts.edid";
    public static final String PROP_HDMI_FRAMERATE_PRIORITY = "persist.vendor.sys.framerate.priority";
    public static final String DISPLY_DEBUG_PROP            = "vendor.display.debug";

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

    public static final String BOX_LINE_OUT                             = "box_line_out";
    public static final String PARA_BOX_LINE_OUT_OFF                    = "enable_line_out=false";
    public static final String PARA_BOX_LINE_OUT_ON                     = "enable_line_out=true";
    public static final int BOX_LINE_OUT_OFF                            = 0;
    public static final int BOX_LINE_OUT_ON                             = 1;

    public static final String BOX_HDMI                                 = "box_hdmi";
    public static final String PARA_BOX_HDMI_OFF                        = "Audio hdmi-out mute=1";
    public static final String PARA_BOX_HDMI_ON                         = "Audio hdmi-out mute=0";
    public static final int BOX_HDMI_OFF                                = 0;
    public static final int BOX_HDMI_ON                                 = 1;

    public static final String TV_SPEAKER                               = "tv_speaker";
    public static final String PARA_TV_SPEAKER_OFF                      = "speaker_mute=1";
    public static final String PARA_TV_SPEAKER_ON                       = "speaker_mute=0";
    public static final int TV_SPEAKER_OFF                              = 0;
    public static final int TV_SPEAKER_ON                               = 1;

    public static final String TV_ARC                                   = "tv_arc";
    public static final String PARA_TV_ARC_OFF                          = "HDMI ARC Switch=0";
    public static final String PARA_TV_ARC_ON                           = "HDMI ARC Switch=1";
    public static final int TV_ARC_OFF                                  = 0;
    public static final int TV_ARC_ON                                   = 1;

    public static final String DB_ID_AUDIO_OUTPUT_DEVICE_ARC_ENABLE     = "db_id_audio_output_device_arc_enable";

    public static final String TV_ARC_LATENCY                           = "tv_arc_latency";
    public static final String PROPERTY_LOCAL_ARC_LATENCY               = "media.amnuplayer.audio.delayus";
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

    public static final String DB_ID_AUDIO_OUTPUT_DEVICE_HDMI_OUT_ENABLE= "db_id_audio_output_device_hdmi_out_enable";
    public static final String HAL_PARAM_HDMI_OUT_ENABLE                = "hal_param_hdmi_output_enable=";
    public static final String HAL_PARAM_EARCTX_EARC_MODE               = "hal_param_earctx_earc_mode=";
    public static final String EARC_ENABLE                              = "earc_enable";

    public static final String DB_ID_SOUND_AD_SWITCH                    = "ad_switch";
    public static final String HAL_PARAM_AD_SWITCH                      = "associate_audio_mixing_enable=";

    //surround sound formats, must sync with Settings.Global
    public static final String ENCODED_SURROUND_OUTPUT                  = "encoded_surround_output";
    public static final String ENCODED_SURROUND_OUTPUT_ENABLED_FORMATS  = "encoded_surround_output_enabled_formats";
    public static final int ENCODED_SURROUND_OUTPUT_AUTO                = 0;
    public static final int ENCODED_SURROUND_OUTPUT_NEVER               = 1;
    public static final int ENCODED_SURROUND_OUTPUT_ALWAYS              = 2;
    public static final int ENCODED_SURROUND_OUTPUT_MANUAL              = 3;

    public static final String SOUND_OUTPUT_DEVICE                      = "sound_output_device";
    public static final String PARA_SOUND_OUTPUT_DEVICE_SPEAKER         = "sound_output_device=speak";
    public static final String PARA_SOUND_OUTPUT_DEVICE_ARC             = "sound_output_device=arc";
    public static final int SOUND_OUTPUT_DEVICE_SPEAKER                 = 0;
    public static final int SOUND_OUTPUT_DEVICE_ARC                     = 1;

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
    public static final String HDMI_480                     = "480";
    public static final String HDMI_576                     = "576";
    public static final String HDMI_720                     = "720p";
    public static final String HDMI_1080                    = "1080";
    public static final String HDMI_4K2K                    = "2160p";
    public static final String HDMI_SMPTE                   = "smpte";

    private static final String HDR_POLICY_SOURCE           = "1";
    private static final String HDR_POLICY_SINK             = "0";

    private static final String DEFAULT_HDR_PRIORITY        = "0";

    private static final String HDMI_OFFSET_ENABLE           = "1";
    private static final String HDMI_OFFSET_DISABLE          = "0";

    private String DEFAULT_OUTPUT_MODE                      = "720p60hz";

    //ac4 enhancer
    public static final String AC4_DIAGLOGUE_ENHANCEMENT_VALUE   = "ac4_diaglogue_enhancer_value";
    public static final String DIAGLOGUE_ENHANCEMENT_SWITCH      = "diaglogue_enhancement";

    public static final int DIAGLOGUE_ENHANCEMENT_OFF       = 0;
    public static final int DIAGLOGUE_ENHANCEMENT_LOW       = 4;
    public static final int DIAGLOGUE_ENHANCEMENT_MEDIUM    = 8;
    public static final int DIAGLOGUE_ENHANCEMENT_HIGH      = 12;

    public static final String FORCE_DDP_SWITCH      = "force_ddp_enable";
    public static final int FORCE_DDP_OFF   = 0;
    public static final int FORCE_DDP_ON    = 1;

    private static final String[] MODE_RESOLUTION_FIRST = {
        "480i60hz",
        "576i50hz",
        "480p60hz",
        "576p50hz",
        "720p50hz",
        "720p60hz",
        "1080p50hz",
        "1080p60hz",
        "2160p24hz",
        "2160p25hz",
        "2160p30hz",
        "2160p50hz",
        "2160p60hz",
    };
    private static final String[] MODE_FRAMERATE_FIRST = {
        "480i60hz",
        "576i50hz",
        "480p60hz",
        "576p50hz",
        "720p50hz",
        "720p60hz",
        "2160p24hz",
        "2160p25hz",
        "2160p30hz",
        "1080p50hz",
        "1080p60hz",
        "2160p50hz",
        "2160p60hz",
    };
    private static final String[] HDMI_COLOR_LIST = {
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
    private static String currentColorAttribute = null;
    private static String currentOutputmode = null;
    private boolean ifModeSetting = false;
    private final Context mContext;
    private final ContentResolver mResolver;
    final Object mLock = new Object[0];

    private SystemControlManager mSystemControl;
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
         return mSystemControl.getPropertyBoolean(DISPLY_DEBUG_PROP, false);
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
        mSystemControl.setBootenv(ENV_IS_BEST_MODE, "false");
        mSystemControl.setBootenv(ENV_COLORATTRIBUTE, colorValue);
    }

    public String getCurrentColorAttribute() {
       String colorValue = mSystemControl.getDeepColorAttr(getCurrentOutputMode());
       return colorValue;
    }

    public String getHdmiColorSupportList() {
        String list = readSupportList(HDMI_COLOR_SUPPORT_LIST);

        if (DEBUG)
            Log.d(TAG, "getHdmiColorSupportList :" + list);
        return list;
    }

    public String getHdmiVesaSupportList() {
        String list = readSupportList(HDMI_VESA_SUPPORT_LIST).replaceAll("[*]", "");

        if (DEBUG)
            Log.d(TAG, "getHdmiVesaSupportList :" + list);
            return list;
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

            if (DEBUG)
                Log.d(TAG, "change mode from " + oldMode + " -> " + newMode);

            mSystemControl.setMboxOutputMode(newMode);

            Intent intent = new Intent(ACTION_HDMI_MODE_CHANGED);
            //intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT);
            intent.putExtra(EXTRA_HDMI_MODE, newMode);
            mContext.sendStickyBroadcastAsUser(intent, UserHandle.ALL);
        }
    }

    public void setOsdMouse(String curMode) {
        if (DEBUG)
            Log.d(TAG, "set osd mouse curMode " + curMode);
        mSystemControl.setOsdMouseMode(curMode);
    }

    public void setOsdMouse(int x, int y, int w, int h) {
        mSystemControl.setOsdMousePara(x, y, w, h);
    }

    public boolean isEarcEnabled (){
        return Settings.Global.getInt(mContext.getContentResolver(), EARC_ENABLE, 1) == 1;
    }

    public void enableEarc(boolean value) {
        Log.d(TAG, "enable eARC Audio : " + value);
        Settings.Global.putInt(mContext.getContentResolver(), EARC_ENABLE, (value ? 1 : 0));
        //mAudioManager.setParameters(HAL_PARAM_EARCTX_EARC_MODE + (value ? 1 : 0));
    }

    public String getCurrentOutputMode() {
        return mSystemControl.getActiveDispMode();
    }

    public int getHdrPriority() {
        String curType = getBootenv(ENV_HDR_PRIORITY, DEFAULT_HDR_PRIORITY);
        Log.d(TAG, "getHdrPriority curType: " + curType);
        return Integer.parseInt(curType);
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

    public String getHdmiSupportList() {
        String list = readSupportList(HDMI_SUPPORT_LIST).replaceAll("[*]", "");

        if (DEBUG)
            Log.d(TAG, "getHdmiSupportList :" + list);
        return list;
    }

    public String getHighestMatchResolution() {
        String value = readSupportList(HDMI_SUPPORT_LIST);
        if (getPropertyBoolean(PROP_HDMI_FRAMERATE_PRIORITY, true)) {
            for (int i = MODE_FRAMERATE_FIRST.length - 1; i >= 0 ; i--) {
                if (value.contains(MODE_FRAMERATE_FIRST[i])) {
                    return MODE_FRAMERATE_FIRST[i];
                }
            }
        } else {
            for (int i = MODE_RESOLUTION_FIRST.length - 1; i >= 0 ; i--) {
                if (value.contains(MODE_RESOLUTION_FIRST[i])) {
                    return MODE_RESOLUTION_FIRST[i];
                }
            }
        }

        return getPropertyString(PROP_BEST_OUTPUT_MODE, DEFAULT_OUTPUT_MODE);
    }

    public boolean isAudioSupportMs12System() {
        return mAudioManager.getParameters(PARA_AUDIO_DOLBY_MS12).contains(PARA_AUDIO_DOLBY_MS12_ENABLE);
    }

    public String getSupportedResolution() {
        String curMode = getBootenv(ENV_HDMI_MODE, DEFAULT_OUTPUT_MODE);

        if (DEBUG)
            Log.d(TAG, "get supported resolution curMode:" + curMode);

        ArrayList<String> HdmiSupportModeList = new ArrayList<String>();
        mSystemControl.getSupportDispModeList(HdmiSupportModeList);

        if (HdmiSupportModeList.contains(curMode)) {
            return curMode;
        }

        return getHighestMatchResolution();
    }

    private boolean isSupportHdmiMode(String hdmi_mode) {
        String curMode        = null;
        curMode = hdmi_mode.replaceAll("[*]", "");
        if (curMode.contains("2160p60hz") || curMode.contains("2160p50hz")
            || curMode.contains("smpte60hz") || curMode.contains("smpte50hz")) {
            for (int j = 0; j < HDMI_COLOR_LIST.length; j++) {
                String colorvalue      = null;
                colorvalue                = HDMI_COLOR_LIST[j];
                if (colorvalue.contains("8bit"))  {
                    if (isModeSupportColor(curMode, colorvalue)) {
                        return true ;
                    }
                }
            }
            return false ;
        }
        return true ;
    }

    private String readSupportList(String path) {
        String fullStr = mSystemControl.readSysFsOri(path).replaceAll("\n", ",");
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

    public String getHdrStrategy(){
        String dolbyvisionType = getBootenv(ENV_HDR_POLICY, HDR_POLICY_SINK);
        return dolbyvisionType;
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
            writeSysfs(AUIDO_DSP_AC3_DRC, "drchighcutscale 0x64");
            writeSysfs(AUIDO_DSP_AC3_DRC, "drclowboostscale 0x64");
        } else {           //close DRC
            writeSysfs(AUIDO_DSP_AC3_DRC, "drchighcutscale 0");
            writeSysfs(AUIDO_DSP_AC3_DRC, "drclowboostscale 0");
        }
    }

    public void setDoblyMode (String mode) {
        //"CUSTOM_0","CUSTOM_1","LINE","RF"; default use "LINE"
        int i = Integer.parseInt(mode);
        if (i >= 0 && i <= 3) {
            writeSysfs(AUIDO_DSP_AC3_DRC, "drcmode" + " " + mode);
        } else {
            writeSysfs(AUIDO_DSP_AC3_DRC, "drcmode" + " " + DEFAULT_DRCMODE);
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
        writeSysfs(AUIDO_DSP_DTS_DEC, String.format("0x%02x", val));
    }
    /**
    * @Deprecated
    **/
    public void setDTS_DownmixMode(String mode) {
        // 0: Lo/Ro;   1: Lt/Rt;  default 0
        int i = Integer.parseInt(mode);
        if (i >= 0 && i <= 1) {
            writeSysfs(AUIDO_DSP_DTS_DEC, "dtsdmxmode" + " " + mode);
        } else {
            writeSysfs(AUIDO_DSP_DTS_DEC, "dtsdmxmode" + " " + "0");
        }
    }
    /**
    * @Deprecated
    **/
    public void enableDTS_DRC_scale_control (boolean enable) {
        if (enable) {
            writeSysfs(AUIDO_DSP_DTS_DEC, "dtsdrcscale 0x64");
        } else {
            writeSysfs(AUIDO_DSP_DTS_DEC, "dtsdrcscale 0");
        }
    }
    /**
    * @Deprecated
    **/
    public void enableDTS_Dial_Norm_control (boolean enable) {
        if (enable) {
            writeSysfs(AUIDO_DSP_DTS_DEC, "dtsdialnorm 1");
        } else {
            writeSysfs(AUIDO_DSP_DTS_DEC, "dtsdialnorm 0");
        }
    }

    private String getProperty(String key) {
        if (DEBUG)
            Log.i(TAG, "getProperty key:" + key);
        return mSystemControl.getProperty(key);
    }

    private String getPropertyString(String key, String def) {
        if (DEBUG)
            Log.i(TAG, "getPropertyString key:" + key + " def:" + def);
        return mSystemControl.getPropertyString(key, def);
    }

    private int getPropertyInt(String key,int def) {
        if (DEBUG)
            Log.i(TAG, "getPropertyInt key:" + key + " def:" + def);
        return mSystemControl.getPropertyInt(key, def);
    }

    private long getPropertyLong(String key,long def) {
        if (DEBUG)
            Log.i(TAG, "getPropertyLong key:" + key + " def:" + def);
        return mSystemControl.getPropertyLong(key, def);
    }

    private boolean getPropertyBoolean(String key,boolean def) {
        if (DEBUG)
            Log.i(TAG, "getPropertyBoolean key:" + key + " def:" + def);
        return mSystemControl.getPropertyBoolean(key, def);
    }

    private void setProperty(String key, String value) {
        if (DEBUG)
            Log.i(TAG, "setProperty key:" + key + " value:" + value);
        mSystemControl.setProperty(key, value);
    }

    private String getBootenv(String key, String value) {
        if (DEBUG)
            Log.i(TAG, "getBootenv key:" + key + " def value:" + value);
        return mSystemControl.getBootenv(key, value);
    }

    private int getBootenvInt(String key, String value) {
        if (DEBUG)
            Log.i(TAG, "getBootenvInt key:" + key + " def value:" + value);
        return Integer.parseInt(mSystemControl.getBootenv(key, value));
    }

    private void setBootenv(String key, String value) {
        if (DEBUG)
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

        if (DEBUG)
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
        if (DEBUG)
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

    public void enableBoxLineOutAudio(boolean value) {
        if (value) {
            mAudioManager.setParameters(PARA_BOX_LINE_OUT_ON);
        } else {
            mAudioManager.setParameters(PARA_BOX_LINE_OUT_OFF);
        }
    }

    public void enableBoxHdmiAudio(boolean value) {
        if (value) {
            mAudioManager.setParameters(PARA_BOX_HDMI_ON);
        } else {
            mAudioManager.setParameters(PARA_BOX_HDMI_OFF);
        }
    }

    public void enableTvSpeakerAudio(boolean value) {
        if (value) {
            mAudioManager.setParameters(PARA_TV_SPEAKER_ON);
        } else {
            mAudioManager.setParameters(PARA_TV_SPEAKER_OFF);
        }
    }

    private final SelectCallback mSelectCallback = new SelectCallback() {
        @Override
        public void onComplete(int result) {
            Log.d(TAG, "setSystemAudioMode onComplete result:" + result);
        }
    };

    public void enableTvArcAudio(boolean value) {
        if (value) {
            mAudioManager.setParameters(PARA_TV_ARC_ON);
        } else {
            mAudioManager.setParameters(PARA_TV_ARC_OFF);
        }
        mTvClient.setSystemAudioMode(value, mSelectCallback);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_AUDIO_OUTPUT_DEVICE_ARC_ENABLE, (value ? 1 : 0));
    }

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

    public void setSoundOutputStatus (int mode) {
        switch (mode) {
            case SOUND_OUTPUT_DEVICE_SPEAKER:
                enableTvSpeakerAudio(true);
                enableTvArcAudio(false);
                break;
            case SOUND_OUTPUT_DEVICE_ARC:
                enableTvSpeakerAudio(false);
                enableTvArcAudio(true);
                break;
        }
    }

    public boolean isTvAudioHdmiOutOn() {
        int value = Settings.Global.getInt(mContext.getContentResolver(), OutputModeManager.DB_ID_AUDIO_OUTPUT_DEVICE_HDMI_OUT_ENABLE, 0);
        return (value != 0);
    }

    public void setTvAudioHdmiOutOn(boolean enable) {
        mAudioManager.setParameters(HAL_PARAM_HDMI_OUT_ENABLE + enable);
        Settings.Global.putInt(mContext.getContentResolver(), OutputModeManager.DB_ID_AUDIO_OUTPUT_DEVICE_HDMI_OUT_ENABLE, enable ? 1 : 0);
    }

    public void setAc4DialogEnhancer(int newVal) {
        Log.d(TAG, "setAc4DialogEnhancer: " + newVal);
        switch (newVal) {
            case DIAGLOGUE_ENHANCEMENT_OFF:
                Settings.Global.putInt(mResolver, DIAGLOGUE_ENHANCEMENT_SWITCH, 0);
                mAudioManager.setParameters("diaglogue_enhancement=0");
                break;
            case DIAGLOGUE_ENHANCEMENT_LOW:
                Settings.Global.putInt(mResolver, DIAGLOGUE_ENHANCEMENT_SWITCH, 4);
                mAudioManager.setParameters("diaglogue_enhancement=4");
                break;
            case DIAGLOGUE_ENHANCEMENT_MEDIUM:
                Settings.Global.putInt(mResolver, DIAGLOGUE_ENHANCEMENT_SWITCH, 8);
                mAudioManager.setParameters("diaglogue_enhancement=8");
                break;
            case DIAGLOGUE_ENHANCEMENT_HIGH:
                Settings.Global.putInt(mResolver, DIAGLOGUE_ENHANCEMENT_SWITCH, 12);
                mAudioManager.setParameters("diaglogue_enhancement=12");
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
        return Settings.Global.getInt(mResolver, DIAGLOGUE_ENHANCEMENT_SWITCH, DIAGLOGUE_ENHANCEMENT_OFF);
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

    public void setAdSurportEnable(boolean newVal) {
        DataProviderManager.putIntValue(mContext, DB_ID_SOUND_AD_SWITCH, newVal ? 1 : 0);
        mAudioManager.setParameters(HAL_PARAM_AD_SWITCH + (newVal ? 1 : 0));
    }

    public boolean getAdSurportEnable() {
        DataProviderManager.getIntValue(mContext, DB_ID_SOUND_AD_SWITCH, 0) ;
        return DataProviderManager.getIntValue(mContext, DB_ID_SOUND_AD_SWITCH, 0) != 0;
    }

    public void initSoundParametersAfterBoot() {
        if (DroidLogicUtils.isTv()) {
            Log.d(TAG, "initSoundParametersAfterBoot start");
            //Settings.Global.putInt(mContext.getContentResolver(), DB_ID_AUDIO_OUTPUT_DEVICE_ARC_ENABLE, 0);
            final int virtualsurround = Settings.Global.getInt(mResolver, VIRTUAL_SURROUND, VIRTUAL_SURROUND_OFF);
            setVirtualSurround(virtualsurround);
            setTvAudioHdmiOutOn(isTvAudioHdmiOutOn());
        } else {
            final int boxlineout = Settings.Global.getInt(mResolver, BOX_LINE_OUT, BOX_LINE_OUT_OFF);
            enableBoxLineOutAudio(boxlineout == BOX_LINE_OUT_ON);
            final int boxhdmi = Settings.Global.getInt(mResolver, BOX_HDMI, BOX_HDMI_ON);
            enableBoxHdmiAudio(boxhdmi == BOX_HDMI_ON);
        }
        setSoundSpdifEnable(getSoundSpdifEnable());
        setAdSurportEnable(getAdSurportEnable());
        setAc4DialogEnhancer(getAc4DialogEnhancer());
        setForceDDPEnable(getForceDDPEnable());
        //enableEarc(isEarcEnabled ());
        AudioConfigManager.getInstance(mContext).initAudioConfigSettings();
    }

    public void resetSoundParameters() {
        if (DroidLogicUtils.isTv()) {
            enableTvSpeakerAudio(false);
            enableTvArcAudio(false);
            setVirtualSurround(VIRTUAL_SURROUND_OFF);
            setSoundOutputStatus(SOUND_OUTPUT_DEVICE_SPEAKER);
        } else {
            enableBoxLineOutAudio(false);
            enableBoxHdmiAudio(false);
        }
    }
}

