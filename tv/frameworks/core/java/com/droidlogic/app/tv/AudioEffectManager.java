/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.app.tv;

import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.ComponentName;
import android.os.IBinder;
import android.os.RemoteCallbackList;
import android.os.RemoteException;
import android.util.Log;

import com.droidlogic.audioservice.services.IAudioEffectsService;

public class AudioEffectManager {
    private String TAG = "AudioEffectManager";
    public static final String SERVICE_PACKEGE_NANME = "com.droidlogic";
    public static final String SERVICE_NANME = "com.droidlogic.audioservice.services.AudioEffectsService";
    private IAudioEffectsService mAudioEffectsService = null;
    private Context mContext;

    private boolean mDebug = true;
    private int RETRY_MAX = 10;

    /* [setSoundMode] EQ sound mode type */
    public static final int EQ_SOUND_MODE_STANDARD                      = 0;
    public static final int EQ_SOUND_MODE_MUSIC                         = 1;
    public static final int EQ_SOUND_MODE_NEWS                          = 2;
    public static final int EQ_SOUND_MODE_THEATER                       = 3;
    public static final int EQ_SOUND_MODE_GAME                          = 4;
    public static final int EQ_SOUND_MODE_CUSTOM                        = 5;

    /* [setUserSoundModeParam] custom sound mode EQ band type */
    public static final int EQ_SOUND_MODE_EFFECT_BAND1                  = 0;
    public static final int EQ_SOUND_MODE_EFFECT_BAND2                  = 1;
    public static final int EQ_SOUND_MODE_EFFECT_BAND3                  = 2;
    public static final int EQ_SOUND_MODE_EFFECT_BAND4                  = 3;
    public static final int EQ_SOUND_MODE_EFFECT_BAND5                  = 4;

    /* [setDialogClarityMode] Modes of dialog clarity */
    public static final int DIALOG_CLARITY_MODE_OFF                     = 0;
    public static final int DIALOG_CLARITY_MODE_LOW                     = 1;
    public static final int DIALOG_CLARITY_MODE_HIGH                    = 2;

    /* [setDbxAdvancedModeParam] DBX sound mode param type */
    public static final int DBX_ADVANCED_MODE_PRARM_TYPE_SONICS         = 0;
    public static final int DBX_ADVANCED_MODE_PRARM_TYPE_VOLUME         = 1;
    public static final int DBX_ADVANCED_MODE_PRARM_TYPE_SURROUND       = 2;

    /* [setDbxSoundMode] DBX sound mode */
    public static final int DBX_SOUND_MODE_STANDARD                     = 0;
    public static final int DBX_SOUND_MODE_MUSIC                        = 1;
    public static final int DBX_SOUND_MODE_MOVIE                        = 2;
    public static final int DBX_SOUND_MODE_THEATER                      = 3;
    public static final int DBX_SOUND_MODE_ADVANCED                     = 4;

    /* [setDtsVirtualXMode] VirtualX effect mode */
    public static final int SOUND_EFFECT_VIRTUALX_MODE_OFF              = 0;
    public static final int SOUND_EFFECT_VIRTUALX_MODE_BASS             = 1;
    public static final int SOUND_EFFECT_VIRTUALX_MODE_FULL             = 2;

    /* init value for first boot */
    public static final int EFFECT_BASS_DEFAULT                         = 50;   // 0 - 100
    public static final int EFFECT_TREBLE_DEFAULT                       = 50;   // 0 - 100
    public static final int EFFECT_BALANCE_DEFAULT                      = 50;   // 0 - 100

    public static final int SOUND_EFFECT_SURROUND_ENABLE_DEFAULT        = 0;        // OFF
    public static final int SOUND_EFFECT_DIALOG_CLARITY_ENABLE_DEFAULT  = 0;        // OFF
    public static final int SOUND_EFFECT_TRUBASS_ENABLE_DEFAULT         = 0;        // OFF
    public static final int SOUND_EFFECT_VIRTUALX_MODE_DEFAULT          = SOUND_EFFECT_VIRTUALX_MODE_OFF;
    public static final int SOUND_EFFECT_TRUVOLUME_HD_ENABLE_DEFAULT    = 0;        // OFF
    public static final int SOUND_EFFECT_DBX_ENABLE_DEFAULT             = 0;        // OFF
    public static final int SOUND_EFFECT_DBX_SOUND_MODE_DEFAULT         = DBX_SOUND_MODE_STANDARD;

