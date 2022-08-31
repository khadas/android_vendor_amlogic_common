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
import android.os.Handler;
import android.os.Message;
import android.os.SystemProperties;
import com.android.tv.settings.SettingsPreferenceFragment;
import androidx.preference.ListPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceCategory;
import android.util.Log;
import android.text.TextUtils;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.view.LayoutInflater;
import android.view.ViewGroup;
import android.content.Context;
import android.app.AlertDialog;
import android.view.View.OnClickListener;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.content.DialogInterface;
import android.content.DialogInterface.OnDismissListener;

import com.droidlogic.app.AudioConfigManager;
import com.droidlogic.app.tv.AudioEffectManager;
import com.droidlogic.app.tv.TvControlManager;
import com.droidlogic.app.OutputModeManager;
import com.droidlogic.app.SystemControlManager;

import com.android.tv.settings.TvSettingsActivity;
import com.android.tv.settings.R;
import com.android.tv.settings.tvoption.SoundParameterSettingManager;

public class SoundModeFragment extends SettingsPreferenceFragment implements Preference.OnPreferenceChangeListener, SeekBar.OnSeekBarChangeListener {

    private static final String TAG = "SoundModeFragment";

    private static final String TV_EQ_MODE                                  = "key_tv_sound_mode";
    private static final String TV_TREBLE_BASS_SETTINGS                     = "treble_bass_effect_settings";
    private static final String TV_BALANCE_SETTINGS                         = "balance_effect_settings";
    private static final String TV_VIRTUAL_SURROUND_SETTINGS                = "tv_sound_virtual_surround";
    private static final String TV_SOUND_OUT                                = "tv_sound_output_device";
    private static final String KEY_DOLBY_DAP_EFFECT                        = "key_dolby_dap_effect";
    private static final String AUDIO_ONLY                                  = "tv_sound_audio_only";
    private static final String KEY_TV_SOUND_AUDIO_SOURCE_SELECT            = "key_tv_sound_audio_source_select";

    private AudioConfigManager mAudioConfigManager;
    private AudioEffectManager mAudioEffectManager;
    private TvControlManager mTvControlManager;
    private SoundParameterSettingManager mSoundParameterSettingManager;
    private OutputModeManager mOutputModeManager;

    private static final int AUDIO_OUTPUT_DELAY_SOURCE_DEFAULT               = 0;

    private static int mCurrentSettingSourceId = AudioConfigManager.AUDIO_OUTPUT_DELAY_SOURCE_ATV;
    private ListPreference mTvSourceSelectPref;
    private SeekBar mSeekBarAudioOutputDelaySpeaker;
    private SeekBar mSeekBarAudioOutputDelaySpdif;
    private SeekBar mSeekBarAudioOutputDelayHeadphone;
    private SeekBar mSeekBarAudioPrescale;
    private TextView mTextAudioOutputDelaySpeaker;
    private TextView mTextAudioOutputDelaySpdif;
    private TextView mTextAudioOutputDelayHeadphone;
    private TextView mTextAudioPrescale;
    private boolean mIsDelayAndPrescaleSeekBarInited = false;

    private static final int UI_LOAD_TIMEOUT = 50;//100ms
    private static final int LOAD_UI = 0;
    private static final int AUDIOONLY = 0;

