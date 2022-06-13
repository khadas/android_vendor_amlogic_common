/*
 * Copyright (C) 2015 The Android Open Source Project
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
 * limitations under the License
 */

package com.android.tv.settings.pqsettings;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.text.TextUtils;
import android.util.Log;
import android.os.SystemProperties;
import android.provider.Settings;
import android.widget.Toast;

import com.android.tv.settings.R;
import com.android.tv.settings.SettingsConstant;
import com.droidlogic.app.SystemControlManager;
import com.droidlogic.app.tv.TvControlManager;
import com.droidlogic.app.tv.TvInSignalInfo;
import com.droidlogic.app.tv.DroidLogicTvUtils;
import com.droidlogic.app.tv.ChannelInfo;
import com.droidlogic.app.tv.TvDataBaseManager;
import com.droidlogic.app.DataProviderManager;
import android.media.tv.TvContract;


public class PQSettingsManager {
    public static final String TAG                                  = "PQSettingsManager";
    public static final String CURRENT_DEVICE_ID                    = "current_device_id";
    public static final String CURRENT_CHANNEL_ID                   = "current_channel_id";
    public static final String TV_CURRENT_DEVICE_ID                 = "tv_current_device_id";

    public static final String KEY_PICTURE                          = "picture";
    public static final String KEY_PICTURE_MODE                     = "picture_mode";
    public static final String KEY_BRIGHTNESS                       = "brightness";
    public static final String KEY_CONTRAST                         = "contrast";
    public static final String KEY_COLOR                            = "color";
    public static final String KEY_SHARPNESS                        = "sharpness";
    public static final String KEY_BACKLIGHT                        = "backlight";
    public static final String KEY_TONE                             = "tone";
    public static final String KEY_COLOR_TEMPERATURE                = "color_temperature";
    public static final String KEY_ASPECT_RATIO                     = "aspect_ratio";
    public static final String KEY_DNR                              = "dnr";
    public static final String KEY_3D_SETTINGS                      = "settings_3d";

    public static final String STATUS_STANDARD                      = "standard";
    public static final String STATUS_VIVID                         = "vivid";
    public static final String STATUS_SOFT                          = "soft";
    public static final String STATUS_SPORT                         = "sport";
    public static final String STATUS_MONITOR                       = "monitor";
    public static final String STATUS_GAME                          = "game";
    public static final String STATUS_USER                          = "user";
    public static final String STATUS_WARM                          = "warm";
    public static final String STATUS_MUSIC                         = "music";
    public static final String STATUS_NEWS                          = "news";
    public static final String STATUS_MOVIE                         = "movie";
    public static final String STATUS_COOL                          = "cool";
    public static final String STATUS_ON                            = "on";
    public static final String STATUS_OFF                           = "off";
    public static final String STATUS_AUTO                          = "auto";
    public static final String STATUS_4_TO_3                        = "4:3";
    public static final String STATUS_PANORAMA                      = "panorama";
    public static final String STATUS_FULL_SCREEN                   = "full_screen";
    public static final String STATUS_MEDIUM                        = "medium";
    public static final String STATUS_HIGH                          = "high";
    public static final String STATUS_LOW                           = "low";
    public static final String STATUS_3D_LR_MODE                    = "left right mode";
    public static final String STATUS_3D_RL_MODE                    = "right left mode";
    public static final String STATUS_3D_UD_MODE                    = "up down mode";
    public static final String STATUS_3D_DU_MODE                    = "down up mode";
    public static final String STATUS_3D_TO_2D                      = "3D to 2D";
    public static final String STATUS_PCM                           = "pcm";
    public static final String STATUS_STEREO                        = "stereo";
    public static final String STATUS_LEFT_CHANNEL                  = "left channel";
    public static final String STATUS_RIGHT_CHANNEL                 = "right channel";
    public static final String STATUS_RAW                           = "raw";

    public static final int PERCENT_INCREASE                        = 1;
    public static final int PERCENT_DECREASE                        = -1;
    public static final int ADVANCED_GAMMA_FIXED_DIFFERENCE         = -6;
    public static String currentTag = null;

    private Resources mResources;
    private Context mContext;
    private SystemControlManager mSystemControlManager;
    private TvControlManager mTvControlManager;
    private TvDataBaseManager mTvDataBaseManager;
    private TvControlManager.SourceInput mTvSourceInput;
    private TvControlManager.SourceInput_Type mVirtualTvSource;
    private TvControlManager.SourceInput_Type mTvSource;
    private int mDeviceId;
    private long mChannelId;
    private int mVideoStd;
    private Activity mActivity;

    public PQSettingsManager (Context context) {
        mContext = context;
        mActivity = (Activity)context;
        mDeviceId = mActivity.getIntent().getIntExtra(TV_CURRENT_DEVICE_ID, -1);
        mChannelId = mActivity.getIntent().getLongExtra(CURRENT_CHANNEL_ID, -1);
        mResources = mContext.getResources();
        mSystemControlManager = SystemControlManager.getInstance();
        if (SettingsConstant.needDroidlogicTvFeature(mContext)) {
            ChannelInfo currentChannel;
            if (mTvControlManager == null) {
                mTvControlManager = TvControlManager.getInstance();
            }
            mTvDataBaseManager = new TvDataBaseManager(context);
            mTvSource = DroidLogicTvUtils.parseTvSourceTypeFromDeviceId(mDeviceId);
            mTvSourceInput = DroidLogicTvUtils.parseTvSourceInputFromDeviceId(mDeviceId);
            mVirtualTvSource = mTvSource;

            if (mTvSource == TvControlManager.SourceInput_Type.SOURCE_TYPE_ADTV) {
                Log.d(TAG, "channelId: " + mChannelId);
                currentChannel = mTvDataBaseManager.getChannelInfo(TvContract.buildChannelUri(mChannelId));
                if (currentChannel != null) {
                    mVideoStd = currentChannel.getVideoStd();
                    mTvSource = DroidLogicTvUtils.parseTvSourceTypeFromSigType(DroidLogicTvUtils.getSigType(currentChannel));
                    mTvSourceInput = DroidLogicTvUtils.parseTvSourceInputFromSigType(DroidLogicTvUtils.getSigType(currentChannel));
                    Log.d(TAG, "currentChannel != null");
                } else {
                    mVideoStd = -1;
                    Log.d(TAG, "currentChannel == null");
                }
                if (mVirtualTvSource == mTvSource) {//no channels in adtv input, DTV for default.
                    mTvSource = TvControlManager.SourceInput_Type.SOURCE_TYPE_DTV;
                    mTvSourceInput = TvControlManager.SourceInput.DTV;
                    Log.d(TAG, "no channels in adtv input, DTV for default.");
                }
            }
        }
        Log.d(TAG, "mDeviceId: " + mDeviceId);
    }

