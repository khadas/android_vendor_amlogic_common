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

package com.android.tv.settings;

import android.content.ContentResolver;
import android.content.Context;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.os.Bundle;
import android.os.SystemProperties;
import android.provider.Settings;
import androidx.preference.SwitchPreference;
import androidx.preference.ListPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceCategory;
import androidx.preference.PreferenceScreen;
import androidx.preference.PreferenceViewHolder;
import androidx.preference.SeekBarPreference;
import androidx.preference.TwoStatePreference;
import android.text.TextUtils;
import android.util.Log;

import java.util.*;
import java.lang.reflect.Method;

import com.droidlogic.app.OutputModeManager;
import com.droidlogic.app.SystemControlManager;
import com.droidlogic.app.AudioConfigManager;
import com.droidlogic.app.tv.AudioEffectManager;
import com.android.tv.settings.SettingsConstant;
import com.android.tv.settings.SettingsPreferenceFragment;
import com.android.tv.settings.R;
import com.android.tv.settings.tvoption.SoundParameterSettingManager;

public class SoundFragment extends SettingsPreferenceFragment implements Preference.OnPreferenceChangeListener {
    public static final String TAG = "SoundFragment";

    private static final String KEY_DOLBY_DRCMODE_PASSTHROUGH               = "key_dolby_drc_mode";          /* Dolby Sounds */
    private static final String KEY_DIGITALSOUND_CATEGORY                   = "surround_sound_category";
    private static final String KEY_DIGITALSOUND_PREFIX                     = "digital_subformat_";
    public static final String KEY_DIGITALSOUND_FORMAT                      = "key_digital_audio_format";
    private static final String KEY_DTSDRCMODE_PASSTHROUGH                  = "dtsdrc_mode";
    private static final String KEY_DTSDRCCUSTOMMODE_PASSTHROUGH            = "dtsdrc_custom_mode";
    private static final String KEY_SOUND_AD_MIXING                         = "key_sound_ad_mixing";
    private static final String KEY_DAP                                     = "dolby_audio_processing";
    private static final String KEY_ARC_LATENCY                             = "arc_latency";                /* HDMI/ARC latency */
    public static final String KEY_AUDIO_OUTPUT_LATENCY                     = "key_audio_output_latency";   /* Audio Output Latency */
    public static final String KEY_FORCE_DDP                                = "key_force_ddp";   /* Audio Output DDP for ms12 v2 ,default is mat*/
    public static final String KEY_SOUND_TV_OUTPUT_DEVICE_HDMI_OUT          = "key_sound_tv_output_device_hdmi_out";   /* for soundbar product HDMI TX output*/
    public static final String KEY_TV_DTS_VIRTUALX_EFFECT                   = "key_dts_virtualx_settings_truvolumehd";   /* for soundbar product HDMI TX output*/
    public static final String KEY_DTS_VIRTUALX_SETTINGS                    = "key_dts_virtualx_settings";


    public static final int KEY_AUDIO_OUTPUT_LATENCY_STEP                   = 10; // 10ms

    private OutputModeManager mOutputModeManager;
    private AudioConfigManager mAudioConfigManager;
    private SystemControlManager mSystemControlManager;
    private AudioEffectManager mAudioEffectManager;
    private SoundParameterSettingManager mSoundParameterSettingManager;
    private PreferenceCategory mCategoryPref;
    private Map<Integer, Boolean> mFormats;
    public static SoundFragment newInstance() {
        return new SoundFragment();
    }

    private boolean CanDebug() {
        return SystemProperties.getBoolean("sys.sound.debug", false);
    }

    private String[] getArrayString(int resid) {
        return getActivity().getResources().getStringArray(resid);
    }

