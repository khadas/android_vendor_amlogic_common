/*
 * Copyright (C) 2015 The Android Open Source Project
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
 * limitations under the License
 */

package com.android.tv.settings.tvoption;

import android.content.ContentResolver;
import android.content.Context;
import android.os.Bundle;
import android.os.SystemProperties;
import android.provider.Settings;
import androidx.preference.SwitchPreference;
import com.android.tv.settings.SettingsPreferenceFragment;
import androidx.preference.ListPreference;
import androidx.preference.Preference;
import androidx.preference.SwitchPreferenceCompat;
import androidx.preference.TwoStatePreference;
import androidx.preference.PreferenceScreen;
import android.text.TextUtils;
import android.util.Log;

import java.util.List;
import java.util.ArrayList;

import com.droidlogic.app.OutputModeManager;
import com.android.tv.settings.SettingsConstant;
import com.android.tv.settings.R;

public class NoSignalScreenStatusFragment extends SettingsPreferenceFragment implements Preference.OnPreferenceChangeListener{

    private static final String TAG = "NoSignalScreenStatusFragment";

    private static final String STATIC_FRAME = "tv_static_frame";
    private static final String BLACK_SCREEN = "tv_black_screen";
    private static final String BLUE_SCREEN  = "tv_blue_screen";

    private TvOptionSettingManager mTvOptionSettingManager;
    private TwoStatePreference mStaticFramePreference;
    private TwoStatePreference mBlackScreenPreference;
    private TwoStatePreference mBlueScreenPreference;
    public static NoSignalScreenStatusFragment newInstance() {
        return new NoSignalScreenStatusFragment();
    }

    private boolean CanDebug() {
        return TvOptionFragment.CanDebug();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public void onResume() {
        super.onResume();
        update();
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        if (CanDebug()) {
            Log.d(TAG, "[onCreatePreferences]");
        }
        setPreferencesFromResource(R.xml.tv_nosignal_screen_status, null);
        if (mTvOptionSettingManager == null) {
            mTvOptionSettingManager = new TvOptionSettingManager(getActivity(), false);
        }
        mStaticFramePreference = (TwoStatePreference) findPreference(STATIC_FRAME);
        mBlackScreenPreference = (TwoStatePreference) findPreference(BLACK_SCREEN);
        mBlueScreenPreference = (TwoStatePreference) findPreference(BLUE_SCREEN);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        if (CanDebug()) {
            Log.d(TAG, "[onPreferenceChange] preference.getKey() = " + preference.getKey() + ", newValue = " + newValue);
        }
        return true;
    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        if (CanDebug()) {
            Log.d(TAG, "[onPreferenceTreeClick] preference.getKey() = " + preference.getKey());
        }
        boolean isChecked;
        switch (preference.getKey()) {
            case STATIC_FRAME:
                isChecked = mStaticFramePreference.isChecked();
                mTvOptionSettingManager.setStaticFrameStatus(isChecked?1:0, 1);
                break;
            case BLACK_SCREEN:
                isChecked = mBlackScreenPreference.isChecked();
                mTvOptionSettingManager.setScreenColorForSignalChange(isChecked?0:1, 1);
                updateScreenColorStatus(!isChecked);
                break;
            case BLUE_SCREEN:
                isChecked = mBlueScreenPreference.isChecked();
                mTvOptionSettingManager.setScreenColorForSignalChange(isChecked?1:0, 1);
                updateScreenColorStatus(isChecked);
                break;
        }
        return super.onPreferenceTreeClick(preference);
    }

    @Override
    public int getMetricsCategory() {
        return 0;
    }

    private void update() {
        if (CanDebug()) {
            Log.d(TAG, "[update]");
        }
        int isStaticFrameEnable = mTvOptionSettingManager.getStaticFrameStatus();
        boolean enable = (isStaticFrameEnable == 1);
        if (mTvOptionSettingManager.isChannalSource()) {
            mStaticFramePreference.setVisible(true);
            mStaticFramePreference.setChecked(enable);
        } else {
            mStaticFramePreference.setVisible(false);
            mStaticFramePreference.setChecked(false);
        }
        int isblueScreenEnable = mTvOptionSettingManager.getScreenColorForSignalChange();
        enable = (isblueScreenEnable == 1);
        updateScreenColorStatus(enable);
    }

    private void updateScreenColorStatus(boolean isblueScreenEnable) {
        if (CanDebug()) {
            Log.d(TAG, "[updateScreenColorStatus]");
        }
        if (isblueScreenEnable) {
            mBlackScreenPreference.setChecked(false);
            mBlueScreenPreference.setChecked(true);
        } else {
            mBlackScreenPreference.setChecked(true);
            mBlueScreenPreference.setChecked(false);
        }
    }
}
