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
import android.content.Intent;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;

public class MultiDisplayService extends Service {
    private static final String TAG = "MM-Service";
    private DisplayControllerService mDisplayControlService;
    private MultiDisplayController mController;
    private Context mContext;

    @Override
    public void onCreate() {
        super.onCreate();
        mContext = getApplicationContext();
        mDisplayControlService = new DisplayControllerService();
        mController = new MultiDisplayController(mContext);
        Log.d(TAG,"onCreate");
    }

    @Override
    public IBinder onBind(Intent intent) {
        Log.d(TAG,"onBind");
        return mDisplayControlService;
    }

    @Override
    public void onDestroy() {
        Log.d(TAG,"onDestroy");
        super.onDestroy();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.d(TAG,"onStartCommand");
        return super.onStartCommand(intent, flags, startId);
    }

    class DisplayControllerService extends IMirrorDisplayInterface.Stub {
        @Override
        public boolean startMirror(int displayId, int tDisplayId) throws RemoteException {
            Log.d(TAG,"startMirror"+displayId+" to"+tDisplayId);
            return mController.mirroringDisplay(displayId, tDisplayId);
        }

       @Override
        public boolean isMirrored(int displayId) throws RemoteException {
            Log.d(TAG,"isMirrored"+displayId);
            return mController.isMapping(displayId);
        }

        @Override
        public void stopMirror(int displayId) throws RemoteException {
            Log.d(TAG,"stopMirror" + displayId);
            mController.stopMapping(displayId);
        }

        @Override
        public boolean swithDisplay(int displayId, int tDisplayId) throws RemoteException {
            Log.d(TAG,"swithDisplay"+displayId+" to"+tDisplayId);
            return mController.switchDisplay(displayId,tDisplayId);
        }
    }
}