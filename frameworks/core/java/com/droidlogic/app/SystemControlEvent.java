/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC SystemControlEvent
 */

package com.droidlogic.app;

import android.content.Context;
import android.content.Intent;
import android.os.UserHandle;
import android.util.Log;
import android.media.AudioManager;

import java.lang.reflect.Method;
import java.lang.reflect.InvocationTargetException;

import com.droidlogic.app.OutputModeManager;
import vendor.amlogic.hardware.systemcontrol.V1_0.ISystemControlCallback;

//this event from native system control service
public class SystemControlEvent extends ISystemControlCallback.Stub {
    private static final String TAG                             = "SystemControlEvent";

    public static final String ACTION_SYSTEM_CONTROL_EVENT      = "droidlogic.intent.action.SYSTEM_CONTROL_EVENT";
    public final static String ACTION_HDMI_PLUGGED              = "android.intent.action.HDMI_PLUGGED";
    public final static String EXTRA_HDMI_PLUGGED_STATE         = "state";
    public static final String EVENT_TYPE                       = "event";

    //must sync with DisplayMode.h
    public static final int EVENT_OUTPUT_MODE_CHANGE            = 0;
    public static final int EVENT_DIGITAL_MODE_CHANGE           = 1;
    public static final int EVENT_HDMI_PLUG_OUT                 = 2;
    public static final int EVENT_HDMI_PLUG_IN                  = 3;
    public static final int EVENT_HDMI_AUDIO_OUT                = 4;
    public static final int EVENT_HDMI_AUDIO_IN                 = 5;

    // AudioManager.DEVICE_OUT_HDMI
    public static final int DEVICE_OUT_AUX_DIGITAL              = 0x400;


    private Context mContext = null;
    private final AudioManager mAudioManager;

    private FBCUpgradeEventListener mFBCUpgradeEventListener = null;
    private DisplayModeListener     mDisplayModeListener     = null;
    private AudioEventListener      mAudioListener           = null;
    private HdrInfoListener         mHdrInfoListener         = null;

    public SystemControlEvent(Context context) {
        mContext = context;
        mAudioManager = (AudioManager)context.getSystemService(Context.AUDIO_SERVICE);
    }

    @Override
    public void notifyCallback(int event) {
        Log.i(TAG, "system control callback event: " + event);
        Intent intent;
        if (event == EVENT_HDMI_PLUG_OUT || event == EVENT_HDMI_PLUG_IN) {
            intent = new Intent(ACTION_HDMI_PLUGGED);
            boolean  plugged = (event - EVENT_HDMI_PLUG_OUT) ==1 ? true : false;
            intent.putExtra(EXTRA_HDMI_PLUGGED_STATE, plugged);
        } else if (event == EVENT_HDMI_AUDIO_OUT || event == EVENT_HDMI_AUDIO_IN) {
            setWiredDeviceConnectionState(DEVICE_OUT_AUX_DIGITAL, (event - EVENT_HDMI_AUDIO_OUT), "", "");
            //mAudioManager.setWiredDeviceConnectionState(AudioManager.DEVICE_OUT_HDMI, (event - EVENT_HDMI_AUDIO_OUT), "", "");
            return;
        }  else {
            intent = new Intent(ACTION_SYSTEM_CONTROL_EVENT);
            intent.putExtra(EVENT_TYPE, event);
            if (EVENT_OUTPUT_MODE_CHANGE == event) {
                setAudioStateWhenDisplayModeChanged();
            }
        }
        mContext.sendStickyBroadcastAsUser(intent, UserHandle.ALL);
    }

    public void notifySetDisplayModeCallback(int mode) {
        Log.d(TAG, "notifySetDisplayModeCallback mode:" + mode);
        if (mDisplayModeListener != null) {
            mDisplayModeListener.onSetDisplayMode(mode);
        } else {
            Log.e(TAG, "mDisplayModeListener is null");
        }
    }

    public interface DisplayModeListener {
        void onSetDisplayMode(int mode);
    }

    public void setDisplayModeListener(DisplayModeListener l) {
        mDisplayModeListener = l;
    }

    public void notifyHdrInfoChangedCallback(int newHdrInfo) {
        //Log.d(TAG, "notifyHdrInfoChangedCallback newHdrInfo:" + newHdrInfo);
        if (mHdrInfoListener != null) {
            mHdrInfoListener.onHdrInfoChange(newHdrInfo);
        } else {
            //Log.e(TAG, "mHdrInfoListener is null");
        }
    }

    public interface HdrInfoListener {
        void onHdrInfoChange(int newHdrInfo);
    }

    public void setHdrInfoListener(HdrInfoListener l) {
        mHdrInfoListener = l;
    }

    public void notifyFBCUpgradeCallback(int state, int param) {
        Log.i(TAG, "FBCUpgradeCallback: state: " + state + "param:" + param);
        if (mFBCUpgradeEventListener != null) {
            mFBCUpgradeEventListener.HandleFBCUpgradeEvent(state, param);
        } else {
            Log.e(TAG, "mFBCUpgradeEventListener is null");
        }
    }

    public void notifyAudioCallback(int param1, int param2, int param3, int param4) {
        Log.d(TAG, "notify audio callback param1:" + param1 + "param2:" + param2 + "param3:" + param3 + "param4:" + param4);
        if (mAudioListener != null) {
            mAudioListener.HandleAudioEvent(param1, param2, param3, param4);
        }
    }

    public interface AudioEventListener {
        void HandleAudioEvent(int param1, int param2, int param3, int param4);
    }

    public void SetAudioEventListener(AudioEventListener l) {
        mAudioListener = l;
    }

    public interface FBCUpgradeEventListener {
        void HandleFBCUpgradeEvent(int state, int param);
    }

    public void SetFBCUpgradeEventListener (FBCUpgradeEventListener l) {
        Log.d(TAG, "SetFBCUpgradeEventListener");
        mFBCUpgradeEventListener  = l;
    }


    private void setWiredDeviceConnectionState(int type, int state, String address, String name) {
        try {
            Class<?> audioManager = Class.forName("android.media.AudioManager");
            Method setwireState = audioManager.getMethod("setWiredDeviceConnectionState",
                                    int.class, int.class, String.class, String.class);
            Log.d(TAG,"setWireDeviceConnectionState "+setwireState);

            setwireState.invoke(mAudioManager, type, state, address, name);

        } catch(ClassNotFoundException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NoSuchMethodException ex) {
            ex.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }

    private void setAudioStateWhenDisplayModeChanged() {
        // get output size, if 480/576 then set audio param
        OutputModeManager outModeManager = OutputModeManager.getInstance(mContext);
        String strMode = outModeManager.getCurrentOutputMode();
        boolean ddpEnable = false;
        if (strMode.contains("480p") || strMode.contains("576p")) {
            ddpEnable = true;
        }
        Log.i(TAG, "Cur output mode=" + strMode +
            ", Prev DDP enable=" + outModeManager.getForceDDPEnable() +
            ", need set DDP enable=" + ddpEnable);
        if (outModeManager.getForceDDPEnable() != ddpEnable) {
            outModeManager.setForceDDPEnable(ddpEnable);
        }
    }
}
