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


public class PQAdvancedColorTemperatureFragment extends SettingsPreferenceFragment implements Preference.OnPreferenceChangeListener {
    public static final String TAG = "PQAdvancedColorTemperatureFragment";

    private static final String PQ_PICTRUE_ADVANCED_COLOR_TEMPERATURE_WBSETTING = "pq_pictrue_advanced_color_temperature_wbsetting";
    private static final String PQ_PICTRUE_ADVANCED_COLOR_TEMPERATURE_RGAIN = "pq_pictrue_advanced_color_temperature_rgain";
    private static final String PQ_PICTRUE_ADVANCED_COLOR_TEMPERATURE_GGAIN = "pq_pictrue_advanced_color_temperature_ggain";
    private static final String PQ_PICTRUE_ADVANCED_COLOR_TEMPERATURE_BGAIN = "pq_pictrue_advanced_color_temperature_bgain";
    private static final String PQ_PICTRUE_ADVANCED_COLOR_TEMPERATURE_ROFFSET = "pq_pictrue_advanced_color_temperature_roffset";
    private static final String PQ_PICTRUE_ADVANCED_COLOR_TEMPERATURE_GOFFSET = "pq_pictrue_advanced_color_temperature_goffset";
    private static final String PQ_PICTRUE_ADVANCED_COLOR_TEMPERATURE_BOFFSET = "pq_pictrue_advanced_color_temperature_boffset";
    private static final String PQ_PICTRUE_ADVANCED_COLOR_TEMPERATURE_WBRESET = "pq_pictrue_advanced_color_temperature_wbreset";
    private static final int PQ_PICTRUE_ADVANCED_COLOR_TEMPERATURE_STEP = 1;

    private PQSettingsManager mPQSettingsManager;

