/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC HdmiCecFragment
 */

package com.android.tv.settings.tvoption;

import android.app.Activity;
import android.content.Context;
import android.content.ContentResolver;
import android.content.ComponentName;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.UserHandle;
import android.provider.Settings;
import com.android.tv.settings.SettingsPreferenceFragment;
import androidx.preference.ListPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceGroup;
import androidx.preference.SeekBarPreference;
import androidx.preference.TwoStatePreference;
import android.text.TextUtils;
import android.util.Log;

import com.droidlogic.app.HdmiCecManager;
import com.droidlogic.app.OutputModeManager;
import com.droidlogic.app.SystemControlManager;
import com.droidlogic.app.AudioConfigManager;
import com.android.tv.settings.R;
import com.android.tv.settings.RadioPreference;
import com.android.tv.settings.SettingsConstant;
import com.android.tv.settings.SoundFragment;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;

import java.util.*;

import android.os.SystemProperties;
/**
 * Fragment to control HDMI Cec settings.
 */
public class HdmiCecFragment extends SettingsPreferenceFragment implements Preference.OnPreferenceChangeListener{

    private static final String TAG = "HdmiCecFragment";
    private static HdmiCecFragment mHdmiCecFragment = null;

    private static final String KEY_CEC_SWITCH                  = "key_cec_switch";
    private static final String KEY_CEC_ONE_KEY_PLAY            = "key_cec_one_key_play";
    private static final String KEY_CEC_AUTO_POWER_OFF          = "key_cec_auto_power_off";
    private static final String KEY_CEC_AUTO_WAKE_UP            = "key_cec_auto_wake_up";
    private static final String KEY_CEC_AUTO_CHANGE_LANGUAGE    = "key_cec_auto_change_language";
    private static final String KEY_CEC_ARC_SWITCH              = "key_cec_arc_switch";
    private static final String KEY_CEC_DEVICE_LIST             = "key_cec_device_list";
    private static final String KEY_EARC_SWITCH                 = "key_earc_switch";
    private static final String KEY_ARC_AND_EARC_SWITCH         = "key_arc_and_earc_switch";
    private static final String KEY_ARC_EARC_MODE_AUTO          = "arc_earc_mode_auto";
    private static final String KEY_ARC_EARC_MODE_ARC           = "arc_earc_mode_arc";
    //private static final String KEY_ARC_EARC_MODE_GROUP         = "arc_earc_mode";

    private static final int TIPS_TYPE_CEC = 0;
    private static final int TIPS_TYPE_ARC = 1;

    private TwoStatePreference mCecSwitchPref;
    private TwoStatePreference mCecOnekeyPlayPref;
    private TwoStatePreference mCecDeviceAutoPoweroffPref;
    private TwoStatePreference mCecAutoWakeupPref;
    private TwoStatePreference mCecAutoChangeLanguagePref;
    private TwoStatePreference mArcSwitchPref;
    private TwoStatePreference mEarcSwitchPref;
    private TwoStatePreference mArcNEarcSwitchPref;
    private RadioPreference mArcEarcModeAutoPref;
    private RadioPreference mArcEarcModeARCPref;
    private PreferenceGroup mArcEarcModeGroup;

    private SystemControlManager mSystemControlManager = SystemControlManager.getInstance();
    private SoundParameterSettingManager mSoundParameterSettingManager;
    private AudioConfigManager mAudioConfigManager = null;
    private HdmiCecManager mHdmiCecManager;
    private OutputModeManager mOutputModeManager;
    private static long lastObserveredCECTime = 0;
    private static long lastObserveredArcEarcTime = 0;

    public static HdmiCecFragment newInstance() {
        if (mHdmiCecFragment == null) {
            mHdmiCecFragment = new HdmiCecFragment();
        }
        return mHdmiCecFragment;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        Log.i(TAG, "---onCreate");
        mHdmiCecManager = new HdmiCecManager(getContext());
        mOutputModeManager = new OutputModeManager(getContext());
        if (mAudioConfigManager == null) {
            mAudioConfigManager = AudioConfigManager.getInstance(getActivity());
        }
        if (mSoundParameterSettingManager == null) {
            mSoundParameterSettingManager = new SoundParameterSettingManager(getActivity());
        }
        super.onCreate(savedInstanceState);
    }

