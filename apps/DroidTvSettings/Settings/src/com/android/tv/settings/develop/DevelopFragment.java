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

package com.android.tv.settings.develop;

import android.os.Bundle;
import android.os.Handler;
import androidx.preference.SwitchPreference;
import com.android.tv.settings.SettingsPreferenceFragment;
import androidx.preference.Preference;
import androidx.preference.PreferenceCategory;
import androidx.preference.ListPreference;
import android.os.SystemProperties;
import android.text.TextUtils;
import com.android.tv.settings.util.DroidUtils;
import com.android.tv.settings.SettingsConstant;
import com.android.tv.settings.R;
import com.droidlogic.app.SystemControlManager;
import android.util.Log;

public class DevelopFragment extends SettingsPreferenceFragment implements Preference.OnPreferenceChangeListener {

    private static final String TAG = "DevelopFragment";

    private static final String KEY_KERNEL_LOG_CONFIG    = "kernel_loglevel_config";
    private static final String KEY_DTVKIT               = "dtvkit_features";

    public static final String ENV_KERNEL_LOG_LEVEL      = "ubootenv.var.loglevel";

    private SystemControlManager mSystemControlManager;
    private Preference mKernelLogPref;
    private Preference mDtvkitPref;


    public static DevelopFragment newInstance() {
        return new DevelopFragment();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        setPreferencesFromResource(R.xml.develop, null);
        mSystemControlManager = SystemControlManager.getInstance();

        mKernelLogPref=findPreference(KEY_KERNEL_LOG_CONFIG);
        mDtvkitPref=findPreference(KEY_DTVKIT);
        mKernelLogPref.setOnPreferenceChangeListener(this);
        if (!SystemProperties.get("ro.product.brand").contains("Amlogic")) {
            mKernelLogPref.setVisible(false);
            mDtvkitPref.setVisible(false);
        }
        String level = mSystemControlManager.getBootenv(ENV_KERNEL_LOG_LEVEL, "1");
        if (level.contains("1")) {
            ((SwitchPreference)mKernelLogPref).setChecked(false);
            mKernelLogPref.setSummary(R.string.captions_display_off);
        } else {
            ((SwitchPreference)mKernelLogPref).setChecked(true);
            mKernelLogPref.setSummary(R.string.captions_display_on);
        }

    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        if (TextUtils.equals(preference.getKey(), KEY_KERNEL_LOG_CONFIG)) {
            String level = mSystemControlManager.getBootenv(ENV_KERNEL_LOG_LEVEL, "1");
            if (level.contains("1")) {
                mSystemControlManager.setBootenv(ENV_KERNEL_LOG_LEVEL, "7");
                mKernelLogPref.setSummary(R.string.captions_display_on);
            } else {
                mSystemControlManager.setBootenv(ENV_KERNEL_LOG_LEVEL, "1");
                mKernelLogPref.setSummary(R.string.captions_display_off);
            }
        }
        return true;
    }
    @Override
    public int getMetricsCategory() {
        return 0;
    }
}

