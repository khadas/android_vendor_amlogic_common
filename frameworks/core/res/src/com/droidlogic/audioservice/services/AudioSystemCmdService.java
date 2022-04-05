/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC AudioSystemCmdService
 */

package com.droidlogic.audioservice.services;

import java.util.ArrayList;
import java.util.List;

import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.database.ContentObserver;
import android.os.IBinder;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.SystemProperties;

import android.provider.Settings;
import android.util.Log;
import android.util.Slog;
import android.media.AudioDevicePort;
import android.media.AudioFormat;
import android.media.AudioGain;
import android.media.AudioGainConfig;
import android.media.AudioManager;
import android.media.AudioPatch;
import android.media.AudioPort;
import android.media.AudioPortConfig;
import android.media.AudioRoutesInfo;
import android.media.AudioSystem;

import android.media.IAudioRoutesObserver;
import android.media.IAudioService;
import android.media.tv.TvInputManager;
import android.net.Uri;
import com.droidlogic.app.AudioSystemCmdManager;
import com.droidlogic.app.DroidLogicUtils;
import com.droidlogic.app.OutputModeManager;
import com.droidlogic.app.SystemControlEvent;
import com.droidlogic.app.SystemControlManager;
import com.droidlogic.app.tv.DroidLogicTvUtils;
import com.droidlogic.app.tv.TvControlManager;
import com.droidlogic.UEventObserver;

//this service used to call audio system commands
public class AudioSystemCmdService extends Service {
    private static final String TAG = AudioSystemCmdService.class.getSimpleName();
    private static AudioSystemCmdService mAudioSystemCmdService = null;
    private SystemControlEvent mSystemControlEvent;
    private SystemControlManager mSystemControlManager;
    private DtvKitAudioEvent mDtvKitAudioEvent = null;
    private ADtvAudioEvent mADtvAudioEvent = null;
    private AudioManager mAudioManager = null;
    private AudioPatch mAudioPatch = null;
    private Context mContext;
    private int mCurrentIndex = 0;
    private int mCommitedIndex = -1;
    private int mCurrentMaxIndex = 0;
    private int mCurrentMinIndex = 0;
    private final Object mLock = new Object();
    private final Handler mHandler = new Handler();
    private AudioDevicePort mAudioSource;
    private List<AudioDevicePort> mAudioSink = new ArrayList<>();
    private float mCommittedSourceVolume = -1f;
    private float mSourceVolume = 1.0f;
    private int mDesiredSamplingRate = 0;
    private int mDesiredChannelMask = AudioFormat.CHANNEL_OUT_DEFAULT;
    private int mDesiredFormat = AudioFormat.ENCODING_DEFAULT;
    private int mCurrentFmt = -1;
    private int mCurrentPid = -1;
    private int mCurrentSubFmt = -1;
    private int mCurrentSubPid = -1;
    private int mCurrentHasDtvVideo = 0;
    private int mDtvDemuxIdBase = 20;
    private int mDtvDemuxIdCurrentWork = 0;
    private int mDtvDemuxIdCurrentRecive = 0;
    private int mCurSourceType = DroidLogicTvUtils.SOURCE_TYPE_OTHER;

    private TvInputManager mTvInputManager;
    protected TvControlManager mTvControlManager;
    private static final String PATH_AUDIOFORMAT_UEVENT = "/devices/platform/auge_sound";
    private static final String ACTION_AUDIO_FORMAT_CHANGE = "droidlogic.audioservice.action.AUDIO_FORMAT";
    private static final String AUDIO_FORMAT_KEY = "audio_format";
    private static final String AUDIO_FORMAT_VALUE_KEY = "audio_format_value";
    private final UEventObserver mObserver = new UEventObserver() {
        @Override
        public void onUEvent(UEventObserver.UEvent event) {
            if (DroidLogicUtils.getAudioDebugEnable()) {
                Log.d(TAG, "UEVENT: " + event.toString());
                Log.d(TAG, "DEVPATH: " + event.get("DEVPATH"));
            }

            if (PATH_AUDIOFORMAT_UEVENT.equals(event.get("DEVPATH", null))) {
                String audioFormatStr = event.get("AUDIO_FORMAT", null);
                if (audioFormatStr == null) {
                    Log.e(TAG, "Error! got audio uevent from kernel, but no AUDIO_FORMAT value set!");
                    return;
                }
                if (DroidLogicUtils.getAudioDebugEnable()) {
                    Log.d(TAG, "AUDIO_FORMAT = " + audioFormatStr);
                }
                final int audioFormat = Integer.parseInt(audioFormatStr.substring(audioFormatStr.indexOf("=")+1));
                if (audioFormat < 0) {
                    Log.d(TAG, "ignoring incorrect audio event format:" + audioFormat);
                    return;
                } else {
                    String extra = covertAudioFormatIndextToString(audioFormat);
                    Intent intent = new Intent(ACTION_AUDIO_FORMAT_CHANGE);
                    intent.putExtra(AUDIO_FORMAT_KEY, extra);
                    intent.putExtra(AUDIO_FORMAT_VALUE_KEY,audioFormat);
                    mContext.sendBroadcast(intent);
                }
            }
        }
    };

