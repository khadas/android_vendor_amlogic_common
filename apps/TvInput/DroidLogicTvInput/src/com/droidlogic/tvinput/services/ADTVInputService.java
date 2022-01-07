/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.tvinput.services;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ContentUris;
import android.content.pm.ResolveInfo;
import android.media.tv.TvContentRating;
import android.media.tv.TvInputManager;
import android.media.tv.TvInputInfo;
import android.media.tv.TvInputHardwareInfo;
import android.media.tv.TvTrackInfo;
import android.media.tv.TvContract;
import android.media.tv.TvStreamConfig;
import android.media.tv.TvInputManager.Hardware;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.Message;
import android.os.RemoteException;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.database.ContentObserver;

import com.droidlogic.tvinput.Utils;

import com.droidlogic.app.tv.TvDataBaseManager;
import com.droidlogic.app.tv.TvChannelParams;
import com.droidlogic.app.tv.DroidLogicTvUtils;
import com.droidlogic.app.tv.ChannelInfo;
import com.droidlogic.app.tv.Program;
import com.droidlogic.app.tv.TvMultilingualText;
import com.droidlogic.app.tv.TvTime;
import com.droidlogic.app.tv.TvStoreManager;
import com.droidlogic.app.tv.TvChannelSetting;
import com.droidlogic.app.tv.TvMTSSetting;
import com.droidlogic.app.tv.TvControlDataManager;
import com.droidlogic.app.SystemControlManager;

import java.util.HashSet;
import java.util.Set;
import java.util.Map;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Arrays;

import com.droidlogic.app.tv.TvControlManager;

import java.util.HashMap;
import java.util.Map;
import android.net.Uri;
import android.view.Surface;
import org.json.JSONArray;
import org.json.JSONObject;
import org.json.JSONException;

public class ADTVInputService extends DTVInputService {

    private static final String TAG = "ADTVInputService";

    @Override
    public void onCreate() {
        super.onCreate();
        initInputService(DroidLogicTvUtils.DEVICE_ID_ADTV, ADTVInputService.class.getName());
    }

    @Override
    public Session onCreateSession(String inputId) {
        registerInput(inputId);
        mCurrentSession = new ADTVSessionImpl(this, inputId, getHardwareDeviceId(inputId));
        mCurrentSession.setSessionId(id);
        registerInputSession(mCurrentSession);
        sessionMap.put(id, mCurrentSession);
        id++;

        return mCurrentSession;
    }

    public class ADTVSessionImpl extends DTVInputService.DTVSessionImpl implements TvControlManager.AVPlaybackListener , TvControlManager.PlayerInstanceNoListener{

        protected ADTVSessionImpl(Context context, String inputId, int deviceId) {
            super(context, inputId, deviceId);
        }

        @Override
        protected void checkContentBlockNeeded(ChannelInfo channelInfo) {
            doParentalControls(channelInfo);
        }

