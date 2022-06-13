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

package com.android.tv.settings.display.outputmode;

import android.app.Activity;
import android.app.AlarmManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import androidx.annotation.Keep;
import com.android.tv.settings.SettingsPreferenceFragment;
import androidx.preference.CheckBoxPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceScreen;
import android.text.TextUtils;
import com.android.tv.settings.dialog.old.Action;
import com.android.tv.settings.RadioPreference;
import com.android.tv.settings.R;

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

@Keep
public class ColorAttributeFragment extends SettingsPreferenceFragment {
    private static final String LOG_TAG = "ColorAttributeFragment";
    private OutputUiManager mOutputUiManager;
    private static String saveValue = null;
    private static String curValue = null;
    private static String curMode = null;
    private static final int MSG_FRESH_UI = 0;
    private IntentFilter mIntentFilter;
    public boolean hpdFlag = false;
    private static final String DEFAULT_VALUE = "444,8bit";
    private static final String DEFAULT_TITLE = "YCbCr444 8bit";

    private ArrayList<String> colorTitleList = new ArrayList();
    private BroadcastReceiver mIntentReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            hpdFlag = intent.getBooleanExtra ("state", false);
            mHandler.sendEmptyMessageDelayed(MSG_FRESH_UI, hpdFlag ^ isHdmiMode() ? 2000 : 1000);
        }
    };
    public static ColorAttributeFragment newInstance() {
        return new ColorAttributeFragment();
    }
    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        mOutputUiManager = new OutputUiManager(getActivity());
        mIntentFilter = new IntentFilter("android.intent.action.HDMI_PLUGGED");
        mIntentFilter.addAction(Intent.ACTION_TIME_TICK);
        updatePreferenceFragment();
    }
    private boolean needfresh() {
        ArrayList<String> list = mOutputUiManager.getColorTitleList();
        Log.d(LOG_TAG, "colorTitleList: " + colorTitleList.toString() + "\n list: " + list.toString());
        if (colorTitleList.size() > 0 && colorTitleList.size() == list.size()) {
            for (String title : colorTitleList) {
                if (!list.contains(title))
                    return true;
            }
        }else {
            return true;
        }
        return false;
    }
    private void updatePreferenceFragment() {
        mOutputUiManager.updateUiMode();
        if (!needfresh()) return;
        final Context themedContext = getPreferenceManager().getContext();
        final PreferenceScreen screen = getPreferenceManager().createPreferenceScreen(
                themedContext);
        screen.setTitle(R.string.device_outputmode_color_space);
        setPreferenceScreen(screen);
        if (!isHdmiMode()) {
            colorTitleList.clear();
            return;
        }
        final List<Action> InfoList = getMainActions();
        for (final Action Info : InfoList) {
            final String InfoTag = Info.getKey();
            final RadioPreference radioPreference = new RadioPreference(themedContext);
            radioPreference.setKey(InfoTag);
            radioPreference.setPersistent(false);
            radioPreference.setTitle(Info.getTitle());
            radioPreference.setLayoutResource(R.layout.preference_reversed_widget);
            if (Info.isChecked()) {
                radioPreference.setChecked(true);
            }
            screen.addPreference(radioPreference);
        }
    }
    private boolean isModeSupportColor(final String curMode, final String curValue){
        return mOutputUiManager.isModeSupportColor(curMode, curValue);
    }

    private ArrayList<Action> getMainActions() {
        ArrayList<Action> actions = new ArrayList<Action>();
        colorTitleList.clear();
        ArrayList<String> mList = mOutputUiManager.getColorTitleList();
        for (String color:mList) {
            colorTitleList.add(color);
        }
        ArrayList<String> colorValueList = mOutputUiManager.getColorValueList();
        String value = null;
        String filterValue = null;
        String  curColorSpaceValue = mOutputUiManager.getCurrentColorAttribute();
        Log.i(LOG_TAG,"curColorSpaceValue: "+curColorSpaceValue);
        if (curColorSpaceValue.equals("default"))
            curColorSpaceValue = DEFAULT_VALUE;
        for (int i = 0; i < colorTitleList.size(); i++) {
            value = colorValueList.get(i).trim();
            curMode = mOutputUiManager.getCurrentMode().trim();
            if (!isModeSupportColor(curMode, value)) {
                continue;
            }
            filterValue += value;
        }

        for (int i = 0; i < OutputUiManager.HDMI_COLOR_LIST.length; i++) {
            if (filterValue.contains(OutputUiManager.HDMI_COLOR_LIST[i])) {
                if (curColorSpaceValue.contains(OutputUiManager.HDMI_COLOR_LIST[i])) {
                    actions.add(new Action.Builder().key(OutputUiManager.HDMI_COLOR_LIST[i])
                        .title("        " + OutputUiManager.HDMI_COLOR_TITLE[i])
                        .checked(true).build());
                } else {
                    actions.add(new Action.Builder().key(OutputUiManager.HDMI_COLOR_LIST[i])
                        .title("        " + OutputUiManager.HDMI_COLOR_TITLE[i])
                        .description("").build());
                }
            }
        }
        if (actions.size() == 0) {
            actions.add(new Action.Builder().key(DEFAULT_VALUE)
                .title("        " + DEFAULT_TITLE)
                .checked(true).build());
        }
        return actions;
    }
    @Override
    public void onResume() {
        super.onResume();
        getActivity().registerReceiver(mIntentReceiver, mIntentFilter);
        mHandler.sendEmptyMessage(MSG_FRESH_UI);
    }

    @Override
    public void onPause() {
        super.onPause();
        getActivity().unregisterReceiver(mIntentReceiver);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        if (preference instanceof RadioPreference) {
            final RadioPreference radioPreference = (RadioPreference) preference;
            radioPreference.clearOtherRadioPreferences(getPreferenceScreen());
            if (radioPreference.isChecked()) {
                if (onClickHandle(radioPreference.getKey()) == true) {
                    radioPreference.setChecked(true);
                }
            } else {
                radioPreference.setChecked(true);
                Log.i(LOG_TAG,"not checked");
            }
        }
      return super.onPreferenceTreeClick(preference);
    }

    @Override
    public int getMetricsCategory() {
        return 0;
    }

    public boolean onClickHandle(String key) {
        curValue = key;
        saveValue= mOutputUiManager.getCurrentColorAttribute().trim();
        if (saveValue.equals("default"))
            saveValue = DEFAULT_VALUE;
        curMode = mOutputUiManager.getCurrentMode().trim();
        Log.i(LOG_TAG,"Set Color Space Value: "+curValue + "CurValue: "+saveValue);
        if (!curValue.equals(saveValue)) {
            if (isModeSupportColor(curMode,curValue)) {
               mOutputUiManager.changeColorAttribte(curValue);
            } else {
                curValue = DEFAULT_VALUE;
                mOutputUiManager.changeColorAttribte(curValue);
            }
            return true;
        }
        return false;
    }

    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_FRESH_UI:
                    updatePreferenceFragment();
                    break;
            }
        }
    };
    private boolean isHdmiMode() {
        return mOutputUiManager.isHdmiMode();
    }
}
