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

package com.android.tv.settings.display.outputmode;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.SystemProperties;
import android.provider.Settings;
import androidx.preference.SwitchPreference;
import com.android.tv.settings.SettingsPreferenceFragment;
import androidx.preference.ListPreference;
import androidx.preference.Preference;
import android.text.TextUtils;
import android.text.format.DateFormat;

import com.android.tv.settings.SettingsConstant;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.LayoutInflater;
import android.view.Window;
import android.view.WindowManager;
import android.widget.TextView;
import java.util.Timer;
import java.util.TimerTask;

import android.util.Log;
import com.android.tv.settings.R;
import com.droidlogic.app.DolbyVisionSettingManager;
import com.droidlogic.app.OutputModeManager;
import com.droidlogic.app.SystemControlManager;


public class ScreenResolutionFragment extends SettingsPreferenceFragment implements
        Preference.OnPreferenceChangeListener, OnClickListener {
    private static final String LOG_TAG = "ScreenResolutionFragment";
    private static final String KEY_COLORSPACE = "colorspace_setting";
    private static final String KEY_COLORDEPTH = "colordepth_setting";
    private static final String KEY_DISPLAYMODE = "displaymode_setting";
    private static final String KEY_BEST_RESOLUTION = "best_resolution";
    private static final String KEY_BEST_DOLBYVISION = "best_dolbyvision";
    private static final String KEY_DOLBYVISION = "dolby_vision";
    private static final String KEY_HDR_PRIORITY = "hdr_priority";
    private static final String KEY_HDR_POLICY = "hdr_policy";
    private static final String KEY_DOLBYVISION_PRIORITY = "dolby_vision_graphics_priority";
    private static final String DEFAULT_VALUE = "444,8bit";
    private static final String HDMI_OUTPUT_MODE = "dummy_l";

    private static boolean DEBUG = Log.isLoggable(LOG_TAG,Log.DEBUG);
    private static final int DV_LL_RGB            = 3;
    private static final int DV_LL_YUV            = 2;
    private static final int DV_ENABLE            = 1;
    private static final int DV_DISABLE           = 0;

    private String preMode;
    private String preDeepColor;
    private View view_dialog;
    private TextView tx_title;
    private TextView tx_content;
    private Timer timer;
    private TimerTask task;
    private AlertDialog mAlertDialog = null;
    private int countdown = 15;
    private static String mode = null;
    private static final int MSG_FRESH_UI = 0;
    private static final int MSG_COUNT_DOWN = 1;
    private static final int MSG_PLUG_FRESH_UI = 2;

    private OutputModeManager mOutputModeManager;
    private DolbyVisionSettingManager mDolbyVisionSettingManager;
    private Preference mBestResolutionPref;
    private Preference mBestDolbyVisionPref;
    private Preference mDisplayModePref;
    private Preference mDeepColorPref;
    private Preference mColorDepthPref;
    private Preference mDolbyVisionPref;
    private Preference mHdrPriorityPref;
    private Preference mHdrPolicyPref;
    private Preference mGraphicsPriorityPref;
    private OutputUiManager mOutputUiManager;
    private SystemControlManager mSystemControlManager;
    private IntentFilter mIntentFilter;
    public boolean hpdFlag = false;

    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_FRESH_UI:
                    if (!(HDMI_OUTPUT_MODE.equals(mOutputModeManager.getCurrentOutputMode()) && isHdmiMode())) {
                       if (DEBUG)
                           Log.d(LOG_TAG,"CurrentOutputMode:"+mOutputModeManager.getCurrentOutputMode() +" HdmiMode"+isHdmiMode());
                       updateScreenResolutionDisplay();
                    }
                    break;
                case MSG_COUNT_DOWN:
                    tx_title.setText(Integer.toString(countdown) + " " +
                        getResources().getString(R.string.device_outputmode_countdown));
                    if (countdown == 0) {
                        if (mAlertDialog != null) {
                            mAlertDialog.dismiss();
                        }
                        recoverOutputMode();
                        task.cancel();
                    }
                    countdown--;
                    break;
            }
        }
    };
    private BroadcastReceiver mIntentReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            hpdFlag = intent.getBooleanExtra ("state", false);
            mHandler.sendEmptyMessageDelayed(MSG_FRESH_UI, hpdFlag ^ isHdmiMode() ? 2000 : 1000);
        }
    };

    public static ScreenResolutionFragment newInstance() {
        return new ScreenResolutionFragment();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mOutputUiManager = new OutputUiManager(getActivity());
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        setPreferencesFromResource(R.xml.screen_resolution, null);

        mOutputModeManager = OutputModeManager.getInstance(getActivity());
        mSystemControlManager = SystemControlManager.getInstance();
        mDolbyVisionSettingManager = new DolbyVisionSettingManager((Context) getActivity());
        mBestResolutionPref = findPreference(KEY_BEST_RESOLUTION);
        mBestDolbyVisionPref = findPreference(KEY_BEST_DOLBYVISION);
        mBestResolutionPref.setOnPreferenceChangeListener(this);
        mBestDolbyVisionPref.setOnPreferenceChangeListener(this);
        mDisplayModePref = findPreference(KEY_DISPLAYMODE);
        mDeepColorPref = findPreference(KEY_COLORSPACE);
        mColorDepthPref = findPreference(KEY_COLORDEPTH);
        mDolbyVisionPref = findPreference(KEY_DOLBYVISION);
        mHdrPriorityPref = findPreference(KEY_HDR_PRIORITY);
        mHdrPolicyPref = findPreference(KEY_HDR_POLICY);
        mIntentFilter = new IntentFilter("android.intent.action.HDMI_PLUGGED");
        mIntentFilter.addAction(Intent.ACTION_TIME_TICK);
        mGraphicsPriorityPref = findPreference(KEY_DOLBYVISION_PRIORITY);
    }

    @Override
    public void onResume() {
        super.onResume();
        getActivity().registerReceiver(mIntentReceiver, mIntentFilter);
        mHandler.sendEmptyMessage(MSG_FRESH_UI);
    }
    @Override
    public void onDestroy() {
        super.onDestroy();
    }
    @Override
    public void onPause() {
        super.onPause();
        getActivity().unregisterReceiver(mIntentReceiver);
    }

    private void updateScreenResolutionDisplay() {
        if (DEBUG)
            Log.d(LOG_TAG,"showUI and at this time current isBestOutputmode? "+ isBestResolution());

        if (getContext() == null) {
            Log.d(LOG_TAG,"context is NULL");
            return;
        }

        mOutputUiManager.updateUiMode();

        // output mode.
        mDisplayModePref.setSummary(mOutputUiManager.getOutputmodeTitleList().get(mOutputUiManager.getCurrentModeIndex()));

        boolean dvFlag = mOutputUiManager.isDolbyVisionEnable() && mOutputUiManager.isTvSupportDolbyVision();
        //socSupportDv: if the chip is G12A/G12B/SM1 and T962E/E2 etc, it will be true
        //platformSuportDv: if the chip support dv and the code contains dovi.ko, it will be true
        boolean isSocSupportDv = mDolbyVisionSettingManager.isSocSupportDolbyVision();
        boolean socSupportDv   = isSocSupportDv &&
                (!SettingsConstant.needDroidlogicTvFeature(getPreferenceManager().getContext()) || SystemProperties.getBoolean("vendor.tv.soc.as.mbox", false));
        boolean platformSuportDv = mDolbyVisionSettingManager.isMboxSupportDolbyVision();
        boolean displayConfig    = SettingsConstant.needDroidlogicBestDolbyVision(getPreferenceManager().getContext());
        boolean customConfig     = mOutputModeManager.isSupportNetflix();
        boolean debugConfig      = mOutputModeManager.isSupportDisplayDebug();

        Log.d(LOG_TAG,"isSocSupportDv "+ isSocSupportDv);
        Log.d(LOG_TAG,"platformSuportDv "+ platformSuportDv);
        Log.d(LOG_TAG,"socSupportDv "+ socSupportDv);
        Log.d(LOG_TAG,"displayConfig "+ displayConfig);
        Log.d(LOG_TAG,"customConfig "+ customConfig);
        Log.d(LOG_TAG,"debugConfig "+ debugConfig);

        if (isHdmiMode()) {
            // Auto Best Resolution.
            mBestResolutionPref.setVisible(true);
            mBestResolutionPref.setEnabled(true);
            ((SwitchPreference)mBestResolutionPref).setChecked(isBestResolution());
            if (isBestResolution()) {
                mBestResolutionPref.setSummary(R.string.captions_display_on);
            }else {
                mBestResolutionPref.setSummary(R.string.captions_display_off);
            }

            // deep space
            mDeepColorPref.setVisible(true);
            mDeepColorPref.setEnabled(!dvFlag);
            mDeepColorPref.setSummary(mOutputUiManager.getCurrentColorSpaceTitle());

            // color sapce
            mColorDepthPref.setVisible(false);
            mColorDepthPref.setEnabled(!dvFlag);
            mColorDepthPref.setSummary(
                mOutputUiManager.getCurrentColorDepthAttr().contains("8bit") ? "off":"on");

            // dolby vision
            mDolbyVisionPref.setVisible(platformSuportDv && displayConfig);
            mDolbyVisionPref.setEnabled(displayConfig);
            if (true == mOutputUiManager.isDolbyVisionEnable()) {
                if (mDolbyVisionSettingManager.getDolbyVisionType() == 2) {
                    mDolbyVisionPref.setSummary(R.string.dolby_vision_low_latency_yuv);
                } else if (mDolbyVisionSettingManager.getDolbyVisionType() == 3) {
                    mDolbyVisionPref.setSummary(R.string.dolby_vision_low_latency_rgb);
                } else {
                    if (mOutputUiManager.isTvSupportDolbyVision()) {
                        mDolbyVisionPref.setSummary(R.string.dolby_vision_sink_led);
                    } else {
                        mDolbyVisionPref.setSummary(R.string.dolby_vision_default_enable);
                    }
                }
            } else {
                mDolbyVisionPref.setSummary(R.string.dolby_vision_off);
            }

            // HDR policy
            mHdrPolicyPref.setVisible(isSocSupportDv);
            if (mOutputUiManager.getHdrStrategy().equals("0")) {
                mHdrPolicyPref.setSummary(R.string.hdr_policy_sink);
            } else if (mOutputUiManager.getHdrStrategy().equals("1")) {
                mHdrPolicyPref.setSummary(R.string.hdr_policy_source);
            }

            //dolby vision graphic
            mGraphicsPriorityPref.setVisible(isSocSupportDv && mOutputUiManager.isDolbyVisionEnable() && displayConfig);
            if (mDolbyVisionSettingManager.getGraphicsPriority().equals("1")) {
                mGraphicsPriorityPref.setSummary(R.string.graphics_priority);
            } else if (mDolbyVisionSettingManager.getGraphicsPriority().equals("0")) {
                mGraphicsPriorityPref.setSummary(R.string.video_priority);
            }

            // hdr priority
            mHdrPriorityPref.setVisible(platformSuportDv);
            if (mOutputModeManager.getHdrPriority() == 1) {
                mHdrPriorityPref.setSummary(R.string.hdr10);
            } else if (mOutputModeManager.getHdrPriority() == 2) {
                mHdrPriorityPref.setSummary(R.string.sdr);
            } else {
                mHdrPriorityPref.setSummary(R.string.dolby_vision);
            }

            // best dolby vision.
            mBestDolbyVisionPref.setVisible(false);
            ((SwitchPreference)mBestDolbyVisionPref).setChecked(isBestDolbyVsion());
            if (isBestDolbyVsion()) {
                mBestDolbyVisionPref.setSummary(R.string.captions_display_on);
            }else {
                mBestDolbyVisionPref.setSummary(R.string.captions_display_off);
            }

            //for custom design
            if (!debugConfig && customConfig) {
                mDolbyVisionPref.setVisible(false);
                if (mOutputUiManager.isDolbyVisionEnable())
                    mDeepColorPref.setVisible(false);
                mGraphicsPriorityPref.setEnabled(false);
                mHdrPolicyPref.setVisible(false);
            }

        } else {
            mBestResolutionPref.setVisible(false);
            mDeepColorPref.setVisible(false);
            mColorDepthPref.setVisible(false);
            mBestDolbyVisionPref.setVisible(false);
            mDolbyVisionPref.setVisible(false);
            mGraphicsPriorityPref.setVisible(false);
            mHdrPolicyPref.setVisible(false);
            mHdrPriorityPref.setVisible(false);
        }
    }

    /**
     * recover previous output mode and best resolution state.
     */
    private void recoverOutputMode() {
        if (DEBUG)
            Log.d(LOG_TAG,"recoverOutputMode"+preMode+"/"+getCurrentDisplayMode());

        mOutputUiManager.setDeepColorAttribute(preDeepColor);
        mOutputUiManager.change2NewMode(preMode);

        mHandler.sendEmptyMessage(MSG_FRESH_UI);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        if (TextUtils.equals(preference.getKey(), KEY_BEST_RESOLUTION)) {
            preMode = getCurrentDisplayMode();
            preDeepColor = getCurrentDeepColor();
            if ((boolean)newValue) {
                setBestResolution();
                mHandler.sendEmptyMessage(MSG_FRESH_UI);
                if (isBestResolution()) {
                    showDialog();
                }
            } else {
                mOutputUiManager.change2NewMode(preMode);
                mHandler.sendEmptyMessage(MSG_FRESH_UI);
            }
        } else if (TextUtils.equals(preference.getKey(), KEY_BEST_DOLBYVISION)) {
            int type = mDolbyVisionSettingManager.getDolbyVisionType();
            String mode = mDolbyVisionSettingManager.isTvSupportDolbyVision();
            if (!isBestDolbyVsion()) {
                if (!mode.equals("")) {
                    if (mode.contains("LL_YCbCr_422_12BIT")) {
                        mDolbyVisionSettingManager.setDolbyVisionEnable(DV_LL_YUV);
                    } else if (mode.contains("DV_RGB_444_8BIT")) {
                        mDolbyVisionSettingManager.setDolbyVisionEnable(DV_ENABLE);
                    } else if ((mode.contains("LL_RGB_444_12BIT") || mode.contains("LL_RGB_444_10BIT"))) {
                        mDolbyVisionSettingManager.setDolbyVisionEnable(DV_LL_RGB);
                    }
                }
                setBestDolbyVision(true);
            } else {
                setBestDolbyVision(false);
            }

            mHandler.sendEmptyMessage(MSG_FRESH_UI);
        }
        return true;
    }

    @Override
    public int getMetricsCategory() {
        return 0;
    }

    private boolean isBestResolution() {
        return mOutputUiManager.isBestOutputmode();
    }
    private boolean isBestDolbyVsion() {
        return mOutputUiManager.isBestDolbyVsion();
    }

    /**
     * Taggle best resolution state.
     * if current best resolution state is enable, it will disable best resolution after method.
     * if current best resolution state is disable, it will enable best resolution after method.
     */
    private void setBestResolution() {
        mOutputUiManager.change2BestMode();
    }
    private void setBestDolbyVision(boolean enable) {
        mOutputUiManager.setBestDolbyVision(enable);
    }
    private String getCurrentDisplayMode() {
        return mOutputUiManager.getCurrentMode().trim();
    }
    private String getCurrentDeepColor() {
        String value = mOutputUiManager.getCurrentColorAttribute().toString().trim();
        if (value.equals("default") || value == "" || value.equals(""))
            return DEFAULT_VALUE;
        return value;
    }
    private boolean isHdmiMode() {
        return mOutputUiManager.isHdmiMode();
    }

    /**
     * show Alert Dialog to Users.
     * Tips: Users can confirm current state, or cancel to recover previous state.
     */
    private void showDialog () {
        if (mAlertDialog == null) {
            LayoutInflater inflater = (LayoutInflater)getActivity().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            view_dialog = inflater.inflate(R.layout.dialog_outputmode, null);

            AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
            mAlertDialog = builder.create();
            mAlertDialog.getWindow().setType(WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY);

            tx_title = (TextView)view_dialog.findViewById(R.id.dialog_title);
            tx_content = (TextView)view_dialog.findViewById(R.id.dialog_content);

            TextView button_cancel = (TextView)view_dialog.findViewById(R.id.dialog_cancel);
            button_cancel.setOnClickListener(this);

            TextView button_ok = (TextView)view_dialog.findViewById(R.id.dialog_ok);
            button_ok.setOnClickListener(this);
        }
        mAlertDialog.show();
        mAlertDialog.getWindow().setContentView(view_dialog);
        mAlertDialog.setCancelable(false);

        tx_content.setText(getResources().getString(R.string.device_outputmode_change)
            + " " +mOutputUiManager.getOutputmodeTitleList().get(mOutputUiManager.getCurrentModeIndex()));

        countdown = 15;
        if (timer == null)
            timer = new Timer();
        if (task != null)
            task.cancel();
        task = new DialogTimerTask();
        timer.schedule(task, 0, 1000);
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.dialog_cancel:
                if (mAlertDialog != null) {
                    mAlertDialog.dismiss();
                }
                recoverOutputMode();
                break;
            case R.id.dialog_ok:
                if (mAlertDialog != null) {
                    mAlertDialog.dismiss();
                }
                break;
        }
        task.cancel();
    }
    private class DialogTimerTask extends TimerTask {
        @Override
        public void run() {
            if (mHandler != null) {
                mHandler.sendEmptyMessage(MSG_COUNT_DOWN);
            }
        }
    };
}
