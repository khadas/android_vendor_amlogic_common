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

package com.android.tv.settings;

import android.app.Activity;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.hardware.hdmi.HdmiDeviceInfo;
import android.media.tv.TvInputInfo;
import android.media.tv.TvInputManager;
import android.os.Bundle;
import android.os.SystemClock;
import android.os.SystemProperties;
import android.provider.Settings;
import androidx.leanback.preference.LeanbackPreferenceFragment;
import androidx.preference.ListPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceScreen;
import androidx.preference.TwoStatePreference;
import android.text.TextUtils;
import android.util.Log;
import android.widget.ListAdapter;

import com.droidlogic.app.DataProviderManager;
import com.android.tv.settings.SettingsConstant;
import com.android.tv.settings.R;

import java.util.Collections;
import java.util.Comparator;
import java.util.List;

import com.droidlogic.app.tv.TvControlManager;
import com.droidlogic.app.tv.DroidLogicTvUtils;
import com.droidlogic.app.tv.TvScanConfig;
import com.droidlogic.app.tv.ChannelInfo;

import android.hardware.hdmi.HdmiControlManager;
import android.hardware.hdmi.HdmiTvClient;
import android.media.tv.TvInputHardwareInfo;
import java.util.ArrayList;
import com.droidlogic.app.SystemControlManager;

public class TvSourceFragment extends LeanbackPreferenceFragment {

    private static final String TAG = "TvSourceFragment";
    private static final boolean DEBUG = true;

    private static final int MODE_GLOBAL = 0;
    private static final int MODE_LIVE_TV = 1;

    private static final String COMMANDACTION = "action.startlivetv.settingui";
    private static final String PACKAGE_DROIDLOGIC_TVINPUT = "com.droidlogic.tvinput";
    private static final String PACKAGE_DROIDLOGIC_DTVKIT = "com.droidlogic.dtvkit.inputsource";
    private static final String PACKAGE_GOOGLE_VIDEOS = "com.google.android.videos";

    private static final String DATA_FROM_TV_APP = "from_live_tv";
    private static final String DATA_REQUEST_PACKAGE = "requestpackage";

    private static final String INPUT_ADTV = "ADTV";
    private static final String INPUT_ATV = "ATV";
    private static final String INPUT_DTV = "DTV";
    private static final String INPUT_AV = "AV";
    private static final String INPUT_HDMI = "HDMI";
    private static final String INPUT_HDMI_LOWER = "Hdmi";

    private final String DTVKITSOURCE = "com.droidlogic.dtvkit.inputsource/.DtvkitTvInput/HW19";

    private static final int RESULT_OK = -1;
    private static final int LOGICAL_ADDRESS_AUDIO_SYSTEM = 5;

    private TvInputManager mTvInputManager;
    private TvControlManager mTvControlManager;
    private HdmiControlManager mHdmiControlManager;

    private final InputsComparator mComparator = new InputsComparator();
    private Context mContext;
    private String mPreSource;
    private String mCurSource;

    private boolean mFromTvApp;
    private String mStartPackage;

    private HdmiTvClient mTvClient;

    // if Fragment has no nullary constructor, it might throw InstantiationException, so add this constructor.
    // For more details, you can visit http://blog.csdn.net/xplee0576/article/details/43057633 .
    public TvSourceFragment() {}

