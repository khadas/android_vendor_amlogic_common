/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC AudioEffectsSettingManagerService
 */

package com.droidlogic.audioservice.services;

import android.app.Service;
import android.content.Context;
import android.content.ContentProviderClient;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.BroadcastReceiver;
import android.database.ContentObserver;
import android.media.tv.TvContract;
import android.net.Uri;
import android.os.IBinder;
import android.os.UserHandle;
import android.os.Binder;
import android.os.Handler;
import android.os.Message;
import android.os.SystemProperties;
import android.provider.Settings;
import android.text.TextUtils;
import android.util.Log;

import com.droidlogic.app.DroidLogicUtils;
import com.droidlogic.app.OutputModeManager;
import com.droidlogic.app.SystemControlManager;
import com.droidlogic.app.tv.AudioEffectManager;
import com.droidlogic.audioservice.settings.SoundEffectManager;

/**
 * This Service modifies Audio and Picture Quality TV Settings.
 * It contains platform specific implementation of the TvTweak IOemSettings interface.
 */
public class AudioEffectsService extends Service {
    private static final String TAG = AudioEffectsService.class.getSimpleName();
    public static final String PACKEGE_NANME = "com.droidlogic";

    private static boolean DEBUG = true;
    private SoundEffectManager mSoundEffectManager;
    private AudioEffectsService mAudioEffectsService;
    private Context mContext = null;

    public AudioEffectsService() {
        mAudioEffectsService = this;
    }

    @Override
    public void onCreate() {
        if (DEBUG) Log.d(TAG, "AudioEffectsService onCreate");
        mContext = this;
        mSoundEffectManager = SoundEffectManager.getInstance(mContext);
        mHandler.sendEmptyMessage(MSG_CHECK_BOOTVIDEO_FINISHED);
    }

    @Override
    public void onDestroy() {
        if (DEBUG) Log.d(TAG, "onDestroy");
        if (mSoundEffectManager != null) {
            mSoundEffectManager.cleanupAudioEffects();
        }
        unregisterCommandReceiver(this);
    }

    @Override
    public void onLowMemory() {
        if (DEBUG) Log.w(TAG, "onLowMemory");
    }

    @Override
    public IBinder onBind(Intent intent) {
        if (DEBUG) Log.d(TAG, "onBind");
        return mBinder;
    }

    private boolean isBootvideoStopped() {
        ContentProviderClient tvProvider = getContentResolver().acquireContentProviderClient(TvContract.AUTHORITY);

        return (tvProvider != null) &&
                (((SystemProperties.getInt("persist.vendor.media.bootvideo", 50)  > 100)
                        && TextUtils.equals(SystemProperties.get("service.bootvideo.exit", "1"), "0"))
                || ((SystemProperties.getInt("persist.vendor.media.bootvideo", 50)  <= 100)));
    }

