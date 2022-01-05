/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.tvinput.services;

import android.content.Context;
import android.content.BroadcastReceiver;
import android.content.IntentFilter;
import android.content.Intent;
import android.hardware.hdmi.HdmiControlManager;
import android.hardware.hdmi.HdmiTvClient;
import android.hardware.hdmi.HdmiDeviceInfo;
import android.hardware.hdmi.HdmiTvClient.SelectCallback;
import android.media.tv.TvContentRating;
import android.media.tv.TvInputManager;
import android.media.tv.TvInputService;
import android.media.tv.TvInputInfo;
import android.media.tv.TvStreamConfig;
import android.media.tv.TvInputManager.Hardware;
import android.provider.Settings;
import android.provider.Settings.Global;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.os.SystemClock;
import android.os.SystemProperties;
import android.text.TextUtils;
import android.util.Log;
import android.view.KeyEvent;
import android.view.Surface;
import android.view.View;
import android.view.LayoutInflater;

import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import com.droidlogic.app.AudioSystemCmdManager;
import com.droidlogic.app.DataProviderManager;
import com.droidlogic.app.SystemControlManager;
import com.droidlogic.app.SystemControlEvent;
import com.droidlogic.app.tv.DroidLogicHdmiCecManager;
import com.droidlogic.app.tv.DroidLogicTvUtils;
import com.droidlogic.app.tv.TvControlManager;

import com.droidlogic.tvinput.widget.DroidLogicOverlayView;