        @Override
        protected boolean playProgram(ChannelInfo info) {
            if (info == null)
                return false;

            if (mSurface == null) {
                Log.d(TAG, "surface is null, drop playProgram");
                return true;
            }

            //info.print();
            if (enableChannelBlockInServer()) {
                mTvControlManager.request("ADTV.block", "{\"isblocked\":" + info.isLocked() + ",\"tunning\":" + true + "}");
            }
            if (mSystemControlManager.getPropertyBoolean(DroidLogicTvUtils.PROP_NEED_FAST_SWITCH, false)) {
                Log.d(TAG, "fast switch mode, no need replay channel");
                mSystemControlManager.setProperty(DroidLogicTvUtils.PROP_NEED_FAST_SWITCH, "false");
                if (info.isAnalogChannel()) {
                    openTvAudio(DroidLogicTvUtils.SOURCE_TYPE_ATV);
                    if (enableChannelBlockInServer()) {
                        mTvControlManager.SetAVPlaybackListener(this);
                        super.onSetStreamVolume(0.0f);
                    }
                } else {
                    mTvControlManager.SetAVPlaybackListener(this);
                    mTvControlManager.SetPlayerInstanceNoListener(this);
                }
            } else {
                int audioAuto = getAudioAuto(info);
                ChannelInfo.Audio audio = null;
                if (mCurrentAudios != null && mCurrentAudios.size() > 0 && audioAuto >= 0) {
                    audio = mCurrentAudios.get(audioAuto);
                }

                if (info.isAnalogChannel()) {
                    if (info.isNtscChannel())
                        muteVideo(false);

                    /* open atv audio and mute when notify Video Available */
                    openTvAudio(DroidLogicTvUtils.SOURCE_TYPE_ATV);
                    TvControlManager.FEParas fe = new TvControlManager.FEParas();
                    fe.setFrequency(info.getFrequency() + info.getFineTune());
                    fe.setVideoStd(info.getVideoStd());
                    fe.setAudioStd(info.getAudioStd());
                    fe.setVfmt(info.getVfmt());
                    fe.setAudioOutPutMode(info.getAudioOutPutMode());
                    fe.setAfc(0);
                    StringBuilder param = new StringBuilder("{")
                            .append("\"type\":\"atv\"")
                            .append("," + fe.toString("fe"))
                            .append(",\"a\":{\"AudComp\":"+info.getAudioCompensation()+"}")
                            .append("}");

                    mTvControlManager.startPlay("ntsc", param.toString());
                    if (enableChannelBlockInServer()) {
                        mTvControlManager.SetAVPlaybackListener(this);
                        super.onSetStreamVolume(0.0f);
                    }
                } else {
                    String subPidString = new String("\"pid\":0");
                    int subCount = (info.getSubtitlePids() == null)? 0 : info.getSubtitlePids().length;
                    if (subCount != 0) {
                        subPidString = new String("");
                        for (int i = 0; i < subCount; i++) {
                            subPidString = subPidString + "\"pid" + i+ "\":" + info.getSubtitlePids()[i];
                            if (i != (subCount - 1 ))
                                subPidString = subPidString + ",";
                        }
                        Log.d(TAG, "subpid- string: " + subPidString);
                    }
                    JSONArray as = new JSONArray();;
                    try {
                        /*audio tracks*/
                        int audioCount = (info.getAudioPids() == null)? 0 : info.getAudioPids().length;
                        if (audioCount != 0) {
                            for (int i = 0; i < audioCount; i++) {
                                as.put(new JSONObject()
                                        .put("pid", info.getAudioPids()[i])
                                        .put("fmt", info.getAudioFormats()[i]));
                            }
                        }
                        Log.d(TAG, "audio string: " + as.toString());
                    } catch (JSONException e) {
                        Log.e(TAG, "Json fail for audio param:"+e);
                    }
                    TvControlManager.FEParas fe = new TvControlManager.FEParas(info.getFEParas());

                    mTvControlManager.SetAVPlaybackListener(this);
                    mTvControlManager.SetPlayerInstanceNoListener(this);
                    openTvAudio(DroidLogicTvUtils.SOURCE_TYPE_DTV);
                    int timeshiftMaxTime = mSystemControlManager.getPropertyInt("tv.dtv.tf.max.time", 10*60);/*seconds*/
                    int timeshiftMaxSize = mSystemControlManager.getPropertyInt(MAX_CACHE_SIZE_KEY, MAX_CACHE_SIZE_DEF * 1024);/*bytes*/
                    String timeshiftPath = mSystemControlManager.getPropertyString("tv.dtv.tf.path", getCacheStoragePath());
                    StringBuilder param = new StringBuilder("{")
                            .append("\"fe\":" + info.getFEParas())
                            .append(",\"v\":{\"pid\":"+info.getVideoPid()+",\"fmt\":"+info.getVfmt()+"}")
                            .append(",\"a\":{\"pid\":"+(audio != null ? audio.mPid : -1)+",\"fmt\":"+(audio != null ? audio.mFormat : -1)+",\"AudComp\":"+info.getAudioCompensation()+"}")
                            .append(",\"p\":{\"pid\":"+info.getPcrPid()+"}")
                            //.append(",\"para\":{"+"\"disableTimeShifting\":1"+"}")
                            .append(",\"para\":{")
                            .append("\"max\":{"+"\"time\":"+timeshiftMaxTime+"}")//",\"size\":"+timeshiftMaxSize+
                            .append(",\"path\":\""+timeshiftPath+"\"")
                            .append(",\"subpid\":{" + subPidString)
                            .append("},\"subcnt\":" + subCount)
                            .append(",\"as\":" + as.toString())
                            .append("}}");
                    Log.d(TAG, "playProgram adtvparam: " + param.toString());

                    mTvControlManager.startPlay("atsc", param.toString());
                    mTvControlManager.DtvSetAudioChannleMod(info.getAudioChannel());
                    startAudioADMainMix(info, audioAuto);
                }
                mSystemControlManager.setProperty(DTV_AUDIO_TRACK_IDX,
                ((audioAuto>=0)? String.valueOf(audioAuto) : "-1"));
                mSystemControlManager.setProperty(DTV_AUDIO_TRACK_ID, generateAudioIdString(audio));
            }

            notifyTracks(info);
            tryStartSubtitle(info);

            isTvPlaying = true;
            return true;
        }

