/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.tvinput.settings;

import android.R.integer;
import android.app.ActivityManager;
import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.ComponentName;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
//import android.content.pm.IPackageDataObserver;
import android.content.res.Resources;
import android.text.TextUtils;
import android.util.Log;
import android.os.Bundle;
import android.os.SystemClock;
import android.os.SystemProperties;

import java.util.ArrayList;
import java.util.Locale;
import java.util.HashMap;
import java.lang.System;
import java.lang.reflect.Method;

import android.media.tv.TvInputInfo;
import com.droidlogic.app.SystemControlManager;

import android.media.tv.TvContract;
import android.media.tv.TvContract.Channels;
import com.droidlogic.app.tv.ChannelInfo;
import com.droidlogic.app.tv.TvDataBaseManager;
import com.droidlogic.app.tv.TvControlManager;
import com.droidlogic.app.tv.TvInSignalInfo;
import com.droidlogic.app.tv.DroidLogicTvUtils;
import com.droidlogic.app.tv.TvMultilingualText;
import com.droidlogic.app.tv.TvScanConfig;
import com.droidlogic.app.tv.TvMTSSetting;
import com.droidlogic.app.DataProviderManager;
import com.droidlogic.tvinput.R;

public class SettingsManager {
    public static final String TAG = "SettingsManager";

    public static final String KEY_PICTURE                          = "picture";
    public static final String KEY_PICTURE_MODE                     = "picture_mode";
    public static final String KEY_BRIGHTNESS                       = "brightness";
    public static final String KEY_CONTRAST                         = "contrast";
    public static final String KEY_COLOR                            = "color";
    public static final String KEY_SHARPNESS                        = "sharpness";
    public static final String KEY_BACKLIGHT                        = "backlight";
    public static final String KEY_TINT                             = "tint";
    public static final String KEY_COLOR_TEMPERATURE                = "color_temperature";
    public static final String KEY_ASPECT_RATIO                     = "aspect_ratio";
    public static final String KEY_DNR                              = "dnr";
    public static final String KEY_3D_SETTINGS                      = "settings_3d";

    public static final String KEY_SOUND                            = "sound";
    public static final String KEY_SOUND_MODE                       = "sound_mode";
    public static final String KEY_TREBLE                           = "treble";
    public static final String KEY_BASS                             = "bass";
    public static final String KEY_BALANCE                          = "balance";
    public static final String KEY_SPDIF                            = "spdif";
    public static final String KEY_VIRTUAL_SURROUND                 = "virtual_surround";
    public static final String KEY_SURROUND                         = "surround";
    public static final String KEY_DIALOG_CLARITY                   = "dialog_clarity";
    public static final String KEY_BASS_BOOST                       = "bass_boost";

    public static final String KEY_CHANNEL                          = "channel";
    public static final String KEY_AUIDO_TRACK                      = "audio_track";
    public static final String KEY_SOUND_CHANNEL                    = "sound_channel";
    public static final String KEY_CHANNEL_INFO                     = "channel_info";
    public static final String KEY_COLOR_SYSTEM                     = "color_system";
    public static final String KEY_SOUND_SYSTEM                     = "sound_system";
    public static final String KEY_VOLUME_COMPENSATE                = "volume_compensate";
    public static final String KEY_FINE_TUNE                        = "fine_tune";
    public static final String KEY_MANUAL_SEARCH                    = "manual_search";
    public static final String KEY_AUTO_SEARCH                      = "auto_search";
    public static final String KEY_CHANNEL_EDIT                     = "channel_edit";
    public static final String KEY_SWITCH_CHANNEL                   = "switch_channel";
    public static final String KEY_MTS                              ="mts";

    public static final String KEY_SETTINGS                         = "settings";
    public static final String KEY_DTV_TYPE                         = "dtv_type";
    public static final String KEY_SLEEP_TIMER                      = "sleep_timer";
    public static final String KEY_MENU_TIME                        = "menu_time";
    public static final String KEY_STARTUP_SETTING                  = "startup_setting";
    public static final String KEY_DYNAMIC_BACKLIGHT                = "dynamic_backlight";
    public static final String KEY_RESTORE_FACTORY                  = "restore_factory";
    public static final String KEY_DEFAULT_LANGUAGE                 = "default_language";
    public static final String KEY_SUBTITLE_SWITCH                  = "sub_switch";
    public static final String KEY_AD_SWITCH                        = "ad_switch";
    public static final String KEY_AD_MIX                           = "ad_mix_level";
    public static final String KEY_AD_LIST                          = "ad_list";
    public static final String KEY_HDMI20                           = "hdmi20";
    public static final String KEY_FBC_UPGRADE                      ="fbc_upgrade";

