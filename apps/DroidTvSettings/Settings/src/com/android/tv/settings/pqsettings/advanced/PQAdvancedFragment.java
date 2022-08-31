/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC PQAdvancedFragment
 */



package com.android.tv.settings.pqsettings.advanced;

import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.os.RemoteException;
import android.os.SystemProperties;
import android.provider.Settings;
import androidx.preference.SwitchPreference;
import com.android.tv.settings.SettingsPreferenceFragment;
import androidx.preference.ListPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceScreen;
import androidx.preference.TwoStatePreference;
import androidx.preference.PreferenceCategory;
import androidx.preference.Preference.OnPreferenceChangeListener;
import android.util.ArrayMap;
import android.util.Log;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.ViewGroup;



import com.droidlogic.app.DisplayPositionManager;
import com.android.tv.settings.R;
import com.android.tv.settings.RadioPreference;
import com.android.tv.settings.dialog.old.Action;
import com.android.tv.settings.SettingsConstant;
import com.android.tv.settings.pqsettings.PQSettingsManager;

import java.util.List;
import java.util.Map;
import java.util.ArrayList;

public class PQAdvancedFragment extends SettingsPreferenceFragment implements Preference.OnPreferenceChangeListener {
    private static final String TAG = "PQAdvancedFragment";

    private static final String PQ_PICTRUE_ADVANCED_DYNAMIC_TONE_MAPPING = "pq_pictrue_advanced_dynamic_tone_mapping";
    private static final String PQ_PICTRUE_ADVANCED_COLOR_MANAGEMENT = "pq_pictrue_advanced_color_management";
    private static final String PQ_HDMI_COLOR_RANGE = "pq_hdmi_color_range";
    private static final String PQ_PICTRUE_ADVANCED_COLOR_SPACE = "pq_pictrue_advanced_color_space";
    private static final String PQ_PICTRUE_ADVANCED_GLOBAL_DIMMING = "pq_pictrue_advanced_global_dimming";
    private static final String PQ_PICTRUE_ADVANCED_LOCAL_DIMMING = "pq_pictrue_advanced_local_dimming";
    private static final String PQ_PICTRUE_ADVANCED_BLACK_STRETCH = "pq_pictrue_advanced_black_stretch";
    private static final String PQ_PICTRUE_ADVANCED_DNLP = "pq_pictrue_advanced_dnlp";
    private static final String PQ_PICTRUE_ADVANCED_LOCAL_CONTRAST = "pq_pictrue_advanced_local_contrast";
    private static final String PQ_PICTRUE_ADVANCED_SR = "pq_pictrue_advanced_sr";
    private static final String PQ_DNR = "pq_dnr";
    private static final String PQ_PICTRUE_ADVANCED_DEBLOCK = "pq_pictrue_advanced_deblock";
    private static final String PQ_PICTRUE_ADVANCED_DEMOSQUITO = "pq_pictrue_advanced_demosquito";
    private static final String PQ_PICTRUE_ADVANCED_DECONTOUR = "pq_pictrue_advanced_decontour";

    private static final String PQ_PICTRUE_ADVANCED_GAMMA = "pq_pictrue_advanced_gamma";
    private static final String PQ_PICTRUE_ADVANCED_MANUAL_GAMMA = "pq_pictrue_advanced_manual_gamma";
    private static final String PQ_PICTRUE_ADVANCED_COLOR_TEMPERATURE = "pq_pictrue_advanced_color_temperature";
    private static final String PQ_PICTRUE_ADVANCED_COLOR_CUSTOMIZE = "pq_pictrue_advanced_color_customize";
    private static final String PQ_PICTRUE_ADVANCED_MEMC = "pq_pictrue_advanced_memc";

    private static final String PQ_PICTRUE_T3 = "NNNN";
    private static final String PQ_PICTRUE_T5 = "T963";


    private static final int PQ_PICTRUE_ADVANCED_SOURCE_HDR = 1;


    private PQSettingsManager mPQSettingsManager;

    private Preference pq_brightnessPref;


    public static PQAdvancedFragment newInstance() {
        return new PQAdvancedFragment();
    }

