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

package com.droidlogic.audioservice.settings;

import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.text.TextUtils;
import android.util.Log;
import android.os.SystemClock;
import android.media.AudioManager;
import android.app.ActivityManager;
import android.content.pm.PackageManager;
import android.provider.Settings;
import android.widget.Toast;
import android.app.AlarmManager;
import android.app.PendingIntent;
import android.media.audiofx.AudioEffect;
import android.content.SharedPreferences;
import android.media.AudioFormat;
import android.media.AudioTrack;

import java.io.File;
import java.lang.reflect.Constructor;
import java.lang.reflect.Method;
import java.util.*;
import java.math.BigDecimal;
import java.text.DecimalFormat;
import com.droidlogic.app.DroidLogicUtils;
import com.droidlogic.app.OutputModeManager;
import com.droidlogic.app.tv.AudioEffectManager;

public class SoundEffectManager {

    public static final String TAG = "SoundEffectManager";

    private static final UUID EFFECT_TYPE_TRUSURROUND           = UUID.fromString("1424f5a0-2457-11e6-9fe0-0002a5d5c51b");
    private static final UUID EFFECT_TYPE_BALANCE               = UUID.fromString("7cb34dc0-242e-11e6-bb63-0002a5d5c51b");
    private static final UUID EFFECT_TYPE_TREBLE_BASS           = UUID.fromString("7e282240-242e-11e6-bb63-0002a5d5c51b");
    private static final UUID EFFECT_TYPE_DAP_1_3_2             = UUID.fromString("3337b21d-c8e6-4bbd-8f24-698ade8491b9");
    private static final UUID EFFECT_TYPE_DAP_2_4               = UUID.fromString("34033483-c5e9-4ff6-8b6b-0002a5d5c51b");

    private static final UUID EFFECT_TYPE_EQ                    = UUID.fromString("ce2c14af-84df-4c36-acf5-87e428ed05fc");
    private static final UUID EFFECT_TYPE_AGC                   = UUID.fromString("4a959f5c-e33a-4df2-8c3f-3066f9275edf");
    private static final UUID EFFECT_TYPE_VIRTUAL_SURROUND      = UUID.fromString("c656ec6f-d6be-4e7f-854b-1218077f3915");
    private static final UUID EFFECT_TYPE_VIRTUAL_X             = UUID.fromString("5112a99e-b8b9-4c5e-91fd-a804d29c36b2");
    private static final UUID EFFECT_TYPE_DBX                   = UUID.fromString("a41cedc0-578e-11e5-9cb0-0002a5d5c51b");

    private static final UUID EFFECT_UUID_VIRTUAL_X             = UUID.fromString("61821587-ce3c-4aac-9122-86d874ea1fb1");
    private static final UUID EFFECT_UUID_DBX                   = UUID.fromString("07210842-7432-4624-8b97-35ac8782efa3");

    // defined index ID for DB storage
    public static final String DB_ID_SOUND_EFFECT_BASS                          = "db_id_sound_effect_bass";
    public static final String DB_ID_SOUND_EFFECT_TREBLE                        = "db_id_sound_effect_treble";
    public static final String DB_ID_SOUND_EFFECT_BALANCE                       = "db_id_sound_effect_balance";
    public static final String DB_ID_SOUND_EFFECT_DIALOG_CLARITY                = "db_id_sound_effect_dialog_clarity";
    public static final String DB_ID_SOUND_EFFECT_SURROUND                      = "db_id_sound_effect_surround";
    public static final String DB_ID_SOUND_EFFECT_TRUBASS                       = "db_id_sound_effect_tru_bass";
    public static final String DB_ID_SOUND_EFFECT_SOUND_MODE                    = "db_id_sound_effect_sound_mode";
    public static final String DB_ID_SOUND_EFFECT_SOUND_MODE_TYPE               = "db_id_sound_effect_sound_mode_type";
    public static final String DB_ID_SOUND_EFFECT_SOUND_MODE_TYPE_DAP           = "db_id_sound_effect_sound_mode_type_dap";
    public static final String DB_ID_SOUND_EFFECT_SOUND_MODE_TYPE_EQ            = "db_id_sound_effect_sound_mode_type_eq";
    public static final String DB_ID_SOUND_EFFECT_SOUND_MODE_DAP_VALUE          = "db_id_sound_effect_sound_mode_dap";
    public static final String DB_ID_SOUND_EFFECT_SOUND_MODE_EQ_VALUE           = "db_id_sound_effect_sound_mode_eq";
    public static final String DB_ID_SOUND_EFFECT_BAND1                         = "db_id_sound_effect_band1";
    public static final String DB_ID_SOUND_EFFECT_BAND2                         = "db_id_sound_effect_band2";
    public static final String DB_ID_SOUND_EFFECT_BAND3                         = "db_id_sound_effect_band3";
    public static final String DB_ID_SOUND_EFFECT_BAND4                         = "db_id_sound_effect_band4";
    public static final String DB_ID_SOUND_EFFECT_BAND5                         = "db_id_sound_effect_band5";
    public static final String DB_ID_SOUND_EFFECT_AGC_ENABLE                    = "db_id_sound_effect_agc_on";
    public static final String DB_ID_SOUND_EFFECT_AGC_MAX_LEVEL                 = "db_id_sound_effect_agc_level";
    public static final String DB_ID_SOUND_EFFECT_AGC_ATTACK_TIME               = "db_id_sound_effect_agc_attack";
    public static final String DB_ID_SOUND_EFFECT_AGC_RELEASE_TIME              = "db_id_sound_effect_agc_release";
    public static final String DB_ID_SOUND_EFFECT_AGC_SOURCE_ID                 = "db_id_sound_avl_source_id";
    public static final String DB_ID_SOUND_EFFECT_VIRTUALX_MODE                 = "db_id_sound_effect_virtualx_mode";
    public static final String DB_ID_SOUND_EFFECT_TREVOLUME_HD                  = "db_id_sound_effect_truvolume_hd";
    public static final String DB_ID_SOUND_EFFECT_DBX_ENABLE                    = "db_id_sound_effect_dbx_enable";
    public static final String DB_ID_SOUND_EFFECT_DBX_SOUND_MODE                = "db_id_sound_effect_dbx_sound_mode";
    public static final String DB_ID_SOUND_EFFECT_DBX_ADVANCED_MODE_SONICS      = "db_id_sound_effect_dbx_advanced_mode_sonics";
    public static final String DB_ID_SOUND_EFFECT_DBX_ADVANCED_MODE_VOLUME      = "db_id_sound_effect_dbx_advanced_mode_volume";
    public static final String DB_ID_SOUND_EFFECT_DBX_ADVANCED_MODE_SURROUND    = "db_id_sound_effect_dbx_advanced_mode_surround";

    /* [DAP 1.3.2] */
    public static final String DB_ID_SOUND_EFFECT_DAP_SAVED                     = "db_id_sound_effect_dap_saved";
    public static final String DB_ID_SOUND_EFFECT_DAP_MODE                      = "db_id_sound_effect_dap_mode";
    public static final String DB_ID_SOUND_EFFECT_DAP_GEQ_ENABLE                = "db_id_sound_effect_dap_geq";
    public static final String DB_ID_SOUND_EFFECT_DAP_POST_GAIN                 = "db_id_sound_effect_dap_post_gain";
    public static final String DB_ID_SOUND_EFFECT_DAP_VL_ENABLE                 = "db_id_sound_effect_dap_vl";
    public static final String DB_ID_SOUND_EFFECT_DAP_VL_AMOUNT                 = "db_id_sound_effect_dap_vl_amount";
    public static final String DB_ID_SOUND_EFFECT_DAP_DE_ENABLE                 = "db_id_sound_effect_dap_de";
    public static final String DB_ID_SOUND_EFFECT_DAP_DE_AMOUNT                 = "db_id_sound_effect_dap_de_amount";
    public static final String DB_ID_SOUND_EFFECT_DAP_SURROUND_ENABLE           = "db_id_sound_effect_dap_surround";
    public static final String DB_ID_SOUND_EFFECT_DAP_SURROUND_BOOST            = "db_id_sound_effect_dap_surround_boost";
    public static final String DB_ID_SOUND_EFFECT_DAP_GEQ_BAND1                 = "db_id_sound_effect_dap_geq_band1";
    public static final String DB_ID_SOUND_EFFECT_DAP_GEQ_BAND2                 = "db_id_sound_effect_dap_geq_band2";
    public static final String DB_ID_SOUND_EFFECT_DAP_GEQ_BAND3                 = "db_id_sound_effect_dap_geq_band3";
    public static final String DB_ID_SOUND_EFFECT_DAP_GEQ_BAND4                 = "db_id_sound_effect_dap_geq_band4";
    public static final String DB_ID_SOUND_EFFECT_DAP_GEQ_BAND5                 = "db_id_sound_effect_dap_geq_band5";

    /* [DAP 2.4] */
    public static final String DB_ID_SOUND_EFFECT_DAP_2_4_PROFILE               = "db_id_sound_effect_dap_2_4_profile";

    private static final String DB_ID_SOUND_EFFECT_DAP_2_4_SURROUND_VIRTUALIZER_MODE     = "db_id_sound_effect_dap_2_4_surround_virtualizer_mode";
    private static final String DB_ID_SOUND_EFFECT_DAP_2_4_SURROUND_VIRTUALIZER_BOOST    = "db_id_sound_effect_dap_2_4_surround_virtualizer_boost";
    private static final String DB_ID_SOUND_EFFECT_DAP_2_4_DIALOGUE_ENHANCER_ENABLE      = "db_id_sound_effect_dap_2_4_dialogue_enhancer_enable";
    private static final String DB_ID_SOUND_EFFECT_DAP_2_4_DIALOGUE_ENHANCER_AMOUNT      = "db_id_sound_effect_dap_2_4_dialogue_enhancer_amount";
    private static final String DB_ID_SOUND_EFFECT_DAP_2_4_BASS_ENHANCER_ENABLE          = "db_id_sound_effect_dap_2_4_bass_enhancer_enable";
    private static final String DB_ID_SOUND_EFFECT_DAP_2_4_BASS_ENHANCER_BOOST           = "db_id_sound_effect_dap_2_4_bass_enhancer_boost";
    private static final String DB_ID_SOUND_EFFECT_DAP_2_4_BASS_ENHANCER_CUTOFFX100      = "db_id_sound_effect_dap_2_4_bass_enhancer_cutoffX100";
    private static final String DB_ID_SOUND_EFFECT_DAP_2_4_BASS_ENHANCER_CUTOFFX1        = "db_id_sound_effect_dap_2_4_bass_enhancer_cutoffX1";
    private static final String DB_ID_SOUND_EFFECT_DAP_2_4_BASS_ENHANCER_WIDTH           = "db_id_sound_effect_dap_2_4_bass_enhancer_width";

    private static final String DB_ID_SOUND_EFFECT_DAP_2_4_MI_STEERING                   = "db_id_sound_effect_dap_2_4_mi_steering";
    private static final String DB_ID_SOUND_EFFECT_DAP_2_4_SURROUND_DECODER_ENABLE       = "db_id_sound_effect_dap_2_4_surround_decoder_enable";
    private static final String DB_ID_SOUND_EFFECT_DAP_2_4_LEVELER_MODE                  = "db_id_sound_effect_dap_2_4_leveler_mode";
    private static final String DB_ID_SOUND_EFFECT_DAP_2_4_LEVELER_AMOUNT                = "db_id_sound_effect_dap_2_4_leveler_strength";

    //set id
    public static final int SET_BASS                                    = 0;
    public static final int SET_TREBLE                                  = 1;
    public static final int SET_BALANCE                                 = 2;
    public static final int SET_DIALOG_CLARITY_MODE                     = 3;
    public static final int SET_SURROUND_ENABLE                         = 4;
    public static final int SET_TRUBASS_ENABLE                          = 5;
    public static final int SET_SOUND_MODE                              = 6;
    public static final int SET_EFFECT_BAND1                            = 7;
    public static final int SET_EFFECT_BAND2                            = 8;
    public static final int SET_EFFECT_BAND3                            = 9;
    public static final int SET_EFFECT_BAND4                            = 10;
    public static final int SET_EFFECT_BAND5                            = 11;
    public static final int SET_AGC_ENABLE                              = 12;
    public static final int SET_AGC_MAX_LEVEL                           = 13;
    public static final int SET_AGC_ATTACK_TIME                         = 14;
    public static final int SET_AGC_RELEASE_TIME                        = 15;
    public static final int SET_AGC_SOURCE_ID                           = 16;
    public static final int SET_VIRTUAL_SURROUND                        = 17;
    public static final int SET_VIRTUALX_MODE                           = 18;
    public static final int SET_TRUVOLUME_HD_ENABLE                     = 19;
    public static final int SET_DBX_ENABLE                              = 20;
    public static final int SET_DBX_SOUND_MODE                          = 21;
    public static final int SET_DBX_SOUND_MODE_ADVANCED_SONICS          = 22;
    public static final int SET_DBX_SOUND_MODE_ADVANCED_VOLUME          = 23;
    public static final int SET_DBX_SOUND_MODE_ADVANCED_SURROUND        = 24;

    //SoundMode mode.  Parameter ID
    public static final int PARAM_SRS_PARAM_DIALOGCLARTY_MODE           = 1;
    public static final int PARAM_SRS_PARAM_SURROUND_MODE               = 2;
    public static final int PARAM_SRS_PARAM_VOLUME_MODE                 = 3;
    public static final int PARAM_SRS_PARAM_TRUEBASS_ENABLE             = 5;

