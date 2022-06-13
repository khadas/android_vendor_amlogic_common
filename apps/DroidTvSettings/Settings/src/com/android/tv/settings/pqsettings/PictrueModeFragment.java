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

package com.android.tv.settings.pqsettings;

import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;

import android.os.Bundle;
import android.os.SystemProperties;
import android.provider.Settings;
import androidx.preference.SwitchPreference;
import com.android.tv.settings.SettingsPreferenceFragment;
import androidx.preference.ListPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceScreen;
import androidx.preference.TwoStatePreference;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.widget.Toast;
import android.media.tv.TvInputInfo;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.view.LayoutInflater;
import android.view.ViewGroup;


import com.android.tv.settings.SettingsConstant;
import com.android.tv.settings.MainFragment;
import com.android.tv.settings.R;
import com.android.tv.settings.SettingsConstant;

import com.droidlogic.app.OutputModeManager;
import com.droidlogic.app.tv.DroidLogicTvUtils;
import com.droidlogic.app.tv.TvControlManager;

import java.util.ArrayList;
import java.util.List;

public class PictrueModeFragment extends SettingsPreferenceFragment implements Preference.OnPreferenceChangeListener {

    private static final String TAG = "PictrueModeFragment";
    private static final String PQ_PICTRUE_MODE = "pq_pictrue_mode";
    private static final String PQ_PICTRUE_MODE_SDR = "pq_pictrue_mode_sdr";
    private static final String PQ_PICTRUE_MODE_HDR10= "pq_pictrue_mode_hdr10";
    private static final String PQ_PICTRUE_MODE_HDR10PLUS= "pq_pictrue_mode_hdr10plus";
    private static final String PQ_PICTRUE_MODE_HLG = "pq_pictrue_mode_hlg";
    private static final String PQ_PICTRUE_MODE_DOLBYVISION = "pq_pictrue_mode_dolbyvision";
    private static final String PQ_PICTRUE_MODE_CVUA = "pq_pictrue_mode_cvua";

    private static String FLAG_CURRENT_SOURCE = "SDR";
    private boolean FLAG_PQ_PICTRUE_MODE = false;
    private boolean FLAG_PQ_PICTRUE_MODE_SDR = false;
    private boolean FLAG_PQ_PICTRUE_MODE_HDR10= false;
    private boolean FLAG_PQ_PICTRUE_MODE_HDR10PLUS= false;
    private boolean FLAG_PQ_PICTRUE_MODE_HLG = false;
    private boolean FLAG_PQ_PICTRUE_MODE_DOLBYVISION = false;
    private boolean FLAG_PQ_PICTRUE_MODE_CVUA = false;

    private static final String PQ_CUSTOM = "pq_custom";
    private static final String PQ_AI_PQ = "pq_ai_pq";
    private static final String PQ_AI_SR = "pq_ai_sr_enable";
    private static final String PQ_ASPECT_RATIO = "pq_aspect_ratio";
    private static final String PQ_BACKLIGHT = "pq_backlight";
    private static final String PQ_ADVANCED = "pq_advanced";
    private static final String PQ_ALLRESET = "pq_allreset";

    private static final String CURRENT_DEVICE_ID = "current_device_id";
    private static final String TV_CURRENT_DEVICE_ID = "tv_current_device_id";
    private static final String DTVKIT_PACKAGE = "org.dtvkit.inputsource";

    private static final String PQ_PICTRUE_T5 = "T963";

    private SwitchPreference mAisrSwith;

    private PQSettingsManager mPQSettingsManager;

    public static PictrueModeFragment newInstance() {
        return new PictrueModeFragment();
    }

    private boolean CanDebug() {
        return PQSettingsManager.CanDebug();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (mPQSettingsManager == null) {
            mPQSettingsManager = new PQSettingsManager(getActivity());
        }
        getCurrentSource();
        //Use the interface to get the current source
    }

