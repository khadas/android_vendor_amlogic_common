/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC PQAdvancedColorCustomizeFragment
 */



package com.android.tv.settings.pqsettings.advanced;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.RemoteException;
import android.provider.Settings;
import androidx.preference.SwitchPreference;
import com.android.tv.settings.SettingsPreferenceFragment;
import androidx.preference.ListPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceScreen;
import androidx.preference.TwoStatePreference;
import androidx.preference.PreferenceCategory;
import android.util.ArrayMap;
import android.util.Log;
import android.text.TextUtils;

import com.droidlogic.app.DisplayPositionManager;
import com.android.tv.settings.R;
import com.android.tv.settings.RadioPreference;
import com.android.tv.settings.dialog.old.Action;
import com.android.tv.settings.SettingsConstant;
import com.android.tv.settings.pqsettings.PQSettingsManager;

import java.util.List;
import java.util.Map;
import java.util.ArrayList;

public class PQAdvancedColorCustomizeFragment extends SettingsPreferenceFragment {
    private static final String TAG = "PQAdvancedColorCustomizeFragment";
    private static final String PQ_ADVANCED_COLOR_CUSTOMIZE_ALLRESET = "pq_pictrue_advanced_color_customize_reset";

    //private final static int ZOOMINSTEP = 1;
    //private final static int ZOOMOUTSTEP = -1;

    //private final static int MAXBRIGHTNESSHEIGHT = 100;
    //private final static int MINBRIGHTNESSHEIGHT = 0;
    //private static final String PQ_BRIGHTNESS = "pq_brightness";
    //private static final String PQ_BRIGHTNESS_IN = "pq_brightness_in";
    //private static final String PQ_BRIGHTNESS_OUT = "pq_brightness_out";

    private PQSettingsManager mPQSettingsManager;

    //private Preference pq_brightnessPref;
    //private PreferenceCategory mPQBrightnessPref;
    //private Preference pq_brightnessinPref;
    //private Preference pq_brightnessoutPref;


    public static PQAdvancedColorCustomizeFragment newInstance() {
        return new PQAdvancedColorCustomizeFragment();
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        boolean isTv = SettingsConstant.needDroidlogicTvFeature(getActivity());
        setPreferencesFromResource(R.xml.pq_pictrue_advanced_color_customize, null);
        updateMainScreen();
    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        if (CanDebug()) Log.d(TAG, "[onPreferenceTreeClick] preference.getKey() = " + preference.getKey());
        switch (preference.getKey()) {
            case PQ_ADVANCED_COLOR_CUSTOMIZE_ALLRESET:
                Intent PQAdvancedColorCustomizeResetAllResetIntent = new Intent();
                PQAdvancedColorCustomizeResetAllResetIntent.setClassName(
                        "com.android.tv.settings",
                        "com.android.tv.settings.pqsettings.advanced.PQAdvancedColorCustomizeResetAllActivity");
                startActivity(PQAdvancedColorCustomizeResetAllResetIntent);
                break;
        }
        return super.onPreferenceTreeClick(preference);
    }

    @Override
    public int getMetricsCategory() {
        return 0;
    }

    private void updateMainScreen() {

    }

    private boolean CanDebug() {
        return PQSettingsManager.CanDebug();
    }

}
