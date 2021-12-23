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

package com.android.tv.settings.soundeffect;

import android.os.Bundle;
import androidx.preference.ListPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceCategory;
import androidx.preference.SeekBarPreference;
import androidx.preference.SwitchPreference;
import androidx.preference.TwoStatePreference;
import android.util.Log;

import com.android.tv.settings.R;
import com.android.tv.settings.SettingsPreferenceFragment;
import com.android.tv.settings.tvoption.SoundParameterSettingManager;
import com.droidlogic.app.tv.AudioEffectManager;

public class DolbyAudioEffect_2_4_Fragment extends SettingsPreferenceFragment implements Preference.OnPreferenceChangeListener {

    private static final String TAG = "Dap_2_4_ModeFragment";

    private static final String KEY_DAP_2_4_PROFILE                    = "key_dap_2_4_profile";
    private static final String KEY_DAP_2_4_DETAIL                     = "key_dap_2_4_detail";

    private static final String KEY_DAP_2_4_SURROUND_VIRTUALIZER_MODE  = "key_dap_2_4_surround_virtualizer_mode";
    private static final String KEY_DAP_2_4_SURROUND_VIRTUALIZER_BOOST = "key_dap_2_4_surround_virtualizer_boost";
    private static final String KEY_DAP_2_4_DIALOGUE_ENHANCER_ENABLE   = "key_dap_2_4_dialogue_enhancer_enable";
    private static final String KEY_DAP_2_4_DIALOGUE_ENHANCER_AMOUNT   = "key_dap_2_4_dialogue_enhancer_amount";
    private static final String KEY_DAP_2_4_BASS_ENHANCER_ENABLE       = "key_dap_2_4_bass_enhancer_enable";
    private static final String KEY_DAP_2_4_BASS_ENHANCER_BOOST        = "key_dap_2_4_bass_enhancer_boost";
    private static final String KEY_DAP_2_4_BASS_ENHANCER_CUTOFFX100   = "key_dap_2_4_bass_enhancer_cutoffX100";
    private static final String KEY_DAP_2_4_BASS_ENHANCER_CUTOFFX1     = "key_dap_2_4_bass_enhancer_cutoffX1";
    private static final String KEY_DAP_2_4_BASS_ENHANCER_WIDTH        = "key_dap_2_4_bass_enhancer_width";
    private static final String KEY_DAP_2_4_MI_STEERING                = "key_dap_2_4_mi_steering";
    private static final String KEY_DAP_2_4_SURROUND_DECODER_ENABLE    = "key_dap_2_4_surround_decoder_enable";
    private static final String KEY_DAP_2_4_LEVELER_SETTING            = "key_dap_2_4_leveler_setting";
    private static final String KEY_DAP_2_4_LEVELER_AMOUNT             = "key_dap_2_4_leveler_amount";

    private PreferenceCategory mDap24DetailPref;

    private ListPreference mSuvPref;
    private SeekBarPreference mSuvBoostPref;
    private TwoStatePreference mDePref;
    private SeekBarPreference mDeAmountPref;
    private TwoStatePreference mBePref;
    private SeekBarPreference mBeBoostPref;
    private SeekBarPreference mBeCutoffX100Pref;
    private SeekBarPreference mBeCutoffX1Pref;
    private SeekBarPreference mBeWidthPref;
    private TwoStatePreference mMsPref;
    private TwoStatePreference mSdePref;
    private ListPreference mLePref;
    private SeekBarPreference mLeAmountPref;

    private AudioEffectManager mAudioEffectManager;

