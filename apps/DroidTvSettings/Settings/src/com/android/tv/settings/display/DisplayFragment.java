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

package com.android.tv.settings.display;

import android.os.Bundle;
import android.os.Handler;
import com.android.tv.settings.SettingsPreferenceFragment;
import androidx.preference.Preference;
import androidx.preference.PreferenceCategory;
import androidx.preference.ListPreference;
import android.os.SystemProperties;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;
import android.text.TextUtils;
import com.android.tv.settings.util.DroidUtils;
import com.android.tv.settings.SettingsConstant;
import com.android.tv.settings.R;
import com.droidlogic.app.SystemControlManager;
import android.util.Log;

public class DisplayFragment extends SettingsPreferenceFragment implements Preference.OnPreferenceChangeListener {

    private static final String TAG = "DisplayFragment";

    private static final String KEY_POSITION           = "position";
    private static final String KEY_OUTPUTMODE         = "outputmode";
    private static final String KEY_HDR                = "hdr";
    private static final String KEY_SDR                = "sdr";
    private static final String KEY_DOLBY_VISION       = "dolby_vision";
    private static final String KEY_ALLM_MODE          = "allm_mode";
    private static final String KEY_GAME_CONTENT_TYPE  = "game_content_type";
    private static final String KEY_MEMC               = "memc";
    private static final String PROP_MEMC              = "persist.vendor.sys.memc";

    private static final int MEMC_OFF                  = 0;
    private static final int MEMC_ON                   = 1;
    private static final String KEY_SCREEN_BRIGHTNESS = "screen_brightness";
	
    private ListPreference mAllmPref;
    private ListPreference mMEMCPref;
    private SystemControlManager mSystemControlManager;
    private static int memcStatus = 0;
    private static boolean isT3Device = false;

    public static DisplayFragment newInstance() {
        return new DisplayFragment();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        setPreferencesFromResource(R.xml.display, null);
        boolean tvFlag = SettingsConstant.needDroidlogicTvFeature(getContext())
            && (SystemProperties.getBoolean("vendor.tv.soc.as.mbox", false) == false);
        final Preference outputmodePref = findPreference(KEY_OUTPUTMODE);
        outputmodePref.setVisible(SettingsConstant.needScreenResolutionFeture(getContext()) && !tvFlag);
        final Preference brightnessPref = findPreference(KEY_SCREEN_BRIGHTNESS);
        brightnessPref.setVisible(true);
        final Preference screenPositionPref = findPreference(KEY_POSITION);
        screenPositionPref.setVisible(!tvFlag);

        final Preference sdrPref = findPreference(KEY_SDR);
        sdrPref.setVisible(false);

        final Preference hdrPref = findPreference(KEY_HDR);
        hdrPref.setVisible(false);

        final Preference dvPref =(Preference) findPreference(KEY_DOLBY_VISION);
        dvPref.setVisible((SystemProperties.getBoolean("vendor.system.support.dolbyvision", false) == true)
            && tvFlag);

        mSystemControlManager = SystemControlManager.getInstance();
        mAllmPref = (ListPreference) findPreference(KEY_ALLM_MODE);
        mAllmPref.setOnPreferenceChangeListener(this);
        mAllmPref.setVisible(SystemProperties.getBoolean("ro.vendor.debug.allm", false));

        mSystemControlManager = SystemControlManager.getInstance();
        mAllmPref = (ListPreference) findPreference(KEY_GAME_CONTENT_TYPE);
        mAllmPref.setOnPreferenceChangeListener(this);
        mAllmPref.setVisible(SystemProperties.getBoolean("ro.vendor.debug.allm", false));

        if (SystemProperties.getBoolean(PROP_MEMC, false)  == true) {
                memcStatus = MEMC_ON;
        }else {
                memcStatus = MEMC_OFF;
        }

        isT3Device = mSystemControlManager.hasMemcFunc();
        mMEMCPref = (ListPreference) findPreference(KEY_MEMC);
        mMEMCPref.setValueIndex(memcStatus);
        mMEMCPref.setVisible(isT3Device);
        mMEMCPref.setOnPreferenceChangeListener(this);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        if (TextUtils.equals(preference.getKey(), KEY_ALLM_MODE)) {
            int allmmode = Integer.parseInt((String)newValue);
            if (allmmode == 0) {
                /* set allmmode to 0 is not really disable the allm mode
                   the VSIF still contain the allm mode info (0), and it still
                   will conflit with Dobly Vision.
                   so amlogic add a new value -1 to readlly disable disable allm
                   mode, not only driver info, but also VSIF info
                */
                allmmode = -1;
            }
            mSystemControlManager.setALLMMode(allmmode);
        }

        if (TextUtils.equals(preference.getKey(), KEY_MEMC)) {
            memcStatus = Integer.parseInt((String)newValue);
            //mSystemControlManager.setALLMMode(allmmode);
            if (mSystemControlManager.hasMemcFunc()) {
                if (memcStatus == MEMC_ON) {
                    mSystemControlManager.memcContrl(true);
                }else if (memcStatus == MEMC_OFF) {
                    mSystemControlManager.memcContrl(false);
                }
            }
        }

        if (TextUtils.equals(preference.getKey(), KEY_GAME_CONTENT_TYPE)) {
            mSystemControlManager.sendHDMIContentType(Integer.parseInt((String)newValue));
        }
        return true;
    }

    @Override
    public int getMetricsCategory() {
        return 0;
    }

}
