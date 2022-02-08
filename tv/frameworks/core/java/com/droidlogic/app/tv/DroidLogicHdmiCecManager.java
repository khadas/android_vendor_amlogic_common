/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.app.tv;

import android.content.Context;
import android.content.ContentResolver;
import android.content.pm.PackageManager;
import android.database.ContentObserver;
import android.hardware.hdmi.HdmiControlManager;
import android.hardware.hdmi.HdmiDeviceInfo;
import android.hardware.hdmi.HdmiClient;
import android.hardware.hdmi.HdmiTvClient;
import android.hardware.hdmi.HdmiTvClient.SelectCallback;
import android.hardware.hdmi.HdmiSwitchClient;
import android.hardware.hdmi.HdmiSwitchClient.OnSelectListener;

import android.net.Uri;
import android.os.Handler;
import android.os.Message;
import android.provider.Settings;
import android.provider.Settings.Global;
import android.media.tv.TvInputHardwareInfo;
import android.media.tv.TvInputManager;
import android.media.tv.TvInputInfo;
import android.util.Log;
import android.view.KeyEvent;

import java.util.List;
import java.util.ArrayList;
import java.util.Collections;

public class DroidLogicHdmiCecManager {
    private static final String TAG = "DroidLogicHdmiCecManager";

    private static final String HDMI_CONTROL_ENABLED = "hdmi_control_enabled";
    private static final String FEATURE_HDMI_CEC = "android.hardware.hdmi.cec";

    private static final int ENABLED = 1;

    private static final int DEVICE_SELECT_PROTECTION_TIME = 1500;

    private static final int MSG_DEVICE_SELECT = 0;
    private static final int MSG_PORT_SELECT = 1;
    private static final int MSG_SELECT_PROTECTION = 2;
    private static final int MSG_SEND_KEY_EVENT = 3;
    private static final int MSG_SWITCH_SELECT = 4;

    private static DroidLogicHdmiCecManager mInstance;

    private Context mContext;
    private HdmiControlManager mHdmiControlManager;
    private HdmiTvClient mTvClient;
    private HdmiSwitchClient mSwitchClient;
    private HdmiClient mClient;
    private TvInputManager mTvInputManager;
    private TvControlDataManager mTvControlDataManager;
    private TvControlManager mTvControlManager;

    private SelectDeviceInfo mCurrentSelect;
    private SelectDeviceInfo mSelectingDevice;

    private final SelectDeviceInfo INTERNAL_DEVICE = new SelectDeviceInfo(HdmiDeviceInfo.ADDR_INTERNAL);

    // If this class is used then there is tv feature, but cec not sure.
    private boolean mHasCecFeature;

    // There might be a case where the old session onSetMain false is called after the new one on
    // onSetMain true. Even though we hope this never happens, we still add a protetion mechanism.
    private boolean mInSelectProtection;

    private int mKeyCodeMediaPlayPauseCount;

