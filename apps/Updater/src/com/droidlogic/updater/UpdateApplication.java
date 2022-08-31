package com.droidlogic.updater;


import android.app.Application;
import android.os.UpdateEngine;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;

import com.droidlogic.updater.util.PermissionUtils;

public class UpdateApplication extends Application {
    public static String TAG = "ABUpdate";
    private HandlerThread mWorkingThread;
    private UpdateManager mUpdateManager;
    private Handler mWorkHandler;
    public static boolean mRunningForce;
    public static boolean mRunningUpgrade;
    @Override
    public void onCreate() {
        super.onCreate();
        if (PermissionUtils.CanDebug()) Log.d(TAG,"APP:onCreate");
        mWorkingThread = new HandlerThread("update_engine");
        mWorkingThread.start();
        mWorkHandler = new Handler(mWorkingThread.getLooper());
        mUpdateManager = new UpdateManager(new UpdateEngine(), mWorkHandler);
        this.mUpdateManager.bind();
    }
    public UpdateManager getUpdateManager() {
        return mUpdateManager;
    }

    public Handler getWorkHandler() {
        return mWorkHandler;
    }

    @Override
    public void onTerminate() {
        super.onTerminate();
        if (PermissionUtils.CanDebug()) Log.d(TAG,"APP:onTerminate");
        mWorkingThread.quitSafely();
        this.mUpdateManager.unbind();
    }
}