    // DBX sound mode default param [sonics, volume, surround]
    public static final byte[][] SOUND_EFFECT_DBX_SOUND_MODE_ARRAY_DEFAULT = {
            {4, 2, 2},  // standard mode
            {0, 2, 2},  // music mode
            {0, 2, 0},  // movie mode
            {0, 1, 2},  // theater mode
            {4, 2, 2},  // advance mode default db value
    };

    /****************************DAP effect cmd*******************************/
    public static final int SOUND_EFFECT_DAP_VERSION_1_3_2  = 0;
    public static final int SOUND_EFFECT_DAP_VERSION_2_4    = 1;
    public static final int SOUND_EFFECT_DAP_VERSION        = SOUND_EFFECT_DAP_VERSION_2_4;

    /* [DAP 1.3.2] */
    public static final int CMD_DAP_1_3_2_BASE_VALUE        = 0;
    public static final int CMD_DAP_ENABLE                  = 0;
    public static final int CMD_DAP_EFFECT_MODE             = 1;
    public static final int CMD_DAP_GEQ_GAINS               = 2;
    public static final int CMD_DAP_GEQ_ENABLE              = 3;
    public static final int CMD_DAP_POST_GAIN               = 4;
    public static final int CMD_DAP_VL_ENABLE               = 5;
    public static final int CMD_DAP_VL_AMOUNT               = 6;
    public static final int CMD_DAP_DE_ENABLE               = 7;
    public static final int CMD_DAP_DE_AMOUNT               = 8;
    public static final int CMD_DAP_SURROUND_ENABLE         = 9;
    public static final int CMD_DAP_SURROUND_BOOST          = 10;
    public static final int CMD_DAP_VIRTUALIZER_ENABLE      = 11;

    public static final int SUBCMD_DAP_GEQ_BAND1            = 0x100;
    public static final int SUBCMD_DAP_GEQ_BAND2            = 0x101;
    public static final int SUBCMD_DAP_GEQ_BAND3            = 0x102;
    public static final int SUBCMD_DAP_GEQ_BAND4            = 0x103;
    public static final int SUBCMD_DAP_GEQ_BAND5            = 0x104;

    /* [DAP 2.4] */
    public static final int CMD_DAP_2_4_BASE_VALUE                      = 1000;

    public static final int CMD_DAP_2_4_PROFILE                         = 1000;
    public static final int CMD_DAP_2_4_SURROUND_VIRTUALIZER            = 1004;
    public static final int CMD_DAP_2_4_DIALOGUE_ENHANCER               = 1007;
    public static final int CMD_DAP_2_4_BASS_ENHANCER                   = 1008;
    public static final int CMD_DAP_2_4_MI_STEERING                     = 1012;
    public static final int CMD_DAP_2_4_SURROUND_DECODER_ENABLE         = 1013;
    public static final int CMD_DAP_2_4_LEVELER                         = 1015;

    public static final int SUBCMD_DAP_2_4_BASE_VALUE                   = 2000;

    public static final int SUBCMD_DAP_2_4_SURROUND_VIRTUALIZER_BOOST   = 2000;
    public static final int SUBCMD_DAP_2_4_DIALOGUE_ENHANCER_AMOUNT     = 2001;
    public static final int SUBCMD_DAP_2_4_BASS_ENHANCER_BOOST          = 2002;
    public static final int SUBCMD_DAP_2_4_BASS_ENHANCER_CUTOFFX100     = 2003;
    public static final int SUBCMD_DAP_2_4_BASS_ENHANCER_CUTOFFX1       = 2004;
    public static final int SUBCMD_DAP_2_4_BASS_ENHANCER_WIDTH          = 2005;
    public static final int SUBCMD_DAP_2_4_LEVELER_AMOUNT               = 2006;

    /*************************************************************************/