    static public boolean CanDebug() {
        return SystemProperties.getBoolean("sys.pqsetting.debug", false);
    }

    public static final int PIC_STANDARD = 0;
    public static final int PIC_VIVID = 1;
    public static final int PIC_SOFT = 2;
    public static final int PIC_USER = 3;
    public static final int PIC_MOVIE = 4;
    public static final int PIC_MONITOR = 6;
    public static final int PIC_GAME = 7;
    public static final int PIC_SPORT = 8;

    public String getPictureModeStatus () {
        int pictureModeIndex = mSystemControlManager.GetPQMode();
        if (CanDebug()) Log.d(TAG, "getPictureModeStatus : " + pictureModeIndex);
        switch (pictureModeIndex) {
            case PIC_STANDARD:
                return STATUS_STANDARD;
            case PIC_VIVID:
                return STATUS_VIVID;
            case PIC_SOFT:
                return STATUS_SOFT;
            case PIC_USER:
                return STATUS_USER;
            case PIC_MONITOR:
                return STATUS_MONITOR;
            case PIC_SPORT:
                return STATUS_SPORT;
            case PIC_MOVIE:
                return STATUS_MOVIE;
            case PIC_GAME:
                return STATUS_GAME;
            default:
                return STATUS_STANDARD;
        }
    }

    public String getChipVersionInfo () {
        String chipVersion = mSystemControlManager.getChipVersionInfo();
        if (CanDebug()) Log.d(TAG, "getChipVersionInfo: "+chipVersion);
        return chipVersion;
    }

    public int getBrightnessStatus () {
        int value = mSystemControlManager.GetBrightness();
        if (CanDebug()) Log.d(TAG, "getBrightnessStatus : " + value);
        return value;
    }

    public int getContrastStatus () {
        int value = mSystemControlManager.GetContrast();
        if (CanDebug()) Log.d(TAG, "getContrastStatus : " + value);
        return value;
    }

    public int getColorStatus () {
        int value = mSystemControlManager.GetSaturation();
        if (CanDebug()) Log.d(TAG, "getColorStatus : " + value);
        return value;
    }

    public int getSharpnessStatus () {
        int value = mSystemControlManager.GetSharpness();
        if (CanDebug()) Log.d(TAG, "getSharpnessStatus : " + value);
        return value;
    }

    public int getToneStatus () {
        int value = mSystemControlManager.GetHue();
        if (CanDebug()) Log.d(TAG, "getTintStatus : " + value);
        return value;
    }

    public int getPictureModeSource () {
        if (CanDebug()) Log.d(TAG, "getPictureModeSource");
        return mSystemControlManager.GetSourceHdrType();
    }

    public enum Aspect_Ratio_Mode {
        ASPEC_RATIO_AUTO(0),
        ASPEC_RATIO_43(1),
        ASPEC_RATIO_PANORAMA(2),
        ASPEC_RATIO_FULL_SCREEN(3),
        ASPEC_RATIO_DOT_BY_NOT(4);

        private int val;

        Aspect_Ratio_Mode(int val) {
            this.val = val;
        }

        public int toInt() {
            return this.val;
        }
    }

    public int getAspectRatioStatus () {
        int itemPosition = mSystemControlManager.GetDisplayMode(TvControlManager.SourceInput.XXXX.toInt());
        if (CanDebug()) Log.d(TAG, "getAspectRatioStatus:" + itemPosition);
        if (itemPosition == SystemControlManager.Display_Mode.DISPLAY_MODE_MODE43.toInt())
            return Aspect_Ratio_Mode.ASPEC_RATIO_43.toInt();
        else if (itemPosition == SystemControlManager.Display_Mode.DISPLAY_MODE_FULL.toInt())
            return Aspect_Ratio_Mode.ASPEC_RATIO_PANORAMA.toInt();
        else if (itemPosition == SystemControlManager.Display_Mode.DISPLAY_MODE_169.toInt())
            return Aspect_Ratio_Mode.ASPEC_RATIO_FULL_SCREEN.toInt();
        else if (itemPosition == SystemControlManager.Display_Mode. DISPLAY_MODE_NOSCALEUP.toInt())
            return Aspect_Ratio_Mode.ASPEC_RATIO_DOT_BY_NOT.toInt();
        else
            return Aspect_Ratio_Mode.ASPEC_RATIO_AUTO.toInt();
    }

    public int getAIPQStatus () {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAspectRatioStatus");
        boolean AIPQStatus = mSystemControlManager.getAipqEnable();
        return AIPQStatus ? 0: 1;//0 is on ,1 is off
    }

    public boolean getAisr() {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAisr");
        return mSystemControlManager.getAisr();
    }

    public int GetSourceHdrType () {
        if (CanDebug()) Log.d(TAG, "GetSourceHdrType");
        return mSystemControlManager.GetSourceHdrType();
    }

    public int getAdvancedDynamicToneMappingStatus () {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedDynamicToneMappingStatus");
        int dynamicToneMappingStatus = mSystemControlManager.GetHDRTMOMode();
        return dynamicToneMappingStatus != -1 ? dynamicToneMappingStatus: 0;//0 is on ,1 is off
    }

    public int getAdvancedColorManagementStatus () {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedColorManagementStatus");
        int colorManagementStatus = mSystemControlManager.GetColorBaseMode();
        return colorManagementStatus != -1 ? colorManagementStatus : 0;
    }

    public int getAdvancedColorSpaceStatus () {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedColorSpaceStatus");
        return 0;
    }

    public int getAdvancedGlobalDimmingStatus () {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedGlobalDimmingStatus");
        return mSystemControlManager.GetDynamicBacklight();
    }

    public int getAdvancedLocalDimmingStatus () {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedLocalDimmingStatus");
        return 0;
    }

    public int getAdvancedBlackStretchStatus () {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedBlackStretchStatus");
        return mSystemControlManager.GetBlackExtensionMode();
    }

