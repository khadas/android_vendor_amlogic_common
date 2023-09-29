/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC HdmiCecService
 */

package com.droidlogic.hdmi;

import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.database.ContentObserver;
import android.hardware.hdmi.HdmiControlManager;
import android.hardware.hdmi.HdmiPlaybackClient;
import android.net.Uri;
import android.provider.Settings;
import android.os.Handler;
import android.os.IBinder;
import android.util.Log;


public class HdmiCecService extends Service {
    private static final String TAG = "HdmiCecService";
    private static final boolean DEBUG = Log.isLoggable(TAG, Log.DEBUG);

    private static final int MESSAGE_VENDOR_COMMAND = 0x89;

    private static final int ACTIVENESS_STATE_ON = 1;
    private static final int ACTIVENESS_STATE_OFF = 0;

    // The length of vendor specific message is permanent 3.
    private static final int LENGTH_VENDOR_COMMAND = 3;
    // VENDOR_COMMAND of nts activeness.
    private static final int VENDOR_CMD_ACTIVENESS = 1;

    /** Logical address reserved for future usage */
    public static final int ADDR_BACKUP_1 = 12;
    /** Logical address used in the destination address field for broadcast messages */
    public static final int ADDR_BROADCAST = 15;

    // Used to match the vendor specific message
    private static final String LANG_VENDOR_CALLBACK = "aml";
    // Netflix feature for stb device
    private static final String FEATURE_SOFTWARE_NETFLIX = "droidlogic.software.netflix";

    private HdmiControlManager mHdmiControlManager;
    private HdmiPlaybackClient mPlayback;
    private HdmiCecAidlClient mHdmiCecAidlClient;

    private boolean mIsActive = true;
    private Handler mHandler = new Handler();
    private HdmiCecServiceCallback mCecServiceCallback;

    private HdmiCecActiveness mActiveness;

    private final HdmiCecReceiver mHdmiCecReceiver = new HdmiCecReceiver();

    @Override
    public void onCreate() {
        Log.d(TAG, "onCreate");
        mHdmiControlManager = (HdmiControlManager)this.getSystemService(Context.HDMI_CONTROL_SERVICE);
        if (null == mHdmiControlManager) {
            return;
        }
        mPlayback = mHdmiControlManager.getPlaybackClient();
        if (null == mPlayback) {
            Log.d(TAG, "It's none playback device");
            return;
        }
        if (!getPackageManager().hasSystemFeature(FEATURE_SOFTWARE_NETFLIX) && (!DEBUG)) {
            Log.i(TAG, "Netflix feature is not supported");
            return;
        }
        mHdmiCecAidlClient = new HdmiCecAidlClient();
        if (!mHdmiCecAidlClient.connectToHal()) {
            Log.e(TAG, "Failed to be onnected with cec service!");
            mHdmiCecAidlClient = null;
            return;
        }

        mHdmiCecAidlClient.setLanguage(LANG_VENDOR_CALLBACK);
        mCecServiceCallback = new HdmiCecServiceCallback(mHandler);
        mHdmiCecAidlClient.setCallback(mCecServiceCallback);

        mActiveness = new HdmiCecActiveness(this);

        registerReceiver();
    }

    @Override
    public void onDestroy() {
        Log.d(TAG, "onDestroy");
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    private void updateActiveState(boolean active) {
        Log.d(TAG, "updateActiveState active:" + active + " old:" + mIsActive);
        mIsActive = active;
        mActiveness.setState(active);
    }

    private void registerReceiver() {
        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_SCREEN_OFF);
        //filter.addAction(Intent.ACTION_SCREEN_ON);
        registerReceiver(mHdmiCecReceiver, filter);
    }

    private class HdmiCecServiceCallback implements HdmiCecAidlClient.CecCallback {
        Handler mCallbackHandler;
        public HdmiCecServiceCallback(Handler handler) {
            mCallbackHandler = handler;
        }

        public void onCecMessage(int initiator, int destination, byte[] body) {
            // The opcode needs to be processed with & 0xff.
            int opcode = body[0] & 0xff;
            if (DEBUG) {
                Log.d(TAG, String.format("onCecMessage 0x%X 0x%X 0x:%02X len:%d",
                            initiator, destination, opcode, body.length));
            }
            switch (opcode) {
                case MESSAGE_VENDOR_COMMAND:
                    if (initiator != ADDR_BACKUP_1 // 0xc 12
                        || destination != ADDR_BROADCAST // 0xf 15
                        || body.length != LENGTH_VENDOR_COMMAND) { // 0x89
                        return;
                    }
                    int vendorCmd = body[1] & 0xff;
                    int value = body[2] & 0xff;
                    if (DEBUG) {
                        Log.d(TAG, "Got new cmd:" + vendorCmd + " value:" + value);
                    }
                    // The activeness vendor command should be "cf 89 01 01" or "cf 89 01 00"
                    if (vendorCmd == VENDOR_CMD_ACTIVENESS) {
                        final boolean active = value == ACTIVENESS_STATE_ON;
                        //Log.d(TAG, "Got new active state:" + active);
                        updateActiveState(active);
                    }
                    break;
            }
        }
    }

    private class HdmiCecReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            switch (intent.getAction()) {
                case Intent.ACTION_SCREEN_OFF:
                    Log.d(TAG, "screen off");
                    updateActiveState(false);
                    break;
            }
        }
    }
}

