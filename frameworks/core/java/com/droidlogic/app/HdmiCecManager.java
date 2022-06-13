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

    // Prop used for hdmi cec hal as it can't directly read system prop or Settings.
    public static final String PERSIST_HDMI_CEC_SET_MENU_LANGUAGE = "persist.vendor.sys.cec.set_menu_language";
    public static final String PERSIST_HDMI_CEC_DEVICE_AUTO_POWEROFF = "persist.vendor.sys.cec.deviceautopoweroff";
    public static final String PERSIST_HDMI_CEC_AUTO_WAKEUP = "persist.vendor.sys.cec.autowakeup";
    public static final String PERSIST_HDMI_CEC_ONE_TOUCH_PLAY = "persist.vendor.sys.cec.onetouchplay";

    public static final int ON = 1;
    public static final int OFF = 0;

    private Context mContext;
    //private SystemControlManager mSystemControlManager;

    public HdmiCecManager(Context context) {
        mContext = context;
        //mSystemControlManager = SystemControlManager.getInstance();
    }

    public boolean isHdmiControlEnabled() {
        return readValue(SETTINGS_HDMI_CONTROL_ENABLED);
    }

    public boolean isOneTouchPlayEnabled() {
        return readValue(SETTINGS_ONE_TOUCH_PLAY);
    }

    public boolean isAutoPowerOffEnabled(boolean def) {
        return readValue(SETTINGS_AUTO_POWER_OFF, def ? ON : OFF);
    }

    public boolean isAutoPowerOffEnabled() {
        return readValue(SETTINGS_AUTO_POWER_OFF);
    }

    public boolean isAutoWakeUpEnabled() {
        return readValue(SETTINGS_AUTO_WAKE_UP);
    }

    public boolean isAutoChangeLanguageEnabled() {
        return readValue(SETTINGS_AUTO_LANGUAGE_CHANGE);
    }

    public boolean isArcEnabled() {
        return readValue(SETTINGS_ARC_ENABLED);
    }

    public void enableHdmiControl(boolean value) {
        writeValue(SETTINGS_HDMI_CONTROL_ENABLED, value);
    }

    public void enableOneTouchPlay(boolean value) {
        writeValue(SETTINGS_ONE_TOUCH_PLAY, value);
        /*mSystemControlManager.setProperty(PERSIST_HDMI_CEC_ONE_TOUCH_PLAY,
                value ? "true" : "false");*/
    }

    public void enableAutoPowerOff(boolean value) {
        writeValue(SETTINGS_AUTO_POWER_OFF, value);
        /*mSystemControlManager.setProperty(PERSIST_HDMI_CEC_DEVICE_AUTO_POWEROFF,
                value ? "true" : "false");*/
    }

    public void enableAutoWakeUp(boolean value) {
        writeValue(SETTINGS_AUTO_WAKE_UP, value);
        /*mSystemControlManager.setProperty(PERSIST_HDMI_CEC_AUTO_WAKEUP,
                value ? "true" : "false");*/
    }

    public void enableAutoChangeLanguage(boolean value) {
        writeValue(SETTINGS_AUTO_LANGUAGE_CHANGE, value);
        /*mSystemControlManager.setProperty(PERSIST_HDMI_CEC_SET_MENU_LANGUAGE,
                value ? "true" : "false");*/
    }

    public void enableArc(boolean value) {
        writeValue(SETTINGS_ARC_ENABLED, value);
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