    private final Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_DEVICE_SELECT:
                    mTvClient.deviceSelect(mCurrentSelect.getLogicalAddress(), mSelectCallback);
                    break;
                case MSG_PORT_SELECT:
                    mTvClient.portSelect(mCurrentSelect.getPortId(), mSelectCallback);
                    break;
                case MSG_SELECT_PROTECTION:
                    mInSelectProtection = false;
                    break;
                case MSG_SEND_KEY_EVENT:
                    mClient.sendKeyEvent((int)msg.arg1, (boolean)msg.obj);
                    break;
                case MSG_SWITCH_SELECT:
                    mSwitchClient.selectPort(mCurrentSelect.getPortId(), mSelectListener);
                    break;
                default:
                    break;
            }
        }
    };

    private final SelectCallback mSelectCallback = new SelectCallback() {
        @Override
        public void onComplete(int result) {
            Log.d(TAG, "select onComplete result = " + result);
        }
    };

    private final OnSelectListener mSelectListener = new OnSelectListener() {
        @Override
        public void onSelect(int result) {
            Log.d(TAG, "select onSelect result = " + result);
        }
    };

    public static synchronized DroidLogicHdmiCecManager getInstance(Context context) {
        if (mInstance == null) {
            mInstance = new DroidLogicHdmiCecManager(context);
        }
        return mInstance;
    }

    private DroidLogicHdmiCecManager(Context context) {
        mContext = context;
        mTvInputManager = (TvInputManager) context.getSystemService(Context.TV_INPUT_SERVICE);
        mTvControlDataManager = TvControlDataManager.getInstance(mContext);

        PackageManager pm = context.getPackageManager();
        mHasCecFeature = pm.hasSystemFeature(FEATURE_HDMI_CEC);
        Log.i(TAG, "cec feature exist:" + mHasCecFeature);
        if (!mHasCecFeature) {
            return;
        }

        mHdmiControlManager = (HdmiControlManager) context.getSystemService(Context.HDMI_CONTROL_SERVICE);
        mTvClient = mHdmiControlManager.getTvClient();
        mSwitchClient = mHdmiControlManager.getSwitchClient();
        if (mTvClient != null) {
            mClient = mTvClient;
            Log.i(TAG, "get tv client.");
        } else {
            // Customers might demands the remote of a soundbar could send key events.
            mClient = mHdmiControlManager.getClient(HdmiDeviceInfo.DEVICE_AUDIO_SYSTEM);
            if (mClient == null) {
                Log.e(TAG, "no tv or audio system client, are you sure the cec function is all right?");
            }
        }

        registerSettingsObserver();

        InputChangeAdapter.getInstance(mContext);
    }

    /**
     * 1. Device select or Port Select
     * a. hdmi device id
     * b. onSetMain true
     * c. HdmiDeviceInfo in TvInputInfo is null? deviceSelect : portSelect.

     * 2. When to call internal address 0
     * This is when there is no hdmi device's Session onSetMain true. We could do it like this:
     * a. hdmi device id
     * b. onSetMain false
     * c. exclude the special case in which there is no need.

     * 3. There are 4 cases in which hdmi device's Session onSetMain false is called.
     * a. Tune from hdmi device A to hdmi B.--> onSetMain hdmi true remove old messages.
     * b. Tune from hdmi device to none hdmi device. -->onSetMain false message is not removed.
     * c. Quit LiveTv to MboxLauncher which has no TvView of TV source.-->onSetMain false message is not removed.
     * d. Tune from LiveTV's hdmi device A to MboxLauncher's same one.---> If the device id is the same but
     *    session id is not, and in this scenario the old Session's onSetMain false might happens after the
     *    new one has been callded onSetMain true, we should abort calling internal address.
     */
    public void onSetMain(boolean isMain, String inputId, int deviceId, int sessionId) {
        if (!mHasCecFeature) {
            Log.v(TAG, "onSetMain no cec then no need.");
            return;
        }

        mSelectingDevice = new SelectDeviceInfo(inputId, deviceId, sessionId);

        if (mTvClient == null) {
            Log.e(TAG, "onSetMain tv client null.");
            onSetMainForSoundbar(isMain, inputId, deviceId, sessionId);
            return;
        }

        if (isHdmiDeviceId(deviceId)) { // Hdmi device
            if (isMain) {
                TvInputInfo info = mTvInputManager.getTvInputInfo(inputId);
                if (info == null) {
                    Log.e(TAG, "onSetMain can't get tv input info!");
                    return;
                }
                // Always previously use deviceSelect if we could get the logical address of the source.
                // For projects like Amazon Fireos, it should directly only use the HdmiDeviceInfo
                // In the TvInputInfo to do deviceSelect, as to solve the auto jump issue.
                HdmiDeviceInfo hdmiDevice = info.getHdmiDeviceInfo();
                if (hdmiDevice == null) {
                    hdmiDevice = getHdmiDeviceInfo(inputId);
                }

                if (hdmiDevice != null) {
                    Log.d(TAG, "onSetMain hdmi device " + hdmiDevice);
                    mSelectingDevice.setLogicalAddress(hdmiDevice.getLogicalAddress());
                    mCurrentSelect = mSelectingDevice;
                    // case 0: device select hdmi
                    deviceSelect();
                } else {
                    // case 1: port select hdmi
                    int portId = getPortIdByDeviceId(deviceId);
                    mSelectingDevice.setPortId(portId);
                    mCurrentSelect = mSelectingDevice;
                    portSelect();
                }
            } else {
                // case 3: leave hdmi, try to device select 0
                if (deviceId == mCurrentSelect.getDeviceId() && (sessionId != mCurrentSelect.getSessionId())) {
                    // Same hdmi but different session id. In this case the sequence of setMain is not for sure.
                    return;
                }
                mSelectingDevice = INTERNAL_DEVICE;
                deviceSelectInternalDelayed();
            }
        } else if (isMain) {
            mCurrentSelect = INTERNAL_DEVICE;
            deviceSelect();
        }
    }

    private void onSetMainForSoundbar(boolean isMain, String inputId, int deviceId, int sessionId) {
        if (mSwitchClient == null) {
            Log.e(TAG, "onSetMain switch client null.");
            return;
        }
        if (isHdmiDeviceId(deviceId) && isMain) { // Hdmi device
            int portId = getPortIdByDeviceId(deviceId);
            mSelectingDevice.setPortId(portId);
            mCurrentSelect = mSelectingDevice;
            Log.d(TAG, "onSetMainForSoundbar " + mCurrentSelect);
            mHandler.removeMessages(MSG_SWITCH_SELECT);
            mHandler.sendMessage(mHandler.obtainMessage(MSG_SWITCH_SELECT));
        }
    }

    public String getCurrentInput() {
        String currentInput = "";
        if (mCurrentSelect != null) {
            currentInput = mCurrentSelect.getInputId();
        }
        return currentInput;
    }

    private void registerSettingsObserver() {
        DroidLogicSettingsObserver observer = new DroidLogicSettingsObserver(mHandler);
        String[] settings = new String[] {
            HDMI_CONTROL_ENABLED
        };
        for (String s : settings) {
            mContext.getContentResolver().registerContentObserver(Global.getUriFor(s), false, observer);
        }
    }

    /**
    * generally used to switch source.
    */
    private void deviceSelect() {
        Log.d(TAG, "deviceSelect " + mCurrentSelect);
        removePreviousMessages();
        mHandler.sendMessage(mHandler.obtainMessage(MSG_DEVICE_SELECT));

        mInSelectProtection = true;
        mHandler.sendEmptyMessageDelayed(MSG_SELECT_PROTECTION, DEVICE_SELECT_PROTECTION_TIME);
    }

    private void deviceSelectInternalDelayed() {
        if (mInSelectProtection) {
            // If there has just been a valid hdmi tune action, then we should protect it.
            Log.e(TAG, "selectHdmiDevice protection time and no select internal address");
            return;
        }
        // No remove previous select messages in here.
        mCurrentSelect = mSelectingDevice;
        Log.d(TAG, "deviceSelectInternalDelayed " + mCurrentSelect);

        mHandler.sendMessageDelayed(mHandler.obtainMessage(MSG_DEVICE_SELECT), DEVICE_SELECT_PROTECTION_TIME);
    }

    /**
     * only used in special senarios where can't get logical address like
     * open cec switch. Tv will not do the tune action and the hdmi device
     * list has not been created for the connected devices.
     */
    private void portSelect() {
        Log.d(TAG, "portSelect " + mCurrentSelect);

        removePreviousMessages();
        mHandler.sendMessage(mHandler.obtainMessage(MSG_PORT_SELECT));

        mInSelectProtection = true;
        mHandler.sendEmptyMessageDelayed(MSG_SELECT_PROTECTION, DEVICE_SELECT_PROTECTION_TIME);
    }

    /**
     * When there is a new device select request, it's need to remove the previous
     * request first. A customed senario is that when user switches to a different
     * channel, there is a deviceSelect 0 first and then a deviceSelect 4, we need
     * to make sure the deviceSelect 0 is not finally performed so that there are
     * not so many meaningless routing messages.
     */
    private void removePreviousMessages() {
        mHandler.removeMessages(MSG_DEVICE_SELECT);
        mHandler.removeMessages(MSG_PORT_SELECT);
        mHandler.removeMessages(MSG_SELECT_PROTECTION);
    }

    public HdmiDeviceInfo getHdmiDeviceInfo(String iputId) {
        List<TvInputInfo> tvInputList = mTvInputManager.getTvInputList();
        for (TvInputInfo info : tvInputList) {
            HdmiDeviceInfo hdmiDeviceInfo = info.getHdmiDeviceInfo();
            if (hdmiDeviceInfo != null) {
                if (iputId.equals(info.getId()) || iputId.equals(info.getParentId())) {
                    return hdmiDeviceInfo;
                }
            }
        }
        return null;
    }

    public void setDeviceIdForCec(int deviceId){
        // Give cec hal a chance to filter strange <Active Source>
        Log.d(TAG, "setDeviceIdForCec " + deviceId);
        TvControlManager.getInstance().setDeviceIdForCec(deviceId);
    }

    public int getPortIdByDeviceId(int deviceId) {
        List<TvInputHardwareInfo> hardwareList = mTvInputManager.getHardwareList();
        if (hardwareList == null || hardwareList.size() == 0) {
            return -1;
        }

        for (TvInputHardwareInfo hardwareInfo : hardwareList) {
            if (deviceId == hardwareInfo.getDeviceId()) {
                return hardwareInfo.getHdmiPortId();
            }
        }
        return -1;
    }

    public int getInputSourceDeviceId() {
        return  mTvControlDataManager.getInt(mContext.getContentResolver(), DroidLogicTvUtils.TV_CURRENT_DEVICE_ID, 0);
    }

    public boolean isHdmiDeviceId(int deviceId) {
        return deviceId >= DroidLogicTvUtils.DEVICE_ID_HDMI1
                && deviceId <= DroidLogicTvUtils.DEVICE_ID_HDMI4;
    }

    public boolean sendKeyEvent(int keyCode, boolean isPressed) {
        if (!mHasCecFeature) {
            Log.v(TAG, "sendKeyEvent no cec feature");
            return false;
        }

        if (mClient == null) {
            Log.e(TAG, "sendKeyEvent hdmi client null!");
            return false;
        }
        if (KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE == keyCode) {
            // The play/pause key of TV RCU is keycode of KEYCODE_MEDIA_PLAY_PAUSE, and it's translated into
            // CEC_KEYCODE_PAUSE in HdmiCecKeycode. This will cause behaviours out of control.
            if ((mKeyCodeMediaPlayPauseCount % 2) == 0) {
                keyCode = KeyEvent.KEYCODE_MEDIA_PAUSE;
            } else {
                keyCode = KeyEvent.KEYCODE_MEDIA_PLAY;
            }
            if (isPressed) {
                mKeyCodeMediaPlayPauseCount++;
            }
        }
        Log.d(TAG, "sendKeyEvent keycode:" + keyCode + " pressed:" + isPressed);

        Message msg = mHandler.obtainMessage(MSG_SEND_KEY_EVENT, keyCode, 0, isPressed);
        mHandler.sendMessage(msg);
        return true;
    }

    private class SelectDeviceInfo {
        String inputId = "";
        int deviceId;
        int sessionId;

        int logicalAddress;
        int portId;

        SelectDeviceInfo(int logicalAddress) {
            this.logicalAddress = logicalAddress;
        }

        SelectDeviceInfo(String inputId, int deviceId, int sessionId) {
            this.inputId = inputId;
            this.deviceId = deviceId;
            this.sessionId = sessionId;
        }

        String getInputId() {
            return inputId;
        }

        int getDeviceId() {
            return deviceId;
        }

        int getSessionId() {
            return sessionId;
        }

        void setLogicalAddress(int logicalAddress) {
            this.logicalAddress = logicalAddress;
        }

        int getLogicalAddress() {
            return logicalAddress;
        }

        void setPortId(int portId) {
            this.portId = portId;
        }

        int getPortId() {
            return portId;
        }

        @Override
        public String toString() {
            StringBuffer sb = new StringBuffer();
            sb.append("current selected device info ");
            sb.append("inputId: ").append(inputId).append(" ")
              .append("deviceId: ").append(deviceId).append(" ")
              .append("sessionId: ").append(sessionId).append(" ")
              .append("logicalAddress: ").append(logicalAddress).append(" ")
              .append("portId: ").append(portId);
            return sb.toString();
        }
    }

    private class DroidLogicSettingsObserver extends ContentObserver {
        public DroidLogicSettingsObserver(Handler handler) {
            super(handler);
        }

        @Override
        public void onChange(boolean selfChange, Uri uri) {
            String option = uri.getLastPathSegment();
            boolean enabled = Global.getInt(mContext.getContentResolver(),
                                option, ENABLED) == ENABLED;
            Log.d(TAG, "cec switch changes to " + enabled);
            switch (option) {
                case HDMI_CONTROL_ENABLED:
                    if (!enabled) {
                        // no need to do supplement select
                        return;
                    }
                    if (mTvClient == null) {
                        Log.d(TAG, "tv client null.");
                        return;
                    }
                    if (mTvClient == null) {
                        Log.d(TAG, "tv client null.");
                        return;
                    }
                    if (mCurrentSelect == null) {
                        Log.d(TAG, "cec enabled while current select is null.");
                        return;
                    }
                    if (isHdmiDeviceId(mCurrentSelect.getDeviceId())) {
                        // In accord with the device channel LiveTv tuned.
                        Log.d(TAG, "cec settings is enabled! " + mCurrentSelect);
                        if (mCurrentSelect.getLogicalAddress() != 0) {
                            deviceSelect();
                        } else if (mCurrentSelect.getPortId() != 0) {
                            portSelect();
                        }
                    }
                    break;
            }
        }
    }
}
