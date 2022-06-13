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


public class PQAdvancedManualGammaFragment extends SettingsPreferenceFragment implements Preference.OnPreferenceChangeListener {
    public static final String TAG = "PQAdvancedManualGammaFragment";

    private static final String PQ_PICTRUE_ADVANCED_MANUAL_GAMMA_LEVEL = "pq_pictrue_advanced_manual_gamma_level";
    private static final String PQ_PICTRUE_ADVANCED_MANUAL_GAMMA_RGAIN = "pq_pictrue_advanced_manual_gamma_rgain";
    private static final String PQ_PICTRUE_ADVANCED_MANUAL_GAMMA_GGAIN = "pq_pictrue_advanced_manual_gamma_ggain";
    private static final String PQ_PICTRUE_ADVANCED_MANUAL_GAMMA_BGAIN = "pq_pictrue_advanced_manual_gamma_bgain";
    private static final String PQ_PICTRUE_ADVANCED_MANUAL_GAMMA_RESET = "pq_pictrue_advanced_manual_gamma_reset";
    private static final int PQ_PICTRUE_ADVANCED_MANUAL_STEP = 1;

    private PQSettingsManager mPQSettingsManager;

    public static PQAdvancedManualGammaFragment newInstance() {
        return new PQAdvancedManualGammaFragment();
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
        setPreferencesFromResource(R.xml.pq_pictrue_advanced_manual_gamma, null);

        if (mPQSettingsManager == null) {
            mPQSettingsManager = new PQSettingsManager(getActivity());
        }

        final SeekBarPreference PQPictureAdvancedManualGammaLevelPref = (SeekBarPreference) findPreference(PQ_PICTRUE_ADVANCED_MANUAL_GAMMA_LEVEL);
        final SeekBarPreference PQPictureAdvancedManualGammaRGainPref = (SeekBarPreference) findPreference(PQ_PICTRUE_ADVANCED_MANUAL_GAMMA_RGAIN);
        final SeekBarPreference PQPictureAdvancedManualGammaGGainPref = (SeekBarPreference) findPreference(PQ_PICTRUE_ADVANCED_MANUAL_GAMMA_GGAIN);
        final SeekBarPreference PQPictureAdvancedManualGammaBGainPref = (SeekBarPreference) findPreference(PQ_PICTRUE_ADVANCED_MANUAL_GAMMA_BGAIN);
        final Preference PQPictureAdvancedManualGammaResetPref = (Preference) findPreference(PQ_PICTRUE_ADVANCED_MANUAL_GAMMA_RESET);

        PQPictureAdvancedManualGammaLevelPref.setOnPreferenceChangeListener(this);
        PQPictureAdvancedManualGammaLevelPref.setSeekBarIncrement(PQ_PICTRUE_ADVANCED_MANUAL_STEP);
        PQPictureAdvancedManualGammaLevelPref.setMin(0);
        PQPictureAdvancedManualGammaLevelPref.setMax(10);
        PQPictureAdvancedManualGammaLevelPref.setValue(mPQSettingsManager.getAdvancedManualGammaLevelStatus());
        PQPictureAdvancedManualGammaLevelPref.setVisible(true);

        PQPictureAdvancedManualGammaRGainPref.setOnPreferenceChangeListener(this);
        PQPictureAdvancedManualGammaRGainPref.setSeekBarIncrement(PQ_PICTRUE_ADVANCED_MANUAL_STEP);
        PQPictureAdvancedManualGammaRGainPref.setMin(-50);
        PQPictureAdvancedManualGammaRGainPref.setMax(50);
        PQPictureAdvancedManualGammaRGainPref.setValue(mPQSettingsManager.getAdvancedManualGammaRGainStatus());
        PQPictureAdvancedManualGammaRGainPref.setVisible(true);

        PQPictureAdvancedManualGammaGGainPref.setOnPreferenceChangeListener(this);
        PQPictureAdvancedManualGammaGGainPref.setSeekBarIncrement(PQ_PICTRUE_ADVANCED_MANUAL_STEP);
        PQPictureAdvancedManualGammaGGainPref.setMin(-50);
        PQPictureAdvancedManualGammaGGainPref.setMax(50);
        PQPictureAdvancedManualGammaGGainPref.setValue(mPQSettingsManager.getAdvancedManualGammaGGainStatus());
        PQPictureAdvancedManualGammaGGainPref.setVisible(true);

        PQPictureAdvancedManualGammaBGainPref.setOnPreferenceChangeListener(this);
        PQPictureAdvancedManualGammaBGainPref.setSeekBarIncrement(PQ_PICTRUE_ADVANCED_MANUAL_STEP);
        PQPictureAdvancedManualGammaBGainPref.setMin(-50);
        PQPictureAdvancedManualGammaBGainPref.setMax(50);
        PQPictureAdvancedManualGammaBGainPref.setValue(mPQSettingsManager.getAdvancedManualGammaBGainStatus());
        PQPictureAdvancedManualGammaBGainPref.setVisible(true);

    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        if (CanDebug()) Log.d(TAG, "[onPreferenceTreeClick] preference.getKey() = " + preference.getKey());
        switch (preference.getKey()) {
            case PQ_PICTRUE_ADVANCED_MANUAL_GAMMA_RESET:
                Intent PQPictureAdvancedManualGammaResetAllIntent = new Intent();
                PQPictureAdvancedManualGammaResetAllIntent.setClassName(
                        "com.android.tv.settings",
                        "com.android.tv.settings.pqsettings.advanced.PQAdvancedManualGammaResetAllActivity");
                startActivity(PQPictureAdvancedManualGammaResetAllIntent);
                break;
        }
        return super.onPreferenceTreeClick(preference);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        switch (preference.getKey()) {
            case PQ_PICTRUE_ADVANCED_MANUAL_GAMMA_LEVEL:
                mPQSettingsManager.setAdvancedManualGammaLevelStatus((int)newValue);
                //final int progress = Integer.parseInt((String)newValue);
                //mAudioEffectManager.setDapParam(AudioEffectManager.CMD_DAP_GEQ_ENABLE, progress);
                //updateGeq(progress);
                break;
            case PQ_PICTRUE_ADVANCED_MANUAL_GAMMA_RGAIN:
                mPQSettingsManager.setAdvancedManualGammaRGainStatus((int)newValue);
                //mAudioEffectManager.setDapParam(AudioEffectManager.CMD_DAP_VL_AMOUNT, (int)newValue);
                break;
            case PQ_PICTRUE_ADVANCED_MANUAL_GAMMA_GGAIN:
                mPQSettingsManager.setAdvancedManualGammaGGainStatus((int)newValue);
                //mAudioEffectManager.setDapParam(AudioEffectManager.CMD_DAP_DE_AMOUNT, (int)newValue);
                break;
            case PQ_PICTRUE_ADVANCED_MANUAL_GAMMA_BGAIN:
                mPQSettingsManager.setAdvancedManualGammaBGainStatus((int)newValue);
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
