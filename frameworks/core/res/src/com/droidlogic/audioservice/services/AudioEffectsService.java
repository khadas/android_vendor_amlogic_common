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
import com.droidlogic.app.AudioEffectManager;
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
        handleActionStartUp();
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

    private final IAudioEffectsService.Stub mBinder = new IAudioEffectsService.Stub() {
        public void createAudioEffects() {
            mSoundEffectManager.createAudioEffects();
        }

        public boolean isSupportVirtualX() {
            return mSoundEffectManager.isSupportVirtualX();
        }

        public void setDtsVirtualXMode(int virtualXMode) {
            mSoundEffectManager.setDtsVirtualXMode(virtualXMode);
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

        public int getVirtualSurroundStatus() {
            return mSoundEffectManager.getVirtualSurroundStatus();
        }

        public void setSoundMode (int mode) {
            mSoundEffectManager.setSoundMode(mode);
        }

        public void setSoundModeByObserver (int mode, int bandSum) {
            mSoundEffectManager.setSoundModeByObserver(mode, bandSum);
        }

        public void setUserSoundModeParam(int bandNumber, int value, int bandSum) {
            mSoundEffectManager.setUserSoundModeParam(bandNumber, value, bandSum);
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

        public void setVirtualSurround (int mode) {
            mSoundEffectManager.setVirtualSurround (mode);
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
            //mSoundEffectManager.saveDapParam(id, value);
            mSoundEffectManager.setDapParam(id, value);
        }

        public int getDapParam(int id) {
            return mSoundEffectManager.getDapParam(id);
        }

        public void initDapAudioEffect() {
            mSoundEffectManager.initDapAudioEffect();
        }

        public void setDpeParam(int id, int value) {
            mSoundEffectManager.setDpeParam(id, value);
        }

        public int getDpeParam(int id) {
            return mSoundEffectManager.getDpeParam(id);
        }

        public void initDpeAudioEffect() {
            mSoundEffectManager.initDpeAudioEffect();
        }

        public void setAudioEffectOn(int id, boolean dbSwitch) {
            mSoundEffectManager.setAudioEffectOn(id, dbSwitch);
        }

        public boolean isAudioEffectOn(int id) {
            return mSoundEffectManager.isAudioEffectOn(id);
        }
        public void setHpeqBandNum(int id, int value) {
            mSoundEffectManager.setHpeqBandNum(id, value);
        }
        public int getHpeqBandNum(int id) {
            return mSoundEffectManager.getHpeqBandNum(id);
        }
    };

    private void handleActionStartUp() {
        boolean isDapValid = OutputModeManager.getInstance(mContext).isAudioSupportMs12System();
        Log.i(TAG, "handleActionStartUp needAudioEffectFeture:" + DroidLogicUtils.isTv() + ", isDapValid:" + isDapValid);
        // This will apply the saved audio settings on boot
        if (mSoundEffectManager != null) {
            mSoundEffectManager.createAudioEffectsByIndex();
        }
        registerCommandReceiver(this);
    }

    private static final String RESET_ACTION = "droid.action.resetsoundeffect";

    private void registerCommandReceiver(Context context) {
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(RESET_ACTION);
        context.registerReceiver(mSoundEffectSettingsReceiver, intentFilter, context.RECEIVER_EXPORTED);
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
                    int bandSum = mSoundEffectManager.getHpeqBandNum(AudioEffectManager.DEBUG_HPEQ_BAND_NUM_UI);
                    Log.d(TAG, "onChange setSoundMode " + uri.getLastPathSegment() + ":" + mode);
                    mSoundEffectManager.setSoundModeByObserver(mode, bandSum);
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
                }
            }
        }
    };
}
