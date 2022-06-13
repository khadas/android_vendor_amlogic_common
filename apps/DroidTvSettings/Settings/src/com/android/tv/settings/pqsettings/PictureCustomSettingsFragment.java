/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC PictureCustomSettingsFragment
 */



package com.android.tv.settings.pqsettings;

import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.os.RemoteException;
import android.provider.Settings;
import androidx.preference.SwitchPreference;
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

import java.util.List;
import java.util.Map;
import java.util.ArrayList;

public class PictureCustomSettingsFragment extends SettingsPreferenceFragment {
    private static final String TAG = "PictureCustomSettingsFragment";

    private final static int ZOOMINSTEP = 1;
    private final static int ZOOMOUTSTEP = -1;

    //private final static int MAXBRIGHTNESSHEIGHT = 100;
    //private final static int MINBRIGHTNESSHEIGHT = 0;
    private static final String PQ_BRIGHTNESS = "pq_brightness";
    private static final String PQ_BRIGHTNESS_IN = "pq_brightness_in";
    private static final String PQ_BRIGHTNESS_OUT = "pq_brightness_out";

    //private final static int MAXCONTRASTHEIGHT = 100;
    //private final static int MINCONTRASTHEIGHT = 0;
    private static final String PQ_CONTRAST="pq_contrast";
    private static final String PQ_CONTRAST_IN = "pq_contrast_in";
    private static final String PQ_CONTRAST_OUT = "pq_contrast_out";

    //private final static int MAXSATURATIONHEIGHT = 100;
    //private final static int MINSATURATIONHEIGHT = 0;
    private static final String PQ_SATURATION="pq_saturation";
    private static final String PQ_SATURATION_IN = "pq_saturation_in";
    private static final String PQ_SATURATION_OUT = "pq_saturation_out";

    private static final String PQ_SHARPNESS="pq_sharpness";
    private static final String PQ_SHARPNESS_IN = "pq_sharpness_in";
    private static final String PQ_SHARPNESS_OUT = "pq_sharpness_out";

    private static final String PQ_HUE="pq_hue";
    private static final String PQ_HUE_IN = "pq_hue_in";
    private static final String PQ_HUE_OUT = "pq_hue_out";

    private PQSettingsManager mPQSettingsManager;

    private Preference pq_brightnessPref;
    private PreferenceCategory mPQBrightnessPref;
    private Preference pq_brightnessinPref;
    private Preference pq_brightnessoutPref;

    private Preference pq_contrastPref;
    private PreferenceCategory mPQContrastPref;
    private Preference pq_contrastinPref;
    private Preference pq_contrastoutPref;

    private Preference pq_saturationPref;
    private PreferenceCategory mPQSaturationPref;
    private Preference pq_saturationinPref;
    private Preference pq_saturationoutPref;

    private Preference pq_sharpnessPref;
    private PreferenceCategory mPQSharpnessPref;
    private Preference pq_sharpnessinPref;
    private Preference pq_sharpnessoutPref;

    private Preference pq_huePref;
    private PreferenceCategory mPQHuePref;
    private Preference pq_hueinPref;
    private Preference pq_hueoutPref;


