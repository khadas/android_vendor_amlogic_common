/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC ShuntdownService
 */

package com.droidlogic;

import android.app.ActivityManager;
import android.app.ActivityManager.RunningAppProcessInfo;
import android.app.ActivityManager.RunningTaskInfo;
import android.app.Service;
import android.content.Context;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.IBinder;
import android.os.Process;
import android.os.UserHandle;
import android.provider.Settings;
import android.text.TextUtils;
import android.util.Log;
import java.util.List;
import android.bluetooth.BluetoothAdapter;
import android.os.RemoteException;
import android.os.SystemProperties;
import android.os.SystemClock;
import android.app.admin.DevicePolicyManager;
import android.os.PowerManager;



public class ShuntdownService extends Service {

    private static final String TAG = "ShuntdownService";

    public static final String BT_NAME                  = "persist.vendor.bt_vendor";
    public static final String BLUETOOTH_PKG_NAME           = "com.android.bluetooth";
    private static final int BT_SLEEP_TIME = 300;
    private boolean amlbt = false;
    private Context mContext;
    private BluetoothAdapter bluetoothAdapter;
    private DevicePolicyManager mDevicePolicyManager;
    private ActivityManager mActivityManager;


    @Override
    public void onCreate() {
        super.onCreate();
        mContext = this;
        bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        mDevicePolicyManager = (DevicePolicyManager) getSystemService(Context.DEVICE_POLICY_SERVICE);
        mActivityManager = (ActivityManager) mContext.getSystemService (Context.ACTIVITY_SERVICE);
        if (SystemProperties.get(BT_NAME, "null").indexOf("aml") != -1 ||
            SystemProperties.get(BT_NAME, "null").indexOf("qca") != -1)
            amlbt = true;

        if (amlbt) {
            IntentFilter shundownfilter = new IntentFilter();
            shundownfilter.addAction(Intent.ACTION_SHUTDOWN);
            registerReceiver (shundownReceiver, shundownfilter);
        }
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        return super.onStartCommand(intent, flags, startId);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }


    private BroadcastReceiver shundownReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            Log.d(TAG, "action: " + action);
            //turn off the screen
            mDevicePolicyManager.lockNow();

            //bluetoothAdapter.disable(false);
            //donot save shundown status setting global bluetooth_on
            try {
                bluetoothAdapter.getClass().getMethod("disable", boolean.class)
                    .invoke(bluetoothAdapter,false);
            } catch (Exception e) {
                e.printStackTrace();
            }

            int btpid = -1;
            do {
                SystemClock.sleep(BT_SLEEP_TIME);
                btpid = getBtPid();
                Log.e(TAG, " getbtPid =" + btpid);
            } while (btpid > 0);
            Log.e(TAG, " bluetooth shutdown  done ");
        }

    };


    private int getBtPid() {
        List<RunningAppProcessInfo> services = mActivityManager.getRunningAppProcesses();
        for (int i = 0; i < services.size(); i++) {
            String servicename = services.get (i).processName;
            if (servicename.contains (BLUETOOTH_PKG_NAME)) {
                Log.d (TAG, "find process: " + servicename + " pid: " + services.get (i).pid);
                return services.get (i).pid;
            }
        }
        return -1;
    }
}
