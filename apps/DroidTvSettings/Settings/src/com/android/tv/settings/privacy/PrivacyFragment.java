/*
 * Copyright (C) 2020 The Android Open Source Project
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
 * limitations under the License.
 */

package com.android.tv.settings.privacy;

import static com.android.tv.settings.overlay.FlavorUtils.FLAVOR_CLASSIC;
import static com.android.tv.settings.overlay.FlavorUtils.FLAVOR_TWO_PANEL;
import static com.android.tv.settings.overlay.FlavorUtils.FLAVOR_VENDOR;
import static com.android.tv.settings.overlay.FlavorUtils.FLAVOR_X;
import static com.android.tv.settings.util.InstrumentationUtils.logEntrySelected;

import android.app.tvsettings.TvSettingsEnums;
import android.content.Intent;
import android.os.Bundle;

import androidx.annotation.Keep;
import androidx.preference.Preference;
import androidx.preference.PreferenceCategory;

import com.android.internal.logging.nano.MetricsProto;
import com.android.tv.settings.R;
import com.android.tv.settings.SettingsPreferenceFragment;
import com.android.tv.settings.overlay.FlavorUtils;
import com.android.tv.settings.util.SliceUtils;
import com.android.tv.twopanelsettings.slices.CustomContentDescriptionPreference;
import com.android.tv.twopanelsettings.slices.SlicePreference;

/**
 * The Privacy policies screen in Settings.
 */
@Keep
public class PrivacyFragment extends SettingsPreferenceFragment {

    private static final String KEY_ACCOUNT_SETTINGS_CATEGORY = "accountSettings";
    private static final String KEY_USAGE = "usageAndDiag";
    private static final String KEY_ADS = "ads";
    private static final String KEY_ASSISTANT = "assistant";
    private static final String KEY_PURCHASES = "purchases";

    private int getPreferenceScreenResId() {
        switch (FlavorUtils.getFlavor(getContext())) {
            case FLAVOR_CLASSIC:
            case FLAVOR_TWO_PANEL:
                return R.xml.privacy;
            case FLAVOR_X:
            case FLAVOR_VENDOR:
                return R.xml.privacy_x;
            default:
                return R.xml.privacy;
        }
    }

    @Override
    public void onCreatePreferences(Bundle bundle, String s) {
        setPreferencesFromResource(getPreferenceScreenResId(), null);
        PreferenceCategory accountPrefCategory = findPreference(KEY_ACCOUNT_SETTINGS_CATEGORY);
        Preference assistantSlicePreference = findPreference(KEY_ASSISTANT);
        Preference purchasesSlicePreference = findPreference(KEY_PURCHASES);
        Preference adsPreference = findPreference(KEY_ADS);

        if (FlavorUtils.getFeatureFactory(getContext()).getBasicModeFeatureProvider()
                .isBasicMode(getContext())) {
            accountPrefCategory.setVisible(false);
            assistantSlicePreference.setVisible(false);
            purchasesSlicePreference.setVisible(false);
            adsPreference.setVisible(false);
            return;
        }
        if (assistantSlicePreference instanceof SlicePreference
                && SliceUtils.isSliceProviderValid(
                        getContext(), ((SlicePreference) assistantSlicePreference).getUri())) {
            assistantSlicePreference.setVisible(true);
        }
        if (purchasesSlicePreference instanceof SlicePreference
                && SliceUtils.isSliceProviderValid(
                        getContext(), ((SlicePreference) purchasesSlicePreference).getUri())) {
            purchasesSlicePreference.setVisible(true);
        }
        accountPrefCategory.setVisible(
                assistantSlicePreference.isVisible() || purchasesSlicePreference.isVisible());
        findPreference(KEY_ADS).setOnPreferenceClickListener(preference -> {
            Intent intent = new Intent();
            intent.setAction("com.google.android.gms.settings.ADS_PRIVACY");
            startActivity(intent);
            return true;
        });
        if (adsPreference instanceof CustomContentDescriptionPreference) {
            ((CustomContentDescriptionPreference) adsPreference).setContentDescription(
                    getResources().getString(R.string.ads_content_description));
        }
    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        switch (preference.getKey()) {
            case KEY_USAGE:
                logEntrySelected(TvSettingsEnums.PRIVACY_DIAGNOSTICS);
                break;
            case KEY_ADS:
                logEntrySelected(TvSettingsEnums.PRIVACY_ADS);
                break;
        }
        return super.onPreferenceTreeClick(preference);
    }

    @Override
    public int getMetricsCategory() {
        return MetricsProto.MetricsEvent.PRIVACY;
    }

    @Override
    protected int getPageId() {
        return TvSettingsEnums.PRIVACY;
    }
}