    //Balance level.  Parameter ID
    public static final int PARAM_BALANCE_LEVEL                         = 0;

    //Tone level.  Parameter ID for
    public static final int PARAM_BASS_LEVEL                            = 0;
    public static final int PARAM_TREBLE_LEVEL                          = 1;

    //dap AudioEffect, [ HPEQparams ] enumeration alignment in Hpeq.cpp
    public static final int PARAM_EQ_ENABLE                             = 0;
    public static final int PARAM_EQ_EFFECT                             = 1;
    public static final int PARAM_EQ_CUSTOM                             = 2;
    //agc effect define
    public static final int PARAM_AGC_ENABLE                            = 0;
    public static final int PARAM_AGC_MAX_LEVEL                         = 1;
    public static final int PARAM_AGC_ATTACK_TIME                       = 4;
    public static final int PARAM_AGC_RELEASE_TIME                      = 5;
    public static final int PARAM_AGC_SOURCE_ID                         = 6;

    public static final boolean DEFAULT_AGC_ENABLE                      = false;//enable 1, disable 0
    public static final int DEFAULT_AGC_MAX_LEVEL                       = -18;  //db
    public static final int DEFAULT_AGC_ATTACK_TIME                     = 10;   //ms
    public static final int DEFAULT_AGC_RELEASE_TIME                    = 2;    //s
    public static final int DEFAULT_AGC_SOURCE_ID                       = 3;
    //virtual surround
    public static final int PARAM_VIRTUALSURROUND                       = 0;

    //definition off and on
    private static final int PARAMETERS_SWITCH_OFF                      = 1;
    private static final int PARAMETERS_SWITCH_ON                       = 0;

    private static final int UI_SWITCH_OFF                              = 0;
    private static final int UI_SWITCH_ON                               = 1;

    private static final int PARAMETERS_DAP_ENABLE                      = 1;
    private static final int PARAMETERS_DAP_DISABLE                     = 0;
    //band 1, band 2, band 3, band 4, band 5  need transfer 0~100 to -10~10
    private static final int[] EFFECT_SOUND_MODE_USER_BAND              = {50, 50, 50, 50, 50};
    private static final int EFFECT_SOUND_TYPE_NUM = 6;

    // Virtual X effect param type
    private static final int PARAM_DTS_PARAM_MBHL_ENABLE_I32            = 0;
    private static final int PARAM_DTS_PARAM_TBHDX_ENABLE_I32           = 35;
    private static final int PARAM_DTS_PARAM_VX_ENABLE_I32              = 46;
    private static final int PARAM_DTS_PARAM_LOUDNESS_CONTROL_ENABLE_I32= 67;

    // DBX effect param type
    private static final int PARAM_DBX_PARAM_ENABLE                     = 0;
    private static final int PARAM_DBX_SET_MODE                         = 1;

    private static final int DAP_CPDP_OUTPUT_2_SPEAKER      = 7;
    private static final int DAP_CPDP_OUTPUT_2_HEADPHONE    = 6;

    // Prefix to append to audio preferences file
    private Context mContext;

    //sound effects
    private AudioEffect mVirtualX;
    private AudioEffect mTruSurround;
    private AudioEffect mBalance;
    private AudioEffect mTrebleBass;
    private AudioEffect mSoundMode;
    private AudioEffect mAgc;
    private AudioEffect mVirtualSurround;
    private AudioEffect mDbx;
    private AudioEffect mDap;

    private boolean mSupportVirtualX;
    private boolean mSupportMs12Dap = false;
    private boolean mEffectInit = false;

    private static SoundEffectManager mInstance;

    public static synchronized SoundEffectManager getInstance(Context context) {
        if (null == mInstance) {
            mInstance = new SoundEffectManager(context);
        }
        return mInstance;
    }
    private SoundEffectManager (Context context) {
        Log.d(TAG, "SoundEffectManager construction");
        mContext = context;
        mSupportMs12Dap = OutputModeManager.getInstance(mContext).isAudioSupportMs12System();
    }

    public void createAudioEffects() {
        if (mEffectInit) {
            Log.w(TAG, "createAudioEffects Already init effect, return");
            return;
        }
        Log.d(TAG, "createAudioEffects Start to create audio effects...");
        mSupportVirtualX = false;
        if (isDtsVXValid()) {
            mSupportVirtualX = creatVirtualXAudioEffects();
        }
        if (!mSupportVirtualX) {
            Log.i(TAG, "current not support Virtual X, begin to create TruSurround effect");
            creatTruSurroundAudioEffects();
        }
        creatVirtualSurroundAudioEffects();
        creatTrebleBassAudioEffects();
        if (!mSupportMs12Dap) {
            creatEqAudioEffects();
        }
        creatDbxAudioEffects();
        creatBalanceAudioEffects();
        mEffectInit = true;
    }

    public void cleanupAudioEffects() {
        if (mEffectInit) {
            Log.w(TAG, "cleanupAudioEffects effect not create, return.");
            return;
        }
        if (mBalance!= null) {
            mBalance.setEnabled(false);
            mBalance.release();
            mBalance = null;
        }
        if (mTruSurround!= null) {
            mTruSurround.setEnabled(false);
            mTruSurround.release();
            mTruSurround = null;
        }
        if (mTrebleBass!= null) {
            mTrebleBass.setEnabled(false);
            mTrebleBass.release();
            mTrebleBass = null;
        }
        if (mSoundMode!= null) {
            mSoundMode.setEnabled(false);
            mSoundMode.release();
            mSoundMode = null;
        }
        if (mAgc!= null) {
            mAgc.setEnabled(false);
            mAgc.release();
            mAgc = null;
        }
        if (mVirtualSurround != null) {
            mVirtualSurround.setEnabled(false);
            mVirtualSurround.release();
            mVirtualSurround = null;
        }
        if (mVirtualX!= null) {
            mVirtualX.setEnabled(false);
            mVirtualX.release();
            mVirtualX = null;
        }
        if (mDbx!= null) {
            mDbx.setEnabled(false);
            mDbx.release();
            mDbx = null;
        }
        mEffectInit = false;
    }

