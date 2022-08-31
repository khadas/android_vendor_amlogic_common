/*
 * Copyright (C) 2020 The Android Open Source Project
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

package com.android.tv.settings.device.displaysound;

import static com.android.tv.settings.util.InstrumentationUtils.logEntrySelected;
import static com.android.tv.settings.util.InstrumentationUtils.logToggleInteracted;

import android.app.tvsettings.TvSettingsEnums;
import android.content.Context;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.os.Bundle;
import android.provider.Settings;
import android.text.TextUtils;

import androidx.annotation.Keep;
import androidx.preference.ListPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceCategory;
import androidx.preference.PreferenceViewHolder;
import androidx.preference.SwitchPreference;

import com.android.internal.annotations.VisibleForTesting;
import com.android.internal.logging.nano.MetricsProto;
import com.android.settingslib.core.AbstractPreferenceController;
import com.android.tv.settings.PreferenceControllerFragment;
import com.android.tv.settings.R;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

// DroidLogic start modify, add passthrough feature and remove manual feature.
import java.util.Arrays;
import androidx.preference.PreferenceScreen;
import com.droidlogic.app.OutputModeManager;
import com.android.tv.settings.tvoption.SoundParameterSettingManager;
import android.util.Log;
// DroidLogic end

/**
 * The "Advanced sound settings" screen in TV Settings.
 */