    /* [CMD_DAP_2_4_PROFILE] sound mode */
    public static final int SOUND_EFFECT_DAP_2_4_PROFILE_MOVIE           = 0;
    public static final int SOUND_EFFECT_DAP_2_4_PROFILE_MUSIC           = 1;
    public static final int SOUND_EFFECT_DAP_2_4_PROFILE_GAME            = 2;
    public static final int SOUND_EFFECT_DAP_2_4_PROFILE_NIGHT           = 3;
    public static final int SOUND_EFFECT_DAP_2_4_PROFILE_VOICE           = 4;
    public static final int SOUND_EFFECT_DAP_2_4_PROFILE_USER_SELECTABLE = 5;
    public static final int SOUND_EFFECT_DAP_2_4_PROFILE_OFF             = 6;
    public static final int SOUND_EFFECT_DAP_2_4_PROFILE_MIN             = SOUND_EFFECT_DAP_2_4_PROFILE_MOVIE;
    public static final int SOUND_EFFECT_DAP_2_4_PROFILE_MAX             = SOUND_EFFECT_DAP_2_4_PROFILE_OFF;

    public static final int SOUND_EFFECT_DAP_2_4_SURROUND_VIRTUALIZER_OFF      = 0;
    public static final int SOUND_EFFECT_DAP_2_4_SURROUND_VIRTUALIZER_ON       = 1;
    public static final int SOUND_EFFECT_DAP_2_4_SURROUND_VIRTUALIZER_AUTO     = 2;

    public static final int SOUND_EFFECT_DAP_2_4_LEVELER_OFF                   = 0;
    public static final int SOUND_EFFECT_DAP_2_4_LEVELER_ON                    = 1;
    public static final int SOUND_EFFECT_DAP_2_4_LEVELER_AUTO                  = 2;


    /* DAP 2.4 default setting */
    public static final int DAP_2_4_SURROUND_VIRTUALIZER_DEFAULT               = 1;
    public static final int DAP_2_4_SURROUND_VIRTUALIZER_BOOST_DEFAULT         = 96;
    public static final int DAP_2_4_DIALOGUE_ENHANCER_DEFAULT                  = 0;
    public static final int DAP_2_4_DIALOGUE_ENHANCER_AMOUNT_DEFAULT           = 0;
    public static final int DAP_2_4_BASS_ENHANCER_DEFAULT                      = 0;
    public static final int DAP_2_4_BASS_ENHANCER_BOOST_DEFAULT                = 192;
    public static final int DAP_2_4_BASS_ENHANCER_CUTOFFX100_DEFAULT           = 2;//X100
    public static final int DAP_2_4_BASS_ENHANCER_CUTOFFX1_DEFAULT             = 0;//X1
    public static final int DAP_2_4_BASS_ENHANCER_WIDTH_DEFAULT                = 16;
    public static final int DAP_2_4_MI_STEERING_DEFAULT                        = 0;
    public static final int DAP_2_4_SURROUND_DECODER_ENABLE_DEFAULT            = 1;
    public static final int DAP_2_4_LEVELER_DEFAULT                            = 0;
    public static final int DAP_2_4_LEVELER_AMOUNT_DEFAULT                     = 4;

    /* DAP 1.3.2 sound mode */
    public static final int DAP_MODE_OFF                    = 0;
    public static final int DAP_MODE_MOVIE                  = 1;
    public static final int DAP_MODE_MUSIC                  = 2;
    public static final int DAP_MODE_NIGHT                  = 3;
    public static final int DAP_MODE_USER                   = 4;
    public static final int DAP_MODE_DEFAULT                = DAP_MODE_MUSIC;

    public static final int DAP_SURROUND_SPEAKER            = 0;
    public static final int DAP_SURROUND_HEADPHONE          = 1;
    public static final int DAP_SURROUND_DEFAULT            = DAP_SURROUND_SPEAKER;

    public static final int DAP_GEQ_OFF                     = 0;
    public static final int DAP_GEQ_INIT                    = 1;
    public static final int DAP_GEQ_OPEN                    = 2;
    public static final int DAP_GEQ_RICH                    = 3;
    public static final int DAP_GEQ_FOCUSED                 = 4;
    public static final int DAP_GEQ_USER                    = 5;
    public static final int DAP_GEQ_DEFAULT                 = DAP_GEQ_INIT;

    public static final int DAP_OFF                         = 0;
    public static final int DAP_ON                          = 1;

