/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC
 */

package com.droidlogic;

import android.app.ActivityManager;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
//import android.os.ServiceManager;
//import android.os.UEventObserver;
import android.util.Log;
//import android.view.IWindowManager;

import java.lang.Integer;

import com.droidlogic.app.SystemControlManager;

public class DDRBandwidthService extends Service {
    private static final String TAG = "DDRBandwidthService";
    private static final String BANDWIDTH_BUSY_PATH = "/sys/class/aml_ddr/busy";
    private static final String BANDWIDTH_THRESHOLD_PATH = "/sys/class/aml_ddr/threshold";
    private static final String BANDWIDTH_URGENT_PATH = "/sys/class/aml_ddr/urgent";
    private static final String BANDWIDTH_MODE_PATH = "/sys/class/aml_ddr/mode";
    private static final String BANDWIDTH_CPU_PATH = "/sys/class/aml_ddr/cpu_type";
    private static final String BANDWIDTH_VIDEO_SKIP_PATH = "/sys/module/amvideo/parameters/force_vskip_cnt";
    private static final String PORPERTY_BUSY = "vendor.sys.bandwidth.busy";
    private static final String PORPERTY_THRESHOLD = "vendor.sys.bandwidth.threshold";
    private static final String PORPERTY_POLICY = "vendor.sys.bandwidth.policy";
    private static final int POLICY_URGENT_CONFIG = 0x1;
    private static final int POLICY_MALI_CORE_REDUCE = 0x2;
    private static final int POLICY_DISABLE_ANIMATION = 0x4;
    private static final int POLICY_LIMIT_BACKGROUND_PROCESS = 0x8;
    private static final int POLICY_VIDEO_VSKIP = 0x10;
    private static final int DEFAULTL_POLICY_VALUE = 0x7;
    private static final int MESON_CPU_MAJOR_ID_GXL = 0x21;
    private static final int MESON_CPU_MAJOR_ID_G12A = 0x28;

    private SystemControlManager mScm;
    //private IWindowManager mWm;
    private boolean mStat;
    private int mPolicy;
    private int mCpuType;

    private UEventObserver mUEventObserver = new UEventObserver() {
        @Override
        public void onUEvent(UEvent event) {
            String val = event.get("STATE");
            if ("ACA=1".equals(val)) {
                handleBandwidthBusy(true);
            } else {
                handleBandwidthBusy(false);
            }
        }
    };

    private void ueventInit() {
        String val = mScm.getPropertyString(PORPERTY_THRESHOLD, "0");
        if (!"0".equals(val)) {
            mScm.writeSysFs(BANDWIDTH_THRESHOLD_PATH, val);
        }
        mPolicy = mScm.getPropertyInt(PORPERTY_POLICY, DEFAULTL_POLICY_VALUE);
        val = mScm.readSysFs(BANDWIDTH_CPU_PATH);
        mCpuType = Integer.parseInt(val, 16);
        mScm.writeSysFs(BANDWIDTH_MODE_PATH, "2");
        mUEventObserver.startObserving("ddr_extcon_bandwidth");
    }

    private void handleBandwidthBusy(boolean busy) {
        if (busy == mStat)
            return;
        //Log.d(TAG, "stat changed: "+busy+", policy="+mPolicy);
        mStat = busy;
        if ((mPolicy & POLICY_URGENT_CONFIG) != 0) {
            if (mCpuType >= MESON_CPU_MAJOR_ID_G12A) {
                mScm.writeSysFs(BANDWIDTH_URGENT_PATH, busy?"3F0010 4":"3F0010 0");
                mScm.writeSysFs(BANDWIDTH_URGENT_PATH, busy?"3 1":"3 0");
            } else if (mCpuType >= MESON_CPU_MAJOR_ID_GXL) {
                mScm.writeSysFs(BANDWIDTH_URGENT_PATH, busy?"3F10 4":"3F10 0");
                mScm.writeSysFs(BANDWIDTH_URGENT_PATH, busy?"7 1":"7 0");
            }
        }
        if ((mPolicy & POLICY_MALI_CORE_REDUCE) != 0) {
            mScm.setProperty(PORPERTY_BUSY, busy?"1":"0");
        }
        /*if ((mPolicy & POLICY_DISABLE_ANIMATION) != 0) {
            try {
                if (busy) {
                    mWm.setAnimationScale(0, 0.0f);
                    mWm.setAnimationScale(1, 0.0f);
                    mWm.setAnimationScale(2, 0.0f);
                } else {
                    mWm.setAnimationScale(0, 1.0f);
                    mWm.setAnimationScale(1, 1.0f);
                    mWm.setAnimationScale(2, 1.0f);
                }
            } catch (Exception e) {
                Log.d(TAG, "get exception: "+e);
            }
        }
        if ((mPolicy & POLICY_LIMIT_BACKGROUND_PROCESS) != 0) {
            try {
                ActivityManager.getService().setProcessLimit(busy?2:-1);
            } catch (Exception e) {
                Log.d(TAG, "get exception: "+e);
            }
        }*/
        if ((mPolicy & POLICY_VIDEO_VSKIP) != 0) {
            mScm.writeSysFs(BANDWIDTH_VIDEO_SKIP_PATH, busy?"1":"0");
        }
    }

    @Override
    public void onCreate() {
        super.onCreate();
        mScm = SystemControlManager.getInstance();
        //mWm = IWindowManager.Stub.asInterface(ServiceManager.getService("window"));
        mStat = false;
        ueventInit();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        return super.onStartCommand(intent, flags, startId);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        mUEventObserver.stopObserving();
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }
}

