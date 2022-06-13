/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC NetflixService
 */

package com.droidlogic;

import android.app.ActivityManager;
import android.app.IActivityManager;
import android.app.IProcessObserver;
import android.app.Service;
import android.content.Context;
import android.content.BroadcastReceiver;
import android.content.Intent;
import android.content.IntentFilter;
import android.media.AudioManager;
import android.media.AudioFormat;
import android.net.Uri;
import android.os.IBinder;
import android.os.RemoteException;
import android.provider.Settings;
import android.text.TextUtils;
import android.util.Log;
import android.database.ContentObserver;

import org.json.JSONObject;

import android.os.Handler;

import java.io.File;
import java.util.List;
import java.util.Scanner;

import com.droidlogic.app.DroidLogicUtils;
import com.droidlogic.app.SystemControlManager;
import com.droidlogic.app.OutputModeManager;

public class NetflixService extends Service {
    private static final String TAG = "NetflixService";

    public static final String FEATURE_SOFTWARE_NETFLIX = "droidlogic.software.netflix";

    private static final String NETFLIX_PKG_NAME = "com.netflix.ninja";
    private static final String YOUTUBE_PKG_NAME = "com.google.android.youtube.tv";
    private static final String SYS_AUDIO_CAP = "/sys/class/amhdmitx/amhdmitx0/aud_cap";
    private static final String WAKEUP_REASON_DEVICE = "/sys/class/meson_pm/suspend_reason";
    private static final String WAKEUP_REASON_DEVICE_OTHER = "/sys/devices/platform/aml_pm/suspend_reason";
    private static final String NRDP_PLATFORM_CAP = "nrdp_platform_capabilities";
    private static final String NRDP_AUDIO_PLATFORM_CAP = "nrdp_audio_platform_capabilities";
    private static final String NRDP_AUDIO_PLATFORM_CAP_MS12 = "nrdp_audio_platform_capabilities_ms12";
    private static final String NRDP_PLATFORM_CONFIG_DIR = "/vendor/etc/";
    private static final String NRDP_EXTERNAL_SURROUND = "nrdp_external_surround_sound_enabled";
    private static final int WAKEUP_REASON_CUSTOM = 9;
    private static boolean atmosSupported = false;
    private static boolean doblySupported = false;

    private boolean mIsNetflixFg = false;
    private boolean mIsYoutubeFg = false;
    private boolean hasMS12 = false;
    private Context mContext;
    private SystemControlManager mSCM;
    private AudioManager mAudioManager;
    private SettingsObserver mSettingsObserver;
    private OutputModeManager mOutputModeManager = null;

    private final Object mLock = new Object();
    private IActivityManager mIActivityManager;
    private ProcessObserver mProcessObserver;

    private class SettingsObserver extends ContentObserver {
        public SettingsObserver(Handler handler) {
            super(handler);
        }

        @Override
        public void onChange(boolean selfChange, Uri uri) {
            int surround = mOutputModeManager.getDigitalAudioFormatOut();
            Log.i(TAG, "onChange surround: " + DroidLogicUtils.audioFormatOutputToString(surround));
            switch (surround) {
                case OutputModeManager.DIGITAL_AUDIO_FORMAT_AUTO:
                case OutputModeManager.DIGITAL_AUDIO_FORMAT_PASSTHROUGH:
                    Log.i(TAG, "onChange auto/passthrough ATMOS: " + atmosSupported);
                    setNrdpCapabilitiesIfNeed(NRDP_AUDIO_PLATFORM_CAP, true);
                    setAtmosEnabled(atmosSupported);
                    if (hasMS12) {
                        setUiAudioBufferDelayOffset(doblySupported);
                    }
                    break;
                case OutputModeManager.DIGITAL_AUDIO_FORMAT_MANUAL:
                    String subformat = Settings.Global.getString(mContext.getContentResolver(), OutputModeManager.DIGITAL_AUDIO_SUBFORMAT);
                    Log.i(TAG, "onChange manual subformat: " + subformat);
                    setAtmosEnabled(subformat.contains(AudioFormat.ENCODING_E_AC3_JOC + ""));
                    if (hasMS12) {
                        setUiAudioBufferDelayOffset(doblySupported);
                    }
                    break;
                case OutputModeManager.DIGITAL_AUDIO_FORMAT_PCM:
                    if (hasMS12) {
                        setUiAudioBufferDelayOffset(false);
                    }
                    break;
                default:
                    Log.d(TAG, "error surround format");
                    break;
            }
        }
    }

