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
import android.app.TaskStackListener;
import android.app.ActivityTaskManager.RootTaskInfo;

import android.app.Service;
import android.hardware.hdmi.HdmiControlManager;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.ComponentName;
import android.content.Context;
import android.content.BroadcastReceiver;
import android.content.Intent;
import android.content.IntentFilter;
import android.media.AudioManager;
import android.media.AudioFormat;
import android.net.Uri;
import android.os.IBinder;
import android.os.RemoteException;
import android.os.Handler;
import android.os.Message;


import android.provider.Settings;
import android.text.TextUtils;
import android.util.Log;
import android.database.ContentObserver;
import android.content.ContentResolver;
import android.provider.DeviceConfig;
import org.json.JSONObject;
import android.hardware.display.DisplayManager;
import android.hardware.display.HdrConversionMode;

import android.view.Display;
import android.os.Handler;

import java.io.File;
import java.lang.NumberFormatException;
import java.lang.StringBuffer;
import java.io.FileInputStream;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Scanner;
import android.os.SystemProperties;
import android.os.HandlerExecutor;



import com.droidlogic.app.DroidLogicUtils;
import com.droidlogic.app.SystemControlManager;
import com.droidlogic.app.OutputModeManager;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class NetflixService extends Service {
    private static final String TAG = "NetflixService";

    public static final String FEATURE_SOFTWARE_NETFLIX = "droidlogic.software.netflix";

    private static final String NETFLIX_PKG_NAME = "com.netflix.ninja";
    private static final String YOUTUBE_PKG_NAME = "com.google.android.youtube.tv";
    private static final String LAUNCHER_PKG_NAME = "com.google.android.apps.tv.launcherx";
    private static final String SYS_AUDIO_CAP = "/sys/class/amhdmitx/amhdmitx0/aud_cap";
    private static final String WAKEUP_REASON_DEVICE = "/sys/class/meson_pm/suspend_reason";
    private static final String WAKEUP_REASON_DEVICE_OTHER = "/sys/devices/platform/aml_pm/suspend_reason";
    private static final String NRDP_PLATFORM_CAP = "nrdp_platform_capabilities";
    private static final String NRDP_AUDIO_PLATFORM_CAP = "nrdp_audio_platform_capabilities";
    private static final String NRDP_AUDIO_PLATFORM_CAP_MS12 = "nrdp_audio_platform_capabilities_ms12";
    private static final String NRDP_PLATFORM_CONFIG_DIR = "/vendor/etc/";
    private static final String DOLBY_LIB = "dolby_lib";
    private static final String NETFLIX_KEY_POWER_MODE = "power_on";
    private static final String ACTION_LAUNCH_APP = "com.google.global_button.ACTION_LAUNCH_APP";
    private static final String ACTION_LAUNCH_BENCH_APP = "com.amlogic.ACTION_LAUNCH_BENCH_APP";
    private static final String EXTRA_PACKAGE_NAME = "launchPackageName";
    private static final String EXTRA_LAUNCH_INTENT = "launchIntent";
    private static final String NETFLIX_INTENT = "com.netflix.action.NETFLIX_KEY_START";
     // Power State Change on Active Source Lost Settings values
    private static final String LOST_NONE = "none";
    private static final String LOST_STANDBY_NOW = "standby_now";
    private static final String TEMP_HDR = "temp_hdr";
    private final String NDRP_CEC_STATUS = "nrdp_video_platform_capabilities";

    private static final String STR_ALWAYS = "0";
    private static final String STR_ADAPTIVE = "1";
    private static final int WAKEUP_REASON_CUSTOM = 9;
    private static final int MSG_UPDATA = 1;
    private static final int MSG_UPDATA_DISPLAY = 2;
    private static final int UI_AUDIO_DELAY_OFFSET_TV_NON_DOLBY = 60;
    private static final int UI_AUDIO_DELAY_OFFSET_TV_MS12 = 110;
    private static final int UI_AUDIO_DELAY_OFFSET_OTT_DOLBY = 70;
    private static final int UI_AUDIO_DELAY_OFFSET_OTT_PCM = 75;
    private static boolean atmosSupported = false;
    private static boolean atmosSupportedByConfig = false;
    private static boolean doblySupported = false;
    private boolean mIsNetflixFg = false;
    private boolean mIsYoutubeFg = false;
    private boolean hasMS12 = false;
    private boolean tempHDR = false;
    private Context mContext;
    private SystemControlManager mSCM;
    private AudioManager mAudioManager;
    private HdmiControlManager mHdmiControlManager;
    private DisplayManager mDisplayManager;
    private SettingsObserver mSettingsObserver;
    private CecStatusObserver mCecStatusObserver;
    private OutputModeManager mOutputModeManager = null;
    private final Object mLock = new Object();
    private IActivityManager mIActivityManager;
    private ProcessObserver mProcessObserver;
    private DeviceConfigListener mDeviceConfigListener = null;
    private  Handler mMsgHandler;
    private String mOriginalPowerStateChangeValue;
    private HdrConversionMode mHdrConversionMode;


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

    private class CecStatusObserver extends ContentObserver {
        public CecStatusObserver(Handler handler) {
            super(handler);
        }

        @Override
        public void onChange(boolean selfChange, Uri uri, int flags) {
            notifyChange("nrdp_video_platform_capabilities/activeCecState");
        }

        private void notifyChange(String settingsNote) {
            ContentResolver cr = mContext.getContentResolver();
            cr.notifyChange(Settings.Global.getUriFor(settingsNote), null,
                ContentResolver.NOTIFY_NO_DELAY);
            Log.i(TAG,"notify activeness changes without delay");
        }
    }

    private final class DeviceConfigListener implements DeviceConfig.OnPropertiesChangedListener {
        private static final String KEY_LIGHT_IDLE_AFTER_INACTIVE_TIMEOUT = "light_after_inactive_to";
        private static final String LIGHT_IDLE_AFTER_INACTIVE_TIMEOUT_VALUE = "3600000";

        public DeviceConfigListener() {
            Log.d(TAG, "DeviceConfigListener");
            DeviceConfig.addOnPropertiesChangedListener(DeviceConfig.NAMESPACE_DEVICE_IDLE, new HandlerExecutor(new Handler()), this);
            setDefaultVal();
        }

        public void onPropertiesChanged(DeviceConfig.Properties properties) {
            for (String name : properties.getKeyset()) {
                if (name == null) {
                    continue;
                }

                switch (name) {
                case KEY_LIGHT_IDLE_AFTER_INACTIVE_TIMEOUT:

                    long light_after_inactive_to = properties.getLong(KEY_LIGHT_IDLE_AFTER_INACTIVE_TIMEOUT, 60000);

                    if (light_after_inactive_to != Long.parseLong(LIGHT_IDLE_AFTER_INACTIVE_TIMEOUT_VALUE)) {
                        Log.d(TAG, "DEVICE_IDLE changed light_after_inactive_to = " + light_after_inactive_to);
                        setDefaultVal();
                    }

                    break;
                }
            }
        }

        private void setDefaultVal() {
            boolean deviceConfigSetBoolean = DeviceConfig.setProperty(DeviceConfig.NAMESPACE_DEVICE_IDLE,
                    KEY_LIGHT_IDLE_AFTER_INACTIVE_TIMEOUT,
                    LIGHT_IDLE_AFTER_INACTIVE_TIMEOUT_VALUE, false);
            Log.d(TAG,
                "set DEVICE_IDLE light_after_inactive_to = " + LIGHT_IDLE_AFTER_INACTIVE_TIMEOUT_VALUE + ":" + deviceConfigSetBoolean);
        }
    }

    private BroadcastReceiver mHPReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            boolean isConnected = intent.getBooleanExtra("state", false);
            refreshAudioCapabilities(isConnected);
            if (isConnected) {
                mMsgHandler.sendEmptyMessageDelayed(MSG_UPDATA_DISPLAY,2000);
            }
        }
    };

    @Override
    public void onCreate() {
        super.onCreate();
        mContext = this;
        mSCM = SystemControlManager.getInstance();
        mAudioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
        mOutputModeManager = OutputModeManager.getInstance(mContext);
        mHdmiControlManager = (HdmiControlManager)mContext.getSystemService(Context.HDMI_CONTROL_SERVICE);
        mDisplayManager = (DisplayManager)getSystemService(DisplayManager.class);

        hasMS12 = mOutputModeManager.isAudioSupportMs12System();
        initNrdpCapabilities();
        atmosSupportedByConfig = isAtmosConfiged();
        Log.d(TAG, "atmosSupportedByConfig = " + atmosSupportedByConfig);

        IntentFilter filter = new IntentFilter("android.intent.action.HDMI_PLUGGED");
        filter.addAction(ACTION_LAUNCH_BENCH_APP);
        registerReceiver(mHPReceiver, filter, mContext.RECEIVER_EXPORTED);
        refreshAudioCapabilities(true);

        mSettingsObserver = new SettingsObserver(new Handler());
        getContentResolver().registerContentObserver(Settings.Global.getUriFor(OutputModeManager.DIGITAL_AUDIO_FORMAT),
                false, mSettingsObserver);
        getContentResolver().registerContentObserver(Settings.Global.getUriFor(OutputModeManager.DIGITAL_AUDIO_SUBFORMAT),
                false, mSettingsObserver);
        mCecStatusObserver = new CecStatusObserver(new Handler());
        getContentResolver().registerContentObserver(Settings.Global.getUriFor(NDRP_CEC_STATUS),
                false, mCecStatusObserver);

        startNetflixIfNeed();

        mDeviceConfigListener = new DeviceConfigListener();

        mProcessObserver = new ProcessObserver();
        mIActivityManager = ActivityManager.getService();
        try {
            mIActivityManager.registerProcessObserver(mProcessObserver);
            mIActivityManager.registerTaskStackListener(mTaskStackListener);
        } catch (RemoteException e) {
            Log.e(TAG, "could not get IActivityManager");
        }
        if (SystemProperties.get("sys.vendor.ethernet.wol", "enable").equals("enable")) {
            if (mSCM != null)
                mSCM.writeSysFs("/sys/class/ethernet/wol" , "1");
        }
        mMsgHandler = new Handler() {
            @Override
            public void handleMessage(Message msg) {
                switch (msg.what) {
                    case MSG_UPDATA:
                        Log.d(TAG, "handleMessage");
                        netflixFGStateUpdate();
                        break;
                    case MSG_UPDATA_DISPLAY:
                        Log.d(TAG, "handleMessage display");
                        resetDisplayConversionMode();
                        break;
                    default:
                        Log.d(TAG, "No handler case available for message: " + msg.what);
                }
            }
        };
        resetHdrPolicy();
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

        mDeviceConfigListener = null;

        super.onDestroy();
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }


    private void resetDisplayConversionMode(){
         if (mDisplayManager.getHdrConversionMode().getConversionMode() != HdrConversionMode.HDR_CONVERSION_SYSTEM)
             return;

        int preferredHdrFormat = mDisplayManager.getHdrConversionMode().getPreferredHdrOutputType();
        Log.d(TAG, "now preferredHdrFormat = " + preferredHdrFormat);
        if (preferredHdrFormat != -1
                && !isHdrFormatSupported(mDisplayManager.getDisplay(Display.DEFAULT_DISPLAY).getMode(), preferredHdrFormat)) {
            HdrConversionMode systemHdrConversionMode = new HdrConversionMode(
                    HdrConversionMode.HDR_CONVERSION_SYSTEM);
            mDisplayManager.setHdrConversionMode(systemHdrConversionMode);
            Log.d(TAG, "reset HDR_CONVERSION_SYSTEM to right preferredHdrFormat");
        }
    }

    private boolean isHdrFormatSupported(Display.Mode mode, int hdrFormat) {
        return Arrays.stream(mode.getSupportedHdrTypes()).anyMatch(
        hdr -> hdr == hdrFormat);
   }

    private void initNrdpCapabilities() {
        String buildDate = PlatformAPI.getStringProperty("ro.build.version.incremental", "");
        boolean needUpdate = !buildDate.equals(SettingsPref.getSavedBuildDate(mContext));
        boolean audioNeedUpdate = false;
        if (needUpdate && hasMS12) {
            Settings.Global.putInt(mContext.getContentResolver(), DOLBY_LIB, 2);
        }
        int dolbyint = Settings.Global.getInt(mContext.getContentResolver(), DOLBY_LIB, 0);
        if ( hasMS12 && (dolbyint != 2)) {
            audioNeedUpdate =true;
            Settings.Global.putInt(mContext.getContentResolver(), DOLBY_LIB, 2);
        }
        if ( !hasMS12 && (dolbyint != 0)) {
            audioNeedUpdate =true;
            Settings.Global.putInt(mContext.getContentResolver(), DOLBY_LIB, 0);
        }
        setNrdpCapabilitiesIfNeed(NRDP_PLATFORM_CAP, needUpdate);
        setNrdpCapabilitiesIfNeed(NRDP_AUDIO_PLATFORM_CAP, needUpdate || audioNeedUpdate);
        if (needUpdate) {
            SettingsPref.setSavedBuildDate(mContext, buildDate);
        }
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
            Log.i(TAG, "launchNetflix");
            launchNetflix();
        }
    }

    private void launchNetflixAtv() {
        Log.i(TAG, "launchNetflix atv from power on");
        Intent netflixIntent = new Intent();
        netflixIntent.setAction(NETFLIX_INTENT);
        netflixIntent.setPackage(NETFLIX_PKG_NAME);
        netflixIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_REORDER_TO_FRONT);
        netflixIntent.putExtra(NETFLIX_KEY_POWER_MODE, true); //true for powerOnFromNetflixButton
        mContext.startActivity(netflixIntent);
    }

    private void launchNetflix() {
        PackageManager packageManager = mContext.getPackageManager();
        if (packageManager.getLaunchIntentForPackage(NETFLIX_PKG_NAME) == null) {
            Log.e(TAG, "Cannot find intent for Netlix package: " + NETFLIX_PKG_NAME);
            return;
        }
        String globalButtonLaunch = mContext.getString(R.string.config_globalButtonLaunch);
        Log.d(TAG, " globalButtonLaunch component: " + globalButtonLaunch);

        Intent intent = new Intent(ACTION_LAUNCH_APP);
        intent.setComponent(ComponentName.unflattenFromString(globalButtonLaunch));
        intent.putExtra(EXTRA_PACKAGE_NAME, NETFLIX_PKG_NAME);
        intent.addFlags(Intent.FLAG_INCLUDE_STOPPED_PACKAGES | Intent.FLAG_RECEIVER_FOREGROUND);

        Intent launchIntent  = new Intent(NETFLIX_INTENT);
        launchIntent.setPackage(NETFLIX_PKG_NAME);
        launchIntent.putExtra(NETFLIX_KEY_POWER_MODE, true);
        launchIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_INCLUDE_STOPPED_PACKAGES
            | Intent.FLAG_RECEIVER_FOREGROUND | Intent.FLAG_ACTIVITY_REORDER_TO_FRONT);
        intent.putExtra(EXTRA_LAUNCH_INTENT, launchIntent);

        if (canIntentBeHandled(mContext, intent)) {
            Log.d(TAG, "launchNetflix gtv from power on ");
            mContext.sendBroadcast(intent);
        } else {
            launchNetflixAtv();
        }
    }

    private  boolean canIntentBeHandled(Context context, Intent intent) {
        List<ResolveInfo> receivers = context.getPackageManager().queryBroadcastReceivers(
            intent, PackageManager.MATCH_ALL);
        Log.d(TAG, "receivers " + receivers);
        if (receivers != null && receivers.size() > 0) {
            return true;
        }
        return false;
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
                if (info.importance == ActivityManager.RunningAppProcessInfo.IMPORTANCE_FOREGROUND) return true;
                else {
                    return isTopActivity(pkgName);
                }
            }
        }

        return isTopActivity(pkgName);
    }

    private boolean isTopActivity(String pkgName){
        ActivityManager am = (ActivityManager) mContext.getSystemService(Context.ACTIVITY_SERVICE);
        List<ActivityManager.RunningTaskInfo> infos = am.getRunningTasks(1);
        ComponentName componentInfo = infos.get(0).topActivity;

        if (componentInfo.getPackageName().equals(pkgName)) {
            Log.d(TAG,pkgName + " is top activity!");
            return true;
        }else{
            Log.d(TAG,pkgName + "is not top activity.");
            return false;
        }
    }

    private boolean isTvtsOrCtsRunning() {
        ActivityManager am = (ActivityManager) mContext.getSystemService(Context.ACTIVITY_SERVICE);
        List<ActivityManager.RunningAppProcessInfo> infos = am.getRunningAppProcesses();

        for (int i = 0; i < infos.size(); i++) {
            ActivityManager.RunningAppProcessInfo info = infos.get(i);
            if (info.processName.contains("tvts") || info.processName.contains("leanbackjank") ||
                    info.processName.contains("cts")) {
                Log.d(TAG, "processName:" + info.processName);
                return true;
            }
        }

        return false;
    }

    private boolean isTopTask(String pkgName){
        try {
             // return if the activity monitor is no longer used
            if (mIActivityManager == null) {
                 return false;
            }
            List<RootTaskInfo> infos = mIActivityManager.getAllRootTaskInfos();
            for (RootTaskInfo info : infos) {
                if (!info.visible) {
                    continue;
                }
                ComponentName componentInfo = info.topActivity;
                if (componentInfo.getPackageName().equals(pkgName)) {
                    Log.d(TAG,pkgName + " is top activity!");
                    return true;
                }else{
                    Log.d(TAG,pkgName + " is not top activity.");
                    return false;
                }
            }
        }catch (RemoteException e) {
            Log.e(TAG, "Cannot getTasks", e);
        }
        return false;
    }

    private void refreshAudioCapabilities(boolean isHdmiPlugged) {
        boolean isTv = DroidLogicUtils.isTv();
        int surround = mOutputModeManager.getDigitalAudioFormatOut();
        Log.i(TAG, "onReceived HDMI_PLUGGED: " + isHdmiPlugged + ", isTv:" + isTv + ", surround:" +
                DroidLogicUtils.audioFormatOutputToString(surround));
        if (isTv) {
            tvNrdpAudioPlatformCapabilitiesConfig();
        } else {
            String audioSinkCap = mSCM.readSysFs(SYS_AUDIO_CAP);
            atmosSupported = audioSinkCap.contains("Dolby_Digital+/ATMOS");
            doblySupported = audioSinkCap.contains("Dolby_Digital");
            if (isHdmiPlugged && (OutputModeManager.DIGITAL_AUDIO_FORMAT_AUTO == surround
                || OutputModeManager.DIGITAL_AUDIO_FORMAT_PASSTHROUGH == surround) ) {
                Log.i(TAG, "ATMOS: " + atmosSupported + ", audioSinkCap: " + audioSinkCap);
                setAtmosEnabled(atmosSupported);
                if (hasMS12) {
                    setUiAudioBufferDelayOffset(doblySupported);
                }
            }
        }
    }

    private void setAtmosEnabled(boolean enabled) {
        // Refer to /vendor/etc/nrdp_audio_platform_capabilities.json
        String audioCap = Settings.Global.getString(getContentResolver(), NRDP_AUDIO_PLATFORM_CAP);
        if (audioCap == null)
            return;

        if (!SystemProperties.get("sys.vendor.atmos.passthrough").equals("enable")) {
            if (!hasMS12 ||
                (mOutputModeManager.getDigitalAudioFormatOut() == OutputModeManager.DIGITAL_AUDIO_FORMAT_PASSTHROUGH))
                return;
        }

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

    private void setUiAudioBufferDelayOffset(boolean isDolbySupported) {
        setUiAudioBufferDelayOffset(isDolbySupported ? UI_AUDIO_DELAY_OFFSET_OTT_DOLBY : UI_AUDIO_DELAY_OFFSET_OTT_PCM);
    }

    private void setUiAudioBufferDelayOffset(int setOffset) {
        // Refer to /vendor/etc/nrdp_audio_platform_capabilities.json
        String audioCap = Settings.Global.getString(getContentResolver(), NRDP_AUDIO_PLATFORM_CAP);
        if (audioCap == null)
            return;

        try {
            JSONObject rootObject = new JSONObject(audioCap);
            JSONObject audioCapsObject = rootObject.getJSONObject("audiocaps");
            int uiOffset = audioCapsObject.getInt("uiAudioBufferDelayOffset");
            if (uiOffset != setOffset) {
                Log.i(TAG, "uiOffset from  " + uiOffset + "to " + setOffset);
                audioCapsObject.put("uiAudioBufferDelayOffset", setOffset);
                Settings.Global.putString(getContentResolver(), NRDP_AUDIO_PLATFORM_CAP, rootObject.toString());
            }
        } catch (org.json.JSONException e) {
            e.printStackTrace();
        }
    }

    private boolean isAtmosConfiged() {
        String capName_File = NRDP_AUDIO_PLATFORM_CAP;
        if (hasMS12 && mOutputModeManager.getDigitalAudioFormatOut() == OutputModeManager.DIGITAL_AUDIO_FORMAT_AUTO) {
            capName_File = NRDP_AUDIO_PLATFORM_CAP_MS12;
        }

        try {
            Log.i(TAG, "capName_File = " + capName_File);
            StringBuilder sb = new StringBuilder();
            Scanner scanner = new Scanner(new File(NRDP_PLATFORM_CONFIG_DIR + capName_File + ".json"));
            while (scanner.hasNextLine()) {
                sb.append(scanner.nextLine());
                sb.append('\n');
            }
            scanner.close();

            JSONObject rootObject = new JSONObject(sb.toString());
            JSONObject audioCapsObject = rootObject.getJSONObject("audiocaps");
            JSONObject atmosObject = audioCapsObject.getJSONObject("atmos");

            return atmosObject.getBoolean("enabled");
        } catch(java.io.FileNotFoundException e) {
            Log.d(TAG, e.getMessage());
        } catch(Exception e) {
            e.printStackTrace();
        }

            return false;
        }

    boolean isArcPluged(String hdmiArcStr)
    {
        if (TextUtils.isEmpty(hdmiArcStr)) {
            Log.e(TAG, "hdmiArcStr is empty!");
            return false;
        }

        if (hdmiArcStr != null && hdmiArcStr.contains("7, 0, 0, 0, 0") && hdmiArcStr.contains("10, 0, 0, 0, 0"))
            return false;

        return true;
    }

    boolean isArcSupportAtmos(String hdmiArcStr)
    {
        String regex = "\\[(10,\\s+\\d+,\\s+\\d+,\\s+\\d+,\\s+\\d+)]\\|set_";
        Pattern pattern = Pattern.compile(regex);
        Matcher matcher = pattern.matcher(hdmiArcStr);
        String formatStr = null;

        if (matcher.find()) {
            formatStr = matcher.group(1);
            if (!TextUtils.isEmpty(formatStr)) {
                Log.d(TAG, "formatStr:" + formatStr);
                String[] formatArray = formatStr.split(",");
                if (formatArray.length  > 0) {
                    atmosSupported = (Integer.parseInt(formatArray[formatArray.length - 1].trim()) & 0x1)	> 0;
                    Log.d(TAG,  "atmosSupported:" + atmosSupported  + " ,value:" + formatArray[formatArray.length - 1]);
                    return atmosSupported;
                }
            }
        }

        return false;
    }

    private void tvNrdpAudioPlatformCapabilitiesConfig()
    {
        boolean isAtmos = atmosSupportedByConfig;

        String hdmiArcStr = Settings.System.getString(getContentResolver(), "settings_audio_descriptor");

        if (!TextUtils.isEmpty(hdmiArcStr)) {
            Log.d(TAG, "hdmiArcStr = " + hdmiArcStr);
            if (isArcPluged(hdmiArcStr)) {
                isAtmos = isArcSupportAtmos(hdmiArcStr);
            }
        }

        setAtmosEnabled(isAtmos);

        setUiAudioBufferDelayOffset(hasMS12 ? UI_AUDIO_DELAY_OFFSET_TV_MS12 : UI_AUDIO_DELAY_OFFSET_TV_NON_DOLBY);
    }

    private void setAlwaysHDR(boolean NetflixIsForeground) {
        //when netflix is fg, enable always HDR whatever.
        if (tempHDR) {
           Log.i(TAG, "setHdrStrategy adaptive default");
           mSCM.setHdrStrategy(STR_ADAPTIVE);
           tempHDR = false;
        }
        if (NetflixIsForeground && mOutputModeManager.getHdrStrategy().startsWith(STR_ADAPTIVE) &&
                                mDisplayManager.getDisplay(Display.DEFAULT_DISPLAY).isHdr()) {
            Log.i(TAG, "setHdrStrategy  always");
            mSCM.setHdrStrategy(STR_ALWAYS);
            tempHDR = true;
        }
         Log.d(TAG,"NetflixIsForeground,startsWith,isHdr: "+NetflixIsForeground
                +mOutputModeManager.getHdrStrategy().startsWith(STR_ADAPTIVE)+mDisplayManager.getDisplay(Display.DEFAULT_DISPLAY).isHdr());
    }


    private void setHDRConversionMode(boolean NetflixIsForeground) {
        boolean isPassThroughHdr = mDisplayManager.getHdrConversionModeSetting().equals(new HdrConversionMode(
                            HdrConversionMode.HDR_CONVERSION_PASSTHROUGH));
        Log.d(TAG,"NetflixIsForeground = " + NetflixIsForeground
                +" ,isPassThroughHdr = " + isPassThroughHdr
                + ", is display support hdr = " + mDisplayManager.getDisplay(Display.DEFAULT_DISPLAY).isHdr());
        if (tempHDR && (!NetflixIsForeground)) {
            Log.i(TAG, "setHdrStrategy adaptive default");
            mHdrConversionMode =new HdrConversionMode(HdrConversionMode.HDR_CONVERSION_PASSTHROUGH);
            mDisplayManager.setHdrConversionMode(mHdrConversionMode);
            tempHDR = false;
            Settings.Global.putInt(mContext.getContentResolver(), TEMP_HDR, 0);
            return;
        }
        if (NetflixIsForeground && isPassThroughHdr &&
                                mDisplayManager.getDisplay(Display.DEFAULT_DISPLAY).isHdr()) {
            Log.i(TAG, "setHdrStrategy  always");
            mHdrConversionMode = new HdrConversionMode(HdrConversionMode.HDR_CONVERSION_SYSTEM);
            mDisplayManager.setHdrConversionMode(mHdrConversionMode);
            tempHDR = true;
            Settings.Global.putInt(mContext.getContentResolver(), TEMP_HDR, 1);
        }
    }

    private void resetHdrPolicy() {
        int hdrpolicy = Settings.Global.getInt(mContext.getContentResolver(), TEMP_HDR, 0);
        if (hdrpolicy == 1) {
            Log.i(TAG, "reset HdrStrategy adaptive default");
            mHdrConversionMode =new HdrConversionMode(HdrConversionMode.HDR_CONVERSION_PASSTHROUGH);
            mDisplayManager.setHdrConversionMode(mHdrConversionMode);
            Settings.Global.putInt(mContext.getContentResolver(), TEMP_HDR, 0);
        }
    }

    private void netflixFGStateUpdate() {
        synchronized (mLock) {
            boolean fg = isTopTask(NETFLIX_PKG_NAME);
            boolean netflix = isVisibleApp(NETFLIX_PKG_NAME);
            if (netflix  && !isTvtsOrCtsRunning()) {
                ActivityManager am = (ActivityManager) mContext.getSystemService(Context.ACTIVITY_SERVICE);
                final List<ActivityManager.RunningAppProcessInfo> procs = am.getRunningAppProcesses();
                for (ActivityManager.RunningAppProcessInfo info: procs) {
                    if (info.importance
                            == ActivityManager.RunningAppProcessInfo.IMPORTANCE_CACHED
                            && !TextUtils.equals(NETFLIX_PKG_NAME, info.processName)
                            && !TextUtils.equals(YOUTUBE_PKG_NAME, info.processName)
                            && !TextUtils.equals(LAUNCHER_PKG_NAME, info.processName)) {
                        am.killBackgroundProcesses(info.pkgList[0]);
                    }
                }
            }
            Log.i(TAG,"fg: "+fg + "  mIsNetflixFg: "+ mIsNetflixFg);
            if (fg ^ mIsNetflixFg) {
                Log.i(TAG, "Netflix status changed from " + (mIsNetflixFg ? "fg" : "bg") + " -> " + (fg ? "fg" : "bg"));
                mIsNetflixFg = fg;

                mAudioManager.setParameters("continuous_audio_mode=" + (fg ? "1" : "0"));
                mSCM.setProperty("vendor.netflix.state", fg ? "fg" : "bg");
                if (fg) {
                    mOriginalPowerStateChangeValue = mHdmiControlManager.getPowerStateChangeOnActiveSourceLost();
                    mHdmiControlManager.setPowerStateChangeOnActiveSourceLost(LOST_NONE);
                } else {
                    mHdmiControlManager.setPowerStateChangeOnActiveSourceLost(mOriginalPowerStateChangeValue);
                }
            }

            boolean fgYoutube = isTopTask(YOUTUBE_PKG_NAME);
            if (fgYoutube ^ mIsYoutubeFg) {
                Log.i(TAG, "Youtube status changed from " + (mIsYoutubeFg ? "fg" : "bg") + " -> " + (fgYoutube ? "fg" : "bg"));
                mIsYoutubeFg = fgYoutube;
                mAudioManager.setParameters("compensate_video_enable=" + (fgYoutube ? "1" : "0"));
            }

        }
    }

    private final TaskStackListener mTaskStackListener = new TaskStackListener() {
        @Override
        public void onTaskStackChanged() {
            Log.i(TAG, "onTaskStackChanged");
            mMsgHandler.sendEmptyMessageDelayed(MSG_UPDATA,700);
        }
    };


    private class ProcessObserver extends IProcessObserver.Stub {
        @Override
        public void onForegroundActivitiesChanged(int pid, int uid, boolean foregroundActivities) {
            Log.d(TAG, "onForegroundActivitiesChanged pid:" + pid + ",uid:" + uid + ",fg:" + foregroundActivities);
            mMsgHandler.sendEmptyMessageDelayed(MSG_UPDATA,700);
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

