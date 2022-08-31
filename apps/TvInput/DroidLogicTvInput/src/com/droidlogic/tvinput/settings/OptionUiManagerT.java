/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.tvinput.settings;

import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.media.tv.TvContract;
import android.media.tv.TvInputInfo;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.text.Editable;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnFocusChangeListener;
import android.view.ViewGroup;
import android.widget.SimpleAdapter;
import android.widget.TextView;
import android.widget.Toast;
import android.content.IntentFilter;
import android.content.BroadcastReceiver;

import com.droidlogic.app.SystemControlManager;
import com.droidlogic.app.tv.ChannelInfo;
import com.droidlogic.app.tv.DroidLogicTvUtils;
import com.droidlogic.app.tv.TvChannelParams;
import com.droidlogic.app.tv.TvControlManager;
import com.droidlogic.app.tv.TvDataBaseManager;
import com.droidlogic.app.tv.TvMultilingualText;
import com.droidlogic.app.tv.TvScanConfig;
import com.droidlogic.app.DataProviderManager;
import com.droidlogic.tvinput.R;

import java.lang.StringBuilder;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

import vendor.amlogic.hardware.tvserver.V1_0.FreqList;

//import com.droidlogic.app.tv.TvControlManager.FreqList;

public class OptionUiManagerT implements  OnFocusChangeListener, TvControlManager.ScannerEventListener {
    public static final String TAG = "OptionUiManagerT";

    public static final int OPTION_PICTURE_MODE = 100;
    public static final int OPTION_BRIGHTNESS = 101;
    public static final int OPTION_CONTRAST = 102;
    public static final int OPTION_COLOR = 103;
    public static final int OPTION_SHARPNESS = 104;
    public static final int OPTION_BACKLIGHT = 105;
    public static final int OPTION_TINT = 106;
    public static final int OPTION_COLOR_TEMPERATURE = 107;
    public static final int OPTION_ASPECT_RATIO = 108;
    public static final int OPTION_DNR = 109;
    public static final int OPTION_3D_SETTINGS = 110;

    public static final int OPTION_SOUND_MODE = 200;
    public static final int OPTION_TREBLE = 201;
    public static final int OPTION_BASS = 202;
    public static final int OPTION_BALANCE = 203;
    public static final int OPTION_SPDIF = 204;
    public static final int OPTION_DIALOG_CLARITY = 205;
    public static final int OPTION_BASS_BOOST = 206;
    public static final int OPTION_SURROUND = 207;
    public static final int OPTION_VIRTUAL_SURROUND = 208;

    public static final int OPTION_AUDIO_TRACK = 300;
    public static final int OPTION_SOUND_CHANNEL = 301;
    public static final int OPTION_CHANNEL_INFO = 302;
    public static final int OPTION_COLOR_SYSTEM = 303;
    public static final int OPTION_SOUND_SYSTEM = 304;
    public static final int OPTION_VOLUME_COMPENSATE = 305;
    public static final int OPTION_FINE_TUNE = 306;
    public static final int OPTION_MANUAL_SEARCH = 307;
    public static final int OPTION_AUTO_SEARCH = 38;
    public static final int OPTION_CHANNEL_EDIT = 39;
    public static final int OPTION_SWITCH_CHANNEL = 310;
    public static final int OPTION_MTS = 311;

    public static final int OPTION_DTV_TYPE = 400;
    public static final int OPTION_SLEEP_TIMER = 401;
    public static final int OPTION_MENU_TIME = 402;
    public static final int OPTION_STARTUP_SETTING = 403;
    public static final int OPTION_DYNAMIC_BACKLIGHT = 404;
    public static final int OPTION_RESTORE_FACTORY = 405;
    public static final int OPTION_DEFAULT_LANGUAGE = 406;
    public static final int OPTION_SUBTITLE_SWITCH = 407;
    public static final int OPTION_HDMI20 = 408;
    public static final int OPTION_FBC_UPGRADE = 409;
    public static final int OPTION_AD_SWITCH = 410;
    public static final int OPTION_AD_MIX = 411;
    public static final int OPTION_AD_LIST = 412;

    public static final int ALPHA_NO_FOCUS = 230;
    public static final int ALPHA_FOCUSED = 255;

    public static final int ATV_MIN_KHZ = 42250;
    public static final int ATV_MAX_KHZ = 868250;

    private static final int PADDING_LEFT = 50;

    private static final int PROCCESS = 2;
    private static final int CHANNEL = 3;
    private static final int STATUS = 4;
    private static final int NUMBER_SEARCH_START = 5;
    private static final int FREQUENCY = 6;
    private static final int CHANNELNUMBER = 7;
    private static final int EXIT_SCAN = 8;
    private static final int SAVING = 9;

    private static final int EVENT_SCAN_END_PERCENT = 100;

    private Context mContext;
    private Resources mResources;
    private SettingsManager mSettingsManager;
    private TvControlManager mTvControlManager;
    private int optionTag = OPTION_PICTURE_MODE;
    private String optionKey = null;
    private int channelNumber = 0;//for setting show searched tv channelNumber
    private int dtvNumber = 0;//for setting show searched dtv channelNumber
    private int atvNumber = 0;//for setting show searched atv channelNumber
    private int radioNumber = 0;//for setting show searched radio channelNumber
    private int tvDisplayNumber = 0;//for db store TV channel's channel displayNumber
    private int radioDisplayNumber = 0;//for db store Radio channel's channel displayNumber
    List<ChannelInfo> mChannels = new ArrayList<ChannelInfo>();
    private SystemControlManager mSystemControlManager;
    private TvDataBaseManager mTvDataBaseManager;

    public static final int SEARCH_STOPPED = 0;
    public static final int SEARCH_RUNNING = 1;
    public static final int SEARCH_PAUSED = 2;
    private int isSearching = SEARCH_STOPPED;

    public static final int AUTO_SEARCH = 0;
    public static final int MANUAL_SEARCH = 1;
    private int searchType = AUTO_SEARCH;

    private Toast toast = null;

    private Handler mHandler;
    private boolean isLiveTvScaning = false;
    private String mLiveTvFrequencyFrom = "0";
    private String mLiveTvFrequencyTo = "0";
    private String DEFAULTSTARTFREQUENCY = "0";
    private String DEFAULTENDFREQUENCY = "868250";
    private boolean mSearchDtv = false;
    private boolean mSearchAtv = false;
    private int mAtsccMode = 0;
    private boolean mLiveTvManualSearch = false;
    private boolean mLiveTvAutoSearch = false;
    private int mScanStage = -1;

    /* config in file 'tvconfig.cfg' [atv.auto.scan.mode] */
    /* 0: freq table list sacn mode */
    /* 1: all band sacn mode */
    private int mATvAutoScanMode = 0;
    private static final String AUTO_QAM_FAST_SEARCH_PATH = "/sys/kernel/debug/demod/dvbc_channel_fast";
    private static final String AUTO_QAM_FAST_SEARCH_ON = "fast_search on";
    private static final String AUTO_QAM_FAST_SEARCH_OFF = "fast_search off";

    public boolean isSearching() {
        return (isSearching != SEARCH_STOPPED);
    }
    public boolean isManualSearching() {
        return (isSearching() && (searchType == MANUAL_SEARCH));
    }

   public OptionUiManagerT(Context context, Intent intent, boolean tvscan) {
        mContext = context;
        mResources = mContext.getResources();
        isLiveTvScaning = tvscan;
        mSettingsManager = new SettingsManager(mContext, intent);
        init();
    }

    public void init () {
        mTvControlManager = TvControlManager.getInstance();
        mTvControlManager.setScannerListener(this);
        mSystemControlManager = SystemControlManager.getInstance();
        mTvDataBaseManager = new TvDataBaseManager(mContext);
        registerChannelScanStartReceiver();
    }

    public int getOptionTag() {
        return optionTag;
    }

    public void setOptionListener(View view) {
        for (int i = 0; i < ((ViewGroup) view).getChildCount(); i++) {
            View child = ((ViewGroup) view).getChildAt(i);
            if (child != null && child.hasFocusable() && child instanceof TextView) {
                //child.setOnClickListener(this);
                child.setOnFocusChangeListener(this);
                //child.setOnKeyListener(this);
            }
        }
    }

    @Override
    public void onFocusChange(View v, boolean hasFocus) {
        if (v instanceof TextView) {
            if (hasFocus) {
                ((TextView) v).setTextColor(mResources.getColor(R.color.color_text_focused));
            } else
                ((TextView) v).setTextColor(mResources.getColor(R.color.color_text_item));
        }
    }

    private void saveSearchStatus (int searchState) {
        DataProviderManager.putIntValue(mContext, DroidLogicTvUtils.TV_SEARCHING_STATE, searchState);
    }