    private boolean creatVirtualXAudioEffects() {
        try {
            if (mVirtualX == null) {
                if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "begin to create VirtualX effect");
                mVirtualX = new AudioEffect(EFFECT_TYPE_VIRTUAL_X, EFFECT_UUID_VIRTUAL_X, 0, 0);
            }
            int result = mVirtualX.setEnabled(true);
            if (result != AudioEffect.SUCCESS) {
                Log.e(TAG, "enable VirtualX effect fail, ret:" + result);
            }
            return true;
        } catch (RuntimeException e) {
            Log.e(TAG, "create VirtualX effect fail", e);
            return false;
        }
    }

    private boolean creatTruSurroundAudioEffects() {
        try {
            if (mTruSurround == null) {
                if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "begin to create TruSurround effect");
                mTruSurround = new AudioEffect(EFFECT_TYPE_TRUSURROUND, AudioEffect.EFFECT_TYPE_NULL, 0, 0);
            }
            int result = mTruSurround.setEnabled(true);
            if (result != AudioEffect.SUCCESS) {
                Log.e(TAG, "enable TruSurround effect fail, ret:" + result);
            }
            return true;
        } catch (RuntimeException e) {
            Log.e(TAG, "Unable to create mTruSurround audio effect", e);
            return false;
        }
    }

    private boolean creatBalanceAudioEffects() {
        try {
            if (mBalance == null) {
                if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "creatBalanceAudioEffects");
                mBalance = new AudioEffect(EFFECT_TYPE_BALANCE, AudioEffect.EFFECT_TYPE_NULL, 0, 0);
            }
            return true;
        } catch (RuntimeException e) {
            Log.e(TAG, "Unable to create mBalance audio effect", e);
            return false;
        }
    }

    private boolean creatTrebleBassAudioEffects() {
        try {
            if (mTrebleBass == null) {
                if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "creatTrebleBassAudioEffects");
                mTrebleBass = new AudioEffect(EFFECT_TYPE_TREBLE_BASS, AudioEffect.EFFECT_TYPE_NULL, 0, 0);
            }
            return true;
        } catch (RuntimeException e) {
            Log.e(TAG, "Unable to create mTrebleBass audio effect", e);
            return false;
        }
    }

    private boolean creatEqAudioEffects() {
        try {
            if (mSoundMode == null) {
                if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "creatEqAudioEffects");
                mSoundMode = new AudioEffect(EFFECT_TYPE_EQ, AudioEffect.EFFECT_TYPE_NULL, 0, 0);
                int result = mSoundMode.setEnabled(true);
                if (result == AudioEffect.SUCCESS) {
                    if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "creatEqAudioEffects enable eq");
                    mSoundMode.setParameter(PARAM_EQ_ENABLE, PARAMETERS_DAP_ENABLE);
                    Settings.Global.putString(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_SOUND_MODE_TYPE,
                            DB_ID_SOUND_EFFECT_SOUND_MODE_TYPE_EQ);
                }
            }
            return true;
        } catch (RuntimeException e) {
            Log.e(TAG, "Unable to create Eq audio effect", e);
            return false;
        }
    }

    private boolean creatAgcAudioEffects() {
        try {
            if (mAgc == null) {
                if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "creatAgcAudioEffects");
                mAgc = new AudioEffect(EFFECT_TYPE_AGC, AudioEffect.EFFECT_TYPE_NULL, 0, 0);
            }
            return true;
        } catch (RuntimeException e) {
            Log.e(TAG, "Unable to create Agc audio effect", e);
            return false;
        }
    }

    private boolean creatVirtualSurroundAudioEffects() {
        try {
            if (mVirtualSurround == null) {
                if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "creatVirtualSurroundAudioEffects");
                mVirtualSurround = new AudioEffect(EFFECT_TYPE_VIRTUAL_SURROUND, AudioEffect.EFFECT_TYPE_NULL, 0, 0);
            }
            return true;
        } catch (RuntimeException e) {
            Log.e(TAG, "Unable to create VirtualSurround audio effect", e);
            return false;
        }
    }

    private boolean creatDbxAudioEffects() {
        try {
            if (mDbx == null) {
                if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "begin to create DBX effect");
                mDbx = new AudioEffect(EFFECT_TYPE_DBX, EFFECT_UUID_DBX, 0, 0);
            }
            int result = mDbx.setEnabled(true);
            if (result != AudioEffect.SUCCESS) {
                Log.e(TAG, "enable DBX effect fail, ret:" + result);
            }
            return true;
        } catch (RuntimeException e) {
            Log.e(TAG, "create DBX effect fail", e);
            return false;
        }
    }

    public boolean isSupportVirtualX() {
        return mSupportVirtualX;
    }

    public boolean isDtsVXValid() {
        File fl = new File("/vendor/lib/soundfx/libvx.so");
        return fl.exists();
    }

    public void setDtsVirtualXMode(int virtalXMode) {
        if (null == mVirtualX) {
            Log.e(TAG, "The VirtualX effect is not created, the mode cannot be setDtsVirtualXMode.");
            return;
        }
        if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "setDtsVirtualXMode = " + virtalXMode);
        switch (virtalXMode) {
            case AudioEffectManager.SOUND_EFFECT_VIRTUALX_MODE_OFF:
                mVirtualX.setParameter(PARAM_DTS_PARAM_MBHL_ENABLE_I32, 0);
                mVirtualX.setParameter(PARAM_DTS_PARAM_TBHDX_ENABLE_I32, 0);
                mVirtualX.setParameter(PARAM_DTS_PARAM_VX_ENABLE_I32, 0);
                break;
            case AudioEffectManager.SOUND_EFFECT_VIRTUALX_MODE_BASS:
                mVirtualX.setParameter(PARAM_DTS_PARAM_MBHL_ENABLE_I32, 1);
                mVirtualX.setParameter(PARAM_DTS_PARAM_TBHDX_ENABLE_I32, 1);
                mVirtualX.setParameter(PARAM_DTS_PARAM_VX_ENABLE_I32, 0);
                break;
            case AudioEffectManager.SOUND_EFFECT_VIRTUALX_MODE_FULL:
                mVirtualX.setParameter(PARAM_DTS_PARAM_MBHL_ENABLE_I32, 1);
                mVirtualX.setParameter(PARAM_DTS_PARAM_TBHDX_ENABLE_I32, 1);
                mVirtualX.setParameter(PARAM_DTS_PARAM_VX_ENABLE_I32, 1);
                break;
            default:
                Log.w(TAG, "VirtualX effect mode invalid, mode:" + virtalXMode);
                return;
        }
        saveAudioParameters(SET_VIRTUALX_MODE, virtalXMode);
    }

    public int getDtsVirtualXMode() {
        return getSavedAudioParameters(SET_VIRTUALX_MODE);
    }

    public void setDtsTruVolumeHdEnable(boolean enable) {
        if (null == mVirtualX) {
            Log.e(TAG, "The VirtualX effect is not created, the mode cannot be setDtsTruVolumeHdEnable.");
            return;
        }
        if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "setDtsTruVolumeHdEnable = " + enable);
        int dbSwitch = enable ? 1 : 0;
        mVirtualX.setParameter(PARAM_DTS_PARAM_LOUDNESS_CONTROL_ENABLE_I32, dbSwitch);
        saveAudioParameters(SET_TRUVOLUME_HD_ENABLE, dbSwitch);
    }

    public boolean getDtsTruVolumeHdEnable() {
        int dbSwitch = getSavedAudioParameters(SET_TRUVOLUME_HD_ENABLE);
        boolean enable = (1 == dbSwitch);
        if (dbSwitch != 1 && dbSwitch != 0) {
            Log.w(TAG, "DTS Tru Volume HD db value invalid, db:" + dbSwitch + ", return default false");
        }
        return enable;
    }

    public int getSoundModeStatus () {
        int saveresult = -1;
        if (mSoundMode == null) {
            Log.e(TAG, "getSoundModeStatus eq sound is not created");
            return AudioEffectManager.EQ_SOUND_MODE_STANDARD;
        }
        int[] value = new int[1];
        mSoundMode.getParameter(PARAM_EQ_EFFECT, value);
        saveresult = getSavedAudioParameters(SET_SOUND_MODE);
        if (saveresult != value[0]) {
            Log.w(TAG, "getSoundModeStatus erro get: " + value[0] + ", saved: " + saveresult);
        } else if (DroidLogicUtils.getAudioDebugEnable()) {
            Log.d(TAG, "getSoundModeStatus = " + saveresult);
        }
        return saveresult;
    }

    // The interface will be deprecated.
    public int getSoundModule() {
        return 1;
    }

    public int getTrebleStatus () {
        int saveresult = -1;
        if (!creatTrebleBassAudioEffects()) {
            Log.e(TAG, "getTrebleStatus mTrebleBass creat fail");
            return AudioEffectManager.EFFECT_TREBLE_DEFAULT;
        }
        int[] value = new int[1];
        mTrebleBass.getParameter(PARAM_TREBLE_LEVEL, value);
        saveresult = getSavedAudioParameters(SET_TREBLE);
        if (saveresult != value[0]) {
            Log.w(TAG, "getTrebleStatus erro get: " + value[0] + ", saved: " + saveresult);
        } else if (DroidLogicUtils.getAudioDebugEnable()) {
            Log.d(TAG, "getTrebleStatus = " + saveresult);
        }
        return saveresult;
    }

    public int getBassStatus () {
        int saveresult = -1;
        if (!creatTrebleBassAudioEffects()) {
            Log.e(TAG, "getBassStatus mTrebleBass creat fail");
            return AudioEffectManager.EFFECT_BASS_DEFAULT;
        }
        int[] value = new int[1];
        mTrebleBass.getParameter(PARAM_BASS_LEVEL, value);
        saveresult = getSavedAudioParameters(SET_BASS);
        if (saveresult != value[0]) {
            Log.w(TAG, "getBassStatus erro get: " + value[0] + ", saved: " + saveresult);
        } else if (DroidLogicUtils.getAudioDebugEnable()) {
            Log.d(TAG, "getBassStatus = " + saveresult);
        }
        return saveresult;
    }

    public int getBalanceStatus () {
        int saveresult = -1;
        if (!creatBalanceAudioEffects()) {
            Log.e(TAG, "getBalanceStatus mBalance creat fail");
            return AudioEffectManager.EFFECT_BALANCE_DEFAULT;
        }
        int[] value = new int[1];
        mBalance.getParameter(PARAM_BALANCE_LEVEL, value);
        saveresult = getSavedAudioParameters(SET_BALANCE);
        if (saveresult != value[0]) {
            Log.w(TAG, "getBalanceStatus erro get: " + value[0] + ", saved: " + saveresult);
        } else if (DroidLogicUtils.getAudioDebugEnable()) {
            Log.d(TAG, "getBalanceStatus = " + saveresult);
        }
        return saveresult;
    }

    public boolean getAgcEnableStatus () {
        int saveresult = -1;
        if (!creatAgcAudioEffects()) {
            Log.e(TAG, "getAgcEnableStatus mAgc creat fail");
            return DEFAULT_AGC_ENABLE;
        }
        int[] value = new int[1];
        mAgc.getParameter(PARAM_AGC_ENABLE, value);
        saveresult = getSavedAudioParameters(SET_AGC_ENABLE);
        if (saveresult != value[0]) {
            Log.w(TAG, "getAgcEnableStatus erro get: " + value[0] + ", saved: " + saveresult);
        } else if (DroidLogicUtils.getAudioDebugEnable()) {
            Log.d(TAG, "getAgcEnableStatus = " + saveresult);
        }
        return saveresult == 1;
    }

    public int getAgcMaxLevelStatus () {
        int saveresult = -1;
        if (!creatAgcAudioEffects()) {
            Log.e(TAG, "getAgcEnableStatus mAgc creat fail");
            return DEFAULT_AGC_MAX_LEVEL;
        }
        int[] value = new int[1];
        mAgc.getParameter(PARAM_AGC_MAX_LEVEL, value);
        saveresult = getSavedAudioParameters(SET_AGC_MAX_LEVEL);
        if (saveresult != value[0]) {
            Log.w(TAG, "getAgcMaxLevelStatus erro get: " + value[0] + ", saved: " + saveresult);
        } else if (DroidLogicUtils.getAudioDebugEnable()) {
            Log.d(TAG, "getAgcMaxLevelStatus = " + saveresult);
        }
        return value[0];
    }

    public int getAgcAttackTimeStatus () {
        int saveresult = -1;
        if (!creatAgcAudioEffects()) {
            Log.e(TAG, "getAgcAttackTimeStatus mAgc creat fail");
            return DEFAULT_AGC_ATTACK_TIME;
        }
        int[] value = new int[1];
        mAgc.getParameter(PARAM_AGC_ATTACK_TIME, value);
        saveresult = getSavedAudioParameters(SET_AGC_ATTACK_TIME);
        if (saveresult != value[0] / 48) {
            Log.w(TAG, "getAgcAttackTimeStatus erro get: " + value[0] + ", saved: " + saveresult);
        } else if (DroidLogicUtils.getAudioDebugEnable()) {
            Log.d(TAG, "getAgcAttackTimeStatus = " + saveresult);
        }
        //value may be changed realtime
        return value[0] / 48;
    }

    public int getAgcReleaseTimeStatus () {
        int saveresult = -1;
        if (!creatAgcAudioEffects()) {
            Log.e(TAG, "getAgcReleaseTimeStatus mAgc creat fail");
            return DEFAULT_AGC_RELEASE_TIME;
        }
        int[] value = new int[1];
        mAgc.getParameter(PARAM_AGC_RELEASE_TIME, value);
        saveresult = getSavedAudioParameters(SET_AGC_RELEASE_TIME);
        if (saveresult != value[0]) {
            Log.w(TAG, "getAgcReleaseTimeStatus erro get: " + value[0] + ", saved: " + saveresult);
        } else if (DroidLogicUtils.getAudioDebugEnable()) {
            Log.d(TAG, "getAgcReleaseTimeStatus = " + saveresult);
        }
        //value may be changed realtime
        return value[0];
    }

    public int getAgcSourceIdStatus () {
        int saveresult = -1;
        if (!creatAgcAudioEffects()) {
            Log.e(TAG, "getAgcSourceIdStatus mAgc creat fail");
            return DEFAULT_AGC_RELEASE_TIME;
        }
        int[] value = new int[1];
        mAgc.getParameter(PARAM_AGC_SOURCE_ID, value);
        saveresult = getSavedAudioParameters(SET_AGC_SOURCE_ID);
        if (saveresult != value[0]) {
            Log.w(TAG, "getAgcSourceIdStatus erro get: " + value[0] + ", saved: " + saveresult);
        } else if (DroidLogicUtils.getAudioDebugEnable()) {
            Log.d(TAG, "getAgcSourceIdStatus = " + saveresult);
        }
        //value may be changed realtime
        return value[0];
    }

    // 0 1 ~ off on
    public int getVirtualSurroundStatus() {
        int saveresult = -1;
        if (!creatVirtualSurroundAudioEffects()) {
            Log.e(TAG, "getVirtualSurroundStatus mVirtualSurround creat fail");
            return OutputModeManager.VIRTUAL_SURROUND_OFF;
        }
        int[] value = new int[1];
        mVirtualSurround.getParameter(PARAM_VIRTUALSURROUND, value);
        saveresult = getSavedAudioParameters(SET_VIRTUAL_SURROUND);
        if (saveresult != value[0]) {
            Log.w(TAG, "getVirtualSurroundStatus erro get: " + value[0] + ", saved: " + saveresult);
        } else if (DroidLogicUtils.getAudioDebugEnable()) {
            Log.d(TAG, "getVirtualSurroundStatus = " + saveresult);
        }
        return saveresult;
    }

    //set sound mode except customed one
    public void setSoundMode (int mode) {
        //need to set sound mode by observer listener
        saveAudioParameters(SET_SOUND_MODE, mode);
    }

    public void setSoundModeByObserver (int mode) {
        if (mSoundMode == null) {
            Log.e(TAG, "setSoundModeByObserver eq sound is not created");
            return;
        }
        int result = mSoundMode.setEnabled(true);
        if (result == AudioEffect.SUCCESS) {
            if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "setSoundMode = " + mode);
            mSoundMode.setParameter(PARAM_EQ_EFFECT, mode);
            if (mode == AudioEffectManager.EQ_SOUND_MODE_CUSTOM) {
                //set one band, at the same time the others will be set
                setDifferentBandEffects(AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND1, getSavedAudioParameters(SET_EFFECT_BAND1), false);
            }
            //need to set sound mode by observer listener
            //saveAudioParameters(SET_SOUND_MODE, mode);
        }
    }

    public void setUserSoundModeParam(int bandNumber, int value) {
        if (null == mSoundMode) {
            Log.e(TAG, "The EQ effect is not created, the mode cannot be setUserSoundModeParam.");
            return;
        }
        if (DroidLogicUtils.getAudioDebugEnable()) {
            Log.d(TAG, "setUserSoundModeParam bandNumber:" + bandNumber + ", value:" + value);
        }
        if (bandNumber > AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND5 || bandNumber < AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND1) {
            Log.e(TAG, "the EQ band number:" + bandNumber + " invalid, set failed");
            return;
        }
        setDifferentBandEffects(bandNumber, value, true);
    }

    public int getUserSoundModeParam(int bandNumber) {
        if (bandNumber > AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND5 || bandNumber < AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND1) {
            Log.e(TAG, "the EQ band number:" + bandNumber + " invalid, set failed");
            return 0;
        }
        int value = 0;
        value = getSavedAudioParameters(bandNumber + SET_EFFECT_BAND1);
        if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "getUserSoundModeParam band number:" + bandNumber+ ", value:" + value);
        return value;
    }

    private void setDifferentBandEffects(int bandnum, int value, boolean needsave) {
        if (mSoundMode == null) {
            Log.e(TAG, "setDifferentBandEffects eq sound is not created");
            return;
        }
        int result = mSoundMode.setEnabled(true);
        if (result == AudioEffect.SUCCESS) {
            if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "setDifferentBandEffects: NO." + bandnum + " = " + value);
            byte[] fiveband = new byte[5];
            for (int i = AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND1; i <= AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND5; i++) {
                if (bandnum == i) {
                    fiveband[i] = (byte)MappingLine(value, true);
                } else {
                    fiveband[i] = (byte) MappingLine(getSavedAudioParameters(i + SET_EFFECT_BAND1), true);
                }
            }
            Log.i(TAG, "set eq custom effect band value: " + Arrays.toString(fiveband));
            mSoundMode.setParameter(PARAM_EQ_CUSTOM, fiveband);
            if (needsave) {
                saveAudioParameters(bandnum + SET_EFFECT_BAND1, value);
            }
        }
    }
    //convert -10~10 to 0~100 controled by need or not
    private int unMappingLine(int mapval, boolean need) {
        if (!need) {
            return mapval;
        }

        final int MIN_UI_VAL = -10;
        final int MAX_UI_VAL = 10;
        final int MIN_VAL = 0;
        final int MAX_VAL = 100;
        if (mapval > MAX_UI_VAL || mapval < MIN_UI_VAL) {
            Log.e(TAG, "unMappingLine: map value:" + mapval + " invalid. set default value:" + (MAX_VAL - MIN_VAL) / 2);
            return (MAX_VAL - MIN_VAL) / 2;
        }
        return (mapval - MIN_UI_VAL) * (MAX_VAL - MIN_VAL) / (MAX_UI_VAL - MIN_UI_VAL);
    }

    //convert 0~100 to -10~10 controled by need or not
    private int MappingLine(int mapval, boolean need) {
        if (!need) {
            return mapval;
        }
        final int MIN_UI_VAL = 0;
        final int MAX_UI_VAL = 100;
        final int MIN_VAL = -10;
        final int MAX_VAL = 10;
        if (MIN_VAL < 0) {
            return (mapval - (MAX_UI_VAL + MIN_UI_VAL) / 2) * (MAX_VAL - MIN_VAL)
                   / (MAX_UI_VAL - MIN_UI_VAL);
        } else {
            return (mapval - MIN_UI_VAL) * (MAX_VAL - MIN_VAL) / (MAX_UI_VAL - MIN_UI_VAL);
        }
    }

    public void setTreble (int step) {
        if (!creatTrebleBassAudioEffects()) {
            Log.e(TAG, "setTreble mTrebleBass creat fail");
            return;
        }
        int result = mTrebleBass.setEnabled(true);
        if (result == AudioEffect.SUCCESS) {
            if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "setTreble = " + step);
            mTrebleBass.setParameter(PARAM_TREBLE_LEVEL, step);
            saveAudioParameters(SET_TREBLE, step);
        }
    }

    public void setBass (int step) {
        if (!creatTrebleBassAudioEffects()) {
            Log.e(TAG, "setBass mTrebleBass creat fail");
            return;
        }
        int result = mTrebleBass.setEnabled(true);
        if (result == AudioEffect.SUCCESS) {
            if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "setBass = " + step);
            mTrebleBass.setParameter(PARAM_BASS_LEVEL, step);
            saveAudioParameters(SET_BASS, step);
        }
    }

    public void setBalance (int step) {
        if (!creatBalanceAudioEffects()) {
            Log.e(TAG, "setBalance mBalance creat fail");
            return;
        }
        int result = mBalance.setEnabled(true);
        if (result == AudioEffect.SUCCESS) {
            if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "setBalance = " + step);
            mBalance.setParameter(PARAM_BALANCE_LEVEL, step);
            saveAudioParameters(SET_BALANCE, step);
        }
    }

    public void setSurroundEnable(boolean enable) {
        if (null == mTruSurround) {
            Log.e(TAG, "The Dts TruSurround effect is not created, the mode cannot be setSurroundEnable.");
            return;
        }
        if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "setSurroundEnable = " + enable);
        int dbSwitch = enable ? 1 : 0;
        mTruSurround.setParameter(PARAM_SRS_PARAM_SURROUND_MODE, dbSwitch);
        saveAudioParameters(SET_SURROUND_ENABLE, dbSwitch);
    }

    public boolean getSurroundEnable() {
        int dbSwitch = getSavedAudioParameters(SET_SURROUND_ENABLE);
        boolean enable = (1 == dbSwitch);
        if (dbSwitch != 1 && dbSwitch != 0) {
            Log.w(TAG, "DTS Surround enable db value invalid, db:" + dbSwitch + ", return default false");
        }
        return enable;
    }

    public void setDialogClarityMode(int mode) {
        if (null == mTruSurround) {
            Log.e(TAG, "The DTS TruSurround effect is not created, the mode cannot be setDialogClarityMode.");
            return;
        }
        if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "setDialogClarityMode = " + mode);
        mTruSurround.setParameter(PARAM_SRS_PARAM_DIALOGCLARTY_MODE, mode);
        saveAudioParameters(SET_DIALOG_CLARITY_MODE, mode);
    }

    public int getDialogClarityMode() {
        return getSavedAudioParameters(SET_DIALOG_CLARITY_MODE);
    }

    public void setTruBassEnable(boolean enable) {
        if (null == mTruSurround) {
            Log.e(TAG, "The DTS TruSurround effect is not created, the mode cannot be setTruBassEnable.");
            return;
        }
        if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "setTruBassEnable = " + enable);
        int dbSwitch = enable ? 1 : 0;
        mTruSurround.setParameter(PARAM_SRS_PARAM_TRUEBASS_ENABLE, dbSwitch);
        saveAudioParameters(SET_TRUBASS_ENABLE, dbSwitch);
    }

    public boolean getTruBassEnable() {
        int dbSwitch = getSavedAudioParameters(SET_TRUBASS_ENABLE);
        boolean enable = (1 == dbSwitch);
        if (dbSwitch != 1 && dbSwitch != 0) {
            Log.w(TAG, "DTS TreBass db value invalid, db:" + dbSwitch + ", return default false");
        }
        return enable;
    }

    public void setAgcEnable (boolean enable) {
        if (!creatAgcAudioEffects()) {
            Log.e(TAG, "setAgcEnable mAgc creat fail");
            return;
        }
        int dbSwitch = enable ? 1 : 0;
        int result = mAgc.setEnabled(true);
        if (result == AudioEffect.SUCCESS) {
            if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "setAgcEnable = " + dbSwitch);
            mAgc.setParameter(PARAM_AGC_ENABLE, dbSwitch);
            saveAudioParameters(SET_AGC_ENABLE, dbSwitch);
        }
    }

    public void setAgcMaxLevel (int step) {
        if (!creatAgcAudioEffects()) {
            Log.e(TAG, "setAgcMaxLevel mAgc creat fail");
            return;
        }
        int result = mAgc.setEnabled(true);
        if (result == AudioEffect.SUCCESS) {
            if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "setAgcMaxLevel = " + step);
            mAgc.setParameter(PARAM_AGC_MAX_LEVEL, step);
            saveAudioParameters(SET_AGC_MAX_LEVEL, step);
        }
    }

    public void setAgcAttackTime (int step) {
        if (!creatAgcAudioEffects()) {
            Log.e(TAG, "setAgcAttackTime mAgc creat fail");
            return;
        }
        int result = mAgc.setEnabled(true);
        if (result == AudioEffect.SUCCESS) {
            if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "setAgcAttackTime = " + step);
            mAgc.setParameter(PARAM_AGC_ATTACK_TIME, step * 48);
            saveAudioParameters(SET_AGC_ATTACK_TIME, step);
        }
    }

    public void setAgcReleaseTime (int step) {
        if (!creatAgcAudioEffects()) {
            Log.e(TAG, "setAgcReleaseTime mAgc creat fail");
            return;
        }
        int result = mAgc.setEnabled(true);
        if (result == AudioEffect.SUCCESS) {
            if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "setAgcReleaseTime = " + step);
            mAgc.setParameter(PARAM_AGC_RELEASE_TIME, step);
            saveAudioParameters(SET_AGC_RELEASE_TIME, step);
        }
    }

    public void setSourceIdForAvl (int step) {
        if (!creatAgcAudioEffects()) {
            Log.e(TAG, "setSourceIdForAvl mAgc creat fail");
            return;
        }
        int result = mAgc.setEnabled(true);
        if (result == AudioEffect.SUCCESS) {
            if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "setSourceIdForAvl = " + step);
            mAgc.setParameter(PARAM_AGC_SOURCE_ID, step);
            saveAudioParameters(SET_AGC_SOURCE_ID, step);
        }
    }

    public void setVirtualSurround (int mode) {
        if (!creatVirtualSurroundAudioEffects()) {
            Log.e(TAG, "setVirtualSurround mVirtualSurround creat fail");
            return;
        }
        int result = mVirtualSurround.setEnabled(true);
        if (result == AudioEffect.SUCCESS) {
            if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "setVirtualSurround = " + mode);
            mVirtualSurround.setParameter(PARAM_VIRTUALSURROUND, mode);
            saveAudioParameters(SET_VIRTUAL_SURROUND, mode);
        }
    }

    public void setDbxEnable(boolean enable) {
        if (null == mDbx) {
            Log.e(TAG, "The DBX effect is not created, the mode cannot be setDbxEnable.");
            return;
        }
        if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "setDbxEnable = " + enable);
        int dbSwitch = enable ? 1 : 0;
        mDbx.setParameter(PARAM_DBX_PARAM_ENABLE, dbSwitch);
        saveAudioParameters(SET_DBX_ENABLE, dbSwitch);
    }

    public boolean getDbxEnable() {
        int dbSwitch = getSavedAudioParameters(SET_DBX_ENABLE);
        boolean enable = (1 == dbSwitch);
        if (dbSwitch != 1 && dbSwitch != 0) {
            Log.w(TAG, "DBX enable db value invalid, db:" + dbSwitch + ", return default false");
        }
        if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "DBX get enable: " + enable);
        return enable;
    }

    public void setDbxSoundMode(int dbxMode) {
        if (null == mDbx) {
            Log.e(TAG, "The DBX effect is not created, the mode cannot be setDbxSoundMode.");
            return;
        }
        if (dbxMode > AudioEffectManager.DBX_SOUND_MODE_ADVANCED || dbxMode < AudioEffectManager.DBX_SOUND_MODE_STANDARD) {
            Log.w(TAG, "DBX sound mode invalid, mode:" + dbxMode + "setDbxSoundMode failed");
            return;
        }
        if (AudioEffectManager.DBX_SOUND_MODE_ADVANCED == dbxMode) {
            byte[] param = new byte[AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_SURROUND + 1];
            param[AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_SONICS] = (byte) getSavedAudioParameters(SET_DBX_SOUND_MODE_ADVANCED_SONICS);
            param[AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_VOLUME] = (byte) getSavedAudioParameters(SET_DBX_SOUND_MODE_ADVANCED_VOLUME);
            param[AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_SURROUND] = (byte) getSavedAudioParameters(SET_DBX_SOUND_MODE_ADVANCED_SURROUND);
            Log.i(TAG, "DBX set sound mode:" + dbxMode + ", param: " + Arrays.toString(param));
            mDbx.setParameter(PARAM_DBX_SET_MODE, param);
        } else {
            Log.i(TAG, "DBX set sound mode:" + dbxMode + ", param: " + Arrays.toString(AudioEffectManager.SOUND_EFFECT_DBX_SOUND_MODE_ARRAY_DEFAULT[dbxMode]));
            mDbx.setParameter(PARAM_DBX_SET_MODE, AudioEffectManager.SOUND_EFFECT_DBX_SOUND_MODE_ARRAY_DEFAULT[dbxMode]);
        }
        saveAudioParameters(SET_DBX_SOUND_MODE, dbxMode);
    }

    public int getDbxSoundMode() {
        int soundMode = 0;
        soundMode = getSavedAudioParameters(SET_DBX_SOUND_MODE);
        if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "DBX get sound mode: " + soundMode);
        return soundMode;
    }

    public void setDbxAdvancedModeParam(int paramType, int value) {
        if (null == mDbx) {
            Log.e(TAG, "The DBX effect is not created, the mode cannot be setDbxAdvancedModeParam.");
            return;
        }
        switch (paramType) {
            case AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_SONICS:
                if (value > 4 || value < 0) {
                    Log.e(TAG, "setDbxAdvancedModeParam invalid sonics value:" + value);
                    return;
                }
                saveAudioParameters(SET_DBX_SOUND_MODE_ADVANCED_SONICS, value);
                break;
            case AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_VOLUME:
                if (value > 2 || value < 0) {
                    Log.e(TAG, "setDbxAdvancedModeParam invalid volume value:" + value);
                    return;
                }
                saveAudioParameters(SET_DBX_SOUND_MODE_ADVANCED_VOLUME, value);
                break;
            case AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_SURROUND:
                if (value > 2 || value < 0) {
                    Log.e(TAG, "setDbxAdvancedModeParam invalid surround value:" + value);
                    return;
                }
                saveAudioParameters(SET_DBX_SOUND_MODE_ADVANCED_SURROUND, value);
                break;
            default:
                Log.e(TAG, "setDbxAdvancedModeParam invalid type:" + paramType);
                return;
        }
        byte[] param = new byte[AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_SURROUND + 1];
        param[AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_SONICS] = (byte) getSavedAudioParameters(SET_DBX_SOUND_MODE_ADVANCED_SONICS);
        param[AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_VOLUME] = (byte) getSavedAudioParameters(SET_DBX_SOUND_MODE_ADVANCED_VOLUME);
        param[AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_SURROUND] = (byte) getSavedAudioParameters(SET_DBX_SOUND_MODE_ADVANCED_SURROUND);
        if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "DBX set advanced sound mode param: " + Arrays.toString(param));
        mDbx.setParameter(PARAM_DBX_SET_MODE, param);
    }

    public int getDbxAdvancedModeParam(int paramType) {
        int value = 0;
        switch (paramType) {
            case AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_SONICS:
                value = getSavedAudioParameters(SET_DBX_SOUND_MODE_ADVANCED_SONICS);
                break;
            case AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_VOLUME:
                value = getSavedAudioParameters(SET_DBX_SOUND_MODE_ADVANCED_VOLUME);
                break;
            case AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_SURROUND:
                value = getSavedAudioParameters(SET_DBX_SOUND_MODE_ADVANCED_SURROUND);
                break;
            default:
                Log.e(TAG, "getDbxAdvancedModeParam invalid type:" + paramType + ", return 0");
                break;
        }
        if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "getDbxAdvancedModeParam paramType:" + paramType+ ", value:" + value);
        return value;
    }

    private void saveAudioParameters(int id, int value) {
        if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "saveAudioParameters id:" + id+ ", value:" + value);
        switch (id) {
            case SET_BASS:
                if (AudioEffectManager.EQ_SOUND_MODE_CUSTOM == getSoundModeFromDb() || mSupportMs12Dap) {
                    Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BASS, value);
                }
                break;
            case SET_TREBLE:
                if (AudioEffectManager.EQ_SOUND_MODE_CUSTOM == getSoundModeFromDb() || mSupportMs12Dap) {
                    Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_TREBLE, value);
                }
                break;
            case SET_BALANCE:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BALANCE, value);
                break;
            case SET_DIALOG_CLARITY_MODE:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DIALOG_CLARITY, value);
                break;
            case SET_SURROUND_ENABLE:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_SURROUND, value);
                break;
            case SET_TRUBASS_ENABLE:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_TRUBASS, value);
                break;
            case SET_SOUND_MODE:
                String soundmodetype = Settings.Global.getString(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_SOUND_MODE_TYPE);
                if (soundmodetype == null || DB_ID_SOUND_EFFECT_SOUND_MODE_TYPE_EQ.equals(soundmodetype)) {
                    Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_SOUND_MODE_EQ_VALUE, value);
                } else if ((DB_ID_SOUND_EFFECT_SOUND_MODE_TYPE_DAP.equals(soundmodetype))) {
                    Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_SOUND_MODE_DAP_VALUE, value);
                } else {
                    Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_SOUND_MODE, value);
                }
                break;
            case SET_EFFECT_BAND1:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND1, value);
                break;
            case SET_EFFECT_BAND2:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND2, value);
                break;
            case SET_EFFECT_BAND3:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND3, value);
                break;
            case SET_EFFECT_BAND4:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND4, value);
                break;
            case SET_EFFECT_BAND5:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND5, value);
                break;
            case SET_AGC_ENABLE:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_AGC_ENABLE, value);
                break;
            case SET_AGC_MAX_LEVEL:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_AGC_MAX_LEVEL, value);
                break;
            case SET_AGC_ATTACK_TIME:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_AGC_ATTACK_TIME, value);
                break;
            case SET_AGC_RELEASE_TIME:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_AGC_RELEASE_TIME, value);
                break;
            case SET_AGC_SOURCE_ID:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_AGC_SOURCE_ID, value);
                break;
            case SET_VIRTUAL_SURROUND:
                Settings.Global.putInt(mContext.getContentResolver(), OutputModeManager.VIRTUAL_SURROUND, value);
                break;
            case SET_VIRTUALX_MODE:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_VIRTUALX_MODE, value);
                break;
            case SET_TRUVOLUME_HD_ENABLE:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_TREVOLUME_HD, value);
                break;
            case SET_DBX_ENABLE:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DBX_ENABLE, value);
                break;
            case SET_DBX_SOUND_MODE:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DBX_SOUND_MODE, value);
                break;
            case SET_DBX_SOUND_MODE_ADVANCED_SONICS:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DBX_ADVANCED_MODE_SONICS, value);
                break;
            case SET_DBX_SOUND_MODE_ADVANCED_VOLUME:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DBX_ADVANCED_MODE_VOLUME, value);
                break;
            case SET_DBX_SOUND_MODE_ADVANCED_SURROUND:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DBX_ADVANCED_MODE_SURROUND, value);
                break;
            default:
                break;
        }
    }

    private int getSoundModeFromDb() {
        String soundmodetype = Settings.Global.getString(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_SOUND_MODE_TYPE);
        if (soundmodetype == null || DB_ID_SOUND_EFFECT_SOUND_MODE_TYPE_EQ.equals(soundmodetype)) {
            return Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_SOUND_MODE_EQ_VALUE, AudioEffectManager.EQ_SOUND_MODE_STANDARD);
        } else if ((DB_ID_SOUND_EFFECT_SOUND_MODE_TYPE_DAP.equals(soundmodetype))) {
            return Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_SOUND_MODE_DAP_VALUE, AudioEffectManager.EQ_SOUND_MODE_STANDARD);
        } else {
            return Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_SOUND_MODE, AudioEffectManager.EQ_SOUND_MODE_STANDARD);
        }
    }

    private int getSavedAudioParameters(int id) {
        if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "getSavedAudioParameters id:" + id);
        int result = -1;
        switch (id) {
            case SET_BASS:
                if (AudioEffectManager.EQ_SOUND_MODE_CUSTOM == getSoundModeFromDb() || mSupportMs12Dap) {
                    result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BASS, AudioEffectManager.EFFECT_BASS_DEFAULT);
                } else {
                    result = AudioEffectManager.EFFECT_BASS_DEFAULT;
                }
                break;
            case SET_TREBLE:
                if (AudioEffectManager.EQ_SOUND_MODE_CUSTOM == getSoundModeFromDb() || mSupportMs12Dap) {
                    result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_TREBLE, AudioEffectManager.EFFECT_TREBLE_DEFAULT);
                } else {
                    result = AudioEffectManager.EFFECT_TREBLE_DEFAULT;
                }
                break;
            case SET_BALANCE:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BALANCE, AudioEffectManager.EFFECT_BALANCE_DEFAULT);
                break;
            case SET_DIALOG_CLARITY_MODE:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DIALOG_CLARITY, AudioEffectManager.SOUND_EFFECT_DIALOG_CLARITY_ENABLE_DEFAULT);
                break;
            case SET_SURROUND_ENABLE:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_SURROUND, AudioEffectManager.SOUND_EFFECT_SURROUND_ENABLE_DEFAULT);
                break;
            case SET_TRUBASS_ENABLE:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_TRUBASS, AudioEffectManager.SOUND_EFFECT_TRUBASS_ENABLE_DEFAULT);
                break;
            case SET_SOUND_MODE:
                result = getSoundModeFromDb();
                Log.d(TAG, "getSavedAudioParameters SET_SOUND_MODE = " + result);
                break;
            case SET_EFFECT_BAND1:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND1, EFFECT_SOUND_MODE_USER_BAND[AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND1]);
                break;
            case SET_EFFECT_BAND2:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND2, EFFECT_SOUND_MODE_USER_BAND[AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND2]);
                break;
            case SET_EFFECT_BAND3:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND3, EFFECT_SOUND_MODE_USER_BAND[AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND3]);
                break;
            case SET_EFFECT_BAND4:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND4, EFFECT_SOUND_MODE_USER_BAND[AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND4]);
                break;
            case SET_EFFECT_BAND5:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND5, EFFECT_SOUND_MODE_USER_BAND[AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND5]);
                break;
            case SET_AGC_ENABLE:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_AGC_ENABLE, DEFAULT_AGC_ENABLE ? 1 : 0);
                break;
            case SET_AGC_MAX_LEVEL:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_AGC_MAX_LEVEL, DEFAULT_AGC_MAX_LEVEL);
                break;
            case SET_AGC_ATTACK_TIME:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_AGC_ATTACK_TIME, DEFAULT_AGC_ATTACK_TIME);
                break;
            case SET_AGC_RELEASE_TIME:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_AGC_RELEASE_TIME, DEFAULT_AGC_RELEASE_TIME);
                break;
            case SET_AGC_SOURCE_ID:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_AGC_SOURCE_ID, DEFAULT_AGC_SOURCE_ID);
                break;
            case SET_VIRTUAL_SURROUND:
                result = Settings.Global.getInt(mContext.getContentResolver(), OutputModeManager.VIRTUAL_SURROUND, OutputModeManager.VIRTUAL_SURROUND_OFF);
                break;
            case SET_VIRTUALX_MODE:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_VIRTUALX_MODE, AudioEffectManager.SOUND_EFFECT_VIRTUALX_MODE_DEFAULT);
                break;
            case SET_TRUVOLUME_HD_ENABLE:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_TREVOLUME_HD, AudioEffectManager.SOUND_EFFECT_TRUVOLUME_HD_ENABLE_DEFAULT);
                break;
            case SET_DBX_ENABLE:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DBX_ENABLE, AudioEffectManager.SOUND_EFFECT_DBX_ENABLE_DEFAULT);
                break;
            case SET_DBX_SOUND_MODE:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DBX_SOUND_MODE, AudioEffectManager.SOUND_EFFECT_DBX_SOUND_MODE_DEFAULT);
                break;
            case SET_DBX_SOUND_MODE_ADVANCED_SONICS:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DBX_ADVANCED_MODE_SONICS,
                        AudioEffectManager.SOUND_EFFECT_DBX_SOUND_MODE_ARRAY_DEFAULT[AudioEffectManager.DBX_SOUND_MODE_ADVANCED][AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_SONICS]);
                break;
            case SET_DBX_SOUND_MODE_ADVANCED_VOLUME:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DBX_ADVANCED_MODE_VOLUME,
                        AudioEffectManager.SOUND_EFFECT_DBX_SOUND_MODE_ARRAY_DEFAULT[AudioEffectManager.DBX_SOUND_MODE_ADVANCED][AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_VOLUME]);
                break;
            case SET_DBX_SOUND_MODE_ADVANCED_SURROUND:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DBX_ADVANCED_MODE_SURROUND,
                        AudioEffectManager.SOUND_EFFECT_DBX_SOUND_MODE_ARRAY_DEFAULT[AudioEffectManager.DBX_SOUND_MODE_ADVANCED][AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_SURROUND]);
                break;
            default:
                break;
        }
        return result;
    }

    public void initSoundEffectSettings() {
        Log.d(TAG, "initSoundEffectSettings...");
        if (Settings.Global.getInt(mContext.getContentResolver(), "set_five_band", 0) == 0) {
            if (mSoundMode != null) {
                byte[] fiveBandNum = new byte[5];
                mSoundMode.getParameter(PARAM_EQ_CUSTOM, fiveBandNum);
                for (int i = SET_EFFECT_BAND1; i <= SET_EFFECT_BAND5; i++) {
                    saveAudioParameters(i, unMappingLine(fiveBandNum[i - SET_EFFECT_BAND1], true));
                }
            } else {
                for (int i = SET_EFFECT_BAND1; i <= SET_EFFECT_BAND5; i++) {
                    saveAudioParameters(i, EFFECT_SOUND_MODE_USER_BAND[i - SET_EFFECT_BAND1]);
                }
                Log.w(TAG, "get default band value fail, set default value, mSoundMode == null");
            }
            Settings.Global.putInt(mContext.getContentResolver(), "set_five_band", 1);
        }

        int soundMode = getSavedAudioParameters(SET_SOUND_MODE);
        setSoundModeByObserver(soundMode);
        setBass(getSavedAudioParameters(SET_BASS));
        setTreble(getSavedAudioParameters(SET_TREBLE));
        setAgcEnable(getSavedAudioParameters(SET_AGC_ENABLE) != 0);
        setAgcMaxLevel(getSavedAudioParameters(SET_AGC_MAX_LEVEL));
        setAgcAttackTime (getSavedAudioParameters(SET_AGC_ATTACK_TIME));
        setAgcReleaseTime(getSavedAudioParameters(SET_AGC_RELEASE_TIME));
        setSourceIdForAvl(getSavedAudioParameters(SET_AGC_SOURCE_ID));
        setVirtualSurround(getSavedAudioParameters(SET_VIRTUAL_SURROUND));
        setBalance(getSavedAudioParameters(SET_BALANCE));
        applyAudioEffectByPlayEmptyTrack();

        if (isSupportVirtualX()) {
            setDtsVirtualXMode(getDtsVirtualXMode());
            setDtsTruVolumeHdEnable(getDtsTruVolumeHdEnable());
        } else {
            setSurroundEnable(getSurroundEnable());
            setDialogClarityMode(getDialogClarityMode());
            setTruBassEnable(getTruBassEnable());
        }

        boolean dbxStatus = getDbxEnable();
        if (dbxStatus) {
            int dbxMode = getSavedAudioParameters(SET_DBX_SOUND_MODE);
            setDbxEnable(dbxStatus);
            setDbxSoundMode(dbxMode);
        }
    }

    public void resetSoundEffectSettings() {
        Log.d(TAG, "resetSoundEffectSettings");
        cleanupAudioEffects();
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BASS, AudioEffectManager.EFFECT_BASS_DEFAULT);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_TREBLE, AudioEffectManager.EFFECT_TREBLE_DEFAULT);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BALANCE, AudioEffectManager.EFFECT_BALANCE_DEFAULT);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DIALOG_CLARITY, AudioEffectManager.SOUND_EFFECT_DIALOG_CLARITY_ENABLE_DEFAULT);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_SURROUND, AudioEffectManager.SOUND_EFFECT_SURROUND_ENABLE_DEFAULT);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_TRUBASS, AudioEffectManager.SOUND_EFFECT_TRUBASS_ENABLE_DEFAULT);
        Settings.Global.putString(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_SOUND_MODE_TYPE, DB_ID_SOUND_EFFECT_SOUND_MODE_TYPE_EQ);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_SOUND_MODE, AudioEffectManager.EQ_SOUND_MODE_STANDARD);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_SOUND_MODE_DAP_VALUE, AudioEffectManager.EQ_SOUND_MODE_STANDARD);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_SOUND_MODE_EQ_VALUE, AudioEffectManager.EQ_SOUND_MODE_STANDARD);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND1, EFFECT_SOUND_MODE_USER_BAND[AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND1]);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND2, EFFECT_SOUND_MODE_USER_BAND[AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND2]);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND3, EFFECT_SOUND_MODE_USER_BAND[AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND3]);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND4, EFFECT_SOUND_MODE_USER_BAND[AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND4]);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND5, EFFECT_SOUND_MODE_USER_BAND[AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND5]);
        Settings.Global.putInt(mContext.getContentResolver(), "set_five_band", 0);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_AGC_ENABLE, DEFAULT_AGC_ENABLE ? 1 : 0);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_AGC_MAX_LEVEL, DEFAULT_AGC_MAX_LEVEL);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_AGC_ATTACK_TIME, DEFAULT_AGC_ATTACK_TIME);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_AGC_RELEASE_TIME, DEFAULT_AGC_RELEASE_TIME);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_AGC_SOURCE_ID, DEFAULT_AGC_SOURCE_ID);
        Settings.Global.putInt(mContext.getContentResolver(), OutputModeManager.VIRTUAL_SURROUND, OutputModeManager.VIRTUAL_SURROUND_OFF);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_VIRTUALX_MODE, AudioEffectManager.SOUND_EFFECT_VIRTUALX_MODE_DEFAULT);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_TREVOLUME_HD, AudioEffectManager.SOUND_EFFECT_TRUVOLUME_HD_ENABLE_DEFAULT);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DBX_ENABLE, AudioEffectManager.SOUND_EFFECT_DBX_ENABLE_DEFAULT);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DBX_SOUND_MODE, AudioEffectManager.SOUND_EFFECT_DBX_SOUND_MODE_DEFAULT);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DBX_ADVANCED_MODE_SONICS,
                AudioEffectManager.SOUND_EFFECT_DBX_SOUND_MODE_ARRAY_DEFAULT[AudioEffectManager.DBX_SOUND_MODE_ADVANCED][AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_SONICS]);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DBX_ADVANCED_MODE_VOLUME,
                AudioEffectManager.SOUND_EFFECT_DBX_SOUND_MODE_ARRAY_DEFAULT[AudioEffectManager.DBX_SOUND_MODE_ADVANCED][AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_VOLUME]);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DBX_ADVANCED_MODE_SURROUND,
                AudioEffectManager.SOUND_EFFECT_DBX_SOUND_MODE_ARRAY_DEFAULT[AudioEffectManager.DBX_SOUND_MODE_ADVANCED][AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_SURROUND]);
        initSoundEffectSettings();
    }

    private void applyAudioEffectByPlayEmptyTrack() {
        int bufsize = AudioTrack.getMinBufferSize(8000, AudioFormat.CHANNEL_OUT_STEREO, AudioFormat.ENCODING_PCM_16BIT);
        if (bufsize <= 0)
            return;
        byte data[] = new byte[bufsize];
        AudioTrack trackplayer = new AudioTrack(AudioManager.STREAM_MUSIC, 8000, AudioFormat.CHANNEL_OUT_STEREO,
                AudioFormat.ENCODING_PCM_16BIT, bufsize, AudioTrack.MODE_STREAM);
        trackplayer.play();
        for (int i = 0; i <= 5; i++) {
            trackplayer.write(data, 0, data.length);
        }
        trackplayer.stop();
        trackplayer.release();
    }

    private boolean creatDapAudioEffect() {
        try {
            if (mDap == null) {
                if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "creatDapAudioEffect");
                if (AudioEffectManager.SOUND_EFFECT_DAP_VERSION == AudioEffectManager.SOUND_EFFECT_DAP_VERSION_1_3_2) {
                    mDap = new AudioEffect(EFFECT_TYPE_DAP_1_3_2, AudioEffect.EFFECT_TYPE_NULL, 0, 0);
                } else if (AudioEffectManager.SOUND_EFFECT_DAP_VERSION == AudioEffectManager.SOUND_EFFECT_DAP_VERSION_2_4){
                    mDap = new AudioEffect(EFFECT_TYPE_DAP_2_4, AudioEffect.EFFECT_TYPE_NULL, 0, 0);
                }
                int result = mDap.setEnabled(true);
                if (result == AudioEffect.SUCCESS) {
                    Log.d(TAG, "creatDapAudioEffect setEnabled success");
                } else {
                    Log.w(TAG, "creatDapAudioEffect setEnabled error: "+result);
                    return false;
                }
            }
            return true;
        } catch (RuntimeException e) {
            Log.e(TAG, "Unable to create Dap audio effect", e);
            return false;
        }
    }

    private void initDap_1_3_2() {
       int mode = getDapParam(AudioEffectManager.CMD_DAP_EFFECT_MODE);
       if (Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_SAVED, 0) == 0) {
           Log.i(TAG, "initDap_1_3_2 first boot.");
           int id = 0;
           //the first time, use the param from so load from ini file
           setDapParam(AudioEffectManager.CMD_DAP_EFFECT_MODE, AudioEffectManager.DAP_MODE_USER);
           for (id = AudioEffectManager.CMD_DAP_GEQ_ENABLE; id <= AudioEffectManager.CMD_DAP_VIRTUALIZER_ENABLE; id++)
               saveDapParam(id, getDapParamInternal(id));
           for (id = AudioEffectManager.SUBCMD_DAP_GEQ_BAND1; id <= AudioEffectManager.SUBCMD_DAP_GEQ_BAND5; id++)
               saveDapParam(id, getDapParamInternal(id));
           Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_SAVED, 1);
       } else {
           saveDapParam(AudioEffectManager.CMD_DAP_EFFECT_MODE, AudioEffectManager.DAP_MODE_USER);
           setDapParam(AudioEffectManager.CMD_DAP_VL_ENABLE, getDapParam(AudioEffectManager.CMD_DAP_VL_ENABLE));
           setDapParam(AudioEffectManager.CMD_DAP_VL_AMOUNT, getDapParam(AudioEffectManager.CMD_DAP_VL_AMOUNT));
           setDapParam(AudioEffectManager.CMD_DAP_DE_ENABLE, getDapParam(AudioEffectManager.CMD_DAP_DE_ENABLE));
           setDapParam(AudioEffectManager.CMD_DAP_DE_AMOUNT, getDapParam(AudioEffectManager.CMD_DAP_DE_AMOUNT));
           setDapParam(AudioEffectManager.CMD_DAP_SURROUND_BOOST, getDapParam(AudioEffectManager.CMD_DAP_SURROUND_BOOST));
           setDapParam(AudioEffectManager.CMD_DAP_GEQ_ENABLE, getDapParam(AudioEffectManager.CMD_DAP_GEQ_ENABLE));
           setDapParam(AudioEffectManager.SUBCMD_DAP_GEQ_BAND1, getDapParam(AudioEffectManager.SUBCMD_DAP_GEQ_BAND1));
           saveDapParam(AudioEffectManager.CMD_DAP_EFFECT_MODE, mode);
       }
       setDapParam(AudioEffectManager.CMD_DAP_EFFECT_MODE, mode);
   }

   private void initDap_2_4() {
       int value = 0;
       byte[] tempValue = new byte[2];
       byte[] tempValue2 = new byte[6];
       if (Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_SAVED, 0) == 0) {
           Log.i(TAG, "initDap_2_4 first boot.");
           int mode = getDapParamInternal(AudioEffectManager.CMD_DAP_2_4_PROFILE);
           Log.i(TAG, "PROFILE first boot init mode: " + mode);
           saveDbDap24Param(AudioEffectManager.CMD_DAP_2_4_PROFILE, mode);
           for (int i = AudioEffectManager.SOUND_EFFECT_DAP_2_4_PROFILE_MIN; i < AudioEffectManager.SOUND_EFFECT_DAP_2_4_PROFILE_MAX; i++) {
               mDap.setParameter(AudioEffectManager.CMD_DAP_2_4_PROFILE - AudioEffectManager.CMD_DAP_2_4_BASE_VALUE, i);
           }
           mDap.setParameter(AudioEffectManager.CMD_DAP_2_4_PROFILE - AudioEffectManager.CMD_DAP_2_4_BASE_VALUE, mode);
           Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_SAVED, 1);
       } else {
           value = getDbDap24Param(AudioEffectManager.CMD_DAP_2_4_PROFILE);
           if (value < 0) {
                value = getDapParamInternal(AudioEffectManager.CMD_DAP_2_4_PROFILE);
                saveDbDap24Param(AudioEffectManager.CMD_DAP_2_4_PROFILE, value);
           }
           Log.i(TAG, "PROFILE init value: " + value);
           mDap.setParameter(AudioEffectManager.CMD_DAP_2_4_PROFILE - AudioEffectManager.CMD_DAP_2_4_BASE_VALUE, value);

           tempValue[0] = (byte)getDbDap24Param(AudioEffectManager.CMD_DAP_2_4_SURROUND_VIRTUALIZER);
           Log.i(TAG, "SURROUND_VIRTUALIZER init value: " + tempValue[0]);
           tempValue[1] = (byte)getDbDap24Param(AudioEffectManager.SUBCMD_DAP_2_4_SURROUND_VIRTUALIZER_BOOST);
           Log.i(TAG, "SURROUND_VIRTUALIZER_BOOST init value: " + tempValue[1]);
           mDap.setParameter(AudioEffectManager.CMD_DAP_2_4_SURROUND_VIRTUALIZER - AudioEffectManager.CMD_DAP_2_4_BASE_VALUE, tempValue);

           tempValue[0] = (byte)getDbDap24Param(AudioEffectManager.CMD_DAP_2_4_DIALOGUE_ENHANCER);
           Log.i(TAG, "DIALOGUE_ENHANCER init value: " + tempValue[0]);
           tempValue[1] = (byte)getDbDap24Param(AudioEffectManager.SUBCMD_DAP_2_4_DIALOGUE_ENHANCER_AMOUNT);
           Log.i(TAG, "DIALOGUE_ENHANCER_AMOUNT init value: " + tempValue[1]);
           mDap.setParameter(AudioEffectManager.CMD_DAP_2_4_DIALOGUE_ENHANCER - AudioEffectManager.CMD_DAP_2_4_BASE_VALUE, tempValue);

           tempValue2[0] = (byte)getDbDap24Param(AudioEffectManager.CMD_DAP_2_4_BASS_ENHANCER);
           Log.i(TAG, "BASS_ENHANCER init value: " + tempValue2[0]);
           int tempInt = getDbDap24Param(AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_BOOST);
           byte[] enbo = mDap.intToByteArray(tempInt);
           tempValue2[1] = enbo[0];
           tempValue2[2] = enbo[1];
           int enboInt = mDap.byteArrayToInt(enbo);
           Log.i(TAG, "BASS_ENHANCER_BOOST init value: " + enboInt);
           int tempInt2 = (getDbDap24Param(AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_CUTOFFX100)
            + getDbDap24Param(AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_CUTOFFX1));
           byte[] encu = mDap.intToByteArray(tempInt2);
           tempValue2[3] = encu[0];
           tempValue2[4] = encu[1];
           int encuInt = mDap.byteArrayToInt(encu);
           Log.i(TAG, "BASS_ENHANCER_CUTOFF init value: " + encuInt);
           tempValue2[5] = (byte)getDbDap24Param(AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_WIDTH);
           Log.i(TAG, "BASS_ENHANCER_WIDTH init value: " + tempValue2[5]);
           mDap.setParameter(AudioEffectManager.CMD_DAP_2_4_BASS_ENHANCER - AudioEffectManager.CMD_DAP_2_4_BASE_VALUE, tempValue2);

           value = getDbDap24Param(AudioEffectManager.CMD_DAP_2_4_MI_STEERING);
           Log.i(TAG, "MI_STEERING init value: " + value);
           mDap.setParameter(AudioEffectManager.CMD_DAP_2_4_MI_STEERING - AudioEffectManager.CMD_DAP_2_4_BASE_VALUE, value);

           value = getDbDap24Param(AudioEffectManager.CMD_DAP_2_4_SURROUND_DECODER_ENABLE);
           Log.i(TAG, "SURROUND_DECODER_ENABLE init value: " + value);
           mDap.setParameter(AudioEffectManager.CMD_DAP_2_4_SURROUND_DECODER_ENABLE - AudioEffectManager.CMD_DAP_2_4_BASE_VALUE, value);

           tempValue[0] = (byte)getDbDap24Param(AudioEffectManager.CMD_DAP_2_4_LEVELER);
           Log.i(TAG, "LEVELER init value: " + tempValue[0]);
           tempValue[1] = (byte)getDbDap24Param(AudioEffectManager.SUBCMD_DAP_2_4_LEVELER_AMOUNT);
           Log.i(TAG, "LEVELER_AMOUNT init value: " + tempValue[1]);
           mDap.setParameter(AudioEffectManager.CMD_DAP_2_4_LEVELER - AudioEffectManager.CMD_DAP_2_4_BASE_VALUE, tempValue);
       }
   }

   public void initDapAudioEffect() {
       Log.i(TAG, "initDapAudioEffect dap version:" + AudioEffectManager.SOUND_EFFECT_DAP_VERSION);
       if (!creatDapAudioEffect()) {
           Log.e(TAG, "initDapAudioEffect dap create fail");
           return;
       }

       if (AudioEffectManager.SOUND_EFFECT_DAP_VERSION == AudioEffectManager.SOUND_EFFECT_DAP_VERSION_1_3_2) {
           initDap_1_3_2();
           Log.e(TAG, "init DAP1.3.2 ok");
       } else if (AudioEffectManager.SOUND_EFFECT_DAP_VERSION == AudioEffectManager.SOUND_EFFECT_DAP_VERSION_2_4){
           Log.e(TAG, "init DAP2.4 ok");
           initDap_2_4();
       }
       applyAudioEffectByPlayEmptyTrack();
   }


    private int getDapParamInternal(int id) {
        int result = 0;
        int[] value = new int[1];
        switch (id) {
            case AudioEffectManager.SUBCMD_DAP_GEQ_BAND1:
            case AudioEffectManager.SUBCMD_DAP_GEQ_BAND2:
            case AudioEffectManager.SUBCMD_DAP_GEQ_BAND3:
            case AudioEffectManager.SUBCMD_DAP_GEQ_BAND4:
            case AudioEffectManager.SUBCMD_DAP_GEQ_BAND5:
                byte[] tempValue = new byte[5];
                mDap.getParameter(AudioEffectManager.CMD_DAP_GEQ_GAINS, tempValue);
                result = tempValue[id - AudioEffectManager.SUBCMD_DAP_GEQ_BAND1];
                break;
            case AudioEffectManager.CMD_DAP_2_4_PROFILE:
                mDap.getParameter(id - AudioEffectManager.CMD_DAP_2_4_BASE_VALUE, value);
                if (value[0] < AudioEffectManager.SOUND_EFFECT_DAP_2_4_PROFILE_MIN ||  value[0] > AudioEffectManager.SOUND_EFFECT_DAP_2_4_PROFILE_MAX) {
                    Log.w(TAG, "getDapParamInternal DAP 2.4 profile mode:" + value[0] + " invalid, set the default:" +
                            AudioEffectManager.SOUND_EFFECT_DAP_2_4_PROFILE_MIN + ". id:" + id);
                    value[0] = AudioEffectManager.SOUND_EFFECT_DAP_2_4_PROFILE_MIN;
                }
                //get profile mode
                result = value[0];
                Log.e(TAG, "getDapParamInternal DAP 2.4 profile mode:" + value[0]);
                break;
            case AudioEffectManager.CMD_DAP_2_4_MI_STEERING:
                mDap.getParameter(AudioEffectManager.CMD_DAP_2_4_MI_STEERING - AudioEffectManager.CMD_DAP_2_4_BASE_VALUE, value);
                result = value[0];
                break;
            default:
                mDap.getParameter(id, value);
                result = value[0];
                break;
        }
        return result;
    }

    public void setDapParam(int id, int value) {
        if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "setDapParam id:" + id + ", value:" + value);
        byte[] fiveband = new byte[5];
        switch (id) {
            case AudioEffectManager.CMD_DAP_ENABLE:
            case AudioEffectManager.CMD_DAP_EFFECT_MODE:
            case AudioEffectManager.CMD_DAP_VL_ENABLE:
            case AudioEffectManager.CMD_DAP_VL_AMOUNT:
            case AudioEffectManager.CMD_DAP_DE_ENABLE:
            case AudioEffectManager.CMD_DAP_DE_AMOUNT:
            case AudioEffectManager.CMD_DAP_POST_GAIN:
            case AudioEffectManager.CMD_DAP_GEQ_ENABLE:
            case AudioEffectManager.CMD_DAP_SURROUND_ENABLE:
            case AudioEffectManager.CMD_DAP_SURROUND_BOOST:
                mDap.setParameter(id, value);
                saveDapParam(id, value);
                break;
            case AudioEffectManager.CMD_DAP_VIRTUALIZER_ENABLE:
                if (value == AudioEffectManager.DAP_SURROUND_SPEAKER)
                    mDap.setParameter(id, DAP_CPDP_OUTPUT_2_SPEAKER);
                else if (value == AudioEffectManager.DAP_SURROUND_HEADPHONE)
                    mDap.setParameter(id, DAP_CPDP_OUTPUT_2_HEADPHONE);
                saveDapParam(id, value);
                break;
            case AudioEffectManager.SUBCMD_DAP_GEQ_BAND1:
            case AudioEffectManager.SUBCMD_DAP_GEQ_BAND2:
            case AudioEffectManager.SUBCMD_DAP_GEQ_BAND3:
            case AudioEffectManager.SUBCMD_DAP_GEQ_BAND4:
            case AudioEffectManager.SUBCMD_DAP_GEQ_BAND5:
                fiveband[0] = (byte)getDapParam(AudioEffectManager.SUBCMD_DAP_GEQ_BAND1);
                fiveband[1] = (byte)getDapParam(AudioEffectManager.SUBCMD_DAP_GEQ_BAND2);
                fiveband[2] = (byte)getDapParam(AudioEffectManager.SUBCMD_DAP_GEQ_BAND3);
                fiveband[3] = (byte)getDapParam(AudioEffectManager.SUBCMD_DAP_GEQ_BAND4);
                fiveband[4] = (byte)getDapParam(AudioEffectManager.SUBCMD_DAP_GEQ_BAND5);
                fiveband[id - AudioEffectManager.SUBCMD_DAP_GEQ_BAND1] = (byte)value;
                mDap.setParameter(AudioEffectManager.CMD_DAP_GEQ_GAINS, fiveband);
                saveDapParam(id, value);
                break;
            case AudioEffectManager.CMD_DAP_2_4_PROFILE:
                mDap.setParameter(id - AudioEffectManager.CMD_DAP_2_4_BASE_VALUE, value);
                saveDbDap24Param(id, value);
                break;
            case AudioEffectManager.CMD_DAP_2_4_SURROUND_VIRTUALIZER:
            case AudioEffectManager.SUBCMD_DAP_2_4_SURROUND_VIRTUALIZER_BOOST:
                byte[] tempValue = new byte[2];
                tempValue[0] = (byte)getDapParam(AudioEffectManager.CMD_DAP_2_4_SURROUND_VIRTUALIZER);
                tempValue[1] = (byte)getDapParam(AudioEffectManager.SUBCMD_DAP_2_4_SURROUND_VIRTUALIZER_BOOST);
                if (id < AudioEffectManager.SUBCMD_DAP_2_4_BASE_VALUE) {
                    tempValue[0] = (byte)value;
                } else {
                    tempValue[1] = (byte)value;
                }
                mDap.setParameter(AudioEffectManager.CMD_DAP_2_4_SURROUND_VIRTUALIZER - AudioEffectManager.CMD_DAP_2_4_BASE_VALUE, tempValue);
                saveDbDap24Param(id, value);
                break;
            case AudioEffectManager.CMD_DAP_2_4_DIALOGUE_ENHANCER:
            case AudioEffectManager.SUBCMD_DAP_2_4_DIALOGUE_ENHANCER_AMOUNT:
                byte[] tempValue_1 = new byte[2];
                tempValue_1[0] = (byte)getDapParam(AudioEffectManager.CMD_DAP_2_4_DIALOGUE_ENHANCER);
                tempValue_1[1] = (byte)getDapParam(AudioEffectManager.SUBCMD_DAP_2_4_DIALOGUE_ENHANCER_AMOUNT);
                if (id < AudioEffectManager.SUBCMD_DAP_2_4_BASE_VALUE) {
                    tempValue_1[0] = (byte)value;
                } else {
                    tempValue_1[1] = (byte)value;
                }
                mDap.setParameter(AudioEffectManager.CMD_DAP_2_4_DIALOGUE_ENHANCER - AudioEffectManager.CMD_DAP_2_4_BASE_VALUE, tempValue_1);
                saveDbDap24Param(id, value);
                break;
            case AudioEffectManager.CMD_DAP_2_4_BASS_ENHANCER:
            case AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_BOOST:
            case AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_CUTOFFX100:
            case AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_CUTOFFX1:
            case AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_WIDTH:
                byte[] tempValue_2 = new byte[6];
                tempValue_2[0] = (byte)getDapParam(AudioEffectManager.CMD_DAP_2_4_BASS_ENHANCER);
                int tempInt = getDapParam(AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_BOOST);
                byte[] enbo = mDap.intToByteArray(tempInt);
                tempValue_2[1] = enbo[0];
                tempValue_2[2] = enbo[1];
                int enboInt = mDap.byteArrayToInt(enbo);
                int tempInt2 = (getDapParam(AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_CUTOFFX100)
                    + getDapParam(AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_CUTOFFX1));
                byte[] encu = mDap.intToByteArray(tempInt2);
                tempValue_2[3] = encu[0];
                tempValue_2[4] = encu[1];
                int encuInt = mDap.byteArrayToInt(encu);
                tempValue_2[5] = (byte)getDapParam(AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_WIDTH);
                Log.d(TAG, "BASS_ENHANCER getDapParam enable:" + tempValue_2[0] + ", boost:" + enboInt + ", cutoff:" + encuInt + ", width:" + tempValue_2[5]);
                if (id < AudioEffectManager.SUBCMD_DAP_2_4_BASE_VALUE) {
                    tempValue_2[0] = (byte)value;
                } else if (id == AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_BOOST) {
                    enbo = mDap.intToByteArray(value);
                    tempValue_2[1] = enbo[0];
                    tempValue_2[2] = enbo[1];
                } else if (id == AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_CUTOFFX100) {
                    encu = mDap.intToByteArray(value * 100 +
                        getDapParam(AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_CUTOFFX1));
                    tempValue_2[3] = encu[0];
                    tempValue_2[4] = encu[1];
                } else if (id == AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_CUTOFFX1) {
                    encu = mDap.intToByteArray(value  +
                        getDapParam(AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_CUTOFFX100));
                    tempValue_2[3] = encu[0];
                    tempValue_2[4] = encu[1];
                }
                else {
                    tempValue_2[5] = (byte)value;
                }
                int enbo2Int = mDap.byteArrayToInt(enbo);
                int encu2Int = mDap.byteArrayToInt(encu);
                mDap.setParameter(AudioEffectManager.CMD_DAP_2_4_BASS_ENHANCER - AudioEffectManager.CMD_DAP_2_4_BASE_VALUE, tempValue_2);
                Log.d(TAG, "BASS_ENHANCER setParameter enable:" + tempValue_2[0] + ", boost:" + enbo2Int + ", cutoff:" + encu2Int + ", width:" + tempValue_2[5]);
                saveDbDap24Param(id, value);
                break;
            case AudioEffectManager.CMD_DAP_2_4_LEVELER:
            case AudioEffectManager.SUBCMD_DAP_2_4_LEVELER_AMOUNT:
                byte[] tempValue_3 = new byte[2];
                tempValue_3[0] = (byte)getDapParam(AudioEffectManager.CMD_DAP_2_4_LEVELER);
                tempValue_3[1] = (byte)getDapParam(AudioEffectManager.SUBCMD_DAP_2_4_LEVELER_AMOUNT);
                if (id < AudioEffectManager.SUBCMD_DAP_2_4_BASE_VALUE) {
                    tempValue_3[0] = (byte)value;
                } else {
                    tempValue_3[1] = (byte)value;
                }
                mDap.setParameter(AudioEffectManager.CMD_DAP_2_4_LEVELER - AudioEffectManager.CMD_DAP_2_4_BASE_VALUE, tempValue_3);
                saveDbDap24Param(id, value);
                break;
            case AudioEffectManager.CMD_DAP_2_4_MI_STEERING:
            case AudioEffectManager.CMD_DAP_2_4_SURROUND_DECODER_ENABLE:
                mDap.setParameter(id - AudioEffectManager.CMD_DAP_2_4_BASE_VALUE, value);
                saveDbDap24Param(id, value);
                break;
        }
    }

    public void saveDapParam (int id, int value) {
        if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "saveDapParam id:" + id + ", value:" + value);
        int param = 0;
        int dapEffectMode = getDapParam(AudioEffectManager.CMD_DAP_EFFECT_MODE);
        if ((id != AudioEffectManager.CMD_DAP_EFFECT_MODE) && (dapEffectMode != AudioEffectManager.DAP_MODE_USER)) {
            Log.i(TAG, "saveDapParam id:" + id + " is not EFFECT_MODE or effect mode:" + dapEffectMode + " not user, return.");
            return;
        }

        switch (id) {
            case AudioEffectManager.CMD_DAP_EFFECT_MODE:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_MODE, value);
                break;
            case AudioEffectManager.CMD_DAP_VL_ENABLE:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_VL_ENABLE, value);
                break;
            case AudioEffectManager.CMD_DAP_VL_AMOUNT:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_VL_AMOUNT, value);
                break;
            case AudioEffectManager.CMD_DAP_DE_ENABLE:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_DE_ENABLE, value);
                break;
            case AudioEffectManager.CMD_DAP_DE_AMOUNT:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_DE_AMOUNT, value);
                break;
            case AudioEffectManager.CMD_DAP_SURROUND_ENABLE:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_SURROUND_ENABLE, value);
                break;
            case AudioEffectManager.CMD_DAP_SURROUND_BOOST:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_SURROUND_BOOST, value);
                break;
            case AudioEffectManager.CMD_DAP_POST_GAIN:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_POST_GAIN, value);
                break;
            case AudioEffectManager.CMD_DAP_GEQ_ENABLE:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_GEQ_ENABLE, value);
                break;
            case AudioEffectManager.SUBCMD_DAP_GEQ_BAND1:
                param = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_GEQ_ENABLE, AudioEffectManager.DAP_GEQ_DEFAULT);
                if (param == AudioEffectManager.DAP_GEQ_USER)
                    Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_GEQ_BAND1, value);
                break;
            case AudioEffectManager.SUBCMD_DAP_GEQ_BAND2:
                param = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_GEQ_ENABLE, AudioEffectManager.DAP_GEQ_DEFAULT);
                if (param == AudioEffectManager.DAP_GEQ_USER)
                    Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_GEQ_BAND2, value);
                break;
            case AudioEffectManager.SUBCMD_DAP_GEQ_BAND3:
                param = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_GEQ_ENABLE, AudioEffectManager.DAP_GEQ_DEFAULT);
                if (param == AudioEffectManager.DAP_GEQ_USER)
                    Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_GEQ_BAND3, value);
                break;
            case AudioEffectManager.SUBCMD_DAP_GEQ_BAND4:
                param = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_GEQ_ENABLE, AudioEffectManager.DAP_GEQ_DEFAULT);
                if (param == AudioEffectManager.DAP_GEQ_USER)
                    Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_GEQ_BAND4, value);
                break;
            case AudioEffectManager.SUBCMD_DAP_GEQ_BAND5:
                param = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_GEQ_ENABLE, AudioEffectManager.DAP_GEQ_DEFAULT);
                if (param == AudioEffectManager.DAP_GEQ_USER)
                    Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_GEQ_BAND5, value);
                break;
        }
    }

    public int getDapParam(int id) {
        int value = -1, param = 0;
        int dbValue = 0;
        if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "getDapParam id:" + id);

        if (AudioEffectManager.SOUND_EFFECT_DAP_VERSION == AudioEffectManager.SOUND_EFFECT_DAP_VERSION_1_3_2
                && id != AudioEffectManager.CMD_DAP_EFFECT_MODE) {
            value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_MODE, -1);
            if (value != AudioEffectManager.DAP_MODE_USER) {
                return getDapParamInternal(id);
            }
        }

        switch (id) {
            case AudioEffectManager.CMD_DAP_EFFECT_MODE:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_MODE, -1);
                if (value < 0) {
                    value = getDapParamInternal(AudioEffectManager.CMD_DAP_EFFECT_MODE);
                    saveDapParam(id, value);
                }
                break;
            case AudioEffectManager.CMD_DAP_VL_ENABLE:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_VL_ENABLE, AudioEffectManager.DAP_VL_DEFAULT);
                break;
            case AudioEffectManager.CMD_DAP_VL_AMOUNT:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_VL_AMOUNT, AudioEffectManager.DAP_VL_AMOUNT_DEFAULT);
                break;
            case AudioEffectManager.CMD_DAP_DE_ENABLE:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_DE_ENABLE, AudioEffectManager.DAP_DE_DEFAULT);
                break;
            case AudioEffectManager.CMD_DAP_SURROUND_ENABLE:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_SURROUND_ENABLE, AudioEffectManager.DAP_SURROUND_DEFAULT);
                break;
            case AudioEffectManager.CMD_DAP_SURROUND_BOOST:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_SURROUND_BOOST, AudioEffectManager.DAP_SURROUND_BOOST_DEFAULT);
                break;
            case AudioEffectManager.CMD_DAP_DE_AMOUNT:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_DE_AMOUNT, AudioEffectManager.DAP_DE_AMOUNT_DEFAULT);
                break;
            case AudioEffectManager.CMD_DAP_POST_GAIN:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_POST_GAIN, AudioEffectManager.DAP_POST_GAIN_DEFAULT);
                break;
            case AudioEffectManager.CMD_DAP_GEQ_ENABLE:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_GEQ_ENABLE, AudioEffectManager.DAP_GEQ_DEFAULT);
                break;
            case AudioEffectManager.SUBCMD_DAP_GEQ_BAND1:
                param = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_GEQ_ENABLE, AudioEffectManager.DAP_GEQ_DEFAULT);
                if (param == AudioEffectManager.DAP_GEQ_USER)
                    value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_GEQ_BAND1, AudioEffectManager.DAP_GEQ_GAIN_DEFAULT);
                else
                    value = getDapParamInternal(id);
                break;
            case AudioEffectManager.SUBCMD_DAP_GEQ_BAND2:
                param = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_GEQ_ENABLE, AudioEffectManager.DAP_GEQ_DEFAULT);
                if (param == AudioEffectManager.DAP_GEQ_USER)
                    value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_GEQ_BAND2, AudioEffectManager.DAP_GEQ_GAIN_DEFAULT);
                else
                    value = getDapParamInternal(id);
                break;
            case AudioEffectManager.SUBCMD_DAP_GEQ_BAND3:
                param = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_GEQ_ENABLE, AudioEffectManager.DAP_GEQ_DEFAULT);
                if (param == AudioEffectManager.DAP_GEQ_USER)
                    value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_GEQ_BAND3, AudioEffectManager.DAP_GEQ_GAIN_DEFAULT);
                else
                    value = getDapParamInternal(id);
                break;
            case AudioEffectManager.SUBCMD_DAP_GEQ_BAND4:
                param = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_GEQ_ENABLE, AudioEffectManager.DAP_GEQ_DEFAULT);
                if (param == AudioEffectManager.DAP_GEQ_USER)
                    value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_GEQ_BAND4, AudioEffectManager.DAP_GEQ_GAIN_DEFAULT);
                else
                    value = getDapParamInternal(id);
                break;
            case AudioEffectManager.SUBCMD_DAP_GEQ_BAND5:
                param = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_GEQ_ENABLE, AudioEffectManager.DAP_GEQ_DEFAULT);
                if (param == AudioEffectManager.DAP_GEQ_USER)
                    value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_GEQ_BAND5, AudioEffectManager.DAP_GEQ_GAIN_DEFAULT);
                else
                    value = getDapParamInternal(id);
                break;
            case AudioEffectManager.CMD_DAP_2_4_PROFILE:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_2_4_PROFILE, -1);
                if (value < 0) {
                    value = getDapParamInternal(AudioEffectManager.CMD_DAP_2_4_PROFILE);
                    Log.w(TAG, "getDapParam id:2_4_PROFILE hal value:" + value);
                    saveDbDap24Param(id, value);
                }
                break;
            case AudioEffectManager.CMD_DAP_2_4_MI_STEERING:
            case AudioEffectManager.CMD_DAP_2_4_SURROUND_DECODER_ENABLE:
            case AudioEffectManager.CMD_DAP_2_4_SURROUND_VIRTUALIZER:
            case AudioEffectManager.SUBCMD_DAP_2_4_SURROUND_VIRTUALIZER_BOOST:
            case AudioEffectManager.CMD_DAP_2_4_DIALOGUE_ENHANCER:
            case AudioEffectManager.SUBCMD_DAP_2_4_DIALOGUE_ENHANCER_AMOUNT:
            case AudioEffectManager.CMD_DAP_2_4_BASS_ENHANCER:
            case AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_BOOST:
            case AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_CUTOFFX100:
            case AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_CUTOFFX1:
            case AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_WIDTH:
            case AudioEffectManager.CMD_DAP_2_4_LEVELER:
            case AudioEffectManager.SUBCMD_DAP_2_4_LEVELER_AMOUNT:
                value = getDbDap24Param(id);
                Log.w(TAG, "getDbDap24Param id:" + id + ", value:" + value);
                break;
        }
        return value;
    }

    private int getDbIntValue(String id, int defaultValue) {
        return Settings.Global.getInt(mContext.getContentResolver(), id, defaultValue);
    }

    private void setDbIntValue(String id, int Value) {
        Settings.Global.putInt(mContext.getContentResolver(), id, Value);
    }

    private int getDbDap24Param(int id) {
        if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "getDbDap24Param id:" + id);
        int result = -1;
        switch (id) {
            case AudioEffectManager.CMD_DAP_2_4_PROFILE:
                result = getDbIntValue(DB_ID_SOUND_EFFECT_DAP_2_4_PROFILE, -1);
                break;
            case AudioEffectManager.CMD_DAP_2_4_SURROUND_VIRTUALIZER:
                result = getDbIntValue(DB_ID_SOUND_EFFECT_DAP_2_4_SURROUND_VIRTUALIZER_MODE, AudioEffectManager.DAP_2_4_SURROUND_VIRTUALIZER_DEFAULT);
                break;
            case AudioEffectManager.SUBCMD_DAP_2_4_SURROUND_VIRTUALIZER_BOOST:
                result = getDbIntValue(DB_ID_SOUND_EFFECT_DAP_2_4_SURROUND_VIRTUALIZER_BOOST, AudioEffectManager.DAP_2_4_SURROUND_VIRTUALIZER_BOOST_DEFAULT);
                break;
            case AudioEffectManager.CMD_DAP_2_4_DIALOGUE_ENHANCER:
                result = getDbIntValue(DB_ID_SOUND_EFFECT_DAP_2_4_DIALOGUE_ENHANCER_ENABLE, AudioEffectManager.DAP_2_4_DIALOGUE_ENHANCER_DEFAULT);
                break;
            case AudioEffectManager.SUBCMD_DAP_2_4_DIALOGUE_ENHANCER_AMOUNT:
                result = getDbIntValue(DB_ID_SOUND_EFFECT_DAP_2_4_DIALOGUE_ENHANCER_AMOUNT, AudioEffectManager.DAP_2_4_DIALOGUE_ENHANCER_AMOUNT_DEFAULT);
                break;
            case AudioEffectManager.CMD_DAP_2_4_BASS_ENHANCER:
                result = getDbIntValue(DB_ID_SOUND_EFFECT_DAP_2_4_BASS_ENHANCER_ENABLE, AudioEffectManager.DAP_2_4_BASS_ENHANCER_DEFAULT);
                break;
            case AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_BOOST:
                result = getDbIntValue(DB_ID_SOUND_EFFECT_DAP_2_4_BASS_ENHANCER_BOOST, AudioEffectManager.DAP_2_4_BASS_ENHANCER_BOOST_DEFAULT);
                break;
            case AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_CUTOFFX100:
                result = getDbIntValue(DB_ID_SOUND_EFFECT_DAP_2_4_BASS_ENHANCER_CUTOFFX100, AudioEffectManager.DAP_2_4_BASS_ENHANCER_CUTOFFX100_DEFAULT) * 100;
                break;
            case AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_CUTOFFX1:
                result = getDbIntValue(DB_ID_SOUND_EFFECT_DAP_2_4_BASS_ENHANCER_CUTOFFX1, AudioEffectManager.DAP_2_4_BASS_ENHANCER_CUTOFFX1_DEFAULT);
                break;
            case AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_WIDTH:
                result = getDbIntValue(DB_ID_SOUND_EFFECT_DAP_2_4_BASS_ENHANCER_WIDTH, AudioEffectManager.DAP_2_4_BASS_ENHANCER_WIDTH_DEFAULT);
                break;
            case AudioEffectManager.CMD_DAP_2_4_MI_STEERING:
                result = getDbIntValue(DB_ID_SOUND_EFFECT_DAP_2_4_MI_STEERING, AudioEffectManager.DAP_2_4_MI_STEERING_DEFAULT);
                break;
            case AudioEffectManager.CMD_DAP_2_4_SURROUND_DECODER_ENABLE:
                result = getDbIntValue(DB_ID_SOUND_EFFECT_DAP_2_4_SURROUND_DECODER_ENABLE, AudioEffectManager.DAP_2_4_SURROUND_DECODER_ENABLE_DEFAULT);
                break;
            case AudioEffectManager.CMD_DAP_2_4_LEVELER:
                result = getDbIntValue(DB_ID_SOUND_EFFECT_DAP_2_4_LEVELER_MODE, AudioEffectManager.DAP_2_4_LEVELER_DEFAULT);
                break;
            case AudioEffectManager.SUBCMD_DAP_2_4_LEVELER_AMOUNT:
                result = getDbIntValue(DB_ID_SOUND_EFFECT_DAP_2_4_LEVELER_AMOUNT, AudioEffectManager.DAP_2_4_LEVELER_AMOUNT_DEFAULT);
                break;
        }
        return result;
    }

    private void saveDbDap24Param(int id, int value) {
        if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "saveDbDap24Param id:" + id + ", value:" + value);
        switch (id) {
            case AudioEffectManager.CMD_DAP_2_4_PROFILE:
                setDbIntValue(DB_ID_SOUND_EFFECT_DAP_2_4_PROFILE, value);
                break;
            case AudioEffectManager.CMD_DAP_2_4_SURROUND_VIRTUALIZER:
                setDbIntValue(DB_ID_SOUND_EFFECT_DAP_2_4_SURROUND_VIRTUALIZER_MODE, value);
                break;
            case AudioEffectManager.SUBCMD_DAP_2_4_SURROUND_VIRTUALIZER_BOOST:
                setDbIntValue(DB_ID_SOUND_EFFECT_DAP_2_4_SURROUND_VIRTUALIZER_BOOST, value);
                break;
            case AudioEffectManager.CMD_DAP_2_4_DIALOGUE_ENHANCER:
                setDbIntValue(DB_ID_SOUND_EFFECT_DAP_2_4_DIALOGUE_ENHANCER_ENABLE, value);
                break;
            case AudioEffectManager.SUBCMD_DAP_2_4_DIALOGUE_ENHANCER_AMOUNT:
                setDbIntValue(DB_ID_SOUND_EFFECT_DAP_2_4_DIALOGUE_ENHANCER_AMOUNT, value);
                break;
            case AudioEffectManager.CMD_DAP_2_4_BASS_ENHANCER:
                setDbIntValue(DB_ID_SOUND_EFFECT_DAP_2_4_BASS_ENHANCER_ENABLE, value);
                break;
            case AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_BOOST:
                setDbIntValue(DB_ID_SOUND_EFFECT_DAP_2_4_BASS_ENHANCER_BOOST, value);
                break;
            case AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_CUTOFFX100:
                setDbIntValue(DB_ID_SOUND_EFFECT_DAP_2_4_BASS_ENHANCER_CUTOFFX100, value);
                break;
            case AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_CUTOFFX1:
                setDbIntValue(DB_ID_SOUND_EFFECT_DAP_2_4_BASS_ENHANCER_CUTOFFX1, value);
                break;
            case AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_WIDTH:
                setDbIntValue(DB_ID_SOUND_EFFECT_DAP_2_4_BASS_ENHANCER_WIDTH, value);
                break;
            case AudioEffectManager.CMD_DAP_2_4_MI_STEERING:
                setDbIntValue(DB_ID_SOUND_EFFECT_DAP_2_4_MI_STEERING, value);
                break;
            case AudioEffectManager.CMD_DAP_2_4_SURROUND_DECODER_ENABLE:
                setDbIntValue(DB_ID_SOUND_EFFECT_DAP_2_4_SURROUND_DECODER_ENABLE, value);
                break;
            case AudioEffectManager.CMD_DAP_2_4_LEVELER:
                setDbIntValue(DB_ID_SOUND_EFFECT_DAP_2_4_LEVELER_MODE, value);
                break;
            case AudioEffectManager.SUBCMD_DAP_2_4_LEVELER_AMOUNT:
                setDbIntValue(DB_ID_SOUND_EFFECT_DAP_2_4_LEVELER_AMOUNT, value);
                break;
            default:
                break;
        }
    }

}