    public int getAdvancedDNLPStatus () {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedDNLPStatus");
        int CurrentSourceInfo[] = mSystemControlManager.GetCurrentSourceInfo();
        int DNLPStatus = mSystemControlManager.getDNLPCurveParams(SystemControlManager.SourceInput.valueOf(CurrentSourceInfo[0]), SystemControlManager.SignalFmt.valueOf(CurrentSourceInfo[1]), SystemControlManager.TransFmt.valueOf(CurrentSourceInfo[2]));
        return DNLPStatus != -1? DNLPStatus :0;
    }

    public int getAdvancedLocalContrastStatus () {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedLocalContrastStatus");
        return mSystemControlManager.GetLocalContrastMode();
    }

    public int getAdvancedSRStatus () {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedSRStatus");
        return 0;
    }

    public int getAdvancedDeBlockStatus () {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedDeBlockStatus");
        int deBlockStatus = mSystemControlManager.GetDeblockMode();
        return deBlockStatus != -1? deBlockStatus :0;
    }

    public int getAdvancedDeMosquitoStatus () {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedDeMosquitoStatus");
        int deMosquitoStatus = mSystemControlManager.GetDemoSquitoMode();
        return deMosquitoStatus != -1? deMosquitoStatus :0;
    }

    public int getAdvancedDecontourStatus () {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedDecontourStatus");
        return mSystemControlManager.GetSmoothPlusMode();
    }

    public int getAdvancedMemcSwitchStatus () {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedMemcSwitchStatus");
        return 0;
    }

    public int getAdvancedMemcCustomizeDejudderStatus () {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedMemcCustomizeDejudderStatus");
        return 5;
    }

    public int getAdvancedMemcCustomizeDeblurStatus () {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedMemcCustomizeDeblurStatus");
        return 5;
    }

    public int getAdvancedGammaStatus () {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedGammaStatus");
        return mSystemControlManager.GetGammaValue() + ADVANCED_GAMMA_FIXED_DIFFERENCE;
    }

    public int getAdvancedManualGammaLevelStatus () {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedManualGammaLevelStatus");
        return 0;
    }

    public int getAdvancedManualGammaRGainStatus () {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedManualGammaRGainStatus");
        return 0;
    }

    public int getAdvancedManualGammaGGainStatus () {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedManualGammaGGainStatus");
        return 0;
    }

    public int getAdvancedManualGammaBGainStatus () {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedManualGammaBGainStatus");
        return 0;
    }

    public int getColorTemperatureStatus () {
        int itemPosition = mSystemControlManager.GetColorTemperature();
        if (CanDebug()) Log.d(TAG, "getColorTemperatureStatus : " + itemPosition);
        return itemPosition;
    }

    public int getAdvancedColorTemperatureRGainStatus () {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedColorTemperatureRGainStatus");
        return mSystemControlManager.GetColorTemperatureUserParam().r_gain;
    }

    public int getAdvancedColorTemperatureGGainStatus () {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedColorTemperatureGGainStatus");
        return mSystemControlManager.GetColorTemperatureUserParam().g_gain;
    }

    public int getAdvancedColorTemperatureBGainStatus () {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedColorTemperatureBGainStatus");
        return mSystemControlManager.GetColorTemperatureUserParam().b_gain;
    }

    public int getAdvancedColorTemperatureROffsetStatus () {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedColorTemperatureROffsetStatus");
        return mSystemControlManager.GetColorTemperatureUserParam().r_offset;
    }

    public int getAdvancedColorTemperatureGOffsetStatus () {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedColorTemperatureGOffsetStatus");
        return mSystemControlManager.GetColorTemperatureUserParam().g_offset;
    }

    public int getAdvancedColorTemperatureBOffsetStatus () {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedColorTemperatureBOffsetStatus");
        return mSystemControlManager.GetColorTemperatureUserParam().b_offset;
    }

    public int getDnrStatus () {
        int itemPosition = mSystemControlManager.GetNoiseReductionMode();
        if (CanDebug()) Log.d(TAG, "getDnrStatus : " + itemPosition);
        return itemPosition;
    }

    public int getAdvancedColorCustomizeCyanSaturationStatus() {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedColorCustomizeCyanSaturationStatus");
        return 0;
    }

    public int getAdvancedColorCustomizeCyanLumaStatus() {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedColorCustomizeCyanLumaStatus");
        return 0;
    }

    public int getAdvancedColorCustomizeCyanHueStatus() {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedColorCustomizeCyanHueStatus");
        return 0;
    }

    public int getAdvancedColorCustomizeBlueSaturationStatus() {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedColorCustomizeBlueSaturationStatus");
        return 0;
    }

    public int getAdvancedColorCustomizeBlueLumaStatus() {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedColorCustomizeBlueLumaStatus");
        return 0;
    }

    public int getAdvancedColorCustomizeBlueHueStatus() {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedColorCustomizeBlueHueStatus");
        return 0;
    }

    public int getAdvancedColorCustomizeBlueGreenSaturationStatus() {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedColorCustomizeBlueGreenSaturationStatus");
        return 0;
    }

    public int getAdvancedColorCustomizeBlueGreenLumaStatus() {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedColorCustomizeBlueGreenLumaStatus");
        return 0;
    }

    public int getAdvancedColorCustomizeBlueGreenHueStatus() {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedColorCustomizeBlueGreenHueStatus");
        return 0;
    }

    public int getAdvancedColorCustomizeGreenSaturationStatus() {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedColorCustomizeGreenSaturationStatus");
        return 0;
    }

    public int getAdvancedColorCustomizeGreenLumaStatus() {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedColorCustomizeGreenLumaStatus");
        return 0;
    }

    public int getAdvancedColorCustomizeGreenHueStatus() {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedColorCustomizeGreenHueStatus");
        return 0;
    }

    public int getAdvancedColorCustomizeMagentaSaturationStatus() {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedColorCustomizeMagentaSaturationStatus");
        return 0;
    }

    public int getAdvancedColorCustomizeMagentaLumaStatus() {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedColorCustomizeMagentaLumaStatus");
        return 0;
    }

    public int getAdvancedColorCustomizeMagentaHueStatus() {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedColorCustomizeMagentaHueStatus");
        return 0;
    }