    @Override
    public void onResume() {
        super.onResume();
        //Use the interface to get the current source
        final ListPreference picturemodePref = (ListPreference) findPreference(PQ_PICTRUE_MODE);
        if (mPQSettingsManager == null) {
            mPQSettingsManager = new PQSettingsManager(getActivity());
        }
        getCurrentSource();

        if (mPQSettingsManager.isHdmiSource()) {
            picturemodePref.setEntries(setHdmiPicEntries());
            picturemodePref.setEntryValues(setHdmiPicEntryValues());
        }

        picturemodePref.setValue(mPQSettingsManager.getPictureModeStatus());

        int is_from_live_tv = getActivity().getIntent().getIntExtra("from_live_tv", 0);
        String currentInputInfoId = getActivity().getIntent().getStringExtra("current_tvinputinfo_id");
        boolean isTv = SettingsConstant.needDroidlogicTvFeature(getActivity());
        boolean hasMboxFeature = SettingsConstant.hasMboxFeature(getActivity());
        String curPictureMode = mPQSettingsManager.getPictureModeStatus();
        final Preference backlightPref = (Preference) findPreference(PQ_BACKLIGHT);
        if ((isTv && getActivity().getResources().getBoolean(R.bool.tv_pq_need_backlight)) ||
                (!isTv && getActivity().getResources().getBoolean(R.bool.box_pq_need_backlight)) ||
                (isTv && isDtvKitInput(currentInputInfoId))) {
            backlightPref.setSummary(mPQSettingsManager.getBacklightStatus() + "%");
        } else {
            backlightPref.setVisible(false);
        }

        final Preference pictureCustomerPref = (Preference) findPreference(PQ_CUSTOM);
        if (curPictureMode.equals(PQSettingsManager.STATUS_MONITOR) ||
            curPictureMode.equals(PQSettingsManager.STATUS_GAME)) {
            pictureCustomerPref.setVisible(false);
        } else {
            pictureCustomerPref.setVisible(true);
        }
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        Log.d(TAG, "onCreateView");
        final View innerView = super.onCreateView(inflater, container, savedInstanceState);
        if (getActivity().getIntent().getIntExtra("from_live_tv", 0) == 1) {
            //MainFragment.changeToLiveTvStyle(innerView, getActivity());
        }
        return innerView;
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        Log.d(TAG, "onCreatePreferences");
        setPreferencesFromResource(R.xml.pq_pictrue_mode, null);
        if (mPQSettingsManager == null) {
            mPQSettingsManager = new PQSettingsManager(getActivity());
        }
        getCurrentSource();
        int is_from_live_tv = getActivity().getIntent().getIntExtra("from_live_tv", 0);
        boolean isTv = SettingsConstant.needDroidlogicTvFeature(getActivity());
        boolean hasMboxFeature = SettingsConstant.hasMboxFeature(getActivity());
        String curPictureMode = mPQSettingsManager.getPictureModeStatus();
        final ListPreference picturemodePref = (ListPreference) findPreference(PQ_PICTRUE_MODE);
        final ListPreference picturemodesdrPref = (ListPreference) findPreference(PQ_PICTRUE_MODE_SDR);
        final ListPreference picturemodehdr10Pref = (ListPreference) findPreference(PQ_PICTRUE_MODE_HDR10);
        final ListPreference picturemodehdr10plusPref = (ListPreference) findPreference(PQ_PICTRUE_MODE_HDR10PLUS);
        final ListPreference picturemodehlgPref = (ListPreference) findPreference(PQ_PICTRUE_MODE_HLG);
        final ListPreference picturemodedolbyvisionPref = (ListPreference) findPreference(PQ_PICTRUE_MODE_DOLBYVISION);
        final ListPreference picturemodecvuaPref = (ListPreference) findPreference(PQ_PICTRUE_MODE_CVUA);

        if (mPQSettingsManager.isHdmiSource()) {
            picturemodePref.setEntries(setHdmiPicEntries());
            picturemodePref.setEntryValues(setHdmiPicEntryValues());
        }

        Log.d(TAG, "curPictureMode: " + curPictureMode + "isTv: " + isTv + "isLiveTv: " + is_from_live_tv);

        if ((FLAG_PQ_PICTRUE_MODE && isTv && getActivity().getResources().getBoolean(R.bool.tv_pq_need_pictrue_mode)) ||
                (FLAG_PQ_PICTRUE_MODE && !isTv && getActivity().getResources().getBoolean(R.bool.box_pq_need_pictrue_mode))) {
            picturemodePref.setValue(curPictureMode);
            picturemodePref.setOnPreferenceChangeListener(this);
        } else {
            picturemodePref.setVisible(false);
        }

        if ((FLAG_PQ_PICTRUE_MODE_SDR && isTv && getActivity().getResources().getBoolean(R.bool.tv_pq_need_pictrue_mode)) ||
                (FLAG_PQ_PICTRUE_MODE_SDR && !isTv && getActivity().getResources().getBoolean(R.bool.box_pq_need_pictrue_mode))) {
            picturemodesdrPref.setVisible(true);
        } else {
            picturemodesdrPref.setVisible(false);
        }

        if ((FLAG_PQ_PICTRUE_MODE_HDR10 && isTv && getActivity().getResources().getBoolean(R.bool.tv_pq_need_pictrue_mode)) ||
                (FLAG_PQ_PICTRUE_MODE_HDR10 && !isTv && getActivity().getResources().getBoolean(R.bool.box_pq_need_pictrue_mode))) {
            picturemodehdr10Pref.setVisible(true);
        } else {
            picturemodehdr10Pref.setVisible(false);
        }

        if ((FLAG_PQ_PICTRUE_MODE_HDR10PLUS && isTv && getActivity().getResources().getBoolean(R.bool.tv_pq_need_pictrue_mode)) ||
                (FLAG_PQ_PICTRUE_MODE_HDR10PLUS && !isTv && getActivity().getResources().getBoolean(R.bool.box_pq_need_pictrue_mode))) {
            picturemodehdr10plusPref.setVisible(true);
        } else {
            picturemodehdr10plusPref.setVisible(false);
        }

        if ((FLAG_PQ_PICTRUE_MODE_HLG && isTv && getActivity().getResources().getBoolean(R.bool.tv_pq_need_pictrue_mode)) ||
                (FLAG_PQ_PICTRUE_MODE_HLG && !isTv && getActivity().getResources().getBoolean(R.bool.box_pq_need_pictrue_mode))) {
            picturemodehlgPref.setVisible(true);
        } else {
            picturemodehlgPref.setVisible(false);
        }

        if ((FLAG_PQ_PICTRUE_MODE_DOLBYVISION && isTv && getActivity().getResources().getBoolean(R.bool.tv_pq_need_pictrue_mode)) ||
                (FLAG_PQ_PICTRUE_MODE_DOLBYVISION && !isTv && getActivity().getResources().getBoolean(R.bool.box_pq_need_pictrue_mode))) {
            picturemodedolbyvisionPref.setVisible(true);
        } else {
            picturemodedolbyvisionPref.setVisible(false);
        }

        if ((FLAG_PQ_PICTRUE_MODE_CVUA && isTv && getActivity().getResources().getBoolean(R.bool.tv_pq_need_pictrue_mode)) ||
                (FLAG_PQ_PICTRUE_MODE_CVUA && !isTv && getActivity().getResources().getBoolean(R.bool.box_pq_need_pictrue_mode))) {
            picturemodecvuaPref.setVisible(true);
        } else {
            picturemodecvuaPref.setVisible(false);
        }

        final Preference pictureCustomerPref = (Preference) findPreference(PQ_CUSTOM);
        if (curPictureMode.equals(PQSettingsManager.STATUS_MONITOR) ||
            curPictureMode.equals(PQSettingsManager.STATUS_GAME)) {
            pictureCustomerPref.setVisible(false);
        } else {
            pictureCustomerPref.setVisible(true);
        }

        final ListPreference aspectratioPref = (ListPreference) findPreference(PQ_ASPECT_RATIO);
        if (is_from_live_tv == 1 || (isTv && getActivity().getResources().getBoolean(R.bool.tv_pq_need_aspect_ratio)) ||
                    (!isTv && getActivity().getResources().getBoolean(R.bool.box_pq_need_aspect_ratio))) {
            aspectratioPref.setValueIndex(mPQSettingsManager.getAspectRatioStatus());
            aspectratioPref.setOnPreferenceChangeListener(this);
        } else {
            aspectratioPref.setVisible(false);
        }

        final ListPreference aipqPref = (ListPreference) findPreference(PQ_AI_PQ);
        // First judge like this, and add specific judgment criteria later
        if (mPQSettingsManager.getChipVersionInfo() != null &&
                    PQ_PICTRUE_T5 == mPQSettingsManager.getChipVersionInfo()) {
            aipqPref.setValueIndex(mPQSettingsManager.getAIPQStatus());
            //aipqPref.setValueIndex(1);
            aipqPref.setOnPreferenceChangeListener(this);
        } else {
            aipqPref.setVisible(false);
        }

        mAisrSwith = findPreference(PQ_AI_SR);
        // First judge like this, and add specific judgment criteria later
        //if (mPQSettingsManager.hasAisrFunc()) {
        //    mAisrSwith.setEnabled(true);
        //    mAisrSwith.setChecked(mPQSettingsManager.getAisr());
        //    mAisrSwith.setOnPreferenceChangeListener(this);
        //} else {
            mAisrSwith.setVisible(false);
        //}

        final Preference backlightPref = (Preference) findPreference(PQ_BACKLIGHT);
        if ((isTv && getActivity().getResources().getBoolean(R.bool.tv_pq_need_backlight)) ||
                (!isTv && getActivity().getResources().getBoolean(R.bool.box_pq_need_backlight))) {
            backlightPref.setSummary(mPQSettingsManager.getBacklightStatus() + "%");
        } else {
            backlightPref.setVisible(false);
        }

        final Preference pictureAllResetPref = (Preference) findPreference(PQ_ALLRESET);
        if (curPictureMode.equals(PQSettingsManager.STATUS_MONITOR) ||
            curPictureMode.equals(PQSettingsManager.STATUS_GAME)) {
            pictureAllResetPref.setVisible(false);
        } else {
            pictureAllResetPref.setVisible(true);
        }
    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        if (CanDebug()) Log.d(TAG, "[onPreferenceTreeClick] preference.getKey() = " + preference.getKey());
        switch (preference.getKey()) {
            case PQ_ALLRESET:
                Intent PQAllResetIntent = new Intent();
                PQAllResetIntent.setClassName(
                        "com.android.tv.settings",
                        "com.android.tv.settings.pqsettings.PQResetAllActivity");
                startActivity(PQAllResetIntent);
                break;
        }
        return super.onPreferenceTreeClick(preference);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        Log.d(TAG, "[onPreferenceChange] preference.getKey() = " + preference.getKey() + ", newValue = " + newValue);
        //final int selection = Integer.parseInt((String)newValue);
        if (TextUtils.equals(preference.getKey(), PQ_PICTRUE_MODE)) {
            mPQSettingsManager.setPictureMode((String)newValue);
        } else if (TextUtils.equals(preference.getKey(), PQ_PICTRUE_MODE_SDR)) {
            //mPQSettingsManager.setPictureMode((String)newValue);
        } else if (TextUtils.equals(preference.getKey(), PQ_PICTRUE_MODE_HDR10)) {
            //mPQSettingsManager.setPictureMode((String)newValue);
        } else if (TextUtils.equals(preference.getKey(), PQ_PICTRUE_MODE_HDR10PLUS)) {
            //mPQSettingsManager.setPictureMode((String)newValue);
        } else if (TextUtils.equals(preference.getKey(), PQ_PICTRUE_MODE_HLG)) {
            //mPQSettingsManager.setPictureMode((String)newValue);
        } else if (TextUtils.equals(preference.getKey(), PQ_PICTRUE_MODE_DOLBYVISION)) {
            //mPQSettingsManager.setPictureMode((String)newValue);
        } else if (TextUtils.equals(preference.getKey(), PQ_PICTRUE_MODE_CVUA)) {
            //mPQSettingsManager.setPictureMode((String)newValue);
        } else if (TextUtils.equals(preference.getKey(), PQ_ASPECT_RATIO)) {
            final int selection = Integer.parseInt((String)newValue);
            mPQSettingsManager.setAspectRatio(selection);
        } else if (TextUtils.equals(preference.getKey(), PQ_AI_PQ)) {
            final int selection = Integer.parseInt((String)newValue);
            mPQSettingsManager.setAIPQ(selection);
        } else if (TextUtils.equals(preference.getKey(), PQ_AI_SR)) {
            mPQSettingsManager.setAisr((boolean) newValue);
        }
        return true;
    }