        protected void muteVideo(boolean mute) {
            if (mute)
                mSystemControlManager.writeSysFs("/sys/class/deinterlace/di0/config", "hold_video 1");
            else
                mSystemControlManager.writeSysFs("/sys/class/deinterlace/di0/config", "hold_video 0");
        }

        @Override
        protected boolean startSubtitle(ChannelInfo channelInfo) {
            if (super.startSubtitle(channelInfo))
                return true;

            if (channelInfo != null && channelInfo.isNtscChannel()) {
                startSubtitleCCBackground(channelInfo);
                return true;
            }
            return false;
        }

        @Override
        protected void stopSubtitleBlock(ChannelInfo channel) {
            Log.d(TAG, "stopSubtitleBlock");

            if (channel.isNtscChannel())
                startSubtitleCCBackground(channel);
            else
                super.stopSubtitleBlock(channel);
        }

        private TvContentRating[] mATVContentRatings = null;

        @Override
        protected boolean isAtsc(ChannelInfo info) {
            return info.isNtscChannel() || super.isAtsc(info);
        }

        @Override
        protected boolean tryPlayProgram(ChannelInfo info) {
            mATVContentRatings = null;
            return super.tryPlayProgram(info);
        }

        @Override
        protected TvContentRating[] getContentRatingsOfCurrentProgram(ChannelInfo channelInfo) {
            if (channelInfo != null && channelInfo.isAnalogChannel())
                return Program.stringToContentRatings(channelInfo.getContentRatings());
                //return mATVContentRatings;
            else
                return super.getContentRatingsOfCurrentProgram(channelInfo);
        }

        @Override
        public void onSubtitleData(String json) {
//            Log.d(TAG, "onSubtitleData json: " + json);
//            Log.d(TAG, "onSubtitleData curchannel:"+(mCurrentChannel!=null?mCurrentChannel.getDisplayName():"null"));
//            if (mCurrentChannel != null && mCurrentChannel.isAnalogChannel()) {
//                mATVContentRatings = DroidLogicTvUtils.parseARatings(json);
//                if (/*mATVContentRatings != null && */json.contains("Aratings") && !TextUtils.equals(json, mCurrentChannel.getContentRatings())) {
//                    TvDataBaseManager tvdatabasemanager = new TvDataBaseManager(mContext);
//                    mCurrentChannel.setContentRatings(json);
//                    tvdatabasemanager.updateOrinsertAtvChannel(mCurrentChannel);
//                }
//
//                if (mHandler != null)
//                    mHandler.sendMessage(mHandler.obtainMessage(MSG_PARENTAL_CONTROL, this));
//            }
            super.onSubtitleData(json);
        }

        public String onReadSysFs(String node) {
            return super.onReadSysFs(node);
        }

        public void onWriteSysFs(String node, String value) {
            super.onWriteSysFs(node, value);
        }

