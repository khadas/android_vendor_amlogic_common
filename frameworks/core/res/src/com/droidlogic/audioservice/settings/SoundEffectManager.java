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
import com.droidlogic.app.AudioEffectManager;

public class SoundEffectManager {

    public static final String TAG = "SoundEffectManager";

    private static final UUID EFFECT_TYPE_BALANCE               = UUID.fromString("7cb34dc0-242e-11e6-bb63-0002a5d5c51b");
    private static final UUID EFFECT_TYPE_TREBLE_BASS           = UUID.fromString("7e282240-242e-11e6-bb63-0002a5d5c51b");
    private static final UUID EFFECT_TYPE_DAP_1_3_2             = UUID.fromString("3337b21d-c8e6-4bbd-8f24-698ade8491b9");
    private static final UUID EFFECT_TYPE_DAP_2_4               = UUID.fromString("34033483-c5e9-4ff6-8b6b-0002a5d5c51b");
    private static final UUID EFFECT_TYPE_DPE                   = UUID.fromString("f70bcbf4-7457-11ec-b4f0-020017000b7b");

    private static final UUID EFFECT_TYPE_EQ                    = UUID.fromString("ce2c14af-84df-4c36-acf5-87e428ed05fc");
    private static final UUID EFFECT_TYPE_VIRTUAL_SURROUND      = UUID.fromString("c656ec6f-d6be-4e7f-854b-1218077f3915");
    private static final UUID EFFECT_TYPE_VIRTUAL_X             = UUID.fromString("5112a99e-b8b9-4c5e-91fd-a804d29c36b2");

    private static final UUID EFFECT_UUID_VIRTUAL_X             = UUID.fromString("61821587-ce3c-4aac-9122-86d874ea1fb1");
    private static final UUID EFFECT_UUID_DBX                   = UUID.fromString("07210842-7432-4624-8b97-35ac8782efa3");

    // defined index ID for DB storage
    public static final String DB_ID_SOUND_EFFECT_BASS                          = "db_id_sound_effect_bass";
    public static final String DB_ID_SOUND_EFFECT_TREBLE                        = "db_id_sound_effect_treble";
    public static final String DB_ID_SOUND_EFFECT_BALANCE                       = "db_id_sound_effect_balance";
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
    public static final String DB_ID_SOUND_EFFECT_BAND6                         = "db_id_sound_effect_band6";
    public static final String DB_ID_SOUND_EFFECT_BAND7                         = "db_id_sound_effect_band7";
    public static final String DB_ID_SOUND_EFFECT_BAND8                         = "db_id_sound_effect_band8";
    public static final String DB_ID_SOUND_EFFECT_BAND9                         = "db_id_sound_effect_band9";
    public static final String DB_ID_SOUND_EFFECT_VIRTUALX_MODE                 = "db_id_sound_effect_virtualx_mode";
    public static final String DB_ID_SOUND_EFFECT_TREVOLUME_HD                  = "db_id_sound_effect_truvolume_hd";
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
    public static final String DB_ID_SOUND_EFFECT_DAP_2_4_PROFILE                       = "db_id_sound_effect_dap_2_4_profile";

    public static final String DB_ID_SOUND_EFFECT_DAP_2_4_SURROUND_VIRTUALIZER_MODE     = "db_id_sound_effect_dap_2_4_surround_virtualizer_mode";
    public static final String DB_ID_SOUND_EFFECT_DAP_2_4_SURROUND_VIRTUALIZER_BOOST    = "db_id_sound_effect_dap_2_4_surround_virtualizer_boost";
    public static final String DB_ID_SOUND_EFFECT_DAP_2_4_DIALOGUE_ENHANCER_ENABLE      = "db_id_sound_effect_dap_2_4_dialogue_enhancer_enable";
    public static final String DB_ID_SOUND_EFFECT_DAP_2_4_DIALOGUE_ENHANCER_AMOUNT      = "db_id_sound_effect_dap_2_4_dialogue_enhancer_amount";
    public static final String DB_ID_SOUND_EFFECT_DAP_2_4_BASS_ENHANCER_ENABLE          = "db_id_sound_effect_dap_2_4_bass_enhancer_enable";
    public static final String DB_ID_SOUND_EFFECT_DAP_2_4_BASS_ENHANCER_BOOST           = "db_id_sound_effect_dap_2_4_bass_enhancer_boost";
    public static final String DB_ID_SOUND_EFFECT_DAP_2_4_BASS_ENHANCER_CUTOFFX100      = "db_id_sound_effect_dap_2_4_bass_enhancer_cutoffX100";
    public static final String DB_ID_SOUND_EFFECT_DAP_2_4_BASS_ENHANCER_CUTOFFX1        = "db_id_sound_effect_dap_2_4_bass_enhancer_cutoffX1";
    public static final String DB_ID_SOUND_EFFECT_DAP_2_4_BASS_ENHANCER_WIDTH           = "db_id_sound_effect_dap_2_4_bass_enhancer_width";

    public static final String DB_ID_SOUND_EFFECT_DAP_2_4_MI_STEERING                   = "db_id_sound_effect_dap_2_4_mi_steering";
    public static final String DB_ID_SOUND_EFFECT_DAP_2_4_SURROUND_DECODER_ENABLE       = "db_id_sound_effect_dap_2_4_surround_decoder_enable";
    public static final String DB_ID_SOUND_EFFECT_DAP_2_4_LEVELER_MODE                  = "db_id_sound_effect_dap_2_4_leveler_mode";
    public static final String DB_ID_SOUND_EFFECT_DAP_2_4_LEVELER_AMOUNT                = "db_id_sound_effect_dap_2_4_leveler_strength";

    // defined index ID for DB DPE storage
    public static final String DB_ID_SOUND_EFFECT_DPE_ENABLED                            = "db_id_sound_effect_dpe_enabled";
    public static final String DB_ID_SOUND_EFFECT_DPE_SAVED                              = "db_id_sound_effect_dpe_saved";
    public static final String DB_ID_SOUND_EFFECT_DPE_INPUTGAIN                          = "db_id_sound_effect_dpe_inputgain";

    // defined index ID for DB DPE pre eq
    public static final String DB_ID_SOUND_EFFECT_DPE_PRE_EQ                             = "db_id_sound_effect_dpe_pre_eq";
    // defined index ID for DB DPE pre eq band 0
    public static final String DB_ID_SOUND_EFFECT_DPE_PRE_EQ_BAND0                       = "db_id_sound_effect_dpe_pre_eq_band0";
    public static final String DB_ID_SOUND_EFFECT_DPE_PRE_EQ_BAND0_CUTOFFFREQUENCY       = "db_id_sound_effect_dpe_pre_eq_band0_cutofffrequency";
    public static final String DB_ID_SOUND_EFFECT_DPE_PRE_EQ_BAND0_GAIN                  = "db_id_sound_effect_dpe_pre_eq_band0_gain";
    // defined index ID for DB DPE pre eq band 1
    public static final String DB_ID_SOUND_EFFECT_DPE_PRE_EQ_BAND1                       = "db_id_sound_effect_dpe_pre_eq_band1";
    public static final String DB_ID_SOUND_EFFECT_DPE_PRE_EQ_BAND1_CUTOFFFREQUENCY       = "db_id_sound_effect_dpe_pre_eq_band1_cutofffrequency";
    public static final String DB_ID_SOUND_EFFECT_DPE_PRE_EQ_BAND1_GAIN                  = "db_id_sound_effect_dpe_pre_eq_band1_gain";
    // defined index ID for DB DPE pre eq band 2
    public static final String DB_ID_SOUND_EFFECT_DPE_PRE_EQ_BAND2                       = "db_id_sound_effect_dpe_pre_eq_band2";
    public static final String DB_ID_SOUND_EFFECT_DPE_PRE_EQ_BAND2_CUTOFFFREQUENCY       = "db_id_sound_effect_dpe_pre_eq_band2_cutofffrequency";
    public static final String DB_ID_SOUND_EFFECT_DPE_PRE_EQ_BAND2_GAIN                  = "db_id_sound_effect_dpe_pre_eq_band2_gain";

    // defined index ID for DB DPE mbc
    public static final String DB_ID_SOUND_EFFECT_DPE_MBC                                = "db_id_sound_effect_dpe_mbc";
    // defined index ID for DB DPE mbc band 0
    public static final String DB_ID_SOUND_EFFECT_DPE_MBC_BAND0                          = "db_id_sound_effect_dpe_mbc_band0";
    public static final String DB_ID_SOUND_EFFECT_DPE_MBC_BAND0_CUTOFFFREQUENCY          = "db_id_sound_effect_dpe_mbc_band0_cutoffFrequency";
    public static final String DB_ID_SOUND_EFFECT_DPE_MBC_BAND0_ATTACKTIME               = "db_id_sound_effect_dpe_mbc_band0_attacktime";
    public static final String DB_ID_SOUND_EFFECT_DPE_MBC_BAND0_RELEASETIME              = "db_id_sound_effect_dpe_mbc_band0_releasetime";
    public static final String DB_ID_SOUND_EFFECT_DPE_MBC_BAND0_RATIO                    = "db_id_sound_effect_dpe_mbc_band0_ratio";
    public static final String DB_ID_SOUND_EFFECT_DPE_MBC_BAND0_THRESHOLD                = "db_id_sound_effect_dpe_mbc_band0_threshold";
    public static final String DB_ID_SOUND_EFFECT_DPE_MBC_BAND0_KNEEWIDTH                = "db_id_sound_effect_dpe_mbc_band0_kneewidth";
    public static final String DB_ID_SOUND_EFFECT_DPE_MBC_BAND0_NOISEGATE_THRESHOLD      = "db_id_sound_effect_dpe_mbc_band0_noisegate_threshold";
    public static final String DB_ID_SOUND_EFFECT_DPE_MBC_BAND0_EXPANDER_RATIO           = "db_id_sound_effect_dpe_mbc_band0_expander_ratio";
    public static final String DB_ID_SOUND_EFFECT_DPE_MBC_BAND0_PRE_GAIN                 = "db_id_sound_effect_dpe_mbc_band0_pre_gain";
    public static final String DB_ID_SOUND_EFFECT_DPE_MBC_BAND0_POST_GAIN                = "db_id_sound_effect_dpe_mbc_band0_post_gain";
    // defined index ID for DB DPE mbc band 0
    public static final String DB_ID_SOUND_EFFECT_DPE_MBC_BAND1                          = "db_id_sound_effect_dpe_mbc_band1";
    public static final String DB_ID_SOUND_EFFECT_DPE_MBC_BAND1_CUTOFFFREQUENCY          = "db_id_sound_effect_dpe_mbc_band1_cutoffFrequency";
    public static final String DB_ID_SOUND_EFFECT_DPE_MBC_BAND1_ATTACKTIME               = "db_id_sound_effect_dpe_mbc_band1_attacktime";
    public static final String DB_ID_SOUND_EFFECT_DPE_MBC_BAND1_RELEASETIME              = "db_id_sound_effect_dpe_mbc_band1_releasetime";
    public static final String DB_ID_SOUND_EFFECT_DPE_MBC_BAND1_RATIO                    = "db_id_sound_effect_dpe_mbc_band1_ratio";
    public static final String DB_ID_SOUND_EFFECT_DPE_MBC_BAND1_THRESHOLD                = "db_id_sound_effect_dpe_mbc_band1_threshold";
    public static final String DB_ID_SOUND_EFFECT_DPE_MBC_BAND1_KNEEWIDTH                = "db_id_sound_effect_dpe_mbc_band1_kneewidth";
    public static final String DB_ID_SOUND_EFFECT_DPE_MBC_BAND1_NOISEGATE_THRESHOLD      = "db_id_sound_effect_dpe_mbc_band1_noisegate_threshold";
    public static final String DB_ID_SOUND_EFFECT_DPE_MBC_BAND1_EXPANDER_RATIO           = "db_id_sound_effect_dpe_mbc_band1_expander_ratio";
    public static final String DB_ID_SOUND_EFFECT_DPE_MBC_BAND1_PRE_GAIN                 = "db_id_sound_effect_dpe_mbc_band1_pre_gain";
    public static final String DB_ID_SOUND_EFFECT_DPE_MBC_BAND1_POST_GAIN                = "db_id_sound_effect_dpe_mbc_band1_post_gain";
    // defined index ID for DB DPE mbc band 0
    public static final String DB_ID_SOUND_EFFECT_DPE_MBC_BAND2                          = "db_id_sound_effect_dpe_mbc_band2";
    public static final String DB_ID_SOUND_EFFECT_DPE_MBC_BAND2_CUTOFFFREQUENCY          = "db_id_sound_effect_dpe_mbc_band2_cutoffFrequency";
    public static final String DB_ID_SOUND_EFFECT_DPE_MBC_BAND2_ATTACKTIME               = "db_id_sound_effect_dpe_mbc_band2_attacktime";
    public static final String DB_ID_SOUND_EFFECT_DPE_MBC_BAND2_RELEASETIME              = "db_id_sound_effect_dpe_mbc_band2_releasetime";
    public static final String DB_ID_SOUND_EFFECT_DPE_MBC_BAND2_RATIO                    = "db_id_sound_effect_dpe_mbc_band2_ratio";
    public static final String DB_ID_SOUND_EFFECT_DPE_MBC_BAND2_THRESHOLD                = "db_id_sound_effect_dpe_mbc_band2_threshold";
    public static final String DB_ID_SOUND_EFFECT_DPE_MBC_BAND2_KNEEWIDTH                = "db_id_sound_effect_dpe_mbc_band2_kneewidth";
    public static final String DB_ID_SOUND_EFFECT_DPE_MBC_BAND2_NOISEGATE_THRESHOLD      = "db_id_sound_effect_dpe_mbc_band2_noisegate_threshold";
    public static final String DB_ID_SOUND_EFFECT_DPE_MBC_BAND2_EXPANDER_RATIO           = "db_id_sound_effect_dpe_mbc_band2_expander_ratio";
    public static final String DB_ID_SOUND_EFFECT_DPE_MBC_BAND2_PRE_GAIN                 = "db_id_sound_effect_dpe_mbc_band2_pre_gain";
    public static final String DB_ID_SOUND_EFFECT_DPE_MBC_BAND2_POST_GAIN                = "db_id_sound_effect_dpe_mbc_band2_post_gain";