    @Override
    public void onAttach(Context context) {
        AudioManager am = context.getSystemService(AudioManager.class);
        try {
            Method getSurroundFormats = AudioManager.class.getMethod("getSurroundFormats");
            mFormats = (Map<Integer, Boolean>)getSurroundFormats.invoke(am);
        }catch(Exception ex){
            ex.printStackTrace();
        }

        super.onAttach(context);
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        if (mSoundParameterSettingManager == null) {
            mSoundParameterSettingManager = new SoundParameterSettingManager(getActivity());
        }
        if (mAudioConfigManager == null) {
            mAudioConfigManager = AudioConfigManager.getInstance(getActivity());
        }
        super.onCreate(savedInstanceState);
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        setPreferencesFromResource(R.xml.sound, null);
        if (mAudioEffectManager == null) {
            mAudioEffectManager = AudioEffectManager.getInstance(getActivity().getApplicationContext());
        }

        final ListPreference dolbyDrcmodePref = (ListPreference) findPreference(KEY_DOLBY_DRCMODE_PASSTHROUGH);

        final ListPreference dtsdrccustommodePref = (ListPreference) findPreference(KEY_DTSDRCCUSTOMMODE_PASSTHROUGH);
        final ListPreference dtsdrcmodePref = (ListPreference) findPreference(KEY_DTSDRCMODE_PASSTHROUGH);
        final TwoStatePreference adsurport = (TwoStatePreference) findPreference(KEY_SOUND_AD_MIXING);
        final Preference dapPref = (Preference) findPreference(KEY_DAP);
        final SeekBarPreference arcPref = (SeekBarPreference) findPreference(KEY_ARC_LATENCY);
        final SeekBarPreference audioOutputLatencyPref = (SeekBarPreference) findPreference(KEY_AUDIO_OUTPUT_LATENCY);
        final TwoStatePreference forceddp = (TwoStatePreference) findPreference(KEY_FORCE_DDP);
        final TwoStatePreference hdmiOutPref = (TwoStatePreference) findPreference(KEY_SOUND_TV_OUTPUT_DEVICE_HDMI_OUT);
        final Preference mDtsVirtualxSettingsPref = (Preference) findPreference(KEY_DTS_VIRTUALX_SETTINGS);
        final TwoStatePreference mTruVolumeHdPref = (TwoStatePreference) findPreference(KEY_TV_DTS_VIRTUALX_EFFECT);

        mSystemControlManager = SystemControlManager.getInstance();

        mOutputModeManager = OutputModeManager.getInstance(getActivity());
        if (!mOutputModeManager.isAudioSupportMs12System())
            dapPref.setVisible(false);

        forceddp.setChecked(mOutputModeManager.getForceDDPEnable());
        if (!mOutputModeManager.isAudioSupportMs12System())
            forceddp.setVisible(false);

        dolbyDrcmodePref.setValue(mSoundParameterSettingManager.getDrcModePassthroughSetting());
        dolbyDrcmodePref.setOnPreferenceChangeListener(this);

        dtsdrcmodePref.setValue(mSystemControlManager.getPropertyString("persist.vendor.sys.dtsdrcscale", OutputModeManager.DEFAULT_DRC_SCALE));
        dtsdrcmodePref.setOnPreferenceChangeListener(this);
        adsurport.setChecked(mOutputModeManager.getAdSurportEnable());
        arcPref.setOnPreferenceChangeListener(this);
        arcPref.setMax(OutputModeManager.TV_ARC_LATENCY_MAX);
        arcPref.setMin(OutputModeManager.TV_ARC_LATENCY_MIN);
        arcPref.setSeekBarIncrement(10);
        arcPref.setValue(mSoundParameterSettingManager.getARCLatency());
        /* Temporary unused "HDMI/ARC latency" */
        arcPref.setVisible(false);

        audioOutputLatencyPref.setOnPreferenceChangeListener(this);
        audioOutputLatencyPref.setMax(AudioConfigManager.HAL_AUDIO_OUT_DEV_DELAY_MIN);
        audioOutputLatencyPref.setMin(AudioConfigManager.HAL_AUDIO_OUT_DEV_DELAY_MAX);
        audioOutputLatencyPref.setSeekBarIncrement(KEY_AUDIO_OUTPUT_LATENCY_STEP);
        audioOutputLatencyPref.setValue(mAudioConfigManager.getAudioOutputAllDelay());

        /* Temporary unused "Audio Output Latency " */
        audioOutputLatencyPref.setVisible(false);

        boolean tvFlag = SettingsConstant.needDroidlogicTvFeature(getContext());
        if (!mSystemControlManager.getPropertyBoolean("ro.vendor.platform.support.dolby", false)) {
            dolbyDrcmodePref.setVisible(false);
            Log.d(TAG, "platform doesn't support dolby");
        }
        if (!mSystemControlManager.getPropertyBoolean("ro.vendor.platform.support.dts", false)) {
            dtsdrcmodePref.setVisible(false);
            dtsdrccustommodePref.setVisible(false);
            Log.d(TAG, "platform doesn't support dts");
        } else if (mSystemControlManager.getPropertyBoolean("persist.vendor.sys.dtsdrccustom", false)) {
            dtsdrcmodePref.setVisible(false);
        } else {
            dtsdrccustommodePref.setVisible(false);
        }
        hdmiOutPref.setVisible(true);
        hdmiOutPref.setChecked(mOutputModeManager.isTvAudioHdmiOutOn());

        final ListPreference digitalSoundPref = (ListPreference) findPreference(KEY_DIGITALSOUND_FORMAT);
        digitalSoundPref.setValue(mSoundParameterSettingManager.getDigitalAudioFormat());
        if (tvFlag) {
            /* not support passthrough when ms12 so are not included.*/
            if (!mSoundParameterSettingManager.isAudioSupportMs12System()) {
                String[] entry = getArrayString(R.array.digital_sounds_tv_entries);
                String[] entryValue = getArrayString(R.array.digital_sounds_tv_entry_values);
                List<String> entryList = new ArrayList<String>(Arrays.asList(entry));
                List<String> entryValueList = new ArrayList<String>(Arrays.asList(entryValue));
                entryList.remove("Passthough");
                entryValueList.remove(SoundParameterSettingManager.DIGITAL_SOUND_PASSTHROUGH);
                digitalSoundPref.setEntries(entryList.toArray(new String[]{}));
                digitalSoundPref.setEntryValues(entryValueList.toArray(new String[]{}));
            } else {
                digitalSoundPref.setEntries(R.array.digital_sounds_tv_entries);
                digitalSoundPref.setEntryValues(R.array.digital_sounds_tv_entry_values);
            }
        } else {
            digitalSoundPref.setEntries(R.array.digital_sounds_box_entries);
            digitalSoundPref.setEntryValues(R.array.digital_sounds_box_entry_values);
            //adsurport.setVisible(false);
        }
        digitalSoundPref.setOnPreferenceChangeListener(this);
        mCategoryPref = (PreferenceCategory) findPreference(KEY_DIGITALSOUND_CATEGORY);
        digitalSoundPref.setVisible(false);

        mDtsVirtualxSettingsPref.setEnabled(mAudioEffectManager.isSupportVirtualX());
        mDtsVirtualxSettingsPref.setVisible(mAudioEffectManager.isSupportVirtualX());

        mTruVolumeHdPref.setChecked(mAudioEffectManager.getDtsTruVolumeHdEnable());
        mTruVolumeHdPref.setEnabled(mAudioEffectManager.isSupportVirtualX());
        mTruVolumeHdPref.setVisible(mAudioEffectManager.isSupportVirtualX());

        //remove unuse setting
        createFormatPreferences();
        updateFormatPreferencesStates();
    }

