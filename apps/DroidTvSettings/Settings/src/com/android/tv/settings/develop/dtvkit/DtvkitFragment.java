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

package com.android.tv.settings.develop.dtvkit;

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

import android.content.Context;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.LayoutInflater;
import android.view.Window;
import android.view.WindowManager;
import android.widget.TextView;
import android.view.Display;
import android.view.WindowManager;
import android.provider.Settings;
import com.android.tv.settings.develop.dtvkit.DtvkitRebootConfirmFragment;
import com.android.tv.twopanelsettings.TwoPanelSettingsFragment;
import androidx.leanback.preference.LeanbackSettingsFragmentCompat;
import androidx.fragment.app.Fragment;
import android.app.tvsettings.TvSettingsEnums;
import static com.android.tv.settings.util.InstrumentationUtils.logEntrySelected;

public class DtvkitFragment extends SettingsPreferenceFragment implements Preference.OnPreferenceChangeListener {

	private static final String TAG = "DtvkitFragment";
    private static final String KEY_DTVKIT_FCC    = "dtvkit_fcc";
    private static final String KEY_DTVKIT_PIP    = "dtvkit_pip";
    private static final String KEY_DVBC_DVBS_SWITCH = "dvbc_dvbs_switch";

    private static final String PROP_DTV_PIPFCC    = "vendor.tv.dtv.pipfcc.architecture";
    private static final String PROP_DTV_PIPLINE   = "vendor.amtsplayer.pipeline";
    private static final String PROP_DTV_PIP       = "vendor.tv.dtv.enable.pip";
    private static final String PROP_DTV_FCC       = "vendor.tv.dtv.enable.fcc";
    private static final String PROP_DTV_AUDIO     = "vendor.dtv.audio.skipamadec";
    private static final String PROP_DVBC_DVBS_SWITCH     = "persist.vendor.tvconfig.path";
    private static final String PROP_DVBC_PATH     = "/mnt/vendor/odm_ext/etc/tvconfig/dtvkit/config.xml";
    private static final String PROP_DVBS_PATH     = "/mnt/vendor/odm_ext/etc/tvconfig/dtvkit/config_dvbs.xml";

    private Preference mDtvkitFCC;
    private Preference mDtvkitPIP;
    private Preference mDvbcDvbsSwitchPref;

	private SystemControlManager mSystemControlManager;