    @Override
    public int getMetricsCategory() {
        return 0;
    }

    private final int[] HDMI_PIC_RES = {R.string.pq_standard, R.string.pq_vivid, R.string.pq_soft, R.string.pq_sport, R.string.pq_movie, R.string.pq_monitor,R.string.pq_game,R.string.pq_user};
    private final String[] HDMI_PIC_MODE = {PQSettingsManager.STATUS_STANDARD, PQSettingsManager.STATUS_VIVID, PQSettingsManager.STATUS_SOFT,
        PQSettingsManager.STATUS_SPORT, PQSettingsManager.STATUS_MOVIE, PQSettingsManager.STATUS_MONITOR, PQSettingsManager.STATUS_GAME, PQSettingsManager.STATUS_USER};

    private String[] setHdmiPicEntries() {
        String[] temp = null;//new String[HDMI_PIC_RES.length];
        List<String> list = new ArrayList<String>();
        if (mPQSettingsManager == null) {
            mPQSettingsManager = new PQSettingsManager(getActivity());
        }
        if (mPQSettingsManager.isHdmiSource()) {
            for (int i = 0; i < HDMI_PIC_RES.length; i++) {
                list.add(getString(HDMI_PIC_RES[i]));
            }
        }
        temp = (String[])list.toArray(new String[list.size()]);

        return temp;
    }