    // defined index ID for DB DPE post eq
    public static final String DB_ID_SOUND_EFFECT_DPE_POST_EQ                            = "db_id_sound_effect_dpe_post_eq";
    // defined index ID for DB DPE post eq band 0
    public static final String DB_ID_SOUND_EFFECT_DPE_POST_EQ_BAND0                      = "db_id_sound_effect_dpe_post_eq_band0";
    public static final String DB_ID_SOUND_EFFECT_DPE_POST_EQ_BAND0_CUTOFFFREQUENCY      = "db_id_sound_effect_dpe_post_eq_band0_cutofffrequency";
    public static final String DB_ID_SOUND_EFFECT_DPE_POST_EQ_BAND0_GAIN                 = "db_id_sound_effect_dpe_post_eq_band0_gain";
    // defined index ID for DB DPE post eq band 1
    public static final String DB_ID_SOUND_EFFECT_DPE_POST_EQ_BAND1                      = "db_id_sound_effect_dpe_post_eq_band1";
    public static final String DB_ID_SOUND_EFFECT_DPE_POST_EQ_BAND1_CUTOFFFREQUENCY      = "db_id_sound_effect_dpe_post_eq_band1_cutofffrequency";
    public static final String DB_ID_SOUND_EFFECT_DPE_POST_EQ_BAND1_GAIN                 = "db_id_sound_effect_dpe_post_eq_band1_gain";
    // defined index ID for DB DPE post eq band 2
    public static final String DB_ID_SOUND_EFFECT_DPE_POST_EQ_BAND2                      = "db_id_sound_effect_dpe_post_eq_band2";
    public static final String DB_ID_SOUND_EFFECT_DPE_POST_EQ_BAND2_CUTOFFFREQUENCY      = "db_id_sound_effect_dpe_post_eq_band2_cutofffrequency";
    public static final String DB_ID_SOUND_EFFECT_DPE_POST_EQ_BAND2_GAIN                 = "db_id_sound_effect_dpe_post_eq_band2_gain";

    // defined index ID for DB DPE limiter
    public static final String DB_ID_SOUND_EFFECT_DPE_LIMITER                            = "db_id_sound_effect_dpe_limiter";
    public static final String DB_ID_SOUND_EFFECT_DPE_LIMITER_LINKGROUP                  = "db_id_sound_effect_dpe_limiter_linkgroup";
    public static final String DB_ID_SOUND_EFFECT_DPE_LIMITER_ATTACKTIME                 = "db_id_sound_effect_dpe_limiter_attacktime";
    public static final String DB_ID_SOUND_EFFECT_DPE_LIMITER_RELEASETIMR                = "db_id_sound_effect_dpe_limiter_releasetime";
    public static final String DB_ID_SOUND_EFFECT_DPE_LIMITER_RATIO                      = "db_id_sound_effect_dpe_limiter_ratio";
    public static final String DB_ID_SOUND_EFFECT_DPE_LIMITER_THRESHOLD                  = "db_id_sound_effect_dpe_limiter_threshold";
    public static final String DB_ID_SOUND_EFFECT_DPE_LIMITER_POST_GAIN                  = "db_id_sound_effect_dpe_limiter_post_gain";

    // defined index ID for DB AUDIO EFFECT DEBUG
    public static final String DB_ID_SOUND_EFFECT_HPEQ_DEBUG                             = "db_id_sound_effect_hpeq_debug";
    public static final String DB_ID_SOUND_EFFECT_BALANCE_DEBUG                          = "db_id_sound_effect_balance_debug";
    public static final String DB_ID_SOUND_EFFECT_TREBLEBASS_DEBUG                       = "db_id_sound_effect_treblebass_debug";
    public static final String DB_ID_SOUND_EFFECT_VIRTUAL_SURROUND_DEBUG                 = "db_id_sound_effect_virtual_surround_debug";
    public static final String DB_ID_SOUND_EFFECT_DPE_DEBUG                              = "db_id_sound_effect_dpe_debug";
    public static final String DB_ID_SOUND_EFFECT_VIRTUAL_X_DEBUG                        = "db_id_sound_effect_virtual_x_debug";
    public static final String DB_ID_SOUND_EFFECT_DAP_2_DEBUG                            = "db_id_sound_effect_dap_2_debug";
    public static final String DB_ID_SOUND_EFFECT_HPEQ_BAND_NUM_DEBUG                    = "db_id_sound_effect_hpeq_band_num_debug";

    //set id
    public static final int SET_BASS                                    = 0;
    public static final int SET_TREBLE                                  = 1;
    public static final int SET_BALANCE                                 = 2;
    public static final int SET_SOUND_MODE                              = 3;
    public static final int SET_EFFECT_BAND1                            = 4;
    public static final int SET_EFFECT_BAND2                            = 5;
    public static final int SET_EFFECT_BAND3                            = 6;
    public static final int SET_EFFECT_BAND4                            = 7;
    public static final int SET_EFFECT_BAND5                            = 8;
    public static final int SET_EFFECT_BAND6                            = 9;
    public static final int SET_EFFECT_BAND7                            = 10;
    public static final int SET_EFFECT_BAND8                            = 11;
    public static final int SET_EFFECT_BAND9                            = 12;
    public static final int SET_VIRTUAL_SURROUND                        = 13;
    public static final int SET_VIRTUALX_MODE                           = 14;
    public static final int SET_TRUVOLUME_HD_ENABLE                     = 15;

    //Balance level.  Parameter ID
    public static final int PARAM_BALANCE_LEVEL                         = 0;

    //Tone level.  Parameter ID for
    public static final int PARAM_BASS_LEVEL                            = 0;
    public static final int PARAM_TREBLE_LEVEL                          = 1;

    //dap AudioEffect, [ HPEQparams ] enumeration alignment in Hpeq.cpp
    public static final int PARAM_EQ_ENABLE                             = 0;
    public static final int PARAM_EQ_BAND_NUM                           = 1;
    public static final int PARAM_EQ_EFFECT                             = 2;
    public static final int PARAM_EQ_CUSTOM                             = 3;

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
    private static final int[] EFFECT_SOUND_MODE_USER_BAND              = {50, 50, 50, 50, 50, 50, 50, 50, 50};
    private static final int EFFECT_SOUND_TYPE_NUM = 6;

    // Virtual X effect param type
    private static final int PARAM_DTS_PARAM_MBHL_ENABLE_I32            = 0;
    private static final int PARAM_DTS_PARAM_TBHDX_ENABLE_I32           = 35;
    private static final int PARAM_DTS_PARAM_VX_ENABLE_I32              = 46;
    private static final int PARAM_DTS_PARAM_LOUDNESS_CONTROL_ENABLE_I32= 67;

    private static final int DAP_CPDP_OUTPUT_2_SPEAKER      = 7;
    private static final int DAP_CPDP_OUTPUT_2_HEADPHONE    = 6;

    // Prefix to append to audio preferences file
    private Context mContext;

    //sound effects
    private AudioEffect mVirtualX;
    private AudioEffect mBalance;
    private AudioEffect mTrebleBass;
    private AudioEffect mSoundMode;
    private AudioEffect mVirtualSurround;
    private AudioEffect mDap;
    private AudioEffect mDpe;

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
        creatVirtualSurroundAudioEffects();
        creatTrebleBassAudioEffects();
        if (!mSupportMs12Dap) {
            creatEqAudioEffects();
        }
        creatBalanceAudioEffects();