    public int getAdvancedColorCustomizeRedSaturationStatus() {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedColorCustomizeRedSaturationStatus");
        return 0;
    }

    public int getAdvancedColorCustomizeRedLumaStatus() {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedColorCustomizeRedLumaStatus");
        return 0;
    }

    public int getAdvancedColorCustomizeRedHueStatus() {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedColorCustomizeRedHueStatus");
        return 0;
    }

    public int getAdvancedColorCustomizeSkinSaturationStatus() {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedColorCustomizeSkinSaturationStatus");
        return 0;
    }

    public int getAdvancedColorCustomizeSkinLumaStatus() {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedColorCustomizeSkinLumaStatus");
        return 0;
    }

    public int getAdvancedColorCustomizeSkinHueStatus() {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedColorCustomizeSkinHueStatus");
        return 0;
    }

    public int getAdvancedColorCustomizeYellowSaturationStatus() {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedColorCustomizeYellowSaturationStatus");
        return 0;
    }

    public int getAdvancedColorCustomizeYellowLumaStatus() {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedColorCustomizeYellowLumaStatus");
        return 0;
    }

    public int getAdvancedColorCustomizeYellowHueStatus() {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedColorCustomizeYellowHueStatus");
        return 0;
    }

    public int getAdvancedColorCustomizeYellowGreenSaturationStatus() {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedColorCustomizeYellowGreenSaturationStatus");
        return 0;
    }

    public int getAdvancedColorCustomizeYellowGreenLumaStatus() {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedColorCustomizeYellowGreenLumaStatus");
        return 0;
    }

    public int getAdvancedColorCustomizeYellowGreenHueStatus() {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedColorCustomizeYellowGreenHueStatus");
        return 0;
    }

    public void setPictureMode (String mode) {
        if (CanDebug()) Log.d(TAG, "setPictureMode : " + mode);
        if (mode.equals(STATUS_STANDARD)) {
            mSystemControlManager.SetPQMode(PIC_STANDARD, 1, 0);
        } else if (mode.equals(STATUS_VIVID)) {
            mSystemControlManager.SetPQMode(PIC_VIVID, 1, 0);
        } else if (mode.equals(STATUS_SOFT)) {
            mSystemControlManager.SetPQMode(PIC_SOFT, 1, 0);
        } else if (mode.equals(STATUS_USER)) {
            mSystemControlManager.SetPQMode(PIC_USER, 1, 0);
        } else if (mode.equals(STATUS_MONITOR)) {
            mSystemControlManager.SetPQMode(PIC_MONITOR, 1, 0);
        } else if (mode.equals(STATUS_SPORT)) {
            mSystemControlManager.SetPQMode(PIC_SPORT, 1, 0);
        } else if (mode.equals(STATUS_MOVIE)) {
            mSystemControlManager.SetPQMode(PIC_MOVIE, 1, 0);
        } else if (mode.equals(STATUS_GAME)) {
            mSystemControlManager.SetPQMode(PIC_GAME, 1, 0);
        }
    }

    public void setBrightness (int step) {
        if (CanDebug())  Log.d(TAG, "setBrightness step : " + step );
        int PQMode = mSystemControlManager.GetPQMode();
        if (PQMode == 3) {
            int tmp = mSystemControlManager.GetBrightness();
            mSystemControlManager.SetBrightness(tmp + step, 1);
        } else {
            mSystemControlManager.SetBrightness(setPictureUserMode(KEY_BRIGHTNESS) + step, 1);
        }
    }

    public void setContrast (int step) {
        if (CanDebug())  Log.d(TAG, "setContrast step : " + step);
        int PQMode = mSystemControlManager.GetPQMode();
        if (PQMode == 3) {
            int tmp = mSystemControlManager.GetContrast();
            mSystemControlManager.SetContrast(tmp + step, 1);
        } else {
            mSystemControlManager.SetContrast(setPictureUserMode(KEY_CONTRAST) + step, 1);
        }
    }

    public void setColor (int step) {
        if (CanDebug())  Log.d(TAG, "setColor step : " + step);
        int PQMode = mSystemControlManager.GetPQMode();
        if (PQMode == 3) {
            int tmp = mSystemControlManager.GetSaturation();
            mSystemControlManager.SetSaturation(tmp + step, 1);
        } else {
            mSystemControlManager.SetSaturation(setPictureUserMode(KEY_COLOR) + step, 1);
        }
    }

    public void setSharpness (int step) {
        if (CanDebug())  Log.d(TAG, "setSharpness step : " + step);
        int PQMode = mSystemControlManager.GetPQMode();
        if (PQMode == 3) {
            int tmp = mSystemControlManager.GetSharpness();
            mSystemControlManager.SetSharpness(tmp + step , 1 , 1);
        } else
            mSystemControlManager.SetSharpness(setPictureUserMode(KEY_SHARPNESS) + step, 1 , 1);
    }

    public void setTone(int step) {
        if (CanDebug())  Log.d(TAG, "setTint step : " + step);
        int PQMode = mSystemControlManager.GetPQMode();
        if (PQMode == 3) {
            int tmp = mSystemControlManager.GetHue();
            mSystemControlManager.SetHue(tmp + step, 1);
        } else {
            mSystemControlManager.SetHue(setPictureUserMode(KEY_TONE) + step, 1);
        }
    }

    public String getVideoStd () {
        if (mVideoStd != -1) {
            switch (mVideoStd) {
                case 1:
                    return mResources.getString(R.string.pal);
                case 2:
                    return mResources.getString(R.string.ntsc);
                default:
                    return mResources.getString(R.string.pal);
            }
        }
        return null;
    }

    public boolean isNtscSignalOrNot() {
        if (!SettingsConstant.needDroidlogicTvFeature(mContext)) {
            return false;
        }
        TvInSignalInfo info;
        String videoStd = getVideoStd();
        if (mTvSource == TvControlManager.SourceInput_Type.SOURCE_TYPE_TV
            && videoStd != null && videoStd.equals(mResources.getString(R.string.ntsc))) {
            if (mTvControlManager == null) {
                mTvControlManager = TvControlManager.getInstance();
            }
            info = mTvControlManager.GetCurrentSignalInfo();
            if (info.sigStatus == TvInSignalInfo.SignalStatus.TVIN_SIG_STATUS_STABLE) {
                if (CanDebug()) Log.d(TAG, "ATV NTSC mode signal is stable, show Tint");
                return true;
            }
        } else if (mTvSource == TvControlManager.SourceInput_Type.SOURCE_TYPE_AV) {
            if (mTvControlManager == null) {
                mTvControlManager = TvControlManager.getInstance();
            }
            info = mTvControlManager.GetCurrentSignalInfo();
            if (info.sigStatus == TvInSignalInfo.SignalStatus.TVIN_SIG_STATUS_STABLE) {
                String[] strings = info.sigFmt.toString().split("_");
                if (strings[4].contains("NTSC")) {
                    if (CanDebug()) Log.d(TAG, "AV NTSC mode signal is stable, show Tint");
                    return true;
                }
            }
        }
        return false;
    }

