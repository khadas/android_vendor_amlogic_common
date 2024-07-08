/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC DroidLogicPowerService
 */

package com.droidlogic;

import android.app.Service;
import android.content.Context;
import android.content.BroadcastReceiver;
import android.content.Intent;
import android.content.IntentFilter;
import android.media.AudioManager;
import android.net.wifi.WifiManager;
import android.net.Uri;
import android.os.IBinder;
import android.os.SystemProperties;
import android.util.Log;

import com.droidlogic.app.AudioSettingManager;
import com.droidlogic.app.DroidLogicUtils;
import com.droidlogic.app.SystemControlManager;

import java.io.IOException;

public class DroidLogicPowerService extends Service {
    private static final String TAG = "DroidLogicPowerService";
    private SystemControlManager mSystemControlManager = null;
    private AudioManager mAudioManager = null;
    private boolean mWifiDisableWhenSuspend = false;
    private static final int POWER_SUSPEND_OFF = 0;
    private static final int POWER_SUSPEND_ON = 1;
    private static final int POWER_SUSPEND_SHUTDOWN = 2;

    private BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            Log.d(TAG, "action: " + action);
            if (Intent.ACTION_SCREEN_ON.equals(action)) {
                String enableVad = mSystemControlManager.getPropertyString(AudioSettingManager.AUDIO_VAD_PROPERTY_VADWAKE, AudioSettingManager.AUDIO_VAD_STRING_VAD_OFF);
                if (enableVad.equals(AudioSettingManager.AUDIO_VAD_STRING_VAD_ON)) {
                    mAudioManager.setParameters("hal_param_vad_wakeup=resume");
                }
                setSuspendState(POWER_SUSPEND_OFF);
                setWifiState(context, true);
            } else if (Intent.ACTION_SCREEN_OFF.equals(action)) {
                setSuspendState(POWER_SUSPEND_ON);
                setWifiState(context, false);
                String enableVad = mSystemControlManager.getPropertyString(AudioSettingManager.AUDIO_VAD_PROPERTY_VADWAKE, AudioSettingManager.AUDIO_VAD_STRING_VAD_OFF);
                if (enableVad.equals(AudioSettingManager.AUDIO_VAD_STRING_VAD_ON)) {
                    mAudioManager.setParameters("hal_param_vad_wakeup=suspend");
                }
            } else if (Intent.ACTION_SHUTDOWN.equals(action)) {
                setSuspendState(POWER_SUSPEND_SHUTDOWN);
            } else if (Intent.ACTION_PACKAGE_ADDED.equals(intent.getAction())) {
                if ("anemone".equals(SystemProperties.get("ro.product.device"))) {
                    Uri data = intent.getData();
                    if (data != null) {
                        String packageName = data.getSchemeSpecificPart();
                        if (packageName.equals("android.server.wm.jetpack")) {
                            Log.d(TAG, "ADDED packageName: android.server.wm.jetpack");
                            try {
                                Process process = Runtime.getRuntime().exec("wm density 320");
                                process.waitFor();
                                process.destroy();
                            } catch (IOException | InterruptedException e) {
                                e.printStackTrace();
                            }
                        }
                    }
                }
            } else if (Intent.ACTION_PACKAGE_REMOVED.equals(intent.getAction())) {
                if ("anemone".equals(SystemProperties.get("ro.product.device"))) {
                    Uri data = intent.getData();
                    if (data != null) {
                        String packageName = data.getSchemeSpecificPart();
                        if (packageName.equals("android.server.wm.jetpack")) {
                            Log.d(TAG, "REMOVED packageName: android.server.wm.jetpack");
                            try {
                                Process process = Runtime.getRuntime().exec("wm density 240");
                                process.waitFor();
                                process.destroy();
                            } catch (IOException | InterruptedException e) {
                                e.printStackTrace();
                            }
                        }
                    }
                }
            }
        }
    };

    @Override
    public void onCreate() {
        super.onCreate();
        mSystemControlManager = SystemControlManager.getInstance();
        mAudioManager = (AudioManager) this.getSystemService(this.AUDIO_SERVICE);

        //register filter for screen intent
        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_SCREEN_OFF);
        filter.addAction(Intent.ACTION_SCREEN_ON);
        filter.addAction(Intent.ACTION_SHUTDOWN);
        registerReceiver (mReceiver, filter, this.RECEIVER_EXPORTED);

        //register filter for package intent
        IntentFilter packageFilter = new IntentFilter();
        packageFilter.addAction(Intent.ACTION_PACKAGE_ADDED);
        packageFilter.addAction(Intent.ACTION_PACKAGE_REMOVED);
        packageFilter.addDataScheme("package");
        registerReceiver(mReceiver, packageFilter, this.RECEIVER_EXPORTED);
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        return super.onStartCommand(intent, flags, startId);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        unregisterReceiver(mReceiver);
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    private void setWifiState(Context context, boolean state) {
        if (mSystemControlManager.getPropertyBoolean("ro.vendor.platform.wifi.suspend", false) == false) {
            return;
        }

        WifiManager wm = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);

        if (state) {
            if (mWifiDisableWhenSuspend == true) {
                try {
                    wm.setWifiEnabled(true);
                    mWifiDisableWhenSuspend = false;
                } catch (Exception e) {
                    /* ignore - local call */
                }
            }
        } else {
            int wifiState = wm.getWifiState();
            if (wifiState == WifiManager.WIFI_STATE_ENABLING
                    || wifiState == WifiManager.WIFI_STATE_ENABLED) {
                try {
                    wm.setWifiEnabled(false);
                    mWifiDisableWhenSuspend = true;
                } catch (Exception e) {
                    /* ignore - local call */
                }
                try {
                    Thread.sleep(2300);
                } catch (InterruptedException ignore) {
                }
            }
        }

        Log.d(TAG, "setWifiState: " + state);
    }

    private static final String KILL_ESM_PATH = "/sys/module/tvin_hdmirx/parameters/hdcp22_kill_esm";
    private static final String KILL_ESM_PATH_54 = "sys/module/aml_media/parameters/hdcp22_kill_esm";
    private static final String VIDEO_GLOBAL_OUTPUT_PATH = "/sys/class/video/video_global_output";

    private void setSuspendState(int state) {
        if (!DroidLogicUtils.isTv()) {
            return;
        }

        if (state == POWER_SUSPEND_SHUTDOWN) {
            mSystemControlManager.writeSysFs(VIDEO_GLOBAL_OUTPUT_PATH, "0");
            mSystemControlManager.writeSysFs(KILL_ESM_PATH, "1");
            mSystemControlManager.writeSysFs(KILL_ESM_PATH_54, "1");
        }

        if (state == POWER_SUSPEND_ON) {
            mSystemControlManager.setBootenv("ubootenv.var.suspend", "on");
        } else if (state == POWER_SUSPEND_OFF) {
            mSystemControlManager.setBootenv("ubootenv.var.suspend", "off");
        } else if (state == POWER_SUSPEND_SHUTDOWN) {
            mSystemControlManager.setBootenv("ubootenv.var.suspend", "shutdown");
        }

        Log.d(TAG, "setSuspendState: " + state);
    }
}
