/*
* opyright (C) 2015 The Android Open Source Project
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

import android.app.Activity;
import android.app.AlarmManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.media.tv.TvInputInfo;
import android.media.tv.TvInputManager;
import android.os.Bundle;
import androidx.annotation.Keep;
import com.android.tv.settings.SettingsPreferenceFragment;
import androidx.preference.CheckBoxPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceScreen;
import android.text.TextUtils;

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.provider.Settings;
import android.util.Log;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;
import android.content.BroadcastReceiver;
import android.content.IntentFilter;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import android.hardware.hdmi.HdmiControlManager;
import android.hardware.hdmi.HdmiDeviceInfo;
import android.hardware.hdmi.HdmiTvClient;
import android.hardware.hdmi.HdmiTvClient.SelectCallback;
import android.os.SystemProperties;
import com.android.tv.settings.SettingsConstant;
import androidx.preference.Preference;
import com.android.tv.settings.R;


@Keep
public class HdmiCecDeviceSelectFragment extends SettingsPreferenceFragment implements Preference.OnPreferenceClickListener {
    private static final String TAG = "HdmiCecDeviceSelectFragment";

    private final String ACTION_START_LIVE_TV = "action.startlivetv.settingui";
    private final String FROM_TV_SOURCE = "from_tv_source";

    private static final String[] DEFAULT_NAMES = {
        "TV",
        "Recorder_1",
        "Recorder_2",
        "Tuner_1",
        "Playback_1",
        "AudioSystem",
        "Tuner_2",
        "Tuner_3",
        "Playback_2",
        "Recorder_3",
        "Tuner_4",
        "Playback_3",
        "Reserved_1",
        "Reserved_2",
        "Secondary_TV",
    };

    HdmiControlManager mHdmiControlManager;
    TvInputManager mTvInputManager;
    HdmiTvClient mTvClient;

    private ArrayList<HdmiDeviceInfo> mHdmiDeviceInfoList = new ArrayList();
    public static HdmiCecDeviceSelectFragment newInstance() {
        return new HdmiCecDeviceSelectFragment();
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        boolean tvFlag = SettingsConstant.needDroidlogicTvFeature(getContext())
                    && (SystemProperties.getBoolean("vendor.tv.soc.as.mbox", false) == false);
        if (tvFlag) {
            mTvInputManager = (TvInputManager) getActivity()
                                            .getSystemService(Context.TV_INPUT_SERVICE);
            mHdmiControlManager = (HdmiControlManager) getActivity()
                                        .getSystemService(Context.HDMI_CONTROL_SERVICE);
            mTvClient = mHdmiControlManager.getTvClient();
            updatePreferenceFragment();
        }
    }


    @Override
    public int getMetricsCategory() {
        return 0;
    }

    private void updatePreferenceFragment() {
        Log.d(TAG, "updatePreferenceFragment");
        final Context themedContext = getPreferenceManager().getContext();
        final PreferenceScreen screen = getPreferenceManager().createPreferenceScreen(
                themedContext);
        screen.setTitle(R.string.title_cec_device_list);
        setPreferenceScreen(screen);
        int logicalAddress;
        String deviceName = "";
        mHdmiDeviceInfoList.clear();
        if (mTvClient == null) {
            Log.e(TAG, "tv null!");
            return;
        }
        for (HdmiDeviceInfo info : mTvClient.getDeviceList()) {
            if (info != null && info.isSourceType()) {
                mHdmiDeviceInfoList.add(info);
                logicalAddress = info.getLogicalAddress();
                deviceName = (info.getDisplayName() != null ? info.getDisplayName() : DEFAULT_NAMES[logicalAddress]);
                Log.d(TAG, "logicalAddress = " + logicalAddress + ", deviceName = " + deviceName);
                final Preference preference = new Preference(themedContext);
                preference.setTitle(deviceName);
                preference.setKey(String.valueOf(logicalAddress));
                preference.setOnPreferenceClickListener(this);
                screen.addPreference(preference);
            }
        }
    }

    @Override
    public boolean onPreferenceClick(Preference preference) {
        Log.d(TAG, "onPreferenceClick " + preference.getKey());
        final int logicalAddress = Integer.parseInt(preference.getKey());
        if (mTvClient == null) {
            Log.e(TAG, "onPreferenceClick tv null!");
            return false;
        }
        mTvClient.deviceSelect(logicalAddress, result -> {
            Log.d(TAG, "device select result=" + result);
        });
        return true;
    }

    private void tuneToTvInput(int logicalAddress) {
        if (mTvInputManager == null) {
            Log.e(TAG, "tuneToTvInput tv null!");
            return;
        }

        List<TvInputInfo> inputList = mTvInputManager.getTvInputList();
        for (TvInputInfo input : inputList) {
            if (input != null &&
                input.getHdmiDeviceInfo() != null &&
                input.getHdmiDeviceInfo().getLogicalAddress() == logicalAddress) {
                Log.d(TAG, "select tv input " + input);
                startLiveTv(input);
                return;
            }
        }
    }

    private void startLiveTv(TvInputInfo input) {
        Intent intent = new Intent();
        intent.setAction(ACTION_START_LIVE_TV);
        intent.putExtra(FROM_TV_SOURCE, true);
        intent.putExtra(TvInputInfo.EXTRA_INPUT_ID, input.getId());
        getActivity().sendBroadcast(intent);
    }
}