    public static final int DAP_VL_DEFAULT                  = DAP_ON;
    public static final int DAP_VL_AMOUNT_DEFAULT           = 0;
    public static final int DAP_DE_DEFAULT                  = DAP_OFF;
    public static final int DAP_DE_AMOUNT_DEFAULT           = 0;
    public static final int DAP_SURROUND_BOOST_DEFAULT      = 0;
    public static final int DAP_POST_GAIN_DEFAULT           = 0;
    public static final int DAP_GEQ_GAIN_DEFAULT            = 0;
    private static AudioEffectManager mInstance;

    public static AudioEffectManager getInstance(Context context) {
        if (null == mInstance) {
            mInstance = new AudioEffectManager(context);
        }
        return mInstance;
    }

    public AudioEffectManager(Context context) {
        mContext = context;
        LOGI("construction AudioEffectManager");
        getService();
    }

    private void LOGI(String msg) {
        if (mDebug) Log.i(TAG, msg);
    }

    private void getService() {
        LOGI("=====[getService]");
        int retry = RETRY_MAX;
        boolean mIsBind = false;
        try {
            synchronized (this) {
                while (true) {
                    Intent intent = new Intent();
                    intent.setAction(SERVICE_NANME);
                    intent.setPackage(SERVICE_PACKEGE_NANME);
                    mIsBind = mContext.bindService(intent, serConn, mContext.BIND_AUTO_CREATE);
                    LOGI("=====[getService] mIsBind: " + mIsBind + ", retry:" + retry);
                    if (mIsBind || retry <= 0) {
                        break;
                    }
                    retry --;
                    Thread.sleep(500);
                }
            }
        } catch (InterruptedException e){}
    }