    public TvSourceFragment(Context context) {
        mContext = context;
        mTvInputManager = (TvInputManager)context.getSystemService(Context.TV_INPUT_SERVICE);
        mTvControlManager = TvControlManager.getInstance();
        mHdmiControlManager = (HdmiControlManager) context.getSystemService(Context.HDMI_CONTROL_SERVICE);
        if (mHdmiControlManager != null) {
            mTvClient = mHdmiControlManager.getTvClient();
        }
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        Intent intent = null;

        if (mContext != null) {
            intent = ((Activity)mContext).getIntent();
        }

        if (DEBUG)  Log.d(TAG, "onCreatePreferences  intent= "+ intent);
        if (intent != null) {
            mFromTvApp = intent.getIntExtra(DATA_FROM_TV_APP, MODE_GLOBAL) == MODE_LIVE_TV;
            mStartPackage = intent.getStringExtra(DATA_REQUEST_PACKAGE);
        }
        updatePreferenceFragment();
    }
    public void calculatePreSrcToCurSrc(Preference preference) {
        String currentInputId = DroidLogicTvUtils.getCurrentInputId(mContext);
        if (currentInputId != null) {
            if (currentInputId.contains(INPUT_ADTV)) {
                if (DroidLogicTvUtils.isATV(mContext)) {
                    mPreSource = INPUT_ATV;
                } else {
                    mPreSource = INPUT_DTV;
                }
            } else if (currentInputId.contains(INPUT_AV)){
                mPreSource = INPUT_AV;
            } else if (currentInputId.contains(INPUT_HDMI_LOWER)) {
                mPreSource = INPUT_HDMI;
            }
        }

        String inputId = preference.getKey();
         if (!TextUtils.isEmpty(inputId) && inputId.contains(INPUT_HDMI_LOWER)) {
            mCurSource = INPUT_HDMI;
        } else if (TextUtils.regionMatches(preference.getTitle(), 0, INPUT_AV, 0, 2)) {
            mCurSource = INPUT_AV;
        } else if (TextUtils.regionMatches(preference.getTitle(), 0, INPUT_ATV, 0, 3)) {
            mCurSource = INPUT_ATV;
        } else if (TextUtils.regionMatches(preference.getTitle(), 0, INPUT_DTV, 0, 3)) {
            mCurSource = INPUT_DTV;
        }
        Log.d(TAG, "onPreferenceTreeClick SwitchSourceTime PreSource - CurSource " + mPreSource + "-" + mCurSource);
    }
    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
            calculatePreSrcToCurSrc(preference);
            float Time= (float) SystemClock.uptimeMillis() / 1000;
            Log.d(TAG, "onPreferenceTreeClick SwitchSourceTime = " + Time);
            final Preference sourcePreference = preference;
            List<TvInputInfo> inputList = mTvInputManager.getTvInputList();
            for (TvInputInfo input : inputList) {
                if (sourcePreference.getKey().equals(input.getId())) {
                    if (DEBUG) Log.d(TAG, "onPreferenceTreeClick:  info=" + input);
                    DroidLogicTvUtils.setCurrentInputId(mContext, input.getId());
                    if (!input.isPassthroughInput()) {
                        DroidLogicTvUtils.setSearchInputId(mContext, input.getId(), false);
                        if (TextUtils.equals(sourcePreference.getTitle(), mContext.getResources().getString(R.string.input_atv))) {
                            DroidLogicTvUtils.setSearchType(mContext, TvScanConfig.TV_SEARCH_TYPE.get(TvScanConfig.TV_SEARCH_TYPE_ATV_INDEX));
                        } else if (TextUtils.equals(sourcePreference.getTitle(), mContext.getResources().getString(R.string.input_dtv))) {
                            String country = DroidLogicTvUtils.getCountry(mContext);
                            ArrayList<String> dtvList = TvScanConfig.GetTvDtvSystemList(country);
                            DroidLogicTvUtils.setSearchType(mContext, dtvList.get(0));
                        }
                    }

                    Settings.System.putInt(mContext.getContentResolver(), DroidLogicTvUtils.TV_CURRENT_DEVICE_ID,
                            DroidLogicTvUtils.getHardwareDeviceId(input));

                    SystemControlManager mSystemControlManager = SystemControlManager.getInstance();
                    if (DTVKITSOURCE.equals(input.getId())) {//DTVKIT SOURCE
                        if (DEBUG) Log.d(TAG, "DtvKit source");
                        mSystemControlManager.SetDtvKitSourceEnable(1);
                    } else {
                        if (DEBUG) Log.d(TAG, "Not DtvKit source");
                        mSystemControlManager.SetDtvKitSourceEnable(0);
                    }

                    if (mFromTvApp) {
                        Intent intent = new Intent();
                        intent.setAction(COMMANDACTION);
                        intent.putExtra("from_tv_source", true);
                        intent.putExtra(TvInputInfo.EXTRA_INPUT_ID, input.getId());
                        getActivity().sendBroadcast(intent);
                    } else {
                        Intent intent = new Intent(TvInputManager.ACTION_SETUP_INPUTS);
                        intent.putExtra("from_tv_source", true);
                        intent.putExtra(TvInputInfo.EXTRA_INPUT_ID, input.getId());
                        if (mStartPackage != null && mStartPackage.equals("com.droidlogic.mboxlauncher")) {
                            ((Activity)mContext).setResult(RESULT_OK, intent);
                        } else {
                            getPreferenceManager().getContext().startActivity(intent);
                        }
                    }
                    ((Activity)mContext).finish();
                   break;
               }
        }
        return super.onPreferenceTreeClick(preference);
    }

    private void updatePreferenceFragment() {
        final Context themedContext = getPreferenceManager().getContext();
        final PreferenceScreen screen = getPreferenceManager().createPreferenceScreen(
                themedContext);
        screen.setTitle(R.string.tv_source);
        setPreferenceScreen(screen);

        List<TvInputInfo> inputList = mTvInputManager.getTvInputList();
        Collections.sort(inputList, mComparator);
        List<Preference> preferenceList = new ArrayList<Preference>();
        List<HdmiDeviceInfo> hdmiList = getHdmiList();
        HdmiDeviceInfo audioSystem = getOrigHdmiDevice(LOGICAL_ADDRESS_AUDIO_SYSTEM, hdmiList);

        for (TvInputInfo input : inputList) {
            Log.d(TAG,"updatePreferenceFragment input " + input + "-->" + input.getType());
            if (input.isHidden(themedContext)) {
                Log.d(TAG, "updatePreferenceFragment this input hidden");
                continue;
            }
            if (input.isPassthroughInput() && input.getParentId() != null) {
                // DroidSettings always show the fixed hdmi port related sources, even though
                // there are no devices connected, so we should only care about the parent
                // sources.
                continue;
            }
            Preference sourcePreference = new Preference(themedContext);
            sourcePreference.setKey(input.getId());
            sourcePreference.setPersistent(false);
            sourcePreference.setIcon(getIcon(input, isInputEnabled(input)));
            sourcePreference.setTitle(getTitle(themedContext, input, audioSystem, hdmiList));
            preferenceList.add(sourcePreference);
            addSpecificDtv(themedContext, input, preferenceList);
        }
        for (Preference sourcePreference : preferenceList) {
            screen.addPreference(sourcePreference);
        }
    }

    private void addSpecificDtv(Context themedContext, TvInputInfo input, List<Preference> preferenceList) {
        if (DroidLogicTvUtils.isChina(themedContext)
            && input.getType() == TvInputInfo.TYPE_TUNER
            && PACKAGE_DROIDLOGIC_TVINPUT.equals(input.getServiceInfo().packageName)) {
            Preference sourcePreferenceDtv = new Preference(themedContext);
            sourcePreferenceDtv.setKey(input.getId());
            sourcePreferenceDtv.setPersistent(false);
            sourcePreferenceDtv.setIcon(R.drawable.ic_dtv_connected);
            sourcePreferenceDtv.setTitle(R.string.input_dtv);
            if (mTvControlManager.GetHotPlugDetectEnableStatus()) {
                sourcePreferenceDtv.setEnabled(isInputEnabled(input));
            }
            preferenceList.add(sourcePreferenceDtv);
        }
    }

    private CharSequence getTitle(Context themedContext, TvInputInfo input, HdmiDeviceInfo audioSystem, List<HdmiDeviceInfo> hdmiList) {
        CharSequence title = "";
        CharSequence label = input.loadLabel(themedContext);
        CharSequence customLabel = input.loadCustomLabel(themedContext);
        if (TextUtils.isEmpty(customLabel) || customLabel.equals(label)) {
            title = label;
        } else {
            title = customLabel;
        }
        Log.d(TAG, "getTitle default " + title + ", label = " + label + ", customLabel = " + customLabel);
        if (input.isPassthroughInput()) {
            int portId = DroidLogicTvUtils.getPortId(input);
            if (audioSystem != null && audioSystem.getPortId() == portId) {
                // there is an audiosystem connected.
                title = audioSystem.getDisplayName();
            } else {
                HdmiDeviceInfo hdmiDevice = getOrigHdmiDeviceByPort(portId, hdmiList);
                if (hdmiDevice != null) {
                    // there is a playback connected.
                    title = hdmiDevice.getDisplayName();
                }
            }
        } else if (input.getType() == TvInputInfo.TYPE_TUNER) {
            title = getTitleForTuner(themedContext, input.getServiceInfo().packageName, title, input);
        } else if (TextUtils.isEmpty(title)) {
            title = input.getServiceInfo().name;
        }
        Log.d(TAG, "getTitle " + title);
        return title;
    }

    private CharSequence getTitleForTuner(Context themedContext, String packageName, CharSequence label, TvInputInfo input) {
        CharSequence title = label;
        if (PACKAGE_DROIDLOGIC_TVINPUT.equals(packageName)) {
            title = themedContext.getString(DroidLogicTvUtils.isChina(themedContext) ? R.string.input_atv : R.string.input_long_label_for_tuner);
        } else if (TextUtils.isEmpty(label)) {
            if (PACKAGE_DROIDLOGIC_DTVKIT.equals(packageName)) {
                title = themedContext.getString(R.string.input_dtv_kit);
            } else if (PACKAGE_GOOGLE_VIDEOS.equals(packageName)) {
                title = themedContext.getString(R.string.input_google_channel);
            } else {
                title = input.getServiceInfo().name;
            }
        }

        Log.d(TAG, "getTitleForTuner title " + title + " for package " + packageName);
        return title;
    }

    private List<HdmiDeviceInfo> getHdmiList() {
        if (mTvClient == null) {
            Log.e(TAG, "mTvClient null!");
            return null;
        }
        return mTvClient.getDeviceList();
    }

    /**
     * The update of hdmi device info will not notify TvInputManagerService now.
     */
    private HdmiDeviceInfo getOrigHdmiDeviceByPort(int portId, List<HdmiDeviceInfo> hdmiList) {
        if (hdmiList == null) {
            Log.d(TAG, "mTvInputManager or mTvClient maybe null");
            return null;
        }
        for (HdmiDeviceInfo info : hdmiList) {
            if (info.getPortId() == portId) {
                return info;
            }
        }
        return null;
    }

    private HdmiDeviceInfo getOrigHdmiDevice(int logicalAddress, List<HdmiDeviceInfo> hdmiList) {
        if (hdmiList == null) {
            Log.d(TAG, "mTvInputManager or mTvClient maybe null");
            return null;
        }
        for (HdmiDeviceInfo info : hdmiList) {
            if ((info.getLogicalAddress() == logicalAddress)) {
                return info;
            }
        }
        return null;
    }

    private boolean isInputEnabled(TvInputInfo input) {
        HdmiDeviceInfo hdmiInfo = input.getHdmiDeviceInfo();
        if (hdmiInfo != null) {
            if (DEBUG) Log.d(TAG, "isInputEnabled:  hdmiInfo="+ hdmiInfo);
            return true;
        }

        int deviceId = DroidLogicTvUtils.getHardwareDeviceId(input);
        if (DEBUG) {
            Log.d(TAG, "===== getHardwareDeviceId:tvInputId = " + input.getId());
            Log.d(TAG, "===== deviceId : "+ deviceId);
        }
        TvControlManager.SourceInput tvSourceInput = DroidLogicTvUtils.parseTvSourceInputFromDeviceId(deviceId);
        int connectStatus = -1;
        if (tvSourceInput != null) {
            connectStatus = mTvControlManager.GetSourceConnectStatus(tvSourceInput);
        } else {
            if (DEBUG) {
                Log.w(TAG, "===== cannot find tvSourceInput");
            }
        }

        return !input.isPassthroughInput() || 1 == connectStatus || deviceId == DroidLogicTvUtils.DEVICE_ID_SPDIF;
    }

    private class InputsComparator implements Comparator<TvInputInfo> {
        @Override
        public int compare(TvInputInfo lhs, TvInputInfo rhs) {
            if (lhs == null) {
                return (rhs == null) ? 0 : 1;
            }
            if (rhs == null) {
                return -1;
            }

           /* boolean enabledL = isInputEnabled(lhs);
            boolean enabledR = isInputEnabled(rhs);
            if (enabledL != enabledR) {
                return enabledL ? -1 : 1;
            }*/

            int priorityL = getPriority(lhs);
            int priorityR = getPriority(rhs);
            if (priorityL != priorityR) {
                return priorityR - priorityL;
            }

            String customLabelL = (String) lhs.loadCustomLabel(getContext());
            String customLabelR = (String) rhs.loadCustomLabel(getContext());
            if (!TextUtils.equals(customLabelL, customLabelR)) {
                customLabelL = customLabelL == null ? "" : customLabelL;
                customLabelR = customLabelR == null ? "" : customLabelR;
                return customLabelL.compareToIgnoreCase(customLabelR);
            }

            String labelL = (String) lhs.loadLabel(getContext());
            String labelR = (String) rhs.loadLabel(getContext());
            labelL = labelL == null ? "" : labelL;
            labelR = labelR == null ? "" : labelR;
            return labelL.compareToIgnoreCase(labelR);
        }

        private int getPriority(TvInputInfo info) {
            switch (info.getType()) {
                case TvInputInfo.TYPE_TUNER:
                    return 9;
                case TvInputInfo.TYPE_HDMI:
                    HdmiDeviceInfo hdmiInfo = info.getHdmiDeviceInfo();
                    if (hdmiInfo != null && hdmiInfo.isCecDevice()) {
                        return 8;
                    }
                    return 7;
                case TvInputInfo.TYPE_DVI:
                    return 6;
                case TvInputInfo.TYPE_COMPONENT:
                    return 5;
                case TvInputInfo.TYPE_SVIDEO:
                    return 4;
                case TvInputInfo.TYPE_COMPOSITE:
                    return 3;
                case TvInputInfo.TYPE_DISPLAY_PORT:
                    return 2;
                case TvInputInfo.TYPE_VGA:
                    return 1;
                case TvInputInfo.TYPE_SCART:
                default:
                    return 0;
            }
        }
    }

    private int getIcon(TvInputInfo info, boolean isConnected) {
        int icon = R.drawable.ic_dtv_connected;
        if (info.isPassthroughInput()) {
            icon = getIconForPassthrough(info, isConnected);
        }
        return icon;
    }

    private int getIconForPassthrough(TvInputInfo info, boolean isConnected) {
        switch (info.getType()) {
            case TvInputInfo.TYPE_TUNER:
                if (isConnected) {
                    return DroidLogicTvUtils.isChina(mContext) ? R.drawable.ic_atv_connected : R.drawable.ic_atsc_connected;
                } else {
                    return DroidLogicTvUtils.isChina(mContext) ? R.drawable.ic_atv_disconnected : R.drawable.ic_atsc_disconnected;
                }
            case TvInputInfo.TYPE_HDMI:
                if (isConnected) {
                    return R.drawable.ic_hdmi_connected;
                } else {
                    return R.drawable.ic_hdmi_disconnected;
                }
            case TvInputInfo.TYPE_COMPOSITE:
                if (isConnected) {
                    return R.drawable.ic_av_connected;
                } else {
                    return R.drawable.ic_av_disconnected;
                }
            default:
                 if (isConnected) {
                    return R.drawable.ic_spdif_connected;
                } else {
                    return R.drawable.ic_spdif_disconnected;
                }
         }
     }
}