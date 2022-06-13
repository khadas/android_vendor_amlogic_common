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

package com.android.tv.settings.wifi;
import static android.net.ConnectivityManager.ACTION_TETHER_STATE_CHANGED;
import static android.net.wifi.WifiManager.WIFI_AP_STATE_CHANGED_ACTION;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.wifi.SoftApConfiguration;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.UserManager;
import androidx.annotation.VisibleForTesting;
import android.util.Log;
import androidx.preference.ListPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceGroup;
import androidx.preference.SeekBarPreference;
import androidx.preference.TwoStatePreference;
import com.android.settingslib.core.AbstractPreferenceController;
import com.android.internal.logging.nano.MetricsProto;
import com.android.tv.settings.R;
import java.util.ArrayList;
import java.util.List;
public class HotSpotFragment extends HotSpotBaseFragment implements
                WifiTetherBasePreferenceController.OnTetherConfigUpdateListener,
                WifiTetherEnablePreferenceController.CheckboxCallback{
    private static final String TAG = "HotSpotFragment";
    private static final IntentFilter TETHER_STATE_CHANGE_FILTER;
    static {
        TETHER_STATE_CHANGE_FILTER = new IntentFilter(ACTION_TETHER_STATE_CHANGED);
        TETHER_STATE_CHANGE_FILTER.addAction(WIFI_AP_STATE_CHANGED_ACTION);
    }

    private static final String KEY_WIFI_ENABLE_HOTSPOT = "hotspot_settings_enable";
    private static final String KEY_WIFI_TETHER_AUTO_OFF = "wifi_tether_auto_turn_off";
    private WifiTetherEnablePreferenceController mSwitchBarController;
    private WifiTetherSSIDPreferenceController mSSIDPreferenceController;
    private WifiTetherPasswordPreferenceController mPasswordPreferenceController;
    private WifiTetherSecurityPreferenceController mSecurityPreferenceController;
    private WifiTetherApBandPreferenceController mApBandPreferenceController;
    private WifiTetherAutoOffPreferenceController mAutoOffController;
    private boolean mRestartWifiApAfterConfigChange;

    private final ArrayList<Preference> mAllPrefs = new ArrayList<>();
    private WifiManager mWifiManager;
    TetherChangeReceiver mTetherChangeReceiver;
    private Context mContext;
    public static HotSpotFragment newInstance() {
        return new HotSpotFragment();
    }

    @Override
    public void onAttach(Context context) {
        Log.d(TAG,"onAttach");
        super.onAttach(context);
        mContext = context;
        mWifiManager = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
        mTetherChangeReceiver = new TetherChangeReceiver();
    }
    @Override
    public void onDetach() {
        super.onDetach();

    }
    protected int getPreferenceScreenResId(){
        return R.xml.hotspot;
    }
    @Override
    protected List<AbstractPreferenceController> createPreferenceControllers(Context context) {
        Log.d(TAG,"createPreferenceControllers");
        final List<AbstractPreferenceController> controllers = new ArrayList<>();
        mSSIDPreferenceController = new WifiTetherSSIDPreferenceController(context, this);
        mSecurityPreferenceController = new WifiTetherSecurityPreferenceController(context, this);
        mPasswordPreferenceController = new WifiTetherPasswordPreferenceController(context, this);
        mApBandPreferenceController = new WifiTetherApBandPreferenceController(context, this);
        mAutoOffController = new WifiTetherAutoOffPreferenceController(context, KEY_WIFI_TETHER_AUTO_OFF);
        controllers.add(mSSIDPreferenceController);
        controllers.add(mSecurityPreferenceController);
        controllers.add(mPasswordPreferenceController);
        controllers.add(mApBandPreferenceController);
        controllers.add(mAutoOffController);
        return controllers;
    }

    private void updateDisplayWithNewConfig() {
        Log.d(TAG,"updateDisplayWithNewConfig");
        use(WifiTetherSSIDPreferenceController.class)
                .updateDisplay();
        use(WifiTetherSecurityPreferenceController.class)
                .updateDisplay();
        use(WifiTetherPasswordPreferenceController.class)
                .updateDisplay();
        use(WifiTetherApBandPreferenceController.class)
                .updateDisplay();

    }
    @Override
    public void onPrefCheckedChange(boolean enable) {
        Log.d(TAG,"onPrefCheckedChange");
        updateEnabledConfig(enable);
    }
    private void updateEnabledConfig(boolean enable) {
        use(WifiTetherSSIDPreferenceController.class)
                .setEnabled(enable);
        use(WifiTetherSecurityPreferenceController.class)
                .setEnabled(enable);
        use(WifiTetherPasswordPreferenceController.class)
                .setEnabled(enable);
        use(WifiTetherApBandPreferenceController.class)
                .setEnabled(enable);
        use(WifiTetherApBandPreferenceController.class)
                .updateDisplay();
        use(WifiTetherAutoOffPreferenceController.class).setEnabled(enable);
    }
    @Override
    public void onTetherConfigUpdated() {
        Log.d(TAG,"onTetherConfigUpdated");
        final SoftApConfiguration config = buildNewConfig();
        mPasswordPreferenceController.updateVisibility(config.getSecurityType());

        /**
         * if soft AP is stopped, bring up
         * else restart with new config
         * TODO: update config on a running access point when framework support is added
         */
        if (mWifiManager.getWifiApState() == WifiManager.WIFI_AP_STATE_ENABLED) {
            Log.d("TetheringSettings",
                    "Wifi AP config changed while enabled, stop and restart");
            mRestartWifiApAfterConfigChange = true;
            mSwitchBarController.stopTether();
        }
        mWifiManager.setSoftApConfiguration(config);

    }
    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        Log.d(TAG,"onActivityCreated");
    }
    @Override
    public void onStart() {
        Log.d(TAG,"onStart");
        super.onStart();
        final Context context = getContext();
        if (context != null) {
            context.registerReceiver(mTetherChangeReceiver, TETHER_STATE_CHANGE_FILTER);
        }
        mSwitchBarController.onStart();
    }
    @Override
    public void onResume() {
        super.onResume();
        updatePreferenceStates();
    }
    @Override
    public void onStop() {
        Log.d(TAG,"onStop");
        super.onStop();
        final Context context = getContext();
        if (context != null) {
            context.unregisterReceiver(mTetherChangeReceiver);
        }
        mSwitchBarController.onStop();
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        Log.d(TAG,"onCreatePreferences");
        super.onCreatePreferences(savedInstanceState,rootKey);
        mSwitchBarController = new WifiTetherEnablePreferenceController(mContext,KEY_WIFI_ENABLE_HOTSPOT,this);
        mSwitchBarController.displayPreference(getPreferenceScreen());
    }

    private SoftApConfiguration buildNewConfig() {
        final SoftApConfiguration.Builder configBuilder = new SoftApConfiguration.Builder();
        final int securityType = mSecurityPreferenceController.getSecurityType();
        configBuilder.setSsid(mSSIDPreferenceController.getSSID());
        if (securityType == SoftApConfiguration.SECURITY_TYPE_WPA2_PSK) {
            configBuilder.setPassphrase(
                    mPasswordPreferenceController.getPasswordValidated(securityType),
                    SoftApConfiguration.SECURITY_TYPE_WPA2_PSK);
        }
        configBuilder.setBand(mApBandPreferenceController.getBandIndex());
        return configBuilder.build();
    }


    private void startTether() {
        mRestartWifiApAfterConfigChange = false;
        mSwitchBarController.startTether();
    }
    class TetherChangeReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context content, Intent intent) {
            String action = intent.getAction();
            Log.d(TAG, "updating display config due to receiving broadcast action " + action);
            updateDisplayWithNewConfig();
            if (action.equals(ACTION_TETHER_STATE_CHANGED)) {
                if (mWifiManager.getWifiApState() == WifiManager.WIFI_AP_STATE_DISABLED
                        && mRestartWifiApAfterConfigChange) {
                    startTether();
                }
            } else if (action.equals(WIFI_AP_STATE_CHANGED_ACTION)) {
                int state = intent.getIntExtra(WifiManager.EXTRA_WIFI_AP_STATE, 0);
                Log.d(TAG,"state:"+state+"mRestartWifiApAfterConfigChange:"+mRestartWifiApAfterConfigChange);
                if (state == WifiManager.WIFI_AP_STATE_DISABLED
                        && mRestartWifiApAfterConfigChange) {
                    startTether();
                }
            }
        }
    }

}
