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
import android.hardware.display.DisplayManager;
import android.media.tv.TvContract;
import android.os.Handler;
import android.os.Message;
import android.os.SystemProperties;
import android.os.PowerManager;
import android.text.TextUtils;
import android.util.Log;
import android.provider.Settings;
import android.text.TextUtils;
import android.view.Display;
import com.droidlogic.app.AudioSettingManager;
import com.droidlogic.app.AudioSystemCmdManager;
import com.droidlogic.app.DroidLogicUtils;
import com.droidlogic.app.OutputModeManager;
import com.droidlogic.app.SystemControlEvent;
import com.droidlogic.app.SystemControlManager;
import android.content.pm.PackageManager;
import com.droidlogic.btpair.BluetoothAutoPairReceiver;

public class DroidlogicApplication extends Application {
    private static final String TAG = "DroidlogicApplication";
    private AudioSettingManager mAudioSettingManager;
    private SystemControlEvent mSystemControlEvent;
    private SystemControlManager mSystemControlManager;
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
        // Should not do in java
        //register system control callback
        mSystemControlEvent   = SystemControlEvent.getInstance(this);
        mSystemControlManager = SystemControlManager.getInstance();
        String isSupportDTVKIT = SystemControlManager.getInstance().getPropertyString("ro.vendor.platform.is.tv", "");

        if (isSupportDTVKIT.equals("1")) {

            mSystemControlManager.setListener(mSystemControlEvent);
        }

        // GTVS version default use earlysuspend wakelock
        if (isGtvsVersion() && SystemProperties.getBoolean("ro.vendor.platform.earlysuspend", true)) {
            PowerManager powerManager = (PowerManager) getSystemService(POWER_SERVICE);
            mWakeLock = powerManager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK,
                "EarlysuspendTag[ro.vendor.platform.earlysuspend]:"+this);
            mWakeLock.acquire();
            Log.d(TAG, "wakelocked");
        }
        DisableBtPairInstrumentation(this);
        mSystemControlEvent.SetActiveModeChangeListener(new SystemControlEvent.ActiveModeChangeListener() {
            @Override
            public void changeActiveMode(int width, int height, int framerate){
                Log.d(TAG,"changeActiveMode"+width);
                DroidlogicApplication.this.changeActiveModeInner(width,height,framerate);
            }
        });
    }

    private boolean isGtvsVersion() {
        return !TextUtils.isEmpty(SystemProperties.get("ro.com.google.gmsversion", ""));
    }

    private boolean isBootvideoStopped() {
        ContentProviderClient tvProvider = null;
        return (((SystemProperties.getInt("persist.vendor.media.bootvideo", 50)  > 100)
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
                        mSystemControlManager.setProperty("vendor.sys.display.boot_complete","1");
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

   private void syncDLG(int width, int height, int framerate) {
        String isSupportDTVKIT = SystemControlManager.getInstance().getPropertyString("ro.vendor.platform.is.tv", "");
        boolean tvflag = isSupportDTVKIT.equals("1");
        if (!tvflag) return;
        int uiWidth = 3840;
        int uiHeight = 2160;
        if (isUfr()) {
            uiWidth = 1920;
            uiHeight = 1080;
        }
        if (isModeAvailable(uiWidth,uiHeight, framerate)) {
            changeActiveModeInner(uiWidth, uiHeight, framerate);
        }
    }
    private boolean isModeAvailable(int width, int height, int framerate) {
        DisplayManager displaymanager = getApplicationContext().getSystemService(DisplayManager.class);
        Display defaultDisplay = displaymanager.getDisplay(Display.DEFAULT_DISPLAY);
        Display.Mode[] modes = defaultDisplay.getSupportedModes();
        for (Display.Mode m : modes) {
            if (((int) (m.getRefreshRate() * 100) == framerate) && m.getPhysicalWidth() == width
                && m.getPhysicalHeight() == height) {
                    return true;
                }
        }
        return false;
    }

    private boolean isUfr() {
        DisplayManager displaymanager = getApplicationContext().getSystemService(DisplayManager.class);
        Display defaultDisplay = displaymanager.getDisplay(Display.DEFAULT_DISPLAY);
        Display.Mode[] modes = defaultDisplay.getSupportedModes();
        for (Display.Mode m : modes) {
            if (m.getRefreshRate() >= 144) {
                    return true;
                }
        }
        return false;
    }

    private void changeActiveModeInner(int width, int height, int framerate) {
        DisplayManager displaymanager = getApplicationContext().getSystemService(DisplayManager.class);
        Display defaultDisplay = displaymanager.getDisplay(Display.DEFAULT_DISPLAY);
        Display.Mode[] modes = defaultDisplay.getSupportedModes();
        Log.d(TAG,"changeActiveModeInner "+width+"x"+height+" "+framerate+" fps");
        for (Display.Mode m : modes) {
            Log.d(TAG,"changeActiveModeInner enter "+m+" "+(m.getPhysicalHeight() == height)+"/"+((int) (m.getRefreshRate() * 100) == framerate));
            if (((int) (m.getRefreshRate() * 100) == framerate)) {
                displaymanager.clearGlobalUserPreferredDisplayMode();
                defaultDisplay.clearUserPreferredDisplayMode();
                Log.d(TAG,"changeActiveModeInner-->"+m);
                defaultDisplay.setUserPreferredDisplayMode(m);
                displaymanager.setGlobalUserPreferredDisplayMode(m);
                break;
            }
        }
    }


    private void initAudio () {
        startDroidLogicServices(AudioSystemCmdManager.SERVICE_PACKEGE_NANME, AudioSystemCmdManager.SERVICE_NANME);
        startDroidLogicServices("com.droidlogic", "com.droidlogic.audioservice.services.AudioEffectsService");
        mAudioSettingManager.registerSurroundObserver();
        mAudioSettingManager.initSystemAudioSetting();
        //set sound effect in com.droidlogic.tv.soundeffectsettings
        //set dolby DRC
        SystemControlManager mSystemControlManager = SystemControlManager.getInstance();
        final boolean isSupportDolby = mSystemControlManager.getPropertyBoolean("ro.vendor.platform.support.dolby", false);
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
        value = Settings.Global.getInt(this.getContentResolver(),
            OutputModeManager.DRC_MODE, OutputModeManager.IS_DRC_RF);
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

    private void DisableBtPairInstrumentation(Context context) {
        if (SystemProperties.get("sys.vendor.remote.type", "IR_NONE").contains("BT"))
            return;
        PackageManager pm = context.getPackageManager();
        ComponentName name = new ComponentName(context, BluetoothAutoPairReceiver.class);
        Log.i(TAG, "set BluetoothAutoPairReceiver disabled");
        pm.setComponentEnabledSetting(name, PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
        PackageManager.DONT_KILL_APP);
    }
}