    Handler myHandler = new Handler() {
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what) {
                case LOAD_UI:
                    if (!initView()) {
                        myHandler.sendEmptyMessageDelayed(LOAD_UI, UI_LOAD_TIMEOUT);
                    } else {
                        myHandler.removeCallbacksAndMessages(null);
                    }
                    break;
                default:
                    break;
            }
        }
    };

    public static SoundModeFragment newInstance() {
        return new SoundModeFragment();
    }

    private boolean CanDebug() {
        return OptionParameterManager.CanDebug();
    }

    @Override
    public void onResume() {
        super.onResume();
        if (mAudioEffectManager != null) {
            final ListPreference eqmode = (ListPreference) findPreference(TV_EQ_MODE);
            eqmode.setValueIndex(mAudioEffectManager.getSoundModeStatus());
            final Preference treblebass = (Preference) findPreference(TV_TREBLE_BASS_SETTINGS);
            String treblebasssummary = getShowString(R.string.tv_treble, mAudioEffectManager.getTrebleStatus()) + " " +
                    getShowString(R.string.tv_bass, mAudioEffectManager.getBassStatus());
            treblebass.setSummary(treblebasssummary);
            final Preference balance = (Preference) findPreference(TV_BALANCE_SETTINGS);
            balance.setSummary(getShowString(R.string.tv_balance_effect, mAudioEffectManager.getBalanceStatus()));
        }
        mTvSourceSelectPref.setValueIndex(mCurrentSettingSourceId);
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        Log.d(TAG, "onCreate");
        init();
        super.onCreate(savedInstanceState);
    }

    private void init() {
        mAudioConfigManager = AudioConfigManager.getInstance(getActivity());
        mAudioEffectManager = ((TvSettingsActivity)getActivity()).getAudioEffectManager();
        mTvControlManager = TvControlManager.getInstance();
        mSoundParameterSettingManager = ((TvSettingsActivity)getActivity()).getSoundParameterSettingManager();
        mOutputModeManager = OutputModeManager.getInstance(getActivity());
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        Log.d(TAG, "onCreateView");
        final View innerView = super.onCreateView(inflater, container, savedInstanceState);
        if (getActivity().getIntent().getIntExtra("from_live_tv", 0) == 1) {
            //MainFragment.changeToLiveTvStyle(innerView, getActivity());
        }
        return innerView;
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        setPreferencesFromResource(R.xml.tv_sound_mode, null);
        mTvSourceSelectPref = (ListPreference) findPreference(KEY_TV_SOUND_AUDIO_SOURCE_SELECT);
        mTvSourceSelectPref.setValueIndex(mCurrentSettingSourceId);
        mTvSourceSelectPref.setOnPreferenceChangeListener(this);
        mTvSourceSelectPref.setEnabled(true);
        myHandler.sendEmptyMessage(LOAD_UI);
    }

    private boolean initView() {
        final ListPreference eqmode = (ListPreference) findPreference(TV_EQ_MODE);
        eqmode.setValueIndex(mAudioEffectManager.getSoundModeStatus());
        eqmode.setOnPreferenceChangeListener(this);

        final Preference dapPref = (Preference) findPreference(KEY_DOLBY_DAP_EFFECT);
        if (mOutputModeManager.isAudioSupportMs12System())
            eqmode.setVisible(false);
        else
            dapPref.setVisible(false);

        final ListPreference virtualsurround = (ListPreference) findPreference(TV_VIRTUAL_SURROUND_SETTINGS);
        virtualsurround.setValueIndex(mAudioEffectManager.getVirtualSurroundStatus());
        virtualsurround.setOnPreferenceChangeListener(this);

        final ListPreference soundout = (ListPreference) findPreference(TV_SOUND_OUT);
        soundout.setValueIndex(mSoundParameterSettingManager.getSoundOutputStatus());
        soundout.setOnPreferenceChangeListener(this);

        final Preference treblebass = (Preference) findPreference(TV_TREBLE_BASS_SETTINGS);
        String treblebasssummary = getShowString(R.string.tv_treble, mAudioEffectManager.getTrebleStatus()) + " " +
                getShowString(R.string.tv_bass, mAudioEffectManager.getBassStatus());
        treblebass.setSummary(treblebasssummary);

        final Preference balance = (Preference) findPreference(TV_BALANCE_SETTINGS);
        balance.setSummary(getShowString(R.string.tv_balance_effect, mAudioEffectManager.getBalanceStatus()));

        final Preference audio_only = (Preference) findPreference(AUDIO_ONLY);
        return true;
    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        if (CanDebug()) Log.d(TAG, "[onPreferenceTreeClick] preference.getKey() = " + preference.getKey());
        if (TextUtils.equals(preference.getKey(), AUDIO_ONLY)) {
            createUiDialog(AUDIOONLY);
        }

        return super.onPreferenceTreeClick(preference);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        if (CanDebug()) Log.d(TAG, "[onPreferenceChange] preference.getKey() = " + preference.getKey() + ", newValue = " + newValue);
        final int selection = Integer.parseInt((String)newValue);
        if (TextUtils.equals(preference.getKey(), TV_EQ_MODE)) {
            mAudioEffectManager.setSoundMode(selection);
            // non-user sound mode, set default treble and bass value
            int bassValue = AudioEffectManager.EFFECT_BASS_DEFAULT;
            int trebleValue = AudioEffectManager.EFFECT_TREBLE_DEFAULT;
            if (selection == AudioEffectManager.EQ_SOUND_MODE_CUSTOM) {
                createUiDialog();
                // when sound mode is user, set DB treble and bass value
                bassValue = mAudioEffectManager.getBassStatus();
                trebleValue = mAudioEffectManager.getTrebleStatus();
            }
            mAudioEffectManager.setBass(bassValue);
            mAudioEffectManager.setTreble(trebleValue);
        } else if (TextUtils.equals(preference.getKey(), TV_VIRTUAL_SURROUND_SETTINGS)) {
            mAudioEffectManager.setVirtualSurround(selection);
        }else if (TextUtils.equals(preference.getKey(), TV_SOUND_OUT)) {
            mSoundParameterSettingManager.setSoundOutputStatus(selection);
        } else if (TextUtils.equals(preference.getKey(), KEY_TV_SOUND_AUDIO_SOURCE_SELECT)) {
            mCurrentSettingSourceId = selection;
            createOutputDelayAndPrescaleUiDialog();
        }
        return true;
    }

    @Override
    public int getMetricsCategory() {
        return 0;
    }

    private void createOutputDelayAndPrescaleUiDialog() {
        Context context = (Context) (getActivity());
        LayoutInflater inflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View view = inflater.inflate(R.xml.tv_sound_audio_settings_seekbar, null);//tv_sound_audio_settings_seekbar.xml
        AlertDialog.Builder builder = new AlertDialog.Builder(context);
        final AlertDialog mAlertDialog = builder.create();
        mAlertDialog.setOnDismissListener(new DialogInterface.OnDismissListener() {
            @Override
            public void onDismiss(DialogInterface dialog) {
                mIsDelayAndPrescaleSeekBarInited = false;
            }
        });
        mAlertDialog.show();
        mAlertDialog.getWindow().setContentView(view);
        initOutputDelayAndPrescaleSeekBar(view);
    }

    private void createUiDialog () {
        Context context = (Context) (getActivity());
        LayoutInflater inflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View view = inflater.inflate(R.xml.tv_sound_effect_ui, null);//tv_sound_effect_ui
        AlertDialog.Builder builder = new AlertDialog.Builder(context);
        final AlertDialog mAlertDialog = builder.create();
        mAlertDialog.setOnDismissListener(new OnDismissListener() {
            @Override
            public void onDismiss(DialogInterface dialog) {
                mIsAudioEqSeekBarInited = false;
            }
        });
        mAlertDialog.show();
        mAlertDialog.getWindow().setContentView(view);
        //mAlertDialog.getWindow().setLayout(150, 320);
        initSoundModeEqBandSeekBar(view);
    }

    private void createUiDialog (int type) {
        Context context = (Context) (getActivity());
        LayoutInflater inflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View view = inflater.inflate(R.xml.layout_dialog, null);

        AlertDialog.Builder builder = new AlertDialog.Builder(context);
        final AlertDialog mAlertDialog = builder.create();
        mAlertDialog.show();
        mAlertDialog.getWindow().setContentView(view);
        TextView button_cancel = (TextView)view.findViewById(R.id.dialog_cancel);
        TextView dialogtitle = (TextView)view.findViewById(R.id.dialog_title);
        TextView dialogdetails = (TextView)view.findViewById(R.id.dialog_details);
        if (AUDIOONLY == type) {
            dialogtitle.setText(getActivity().getResources().getString(R.string.title_tv_sound_audio_only));
            dialogdetails.setText(getActivity().getResources().getString(R.string.msg_tv_sound_audio_only));
        }
        button_cancel.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                if (mAlertDialog != null)
                    mAlertDialog.dismiss();
            }
        });
        button_cancel.requestFocus();
        TextView button_ok = (TextView)view.findViewById(R.id.dialog_ok);
        button_ok.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                if (AUDIOONLY == type) {
                    mTvControlManager.setLcdEnable(false);
                    SystemProperties.set("persist.audio.only.state", "true");
                }
                mAlertDialog.dismiss();
            }
        });
    }

    private boolean mIsAudioEqSeekBarInited = false;
    private SeekBar mBand1Seekbar;
    private TextView mBand1Text;
    private SeekBar mBand2Seekbar;
    private TextView mBand2Text;
    private SeekBar mBand3Seekbar;
    private TextView mBand3Text;
    private SeekBar mBand4Seekbar;
    private TextView mBand4Text;
    private SeekBar mBand5Seekbar;
    private TextView mBand5Text;

    private void initSoundModeEqBandSeekBar(View view) {
        if (mAudioEffectManager == null) {
            mAudioEffectManager = ((TvSettingsActivity)getActivity()).getAudioEffectManager();
        }
        int status = -1;
        mBand1Seekbar = (SeekBar) view.findViewById(R.id.seekbar_tv_audio_effect_band1);
        mBand1Text = (TextView) view.findViewById(R.id.text_tv_audio_effect_band1);
        status = mAudioEffectManager.getUserSoundModeParam(AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND1);
        mBand1Seekbar.setOnSeekBarChangeListener(this);
        mBand1Seekbar.setProgress(status);
        setShow(AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND1, status);
        mBand1Seekbar.requestFocus();
        mBand2Seekbar = (SeekBar) view.findViewById(R.id.seekbar_tv_audio_effect_band2);
        mBand2Text = (TextView) view.findViewById(R.id.text_tv_audio_effect_band2);
        status = mAudioEffectManager.getUserSoundModeParam(AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND2);
        mBand2Seekbar.setOnSeekBarChangeListener(this);
        mBand2Seekbar.setProgress(status);
        setShow(AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND2, status);
        mBand3Seekbar = (SeekBar) view.findViewById(R.id.seekbar_tv_audio_effect_band3);
        mBand3Text = (TextView) view.findViewById(R.id.text_tv_audio_effect_band3);
        status = mAudioEffectManager.getUserSoundModeParam(AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND3);
        mBand3Seekbar.setOnSeekBarChangeListener(this);
        mBand3Seekbar.setProgress(status);
        setShow(AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND3, status);
        mBand4Seekbar = (SeekBar) view.findViewById(R.id.seekbar_tv_audio_effect_band4);
        mBand4Text = (TextView) view.findViewById(R.id.text_tv_audio_effect_band4);
        status = mAudioEffectManager.getUserSoundModeParam(AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND4);
        mBand4Seekbar.setOnSeekBarChangeListener(this);
        mBand4Seekbar.setProgress(status);
        setShow(AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND4, status);
        mBand5Seekbar = (SeekBar) view.findViewById(R.id.seekbar_tv_audio_effect_band5);
        mBand5Text = (TextView) view.findViewById(R.id.text_tv_audio_effect_band5);
        status = mAudioEffectManager.getUserSoundModeParam(AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND5);
        mBand5Seekbar.setOnSeekBarChangeListener(this);
        mBand5Seekbar.setProgress(status);
        setShow(AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND5, status);
        mIsAudioEqSeekBarInited = true;
    }

    private void initOutputDelayAndPrescaleSeekBar(View view) {
        int delayMs = 0;

        mSeekBarAudioOutputDelaySpeaker = (SeekBar) view.findViewById(R.id.id_seek_bar_audio_delay_speaker);
        mTextAudioOutputDelaySpeaker = (TextView) view.findViewById(R.id.id_text_view_audio_delay_speaker);
        delayMs = mAudioConfigManager.getAudioOutputSpeakerDelay(mCurrentSettingSourceId);
        mSeekBarAudioOutputDelaySpeaker.setOnSeekBarChangeListener(this);
        mSeekBarAudioOutputDelaySpeaker.setProgress(delayMs);
        setShow(R.id.id_seek_bar_audio_delay_speaker, delayMs);
        mSeekBarAudioOutputDelaySpeaker.requestFocus();

        mSeekBarAudioOutputDelaySpdif = (SeekBar) view.findViewById(R.id.id_seek_bar_audio_delay_spdif);
        mTextAudioOutputDelaySpdif = (TextView) view.findViewById(R.id.id_text_view_audio_delay_spdif);
        delayMs = mAudioConfigManager.getAudioOutputSpdifDelay(mCurrentSettingSourceId);
        mSeekBarAudioOutputDelaySpdif.setOnSeekBarChangeListener(this);
        mSeekBarAudioOutputDelaySpdif.setProgress(delayMs);
        setShow(R.id.id_seek_bar_audio_delay_spdif, delayMs);

        mSeekBarAudioOutputDelayHeadphone = (SeekBar) view.findViewById(R.id.id_seek_bar_audio_delay_headphone);
        mTextAudioOutputDelayHeadphone = (TextView) view.findViewById(R.id.id_text_view_audio_delay_headphone);
        delayMs = mAudioConfigManager.getAudioOutputHeadphoneDelay(mCurrentSettingSourceId);
        mSeekBarAudioOutputDelayHeadphone.setOnSeekBarChangeListener(this);
        mSeekBarAudioOutputDelayHeadphone.setProgress(delayMs);
        setShow(R.id.id_seek_bar_audio_delay_headphone, delayMs);

        mSeekBarAudioPrescale = (SeekBar) view.findViewById(R.id.id_seek_bar_audio_prescale);
        mTextAudioPrescale = (TextView) view.findViewById(R.id.id_text_view_audio_prescale);
        int value = mAudioConfigManager.getAudioPrescale(mCurrentSettingSourceId);
        mSeekBarAudioPrescale.setOnSeekBarChangeListener(this);
        mSeekBarAudioPrescale.setProgress(value+150);
        setShow(R.id.id_seek_bar_audio_prescale, value);

        mIsDelayAndPrescaleSeekBarInited = true;
    }

    @Override
    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
        if (!mIsAudioEqSeekBarInited && !mIsDelayAndPrescaleSeekBarInited) {
            return;
        }
        ((TvSettingsActivity)getActivity()).startShowActivityTimer();
        switch (seekBar.getId()) {
            case R.id.seekbar_tv_audio_effect_band1:{
                setShow(AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND1, progress);
                mAudioEffectManager.setUserSoundModeParam(AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND1, progress);
                break;
            }
            case R.id.seekbar_tv_audio_effect_band2:{
                setShow(AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND2, progress);
                mAudioEffectManager.setUserSoundModeParam(AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND2, progress);
                break;
            }
            case R.id.seekbar_tv_audio_effect_band3:{
                setShow(AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND3, progress);
                mAudioEffectManager.setUserSoundModeParam(AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND3, progress);
                break;
            }
            case R.id.seekbar_tv_audio_effect_band4:{
                setShow(AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND4, progress);
                mAudioEffectManager.setUserSoundModeParam(AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND4, progress);
                break;
            }
            case R.id.seekbar_tv_audio_effect_band5:{
                setShow(AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND5, progress);
                mAudioEffectManager.setUserSoundModeParam(AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND5, progress);
                break;
            }
            case R.id.id_seek_bar_audio_delay_speaker:{
                setShow(R.id.id_seek_bar_audio_delay_speaker, progress);
                mAudioConfigManager.setAudioOutputSpeakerDelay(mCurrentSettingSourceId, progress);
                setDelayEnabled();
                break;
            }
            case R.id.id_seek_bar_audio_delay_spdif:{
                setShow(R.id.id_seek_bar_audio_delay_spdif, progress);
                mAudioConfigManager.setAudioOutputSpdifDelay(mCurrentSettingSourceId, progress);
                setDelayEnabled();
                break;
            }
            case R.id.id_seek_bar_audio_delay_headphone:{
                setShow(R.id.id_seek_bar_audio_delay_headphone, progress);
                mAudioConfigManager.setAudioOutputHeadphoneDelay(mCurrentSettingSourceId, progress);
                setDelayEnabled();
                break;
            }
            case R.id.id_seek_bar_audio_prescale:{
                // UI display -150 - 150
                setShow(R.id.id_seek_bar_audio_prescale, progress-150);
                mAudioConfigManager.setAudioPrescale(mCurrentSettingSourceId, progress-150);
                setDelayEnabled();
                break;
            }
            default:
                Log.w(TAG, "onProgressChanged unsupported seekbar id:" + seekBar.getId());
                break;
        }
    }

    @Override
    public void onStopTrackingTouch(SeekBar seekBar) {

    }

    @Override
    public void onStartTrackingTouch(SeekBar seekBar) {

    }

    private void setShow(int id, int value) {
        switch (id) {
            case AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND1:{
                mBand1Text.setText(getShowString(R.string.tv_audio_effect_band1, value));
                break;
            }
            case AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND2:{
                mBand2Text.setText(getShowString(R.string.tv_audio_effect_band2, value));
                break;
            }
            case AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND3:{
                mBand3Text.setText(getShowString(R.string.tv_audio_effect_band3, value));
                break;
            }
            case AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND4:{
                mBand4Text.setText(getShowString(R.string.tv_audio_effect_band4, value));
                break;
            }
            case AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND5:{
                mBand5Text.setText(getShowString(R.string.tv_audio_effect_band5, value));
                break;
            }
            case R.id.id_seek_bar_audio_delay_speaker:{
                mTextAudioOutputDelaySpeaker.setText(getAudioDelayShowString(R.string.title_tv_audio_delay_speaker, value));
                break;
            }
            case R.id.id_seek_bar_audio_delay_spdif:{
                mTextAudioOutputDelaySpdif.setText(getAudioDelayShowString(R.string.title_tv_audio_delay_spdif, value));
                break;
            }
            case R.id.id_seek_bar_audio_delay_headphone:{
                mTextAudioOutputDelayHeadphone.setText(getAudioDelayShowString(R.string.title_tv_audio_delay_headphone, value));
                break;
            }
            case R.id.id_seek_bar_audio_prescale:{
                mTextAudioPrescale.setText(getActivity().getResources().getString(R.string.title_tv_audio_prescale) + ": " + value/10.0 + " dB");
                break;
            }
            default:
                break;
        }
    }

    private String getAudioDelayShowString(int resid, int value) {
        return getActivity().getResources().getString(resid) + ": " + value + " ms";
    }

    private void setDelayEnabled () {
        SystemControlManager.getInstance().setProperty(AudioConfigManager.PROP_AUDIO_DELAY_ENABLED, "true");
    }

    private String getShowString(int resid, int value) {
        return getActivity().getResources().getString(resid) + " " + value + "%";
    }

    private String[] getArrayString(int resid) {
        return getActivity().getResources().getStringArray(resid);
    }
}
