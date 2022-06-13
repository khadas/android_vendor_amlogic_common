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

package com.android.tv.settings.more;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.accounts.AuthenticatorDescription;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.media.tv.TvInputInfo;
import android.media.tv.TvInputManager;
import android.os.Bundle;
import android.os.SystemProperties;
import android.os.UserHandle;
import android.os.RemoteException;
import android.provider.Settings;
import androidx.preference.Preference;
import androidx.preference.PreferenceGroup;
import android.text.TextUtils;
import android.util.ArraySet;
import android.util.Log;
import android.content.ActivityNotFoundException;

import com.android.tv.settings.overlay.FlavorUtils;
import com.android.tv.settings.util.DroidUtils;
import com.android.tv.settings.SettingsConstant;
import com.android.tv.settings.SettingsPreferenceFragment;
import com.android.tv.settings.tvoption.DroidSettingsModeFragment;
import com.android.tv.settings.tvoption.SoundParameterSettingManager;

import static com.android.tv.settings.overlay.FlavorUtils.FLAVOR_CLASSIC;
import static com.android.tv.settings.overlay.FlavorUtils.FLAVOR_TWO_PANEL;
import static com.android.tv.settings.overlay.FlavorUtils.FLAVOR_VENDOR;
import static com.android.tv.settings.overlay.FlavorUtils.FLAVOR_X;

import com.droidlogic.app.DroidLogicUtils;
import com.droidlogic.app.tv.TvControlManager;
import com.droidlogic.app.tv.DroidLogicTvUtils;
import com.droidlogic.app.SystemControlManager;

import java.util.ArrayList;
import java.util.Set;
import java.util.List;
import com.android.tv.settings.R;

public class MorePrefFragment extends SettingsPreferenceFragment {
    private static final String TAG = "MorePrefFragment";

    private static final String KEY_MAIN_MENU = "moresettings";
    private static final String KEY_DISPLAY = "display";
    private static final String KEY_MBOX_SOUNDS = "mbox_sound";
    private static final String KEY_POWERKEY = "powerkey_action";
    private static final String KEY_POWERONMODE = "poweronmode_action";
    private static final String MORE_SETTINGS_APP_PACKAGE = "com.android.settings";
    private static final String KEY_UPGRADE_BLUTOOTH_REMOTE = "upgrade_bluetooth_remote";
    private static final String KEY_PLAYBACK_SETTINGS = "playback_settings";
    private static final String KEY_SOUNDS = "key_sound_effects";
    private static final String KEY_KEYSTONE = "keyStone";
    private static final String KEY_NETFLIX_ESN = "netflix_esn";
    private static final String KEY_VERSION = "hailstorm_ver";
    private static final String KEY_MORE_SETTINGS = "more";
    private static final String KEY_PICTURE = "pictrue_mode";
    private static final String KEY_TV_OPTION = "tv_option";
    private static final String KEY_TV_CHANNEL = "channel";
    private static final String KEY_TV_SETTINGS = "tv_settings";
    private static final String KEY_HDMI_CEC_CONTROL = "hdmicec";
    private static final String KEY_ADVANCE_SOUND = "advanced_sound_settings";
    private static final String DTVKIT_PACKAGE = "org.dtvkit.inputsource";
    private static final String HAILSTORM_VERSION_PROP = "ro.vendor.hailstorm.version";
    private static final String DEBUG_DISPLY_PROP = "vendor.display.debug";
    static final String KEY_DEVELOP_OPTION = "amlogic_developer_options";

    private Preference mUpgradeBluetoothRemote;
    private Preference mSoundsPref;

    private String mEsnText;
    private SystemControlManager mSystemControlManager;

