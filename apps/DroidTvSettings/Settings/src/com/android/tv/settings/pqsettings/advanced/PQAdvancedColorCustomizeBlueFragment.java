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


public class PQAdvancedColorCustomizeBlueFragment extends SettingsPreferenceFragment implements Preference.OnPreferenceChangeListener {
    public static final String TAG = "PQAdvancedColorCustomizeBlueFragment";

    private static final String PQ_PICTRUE_ADVANCED_COLOR_CUSTOMIZE_BLUE_SATURATION = "pq_pictrue_advanced_color_customize_blue_saturation";
    private static final String PQ_PICTRUE_ADVANCED_COLOR_CUSTOMIZE_BLUE_LUMA = "pq_pictrue_advanced_color_customize_blue_luma";
    private static final String PQ_PICTRUE_ADVANCED_COLOR_CUSTOMIZE_BLUE_HUE = "pq_pictrue_advanced_color_customize_blue_hue";
    private static final int PQ_PICTRUE_ADVANCED_COLOR_CUSTOMIZE_BLUE_STEP = 1;

    private PQSettingsManager mPQSettingsManager;

    public static PQAdvancedColorCustomizeBlueFragment newInstance() {
        return new PQAdvancedColorCustomizeBlueFragment();
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
        setPreferencesFromResource(R.xml.pq_pictrue_advanced_color_customize_blue, null);

        if (mPQSettingsManager == null) {
            mPQSettingsManager = new PQSettingsManager(getActivity());
        }

        final SeekBarPreference PQPictureAdvancedColorCustomizeBlueSaturationPref = (SeekBarPreference) findPreference(PQ_PICTRUE_ADVANCED_COLOR_CUSTOMIZE_BLUE_SATURATION);
        final SeekBarPreference PQPictureAdvancedColorCustomizeBlueLumaPref = (SeekBarPreference) findPreference(PQ_PICTRUE_ADVANCED_COLOR_CUSTOMIZE_BLUE_LUMA);
        final SeekBarPreference PQPictureAdvancedColorCustomizeBlueHuePref = (SeekBarPreference) findPreference(PQ_PICTRUE_ADVANCED_COLOR_CUSTOMIZE_BLUE_HUE);

        PQPictureAdvancedColorCustomizeBlueSaturationPref.setOnPreferenceChangeListener(this);
        PQPictureAdvancedColorCustomizeBlueSaturationPref.setSeekBarIncrement(PQ_PICTRUE_ADVANCED_COLOR_CUSTOMIZE_BLUE_STEP);
        PQPictureAdvancedColorCustomizeBlueSaturationPref.setMin(-50);
        PQPictureAdvancedColorCustomizeBlueSaturationPref.setMax(50);
        PQPictureAdvancedColorCustomizeBlueSaturationPref.setValue(mPQSettingsManager.getAdvancedColorCustomizeBlueSaturationStatus());
        PQPictureAdvancedColorCustomizeBlueSaturationPref.setVisible(true);

        PQPictureAdvancedColorCustomizeBlueLumaPref.setOnPreferenceChangeListener(this);
        PQPictureAdvancedColorCustomizeBlueLumaPref.setSeekBarIncrement(PQ_PICTRUE_ADVANCED_COLOR_CUSTOMIZE_BLUE_STEP);
        PQPictureAdvancedColorCustomizeBlueLumaPref.setMin(-15);
        PQPictureAdvancedColorCustomizeBlueLumaPref.setMax(15);
        PQPictureAdvancedColorCustomizeBlueLumaPref.setValue(mPQSettingsManager.getAdvancedColorCustomizeBlueLumaStatus());
        PQPictureAdvancedColorCustomizeBlueLumaPref.setVisible(true);

        PQPictureAdvancedColorCustomizeBlueHuePref.setOnPreferenceChangeListener(this);
        PQPictureAdvancedColorCustomizeBlueHuePref.setSeekBarIncrement(PQ_PICTRUE_ADVANCED_COLOR_CUSTOMIZE_BLUE_STEP);
        PQPictureAdvancedColorCustomizeBlueHuePref.setMin(-50);
        PQPictureAdvancedColorCustomizeBlueHuePref.setMax(50);
        PQPictureAdvancedColorCustomizeBlueHuePref.setValue(mPQSettingsManager.getAdvancedColorCustomizeBlueHueStatus());
        PQPictureAdvancedColorCustomizeBlueHuePref.setVisible(true);

    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        if (CanDebug()) Log.d(TAG, "[onPreferenceTreeClick] preference.getKey() = " + preference.getKey());
        return super.onPreferenceTreeClick(preference);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        switch (preference.getKey()) {
            case PQ_PICTRUE_ADVANCED_COLOR_CUSTOMIZE_BLUE_SATURATION:
                mPQSettingsManager.setAdvancedColorCustomizeBlueSaturationStatus((int)newValue);
                //mAudioEffectManager.setDapParam(AudioEffectManager.CMD_DAP_VL_AMOUNT, (int)newValue);
                break;
            case PQ_PICTRUE_ADVANCED_COLOR_CUSTOMIZE_BLUE_LUMA:
                mPQSettingsManager.setAdvancedColorCustomizeBlueLumaStatus((int)newValue);
                //mAudioEffectManager.setDapParam(AudioEffectManager.CMD_DAP_DE_AMOUNT, (int)newValue);
                break;
            case PQ_PICTRUE_ADVANCED_COLOR_CUSTOMIZE_BLUE_HUE:
                mPQSettingsManager.setAdvancedColorCustomizeBlueHueStatus((int)newValue);
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
