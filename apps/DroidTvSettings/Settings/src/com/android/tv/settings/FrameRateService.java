/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License
 */

package com.android.tv.settings;

import android.app.Service;
import android.content.Intent;
import android.os.Binder;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.app.Notification;
import android.app.PendingIntent;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;

import com.droidlogic.app.SystemControlManager;

public class FrameRateService extends Service {
    public static final String TAG = "FrameRateService";
    private final IBinder mBinder = new FrameRateBinder();
    public static int mThreadId = -1;
    private static final int EVENT_UPDATE_UI = 1;
    private static final int R_EVENT_READ = 2;
    private static final int TIMEDELAY = 30;
    private static boolean mEnabled;
    private static FrameRateRemoteView mRemoteView;
    private Handler mHandler;

    private static final String SCENE_FS = "/sys/class/display/frame_rate";

    private SystemControlManager mSystemControlManager;
    private static HandlerThread workThread = new HandlerThread("FrameRateThread");
    private static WorkHandler workHandler;
    private static boolean initial = false;
    private final static String NOTIFICATION_CHANNEL_ID = "CHANNEL_ID";
    private final static String NOTIFICATION_CHANNEL_NAME = "CHANNEL_NAME";
    private final static int FOREGROUND_ID = 0;

    private static final String PROP_FRAMERATE_ENABLE = "persist.vendor.sys.framerate.enable";
    private static final String FRAMERATE_PROP = "persist.vendor.sys.framerate.feature";
    public FrameRateService() {
        if (workHandler == null || workHandler.getLooper() == null) {
            workThread.start();
            workHandler = new WorkHandler(workThread.getLooper());
            mSystemControlManager = SystemControlManager.getInstance();
            mRemoteView = FrameRateRemoteView.getInstance();
            mHandler = new Handler() {
                @Override
                public void handleMessage(Message msg) {
                    switch (msg.what) {
                        case EVENT_UPDATE_UI:
                            String text = (String) msg.obj;
                            updateRemoveView(text);
                            break;
                    }
                    super.handleMessage(msg);
                }
            };
        }
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    @Override
    public void onCreate() {
        super.onCreate();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        mSystemControlManager = SystemControlManager.getInstance();
        if (getFrameRateEnabled()) {
            enableFrameRate();
        }

        Notification notification = new Notification(FOREGROUND_ID, NOTIFICATION_CHANNEL_NAME, System.currentTimeMillis());
        PendingIntent pendingintent = PendingIntent.getActivity(this, 0, intent, 0);
        notification.setLatestEventInfo(this, "FrameRate", "startForeground", pendingintent);
        startForeground(FOREGROUND_ID, notification);
        return START_NOT_STICKY;
    }

    @Override
    public void onDestroy() {
        stopForeground(true);
        super.onDestroy();
    }

    class WorkHandler extends Handler {
        public WorkHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case R_EVENT_READ:
                    String scenseVal = mSystemControlManager.readSysFs(SCENE_FS);
                    updateUI(scenseVal);

                    workHandler.sendEmptyMessageDelayed(R_EVENT_READ, TIMEDELAY);
                    break;
            }
        }
    }

    public class FrameRateBinder extends Binder {
        FrameRateService getService() {
            return FrameRateService.this;
        }
    }

    public boolean isShowing() {
        return mRemoteView != null && mRemoteView.isShow();
    }

    public boolean getFrameRateEnabled() {
        return mSystemControlManager.getPropertyBoolean(PROP_FRAMERATE_ENABLE, false)
                && mSystemControlManager.getPropertyBoolean(FRAMERATE_PROP, false);
    }

    public void setFrameRateEnabled(final boolean enable) {
        if (enable) {
            enableFrameRate();
            mSystemControlManager.setProperty(PROP_FRAMERATE_ENABLE, "true");
        } else {
            disableFrameRate();
            mSystemControlManager.setProperty(PROP_FRAMERATE_ENABLE, "false");
        }
    }

    private void updateRemoveView(String value) {
        mRemoteView.updateUI(value);
    }

    private void enableFrameRate() {
        Log.d(TAG, "enableFrameRate" + mEnabled);
        if (!mEnabled) {
            mEnabled = true;
            showFrameRateTopView(true);
            workHandler.sendEmptyMessageDelayed(R_EVENT_READ, TIMEDELAY);
        }
    }

    private void disableFrameRate() {
        Log.d(TAG, "disableFrameRate" + mEnabled);
        if (mEnabled) {
            mEnabled = false;
            workHandler.removeMessages(R_EVENT_READ);
            showFrameRateTopView(false);
        } else {
            workHandler.removeMessages(R_EVENT_READ);
        }
    }

    public void updateUI(String ValStr) {
        Log.d(TAG, "updateUI ValStr" + ValStr);
        String newVal = ValStr;
        if (newVal == null || newVal.length() <= 0) {
            newVal = getResources().getString(R.string.frame_rate_prepare);
        }
        if (newVal != null && newVal.length() > 0) {
            Message msg = mHandler.obtainMessage();
            msg.what = EVENT_UPDATE_UI;
            msg.obj = newVal;
            mHandler.sendMessage(msg);
        }
    }

    //display top view
    private void showFrameRateTopView(boolean show) {
        Log.d(TAG, "mRemoteView.isCreated()" + mRemoteView.isCreated() + "mRemoteView" + mRemoteView);
        if (show) {
            if (!mRemoteView.isCreated()) {
                mRemoteView.createView(getApplicationContext());
            }
            mRemoteView.show();
        } else {
            mRemoteView.hide();
        }
    }

}