    private String[] getArrayString(int resid) {
        return getActivity().getResources().getStringArray(resid);
    }

    @Override
    public void onResume() {
        super.onResume();
        refresh();
    }

    @Override
    public void onPause() {
        super.onPause();
        mHandler.removeCallbacksAndMessages(null);
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        Log.i(TAG, "---onCreatePreferences");
        setPreferencesFromResource(R.xml.hdmicec, null);
        boolean tvFlag = SettingsConstant.needDroidlogicTvFeature(getContext())
                    && (SystemProperties.getBoolean("vendor.tv.soc.as.mbox", false) == false);
        mCecSwitchPref = (TwoStatePreference) findPreference(KEY_CEC_SWITCH);
        mCecOnekeyPlayPref = (TwoStatePreference) findPreference(KEY_CEC_ONE_KEY_PLAY);
        mCecDeviceAutoPoweroffPref = (TwoStatePreference) findPreference(KEY_CEC_AUTO_POWER_OFF);
        mCecAutoWakeupPref = (TwoStatePreference) findPreference(KEY_CEC_AUTO_WAKE_UP);
        mCecAutoChangeLanguagePref = (TwoStatePreference) findPreference(KEY_CEC_AUTO_CHANGE_LANGUAGE);
        mArcSwitchPref = (TwoStatePreference) findPreference(KEY_CEC_ARC_SWITCH);
        mEarcSwitchPref = (TwoStatePreference) findPreference(KEY_EARC_SWITCH);
        mArcNEarcSwitchPref = (TwoStatePreference) findPreference(KEY_ARC_AND_EARC_SWITCH);
        mArcEarcModeAutoPref= (RadioPreference) findPreference(KEY_ARC_EARC_MODE_AUTO);
        mArcEarcModeARCPref= (RadioPreference) findPreference(KEY_ARC_EARC_MODE_ARC);
        //mArcEarcModeGroup = (PreferenceGroup) findPreference(KEY_ARC_EARC_MODE_GROUP);

        final Preference hdmiDeviceSelectPref = findPreference(KEY_CEC_DEVICE_LIST);
        if (mHdmiCecFragment == null) {
            mHdmiCecFragment = newInstance();
        }
        hdmiDeviceSelectPref.setOnPreferenceChangeListener(mHdmiCecFragment);

        final ListPreference digitalSoundPref = (ListPreference) findPreference(SoundFragment.KEY_DIGITALSOUND_FORMAT);
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

        final SeekBarPreference audioOutputLatencyPref = (SeekBarPreference) findPreference(SoundFragment.KEY_AUDIO_OUTPUT_LATENCY);
        audioOutputLatencyPref.setOnPreferenceChangeListener(this);
        audioOutputLatencyPref.setMax(AudioConfigManager.HAL_AUDIO_OUT_DEV_DELAY_MAX);
        audioOutputLatencyPref.setMin(AudioConfigManager.HAL_AUDIO_OUT_DEV_DELAY_MIN);
        audioOutputLatencyPref.setSeekBarIncrement(SoundFragment.KEY_AUDIO_OUTPUT_LATENCY_STEP);
        audioOutputLatencyPref.setValue(mAudioConfigManager.getAudioOutputAllDelay());

        mCecOnekeyPlayPref.setVisible(!tvFlag);
        mCecAutoWakeupPref.setVisible(tvFlag);
        mCecSwitchPref.setVisible(true);
        mArcSwitchPref.setVisible(false);
        mEarcSwitchPref.setVisible(false);
        hdmiDeviceSelectPref.setVisible(tvFlag);
        audioOutputLatencyPref.setVisible(tvFlag);
        digitalSoundPref.setVisible(false);
        // The project should use ro.hdmi.set_menu_language to device whether open this function.
        mCecAutoChangeLanguagePref.setVisible(!tvFlag);
        boolean isChecked = mHdmiCecManager.isArcEnabled();
        mArcNEarcSwitchPref.setChecked(isChecked);
        //mHdmiCecManager.enableArc(mArcNEarcSwitchPref.isChecked());
        mArcNEarcSwitchPref.setVisible(tvFlag);
        mArcEarcModeAutoPref.setVisible(tvFlag && mArcNEarcSwitchPref.isChecked());
        mArcEarcModeARCPref.setVisible(tvFlag && mArcNEarcSwitchPref.isChecked());
    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        final String key = preference.getKey();
        if (key == null) {
            return super.onPreferenceTreeClick(preference);
        }
        switch (key) {
        case KEY_CEC_SWITCH:
            mCecSwitchPref.setEnabled(false);
            if (mArcNEarcSwitchPref.isChecked() && !mCecSwitchPref.isChecked()) {
                showTipsDialog(TIPS_TYPE_CEC, getContext().getString(R.string.tips_turn_off_cec));
            } else {
                sendMsgEnableCECSwitch();
                setCECRefPrefEnable(false);
            }
            return true;
        case KEY_CEC_ONE_KEY_PLAY:
            mHdmiCecManager.enableOneTouchPlay(mCecOnekeyPlayPref.isChecked());
            return true;
        case KEY_CEC_AUTO_POWER_OFF:
            mHdmiCecManager.enableAutoPowerOff(mCecDeviceAutoPoweroffPref.isChecked());
            return true;
        case KEY_CEC_AUTO_WAKE_UP:
            mHdmiCecManager.enableAutoWakeUp(mCecAutoWakeupPref.isChecked());
            return true;
        case KEY_CEC_AUTO_CHANGE_LANGUAGE:
            mHdmiCecManager.enableAutoChangeLanguage(mCecAutoChangeLanguagePref.isChecked());
            return true;
        case KEY_CEC_ARC_SWITCH:
            mHdmiCecManager.enableArc(mArcSwitchPref.isChecked());
            mHandler.sendEmptyMessageDelayed(MSG_ENABLE_ARC_SWITCH, TIME_DELAYED);
            mArcSwitchPref.setEnabled(false);
            return true;
        case KEY_EARC_SWITCH:
            mOutputModeManager.enableEarc(mEarcSwitchPref.isChecked());
            mHandler.sendEmptyMessageDelayed(MSG_ENABLE_EARC_SWITCH, TIME_DELAYED);
            mEarcSwitchPref.setEnabled(false);
            return true;
        case KEY_ARC_AND_EARC_SWITCH:
            Log.d(TAG, "arc/earc_switch: " + mArcNEarcSwitchPref.isChecked());
            //mHdmiCecManager.enableArc(mArcNEarcSwitchPref.isChecked());
            if (!mCecSwitchPref.isChecked() && mArcNEarcSwitchPref.isChecked()) {
                showTipsDialog(TIPS_TYPE_ARC, getContext().getString(R.string.tips_turn_on_arc_earc));
            }
            else {
                sendMsgEnableArcEarcSwitch();
                mArcNEarcSwitchPref.setEnabled(false);
                mArcEarcModeAutoPref.setEnabled(false);
                mArcEarcModeARCPref.setEnabled(false);
            }
            return true;
        case KEY_ARC_EARC_MODE_AUTO:
            Log.d(TAG, "arc/earc_switch mode [AUTO]");
            mArcEarcModeAutoPref.setChecked(true);
            mArcEarcModeARCPref.setChecked(false);
            mOutputModeManager.enableEarc(true);
            return true;
        case KEY_ARC_EARC_MODE_ARC:
            Log.d(TAG, "arc/earc_switch mode [ARC]");
            mArcEarcModeARCPref.setChecked(true);
            mArcEarcModeAutoPref.setChecked(false);
            mOutputModeManager.enableEarc(false);
            return true;
        }
        return super.onPreferenceTreeClick(preference);
    }

