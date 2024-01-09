package com.droidlogic;

import android.app.ActivityManager;
import android.app.IActivityManager;
import android.app.IProcessObserver;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;

import com.droidlogic.app.SystemControlManager;

import java.util.ArrayList;
import java.util.List;

public class DroidLogicBenchService extends Service {
    private static final String TAG = "DroidLogicBenchService";
    private static final int BENCH_TEST_APP_FLAG = 0;
    private static final int BENCH_TEST_APP_ENABLE = 1;
    private static final int BENCH_TEST_APP_DISABLE = 2;
    private static final String GEEKBENCH_PKG_NAME = "com.primatelabs.geekbench";
    private static final String GFXBENCH_PKG_NAME = "net.kishonti.gfxbench";
    private static final String PCMARK_PKG_NAME = "com.futuremark.pcmark.android.benchmark";
    private static final String ANTUTU_PKG_NAME = "com.antutu.ABenchMark";
    private static final String ANTUTU_3D_PKG_NAME = "com.antutu.benchmark.full";
    private static final String BASEMARK_PKG_NAME = "com.rightware.BasemarkOSIICN";
    private Context mContext;
    private SystemControlManager mSCM;
    private IActivityManager mIActivityManager;
    private ProcessObserver mProcessObserver;
    private final Object mLock = new Object();
    private ArrayList<String> benchApps;

    private void initPoorApp() {
        benchApps = new ArrayList();
        benchApps.add("com.google.android.inputmethod.latin");
        benchApps.add("com.google.android.katniss");
        benchApps.add("com.android.vending");
        benchApps.add("com.google.android.tvrecommendations");
        benchApps.add("com.google.android.tts");
    }

    @Override
    public void onCreate() {
        super.onCreate();
        Log.d(TAG, "DroidLogicBenchService is oncreate");
        mContext = this;
        mSCM = SystemControlManager.getInstance();
        mProcessObserver = new ProcessObserver();
        mIActivityManager = ActivityManager.getService();
        try {
            mIActivityManager.registerProcessObserver(mProcessObserver);
        } catch (RemoteException e) {
            Log.e(TAG, "could not get IActivityManager");
        }
        initPoorApp();
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    private void hidePoorApp() {
        PackageManager packageManager = mContext.getPackageManager();
        for (String app : benchApps) {
            try {
                packageManager.getPackageInfo(app, PackageManager.GET_ACTIVITIES);
                packageManager.setApplicationEnabledSetting(app, BENCH_TEST_APP_DISABLE, BENCH_TEST_APP_FLAG);
            } catch (Exception e) {
                Log.w(TAG, app + " is not found");
            }

        }
    }

    private void unHidePoorApp() {
        PackageManager packageManager = mContext.getPackageManager();
        for (String app : benchApps) {
            try {
                packageManager.getPackageInfo(app, PackageManager.GET_ACTIVITIES);
                packageManager.setApplicationEnabledSetting(app, BENCH_TEST_APP_ENABLE, BENCH_TEST_APP_FLAG);
            } catch (Exception e) {
                Log.w(TAG, app + " is not found");
            }
        }
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

    private class ProcessObserver extends IProcessObserver.Stub {
        @Override
        public void onForegroundActivitiesChanged(int pid, int uid, boolean foregroundActivities) {
            Log.d(TAG, "onForegroundActivitiesChanged pid:" + pid + ",uid:" + uid + ",fg:" + foregroundActivities);
            new Thread(() -> {
                try {
                    //wait 500ms, for android update process stack
                    Thread.sleep(500);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
                fGStateUpdate();
            }).start();
        }

        private void fGStateUpdate() {
            synchronized (mLock) {

                boolean antutu = isVisibleApp(ANTUTU_PKG_NAME);
                boolean antutu3D = isVisibleApp(ANTUTU_3D_PKG_NAME);
                boolean pcMark = isVisibleApp(PCMARK_PKG_NAME);
                boolean geekbench = isVisibleApp(GEEKBENCH_PKG_NAME);
                boolean gfx = isVisibleApp(GFXBENCH_PKG_NAME);
                boolean baseMark = isVisibleApp(BASEMARK_PKG_NAME);
                if (antutu || antutu3D || pcMark || geekbench || gfx || baseMark) {
                    Log.d(TAG, "bench app is onForeground");
                    hidePoorApp();
                    performanceOptimization(true);
                } else {
                    Log.d(TAG, "bench app is not onForeground and set default param");
                    unHidePoorApp();
                    performanceOptimization(false);
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

    private void performanceOptimization(boolean status) {
        if (mSCM != null) {
            if (status) {
                mSCM.writeSysFs("/sys/class/thermal/thermal_zone0/mode", "disabled");
                mSCM.writeSysFs("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor", "performance");
                mSCM.writeSysFs("/sys/class/devfreq/fe400000.valhall/governor", "performance");
                mSCM.writeSysFs("/proc/sys/kernel/printk", "0");
                mSCM.writeSysFs("/sys/class/mpgpu/scale_mode", "3");
            } else {
                mSCM.writeSysFs("/sys/class/thermal/thermal_zone0/mode", "enable");
                mSCM.writeSysFs("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor", "schedutil");
                mSCM.writeSysFs("/sys/class/devfreq/fe400000.valhall/governor", "simple_ondemand");
                mSCM.writeSysFs("/proc/sys/kernel/printk", "4");
                mSCM.writeSysFs("/sys/class/mpgpu/scale_mode", "1");
            }
        }
    }

}