    public static final String STATUS_STANDARD                      = "standard";
    public static final String STATUS_VIVID                         = "vivid";
    public static final String STATUS_SOFT                          = "soft";
    public static final String STATUS_MONITOR                       = "monitor";
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

    public static final String STATUS_DEFAULT_PERCENT               = "50%";
    public static final double STATUS_DEFAUT_FREQUENCY              = 44250000;
    public static final int PERCENT_INCREASE                        = 1;
    public static final int PERCENT_DECREASE                        = -1;
    public static final int DEFAULT_SLEEP_TIMER                     = 0;
    public static final int DEFAULT_MENU_TIME                       = 10;
    public static final String LAUNCHER_NAME                        = "com.android.launcher";
    public static final String LAUNCHER_ACTIVITY                    = "com.android.launcher2.Launcher";
    public static final String TV_NAME                              = "com.droidlogic.tv";
    public static final String TV_ACTIVITY                          = "com.droidlogic.tv.DroidLogicTv";

    public static final String STRING_ICON                          = "icon";
    public static final String STRING_NAME                          = "name";
    public static final String STRING_STATUS                        = "status";
    public static final String STRING_PRIVATE                       = "private";

    public static final String ATSC_TV_SEARCH_SYS                   = "atsc_tv_search_sys";
    public static final String ATSC_TV_SEARCH_SOUND_SYS             = "atsc_tv_search_sound_sys";
    public static String currentTag = null;
    public static final int STREAM_MUSIC = 3;

    private Context mContext;
    private Resources mResources;
    private String mInputId;
    private TvControlManager.SourceInput_Type mVirtualTvSource;
    private TvControlManager.SourceInput_Type mTvSource;
    private TvControlManager.SourceInput mTvSourceInput;
    private ChannelInfo currentChannel;
    private TvControlManager mTvControlManager;
    private TvDataBaseManager mTvDataBaseManager;
    private SystemControlManager mSystemControlManager;
    private ArrayList<ChannelInfo> videoChannelList;
    private ArrayList<ChannelInfo> radioChannelList;
    private boolean isRadioChannel = false;
    private int mResult = DroidLogicTvUtils.RESULT_OK;

    public SettingsManager (Context context, Intent intent) {
        mContext = context;
        mTvDataBaseManager = new TvDataBaseManager(mContext);
        mSystemControlManager = SystemControlManager.getInstance();

        setCurrentChannelData(intent);

        mTvControlManager = TvControlManager.getInstance();
        mResources = mContext.getResources();
    }

    public void setCurrentChannelData(Intent intent) {
        mInputId = intent.getStringExtra(TvInputInfo.EXTRA_INPUT_ID);
        DroidLogicTvUtils.saveInputId(mContext, mInputId);
        isRadioChannel = intent.getBooleanExtra(DroidLogicTvUtils.EXTRA_IS_RADIO_CHANNEL, false);

        int deviceId = intent.getIntExtra(DroidLogicTvUtils.EXTRA_CHANNEL_DEVICE_ID, -1);
        if (deviceId == -1)//for TIF compatible
            deviceId = DroidLogicTvUtils.getHardwareDeviceId(mInputId);

        mTvSource = DroidLogicTvUtils.parseTvSourceTypeFromDeviceId(deviceId);
        mTvSourceInput = DroidLogicTvUtils.parseTvSourceInputFromDeviceId(deviceId);
        mVirtualTvSource = mTvSource;

        if (mTvSource == TvControlManager.SourceInput_Type.SOURCE_TYPE_ADTV) {
            long channelId = intent.getLongExtra(DroidLogicTvUtils.EXTRA_CHANNEL_NUMBER, -1);
            currentChannel = mTvDataBaseManager.getChannelInfo(TvContract.buildChannelUri(channelId));
            if (currentChannel != null) {
                mTvSource = DroidLogicTvUtils.parseTvSourceTypeFromSigType(DroidLogicTvUtils.getSigType(currentChannel));
                mTvSourceInput = DroidLogicTvUtils.parseTvSourceInputFromSigType(DroidLogicTvUtils.getSigType(currentChannel));
            }
            if (mVirtualTvSource == mTvSource) {//no channels in adtv input, DTV for default.
                mTvSource = TvControlManager.SourceInput_Type.SOURCE_TYPE_DTV;
                mTvSourceInput = TvControlManager.SourceInput.DTV;
            }
        }
        Log.d(TAG, "init SettingsManager curSource=" + mTvSource + " isRadio=" + isRadioChannel);
        Log.d(TAG, "curVirtualSource=" + mVirtualTvSource);

        if (mTvSource == TvControlManager.SourceInput_Type.SOURCE_TYPE_TV
            || mTvSource == TvControlManager.SourceInput_Type.SOURCE_TYPE_DTV) {
            long channelId = intent.getLongExtra(DroidLogicTvUtils.EXTRA_CHANNEL_NUMBER, -1);
            currentChannel = mTvDataBaseManager.getChannelInfo(TvContract.buildChannelUri(channelId));
            if (currentChannel != null) {
                Log.d(TAG, "current channel is: ");
                currentChannel.print();
            } else {
                Log.d(TAG, "current channel is null!!");
            }
        }
    }