    private int getNumberSearchNeed () {
        return DataProviderManager.getIntValue(mContext, DroidLogicTvUtils.IS_NUMBER_SEARCH, -1);
    }

    private void stopSearch() {
        Log.d(TAG, "stopSearch");
        doScanCmd(DroidLogicTvUtils.ACTION_STOP_SCAN, null);
    }

    private void pauseSearch() {
        Log.d(TAG, "pauseSearch");
        doScanCmd(DroidLogicTvUtils.ACTION_ATV_PAUSE_SCAN, null);
        isSearching = SEARCH_PAUSED;
        saveSearchStatus(isSearching);
    }

    private void resumeSearch() {
        Log.d(TAG, "resumeSearch");
        doScanCmd(DroidLogicTvUtils.ACTION_ATV_RESUME_SCAN, null);
        isSearching = SEARCH_RUNNING;
        saveSearchStatus(isSearching);
    }

    private static final int STD = 1;
    private static final int LRC = 2;
    private static final int HRC = 3;
    private static final int AUTO = 4;
    private static final int ATSC_T = 5;
    private static final int ATSC_C_STD_SET = 0;
    private static final int ATSC_C_LRC_SET = 1;
    private static final int ATSC_C_HRC_SET = 2;
    private static final int ATSC_C_AUTO_SET = 3;
    private static final int DTV_TO_ATV = 5;
    private static final int SET_TO_MODE = 1;

