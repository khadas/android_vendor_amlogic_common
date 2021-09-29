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
//import androidx.leanback.preference.LeanbackPreferenceFragment;
import androidx.preference.Preference;
import androidx.preference.TwoStatePreference;
import android.text.TextUtils;
import android.util.Log;
import android.os.SystemProperties;

import com.droidlogic.app.SystemControlManager;
import com.android.tv.settings.util.DroidUtils;
public class AiPqFragment extends SettingsPreferenceFragment implements Preference.OnPreferenceChangeListener {
    private static final String TAG = "AiPqFragment";

    private static final String KEY_ENABLE_AIPQ = "ai_pq_enable";
    public static final String KEY_ENABLE_AIPQ_INFO = "ai_pq_info_enable";
    private static final String KEY_ENABLE_AISR = "ai_sr_enable";
    private static final String SYSFS_DEBUG_VDETECT = "/sys/module/decoder_common/parameters/debug_vdetect";
    private static final String SYSFS_ADD_VDETECT = "/sys/class/vdetect/tv_add_vdetect";
    private Context mContext;
    private SystemControlManager mSystemControlManager;
    private static AiPqService mService;
    private TwoStatePreference enableAipqPref;
    private TwoStatePreference enableAisrPref;
    private TwoStatePreference enableAipqInfoPref;
    public static AiPqFragment newInstance() {
        return new AiPqFragment();
    }

    @Override
    public void onAttach(Context context) {
        Log.d(TAG, "onAttach");
        super.onAttach(context);
        mContext = context;

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
        Log.d(TAG, "onActivityCreated");

    }

    @Override
    public void onStart() {
        Log.d(TAG, "onStart");
        super.onStart();
        mContext.bindService(new Intent(mContext, AiPqService.class), mConnection, mContext.BIND_AUTO_CREATE);
    }

    @Override
    public void onResume() {
        super.onResume();
    }

    @Override
    public void onStop() {
        Log.d(TAG, "onStop");
        super.onStop();
        mContext.unbindService(mConnection);
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        setPreferencesFromResource(R.xml.aipq, null);
        mSystemControlManager = SystemControlManager.getInstance();
        enableAipqPref = (TwoStatePreference) findPreference(KEY_ENABLE_AIPQ);
        enableAipqPref.setOnPreferenceChangeListener(this);
        enableAipqPref.setChecked(getAipqEnabled());

        enableAisrPref = (TwoStatePreference) findPreference(KEY_ENABLE_AISR);
        enableAisrPref.setOnPreferenceChangeListener(this);
        enableAisrPref.setChecked(getAisrEnabled());

        enableAipqInfoPref = (TwoStatePreference) findPreference(KEY_ENABLE_AIPQ_INFO);
        enableAipqInfoPref.setOnPreferenceChangeListener(this);
        if (mService != null) {
            enableAipqInfoPref.setChecked(mService.infoEnabled());
        }
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        Log.d(TAG, "[onPreferenceChange] preference.getKey() = " + preference.getKey() + ", newValue = " + newValue);
        if (TextUtils.equals(preference.getKey(), KEY_ENABLE_AIPQ)) {
            if ((boolean) newValue) {
                enableAipqInfoPref.setEnabled(true);
            } else {
                enableAipqInfoPref.setEnabled(false);
                enableAipqInfoPref.setChecked(false);
                if (mService != null) mService.disableAipq();
            }
            setAipqEnabled((boolean) newValue);
        } else if (TextUtils.equals(preference.getKey(), KEY_ENABLE_AIPQ_INFO)) {
            if ((boolean) newValue) {
                if (mService != null) mService.enableAipq();
            } else {
                if (mService != null) mService.disableAipq();
            }
        } else if (TextUtils.equals(preference.getKey(), KEY_ENABLE_AISR)) {
            setAisrEnabled((boolean) newValue);
            Log.d(TAG, "AipqEnabled=" + getAisrEnabled() + ", Nvalue= " + newValue);
        }
        return true;
    }

    private ServiceConnection mConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            AiPqService.AiPqBinder binder = (AiPqService.AiPqBinder) service;
            mService = binder.getService();
            if (mService != null) {
                enableAipqInfoPref.setChecked(mService.isShowing());
            }
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
        }
    };

    private boolean getAipqEnabled() {
        return mSystemControlManager.getAipqEnable();
    }

    private boolean getAisrEnabled() {
        return mSystemControlManager.getAisr();
    }

    private void setAipqEnabled(boolean enable) {
        mSystemControlManager.setAipqEnable(enable);
    }


    private void setAisrEnabled(boolean enable) {
        //mSystemControlManager.setAiSrEnable(enable);
        mSystemControlManager.aisrContrl(enable);
    }
}