    public static DolbyAudioEffect_2_4_Fragment newInstance() {
        return new DolbyAudioEffect_2_4_Fragment();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public void onResume() {
        super.onResume();
        updateDetail();
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        boolean enable = false;
        int progress = 0;

        if (mAudioEffectManager == null)
            mAudioEffectManager = AudioEffectManager.getInstance(getActivity());

        setPreferencesFromResource(R.xml.dolby_audioeffect_2_4, null);

        final ListPreference dap24Pref = (ListPreference) findPreference(KEY_DAP_2_4_PROFILE);
        mDap24DetailPref = (PreferenceCategory) findPreference(KEY_DAP_2_4_DETAIL);

        mSuvPref = (ListPreference) findPreference(KEY_DAP_2_4_SURROUND_VIRTUALIZER_MODE);
        mSuvBoostPref = (SeekBarPreference) findPreference(KEY_DAP_2_4_SURROUND_VIRTUALIZER_BOOST);
        mDePref = (TwoStatePreference) findPreference(KEY_DAP_2_4_DIALOGUE_ENHANCER_ENABLE);
        mDeAmountPref = (SeekBarPreference) findPreference(KEY_DAP_2_4_DIALOGUE_ENHANCER_AMOUNT);
        mBePref = (TwoStatePreference) findPreference(KEY_DAP_2_4_BASS_ENHANCER_ENABLE);
        mBeBoostPref = (SeekBarPreference) findPreference(KEY_DAP_2_4_BASS_ENHANCER_BOOST);
        mBeCutoffX100Pref = (SeekBarPreference) findPreference(KEY_DAP_2_4_BASS_ENHANCER_CUTOFFX100);
        mBeCutoffX1Pref = (SeekBarPreference) findPreference(KEY_DAP_2_4_BASS_ENHANCER_CUTOFFX1);
        mBeWidthPref = (SeekBarPreference) findPreference(KEY_DAP_2_4_BASS_ENHANCER_WIDTH);
        mMsPref = (TwoStatePreference) findPreference(KEY_DAP_2_4_MI_STEERING);
        mSdePref = (TwoStatePreference) findPreference(KEY_DAP_2_4_SURROUND_DECODER_ENABLE);
        mLePref = (ListPreference) findPreference(KEY_DAP_2_4_LEVELER_SETTING);
        mLeAmountPref = (SeekBarPreference) findPreference(KEY_DAP_2_4_LEVELER_AMOUNT);

        progress = mAudioEffectManager.getDapParam(AudioEffectManager.CMD_DAP_2_4_PROFILE);
        dap24Pref.setValueIndex(progress);
        dap24Pref.setOnPreferenceChangeListener(this);

        mSuvPref.setOnPreferenceChangeListener(this);
        mSuvBoostPref.setMin(0);
        mSuvBoostPref.setMax(96);
        mSuvBoostPref.setOnPreferenceChangeListener(this);
        mSuvBoostPref.setSeekBarIncrement(1);
        mDePref.setOnPreferenceChangeListener(this);
        mDeAmountPref.setMin(0);
        mDeAmountPref.setMax(16);
        mDeAmountPref.setOnPreferenceChangeListener(this);
        mBePref.setOnPreferenceChangeListener(this);
        mBeBoostPref.setMin(0);
        mBeBoostPref.setMax(384);
        mBeBoostPref.setOnPreferenceChangeListener(this);
        mBeBoostPref.setSeekBarIncrement(2);
        mBeCutoffX100Pref.setMin(0);
        mBeCutoffX100Pref.setMax(200);
        mBeCutoffX100Pref.setOnPreferenceChangeListener(this);
        mBeCutoffX100Pref.setSeekBarIncrement(1);
        mBeCutoffX1Pref.setMin(0);
        mBeCutoffX1Pref.setMax(100);
        mBeCutoffX1Pref.setOnPreferenceChangeListener(this);
        mBeCutoffX1Pref.setSeekBarIncrement(1);
        mBeWidthPref.setMin(2);
        mBeWidthPref.setMax(64);
        mBeWidthPref.setOnPreferenceChangeListener(this);
        mBeWidthPref.setSeekBarIncrement(1);
        mMsPref.setOnPreferenceChangeListener(this);
        mSdePref.setOnPreferenceChangeListener(this);
        mLePref.setOnPreferenceChangeListener(this);
        mLeAmountPref.setMin(0);
        mLeAmountPref.setMax(10);
        mLeAmountPref.setOnPreferenceChangeListener(this);

    }

    private String getShowString(int resid) {
        return getActivity().getResources().getString(resid);
    }

    private void updateDetail() {
        boolean enable = false;
        boolean isUserMode = false;
        int val = 0, progress = 0;
        int mode = 0;
        int val_boost = 0, val_cutoffX100 = 0, val_cutoffX1 = 0, val_width = 0;
        int progress_suv = 0, progress_le = 0;

        mode = mAudioEffectManager.getDapParam(AudioEffectManager.CMD_DAP_2_4_PROFILE);

        if (mode != AudioEffectManager.SOUND_EFFECT_DAP_2_4_PROFILE_USER_SELECTABLE) {

            mSuvPref.setVisible(false);
            mSuvBoostPref.setVisible(false);
            mDePref.setVisible(false);
            mDeAmountPref.setVisible(false);
            mBePref.setVisible(false);
            mBeBoostPref.setVisible(false);
            mBeCutoffX100Pref.setVisible(false);
            mBeCutoffX1Pref.setVisible(false);
            mBeWidthPref.setVisible(false);
            mMsPref.setVisible(false);
            mSdePref.setVisible(false);
            mLePref.setVisible(false);
            mLeAmountPref.setVisible(false);
            mDap24DetailPref.setTitle("");
            return;
        } else if (mode == AudioEffectManager.SOUND_EFFECT_DAP_2_4_PROFILE_USER_SELECTABLE) {
            isUserMode = true;
        }

        progress_suv = mAudioEffectManager.getDapParam(AudioEffectManager.CMD_DAP_2_4_SURROUND_VIRTUALIZER);
        val = mAudioEffectManager.getDapParam(AudioEffectManager.SUBCMD_DAP_2_4_SURROUND_VIRTUALIZER_BOOST);
        mSuvBoostPref.setValue(val);
        mSuvBoostPref.setAdjustable(isUserMode);
        mSuvBoostPref.setTitle(isUserMode?getShowString(R.string.title_dap_2_4_surround_virtualizer_boost):"");
        mSuvPref.setVisible(isUserMode);
        if (progress_suv != AudioEffectManager.SOUND_EFFECT_DAP_2_4_SURROUND_VIRTUALIZER_OFF) {
            mSuvBoostPref.setVisible(true);
        } else {
            mSuvBoostPref.setVisible(false);
        }

        enable = (mAudioEffectManager.getDapParam(AudioEffectManager.CMD_DAP_2_4_DIALOGUE_ENHANCER) != AudioEffectManager.DAP_OFF);
        val = mAudioEffectManager.getDapParam(AudioEffectManager.SUBCMD_DAP_2_4_DIALOGUE_ENHANCER_AMOUNT);
        mDePref.setChecked(enable);
        mDeAmountPref.setValue(val);
        mDeAmountPref.setAdjustable(isUserMode);
        mDeAmountPref.setTitle(isUserMode?getShowString(R.string.title_dap_2_4_dialogue_enhancer_amount):"");
        mDePref.setVisible(isUserMode);
        mDeAmountPref.setVisible(enable);

        enable = (mAudioEffectManager.getDapParam(AudioEffectManager.CMD_DAP_2_4_BASS_ENHANCER) != AudioEffectManager.DAP_OFF);
        val_boost = mAudioEffectManager.getDapParam(AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_BOOST);
        val_cutoffX100 = mAudioEffectManager.getDapParam(AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_CUTOFFX100) / 100;
        val_cutoffX1 = mAudioEffectManager.getDapParam(AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_CUTOFFX1);
        val_width = mAudioEffectManager.getDapParam(AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_WIDTH);
        mBePref.setChecked(enable);
        mBeBoostPref.setValue(val_boost);
        mBeCutoffX100Pref.setValue(val_cutoffX100);
        mBeCutoffX1Pref.setValue(val_cutoffX1);
        mBeWidthPref.setValue(val_width);
        mBeBoostPref.setAdjustable(isUserMode);
        mBeCutoffX100Pref.setAdjustable(isUserMode);
        mBeCutoffX1Pref.setAdjustable(isUserMode);
        mBeWidthPref.setAdjustable(isUserMode);
        mBeBoostPref.setTitle(isUserMode?getShowString(R.string.title_dap_2_4_bass_enhancer_boost):"");
        mBeCutoffX100Pref.setTitle(isUserMode?getShowString(R.string.title_dap_2_4_bass_enhancer_cutoffX100):"");
        mBeCutoffX1Pref.setTitle(isUserMode?getShowString(R.string.title_dap_2_4_bass_enhancer_cutoffX1):"");
        mBeWidthPref.setTitle(isUserMode?getShowString(R.string.title_dap_2_4_bass_enhancer_width):"");
        mBePref.setVisible(isUserMode);
        mBeBoostPref.setVisible(enable);
        mBeCutoffX100Pref.setVisible(enable);
        mBeCutoffX1Pref.setVisible(enable);
        mBeWidthPref.setVisible(enable);

        enable = (mAudioEffectManager.getDapParam(AudioEffectManager.CMD_DAP_2_4_MI_STEERING) != AudioEffectManager.DAP_OFF);
        mMsPref.setChecked(enable);
        mMsPref.setVisible(isUserMode);

        enable = (mAudioEffectManager.getDapParam(AudioEffectManager.CMD_DAP_2_4_SURROUND_DECODER_ENABLE) != AudioEffectManager.DAP_OFF);
        mSdePref.setChecked(enable);
        mSdePref.setVisible(isUserMode);

        progress_le = mAudioEffectManager.getDapParam(AudioEffectManager.CMD_DAP_2_4_LEVELER);
        val = mAudioEffectManager.getDapParam(AudioEffectManager.SUBCMD_DAP_2_4_LEVELER_AMOUNT);
        mLeAmountPref.setValue(val);
        mLeAmountPref.setAdjustable(isUserMode);
        mLeAmountPref.setTitle(isUserMode?getShowString(R.string.title_dap_2_4_leveler_amount):"");
        mLePref.setVisible(isUserMode);
        if (progress_le != AudioEffectManager.SOUND_EFFECT_DAP_2_4_LEVELER_OFF) {
            mLeAmountPref.setVisible(true);
        } else {
            mLeAmountPref.setVisible(false);
        }

        mDap24DetailPref.setTitle(R.string.title_dap_2_4_detail);
    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        boolean isChecked;
        switch (preference.getKey()) {
            case KEY_DAP_2_4_DIALOGUE_ENHANCER_ENABLE:
                isChecked = mDePref.isChecked();
                mAudioEffectManager.setDapParam(AudioEffectManager.CMD_DAP_2_4_DIALOGUE_ENHANCER, isChecked?1:0);
                mDeAmountPref.setVisible(isChecked);
                break;
            case KEY_DAP_2_4_BASS_ENHANCER_ENABLE:
                isChecked = mBePref.isChecked();
                mAudioEffectManager.setDapParam(AudioEffectManager.CMD_DAP_2_4_BASS_ENHANCER, isChecked?1:0);
                mBeBoostPref.setVisible(isChecked);
                mBeCutoffX100Pref.setVisible(isChecked);
                mBeCutoffX1Pref.setVisible(isChecked);
                mBeWidthPref.setVisible(isChecked);
                break;
            case KEY_DAP_2_4_MI_STEERING:
                isChecked = mMsPref.isChecked();
                mAudioEffectManager.setDapParam(AudioEffectManager.CMD_DAP_2_4_MI_STEERING, isChecked?1:0);
                break;
            case KEY_DAP_2_4_SURROUND_DECODER_ENABLE:
                isChecked = mSdePref.isChecked();
                mAudioEffectManager.setDapParam(AudioEffectManager.CMD_DAP_2_4_SURROUND_DECODER_ENABLE, isChecked?1:0);
                break;
        }
        return super.onPreferenceTreeClick(preference);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        switch (preference.getKey()) {
            case KEY_DAP_2_4_PROFILE:
                final int selection = Integer.parseInt((String)newValue);
                mAudioEffectManager.setDapParam(AudioEffectManager.CMD_DAP_2_4_PROFILE, selection);
                break;
            case KEY_DAP_2_4_SURROUND_VIRTUALIZER_MODE:
                final int mode = Integer.parseInt((String)newValue);
                mAudioEffectManager.setDapParam(AudioEffectManager.CMD_DAP_2_4_SURROUND_VIRTUALIZER, mode);
                break;
            case KEY_DAP_2_4_SURROUND_VIRTUALIZER_BOOST:
                mAudioEffectManager.setDapParam(AudioEffectManager.SUBCMD_DAP_2_4_SURROUND_VIRTUALIZER_BOOST, (int)newValue);
                break;
            case KEY_DAP_2_4_DIALOGUE_ENHANCER_AMOUNT:
                mAudioEffectManager.setDapParam(AudioEffectManager.SUBCMD_DAP_2_4_DIALOGUE_ENHANCER_AMOUNT, (int)newValue);
                break;
            case KEY_DAP_2_4_BASS_ENHANCER_BOOST:
                mAudioEffectManager.setDapParam(AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_BOOST, (int)newValue);
                break;
            case KEY_DAP_2_4_BASS_ENHANCER_CUTOFFX100:
                mAudioEffectManager.setDapParam(AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_CUTOFFX100, (int)newValue);
                break;
            case KEY_DAP_2_4_BASS_ENHANCER_CUTOFFX1:
                mAudioEffectManager.setDapParam(AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_CUTOFFX1, (int)newValue);
                break;
            case KEY_DAP_2_4_BASS_ENHANCER_WIDTH:
                mAudioEffectManager.setDapParam(AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_WIDTH, (int)newValue);
                break;
            case KEY_DAP_2_4_LEVELER_SETTING:
                final int setting = Integer.parseInt((String)newValue);
                mAudioEffectManager.setDapParam(AudioEffectManager.CMD_DAP_2_4_LEVELER, setting);
                break;
            case KEY_DAP_2_4_LEVELER_AMOUNT:
                mAudioEffectManager.setDapParam(AudioEffectManager.SUBCMD_DAP_2_4_LEVELER_AMOUNT, (int)newValue);
                break;
        }
        return true;
    }

    @Override
    public int getMetricsCategory() {
        return 0;
    }

}