    private ServiceConnection serConn = new ServiceConnection() {
        @Override
        public void onServiceDisconnected(ComponentName name) {
            LOGI("[onServiceDisconnected] mAudioEffectsService: " + mAudioEffectsService);
            mAudioEffectsService = null;

        }
        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            mAudioEffectsService = IAudioEffectsService.Stub.asInterface(service);
            LOGI("[onServiceConnected] mAudioEffectsService: " + mAudioEffectsService);
        }
    };

    public void unBindService() {
        mContext.unbindService(serConn);
    }

    private boolean audioEffectServiceIsNull() {
        if (mAudioEffectsService == null) {
            Log.w(TAG, "mAudioEffectsService is null, pls check stack:");
            Log.w(TAG, Log.getStackTraceString(new Throwable()));
            return true;
        } else {
            return false;
        }
    }

    public void createAudioEffects() {
        if (audioEffectServiceIsNull()) return;
        try {
            mAudioEffectsService.createAudioEffects();
        } catch (RemoteException e) {
            Log.e(TAG, "createAudioEffects failed:" + e);
        }
    }

    public boolean isSupportVirtualX() {
        if (audioEffectServiceIsNull()) return false;
        try {
            return mAudioEffectsService.isSupportVirtualX();
        } catch (RemoteException e) {
            Log.e(TAG, "isSupportVirtualX failed:" + e);
        }
        return false;
    }

    public void setDtsVirtualXMode(int virtalXMode) {
        if (audioEffectServiceIsNull()) return;
        try {
            mAudioEffectsService.setDtsVirtualXMode(virtalXMode);
        } catch (RemoteException e) {
            Log.e(TAG, "setDtsVirtualXMode failed:" + e);
        }
    }

    public int getDtsVirtualXMode() {
        if (audioEffectServiceIsNull()) return 0;
        try {
            return mAudioEffectsService.getDtsVirtualXMode();
        } catch (RemoteException e) {
            Log.e(TAG, "getDtsVirtualXMode failed:" + e);
        }
        return -1;
    }

    public void setDtsTruVolumeHdEnable(boolean enable) {
        if (audioEffectServiceIsNull()) return;
        try {
            mAudioEffectsService.setDtsTruVolumeHdEnable(enable);
        } catch (RemoteException e) {
            Log.e(TAG, "setDtsTruVolumeHdEnable failed:" + e);
        }
    }

    public boolean getDtsTruVolumeHdEnable() {
        if (audioEffectServiceIsNull()) return false;
        try {
            return mAudioEffectsService.getDtsTruVolumeHdEnable();
        } catch (RemoteException e) {
            Log.e(TAG, "getDtsTruVolumeHdEnable failed:" + e);
        }
        return false;
    }

    public int getSoundModeStatus() {
        if (audioEffectServiceIsNull()) return 0;
        try {
            return mAudioEffectsService.getSoundModeStatus();
        } catch (RemoteException e) {
            Log.e(TAG, "getSoundModeStatus failed:" + e);
        }
        return -1;
    }

    public int getSoundModule() {
        if (audioEffectServiceIsNull()) return 0;
        try {
            return mAudioEffectsService.getSoundModule();
        } catch (RemoteException e) {
            Log.e(TAG, "getSoundModule failed:" + e);
        }
        return -1;
    }

    public int getTrebleStatus() {
        if (audioEffectServiceIsNull()) return 0;
        try {
            return mAudioEffectsService.getTrebleStatus();
        } catch (RemoteException e) {
            Log.e(TAG, "getTrebleStatus failed:" + e);
        }
        return -1;
    }

    public int getBassStatus() {
        if (audioEffectServiceIsNull()) return 0;
        try {
            return mAudioEffectsService.getBassStatus();
        } catch (RemoteException e) {
            Log.e(TAG, "getBassStatus failed:" + e);
        }
        return -1;
    }
    public int getBalanceStatus() {
        if (audioEffectServiceIsNull()) return 0;
        try {
            return mAudioEffectsService.getBalanceStatus();
        } catch (RemoteException e) {
            Log.e(TAG, "getBalanceStatus failed:" + e);
        }
        return -1;
    }

    public boolean getAgcEnableStatus() {
        if (audioEffectServiceIsNull()) return false;
        try {
            return mAudioEffectsService.getAgcEnableStatus();
        } catch (RemoteException e) {
            Log.e(TAG, "getAgcEnableStatus failed:" + e);
        }
        return false;
    }

    public int getAgcMaxLevelStatus() {
        if (audioEffectServiceIsNull()) return 0;
        try {
            return mAudioEffectsService.getAgcMaxLevelStatus();
        } catch (RemoteException e) {
            Log.e(TAG, "getAgcMaxLevelStatus failed:" + e);
        }
        return -1;
    }

    public int getAgcAttackTimeStatus() {
        if (audioEffectServiceIsNull()) return 0;
        try {
            return mAudioEffectsService.getAgcAttackTimeStatus();
        } catch (RemoteException e) {
            Log.e(TAG, "getAgcAttackTimeStatus failed:" + e);
        }
        return -1;
    }

    public int getAgcReleaseTimeStatus() {
        if (audioEffectServiceIsNull()) return 0;
        try {
            return mAudioEffectsService.getAgcReleaseTimeStatus();
        } catch (RemoteException e) {
            Log.e(TAG, "getAgcReleaseTimeStatus failed:" + e);
        }
        return -1;
    }

    public int getAgcSourceIdStatus() {
        if (audioEffectServiceIsNull()) return 0;
        try {
            return mAudioEffectsService.getAgcSourceIdStatus();
        } catch (RemoteException e) {
            Log.e(TAG, "getAgcSourceIdStatus failed:" + e);
        }
        return -1;
    }

    public int getVirtualSurroundStatus() {
        if (audioEffectServiceIsNull()) return 0;
        try {
            return mAudioEffectsService.getVirtualSurroundStatus();
        } catch (RemoteException e) {
            Log.e(TAG, "getVirtualSurroundStatus failed:" + e);
        }
        return -1;
    }

    public void setSoundMode(int mode) {
        if (audioEffectServiceIsNull()) return;
        try {
            mAudioEffectsService.setSoundMode(mode);
        } catch (RemoteException e) {
            Log.e(TAG, "setSoundMode failed:" + e);
        }
    }

    public void setSoundModeByObserver(int mode) {
        if (audioEffectServiceIsNull()) return;
        try {
            mAudioEffectsService.setSoundModeByObserver(mode);
        } catch (RemoteException e) {
            Log.e(TAG, "setSoundModeByObserver failed:" + e);
        }
    }

    public void setUserSoundModeParam(int bandNumber, int value) {
        if (audioEffectServiceIsNull()) return;
        try {
            mAudioEffectsService.setUserSoundModeParam(bandNumber, value);
        } catch (RemoteException e) {
            Log.e(TAG, "setUserSoundModeParam failed:" + e);
        }
    }

    public int getUserSoundModeParam(int bandNumber) {
        if (audioEffectServiceIsNull()) return 0;
        try {
            return mAudioEffectsService.getUserSoundModeParam(bandNumber);
        } catch (RemoteException e) {
            Log.e(TAG, "getUserSoundModeParam failed:" + e);
        }
        return -1;
    }

    public void setTreble(int step) {
        if (audioEffectServiceIsNull()) return;
        try {
            mAudioEffectsService.setTreble(step);
        } catch (RemoteException e) {
            Log.e(TAG, "setTreble failed:" + e);
        }
    }

    public void setBass(int step) {
        if (audioEffectServiceIsNull()) return;
        try {
            mAudioEffectsService.setBass(step);
        } catch (RemoteException e) {
            Log.e(TAG, "setBass failed:" + e);
        }
    }

    public void setBalance(int step) {
        if (audioEffectServiceIsNull()) return;
        try {
            mAudioEffectsService.setBalance(step);
        } catch (RemoteException e) {
            Log.e(TAG, "setBalance failed:" + e);
        }
    }

    public void setSurroundEnable(boolean enable) {
        if (audioEffectServiceIsNull()) return;
        try {
            mAudioEffectsService.setSurroundEnable(enable);
        } catch (RemoteException e) {
            Log.e(TAG, "setSurroundEnable failed:" + e);
        }
    }

    public boolean getSurroundEnable() {
        if (audioEffectServiceIsNull()) return false;
        try {
            return mAudioEffectsService.getSurroundEnable();
        } catch (RemoteException e) {
            Log.e(TAG, "getSurroundEnable failed:" + e);
        }
        return false;
    }

    public void setDialogClarityMode(int mode) {
        if (audioEffectServiceIsNull()) return;
        try {
            mAudioEffectsService.setDialogClarityMode(mode);
        } catch (RemoteException e) {
            Log.e(TAG, "setDialogClarityEnable failed:" + e);
        }
    }

    public int getDialogClarityMode() {
        if (audioEffectServiceIsNull()) return 0;
        try {
            return mAudioEffectsService.getDialogClarityMode();
        } catch (RemoteException e) {
            Log.e(TAG, "getDialogClarityEnable failed:" + e);
        }
        return -1;
    }

    public void setTruBassEnable(boolean enable) {
        if (audioEffectServiceIsNull()) return;
        try {
            mAudioEffectsService.setTruBassEnable(enable);
        } catch (RemoteException e) {
            Log.e(TAG, "setTruBassEnable failed:" + e);
        }
    }

    public boolean getTruBassEnable() {
        if (audioEffectServiceIsNull()) return false;
        try {
            return mAudioEffectsService.getTruBassEnable();
        } catch (RemoteException e) {
            Log.e(TAG, "getTruBassEnable failed:" + e);
        }
        return false;
    }

    public void setAgcEnable(boolean enable) {
        if (audioEffectServiceIsNull()) return;
        try {
            mAudioEffectsService.setAgcEnable(enable);
        } catch (RemoteException e) {
            Log.e(TAG, "setAgcEnable failed:" + e);
        }
    }

    public void setAgcMaxLevel(int step) {
        if (audioEffectServiceIsNull()) return;
        try {
            mAudioEffectsService.setAgcMaxLevel(step);
        } catch (RemoteException e) {
            Log.e(TAG, "setAgcMaxLevel failed:" + e);
        }
    }

    public void setAgcAttackTime(int step) {
        if (audioEffectServiceIsNull()) return;
        try {
            mAudioEffectsService.setAgcAttackTime(step);
        } catch (RemoteException e) {
            Log.e(TAG, "setAgcAttackTime failed:" + e);
        }
    }

    public void setAgcReleaseTime(int step) {
        if (audioEffectServiceIsNull()) return;
        try {
            mAudioEffectsService.setAgcReleaseTime(step);
        } catch (RemoteException e) {
            Log.e(TAG, "setAgcReleaseTime failed:" + e);
        }
    }

    public void setSourceIdForAvl(int step) {
        if (audioEffectServiceIsNull()) return;
        try {
            mAudioEffectsService.setSourceIdForAvl(step);
        } catch (RemoteException e) {
            Log.e(TAG, "setSourceIdForAvl failed:" + e);
        }
    }

    public void setVirtualSurround(int mode) {
        if (audioEffectServiceIsNull()) return;
        try {
            mAudioEffectsService.setVirtualSurround(mode);
        } catch (RemoteException e) {
            Log.e(TAG, "setVirtualSurround failed:" + e);
        }
    }

    public void setDbxEnable(boolean enable) {
        if (audioEffectServiceIsNull()) return;
        try {
            mAudioEffectsService.setDbxEnable(enable);
        } catch (RemoteException e) {
            Log.e(TAG, "setDbxEnable failed:" + e);
        }
    }

    public boolean getDbxEnable() {
        if (audioEffectServiceIsNull()) return false;
        try {
            return mAudioEffectsService.getDbxEnable();
        } catch (RemoteException e) {
            Log.e(TAG, "getDbxEnable failed:" + e);
        }
        return false;
    }

    public void setDbxSoundMode(int dbxMode) {
        if (audioEffectServiceIsNull()) return;
        try {
            mAudioEffectsService.setDbxSoundMode(dbxMode);
        } catch (RemoteException e) {
            Log.e(TAG, "setDbxSoundMode failed:" + e);
        }
    }

    public int getDbxSoundMode() {
        if (audioEffectServiceIsNull()) return 0;
        try {
            return mAudioEffectsService.getDbxSoundMode();
        } catch (RemoteException e) {
            Log.e(TAG, "getDbxSoundMode failed:" + e);
        }
        return -1;
    }

    public void setDbxAdvancedModeParam(int paramType, int value) {
        if (audioEffectServiceIsNull()) return;
        try {
            mAudioEffectsService.setDbxAdvancedModeParam(paramType, value);
        } catch (RemoteException e) {
            Log.e(TAG, "setDbxAdvancedModeParam failed:" + e);
        }
    }

    public int getDbxAdvancedModeParam(int paramType) {
        if (audioEffectServiceIsNull()) return 0;
        try {
            return mAudioEffectsService.getDbxAdvancedModeParam(paramType);
        } catch (RemoteException e) {
            Log.e(TAG, "getDbxAdvancedModeParam failed:" + e);
        }
        return -1;
    }

    public void cleanupAudioEffects() {
        if (audioEffectServiceIsNull()) return;
        try {
            mAudioEffectsService.cleanupAudioEffects();
        } catch (RemoteException e) {
            Log.e(TAG, "cleanupAudioEffects failed:" + e);
        }
    }

    public void initSoundEffectSettings() {
        if (audioEffectServiceIsNull()) return;
        try {
            mAudioEffectsService.initSoundEffectSettings();
        } catch (RemoteException e) {
            Log.e(TAG, "initSoundEffectSettings failed:" + e);
        }
    }

    public void resetSoundEffectSettings() {
        if (audioEffectServiceIsNull()) return;
        try {
            mAudioEffectsService.resetSoundEffectSettings();
        } catch (RemoteException e) {
            Log.e(TAG, "resetSoundEffectSettings failed:" + e);
        }
    }

    public void setDapParam(int id, int value) {
        if (audioEffectServiceIsNull()) return;
        try {
            mAudioEffectsService.setDapParam(id, value);
        } catch (RemoteException e) {
            Log.e(TAG, "setDapParam failed:" + e);
        }
    }

    public int getDapParam(int id) {
        if (audioEffectServiceIsNull()) return 0;
        try {
            return mAudioEffectsService.getDapParam(id);
        } catch (RemoteException e) {
            Log.e(TAG, "getDapParam failed:" + e);
        }
        return 0;
    }

    public void initDapAudioEffect() {
        if (audioEffectServiceIsNull()) return;
        try {
            mAudioEffectsService.initDapAudioEffect();
        } catch (RemoteException e) {
            Log.e(TAG, "initDapAudioEffect failed:" + e);
        }
    }
}