    public void setTag (String tag) {
        currentTag = tag;
    }

    public String getTag () {
        return currentTag;
    }

    public TvControlManager.SourceInput_Type getCurentTvSource () {
        return mTvSource;
    }

    public TvControlManager.SourceInput_Type getCurentVirtualTvSource () {
        return mVirtualTvSource;
    }

    public void setActivityResult(int result) {
        mResult = result;
    }

    public int getActivityResult() {
        return mResult;
    }

   



    public String getDtvType() {
        int deviceId = DroidLogicTvUtils.getHardwareDeviceId(mInputId);
        String type = DataProviderManager.getStringValue(mContext, DroidLogicTvUtils.TV_KEY_DTV_TYPE, null);
        if (type != null) {
            return type;
        } else {
            if (deviceId == DroidLogicTvUtils.DEVICE_ID_ADTV ) {
                return TvContract.Channels.TYPE_ATSC_T;
            }else {
                return TvContract.Channels.TYPE_DTMB;
            }
        }
    }

    public int getTvSearchTypeSys() {
        String searchColorSystem = DroidLogicTvUtils.getTvSearchTypeSys(mContext);
        int colorSystem = TvControlManager.ATV_VIDEO_STD_AUTO;
        if (searchColorSystem.equals(TvScanConfig.TV_COLOR_SYS.get(TvScanConfig.TV_COLOR_SYS_AUTO_INDEX))) {
            colorSystem = TvControlManager.ATV_VIDEO_STD_AUTO;
        } else if (searchColorSystem.equals(TvScanConfig.TV_COLOR_SYS.get(TvScanConfig.TV_COLOR_SYS_PAL_INDEX))) {
            colorSystem = TvControlManager.ATV_VIDEO_STD_PAL;
        } else if (searchColorSystem.equals(TvScanConfig.TV_COLOR_SYS.get(TvScanConfig.TV_COLOR_SYS_NTSC_INDEX))) {
            colorSystem = TvControlManager.ATV_VIDEO_STD_NTSC;
        } else if (searchColorSystem.equals(TvScanConfig.TV_COLOR_SYS.get(TvScanConfig.TV_COLOR_SYS_SECAM_INDEX))) {
            colorSystem = TvControlManager.ATV_VIDEO_STD_SECAM;
        } else {
            Log.w(TAG, "unsupport color System: " + searchColorSystem + ", set default: AUTO");
        }
        return colorSystem;
    }

    public int getTvSearchSoundSys() {
        String searchAudioSystem = DroidLogicTvUtils.getTvSearchSoundSys(mContext);
        int audioSystem = TvControlManager.ATV_AUDIO_STD_AUTO;
        if (searchAudioSystem.equals(TvScanConfig.TV_SOUND_SYS.get(TvScanConfig.TV_SOUND_SYS_AUTO_INDEX))) {
            audioSystem = TvControlManager.ATV_AUDIO_STD_AUTO;
        } else if (searchAudioSystem.equals(TvScanConfig.TV_SOUND_SYS.get(TvScanConfig.TV_SOUND_SYS_DK_INDEX))) {
            audioSystem = TvControlManager.ATV_AUDIO_STD_DK;
        } else if (searchAudioSystem.equals(TvScanConfig.TV_SOUND_SYS.get(TvScanConfig.TV_SOUND_SYS_I_INDEX))) {
            audioSystem = TvControlManager.ATV_AUDIO_STD_I;
        } else if (searchAudioSystem.equals(TvScanConfig.TV_SOUND_SYS.get(TvScanConfig.TV_SOUND_SYS_BG_INDEX))) {
            audioSystem = TvControlManager.ATV_AUDIO_STD_BG;
        } else if (searchAudioSystem.equals(TvScanConfig.TV_SOUND_SYS.get(TvScanConfig.TV_SOUND_SYS_M_INDEX))) {
            audioSystem = TvControlManager.ATV_AUDIO_STD_M;
        } else if (searchAudioSystem.equals(TvScanConfig.TV_SOUND_SYS.get(TvScanConfig.TV_SOUND_SYS_L_INDEX))) {
            audioSystem = TvControlManager.ATV_AUDIO_STD_L;
        } else {
            Log.w(TAG, "unsupport audio System: " + searchAudioSystem + ", set default: AUTO");
        }
        return audioSystem;
    }