    private BroadcastReceiver esnReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            mEsnText = intent.getStringExtra("ESNValue");
            findPreference(KEY_NETFLIX_ESN).setSummary(mEsnText);
        }
    };

    public static MorePrefFragment newInstance() {
        return new MorePrefFragment();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    private int getPreferenceScreenResId() {
        Log.d(TAG,"getPreferenceScreenResId"+FlavorUtils.getFlavor(getContext()));
        switch (FlavorUtils.getFlavor(getContext())) {
            case FLAVOR_CLASSIC:
                return R.xml.more;
            case FLAVOR_TWO_PANEL:
                return R.xml.more_two_panel;
            case FLAVOR_X:
                return R.xml.more_x;
            case FLAVOR_VENDOR:
                return R.xml.more_vendor;
            default:
                return R.xml.more;
        }
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        setPreferencesFromResource(getPreferenceScreenResId(), null);
        boolean is_from_new_live_tv = getActivity().getIntent().getIntExtra("from_new_live_tv", 0) == 1;
        boolean is_from_live_tv = getActivity().getIntent().getIntExtra("from_live_tv", 0) == 1 || is_from_new_live_tv;
        String inputId = getActivity().getIntent().getStringExtra("current_tvinputinfo_id");
        //tvFlag, is true when TV and T962E as TV, false when Mbox and T962E as Mbox.
        boolean tvFlag = SettingsConstant.needDroidlogicTvFeature(getContext())
                && (SystemProperties.getBoolean("vendor.tv.soc.as.mbox", false) == false);
        mSystemControlManager = SystemControlManager.getInstance();

        boolean customConfig     = getContext().getPackageManager().hasSystemFeature("droidlogic.software.netflix");
        boolean debugConfig      = mSystemControlManager.getPropertyBoolean(DEBUG_DISPLY_PROP,false);

        final Preference morePref = findPreference(KEY_MAIN_MENU);
        final Preference displayPref = findPreference(KEY_DISPLAY);
        final Preference hdmicecPref = findPreference(KEY_HDMI_CEC_CONTROL);
        final Preference playbackPref = findPreference(KEY_PLAYBACK_SETTINGS);
        mSoundsPref = findPreference(KEY_SOUNDS);
        final Preference mboxSoundsPref = findPreference(KEY_MBOX_SOUNDS);
        final Preference powerKeyPref = findPreference(KEY_POWERKEY);
        final Preference powerKeyOnModePref = findPreference(KEY_POWERONMODE);
        final Preference keyStone = findPreference(KEY_KEYSTONE);
        //BluetoothRemote/HDMI cec/Playback Settings display only in Mbox
        mUpgradeBluetoothRemote = findPreference(KEY_UPGRADE_BLUTOOTH_REMOTE);
        final Preference netflixesnPref = findPreference(KEY_NETFLIX_ESN);
        final Preference versionPref = findPreference(KEY_VERSION);
        final Preference advanced_sound_settings_pref = findPreference(KEY_ADVANCE_SOUND);
        //hide it forcedly as new bluetooth remote upgrade application is not available now
        mUpgradeBluetoothRemote.setVisible(false/*is_from_live_tv ? false : (SettingsConstant.needDroidlogicBluetoothRemoteFeature(getContext()) && !tvFlag)*/);
        hdmicecPref.setVisible((getContext().getPackageManager().hasSystemFeature("android.hardware.hdmi.cec")
                && SettingsConstant.needDroidlogicHdmicecFeature(getContext())) && !is_from_live_tv);
        playbackPref.setVisible(false);
        if (netflixesnPref != null) {
            if (is_from_live_tv) {
                netflixesnPref.setVisible(false);
                versionPref.setVisible(false);
            } else if (getContext().getPackageManager().hasSystemFeature("droidlogic.software.netflix")) {
                netflixesnPref.setVisible(true);
                netflixesnPref.setSummary(mEsnText);
                versionPref.setVisible(true);
                versionPref.setSummary(mSystemControlManager.getPropertyString(HAILSTORM_VERSION_PROP,"no"));
                powerKeyPref.setVisible(false);
                keyStone.setVisible(false);

            } else {
                netflixesnPref.setVisible(false);
                versionPref.setVisible(false);
            }
        }

        final Preference developPref = findPreference(KEY_DEVELOP_OPTION);
        if ((1 == Settings.Global.getInt(getContext().getContentResolver(), Settings.Global.DEVELOPMENT_SETTINGS_ENABLED, 0))
                && SystemProperties.get("ro.product.brand").contains("Amlogic")) {
            developPref.setVisible(true);
        } else {
            developPref.setVisible(false);
        }

        final Preference moreSettingsPref = findPreference(KEY_MORE_SETTINGS);
        if (is_from_live_tv) {
            moreSettingsPref.setVisible(false);
         } else if (!isPackageInstalled(getActivity(), MORE_SETTINGS_APP_PACKAGE)) {
            getPreferenceScreen().removePreference(moreSettingsPref);
        }

        final Preference picturePref = findPreference(KEY_PICTURE);
        final Preference mTvOption = findPreference(KEY_TV_OPTION);
        final Preference channelPref = findPreference(KEY_TV_CHANNEL);
        final Preference settingsPref = findPreference(KEY_TV_SETTINGS);

        if (is_from_live_tv) {
            morePref.setTitle(R.string.settings_menu);
            displayPref.setVisible(false);
            mboxSoundsPref.setVisible(false);
            powerKeyPref.setVisible(false);
            powerKeyOnModePref.setVisible(false);
            mTvOption.setVisible(false);
            moreSettingsPref.setVisible(false);
            keyStone.setVisible(false);
            boolean isPassthrough = isPassthroughInput(inputId);
            if (isPassthrough || is_from_new_live_tv) {
                channelPref.setVisible(false);
            } else {
                channelPref.setVisible(true);
            }
            if (!SettingsConstant.needDroidlogicTvFeature(getContext())) {
                mSoundsPref.setVisible(false);//mbox doesn't surport sound effect
            }
            if (inputId != null && inputId.startsWith(DTVKIT_PACKAGE)) {
                DroidUtils.store(getActivity(), DroidUtils.KEY_HIDE_STARTUP, DroidUtils.VALUE_HIDE_STARTUP);
            } else {
                DroidUtils.store(getActivity(), DroidUtils.KEY_HIDE_STARTUP, DroidUtils.VALUE_SHOW_STARTUP);
            }
        } else {
            picturePref.setVisible(!SettingsConstant.needDroidlogicTvFeature(getContext()));
            mTvOption.setVisible(false);
            mSoundsPref.setVisible(false);
            channelPref.setVisible(false);
            settingsPref.setVisible(false);
            if (!DroidLogicUtils.isTv()) {
                powerKeyOnModePref.setVisible(false);
            }
            DroidUtils.store(getActivity(), DroidUtils.KEY_HIDE_STARTUP, DroidUtils.VALUE_HIDE_STARTUP);
        }

        if (!debugConfig && customConfig) {
             picturePref.setVisible(false);
        }

        Log.d(TAG, "powerkey_action=" + DroidUtils.hasGtvsUiMode());
        if (DroidUtils.hasGtvsUiMode()) {
            Log.i(TAG, "hide powerkey_action");
            powerKeyPref.setVisible(false);
        }
    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        super.onPreferenceTreeClick(preference);
        if (TextUtils.equals(preference.getKey(), KEY_TV_CHANNEL)) {
            startUiInLiveTv(KEY_TV_CHANNEL);
        } else if (TextUtils.equals(preference.getKey(), KEY_KEYSTONE)) {
            startKeyStoneCorrectionActivity(getActivity());
        } else if (TextUtils.equals(preference.getKey(), KEY_ADVANCE_SOUND)) {
             startAdvancedSoundSettingsActivity(getActivity());
        }
        return false;
    }

    @Override
    public int getMetricsCategory() {
        return 0;
    }

    private void startUiInLiveTv(String value) {
        Intent intent = new Intent();
        intent.setAction("action.startlivetv.settingui");
        intent.putExtra(value, true);
        getActivity().sendBroadcast(intent);
        getActivity().finish();
    }

    public static void startKeyStoneCorrectionActivity(Context context){
        try {
            Intent intent = new Intent();
            intent.setClassName("com.android.keystone", "com.android.keystone.keyStoneCorrectionActivity");
            context.startActivity(intent);
        } catch (ActivityNotFoundException e) {
            Log.d(TAG, "startKeyStoneCorrectionActivity not found!");
            return;
        }
    }

    public static void startAdvancedSoundSettingsActivity(Context context){
        try {
            Intent intent = new Intent();
            intent.setClassName("com.android.tv.settings", "com.android.tv.settings.device.displaysound.AdvancedVolumeActivity");
            context.startActivity(intent);
        } catch (ActivityNotFoundException e) {
            Log.d(TAG, "start advanced_sound_settings Activity not found!");
            return;
        }
    }

    @Override
    public void onStart() {
        super.onStart();
    }

    @Override
    public void onResume() {
        super.onResume();
        updateSounds();
        IntentFilter esnIntentFilter = new IntentFilter("com.netflix.ninja.intent.action.ESN_RESPONSE");
        getActivity().getApplicationContext().registerReceiver(esnReceiver, esnIntentFilter,
                "com.netflix.ninja.permission.ESN", null);
        Intent esnQueryIntent = new Intent("com.netflix.ninja.intent.action.ESN");
        esnQueryIntent.setPackage("com.netflix.ninja");
        esnQueryIntent.addFlags(Intent.FLAG_INCLUDE_STOPPED_PACKAGES);
        getActivity().getApplicationContext().sendBroadcast(esnQueryIntent);
    }

    @Override
    public void onPause() {
        super.onPause();
        if (esnReceiver != null) {
            getActivity().getApplicationContext().unregisterReceiver(esnReceiver);
        }
    }

    private void updateSounds() {
        if (mSoundsPref == null) {
            return;
        }

        mSoundsPref.setIcon(SoundParameterSettingManager.getSoundEffectsEnabled(getContext().getContentResolver())
                ? R.drawable.ic_volume_up : R.drawable.ic_volume_off);
    }

    private void hideIfIntentUnhandled(Preference preference) {
        if (preference == null) {
            return;
        }
        preference.setVisible(systemIntentIsHandled(preference.getIntent()) != null);
    }

    private static boolean isPackageInstalled(Context context, String packageName) {
        try {
            return context.getPackageManager().getPackageInfo(packageName, 0) != null;
        } catch (PackageManager.NameNotFoundException e) {
            return false;
        }
    }

    private ResolveInfo systemIntentIsHandled(Intent intent) {
        if (intent == null) {
            return null;
        }

        final PackageManager pm = getContext().getPackageManager();

        for (ResolveInfo info : pm.queryIntentActivities(intent, 0)) {
            if (info.activityInfo != null && info.activityInfo.enabled && (info.activityInfo.applicationInfo.flags
                    & ApplicationInfo.FLAG_SYSTEM) == ApplicationInfo.FLAG_SYSTEM) {
                return info;
            }
        }
        return null;
    }

    public boolean isPassthroughInput(String inputId) {
        boolean result = false;
        try {
            TvInputManager tvInputManager = (TvInputManager)getActivity().getSystemService(Context.TV_INPUT_SERVICE);
            List<TvInputInfo> inputList = tvInputManager.getTvInputList();
            for (TvInputInfo input : inputList) {
                if (input.isPassthroughInput() && TextUtils.equals(inputId, input.getId())) {
                    result = true;
                    break;
                }
            }
        } catch (Exception e) {
            Log.i(TAG, "isPassthroughInput Exception = " + e.getMessage());
        }
        return result;
    }
}