    /** Creates and adds switches for each surround sound format. */
    private void createFormatPreferences() {
        for (Map.Entry<Integer, Boolean> format : mFormats.entrySet()) {
            int formatId = format.getKey();
            boolean enabled = format.getValue();
            SwitchPreference pref = new SwitchPreference(
                    getPreferenceManager().getContext()) {
                @Override
                public void onBindViewHolder(PreferenceViewHolder holder) {
                    super.onBindViewHolder(holder);
                    // Enabling the view will ensure that the preference is focusable even if it
                    // the preference is disabled. This allows the user to scroll down over the
                    // disabled surround sound formats and see them all.
                    holder.itemView.setEnabled(true);
                }
            };
            pref.setTitle(getFormatDisplayName(formatId));
            pref.setKey(KEY_DIGITALSOUND_PREFIX + formatId);
            pref.setChecked(enabled);
            mCategoryPref.addPreference(pref);
        }
    }

    /**
     * @return the display name for each surround sound format.
     */
    private String getFormatDisplayName(int formatId) {
        switch (formatId) {
            case AudioFormat.ENCODING_AC3:
                return getContext().getResources().getString(
                        R.string.surround_sound_format_ac3);
            case AudioFormat.ENCODING_E_AC3:
                return getContext().getResources().getString(
                        R.string.surround_sound_format_e_ac3);
            case AudioFormat.ENCODING_DTS:
                return getContext().getResources().getString(
                        R.string.surround_sound_format_dts);
            case AudioFormat.ENCODING_DTS_HD:
                return getContext().getResources().getString(
                        R.string.surround_sound_format_dts_hd);
            case AudioFormat.ENCODING_AAC_LC:
                return getContext().getResources().getString(
                        R.string.surround_sound_format_aac_lc);
            case AudioFormat.ENCODING_DOLBY_TRUEHD:
                return getContext().getResources().getString(
                        R.string.surround_sound_format_truehd);
            case AudioFormat.ENCODING_E_AC3_JOC:
                return getContext().getResources().getString(
                        R.string.surround_sound_format_e_ac3_joc);
            case AudioFormat.ENCODING_AC4:
                return getContext().getResources().getString(
                        R.string.surround_sound_format_ac4);
            default:
                return "Unknown surround sound format";
        }
    }