	public static DtvkitFragment newInstance() {
		return new DtvkitFragment();
	}

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
	}

	@Override
	public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
		setPreferencesFromResource(R.xml.dtvkit, null);
		mSystemControlManager = SystemControlManager.getInstance();
        mDtvkitFCC=findPreference(KEY_DTVKIT_FCC);
        mDtvkitPIP=findPreference(KEY_DTVKIT_PIP);
        mDvbcDvbsSwitchPref=findPreference(KEY_DVBC_DVBS_SWITCH);
        mDtvkitFCC.setOnPreferenceChangeListener(this);
        mDtvkitPIP.setOnPreferenceChangeListener(this);
        mDvbcDvbsSwitchPref.setOnPreferenceChangeListener(this);
        updateUI();
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        if (TextUtils.equals(preference.getKey(), KEY_DTVKIT_PIP)) {
            if (isEnablePIP()) {
                mSystemControlManager.setProperty(PROP_DTV_PIP, "false");
                if (!isEnableFCC()) {
                    mSystemControlManager.setProperty(PROP_DTV_AUDIO, "false");
                    mSystemControlManager.setProperty(PROP_DTV_PIPFCC, "false");
                    mSystemControlManager.setProperty(PROP_DTV_PIPLINE, "0");
                }
            } else {
                mSystemControlManager.setProperty(PROP_DTV_AUDIO, "true");
                mSystemControlManager.setProperty(PROP_DTV_PIPFCC, "true");
                mSystemControlManager.setProperty(PROP_DTV_PIPLINE, "1");
                mSystemControlManager.setProperty(PROP_DTV_PIP, "true");
                mSystemControlManager.setProperty(PROP_DTV_FCC, "false");
            }
        }
        if (TextUtils.equals(preference.getKey(), KEY_DTVKIT_FCC)) {
            if (isEnableFCC()) {
                mSystemControlManager.setProperty(PROP_DTV_FCC, "false");
                if (!isEnablePIP()) {
                    mSystemControlManager.setProperty(PROP_DTV_AUDIO, "false");
                    mSystemControlManager.setProperty(PROP_DTV_PIPFCC, "false");
                    mSystemControlManager.setProperty(PROP_DTV_PIPLINE, "0");
                }
            } else {
                mSystemControlManager.setProperty(PROP_DTV_AUDIO, "true");
                mSystemControlManager.setProperty(PROP_DTV_PIPFCC, "true");
                mSystemControlManager.setProperty(PROP_DTV_PIPLINE, "1");
                mSystemControlManager.setProperty(PROP_DTV_FCC, "true");
                mSystemControlManager.setProperty(PROP_DTV_PIP, "false");
            }
        }
        if (TextUtils.equals(preference.getKey(), KEY_DVBC_DVBS_SWITCH)) {
            boolean mDvbcDvbsSwitchPropEnabled = false;
            mDvbcDvbsSwitchPropEnabled = getDvbcDvbsSwitchPropEnabled();
            rebootConfirm();
            if (mDvbcDvbsSwitchPropEnabled == getDvbcDvbsSwitchPropEnabled())
                return false;
        }
        updateUI();
        return true;
    }

    private boolean getDvbcDvbsSwitchPropEnabled() {
        String strDvbcDvbsSwitchPropEnabled;
        strDvbcDvbsSwitchPropEnabled = mSystemControlManager.getProperty(PROP_DVBC_DVBS_SWITCH);
        if (strDvbcDvbsSwitchPropEnabled == null )
            return false;
        else {
            if (strDvbcDvbsSwitchPropEnabled.equals(PROP_DVBC_PATH))
                return false;
            else {
                if (strDvbcDvbsSwitchPropEnabled.equals(PROP_DVBS_PATH))
                    return true;
            }
        }

        return false;
    }

    private boolean rebootConfirm() {
        logEntrySelected(TvSettingsEnums.SYSTEM_REBOOT);
        Fragment fragment = getCallbackFragment();
        if (fragment instanceof LeanbackSettingsFragmentCompat) {
            ((LeanbackSettingsFragmentCompat) fragment).startImmersiveFragment(
                    DtvkitRebootConfirmFragment.newInstance(true /* safeMode */));
            return true;
        } else if (fragment instanceof TwoPanelSettingsFragment) {
            ((TwoPanelSettingsFragment) fragment).startImmersiveFragment(
                    DtvkitRebootConfirmFragment.newInstance(true /* safeMode */));
            return true;
        }
        return false;
    }

    private void updateUI() {
        ((SwitchPreference)mDtvkitFCC).setChecked(isEnableFCC());
        ((SwitchPreference)mDtvkitPIP).setChecked(isEnablePIP());

        boolean isDvbcDvbsSwitchEnabled = getDvbcDvbsSwitchPropEnabled();
        ((SwitchPreference)mDvbcDvbsSwitchPref).setChecked(isDvbcDvbsSwitchEnabled);
    }

    private boolean isEnablePIP() {
        if (mSystemControlManager.getPropertyBoolean(PROP_DTV_PIPFCC, false)
            && mSystemControlManager.getProperty(PROP_DTV_PIPLINE).contains("1")
            && mSystemControlManager.getPropertyBoolean(PROP_DTV_PIP, false)) {
            return true;
        }
        return false;
    }
    private boolean isEnableFCC() {
        if (mSystemControlManager.getPropertyBoolean(PROP_DTV_PIPFCC, false)
            && mSystemControlManager.getProperty(PROP_DTV_PIPLINE).contains("1")
            && mSystemControlManager.getPropertyBoolean(PROP_DTV_FCC, false)) {
            return true;
        }
        return false;
    }

    @Override
    public int getMetricsCategory() {
        return 0;
    }
}



