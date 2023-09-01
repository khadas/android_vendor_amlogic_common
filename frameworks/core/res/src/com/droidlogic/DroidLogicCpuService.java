package com.droidlogic;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;

import com.droidlogic.app.SystemControlManager;

public class DroidLogicCpuService extends Service {
    private static final String TAG = "DroidLogicCpuService";
    private SystemControlManager mSCM = null;
    private String mTop;
    private String mForeground;
    private String mBackground;
    private String mSystem;
    private String mRestricted;

    private void setCpusetsDefault() {
        new Thread() {
            @Override
            public void run() {
                try {
                    Thread.sleep(1000*60*3);
                    mSCM.writeSysFs("/dev/cpuset/top-app/cpus", mTop);
                    mSCM.writeSysFs("/dev/cpuset/foreground/cpus", mForeground);
                    mSCM.writeSysFs("/dev/cpuset/background/cpus", mBackground);
                    mSCM.writeSysFs("/dev/cpuset/system-background/cpus", mSystem);
                    mSCM.writeSysFs("/dev/cpuset/restricted/cpus", mRestricted);
                    Log.d(TAG, "end setCpusetsDefault: 0-3");
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }.start();

    }

    private void initCpusets() {
        mTop = mSCM.readSysFsOri("/dev/cpuset/top-app/cpus");
        Log.d(TAG, "top cpus is  " + mTop);
        mForeground = mSCM.readSysFsOri("/dev/cpuset/foreground/cpus");
        Log.d(TAG, "foreground cpus is  " + mForeground);
        mBackground = mSCM.readSysFsOri("/dev/cpuset/background/cpus");
        Log.d(TAG, "background cpus is  " + mBackground);
        mSystem = mSCM.readSysFsOri("/dev/cpuset/system-background/cpus");
        Log.d(TAG, "system background cpus is  " + mSystem);
        mRestricted = mSCM.readSysFsOri("/dev/cpuset/restricted/cpus");
        Log.d(TAG, "restricted cpus is  " + mRestricted);
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        Log.d(TAG, "DroidLogicCpuService is oncreate");
        mSCM = SystemControlManager.getInstance();
        Log.d(TAG, "start setCpusetsDefault: 0-3");
        initCpusets();
        setCpusetsDefault();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
    }
}