    private final BroadcastReceiver mHPReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            boolean isConnected = intent.getBooleanExtra("state", false);
            refreshAudioCapabilities(isConnected);
        }
    };

    @Override
    public void onCreate() {
        super.onCreate();
        mContext = this;
        mSCM = SystemControlManager.getInstance();
        mAudioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
        mOutputModeManager = OutputModeManager.getInstance(mContext);

        String buildDate = PlatformAPI.getStringProperty("ro.build.version.incremental", "");
        boolean needUpdate = !buildDate.equals(SettingsPref.getSavedBuildDate(mContext));
        hasMS12 = mOutputModeManager.isAudioSupportMs12System();
        setNrdpCapabilitiesIfNeed(NRDP_PLATFORM_CAP, needUpdate);
        setNrdpCapabilitiesIfNeed(NRDP_AUDIO_PLATFORM_CAP, needUpdate);
        if (needUpdate) {
            SettingsPref.setSavedBuildDate(mContext, buildDate);
        }

        IntentFilter filter = new IntentFilter("android.intent.action.HDMI_PLUGGED");
        registerReceiver(mHPReceiver, filter);
        refreshAudioCapabilities(true);

        mSettingsObserver = new SettingsObserver(new Handler());
        getContentResolver().registerContentObserver(Settings.Global.getUriFor(OutputModeManager.DIGITAL_AUDIO_FORMAT),
                false, mSettingsObserver);
        getContentResolver().registerContentObserver(Settings.Global.getUriFor(OutputModeManager.DIGITAL_AUDIO_SUBFORMAT),
                false, mSettingsObserver);

        startNetflixIfNeed();

        mProcessObserver = new ProcessObserver();
        mIActivityManager = ActivityManager.getService();
        try {
            mIActivityManager.registerProcessObserver(mProcessObserver);
        } catch (RemoteException e) {
            Log.e(TAG, "could not get IActivityManager");
        }
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        return super.onStartCommand(intent, flags, startId);
    }

    @Override
    public void onDestroy() {
        try {
            mIActivityManager.unregisterProcessObserver(mProcessObserver);
        } catch (RemoteException e) {
            Log.e(TAG, "Failed to unregister listeners", e);
        }
        super.onDestroy();
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    private void startNetflixIfNeed() {
        Scanner scanner = null;
        int reason = -1;
        boolean isSysExists = true;
        String wakeupSys = WAKEUP_REASON_DEVICE;

        if (new File(WAKEUP_REASON_DEVICE).exists()) {
            wakeupSys = WAKEUP_REASON_DEVICE;
        } else if (new File(WAKEUP_REASON_DEVICE_OTHER).exists()) {
            wakeupSys = WAKEUP_REASON_DEVICE_OTHER;
        } else {
            isSysExists = false;
        }

        if (isSysExists) {
            try {
                scanner = new Scanner(new File(wakeupSys));
                reason = scanner.nextInt();
                scanner.close();
            } catch (Exception e) {
                if (scanner != null)
                    scanner.close();
                e.printStackTrace();
                return;
            }

        }

        if (reason == WAKEUP_REASON_CUSTOM) {
            boolean isPowerOn = true;  //false for netflixButton, true for powerOnFromNetflixButton
            Intent i = new Intent("com.netflix.action.NETFLIX_KEY_START");
            i.setPackage("com.netflix.ninja");
            i.putExtra("power_on", isPowerOn);  //"power_on" Boolean Extra must be presented
            i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_REORDER_TO_FRONT);
            mContext.startActivity(i);
        }
    }

    private void setNrdpCapabilitiesIfNeed(String capName, boolean needUpdate) {
        String cap = Settings.Global.getString(getContentResolver(), capName);
        String capName_File = capName;
        Log.i(TAG, capName + ":\n" + cap);
        if (!needUpdate && !TextUtils.isEmpty(cap)) {
            return;
        }

        if (capName.startsWith(NRDP_AUDIO_PLATFORM_CAP) && hasMS12 &&
                mOutputModeManager.getDigitalAudioFormatOut() == OutputModeManager.DIGITAL_AUDIO_FORMAT_AUTO) {
            capName_File = NRDP_AUDIO_PLATFORM_CAP_MS12;
        }

        try {
            Scanner scanner = new Scanner(new File(NRDP_PLATFORM_CONFIG_DIR + capName_File + ".json"));
            StringBuilder sb = new StringBuilder();

            while (scanner.hasNextLine()) {
                sb.append(scanner.nextLine());
                sb.append('\n');
            }

            Settings.Global.putString(getContentResolver(), capName, sb.toString());
            scanner.close();
        } catch (java.io.FileNotFoundException e) {
            Log.d(TAG, e.getMessage());
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public boolean isVisibleApp(String pkgName) {
        ActivityManager am = (ActivityManager) mContext.getSystemService(Context.ACTIVITY_SERVICE);
        List<ActivityManager.RunningAppProcessInfo> infos = am.getRunningAppProcesses();

        for (int i = 0; i < infos.size(); i++) {
            ActivityManager.RunningAppProcessInfo info = infos.get(i);
            if (info.processName.contains(pkgName)) {
                Log.d(TAG, "processName:" + info.processName + ",importance:" + info.importance);
                return info.importance == ActivityManager.RunningAppProcessInfo.IMPORTANCE_FOREGROUND;
            }
        }

        return false;
    }

    private void refreshAudioCapabilities(boolean isHdmiPlugged) {
        boolean isTv = DroidLogicUtils.isTv();
        int surround = mOutputModeManager.getDigitalAudioFormatOut();
        Log.i(TAG, "onReceived HDMI_PLUGGED: " + isHdmiPlugged + ", isTv:" + isTv + ", surround:" +
                DroidLogicUtils.audioFormatOutputToString(surround));
        if (!isTv && (OutputModeManager.DIGITAL_AUDIO_FORMAT_MANUAL == surround)) {
            Log.i(TAG, "Set " + NRDP_EXTERNAL_SURROUND + " to " + (isHdmiPlugged ? 1 : 0));
            Settings.Global.putInt(mContext.getContentResolver(),
                    NRDP_EXTERNAL_SURROUND, isHdmiPlugged ? 1 : 0);
        }

        String audioSinkCap = mSCM.readSysFs(SYS_AUDIO_CAP);
        atmosSupported = audioSinkCap.contains("Dobly_Digital+/ATMOS");
        doblySupported = audioSinkCap.contains("Dobly_Digital");
        if (isHdmiPlugged && (OutputModeManager.DIGITAL_AUDIO_FORMAT_AUTO == surround
                || OutputModeManager.DIGITAL_AUDIO_FORMAT_PASSTHROUGH == surround)) {
            Log.i(TAG, "ATMOS: " + atmosSupported + ", audioSinkCap: " + audioSinkCap);
            setAtmosEnabled(atmosSupported);
            if (hasMS12) {
                setUiAudioBufferDelayOffset(doblySupported);
            }
        }
    }

    private void setAtmosEnabled(boolean enabled) {
        // Refer to /vendor/etc/nrdp_audio_platform_capabilities.json
        String audioCap = Settings.Global.getString(getContentResolver(), NRDP_AUDIO_PLATFORM_CAP);
        if (audioCap == null || !hasMS12 ||
                (mOutputModeManager.getDigitalAudioFormatOut() == OutputModeManager.DIGITAL_AUDIO_FORMAT_PASSTHROUGH))
            return;

        try {
            JSONObject rootObject = new JSONObject(audioCap);
            JSONObject audioCapsObject = rootObject.getJSONObject("audiocaps");
            JSONObject atmosObject = audioCapsObject.getJSONObject("atmos");

            boolean isEnabled = atmosObject.getBoolean("enabled");
            if (isEnabled ^ enabled) {
                Log.i(TAG, "set ATMOS support " + isEnabled + " -> " + enabled);
                atmosObject.put("enabled", enabled);
                Settings.Global.putString(getContentResolver(), NRDP_AUDIO_PLATFORM_CAP, rootObject.toString());
            }
        } catch (org.json.JSONException e) {
            e.printStackTrace();
        }
    }

    private void setUiAudioBufferDelayOffset(boolean enabled) {
        // Refer to /vendor/etc/nrdp_audio_platform_capabilities.json
        String audioCap = Settings.Global.getString(getContentResolver(), NRDP_AUDIO_PLATFORM_CAP);
        if (audioCap == null)
            return;

        try {
            JSONObject rootObject = new JSONObject(audioCap);
            JSONObject audioCapsObject = rootObject.getJSONObject("audiocaps");
            int uiOffset = audioCapsObject.getInt("uiAudioBufferDelayOffset");
            int setOffset = enabled ? 90 : 95;
            if (uiOffset != setOffset) {
                Log.i(TAG, "uiOffset from  " + uiOffset + "to " + setOffset);
                audioCapsObject.put("uiAudioBufferDelayOffset", setOffset);
                Settings.Global.putString(getContentResolver(), NRDP_AUDIO_PLATFORM_CAP, rootObject.toString());
            }
        } catch (org.json.JSONException e) {
            e.printStackTrace();
        }
    }

    private class ProcessObserver extends IProcessObserver.Stub {
        @Override
        public void onForegroundActivitiesChanged(int pid, int uid, boolean foregroundActivities) {
            Log.d(TAG, "onForegroundActivitiesChanged pid:" + pid + ",uid:" + uid + ",fg:" + foregroundActivities);
            synchronized (mLock) {
                boolean fg = isVisibleApp(NETFLIX_PKG_NAME);
                if (fg ^ mIsNetflixFg) {
                    Log.i(TAG, "Netflix status changed from " + (mIsNetflixFg ? "fg" : "bg") + " -> " + (fg ? "fg" : "bg"));
                    mIsNetflixFg = fg;

                    mAudioManager.setParameters("continuous_audio_mode=" + (fg ? "1" : "0"));
                    mSCM.setProperty("vendor.netflix.state", fg ? "fg" : "bg");
                }

                boolean fgYoutube = isVisibleApp(YOUTUBE_PKG_NAME);
                if (fgYoutube ^ mIsYoutubeFg) {
                    Log.i(TAG, "Youtube status changed from " + (mIsYoutubeFg ? "fg" : "bg") + " -> " + (fgYoutube ? "fg" : "bg"));
                    mIsYoutubeFg = fgYoutube;
                    mAudioManager.setParameters("compensate_video_enable=" + (fgYoutube ? "1" : "0"));
                }
            }
        }

        @Override
        public void onForegroundServicesChanged(int pid, int uid, int fgServiceTypes) {
            Log.d(TAG, "onForegroundServicesChanged pid:" + pid);
        }

        @Override
        public void onProcessDied(int pid, int uid) {
        }
    }
}