    public static boolean CanDebug() {
        return SystemProperties.getBoolean("sys.pqsetting.debug", false);
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
        setPreferencesFromResource(R.xml.pq_pictrue_advanced, null);
        if (mPQSettingsManager == null) {
            mPQSettingsManager = new PQSettingsManager(getActivity());
        }

        final ListPreference pictureAdvancedDynamicToneMappingPref = (ListPreference) findPreference(PQ_PICTRUE_ADVANCED_DYNAMIC_TONE_MAPPING);
        final ListPreference pictureAdvancedColorManagementPref = (ListPreference) findPreference(PQ_PICTRUE_ADVANCED_COLOR_MANAGEMENT);
        final ListPreference pictureAdvancedColorRangeModePref = (ListPreference) findPreference(PQ_HDMI_COLOR_RANGE);
        final ListPreference pictureAdvancedColorSpacePref = (ListPreference) findPreference(PQ_PICTRUE_ADVANCED_COLOR_SPACE);
        final ListPreference pictureAdvancedGlobalDimmingPref = (ListPreference) findPreference(PQ_PICTRUE_ADVANCED_GLOBAL_DIMMING);
        final ListPreference pictureAdvancedLocalDimmingPref = (ListPreference) findPreference(PQ_PICTRUE_ADVANCED_LOCAL_DIMMING);
        final ListPreference pictureAdvancedBlackStretchPref = (ListPreference) findPreference(PQ_PICTRUE_ADVANCED_BLACK_STRETCH);
        final ListPreference pictureAdvancedDNLPPref = (ListPreference) findPreference(PQ_PICTRUE_ADVANCED_DNLP);
        final ListPreference pictureAdvancedLocalContrastPref = (ListPreference) findPreference(PQ_PICTRUE_ADVANCED_LOCAL_CONTRAST);
        final ListPreference pictureAdvancedSRPref = (ListPreference) findPreference(PQ_PICTRUE_ADVANCED_SR);
        final ListPreference pictureAdvancedDNRPref = (ListPreference) findPreference(PQ_DNR);
        final ListPreference pictureAdvancedDeBlockPref = (ListPreference) findPreference(PQ_PICTRUE_ADVANCED_DEBLOCK);
        final ListPreference pictureAdvancedDeMosquitoPref = (ListPreference) findPreference(PQ_PICTRUE_ADVANCED_DEMOSQUITO);
        final ListPreference pictureAdvancedDecontourPref = (ListPreference) findPreference(PQ_PICTRUE_ADVANCED_DECONTOUR);

        final Preference pictureAdvancedGammaPref = (Preference) findPreference(PQ_PICTRUE_ADVANCED_GAMMA);
        final Preference pictureAdvancedManualGammaPref = (Preference) findPreference(PQ_PICTRUE_ADVANCED_MANUAL_GAMMA);
        final Preference pictureAdvancedColorTemperaturePref = (Preference) findPreference(PQ_PICTRUE_ADVANCED_COLOR_TEMPERATURE);
        final Preference pictureAdvancedColorCustomizePref = (Preference) findPreference(PQ_PICTRUE_ADVANCED_COLOR_CUSTOMIZE);
        final Preference pictureAdvancedMemcPref = (Preference) findPreference(PQ_PICTRUE_ADVANCED_MEMC);

        if (PQ_PICTRUE_ADVANCED_SOURCE_HDR == mPQSettingsManager.GetSourceHdrType() &&
                 (mPQSettingsManager.getChipVersionInfo() != null &&
                  PQ_PICTRUE_T5 == mPQSettingsManager.getChipVersionInfo())) {//Leave blank first, add conditions later
            pictureAdvancedDynamicToneMappingPref.setValueIndex(mPQSettingsManager.getAdvancedDynamicToneMappingStatus());
            pictureAdvancedDynamicToneMappingPref.setOnPreferenceChangeListener(this);
        } else {
            pictureAdvancedDynamicToneMappingPref.setVisible(false);
        }

        if (true) {//Leave blank first, add conditions later
            pictureAdvancedColorManagementPref.setValueIndex(mPQSettingsManager.getAdvancedColorManagementStatus());
            pictureAdvancedColorManagementPref.setOnPreferenceChangeListener(this);
        } else {
            pictureAdvancedColorManagementPref.setVisible(false);
        }

        if (true) {//Leave blank first, add conditions later
            pictureAdvancedColorRangeModePref.setValueIndex(mPQSettingsManager.getHdmiColorRangeStatus());
            pictureAdvancedColorRangeModePref.setOnPreferenceChangeListener(this);
        } else {
            pictureAdvancedColorRangeModePref.setVisible(false);
        }

        if (true) {//Leave blank first, add conditions later
            pictureAdvancedColorSpacePref.setValueIndex(mPQSettingsManager.getAdvancedColorSpaceStatus());
            pictureAdvancedColorSpacePref.setOnPreferenceChangeListener(this);
        } else {
            pictureAdvancedColorSpacePref.setVisible(false);
        }

        if (true) {//Leave blank first, add conditions later
            pictureAdvancedGlobalDimmingPref.setValueIndex(mPQSettingsManager.getAdvancedGlobalDimmingStatus());
            pictureAdvancedGlobalDimmingPref.setOnPreferenceChangeListener(this);
        } else {
            pictureAdvancedGlobalDimmingPref.setVisible(false);
        }

        if (true) {//Leave blank first, add conditions later
            pictureAdvancedLocalDimmingPref.setValueIndex(mPQSettingsManager.getAdvancedLocalDimmingStatus());
            pictureAdvancedLocalDimmingPref.setOnPreferenceChangeListener(this);
        } else {
            pictureAdvancedLocalDimmingPref.setVisible(false);
        }

        if (true) {//Leave blank first, add conditions later
            pictureAdvancedBlackStretchPref.setValueIndex(mPQSettingsManager.getAdvancedBlackStretchStatus());
            pictureAdvancedBlackStretchPref.setOnPreferenceChangeListener(this);
        } else {
            pictureAdvancedBlackStretchPref.setVisible(false);
        }

        if (true) {//Leave blank first, add conditions later
            pictureAdvancedDNLPPref.setValueIndex(mPQSettingsManager.getAdvancedDNLPStatus());
            pictureAdvancedDNLPPref.setOnPreferenceChangeListener(this);
        } else {
            pictureAdvancedDNLPPref.setVisible(false);
        }

        if (true) {//Leave blank first, add conditions later
            pictureAdvancedLocalContrastPref.setValueIndex(mPQSettingsManager.getAdvancedLocalContrastStatus());
            pictureAdvancedLocalContrastPref.setOnPreferenceChangeListener(this);
        } else {
            pictureAdvancedLocalContrastPref.setVisible(false);
        }

        if (true) {//Leave blank first, add conditions later
            pictureAdvancedSRPref.setValueIndex(mPQSettingsManager.getAdvancedSRStatus());
            pictureAdvancedSRPref.setOnPreferenceChangeListener(this);
        } else {
            pictureAdvancedSRPref.setVisible(false);
        }

        if (true) {//Leave blank first, add conditions later
            pictureAdvancedDNRPref.setValueIndex(mPQSettingsManager.getDnrStatus());
            pictureAdvancedDNRPref.setOnPreferenceChangeListener(this);
        } else {
            pictureAdvancedDNRPref.setVisible(false);
        }

        if (true) {//Leave blank first, add conditions later
            pictureAdvancedDeBlockPref.setValueIndex(mPQSettingsManager.getAdvancedDeBlockStatus());
            pictureAdvancedDeBlockPref.setOnPreferenceChangeListener(this);
        } else {
            pictureAdvancedDeBlockPref.setVisible(false);
        }

        if (true) {//Leave blank first, add conditions later
            pictureAdvancedDeMosquitoPref.setValueIndex(mPQSettingsManager.getAdvancedDeMosquitoStatus());
            pictureAdvancedDeMosquitoPref.setOnPreferenceChangeListener(this);
        } else {
            pictureAdvancedDeMosquitoPref.setVisible(false);
        }

        if (mPQSettingsManager.getChipVersionInfo() != null &&
                (PQ_PICTRUE_T5 == mPQSettingsManager.getChipVersionInfo() ||
                 PQ_PICTRUE_T3 == mPQSettingsManager.getChipVersionInfo())) {//Leave blank first, add conditions later
            pictureAdvancedDecontourPref.setValueIndex(mPQSettingsManager.getAdvancedDecontourStatus());
            pictureAdvancedDecontourPref.setOnPreferenceChangeListener(this);
        } else {
            pictureAdvancedDecontourPref.setVisible(false);
        }

        if (true) {//Leave blank first, add conditions later
            pictureAdvancedGammaPref.setVisible(true);
        } else {
            pictureAdvancedGammaPref.setVisible(false);
        }

        if (true) {//Leave blank first, add conditions later
            pictureAdvancedManualGammaPref.setVisible(true);
        } else {
            pictureAdvancedManualGammaPref.setVisible(false);
        }

        if (true) {//Leave blank first, add conditions later
            pictureAdvancedColorTemperaturePref.setVisible(true);
        } else {
            pictureAdvancedColorTemperaturePref.setVisible(false);
        }

        if (true) {//Leave blank first, add conditions later
            pictureAdvancedColorCustomizePref.setVisible(true);
        } else {
            pictureAdvancedColorCustomizePref.setVisible(false);
        }

        if (true) {//Leave blank first, add conditions later
            pictureAdvancedMemcPref.setVisible(true);
        } else {
            pictureAdvancedMemcPref.setVisible(false);
        }
    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        /*switch (preference.getKey()) {
            case PQ_BRIGHTNESS_IN:
                mPQSettingsManager.setBrightness(ZOOMINSTEP);
                break;
        }*/
        return super.onPreferenceTreeClick(preference);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        if (CanDebug()) Log.d(TAG, "[onPreferenceTreeClick] preference.getKey() = " + preference.getKey());
        final int selection = Integer.parseInt((String)newValue);
        switch (preference.getKey()) {
            case PQ_PICTRUE_ADVANCED_DYNAMIC_TONE_MAPPING:
                mPQSettingsManager.setAdvancedDynamicToneMappingStatus(selection);
                break;
            case PQ_PICTRUE_ADVANCED_COLOR_MANAGEMENT:
                mPQSettingsManager.setAdvancedColorManagementStatus(selection);
                break;
            case PQ_HDMI_COLOR_RANGE:
                mPQSettingsManager.setHdmiColorRangeValue(selection);
                break;
            case PQ_PICTRUE_ADVANCED_COLOR_SPACE:
                mPQSettingsManager.setAdvancedColorSpaceStatus(selection);
                break;
            case PQ_PICTRUE_ADVANCED_GLOBAL_DIMMING:
                mPQSettingsManager.setAdvancedGlobalDimmingStatus(selection);
                break;
            case PQ_PICTRUE_ADVANCED_LOCAL_DIMMING:
                mPQSettingsManager.setAdvancedLocalDimmingStatus(selection);
                break;
            case PQ_PICTRUE_ADVANCED_BLACK_STRETCH:
                mPQSettingsManager.setAdvancedBlackStretchStatus(selection);
                break;
            case PQ_PICTRUE_ADVANCED_DNLP:
                mPQSettingsManager.setAdvancedDNLPStatus(selection);
                break;
            case PQ_PICTRUE_ADVANCED_LOCAL_CONTRAST:
                mPQSettingsManager.setAdvancedLocalContrastStatus(selection);
                break;
            case PQ_PICTRUE_ADVANCED_SR:
                mPQSettingsManager.setAdvancedSRStatus(selection);
                break;
            case PQ_DNR:
                mPQSettingsManager.setDnr(selection);
                break;
            case PQ_PICTRUE_ADVANCED_DEBLOCK:
                mPQSettingsManager.setAdvancedDeBlockStatus(selection);
                break;
            case PQ_PICTRUE_ADVANCED_DEMOSQUITO:
                mPQSettingsManager.setAdvancedDeMosquitoStatus(selection);
                break;
            case PQ_PICTRUE_ADVANCED_DECONTOUR:
                mPQSettingsManager.setAdvancedDecontourStatus(selection);
                break;
        }
        return true;
    }

    @Override
    public int getMetricsCategory() {
        return 0;
    }

}
