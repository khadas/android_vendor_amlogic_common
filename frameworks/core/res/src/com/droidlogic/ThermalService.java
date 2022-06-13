/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC ThermalService
 */

package com.droidlogic;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.os.HardwarePropertiesManager;
import android.os.IBinder;
import android.util.Log;

import java.util.Arrays;

/**
 * @author yuehu.mi
 */
public class ThermalService extends Service {
    private static final String TAG = ThermalService.class.getSimpleName();
    private HardwarePropertiesManager mHwManager;

    private final Handler mHandler = new Handler();
    Runnable tempRunnable = new Runnable() {
        @Override
        public void run() {
            if (getDeviceCpuTemperatures()) {
                mHandler.postDelayed(this, 2000);
            }
        }
    };

    private boolean getDeviceCpuTemperatures() {
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.N) {
            if (mHwManager != null) {
                float[] cpuTemps = mHwManager.getDeviceTemperatures(
                        HardwarePropertiesManager.DEVICE_TEMPERATURE_CPU,
                        HardwarePropertiesManager.TEMPERATURE_CURRENT);
                Log.i(TAG, "CPU temperatures: " + Arrays.toString(cpuTemps));
                return true;
            }
        } else {
            Log.e(TAG, "The current API does not support getting temperature");

        }
        return false;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.N) {
            mHwManager = (HardwarePropertiesManager) getSystemService(
                    Context.HARDWARE_PROPERTIES_SERVICE);
        }
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        mHandler.postDelayed(tempRunnable, 1000);
        return super.onStartCommand(intent, flags, startId);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        mHandler.removeCallbacksAndMessages(null);
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }
}
