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

import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.os.SystemProperties;
import android.util.Log;

import androidx.preference.Preference;
import androidx.preference.PreferenceScreen;

import com.android.tv.settings.SettingsPreferenceFragment;
import com.android.tv.settings.dialog.old.Action;
import com.droidlogic.app.DataProviderManager;

import java.util.ArrayList;
import java.util.List;

public class PowerKeyActionDefinitionFragment extends SettingsPreferenceFragment {
    private static final String TAG = PowerKeyActionDefinitionFragment.class.getSimpleName();
    private static final String POWER_KEY_ACTION_RADIO_GROUP = "PowerKeyAction";
    private static final String POWER_KEY_DEFINITION = "power_key_definition";
    private static final String POWER_KEY_SUSPEND = "power_key_suspend";
    private static final String POWER_KEY_SHUTDOWN = "power_key_shutdown";
    private static final String POWER_KEY_RESTART = "power_key_restart";
    private static final String POWER_KEY_FORCE_SUSPEND = "power_key_force_suspend";
    private static final int SUSPEND = 0;
    private static final int SHUTDOWN = 1;
    private static final int RESTART = 2;
    private static final int FORCE_SUSPEND = 3;

    private Context mContext;
    private static final int POWER_KEY_SET_DELAY_MS = 500;
    private final Handler mDelayHandler = new Handler();
    private String mNewKeyDefinition;
    private boolean mIsTv;
    private final Runnable mSetPowerKeyActionRunnable = new Runnable() {
        @Override
        public void run() {
            if (POWER_KEY_SUSPEND.equals(mNewKeyDefinition)) {
                setPowerKeyActionDefinition(SUSPEND);
            } else if (POWER_KEY_SHUTDOWN.equals(mNewKeyDefinition)) {
                setPowerKeyActionDefinition(SHUTDOWN);
            } else if (POWER_KEY_RESTART.equals(mNewKeyDefinition)) {
                setPowerKeyActionDefinition(RESTART);
            } else if (POWER_KEY_FORCE_SUSPEND.equals(mNewKeyDefinition)) {
                setPowerKeyActionDefinition(FORCE_SUSPEND);
            }
        }
    };

    public static PowerKeyActionDefinitionFragment newInstance() {
        return new PowerKeyActionDefinitionFragment();
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        mContext = getPreferenceManager().getContext();
        mIsTv = SettingsConstant.needDroidlogicTvFeature(mContext);
        final PreferenceScreen screen = getPreferenceManager().createPreferenceScreen(mContext);
        screen.setTitle(R.string.system_powerkeyaction);
        Preference activePref = null;

        final List<Action> mPowerKeyInfoList = getActions();
        for (final Action powerKeyInfo : mPowerKeyInfoList) {
            final String powerKeyTag = powerKeyInfo.getKey();
            final RadioPreference radioPreference = new RadioPreference(mContext);
            radioPreference.setKey(powerKeyTag);
            radioPreference.setPersistent(false);
            radioPreference.setTitle(powerKeyInfo.getTitle());
            radioPreference.setRadioGroup(POWER_KEY_ACTION_RADIO_GROUP);
            radioPreference.setLayoutResource(R.layout.preference_reversed_widget);
            if (powerKeyInfo.isChecked()) {
                radioPreference.setChecked(true);
                activePref = radioPreference;
            }
            screen.addPreference(radioPreference);
        }
        if (activePref != null && savedInstanceState == null) {
            scrollToPreference(activePref);
        }
        setPreferenceScreen(screen);
    }

    private ArrayList<Action> getActions() {
        ArrayList<Action> actions = new ArrayList<>();
        int checkedKey = whichPowerKeyDefinition();
        actions.add(new Action.Builder().key(POWER_KEY_SUSPEND).title(getString(R.string.power_action_suspend))
                .checked(checkedKey == SUSPEND).build());
        actions.add(new Action.Builder().key(POWER_KEY_FORCE_SUSPEND).title(getString(R.string.power_action_force_suspend))
                .checked(checkedKey == FORCE_SUSPEND).build());
        actions.add(new Action.Builder().key(POWER_KEY_SHUTDOWN).title(getString(R.string.power_action_shutdown))
                .checked(checkedKey == SHUTDOWN).build());
        if (!mIsTv) {
            actions.add(new Action.Builder().key(POWER_KEY_RESTART).title(getString(R.string.power_action_restart))
                    .checked(checkedKey == RESTART).build());
        }
        return actions;
    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        if (preference instanceof RadioPreference) {
            final RadioPreference radioPreference = (RadioPreference) preference;
            radioPreference.clearOtherRadioPreferences(getPreferenceScreen());
            mNewKeyDefinition = radioPreference.getKey();
            mDelayHandler.removeCallbacks(mSetPowerKeyActionRunnable);
            mDelayHandler.postDelayed(mSetPowerKeyActionRunnable, POWER_KEY_SET_DELAY_MS);
            radioPreference.setChecked(true);
        }
        return super.onPreferenceTreeClick(preference);
    }

    @Override
    public int getMetricsCategory() {
        return 0;
    }

    private int whichPowerKeyDefinition() {
        return DataProviderManager.getIntValue(mContext, POWER_KEY_DEFINITION, SUSPEND);
    }

    private void setPowerKeyActionDefinition(int keyValue) {
        DataProviderManager.putIntValue(mContext, POWER_KEY_DEFINITION, keyValue);
        SystemProperties.set("persist.sys.power.key.action", String.valueOf(keyValue));
        Log.d(TAG, "setPowerKeyActionDefinition keyValue=" + keyValue);
    }
}
