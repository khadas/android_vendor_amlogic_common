/*
 * Copyright (C) 2009 The Android Open Source Project
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
 * limitations under the License.
 */

package com.droidlogic.displaycontrollerservice;

import android.app.Service;
import android.content.Context;

import android.content.ContentResolver;
import android.provider.Settings;
import android.database.ContentObserver;
import android.content.Intent;
import android.os.IBinder;
import android.os.Handler;
import android.os.RemoteException;
import android.util.Log;
import android.net.Uri;

public class MultiDisplayService extends Service {
    private static final String TAG = "MM-Service";
    private static final String RVC_RUNNING = "rvc_running";
    private DisplayControllerService mDisplayControlService;
    private MultiDisplayController mController;
    private ContentObserver mSettingsObserver;
    private Context mContext;

    @Override
    public void onCreate() {
        super.onCreate();
        mContext = getApplicationContext();
        mDisplayControlService = new DisplayControllerService();
        mController = new MultiDisplayController(mContext);
        mSettingsObserver = new SettingsValueChangeContentObserver();
        getContentResolver().registerContentObserver(Settings.Global.getUriFor(RVC_RUNNING),
                false, mSettingsObserver);
        Log.d(TAG, "onCreate");
    }

    @Override
    public IBinder onBind(Intent intent) {
        Log.d(TAG, "onBind");
        return mDisplayControlService;
    }

    @Override
    public void onDestroy() {
        Log.d(TAG, "onDestroy");
        getContentResolver().unregisterContentObserver(mSettingsObserver);
        super.onDestroy();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.d(TAG, "onStartCommand");
        return super.onStartCommand(intent, flags, startId);
    }

    class DisplayControllerService extends IMirrorDisplayInterface.Stub {
        @Override
        public boolean startMirror(int displayId, int tDisplayId, boolean control) throws RemoteException {
            Log.d(TAG, "startMirror" + displayId + " to" + tDisplayId);
            return mController.mirroringDisplay(displayId, tDisplayId, control);
        }

        @Override
        public boolean isMirrored(int displayId) throws RemoteException {
            Log.d(TAG, "isMirrored" + displayId);
            return mController.isMapping(displayId);
        }

        @Override
        public void stopMirror(int displayId) throws RemoteException {
            Log.d(TAG, "stopMirror---" + displayId);
            mController.stopMapping(displayId);
        }

        @Override
        public boolean swithDisplay(int displayId, int tDisplayId) throws RemoteException {
            Log.d(TAG, "swithDisplay" + displayId + " to" + tDisplayId);
            return mController.switchDisplay(displayId, tDisplayId);
        }

        @Override
        public int getPhyPort(int displayId) {
            Log.d(TAG, "getPhyPort" + displayId);
            return mController.getPhyPort(displayId);
        }

        public int getMirroredId(int displayId) {
            Log.d(TAG, "getMirroredId " + displayId);
            return mController.getMirroredId(displayId);
        }

        @Override
        public boolean isMirroring(int displayId) throws RemoteException {
            Log.d(TAG, "isMirroring" + displayId);
            return mController.beMirroring(displayId);
        }

        @Override
        public boolean isControlled(int displayId)throws RemoteException {
            Log.d(TAG, "isControlled" + displayId);
            return mController.isControlled(displayId);
        }
    }
    private class SettingsValueChangeContentObserver extends ContentObserver {
        private final Uri rvcRunningUri = Settings.Global.getUriFor(RVC_RUNNING);

        SettingsValueChangeContentObserver() {
            super(new Handler());
        }

        // onChange is set up to run in service thread.
        @Override
        public void onChange(boolean selfChange, Uri uri) {

            Log.i(TAG, "Uri: " + uri);
            if (rvcRunningUri.equals(uri)) {
                boolean rvcRunning = (Settings.Global.getInt(getContentResolver(),
                        RVC_RUNNING, 0) > 0);
                Log.d(TAG, "rvc_running: " + rvcRunning);
                if (rvcRunning) {
                    mController.stopMirrorIfMirrored();
                }
             }
         }

    };
}