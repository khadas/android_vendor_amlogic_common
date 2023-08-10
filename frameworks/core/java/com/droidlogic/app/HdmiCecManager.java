/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC HdmiCecManager
 */

package com.droidlogic.app;

import android.content.Context;
import android.content.ContentResolver;
import android.hardware.hdmi.HdmiControlManager;
import android.hardware.hdmi.HdmiTvClient;
import android.provider.Settings;
import android.util.Log;

public class HdmiCecManager {
    private static final String TAG = "HdmiCecManager";

    // As the string values in framework Settings.java is with hide annotation, give a copy here
    public static final String SETTINGS_HDMI_CONTROL_ENABLED = "hdmi_control_enabled";
    public static final String SETTINGS_ONE_TOUCH_PLAY = "hdmi_control_one_touch_play_enabled";
    public static final String SETTINGS_AUTO_POWER_OFF = "hdmi_control_auto_device_off_enabled";
    public static final String SETTINGS_AUTO_LANGUAGE_CHANGE = "hdmi_control_auto_language_change_enabled";
    public static final String SETTINGS_AUTO_WAKE_UP = "hdmi_control_auto_wakeup_enabled";
    public static final String SETTINGS_ARC_ENABLED = "hdmi_system_audio_control_enabled";
    public static final String SETTINGS_HDMI_VOLUME_CONTROL = "hdmi_control_volume_control_enabled";
    public static final String SETTINGS_EARC_ENABLE = "earc_enable";

    // Prop used for hdmi cec hal as it can't directly read system prop or Settings.
    public static final String PERSIST_HDMI_CEC_SET_MENU_LANGUAGE = "persist.vendor.sys.cec.set_menu_language";
    public static final String PERSIST_HDMI_CEC_DEVICE_AUTO_POWEROFF = "persist.vendor.sys.cec.deviceautopoweroff";
    public static final String PERSIST_HDMI_CEC_AUTO_WAKEUP = "persist.vendor.sys.cec.autowakeup";
    public static final String PERSIST_HDMI_CEC_ONE_TOUCH_PLAY = "persist.vendor.sys.cec.onetouchplay";

    public static final int ON = 1;
    public static final int OFF = 0;

    private Context mContext;
    private HdmiControlManager mHdmiControlManager;
    private HdmiTvClient mTvClient;

    public HdmiCecManager(Context context) {
        mContext = context;
        mHdmiControlManager = context.getSystemService(HdmiControlManager.class);
        if (mHdmiControlManager == null) {
            Log.e(TAG, "cec service does not exist, no cec settings is needed!");
            return;
        }
        mTvClient = mHdmiControlManager.getTvClient();
    }

    public boolean isTv() {
        return mTvClient != null;
    }

    public boolean isHdmiControlEnabled() {
        if (mHdmiControlManager == null) {
            return false;
        }
        return mHdmiControlManager.getHdmiCecEnabled() == ON;
    }

    public boolean isOneTouchPlayEnabled() {
        return readValue(SETTINGS_ONE_TOUCH_PLAY);
    }

    public boolean isAutoPowerOffEnabled() {
        if (mHdmiControlManager == null) {
            return false;
        }
        if (mTvClient != null) {
            return mHdmiControlManager.getTvSendStandbyOnSleep() == ON;
        }
        return !HdmiControlManager.POWER_CONTROL_MODE_NONE
            .equals(mHdmiControlManager.getPowerControlMode());
    }

    public boolean isAutoWakeUpEnabled() {
        if (mHdmiControlManager == null) {
            return false;
        }
        return mHdmiControlManager.getTvWakeOnOneTouchPlay() == ON;
    }

    public boolean isAutoChangeLanguageEnabled() {
        return readValue(SETTINGS_AUTO_LANGUAGE_CHANGE, ON);
    }

    public boolean isArcEnabled() {
        if (mHdmiControlManager == null) {
            return false;
        }
        return mHdmiControlManager.getSystemAudioControl() == ON;
    }

    public boolean isVolumeControlEnabled() {
        if (mHdmiControlManager == null) {
            return false;
        }
        return mHdmiControlManager.getHdmiCecVolumeControlEnabled() == ON;
    }

    public void enableHdmiControl(boolean value) {
        if (mHdmiControlManager == null) {
            return;
        }
        //writeValue(SETTINGS_HDMI_CONTROL_ENABLED, value);
        mHdmiControlManager.setHdmiCecEnabled(value ? ON : OFF);
    }

    public void enableVolumeControl(boolean value) {
        if (mHdmiControlManager == null) {
            return;
        }
        mHdmiControlManager.setHdmiCecVolumeControlEnabled(value ? ON : OFF);
    }

    public void enableOneTouchPlay(boolean value) {
        writeValue(SETTINGS_ONE_TOUCH_PLAY, value);
    }

    public void enableAutoPowerOff(boolean value) {
        //writeValue(SETTINGS_AUTO_POWER_OFF, value);
        if (mHdmiControlManager == null) {
            return;
        }
        if (mTvClient != null) {
            mHdmiControlManager.setTvSendStandbyOnSleep(value ? ON : OFF);
        } else {
            mHdmiControlManager.setPowerControlMode(value
                ? HdmiControlManager.POWER_CONTROL_MODE_BROADCAST
                : HdmiControlManager.POWER_CONTROL_MODE_NONE);
        }
    }

    public void enableAutoWakeUp(boolean value) {
        //writeValue(SETTINGS_AUTO_WAKE_UP, value);
        if (mHdmiControlManager == null) {
            return;
        }
        mHdmiControlManager.setTvWakeOnOneTouchPlay(value ? ON : OFF);
    }

    public void enableAutoChangeLanguage(boolean value) {
        writeValue(SETTINGS_AUTO_LANGUAGE_CHANGE, value);
    }

    public void enableArc(boolean value) {
        if (mHdmiControlManager == null) {
            return;
        }
        mHdmiControlManager.setSystemAudioControl(value ? ON : OFF);
    }

    public boolean isEarcEnabled() {
         return Settings.Global.getInt(mContext.getContentResolver(), SETTINGS_EARC_ENABLE, ON) == ON;
     }

     public void enableEarc(boolean value) {
         Log.d(TAG, "enable eARC Audio : " + value);
         Settings.Global.putInt(mContext.getContentResolver(), SETTINGS_EARC_ENABLE, (value ? ON : OFF));
     }

    private boolean readValue(String key) {
        return readValue(key, ON);
    }

    private boolean readValue(String key, int def) {
        if (null == mContext) {
            Log.e(TAG, "readValue context null!");
            return false;
        }
        return Settings.Global.getInt(mContext.getContentResolver(), key, def) == ON;
    }

    private void writeValue(String key, boolean value) {
        if (null == mContext) {
            Log.e(TAG, "writeValue context null!");
            return;
        }
        Settings.Global.putInt(mContext.getContentResolver(), key, value ? ON : OFF);
    }
}

