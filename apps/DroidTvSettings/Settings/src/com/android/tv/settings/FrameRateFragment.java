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

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import androidx.preference.Preference;
import androidx.preference.TwoStatePreference;
import android.text.TextUtils;
import android.util.Log;

import com.droidlogic.app.SystemControlManager;

public class FrameRateFragment extends SettingsPreferenceFragment implements Preference.OnPreferenceChangeListener {
    private static final String TAG = "FrameRateFragment";

    private static final String KEY_ENABLE_RATEFRAME = "frame_rate_enable";

    private Context mContext;
    private static FrameRateService mService;
    private TwoStatePreference enableFrameRatePref;
    private SystemControlManager mSystemControlManager;

    private static final String PROP_FRAMERATE_ENABLE = "persist.vendor.sys.framerate.enable";

    private ServiceConnection mConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            FrameRateService.FrameRateBinder binder = (FrameRateService.FrameRateBinder) service;
            mService = binder.getService();
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
        }
    };

    public static FrameRateFragment newInstance() {
        return new FrameRateFragment();
    }

    @Override
    public void onAttach(Context context) {
        Log.d(TAG, "FrameRate onAttach");
        super.onAttach(context);
        mContext = context;
        mContext.bindService(new Intent(mContext, FrameRateService.class),
            mConnection, mContext.BIND_AUTO_CREATE);
    }

    @Override
    public void onDetach() {
        super.onDetach();

    }

    @Override
    public int getMetricsCategory() {
        return 0;
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
    }

    @Override
    public void onStart() {
        super.onStart();
    }

    @Override
    public void onResume() {
        super.onResume();
    }

    @Override
    public void onStop() {
        super.onStop();
        mContext.unbindService(mConnection);
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        setPreferencesFromResource(R.xml.framerate, null);
        mSystemControlManager = SystemControlManager.getInstance();

        enableFrameRatePref = (TwoStatePreference) findPreference(KEY_ENABLE_RATEFRAME);
        enableFrameRatePref.setOnPreferenceChangeListener(this);
        enableFrameRatePref.setChecked(getFrameRateEnabled());

    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        if (TextUtils.equals(preference.getKey(), KEY_ENABLE_RATEFRAME)) {
            setFrameRateEnabled((boolean) newValue);
        }
        return true;
    }

    private boolean getFrameRateEnabled() {
        return mSystemControlManager.getPropertyBoolean(PROP_FRAMERATE_ENABLE, false);
    }

    private void setFrameRateEnabled(boolean enable) {
        if (mService != null) {
            mService.setFrameRateEnabled(enable);
        }
    }

}
