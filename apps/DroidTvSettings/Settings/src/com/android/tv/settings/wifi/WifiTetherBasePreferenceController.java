/*
 * Copyright (C) 2017 The Android Open Source Project
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

package com.android.tv.settings.wifi;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.wifi.WifiManager;
import androidx.preference.Preference;
import androidx.preference.PreferenceScreen;
import android.util.Log;
import com.android.settingslib.core.AbstractPreferenceController;

public abstract class WifiTetherBasePreferenceController extends AbstractPreferenceController
        implements Preference.OnPreferenceChangeListener {
    private static final String TAG = "WifiTetherBasePreferenceController";
    public interface OnTetherConfigUpdateListener {
        void onTetherConfigUpdated();
    }

    protected final WifiManager mWifiManager;
    protected final String[] mWifiRegexs;
    protected final ConnectivityManager mCm;
    protected final OnTetherConfigUpdateListener mListener;

    protected Preference mPreference;

    public WifiTetherBasePreferenceController(Context context,
            OnTetherConfigUpdateListener listener) {
        super(context);
        mListener = listener;
        mWifiManager = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
        mCm = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
        mWifiRegexs = mCm.getTetherableWifiRegexs();
    }

    @Override
    public boolean isAvailable() {
        return mWifiManager != null && mWifiRegexs != null && mWifiRegexs.length > 0;
    }

    @Override
    public void displayPreference(PreferenceScreen screen) {
        Log.d(TAG,"displayPreference"+getPreferenceKey());
        super.displayPreference(screen);
        mPreference = screen.findPreference(getPreferenceKey());
        updateDisplay();
    }
    public void setEnabled(boolean enable) {
        if (mPreference != null) {
            mPreference.setEnabled(enable);
        }
    }
    public abstract void updateDisplay();
}