    private String[] setHdmiPicEntryValues() {
        String[] temp = null;//new String[HDMI_PIC_MODE.length];
        List<String> list = new ArrayList<String>();
        if (mPQSettingsManager == null) {
            mPQSettingsManager = new PQSettingsManager(getActivity());
        }
        if (mPQSettingsManager.isHdmiSource()) {
            for (int i = 0; i < HDMI_PIC_MODE.length; i++) {
                list.add(HDMI_PIC_MODE[i]);
            }
        }
        temp = (String[])list.toArray(new String[list.size()]);

        return temp;
    }

    private static boolean isDtvKitInput(String inputId) {
        boolean result = false;
        if (inputId != null && inputId.startsWith(DTVKIT_PACKAGE)) {
            result = true;
        }
        Log.d(TAG, "isDtvKitInput result = " + result);
        return result;
    }

    public enum Current_Source_Type {
        PQ_PICTRUE_MODE(0),
        PQ_PICTRUE_MODE_HDR10(1),
        PQ_PICTRUE_MODE_HDR10PLUS(2),
        PQ_PICTRUE_MODE_DOLBYVISION(3),
        PQ_PICTRUE_MODE_HLG(5),
        PQ_PICTRUE_MODE_SDR(6),
        PQ_PICTRUE_MODE_CVUA(10);