    private void updateFormatPreferencesStates() {
        boolean show = mSoundParameterSettingManager.DIGITAL_SOUND_MANUAL.equals(
                mSoundParameterSettingManager.getDigitalAudioFormat());
        HashSet<Integer> fmts = new HashSet<>();
        if (show) {
            String enable = mSoundParameterSettingManager.getAudioManualFormats();
            if (!enable.isEmpty()) {
                try {
                    Arrays.stream(enable.split(",")).mapToInt(Integer::parseInt)
                        .forEach(fmts::add);
                } catch (NumberFormatException e) {
                    Log.w(TAG, "DIGITAL_AUDIO_SUBFORMAT misformatted.", e);
                }
            }
        }

        for (Map.Entry<Integer, Boolean> format : mFormats.entrySet()) {
            int formatId = format.getKey();
            String key = KEY_DIGITALSOUND_PREFIX+formatId;
            SwitchPreference preference =
                    (SwitchPreference)mCategoryPref.findPreference(key);
            preference.setVisible(show);
            if (show) {
                if (fmts.contains(formatId))
                    preference.setChecked(true);
                else
                    preference.setChecked(false);
            }
        }
    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        String key = preference.getKey();
        if (key.startsWith(KEY_DIGITALSOUND_PREFIX)) {
            mSoundParameterSettingManager.setAudioManualFormats(
                    Integer.parseInt(key.substring(KEY_DIGITALSOUND_PREFIX.length())),
                    ((SwitchPreference) preference).isChecked());
        } else if (KEY_SOUND_AD_MIXING.equals(key)) {
            final TwoStatePreference adsurport = (TwoStatePreference) findPreference(KEY_SOUND_AD_MIXING);
            mOutputModeManager.setAdSurportEnable(adsurport.isChecked());
        } else if(KEY_FORCE_DDP.equals(key)) {
            TwoStatePreference pref = (TwoStatePreference)preference;
            mOutputModeManager.setForceDDPEnable(pref.isChecked());
        } else if (KEY_SOUND_TV_OUTPUT_DEVICE_HDMI_OUT.equals(key)) {
            final TwoStatePreference hdmiOut = (TwoStatePreference) findPreference(KEY_SOUND_TV_OUTPUT_DEVICE_HDMI_OUT);
            mOutputModeManager.setTvAudioHdmiOutOn(hdmiOut.isChecked());
        } else if (KEY_TV_DTS_VIRTUALX_EFFECT.equals(key)) {
            final TwoStatePreference truVolumeHd = (TwoStatePreference) findPreference(KEY_TV_DTS_VIRTUALX_EFFECT);
            mAudioEffectManager.setDtsTruVolumeHdEnable(truVolumeHd.isChecked());
        }
        return super.onPreferenceTreeClick(preference);
    }

    @Override
    public int getMetricsCategory() {
        return 0;
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        if (TextUtils.equals(preference.getKey(), KEY_DOLBY_DRCMODE_PASSTHROUGH)) {
            final String selection = (String) newValue;
            switch (selection) {
            case SoundParameterSettingManager.DRC_OFF:
                mOutputModeManager.enableDobly_DRC(false);
                mOutputModeManager.setDoblyMode(OutputModeManager.LINE_DRCMODE);
                mSoundParameterSettingManager.setDrcModePassthroughSetting(OutputModeManager.IS_DRC_OFF);
                break;
            case SoundParameterSettingManager.DRC_LINE:
                mOutputModeManager.enableDobly_DRC(true);
                mOutputModeManager.setDoblyMode(OutputModeManager.LINE_DRCMODE);
                mSoundParameterSettingManager.setDrcModePassthroughSetting(OutputModeManager.IS_DRC_LINE);
                break;
            case SoundParameterSettingManager.DRC_RF:
                mOutputModeManager.enableDobly_DRC(false);
                mOutputModeManager.setDoblyMode(OutputModeManager.RF_DRCMODE);
                mSoundParameterSettingManager.setDrcModePassthroughSetting(OutputModeManager.IS_DRC_RF);
                break;
            default:
                throw new IllegalArgumentException("Unknown drc mode pref value");
            }
        } else if (TextUtils.equals(preference.getKey(), KEY_DIGITALSOUND_FORMAT)) {
            final String selection = (String)newValue;
            mSoundParameterSettingManager.setDigitalAudioFormat(selection);
            updateFormatPreferencesStates();
        } else if (TextUtils.equals(preference.getKey(), KEY_DTSDRCMODE_PASSTHROUGH)) {
            final String selection = (String) newValue;
            mOutputModeManager.setDtsDrcScale(selection);
        } else if (TextUtils.equals(preference.getKey(), KEY_ARC_LATENCY)) {
            mSoundParameterSettingManager.setARCLatency((int)newValue);
        } else if (TextUtils.equals(preference.getKey(), KEY_AUDIO_OUTPUT_LATENCY)) {
            mAudioConfigManager.setAudioOutputAllDelay((int)newValue);
        } else if (TextUtils.equals(preference.getKey(), KEY_TV_DTS_VIRTUALX_EFFECT)) {
            mAudioEffectManager.setDtsVirtualXMode((int)newValue);
        }
        return true;
    }
}
