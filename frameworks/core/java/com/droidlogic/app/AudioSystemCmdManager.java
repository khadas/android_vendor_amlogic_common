/*
 * Copyright (C) 2021 Amlogic Corporation.
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

package com.droidlogic.app;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;
import com.droidlogic.audioservice.services.IAudioSystemCmdService;

public class AudioSystemCmdManager {
    private String TAG = "AudioSystemCmdManager";
    public static final String SERVICE_PACKEGE_NANME = "com.droidlogic";
    public static final String SERVICE_NANME = "com.droidlogic.audioservice.services.AudioSystemCmdService";
    private IAudioSystemCmdService mAudioSystemCmdService = null;
    private Context mContext;

    public static final int AUDIO_SERVICE_CMD_START_DECODE                          = 1;
    public static final int AUDIO_SERVICE_CMD_PAUSE_DECODE                          = 2;
    public static final int AUDIO_SERVICE_CMD_RESUME_DECODE                         = 3;
    public static final int AUDIO_SERVICE_CMD_STOP_DECODE                           = 4;
    public static final int AUDIO_SERVICE_CMD_SET_DECODE_AD                         = 5;
    public static final int AUDIO_SERVICE_CMD_SET_VOLUME                            = 6;
    public static final int AUDIO_SERVICE_CMD_SET_MUTE                              = 7;
    public static final int AUDIO_SERVICE_CMD_SET_OUTPUT_MODE                       = 8;
    public static final int AUDIO_SERVICE_CMD_SET_PRE_GAIN                          = 9;
    public static final int AUDIO_SERVICE_CMD_SET_PRE_MUTE                          = 10;
    public static final int AUDIO_SERVICE_CMD_OPEN_DECODER                          = 12;
    public static final int AUDIO_SERVICE_CMD_CLOSE_DECODER                         = 13;
    public static final int AUDIO_SERVICE_CMD_SET_DEMUX_INFO                        = 14;
    public static final int AUDIO_SERVICE_CMD_SET_SECURITY_MEM_LEVEL                = 15;
    public static final int AUDIO_SERVICE_CMD_SET_HAS_VIDEO                         = 16;
    public static final int AUDIO_SERVICE_CMD_SET_MEDIA_SYCN_ID                     = 17;

    //audio ad
    public static final int AUDIO_SERVICE_CMD_AD_DUAL_SUPPORT                       = 20;
    public static final int AUDIO_SERVICE_CMD_AD_MIX_SUPPORT                        = 21;
    public static final int AUDIO_SERVICE_CMD_AD_MIX_LEVEL                          = 22;
    public static final int AUDIO_SERVICE_CMD_AD_SET_MAIN                           = 23;
    public static final int AUDIO_SERVICE_CMD_AD_SET_ASSOCIATE                      = 24;

    public static final int AUDIO_SERVICE_CMD_SET_MEDIA_PRESENTATION_ID             = 25;
    public static final int AUDIO_SERVICE_CMD_SET_AUDIO_PATCH_MANAGE_MODE           = 26;

    private static AudioSystemCmdManager mInstance;

    public static AudioSystemCmdManager getInstance(Context context) {
        if (null == mInstance) {
            mInstance = new AudioSystemCmdManager(context);
        }
        return mInstance;
    }

    public AudioSystemCmdManager(Context context) {
        mContext = context;
        Log.i(TAG, "construction AudioSystemCmdManager");
        getService();
    }

    private void getService() {
        Log.i(TAG, "=====[getService]");
        int retry = 10;
        boolean mIsBind = false;
        try {
            synchronized (this) {
                while (true) {
                    Intent intent = new Intent();
                    intent.setAction(SERVICE_NANME);
                    intent.setPackage(SERVICE_PACKEGE_NANME);
                    mIsBind = mContext.bindService(intent, serConn, mContext.BIND_AUTO_CREATE);
                    Log.i(TAG, "=====[getService] mIsBind: " + mIsBind + ", retry:" + retry);
                    if (mIsBind || retry <= 0) {
                        break;
                    }
                    retry --;
                    Thread.sleep(500);
                }
            }
        } catch (InterruptedException e){}
    }

    private ServiceConnection serConn = new ServiceConnection() {
        @Override
        public void onServiceDisconnected(ComponentName name) {
            Log.i(TAG, "[onServiceDisconnected] mAudioSystemCmdService: " + mAudioSystemCmdService);
            mAudioSystemCmdService = null;

        }
        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            mAudioSystemCmdService = IAudioSystemCmdService.Stub.asInterface(service);
            Log.i(TAG, "[onServiceConnected] mAudioSystemCmdService: " + mAudioSystemCmdService);
        }
    };

    public void unBindService() {
        mContext.unbindService(serConn);
    }

    public static String AudioCmdToString(int cmd) {
        String temp = "["+cmd+"]";
        switch (cmd) {
            case AUDIO_SERVICE_CMD_START_DECODE:
                return temp + "START_DECODE";
            case AUDIO_SERVICE_CMD_PAUSE_DECODE:
                return temp + "PAUSE_DECODE";
            case AUDIO_SERVICE_CMD_RESUME_DECODE:
                return temp + "RESUME_DECODE";
            case AUDIO_SERVICE_CMD_STOP_DECODE:
                return temp + "STOP_DECODE";
            case AUDIO_SERVICE_CMD_SET_DECODE_AD:
                return temp + "SET_DECODE_AD";
            case AUDIO_SERVICE_CMD_SET_VOLUME:
                return temp + "SET_VOLUME";
            case AUDIO_SERVICE_CMD_SET_MUTE:
                return temp + "SET_MUTE";
            case AUDIO_SERVICE_CMD_SET_OUTPUT_MODE:
                return temp + "SET_OUTPUT_MODE";
            case AUDIO_SERVICE_CMD_SET_PRE_GAIN:
                return temp + "SET_PRE_GAIN";
            case AUDIO_SERVICE_CMD_SET_PRE_MUTE:
                return temp + "SET_PRE_MUTE";
            case AUDIO_SERVICE_CMD_OPEN_DECODER:
                return temp + "OPEN_DECODER";
            case AUDIO_SERVICE_CMD_CLOSE_DECODER:
                return temp + "CLOSE_DECODER";
            case AUDIO_SERVICE_CMD_SET_DEMUX_INFO:
                return temp + "SET_DEMUX_INFO";
            case AUDIO_SERVICE_CMD_SET_SECURITY_MEM_LEVEL:
                return temp + "SET_SECURITY_MEM_LEVEL";

            case AUDIO_SERVICE_CMD_AD_DUAL_SUPPORT:
                return temp + "AD_DUAL_SUPPORT";
            case AUDIO_SERVICE_CMD_AD_MIX_SUPPORT:
                return temp + "AD_MIX_SUPPORT";
            case AUDIO_SERVICE_CMD_AD_MIX_LEVEL:
                return temp + "AD_MIX_LEVEL";
            case AUDIO_SERVICE_CMD_AD_SET_MAIN:
                return temp + "AD_SET_MAIN";
            case AUDIO_SERVICE_CMD_AD_SET_ASSOCIATE:
                return temp + "AD_SET_ASSOCIATE";
            case AUDIO_SERVICE_CMD_SET_HAS_VIDEO:
                return temp + "SET_HAS_VIDEO";
            case AUDIO_SERVICE_CMD_SET_MEDIA_PRESENTATION_ID:
                return temp + "SET_MEDIA_PRESENTATION_ID";
            default:
                return temp + "invalid cmd";
        }
    }

    private boolean audioCmdServiceIsNull() {
        if (mAudioSystemCmdService == null) {
            Log.w(TAG, "mAudioSystemCmdService is null, pls check stack:");
            Log.w(TAG, Log.getStackTraceString(new Throwable()));
            return true;
        } else {
            return false;
        }
    }

    public void setParameters(String arg) {
        if (audioCmdServiceIsNull()) return;
        try {
            mAudioSystemCmdService.setParameters(arg);
        } catch (RemoteException e) {
            Log.e(TAG, "setParameters failed:" + e);
        }
    }

    public String getParameters(String arg) {
        if (audioCmdServiceIsNull()) return "";
        try {
            return mAudioSystemCmdService.getParameters(arg);
        } catch (RemoteException e) {
            Log.e(TAG, "getParameters failed:" + e);
            return "";
        }
    }

    public void handleAdtvAudioEvent(int cmd, int param1, int param2) {
        if (audioCmdServiceIsNull()) return;
        try {
            mAudioSystemCmdService.handleAdtvAudioEvent(cmd, param1, param2);
        } catch (RemoteException e) {
            Log.e(TAG, "handleAdtvAudioEvent failed:" + e);
        }
    }

    public void openTvAudio(int sourceType) {
        if (audioCmdServiceIsNull()) return;
        try {
            mAudioSystemCmdService.openTvAudio(sourceType);
        } catch (RemoteException e) {
            Log.e(TAG, "openTvAudio failed:" + e);
        }
    }

    public void closeTvAudio() {
        if (audioCmdServiceIsNull()) return;
        try {
            mAudioSystemCmdService.closeTvAudio();
        } catch (RemoteException e) {
            Log.e(TAG, "closeTvAudio failed:" + e);
        }
    }
}