    public boolean isNtscSignal() {
        final int DEFAULT_VALUE = -1;
        final int NTSC = 2;
        return DataProviderManager.getIntValue(mContext, "curent_video_std", DEFAULT_VALUE) == NTSC;
    }

    public void setAspectRatio(int mode) {
        if (CanDebug()) Log.d(TAG, "setAspectRatio:" + mode);
        int source = TvControlManager.SourceInput.XXXX.toInt();
        if (mode == 0) {
            mSystemControlManager.SetDisplayMode(source, SystemControlManager.Display_Mode.DISPLAY_MODE_NORMAL, 1);
        } else if (mode == 1) {
            mSystemControlManager.SetDisplayMode(source, SystemControlManager.Display_Mode.DISPLAY_MODE_MODE43, 1);
        } else if (mode == 2) {
            mSystemControlManager.SetDisplayMode(source, SystemControlManager.Display_Mode.DISPLAY_MODE_FULL, 1);
        } else if (mode == 3) {
            mSystemControlManager.SetDisplayMode(source, SystemControlManager.Display_Mode.DISPLAY_MODE_169, 1);
        } else if (mode == 4) {
            mSystemControlManager.SetDisplayMode(source, SystemControlManager.Display_Mode.DISPLAY_MODE_NOSCALEUP, 1);
        }
    }

    public void setAIPQ(int mode) {
        if (CanDebug()) Log.d(TAG, "setAIPQ:" + mode);
        mSystemControlManager.setAipqEnable(mode == 0 ? true : false);
    }

    public boolean hasAisrFunc() {
        if (CanDebug()) Log.d(TAG, "hasAisrFunc");
        return mSystemControlManager.hasAisrFunc();
    }

    public boolean setAisr(boolean on) {
        if (CanDebug()) Log.d(TAG, "setAisr:" + on);
        return mSystemControlManager.aisrContrl(on);
    }

