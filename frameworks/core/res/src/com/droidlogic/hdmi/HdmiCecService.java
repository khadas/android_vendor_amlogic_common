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
import android.hardware.hdmi.HdmiControlManager.VendorCommandListener;
import android.hardware.hdmi.HdmiClient;
import android.hardware.hdmi.HdmiPlaybackClient;
import android.net.Uri;
import android.provider.Settings;
import android.provider.Settings.Global;
import android.os.Handler;
import android.os.IBinder;
import android.os.UserHandle;
import android.util.Log;
import android.widget.Toast;

import java.util.Arrays;

import com.droidlogic.R;

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

    private VendorCommandHandler mVendorCommandHandler;
    private SettingsObserver mSettingsObserver;

    @Override
    public void onCreate() {
        Log.d(TAG, "onCreate");
        mHdmiControlManager = (HdmiControlManager)this.getSystemService(Context.HDMI_CONTROL_SERVICE);
        if (null == mHdmiControlManager) {
            return;
        }
        mVendorCommandHandler = new VendorCommandHandler(this);
        mSettingsObserver = new SettingsObserver(mHandler);
        registerObserver();
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
                        mCallbackHandler.post(()->{
                            updateActiveState(active);
                        });
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

    private void registerObserver() {
        ContentResolver resolver = this.getContentResolver();
        String[] settings = new String[] {
            SettingsObserver.CEC_ENABLE_ADB
        };
        for (String s : settings) {
            resolver.registerContentObserver(Global.getUriFor(s), false, mSettingsObserver,
                    UserHandle.USER_ALL);
        }

        mVendorCommandHandler.registerVendorCommandListener();
    }

    private class SettingsObserver extends ContentObserver {
    static final String CEC_ENABLE_ADB = "cec_enable_adb";

        public SettingsObserver(Handler handler) {
            super(handler);
        }

        // onChange is set up to run in service thread.
        @Override
        public void onChange(boolean selfChange, Uri uri) {
            String option = uri.getLastPathSegment();
            boolean enabled = Global.getInt(HdmiCecService.this.getContentResolver(), option, 0) == 1;
            Log.d(TAG, "onChange " + option + " enabled:" + enabled);
            switch (option) {
                case CEC_ENABLE_ADB:
                    if (enabled) {
                        mVendorCommandHandler.sendEnableAdbVendorCommand();
                    }
                    break;
            }
        }
    }

    private class VendorCommandHandler {

        static final byte VENDOR_CMD_ENABLE_ADB = 0x1;
        static final byte VENDOR_PARAM_STATUS_ON = 0x1;
        static final byte VENDOR_PARAM_STATUS_OFF = 0x0;
        static final int VENDOR_ID_DEFAULT = 0x1CA410;

        // 1c a4 10 01 01
        static final int LENGTH_VENDOR_CMD_PARAMS = 5;

        static final int[] LOGICAL_ADDRESS_PLAYBACK = {0x4, 0x8, 0xb};
        static final byte[] VENDOR_PARAMS_ADB_ENABLED = {VENDOR_CMD_ENABLE_ADB, VENDOR_PARAM_STATUS_ON};

        Context mContext;
        HdmiControlManager mHcm;
        HdmiClient mHdmiClient;
        VendorCommandListener mListener;

        public VendorCommandHandler(Context context) {
            mContext = context;;
            mHcm = (HdmiControlManager)context.getSystemService(Context.HDMI_CONTROL_SERVICE);
            if (mHcm == null) {
                return;
            }
            mHdmiClient = mHcm.getTvClient();
            if (mHdmiClient == null) {
                mHdmiClient = mHcm.getPlaybackClient();
            }
            if (mHdmiClient == null) {
                Log.w(TAG, "Can't get any cec client!");
                return;
            }
            mListener = new VendorCommandListener() {
                public void onReceived(int srcAddress, int destAddress, byte[] params, boolean hasVendorId) {
                    Log.d(TAG, "onReceived params:" + Arrays.toString(params) + " has vendor id:" + hasVendorId);
                    if (!hasVendorId) {
                        return;
                    }
                    if (params.length != LENGTH_VENDOR_CMD_PARAMS) {
                        return;
                    }
                    int vendorId = threeBytesToInt(params);
                    Log.d(TAG, "vendor command vendor id " + String.format("%x", vendorId));
                    if (vendorId != VENDOR_ID_DEFAULT) {
                        Log.w(TAG, "Not expected vendor id " + String.format("%x", VENDOR_ID_DEFAULT));
                        return;
                    }
                    int code = (int)(params[LENGTH_VENDOR_CMD_PARAMS - 2] & 0xFF);
                    int state = (int)(params[LENGTH_VENDOR_CMD_PARAMS - 1] & 0xFF);
                    Log.d(TAG, "onReceived vendor command code:" + code + " state:" + state);
                    switch (code) {
                        case VENDOR_CMD_ENABLE_ADB:
                            Log.d(TAG, "enable local usb debugging");
                            mHandler.post(()->{
                                Toast.makeText(mContext, R.string.adb_enabled, Toast.LENGTH_LONG).show();
                            });
                            Global.putInt(mContext.getContentResolver(), Global.ADB_ENABLED, state);
                            break;
                    }
                }

                public void onControlStateChanged(boolean enabled, int reason) {
                }
            };

            Log.d(TAG, "VendorCommandHandler initialized");
        }

        public void registerVendorCommandListener() {
            if (mHdmiClient == null) {
                return;
            }
            Log.d(TAG, "registerVendorCommandListener");
            mHdmiClient.setVendorCommandListener(mListener, VENDOR_ID_DEFAULT);
        }

        public void sendEnableAdbVendorCommand() {
            Log.d(TAG, "sendEnableAdbVendorCommand");
            if (mHdmiClient == null) {
                return;
            }
            for (int address: LOGICAL_ADDRESS_PLAYBACK) {
                mHdmiClient.sendVendorCommand(address, VENDOR_PARAMS_ADB_ENABLED, true);
            }
        }

        private int threeBytesToInt(byte[] data) {
            return ((data[0] & 0xFF) << 16) | ((data[1] & 0xFF) << 8) | (data[2] & 0xFF);
        }
    }
}

