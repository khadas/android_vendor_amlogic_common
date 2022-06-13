/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC PQAdvancedGammaFragment
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

public class PQAdvancedGammaFragment extends SettingsPreferenceFragment implements Preference.OnPreferenceChangeListener {
    private static final String TAG = "PQAdvancedGammaFragment";

    private static final int PQ_PICTRUE_ADVANCED_GAMMA_STEP= 1;
    private static final String PQ_PICTRUE_ADVANCED_GAMMA= "pq_pictrue_advanced_gamma";

    private SeekBarPreference PQPictureAdvancedGammaPref;


    private PQSettingsManager mPQSettingsManager;


    public static PQAdvancedGammaFragment newInstance() {
        return new PQAdvancedGammaFragment();
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
        setPreferencesFromResource(R.xml.pq_pictrue_advanced_gamma, null);
        if (mPQSettingsManager == null) {
            mPQSettingsManager = new PQSettingsManager(getActivity());
        }

        PQPictureAdvancedGammaPref = (SeekBarPreference) findPreference(PQ_PICTRUE_ADVANCED_GAMMA);

        if (true) {//Leave blank first, add conditions later
            PQPictureAdvancedGammaPref.setOnPreferenceChangeListener(this);
            PQPictureAdvancedGammaPref.setSeekBarIncrement(PQ_PICTRUE_ADVANCED_GAMMA_STEP);
            PQPictureAdvancedGammaPref.setMin(-5);
            PQPictureAdvancedGammaPref.setMax(5);
            PQPictureAdvancedGammaPref.setValue(mPQSettingsManager.getAdvancedGammaStatus());
            PQPictureAdvancedGammaPref.setVisible(true);
        } else {
            PQPictureAdvancedGammaPref.setVisible(false);
        }


    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        return super.onPreferenceTreeClick(preference);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        Log.d(TAG, "[onPreferenceTreeClick] preference.getKey() = " + preference.getKey()+" newValue:"+newValue);
        switch (preference.getKey()) {
            case PQ_PICTRUE_ADVANCED_GAMMA:
                mPQSettingsManager.setAdvancedGammaStatus((int)newValue);
                break;

        }
        return true;
    }

    @Override
    public int getMetricsCategory() {
        return 0;
    }

}