    private void startManualSearchAccordingMode() {
        Log.d(TAG, "startManualSearchAccordingMode");
        String channel = mLiveTvFrequencyFrom;
        String channelend = mLiveTvFrequencyTo;

        Bundle bundle = new Bundle();
        TvControlManager.TvMode mode = new TvControlManager.TvMode(mSettingsManager.getDtvType());
        int frequency;
        int atvfrequency;
        int frequencyend;
        int atvfrequencyend;

        /* when use all band scan and only atv */
        if (mATvAutoScanMode == TvControlManager.ScanType.SCAN_ATV_AUTO_ALL_BAND && checkLiveTvAutoScanMode() == ScanType.SCAN_ONLY_ATV) {
            frequency = Integer.valueOf(channel) * 1000000;
            atvfrequency = Integer.valueOf(channel) * 1000000;
            frequencyend = Integer.valueOf(channelend) * 1000000;
            atvfrequencyend = Integer.valueOf(channelend) * 1000000;
        } else if (TvContract.Channels.TYPE_DVB_C.equals(mSettingsManager.getDtvType())
                && DroidLogicTvUtils.TV_SEARCH_MODE_NIT.equals(DataProviderManager.getStringValue(mContext, DroidLogicTvUtils.TV_SEARCH_MODE, ""))) {
            frequency = Integer.valueOf(channel) * 1000000;
            atvfrequency = Integer.valueOf(channel) * 1000000;
            frequencyend = Integer.valueOf(channelend) * 1000000;
            atvfrequencyend = Integer.valueOf(channelend) * 1000000;
        }else {
            frequency = getDvbFrequencyByPd(Integer.valueOf(channel));
            atvfrequency = getAtvFrequencyByPd(Integer.valueOf(channel));
            frequencyend = 0;
            atvfrequencyend = 0;
        }

        Log.d(TAG, "frequency dtv = " + frequency + " dtv end = " + frequencyend + ", atv = " + atvfrequency + ", atv end = " + atvfrequencyend);
        bundle.putInt(DroidLogicTvUtils.PARA_MANUAL_SCAN, frequency);
        bundle.putInt(DroidLogicTvUtils.PARA_MANUAL_SCAN_END, frequencyend > 0 ? frequencyend : frequency);
        bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA1, atvfrequency);
        bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA2, atvfrequencyend > 0 ? atvfrequencyend : atvfrequency);
        bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA3, mSettingsManager.getTvSearchTypeSys());
        bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA4, mSettingsManager.getTvSearchSoundSys());
        int autoscanmode = autoscanmode = checkLiveTvAutoScanMode();

        if (mSettingsManager.getDtvType().equals(TvContract.Channels.TYPE_ATSC_C)) {
            if (mAtsccMode == ATSC_C_STD_SET) {
                mode.setList(STD);
            } else if (mAtsccMode == ATSC_C_LRC_SET) {
                mode.setList(LRC);
            } else if (mAtsccMode == ATSC_C_HRC_SET) {
                mode.setList(HRC);
            } else {// mAtsccMode == ATSC_C_AUTO_SET
                mode.setList(AUTO);
            }
        } else if (mSettingsManager.getDtvType().equals(TvContract.Channels.TYPE_ATSC_T)) {
            mode.setList(ATSC_T);
        }

        switch (autoscanmode) {
        case ScanType.SCAN_ATV_DTV:
            Log.d(TAG, "MANUAL_SCAN_ATV_DTV");
            deleteOtherTypeAtvOrDtvChannels(mode, true, true);
            mode.setExt(mode.getExt() | 1);//mixed adtv
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_MODE, mode.getMode());
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_TYPE_DTV, TvControlManager.ScanType.SCAN_DTV_MANUAL);
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_TYPE_ATV, TvControlManager.ScanType.SCAN_ATV_FREQ);
        break;
        case ScanType.SCAN_ONLY_ATV:
            Log.d(TAG, "MANUAL_SCAN_ONLY_ATV");
            deleteOtherTypeAtvOrDtvChannels(mode, true, false);
            mode.setExt(mode.getExt() | 1);//mixed adtv
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_MODE, mode.getMode());
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_TYPE_DTV, TvControlManager.ScanType.SCAN_DTV_NONE);
            if (mATvAutoScanMode == TvControlManager.ScanType.SCAN_ATV_AUTO_ALL_BAND) {
                bundle.putInt(DroidLogicTvUtils.PARA_SCAN_TYPE_ATV, TvControlManager.ScanType.SCAN_ATV_MANUAL);
            } else {
                bundle.putInt(DroidLogicTvUtils.PARA_SCAN_TYPE_ATV, TvControlManager.ScanType.SCAN_ATV_FREQ);
            }
        break;
        case ScanType.SCAN_ONLY_DTV:
            Log.d(TAG, "MANUAL_SCAN_ONLY_DTV");
            deleteOtherTypeAtvOrDtvChannels(mode, false, true);
            mode.setExt(mode.getExt() | 1);//mixed adtv
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_MODE, mode.getMode());
            if (TvContract.Channels.TYPE_DVB_C.equals(mSettingsManager.getDtvType())
                    && DroidLogicTvUtils.TV_SEARCH_MODE_NIT.equals(DataProviderManager.getStringValue(mContext, DroidLogicTvUtils.TV_SEARCH_MODE, ""))) {
                //get more frequency from nit tables
                bundle.putInt(DroidLogicTvUtils.PARA_SCAN_TYPE_DTV, TvControlManager.ScanType.SCAN_DTV_AUTO);
            } else {
                bundle.putInt(DroidLogicTvUtils.PARA_SCAN_TYPE_DTV, TvControlManager.ScanType.SCAN_DTV_MANUAL);
            }
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_TYPE_ATV, TvControlManager.ScanType.SCAN_ATV_NONE);
        break;
        }
        bundle.putString(TvInputInfo.EXTRA_INPUT_ID,mSettingsManager.getInputId());
        mSettingsManager.sendBroadcastToTvapp(DroidLogicTvUtils.ACTION_DTV_MANUAL_SCAN, bundle);
        doScanCmdForAtscManual(bundle);
        isSearching = SEARCH_RUNNING;
        saveSearchStatus(isSearching);
        mSettingsManager.setActivityResult(DroidLogicTvUtils.RESULT_UPDATE);
    }

    private void doScanCmdForAtscManual (Bundle bundle) {
        mTvControlManager.DtvSetTextCoding("GB2312");
        int dtvMode = (bundle == null ? TvChannelParams.MODE_DTMB
                : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_MODE, TvChannelParams.MODE_DTMB));
        TvControlManager.TvMode tvMode = TvControlManager.TvMode.fromMode(dtvMode);
        if ((tvMode.getExt() & 1) != 0) {
            int atvScanType = (bundle == null ? TvControlManager.ScanType.SCAN_ATV_NONE
                    : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_TYPE_ATV, TvControlManager.ScanType.SCAN_ATV_NONE));
            int dtvScanType = (bundle == null ? TvControlManager.ScanType.SCAN_DTV_NONE
                    : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_TYPE_DTV, TvControlManager.ScanType.SCAN_DTV_NONE));
            int atvFreq1 = (bundle == null ? 0 : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_PARA1, 0));
            int atvFreq2 = (bundle == null ? 0 : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_PARA2, 0));
            int dtvFreq = (bundle == null ? 0 : bundle.getInt(DroidLogicTvUtils.PARA_MANUAL_SCAN, 0));
            int dtvFreqend = (bundle == null ? 0 : bundle.getInt(DroidLogicTvUtils.PARA_MANUAL_SCAN_END, 0));
            int atvVideoStd = (bundle == null ? TvControlManager.ATV_VIDEO_STD_AUTO
                    : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_PARA3, mSettingsManager.getTvSearchTypeSys()));
            int atvAudioStd = (bundle == null ? TvControlManager.ATV_AUDIO_STD_AUTO
                    : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_PARA4, mSettingsManager.getTvSearchSoundSys()));
            TvControlManager.FEParas fe = new TvControlManager.FEParas();
            fe.setMode(tvMode);
            fe.setVideoStd(atvVideoStd);
            fe.setAudioStd(atvAudioStd);
            if (TvContract.Channels.TYPE_DVB_C.equals(tvMode.toType())) {
                int qammode = DataProviderManager.getIntValue(mContext, DroidLogicTvUtils.TV_SEARCH_DVBC_QAM, DroidLogicTvUtils.TV_SEARCH_DVBC_QAM16);
                int symbol = 0;
                String mode = DataProviderManager.getStringValue(mContext, DroidLogicTvUtils.TV_SEARCH_MODE, "");
                if (qammode == DroidLogicTvUtils.TV_SEARCH_DVBC_QAMAUTO) {
                    mSystemControlManager.writeSysFs(AUTO_QAM_FAST_SEARCH_PATH, AUTO_QAM_FAST_SEARCH_ON);
                } else {
                    mSystemControlManager.writeSysFs(AUTO_QAM_FAST_SEARCH_PATH, AUTO_QAM_FAST_SEARCH_OFF);
                }
                fe.setModulation(qammode);
                if (DroidLogicTvUtils.TV_SEARCH_MODE_NIT.equals(mode)) {
                    symbol = DataProviderManager.getIntValue(mContext, DroidLogicTvUtils.TV_SEARCH_DVBC_SYMBOL_RATE, DroidLogicTvUtils.TV_SEARCH_DVBC_SYMBOL_RATE_DEFAULT);
                    if (symbol > 0) {
                       fe.setSymbolrate(symbol);
                    }
                }
                Log.d(TAG, "doScanCmdForAtscManual dvbc qam = " + qammode + ", symbol = " + symbol);
            }
            TvControlManager.ScanParas scan = new TvControlManager.ScanParas();
            if (atvScanType != TvControlManager.ScanType.SCAN_ATV_NONE) {
                //atvFreq1 = dtvFreq;// - 5750000;
                //atvFreq2 = dtvFreq;// + 5250000;
                scan.setAtvFrequency1(atvFreq1);
                scan.setAtvFrequency2(atvFreq2);
                scan.setMode(TvControlManager.ScanParas.MODE_ATV_DTV);
            } else {
                scan.setMode(TvControlManager.ScanParas.MODE_DTV_ATV);
            }

            scan.setAtvMode(atvScanType);
            scan.setDtvMode(dtvScanType);
            scan.setDtvFrequency1(dtvFreq);
            scan.setDtvFrequency2(dtvFreqend > 0 ? dtvFreqend : dtvFreq);
            if (checkLiveTvAutoScanMode() == ScanType.SCAN_ATV_DTV || checkLiveTvAutoScanMode() == ScanType.SCAN_ONLY_ATV) {
                scan.setAtvModifier(TvControlManager.FEParas.K_MODE, tvMode.setList(tvMode.getList() + DTV_TO_ATV).getMode());
            }

            if (mATvAutoScanMode == TvControlManager.ScanType.SCAN_ATV_AUTO_ALL_BAND) {
                scan.setProc(TvControlManager.ScanParas.SCAN_PROCMODE_AUTOPAUSE_ON_ATV_FOUND);
            }

            mTvControlManager.OpenDevForScan(DroidLogicTvUtils.OPEN_DEV_FOR_SCAN_DTV);
            mTvControlManager.TvScan(fe, scan);
        }
    }

    private void startManualSearch() {
        Log.d(TAG, "startManualSearch");
        ViewGroup parent = null;

        if (mSettingsManager.getCurentVirtualTvSource() == TvControlManager.SourceInput_Type.SOURCE_TYPE_ADTV) {
            Log.d(TAG, "ADTV");
            String channel = mLiveTvFrequencyFrom;

            Bundle bundle = new Bundle();
            TvControlManager.TvMode mode = new TvControlManager.TvMode(mSettingsManager.getDtvType());
            int frequency = getDvbFrequencyByPd(Integer.valueOf(channel));
            mode.setExt(mode.getExt() | 1);//mixed adtv
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_MODE, mode.getMode());
            bundle.putInt(DroidLogicTvUtils.PARA_MANUAL_SCAN, frequency);
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_TYPE_DTV, TvControlManager.ScanType.SCAN_DTV_MANUAL);
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_TYPE_ATV, TvControlManager.ScanType.SCAN_ATV_MANUAL);
            //bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA1, 0);
            //bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA2, 0);
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA3, mSettingsManager.getTvSearchTypeSys());
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA4, mSettingsManager.getTvSearchSoundSys());
            bundle.putString(TvInputInfo.EXTRA_INPUT_ID,mSettingsManager.getInputId());
            mSettingsManager.sendBroadcastToTvapp(DroidLogicTvUtils.ACTION_DTV_MANUAL_SCAN, bundle);
            doScanCmd(DroidLogicTvUtils.ACTION_DTV_MANUAL_SCAN, bundle);
            isSearching = SEARCH_RUNNING;
            saveSearchStatus(isSearching);
            mSettingsManager.setActivityResult(DroidLogicTvUtils.RESULT_UPDATE);
        }
        searchType = MANUAL_SEARCH;
    }

    private String parseChannelFrequency(double freq) {
        String frequency = mResources.getString(R.string.start_frequency);
        frequency += Double.toString(freq / (1000 * 1000)) + mResources.getString(R.string.mhz);
        return frequency;
    }

    private final int DTV = 0;
    private final int ATV = 1;

    private int getList(int adtv) {
        if (mSettingsManager.getCurentVirtualTvSource() == TvControlManager.SourceInput_Type.SOURCE_TYPE_ADTV) {
            if (mSettingsManager.getDtvType().equals(TvContract.Channels.TYPE_ATSC_C)
                || mSettingsManager.getDtvType().equals(TvContract.Channels.TYPE_ATSC_T)) {
                boolean isCable = mSettingsManager.getDtvType().equals(TvContract.Channels.TYPE_ATSC_C);
                int cableMode = mAtsccMode + SET_TO_MODE;//select atsc-c std lrc drc
                int autoscanmode = adtv;

                switch (autoscanmode) {
                    case ScanType.SCAN_ATV_DTV:
                    case ScanType.SCAN_ONLY_DTV:
                    if (isCable) {
                        switch (cableMode) {
                            case ScanType.CABLE_MODE_STANDARD: return STD;
                            case ScanType.CABLE_MODE_LRC:      return LRC;
                            case ScanType.CABLE_MODE_HRC:      return HRC;
                            case ScanType.CABLE_MODE_AUTO:     return AUTO;
                        }
                        return STD;
                    }
                    return ATSC_T;
                    case ScanType.SCAN_ONLY_ATV:
                    if (isCable) {
                         switch (cableMode) {
                            case ScanType.CABLE_MODE_STANDARD: return (STD + DTV_TO_ATV);
                            case ScanType.CABLE_MODE_LRC:      return (LRC + DTV_TO_ATV);
                            case ScanType.CABLE_MODE_HRC:      return (HRC + DTV_TO_ATV);
                            case ScanType.CABLE_MODE_AUTO:     return (AUTO + DTV_TO_ATV);
                        }
                        return (STD + DTV_TO_ATV);
                    }
                    return (ATSC_T + DTV_TO_ATV);
                }
            }
        }
        return 0;
    }

    private int getDvbFrequencyByPd(int pd_number) {
        TvControlManager.TvMode mode = new TvControlManager.TvMode(mSettingsManager.getDtvType());
        mode.setList(getList(DTV));
        Log.d(TAG, "[get dtv freq]type:"+mode.toType()+" use list:"+mode.getList());
        return getDvbFrequencyByPd(mode.getMode(), pd_number);
    }

    private int getAtvFrequencyByPd(int pd_number) {
        TvControlManager.TvMode mode = new TvControlManager.TvMode(mSettingsManager.getDtvType());
        mode.setList(getList(ATV));
        Log.d(TAG, "[get atv freq]type: " +mode.toType() + " use list:" + mode.getList());
        return getDvbFrequencyByPd(mode.getMode(), pd_number);
    }

    private int getDvbFrequencyByPd(int tvMode, int pdNumber) {
        ArrayList<FreqList> m_fList = mTvControlManager.DTVGetScanFreqList(tvMode);
        String type = TvControlManager.TvMode.fromMode(tvMode).toType();
        int size = m_fList.size();
        int the_freq = -1;
/*
        if (type.equals(TvContract.Channels.TYPE_ATSC_T)
            || type.equals(TvContract.Channels.TYPE_ATSC_C))
            pdNumber = pdNumber - 1;
*/
        if (pdNumber < 1)
            pdNumber = 1;

        for (int i = 0; i < size; i++) {
            if (pdNumber == m_fList.get(i).channelNum) {
                the_freq = m_fList.get(i).freq;
                break;
            }
        }
        Log.d(TAG, "pdNumber: " + pdNumber + ", the_freq: " + the_freq);
        return (the_freq < 0)? m_fList.get(0).freq : the_freq;
    }

    public ArrayList<HashMap<String, Object>> getSearchedDtvInfo (TvControlManager.ScannerEvent event) {
        ArrayList<HashMap<String, Object>> list =  new ArrayList<HashMap<String, Object>>();

        HashMap<String, Object> item = new HashMap<String, Object>();
        item.put(SettingsManager.STRING_NAME, mResources.getString(R.string.frequency_l) + ":");
        item.put(SettingsManager.STRING_STATUS, Double.toString(event.freq / (1000 * 1000)) +
                 mResources.getString(R.string.mhz));
        list.add(item);

        item = new HashMap<String, Object>();
        item.put(SettingsManager.STRING_NAME, mResources.getString(R.string.quality) + ":");
        item.put(SettingsManager.STRING_STATUS, event.quality + mResources.getString(R.string.db));
        list.add(item);

        item = new HashMap<String, Object>();
        item.put(SettingsManager.STRING_NAME, mResources.getString(R.string.strength) + ":");
        item.put(SettingsManager.STRING_STATUS, event.strength + "%");
        list.add(item);

        item = new HashMap<String, Object>();
        item.put(SettingsManager.STRING_NAME, mResources.getString(R.string.tv_channel) + ":");
        item.put(SettingsManager.STRING_STATUS, channelNumber);
        list.add(item);

        item = new HashMap<String, Object>();
        item.put(SettingsManager.STRING_NAME, mResources.getString(R.string.radio_channel) + ":");
        item.put(SettingsManager.STRING_STATUS, radioNumber);
        list.add(item);

        return list;
    }

    //add by ww
    private void startAutosearch() {
        Log.d(TAG, "startAutoSearch");
        TvControlManager.TvMode mode = new TvControlManager.TvMode(mSettingsManager.getDtvType());
        //mSettingsManager.sendBroadcastToTvapp("search_channel");
        if (mSettingsManager.getCurentVirtualTvSource() == TvControlManager.SourceInput_Type.SOURCE_TYPE_ADTV) {
            Log.d(TAG, "startAutosearch ADTV");

            deleteChannels(mode, true, true);

            Bundle bundle = new Bundle();
            int[] freqPair = new int[2];
            TvScanConfig.GetTvAtvMinMaxFreq(DroidLogicTvUtils.getCountry(mContext), freqPair);
            mode.setExt(mode.getExt() | 1);//mixed adtv
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_MODE, mode.getMode());
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_TYPE_DTV, TvControlManager.ScanType.SCAN_DTV_ALLBAND);
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_TYPE_ATV, TvControlManager.ScanType.SCAN_ATV_AUTO);
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA1, freqPair[0]);
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA2, freqPair[1]);
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA3, mSettingsManager.getTvSearchTypeSys());
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA4, mSettingsManager.getTvSearchSoundSys());
            bundle.putString(TvInputInfo.EXTRA_INPUT_ID,mSettingsManager.getInputId());
            mSettingsManager.sendBroadcastToTvapp(DroidLogicTvUtils.ACTION_DTV_AUTO_SCAN, bundle);
            doScanCmd(DroidLogicTvUtils.ACTION_DTV_AUTO_SCAN, bundle);  //ww
            DataProviderManager.putIntValue(mContext, DroidLogicTvUtils.TV_DTV_CHANNEL_INDEX, -1);
        }
        isSearching = SEARCH_RUNNING;
        saveSearchStatus(isSearching);
        searchType = AUTO_SEARCH;
        mSettingsManager.setActivityResult(DroidLogicTvUtils.RESULT_UPDATE);
    }

    private void deleteChannels(TvControlManager.TvMode mode, boolean deleteAtv, boolean deleteDtv) {
        Log.d(TAG, " delete mode:"+mode.getBase()+" atv:"+deleteAtv+" dtv:"+deleteDtv);

        if (deleteAtv) {
            mSettingsManager.deleteAtvOrDtvChannels(true);//delete all atv channels
        }
        if (deleteDtv) {
            mSettingsManager.deleteAtvOrDtvChannels(false);//delete all dtv channels
        }
    }

    private void deleteOtherTypeAtvOrDtvChannels(TvControlManager.TvMode mode, boolean deleteAtv, boolean deleteDtv) {
        Log.d(TAG, "other type delete mode:"+mode.getBase()+" atv:"+deleteAtv+" dtv:"+deleteDtv);
        if ((mode.getBase() == TvChannelParams.MODE_DTMB)
            || (mode.getBase() == TvChannelParams.MODE_ANALOG)
            || (mode.getBase() == TvChannelParams.MODE_ATSC)) {
            return;
        }
        if (deleteAtv) {
            mSettingsManager.deleteOtherTypeAtvOrDtvChannels(mode.toType(), true);//delete all other atv channels
        }
        if (deleteDtv) {
            mSettingsManager.deleteOtherTypeAtvOrDtvChannels(mode.toType(), false);//delete all other dtv channels
        }
    }

    private void startAutosearchAccrodingTvMode() {
        Log.d(TAG, "startAutosearchAccrodingTvMode");
        TvControlManager.TvMode mode = new TvControlManager.TvMode(mSettingsManager.getDtvType());
        //mSettingsManager.sendBroadcastToTvapp("search_channel");
        Bundle bundle = new Bundle();
        int[] freqPair = new int[2];
        int checkcablemode;
        int autoscanmode = autoscanmode = checkLiveTvAutoScanMode();

        String countryId = DroidLogicTvUtils.getCountry(mContext);
        mATvAutoScanMode = TvScanConfig.GetTvAtvStepScan(countryId);
        if (mATvAutoScanMode == TvControlManager.ScanType.SCAN_ATV_AUTO_ALL_BAND && autoscanmode == ScanType.SCAN_ONLY_ATV) {
            TvScanConfig.GetTvAtvMinMaxFreq(countryId, freqPair);
            Log.d(TAG, "freqPair[0] " + freqPair[0] + ", freqPair[1] " + freqPair[1]);
        }

        bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA1, freqPair[0]);
        bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA2, freqPair[1]);
        bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA3, mSettingsManager.getTvSearchTypeSys());
        bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA4, mSettingsManager.getTvSearchSoundSys());

        Log.d(TAG, "[startAutosearchAccrodingTvMode] autoscanmode = " + autoscanmode);
        switch (autoscanmode) {
            case ScanType.SCAN_ATV_DTV:
                Log.d(TAG, "ADTV");

                deleteChannels(mode, true, true);
                checkcablemode = mAtsccMode + 1;//mAtsccMode value change from 0~2

                if (mSettingsManager.getDtvType().equals(TvContract.Channels.TYPE_ATSC_C)) {
                    if (checkcablemode == ScanType.CABLE_MODE_STANDARD) {
                        mode.setList(STD);
                        bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA5, (STD + DTV_TO_ATV));
                    } else if (checkcablemode == ScanType.CABLE_MODE_LRC) {
                        mode.setList(LRC);
                        bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA5, (LRC + DTV_TO_ATV));
                    } else if (checkcablemode == ScanType.CABLE_MODE_HRC) {
                        mode.setList(HRC);
                        bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA5, (HRC + DTV_TO_ATV));
                    } else {
                        mode.setList(AUTO);
                        bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA5, (AUTO + DTV_TO_ATV));
                    }
                } else if (mSettingsManager.getDtvType().equals(TvContract.Channels.TYPE_ATSC_T)) {
                    mode.setList(ATSC_T);
                    bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA5, (ATSC_T + DTV_TO_ATV));
                }
                mode.setExt(mode.getExt() | 1);//mixed adtv
                bundle.putInt(DroidLogicTvUtils.PARA_SCAN_MODE, mode.getMode());
                bundle.putInt(DroidLogicTvUtils.PARA_SCAN_TYPE_DTV, TvControlManager.ScanType.SCAN_DTV_ALLBAND);
                if (mSettingsManager.getDtvType().equals(TvContract.Channels.TYPE_DTMB))
                    bundle.putInt(DroidLogicTvUtils.PARA_SCAN_TYPE_ATV, TvControlManager.ScanType.SCAN_ATV_AUTO);
                else
                    bundle.putInt(DroidLogicTvUtils.PARA_SCAN_TYPE_ATV, TvControlManager.ScanType.SCAN_ATV_FREQ);
            break;
            case ScanType.SCAN_ONLY_ATV:
                Log.d(TAG, "SOURCE_TYPE_TV");

                deleteChannels(mode, true, false);
                checkcablemode = mAtsccMode + 1;

                if (mSettingsManager.getDtvType().equals(TvContract.Channels.TYPE_ATSC_C)) {
                    if (checkcablemode == ScanType.CABLE_MODE_STANDARD) {
                        bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA5, (STD + DTV_TO_ATV));
                    } else if (checkcablemode == ScanType.CABLE_MODE_LRC) {
                        bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA5, (LRC + DTV_TO_ATV));
                    } else if (checkcablemode == ScanType.CABLE_MODE_HRC) {
                        bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA5, (HRC + DTV_TO_ATV));
                    } else {
                        bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA5, (AUTO + DTV_TO_ATV));
                    }
                } else if (mSettingsManager.getDtvType().equals(TvContract.Channels.TYPE_ATSC_T)) {
                    bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA5, (ATSC_T + DTV_TO_ATV));
                }

                mode.setExt(mode.getExt() | 1);//mixed adtv
                bundle.putInt(DroidLogicTvUtils.PARA_SCAN_MODE, mode.getMode());
                bundle.putInt(DroidLogicTvUtils.PARA_SCAN_TYPE_DTV, TvControlManager.ScanType.SCAN_DTV_NONE);
                if (mATvAutoScanMode == TvControlManager.ScanType.SCAN_ATV_AUTO_ALL_BAND)
                    bundle.putInt(DroidLogicTvUtils.PARA_SCAN_TYPE_ATV, TvControlManager.ScanType.SCAN_ATV_AUTO);
                else
                    bundle.putInt(DroidLogicTvUtils.PARA_SCAN_TYPE_ATV, TvControlManager.ScanType.SCAN_ATV_FREQ);
            break;
            case ScanType.SCAN_ONLY_DTV:
                Log.d(TAG, "SOURCE_TYPE_DTV");

                deleteChannels(mode, false, true);
                checkcablemode = mAtsccMode + 1;

                if (mSettingsManager.getDtvType().equals(TvContract.Channels.TYPE_ATSC_C)) {
                    if (checkcablemode == ScanType.CABLE_MODE_STANDARD) {
                        mode.setList(STD);
                    } else if (checkcablemode == ScanType.CABLE_MODE_LRC) {
                        mode.setList(LRC);
                    } else if (checkcablemode == ScanType.CABLE_MODE_HRC) {
                        mode.setList(HRC);
                    } else {
                        mode.setList(AUTO);
                    }
                } else if (mSettingsManager.getDtvType().equals(TvContract.Channels.TYPE_ATSC_T)) {
                    mode.setList(ATSC_T);
                }
                mode.setExt(mode.getExt() | 1);//mixed adtv
                bundle.putInt(DroidLogicTvUtils.PARA_SCAN_MODE, mode.getMode());
                bundle.putInt(DroidLogicTvUtils.PARA_SCAN_TYPE_DTV, TvControlManager.ScanType.SCAN_DTV_ALLBAND);
                bundle.putInt(DroidLogicTvUtils.PARA_SCAN_TYPE_ATV, TvControlManager.ScanType.SCAN_ATV_NONE);
            break;
        }
        bundle.putString(TvInputInfo.EXTRA_INPUT_ID,mSettingsManager.getInputId());
        mSettingsManager.sendBroadcastToTvapp(DroidLogicTvUtils.ACTION_DTV_AUTO_SCAN, bundle);
        doScanCmd(DroidLogicTvUtils.ACTION_DTV_AUTO_SCAN, bundle);  //ww
        DataProviderManager.putIntValue(mContext, DroidLogicTvUtils.TV_DTV_CHANNEL_INDEX, -1);
        isSearching = SEARCH_RUNNING;
        saveSearchStatus(isSearching);
        searchType = AUTO_SEARCH;
        mSettingsManager.setActivityResult(DroidLogicTvUtils.RESULT_UPDATE);
    }

    private void doScanCmd(String action, Bundle bundle) {
        Log.d(TAG, "doScanCmd:"+action);
        if (DroidLogicTvUtils.ACTION_ATV_AUTO_SCAN.equals(action)) {
            //ww//
            mTvControlManager.OpenDevForScan(DroidLogicTvUtils.OPEN_DEV_FOR_SCAN_ATV);
            mTvControlManager.AtvAutoScan(
                (bundle == null ? TvControlManager.ATV_VIDEO_STD_AUTO
                    : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_PARA1, mSettingsManager.getTvSearchTypeSys())),
                (bundle == null ? TvControlManager.ATV_AUDIO_STD_AUTO
                    : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_PARA2, mSettingsManager.getTvSearchSoundSys())),
                (bundle == null ? 0 : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_PARA3, 0)),
                (bundle == null ? 1 : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_PARA4, 1)));
        } else if (DroidLogicTvUtils.ACTION_ATV_MANUAL_SCAN.equals(action)) {
            if (bundle != null) {
                //ww//
                mTvControlManager.OpenDevForScan(DroidLogicTvUtils.OPEN_DEV_FOR_SCAN_ATV);
                mTvControlManager.AtvManualScan(
                    bundle.getInt(DroidLogicTvUtils.PARA_SCAN_PARA1, 0),
                    bundle.getInt(DroidLogicTvUtils.PARA_SCAN_PARA2, 0),
                    bundle.getInt(DroidLogicTvUtils.PARA_SCAN_PARA3, mSettingsManager.getTvSearchTypeSys()),
                    bundle.getInt(DroidLogicTvUtils.PARA_SCAN_PARA4, mSettingsManager.getTvSearchSoundSys()));
            }
        } else if (DroidLogicTvUtils.ACTION_DTV_AUTO_SCAN.equals(action)) {
            mTvControlManager.DtvSetTextCoding("GB2312");
            int dtvMode = (bundle == null ? TvChannelParams.MODE_DTMB
                    : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_MODE, TvChannelParams.MODE_DTMB));
            TvControlManager.TvMode tvMode = TvControlManager.TvMode.fromMode(dtvMode);
            if ((tvMode.getExt() & 1) != 0) {//ADTV
                int atvScanType = (bundle == null ? TvControlManager.ScanType.SCAN_ATV_NONE
                        : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_TYPE_ATV, TvControlManager.ScanType.SCAN_ATV_NONE));
                int dtvScanType = (bundle == null ? TvControlManager.ScanType.SCAN_DTV_NONE
                        : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_TYPE_DTV, TvControlManager.ScanType.SCAN_DTV_NONE));
                int atvFreq1 = (bundle == null ? 0 : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_PARA1, 0));
                int atvFreq2 = (bundle == null ? 0 : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_PARA2, 0));
                int dtvFreq = (bundle == null ? 0 : bundle.getInt(DroidLogicTvUtils.PARA_MANUAL_SCAN, 0));
                int atvVideoStd = (bundle == null ? TvControlManager.ATV_VIDEO_STD_AUTO
                        : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_PARA3, mSettingsManager.getTvSearchTypeSys()));
                int atvAudioStd = (bundle == null ? TvControlManager.ATV_AUDIO_STD_AUTO
                        : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_PARA4, mSettingsManager.getTvSearchSoundSys()));
                int atvList = (bundle == null ? tvMode.getList()
                        : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_PARA5, tvMode.getList()));
                TvControlManager.FEParas fe = new TvControlManager.FEParas();
                fe.setMode(tvMode);
                fe.setVideoStd(atvVideoStd);
                fe.setAudioStd(atvAudioStd);
                if (TvContract.Channels.TYPE_DVB_C.equals(tvMode.toType())) {
                    int qammode = DataProviderManager.getIntValue(mContext, DroidLogicTvUtils.TV_SEARCH_DVBC_QAM, DroidLogicTvUtils.TV_SEARCH_DVBC_QAM16);
                    if (qammode == DroidLogicTvUtils.TV_SEARCH_DVBC_QAMAUTO) {
                        mSystemControlManager.writeSysFs(AUTO_QAM_FAST_SEARCH_PATH, AUTO_QAM_FAST_SEARCH_ON);
                    } else {
                        mSystemControlManager.writeSysFs(AUTO_QAM_FAST_SEARCH_PATH, AUTO_QAM_FAST_SEARCH_OFF);
                    }
                    fe.setModulation(qammode);
                    Log.d(TAG, "doScanCmd dvbc qam = " + qammode);
                }
                TvControlManager.ScanParas scan = new TvControlManager.ScanParas();
                if (atvScanType != TvControlManager.ScanType.SCAN_ATV_NONE) {
                    scan.setMode(TvControlManager.ScanParas.MODE_ATV_DTV);
                } else {
                    scan.setMode(TvControlManager.ScanParas.MODE_DTV_ATV);
                }
                scan.setAtvMode(atvScanType);
                scan.setDtvMode(dtvScanType);
                scan.setAtvFrequency1(atvFreq1);
                scan.setAtvFrequency2(atvFreq2);
                scan.setDtvFrequency1(dtvFreq);
                scan.setDtvFrequency2(dtvFreq);
                scan.setAtvModifier(TvControlManager.FEParas.K_MODE,
                    TvControlManager.TvMode.fromMode(dtvMode).setList(atvList).getMode());
                //ww//
                mTvControlManager.OpenDevForScan(DroidLogicTvUtils.OPEN_DEV_FOR_SCAN_DTV);
                mTvControlManager.TvScan(fe, scan);
            } else {
                //ww//
                mTvControlManager.OpenDevForScan(DroidLogicTvUtils.OPEN_DEV_FOR_SCAN_DTV);
                mTvControlManager.DtvAutoScan(dtvMode);
            }
        } else if (DroidLogicTvUtils.ACTION_DTV_MANUAL_SCAN.equals(action)) {
            mTvControlManager.DtvSetTextCoding("GB2312");
            int dtvMode = (bundle == null ? TvChannelParams.MODE_DTMB
                    : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_MODE, TvChannelParams.MODE_DTMB));
            TvControlManager.TvMode tvMode = TvControlManager.TvMode.fromMode(dtvMode);
            if ((tvMode.getExt() & 1) != 0) {//ADTV
                int atvScanType = (bundle == null ? TvControlManager.ScanType.SCAN_ATV_NONE
                        : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_TYPE_ATV, TvControlManager.ScanType.SCAN_ATV_NONE));
                int dtvScanType = (bundle == null ? TvControlManager.ScanType.SCAN_DTV_NONE
                        : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_TYPE_DTV, TvControlManager.ScanType.SCAN_DTV_NONE));
                int atvFreq1 = (bundle == null ? 0 : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_PARA1, 0));
                int atvFreq2 = (bundle == null ? 0 : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_PARA2, 0));
                int dtvFreq = (bundle == null ? 0 : bundle.getInt(DroidLogicTvUtils.PARA_MANUAL_SCAN, 0));
                int atvVideoStd = (bundle == null ? TvControlManager.ATV_VIDEO_STD_AUTO
                        : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_PARA3, mSettingsManager.getTvSearchTypeSys()));
                int atvAudioStd = (bundle == null ? TvControlManager.ATV_AUDIO_STD_AUTO
                        : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_PARA4, mSettingsManager.getTvSearchSoundSys()));

                if (atvScanType != TvControlManager.ScanType.SCAN_ATV_NONE) {
                    atvFreq1 = dtvFreq - 9750000;
                    atvFreq2 = dtvFreq + 1250000;
                }
                TvControlManager.FEParas fe = new TvControlManager.FEParas();
                fe.setMode(tvMode);
                fe.setVideoStd(atvVideoStd);
                fe.setAudioStd(atvAudioStd);
                TvControlManager.ScanParas scan = new TvControlManager.ScanParas();
                if (atvScanType != TvControlManager.ScanType.SCAN_ATV_NONE) {
                    scan.setMode(TvControlManager.ScanParas.MODE_ATV_DTV);
                } else {
                    scan.setMode(TvControlManager.ScanParas.MODE_DTV_ATV);
                }
                scan.setAtvMode(atvScanType);
                scan.setDtvMode(dtvScanType);
                scan.setAtvFrequency1(atvFreq1);
                scan.setAtvFrequency2(atvFreq2);
                scan.setDtvFrequency1(dtvFreq);
                scan.setDtvFrequency2(dtvFreq);
                //scan.setDtvStandard(TvControlManager.ScanParas.DTVSTD_ATSC);
                //ww//
                mTvControlManager.OpenDevForScan(DroidLogicTvUtils.OPEN_DEV_FOR_SCAN_DTV);
                mTvControlManager.TvScan(fe, scan);
            } else {
            //ww//
            mTvControlManager.OpenDevForScan(DroidLogicTvUtils.OPEN_DEV_FOR_SCAN_DTV);
                mTvControlManager.DtvManualScan(
                    bundle.getInt(DroidLogicTvUtils.PARA_SCAN_MODE, TvChannelParams.MODE_DTMB),
                    bundle.getInt(DroidLogicTvUtils.PARA_MANUAL_SCAN, 0)
                );
            }
        } else if (DroidLogicTvUtils.ACTION_STOP_SCAN.equals(action)) {
            //ww//
            mTvControlManager.OpenDevForScan(DroidLogicTvUtils.CLOSE_DEV_FOR_SCAN);
            mTvControlManager.DtvStopScan();
        } else if (DroidLogicTvUtils.ACTION_ATV_PAUSE_SCAN.equals(action)) {
            mTvControlManager.AtvDtvPauseScan();
        } else if (DroidLogicTvUtils.ACTION_ATV_RESUME_SCAN.equals(action)) {
            mTvControlManager.AtvDtvResumeScan();
        }
    }

    private String parseFrequencyBand(double freq) {
        String band = "";
        if (freq > 44.25 && freq < 143.25) {
            band = "VHFL";
        } else if (freq >= 143.25 && freq < 426.25) {
            band = "VHFH";
        } else if (freq >= 426.25 && freq < 868.25) {
            band = "UHF";
        }
        return band;
    }

    private int getIntegerFromString(String str) {
        if (str != null && str.contains("%"))
            return Integer.valueOf(str.replaceAll("%", ""));
        else
            return -1;
    }

    @Override
    public void onEvent(TvControlManager.ScannerEvent event) {
        ChannelInfo channel = null;
        String name = null;

        if (!isSearching())
            return;

        switch (event.type) {
            case TvControlManager.EVENT_SCAN_PROGRESS:
                int isNewProgram = 0;
                Log.d(TAG, "onEvent:"+event.precent + "%\tfreq[" + event.freq + "] lock[" + event.lock + "] strength[" + event.strength + "] quality[" + event.quality + "]");

                if (((event.mode & 0xff) == TvChannelParams.MODE_ANALOG) && (event.lock == 0x11)) { //trick here
                    isNewProgram = 1;
                    String mode = DataProviderManager.getStringValue(mContext, "tv_search_mode", "");
                    if ((mTvControlManager.AtvDtvGetScanStatus() & TvControlManager.ATV_DTV_SCAN_STATUS_PAUSED)
                            == TvControlManager.ATV_DTV_SCAN_STATUS_PAUSED) {
                        if (mATvAutoScanMode == TvControlManager.ScanType.SCAN_ATV_AUTO_ALL_BAND && mode.equals("manual")
                                && checkLiveTvAutoScanMode() == ScanType.SCAN_ONLY_ATV) {
                            isSearching = SEARCH_PAUSED;
                            sendMessage(STATUS, -1, "pause");
                        } else {
                            Log.d(TAG, "Resume Scanning");
                            resumeSearch();
                        }
                    }
                } else if (((event.mode & 0xff)  != TvChannelParams.MODE_ANALOG) && (event.programName.length() != 0)) {
                    try {
                        name = TvMultilingualText.getText(event.programName);
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                    isNewProgram = 2;
                }

                if (isNewProgram == 1) {
                    channelNumber++;
                    atvNumber++;
                    sendMessage(CHANNELNUMBER, atvNumber, DroidLogicTvUtils.ATVNUMBER);
                    sendMessage(FREQUENCY, event.freq, DroidLogicTvUtils.ATVPROGRAM);//send found frequency
                    Log.d(TAG, "New ATV Program");
                } else if (isNewProgram == 2) {
                    if (event.srvType == 1) {
                        channelNumber++;
                        dtvNumber++;
                        sendMessage(CHANNELNUMBER, dtvNumber, DroidLogicTvUtils.DTVNUMBER);
                        sendMessage(FREQUENCY, event.freq, DroidLogicTvUtils.DTVPROGRAM);//send found frequency
                    } else if (event.srvType == 2) {
                        radioNumber++;
                        sendMessage(CHANNELNUMBER, radioNumber, DroidLogicTvUtils.RADIONUMBER);
                        sendMessage(FREQUENCY, event.freq, DroidLogicTvUtils.RADIOPROGRAM);//send found frequency
                    }
                    Log.d(TAG, "New DTV Program : [" + name + "] type[" + event.srvType + "]");
                }

                if (isLiveTvScaning) {
                    sendMessage(PROCCESS, event.precent, null);
                    if (mLiveTvManualSearch) {
                        setLiveTvManualSearchInfo(event);
                    } else if (mLiveTvAutoSearch) {
                        setLiveTvAutoSearchFrequency(event);
                    }
                }

                if (((event.mode & 0xff)  == TvChannelParams.MODE_ANALOG) && ((optionTag == OPTION_MANUAL_SEARCH) || isLiveTvScaning)
                    && event.precent == 100)
                    stopSearch();
                break;

            case TvControlManager.EVENT_STORE_BEGIN:
                Log.d(TAG, "onEvent:Store begin");
                mScanStage = TvControlManager.EVENT_STORE_BEGIN;
                break;

            case TvControlManager.EVENT_STORE_END:
                Log.d(TAG, "onEvent:Store end");
                mScanStage = TvControlManager.EVENT_STORE_END;
                String prompt = mResources.getString(R.string.searched);
                if (channelNumber != 0) {
                    prompt += " " + channelNumber + " " + mResources.getString(R.string.tv_channel);
                    if (radioNumber != 0) {
                        prompt += ",";
                    }
                }
                if (radioNumber != 0) {
                    prompt += " " + radioNumber + " " + mResources.getString(R.string.radio_channel);
                }
                //this log print for auto test
                Log.d("TvStoreManager", "video channel number =" + channelNumber + " radio channel number =" + radioNumber);
                showToast(prompt);
                break;

            case TvControlManager.EVENT_SCAN_END:
                Log.d(TAG, "onEvent:Scan end");
                mScanStage = TvControlManager.EVENT_SCAN_END;
                sendMessage(PROCCESS, EVENT_SCAN_END_PERCENT, null);
                stopSearch();
                break;

            case TvControlManager.EVENT_SCAN_EXIT:
                Log.d(TAG, "onEvent:Scan exit.");
                mScanStage = TvControlManager.EVENT_SCAN_EXIT;
                //exit finally after receive store status and show update channels to db for this stage
                mSystemControlManager.setProperty("tv.channels.count", ""+(channelNumber+radioNumber));
                mSystemControlManager.writeSysFs(AUTO_ATSC_C_PATH, AUTO_ATSC_C_MODE_DISABLE);//disable auto select std lrc hrc
                isSearching = SEARCH_STOPPED;
                saveSearchStatus(isSearching);
                if (channelNumber == 0 && radioNumber == 0) {
                    showToast(mResources.getString(R.string.searched) + " 0 " + mResources.getString(R.string.channel));
                }
                if (isLiveTvScaning) {
                    if (getNumberSearchNeed() != 1) {
                        //Log.d(TAG, "=====not number search: EXIT_SCAN");
                        //sendMessage(EXIT_SCAN, -1, null);
                        Log.d(TAG, "=====not number search: SAVING");
                        sendMessage(SAVING, EXIT_SCAN, mResources.getString(R.string.tv_search_channel_saving));
                    } else {
                        //Log.d(TAG, "=====number search: exit");
                        //sendMessage(STATUS, -1, "exit");
                        Log.d(TAG, "=====number search: SAVING");
                        sendMessage(SAVING, STATUS, mResources.getString(R.string.tv_search_channel_saving));
                    }
                }
                break;
            default:
                break;
        }
    }

    private BroadcastReceiver mChannelScanStoreStatusReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (intent != null) {
                if (mScanStage != TvControlManager.EVENT_SCAN_EXIT) {
                    Log.d(TAG, "mChannelScanStoreStatusReceiver skip invalid EVENT_SCAN_EXIT status");
                    return;
                }
                int status = intent.getIntExtra(DroidLogicTvUtils.ACTION_STORE_CHANNEL_STATUS, -1);
                excuteChannelScanStoreStatusByThread(status);
                abortBroadcast();
            }
        }
    };

    private void excuteChannelScanStoreStatusByThread(final int status) {
        if (mHandler != null) {
            mHandler.post(new Runnable() {
                @Override
                    public void run() {
                        switch (status) {
                            case TvControlManager.EVENT_SCAN_EXIT:
                                Log.d(TAG, "excuteChannelScanStoreStatusByThread Scan exit");
                                if (isLiveTvScaning) {
                                    if (getNumberSearchNeed() != 1) {
                                        Log.d(TAG, "=====not number search: EXIT_SCAN");
                                        sendMessage(EXIT_SCAN, SAVING, null);
                                    } else {
                                        Log.d(TAG, "=====number search: exit");
                                        sendMessage(STATUS, -1, "exit");
                                    }
                                }
                                break;
                        }
                    }
            });
        }
    }

    private void registerChannelScanStartReceiver() {
        IntentFilter filter= new IntentFilter();
        filter.addAction(DroidLogicTvUtils.ACTION_STORE_CHANNEL_STATUS);
        mContext.registerReceiver(mChannelScanStoreStatusReceiver, filter);
    }

    private void unRegisterChannelScanStartReceiver() {
        mContext.unregisterReceiver(mChannelScanStoreStatusReceiver);
    }

    private void showToast(String text) {
        LayoutInflater inflater = (LayoutInflater)mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View layout = inflater.inflate(R.layout.layout_toast, null);

        TextView propmt = (TextView)layout.findViewById(R.id.toast_content);
        propmt.setText(text);

        toast = new Toast(mContext);
        toast.setDuration(Toast.LENGTH_SHORT);
        toast.setView(layout);
        toast.show();
    }

    public void release() {
        Log.d(TAG,"release");
        mTvControlManager.setScannerListener(null);
        mHandler.removeCallbacksAndMessages(null);
        unRegisterChannelScanStartReceiver();
        mHandler = null;
        mSettingsManager = null;
        mTvControlManager = null;
    }

    public void setHandler (Handler handler) {
        mHandler = handler;
    }

    public void setFrequency (String value1, String value2) {
        Log.d(TAG, "[setFrequency] value1 = " + value1 + ", value2 = " + value2);
        mLiveTvFrequencyFrom = value1;
        mLiveTvFrequencyTo = value2;
    }

    //return startdtv enddtv startatv endatv
    private int[] getStartAndEndFrequencyForDtmb(int startfre, int endfre) {
        int[] result = new int[4];
        final int atvoffset = 2000 * 1000;//hz
        final int dtvoffset = 150 * 1000;//hz
        if (endfre > startfre) {
            //dtv
            if (startfre * 1000 * 1000 > dtvoffset) {
                result[0] = startfre * 1000 * 1000 - dtvoffset;
            } else {
                result[0] = 0;
            }
            result[1] = endfre * 1000 * 1000 + dtvoffset;
            //atv
            if (startfre * 1000 * 1000 > atvoffset) {
                result[2] = startfre * 1000 * 1000 - atvoffset;
            } else {
                result[2] = 0;
            }
            result[3] = endfre * 1000 * 1000 + atvoffset;
        } else {
            //dtv
            if (endfre * 1000 * 1000 > dtvoffset) {
                result[0] = endfre * 1000 * 1000 - dtvoffset;
            } else {
                result[0] = 0;
            }
            result[1] = startfre + dtvoffset;
            //atv
            if (startfre * 1000 * 1000 > atvoffset) {
                result[2] = startfre * 1000 * 1000 - atvoffset;
            } else {
                result[2] = 0;
            }
            result[3] = endfre * 1000 * 1000 + atvoffset;
        }
        return result;
    }

    public void setSearchSys (boolean value1, boolean value2) {
        Log.d(TAG, "[setSearchSys] value1 = " + value1 + ", value2 = " + value2);
        mSearchDtv = value1;
        mSearchAtv = value2;
    }

    public void setAtsccSearchSys (int value) {
        Log.d(TAG, "[setAtsccSearchSys] value = " + value);
        mAtsccMode = value;
    }

    private void sendMessage(int type, int message, Object information) {
        if (mHandler == null) {
            return;
        }
        Message msg = new Message();
        msg.arg1 = type;
        msg.what = message;
        msg.obj = information;
        mHandler.sendMessage(msg);
    }

    private static final String AUTO_ATSC_C_PATH = "/sys/module/dtvdemod/parameters/auto_search_std";
    private static final String AUTO_ATSC_C_MODE_ENABLE = "1";
    private static final String AUTO_ATSC_C_MODE_DISABLE = "0";
    private static final String AUTO_ATSC_C_ATV_PATH = "/sys/kernel/debug/aml_atvdemod/slow_mode";

    public void callManualSearch () {
        mLiveTvManualSearch = true;
        mLiveTvAutoSearch = false;
        mATvAutoScanMode = TvScanConfig.GetTvAtvStepScan(DroidLogicTvUtils.getCountry(mContext));
        if (TvContract.Channels.TYPE_ATSC_C.equals(mSettingsManager.getDtvType()) && mAtsccMode == ATSC_C_AUTO_SET) {
            mSystemControlManager.writeSysFs(AUTO_ATSC_C_PATH, AUTO_ATSC_C_MODE_ENABLE);
            mSystemControlManager.writeSysFs(AUTO_ATSC_C_ATV_PATH, AUTO_ATSC_C_MODE_ENABLE);
        } else {
            mSystemControlManager.writeSysFs(AUTO_ATSC_C_PATH, AUTO_ATSC_C_MODE_DISABLE);
            mSystemControlManager.writeSysFs(AUTO_ATSC_C_ATV_PATH, AUTO_ATSC_C_MODE_DISABLE);
        }

        if (isSearching == SEARCH_STOPPED) {
            if (mSettingsManager.getCurentVirtualTvSource() == TvControlManager.SourceInput_Type.SOURCE_TYPE_ADTV) {
                startManualSearchAccordingMode();
            } else {
                startManualSearch();
            }
        } else if (isSearching == SEARCH_RUNNING) {//searching running
            pauseSearch();
            //sendMessage(ChannelSearchActivity.STATUS, -1, mContext.getResources().getString(R.string.pause_search));
             return ;
        } else {//searching paused
            resumeSearch();
            //sendMessage(ChannelSearchActivity.STATUS, -1, mContext.getResources().getString(R.string.resume_search));
            return;
        }
    }

    public void callAutosearch () {
        mLiveTvManualSearch = false;
        mLiveTvAutoSearch = true;
        mSystemControlManager.writeSysFs(AUTO_ATSC_C_PATH, AUTO_ATSC_C_MODE_DISABLE);
        mSystemControlManager.writeSysFs(AUTO_ATSC_C_ATV_PATH, AUTO_ATSC_C_MODE_DISABLE);
        if (isSearching == SEARCH_STOPPED) {
            if (mSettingsManager.getCurentVirtualTvSource() == TvControlManager.SourceInput_Type.SOURCE_TYPE_ADTV) {
                startAutosearchAccrodingTvMode();
            }else {
                startAutosearch();
            }
            //sendMessage(ChannelSearchActivity.STATUS, -1, mContext.getResources().getString(R.string.resume_search));
        } else if (isSearching == SEARCH_RUNNING) {//searching running
            pauseSearch();
            //sendMessage(ChannelSearchActivity.STATUS, -1, mContext.getResources().getString(R.string.pause_search));
             return ;
        } else {//searching paused
            resumeSearch();
            //sendMessage(ChannelSearchActivity.STATUS, -1, mContext.getResources().getString(R.string.resume_search));
            return;
        }
    }

    private int checkLiveTvAutoScanMode() {
        Log.d(TAG," TvAutoScanMode mSearchDtv = " + mSearchDtv + ", mSearchAtv = " + mSearchAtv);
        if (mSearchDtv && !mSearchAtv) {
            return ScanType.SCAN_ONLY_DTV;
        } else if (!mSearchDtv && mSearchAtv){
            return ScanType.SCAN_ONLY_ATV;
        }else if (mSearchDtv && mSearchAtv){
            return ScanType.SCAN_ATV_DTV;
        }else {
            return ScanType.SCAN_FAULT;
        }
    }

    private String getEventParams (int frequency, int quality, int strength, int channelNumber, int radioNumber) {
        StringBuilder sb = new StringBuilder(String.valueOf(frequency));
        sb.append("-").append(String.valueOf(quality)).append("-").append(String.valueOf(strength)).append("-").append(String.valueOf(channelNumber)).append("-").append(String.valueOf(radioNumber));
        return sb.toString();
    }

    private void setLiveTvManualSearchInfo(TvControlManager.ScannerEvent event) {
        Log.d(TAG, "[setLiveTvManualSearchInfo] mSettingsManager.getCurentVirtualTvSource()=" + mSettingsManager.getCurentVirtualTvSource());
        if (event == null)
            return;
        String eventParams = getEventParams(event.freq, event.quality, event.strength, channelNumber, radioNumber);
        if (mSettingsManager.getCurentVirtualTvSource() == TvControlManager.SourceInput_Type.SOURCE_TYPE_ADTV) {
            sendMessage(STATUS, channelNumber, null);//send searched channelnumber
            sendMessage(CHANNEL, 0, eventParams);
        }
    }

    private void setLiveTvAutoSearchFrequency(TvControlManager.ScannerEvent event) {
        if (mSettingsManager.getCurentVirtualTvSource() == TvControlManager.SourceInput_Type.SOURCE_TYPE_ADTV) {
            sendMessage(CHANNEL, 0, getEventParams(event.freq, event.quality, event.strength, channelNumber, radioNumber));
        }
    }

