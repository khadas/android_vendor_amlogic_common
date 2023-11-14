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

package com.droidlogic;

import android.app.Service;
import android.content.Intent;
import android.database.ContentObserver;
import android.os.Binder;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.provider.Settings;
import android.util.Log;

import android.content.Context;
import android.database.ContentObserver;
import android.net.Uri;
import android.os.Handler;
import android.provider.Settings.SettingNotFoundException;
import android.provider.Settings;

import com.droidlogic.app.SystemControlManager;

import java.util.HashMap;
import com.droidlogic.R;

public class AiPqService extends Service {
    public static final String TAG = "AiPqService";
    private final IBinder mBinder = new AiPqBinder();
    public static final HashMap<String, String> mScene = new HashMap<String, String>();
    public static int mThreadId = -1;
    private static final int EVENT_UPDATE_UI = 1;
    private static final int R_EVENT_INIT = 1;
    private static final int R_EVENT_READ = 2;
    private static final int TIMEDELAY  = 16;
    private static boolean mEnabled;
    private static AIRemoteView mRemoteView;
    private Handler mHandler;
    private static final String OTHER_SCENE = "7";
    private static final String SCENE_FS = "/sys/class/video/cur_ai_scenes";
    private static final String AIPQ_CONFIG_FILE = "/vendor/etc/scenes_data.txt";
    private static final String MAX_SCENE = "MAX_SCENE";
    private SystemControlManager mSystemControlManager;
    private static HandlerThread workThread = new  HandlerThread("aipq_work");
    private static WorkHandler workHandler;
    private static boolean initial = false;

    private static final int AIPQ_ENABLE = 1;
    private static final int AIPQ_DISABLE = 2;
    private static final int AIPQ_IS_SHOW = 3;
    private static final int AIPQ_INFO_ENABLED = 4;
    private static final String SAVE_AIPQ = "AIPQ";
    private SettingsValueChangeContentObserver mContentOb;
    private static final String PROP_AIPQ_ENABLE = "persist.vendor.sys.aipq.info";

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
        initial = true;
        Log.d(TAG, "scenesval-->"+scenesval+"<--");
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
                    String newVal = updateAipqValue(scenseVal);

                    updateUI(newVal);

                    workHandler.sendEmptyMessageDelayed(R_EVENT_READ,TIMEDELAY);
                    break;
            }
        }
    };

    public class SettingsValueChangeContentObserver extends ContentObserver {
        public SettingsValueChangeContentObserver() {
            super( new Handler());
        }
        @Override
        public void onChange(boolean selfChange) {
            Log.d(TAG,"[SettingsValueChangeContentObserver] onchange = " + Settings.Global.getInt(getContentResolver(), SAVE_AIPQ, 0));
            super.onChange(selfChange);
            switch (Settings.Global.getInt(getContentResolver(), SAVE_AIPQ, 0)) {
                case AIPQ_ENABLE:
                    enableAipq();
                    break;
                case AIPQ_DISABLE:
                    disableAipq();
                    break;
            }
        }
    }

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
        Log.d(TAG, "enableAipq, curenableAipq: " + mEnabled);
        if (!mEnabled) {
            mEnabled = true;
            showAipqTopView(true);
            if (!initial) {
                workHandler.sendEmptyMessage(R_EVENT_INIT);
            }
            workHandler.sendEmptyMessageDelayed(R_EVENT_READ,TIMEDELAY);
        }
    }

    public void disableAipq() {
        Log.d(TAG, "disableAipq, curenableAipq: " + mEnabled);
        if (mEnabled) {
            mEnabled = false;
            workHandler.removeMessages(R_EVENT_READ);
            showAipqTopView(false);
        }else {
            workHandler.removeMessages(R_EVENT_READ);
        }
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if (mSystemControlManager.getPropertyBoolean(PROP_AIPQ_ENABLE, false)
            && mSystemControlManager.getAipqEnable()
            && mSystemControlManager.hasAipqFunc()) {
            enableAipq();
        }
        Log.d(TAG, "[AIPQservice] onStartCommand");
//        return START_NOT_STICKY;
        return START_REDELIVER_INTENT;
    }

    public void updateUI(String newVal) {
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
        Log.i(TAG, "[AIPQservice]  onCreate");

        mContentOb = new SettingsValueChangeContentObserver();
        getContentResolver().registerContentObserver(Settings.Global.getUriFor(SAVE_AIPQ),false, mContentOb);
        int sys_value = Settings.Global.getInt(getContentResolver(), SAVE_AIPQ, 0);
        Log.d(TAG, "[AIPQservice] onCreate sys_value= " + sys_value);
        if ((mSystemControlManager.getPropertyBoolean(PROP_AIPQ_ENABLE, false) == true) && (sys_value == 2)) {
            Settings.Global.putInt(getContentResolver(), SAVE_AIPQ, AIPQ_ENABLE);
            Log.d(TAG, "[AIPQservice] onCreate sys_value set1 ");
        }
        if ((mSystemControlManager.getPropertyBoolean(PROP_AIPQ_ENABLE, false) == false) && (sys_value == 1)) {
            Settings.Global.putInt(getContentResolver(), SAVE_AIPQ, AIPQ_DISABLE);
            Log.d(TAG, "[AIPQservice] onCreate sys_value set2 ");
        }
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        getContentResolver().unregisterContentObserver(mContentOb);
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
        return mEnabled;
    }
    //display top view
    private void showAipqTopView(boolean show) {
        Log.d(TAG, "mRemoteView.isCreated()" + mRemoteView.isCreated() + "mRemoteView" + mRemoteView);
        Log.d(TAG, "[showAipqTopView]  getPropertyBoolean:" + mSystemControlManager.getPropertyBoolean(PROP_AIPQ_ENABLE, false));
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