        private int val;

        Current_Source_Type(int val) {
            this.val = val;
        }

        public int toInt() {
            return this.val;
        }
    }

    private void getCurrentSource() {
        ////Use the interface to get the current source
        int currentPictureModeSource = 0; //mPQSettingsManager.getPictureModeSource();
        if (currentPictureModeSource == Current_Source_Type.PQ_PICTRUE_MODE_SDR.toInt()) {
            FLAG_PQ_PICTRUE_MODE_SDR = true;
            FLAG_PQ_PICTRUE_MODE_HDR10= false;
            FLAG_PQ_PICTRUE_MODE_HDR10PLUS= false;
            FLAG_PQ_PICTRUE_MODE_HLG = false;
            FLAG_PQ_PICTRUE_MODE_DOLBYVISION = false;
            FLAG_PQ_PICTRUE_MODE_CVUA = false;
        } else if (currentPictureModeSource == Current_Source_Type.PQ_PICTRUE_MODE_HDR10.toInt()) {
            FLAG_PQ_PICTRUE_MODE_SDR = false;
            FLAG_PQ_PICTRUE_MODE_HDR10= true;
            FLAG_PQ_PICTRUE_MODE_HDR10PLUS= false;
            FLAG_PQ_PICTRUE_MODE_HLG = false;
            FLAG_PQ_PICTRUE_MODE_DOLBYVISION = false;
            FLAG_PQ_PICTRUE_MODE_CVUA = false;
        } else if (currentPictureModeSource == Current_Source_Type.PQ_PICTRUE_MODE_HDR10PLUS.toInt()) {
            FLAG_PQ_PICTRUE_MODE_SDR = false;
            FLAG_PQ_PICTRUE_MODE_HDR10= false;
            FLAG_PQ_PICTRUE_MODE_HDR10PLUS= true;
            FLAG_PQ_PICTRUE_MODE_HLG = false;
            FLAG_PQ_PICTRUE_MODE_DOLBYVISION = false;
            FLAG_PQ_PICTRUE_MODE_CVUA = false;
        } else if (currentPictureModeSource == Current_Source_Type.PQ_PICTRUE_MODE_HLG.toInt()) {
            FLAG_PQ_PICTRUE_MODE_SDR = false;
            FLAG_PQ_PICTRUE_MODE_HDR10= false;
            FLAG_PQ_PICTRUE_MODE_HDR10PLUS= false;
            FLAG_PQ_PICTRUE_MODE_HLG = true;
            FLAG_PQ_PICTRUE_MODE_DOLBYVISION = false;
            FLAG_PQ_PICTRUE_MODE_CVUA = false;
        } else if (currentPictureModeSource == Current_Source_Type.PQ_PICTRUE_MODE_DOLBYVISION.toInt()) {
            FLAG_PQ_PICTRUE_MODE_SDR = false;
            FLAG_PQ_PICTRUE_MODE_HDR10= false;
            FLAG_PQ_PICTRUE_MODE_HDR10PLUS= false;
            FLAG_PQ_PICTRUE_MODE_HLG = false;
            FLAG_PQ_PICTRUE_MODE_DOLBYVISION = true;
            FLAG_PQ_PICTRUE_MODE_CVUA = false;
        } else if (currentPictureModeSource == Current_Source_Type.PQ_PICTRUE_MODE_CVUA.toInt()) {
            FLAG_PQ_PICTRUE_MODE_SDR = false;
            FLAG_PQ_PICTRUE_MODE_HDR10= false;
            FLAG_PQ_PICTRUE_MODE_HDR10PLUS= false;
            FLAG_PQ_PICTRUE_MODE_HLG = false;
            FLAG_PQ_PICTRUE_MODE_DOLBYVISION = false;
            FLAG_PQ_PICTRUE_MODE_CVUA = true;
        } else {
            FLAG_PQ_PICTRUE_MODE = true;
            FLAG_PQ_PICTRUE_MODE_SDR = false;
            FLAG_PQ_PICTRUE_MODE_HDR10= false;
            FLAG_PQ_PICTRUE_MODE_HDR10PLUS= false;
            FLAG_PQ_PICTRUE_MODE_HLG = false;
            FLAG_PQ_PICTRUE_MODE_DOLBYVISION = false;
            FLAG_PQ_PICTRUE_MODE_CVUA = false;
        }
    }

}
