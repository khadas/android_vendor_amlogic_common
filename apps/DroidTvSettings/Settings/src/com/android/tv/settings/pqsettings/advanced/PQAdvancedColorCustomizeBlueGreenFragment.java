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

package com.android.tv.settings.pqsettings.advanced;

import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import com.android.tv.settings.SettingsPreferenceFragment;
import androidx.preference.Preference;
import androidx.preference.SeekBarPreference;
import androidx.preference.PreferenceCategory;
import androidx.preference.ListPreference;
import android.os.SystemProperties;
import android.util.Log;
import android.text.TextUtils;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import android.provider.Settings;

import com.android.tv.settings.util.DroidUtils;
import com.android.tv.settings.SettingsConstant;
import com.android.tv.settings.R;

import com.android.tv.settings.pqsettings.PQSettingsManager;


public class PQAdvancedColorCustomizeBlueGreenFragment extends SettingsPreferenceFragment implements Preference.OnPreferenceChangeListener {
    public static final String TAG = "PQAdvancedColorCustomizeBlueGreenFragment";

    private static final String PQ_PICTRUE_ADVANCED_COLOR_CUSTOMIZE_BLUE_GREEN_SATURATION = "pq_pictrue_advanced_color_customize_blue_green_saturation";
    private static final String PQ_PICTRUE_ADVANCED_COLOR_CUSTOMIZE_BLUE_GREEN_LUMA = "pq_pictrue_advanced_color_customize_blue_green_luma";
    private static final String PQ_PICTRUE_ADVANCED_COLOR_CUSTOMIZE_BLUE_GREEN_HUE = "pq_pictrue_advanced_color_customize_blue_green_hue";
    private static final int PQ_PICTRUE_ADVANCED_COLOR_CUSTOMIZE_BLUE_GREEN_STEP = 1;

    private PQSettingsManager mPQSettingsManager;

    public static PQAdvancedColorCustomizeBlueGreenFragment newInstance() {
        return new PQAdvancedColorCustomizeBlueGreenFragment();
    }

    private boolean CanDebug() {
        return PQSettingsManager.CanDebug();
    }

    private String[] getArrayString(int resid) {
        return getActivity().getResources().getStringArray(resid);
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (mPQSettingsManager == null) {
            mPQSettingsManager = new PQSettingsManager(getActivity());
        }
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        setPreferencesFromResource(R.xml.pq_pictrue_advanced_color_customize_blue_green, null);

        if (mPQSettingsManager == null) {
            mPQSettingsManager = new PQSettingsManager(getActivity());
        }

        final SeekBarPreference PQPictureAdvancedColorCustomizeBlueGreenSaturationPref = (SeekBarPreference) findPreference(PQ_PICTRUE_ADVANCED_COLOR_CUSTOMIZE_BLUE_GREEN_SATURATION);
        final SeekBarPreference PQPictureAdvancedColorCustomizeBlueGreenLumaPref = (SeekBarPreference) findPreference(PQ_PICTRUE_ADVANCED_COLOR_CUSTOMIZE_BLUE_GREEN_LUMA);
        final SeekBarPreference PQPictureAdvancedColorCustomizeBlueGreenHuePref = (SeekBarPreference) findPreference(PQ_PICTRUE_ADVANCED_COLOR_CUSTOMIZE_BLUE_GREEN_HUE);

        PQPictureAdvancedColorCustomizeBlueGreenSaturationPref.setOnPreferenceChangeListener(this);
        PQPictureAdvancedColorCustomizeBlueGreenSaturationPref.setSeekBarIncrement(PQ_PICTRUE_ADVANCED_COLOR_CUSTOMIZE_BLUE_GREEN_STEP);
        PQPictureAdvancedColorCustomizeBlueGreenSaturationPref.setMin(-50);
        PQPictureAdvancedColorCustomizeBlueGreenSaturationPref.setMax(50);
        PQPictureAdvancedColorCustomizeBlueGreenSaturationPref.setValue(mPQSettingsManager.getAdvancedColorCustomizeBlueGreenSaturationStatus());
        PQPictureAdvancedColorCustomizeBlueGreenSaturationPref.setVisible(true);

        PQPictureAdvancedColorCustomizeBlueGreenLumaPref.setOnPreferenceChangeListener(this);
        PQPictureAdvancedColorCustomizeBlueGreenLumaPref.setSeekBarIncrement(PQ_PICTRUE_ADVANCED_COLOR_CUSTOMIZE_BLUE_GREEN_STEP);
        PQPictureAdvancedColorCustomizeBlueGreenLumaPref.setMin(-15);
        PQPictureAdvancedColorCustomizeBlueGreenLumaPref.setMax(15);
        PQPictureAdvancedColorCustomizeBlueGreenLumaPref.setValue(mPQSettingsManager.getAdvancedColorCustomizeBlueGreenLumaStatus());
        PQPictureAdvancedColorCustomizeBlueGreenLumaPref.setVisible(true);

        PQPictureAdvancedColorCustomizeBlueGreenHuePref.setOnPreferenceChangeListener(this);
        PQPictureAdvancedColorCustomizeBlueGreenHuePref.setSeekBarIncrement(PQ_PICTRUE_ADVANCED_COLOR_CUSTOMIZE_BLUE_GREEN_STEP);
        PQPictureAdvancedColorCustomizeBlueGreenHuePref.setMin(-50);
        PQPictureAdvancedColorCustomizeBlueGreenHuePref.setMax(50);
        PQPictureAdvancedColorCustomizeBlueGreenHuePref.setValue(mPQSettingsManager.getAdvancedColorCustomizeBlueGreenHueStatus());
        PQPictureAdvancedColorCustomizeBlueGreenHuePref.setVisible(true);

    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        if (CanDebug()) Log.d(TAG, "[onPreferenceTreeClick] preference.getKey() = " + preference.getKey());
        return super.onPreferenceTreeClick(preference);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        switch (preference.getKey()) {
            case PQ_PICTRUE_ADVANCED_COLOR_CUSTOMIZE_BLUE_GREEN_SATURATION:
                mPQSettingsManager.setAdvancedColorCustomizeBlueGreenSaturationStatus((int)newValue);
                //mAudioEffectManager.setDapParam(AudioEffectManager.CMD_DAP_VL_AMOUNT, (int)newValue);
                break;
            case PQ_PICTRUE_ADVANCED_COLOR_CUSTOMIZE_BLUE_GREEN_LUMA:
                mPQSettingsManager.setAdvancedColorCustomizeBlueGreenLumaStatus((int)newValue);
                //mAudioEffectManager.setDapParam(AudioEffectManager.CMD_DAP_DE_AMOUNT, (int)newValue);
                break;
            case PQ_PICTRUE_ADVANCED_COLOR_CUSTOMIZE_BLUE_GREEN_HUE:
                mPQSettingsManager.setAdvancedColorCustomizeBlueGreenHueStatus((int)newValue);
                //mAudioEffectManager.setDapParam(AudioEffectManager.CMD_DAP_SURROUND_BOOST, (int)newValue);
                break;
        }
        return true;
    }

    @Override
    public int getMetricsCategory() {
        return 0;
    }

}