    public static PictureCustomSettingsFragment newInstance() {
        return new PictureCustomSettingsFragment();
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        boolean isTv = SettingsConstant.needDroidlogicTvFeature(getActivity());
        setPreferencesFromResource(R.xml.picture_custom_settings, null);
        mPQSettingsManager = new PQSettingsManager((Context)getActivity());

        mPQBrightnessPref       = (PreferenceCategory) findPreference(PQ_BRIGHTNESS);
        mPQContrastPref       = (PreferenceCategory) findPreference(PQ_CONTRAST);
        mPQSaturationPref       = (PreferenceCategory) findPreference(PQ_SATURATION);
        mPQSharpnessPref       = (PreferenceCategory) findPreference(PQ_SHARPNESS);
        mPQHuePref       = (PreferenceCategory) findPreference(PQ_HUE);
        if ((isTv && getActivity().getResources().getBoolean(R.bool.tv_pq_need_brightness)) ||
                (!isTv && getActivity().getResources().getBoolean(R.bool.box_pq_need_brightness))) {
            pq_brightnessinPref  = (Preference) findPreference(PQ_BRIGHTNESS_IN);
            pq_brightnessoutPref = (Preference) findPreference(PQ_BRIGHTNESS_OUT);
        }else{
            getPreferenceScreen().removePreference(mPQBrightnessPref);
        }

        if ((isTv && getActivity().getResources().getBoolean(R.bool.tv_pq_need_contrast)) ||
                (!isTv && getActivity().getResources().getBoolean(R.bool.box_pq_need_contrast))) {
            pq_contrastinPref  = (Preference) findPreference(PQ_CONTRAST_IN);
            pq_contrastoutPref = (Preference) findPreference(PQ_CONTRAST_OUT);
        }else{
            getPreferenceScreen().removePreference(mPQContrastPref);
        }

        if ((isTv && getActivity().getResources().getBoolean(R.bool.tv_pq_need_saturation)) ||
                (!isTv && getActivity().getResources().getBoolean(R.bool.box_pq_need_saturation))) {
            pq_saturationinPref  = (Preference) findPreference(PQ_SATURATION_IN);
            pq_saturationoutPref = (Preference) findPreference(PQ_SATURATION_OUT);
        }else{
            getPreferenceScreen().removePreference(mPQSaturationPref);
        }

        if ((isTv && getActivity().getResources().getBoolean(R.bool.tv_pq_need_sharpness)) ||
                (!isTv && getActivity().getResources().getBoolean(R.bool.box_pq_need_sharpness))) {
            pq_sharpnessinPref  = (Preference) findPreference(PQ_SHARPNESS_IN);
            pq_sharpnessoutPref = (Preference) findPreference(PQ_SHARPNESS_OUT);
        }else{
            getPreferenceScreen().removePreference(mPQSharpnessPref);
        }

        if ((isTv && getActivity().getResources().getBoolean(R.bool.tv_pq_need_hue)) ||
                (!isTv && getActivity().getResources().getBoolean(R.bool.box_pq_need_hue))) {
            pq_hueinPref  = (Preference) findPreference(PQ_HUE_IN);
            pq_hueoutPref = (Preference) findPreference(PQ_HUE_OUT);
        }else{
            getPreferenceScreen().removePreference(mPQHuePref);
        }

        updateMainScreen();
    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        switch (preference.getKey()) {
            case PQ_BRIGHTNESS_IN:
                mPQSettingsManager.setBrightness(ZOOMINSTEP);
                break;
            case PQ_BRIGHTNESS_OUT:
                mPQSettingsManager.setBrightness(ZOOMOUTSTEP);
                break;
            case PQ_CONTRAST_IN:
                mPQSettingsManager.setContrast(ZOOMINSTEP);
                break;
            case PQ_CONTRAST_OUT:
                mPQSettingsManager.setContrast(ZOOMOUTSTEP);
                break;
            case PQ_SATURATION_IN:
                mPQSettingsManager.setColor(ZOOMINSTEP);
                break;
            case PQ_SATURATION_OUT:
                mPQSettingsManager.setColor(ZOOMOUTSTEP);
                break;
            case PQ_SHARPNESS_IN:
                mPQSettingsManager.setSharpness(ZOOMINSTEP);
                break;
            case PQ_SHARPNESS_OUT:
                mPQSettingsManager.setSharpness(ZOOMOUTSTEP);
                break;
            case PQ_HUE_IN:
                mPQSettingsManager.setTone(ZOOMINSTEP);
                break;
            case PQ_HUE_OUT:
                mPQSettingsManager.setTone(ZOOMOUTSTEP);
                break;
        }
        updateMainScreen();
        return true;
    }

    @Override
    public int getMetricsCategory() {
        return 0;
    }

    private void updateMainScreen() {
        int pq_brightness_percent = mPQSettingsManager.getBrightnessStatus();
        mPQBrightnessPref.setTitle(getActivity().getResources().getString(R.string.pq_brightness) + ": " + pq_brightness_percent +"%");

        int pq_contrast_percent = mPQSettingsManager.getContrastStatus();
        mPQContrastPref.setTitle(getActivity().getResources().getString(R.string.pq_contrast ) + ": " + pq_contrast_percent +"%");

        int pq_saturation_percent = mPQSettingsManager.getColorStatus();
        mPQSaturationPref.setTitle(getActivity().getResources().getString(R.string.pq_saturation ) +": "+ pq_saturation_percent +"%");

        int pq_sharpness_percent = mPQSettingsManager.getSharpnessStatus();
        mPQSharpnessPref.setTitle(getActivity().getResources().getString(R.string.pq_sharpness ) +": "+ pq_sharpness_percent +"%");

        int pq_hue_percent = mPQSettingsManager.getToneStatus();
        mPQHuePref.setTitle(getActivity().getResources().getString(R.string.pq_hue ) +": "+ pq_hue_percent +"%");
    }

}