    public void setAdvancedDynamicToneMappingStatus (int value) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedDynamicToneMappingStatus value:"+value);
        mSystemControlManager.SetHDRTMOMode(value, 1);
    }

    public void setAdvancedColorManagementStatus (int value) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedColorManagementStatus value:"+value);
        switch (value) {
                case 0:
                    mSystemControlManager.SetColorBaseMode( SystemControlManager.ColorBaseMode.COLOR_BASE_MODE_OFF, 1);// off
                    break;
                case 1:
                    mSystemControlManager.SetColorBaseMode( SystemControlManager.ColorBaseMode. COLOR_BASE_MODE_OPTIMIZE, 1);// low
                    break;
                case 2:
                    mSystemControlManager.SetColorBaseMode( SystemControlManager.ColorBaseMode. COLOR_BASE_MODE_ENHANCE, 1);// middle
                    break;
                case 3:
                    mSystemControlManager.SetColorBaseMode( SystemControlManager.ColorBaseMode. COLOR_BASE_MODE_MAX, 1); // high
                    break;
                default:
                    mSystemControlManager.SetColorBaseMode( SystemControlManager.ColorBaseMode.COLOR_BASE_MODE_OFF, 1);// off
                    break;
        }
    }

    public void setAdvancedColorSpaceStatus (int value) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedColorSpaceStatus value:"+value);
        switch (value) {
                case 0:
                    // auto
                    break;
                case 1:
                     // srgb/rec.709
                    break;
                case 2:
                     // dci-p3
                    break;
                case 3:
                     // adobe rgb
                    break;
                case 4:
                     // bt.2020
                    break;
                default:
                    // auto
                    break;
        }
    }

    public void setAdvancedGlobalDimmingStatus (int value) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedGlobalDimmingStatus value:"+value);
        mSystemControlManager.SetDynamicBacklight(SystemControlManager.Dynamic_Backlight_Mode.valueOf(value), 1);
    }

    public void setAdvancedLocalDimmingStatus (int value) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedLocalDimmingStatus value:"+value);
        switch (value) {
                case 0:
                    // off
                    break;
                case 1:
                     // low
                    break;
                case 2:
                     // middle
                    break;
                case 3:
                     // high
                    break;
                default:
                    // off
                    break;
        }
    }

    public void setAdvancedBlackStretchStatus (int value) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedBlackStretchStatus value:"+value);
        mSystemControlManager.SetBlackExtensionMode(SystemControlManager.Black_Extension_Mode.valueOf(value), 1);
    }

    public void setAdvancedDNLPStatus (int value) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedDNLPStatus value:"+value);
        int CurrentSourceInfo[] = mSystemControlManager.GetCurrentSourceInfo();
        mSystemControlManager.setDNLPCurveParams(SystemControlManager.SourceInput.valueOf(CurrentSourceInfo[0]), SystemControlManager.SignalFmt.valueOf(CurrentSourceInfo[1]), SystemControlManager.TransFmt.valueOf(CurrentSourceInfo[2]), value);
    }

    public void setAdvancedLocalContrastStatus (int value) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedLocalContrastStatus value:"+value);
        switch (value) {
                case 0:
                    mSystemControlManager.SetLocalContrastMode(SystemControlManager.Local_Contrast_Mode.LOCAL_CONTRAST_MODE_OFF,1);// off
                    break;
                case 1:
                     mSystemControlManager.SetLocalContrastMode(SystemControlManager.Local_Contrast_Mode.LOCAL_CONTRAST_MODE_LOW,1); // low
                    break;
                case 2:
                     mSystemControlManager.SetLocalContrastMode(SystemControlManager.Local_Contrast_Mode.LOCAL_CONTRAST_MODE_MID,1); // middle
                    break;
                case 3:
                     mSystemControlManager.SetLocalContrastMode(SystemControlManager.Local_Contrast_Mode.LOCAL_CONTRAST_MODE_HIGH,1); // high
                    break;
                default:
                     mSystemControlManager.SetLocalContrastMode(SystemControlManager.Local_Contrast_Mode.LOCAL_CONTRAST_MODE_OFF,1); // off
                    break;
        }
    }

    public void setAdvancedSRStatus (int value) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedSRStatus value:"+value);
        switch (value) {
                case 0:
                    // off
                    break;
                case 1:
                     // standard
                    break;
                case 2:
                     // enhance
                    break;
                default:
                    // off
                    break;
        }
    }

    public void setAdvancedDeBlockStatus (int value) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedDeBlockStatus value:"+value);
        mSystemControlManager.SetDeblockMode(SystemControlManager.Deblock_Mode.valueOf(value), 1);
    }

    public void setAdvancedDeMosquitoStatus (int value) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedDeMosquitoStatus value:"+value);
        mSystemControlManager.SetDemoSquitoMode(SystemControlManager.DemoSquito_Mode.valueOf(value), 1);
    }

    public void setAdvancedDecontourStatus (int value) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedDecontourStatus value:"+value);
        mSystemControlManager.SetSmoothPlusMode(value, 1);
    }

    public void setAdvancedMemcSwitchStatus (int value) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedGlobalDimmingStatus value:"+value);
        switch (value) {
                case 0:
                    // off
                    break;
                case 1:
                     // on
                    break;
                default:
                    // off
                    break;
        }
    }

    public void setAdvancedGammaStatus (int value) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedGammaStatus");
        mSystemControlManager.SetGammaValue(value - ADVANCED_GAMMA_FIXED_DIFFERENCE, 1);
    }

    public void setAdvancedManualGammaLevelStatus (int value) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedManualGammaLevelStatus");
    }

    public void setAdvancedManualGammaRGainStatus (int value) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedManualGammaRGainStatus");
    }

    public void setAdvancedManualGammaGGainStatus (int value) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedManualGammaGGainStatus");
    }

    public void setAdvancedManualGammaBGainStatus (int value) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedManualGammaBGainStatus");
    }

    public void setAdvancedMemcCustomizeDejudderStatus (int value) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "getAdvancedMemcSwitchStatus");
    }

    public void setAdvancedMemcCustomizeDeblurStatus (int value) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedMemcCustomizeDeblurStatus");
    }

    // 0 1 2 3 ~ standard warm1 cool warm2
    public void setColorTemperature(int mode) {
        if (CanDebug())  Log.d(TAG, "setColorTemperature : " + mode);
        mSystemControlManager.SetColorTemperature(mode, 1);
    }

    public void setAdvancedColorTemperatureRGainStatus (int value) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedColorTemperatureRGainStatus");
        int currentColorTemperatureType = mSystemControlManager.GetColorTemperature();
        switch (currentColorTemperatureType) {
                case 0:
                    mSystemControlManager.SetColorTemperatureUserParam(SystemControlManager.color_temperature.COLOR_TEMP_STANDARD, 1, SystemControlManager.rgb_type.R_GAIN, value);
                    break;
                case 1:
                    mSystemControlManager.SetColorTemperatureUserParam(SystemControlManager.color_temperature.COLOR_TEMP_WARM, 1, SystemControlManager.rgb_type.R_GAIN, value);
                    break;
                case 2:
                    mSystemControlManager.SetColorTemperatureUserParam(SystemControlManager.color_temperature. COLOR_TEMP_COLD, 1, SystemControlManager.rgb_type.R_GAIN, value);
                    break;
                case 3:
                    mSystemControlManager.SetColorTemperatureUserParam(SystemControlManager.color_temperature.COLOR_TEMP_USER, 1, SystemControlManager.rgb_type.R_GAIN, value);
                    break;
                default:
                    mSystemControlManager.SetColorTemperatureUserParam(SystemControlManager.color_temperature.COLOR_TEMP_STANDARD, 1, SystemControlManager.rgb_type.R_GAIN, value);
                    break;
        }
    }

    public void setAdvancedColorTemperatureGGainStatus (int value) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedColorTemperatureGGainStatus");
        int currentColorTemperatureType = mSystemControlManager.GetColorTemperature();
        switch (currentColorTemperatureType) {
                case 0:
                    mSystemControlManager.SetColorTemperatureUserParam(SystemControlManager.color_temperature.COLOR_TEMP_STANDARD, 1, SystemControlManager.rgb_type.G_GAIN, value);
                    break;
                case 1:
                    mSystemControlManager.SetColorTemperatureUserParam(SystemControlManager.color_temperature.COLOR_TEMP_WARM, 1, SystemControlManager.rgb_type.G_GAIN, value);
                    break;
                case 2:
                    mSystemControlManager.SetColorTemperatureUserParam(SystemControlManager.color_temperature. COLOR_TEMP_COLD, 1, SystemControlManager.rgb_type.G_GAIN, value);
                    break;
                case 3:
                    mSystemControlManager.SetColorTemperatureUserParam(SystemControlManager.color_temperature.COLOR_TEMP_USER, 1, SystemControlManager.rgb_type.G_GAIN, value);
                    break;
                default:
                    mSystemControlManager.SetColorTemperatureUserParam(SystemControlManager.color_temperature.COLOR_TEMP_STANDARD, 1, SystemControlManager.rgb_type.G_GAIN, value);
                    break;
        }
    }

    public void setAdvancedColorTemperatureBGainStatus (int value) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedColorTemperatureBGainStatus");
        int currentColorTemperatureType = mSystemControlManager.GetColorTemperature();
        switch (currentColorTemperatureType) {
                case 0:
                    mSystemControlManager.SetColorTemperatureUserParam(SystemControlManager.color_temperature.COLOR_TEMP_STANDARD, 1, SystemControlManager.rgb_type.B_GAIN, value);
                    break;
                case 1:
                    mSystemControlManager.SetColorTemperatureUserParam(SystemControlManager.color_temperature.COLOR_TEMP_WARM, 1, SystemControlManager.rgb_type.B_GAIN, value);
                    break;
                case 2:
                    mSystemControlManager.SetColorTemperatureUserParam(SystemControlManager.color_temperature. COLOR_TEMP_COLD, 1, SystemControlManager.rgb_type.B_GAIN, value);
                    break;
                case 3:
                    mSystemControlManager.SetColorTemperatureUserParam(SystemControlManager.color_temperature.COLOR_TEMP_USER, 1, SystemControlManager.rgb_type.B_GAIN, value);
                    break;
                default:
                    mSystemControlManager.SetColorTemperatureUserParam(SystemControlManager.color_temperature.COLOR_TEMP_STANDARD, 1, SystemControlManager.rgb_type.B_GAIN, value);
                    break;
        }
    }

    public void setAdvancedColorTemperatureROffsetStatus (int value) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedColorTemperatureROffsetStatus");
        int currentColorTemperatureType = mSystemControlManager.GetColorTemperature();
        switch (currentColorTemperatureType) {
                case 0:
                    mSystemControlManager.SetColorTemperatureUserParam(SystemControlManager.color_temperature.COLOR_TEMP_STANDARD, 1, SystemControlManager.rgb_type.R_POST_OFFSET, value);
                    break;
                case 1:
                    mSystemControlManager.SetColorTemperatureUserParam(SystemControlManager.color_temperature.COLOR_TEMP_WARM, 1, SystemControlManager.rgb_type.R_POST_OFFSET, value);
                    break;
                case 2:
                    mSystemControlManager.SetColorTemperatureUserParam(SystemControlManager.color_temperature. COLOR_TEMP_COLD, 1, SystemControlManager.rgb_type.R_POST_OFFSET, value);
                    break;
                case 3:
                    mSystemControlManager.SetColorTemperatureUserParam(SystemControlManager.color_temperature.COLOR_TEMP_USER, 1, SystemControlManager.rgb_type.R_POST_OFFSET, value);
                    break;
                default:
                    mSystemControlManager.SetColorTemperatureUserParam(SystemControlManager.color_temperature.COLOR_TEMP_STANDARD, 1, SystemControlManager.rgb_type.R_POST_OFFSET, value);
                    break;
        }
    }

    public void setAdvancedColorTemperatureGOffsetStatus (int value) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedColorTemperatureGOffsetStatus");
        int currentColorTemperatureType = mSystemControlManager.GetColorTemperature();
        switch (currentColorTemperatureType) {
                case 0:
                    mSystemControlManager.SetColorTemperatureUserParam(SystemControlManager.color_temperature.COLOR_TEMP_STANDARD, 1, SystemControlManager.rgb_type.G_POST_OFFSET, value);
                    break;
                case 1:
                    mSystemControlManager.SetColorTemperatureUserParam(SystemControlManager.color_temperature.COLOR_TEMP_WARM, 1, SystemControlManager.rgb_type.G_POST_OFFSET, value);
                    break;
                case 2:
                    mSystemControlManager.SetColorTemperatureUserParam(SystemControlManager.color_temperature. COLOR_TEMP_COLD, 1, SystemControlManager.rgb_type.G_POST_OFFSET, value);
                    break;
                case 3:
                    mSystemControlManager.SetColorTemperatureUserParam(SystemControlManager.color_temperature.COLOR_TEMP_USER, 1, SystemControlManager.rgb_type.G_POST_OFFSET, value);
                    break;
                default:
                    mSystemControlManager.SetColorTemperatureUserParam(SystemControlManager.color_temperature.COLOR_TEMP_STANDARD, 1, SystemControlManager.rgb_type.G_POST_OFFSET, value);
                    break;
        }
    }

    public void setAdvancedColorTemperatureBOffsetStatus (int value) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedColorTemperatureBOffsetStatus");
        int currentColorTemperatureType = mSystemControlManager.GetColorTemperature();
        switch (currentColorTemperatureType) {
                case 0:
                    mSystemControlManager.SetColorTemperatureUserParam(SystemControlManager.color_temperature.COLOR_TEMP_STANDARD, 1, SystemControlManager.rgb_type.B_POST_OFFSET, value);
                    break;
                case 1:
                    mSystemControlManager.SetColorTemperatureUserParam(SystemControlManager.color_temperature.COLOR_TEMP_WARM, 1, SystemControlManager.rgb_type.B_POST_OFFSET, value);
                    break;
                case 2:
                    mSystemControlManager.SetColorTemperatureUserParam(SystemControlManager.color_temperature. COLOR_TEMP_COLD, 1, SystemControlManager.rgb_type.B_POST_OFFSET, value);
                    break;
                case 3:
                    mSystemControlManager.SetColorTemperatureUserParam(SystemControlManager.color_temperature.COLOR_TEMP_USER, 1, SystemControlManager.rgb_type.B_POST_OFFSET, value);
                    break;
                default:
                    mSystemControlManager.SetColorTemperatureUserParam(SystemControlManager.color_temperature.COLOR_TEMP_STANDARD, 1, SystemControlManager.rgb_type.B_POST_OFFSET, value);
                    break;
        }
    }

    //0 1 2 3 4 ~ off low medium high auto
    public void setDnr (int mode) {
        if (CanDebug()) Log.d(TAG, "setDnr : "+ mode);
        mSystemControlManager.SetNoiseReductionMode(mode, 1);
    }

    public void setAdvancedColorCustomizeBlueSaturationStatus(int step) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedManualGammaBGainStatus");
    }

    public void setAdvancedColorCustomizeBlueLumaStatus(int step) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedManualGammaBGainStatus");
    }

    public void setAdvancedColorCustomizeBlueHueStatus(int step) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedManualGammaBGainStatus");
    }

    public void setAdvancedColorCustomizeBlueGreenSaturationStatus(int step) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedManualGammaBGainStatus");
    }

    public void setAdvancedColorCustomizeBlueGreenLumaStatus(int step) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedManualGammaBGainStatus");
    }

    public void setAdvancedColorCustomizeBlueGreenHueStatus(int step) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedManualGammaBGainStatus");
    }

    public void setAdvancedColorCustomizeCyanSaturationStatus(int step) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedManualGammaBGainStatus");
    }

    public void setAdvancedColorCustomizeCyanLumaStatus(int step) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedManualGammaBGainStatus");
    }

    public void setAdvancedColorCustomizeCyanHueStatus(int step) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedManualGammaBGainStatus");
    }

    public void setAdvancedColorCustomizeGreenSaturationStatus(int step) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedManualGammaBGainStatus");
    }

    public void setAdvancedColorCustomizeGreenLumaStatus(int step) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedManualGammaBGainStatus");
    }

    public void setAdvancedColorCustomizeGreenHueStatus(int step) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedManualGammaBGainStatus");
    }

    public void setAdvancedColorCustomizeMagentaSaturationStatus(int step) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedManualGammaBGainStatus");
    }

    public void setAdvancedColorCustomizeMagentaLumaStatus(int step) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedManualGammaBGainStatus");
    }

    public void setAdvancedColorCustomizeMagentaHueStatus(int step) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedManualGammaBGainStatus");
    }

    public void setAdvancedColorCustomizeRedSaturationStatus(int step) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedManualGammaBGainStatus");
    }

    public void setAdvancedColorCustomizeRedLumaStatus(int step) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedManualGammaBGainStatus");
    }

    public void setAdvancedColorCustomizeRedHueStatus(int step) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedManualGammaBGainStatus");
    }

    public void setAdvancedColorCustomizeSkinSaturationStatus(int step) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedManualGammaBGainStatus");
    }

    public void setAdvancedColorCustomizeSkinLumaStatus(int step) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedManualGammaBGainStatus");
    }

    public void setAdvancedColorCustomizeSkinHueStatus(int step) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedManualGammaBGainStatus");
    }

    public void setAdvancedColorCustomizeYellowSaturationStatus(int step) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedManualGammaBGainStatus");
    }

    public void setAdvancedColorCustomizeYellowLumaStatus(int step) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedManualGammaBGainStatus");
    }

    public void setAdvancedColorCustomizeYellowHueStatus(int step) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedManualGammaBGainStatus");
    }

    public void setAdvancedColorCustomizeYellowGreenSaturationStatus(int step) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedManualGammaBGainStatus");
    }

    public void setAdvancedColorCustomizeYellowGreenLumaStatus(int step) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedManualGammaBGainStatus");
    }

    public void setAdvancedColorCustomizeYellowGreenHueStatus(int step) {
        // Leave blank first, add later
        if (CanDebug()) Log.d(TAG, "setAdvancedManualGammaBGainStatus");
    }

    private int setPictureUserMode(String key) {
        if (CanDebug()) Log.d(TAG, "setPictureUserMode : "+ key);
        int brightness = mSystemControlManager.GetBrightness();
        int contrast = mSystemControlManager.GetContrast();
        int color = mSystemControlManager.GetSaturation();
        int sharpness = mSystemControlManager.GetSharpness();
        int tint = -1;
        tint = mSystemControlManager.GetHue();
        int ret = -1;

        switch (mSystemControlManager.GetPQMode()) {
            case PIC_STANDARD:
            case PIC_VIVID:
            case PIC_SOFT:
            case PIC_MONITOR:
            case PIC_SPORT:
            case PIC_MOVIE:
            case PIC_GAME:
                setPictureMode(STATUS_USER);
                break;
        }

        if (CanDebug()) Log.d(TAG, " brightness=" + brightness + " contrast=" + contrast + " color=" + color + " sharp=" + sharpness);
        if (!key.equals(KEY_BRIGHTNESS))
            mSystemControlManager.SetBrightness(brightness, 1);
        else
            ret = brightness;

        if (!key.equals(KEY_CONTRAST))
            mSystemControlManager.SetContrast(contrast, 1);
        else
            ret = contrast;

        if (!key.equals(KEY_COLOR))
            mSystemControlManager.SetSaturation(color, 1);
        else
            ret = color;

        if (!key.equals(KEY_SHARPNESS))
            mSystemControlManager.SetSharpness(sharpness, 1 , 1);
        else
            ret = sharpness;

        if (!key.equals(KEY_TONE))
            mSystemControlManager.SetHue(tint, 1);
        else
            ret = tint;
        return ret;
    }

    public void setBacklightValue (int value) {
        if (CanDebug()) Log.d(TAG, "setBacklightValue : "+ value);
        mSystemControlManager.SetBacklight(getBacklightStatus() + value, 1);
    }

    public int getBacklightStatus () {
        int value = mSystemControlManager.GetBacklight();
        if (CanDebug()) Log.d(TAG, "getBacklightStatus : " + value);
        return value;
    }

    public int SSMRecovery() {
        int value = mSystemControlManager.SSMRecovery();
        if (CanDebug()) Log.d(TAG, "SSMRecovery : " + value);
        return value;
    }

    private static final int AUTO_RANGE = 0;
    private static final int FULL_RANGE = 1;
    private static final int LIMIT_RANGE = 2;

    public void setHdmiColorRangeValue (int value) {
        if (CanDebug()) Log.d(TAG, "setHdmiColorRangeValue : "+ value);
        TvControlManager.HdmiColorRangeMode hdmicolor = TvControlManager.HdmiColorRangeMode.AUTO_RANGE;
        switch (value) {
            case AUTO_RANGE :
                hdmicolor = TvControlManager.HdmiColorRangeMode.AUTO_RANGE;
                break;
            case FULL_RANGE :
                hdmicolor = TvControlManager.HdmiColorRangeMode.FULL_RANGE;
                break;
            case LIMIT_RANGE :
                hdmicolor = TvControlManager.HdmiColorRangeMode.LIMIT_RANGE;
                break;
            default:
                hdmicolor = TvControlManager.HdmiColorRangeMode.AUTO_RANGE;
                break;
        }
        if (mTvControlManager == null) {
            mTvControlManager = TvControlManager.getInstance();
        }
        if (mTvControlManager != null) {
            mTvControlManager.SetHdmiColorRangeMode(hdmicolor);
        }
    }

    public int getHdmiColorRangeStatus () {
        int value = 0;
        if (mTvControlManager == null) {
            mTvControlManager = TvControlManager.getInstance();
        }
        if (mTvControlManager != null) {
            value = mTvControlManager.GetHdmiColorRangeMode();
        }
        Log.d(TAG, "getHdmiColorRangeStatus : " + value);
        return value;
    }

    public boolean isHdmiSource() {
        if (mTvSourceInput != null) {
            return mTvSource == TvControlManager.SourceInput_Type.SOURCE_TYPE_HDMI;
        }
        return false;
    }

    public boolean isAvSource() {
        if (mTvSourceInput != null) {
            return mTvSource == TvControlManager.SourceInput_Type.SOURCE_TYPE_AV;
        }
        return false;
    }
}
