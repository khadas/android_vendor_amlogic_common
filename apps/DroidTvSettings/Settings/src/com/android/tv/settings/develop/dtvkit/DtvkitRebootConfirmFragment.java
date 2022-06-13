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

import static com.android.tv.settings.overlay.FlavorUtils.FLAVOR_CLASSIC;
import static com.android.tv.settings.overlay.FlavorUtils.FLAVOR_TWO_PANEL;
import static com.android.tv.settings.overlay.FlavorUtils.FLAVOR_VENDOR;
import static com.android.tv.settings.overlay.FlavorUtils.FLAVOR_X;

import android.content.Context;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.PowerManager;
import android.view.View;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.leanback.app.GuidedStepSupportFragment;
import androidx.leanback.widget.GuidanceStylist;
import androidx.leanback.widget.GuidedAction;

import com.android.tv.settings.R;
import com.android.tv.settings.overlay.FlavorUtils;

import java.util.List;

import android.os.SystemProperties;
import android.text.TextUtils;
import com.android.tv.settings.util.DroidUtils;
import com.android.tv.settings.SettingsConstant;
import com.android.tv.settings.R;
import com.droidlogic.app.SystemControlManager;
import android.provider.Settings;
import android.util.Log;

@Keep
public class DtvkitRebootConfirmFragment extends GuidedStepSupportFragment {

    private static final String TAG = "DtvkitRebootConfirmFragment";
    private static final String ARG_SAFE_MODE = "DtvkitRebootConfirmFragment.safe_mode";
    private static final String PROP_DVBC_DVBS_SWITCH     = "persist.vendor.tvconfig.path";
    private static final String PROP_DVBC_PATH     = "/mnt/vendor/odm_ext/etc/tvconfig/dtvkit/config.xml";
    private static final String PROP_DVBS_PATH     = "/mnt/vendor/odm_ext/etc/tvconfig/dtvkit/config_dvbs.xml";

    public static DtvkitRebootConfirmFragment newInstance(boolean safeMode) {

        Bundle args = new Bundle(1);
        args.putBoolean(ARG_SAFE_MODE, safeMode);

        DtvkitRebootConfirmFragment fragment = new DtvkitRebootConfirmFragment();
        fragment.setArguments(args);
        return fragment;
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        setSelectedActionPosition(1);
    }

    @Override
    public @NonNull
    GuidanceStylist.Guidance onCreateGuidance(Bundle savedInstanceState) {
        if (getArguments().getBoolean(ARG_SAFE_MODE, false)) {
            return new GuidanceStylist.Guidance(
                    getString(R.string.reboot_safemode_confirm),
                    getString(R.string.reboot_safemode_desc),
                    null,
                    getActivity().getDrawable(R.drawable.ic_warning_132dp)
            );
        } else {
            return new GuidanceStylist.Guidance(
                    getString(R.string.system_reboot_confirm),
                    null,
                    null,
                    getActivity().getDrawable(R.drawable.ic_warning_132dp)
            );
        }
    }

    @Override
    public void onCreateActions(@NonNull List<GuidedAction> actions,
            Bundle savedInstanceState) {
        final Context context = getActivity();
        if (getArguments().getBoolean(ARG_SAFE_MODE, false)) {
            actions.add(new GuidedAction.Builder(context)
                    .id(GuidedAction.ACTION_ID_OK)
                    .title(R.string.reboot_safemode_action)
                    .build());
        } else {
            actions.add(new GuidedAction.Builder(context)
                    .id(GuidedAction.ACTION_ID_OK)
                    .title(R.string.restart_button_label)
                    .build());
        }
        actions.add(new GuidedAction.Builder(context)
                .clickAction(GuidedAction.ACTION_ID_CANCEL)
                .build());
    }

    @Override
    public GuidanceStylist onCreateGuidanceStylist() {
        return new GuidanceStylist() {
            @Override
            public int onProvideLayoutId() {
                switch (FlavorUtils.getFlavor(getContext())) {
                    case FLAVOR_CLASSIC:
                    case FLAVOR_TWO_PANEL:
                        return R.layout.confirm_guidance;
                    case FLAVOR_X:
                    case FLAVOR_VENDOR:
                        return R.layout.confirm_guidance_x;
                    default:
                        return R.layout.confirm_guidance;
                }
            }
        };
    }

    private boolean getDvbcDvbsSwitchPropEnabled() {
        String strDvbcDvbsSwitchPropEnabled;
        SystemControlManager mSystemControlManager = SystemControlManager.getInstance();
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

    public void setProp(String str) {
        SystemControlManager mSystemControlManager = SystemControlManager.getInstance();
        //Settings.Global.putInt(getContext().getContentResolver(), Settings.Global.DVBC_DVBS_SWITCH_ENABLED, 1);
        mSystemControlManager.setProperty(PROP_DVBC_DVBS_SWITCH, str);
    }

    @Override
    public void onGuidedActionClicked(GuidedAction action) {
        boolean mDvbcDvbsSwitchPropEnabled = getDvbcDvbsSwitchPropEnabled();
        if (action.getId() == GuidedAction.ACTION_ID_OK) {
            final boolean toSafeMode = getArguments().getBoolean(ARG_SAFE_MODE, false);
            final PowerManager pm =
                    (PowerManager) getActivity().getSystemService(Context.POWER_SERVICE);

            new AsyncTask<Void, Void, Void>() {
                @Override
                protected Void doInBackground(Void... params) {
                    if (toSafeMode) {
                        pm.rebootSafeMode();
                    } else {
                        pm.reboot(null);
                    }
                    return null;
                }
            }.execute();

            if (mDvbcDvbsSwitchPropEnabled == false)
                setProp(PROP_DVBS_PATH);
            else
                setProp(PROP_DVBC_PATH);
        } else {
            getFragmentManager().popBackStack();
            if (mDvbcDvbsSwitchPropEnabled == false)
                setProp(PROP_DVBC_PATH);
            else
                setProp(PROP_DVBS_PATH);
        }
    }
}