    private void sendMsgEnableCECSwitch() {
        long curtime = System.currentTimeMillis();
        long timeDiff = curtime - lastObserveredCECTime;
        Message cecEnabled = mHandler.obtainMessage(MSG_ENABLE_CEC_SWITCH, 0, 0);
        mHandler.removeMessages(MSG_ENABLE_CEC_SWITCH);
        mHandler.sendMessageDelayed(cecEnabled, ((timeDiff > TIME_DELAYED) ? 0 : TIME_DELAYED));
    }

    private void sendMsgEnableArcEarcSwitch() {
        long curtime = System.currentTimeMillis();
        long timeDiff = curtime - lastObserveredArcEarcTime;
        Message arcEarcEnabled = mHandler.obtainMessage(MSG_ENABLE_ARC_EARC_SWITCH, 0, 0);
        mHandler.removeMessages(MSG_ENABLE_ARC_EARC_SWITCH);
        mHandler.sendMessageDelayed(arcEarcEnabled, ((timeDiff > TIME_DELAYED) ? 0 : TIME_DELAYED));
    }

    private void setCECRefPrefEnable(boolean isenable) {
        mCecOnekeyPlayPref.setEnabled(isenable);
        mCecDeviceAutoPoweroffPref.setEnabled(isenable);
        mCecAutoWakeupPref.setEnabled(isenable);
        mCecAutoChangeLanguagePref.setEnabled(isenable);
        mArcSwitchPref.setEnabled(isenable);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        Log.d(TAG, "[onPreferenceChange] preference.getKey() = " + preference.getKey() + ", newValue = " + newValue);
        if (TextUtils.equals(preference.getKey(), SoundFragment.KEY_DIGITALSOUND_FORMAT)) {
            mSoundParameterSettingManager.setDigitalAudioFormat((String)newValue);
        } else if (TextUtils.equals(preference.getKey(), SoundFragment.KEY_AUDIO_OUTPUT_LATENCY)) {
            mAudioConfigManager.setAudioOutputAllDelay((int)newValue);
        }
        return true;
    }