@Keep
public class AdvancedVolumeFragment extends PreferenceControllerFragment implements
        Preference.OnPreferenceChangeListener {
    static final String KEY_SURROUND_PASSTHROUGH = "surround_passthrough";
    static final String KEY_SURROUND_SOUND_FORMAT_PREFIX = "surround_sound_format_";
    // DroidLogic start
    public static final String TAG = "AdvancedVolumeFragment";
    static final String KEY_SPDIF_OUTPUT_SWITCH             = "key_spdif_output_switch";
    static final String KEY_AC4_OUTPUT_SWITCH               = "key_ac4_output_switch";
    // DroidLogic end

    static final String KEY_SUPPORTED_SURROUND_SOUND = "supported_formats";
    static final String KEY_UNSUPPORTED_SURROUND_SOUND = "unsupported_formats";

    static final String VAL_SURROUND_SOUND_AUTO = "auto";
    static final String VAL_SURROUND_SOUND_NEVER = "never";
    // DroidLogic start
    static final String VAL_SURROUND_SOUND_ALWAYS = "always";
    // DroidLogic end
    static final String VAL_SURROUND_SOUND_MANUAL = "manual";

    // DroidLogic start
    static final String VAL_SURROUND_SOUND_PASSTHROUGH = "passthrough";
    static final String VAL_SURROUND_SOUND_PCM = "pcm";
    private AudioManager mAudioManager;
    // DroidLogic end

    private Map<Integer, Boolean> mFormats;
    private Map<Integer, Boolean> mReportedFormats;
    private List<AbstractPreferenceController> mPreferenceControllers;

    // DroidLogic start
    private boolean isfromMainsettings = true;
    private String MainSettings = "MainSettings";
    private OutputModeManager mOutputModeManager;
    private SoundParameterSettingManager mSoundParameterSettingManager;
    private SwitchPreference mSpdifSwitchPref;
    private ListPreference mAc4DialogEnhancerPref;
    // DroidLogic end

    private PreferenceCategory mSupportedFormatsPreferenceCategory;
    private PreferenceCategory mUnsupportedFormatsPreferenceCategory;


    @Override
    public void onAttach(Context context) {
        AudioManager audioManager = getAudioManager();
        mFormats = audioManager.getSurroundFormats();
        mReportedFormats = audioManager.getReportedSurroundFormats();
        // DroidLogic start
        mOutputModeManager = OutputModeManager.getInstance(context);
        // DroidLogic end
        super.onAttach(context);
    }

    protected int getPreferenceScreenResId() {
        return R.xml.advanced_sound;
    }

    // DroidLogic start
    private String[] getArrayString(int resid) {
        return getActivity().getResources().getStringArray(resid);
    }

    private SoundParameterSettingManager getSoundParameterSettingManager() {
        if (mSoundParameterSettingManager == null) {
            mSoundParameterSettingManager = new SoundParameterSettingManager(getActivity());
        }
        return mSoundParameterSettingManager;
    }
    // DroidLogic end

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        setPreferencesFromResource(R.xml.advanced_sound, null /* key */);
        // DroidLogic start
        isfromMainsettings = getActivity().toString().contains(MainSettings) ;
        PreferenceScreen advancedSoundScreenPref = getPreferenceScreen();
        // DroidLogic end
        final ListPreference surroundPref = findPreference(KEY_SURROUND_PASSTHROUGH);
        surroundPref.setValue(getDigitalAudioFormat(getContext()));

        // DroidLogic start
        /*
        String surroundPassthroughSetting = getSurroundPassthroughSetting(getContext());
        surroundPref.setValue(surroundPassthroughSetting);
        */
        // DroidLogic end
        surroundPref.setOnPreferenceChangeListener(this);


        // DroidLogic start
        /* createFormatPreferences();
        if (surroundPassthroughSetting == VAL_SURROUND_SOUND_MANUAL) {
            showFormatPreferences();
        } else {
            hideFormatPreferences();
        }*/
        // DroidLogic end

        // DroidLogic start
        /* not support passthrough when ms12 so are not included.*/
        if (!mOutputModeManager.isAudioSupportMs12System() || isfromMainsettings) {
            String[] entry = getArrayString(R.array.surround_sound_entries);
            String[] entryValue = getArrayString(R.array.surround_sound_entry_values);
            List<String> entryList = new ArrayList<String>(Arrays.asList(entry));
            List<String> entryValueList = new ArrayList<String>(Arrays.asList(entryValue));
            entryList.remove(getActivity().getResources().getString(R.string.surround_sound_passthrough_summary));
            entryValueList.remove("passthrough");
            surroundPref.setEntries(entryList.toArray(new String[]{}));
            surroundPref.setEntryValues(entryValueList.toArray(new String[]{}));
            Log.i(TAG, "current platform not support ms12");
        }

        mSpdifSwitchPref = (SwitchPreference) findPreference(KEY_SPDIF_OUTPUT_SWITCH);
        mSpdifSwitchPref.setChecked(mOutputModeManager.getSoundSpdifEnable());

        // add this for the ac4 enhancer ui
        mAc4DialogEnhancerPref = findPreference(KEY_AC4_OUTPUT_SWITCH);
        mAc4DialogEnhancerPref.setValueIndex(getAc4EnhancerValueSetting(getContext())/4);
        mAc4DialogEnhancerPref.setOnPreferenceChangeListener(this);

        if (!mOutputModeManager.isAudioSupportMs12System()) {// when not the ms12 version hide the mAc4DialogEnhancerPref
            advancedSoundScreenPref.removePreference(mAc4DialogEnhancerPref);
        }
        // DroidLogic end

    }

    @Override
    protected List<AbstractPreferenceController> onCreatePreferenceControllers(Context context) {
        // DroidLogic start, add passthrough feature and remove manual feature.
        Log.d(TAG,"mOutputModeManager onCreatePreferenceControllers");
        mOutputModeManager = OutputModeManager.getInstance(context);
        // DroidLogic end, add passthrough feature and remove manual feature.
        mPreferenceControllers = new ArrayList<>(mFormats.size());
        for (Map.Entry<Integer, Boolean> format : mFormats.entrySet()) {
            mPreferenceControllers.add(new SoundFormatPreferenceController(context,
                    format.getKey() /*formatId*/, mFormats, mReportedFormats));
        }
        return mPreferenceControllers;
    }

    @VisibleForTesting
    AudioManager getAudioManager() {
        return getContext().getSystemService(AudioManager.class);
    }

    /** Creates titles and switches for each surround sound format. */
    private void createFormatPreferences() {
        mSupportedFormatsPreferenceCategory = createPreferenceCategory(
                R.string.surround_sound_supported_title,
                KEY_SUPPORTED_SURROUND_SOUND);
        getPreferenceScreen().addPreference(mSupportedFormatsPreferenceCategory);
        mUnsupportedFormatsPreferenceCategory = createPreferenceCategory(
                R.string.surround_sound_unsupported_title,
                KEY_UNSUPPORTED_SURROUND_SOUND);
        getPreferenceScreen().addPreference(mUnsupportedFormatsPreferenceCategory);

        for (Map.Entry<Integer, Boolean> format : mFormats.entrySet()) {
            int formatId = format.getKey();
            boolean enabled = format.getValue();

            // If the format is not a known surround sound format, do not create a preference
            // for it.
            int titleId = getFormatDisplayResourceId(formatId);
            if (titleId == -1) {
                continue;
            }
            final SwitchPreference pref = new SwitchPreference(getContext()) {
                @Override
                public void onBindViewHolder(PreferenceViewHolder holder) {
                    super.onBindViewHolder(holder);
                    // Enabling the view will ensure that the preference is focusable even if it
                    // the preference is disabled. This allows the user to scroll down over the
                    // disabled surround sound formats and see them all.
                    holder.itemView.setEnabled(true);
                }
            };
            pref.setTitle(titleId);
            pref.setKey(KEY_SURROUND_SOUND_FORMAT_PREFIX + formatId);
            pref.setChecked(enabled);
            if (getEntryId(formatId) != -1) {
                pref.setOnPreferenceClickListener(
                        preference -> {
                            logToggleInteracted(getEntryId(formatId), pref.isChecked());
                            return false;
                        }
                );
            }
            if (mReportedFormats.containsKey(formatId)) {
                mSupportedFormatsPreferenceCategory.addPreference(pref);
            } else {
                mUnsupportedFormatsPreferenceCategory.addPreference(pref);
            }
        }
    }

    private void showFormatPreferences() {
        getPreferenceScreen().addPreference(mSupportedFormatsPreferenceCategory);
        getPreferenceScreen().addPreference(mUnsupportedFormatsPreferenceCategory);
        updateFormatPreferencesStates();
    }

    private void hideFormatPreferences() {
        getPreferenceScreen().removePreference(mSupportedFormatsPreferenceCategory);
        getPreferenceScreen().removePreference(mUnsupportedFormatsPreferenceCategory);
        updateFormatPreferencesStates();
    }

    private PreferenceCategory createPreferenceCategory(int titleResourceId, String key) {
        PreferenceCategory preferenceCategory = new PreferenceCategory(getContext());
        preferenceCategory.setTitle(titleResourceId);
        preferenceCategory.setKey(key);
        return preferenceCategory;
    }

    /**
     * @return the display id for each surround sound format.
     */
    private int getFormatDisplayResourceId(int formatId) {
        switch (formatId) {
            case AudioFormat.ENCODING_AC3:
                return R.string.surround_sound_format_ac3;
            case AudioFormat.ENCODING_E_AC3:
                return R.string.surround_sound_format_e_ac3;
            case AudioFormat.ENCODING_DTS:
                return R.string.surround_sound_format_dts;
            case AudioFormat.ENCODING_DTS_HD:
                return R.string.surround_sound_format_dts_hd;
            case AudioFormat.ENCODING_DOLBY_TRUEHD:
                return R.string.surround_sound_format_dolby_truehd;
            case AudioFormat.ENCODING_E_AC3_JOC:
                return R.string.surround_sound_format_e_ac3_joc;
            case AudioFormat.ENCODING_DOLBY_MAT:
                return R.string.surround_sound_format_dolby_mat;
            default:
                return -1;
        }
    }

    private void updateFormatPreferencesStates() {
        for (AbstractPreferenceController controller : mPreferenceControllers) {
            Preference preference = findPreference(
                    controller.getPreferenceKey());
            if (preference != null) {
                controller.updateState(preference);
            }
        }
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        if (TextUtils.equals(preference.getKey(), KEY_SURROUND_PASSTHROUGH)) {
            final String selection = (String) newValue;
            switch (selection) {
                case VAL_SURROUND_SOUND_AUTO:
                    logEntrySelected(
                            TvSettingsEnums.DISPLAY_SOUND_ADVANCED_SOUNDS_SELECT_FORMATS_AUTO);
                    // DroidLogic start
                    mOutputModeManager.saveDigitalAudioFormatToHal(OutputModeManager.DIGITAL_AUDIO_FORMAT_AUTO, "");
                    // DroidLogic end
                    setSurroundPassthroughSetting(Settings.Global.ENCODED_SURROUND_OUTPUT_AUTO);
                    // DroidLogic start
                    // hideFormatPreferences();
                    // DroidLogic end
                    break;
                case VAL_SURROUND_SOUND_NEVER:
                    logEntrySelected(
                            TvSettingsEnums.DISPLAY_SOUND_ADVANCED_SOUNDS_SELECT_FORMATS_NONE);
                    // DroidLogic start
                    mOutputModeManager.saveDigitalAudioFormatToHal(OutputModeManager.DIGITAL_AUDIO_FORMAT_PCM, "");
                    // DroidLogic end
                    setSurroundPassthroughSetting(Settings.Global.ENCODED_SURROUND_OUTPUT_NEVER);
                    // DroidLogic start
                    // hideFormatPreferences();
                    // DroidLogic end
                    break;
                /* DroidLogic start
                case VAL_SURROUND_SOUND_MANUAL:
                    logEntrySelected(
                            TvSettingsEnums.DISPLAY_SOUND_ADVANCED_SOUNDS_SELECT_FORMATS_MANUAL);
                    setSurroundPassthroughSetting(Settings.Global.ENCODED_SURROUND_OUTPUT_MANUAL);
                    showFormatPreferences();
                DroidLogic end*/
                // DroidLogic start
                case VAL_SURROUND_SOUND_PASSTHROUGH:
                    Log.d(TAG,"VAL_SURROUND_SOUND_PASSTHROUGH"); //// DroidLogic start, add passthrough feature and remove manual feature.
                    mOutputModeManager.saveDigitalAudioFormatToHal(OutputModeManager.DIGITAL_AUDIO_FORMAT_PASSTHROUGH, "");
                    setSurroundPassthroughSetting(Settings.Global.ENCODED_SURROUND_OUTPUT_AUTO);
                    break;
                case VAL_SURROUND_SOUND_ALWAYS:
                    // On Android P ALWAYS is replaced by MANUAL.
                    mOutputModeManager.setDigitalAudioFormatOut(OutputModeManager.DIGITAL_AUDIO_FORMAT_PASSTHROUGH);
                    setSurroundPassthroughSetting(Settings.Global.ENCODED_SURROUND_OUTPUT_ALWAYS);
                    break;
                // DroidLogic end
                /* DroidLogic start
                case VAL_SURROUND_SOUND_ALWAYS:
                    // On Android P ALWAYS is replaced by MANUAL.
                    logEntrySelected(
                            TvSettingsEnums.DISPLAY_SOUND_ADVANCED_SOUNDS_SELECT_FORMATS_MANUAL);
                    setSurroundPassthroughSetting(Settings.Global.ENCODED_SURROUND_OUTPUT_ALWAYS);
                    break;
                case VAL_SURROUND_SOUND_MANUAL:
                    logEntrySelected(
                            TvSettingsEnums.DISPLAY_SOUND_ADVANCED_SOUNDS_SELECT_FORMATS_MANUAL);
                    setSurroundPassthroughSetting(Settings.Global.ENCODED_SURROUND_OUTPUT_MANUAL);
                    break;
                DroidLogic end */
                // DroidLogic end, add passthrough feature and remove manual feature.
                default:
                    throw new IllegalArgumentException("Unknown surround sound pref value: "
                            + selection);
            }
            updateFormatPreferencesStates();
            return true;
        }
        // DroidLogic start
        if (TextUtils.equals(preference.getKey(), KEY_AC4_OUTPUT_SWITCH)) {
            final int selection1 = Integer.parseInt(newValue.toString());
            Log.d(TAG,"KEY_AC4_OUTPUT_SWITCH:" + selection1);
            switch (selection1) {
                case OutputModeManager.DIAGLOGUE_ENHANCEMENT_OFF:
                    if (mOutputModeManager.isAudioSupportMs12System()) {
                        mOutputModeManager.setAc4DialogEnhancer(0);
                    }
                    break;
                case OutputModeManager.DIAGLOGUE_ENHANCEMENT_LOW:
                    if (mOutputModeManager.isAudioSupportMs12System()) {
                        mOutputModeManager.setAc4DialogEnhancer(4);
                    }
                    break;
                case OutputModeManager.DIAGLOGUE_ENHANCEMENT_MEDIUM:
                    if (mOutputModeManager.isAudioSupportMs12System()) {
                        mOutputModeManager.setAc4DialogEnhancer(8);
                    }
                    break;
                case OutputModeManager.DIAGLOGUE_ENHANCEMENT_HIGH:
                    if (mOutputModeManager.isAudioSupportMs12System()) {
                        mOutputModeManager.setAc4DialogEnhancer(12);
                    }
                    break;
                default:
                    throw new IllegalArgumentException("Unknown ac4 pref value: "
                            + selection1);
            }
            updateFormatPreferencesStates();
            return true;
       }
       // DroidLogic end
        return true;
    }

    // DroidLogic start
    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        String key = preference.getKey();
        if (key.equals(KEY_SPDIF_OUTPUT_SWITCH)) {
            mOutputModeManager.setSoundSpdifEnable(mSpdifSwitchPref.isChecked());
            return true;
        }
        return super.onPreferenceTreeClick(preference);
    }
    // DroidLogic end

    private void setSurroundPassthroughSetting(int newVal) {
        Settings.Global.putInt(getContext().getContentResolver(),
                Settings.Global.ENCODED_SURROUND_OUTPUT, newVal);
    }

    static String getSurroundPassthroughSetting(Context context) {
        final int value = Settings.Global.getInt(context.getContentResolver(),
                Settings.Global.ENCODED_SURROUND_OUTPUT,
                Settings.Global.ENCODED_SURROUND_OUTPUT_AUTO);

        switch (value) {
            case Settings.Global.ENCODED_SURROUND_OUTPUT_MANUAL:
                return VAL_SURROUND_SOUND_MANUAL;
            case Settings.Global.ENCODED_SURROUND_OUTPUT_NEVER:
                return VAL_SURROUND_SOUND_NEVER;
            // DroidLogic start, add passthrough feature and remove manual feature.
            case Settings.Global.ENCODED_SURROUND_OUTPUT_ALWAYS:
                 return VAL_SURROUND_SOUND_PASSTHROUGH;
            // DroidLogic end, add passthrough feature and remove manual feature.
            case Settings.Global.ENCODED_SURROUND_OUTPUT_AUTO:
            default:
                return VAL_SURROUND_SOUND_AUTO;
            // DroidLogic start,
            // On Android P ALWAYS is replaced by MANUAL.
            //case Settings.Global.ENCODED_SURROUND_OUTPUT_ALWAYS:
            //case Settings.Global.ENCODED_SURROUND_OUTPUT_MANUAL:
            //    return VAL_SURROUND_SOUND_MANUAL;
            // DroidLogic end
        }
    }

    // DroidLogic start
    public String getDigitalAudioFormat(Context context) {
        final int valueGoogle = Settings.Global.getInt(context.getContentResolver(),
                Settings.Global.ENCODED_SURROUND_OUTPUT,
                Settings.Global.ENCODED_SURROUND_OUTPUT_AUTO);
        if (valueGoogle == Settings.Global.ENCODED_SURROUND_OUTPUT_NEVER) {
            return VAL_SURROUND_SOUND_NEVER;
        }
        final int value = Settings.Global.getInt(context.getContentResolver(),
                OutputModeManager.DIGITAL_AUDIO_FORMAT, OutputModeManager.DIGITAL_AUDIO_FORMAT_AUTO);
        Log.d(TAG, "getDigitalAudioFormat value = " + value);
        String format = "";
        switch (value) {
        case OutputModeManager.DIGITAL_AUDIO_FORMAT_PCM:
            format = VAL_SURROUND_SOUND_PCM;
            break;
        case OutputModeManager.DIGITAL_AUDIO_FORMAT_MANUAL:
            format = VAL_SURROUND_SOUND_MANUAL;
            break;
        case OutputModeManager.DIGITAL_AUDIO_FORMAT_AUTO:
            format = VAL_SURROUND_SOUND_AUTO;
            break;
        case OutputModeManager.DIGITAL_AUDIO_FORMAT_PASSTHROUGH:
            format = VAL_SURROUND_SOUND_PASSTHROUGH;
            break;
        default:
            format = VAL_SURROUND_SOUND_AUTO;
        }
        return format;
    }

    static int getAc4EnhancerValueSetting(Context context) {
        final int value = Settings.Global.getInt(context.getContentResolver(),
                OutputModeManager.DIAGLOGUE_ENHANCEMENT_SWITCH,
                OutputModeManager.DIAGLOGUE_ENHANCEMENT_OFF);
        Log.d(TAG,"[getAc4EnhancerValueSetting]:" + value);
        switch (value) {
            case OutputModeManager.DIAGLOGUE_ENHANCEMENT_OFF:
            default:
                 return 0;
            case OutputModeManager.DIAGLOGUE_ENHANCEMENT_LOW:
                 return 4;
            case OutputModeManager.DIAGLOGUE_ENHANCEMENT_MEDIUM:
                 return 8;
            case OutputModeManager.DIAGLOGUE_ENHANCEMENT_HIGH:
                 return 12;
        }
    }
    // DroidLogic end

    private int getEntryId(int formatId) {
        switch(formatId) {
            case AudioFormat.ENCODING_AC4:
                return TvSettingsEnums.DISPLAY_SOUND_ADVANCED_SOUNDS_DAC4;
            case AudioFormat.ENCODING_E_AC3_JOC:
                return TvSettingsEnums.DISPLAY_SOUND_ADVANCED_SOUNDS_DADDP;
            case AudioFormat.ENCODING_AC3:
                return TvSettingsEnums.DISPLAY_SOUND_ADVANCED_SOUNDS_DD;
            case AudioFormat.ENCODING_E_AC3:
                return TvSettingsEnums.DISPLAY_SOUND_ADVANCED_SOUNDS_DDP;
            case AudioFormat.ENCODING_DTS:
                return TvSettingsEnums.DISPLAY_SOUND_ADVANCED_SOUNDS_DTS;
            case AudioFormat.ENCODING_DTS_HD:
                return TvSettingsEnums.DISPLAY_SOUND_ADVANCED_SOUNDS_DTSHD;
            case AudioFormat.ENCODING_AAC_LC:
                return TvSettingsEnums.DISPLAY_SOUND_ADVANCED_SOUNDS_AAC;
            case AudioFormat.ENCODING_DOLBY_TRUEHD:
                return TvSettingsEnums.DISPLAY_SOUND_ADVANCED_SOUNDS_DTHD;
            default:
                return -1;
        }
    }

    @Override
    public int getMetricsCategory() {
        return MetricsProto.MetricsEvent.SOUND;
    }

    @Override
    protected int getPageId() {
        return TvSettingsEnums.DISPLAY_SOUND_ADVANCED_SOUNDS;
    }
}
