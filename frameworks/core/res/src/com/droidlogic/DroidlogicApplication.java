/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC UsbCameraReceiver
 */

package com.droidlogic;

import android.app.Application;
import android.content.ComponentName;
import android.content.Intent;
import android.content.ContentProviderClient;
import android.content.Context;

import android.media.tv.TvContract;
import android.os.Handler;
import android.os.Message;
import android.os.SystemProperties;
import android.os.PowerManager;
import android.text.TextUtils;
import android.util.Log;
import android.provider.Settings;
import android.text.TextUtils;

import com.droidlogic.app.AudioSettingManager;
import com.droidlogic.app.AudioSystemCmdManager;
import com.droidlogic.app.DroidLogicUtils;
import com.droidlogic.app.tv.AudioEffectManager;
import com.droidlogic.app.OutputModeManager;
import com.droidlogic.app.SystemControlManager;

public class DroidlogicApplication extends Application {
    private static final String TAG = "DroidlogicApplication";
    private AudioSettingManager mAudioSettingManager;
    private PowerManager.WakeLock mWakeLock;
    public static final String DRC_OFF = "off";
    public static final String DRC_LINE = "line";
    public static final String DRC_RF = "rf";

    @Override
    public void onCreate() {
        super.onCreate();
        Log.d(TAG, "onCreate");
        mAudioSettingManager = new AudioSettingManager(this);
        mHandler.sendEmptyMessage(MSG_CHECK_BOOTVIDEO_FINISHED);

        // GTVS version default use earlysuspend wakelock
        if (isGtvsVersion() && SystemProperties.getBoolean("ro.vendor.platform.earlysuspend", true)) {
            PowerManager powerManager = (PowerManager) getSystemService(POWER_SERVICE);
            mWakeLock = powerManager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK,
                "EarlysuspendTag[ro.vendor.platform.earlysuspend]:"+this);
            mWakeLock.acquire();
            Log.d(TAG, "wakelocked");
        }
    }

    private boolean isGtvsVersion() {
        return !TextUtils.isEmpty(SystemProperties.get("ro.com.google.gmsversion", ""));
    }

    private boolean isBootvideoStopped() {
        ContentProviderClient tvProvider = null;

        if (mAudioSettingManager.isTunerAudio()) {
            tvProvider = getContentResolver().acquireContentProviderClient(TvContract.AUTHORITY);
        }

        return (mAudioSettingManager.isTunerAudio() && tvProvider != null || !mAudioSettingManager.isTunerAudio()) &&
                (((SystemProperties.getInt("persist.vendor.media.bootvideo", 50)  > 100)
                        && TextUtils.equals(SystemProperties.get("service.bootvideo.exit", "1"), "0"))
                || ((SystemProperties.getInt("persist.vendor.media.bootvideo", 50)  <= 100)));
    }

    private static final int MSG_CHECK_BOOTVIDEO_FINISHED = 0;
    private Handler mHandler = new Handler() {
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_CHECK_BOOTVIDEO_FINISHED:
                    if (isBootvideoStopped()) {
                        Log.d(TAG, "bootvideo stopped, start initializing audio");
                        initAudio();
                    } else {
                        if (DroidLogicUtils.getAudioDebugEnable()) {
                            Log.d(TAG, "handleMessage sendEmptyMessageDelayed MSG_CHECK_BOOTVIDEO_FINISHED");
                        }
                        mHandler.sendEmptyMessageDelayed(MSG_CHECK_BOOTVIDEO_FINISHED, 10);
                    }
                    break;
                default:
                    break;
            }
        }
    };

    private void initAudio () {
        startDroidLogicServices(AudioSystemCmdManager.SERVICE_PACKEGE_NANME, AudioSystemCmdManager.SERVICE_NANME);
        startDroidLogicServices(AudioEffectManager.SERVICE_PACKEGE_NANME, AudioEffectManager.SERVICE_NANME);
        mAudioSettingManager.registerSurroundObserver();
        mAudioSettingManager.initSystemAudioSetting();
        //set sound effect in com.droidlogic.tv.soundeffectsettings
        //set dolby DRC
        SystemControlManager mSystenControlManager = SystemControlManager.getInstance();
        final boolean isSupportDolby = mSystenControlManager.getPropertyBoolean("ro.vendor.platform.support.dolby", false);
        if (isSupportDolby) {
            setDoblyMode(this);
        }
    }
    private void setDoblyMode(final Context context) {
         new Thread(new Runnable() {
             @Override
             public void run() {
                 OutputModeManager mOutputModeManager = new OutputModeManager(context);
                 String selection  = getDrcModePassthroughSetting();
                 Log.i(TAG, "setDoblyMode selection  " + selection);
                 if (null != mOutputModeManager) {
                     switch (selection) {
                     case DRC_OFF:
                         mOutputModeManager.enableDobly_DRC(false);
                         mOutputModeManager.setDoblyMode(OutputModeManager.LINE_DRCMODE);
                         setDrcModePassthroughSetting(OutputModeManager.IS_DRC_OFF);
                         break;
                     case DRC_LINE:
                         mOutputModeManager.enableDobly_DRC(true);
                         mOutputModeManager.setDoblyMode(OutputModeManager.LINE_DRCMODE);
                         setDrcModePassthroughSetting(OutputModeManager.IS_DRC_LINE);
                         break;
                     case DRC_RF:
                         mOutputModeManager.enableDobly_DRC(false);
                         mOutputModeManager.setDoblyMode(OutputModeManager.RF_DRCMODE);
                         setDrcModePassthroughSetting(OutputModeManager.IS_DRC_RF);
                         break;
                     default:
                         throw new IllegalArgumentException("Unknown drc mode pref value");
                     }
                 } else {
                     Log.e(TAG, "setDoblyMode mOutputModeManager is null");
                 }

             }
         }).start();
     }
    public void setDrcModePassthroughSetting(int newVal) {
        Settings.Global.putInt(this.getContentResolver(),
                OutputModeManager.DRC_MODE, newVal);
    }
    public String getDrcModePassthroughSetting() {
    String isSupportDTVKIT = SystemControlManager.getInstance().getPropertyString("ro.vendor.platform.is.tv", "");
    boolean tvflag = isSupportDTVKIT.equals("1");

        int value;
        if (tvflag) {
            value = Settings.Global.getInt(this.getContentResolver(),
                OutputModeManager.DRC_MODE, OutputModeManager.IS_DRC_RF);
        } else {
            value = Settings.Global.getInt(this.getContentResolver(),
                OutputModeManager.DRC_MODE, OutputModeManager.IS_DRC_LINE);
        }

        switch (value) {
        case OutputModeManager.IS_DRC_OFF:
            return DRC_OFF;
        case OutputModeManager.IS_DRC_LINE:
        default:
            return DRC_LINE;
        case OutputModeManager.IS_DRC_RF:
            return DRC_RF;
        }
    }

    private void startDroidLogicServices (String packageName, String name) {
        Intent intent = new Intent();
        intent.setComponent(new ComponentName(packageName, name));
        intent.setAction(name + ".STARTUP");
        startService(intent);
        Log.i(TAG, "startDroidLogicServices startup service:" + name);
    }
}