    @Override
    public int getMetricsCategory() {
        return 0;
    }

    private void refresh() {
        boolean hdmiControlEnabled = mHdmiCecManager.isHdmiControlEnabled();
        mCecSwitchPref.setChecked(hdmiControlEnabled);
        mCecOnekeyPlayPref.setChecked(mHdmiCecManager.isOneTouchPlayEnabled());
        mCecOnekeyPlayPref.setEnabled(hdmiControlEnabled);
        mCecDeviceAutoPoweroffPref.setChecked(mHdmiCecManager.isAutoPowerOffEnabled());
        mCecDeviceAutoPoweroffPref.setEnabled(hdmiControlEnabled);
        mCecAutoWakeupPref.setChecked(mHdmiCecManager.isAutoWakeUpEnabled());
        mCecAutoWakeupPref.setEnabled(hdmiControlEnabled);
        mCecAutoChangeLanguagePref.setChecked(mHdmiCecManager.isAutoChangeLanguageEnabled());
        mCecAutoChangeLanguagePref.setEnabled(hdmiControlEnabled);

        boolean arcEnabled = mHdmiCecManager.isArcEnabled();
        boolean earcEnabled = mOutputModeManager.isEarcEnabled();

        if (arcEnabled) {
            Log.i(TAG, "arcEnabled:" + arcEnabled + ",earcEnabled:" + earcEnabled);
            if (earcEnabled) {
                mArcEarcModeAutoPref.setChecked(true);
                mArcEarcModeARCPref.setChecked(false);
            } else {
                mArcEarcModeAutoPref.setChecked(false);
                mArcEarcModeARCPref.setChecked(true);
            }
        } else {
            mArcEarcModeAutoPref.setChecked(true);
            mArcEarcModeARCPref.setChecked(false);
        }
    }

