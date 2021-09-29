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

import java.util.HashMap;
import java.util.Random;

public class AiPqService extends Service {
    public static final String AIPQ  = "ai_pq";
    private final IBinder mBinder = new AiPqBinder();
    public static final HashMap<String, String> mScene = new HashMap<String, String>();
    public static int mThreadId = -1;
    private static final int EVENT_UPDATE_UI = 1;
    private static final int R_EVENT_INIT = 1;
    private static final int R_EVENT_READ = 2;
    private static final int TIMEDELAY  = 16;
    private static boolean enabled;
    private static AIRemoteView mRemoteView;
    private Handler mHandler;
    private static final String OTHER_SCENE = "7";
    private static final String SCENE_FS = "/sys/class/video/cur_ai_scenes";
    private static final String AIPQ_CONFG_FILE = "/vendor/etc/scenes_data.txt";
    private static final String MAX_SCENE = "MAX_SCENE";
    private SystemControlManager mSystemControlManager;
    private static HandlerThread workThread = new  HandlerThread("aipq_work");
    private static WorkHandler workHandler;
    private static boolean initial = false;
    private final static String NOTIFICATION_CHANNEL_ID = "CHANNEL_ID";
    private final static String NOTIFICATION_CHANNEL_NAME = "CHANNEL_NAME";
    private final static int FOREGROUND_ID = 0;

    public AiPqService() {
        if (!initial || workHandler.getLooper() == null) {
            initial = false;
            workThread.start();
            workHandler = new WorkHandler(workThread.getLooper());
            mSystemControlManager = SystemControlManager.getInstance();
            mRemoteView = AIRemoteView.getInstance();
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
            workHandler.sendEmptyMessage(R_EVENT_INIT);
        }
    }

    private void initScienceTree() {
        String scenesval = mSystemControlManager.getAipqTable();
        Log.d("AIPQ_TABLE", "getAipqTable: " + scenesval);
        initial = true;
        Log.d(AIPQ,"scenesval-->"+scenesval+"<--");
        if (scenesval != null && scenesval.contains(":")) {
            String readScense[] = scenesval.split(":");
            int j=0;
            for (int i=0;i<readScense.length;i++) {
                if (readScense[i] == null || readScense[i].trim() == null/*
                        || MAX_SCENE.equals( readScense[i].trim())*/
                        || readScense[i].trim().length() == 0)
                    continue;
                mScene.put(j+"",readScense[i].trim());
                j++;
            }
        }
    }

    class WorkHandler extends Handler{
        public WorkHandler(Looper looper) {
            super(looper);
        }
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case R_EVENT_INIT:
                    initScienceTree();
                    break;
                case R_EVENT_READ:
                    String scenseVal = mSystemControlManager.readSysFs(SCENE_FS);
                    updateUI(scenseVal);

                    workHandler.sendEmptyMessageDelayed(R_EVENT_READ,TIMEDELAY);
                    break;
            }
        }
    };

    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    public class AiPqBinder extends Binder {
        AiPqService getService() {
            return AiPqService.this;
        }
    }

    private void updateRemoveView(String value) {
        mRemoteView.updateUI(value);
    }

    public boolean isShowing() {
        return mRemoteView != null && mRemoteView.isShow();
    }

    public void enableAipq() {
        Log.d("V", "enableAipq" + enabled);
        if (!enabled) {
            enabled = true;
            showAipqTopView(true);
            if (!initial) {
                workHandler.sendEmptyMessage(R_EVENT_INIT);
            }
            workHandler.sendEmptyMessageDelayed(R_EVENT_READ,TIMEDELAY);
        }
    }

    public void disableAipq() {
        Log.d("V", "disableAipq" + enabled);
        if (enabled) {
            enabled = false;
            workHandler.removeMessages(R_EVENT_READ);
            showAipqTopView(false);
        }else {
            workHandler.removeMessages(R_EVENT_READ);
        }
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        mSystemControlManager = SystemControlManager.getInstance();
        SharedPreferences DealData = PreferenceManager.getDefaultSharedPreferences(this);
        Boolean value = DealData.getBoolean(AiPqFragment.KEY_ENABLE_AIPQ_INFO, false);
        if (mSystemControlManager.getAipqEnable()) {
            enableAipq();
        }

        Notification notification = new Notification(FOREGROUND_ID, NOTIFICATION_CHANNEL_NAME, System.currentTimeMillis());
        PendingIntent pendingintent = PendingIntent.getActivity(this, 0, intent, 0);
        notification.setLatestEventInfo(this, "aipq_info", "startForeground", pendingintent);
        startForeground(FOREGROUND_ID, notification);
        return START_NOT_STICKY;
    }

    public void updateUI(String ValStr) {
        String newVal = updateAipqValue(ValStr);
        if (newVal == null || newVal.length() <= 0) {
            newVal = AiPqService.this.getResources().getString(R.string.prepare);
        }
        if (newVal != null && newVal.length() > 0) {
            Message msg = mHandler.obtainMessage();
            msg.what = EVENT_UPDATE_UI;
            msg.obj = newVal;
            mHandler.sendMessage(msg);
        }
    }
    @Override
    public void onCreate() {
        super.onCreate();
    }

    @Override
    public void onDestroy() {
        stopForeground(true);
        super.onDestroy();
    }

    private String updateAipqValue(String ValStr) {
        if (ValStr == null || ValStr.isEmpty()) {return "";}
        String[] sciences = ValStr.split(";");
        StringBuilder str = new StringBuilder();
        for (String science : sciences) {
            if (!science.contains(":") && mScene.get(science.trim()) != null) {
                str.append(mScene.get(science.trim()) + ":" + 0 + "%" + "  ");
            } else if (science.contains(":")) {
                String[] par = science.split("\\:");
                if (par.length != 2) continue;
                //Log.d("TAG","par[0]="+par[0]);
                if (par[0].equals(OTHER_SCENE)) {
                    continue;
                }
                double val = 0;
                try {
                    val = 1.0 * Double.valueOf(par[1]);
                    val = val / 100;
                } catch (NumberFormatException ex) {
                    ex.printStackTrace();
                    val = 0;
                } finally {
                    if (mScene.get(par[0].trim()) != null) {
                        str.append(mScene.get(par[0].trim()) + ":" + val + "%" + "\n");
                    }
                }

            }

        }
        return str.toString();
    }
    public boolean infoEnabled() {
        return enabled;
    }
    //display top view
    private void showAipqTopView(boolean show) {
        Log.d("V", "mRemoteView.isCreated()" + mRemoteView.isCreated() + "mRemoteView" + mRemoteView);
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
