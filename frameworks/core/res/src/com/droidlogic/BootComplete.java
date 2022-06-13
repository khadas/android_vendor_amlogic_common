/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC BootComplete
 */

package com.droidlogic;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
//import android.hardware.hdmi.HdmiDeviceInfo;
import android.os.UserHandle;
import android.os.UserManager;
import android.provider.Settings;
import android.content.ContentResolver;
import android.util.Log;
import android.provider.Settings;
import android.database.ContentObserver;
import android.net.Uri;
import android.os.Handler;

import android.content.ComponentName;
import android.content.ServiceConnection;
import android.os.IBinder;
import android.os.RemoteException;
import java.lang.reflect.AccessibleObject;
import java.lang.reflect.Field;


import com.droidlogic.app.DroidLogicUtils;
import com.droidlogic.app.PlayBackManager;
import com.droidlogic.app.SystemControlEvent;
import com.droidlogic.app.SystemControlManager;
import com.droidlogic.app.UsbCameraManager;

public class BootComplete extends BroadcastReceiver {
    private static final String TAG             = "BootComplete";
    private static final String DECRYPT_STATE = "encrypted";
    private static final String DECRYPT_TYPE = "file";
    private static final String DROID_SETTINGS_PACKAGE = "com.droidlogic.tv.settings";
    private static final String DROID_SETTINGS_ENCRYPTKEEPERFBE = "com.droidlogic.tv.settings.CryptKeeperFBE";

    private SystemControlEvent mSystemControlEvent;
    /*private boolean mHasTvUiMode;*/
    private SystemControlManager mSystemControlManager;

    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();
        Log.i(TAG, "action: " + action);
        if (SettingsPref.getSavedBootCompletedStatus(context)) {
            SettingsPref.setSavedBootCompletedStatus(context, false);
            return;
        }
        SettingsPref.setSavedBootCompletedStatus(context, true);
        mSystemControlManager =  SystemControlManager.getInstance();
        /*mHasTvUiMode = DroidLogicUtils.isTv();*/
        final ContentResolver resolver = context.getContentResolver();

// Should not do in java
//        //register system control callback
        mSystemControlEvent = SystemControlEvent.getInstance(context);
        mSystemControlManager.setListener(mSystemControlEvent);

        if (SettingsPref.getFirstRun(context)) {
            Log.i(TAG, "first running: " + context.getPackageName());
            Settings.Secure.putInt(context.getContentResolver(), Settings.Secure.SHOW_IME_WITH_HARD_KEYBOARD, 1);
            SettingsPref.setFirstRun(context, false);
        }

        //use to check whether disable camera or not
        new UsbCameraManager(context).bootReady();

        new PlayBackManager(context).initHdmiSelfadaption();

        if (getBooleanProperty("ro.vendor.subtitle.enable_fallback_display", false)) {
            context.startService(new Intent(context, SubtitleDisplayer.class));
        }
        if (context.getPackageManager().hasSystemFeature(NetflixService.FEATURE_SOFTWARE_NETFLIX)) {
            context.startService(new Intent(context, NetflixService.class));
        }

        context.startService(new Intent(context,NtpService.class));
        context.startService(new Intent(context,ShuntdownService.class));

        /*if (mHasTvUiMode)
            context.startService(new Intent(context, DroidLogicPowerService.class));*/

        if (getBooleanProperty("ro.vendor.platform.support.network_led", false) == true)
            context.startService(new Intent(context, NetworkSwitchService.class));

        /*if (mHasTvUiMode)
            context.startService(new Intent(context, EsmService.class));*/

        if (getBooleanProperty("vendor.sys.bandwidth.enable", false))
            context.startService(new Intent(context, DDRBandwidthService.class));

        /*  AML default rotation config, cannot use with shipping_api_level=28
            String rotProp = mSystemControlManager.getPropertyString("persist.vendor.sys.app.rotation", "");
            ContentResolver res = context.getContentResolver();
            int acceRotation = Settings.System.getIntForUser(res,
                Settings.System.ACCELEROMETER_ROTATION,
                0,
                UserHandle.USER_CURRENT);
            if (rotProp != null && ("middle_port".equals(rotProp) || "force_land".equals(rotProp))) {
                    if (0 != acceRotation) {
                        Settings.System.putIntForUser(res,
                            Settings.System.ACCELEROMETER_ROTATION,
                            0,
                            UserHandle.USER_CURRENT);
                    }
            }
         */

        // enableCryptKeeperComponent(context);

        if (Intent.ACTION_BOOT_COMPLETED.equals(action)) {
            SettingsPref.setSavedBootCompletedStatus(context, false);
        }
        updateDeveloperOptionsWatcher(context);
    }

    private boolean getBooleanProperty(String property, boolean defVal) {
        try {
            return (boolean)Class.forName("android.os.SystemProperties")
                .getMethod("getBoolean", new Class[] { String.class, Boolean.TYPE })
                .invoke(null, new Object[] { property, defVal });
        } catch(Exception e) {
            e.printStackTrace();
        }
        return false;
    }

/*    private void enableCryptKeeperComponent(Context context) {
        String state = SystemProperties.get("ro.crypto.state");
        String type = SystemProperties.get("ro.crypto.type");
        boolean isMultiUser = UserManager.supportsMultipleUsers();
        if (("".equals(state) || !DECRYPT_STATE.equals(state) || !DECRYPT_TYPE.equals(type)) || !isMultiUser) {
            return;
        }

        PackageManager pm = context.getPackageManager();
        ComponentName name = new ComponentName(DROID_SETTINGS_PACKAGE, DROID_SETTINGS_ENCRYPTKEEPERFBE);
        Log.d(TAG, "enableCryptKeeperComponent " + name);
        try {
            pm.setComponentEnabledSetting(name, PackageManager.COMPONENT_ENABLED_STATE_ENABLED,
                    PackageManager.DONT_KILL_APP);
        } catch (Exception e) {
            Log.e(TAG, e.toString());
        }
    }
*/

    private boolean needCecExtend(SystemControlManager sm, Context context) {
        //return sm.getPropertyInt("ro.hdmi.device_type", -1) == HdmiDeviceInfo.DEVICE_PLAYBACK;
        return true;
    }

    private static void updateDeveloperOptionsWatcher(final Context context) {
        Uri settingUri = Settings.Global.getUriFor(
                Settings.Global.DEVELOPMENT_SETTINGS_ENABLED);

        ContentObserver developerOptionsObserver =
                new ContentObserver(new Handler()) {
                    @Override
                    public void onChange(boolean selfChange) {
                        super.onChange(selfChange);

                        boolean developerOptionsEnabled = (1 ==
                                Settings.Global.getInt(context.getContentResolver(),
                                        Settings.Global.DEVELOPMENT_SETTINGS_ENABLED, 0));

                        Log.d(TAG, "onChange developerOptionsEnabled=" + developerOptionsEnabled);
                        if (developerOptionsEnabled) {
                            context.startService(new Intent(context, ThermalService.class));
                        } else {
                            context.stopService(new Intent(context, ThermalService.class));
                        }
                    }
                };

        context.getContentResolver().registerContentObserver(settingUri,
                false, developerOptionsObserver);
        developerOptionsObserver.onChange(true);
    }

}
