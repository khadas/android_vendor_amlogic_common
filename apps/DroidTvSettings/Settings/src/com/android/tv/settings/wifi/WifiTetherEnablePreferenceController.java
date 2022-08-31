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

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.wifi.WifiManager;
import android.os.Handler;
import android.os.Looper;
import android.provider.Settings;
import androidx.preference.SwitchPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceScreen;
import android.util.Log;

import static android.net.ConnectivityManager.TETHERING_WIFI;

public class WifiTetherEnablePreferenceController extends BasePreferenceController implements
        Preference.OnPreferenceChangeListener {
    private static final String TAG = "WifiTetherEnablePreferenceController";
    private static final IntentFilter WIFI_INTENT_FILTER;

    static {
        WIFI_INTENT_FILTER = new IntentFilter(WifiManager.WIFI_AP_STATE_CHANGED_ACTION);
        WIFI_INTENT_FILTER.addAction(Intent.ACTION_AIRPLANE_MODE_CHANGED);
    }

    private final Context mContext;
    private final ConnectivityManager mConnectivityManager;
    private final WifiManager mWifiManager;
    private SwitchPreference mSwitchPref;
    final ConnectivityManager.OnStartTetheringCallback mOnStartTetheringCallback =
            new ConnectivityManager.OnStartTetheringCallback() {
                @Override
                public void onTetheringFailed() {
                    Log.d(TAG, "onTetheringFailed");
                    super.onTetheringFailed();
                    mSwitchPref.setChecked(false);
                    updateWifiSwitch();
                }

                @Override
                public void onTetheringStarted() {
                    Log.d(TAG, "onTetheringStarted");
                    super.onTetheringStarted();
                    //mSwitchPref.setChecked(true);
                    updateWifiSwitch();
                }
            };
    private boolean settingsOn;
    private String mPrefKey;
    private CheckboxCallback mCallBack;
    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (WifiManager.WIFI_AP_STATE_CHANGED_ACTION.equals(action)) {
                final int state = intent.getIntExtra(
                        WifiManager.EXTRA_WIFI_AP_STATE, WifiManager.WIFI_AP_STATE_FAILED);
                handleWifiApStateChanged(state);
            } else if (Intent.ACTION_AIRPLANE_MODE_CHANGED.equals(action)) {
                updateWifiSwitch();
            }
        }
    };

    public WifiTetherEnablePreferenceController(Context context, String preferenceKey, CheckboxCallback callback) {
        super(context, preferenceKey);
        mPrefKey = preferenceKey;
        mContext = context;
        mConnectivityManager =
                (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
        mWifiManager = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
        mCallBack = callback;
        //mSwitchPref.setChecked(mWifiManager.getWifiApState() == WifiManager.WIFI_AP_STATE_ENABLED);
        // updateWifiSwitch();
    }

    @Override
    public int getAvailabilityStatus() {
        return AVAILABLE;
    }

    void startTether() {
        Log.d(TAG, "startTether");
        mSwitchPref.setEnabled(false);
        mConnectivityManager.startTethering(TETHERING_WIFI, true /* showProvisioningUi */,
                mOnStartTetheringCallback, new Handler(Looper.getMainLooper()));
    }

    void stopTether() {
        Log.d(TAG, "stopTether");
        mSwitchPref.setEnabled(false);
        mConnectivityManager.stopTethering(TETHERING_WIFI);
    }

    @Override
    public void updateState(Preference preference) {
        mSwitchPref = (SwitchPreference) preference;
        Log.d(TAG, "updateState");
    }

    public void onStart() {
        Log.d(TAG, "onStart");
        mContext.registerReceiver(mReceiver, WIFI_INTENT_FILTER);
    }

    public boolean hotSpotEnabled() {
        return mSwitchPref.isChecked();
    }

    public void onStop() {
        Log.d(TAG, "onStop");
        mContext.unregisterReceiver(mReceiver);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {

        mSwitchPref = (SwitchPreference) preference;
        settingsOn = (Boolean) newValue;
        Log.d(TAG, "onPreferenceChange" + settingsOn + "mSwitchPref.isChecked()" + mSwitchPref.isChecked()
                + "mWifiManager.isWifiApEnabled()" + mWifiManager.isWifiApEnabled());
        if (!settingsOn && mWifiManager.isWifiApEnabled()) {
            stopTether();
        }
        if (settingsOn && !mWifiManager.isWifiApEnabled()) {
            startTether();
        }
        return true;
    }

    private void updateWifiSwitch() {
        boolean isAirplaneMode = Settings.Global.getInt(mContext.getContentResolver(),
                Settings.Global.AIRPLANE_MODE_ON, 0) != 0;
        if (!isAirplaneMode) {
            mSwitchPref.setEnabled(true);
        } else {
            mSwitchPref.setEnabled(false);
        }
    }

    @Override
    public void displayPreference(PreferenceScreen screen) {
        super.displayPreference(screen);
        mSwitchPref = (SwitchPreference) screen.findPreference(getPreferenceKey());
        mSwitchPref.setOnPreferenceChangeListener(this);
        Log.d(TAG, "mWifiManager.getWifiApState() " + mWifiManager.getWifiApState());
        mSwitchPref.setChecked(mWifiManager.getWifiApState() == WifiManager.WIFI_AP_STATE_ENABLED);
        updateWifiSwitch();
        mCallBack.onPrefCheckedChange(mSwitchPref.isChecked());
    }

    private void handleWifiApStateChanged(int state) {
        Log.d(TAG, "handleWifiApStateChanged: " + state);
        switch (state) {
            case WifiManager.WIFI_AP_STATE_ENABLING: //12
                mSwitchPref.setEnabled(false);
                break;
            case WifiManager.WIFI_AP_STATE_ENABLED: //13
                if (!mSwitchPref.isChecked()) {
                    mSwitchPref.setChecked(true);
                }
                updateWifiSwitch();
                break;
            case WifiManager.WIFI_AP_STATE_DISABLING: //10
                if (mSwitchPref.isChecked()) {
                    mSwitchPref.setChecked(false);
                }
                mSwitchPref.setEnabled(false);
                break;
            case WifiManager.WIFI_AP_STATE_DISABLED: //11
                mSwitchPref.setChecked(false);
                updateWifiSwitch();
                break;
            default:
                mSwitchPref.setChecked(false);
                updateWifiSwitch();
                break;
        }
        mCallBack.onPrefCheckedChange(mSwitchPref.isChecked());
    }

    public interface CheckboxCallback {
        public void onPrefCheckedChange(boolean enable);
    }
}