/************************mSettingsManager interface start****************************************8*/
    public TvControlManager.SourceInput_Type getCurentVirtualTvSource () {
        return mSettingsManager.getCurentVirtualTvSource();
    }

    public TvControlManager.SourceInput_Type getCurentTvSource () {
        return mSettingsManager.getCurentTvSource();
    }

    public void startTvPlayAndSetSourceInput() {
            mSettingsManager.startTvPlayAndSetSourceInput();
    }

    public int getActivityResult() {
        if (mSettingsManager != null)
            return mSettingsManager.getActivityResult();
        else
            return -1;
    }

    public static final int SET_DTMB = 0;
    public static final int SET_DVB_C = 1;
    public static final int SET_DVB_T = 2;
    public static final int SET_DVB_T2 = 3;
    public static final int SET_ATSC_T = 4;
    public static final int SET_ATSC_C= 5;
    public static final int SET_ISDB_T = 6;

    public int getDtvTypeStatus() {
        String type = mSettingsManager.getDtvType();
        int ret = SET_ATSC_T;
        if (TextUtils.equals(type, TvContract.Channels.TYPE_DTMB)) {
                ret = SET_DTMB;
        } else if (TextUtils.equals(type, TvContract.Channels.TYPE_DVB_C)) {
                ret = SET_DVB_C;
        } else if (TextUtils.equals(type, TvContract.Channels.TYPE_DVB_T)) {
                ret = SET_DVB_T;
        } else if (TextUtils.equals(type, TvContract.Channels.TYPE_DVB_T2)) {
                ret = SET_DVB_T2;
        } else if (TextUtils.equals(type, TvContract.Channels.TYPE_ATSC_T)) {
                ret = SET_ATSC_T;
        } else if (TextUtils.equals(type, TvContract.Channels.TYPE_ATSC_C)) {
                ret = SET_ATSC_C;
        } else if (TextUtils.equals(type, TvContract.Channels.TYPE_ISDB_T)) {
                ret = SET_ISDB_T;
        }
        return ret;
    }

    public void setDtvType(int value) {
        String type = null;
        switch (value) {
            case SET_DTMB:
                type = TvContract.Channels.TYPE_DTMB;
                break;
            case SET_DVB_C:
                type = TvContract.Channels.TYPE_DVB_C;
                break;
            case SET_DVB_T:
                type = TvContract.Channels.TYPE_DVB_T;
                break;
            case SET_DVB_T2:
                type = TvContract.Channels.TYPE_DVB_T2;
                break;
            case SET_ATSC_T:
                type = TvContract.Channels.TYPE_ATSC_T;
                break;
            case SET_ATSC_C:
                type = TvContract.Channels.TYPE_ATSC_C;
                break;
            case SET_ISDB_T:
                type = TvContract.Channels.TYPE_ISDB_T;
                break;
        }
        if (type != null) {
            mSettingsManager.setDtvType(type);
        }
    }

    public void setDtvType(String type) {
        if (type != null) {
            mSettingsManager.setDtvType(type);
        }
    }

    public String getDtvType() {
        return mSettingsManager.getDtvType();
    }

    public String getTVSupportCountries() {
        return mTvControlManager.GetTVSupportCountries();
    }

    public void SetTvCountry(String country) {
        mTvControlManager.SetTvCountry(country);
    }

    public String getDefaultDtvType() {
        return mSettingsManager.getDefaultDtvType();
    }

    private boolean mNumberSearchNeed = false;

    public void setNumberSearchNeed(boolean need) {
        mNumberSearchNeed = need;
    }

/************************mSettingsManager interface end****************************************8*/

/************************mTvControlManager interface start****************************************8*/
    public void DtvStopScan() {
        mTvControlManager.DtvStopScan();
    }

    public void StopTv() {
        mTvControlManager.StopTv();
    }

/************************mTvControlManager interface start****************************************8*/
}
