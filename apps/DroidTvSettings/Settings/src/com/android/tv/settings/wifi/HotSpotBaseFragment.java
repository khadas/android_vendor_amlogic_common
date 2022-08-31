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

package com.android.tv.settings.wifi;
import static android.net.ConnectivityManager.ACTION_TETHER_STATE_CHANGED;
import static android.net.wifi.WifiManager.WIFI_AP_STATE_CHANGED_ACTION;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.UserManager;
import androidx.annotation.VisibleForTesting;
import android.util.Log;
import android.util.ArrayMap;
import androidx.preference.ListPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceScreen;
import androidx.preference.PreferenceGroup;
import androidx.preference.PreferenceManager;
import androidx.preference.SeekBarPreference;
import androidx.preference.TwoStatePreference;
import com.android.settingslib.core.AbstractPreferenceController;
import com.android.internal.logging.nano.MetricsProto;
import com.android.tv.settings.R;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Collection;

import com.android.tv.settings.SettingsPreferenceFragment;

public abstract class HotSpotBaseFragment extends SettingsPreferenceFragment{
    private static final String TAG = "HotSpotBaseFragment";
    private final Map<Class, List<AbstractPreferenceController>> mPreferenceControllers =
            new ArrayMap<>();
    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        Log.d(TAG,"HotSpotBaseFragment onAttach");
        final List<AbstractPreferenceController> controllers = new ArrayList<>();
        // Load preference controllers from code
        final List<AbstractPreferenceController> controllersFromCode =
                createPreferenceControllers(context);

        final List<BasePreferenceController> controllersFromXml = PreferenceControllerListHelper
                .getPreferenceControllersFromXml(context, getPreferenceScreenResId());
        // Filter xml-based controllers in case a similar controller is created from code already.
        final List<BasePreferenceController> uniqueControllerFromXml =
                PreferenceControllerListHelper.filterControllers(
                        controllersFromXml, controllersFromCode);

        // Add unique controllers to list.
        if (controllersFromCode != null) {
            controllers.addAll(controllersFromCode);
        }
        controllers.addAll(uniqueControllerFromXml);
        for (AbstractPreferenceController controller : controllers) {
            addPreferenceController(controller);
            Log.d(TAG,"addPreferenceController here"+controller.getClass().getName());
        }
    }
    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        refreshAllPreferences();
    }
        /**
     * Displays resource based tiles.
     */
    private void displayResourceTiles() {
        final int resId = getPreferenceScreenResId();
        if (resId <= 0) {
            return;
        }
        addPreferencesFromResource(resId);
        final PreferenceScreen screen = getPreferenceScreen();
        mPreferenceControllers.values().stream().flatMap(Collection::stream).forEach(
                controller -> controller.displayPreference(screen));
    }
    private void refreshAllPreferences() {
        // First remove old preferences.
        if (getPreferenceScreen() != null) {
            // Intentionally do not cache PreferenceScreen because it will be recreated later.
            getPreferenceScreen().removeAll();
        }

        // Add resource based tiles.
        displayResourceTiles();

    }

    protected <T extends AbstractPreferenceController> T use(Class<T> clazz) {
        List<AbstractPreferenceController> controllerList = mPreferenceControllers.get(clazz);
        if (controllerList != null) {
            if (controllerList.size() > 1) {
                Log.w(TAG, "Multiple controllers of Class " + clazz.getSimpleName()
                        + " found, returning first one.");
            }
            return (T) controllerList.get(0);
        }

        return null;
    }
        /**
     * Get a list of {@link AbstractPreferenceController} for this fragment.
     */
    protected List<AbstractPreferenceController> createPreferenceControllers(Context context) {
        return null;
    }
    protected void addPreferenceController(AbstractPreferenceController controller) {
        if (mPreferenceControllers.get(controller.getClass()) == null) {
            mPreferenceControllers.put(controller.getClass(), new ArrayList<>());
        }
        Log.d(TAG,"addPreferenceController"+controller.getClass());
        mPreferenceControllers.get(controller.getClass()).add(controller);
    }
      @Override
    public void onCreate(Bundle icicle) {
        Log.d(TAG,"HotSpotBaseFragment called");
        super.onCreate(icicle);
        // Set ComparisonCallback so we get better animation when list changes.
        getPreferenceManager().setPreferenceComparisonCallback(
                new PreferenceManager.SimplePreferenceComparisonCallback());
    }

    protected abstract int getPreferenceScreenResId();
    @Override
    public void onResume() {
        super.onResume();
        updatePreferenceStates();
    }
        /**
     * Update state of each preference managed by PreferenceController.
     */
    protected void updatePreferenceStates() {
        final PreferenceScreen screen = getPreferenceScreen();
        Collection<List<AbstractPreferenceController>> controllerLists =
                mPreferenceControllers.values();
        for (List<AbstractPreferenceController> controllerList : controllerLists) {
            for (AbstractPreferenceController controller : controllerList) {
                if (!controller.isAvailable()) {
                    continue;
                }
                final String key = controller.getPreferenceKey();

                final Preference preference = screen.findPreference(key);
                if (preference == null) {
                    Log.d(TAG, String.format("Cannot find preference with key %s in Controller %s",
                            key, controller.getClass().getSimpleName()));
                    continue;
                }
                controller.updateState(preference);
            }
        }
    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        Collection<List<AbstractPreferenceController>> controllers =
                mPreferenceControllers.values();
        // Give all controllers a chance to handle click.
        for (List<AbstractPreferenceController> controllerList : controllers) {
            for (AbstractPreferenceController controller : controllerList) {
                if (controller.handlePreferenceTreeClick(preference)) {
                    return true;
                }
            }
        }
        return super.onPreferenceTreeClick(preference);
    }

    @Override
    public int getMetricsCategory() {
        return 0;
    }

}