        creatDpeAudioEffect();
        mEffectInit = true;
    }

    public void cleanupAudioEffects() {
        if (mEffectInit) {
            Log.w(TAG, "cleanupAudioEffects effect not create, return.");
            return;
        }
        cleanupBalanceAudioEffects();
        cleanupDapAudioEffects();
        cleanupDpeAudioEffects();
        cleanupEqAudioEffects();
        cleanupTrebleBassAudioEffects();
        cleanupVirtualSurroundAudioEffects();
        cleanupVirtualXAudioEffects();
        mEffectInit = false;
    }

    public boolean cleanupVirtualXAudioEffects() {
        try {
            if (mVirtualX != null) {
                mVirtualX.setEnabled(false);
                mVirtualX.release();
                mVirtualX = null;
            }
            return true;
        } catch (RuntimeException e) {
            Log.e(TAG, "cleanup VirtualX effect fail", e);
            return false;
        }
    }

    public boolean cleanupBalanceAudioEffects() {
        try {
            if (mBalance != null) {
                mBalance.setEnabled(false);
                mBalance.release();
                mBalance = null;
                Log.e(TAG, "cleanup Balance effect successful");
            }
            return true;
        } catch (RuntimeException e) {
            Log.e(TAG, "cleanup Balance effect fail", e);
            return false;
        }
    }

    public boolean cleanupTrebleBassAudioEffects() {
        try {
            if (mTrebleBass != null) {
                mTrebleBass.setEnabled(false);
                mTrebleBass.release();
                mTrebleBass = null;
            }
            return true;
        } catch (RuntimeException e) {
            Log.e(TAG, "cleanup TrebleBass effect fail", e);
            return false;
        }
    }

    //HPEQ effect
    public boolean cleanupEqAudioEffects() {
        try {
            if (mSoundMode != null) {
                mSoundMode.setEnabled(false);
                mSoundMode.release();
                mSoundMode = null;
            }
            return true;
        } catch (RuntimeException e) {
            Log.e(TAG, "cleanup hpeq effect fail", e);
            return false;
        }
    }

    public boolean cleanupVirtualSurroundAudioEffects() {
        try {
            if (mVirtualSurround != null) {
                mVirtualSurround.setEnabled(false);
                mVirtualSurround.release();
                mVirtualSurround = null;
            }
            return true;
        } catch (RuntimeException e) {
            Log.e(TAG, "cleanup VirtualSurround effect fail", e);
            return false;
        }
    }

    public boolean cleanupDpeAudioEffects() {
        try {
            if (mDpe != null) {
                mDpe.setEnabled(false);
                mDpe.release();
                mDpe = null;
            }
            return true;
        } catch (RuntimeException e) {
            Log.e(TAG, "cleanup DPE effect fail", e);
            return false;
        }
    }
    public boolean cleanupDapAudioEffects() {
        try {
            if (mDap != null) {
                mDap.setEnabled(false);
                mDap.release();
                mDap = null;
            }
            return true;
        } catch (RuntimeException e) {
            Log.e(TAG, "cleanup DAP effect fail", e);
            return false;
        }
    }

    public boolean creatVirtualXAudioEffects() {
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

    public boolean creatBalanceAudioEffects() {
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

    public boolean creatTrebleBassAudioEffects() {
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

    public boolean creatEqAudioEffects() {
        try {
            if (mSoundMode == null) {
                if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "creatEqAudioEffects");
                mSoundMode = new AudioEffect(EFFECT_TYPE_EQ, AudioEffect.EFFECT_TYPE_NULL, 0, 0);
                int result = mSoundMode.setEnabled(true);
                if (result == AudioEffect.SUCCESS) {
                    if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "creatEqAudioEffects enable eq");
                    mSoundMode.setParameter(PARAM_EQ_ENABLE, 1); //TODO:define 1
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

    public boolean creatVirtualSurroundAudioEffects() {
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

    public boolean isSupportVirtualX() {
        return mSupportVirtualX;
    }

    public boolean isDtsVXValid() {
        File fl = new File("/vendor/lib/soundfx/libvx.so");
        if (fl.exists()) {
            return true;
        }

        File flv = new File("/vendor/lib/soundfx/libvxv4.so");
        return flv.exists();
    }

    public void setDtsVirtualXMode(int virtualXMode) {
        if (null == mVirtualX) {
            Log.e(TAG, "The VirtualX effect is not created, the mode cannot be setDtsVirtualXMode.");
            return;
        }
        if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "setDtsVirtualXMode = " + virtualXMode);
        switch (virtualXMode) {
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
                Log.w(TAG, "VirtualX effect mode invalid, mode:" + virtualXMode);
                return;
        }
        saveAudioParameters(SET_VIRTUALX_MODE, virtualXMode);
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
        if (mTrebleBass == null) {
            Log.e(TAG, "getTrebleStatus failed ! TrebleBass is not created");
            return 0;
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
        if (mTrebleBass == null) {
            Log.e(TAG, "getBassStatus failed ! TrebleBass is not created");
            return 0;
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
        if (mBalance == null) {
            Log.e(TAG, "getBalanceStatus failed ! Balance is not created");
            return 0;
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

    // 0 1 ~ off on
    public int getVirtualSurroundStatus() {
        int saveresult = -1;
        if (mVirtualSurround == null) {
            Log.e(TAG, "getVirtualSurroundStatus failed ! VirtualSurround is not created");
            return 0;
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

    public void setSoundModeByObserver (int mode, int bandSum) {
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
                setDifferentBandEffects(AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND1, getSavedAudioParameters(SET_EFFECT_BAND1), false, bandSum);
            }
            //need to set sound mode by observer listener
            //saveAudioParameters(SET_SOUND_MODE, mode);
        }
    }

    public void setUserSoundModeParam(int bandNumber, int value, int bandSum) {
        if (null == mSoundMode) {
            Log.e(TAG, "The EQ effect is not created, the mode cannot be setUserSoundModeParam.");
            return;
        }
        if (DroidLogicUtils.getAudioDebugEnable()) {
            Log.d(TAG, "setUserSoundModeParam bandNumber:" + bandNumber + ", value:" + value);
        }
        if (bandNumber > AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND9 || bandNumber < AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND1) {
            Log.e(TAG, "the EQ band number:" + bandNumber + " invalid, set failed");
            return;
        }
        if (bandSum != AudioEffectManager.HPEQ_5_BAND && bandSum != AudioEffectManager.HPEQ_7_BAND && bandSum != AudioEffectManager.HPEQ_9_BAND) {
            Log.e(TAG, "the EQ band sum:" + bandSum + " invalid, set failed");
            return;
        }
        setDifferentBandEffects(bandNumber, value, true, bandSum);
    }

    public int getUserSoundModeParam(int bandNumber) {
        if (bandNumber > AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND9 || bandNumber < AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND1) {
            Log.e(TAG, "the EQ band number:" + bandNumber + " invalid, get failed");
            return 0;
        }
        int value = 0;
        value = getSavedAudioParameters(bandNumber + SET_EFFECT_BAND1);
        if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "getUserSoundModeParam band number:" + bandNumber+ ", value:" + value);
        return value;
    }

    private void setDifferentBandEffects(int bandnum, int value, boolean needsave, int bandSum) {
        if (mSoundMode == null) {
            Log.e(TAG, "setDifferentBandEffects eq sound is not created");
            return;
        }
        int result = mSoundMode.setEnabled(true);
        if (result == AudioEffect.SUCCESS) {
            if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "setDifferentBandEffects: NO." + bandnum + " = " + value);
            byte[] needband = new byte[9];
            for (int i = AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND1; i <= AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND9; i++) {
                if (bandnum == i) {
                    needband[i] = (byte)MappingLine(value, true);
                } else {
                    needband[i] = (byte) MappingLine(getSavedAudioParameters(i + SET_EFFECT_BAND1), true);
                }
            }
            Log.i(TAG, "set eq custom effect band value: " + Arrays.toString(needband));
            mSoundMode.setParameter(PARAM_EQ_CUSTOM, needband);
            if (needsave) {
                saveAudioParameters(bandnum + SET_EFFECT_BAND1, value);
            }
        }
    }
    //convert -10~10 to 0~100 controlled by need or not
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

    //convert 0~100 to -10~10 controlled by need or not
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

    private void saveAudioParameters(int id, int value) {
        if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "saveAudioParameters id:" + id+ ", value:" + value);
        switch (id) {
            case SET_BASS:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BASS, value);
                break;
            case SET_TREBLE:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_TREBLE, value);
                break;
            case SET_BALANCE:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BALANCE, value);
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
            case SET_EFFECT_BAND6:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND6, value);
                break;
            case SET_EFFECT_BAND7:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND7, value);
                break;
            case SET_EFFECT_BAND8:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND8, value);
                break;
            case SET_EFFECT_BAND9:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND9, value);
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
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BASS, AudioEffectManager.EFFECT_BASS_DEFAULT);
                break;
            case SET_TREBLE:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_TREBLE, AudioEffectManager.EFFECT_TREBLE_DEFAULT);
                break;
            case SET_BALANCE:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BALANCE, AudioEffectManager.EFFECT_BALANCE_DEFAULT);
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
            case SET_EFFECT_BAND6:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND6, EFFECT_SOUND_MODE_USER_BAND[AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND6]);
                break;
            case SET_EFFECT_BAND7:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND7, EFFECT_SOUND_MODE_USER_BAND[AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND7]);
                break;
            case SET_EFFECT_BAND8:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND8, EFFECT_SOUND_MODE_USER_BAND[AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND8]);
                break;
            case SET_EFFECT_BAND9:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND9, EFFECT_SOUND_MODE_USER_BAND[AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND9]);
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
            default:
                break;
        }
        return result;
    }

    public void initEqAudioEffects() {
        int bandSum = getHpeqBandNum(AudioEffectManager.DEBUG_HPEQ_BAND_NUM_UI);
        setHpeqBandNum(AudioEffectManager.DEBUG_HPEQ_BAND_NUM_UI, bandSum);
        Log.d(TAG, " initEqAudioEffects bandSum :" + bandSum);
        if (Settings.Global.getInt(mContext.getContentResolver(), "set_eq_band", 0) == 0) {
            if (mSoundMode != null) {
                byte[] eqBandNum = new byte[9];
                mSoundMode.getParameter(PARAM_EQ_CUSTOM, eqBandNum);
                for (int i = SET_EFFECT_BAND1; i <= SET_EFFECT_BAND9; i++) {
                    saveAudioParameters(i, unMappingLine(eqBandNum[i - SET_EFFECT_BAND1], true));
                }
            } else {
                for (int i = SET_EFFECT_BAND1; i <= SET_EFFECT_BAND9; i++) {
                    saveAudioParameters(i, EFFECT_SOUND_MODE_USER_BAND[i - SET_EFFECT_BAND1]);
                }
                Log.w(TAG, "get default band value fail, set default value, mSoundMode == null");
            }
            Settings.Global.putInt(mContext.getContentResolver(), "set_eq_band", 1);
        }

        int soundMode = getSavedAudioParameters(SET_SOUND_MODE);
        setSoundModeByObserver(soundMode, bandSum);
    }

    public void initTrebleBassAudioEffects() {
        Log.d(TAG, "initTrebleBassAudioEffects...");
        setBass(getSavedAudioParameters(SET_BASS));
        setTreble(getSavedAudioParameters(SET_TREBLE));
    }

    public void initVirtualSurroundAudioEffects() {
        Log.d(TAG, "initVirtualSurroundAudioEffects...");
        setVirtualSurround(getSavedAudioParameters(SET_VIRTUAL_SURROUND));
    }

    public void initBalanceAudioEffects() {
        Log.d(TAG, "initBalanceAudioEffects...");
        setBalance(getSavedAudioParameters(SET_BALANCE));
    }

    public void initVirtualXAudioEffects() {
        Log.d(TAG, "initVirtualXAudioEffects...");
        setDtsVirtualXMode(getDtsVirtualXMode());
        setDtsTruVolumeHdEnable(getDtsTruVolumeHdEnable());
    }

    public void initSoundEffectSettings() {
        Log.d(TAG, "initSoundEffectSettings...");
        initEqAudioEffects();
        initTrebleBassAudioEffects();
        initVirtualSurroundAudioEffects();
        initBalanceAudioEffects();
        applyAudioEffectByPlayEmptyTrack();
        if (isSupportVirtualX()) {
            initVirtualXAudioEffects();
        }
    }

    public void resetSoundEffectSettings() {
        Log.d(TAG, "resetSoundEffectSettings");
        cleanupAudioEffects();
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BASS, AudioEffectManager.EFFECT_BASS_DEFAULT);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_TREBLE, AudioEffectManager.EFFECT_TREBLE_DEFAULT);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BALANCE, AudioEffectManager.EFFECT_BALANCE_DEFAULT);
        Settings.Global.putString(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_SOUND_MODE_TYPE, DB_ID_SOUND_EFFECT_SOUND_MODE_TYPE_EQ);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_SOUND_MODE, AudioEffectManager.EQ_SOUND_MODE_STANDARD);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_SOUND_MODE_DAP_VALUE, AudioEffectManager.EQ_SOUND_MODE_STANDARD);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_SOUND_MODE_EQ_VALUE, AudioEffectManager.EQ_SOUND_MODE_STANDARD);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND1, EFFECT_SOUND_MODE_USER_BAND[AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND1]);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND2, EFFECT_SOUND_MODE_USER_BAND[AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND2]);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND3, EFFECT_SOUND_MODE_USER_BAND[AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND3]);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND4, EFFECT_SOUND_MODE_USER_BAND[AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND4]);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND5, EFFECT_SOUND_MODE_USER_BAND[AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND5]);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND6, EFFECT_SOUND_MODE_USER_BAND[AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND6]);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND7, EFFECT_SOUND_MODE_USER_BAND[AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND7]);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND8, EFFECT_SOUND_MODE_USER_BAND[AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND8]);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND9, EFFECT_SOUND_MODE_USER_BAND[AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND9]);
        Settings.Global.putInt(mContext.getContentResolver(), "set_eq_band", 0);
        Settings.Global.putInt(mContext.getContentResolver(), OutputModeManager.VIRTUAL_SURROUND, OutputModeManager.VIRTUAL_SURROUND_OFF);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_VIRTUALX_MODE, AudioEffectManager.SOUND_EFFECT_VIRTUALX_MODE_DEFAULT);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_TREVOLUME_HD, AudioEffectManager.SOUND_EFFECT_TRUVOLUME_HD_ENABLE_DEFAULT);
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

    public boolean creatDapAudioEffect() {
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

           if (value == AudioEffectManager.SOUND_EFFECT_DAP_2_4_PROFILE_USER_SELECTABLE) {
               tempValue[0] = (byte)getDbDap24Param(AudioEffectManager.CMD_DAP_2_4_SURROUND_VIRTUALIZER);
               //Log.i(TAG, "SURROUND_VIRTUALIZER init value: " + tempValue[0]);
               tempValue[1] = (byte)getDbDap24Param(AudioEffectManager.SUBCMD_DAP_2_4_SURROUND_VIRTUALIZER_BOOST);
               //Log.i(TAG, "SURROUND_VIRTUALIZER_BOOST init value: " + tempValue[1]);
               mDap.setParameter(AudioEffectManager.CMD_DAP_2_4_SURROUND_VIRTUALIZER - AudioEffectManager.CMD_DAP_2_4_BASE_VALUE, tempValue);

               tempValue[0] = (byte)getDbDap24Param(AudioEffectManager.CMD_DAP_2_4_DIALOGUE_ENHANCER);
               //Log.i(TAG, "DIALOGUE_ENHANCER init value: " + tempValue[0]);
               tempValue[1] = (byte)getDbDap24Param(AudioEffectManager.SUBCMD_DAP_2_4_DIALOGUE_ENHANCER_AMOUNT);
               //Log.i(TAG, "DIALOGUE_ENHANCER_AMOUNT init value: " + tempValue[1]);
               mDap.setParameter(AudioEffectManager.CMD_DAP_2_4_DIALOGUE_ENHANCER - AudioEffectManager.CMD_DAP_2_4_BASE_VALUE, tempValue);

               tempValue2[0] = (byte)getDbDap24Param(AudioEffectManager.CMD_DAP_2_4_BASS_ENHANCER);
               //Log.i(TAG, "BASS_ENHANCER init value: " + tempValue2[0]);
               int tempInt = getDbDap24Param(AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_BOOST);
               byte[] enbo = mDap.intToByteArray(tempInt);
               tempValue2[1] = enbo[0];
               tempValue2[2] = enbo[1];
               int enboInt = mDap.byteArrayToInt(enbo);
               //Log.i(TAG, "BASS_ENHANCER_BOOST init value: " + enboInt);
               int tempInt2 = (getDbDap24Param(AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_CUTOFF_100)
                + getDbDap24Param(AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_CUTOFF_1));
               byte[] encu = mDap.intToByteArray(tempInt2);
               tempValue2[3] = encu[0];
               tempValue2[4] = encu[1];
               int encuInt = mDap.byteArrayToInt(encu);
               //Log.i(TAG, "BASS_ENHANCER_CUTOFF init value: " + encuInt);
               tempValue2[5] = (byte)getDbDap24Param(AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_WIDTH);
               //Log.i(TAG, "BASS_ENHANCER_WIDTH init value: " + tempValue2[5]);
               mDap.setParameter(AudioEffectManager.CMD_DAP_2_4_BASS_ENHANCER - AudioEffectManager.CMD_DAP_2_4_BASE_VALUE, tempValue2);

               value = getDbDap24Param(AudioEffectManager.CMD_DAP_2_4_MI_STEERING);
               //Log.i(TAG, "MI_STEERING init value: " + value);
               mDap.setParameter(AudioEffectManager.CMD_DAP_2_4_MI_STEERING - AudioEffectManager.CMD_DAP_2_4_BASE_VALUE, value);

               value = getDbDap24Param(AudioEffectManager.CMD_DAP_2_4_SURROUND_DECODER_ENABLE);
               //Log.i(TAG, "SURROUND_DECODER_ENABLE init value: " + value);
               mDap.setParameter(AudioEffectManager.CMD_DAP_2_4_SURROUND_DECODER_ENABLE - AudioEffectManager.CMD_DAP_2_4_BASE_VALUE, value);

               tempValue[0] = (byte)getDbDap24Param(AudioEffectManager.CMD_DAP_2_4_LEVELER);
               //Log.i(TAG, "LEVELER init value: " + tempValue[0]);
               tempValue[1] = (byte)getDbDap24Param(AudioEffectManager.SUBCMD_DAP_2_4_LEVELER_AMOUNT);
               //Log.i(TAG, "LEVELER_AMOUNT init value: " + tempValue[1]);
               mDap.setParameter(AudioEffectManager.CMD_DAP_2_4_LEVELER - AudioEffectManager.CMD_DAP_2_4_BASE_VALUE, tempValue);
           }
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
                /*
                if (value == AudioEffectManager.DAP_SURROUND_SPEAKER)
                    mDap.setParameter(id, DAP_CPDP_OUTPUT_2_SPEAKER);
                else if (value == AudioEffectManager.DAP_SURROUND_HEADPHONE)
                    mDap.setParameter(id, DAP_CPDP_OUTPUT_2_HEADPHONE);
                */
                mDap.setParameter(id, value);
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
                byte[] tempValueHal = new byte[2];
                byte[] tempValueHal2 = new byte[6];
                mDap.setParameter(id - AudioEffectManager.CMD_DAP_2_4_BASE_VALUE, value);
                Log.i(TAG, "PROFILE  value: " + value);
                int valueHal = value;
                if (valueHal == AudioEffectManager.SOUND_EFFECT_DAP_2_4_PROFILE_USER_SELECTABLE) {
                    tempValueHal[0] = (byte)getDbDap24Param(AudioEffectManager.CMD_DAP_2_4_SURROUND_VIRTUALIZER);
                    tempValueHal[1] = (byte)getDbDap24Param(AudioEffectManager.SUBCMD_DAP_2_4_SURROUND_VIRTUALIZER_BOOST);
                    mDap.setParameter(AudioEffectManager.CMD_DAP_2_4_SURROUND_VIRTUALIZER - AudioEffectManager.CMD_DAP_2_4_BASE_VALUE, tempValueHal);

                    tempValueHal[0] = (byte)getDbDap24Param(AudioEffectManager.CMD_DAP_2_4_DIALOGUE_ENHANCER);
                    tempValueHal[1] = (byte)getDbDap24Param(AudioEffectManager.SUBCMD_DAP_2_4_DIALOGUE_ENHANCER_AMOUNT);
                    mDap.setParameter(AudioEffectManager.CMD_DAP_2_4_DIALOGUE_ENHANCER - AudioEffectManager.CMD_DAP_2_4_BASE_VALUE, tempValueHal);

                    tempValueHal2[0] = (byte)getDbDap24Param(AudioEffectManager.CMD_DAP_2_4_BASS_ENHANCER);
                    int tempIntHal = getDbDap24Param(AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_BOOST);
                    byte[] enboHal = mDap.intToByteArray(tempIntHal);
                    tempValueHal2[1] = enboHal[0];
                    tempValueHal2[2] = enboHal[1];
                    int enboIntHal = mDap.byteArrayToInt(enboHal);
                    int tempIntHal2 = (getDbDap24Param(AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_CUTOFF_100)
                     + getDbDap24Param(AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_CUTOFF_1));
                    byte[] encuHal = mDap.intToByteArray(tempIntHal2);
                    tempValueHal2[3] = encuHal[0];
                    tempValueHal2[4] = encuHal[1];
                    int encuIntHal = mDap.byteArrayToInt(encuHal);
                    tempValueHal2[5] = (byte)getDbDap24Param(AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_WIDTH);
                    mDap.setParameter(AudioEffectManager.CMD_DAP_2_4_BASS_ENHANCER - AudioEffectManager.CMD_DAP_2_4_BASE_VALUE, tempValueHal2);

                    valueHal = getDbDap24Param(AudioEffectManager.CMD_DAP_2_4_MI_STEERING);
                    mDap.setParameter(AudioEffectManager.CMD_DAP_2_4_MI_STEERING - AudioEffectManager.CMD_DAP_2_4_BASE_VALUE, valueHal);

                    valueHal = getDbDap24Param(AudioEffectManager.CMD_DAP_2_4_SURROUND_DECODER_ENABLE);
                    mDap.setParameter(AudioEffectManager.CMD_DAP_2_4_SURROUND_DECODER_ENABLE - AudioEffectManager.CMD_DAP_2_4_BASE_VALUE, valueHal);

                    tempValueHal[0] = (byte)getDbDap24Param(AudioEffectManager.CMD_DAP_2_4_LEVELER);
                    tempValueHal[1] = (byte)getDbDap24Param(AudioEffectManager.SUBCMD_DAP_2_4_LEVELER_AMOUNT);
                    mDap.setParameter(AudioEffectManager.CMD_DAP_2_4_LEVELER - AudioEffectManager.CMD_DAP_2_4_BASE_VALUE, tempValueHal);
                }
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
                if (tempValue[0] == 1 || tempValue[0] == 2) {
                    mDap.setParameter(AudioEffectManager.CMD_DAP_2_4_SURROUND_DECODER_ENABLE - AudioEffectManager.CMD_DAP_2_4_BASE_VALUE, 1);
                } else {
                    mDap.setParameter(AudioEffectManager.CMD_DAP_2_4_SURROUND_DECODER_ENABLE - AudioEffectManager.CMD_DAP_2_4_BASE_VALUE, 0);
                }
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
            case AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_CUTOFF_100:
            case AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_CUTOFF_1:
            case AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_WIDTH:
                byte[] tempValue_2 = new byte[6];
                tempValue_2[0] = (byte)getDapParam(AudioEffectManager.CMD_DAP_2_4_BASS_ENHANCER);
                int tempInt = getDapParam(AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_BOOST);
                byte[] enbo = mDap.intToByteArray(tempInt);
                tempValue_2[1] = enbo[0];
                tempValue_2[2] = enbo[1];
                int enboInt = mDap.byteArrayToInt(enbo);
                int tempInt2 = (getDapParam(AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_CUTOFF_100)
                    + getDapParam(AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_CUTOFF_1));
                byte[] encu = mDap.intToByteArray(tempInt2);
                tempValue_2[3] = encu[0];
                tempValue_2[4] = encu[1];
                int encuInt = mDap.byteArrayToInt(encu);
                tempValue_2[5] = (byte)getDapParam(AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_WIDTH);
                //Log.d(TAG, "BASS_ENHANCER getDapParam enable:" + tempValue_2[0] + ", boost:" + enboInt + ", cutoff:" + encuInt + ", width:" + tempValue_2[5]);
                if (id < AudioEffectManager.SUBCMD_DAP_2_4_BASE_VALUE) {
                    tempValue_2[0] = (byte)value;
                } else if (id == AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_BOOST) {
                    enbo = mDap.intToByteArray(value);
                    tempValue_2[1] = enbo[0];
                    tempValue_2[2] = enbo[1];
                } else if (id == AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_CUTOFF_100) {
                    encu = mDap.intToByteArray(value * 100 +
                        getDapParam(AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_CUTOFF_1));
                    tempValue_2[3] = encu[0];
                    tempValue_2[4] = encu[1];
                } else if (id == AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_CUTOFF_1) {
                    encu = mDap.intToByteArray(value  +
                        getDapParam(AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_CUTOFF_100));
                    tempValue_2[3] = encu[0];
                    tempValue_2[4] = encu[1];
                }
                else {
                    tempValue_2[5] = (byte)value;
                }
                int enbo2Int = mDap.byteArrayToInt(enbo);
                int encu2Int = mDap.byteArrayToInt(encu);
                mDap.setParameter(AudioEffectManager.CMD_DAP_2_4_BASS_ENHANCER - AudioEffectManager.CMD_DAP_2_4_BASE_VALUE, tempValue_2);
                //Log.d(TAG, "BASS_ENHANCER setParameter enable:" + tempValue_2[0] + ", boost:" + enbo2Int + ", cutoff:" + encu2Int + ", width:" + tempValue_2[5]);
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
                    //Log.w(TAG, "getDapParam id:2_4_PROFILE hal value:" + value);
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
            case AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_CUTOFF_100:
            case AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_CUTOFF_1:
            case AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_WIDTH:
            case AudioEffectManager.CMD_DAP_2_4_LEVELER:
            case AudioEffectManager.SUBCMD_DAP_2_4_LEVELER_AMOUNT:
                value = getDbDap24Param(id);
                //Log.w(TAG, "getDbDap24Param id:" + id + ", value:" + value);
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
            case AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_CUTOFF_100:
                result = getDbIntValue(DB_ID_SOUND_EFFECT_DAP_2_4_BASS_ENHANCER_CUTOFFX100, AudioEffectManager.DAP_2_4_BASS_ENHANCER_CUTOFFX100_DEFAULT) * 100;
                break;
            case AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_CUTOFF_1:
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
            case AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_CUTOFF_100:
                setDbIntValue(DB_ID_SOUND_EFFECT_DAP_2_4_BASS_ENHANCER_CUTOFFX100, value);
                break;
            case AudioEffectManager.SUBCMD_DAP_2_4_BASS_ENHANCER_CUTOFF_1:
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


    public boolean creatDpeAudioEffect() {
        try {
            if (mDpe == null) {
                if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "creatDpeAudioEffect");
                mDpe = new AudioEffect(EFFECT_TYPE_DPE, AudioEffect.EFFECT_TYPE_NULL, 0, 0);
                int result = mDpe.setEnabled(true);
                if (result == AudioEffect.SUCCESS) {
                    Log.d(TAG, "creatDpeAudioEffect setEnabled success");
                } else {
                    Log.w(TAG, "creatDpeAudioEffect setEnabled error: " + result);
                    return false;
                }
            }
            return true;
        } catch (RuntimeException e) {
            Log.e(TAG, "Unable to create Dpe audio effect", e);
            return false;
        }
    }


    //dpe init
    public void initDpeAudioEffect() {
        Log.i(TAG, "initDpeAudioEffect");
        if (!creatDpeAudioEffect()) {
            Log.e(TAG, "initDpeAudioEffect dpe create fail");
            return;
        }

        int value = 0;

        value = getDpeParam(AudioEffectManager.CMD_DPE_ENABLED);
        setDpeParam(AudioEffectManager.CMD_DPE_ENABLED, getDpeParam(AudioEffectManager.CMD_DPE_ENABLED));
        //Log.d(TAG, "setDpeParam,CMD_DPE_ENABLED:" + value);

        if (value == AudioEffectManager.DPE_ON) {
            value = getDpeParam(AudioEffectManager.CMD_DPE_INPUTGAIN);
            setDpeParam(AudioEffectManager.CMD_DPE_INPUTGAIN, getDpeParam(AudioEffectManager.CMD_DPE_INPUTGAIN));
            //Log.d(TAG, "setDpeParam,CMD_DPE_INPUTGAIN:" + value);

            value = getDpeParam(AudioEffectManager.CMD_DPE_PRE_EQ);
            setDpeParam(AudioEffectManager.CMD_DPE_PRE_EQ, getDpeParam(AudioEffectManager.CMD_DPE_PRE_EQ));
            //Log.d(TAG, "setDpeParam,CMD_DPE_PRE_EQ:" + value);

            value = getDpeParam(AudioEffectManager.CMD_DPE_MBC);
            setDpeParam(AudioEffectManager.CMD_DPE_MBC, getDpeParam(AudioEffectManager.CMD_DPE_MBC));
            //Log.d(TAG, "setDpeParam,CMD_DPE_MBC:" + value);

            value = getDpeParam(AudioEffectManager.CMD_DPE_POST_EQ);
            setDpeParam(AudioEffectManager.CMD_DPE_POST_EQ, getDpeParam(AudioEffectManager.CMD_DPE_POST_EQ));
            //Log.d(TAG, "setDpeParam,CMD_DPE_POST_EQ:" + value);

            value = getDpeParam(AudioEffectManager.CMD_DPE_LIMITER);
            setDpeParam(AudioEffectManager.CMD_DPE_LIMITER, getDpeParam(AudioEffectManager.CMD_DPE_LIMITER));
            //Log.d(TAG, "setDpeParam,CMD_DPE_LIMITER:" + value);

            //pre eq band
            value = getDpeParam(AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND0_CUTOFFFREQUENCY);
            setDpeParam(AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND0_CUTOFFFREQUENCY, getDpeParam(AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND0_CUTOFFFREQUENCY));
            //Log.d(TAG, "setDpeParam,SUBCMD_DPE_PRE_EQ_BAND0_CUTOFFFREQUENCY:" + value);

            setDpeParam(AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND0_GAIN, getDpeParam(AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND0_GAIN));
            setDpeParam(AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND1_CUTOFFFREQUENCY, getDpeParam(AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND1_CUTOFFFREQUENCY));
            setDpeParam(AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND1_GAIN, getDpeParam(AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND1_GAIN));
            setDpeParam(AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND2_CUTOFFFREQUENCY, getDpeParam(AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND2_CUTOFFFREQUENCY));
            setDpeParam(AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND2_GAIN, getDpeParam(AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND2_GAIN));

            //post eq band
            setDpeParam(AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND0_CUTOFFFREQUENCY, getDpeParam(AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND0_CUTOFFFREQUENCY));
            setDpeParam(AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND0_GAIN, getDpeParam(AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND0_GAIN));
            setDpeParam(AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND1_CUTOFFFREQUENCY, getDpeParam(AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND1_CUTOFFFREQUENCY));
            setDpeParam(AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND1_GAIN, getDpeParam(AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND1_GAIN));
            setDpeParam(AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND2_CUTOFFFREQUENCY, getDpeParam(AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND2_CUTOFFFREQUENCY));
            setDpeParam(AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND2_GAIN, getDpeParam(AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND2_GAIN));

            int id = 0;
            //mbc band
            for (id = AudioEffectManager.SUBCMD_DPE_MBC_BAND0_CUTOFFFREQUENCY; id <= AudioEffectManager.SUBCMD_DPE_MBC_BAND0_POSTGAIN; id++) {
                setDpeParam(id, getDpeParam(id));
            }
            for (id = AudioEffectManager.SUBCMD_DPE_MBC_BAND1_CUTOFFFREQUENCY; id <= AudioEffectManager.SUBCMD_DPE_MBC_BAND1_POSTGAIN; id++) {
                setDpeParam(id, getDpeParam(id));
            }
            for (id = AudioEffectManager.SUBCMD_DPE_MBC_BAND2_CUTOFFFREQUENCY; id <= AudioEffectManager.SUBCMD_DPE_MBC_BAND2_POSTGAIN; id++) {
                setDpeParam(id, getDpeParam(id));
            }

            //limiter
            for (id = AudioEffectManager.SUBCMD_DPE_LIMITER_ATTACKTIME; id <= AudioEffectManager.SUBCMD_DPE_LIMITER_POSTGAIN; id++) {
                setDpeParam(id, getDpeParam(id));
            }
        }

        applyAudioEffectByPlayEmptyTrack();
    }


    //dpe get param internal
    private int getDpeParamInternal (int id) {
        if (mDpe == null) {
            Log.d(TAG, "getDpeParamInternal DPE Effect is not created");
            return 0;
        }
        int result = 0;
        int[] value = new int[1];
        switch (id) {
            //dpe enabled
            case AudioEffectManager.CMD_DPE_ENABLED:
                byte[] tempEngineVa = new byte[4];
                byte[] tempEngineValue = new byte[36];

                List<byte[]> listEngine = new ArrayList<>();
                listEngine.add(tempEngineVa);
                listEngine.add(tempEngineValue);

                byte[] engineBytes = mergeByte(listEngine);
                byte[] tempEngineParam = new byte[] {(byte)id,0,0,0};
                mDpe.getParameter(tempEngineParam, engineBytes);
                result = mDpe.byteArrayToInt(tempEngineVa);
                break;

            //dpe inputgain
            case AudioEffectManager.CMD_DPE_INPUTGAIN:
                byte[] tempInputValue = new byte[4];
                byte[] tempParam_input = new byte[] {(byte)id, 0, 0, 0, 0, 0, 0, 0};  //channel 0
                mDpe.getParameter(tempParam_input, tempInputValue);
                result = (int)mDpe.byteArrayToFloat(tempInputValue);
                Log.d(TAG, "Inputgain hal Value: " + result);
                break;

            // pre eq band 0
            case AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND0:
            case AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND0_CUTOFFFREQUENCY:
            case AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND0_GAIN:
                byte[] tempParamPreEq0Cmd = new byte[4];
                byte[] tempParamPreEq0Ch = new byte[4];
                byte[] tempParamPreEq0Band = new byte[4];
                tempParamPreEq0Cmd = mDpe.intToByteArray(AudioEffectManager.CMD_DPE_PRE_EQ_BAND);
                tempParamPreEq0Ch = mDpe.intToByteArray(0); //channel 0
                tempParamPreEq0Band = mDpe.intToByteArray(0); // band 0

                List<byte[]> listParamPreEq0 = new ArrayList<>();
                listParamPreEq0.add(tempParamPreEq0Cmd);
                listParamPreEq0.add(tempParamPreEq0Ch);
                listParamPreEq0.add(tempParamPreEq0Band);
                byte[] tempParam_preEq0 = mergeByte(listParamPreEq0);

                byte[] tempValuePreEq0En = new byte[4];
                byte[] tempValuePreEq0Cut = new byte[4];
                byte[] tempValuePreEq0Gain = new byte[4];

                List<byte[]> listValuePreEq0 = new ArrayList<>();
                listValuePreEq0.add(tempValuePreEq0En);
                listValuePreEq0.add(tempValuePreEq0Cut);
                listValuePreEq0.add(tempValuePreEq0Gain);
                byte[] tempValue_preEq0 = mergeByte(listValuePreEq0);

                mDpe.getParameter(tempParam_preEq0, tempValue_preEq0);

                //get
                if (id == AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND0_CUTOFFFREQUENCY) {
                    result = (int)mDpe.byteArrayToFloat(tempValuePreEq0Cut);
                } else if (id == AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND0_GAIN) {
                    result = (int)mDpe.byteArrayToFloat(tempValuePreEq0Gain);
                } else if (id == AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND0) {
                    result = (int)mDpe.byteArrayToFloat(tempValuePreEq0En);
                }
                break;

        }
        return result;
    }

    // list merge byte
    public static byte[] mergeByte(List<byte[]> values) {
        int lengthByte = 0;
        for (byte[] value : values) {
            lengthByte += value.length;
        }
        byte[] allBytes = new byte[lengthByte];
        int countLength = 0;
        for (byte[] b : values) {
            System.arraycopy(b, 0, allBytes, countLength, b.length);
            countLength += b.length;
        }
        return allBytes;
    }


    //dpe set param
    public void setDpeParam (int id, int value) {
        if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "setDpeParam id:" + id + ", value:" + value);
        if (mDpe == null) {
            Log.d(TAG, "setDpeParam DPE Effect is not created");
            return;
        }
        switch (id) {
            //set dpe enabled
            case AudioEffectManager.CMD_DPE_ENABLED:
                byte[] tempEngineVa = new byte[4];
                byte[] tempEnginePref = new byte[4];
                byte[] tempEnginePreEqIu = new byte[4];
                byte[] tempEnginePreEqBc = new byte[4];
                byte[] tempEngineMbcIu = new byte[4];
                byte[] tempEngineMbcBc = new byte[4];
                byte[] tempEnginePostEqIu = new byte[4];
                byte[] tempEnginePostEqBc = new byte[4];
                byte[] tempEngineLimiterIu = new byte[4];

                tempEngineVa = mDpe.intToByteArray(value - 1);
                tempEnginePref = mDpe.floatToByteArray(AudioEffectManager.DEFAULT_DPE_FRAME_DURATION);
                tempEnginePreEqIu = mDpe.intToByteArray(value);
                tempEnginePreEqBc = mDpe.intToByteArray(AudioEffectManager.DEFAULT_DPE_BAND_AMOUNT);
                tempEngineMbcIu = mDpe.intToByteArray(value);
                tempEngineMbcBc = mDpe.intToByteArray(AudioEffectManager.DEFAULT_DPE_BAND_AMOUNT);
                tempEnginePostEqIu = mDpe.intToByteArray(value);
                tempEnginePostEqBc = mDpe.intToByteArray(AudioEffectManager.DEFAULT_DPE_BAND_AMOUNT);
                tempEngineLimiterIu = mDpe.intToByteArray(value);

                List<byte[]> listEngine = new ArrayList<>();
                listEngine.add(tempEngineVa);
                listEngine.add(tempEnginePref);
                listEngine.add(tempEnginePreEqIu);
                listEngine.add(tempEnginePreEqBc);
                listEngine.add(tempEngineMbcIu);
                listEngine.add(tempEngineMbcBc);
                listEngine.add(tempEnginePostEqIu);
                listEngine.add(tempEnginePostEqBc);
                listEngine.add(tempEngineLimiterIu);

                byte[] engineBytes = mergeByte(listEngine);
                byte[] tempEngineParam = new byte[] {(byte)id,0,0,0};
                mDpe.setParameter(tempEngineParam, engineBytes);
                saveDpeParam(id, value);
                break;

            // set dpe inputgain
            case AudioEffectManager.CMD_DPE_INPUTGAIN:
                byte[] tempInputValue = new byte[4];
                tempInputValue = mDpe.floatToByteArray((float)value);
                byte[] tempParam_input = new byte[] {(byte)id, 0, 0, 0, 0, 0, 0, 0};  //channel 0
                mDpe.setParameter(tempParam_input, tempInputValue);
                tempParam_input[4] = 1;  //channel 1
                mDpe.setParameter(tempParam_input, tempInputValue);
                saveDpeParam(id, value);
                break;

            // set eq mbc inuse
            case AudioEffectManager.CMD_DPE_PRE_EQ:
            case AudioEffectManager.CMD_DPE_MBC:
            case AudioEffectManager.CMD_DPE_POST_EQ:
                byte[] tempParam_on = new byte[] {(byte)id, 0, 0, 0, 0, 0, 0, 0}; //channel 0
                if (value == 1) {
                    byte[] tempValue_on = new byte[] {1, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0};
                    mDpe.setParameter(tempParam_on, tempValue_on);
                    tempParam_on[4] = 1; //channel 1
                    mDpe.setParameter(tempParam_on, tempValue_on);
                } else {
                    byte[] tempValue_on = new byte[] {0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0};
                    mDpe.setParameter(tempParam_on, tempValue_on);
                    tempParam_on[4] = 1; //channel 1
                    mDpe.setParameter(tempParam_on, tempValue_on);
                }
                saveDpeParam(id, value);
                break;

            // pre eq band 0
            case AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND0:
            case AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND0_CUTOFFFREQUENCY:
            case AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND0_GAIN:
                byte[] tempParamPreEq0Cmd = new byte[4];
                byte[] tempParamPreEq0Ch = new byte[4];
                byte[] tempParamPreEq0Band = new byte[4];
                tempParamPreEq0Cmd = mDpe.intToByteArray(AudioEffectManager.CMD_DPE_PRE_EQ_BAND);
                tempParamPreEq0Ch = mDpe.intToByteArray(0); //channel 0
                tempParamPreEq0Band = mDpe.intToByteArray(0); // band 0

                List<byte[]> listParamPreEq0 = new ArrayList<>();
                listParamPreEq0.add(tempParamPreEq0Cmd);
                listParamPreEq0.add(tempParamPreEq0Ch);
                listParamPreEq0.add(tempParamPreEq0Band);
                byte[] tempParam_preEq0 = mergeByte(listParamPreEq0);

                byte[] tempValuePreEq0En = new byte[4];
                byte[] tempValuePreEq0Cut = new byte[4];
                byte[] tempValuePreEq0Gain = new byte[4];

                tempValuePreEq0En = mDpe.intToByteArray(getDpeParam(AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND0));
                tempValuePreEq0Cut = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND0_CUTOFFFREQUENCY));
                tempValuePreEq0Gain = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND0_GAIN));

                //set
                List<byte[]> listValuePreEq0 = new ArrayList<>();
                if (id == AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND0_CUTOFFFREQUENCY) {
                    tempValuePreEq0Cut = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND0_GAIN) {
                    tempValuePreEq0Gain = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND0) {
                    tempValuePreEq0En = mDpe.intToByteArray(value);
                }

                listValuePreEq0.add(tempValuePreEq0En);
                listValuePreEq0.add(tempValuePreEq0Cut);
                listValuePreEq0.add(tempValuePreEq0Gain);
                byte[] tempValue_preEq0 = mergeByte(listValuePreEq0);

                mDpe.setParameter(tempParam_preEq0, tempValue_preEq0);
                tempParam_preEq0[4] = 1; //channel 1
                mDpe.setParameter(tempParam_preEq0, tempValue_preEq0);
                saveDpeParam(id, value);
                break;

            // pre eq band 1
            case AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND1:
            case AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND1_CUTOFFFREQUENCY:
            case AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND1_GAIN:
                byte[] tempParamPreEq1Cmd = new byte[4];
                byte[] tempParamPreEq1Ch = new byte[4];
                byte[] tempParamPreEq1Band = new byte[4];
                tempParamPreEq1Cmd = mDpe.intToByteArray(AudioEffectManager.CMD_DPE_PRE_EQ_BAND);
                tempParamPreEq1Ch = mDpe.intToByteArray(0); //channel 0
                tempParamPreEq1Band = mDpe.intToByteArray(1); // band 1

                List<byte[]> listParamPreEq1 = new ArrayList<>();
                listParamPreEq1.add(tempParamPreEq1Cmd);
                listParamPreEq1.add(tempParamPreEq1Ch);
                listParamPreEq1.add(tempParamPreEq1Band);
                byte[] tempParam_preEq1 = mergeByte(listParamPreEq1);

                byte[] tempValuePreEq1En = new byte[4];
                byte[] tempValuePreEq1Cut = new byte[4];
                byte[] tempValuePreEq1Gain = new byte[4];

                tempValuePreEq1En = mDpe.intToByteArray(getDpeParam(AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND1));
                tempValuePreEq1Cut = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND1_CUTOFFFREQUENCY));
                tempValuePreEq1Gain = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND1_GAIN));

                //set
                List<byte[]> listValuePreEq1 = new ArrayList<>();
                if (id == AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND1_CUTOFFFREQUENCY) {
                    tempValuePreEq1Cut = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND1_GAIN) {
                    tempValuePreEq1Gain = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND1) {
                    tempValuePreEq1En = mDpe.intToByteArray(value);
                }

                listValuePreEq1.add(tempValuePreEq1En);
                listValuePreEq1.add(tempValuePreEq1Cut);
                listValuePreEq1.add(tempValuePreEq1Gain);
                byte[] tempValue_preEq1 = mergeByte(listValuePreEq1);

                mDpe.setParameter(tempParam_preEq1, tempValue_preEq1);
                tempParam_preEq1[4] = 1; //channel 1
                mDpe.setParameter(tempParam_preEq1, tempValue_preEq1);
                saveDpeParam(id, value);
                break;

            // pre eq band 2
            case AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND2:
            case AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND2_CUTOFFFREQUENCY:
            case AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND2_GAIN:
                byte[] tempParamPreEq2Cmd = new byte[4];
                byte[] tempParamPreEq2Ch = new byte[4];
                byte[] tempParamPreEq2Band = new byte[4];
                tempParamPreEq2Cmd = mDpe.intToByteArray(AudioEffectManager.CMD_DPE_PRE_EQ_BAND);
                tempParamPreEq2Ch = mDpe.intToByteArray(0); //channel 0
                tempParamPreEq2Band = mDpe.intToByteArray(2); // band 2

                List<byte[]> listParamPreEq2 = new ArrayList<>();
                listParamPreEq2.add(tempParamPreEq2Cmd);
                listParamPreEq2.add(tempParamPreEq2Ch);
                listParamPreEq2.add(tempParamPreEq2Band);
                byte[] tempParam_preEq2 = mergeByte(listParamPreEq2);

                byte[] tempValuePreEq2En = new byte[4];
                byte[] tempValuePreEq2Cut = new byte[4];
                byte[] tempValuePreEq2Gain = new byte[4];

                tempValuePreEq2En = mDpe.intToByteArray(getDpeParam(AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND2));
                tempValuePreEq2Cut = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND2_CUTOFFFREQUENCY));
                tempValuePreEq2Gain = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND2_GAIN));

                //set
                List<byte[]> listValuePreEq2 = new ArrayList<>();
                if (id == AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND2_CUTOFFFREQUENCY) {
                    tempValuePreEq2Cut = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND2_GAIN) {
                    tempValuePreEq2Gain = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND2) {
                    tempValuePreEq2En = mDpe.intToByteArray(value);
                }

                listValuePreEq2.add(tempValuePreEq2En);
                listValuePreEq2.add(tempValuePreEq2Cut);
                listValuePreEq2.add(tempValuePreEq2Gain);
                byte[] tempValue_preEq2 = mergeByte(listValuePreEq2);

                mDpe.setParameter(tempParam_preEq2, tempValue_preEq2);
                tempParam_preEq2[4] = 1; //channel 1
                mDpe.setParameter(tempParam_preEq2, tempValue_preEq2);
                saveDpeParam(id, value);
                break;

            // post eq band 0
            case AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND0:
            case AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND0_CUTOFFFREQUENCY:
            case AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND0_GAIN:
                byte[] tempParamPostEq0Cmd = new byte[4];
                byte[] tempParamPostEq0Ch = new byte[4];
                byte[] tempParamPostEq0Band = new byte[4];
                tempParamPostEq0Cmd = mDpe.intToByteArray(AudioEffectManager.CMD_DPE_POST_EQ_BAND);
                tempParamPostEq0Ch = mDpe.intToByteArray(0); //channel 0
                tempParamPostEq0Band = mDpe.intToByteArray(0); // band 0

                List<byte[]> listParamPostEq0 = new ArrayList<>();
                listParamPostEq0.add(tempParamPostEq0Cmd);
                listParamPostEq0.add(tempParamPostEq0Ch);
                listParamPostEq0.add(tempParamPostEq0Band);
                byte[] tempParam_postEq0 = mergeByte(listParamPostEq0);

                byte[] tempValuePostEq0En = new byte[4];
                byte[] tempValuePostEq0Cut = new byte[4];
                byte[] tempValuePostEq0Gain = new byte[4];

                tempValuePostEq0En = mDpe.intToByteArray(getDpeParam(AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND0));
                tempValuePostEq0Cut = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND0_CUTOFFFREQUENCY));
                tempValuePostEq0Gain = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND0_GAIN));

                //set
                List<byte[]> listValuePostEq0 = new ArrayList<>();
                if (id == AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND0_CUTOFFFREQUENCY) {
                    tempValuePostEq0Cut = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND0_GAIN) {
                    tempValuePostEq0Gain = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND0) {
                    tempValuePostEq0En = mDpe.intToByteArray(value);
                }

                listValuePostEq0.add(tempValuePostEq0En);
                listValuePostEq0.add(tempValuePostEq0Cut);
                listValuePostEq0.add(tempValuePostEq0Gain);
                byte[] tempValue_postEq0 = mergeByte(listValuePostEq0);

                mDpe.setParameter(tempParam_postEq0, tempValue_postEq0);
                tempParam_postEq0[4] = 1; //channel 1
                mDpe.setParameter(tempParam_postEq0, tempValue_postEq0);
                saveDpeParam(id, value);
                break;

            // post eq band 1
            case AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND1:
            case AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND1_CUTOFFFREQUENCY:
            case AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND1_GAIN:
                byte[] tempParamPostEq1Cmd = new byte[4];
                byte[] tempParamPostEq1Ch = new byte[4];
                byte[] tempParamPostEq1Band = new byte[4];
                tempParamPostEq1Cmd = mDpe.intToByteArray(AudioEffectManager.CMD_DPE_POST_EQ_BAND);
                tempParamPostEq1Ch = mDpe.intToByteArray(0); //channel 0
                tempParamPostEq1Band = mDpe.intToByteArray(1); // band 1

                List<byte[]> listParamPostEq1 = new ArrayList<>();
                listParamPostEq1.add(tempParamPostEq1Cmd);
                listParamPostEq1.add(tempParamPostEq1Ch);
                listParamPostEq1.add(tempParamPostEq1Band);
                byte[] tempParam_postEq1 = mergeByte(listParamPostEq1);

                byte[] tempValuePostEq1En = new byte[4];
                byte[] tempValuePostEq1Cut = new byte[4];
                byte[] tempValuePostEq1Gain = new byte[4];

                tempValuePostEq1En = mDpe.intToByteArray(getDpeParam(AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND1));
                tempValuePostEq1Cut = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND1_CUTOFFFREQUENCY));
                tempValuePostEq1Gain = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND1_GAIN));

                //set
                List<byte[]> listValuePostEq1 = new ArrayList<>();
                if (id == AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND1_CUTOFFFREQUENCY) {
                    tempValuePostEq1Cut = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND1_GAIN) {
                    tempValuePostEq1Gain = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND1) {
                    tempValuePostEq1En = mDpe.intToByteArray(value);
                }

                listValuePostEq1.add(tempValuePostEq1En);
                listValuePostEq1.add(tempValuePostEq1Cut);
                listValuePostEq1.add(tempValuePostEq1Gain);
                byte[] tempValue_postEq1 = mergeByte(listValuePostEq1);

                mDpe.setParameter(tempParam_postEq1, tempValue_postEq1);
                tempParam_postEq1[4] = 1; //channel 1
                mDpe.setParameter(tempParam_postEq1, tempValue_postEq1);
                saveDpeParam(id, value);
                break;

            // post eq band 2
            case AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND2:
            case AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND2_CUTOFFFREQUENCY:
            case AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND2_GAIN:
                byte[] tempParamPostEq2Cmd = new byte[4];
                byte[] tempParamPostEq2Ch = new byte[4];
                byte[] tempParamPostEq2Band = new byte[4];
                tempParamPostEq2Cmd = mDpe.intToByteArray(AudioEffectManager.CMD_DPE_POST_EQ_BAND);
                tempParamPostEq2Ch = mDpe.intToByteArray(0); //channel 0
                tempParamPostEq2Band = mDpe.intToByteArray(2); // band 2

                List<byte[]> listParamPostEq2 = new ArrayList<>();
                listParamPostEq2.add(tempParamPostEq2Cmd);
                listParamPostEq2.add(tempParamPostEq2Ch);
                listParamPostEq2.add(tempParamPostEq2Band);
                byte[] tempParam_postEq2 = mergeByte(listParamPostEq2);

                byte[] tempValuePostEq2En = new byte[4];
                byte[] tempValuePostEq2Cut = new byte[4];
                byte[] tempValuePostEq2Gain = new byte[4];

                tempValuePostEq2En = mDpe.intToByteArray(getDpeParam(AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND2));
                tempValuePostEq2Cut = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND2_CUTOFFFREQUENCY));
                tempValuePostEq2Gain = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND2_GAIN));

                //set
                List<byte[]> listValuePostEq2 = new ArrayList<>();
                if (id == AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND2_CUTOFFFREQUENCY) {
                    tempValuePostEq2Cut = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND2_GAIN) {
                    tempValuePostEq2Gain = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND2) {
                    tempValuePostEq2En = mDpe.intToByteArray(value);
                }

                listValuePostEq2.add(tempValuePostEq2En);
                listValuePostEq2.add(tempValuePostEq2Cut);
                listValuePostEq2.add(tempValuePostEq2Gain);
                byte[] tempValue_postEq2 = mergeByte(listValuePostEq2);

                mDpe.setParameter(tempParam_postEq2, tempValue_postEq2);
                tempParam_postEq2[4] = 1; //channel 1
                mDpe.setParameter(tempParam_postEq2, tempValue_postEq2);
                saveDpeParam(id, value);
                break;

            //mbc band 0
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND0:
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND0_CUTOFFFREQUENCY:
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND0_ATTACKTIME:
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND0_RELEASETIME:
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND0_RATIO:
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND0_THRESHOLD:
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND0_KNEEWIDTH:
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND0_NOISEGATETHRESHOLD:
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND0_EXPANDERRATIO:
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND0_PREGAIN:
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND0_POSTGAIN:
                byte[] tempParamMbc0Cmd = new byte[4];
                byte[] tempParamMbc0Ch = new byte[4];
                byte[] tempParamMbc0Band = new byte[4];
                tempParamMbc0Cmd = mDpe.intToByteArray(AudioEffectManager.CMD_DPE_MBC_BAND);
                tempParamMbc0Ch = mDpe.intToByteArray(0); //channel 0
                tempParamMbc0Band = mDpe.intToByteArray(0); // band 0

                List<byte[]> listParamMbc0 = new ArrayList<>();
                listParamMbc0.add(tempParamMbc0Cmd);
                listParamMbc0.add(tempParamMbc0Ch);
                listParamMbc0.add(tempParamMbc0Band);
                byte[] tempParam_mbc0 = mergeByte(listParamMbc0);

                byte[] tempValueMbc0En = new byte[4];
                byte[] tempValueMbc0Cut = new byte[4];
                byte[] tempValueMbc0Att = new byte[4];
                byte[] tempValueMbc0Relea = new byte[4];
                byte[] tempValueMbc0Ratio = new byte[4];
                byte[] tempValueMbc0Thre = new byte[4];
                byte[] tempValueMbc0Knee = new byte[4];
                byte[] tempValueMbc0Noise = new byte[4];
                byte[] tempValueMbc0Exp = new byte[4];
                byte[] tempValueMbc0Pre = new byte[4];
                byte[] tempValueMbc0Post = new byte[4];

                tempValueMbc0En    = mDpe.intToByteArray(getDpeParam(AudioEffectManager.SUBCMD_DPE_MBC_BAND0));
                tempValueMbc0Cut   = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_MBC_BAND0_CUTOFFFREQUENCY));
                tempValueMbc0Att   = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_MBC_BAND0_ATTACKTIME));
                tempValueMbc0Relea = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_MBC_BAND0_RELEASETIME));
                tempValueMbc0Ratio = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_MBC_BAND0_RATIO));
                tempValueMbc0Thre  = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_MBC_BAND0_THRESHOLD));
                tempValueMbc0Knee  = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_MBC_BAND0_KNEEWIDTH));
                tempValueMbc0Noise = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_MBC_BAND0_NOISEGATETHRESHOLD));
                tempValueMbc0Exp   = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_MBC_BAND0_EXPANDERRATIO));
                tempValueMbc0Pre   = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_MBC_BAND0_PREGAIN));
                tempValueMbc0Post  = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_MBC_BAND0_POSTGAIN));

                //set
                List<byte[]> listValueMbc0 = new ArrayList<>();
                if (id == AudioEffectManager.SUBCMD_DPE_MBC_BAND0_CUTOFFFREQUENCY) {
                    tempValueMbc0Cut = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_MBC_BAND0_ATTACKTIME) {
                    tempValueMbc0Att = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_MBC_BAND0_RELEASETIME) {
                    tempValueMbc0Relea = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_MBC_BAND0_RATIO) {
                    tempValueMbc0Ratio = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_MBC_BAND0_THRESHOLD) {
                    tempValueMbc0Thre = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_MBC_BAND0_KNEEWIDTH) {
                    tempValueMbc0Knee = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_MBC_BAND0_NOISEGATETHRESHOLD) {
                    tempValueMbc0Noise = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_MBC_BAND0_EXPANDERRATIO) {
                    tempValueMbc0Exp = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_MBC_BAND0_PREGAIN) {
                    tempValueMbc0Pre = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_MBC_BAND0_POSTGAIN) {
                    tempValueMbc0Post = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_MBC_BAND0) {
                    tempValueMbc0En = mDpe.intToByteArray(value);
                }

                listValueMbc0.add(tempValueMbc0En);
                listValueMbc0.add(tempValueMbc0Cut);
                listValueMbc0.add(tempValueMbc0Att);
                listValueMbc0.add(tempValueMbc0Relea);
                listValueMbc0.add(tempValueMbc0Ratio);
                listValueMbc0.add(tempValueMbc0Thre);
                listValueMbc0.add(tempValueMbc0Knee);
                listValueMbc0.add(tempValueMbc0Noise);
                listValueMbc0.add(tempValueMbc0Exp);
                listValueMbc0.add(tempValueMbc0Pre);
                listValueMbc0.add(tempValueMbc0Post);
                byte[] tempValue_mbc0 = mergeByte(listValueMbc0);

                mDpe.setParameter(tempParam_mbc0, tempValue_mbc0);
                tempParam_mbc0[4] = 1; //channel 1
                mDpe.setParameter(tempParam_mbc0, tempValue_mbc0);
                saveDpeParam(id, value);
                break;

            //mbc band 1
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND1:
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND1_CUTOFFFREQUENCY:
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND1_ATTACKTIME:
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND1_RELEASETIME:
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND1_RATIO:
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND1_THRESHOLD:
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND1_KNEEWIDTH:
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND1_NOISEGATETHRESHOLD:
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND1_EXPANDERRATIO:
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND1_PREGAIN:
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND1_POSTGAIN:
                byte[] tempParamMbc1Cmd = new byte[4];
                byte[] tempParamMbc1Ch = new byte[4];
                byte[] tempParamMbc1Band = new byte[4];
                tempParamMbc1Cmd = mDpe.intToByteArray(AudioEffectManager.CMD_DPE_MBC_BAND);
                tempParamMbc1Ch = mDpe.intToByteArray(0); //channel 0
                tempParamMbc1Band = mDpe.intToByteArray(1); // band 1

                List<byte[]> listParamMbc1 = new ArrayList<>();
                listParamMbc1.add(tempParamMbc1Cmd);
                listParamMbc1.add(tempParamMbc1Ch);
                listParamMbc1.add(tempParamMbc1Band);
                byte[] tempParam_mbc1 = mergeByte(listParamMbc1);

                byte[] tempValueMbc1En = new byte[4];
                byte[] tempValueMbc1Cut = new byte[4];
                byte[] tempValueMbc1Att = new byte[4];
                byte[] tempValueMbc1Relea = new byte[4];
                byte[] tempValueMbc1Ratio = new byte[4];
                byte[] tempValueMbc1Thre = new byte[4];
                byte[] tempValueMbc1Knee = new byte[4];
                byte[] tempValueMbc1Noise = new byte[4];
                byte[] tempValueMbc1Exp = new byte[4];
                byte[] tempValueMbc1Pre = new byte[4];
                byte[] tempValueMbc1Post = new byte[4];

                tempValueMbc1En    = mDpe.intToByteArray(getDpeParam(AudioEffectManager.SUBCMD_DPE_MBC_BAND1));
                tempValueMbc1Cut   = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_MBC_BAND1_CUTOFFFREQUENCY));
                tempValueMbc1Att   = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_MBC_BAND1_ATTACKTIME));
                tempValueMbc1Relea = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_MBC_BAND1_RELEASETIME));
                tempValueMbc1Ratio = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_MBC_BAND1_RATIO));
                tempValueMbc1Thre  = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_MBC_BAND1_THRESHOLD));
                tempValueMbc1Knee  = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_MBC_BAND1_KNEEWIDTH));
                tempValueMbc1Noise = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_MBC_BAND1_NOISEGATETHRESHOLD));
                tempValueMbc1Exp   = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_MBC_BAND1_EXPANDERRATIO));
                tempValueMbc1Pre   = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_MBC_BAND1_PREGAIN));
                tempValueMbc1Post  = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_MBC_BAND1_POSTGAIN));

                //set
                List<byte[]> listValueMbc1 = new ArrayList<>();
                if (id == AudioEffectManager.SUBCMD_DPE_MBC_BAND1_CUTOFFFREQUENCY) {
                    tempValueMbc1Cut = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_MBC_BAND1_ATTACKTIME) {
                    tempValueMbc1Att = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_MBC_BAND1_RELEASETIME) {
                    tempValueMbc1Relea = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_MBC_BAND1_RATIO) {
                    tempValueMbc1Ratio = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_MBC_BAND1_THRESHOLD) {
                    tempValueMbc1Thre = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_MBC_BAND1_KNEEWIDTH) {
                    tempValueMbc1Knee = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_MBC_BAND1_NOISEGATETHRESHOLD) {
                    tempValueMbc1Noise = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_MBC_BAND1_EXPANDERRATIO) {
                    tempValueMbc1Exp = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_MBC_BAND1_PREGAIN) {
                    tempValueMbc1Pre = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_MBC_BAND1_POSTGAIN) {
                    tempValueMbc1Post = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_MBC_BAND1) {
                    tempValueMbc1En = mDpe.intToByteArray(value);
                }

                listValueMbc1.add(tempValueMbc1En);
                listValueMbc1.add(tempValueMbc1Cut);
                listValueMbc1.add(tempValueMbc1Att);
                listValueMbc1.add(tempValueMbc1Relea);
                listValueMbc1.add(tempValueMbc1Ratio);
                listValueMbc1.add(tempValueMbc1Thre);
                listValueMbc1.add(tempValueMbc1Knee);
                listValueMbc1.add(tempValueMbc1Noise);
                listValueMbc1.add(tempValueMbc1Exp);
                listValueMbc1.add(tempValueMbc1Pre);
                listValueMbc1.add(tempValueMbc1Post);
                byte[] tempValue_mbc1 = mergeByte(listValueMbc1);

                mDpe.setParameter(tempParam_mbc1, tempValue_mbc1);
                tempParam_mbc1[4] = 1; //channel 1
                mDpe.setParameter(tempParam_mbc1, tempValue_mbc1);
                saveDpeParam(id, value);
                break;

            // mbc band 2
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND2:
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND2_CUTOFFFREQUENCY:
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND2_ATTACKTIME:
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND2_RELEASETIME:
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND2_RATIO:
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND2_THRESHOLD:
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND2_KNEEWIDTH:
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND2_NOISEGATETHRESHOLD:
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND2_EXPANDERRATIO:
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND2_PREGAIN:
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND2_POSTGAIN:
                byte[] tempParamMbc2Cmd = new byte[4];
                byte[] tempParamMbc2Ch = new byte[4];
                byte[] tempParamMbc2Band = new byte[4];
                tempParamMbc2Cmd = mDpe.intToByteArray(AudioEffectManager.CMD_DPE_MBC_BAND);
                tempParamMbc2Ch = mDpe.intToByteArray(0); //channel 0
                tempParamMbc2Band = mDpe.intToByteArray(2); // band 2

                List<byte[]> listParamMbc2 = new ArrayList<>();
                listParamMbc2.add(tempParamMbc2Cmd);
                listParamMbc2.add(tempParamMbc2Ch);
                listParamMbc2.add(tempParamMbc2Band);
                byte[] tempParam_mbc2 = mergeByte(listParamMbc2);

                byte[] tempValueMbc2En = new byte[4];
                byte[] tempValueMbc2Cut = new byte[4];
                byte[] tempValueMbc2Att = new byte[4];
                byte[] tempValueMbc2Relea = new byte[4];
                byte[] tempValueMbc2Ratio = new byte[4];
                byte[] tempValueMbc2Thre = new byte[4];
                byte[] tempValueMbc2Knee = new byte[4];
                byte[] tempValueMbc2Noise = new byte[4];
                byte[] tempValueMbc2Exp = new byte[4];
                byte[] tempValueMbc2Pre = new byte[4];
                byte[] tempValueMbc2Post = new byte[4];

                tempValueMbc2En    = mDpe.intToByteArray(getDpeParam(AudioEffectManager.SUBCMD_DPE_MBC_BAND2));
                tempValueMbc2Cut   = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_MBC_BAND2_CUTOFFFREQUENCY));
                tempValueMbc2Att   = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_MBC_BAND2_ATTACKTIME));
                tempValueMbc2Relea = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_MBC_BAND2_RELEASETIME));
                tempValueMbc2Ratio = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_MBC_BAND2_RATIO));
                tempValueMbc2Thre  = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_MBC_BAND2_THRESHOLD));
                tempValueMbc2Knee  = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_MBC_BAND2_KNEEWIDTH));
                tempValueMbc2Noise = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_MBC_BAND2_NOISEGATETHRESHOLD));
                tempValueMbc2Exp   = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_MBC_BAND2_EXPANDERRATIO));
                tempValueMbc2Pre   = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_MBC_BAND2_PREGAIN));
                tempValueMbc2Post  = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_MBC_BAND2_POSTGAIN));

                //set
                List<byte[]> listValueMbc2 = new ArrayList<>();
                if (id == AudioEffectManager.SUBCMD_DPE_MBC_BAND2_CUTOFFFREQUENCY) {
                    tempValueMbc2Cut = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_MBC_BAND2_ATTACKTIME) {
                    tempValueMbc2Att = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_MBC_BAND2_RELEASETIME) {
                    tempValueMbc2Relea = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_MBC_BAND2_RATIO) {
                    tempValueMbc2Ratio = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_MBC_BAND2_THRESHOLD) {
                    tempValueMbc2Thre = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_MBC_BAND2_KNEEWIDTH) {
                    tempValueMbc2Knee = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_MBC_BAND2_NOISEGATETHRESHOLD) {
                    tempValueMbc2Noise = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_MBC_BAND2_EXPANDERRATIO) {
                    tempValueMbc2Exp = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_MBC_BAND2_PREGAIN) {
                    tempValueMbc2Pre = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_MBC_BAND2_POSTGAIN) {
                    tempValueMbc2Post = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_MBC_BAND2) {
                    tempValueMbc2En = mDpe.intToByteArray(value);
                }

                listValueMbc2.add(tempValueMbc2En);
                listValueMbc2.add(tempValueMbc2Cut);
                listValueMbc2.add(tempValueMbc2Att);
                listValueMbc2.add(tempValueMbc2Relea);
                listValueMbc2.add(tempValueMbc2Ratio);
                listValueMbc2.add(tempValueMbc2Thre);
                listValueMbc2.add(tempValueMbc2Knee);
                listValueMbc2.add(tempValueMbc2Noise);
                listValueMbc2.add(tempValueMbc2Exp);
                listValueMbc2.add(tempValueMbc2Pre);
                listValueMbc2.add(tempValueMbc2Post);
                byte[] tempValue_mbc2 = mergeByte(listValueMbc2);

                mDpe.setParameter(tempParam_mbc2, tempValue_mbc2);
                tempParam_mbc2[4] = 1; //channel 1
                mDpe.setParameter(tempParam_mbc2, tempValue_mbc2);
                saveDpeParam(id, value);
                break;

            //limiter param
            case AudioEffectManager.CMD_DPE_LIMITER:
            case AudioEffectManager.SUBCMD_DPE_LIMITER_ATTACKTIME:
            case AudioEffectManager.SUBCMD_DPE_LIMITER_RELEASETIME:
            case AudioEffectManager.SUBCMD_DPE_LIMITER_RATIO:
            case AudioEffectManager.SUBCMD_DPE_LIMITER_THRESHOLD:
            case AudioEffectManager.SUBCMD_DPE_LIMITER_POSTGAIN:
                byte[] tempParam_limiter = new byte[] {AudioEffectManager.CMD_DPE_LIMITER, 0, 0, 0, 0, 0, 0, 0};

                byte[] tempValueLimiterIu    = new byte[4];
                byte[] tempValueLimiterEn    = new byte[4];
                byte[] tempValueLimiterLink  = new byte[] {1, 0, 0, 0};
                byte[] tempValueLimiterAtt   = new byte[4];
                byte[] tempValueLimiterRelea = new byte[4];
                byte[] tempValueLimiterRatio = new byte[4];
                byte[] tempValueLimiterThre  = new byte[4];
                byte[] tempValueLimiterPost  = new byte[4];

                tempValueLimiterIu    = mDpe.intToByteArray(getDpeParam(AudioEffectManager.CMD_DPE_LIMITER));
                tempValueLimiterEn    = mDpe.intToByteArray(getDpeParam(AudioEffectManager.CMD_DPE_LIMITER));
                tempValueLimiterAtt   = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_LIMITER_ATTACKTIME));
                tempValueLimiterRelea = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_LIMITER_RELEASETIME));
                tempValueLimiterRatio = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_LIMITER_RATIO));
                tempValueLimiterThre  = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_LIMITER_THRESHOLD));
                tempValueLimiterPost  = mDpe.floatToByteArray((float)getDpeParam(AudioEffectManager.SUBCMD_DPE_LIMITER_POSTGAIN));

                //set
                List<byte[]> listValueLimiter = new ArrayList<>();
                if (id == AudioEffectManager.SUBCMD_DPE_LIMITER_ATTACKTIME) {
                    tempValueLimiterAtt = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_LIMITER_RELEASETIME) {
                    tempValueLimiterRelea = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_LIMITER_RATIO) {
                    tempValueLimiterRatio = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_LIMITER_THRESHOLD) {
                    tempValueLimiterThre = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.SUBCMD_DPE_LIMITER_POSTGAIN) {
                    tempValueLimiterPost = mDpe.floatToByteArray((float)value);
                } else if (id == AudioEffectManager.CMD_DPE_LIMITER) {
                    tempValueLimiterIu = mDpe.intToByteArray(value);
                    tempValueLimiterEn = mDpe.intToByteArray(value);
                }

                listValueLimiter.add(tempValueLimiterIu);
                listValueLimiter.add(tempValueLimiterEn);
                listValueLimiter.add(tempValueLimiterLink);
                listValueLimiter.add(tempValueLimiterAtt);
                listValueLimiter.add(tempValueLimiterRelea);
                listValueLimiter.add(tempValueLimiterRatio);
                listValueLimiter.add(tempValueLimiterThre);
                listValueLimiter.add(tempValueLimiterPost);
                byte[] tempValue_limiter = mergeByte(listValueLimiter);

                mDpe.setParameter(tempParam_limiter, tempValue_limiter);
                tempParam_limiter[4] = 1; // channel 1
                mDpe.setParameter(tempParam_limiter, tempValue_limiter);
                saveDpeParam(id, value);
                break;
        }
    }


    // dpe get param
    public int getDpeParam(int id) {
        int value = -1;
        if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "getDpeParam id:" + id + ", value:" + value);
        switch (id) {
            case AudioEffectManager.CMD_DPE_ENABLED:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_ENABLED, -1);
                if (value < 0) {
                    value = getDpeParamInternal(AudioEffectManager.CMD_DPE_ENABLED);
                    Log.d(TAG, "getDpeParam id:CMD_DPE_ENABLED hal value:" + value);
                    saveDpeParam(id, value);
                }
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_ENABLED, AudioEffectManager.DEFAULT_DPE_ENABLE);
                Log.d(TAG, "getDpeParam id:CMD_DPE_ENABLED get DPE value:" + value);
                break;
            case AudioEffectManager.CMD_DPE_INPUTGAIN:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_INPUTGAIN, AudioEffectManager.DEFAULT_DPE_INPUTGAIN);
                break;
            case AudioEffectManager.CMD_DPE_PRE_EQ:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_PRE_EQ, AudioEffectManager.DPE_PRE_EQ_OFF);
                break;
            case AudioEffectManager.CMD_DPE_MBC:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC, AudioEffectManager.DPE_MBC_OFF);
                break;
            case AudioEffectManager.CMD_DPE_POST_EQ:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_POST_EQ, AudioEffectManager.DPE_POST_EQ_OFF);
                break;
            case AudioEffectManager.CMD_DPE_LIMITER:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_LIMITER, AudioEffectManager.DPE_LIMITER_OFF);
                break;

            // pre eq band 0
            case AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND0:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_PRE_EQ_BAND0, 0);
                break;
            case AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND0_CUTOFFFREQUENCY:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_PRE_EQ_BAND0_CUTOFFFREQUENCY, AudioEffectManager.DEFAULT_DPE_BAND0_CUTOFFFREQUENCY);
                break;
            case AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND0_GAIN:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_PRE_EQ_BAND0_GAIN, AudioEffectManager.DEFAULT_DPE_EQ_GAIN);
                break;

            // pre eq band 1
            case AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND1:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_PRE_EQ_BAND1, 0);
                break;
            case AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND1_CUTOFFFREQUENCY:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_PRE_EQ_BAND1_CUTOFFFREQUENCY, AudioEffectManager.DEFAULT_DPE_BAND1_CUTOFFFREQUENCY);
                break;
            case AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND1_GAIN:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_PRE_EQ_BAND1_GAIN, AudioEffectManager.DEFAULT_DPE_EQ_GAIN);
                break;

            // pre eq band 2
            case AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND2:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_PRE_EQ_BAND2, 0);
                break;
            case AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND2_CUTOFFFREQUENCY:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_PRE_EQ_BAND2_CUTOFFFREQUENCY, AudioEffectManager.DEFAULT_DPE_BAND2_CUTOFFFREQUENCY);
                break;
            case AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND2_GAIN:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_PRE_EQ_BAND2_GAIN, AudioEffectManager.DAP_GEQ_DEFAULT);
                break;

            // post eq band 0
            case AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND0:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_POST_EQ_BAND0, 0);
                break;
            case AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND0_CUTOFFFREQUENCY:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_POST_EQ_BAND0_CUTOFFFREQUENCY, AudioEffectManager.DEFAULT_DPE_BAND0_CUTOFFFREQUENCY);
                break;
            case AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND0_GAIN:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_POST_EQ_BAND0_GAIN, AudioEffectManager.DEFAULT_DPE_EQ_GAIN);
                break;

            // post eq band 1
            case AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND1:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_POST_EQ_BAND1, 0);
                break;
            case AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND1_CUTOFFFREQUENCY:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_POST_EQ_BAND1_CUTOFFFREQUENCY, AudioEffectManager.DEFAULT_DPE_BAND1_CUTOFFFREQUENCY);
                break;
            case AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND1_GAIN:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_POST_EQ_BAND1_GAIN, AudioEffectManager.DEFAULT_DPE_EQ_GAIN);
                break;

            // post eq band 2
            case AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND2:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_POST_EQ_BAND2, 0);
                break;
            case AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND2_CUTOFFFREQUENCY:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_POST_EQ_BAND2_CUTOFFFREQUENCY, AudioEffectManager.DEFAULT_DPE_BAND2_CUTOFFFREQUENCY);
                break;
            case AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND2_GAIN:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_POST_EQ_BAND2_GAIN, AudioEffectManager.DEFAULT_DPE_EQ_GAIN);
                break;

            // MBC band 0
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND0:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND0, 0);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND0_CUTOFFFREQUENCY:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND0_CUTOFFFREQUENCY, AudioEffectManager.DEFAULT_DPE_BAND0_CUTOFFFREQUENCY);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND0_ATTACKTIME:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND0_ATTACKTIME, AudioEffectManager.DEFAULT_DPE_ATTACKTIME);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND0_RELEASETIME:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND0_RELEASETIME, AudioEffectManager.DEFAULT_DPE_RELEASETIME);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND0_RATIO:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND0_RATIO, AudioEffectManager.DEFAULT_DPE_MBC_RATIO);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND0_THRESHOLD:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND0_THRESHOLD, AudioEffectManager.DEFAULT_DPE_MBC_THRESHOLD);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND0_KNEEWIDTH:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND0_KNEEWIDTH, AudioEffectManager.DEFAULT_DPE_MBC_KNEEWIDTH);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND0_NOISEGATETHRESHOLD:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND0_NOISEGATE_THRESHOLD, AudioEffectManager.DEFAULT_DPE_MBC_NOISEGATETHRESHOLD);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND0_EXPANDERRATIO:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND0_EXPANDER_RATIO, AudioEffectManager.DEFAULT_DPE_MBC_EXPANDERRATIO);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND0_PREGAIN:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND0_PRE_GAIN, AudioEffectManager.DEFAULT_DPE_MBC_PREGAIN);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND0_POSTGAIN:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND0_POST_GAIN, AudioEffectManager.DEFAULT_DPE_MBC_POSTGAIN);
                break;

            // MBC band 1
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND1:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND1, 0);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND1_CUTOFFFREQUENCY:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND1_CUTOFFFREQUENCY, AudioEffectManager.DEFAULT_DPE_BAND1_CUTOFFFREQUENCY);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND1_ATTACKTIME:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND1_ATTACKTIME, AudioEffectManager.DEFAULT_DPE_ATTACKTIME);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND1_RELEASETIME:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND1_RELEASETIME, AudioEffectManager.DEFAULT_DPE_RELEASETIME);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND1_RATIO:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND1_RATIO, AudioEffectManager.DEFAULT_DPE_MBC_RATIO);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND1_THRESHOLD:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND1_THRESHOLD, AudioEffectManager.DEFAULT_DPE_MBC_THRESHOLD);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND1_KNEEWIDTH:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND1_KNEEWIDTH, AudioEffectManager.DEFAULT_DPE_MBC_KNEEWIDTH);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND1_NOISEGATETHRESHOLD:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND1_NOISEGATE_THRESHOLD, AudioEffectManager.DEFAULT_DPE_MBC_NOISEGATETHRESHOLD);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND1_EXPANDERRATIO:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND1_EXPANDER_RATIO, AudioEffectManager.DEFAULT_DPE_MBC_EXPANDERRATIO);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND1_PREGAIN:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND1_PRE_GAIN, AudioEffectManager.DEFAULT_DPE_MBC_PREGAIN);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND1_POSTGAIN:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND1_POST_GAIN, AudioEffectManager.DEFAULT_DPE_MBC_POSTGAIN);
                break;

            // MBC band 2
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND2:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND2, 0);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND2_CUTOFFFREQUENCY:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND2_CUTOFFFREQUENCY, AudioEffectManager.DEFAULT_DPE_BAND2_CUTOFFFREQUENCY);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND2_ATTACKTIME:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND2_ATTACKTIME, AudioEffectManager.DEFAULT_DPE_ATTACKTIME);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND2_RELEASETIME:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND2_RELEASETIME, AudioEffectManager.DEFAULT_DPE_RELEASETIME);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND2_RATIO:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND2_RATIO, AudioEffectManager.DEFAULT_DPE_MBC_RATIO);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND2_THRESHOLD:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND2_THRESHOLD, AudioEffectManager.DEFAULT_DPE_MBC_THRESHOLD);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND2_KNEEWIDTH:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND2_KNEEWIDTH, AudioEffectManager.DEFAULT_DPE_MBC_KNEEWIDTH);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND2_NOISEGATETHRESHOLD:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND2_NOISEGATE_THRESHOLD, AudioEffectManager.DEFAULT_DPE_MBC_NOISEGATETHRESHOLD);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND2_EXPANDERRATIO:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND2_EXPANDER_RATIO, AudioEffectManager.DEFAULT_DPE_MBC_EXPANDERRATIO);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND2_PREGAIN:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND2_PRE_GAIN, AudioEffectManager.DEFAULT_DPE_MBC_PREGAIN);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND2_POSTGAIN:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND2_POST_GAIN, AudioEffectManager.DEFAULT_DPE_MBC_POSTGAIN);
                break;

            // limiter sub param
            case AudioEffectManager.SUBCMD_DPE_LIMITER_ATTACKTIME:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_LIMITER_ATTACKTIME, AudioEffectManager.DEFAULT_DPE_ATTACKTIME);
                break;
            case AudioEffectManager.SUBCMD_DPE_LIMITER_RELEASETIME:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_LIMITER_RELEASETIMR, AudioEffectManager.DEFAULT_DPE_RELEASETIME);
                break;
            case AudioEffectManager.SUBCMD_DPE_LIMITER_RATIO:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_LIMITER_RATIO, AudioEffectManager.DEFAULT_DPE_LIMITER_RATIO);
                break;
            case AudioEffectManager.SUBCMD_DPE_LIMITER_THRESHOLD:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_LIMITER_THRESHOLD, AudioEffectManager.DEFAULT_DPE_LIMITER_THRESHOLD);
                break;
            case AudioEffectManager.SUBCMD_DPE_LIMITER_POSTGAIN:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_LIMITER_POST_GAIN, AudioEffectManager.DEFAULT_DPE_LIMITER_POSTGAIN);
                break;
        }
        return value;
    }


    // dpe save param
    public void saveDpeParam (int id, int value) {
        if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "saveDpeParam id:" + id + ", value:" + value);

        switch (id) {
            case AudioEffectManager.CMD_DPE_ENABLED:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_ENABLED, value);
                break;
            case AudioEffectManager.CMD_DPE_INPUTGAIN:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_INPUTGAIN, value);
                break;
            case AudioEffectManager.CMD_DPE_PRE_EQ:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_PRE_EQ, value);
                break;
            case AudioEffectManager.CMD_DPE_MBC:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC, value);
                break;
            case AudioEffectManager.CMD_DPE_POST_EQ:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_POST_EQ, value);
                break;
            case AudioEffectManager.CMD_DPE_LIMITER:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_LIMITER, value);
                break;

            // pre eq band 0
            case AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND0:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_PRE_EQ_BAND0, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND0_CUTOFFFREQUENCY:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_PRE_EQ_BAND0_CUTOFFFREQUENCY, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND0_GAIN:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_PRE_EQ_BAND0_GAIN, value);
                break;

            // pre eq band 1
            case AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND1:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_PRE_EQ_BAND1, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND1_CUTOFFFREQUENCY:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_PRE_EQ_BAND1_CUTOFFFREQUENCY, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND1_GAIN:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_PRE_EQ_BAND1_GAIN, value);
                break;

            // pre eq band 2
            case AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND2:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_PRE_EQ_BAND2, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND2_CUTOFFFREQUENCY:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_PRE_EQ_BAND2_CUTOFFFREQUENCY, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_PRE_EQ_BAND2_GAIN:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_PRE_EQ_BAND2_GAIN, value);
                break;

            // post eq band 0
            case AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND0:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_POST_EQ_BAND0, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND0_CUTOFFFREQUENCY:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_POST_EQ_BAND0_CUTOFFFREQUENCY, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND0_GAIN:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_POST_EQ_BAND0_GAIN, value);
                break;

            // post eq band 1
            case AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND1:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_POST_EQ_BAND1, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND1_CUTOFFFREQUENCY:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_POST_EQ_BAND1_CUTOFFFREQUENCY, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND1_GAIN:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_POST_EQ_BAND1_GAIN, value);
                break;

            // post eq band 2
            case AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND2:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_POST_EQ_BAND2, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND2_CUTOFFFREQUENCY:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_POST_EQ_BAND2_CUTOFFFREQUENCY, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_POST_EQ_BAND2_GAIN:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_POST_EQ_BAND2_GAIN, value);
                break;

            // MBC band 0
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND0:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND0, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND0_CUTOFFFREQUENCY:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND0_CUTOFFFREQUENCY, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND0_ATTACKTIME:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND0_ATTACKTIME, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND0_RELEASETIME:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND0_RELEASETIME, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND0_RATIO:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND0_RATIO, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND0_THRESHOLD:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND0_THRESHOLD, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND0_KNEEWIDTH:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND0_KNEEWIDTH, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND0_NOISEGATETHRESHOLD:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND0_NOISEGATE_THRESHOLD, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND0_EXPANDERRATIO:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND0_EXPANDER_RATIO, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND0_PREGAIN:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND0_PRE_GAIN, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND0_POSTGAIN:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND0_POST_GAIN, value);
                break;

            // MBC band 1
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND1:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND1, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND1_CUTOFFFREQUENCY:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND1_CUTOFFFREQUENCY, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND1_ATTACKTIME:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND1_ATTACKTIME, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND1_RELEASETIME:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND1_RELEASETIME, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND1_RATIO:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND1_RATIO, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND1_THRESHOLD:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND1_THRESHOLD, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND1_KNEEWIDTH:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND1_KNEEWIDTH, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND1_NOISEGATETHRESHOLD:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND1_NOISEGATE_THRESHOLD, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND1_EXPANDERRATIO:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND1_EXPANDER_RATIO, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND1_PREGAIN:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND1_PRE_GAIN, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND1_POSTGAIN:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND1_POST_GAIN, value);
                break;

            // MBC band 2
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND2:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND2, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND2_CUTOFFFREQUENCY:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND2_CUTOFFFREQUENCY, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND2_ATTACKTIME:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND2_ATTACKTIME, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND2_RELEASETIME:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND2_RELEASETIME, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND2_RATIO:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND2_RATIO, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND2_THRESHOLD:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND2_THRESHOLD, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND2_KNEEWIDTH:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND2_KNEEWIDTH, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND2_NOISEGATETHRESHOLD:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND2_NOISEGATE_THRESHOLD, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND2_EXPANDERRATIO:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND2_EXPANDER_RATIO, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND2_PREGAIN:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND2_PRE_GAIN, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_MBC_BAND2_POSTGAIN:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_MBC_BAND2_POST_GAIN, value);
                break;

            // limiter sub param
            case AudioEffectManager.SUBCMD_DPE_LIMITER_ATTACKTIME:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_LIMITER_ATTACKTIME, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_LIMITER_RELEASETIME:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_LIMITER_RELEASETIMR, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_LIMITER_RATIO:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_LIMITER_RATIO, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_LIMITER_THRESHOLD:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_LIMITER_THRESHOLD, value);
                break;
            case AudioEffectManager.SUBCMD_DPE_LIMITER_POSTGAIN:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_LIMITER_POST_GAIN, value);
                break;
        }
    }

    public void createAudioEffectsByIndex () {
        Log.d(TAG, "createAudioEffects Start to create audio effects...");
        mSupportVirtualX = false;

        if (isAudioEffectOn(AudioEffectManager.DEBUG_HPEQ_UI)) {
            creatEqAudioEffects();
            initEqAudioEffects();
        }

        if (isAudioEffectOn(AudioEffectManager.DEBUG_BALANCE_UI)) {
            creatBalanceAudioEffects();
            initBalanceAudioEffects();
        }

        if (isAudioEffectOn(AudioEffectManager.DEBUG_TREBLEBASS_UI)) {
            creatTrebleBassAudioEffects();
            initTrebleBassAudioEffects();
        }

        if (isAudioEffectOn(AudioEffectManager.DEBUG_VIRTUAL_SURROUND_UI)) {
            creatVirtualSurroundAudioEffects();
            initVirtualSurroundAudioEffects();
        }

        if (isAudioEffectOn(AudioEffectManager.DEBUG_DPE_UI)) {
            creatDpeAudioEffect();
            initDpeAudioEffect();
        }
        if (isAudioEffectOn(AudioEffectManager.DEBUG_VIRTUAL_X_UI)) {
            mSupportVirtualX = creatVirtualXAudioEffects();
            initVirtualXAudioEffects();
        }

        if (isAudioEffectOn(AudioEffectManager.DEBUG_DAP_2_UI)) {
            creatDapAudioEffect();
            initDapAudioEffect();
        }
    }
    public void setAudioEffectOnByIndex (int id, boolean dbSwitch) {
        if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "setAudioEffectOnByIndex id:" + id + ", dbSwitch:" + dbSwitch);

        int value = dbSwitch ? 1 : 0;
        switch (id) {
            case AudioEffectManager.DEBUG_HPEQ_UI:
                if (value == AudioEffectManager.DEBUG_UI_ON) {
                    creatEqAudioEffects();
                    initEqAudioEffects();
                } else {
                    cleanupEqAudioEffects();
                }
                break;
            case AudioEffectManager.DEBUG_BALANCE_UI:
                if (value == AudioEffectManager.DEBUG_UI_ON) {
                    creatBalanceAudioEffects();
                    initBalanceAudioEffects();
                } else {
                    cleanupBalanceAudioEffects();
                }
                break;
            case AudioEffectManager.DEBUG_TREBLEBASS_UI:
                if (value == AudioEffectManager.DEBUG_UI_ON) {
                    creatTrebleBassAudioEffects();
                    initTrebleBassAudioEffects();
                } else {
                    cleanupTrebleBassAudioEffects();
                }
                break;
            case AudioEffectManager.DEBUG_VIRTUAL_SURROUND_UI:
                if (value == AudioEffectManager.DEBUG_UI_ON) {
                    creatVirtualSurroundAudioEffects();
                    initVirtualSurroundAudioEffects();
                } else {
                    cleanupVirtualSurroundAudioEffects();
                }
                break;
            case AudioEffectManager.DEBUG_DPE_UI:
                if (value == AudioEffectManager.DEBUG_UI_ON) {
                    creatDpeAudioEffect();
                    initDpeAudioEffect();
                } else {
                    cleanupDpeAudioEffects();
                }
                break;
            case AudioEffectManager.DEBUG_VIRTUAL_X_UI:
                if (value == AudioEffectManager.DEBUG_UI_ON) {
                    creatVirtualXAudioEffects();
                    initVirtualXAudioEffects();
                } else {
                    cleanupVirtualXAudioEffects();
                }
                break;
            case AudioEffectManager.DEBUG_DAP_2_UI:
                if (value == AudioEffectManager.DEBUG_UI_ON) {
                    creatDapAudioEffect();
                    initDapAudioEffect();
                } else {
                    cleanupDapAudioEffects();
                }
                break;
            default:
                Log.e(TAG, "setAudioEffectOnByIndex id:" + id + " is invalid!");
                break;
        }
    }

    public void setAudioEffectOn (int id, boolean dbSwitch) {
        if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "setAudioEffectOn id:" + id + ", dbSwitch:" + dbSwitch);

        switch (id) {
            case AudioEffectManager.DEBUG_HPEQ_UI:
                setAudioEffectOnByIndex(AudioEffectManager.DEBUG_HPEQ_UI, dbSwitch);
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_HPEQ_DEBUG, dbSwitch ? 1 : 0);
                break;
            case AudioEffectManager.DEBUG_BALANCE_UI:
                setAudioEffectOnByIndex(AudioEffectManager.DEBUG_BALANCE_UI, dbSwitch);
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BALANCE_DEBUG, dbSwitch ? 1 : 0);
                break;
            case AudioEffectManager.DEBUG_TREBLEBASS_UI:
                setAudioEffectOnByIndex(AudioEffectManager.DEBUG_TREBLEBASS_UI, dbSwitch);
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_TREBLEBASS_DEBUG, dbSwitch ? 1 : 0);
                break;
            case AudioEffectManager.DEBUG_VIRTUAL_SURROUND_UI:
                setAudioEffectOnByIndex(AudioEffectManager.DEBUG_VIRTUAL_SURROUND_UI, dbSwitch);
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_VIRTUAL_SURROUND_DEBUG, dbSwitch ? 1 : 0);
                break;
            case AudioEffectManager.DEBUG_DPE_UI:
                setAudioEffectOnByIndex(AudioEffectManager.DEBUG_DPE_UI, dbSwitch);
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_DEBUG, dbSwitch ? 1 : 0);
                break;
            case AudioEffectManager.DEBUG_VIRTUAL_X_UI:
                setAudioEffectOnByIndex(AudioEffectManager.DEBUG_VIRTUAL_X_UI, dbSwitch);
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_VIRTUAL_X_DEBUG, dbSwitch ? 1 : 0);
                break;
            case AudioEffectManager.DEBUG_DAP_2_UI:
                setAudioEffectOnByIndex(AudioEffectManager.DEBUG_DAP_2_UI, dbSwitch);
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_2_DEBUG, dbSwitch ? 1 : 0);
                break;
            default:
                Log.e(TAG, "setAudioEffectOn id:" + id + " is invalid!");
                break;
        }
    }

    public boolean isAudioEffectOn(int id) {
        int value = -1;
        switch (id) {
            case AudioEffectManager.DEBUG_HPEQ_UI:
                if (DroidLogicUtils.isTv()) {
                    value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_HPEQ_DEBUG, AudioEffectManager.DEBUG_UI_ON);
                } else {
                    value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_HPEQ_DEBUG, AudioEffectManager.DEBUG_UI_OFF);
                }
                break;
            case AudioEffectManager.DEBUG_BALANCE_UI:
                if (DroidLogicUtils.isTv()) {
                    value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BALANCE_DEBUG, AudioEffectManager.DEBUG_UI_ON);
                } else {
                    value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BALANCE_DEBUG, AudioEffectManager.DEBUG_UI_OFF);
                }
                break;
            case AudioEffectManager.DEBUG_TREBLEBASS_UI:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_TREBLEBASS_DEBUG, AudioEffectManager.DEBUG_UI_OFF);
                break;
            case AudioEffectManager.DEBUG_VIRTUAL_SURROUND_UI:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_VIRTUAL_SURROUND_DEBUG, AudioEffectManager.DEBUG_UI_OFF);
                break;
            case AudioEffectManager.DEBUG_DPE_UI:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DPE_DEBUG, AudioEffectManager.DEBUG_UI_OFF);
                break;
            case AudioEffectManager.DEBUG_VIRTUAL_X_UI:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_VIRTUAL_X_DEBUG, AudioEffectManager.DEBUG_UI_OFF);
                break;
            case AudioEffectManager.DEBUG_DAP_2_UI:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DAP_2_DEBUG, AudioEffectManager.DEBUG_UI_OFF);
                break;
            default:
                Log.e(TAG, "isAudioEffectOn id:" + id + " is invalid!");
                break;
        }
        if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "isAudioEffectOn id:" + id + ", value:" + value);

        return value == AudioEffectManager.DEBUG_UI_ON;
    }

    public void setHpeqBandNum (int id, int value) {
        if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "setHpeqBandNum id:" + id + ", value:" + value);

        switch (id) {
            case AudioEffectManager.DEBUG_HPEQ_BAND_NUM_UI:
                mSoundMode.setParameter(PARAM_EQ_BAND_NUM, value);
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_HPEQ_BAND_NUM_DEBUG, value);
                break;
            default:
                Log.e(TAG, "setHpeqBandNum id:" + id + " is invalid!");
                break;
        }
    }

    public int getHpeqBandNum(int id) {
        int value = 0;
        switch (id) {
            case AudioEffectManager.DEBUG_HPEQ_BAND_NUM_UI:
                value = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_HPEQ_BAND_NUM_DEBUG, AudioEffectManager.HPEQ_5_BAND);
                break;
            default:
                Log.e(TAG, "getHpeqBandNum id:" + id + " is invalid!");
                break;
        }
        if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "getHpeqBandNum id:" + id + ", value:" + value);

        return value;
    }

}

