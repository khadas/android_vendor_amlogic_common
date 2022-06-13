/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC DisplayPositionFragment
 */



package com.android.tv.settings.display.position;

import android.content.Context;
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

import java.util.List;
import java.util.Map;
import java.util.ArrayList;

public class DisplayPositionFragment extends SettingsPreferenceFragment {
    private static final String TAG = "DisplayPositionFragment";

    private static final String SCREEN_POSITION_SCALE = "screen_position_scale";
    private static final String ZOOM_IN = "zoom_in";
    private static final String ZOOM_OUT = "zoom_out";

    private DisplayPositionManager mDisplayPositionManager;

    private Preference screenPref;
    private PreferenceCategory mPref;
    private Preference zoominPref;
    private Preference zoomoutPref;

    public static DisplayPositionFragment newInstance() {
        return new DisplayPositionFragment();
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        setPreferencesFromResource(R.xml.display_position, null);
        mDisplayPositionManager = new DisplayPositionManager((Context)getActivity());

        mPref       = (PreferenceCategory) findPreference(SCREEN_POSITION_SCALE);
        zoominPref  = (Preference) findPreference(ZOOM_IN);
        zoomoutPref = (Preference) findPreference(ZOOM_OUT);

        updateMainScreen();
    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        switch (preference.getKey()) {
            case ZOOM_IN:
                mDisplayPositionManager.zoomIn();
                break;
            case ZOOM_OUT:
                mDisplayPositionManager.zoomOut();
                break;
        }
        updateMainScreen();
        return true;
    }

    @Override
    public int getMetricsCategory() {
        return 0;
    }

    private void updateMainScreen() {
        int percent = mDisplayPositionManager.getCurrentRateValue();
        mPref.setTitle("current scaling is " + percent +"%");
    }
}