    private String covertAudioFormatIndextToString(int audioFormat) {
        String stringValue = " ";
        switch (audioFormat) {
            case 0:
            case 9:
                stringValue = "PCM";
                break;
            case 1:
                stringValue = "DTS Express";
                break;
            case 3:
                stringValue = "DTS";
                break;
            case 5:
            case 8:
                stringValue = "DTS HD";
                break;
            case 6:
                stringValue = "Multi PCM";
                break;
            case 2:
            case 4:
            case 7:
            case 10:
            case 11:
            case 12:
            case 13:
            case 14:
            case 15:
                stringValue = "Dolby Audio";
                break;
            default:
                Log.w(TAG, "invalid audioFormat value:" + audioFormat);
                break;
        }
        Log.i(TAG, "covertAudioFormatIndextToEnum: audioFormat:" + audioFormat + ", stringVal:" + stringValue);
        return stringValue;
    }

    private final BroadcastReceiver mVolumeReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            handleVolumeChange(context, intent);
        }
    };
    public AudioSystemCmdService() {
        mAudioSystemCmdService = this;
    }

    private boolean mHasStartedDecoder = false;
    private boolean mHasOpenedDecoder = false;
    private boolean mHasReceivedStartDecoderCmd;
    private boolean  mMixAdSupported;
    private boolean  mNotImptTvHardwareInputService = false;
    private boolean mForceManagePatch = false;
    private IAudioService mAudioService;
    private AudioRoutesInfo mCurAudioRoutesInfo;
    private Runnable mHandleAudioSinkUpdatedRunnable;

    private Runnable mHandleTvAudioRunnable;
    private int mDelayTime = 0;

    final IAudioRoutesObserver.Stub mAudioRoutesObserver = new IAudioRoutesObserver.Stub() {
        @Override
        public void dispatchAudioRoutesChanged(final AudioRoutesInfo newRoutes) {
            Log.i(TAG, "dispatchAudioRoutesChanged cur device:" + newRoutes.mainType +
                    ", pre device:" + mCurAudioRoutesInfo.mainType);
            if (DroidLogicUtils.getAudioDebugEnable()) {
                Log.d(TAG, "dispatchAudioRoutesChanged newRoutes:" + newRoutes.toString());
                Log.d(TAG, "dispatchAudioRoutesChanged preRoutes:" + mCurAudioRoutesInfo.toString());
            }
            if (DroidLogicUtils.isTv()) {
                if (newRoutes.mainType == AudioRoutesInfo.MAIN_HDMI) {
                    Settings.Global.putInt(mContext.getContentResolver(), OutputModeManager.SOUND_OUTPUT_DEVICE, OutputModeManager.SOUND_OUTPUT_DEVICE_ARC);
                } else {
                    Settings.Global.putInt(mContext.getContentResolver(), OutputModeManager.SOUND_OUTPUT_DEVICE, OutputModeManager.SOUND_OUTPUT_DEVICE_SPEAKER);
                }
            }
            mCurAudioRoutesInfo = newRoutes;
            mHasStartedDecoder = false;
            mHandler.removeCallbacks(mHandleAudioSinkUpdatedRunnable);
            mHandleAudioSinkUpdatedRunnable = new Runnable() {
                public void run() {
                    synchronized (mLock) {
                        if (mHasOpenedDecoder ) {
                            if (mNotImptTvHardwareInputService)
                                handleAudioSinkUpdated();
                            mHasOpenedDecoder = false;
                            reStartAdecDecoderIfPossible();
                            mHasOpenedDecoder = true;
                        }
                    }
                }
            };
            if (mTvInputManager.getHardwareList() == null) {
                mHandler.post(mHandleAudioSinkUpdatedRunnable);
            } else {
                try {
                    mHandler.postDelayed(mHandleAudioSinkUpdatedRunnable,
                        mAudioService.isBluetoothA2dpOn() ? 2500 : 500);
                } catch (RemoteException e) {
                    e.printStackTrace();
                }
            }

            if (newRoutes.mainType != mCurAudioRoutesInfo.mainType || !newRoutes.toString().equals(mCurAudioRoutesInfo.toString())) {
                mHandler.removeCallbacks(mHandleTvAudioRunnable);
                mDelayTime = newRoutes.mainType != AudioRoutesInfo.MAIN_HDMI ? 500 : 1500;
                mHandleTvAudioRunnable = new Runnable() {
                    public void run() {
                        Log.i(TAG, "dispatchAudioRoutesChanged ADTV mCurSourceType:" + mCurSourceType);
                        if (mCurSourceType == DroidLogicTvUtils.SOURCE_TYPE_ATV) {
                            mAudioManager.setParameters("hal_param_tuner_in=atv");
                        } else if (mCurSourceType == DroidLogicTvUtils.SOURCE_TYPE_DTV){
                            mAudioManager.setParameters("hal_param_tuner_in=dtv");
                            HandleAudioEvent(AudioSystemCmdManager.AUDIO_SERVICE_CMD_START_DECODE,
                                    mCurrentFmt, mCurrentHasDtvVideo, mDtvDemuxIdCurrentWork, false);
                        }
                    }
                };

                try {
                    mHandler.postDelayed(mHandleTvAudioRunnable,
                            mAudioService.isBluetoothA2dpOn() ? mDelayTime + 2000 : mDelayTime);
                } catch (RemoteException e) {
                    e.printStackTrace();
                }
            }
        }
    };

    private final class DtvKitAudioEvent implements SystemControlEvent.AudioEventListener {
        @Override
        public void HandleAudioEvent(int cmd, int param1, int param2, int param3) {
            if (mAudioSystemCmdService != null) {
                mAudioSystemCmdService.HandleAudioEvent(cmd, param1, param2, param3, true);
            } else {
                Log.w(TAG, "DtvKitAudioEvent HandleAudioEvent mAudioSystemCmdService is null");
            }
        }
    }

    private final class ADtvAudioEvent implements TvControlManager.AudioEventListener {
        @Override
        public void HandleAudioEvent(int cmd, int param1, int param2) {
            if (mAudioSystemCmdService != null) {
                mAudioSystemCmdService.HandleAudioEvent(cmd, param1, param2, 0,false);
            } else {
                Log.w(TAG, "ADtvAudioEvent HandleAudioEvent mAudioSystemCmdService is null");
            }
        }
    }

    @Override
    public void onCreate() {
        Log.i(TAG, "onCreate");
        super.onCreate();
        mContext = getApplicationContext();
        mSystemControlManager = SystemControlManager.getInstance();
        mAudioManager = (AudioManager)getSystemService(Context.AUDIO_SERVICE);
        mTvInputManager = (TvInputManager) getSystemService(Context.TV_INPUT_SERVICE);

        if (DroidLogicUtils.isBuildLivetv()) {
            mADtvAudioEvent = new ADtvAudioEvent();
            mTvControlManager = TvControlManager.getInstance();
            mTvControlManager.SetAudioEventListener(mADtvAudioEvent);
        }

        mSystemControlEvent = SystemControlEvent.getInstance(mContext);
        mDtvKitAudioEvent = new DtvKitAudioEvent();
        mSystemControlEvent.SetAudioEventListener(mDtvKitAudioEvent);
        mSystemControlManager.setListener(mSystemControlEvent);
        IBinder b = ServiceManager.getService(Context.AUDIO_SERVICE);
        mAudioService = IAudioService.Stub.asInterface(b);
        try {
            mCurAudioRoutesInfo = mAudioService.startWatchingRoutes(mAudioRoutesObserver);
        } catch (RemoteException e) {
            e.printStackTrace();
        }

        final IntentFilter filter = new IntentFilter();
        filter.addAction(AudioManager.VOLUME_CHANGED_ACTION);
        filter.addAction(AudioManager.STREAM_MUTE_CHANGED_ACTION);
        mContext.registerReceiver(mVolumeReceiver, filter);
        mObserver.startObserving(PATH_AUDIOFORMAT_UEVENT);
        mNotImptTvHardwareInputService = (mTvInputManager.getHardwareList() == null) || (mTvInputManager.getHardwareList().isEmpty());
        Log.d(TAG, "mNotImptTvHardwareInputService:"+ mNotImptTvHardwareInputService + ", mTvInputManager.getHardwareList():" + mTvInputManager.getHardwareList());
        mForceManagePatch =  SystemProperties.getBoolean("vendor.media.dtv.force.manage.patch", false);
        Log.d(TAG, "mForceManagePatch :" + mForceManagePatch);
        updateVolume();

        getContentResolver().registerContentObserver(Settings.Global.getUriFor(OutputModeManager.DB_ID_AUDIO_OUTPUT_DEVICE_ARC_ENABLE),
                false, mAudioOutputParametersObserver);
    }

    private ContentObserver mAudioOutputParametersObserver = new ContentObserver(new Handler()) {
        @Override
        public void onChange(boolean selfChange, Uri uri) {
            int currentArcEnable = Settings.Global.getInt(mContext.getContentResolver(), OutputModeManager.DB_ID_AUDIO_OUTPUT_DEVICE_ARC_ENABLE, 0);
            Log.i(TAG, "onChange ARC enable:" + currentArcEnable + " changed");
            if (uri != null && uri.equals(Settings.Global.getUriFor(OutputModeManager.DB_ID_AUDIO_OUTPUT_DEVICE_ARC_ENABLE))) {
                mAudioManager.setHdmiSystemAudioSupported(currentArcEnable != 0);
            }
        }
    };

    @Override
    public void onDestroy() {
        Log.i(TAG, "onDestroy");
        super.onDestroy();
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    private boolean setAdFunction(int msg, int param1) {
        boolean result = false;
        if (mAudioManager == null) {
            Log.i(TAG, "setAdFunction null audioManager");
            return result;
        }
        Log.d(TAG, "setAdFunction msg = " + msg + ", param1 = " + param1);
        switch (msg) {
            case AudioSystemCmdManager.AUDIO_SERVICE_CMD_AD_DUAL_SUPPORT://dual_decoder_surport for ad & main mix on/off
                mAudioManager.setParameters("hal_param_dual_dec_support=" + param1);
                result = true;
                break;
            case AudioSystemCmdManager.AUDIO_SERVICE_CMD_AD_MIX_SUPPORT://Associated audio mixing on/off
                mAudioManager.setParameters("hal_param_ad_mix_enable=" + param1);
                result = true;
                break;

            case AudioSystemCmdManager.AUDIO_SERVICE_CMD_AD_MIX_LEVEL://Associated audio mixing level
                mAudioManager.setParameters("hal_param_dual_dec_mix_level=" + param1);
                result = true;
                break;
            default:
                Log.i(TAG,"setAdFunction unkown  msg:" + msg + ", param1:" + param1);
                break;
        }
              return result;
    }

    public void HandleAudioEvent(int cmd, int param1, int param2, int param3, boolean isDtvkit) {
        Log.i(TAG, "HandleAudioEvent cmd:"+ AudioSystemCmdManager.AudioCmdToString(cmd) +
                ", param1:" + param1 + ", param2:" + param2 +", param3:" + param3 + ", is " + (isDtvkit ? "": "not ") +"Dtvkit.");
        if (mAudioManager == null) {
            Log.e(TAG, "HandleAudioEvent mAudioManager is null");
            return;
        }
        int cmd_index = cmd;
        if (param3 != -1) {
            cmd = cmd + (param3 << mDtvDemuxIdBase);
            param1 = param1 + (param3 << mDtvDemuxIdBase);
            param2 = param2 + (param3 << mDtvDemuxIdBase);
        }
        mDtvDemuxIdCurrentRecive = param3;
        switch (cmd_index) {
            case AudioSystemCmdManager.AUDIO_SERVICE_CMD_SET_SPDIF_PROTECTION__MODE:
                mAudioManager.setParameters("hal_param_dtv_spdif_protection_mode=" + param1);
                break;
            case AudioSystemCmdManager.AUDIO_SERVICE_CMD_SET_DEMUX_INFO:
                //mAudioManager.setParameters("hal_param_dtv_pid=" + param1);
                mAudioManager.setParameters("hal_param_dtv_demux_id=" + param2);
                mAudioManager.setParameters("hal_param_dtv_cmd=" + cmd);
                break;
            case AudioSystemCmdManager.AUDIO_SERVICE_CMD_SET_SECURITY_MEM_LEVEL:
                mAudioManager.setParameters("hal_param_security_mem_level=" + param1);
                break;
            case AudioSystemCmdManager.AUDIO_SERVICE_CMD_SET_MEDIA_SYCN_ID:
                mAudioManager.setParameters("hal_param_media_sync_id=" + param1);
                break;
            case AudioSystemCmdManager.AUDIO_SERVICE_CMD_SET_HAS_VIDEO:
                mCurrentHasDtvVideo = param1;
                mAudioManager.setParameters("hal_param_has_dtv_video=" + param1);
                break;
            case AudioSystemCmdManager.AUDIO_SERVICE_CMD_START_DECODE:
                if (isDtvkit) {
                    mHasReceivedStartDecoderCmd = true;
                    mHasStartedDecoder = true;
                }
                mCurrentFmt = param1;
                mCurrentPid = param2;
                mDtvDemuxIdCurrentWork = param3;
                mAudioManager.setParameters("hal_param_dtv_audio_fmt=" + param1);
                mAudioManager.setParameters("hal_param_dtv_audio_id=" + param2);
                mAudioManager.setParameters("hal_param_dtv_patch_cmd=" + cmd);
                break;
            case AudioSystemCmdManager.AUDIO_SERVICE_CMD_PAUSE_DECODE:
                mAudioManager.setParameters("hal_param_dtv_patch_cmd=" + cmd);
                break;
            case AudioSystemCmdManager.AUDIO_SERVICE_CMD_RESUME_DECODE:
                mAudioManager.setParameters("hal_param_dtv_patch_cmd=" + cmd);
                break;
            case AudioSystemCmdManager.AUDIO_SERVICE_CMD_STOP_DECODE:
                mHasReceivedStartDecoderCmd = false;
                mAudioManager.setParameters("hal_param_dtv_patch_cmd=" + cmd);
                break;
            case AudioSystemCmdManager.AUDIO_SERVICE_CMD_SET_DECODE_AD:
                if (isDtvkit) {
                    mCurrentSubFmt = param1;
                    mCurrentSubPid = param2;
                }
                mAudioManager.setParameters("hal_param_dtv_sub_audio_fmt=" + param1);
                mAudioManager.setParameters("hal_param_dtv_sub_audio_pid=" + param2);
                mAudioManager.setParameters("hal_param_dtv_patch_cmd=" + cmd);
                Log.d(TAG, "HandleAudioEvent hal_param_dtv_sub_audio_fmt:" + param1 + ", hal_param_dtv_sub_audio_pid:" + param2);
                break;
            case AudioSystemCmdManager.AUDIO_SERVICE_CMD_SET_VOLUME:
                //left to do
                break;
            case AudioSystemCmdManager.AUDIO_SERVICE_CMD_SET_MUTE:
                mAudioManager.setParameters("hal_param_tv_mute=" + param1); /* 1:mute, 0:unmute */
                break;
            case AudioSystemCmdManager.AUDIO_SERVICE_CMD_SET_OUTPUT_MODE:
                //mAudioManager.setParameters("hal_param_dtv_patch_cmd=" + cmd);
                //mAudioManager.setParameters("hal_param_audio_output_mode=" + param1); /* refer to AM_AOUT_OutputMode_t */
                break;
            case AudioSystemCmdManager.AUDIO_SERVICE_CMD_SET_PRE_GAIN:
                mAudioManager.setParameters("hal_param_dtv_patch_cmd=" + cmd);
                break;
            case AudioSystemCmdManager.AUDIO_SERVICE_CMD_SET_PRE_MUTE:
                mAudioManager.setParameters("hal_param_dtv_patch_cmd=" + cmd);
                break;
            case AudioSystemCmdManager.AUDIO_SERVICE_CMD_OPEN_DECODER:
                updateAudioSourceAndAudioSink();
                if (mNotImptTvHardwareInputService && !mHasOpenedDecoder)
                    handleAudioSinkUpdated();
                reStartAdecDecoderIfPossible();
                synchronized (mLock) {
                    mAudioManager.setParameters("hal_param_dtv_fmt=" + param1);
                    mAudioManager.setParameters("hal_param_dtv_pid=" + param2);
                }
                mAudioManager.setParameters("hal_param_dtv_patch_cmd=" + cmd);
                mHasOpenedDecoder = true;
                break;
            case AudioSystemCmdManager.AUDIO_SERVICE_CMD_CLOSE_DECODER://
                mAudioManager.setParameters("hal_param_dtv_patch_cmd=" + cmd);
                if (mAudioPatch != null) {
                   Log.d(TAG, "ADEC_CLOSE_DECODER mAudioPatch:"
                        + mAudioPatch);
                    mAudioManager.releaseAudioPatch(mAudioPatch);
                }
                mAudioPatch = null;
                mAudioSource = null;
                mHasStartedDecoder = false;
                mHasOpenedDecoder = false;
                mMixAdSupported = false;
                mCurrentSubFmt = -1;
                mCurrentSubPid = -1;
                break;
            case AudioSystemCmdManager.AUDIO_SERVICE_CMD_AD_DUAL_SUPPORT:
                setAdFunction(AudioSystemCmdManager.AUDIO_SERVICE_CMD_AD_DUAL_SUPPORT, param1);
                break;
            case AudioSystemCmdManager.AUDIO_SERVICE_CMD_AD_MIX_SUPPORT://Associated audio mixing on/off
                mMixAdSupported = (param1 != 0);
                setAdFunction(AudioSystemCmdManager.AUDIO_SERVICE_CMD_AD_DUAL_SUPPORT, param1);
                setAdFunction(AudioSystemCmdManager.AUDIO_SERVICE_CMD_AD_MIX_SUPPORT, param1);
                Log.d(TAG, "HandleAudioEvent mMixAdSupported:" + mMixAdSupported);
                break;
            case AudioSystemCmdManager.AUDIO_SERVICE_CMD_AD_MIX_LEVEL:
                setAdFunction(AudioSystemCmdManager.AUDIO_SERVICE_CMD_AD_MIX_LEVEL, param2);
                break;
            case AudioSystemCmdManager.AUDIO_SERVICE_CMD_SET_MEDIA_PRESENTATION_ID:
                mAudioManager.setParameters("hal_param_dtv_media_presentation_id=" + param1);
                break;
            case AudioSystemCmdManager.AUDIO_SERVICE_CMD_SET_AUDIO_PATCH_MANAGE_MODE:
                boolean isDvbPlayback = (param1 == 0);
                int forceManagePatchMode = param2;
                if (param3 != 0) {
                    isDvbPlayback = ((param1 -(param3 << mDtvDemuxIdBase)) == 0);
                    forceManagePatchMode = (param2 -(param3 << mDtvDemuxIdBase));
                }
                boolean hasTif = !(mTvInputManager.getHardwareList() == null || mTvInputManager.getHardwareList().isEmpty());

                if (mForceManagePatch) {
                    mNotImptTvHardwareInputService = true;
                } else if (forceManagePatchMode == 0) {
                    // force disable
                    mNotImptTvHardwareInputService = false;
                } else if (forceManagePatchMode == 1) {
                    // force enable
                    mNotImptTvHardwareInputService = true;
                } else {
                    // auto
                    if (hasTif && isDvbPlayback) {
                        mNotImptTvHardwareInputService = false;
                    } else {
                        mNotImptTvHardwareInputService = true;
                    }
                }

                Log.i(TAG, "mForceManagePatch: " + mForceManagePatch
                    + ", isDvbPlayback: " + isDvbPlayback
                    + ", forceManagePatchMode: " + forceManagePatchMode
                    + ", hasTif: " + hasTif
                    + ", so mNotImptTvHardwareInputService set to " + mNotImptTvHardwareInputService);
                break;
            default:
                Log.w(TAG,"HandleAudioEvent unkown audio cmd:" + cmd);
                break;
        }
    }

    private void updateAudioSourceAndAudioSink() {
        mAudioSource = findAudioDevicePort(AudioManager.DEVICE_IN_TV_TUNER, "");
        findAudioSinkFromAudioPolicy(mAudioSink);
    }

    /**
     * Convert volume from float [0.0 - 1.0] to media volume UI index
     */
    private int volumeToMediaIndex(float volume) {
        return mCurrentMinIndex + (int)(volume * (mCurrentMaxIndex - mCurrentMinIndex));
    }

    /**
     * Convert media volume UI index to Milli Bells for a given output device type
     * and gain controller
     */
    private int indexToGainMbForDevice(int index, int device, AudioGain gain) {
        float gainDb = AudioSystem.getStreamVolumeDB(AudioManager.STREAM_MUSIC,
                                                       index,
                                                       device);
        float maxGainDb = AudioSystem.getStreamVolumeDB(AudioManager.STREAM_MUSIC,
                                                        mCurrentMaxIndex,
                                                        device);
        float minGainDb = AudioSystem.getStreamVolumeDB(AudioManager.STREAM_MUSIC,
                                                        mCurrentMinIndex,
                                                        device);

        // Rescale gain from dB to mB and within gain conroller range and snap to steps
        int gainMb = (int)((float)(((gainDb - minGainDb) * (gain.maxValue() - gain.minValue()))
                        / (maxGainDb - minGainDb)) + gain.minValue());
        gainMb = (int)(((float)gainMb / gain.stepValue()) * gain.stepValue());

        return gainMb;
    }

    private void updateVolume() {
        mCurrentMaxIndex = mAudioManager.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
        mCurrentMinIndex = mAudioManager.getStreamMinVolume(AudioManager.STREAM_MUSIC);
        mCurrentIndex = mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC);
        Log.d(TAG, "updateVolume mCurrentIndex:"+ mCurrentIndex + ", mCommitedIndex:" + mCommitedIndex);
    }

    private void handleVolumeChange(Context context, Intent intent) {
        String action = intent.getAction();
        switch (action) {
            case AudioManager.VOLUME_CHANGED_ACTION: {
                int streamType = intent.getIntExtra(AudioManager.EXTRA_VOLUME_STREAM_TYPE, -1);
                if (streamType != AudioManager.STREAM_MUSIC) {
                    return;
                }
                int index = intent.getIntExtra(AudioManager.EXTRA_VOLUME_STREAM_VALUE, 0);
                if (index == mCurrentIndex) {
                    return;
                }
                Log.i(TAG, "handleVolumeChange VOLUME_CHANGED index:" + index);
                mCurrentIndex = index;
                break;
            }
            case AudioManager.STREAM_MUTE_CHANGED_ACTION: {
                int streamType = intent.getIntExtra(AudioManager.EXTRA_VOLUME_STREAM_TYPE, -1);
                if (streamType != AudioManager.STREAM_MUSIC) {
                    return;
                }
                Log.i(TAG, "handleVolumeChange MUTE_CHANGED");
                // volume index will be updated at onMediaStreamVolumeChanged() through
                // updateVolume().
                break;
            }
            default:
                Slog.w(TAG, "handleVolumeChange action:" + action + ", Unrecognized intent: " + intent);
                return;
        }
        synchronized (mLock) {
            if (mNotImptTvHardwareInputService) {
                updateAudioConfigLocked();
            }
        }
    }

    private float getMediaStreamVolume() {
        return (float) mCurrentIndex / (float) mCurrentMaxIndex;
    }

    private void handleAudioSinkUpdated() {
        synchronized (mLock) {
            updateAudioConfigLocked();
        }
    }

    private boolean updateAudioSinkLocked() {
        List<AudioDevicePort> previousSink = mAudioSink;
        mAudioSink = new ArrayList<>();
        findAudioSinkFromAudioPolicy(mAudioSink);

        // Returns true if mAudioSink and previousSink differs.
        if (mAudioSink.size() != previousSink.size()) {
            return true;
        }
        previousSink.removeAll(mAudioSink);
        return !previousSink.isEmpty();
    }

    private void findAudioSinkFromAudioPolicy(List<AudioDevicePort> sinks) {
        sinks.clear();
        ArrayList<AudioPort> audioPorts = new ArrayList<>();
        if (AudioManager.listAudioPorts(audioPorts) != AudioManager.SUCCESS) {
            Log.w(TAG, "findAudioSinkFromAudioPolicy listAudioPorts failed");
            return;
        }
        int sinkDevice = mAudioManager.getDevicesForStream(AudioManager.STREAM_MUSIC);
        AudioDevicePort port;
        for (AudioPort audioPort : audioPorts) {
            if (audioPort instanceof AudioDevicePort) {
                port = (AudioDevicePort)audioPort;
                if ((port.type() & sinkDevice) != 0 && mAudioManager.isOutputDevice(port.type())) {
                    sinks.add(port);
                }
            }
        }
    }

    private AudioDevicePort findAudioDevicePort(int type, String address) {
        if (type == AudioManager.DEVICE_NONE) {
            return null;
        }
        ArrayList<AudioPort> audioPorts = new ArrayList<>();
        if (AudioManager.listAudioPorts(audioPorts) != AudioManager.SUCCESS) {
            Log.w(TAG, "findAudioDevicePort listAudioPorts failed");
            return null;
        }
        AudioDevicePort port;
        for (AudioPort audioPort : audioPorts) {
            if (audioPort instanceof AudioDevicePort) {
                port = (AudioDevicePort)audioPort;
                if (port.type() == type && port.address().equals(address)) {
                    return port;
                }
            }
        }
        return null;
    }

    private void reStartAdecDecoderIfPossible() {
        Log.i(TAG, "reStartAdecDecoderIfPossiblem HasOpenedDecoder:" + mHasOpenedDecoder +
                   " StartDecoderCmd:" + mHasReceivedStartDecoderCmd +
                   ", mMixAdSupported:" + mMixAdSupported);
        if (mAudioSource != null &&!mAudioSink.isEmpty() && !mHasOpenedDecoder ) {
            mAudioManager.setParameters("hal_param_tuner_in=dtv");
            if (mHasReceivedStartDecoderCmd) {
                mAudioManager.setParameters("hal_param_dtv_audio_fmt="+mCurrentFmt);
                mAudioManager.setParameters("hal_param_has_dtv_video="+mCurrentHasDtvVideo);
                int cmd = AudioSystemCmdManager.AUDIO_SERVICE_CMD_START_DECODE + (mDtvDemuxIdCurrentWork << mDtvDemuxIdBase);
                mAudioManager.setParameters("hal_param_dtv_patch_cmd=" + cmd);
                mHasStartedDecoder = true;
             }
        }
    }

    private void updateAudioConfigLocked() {

        boolean sinkUpdated = updateAudioSinkLocked();

        if (mAudioSource == null || mAudioSink.isEmpty()) {
            Log.i(TAG, "updateAudioConfigLocked return, mAudioSource:" +
                    mAudioSource + ", mAudioSink empty:" +  mAudioSink.isEmpty());
            if (mAudioPatch != null) {
                mAudioManager.releaseAudioPatch(mAudioPatch);
                mAudioPatch = null;
                mHasStartedDecoder = false;
            }
            mCommittedSourceVolume = -1f;
            mCommitedIndex = -1;
            return;
        }

        AudioPortConfig sourceConfig = mAudioSource.activeConfig();
        List<AudioPortConfig> sinkConfigs = new ArrayList<>();
        AudioPatch[] audioPatchArray = new AudioPatch[] { mAudioPatch };
        boolean shouldRecreateAudioPatch = sinkUpdated;
        boolean shouldApplyGain = false;

        Log.i(TAG, "updateAudioConfigLocked sinkUpdated:" + sinkUpdated + ", mAudioPatch is empty:"
                + (mAudioPatch == null));
         //mAudioPatch should not be null when current hardware is active.
        if (mAudioPatch == null) {
            shouldRecreateAudioPatch = true;
            mHasStartedDecoder = false;
        }

        for (AudioDevicePort audioSink : mAudioSink) {
            AudioPortConfig sinkConfig = audioSink.activeConfig();
            int sinkSamplingRate = mDesiredSamplingRate;
            int sinkChannelMask = mDesiredChannelMask;
            int sinkFormat = mDesiredFormat;
            // If sinkConfig != null and values are set to default,
            // fill in the sinkConfig values.
            if (sinkConfig != null) {
                if (sinkSamplingRate == 0) {
                    sinkSamplingRate = sinkConfig.samplingRate();
                }
                if (sinkChannelMask == AudioFormat.CHANNEL_OUT_DEFAULT) {
                    sinkChannelMask = sinkConfig.channelMask();
                }
                if (sinkFormat == AudioFormat.ENCODING_DEFAULT) {
                    sinkFormat = sinkConfig.format();
                }
            }

            if (sinkConfig == null
                    || sinkConfig.samplingRate() != sinkSamplingRate
                    || sinkConfig.channelMask() != sinkChannelMask
                    || sinkConfig.format() != sinkFormat) {
                // Check for compatibility and reset to default if necessary.
                if (!intArrayContains(audioSink.samplingRates(), sinkSamplingRate)
                        && audioSink.samplingRates().length > 0) {
                    sinkSamplingRate = audioSink.samplingRates()[0];
                }
                if (!intArrayContains(audioSink.channelMasks(), sinkChannelMask)) {
                    sinkChannelMask = AudioFormat.CHANNEL_OUT_DEFAULT;
                }
                if (!intArrayContains(audioSink.formats(), sinkFormat)) {
                    sinkFormat = AudioFormat.ENCODING_DEFAULT;
                }
                sinkConfig = audioSink.buildConfig(sinkSamplingRate, sinkChannelMask,
                        sinkFormat, null);
                shouldRecreateAudioPatch = true;
            }
            sinkConfigs.add(sinkConfig);
        }

        // Set source gain according to media volume
        // We apply gain on the source but use volume curve corresponding to the sink to match
        // what is done for software source in audio policy manager
//        updateVolume();
//        float volume = mSourceVolume * getMediaStreamVolume();
//        AudioGainConfig sourceGainConfig = null;
//        if (mAudioSource.gains().length > 0 && volume != mCommittedSourceVolume) {
//            AudioGain sourceGain = null;
//            for (AudioGain gain : mAudioSource.gains()) {
//                if ((gain.mode() & AudioGain.MODE_JOINT) != 0) {
//                    sourceGain = gain;
//                    break;
//                }
//            }
//            // NOTE: we only change the source gain in MODE_JOINT here.
//            if (sourceGain != null) {
//                int steps = (sourceGain.maxValue() - sourceGain.minValue())
//                        / sourceGain.stepValue();
//                int gainValue = sourceGain.minValue();
//                if (volume < 1.0f) {
//                    gainValue += sourceGain.stepValue() * (int) (volume * steps + 0.5);
//                } else {
//                    gainValue = sourceGain.maxValue();
//                }
//                // size of gain values is 1 in MODE_JOINT
//                int[] gainValues = new int[] { gainValue };
//                sourceGainConfig = sourceGain.buildConfig(AudioGain.MODE_JOINT,
//                        sourceGain.channelMask(), gainValues, 0);
//            } else {
//                Slog.w(TAG, "No audio source gain with MODE_JOINT support exists.");
//            }
//        }
        updateVolume();

        AudioGainConfig sourceGainConfig = null;
        if (mAudioSource.gains().length > 0) {
            AudioGain sourceGain = null;
            for (AudioGain gain : mAudioSource.gains()) {
                if ((gain.mode() & AudioGain.MODE_JOINT) != 0) {
                    sourceGain = gain;
                    break;
                }
            }
            if (sourceGain != null && ((mSourceVolume != mCommittedSourceVolume) ||
                                       (mCurrentIndex != mCommitedIndex))) {
                // use first sink device as referrence for volume curves
                int deviceType = mAudioSink.get(0).type();

                // first convert source volume to mBs
                int sourceIndex = volumeToMediaIndex(mSourceVolume);
                int sourceGainMb = indexToGainMbForDevice(sourceIndex, deviceType, sourceGain);

                // then convert media volume index to mBs
                int indexGainMb = indexToGainMbForDevice(mCurrentIndex, deviceType, sourceGain);

                Log.d(TAG, "updateAudioConfigLocked mCurrentIndex= "+ mCurrentIndex + ",mCommitedIndex="+mCommitedIndex+",indexGainMb="+indexGainMb);

                // apply combined gains
                int gainValueMb = sourceGainMb + indexGainMb;
                gainValueMb = Math.max(sourceGain.minValue(),
                                       Math.min(sourceGain.maxValue(), gainValueMb));

                // NOTE: we only change the source gain in MODE_JOINT here.
                // size of gain values is 1 in MODE_JOINT
                int[] gainValues = new int[] { gainValueMb };
                sourceGainConfig = sourceGain.buildConfig(AudioGain.MODE_JOINT,
                        sourceGain.channelMask(), gainValues, 0);
            } else {
                Slog.w(TAG, "updateAudioConfigLocked No audio source gain with MODE_JOINT support exists.");
            }
        }

        // sinkConfigs.size() == mAudioSink.size(), and mAudioSink is guaranteed to be
        // non-empty at the beginning of this method.
        AudioPortConfig sinkConfig = sinkConfigs.get(0);
        if (sourceConfig == null || sourceGainConfig != null) {
            int sourceSamplingRate = 0;
            if (intArrayContains(mAudioSource.samplingRates(), sinkConfig.samplingRate())) {
                sourceSamplingRate = sinkConfig.samplingRate();
            } else if (mAudioSource.samplingRates().length > 0) {
                // Use any sampling rate and hope audio patch can handle resampling...
                sourceSamplingRate = mAudioSource.samplingRates()[0];
            }
            int sourceChannelMask = AudioFormat.CHANNEL_IN_DEFAULT;
            for (int inChannelMask : mAudioSource.channelMasks()) {
                if (AudioFormat.channelCountFromOutChannelMask(sinkConfig.channelMask())
                        == AudioFormat.channelCountFromInChannelMask(inChannelMask)) {
                    sourceChannelMask = inChannelMask;
                    break;
                }
            }
            int sourceFormat = AudioFormat.ENCODING_DEFAULT;
            if (intArrayContains(mAudioSource.formats(), sinkConfig.format())) {
                sourceFormat = sinkConfig.format();
            }
            sourceConfig = mAudioSource.buildConfig(sourceSamplingRate, sourceChannelMask,
                    sourceFormat, sourceGainConfig);

            if (mAudioPatch != null) {
                shouldApplyGain = true;
            } else {
                shouldRecreateAudioPatch = true;
            }
        }
        Log.i(TAG, "updateAudioConfigLocked recreatePatch:" + shouldRecreateAudioPatch);
        if (shouldRecreateAudioPatch) {
            //mCommittedSourceVolume = volume;
            if (mAudioPatch != null) {
                mAudioManager.releaseAudioPatch(mAudioPatch);
                audioPatchArray[0] = null;
                mHasStartedDecoder = false;
            }
            mAudioManager.createAudioPatch(
                    audioPatchArray,
                    new AudioPortConfig[] { sourceConfig },
                    sinkConfigs.toArray(new AudioPortConfig[sinkConfigs.size()]));
            mAudioPatch = audioPatchArray[0];
            Log.d(TAG,"createAudioPatch end" + mAudioPatch);
            if (sourceGainConfig != null) {
                mCommitedIndex = mCurrentIndex;
                mCommittedSourceVolume = mSourceVolume;
            }
        }
        if (sourceGainConfig != null &&
                (shouldApplyGain || shouldRecreateAudioPatch)) {
            //mCommittedSourceVolume = volume;
            mAudioManager.setAudioPortGain(mAudioSource, sourceGainConfig);
            mCommitedIndex = mCurrentIndex;
            mCommittedSourceVolume = mSourceVolume;
        }
    }

    private static boolean intArrayContains(int[] array, int value) {
        for (int element : array) {
            if (element == value) return true;
        }
        return false;
    }

    private final IAudioSystemCmdService.Stub mBinder = new IAudioSystemCmdService.Stub() {
        public void setParameters(String arg) {
            if (DroidLogicUtils.getAudioDebugEnable()) {
                Log.d(TAG, "setParameters arg:" + arg);
            }
            mAudioManager.setParameters(arg);
            return;
        }

        public String getParameters(String arg) {
            String value = mAudioManager.getParameters(arg);
            if (DroidLogicUtils.getAudioDebugEnable()) {
                Log.d(TAG, "getParameters arg:" + arg +", value:" + value);
            }
            return value;
        }

        public void handleAdtvAudioEvent(int cmd, int param1, int param2) {
            if (DroidLogicUtils.getAudioDebugEnable()) {
                Log.d(TAG, "handleAdtvAudioEvent cmd:" + AudioSystemCmdManager.AudioCmdToString(cmd));
            }
            HandleAudioEvent(cmd, param1, param2, 0, false);
            return;
        }

        public void openTvAudio(int sourceType) {
            Log.i(TAG, "openTvAudio set source type:" + DroidLogicTvUtils.sourceTypeToString(sourceType));
            mHandler.removeCallbacks(mHandleTvAudioRunnable);
            if (sourceType == DroidLogicTvUtils.SOURCE_TYPE_ATV) {
                mAudioManager.setParameters("hal_param_tuner_in=atv");
            } else if (sourceType == DroidLogicTvUtils.SOURCE_TYPE_DTV) {
                mAudioManager.setParameters("hal_param_tuner_in=dtv");
            } else {
                Log.w(TAG, "openTvAudio unsupported source type:" + sourceType );
            }
            mCurSourceType = sourceType;
            return;
        }

        public void closeTvAudio() {
            Log.i(TAG, "closeTvAudio");
            mHandler.removeCallbacks(mHandleTvAudioRunnable);
            mCurSourceType = DroidLogicTvUtils.SOURCE_TYPE_OTHER;
            mCurrentFmt = -1;
            mCurrentHasDtvVideo = -1;
            return;
        }
    };
}