    private static final int MSG_ENABLE_CEC_SWITCH = 0;
    private static final int MSG_ENABLE_ARC_SWITCH = 1;
    private static final int MSG_ENABLE_EARC_SWITCH = 2;
    private static final int MSG_ENABLE_ARC_EARC_SWITCH = 3;
    private static final int TIME_DELAYED = 5000;//ms
    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_ENABLE_CEC_SWITCH:
                    mCecSwitchPref.setEnabled(true);
                    mHdmiCecManager.enableHdmiControl(mCecSwitchPref.isChecked());
                    boolean hdmiControlEnabled = mHdmiCecManager.isHdmiControlEnabled();
                    Log.d(TAG,"hdmiControlEnabled :" + hdmiControlEnabled);
                    setCECRefPrefEnable(hdmiControlEnabled);
                    lastObserveredCECTime = System.currentTimeMillis();
                    /*
                    mCecOnekeyPlayPref.setEnabled(hdmiControlEnabled);
                    mCecDeviceAutoPoweroffPref.setEnabled(hdmiControlEnabled);
                    mCecAutoWakeupPref.setEnabled(hdmiControlEnabled);
                    mCecAutoChangeLanguagePref.setEnabled(hdmiControlEnabled);
                    mArcSwitchPref.setEnabled(hdmiControlEnabled);
                    */
                    break;
                case MSG_ENABLE_ARC_EARC_SWITCH:
                    Log.d(TAG, "receive msg-arc_earc");
                    mArcNEarcSwitchPref.setEnabled(true);
                    mArcEarcModeAutoPref.setEnabled(true);
                    mArcEarcModeARCPref.setEnabled(true);
                    mArcEarcModeAutoPref.setVisible(mArcNEarcSwitchPref.isChecked());
                    mArcEarcModeARCPref.setVisible(mArcNEarcSwitchPref.isChecked());
                    mHdmiCecManager.enableArc(mArcNEarcSwitchPref.isChecked());
                    if (mArcNEarcSwitchPref.isChecked())
                        mOutputModeManager.enableEarc(mArcEarcModeAutoPref.isChecked());
                    lastObserveredArcEarcTime = System.currentTimeMillis();
                    break;
                default:
                    break;
            }
        }
    };

    private void showTipsDialog(int tipsType, String tips){
        final AlertDialog.Builder tipsDialog = new AlertDialog.Builder(getActivity());
        tipsDialog.setTitle("TIPS");
        tipsDialog.setMessage(tips);
        tipsDialog.setPositiveButton("YES",
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        if (tipsType == TIPS_TYPE_CEC) {
                            Log.d(TAG, "onClick yes, Type CEC, sure to turn off CEC，also turn off arc/eARC");
                            //TODO:turn off CEC
                            mCecSwitchPref.setChecked(false);
                            sendMsgEnableCECSwitch();
                            setCECRefPrefEnable(false);
                            //TODO:turn off ARC\eARC
                            mArcNEarcSwitchPref.setChecked(false);
                            mArcEarcModeAutoPref.setVisible(mArcNEarcSwitchPref.isChecked());
                            mArcEarcModeARCPref.setVisible(mArcNEarcSwitchPref.isChecked());
                            mHdmiCecManager.enableArc(mArcNEarcSwitchPref.isChecked());

                        } else if (tipsType == TIPS_TYPE_ARC) {
                            //TODO:turn on cec ui ref
                            Log.d(TAG, "onClick yes, Type ARC/eARC,sure to turn on arc, also turn on cec");
                            mCecSwitchPref.setChecked(true);
                            sendMsgEnableCECSwitch();
                            setCECRefPrefEnable(false);
                            //turn on ARC
                            mArcEarcModeAutoPref.setVisible(mArcNEarcSwitchPref.isChecked());
                            mArcEarcModeARCPref.setVisible(mArcNEarcSwitchPref.isChecked());
                            mHdmiCecManager.enableArc(mArcNEarcSwitchPref.isChecked());
                            if (mArcEarcModeAutoPref.isChecked()) {
                                mOutputModeManager.enableEarc(true);
                            } else {
                                mOutputModeManager.enableEarc(false);
                            }
                        }
                    }
                });
        tipsDialog.setNegativeButton("CANCEL",
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        //problem
                        if (tipsType == TIPS_TYPE_CEC) {
                            Log.d(TAG, "onClick cancel, type CEC");
                            mCecSwitchPref.setChecked(true);
                            mCecSwitchPref.setEnabled(true);
                        } else if (tipsType == TIPS_TYPE_ARC) {
                            Log.d(TAG, "onClick cancel, type ARC");
                            mArcNEarcSwitchPref.setChecked(false);
                            mHdmiCecManager.enableArc(mArcNEarcSwitchPref.isChecked());
                            mArcEarcModeAutoPref.setVisible(mArcNEarcSwitchPref.isChecked());
                            mArcEarcModeARCPref.setVisible(mArcNEarcSwitchPref.isChecked());
                            Log.d(TAG, "[current value] enableARC: " + mHdmiCecManager.isArcEnabled() + ", enableEARC: " + mOutputModeManager.isEarcEnabled ());
                        }
                    }
                });
        tipsDialog.setCancelable(false);
        tipsDialog.show();
    }
}