    private static final int MSG_CHECK_BOOTVIDEO_FINISHED = 0;
    private Handler mHandler = new Handler() {
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_CHECK_BOOTVIDEO_FINISHED:
                    if (isBootvideoStopped()) {
                        handleActionStartUp();
                    } else {
                        mHandler.sendEmptyMessageDelayed(MSG_CHECK_BOOTVIDEO_FINISHED, 10);
                    }
                    break;
                default:
                    break;
            }
        }
    };

    private final IAudioEffectsService.Stub mBinder = new IAudioEffectsService.Stub() {
        public void createAudioEffects() {
            mSoundEffectManager.createAudioEffects();
        }

        public boolean isSupportVirtualX() {
            return mSoundEffectManager.isSupportVirtualX();
        }

        public void setDtsVirtualXMode(int virtalXMode) {
            mSoundEffectManager.setDtsVirtualXMode(virtalXMode);
        }

        public int getDtsVirtualXMode() {
            return mSoundEffectManager.getDtsVirtualXMode();
        }

        public void setDtsTruVolumeHdEnable(boolean enable) {
            mSoundEffectManager.setDtsTruVolumeHdEnable(enable);
        }

        public boolean getDtsTruVolumeHdEnable() {
            return mSoundEffectManager.getDtsTruVolumeHdEnable();
        }

        public int getSoundModeStatus () {
            return mSoundEffectManager.getSoundModeStatus();
        }

        //return current is eq or dap
        public int getSoundModule() {
            return mSoundEffectManager.getSoundModule();
        }

        public int getTrebleStatus () {
            return mSoundEffectManager.getTrebleStatus();
        }

        public int getBassStatus () {
            return mSoundEffectManager.getBassStatus();
        }

        public int getBalanceStatus () {
            return mSoundEffectManager.getBalanceStatus();
        }

        public boolean getAgcEnableStatus () {
            return mSoundEffectManager.getAgcEnableStatus();
        }

        public int getAgcMaxLevelStatus () {
            return mSoundEffectManager.getAgcMaxLevelStatus();
        }

        public int getAgcAttackTimeStatus () {
            return mSoundEffectManager.getAgcAttackTimeStatus();
        }

        public int getAgcReleaseTimeStatus () {
            return mSoundEffectManager.getAgcReleaseTimeStatus();
        }

        public int getAgcSourceIdStatus () {
            return mSoundEffectManager.getAgcSourceIdStatus();
        }

        public int getVirtualSurroundStatus() {
            return mSoundEffectManager.getVirtualSurroundStatus();
        }

        public void setSoundMode (int mode) {
            mSoundEffectManager.setSoundMode(mode);
        }

        public void setSoundModeByObserver (int mode) {
            mSoundEffectManager.setSoundModeByObserver(mode);
        }

        public void setUserSoundModeParam(int bandNumber, int value) {
            mSoundEffectManager.setUserSoundModeParam(bandNumber, value);
        }

        public int getUserSoundModeParam(int bandNumber) {
            return mSoundEffectManager.getUserSoundModeParam(bandNumber);
        }

        public void setTreble (int step) {
            mSoundEffectManager.setTreble (step);
        }

        public void setBass (int step) {
            mSoundEffectManager.setBass (step);
        }

        public void setBalance (int step) {
            mSoundEffectManager.setBalance (step);
        }

        public void setSurroundEnable(boolean enable) {
            mSoundEffectManager.setSurroundEnable(enable);
        }

        public boolean getSurroundEnable() {
            return mSoundEffectManager.getSurroundEnable();
        }

        public void setDialogClarityMode(int mode) {
            mSoundEffectManager.setDialogClarityMode(mode);
        }

        public int getDialogClarityMode() {
            return mSoundEffectManager.getDialogClarityMode();
        }

        public void setTruBassEnable(boolean enable) {
            mSoundEffectManager.setTruBassEnable(enable);
        }

        public boolean getTruBassEnable() {
            return mSoundEffectManager.getTruBassEnable();
        }

        public void setAgcEnable (boolean enable) {
            mSoundEffectManager.setAgcEnable(enable);
        }

        public void setAgcMaxLevel (int step) {
            mSoundEffectManager.setAgcMaxLevel(step);
        }

        public void setAgcAttackTime (int step) {
            mSoundEffectManager.setAgcAttackTime(step);
        }

        public void setAgcReleaseTime (int step) {
            mSoundEffectManager.setAgcReleaseTime(step);
        }

        public void setSourceIdForAvl (int step) {
            mSoundEffectManager.setSourceIdForAvl(step);
        }

        public void setVirtualSurround (int mode) {
            mSoundEffectManager.setVirtualSurround (mode);
        }

        public void setDbxEnable(boolean enable) {
            mSoundEffectManager.setDbxEnable(enable);
        }

        public boolean getDbxEnable() {
            return mSoundEffectManager.getDbxEnable();
        }

        public void setDbxSoundMode(int dbxMode) {
            mSoundEffectManager.setDbxSoundMode(dbxMode);
        }

        public int getDbxSoundMode() {
            return mSoundEffectManager.getDbxSoundMode();
        }

        public void setDbxAdvancedModeParam(int paramType, int value) {
            mSoundEffectManager.setDbxAdvancedModeParam(paramType, value);
        }

        public int getDbxAdvancedModeParam(int paramType) {
            return mSoundEffectManager.getDbxAdvancedModeParam(paramType);
        }

        public void cleanupAudioEffects() {
            mSoundEffectManager.cleanupAudioEffects();
        }

        public void initSoundEffectSettings() {
            mSoundEffectManager.initSoundEffectSettings();
        }

        public void resetSoundEffectSettings() {
            Log.d(TAG, "resetSoundEffectSettings");
            mSoundEffectManager.resetSoundEffectSettings();
        }

        public void setDapParam(int id, int value) {
//            mSoundEffectManager.saveDapParam(id, value);
            mSoundEffectManager.setDapParam(id, value);
        }

        public int getDapParam(int id) {
            return mSoundEffectManager.getDapParam(id);
        }

        public void initDapAudioEffect() {
            mSoundEffectManager.initDapAudioEffect();
        }
    };

    private void handleActionStartUp() {
        boolean isDapValid = OutputModeManager.getInstance(mContext).isAudioSupportMs12System();
        Log.i(TAG, "handleActionStartUp needAudioEffectFeture:" + DroidLogicUtils.isTv() + ", isDapValid:" + isDapValid);
        if (DroidLogicUtils.isTv()) {
            // This will apply the saved audio settings on boot
            mSoundEffectManager.createAudioEffects();
            mSoundEffectManager.initSoundEffectSettings();
            registerCommandReceiver(this);
        }
        if (isDapValid) {
            mSoundEffectManager.initDapAudioEffect();
        }
    }

    private static final String RESET_ACTION = "droid.action.resetsoundeffect";
    private static final String AVL_SOURCE_ACTION = "droid.action.avlmodule";

    private void registerCommandReceiver(Context context) {
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(RESET_ACTION);
        intentFilter.addAction(AVL_SOURCE_ACTION);
        context.registerReceiver(mSoundEffectSettingsReceiver, intentFilter);
        context.getContentResolver().registerContentObserver(Settings.Global.getUriFor(SoundEffectManager.DB_ID_SOUND_EFFECT_SOUND_MODE), false,
                mSoundEffectParametersObserver);
        context.getContentResolver().registerContentObserver(Settings.Global.getUriFor(SoundEffectManager.DB_ID_SOUND_EFFECT_SOUND_MODE_EQ_VALUE), false,
                mSoundEffectParametersObserver);
        context.getContentResolver().registerContentObserver(Settings.Global.getUriFor(SoundEffectManager.DB_ID_SOUND_EFFECT_SOUND_MODE_DAP_VALUE), false,
                mSoundEffectParametersObserver);
    }

    private void unregisterCommandReceiver(Context context) {
        context.unregisterReceiver(mSoundEffectSettingsReceiver);
        context.getContentResolver().unregisterContentObserver(mSoundEffectParametersObserver);
    }

    private ContentObserver mSoundEffectParametersObserver = new ContentObserver(new Handler()) {
        @Override
        public void onChange(boolean selfChange, Uri uri) {
            if (uri != null) {
                if (uri.equals(Settings.Global.getUriFor(SoundEffectManager.DB_ID_SOUND_EFFECT_SOUND_MODE))
                        || uri.equals(Settings.Global.getUriFor(SoundEffectManager.DB_ID_SOUND_EFFECT_SOUND_MODE_EQ_VALUE))
                        || uri.equals(Settings.Global.getUriFor(SoundEffectManager.DB_ID_SOUND_EFFECT_SOUND_MODE_DAP_VALUE))) {
                    int mode = Settings.Global.getInt(mContext.getContentResolver(), uri.getLastPathSegment(), AudioEffectManager.EQ_SOUND_MODE_STANDARD);
                    Log.d(TAG, "onChange setSoundMode " + uri.getLastPathSegment() + ":" + mode);
                    mSoundEffectManager.setSoundModeByObserver(mode);
                }
            }
        }
    };

    private final BroadcastReceiver mSoundEffectSettingsReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (DEBUG) Log.d(TAG, "intent = " + intent);
            if (intent != null) {
                if (RESET_ACTION.equals(intent.getAction())) {
                    mSoundEffectManager.resetSoundEffectSettings();
                } else if (AVL_SOURCE_ACTION.equals(intent.getAction())) {
                    mSoundEffectManager.setSourceIdForAvl(intent.getIntExtra("source_id", SoundEffectManager.DEFAULT_AGC_SOURCE_ID));
                }
            }
        }
    };
}