public abstract class TvInputBaseSession extends TvInputService.Session implements Handler.Callback,
    SystemControlEvent.HdrInfoListener {
    private static final boolean DEBUG = true;
    private static final String TAG = "TvInputBaseSession";

    private static final int SESSION_CTEATED = 0;
    private static final int SESSION_RELEASED = 1;

    private static final int    MSG_REGISTER_BROADCAST = 8;
    private static final int    MSG_DO_PRI_CMD              = 9;
    protected static final int  MSG_SUBTITLE_SHOW           = 10;
    protected static final int  MSG_SUBTITLE_HIDE           = 11;
    protected static final int  MSG_DO_RELEASE              = 12;
    protected static final int  MSG_AUDIO_MUTE              = 13;
    protected static final int  MSG_IMAGETEXT_SET           = 14;
    protected static final int  MSG_IMAGETEXT_RESET         = 15;

    ////add for get hdmi info
    protected static final int MSG_UPDATE_HDMI_HDR = 30;
    protected static final int MSG_UPDATE_HDMI_AUDIO_FORMAT = 31;
    protected static final int MSG_CLEAR_INFO = 32;
    protected static final int MSG_DELAY_PERIOD = 2000;//2s

    //msg to show dolby vision icon
    protected static final int MSG_SHOW_DOLBY_VSION = 40;
    protected static final int CHECK_DOLBY_VISION_MAX_COUNT = 5;
    protected static final int MSG_DISPLAY_PERIOD = 3000;

    protected static final int TVINPUT_BASE_DELAY_SEND_MSG  = 10; // Filter message within 10ms, only the last message is processed
    private Context mContext;
    public int mId;
    private String mInputId;
    private int mDeviceId;
    private AudioSystemCmdManager mAudioSystemCmdManager;
    private TvInputManager mTvInputManager;
    private boolean mHasRetuned = false;
    protected Handler mSessionHandler;
    private SystemControlManager mSystemControlManager = null;
    private SystemControlEvent mSystemControlEvent;
    private TvControlManager mTvControlManager;
    protected DroidLogicOverlayView mOverlayView = null;

    protected boolean isBlockNoRatingEnable = false;
    protected boolean isUnlockCurrent_NR = false;
    DroidLogicHdmiCecManager mDroidLogicHdmiCecManager = null;
    public boolean isSurfaceAlive = true;

    private boolean isHdmiDevice = false;
    private String mHdmiHdrInfo = null;
    private String mHdmiAudioFormatInfo = null;
    //msg to dolby vision flag
    private int mCheckDolbyVisonCount = 0;

    public TvInputBaseSession(Context context, String inputId, int deviceId) {
        super(context);
        mContext = context;
        mInputId = inputId;
        mDeviceId = deviceId;

        Log.d(TAG, "TvInputBaseSession, inputId " + inputId + " deviceId " + deviceId);
        mSystemControlManager = SystemControlManager.getInstance();
        mSystemControlEvent = SystemControlEvent.getInstance(null);
        mSystemControlManager.setListener(mSystemControlEvent);
        mSystemControlEvent.setHdrInfoListener(this);
        setSessionStateMachine(SESSION_CTEATED);

        mAudioSystemCmdManager = AudioSystemCmdManager.getInstance(mContext);
        mTvControlManager = TvControlManager.getInstance();
        mSessionHandler = new Handler(context.getMainLooper(), this);
        mTvInputManager = (TvInputManager)mContext.getSystemService(Context.TV_INPUT_SERVICE);
        mDroidLogicHdmiCecManager = DroidLogicHdmiCecManager.getInstance(mContext);
        isHdmiDevice = (DroidLogicTvUtils.parseTvSourceTypeFromDeviceId(deviceId) == TvControlManager.SourceInput_Type.SOURCE_TYPE_HDMI);
        sendSessionMessage(MSG_REGISTER_BROADCAST);
    }

    public void setSessionId(int id) {
        mId = id;
    }

    public int getSessionId() {
        return mId;
    }

    public String getInputId() {
        return mInputId;
    }

    public int getDeviceId() {
        return mDeviceId;
    }

    public void sendSessionMessage(int cmd) {
        if (mSessionHandler == null)
            return;
        Message msg = mSessionHandler.obtainMessage(cmd);
        mSessionHandler.removeMessages(msg.what);
        msg.sendToTarget();
    }
    public void doRelease() {
        Log.d(TAG, "doRelease,input: " + mInputId + " " + this);

        mContext.unregisterReceiver(mBroadcastReceiver);
        mSystemControlEvent.setHdrInfoListener(null);
        mSessionHandler.removeCallbacksAndMessages(null);
//        mSessionHandler = null;

        if (setSessionStateMachine(SESSION_RELEASED) == 0) {
            ((DroidLogicTvInputService)mContext).stopTvPlay(mId, true);
        }
    }

    private int setSessionStateMachine (int action) {
        int count = DataProviderManager.getIntValue(mContext, DroidLogicTvUtils.TV_SESSION_COUNT, 0);
        switch (action) {
                case SESSION_CTEATED:
                    count++;
                    break;
                case SESSION_RELEASED:
                    count--;
                    break;
        }

        if (count > 1) {
        } else if (count == 1) {
            if (mSystemControlManager.getPropertyBoolean(DroidLogicTvUtils.PROP_NEED_PREVIEW_WINDOW, false)) {
                int state = DataProviderManager.getIntValue(mContext, DroidLogicTvUtils.TV_SESSION_STATE, DroidLogicTvUtils.STATE_FREE);
                if (state == DroidLogicTvUtils.STATE_SWITCHING_HOME) {
                    DataProviderManager.putIntValue(mContext, DroidLogicTvUtils.TV_SESSION_STATE, DroidLogicTvUtils.STATE_PLAYING_HOME);
                } else if (state == DroidLogicTvUtils.STATE_SWITCHING_TVAPP) {
                    DataProviderManager.putIntValue(mContext, DroidLogicTvUtils.TV_SESSION_STATE, DroidLogicTvUtils.STATE_PLAYING_TVAPP);
                }
            }
        } else {
            DataProviderManager.putIntValue(mContext, DroidLogicTvUtils.TV_SESSION_STATE, DroidLogicTvUtils.STATE_FREE);
            count = 0;
        }
        DataProviderManager.putIntValue(mContext, DroidLogicTvUtils.TV_SESSION_COUNT, count);
        //Log.d(TAG, "setSessionStateMachine current state is :" + TvControlDataManager.getString(mContext.getContentResolver(), DroidLogicTvUtils.TV_SESSION_STATE) + "  count=" + count);

        return count;
    }

    public void doAppPrivateCmd(String action, Bundle bundle) {}
    public void doUnblockContent(TvContentRating rating) {}
    public boolean isRadioChannel() {
        return false;
    }

    @Override
    public void onSurfaceChanged(int format, int width, int height) {
        Log.d(TAG, "onSurfaceChanged: format="+format + " width=" + width + " height=" + height);
        if (width < 720 || height < 480) {
            mTvControlManager.SetPreviewWindowMode(true);
        } else {
            mTvControlManager.SetPreviewWindowMode(false);
        }
    }

    @Override
    public void onSetStreamVolume(float volume) {
        if (DEBUG)
            Log.d(TAG, "onSetStreamVolume volume = " + volume);
        //this function used for parental control, so HDMI source don't need it.
        /*if ((mDeviceId >= DroidLogicTvUtils.DEVICE_ID_HDMI1 && mDeviceId <= DroidLogicTvUtils.DEVICE_ID_HDMI4)) {
            Log.d(TAG, "onSetStreamVolume: hdmi source.");
            return;
        }*/
        Message msg = mSessionHandler.obtainMessage(MSG_AUDIO_MUTE);
        if (0.0 == volume) {
            msg.arg1 = 0;
        } else {
            msg.arg1 = 1;
        }
        mSessionHandler.removeMessages(msg.what);
        mSessionHandler.sendMessageDelayed(msg, TVINPUT_BASE_DELAY_SEND_MSG);
    }

    @Override
    public void onAppPrivateCommand(String action, Bundle data) {
        if (DEBUG)
            Log.d(TAG, "onAppPrivateCommand, action = " + action);

        if (mSessionHandler == null)
            return;
        Message msg = mSessionHandler.obtainMessage(MSG_DO_PRI_CMD);
        mSessionHandler.removeMessages(msg.what);
        msg.setData(data);
        msg.obj = action;
        msg.sendToTarget();
    }

    @Override
    public void onSetCaptionEnabled(boolean enabled) {
        // TODO Auto-generated method stub
    }

    @Override
    public void onUnblockContent(TvContentRating unblockedRating) {
        if (DEBUG)
            Log.d(TAG, "onUnblockContent");

        doUnblockContent(unblockedRating);
    }

    public void initOverlayView(int resId) {
        LayoutInflater inflater = (LayoutInflater)
                mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        mOverlayView = (DroidLogicOverlayView)inflater.inflate(resId, null);
        setOverlayViewEnabled(true);
    }

    private  BroadcastReceiver mBroadcastReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (intent.getAction().equals(Intent.ACTION_SCREEN_OFF)) {
                if (DEBUG) Log.d(TAG, "Received ACTION_SCREEN_OFF");
                setOverlayViewEnabled(false);
            } else if (intent.getAction().equals(Intent.ACTION_SCREEN_ON)) {
                if (DEBUG) Log.d(TAG, "Received ACTION_SCREEN_ON");
                setOverlayViewEnabled(true);
            }
        }
    };

    @Override
    public View onCreateOverlayView() {
        return mOverlayView;
    }

    @Override
    public void onOverlayViewSizeChanged(int width, int height) {
        Log.d(TAG, "onOverlayViewSizeChanged: "+width+","+height);
    }

    @Override
    public void notifyVideoAvailable() {
        Log.d(TAG, "notifyVideoAvailable ");
        super.notifyVideoAvailable();
        if (mOverlayView != null) {
            if (isRadioChannel()) {
                mOverlayView.setImageVisibility(true);
                mOverlayView.setTextVisibility(false);
            } else {
                mOverlayView.setImageVisibility(false);
                mOverlayView.setTextVisibility(false);
            }
        }

        if (isHdmiDevice) {
            checkHdmiInfoOnVideoAvailable();
        }
    }

    @Override
    public void notifyVideoUnavailable(int reason) {
        Log.d(TAG, "notifyVideoUnavailable: "+reason);
        super.notifyVideoUnavailable(reason);
        Message msg = mSessionHandler.obtainMessage(MSG_IMAGETEXT_SET);
        mSessionHandler.removeMessages(msg.what);
        msg.sendToTarget();
        if (isHdmiDevice) {
            checkHdmiInfoOnVideoUnavailable();
        }
    }

    @Override
     public boolean onSetSurface(Surface surface) {
        if (surface == null) {
            mSessionHandler.removeCallbacksAndMessages(null);
            isSurfaceAlive = false;

            setOverlayViewEnabled(false);
            if (mOverlayView != null) {
                mOverlayView.releaseResource();
                mOverlayView = null;
            }
        } else {
            isSurfaceAlive = true;
        }

        return false;
     }

    @Override
    public void onRelease() {
        mSessionHandler.removeCallbacksAndMessages(null);
        if (mSessionHandler == null)
            return;
        Message msg = mSessionHandler.obtainMessage(MSG_DO_RELEASE);
        mSessionHandler.removeMessages(msg.what);
        msg.sendToTarget();
    }

    public void hideUI() {
        if (mOverlayView != null) {
            mOverlayView.setImageVisibility(false);
            mOverlayView.setTextVisibility(false);
            mOverlayView.setSubtitleVisibility(false);
        }
    }

    private void setAudiodMute(boolean mute) {
        Log.d(TAG, "setAudiodMute="+mute);
        if (mute) {
            handleAdtvAudioEvent(AudioSystemCmdManager.AUDIO_SERVICE_CMD_SET_MUTE, 1, 0);
        } else {
            handleAdtvAudioEvent(AudioSystemCmdManager.AUDIO_SERVICE_CMD_SET_MUTE, 0, 0);
        }
    }

    public void openTvAudio (int type){
        mAudioSystemCmdManager.openTvAudio(type);
    }

    public void closeTvAudio (){
        mAudioSystemCmdManager.closeTvAudio();
    }

    public void handleAdtvAudioEvent (int cmd, int param1, int param2){
        mAudioSystemCmdManager.handleAdtvAudioEvent(cmd, param1, param2);
    }

    public void setParameters(String arg) {
        mAudioSystemCmdManager.setParameters(arg);
    }

    public String getParameters(String arg) {
        return mAudioSystemCmdManager.getParameters(arg);
    }

    @Override
    public boolean handleMessage(Message msg) {
        if (!isSurfaceAlive) {
            if (msg.what != MSG_DO_RELEASE && msg.what != MSG_AUDIO_MUTE) {
                return false;
            }
        }

        switch (msg.what) {
            case MSG_REGISTER_BROADCAST:
                    IntentFilter intentFilter = new IntentFilter();
                    intentFilter.addAction(Intent.ACTION_SCREEN_OFF);
                    intentFilter.addAction(Intent.ACTION_SCREEN_ON);
                    mContext.registerReceiver(mBroadcastReceiver, intentFilter);
                break;
            case MSG_DO_PRI_CMD:
                if (isHdmiDevice && DroidLogicTvUtils.ACTION_TIF_SHOW_DOLBY_VISION.equals((String)msg.obj)) {
                    showDolbyVisionInitiativelyFor3s();
                }
                doAppPrivateCmd((String)msg.obj, msg.getData());
                break;
            case MSG_SUBTITLE_SHOW:
                if (mOverlayView != null) {
                    mOverlayView.setSubtitleVisibility(true);
                }
                break;
            case MSG_SUBTITLE_HIDE:
                if (mOverlayView != null) {
                    mOverlayView.setSubtitleVisibility(false);
                }
                break;
            case MSG_DO_RELEASE:
                doRelease();
                break;
            case MSG_AUDIO_MUTE:
                long startTime = SystemClock.uptimeMillis();
                setAudiodMute(msg.arg1 == 0);
                if (DEBUG) Log.d(TAG, "setAudiodMute used " + (SystemClock.uptimeMillis() - startTime) + " ms");
                break;
            case MSG_IMAGETEXT_SET:
                if (mOverlayView != null) {
                    mOverlayView.setImageVisibility(true);
                    mOverlayView.setTextVisibility(true);
                }
                break;
            case MSG_IMAGETEXT_RESET:
                if (mOverlayView != null) {
                    mOverlayView.setImageVisibility(false);
                    mOverlayView.setTextVisibility(false);
                }
                break;
            case MSG_UPDATE_HDMI_HDR:
                checkHdmiHdrInfo();
                break;
            case MSG_UPDATE_HDMI_AUDIO_FORMAT:
                checkHdmiAudioFormat();
                break;
            case MSG_CLEAR_INFO:
                clearInfo();
                break;
            case MSG_SHOW_DOLBY_VSION:
                dealDolbyVisionDisplay(msg.arg1, msg.arg2, msg.obj);
                break;
        }
        return false;
    }

    /****************************** HDMI CEC PART ************************************************/

    /**
     * Only in here should the TvInput switch to the active source generally.
     * In the previous design way, lots of places including LiveTv,DroidTvSettings,TvInput etc.
     * does portSelect or deviceSelect, and in the selectHdmiDevice method of DroidHdmiCecManager,
     * lots of if-else is used. And there are still issues come out today or tomorrow, especailly
     * for projects like amazon which does not use aml LiveTv apps.
     * The most important idea of refactoring the code is that the control logic should be high
     * aggregated and much simple, and only in this way it could be easy to maintain.
     */
    @Override
    public void onSetMain(boolean isMain) {

        Log.d(TAG, "onSetMain, isMain " + isMain + " input " + mInputId + " seq " + getSessionId());

        mDroidLogicHdmiCecManager.onSetMain(isMain, mInputId, mDeviceId, getSessionId());
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        Log.v(TAG, "onKeyUp: " + keyCode);
        if (!isHdmiDevice) {
            return false;
        }
        if (mDroidLogicHdmiCecManager.getHdmiDeviceInfo(mInputId) == null) {
            if (DEBUG) {
                Log.d(TAG, "no send key for no hdmi device " + mInputId);
            }
            return false;
        }
        return mDroidLogicHdmiCecManager.sendKeyEvent(keyCode, false);
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        Log.v(TAG, "onKeyDown: " + keyCode);
        if (!isHdmiDevice) {
            return false;
        }
        if (mDroidLogicHdmiCecManager.getHdmiDeviceInfo(mInputId) == null) {
            if (DEBUG) {
                Log.d(TAG, "no send key for no hdmi device " + mInputId);
            }
            return false;
        }
        return mDroidLogicHdmiCecManager.sendKeyEvent(keyCode, true);
    }

    /****************************** HDMI CEC PART ************************************************/

    @Override
    public void onHdrInfoChange(int newHdrInfo) {
        //if (DEBUG)
            //Log.d(TAG, "onHdrInfoChange: hdr info is: " + newHdrInfo);
        String newHdrtype = getHdmiHdrInfo(newHdrInfo);
        if (!TextUtils.equals(newHdrtype, mHdmiHdrInfo)) {
            mHdmiHdrInfo = newHdrtype;
            if (mSessionHandler != null) {
                mSessionHandler.removeMessages(MSG_UPDATE_HDMI_HDR);
                mSessionHandler.sendEmptyMessageDelayed(MSG_UPDATE_HDMI_HDR, 0);
            } else {
                Log.d(TAG, "onHdrInfoChange: mSessionHandler is null.");
            }
        } else {
            //Log.d(TAG, "onHdrInfoChange: same hdr info.");
        }
    }

    //add for get hdmi info
    private void checkHdmiInfoOnVideoAvailable() {
        Log.d(TAG, "checkHdmiInfoOnVideoAvailable");
        if (mSessionHandler != null) {
            mSessionHandler.removeMessages(MSG_UPDATE_HDMI_AUDIO_FORMAT);
            mSessionHandler.removeMessages(MSG_CLEAR_INFO);
            mSessionHandler.removeMessages(MSG_DISPLAY_PERIOD);
            mSessionHandler.sendEmptyMessageDelayed(MSG_UPDATE_HDMI_AUDIO_FORMAT, MSG_DELAY_PERIOD);
        }
    }

    private void checkHdmiInfoOnVideoUnavailable() {
        Log.d(TAG, "checkHdmiInfoOnVideoUnavailable");
        if (mSessionHandler != null) {
            mSessionHandler.removeMessages(MSG_UPDATE_HDMI_AUDIO_FORMAT);
            mSessionHandler.removeMessages(MSG_CLEAR_INFO);
            mSessionHandler.removeMessages(MSG_DISPLAY_PERIOD);
            mSessionHandler.sendEmptyMessageDelayed(MSG_CLEAR_INFO, MSG_DELAY_PERIOD);
        }
    }

    private void sendHdmiHdrInfoByTif(String hdrInfo) {
        if (mSystemControlManager.getPropertyBoolean("vendor.sys.tv.DolbyVision", false)) {
            hdrInfo = "DOVI";
        }
        Bundle bundle = new Bundle();
        bundle.putString(DroidLogicTvUtils.SIG_INFO_HDMI_HDR, hdrInfo);
        notifySessionEvent(DroidLogicTvUtils.SIG_INFO_HDMI_HDR, bundle);
    }

    private void sendHdmiAudioFormatByTif(String audioFormat) {
        Bundle bundle = new Bundle();
        bundle.putString(DroidLogicTvUtils.SIG_INFO_HDMI_AUDIO_FORMAT, audioFormat);
        notifySessionEvent(DroidLogicTvUtils.SIG_INFO_HDMI_AUDIO_FORMAT, bundle);
    }

    private String getHdmiHdrInfo(int newHdrTypeInfo) {
        String result = null;
        switch (newHdrTypeInfo) {
            case 0:
                result = "UNKOWN";
                break;
            case 1:
                result = "HDR10";
                break;
            case 2:
                result = "HDR10PLUS";
                break;
            case 3:
                result = "DOVI";
                break;
            case 4:
                result = "PRIMESL";
                break;
            case 5:
                result = "HLG";
                break;
            case 6:
                result = "SDR";
                break;
            case 7:
                result = "MVC";
                break;
            default:
                result = "UNKOWN";
                break;
        }
        return result;
    }

    /*
    *PCM audios require  display PCM
    *DD  audios require display Dolby Digital
    *DDP audios require display Dolby Digital Plus
    *DTS  audios require display DTS
    *DTS HD audios require display DTS HD
    */
    private String getHdmiAudioFormat() {
        String result = null;
        String formatInfo = getParameters("HDMIIN Audio Type");
        int audioFormat = parseFirstValidIntegerByPattern(formatInfo);
        switch (audioFormat) {
            case 0:
                result = "PCM";
                break;
            case 1:
                result = "Dolby Digital";
                break;
            case 2:
                result = "Dolby Digital Plus";
                break;
            case 3:
                result = "DTS";
                break;
            case 4:
                result = "DTS HD";
                break;
            case 5:
                result = "TRUE HD";
                break;
            default:
                break;
        }
        return result;
    }

    private void checkHdmiHdrInfo() {
        if (mHdmiHdrInfo != null) {
            sendHdmiHdrInfoByTif(mHdmiHdrInfo);
            checkDolbyVisionStatus(mHdmiHdrInfo);
        }
    }

    public void resetHdmiHdrInfo(int hdrType) {
        mHdmiHdrInfo = getHdmiHdrInfo(hdrType);
        checkHdmiHdrInfo();
    }

    private void checkHdmiAudioFormat() {
        String hdmiAudioFormat = getHdmiAudioFormat();
        if (!TextUtils.equals(hdmiAudioFormat, mHdmiAudioFormatInfo)) {
            mHdmiAudioFormatInfo = hdmiAudioFormat;
            sendHdmiAudioFormatByTif(hdmiAudioFormat);
        }
        if (mSessionHandler != null) {
            mSessionHandler.removeMessages(MSG_UPDATE_HDMI_AUDIO_FORMAT);
            mSessionHandler.sendEmptyMessageDelayed(MSG_UPDATE_HDMI_AUDIO_FORMAT, MSG_DELAY_PERIOD);
        }
    }

    private void clearInfo() {
        mHdmiHdrInfo = null;
        mHdmiAudioFormatInfo = null;
        sendHdmiHdrInfoByTif(null);
        sendHdmiAudioFormatByTif(null);
    }

    private int parseFirstValidIntegerByPattern(String info) {
        int result = -1;
        try {
            String regEx="[0-9]+";
            Pattern p = Pattern.compile(regEx);
            Matcher m = p.matcher(info);
            if (m.find()) {
              result = Integer.valueOf(m.group(0));
            } else {
                Log.d(TAG, "parseFirstValidIntergerByPattern not matched");
            }
        } catch (Exception e) {
            Log.d(TAG, "parseFirstValidIntergerByPattern Exception = " + e.getMessage());
        }
        return result;
    }

    private void checkDolbyVisionStatus(String hdmiHdr) {
        if (mSessionHandler != null) {
            mSessionHandler.removeMessages(MSG_SHOW_DOLBY_VSION);
            mCheckDolbyVisonCount = 0;
            mSessionHandler.sendMessageDelayed(mSessionHandler.obtainMessage(MSG_SHOW_DOLBY_VSION, 0, 2, null), 0);//use arg2 to control display
            mSessionHandler.sendMessageDelayed(mSessionHandler.obtainMessage(MSG_SHOW_DOLBY_VSION, 1, 0, hdmiHdr), 0);//use arg1 to start deal display
        }
    }

    /*
    * arg1 = 1 means start to deal
    * arg2 = 1 means display, arg2 = 2 means hide
    */
    private void dealDolbyVisionDisplay(int arg1, int arg2, Object obj) {
        if (DEBUG) {
            Log.d(TAG, "dealDolbyVisionDisplay arg1 = " + arg1 + ", arg2 = " + arg2 + ", obj = " + obj);
        }
        if (arg1 == 1) {
            String hdmiHdr = (String)obj;
            if (isDolbyVisionType(hdmiHdr)) {
                if (mSessionHandler != null) {
                    mSessionHandler.removeMessages(MSG_SHOW_DOLBY_VSION);
                    mSessionHandler.sendMessageDelayed(mSessionHandler.obtainMessage(MSG_SHOW_DOLBY_VSION, 0, 1, null), 0);
                    mSessionHandler.sendMessageDelayed(mSessionHandler.obtainMessage(MSG_SHOW_DOLBY_VSION, 0, 2, null), MSG_DISPLAY_PERIOD);
                }
            }
        }
        if (arg2 == 1) {
            if (mOverlayView != null) {
                if (!mOverlayView.isDoblyVisionVisible()) {
                    mOverlayView.setDoblyVisionVisibility(true);
                }
            }
        } else if (arg2 == 2) {
            if (mOverlayView != null) {
                if (mOverlayView.isDoblyVisionVisible()) {
                    mOverlayView.setDoblyVisionVisibility(false);
                }
            }
        }
    }

    private boolean isDolbyVisionType(String hdmiHdr) {
        return "DOVI".equals(hdmiHdr) || mSystemControlManager.getPropertyBoolean("vendor.sys.tv.DolbyVision", false);
    }

    public void showDolbyVisionInitiativelyFor3s() {
        if (mHdmiHdrInfo != null && isDolbyVisionType(mHdmiHdrInfo)) {
            if (mOverlayView.isDoblyVisionVisible()) {
                //reset timeout
                mSessionHandler.removeMessages(MSG_SHOW_DOLBY_VSION);
                mSessionHandler.sendMessageDelayed(mSessionHandler.obtainMessage(MSG_SHOW_DOLBY_VSION, 0, 2, null), MSG_DISPLAY_PERIOD);
            } else {
                checkDolbyVisionStatus(mHdmiHdrInfo);
            }
        }
    }
}
