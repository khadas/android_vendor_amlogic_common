/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC BluetoothAutoPairReceiver
 */

package com.droidlogic.btpair;

import android.content.Context;
import android.content.Intent;
import android.util.Log;
import android.os.SystemProperties;
import android.content.BroadcastReceiver;
import android.content.pm.PackageManager;
import android.content.ComponentName;
import android.content.pm.PackageInfo;
import java.util.List;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import java.util.Set;
import android.text.TextUtils;
import android.provider.Settings;
import android.bluetooth.BluetoothClass;

public class BluetoothAutoPairReceiver extends BroadcastReceiver {
    private static final String TAG = "BluetoothAutoPairReceiver";
    private static final boolean DEBUG = true;

    private static String DEFAULT_REMOTE_TYPE = "IR_NONE";

    private void Log(String msg) {
        if (DEBUG) {
            Log.i(TAG, msg);
        }
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();
        if (Intent.ACTION_BOOT_COMPLETED.equals(action)) {
            Log("Received ACTION_BOOT_COMPLETED");
            int isUserSetup = 0;
            try {
                //  Settings.Global.DEVICE_PROVISIONED & Settings.Secure.USER_SETUP_COMPLETE are set as 1 if setupwizard is done.
                // They are always set as 1 on aosp .
                isUserSetup = Settings.Secure.getInt(context.getContentResolver(), Settings.Secure.USER_SETUP_COMPLETE) ;
            } catch (Settings.SettingNotFoundException e) {
                Log("!!!!SettingNotFoundException");
            }
            Log("isUserSetup:"+isUserSetup);
             if (isUserSetup == 1 && isAutoPairNeeded()) {
                Log("No boned bt remote, show droidlogic BT pairing screen!");
                Intent BtSetupIntent = new Intent();
                BtSetupIntent.setComponent(new ComponentName("com.android.tv.settings", "com.android.tv.settings.accessories.AddAccessoryActivity"));
                BtSetupIntent.putExtra("no_input_mode", true);
                BtSetupIntent.putExtra("show_remote_only", true);
                BtSetupIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                context.startActivity(BtSetupIntent);
            } else {
                Log("No need to show BT pairing screen");
            }
        }
    }

    private boolean isAvilible( Context context, String packageName ){
        final PackageManager packageManager = context.getPackageManager();
        List<PackageInfo> pinfo = packageManager.getInstalledPackages(0);
        for ( int i = 0; i < pinfo.size(); i++ )
        {
                if (pinfo.get(i).packageName.equalsIgnoreCase(packageName)) {
                    return true;
                }
        }
        return false;
    }

     private boolean isAutoPairNeeded() {

        /*For aosp , we don't need to show bt pairing screen when boot up forever.*/
        /*if (SystemProperties.get("ro.product.system.name", "aosp").contains("atv_generic"))
            return false;
        else
            return true;*/

        String remote_type = SystemProperties.get("sys.vendor.remote.type", DEFAULT_REMOTE_TYPE);
        if (!remote_type.contains("BT")) {
            Log.d(TAG, "Do not show bt pairing screen in aosp");
            return false;
        }


         BluetoothAdapter mBtAdapter = BluetoothAdapter.getDefaultAdapter();

         if (mBtAdapter == null) {
            Log.w(TAG, "Can't get BT adapter, return");
            return false;
        }
        final Set<BluetoothDevice> bondedDevices = mBtAdapter.getBondedDevices();
        if (bondedDevices == null) {
            Log.i(TAG, "No bondedDevices, return");
            return false;
        }

        for (final BluetoothDevice device : bondedDevices) {
            final String deviceAddress = device.getAddress();
            String deviceName = device.getName() != null ?  device.getName().replaceAll("[ *@#$%^&-]", "") : "null";
            Log.i(TAG, "device: "+ deviceName);
            if (TextUtils.isEmpty(deviceAddress)) {
                Log.w(TAG, "Skipping mysteriously empty bluetooth device");
                continue;
            }

            //No need to show bt pairing screen if there is default bt remote paired.
            BluetoothClass btClass = device.getBluetoothClass();
            if ( remote_type.contains(deviceName) &&
                        btClass != null &&
                        btClass.getMajorDeviceClass() == BluetoothClass.Device.Major.PERIPHERAL)
                return false;
        }
        return true;
     }

}