    public static PQAdvancedColorTemperatureFragment newInstance() {
        return new PQAdvancedColorTemperatureFragment();
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
        setPreferencesFromResource(R.xml.pq_pictrue_advanced_color_temperature, null);

        if (mPQSettingsManager == null) {
            mPQSettingsManager = new PQSettingsManager(getActivity());
        }

        final ListPreference PQPictureAdvancedColorTemperatureWbsettingPref = (ListPreference) findPreference(PQ_PICTRUE_ADVANCED_COLOR_TEMPERATURE_WBSETTING);
        final SeekBarPreference PQPictureAdvancedColorTemperatureRGainPref = (SeekBarPreference) findPreference(PQ_PICTRUE_ADVANCED_COLOR_TEMPERATURE_RGAIN);
        final SeekBarPreference PQPictureAdvancedColorTemperatureGGainPref = (SeekBarPreference) findPreference(PQ_PICTRUE_ADVANCED_COLOR_TEMPERATURE_GGAIN);
        final SeekBarPreference PQPictureAdvancedColorTemperatureBGainPref = (SeekBarPreference) findPreference(PQ_PICTRUE_ADVANCED_COLOR_TEMPERATURE_BGAIN);
        final SeekBarPreference PQPictureAdvancedColorTemperatureROffsetPref = (SeekBarPreference) findPreference(PQ_PICTRUE_ADVANCED_COLOR_TEMPERATURE_ROFFSET);
        final SeekBarPreference PQPictureAdvancedColorTemperatureGOffsetPref = (SeekBarPreference) findPreference(PQ_PICTRUE_ADVANCED_COLOR_TEMPERATURE_GOFFSET);
        final SeekBarPreference PQPictureAdvancedColorTemperatureBOffsetPref = (SeekBarPreference) findPreference(PQ_PICTRUE_ADVANCED_COLOR_TEMPERATURE_BOFFSET);
        final Preference PQPictureAdvancedColorTemperatureResetPref = (Preference) findPreference(PQ_PICTRUE_ADVANCED_COLOR_TEMPERATURE_WBRESET);

        if (true) {//Leave blank first, add conditions later
            PQPictureAdvancedColorTemperatureWbsettingPref.setValueIndex(mPQSettingsManager.getColorTemperatureStatus());
            PQPictureAdvancedColorTemperatureWbsettingPref.setOnPreferenceChangeListener(this);
        } else {
            PQPictureAdvancedColorTemperatureWbsettingPref.setVisible(false);
        }

        PQPictureAdvancedColorTemperatureRGainPref.setOnPreferenceChangeListener(this);
        PQPictureAdvancedColorTemperatureRGainPref.setSeekBarIncrement(PQ_PICTRUE_ADVANCED_COLOR_TEMPERATURE_STEP);
        PQPictureAdvancedColorTemperatureRGainPref.setMin(-50);
        PQPictureAdvancedColorTemperatureRGainPref.setMax(50);
        PQPictureAdvancedColorTemperatureRGainPref.setValue(mPQSettingsManager.getAdvancedColorTemperatureRGainStatus());
        PQPictureAdvancedColorTemperatureRGainPref.setVisible(true);

        PQPictureAdvancedColorTemperatureGGainPref.setOnPreferenceChangeListener(this);
        PQPictureAdvancedColorTemperatureGGainPref.setSeekBarIncrement(PQ_PICTRUE_ADVANCED_COLOR_TEMPERATURE_STEP);
        PQPictureAdvancedColorTemperatureGGainPref.setMin(-50);
        PQPictureAdvancedColorTemperatureGGainPref.setMax(50);
        PQPictureAdvancedColorTemperatureGGainPref.setValue(mPQSettingsManager.getAdvancedColorTemperatureGGainStatus());
        PQPictureAdvancedColorTemperatureGGainPref.setVisible(true);

        PQPictureAdvancedColorTemperatureBGainPref.setOnPreferenceChangeListener(this);
        PQPictureAdvancedColorTemperatureBGainPref.setSeekBarIncrement(PQ_PICTRUE_ADVANCED_COLOR_TEMPERATURE_STEP);
        PQPictureAdvancedColorTemperatureBGainPref.setMin(-50);
        PQPictureAdvancedColorTemperatureBGainPref.setMax(50);
        PQPictureAdvancedColorTemperatureBGainPref.setValue(mPQSettingsManager.getAdvancedColorTemperatureBGainStatus());
        PQPictureAdvancedColorTemperatureBGainPref.setVisible(true);

        PQPictureAdvancedColorTemperatureROffsetPref.setOnPreferenceChangeListener(this);
        PQPictureAdvancedColorTemperatureROffsetPref.setSeekBarIncrement(PQ_PICTRUE_ADVANCED_COLOR_TEMPERATURE_STEP);
        PQPictureAdvancedColorTemperatureROffsetPref.setMin(-50);
        PQPictureAdvancedColorTemperatureROffsetPref.setMax(50);
        PQPictureAdvancedColorTemperatureROffsetPref.setValue(mPQSettingsManager.getAdvancedColorTemperatureROffsetStatus());
        PQPictureAdvancedColorTemperatureROffsetPref.setVisible(true);

        PQPictureAdvancedColorTemperatureGOffsetPref.setOnPreferenceChangeListener(this);
        PQPictureAdvancedColorTemperatureGOffsetPref.setSeekBarIncrement(PQ_PICTRUE_ADVANCED_COLOR_TEMPERATURE_STEP);
        PQPictureAdvancedColorTemperatureGOffsetPref.setMin(-50);
        PQPictureAdvancedColorTemperatureGOffsetPref.setMax(50);
        PQPictureAdvancedColorTemperatureGOffsetPref.setValue(mPQSettingsManager.getAdvancedColorTemperatureGOffsetStatus());
        PQPictureAdvancedColorTemperatureGOffsetPref.setVisible(true);

        PQPictureAdvancedColorTemperatureBOffsetPref.setOnPreferenceChangeListener(this);
        PQPictureAdvancedColorTemperatureBOffsetPref.setSeekBarIncrement(PQ_PICTRUE_ADVANCED_COLOR_TEMPERATURE_STEP);
        PQPictureAdvancedColorTemperatureBOffsetPref.setMin(-50);
        PQPictureAdvancedColorTemperatureBOffsetPref.setMax(50);
        PQPictureAdvancedColorTemperatureBOffsetPref.setValue(mPQSettingsManager.getAdvancedColorTemperatureBOffsetStatus());
        PQPictureAdvancedColorTemperatureBOffsetPref.setVisible(true);

    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        if (CanDebug()) Log.d(TAG, "[onPreferenceTreeClick] preference.getKey() = " + preference.getKey());
        switch (preference.getKey()) {
            case PQ_PICTRUE_ADVANCED_COLOR_TEMPERATURE_WBRESET:
                Intent PQAdvancedColorTemperatureResetAllIntent = new Intent();
                PQAdvancedColorTemperatureResetAllIntent.setClassName(
                        "com.android.tv.settings",
                        "com.android.tv.settings.pqsettings.advanced.PQAdvancedColorTemperatureResetAllActivity");
                startActivity(PQAdvancedColorTemperatureResetAllIntent);
                break;
        }
        return super.onPreferenceTreeClick(preference);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        switch (preference.getKey()) {
            case PQ_PICTRUE_ADVANCED_COLOR_TEMPERATURE_WBSETTING:
                final int selection = Integer.parseInt((String)newValue);
                mPQSettingsManager.setColorTemperature(selection);
                //final int progress = Integer.parseInt((String)newValue);
                //mAudioEffectManager.setDapParam(AudioEffectManager.CMD_DAP_GEQ_ENABLE, progress);
                //updateGeq(progress);
                break;
            case PQ_PICTRUE_ADVANCED_COLOR_TEMPERATURE_RGAIN:
                mPQSettingsManager.setAdvancedColorTemperatureRGainStatus((int)newValue);
                //mAudioEffectManager.setDapParam(AudioEffectManager.CMD_DAP_VL_AMOUNT, (int)newValue);
                break;
            case PQ_PICTRUE_ADVANCED_COLOR_TEMPERATURE_GGAIN:
                mPQSettingsManager.setAdvancedColorTemperatureGGainStatus((int)newValue);
                //mAudioEffectManager.setDapParam(AudioEffectManager.CMD_DAP_DE_AMOUNT, (int)newValue);
                break;
            case PQ_PICTRUE_ADVANCED_COLOR_TEMPERATURE_BGAIN:
                mPQSettingsManager.setAdvancedColorTemperatureBGainStatus((int)newValue);
                //mAudioEffectManager.setDapParam(AudioEffectManager.CMD_DAP_SURROUND_BOOST, (int)newValue);
                break;
            case PQ_PICTRUE_ADVANCED_COLOR_TEMPERATURE_ROFFSET:
                mPQSettingsManager.setAdvancedColorTemperatureROffsetStatus((int)newValue);
                //mAudioEffectManager.setDapParam(AudioEffectManager.CMD_DAP_SURROUND_BOOST, (int)newValue);
                break;
            case PQ_PICTRUE_ADVANCED_COLOR_TEMPERATURE_GOFFSET:
                mPQSettingsManager.setAdvancedColorTemperatureGOffsetStatus((int)newValue);
                //mAudioEffectManager.setDapParam(AudioEffectManager.CMD_DAP_SURROUND_BOOST, (int)newValue);
                break;
            case PQ_PICTRUE_ADVANCED_COLOR_TEMPERATURE_BOFFSET:
                mPQSettingsManager.setAdvancedColorTemperatureBOffsetStatus((int)newValue);
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
