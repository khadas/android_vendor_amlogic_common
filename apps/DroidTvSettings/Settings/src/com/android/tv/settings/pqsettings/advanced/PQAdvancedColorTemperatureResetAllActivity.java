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

package com.android.tv.settings.pqsettings.advanced;

import android.annotation.Nullable;
import android.app.Activity;
import android.app.ActivityManager;
import android.content.Context;
import android.content.Intent;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.UserHandle;
import android.service.oemlock.OemLockManager;
import android.service.persistentdata.PersistentDataBlockManager;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.leanback.app.GuidedStepFragment;
import androidx.leanback.widget.GuidanceStylist;
import androidx.leanback.widget.GuidedAction;

import com.android.tv.settings.pqsettings.PQSettingsManager;

import com.android.tv.settings.R;

import java.util.List;

public class PQAdvancedColorTemperatureResetAllActivity extends Activity {

    private static final String TAG = "PQAdvancedColorTemperatureResetAllActivity";

    /**
     * Support for shutdown-after-reset. If our launch intent has a true value for
     * the boolean extra under the following key, then include it in the intent we
     * use to trigger a factory reset. This will cause us to shut down instead of
     * restart after the reset.
     */
    private static final String SHUTDOWN_INTENT_EXTRA = "shutdown";

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (savedInstanceState == null) {
            GuidedStepFragment.addAsRoot(this, PQAdvancedColorTemperatureFragmentResetAllFragment.newInstance(), android.R.id.content);
        }
    }

    public static class PQAdvancedColorTemperatureFragmentResetAllFragment extends GuidedStepFragment {

        public static PQAdvancedColorTemperatureFragmentResetAllFragment newInstance() {

            Bundle args = new Bundle();

            PQAdvancedColorTemperatureFragmentResetAllFragment fragment = new PQAdvancedColorTemperatureFragmentResetAllFragment();
            fragment.setArguments(args);
            return fragment;
        }

        @NonNull
        @Override
        public GuidanceStylist.Guidance onCreateGuidance(Bundle savedInstanceState) {
            return new GuidanceStylist.Guidance(
                    getString(R.string.pq_advanced_color_temperature_allreset_title),
                    getString(R.string.pq_advanced_color_temperature_allreset_description),
                    null,
                    getContext().getDrawable(R.drawable.ic_settings_backup_restore_132dp));
        }

        @Override
        public void onCreateActions(@NonNull List<GuidedAction> actions,
                Bundle savedInstanceState) {
            actions.add(new GuidedAction.Builder(getContext())
                    .clickAction(GuidedAction.ACTION_ID_CANCEL)
                    .build());
            actions.add(new GuidedAction.Builder(getContext())
                    .clickAction(GuidedAction.ACTION_ID_OK)
                    .title(getString(R.string.pq_advanced_color_temperature_allreset_title))
                    .build());
        }

        @Override
        public void onGuidedActionClicked(GuidedAction action) {
            if (action.getId() == GuidedAction.ACTION_ID_OK) {
                add(getFragmentManager(), PQResetAllConfirmFragment.newInstance());
            } else if (action.getId() == GuidedAction.ACTION_ID_CANCEL) {
                getActivity().finish();
            } else {
                Log.wtf(TAG, "Unknown action clicked");
            }
        }
    }

    public static class PQResetAllConfirmFragment extends GuidedStepFragment {

        private PQSettingsManager mPQSettingsManager;
        public static PQResetAllConfirmFragment newInstance() {

            Bundle args = new Bundle();

            PQResetAllConfirmFragment fragment = new PQResetAllConfirmFragment();
            fragment.setArguments(args);
            return fragment;
        }

        @NonNull
        @Override
        public GuidanceStylist.Guidance onCreateGuidance(Bundle savedInstanceState) {
            if (mPQSettingsManager == null) {
                mPQSettingsManager = new PQSettingsManager(getContext());
            }
            return new GuidanceStylist.Guidance(
                    getString(R.string.pq_advanced_color_temperature_allreset_title),
                    getString(R.string.pq_advanced_color_temperature_allreset_confirm_description),
                    null,
                    getContext().getDrawable(R.drawable.ic_settings_backup_restore_132dp));
        }

        @Override
        public void onCreateActions(@NonNull List<GuidedAction> actions,
                Bundle savedInstanceState) {
            actions.add(new GuidedAction.Builder(getContext())
                    .clickAction(GuidedAction.ACTION_ID_CANCEL)
                    .build());
            actions.add(new GuidedAction.Builder(getContext())
                    .clickAction(GuidedAction.ACTION_ID_OK)
                    .title(getString(R.string.confirm_factory_reset_device))
                    .build());
        }

        @Override
        public void onGuidedActionClicked(GuidedAction action) {
            if (action.getId() == GuidedAction.ACTION_ID_OK) {
                if (ActivityManager.isUserAMonkey()) {
                    Log.v(TAG, "Monkey tried to erase the device. Bad monkey, bad!");
                    getActivity().finish();
                } else {
                    performFactoryReset();
                }
            } else if (action.getId() == GuidedAction.ACTION_ID_CANCEL) {
                getActivity().finish();
            } else {
                Log.wtf(TAG, "Unknown action clicked");
            }
        }

        private void performFactoryReset() {
            Log.d(TAG,"performFactoryReset");
            mPQSettingsManager.setAdvancedColorTemperatureRGainStatus (0);
            mPQSettingsManager.setAdvancedColorTemperatureGGainStatus (0);
            mPQSettingsManager.setAdvancedColorTemperatureBGainStatus (0);
            mPQSettingsManager.setAdvancedColorTemperatureROffsetStatus (0);
            mPQSettingsManager.setAdvancedColorTemperatureGOffsetStatus (0);
            mPQSettingsManager.setAdvancedColorTemperatureBOffsetStatus (0);
            getActivity().finish();
        }
    }
}