        @Override
        protected void setMonitor(ChannelInfo channel) {
            if (channel == null || !channel.isAnalogChannel())
                super.setMonitor(channel);
        }

        @Override
        public void onEvent(int msgType, int programID) {
            Log.d(TAG, "AV evt:" + msgType);
            if (mCurrentSession != null && mCurrentSession.mCurrentChannel != null) {
                if (mCurrentSession.mCurrentChannel.isAnalogChannel()) {
                    if (msgType > TvControlManager.EVENT_AV_PLAYER_UNBLOCK
                        || msgType < TvControlManager.EVENT_AV_PLAYER_BLOCKED) {
                        //event only used in dtv
                        return;
                    }
                }
            }
            super.onEvent(msgType, programID);
        }

        @Override
        public void doAppPrivateCmd(String action, Bundle bundle) {
            int value = 0;

            Log.d(TAG, "doAppPrivateCmd: action:" + action);

            if (mCurrentSession.mCurrentChannel == null)
                return;

            if (DroidLogicTvUtils.ACTION_ATV_SET_FINETUNE.equals(action)) {
                value = bundle.getInt("finetune", mCurrentSession.mCurrentChannel.getFineTune());

                TvChannelSetting.setAtvFineTune(mCurrentSession.mCurrentChannel, value);
            } else if (DroidLogicTvUtils.ACTION_ATV_SET_VIDEO.equals(action)) {
                value = bundle.getInt("video", mCurrentSession.mCurrentChannel.getVideoStd());

                TvChannelSetting.setAtvChannelVideo(mCurrentSession.mCurrentChannel, value);
            } else if (DroidLogicTvUtils.ACTION_ATV_SET_AUDIO.equals(action)) {
                value = bundle.getInt("audio", mCurrentSession.mCurrentChannel.getAudioStd());

                TvChannelSetting.setAtvChannelAudio(mCurrentSession.mCurrentChannel, value);
            } else if (DroidLogicTvUtils.ACTION_ATV_SET_MTS_OUTPUT_MODE.equals(action)) {
                value = bundle.getInt("mts_output_mode", mCurrentSession.mCurrentChannel.getAudioOutPutMode());

                TvMTSSetting.setAtvMTSOutModeValue(value);
            } else if (DroidLogicTvUtils.ACTION_ATV_GET_MTS_OUTPUT_MODE.equals(action)) {
                value = TvMTSSetting.getAtvMTSOutModeValue();

                TvControlDataManager.putIntValue(mContext, DroidLogicTvUtils.ACTION_ATV_GET_MTS_OUTPUT_MODE, value);
            } else if (DroidLogicTvUtils.ACTION_ATV_GET_MTS_INPUT_STD.equals(action)) {
                value = TvMTSSetting.getAtvMTSInSTDValue();

                TvControlDataManager.putIntValue(mContext, DroidLogicTvUtils.ACTION_ATV_GET_MTS_INPUT_STD, value);
            } else if (DroidLogicTvUtils.ACTION_ATV_GET_MTS_INPUT_MODE.equals(action)) {
                value = TvMTSSetting.getAtvMTSInModeValue();

                TvControlDataManager.putIntValue(mContext, DroidLogicTvUtils.ACTION_ATV_GET_MTS_INPUT_MODE, value);
            } else if (DroidLogicTvUtils.ACTION_ATV_SET_VOLUME_COMPENSATE.equals(action)) {
                value = bundle.getInt(DroidLogicTvUtils.KEY_ATV_SET_VALUE, 0);
                TvMTSSetting.setVolumeCompensate(value);
            } else if ("unblockContent".equals(action)
                || "block_channel".equals(action)) {
                super.doAppPrivateCmd(action, bundle);
            } else {
                Log.d(TAG, "doAppPrivateCmd: nonsupport action:" + action);
            }
        }
    }

    public String getDeviceClassName() {
        return ADTVInputService.class.getName();
    }

    public int getDeviceSourceType() {
        return DroidLogicTvUtils.DEVICE_ID_ADTV;
    }

}
