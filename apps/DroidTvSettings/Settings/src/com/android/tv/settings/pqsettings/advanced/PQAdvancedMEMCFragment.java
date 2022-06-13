/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC PQAdvancedMEMCFragment
 */



package com.android.tv.settings.pqsettings.advanced;

import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.os.RemoteException;
import android.provider.Settings;
import androidx.preference.SwitchPreference;
import androidx.preference.SeekBarPreference;
import com.android.tv.settings.SettingsPreferenceFragment;
import androidx.preference.ListPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceScreen;
import androidx.preference.TwoStatePreference;
import androidx.preference.PreferenceCategory;
import android.util.ArrayMap;
import android.util.Log;
import android.text.TextUtils;

import com.droidlogic.app.DisplayPositionManager;
import com.android.tv.settings.R;
import com.android.tv.settings.RadioPreference;
import com.android.tv.settings.dialog.old.Action;
import com.android.tv.settings.SettingsConstant;
import com.android.tv.settings.pqsettings.PQSettingsManager;

import java.util.List;
import java.util.Map;
import java.util.ArrayList;

public class PQAdvancedMEMCFragment extends SettingsPreferenceFragment implements Preference.OnPreferenceChangeListener {
    private static final String TAG = "PQAdvancedMEMCFragment";

    private static final int PQ_PICTRUE_ADVANCED_MEMC_CUSTOMIZE_STEP= 1;
    private static final String PQ_PICTRUE_ADVANCED_MEMC_SWITCH = "pq_pictrue_advanced_memc_switch";
    private static final String PQ_PICTRUE_ADVANCED_MEMC_CUSTOMIZE_DEJUDDER = "pq_pictrue_advanced_memc_customize_dejudder";
    private static final String PQ_PICTRUE_ADVANCED_MEMC_CUSTOMIZE_DEBLUR= "pq_pictrue_advanced_memc_customize_deblur";

    private SeekBarPreference PQPictureAdvancedMemcCustomizeDejudderPref;
    private SeekBarPreference PQPictureAdvancedMemcCustomizeDeblurPref;


    private PQSettingsManager mPQSettingsManager;


    public static PQAdvancedMEMCFragment newInstance() {
        return new PQAdvancedMEMCFragment();
    }

    public static boolean CanDebug() {
        return PQSettingsManager.CanDebug();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (mPQSettingsManager == null) {
            mPQSettingsManager = new PQSettingsManager(getActivity());
        }
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        setPreferencesFromResource(R.xml.pq_pictrue_advanced_memc, null);
        if (mPQSettingsManager == null) {
            mPQSettingsManager = new PQSettingsManager(getActivity());
        }

        final ListPreference pictureAdvancedMemcSwitchPref = (ListPreference) findPreference(PQ_PICTRUE_ADVANCED_MEMC_SWITCH);
        PQPictureAdvancedMemcCustomizeDejudderPref = (SeekBarPreference) findPreference(PQ_PICTRUE_ADVANCED_MEMC_CUSTOMIZE_DEJUDDER);
        PQPictureAdvancedMemcCustomizeDeblurPref = (SeekBarPreference) findPreference(PQ_PICTRUE_ADVANCED_MEMC_CUSTOMIZE_DEBLUR);

        if (true) {//Leave blank first, add conditions later
            pictureAdvancedMemcSwitchPref.setValueIndex(mPQSettingsManager.getAdvancedMemcSwitchStatus());
            pictureAdvancedMemcSwitchPref.setOnPreferenceChangeListener(this);
        } else {
            pictureAdvancedMemcSwitchPref.setVisible(false);
        }

        if (true) {//Leave blank first, add conditions later
            PQPictureAdvancedMemcCustomizeDejudderPref.setOnPreferenceChangeListener(this);
            PQPictureAdvancedMemcCustomizeDejudderPref.setSeekBarIncrement(PQ_PICTRUE_ADVANCED_MEMC_CUSTOMIZE_STEP);
            PQPictureAdvancedMemcCustomizeDejudderPref.setMin(0);
            PQPictureAdvancedMemcCustomizeDejudderPref.setMax(10);
            PQPictureAdvancedMemcCustomizeDejudderPref.setValue(mPQSettingsManager.getAdvancedMemcCustomizeDejudderStatus());
            PQPictureAdvancedMemcCustomizeDejudderPref.setVisible(true);
        } else {
            PQPictureAdvancedMemcCustomizeDejudderPref.setVisible(false);
        }

        if (true) {//Leave blank first, add conditions later
            PQPictureAdvancedMemcCustomizeDeblurPref.setOnPreferenceChangeListener(this);
            PQPictureAdvancedMemcCustomizeDeblurPref.setSeekBarIncrement(PQ_PICTRUE_ADVANCED_MEMC_CUSTOMIZE_STEP);
            PQPictureAdvancedMemcCustomizeDeblurPref.setMin(0);
            PQPictureAdvancedMemcCustomizeDeblurPref.setMax(10);
            PQPictureAdvancedMemcCustomizeDeblurPref.setValue(mPQSettingsManager.getAdvancedMemcCustomizeDeblurStatus());
            PQPictureAdvancedMemcCustomizeDeblurPref.setVisible(true);
        } else {
            PQPictureAdvancedMemcCustomizeDeblurPref.setVisible(false);
        }

        updateMemcCustomizeDisplay(mPQSettingsManager.getAdvancedMemcSwitchStatus());

    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        return super.onPreferenceTreeClick(preference);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        Log.d(TAG, "[onPreferenceTreeClick] preference.getKey() = " + preference.getKey()+" newValue:"+newValue);
        switch (preference.getKey()) {
            case PQ_PICTRUE_ADVANCED_MEMC_SWITCH:
                final int selection = Integer.parseInt((String)newValue);
                updateMemcCustomizeDisplay(selection);
                mPQSettingsManager.setAdvancedMemcSwitchStatus(selection);
                break;
            case PQ_PICTRUE_ADVANCED_MEMC_CUSTOMIZE_DEJUDDER:
                mPQSettingsManager.setAdvancedMemcCustomizeDejudderStatus((int)newValue);
                break;
            case PQ_PICTRUE_ADVANCED_MEMC_CUSTOMIZE_DEBLUR:
                mPQSettingsManager.setAdvancedMemcCustomizeDeblurStatus((int)newValue);
                break;
        }
        return true;
    }

    @Override
    public int getMetricsCategory() {
        return 0;
    }

    private void updateMemcCustomizeDisplay(int value) {
        if ( 1 == value ) {
            PQPictureAdvancedMemcCustomizeDeblurPref.setEnabled(true);
            PQPictureAdvancedMemcCustomizeDejudderPref.setEnabled(true);
        } else {
            PQPictureAdvancedMemcCustomizeDeblurPref.setEnabled(false);
            PQPictureAdvancedMemcCustomizeDejudderPref.setEnabled(false);
        }
    }

}
