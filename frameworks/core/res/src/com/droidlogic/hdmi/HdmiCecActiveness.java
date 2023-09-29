/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.droidlogic.hdmi;

import android.content.Context;
import android.content.ContentResolver;
import android.provider.Settings.Global;

import android.util.Slog;

import org.json.JSONObject;
import org.json.JSONException;


/**
 * Notify the activeness of a playback
 * HDMI-CEC Activeness​integration is required in NRDP 5.2. Following doc provides the guideline
 * for Android TV Partner to integrate HDMI-CEC Activeness for Netflix.
 * Ninja uses "nrdp_video_platform_capabilities" settings to signal video output related events and
 * capabilities. The "nrdp_video_platform_capabilities" setting can be updated by invoking
 * Settings.Global.putString(getContentResolver(), "nrdp_video_platform_capabilities", jsonCaps)
 * jsonCaps is a JSON string, for example: {"activeCecState":"active", "xxx":"yyy"}
 * Ninja APK uses “activeCecState” key in “nrdp_video_platform_capabilities” json value for
 * HDMI-CEC Activeness integration. Android/Fire TV partners must report the correct
 * “activeCecState” value in “nrdp_video_platform_capabilities” json if it’s HDMI source devices
 * and device’s Ninja Validation Version >= ninja_7.
 * Following table describes the supported JSON Keys for “nrdp_video_platform_capabilities”:
 *
 * Accepted values:
 * ● "active": cecState dpi set to CEC_ACTIVE, dpi supportCecActiveVideo set to true
 * ● "inactive": cecState dpi set to CEC_INACTIVE, dpi supportCecActiveVideo set to true
 * ● "unknown": cecState set to CEC_NOT_APPLICABLE, dpi supportCecActiveVideo set to true
 * ● no activeCecState value in "nrdp_video_platform_capabilities" json string: cecState set to
 * CEC_NOT_APPLICABLE, dpi supportCecActiveVideo set to false
 * ● other value: has the same effect as no activeCecState value
 *
 * notes:
 * 1) If the device doesn't support HDMI-CEC integration or it’s non HDMI source source devices(e.g.
 * smart TV), activeCecState should not be set.
 * 2) If the device supports HDMI-CEC integration and it’s HDMI source devices(e.g. set-top-boxes a
 * sticks), activeCecState should be set to "active", "inactive" (or “unknown).
 * 3) HDMI-CEC integration is mandatory for ​HDMI source devices​with Ninja Version >= ninja_7
 */
public class HdmiCecActiveness {
    private static final String TAG = "HdmiCecActiveness";

    private static final String HDMI_ACTIVENESS_KEY = "activeCecState";

    public static final String CEC_ACTIVE = "active";
    public static final String CEC_INACTIVE = "inactive";
    public static final String CEC_NOT_APPLICABLE = "unknown";
    public static final String CEC_DISABLED = "disabled";

    private static final String SETTINGS_CEC_ACTIVENESS = "nrdp_video_platform_capabilities";
    private static final String URI_CEC_ACTIVENESS = "nrdp_video_platform_capabilities/activeCecState";

    private String CEC_ACTIVENESS_ACTIVE_JSON = "";
    private String CEC_ACTIVENESS_INACTIVE_JSON = "";
    private String CEC_ACTIVENESS_UNKNOWN_JSON = "";
    private static final String CEC_ACTIVENESS_DISABLED_JSON = "";

    private Context mContext;
    private String mState;

    public HdmiCecActiveness(Context context) {
        mContext = context;
        init();
    }

    public void setState(boolean state) {
        setState(state ? CEC_ACTIVE : CEC_INACTIVE);
    }

    public void setState(String state) {
        Slog.d(TAG, "setState " + state + " mState:" + mState);
        if (state.equals(mState)) {
            return;
        }
        mState = state;
        String jsonValue = getJsonValue(state);
        ContentResolver cr = mContext.getContentResolver();
        Global.putString(cr, SETTINGS_CEC_ACTIVENESS, jsonValue);
        // In ContentService#dispatch method, if the process state is less than
        // ActivityManager.PROCESS_STATE_IMPORTANT_FOREGROUND, then there could be dispatched with a
        // 10 seconds delay. And finally it will result in a failure in the Netflix testing app.
        cr.notifyChange(Global.getUriFor(URI_CEC_ACTIVENESS), null, ContentResolver.NOTIFY_NO_DELAY);
    }

    private void init() {
        try {
            JSONObject activeness = new JSONObject();
            activeness.put(HDMI_ACTIVENESS_KEY, CEC_ACTIVE);
            CEC_ACTIVENESS_ACTIVE_JSON = activeness.toString();

            activeness.remove(HDMI_ACTIVENESS_KEY);
            activeness.put(HDMI_ACTIVENESS_KEY, CEC_INACTIVE);
            CEC_ACTIVENESS_INACTIVE_JSON = activeness.toString();

            activeness.remove(HDMI_ACTIVENESS_KEY);
            activeness.put(HDMI_ACTIVENESS_KEY, CEC_NOT_APPLICABLE);
            CEC_ACTIVENESS_UNKNOWN_JSON = activeness.toString();

        } catch(JSONException e) {
            Slog.e(TAG, "init HdmiCecActiveness json fail " + e);
        }

        setState(CEC_ACTIVE);
    }

    private String getJsonValue(String state) {
        String activeState = "";
        switch (state) {
            case CEC_ACTIVE:
                activeState = CEC_ACTIVENESS_ACTIVE_JSON;
                break;
            case CEC_INACTIVE:
                activeState = CEC_ACTIVENESS_INACTIVE_JSON;
                break;
            case CEC_NOT_APPLICABLE:
                activeState = CEC_ACTIVENESS_UNKNOWN_JSON;
                break;
            case CEC_DISABLED:
                activeState =CEC_ACTIVENESS_DISABLED_JSON;
                break;
            default:
                break;
        }
        return activeState;
    }

}