    public String getDefaultDtvType() {
        /*int deviceId = DroidLogicTvUtils.getHardwareDeviceId(mInputId);
        if (deviceId == DroidLogicTvUtils.DEVICE_ID_ADTV ) {
            return TvContract.Channels.TYPE_ATSC_T;
        }else {
            return TvContract.Channels.TYPE_DTMB;
        }*/
        return TvContract.Channels.TYPE_ATSC_T;
    }

    public String getDtvTypeStatus(String type) {
        String ret = "";
        if (TextUtils.equals(type, TvContract.Channels.TYPE_DTMB)) {
                ret = mResources.getString(R.string.dtmb);
        } else if (TextUtils.equals(type, TvContract.Channels.TYPE_DVB_C)) {
                ret = mResources.getString(R.string.dvbc);
        } else if (TextUtils.equals(type, TvContract.Channels.TYPE_DVB_T)) {
                ret = mResources.getString(R.string.dvbt);
        } else if (TextUtils.equals(type, TvContract.Channels.TYPE_DVB_S)) {
                ret = mResources.getString(R.string.dvbs);
        } else if (TextUtils.equals(type, TvContract.Channels.TYPE_DVB_C2)) {
                ret = mResources.getString(R.string.dvbc2);
        } else if (TextUtils.equals(type, TvContract.Channels.TYPE_DVB_T2)) {
                ret = mResources.getString(R.string.dvbt2);
        } else if (TextUtils.equals(type, TvContract.Channels.TYPE_DVB_S2)) {
                ret = mResources.getString(R.string.dvbs2);
        } else if (TextUtils.equals(type, TvContract.Channels.TYPE_ATSC_T)) {
                ret = mResources.getString(R.string.atsc_t);
        } else if (TextUtils.equals(type, TvContract.Channels.TYPE_ATSC_C)) {
                ret = mResources.getString(R.string.atsc_c);
        } else if (TextUtils.equals(type, TvContract.Channels.TYPE_ISDB_T)) {
                ret = mResources.getString(R.string.isdb_t);
        }
        return ret;
    }

    public void setDtvType(String type) {
        DataProviderManager.putStringValue(mContext, DroidLogicTvUtils.TV_KEY_DTV_TYPE, type);
    }

    public void setTvSearchTypeSys(int mode) {
        DataProviderManager.putIntValue(mContext, ATSC_TV_SEARCH_SYS, mode);
    }

    private String[] tvPackages = {
        "com.android.providers.tv",
    };

    public String getInputId() {
        return mInputId;
    }

    public void sendBroadcastToTvapp(String extra) {
        Intent intent = new Intent(DroidLogicTvUtils.ACTION_UPDATE_TV_PLAY);
        intent.putExtra("tv_play_extra", extra);
        mContext.sendBroadcast(intent);
    }
    public void sendBroadcastToTvapp(String action, Bundle bundle) {
        Intent intent = new Intent(action);
        intent.putExtra(DroidLogicTvUtils.EXTRA_MORE, bundle);
        mContext.sendBroadcast(intent);
    }

    public void startTvPlayAndSetSourceInput() {
            mTvControlManager.StartTv();
            int deviceId = DroidLogicTvUtils.getHardwareDeviceId(mInputId);
            //Log.e(TAG,"deviceId:"+deviceId);
            mTvControlManager.SetSourceInput(mTvSourceInput, DroidLogicTvUtils.parseTvSourceInputFromDeviceId(deviceId));
        }

    public void deleteChannels(String type) {
            mTvDataBaseManager.deleteChannels(mInputId, type);
    }

    public void deleteAtvOrDtvChannels(boolean isatv) {
        mTvDataBaseManager.deleteAtvOrDtvChannels(isatv);
    }

    public void deleteOtherTypeAtvOrDtvChannels(String type, boolean isatv) {
        mTvDataBaseManager.deleteOtherTypeAtvOrDtvChannels(type, isatv);
    }
}
