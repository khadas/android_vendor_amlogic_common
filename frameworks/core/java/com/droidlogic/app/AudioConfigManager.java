/*
 * Copyright (C) 2009 The Android Open Source Project
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
 * limitations under the License.
 */

package com.droidlogic.app;

import android.util.Log;
import android.content.Context;
import android.provider.Settings;
import android.media.AudioManager;

import java.math.BigDecimal;
import java.text.DecimalFormat;

import com.droidlogic.app.DroidLogicUtils;

public class AudioConfigManager {
    private static final String TAG                         = "AudioConfigManager";
    private Context mContext;
    private static AudioConfigManager mAudioConfigManager = null;
    private AudioManager mAudioManager;

    /* [setAudioOutputSpeakerDelay / setAudioOutputSpdifDelay / setAudioOutputHeadphoneDelay/
     * setAudioPrescale] output delay source define
     */
    public static final int AUDIO_OUTPUT_DELAY_SOURCE_ATV               = 0;
    public static final int AUDIO_OUTPUT_DELAY_SOURCE_DTV               = 1;
    public static final int AUDIO_OUTPUT_DELAY_SOURCE_AV                = 2;
    public static final int AUDIO_OUTPUT_DELAY_SOURCE_HDMI              = 3;
    public static final int AUDIO_OUTPUT_DELAY_SOURCE_MEDIA             = 4;
    public static final int AUDIO_OUTPUT_DELAY_SOURCE_MAX               = 5;
    public static final String PROP_AUDIO_DELAY_ENABLED                 = "persist.vendor.tv.audio.delay.enabled";

    public static final int HAL_AUDIO_OUT_DEV_DELAY_MIN                 = 0;       // ms
    public static final int HAL_AUDIO_OUT_DEV_DELAY_MAX                 = 200;     // ms
    public static final int HAL_AUDIO_OUT_DEV_DELAY_DEFAULT             = 0;       // ms

    // refer to audio hal aml_audio_delay_type_e enum
    private static final int HAL_AUDIO_OUT_DEV_DELAY_SPEAKER            = 0;
    private static final int HAL_AUDIO_OUT_DEV_DELAY_SPDIF              = 1;
    private static final int HAL_AUDIO_OUT_DEV_DELAY_HEADPHONE          = 2;
    private static final int HAL_AUDIO_OUT_DEV_DELAY_ALL                = 3;

    private static final String PARAM_HAL_AUDIO_OUT_DEV_DELAY           = "hal_param_out_dev_delay_time_ms";
    private static final String PARAM_HAL_AUDIO_PRESCALE                = "SOURCE_GAIN";

    private static final int AUDIO_PRESCALE_MIN                         = -150;    // -15 dB
    private static final int AUDIO_PRESCALE_MAX                         = 150;     // 15 dB
    private static final int[] AUDIO_PRESCALE_DEFAULT_ARRAY             = {0, 0, 0, 0, 0}; // ATV, DTV, AV, HDMI, MEDIA, range: [-150 - 150]

    public static final String DB_ID_AUDIO_OUTPUT_ALL_DELAY                     = "db_id_audio_output_all_delay";
    private static final String[] DB_ID_AUDIO_OUTPUT_SPEAKER_DELAY_ARRAY    = {
            "db_id_audio_output_speaker_delay_atv",
            "db_id_audio_output_speaker_delay_dtv",
            "db_id_audio_output_speaker_delay_av",
            "db_id_audio_output_speaker_delay_hdmi",
            "db_id_audio_output_speaker_delay_media",
    };
    private static final String[] DB_ID_AUDIO_OUTPUT_SPDIF_DELAY_ARRAY       = {
            "db_id_audio_output_spdif_delay_atv",
            "db_id_audio_output_spdif_delay_dtv",
            "db_id_audio_output_spdif_delay_av",
            "db_id_audio_output_spdif_delay_hdmi",
            "db_id_audio_output_spdif_delay_media",
    };
    private static final String[] DB_ID_AUDIO_OUTPUT_HEADPHONE_DELAY_ARRAY   = {
            "db_id_audio_output_headphone_delay_atv",
            "db_id_audio_output_headphone_delay_dtv",
            "db_id_audio_output_headphone_delay_av",
            "db_id_audio_output_headphone_delay_hdmi",
            "db_id_audio_output_headphone_delay_media",
    };
    private static final String[] DB_ID_AUDIO_PRESCALE_ARRAY       = {
            "db_id_audio_prescale_atv",
            "db_id_audio_prescale_dtv",
            "db_id_audio_prescale_av",
            "db_id_audio_prescale_hdmi",
            "db_id_audio_prescale_media",
    };

    public static AudioConfigManager getInstance(Context context) {
        synchronized (AudioConfigManager.class) {
            if (mAudioConfigManager == null) {
                mAudioConfigManager = new AudioConfigManager(context);
            }
        }
        return mAudioConfigManager;
    }

    private AudioConfigManager(Context context) {
        mContext = context;
        mAudioManager = (AudioManager) context.getSystemService(context.AUDIO_SERVICE);
    }

    private void setAudioOutputDelayToHal(int output, int delayMs) {
        /* High 16 - bit expression type, low 16 - bit expression delay time. refer to audio hal */
        mAudioManager.setParameters(PARAM_HAL_AUDIO_OUT_DEV_DELAY + "=" + (output << 16 | delayMs));
    }

    private static final String DB_ID_TV_SOURCE_TYPE     = "db_id_tv_source_type";
    public static final int DEVICE_ID_ADTV               = 16;
    private int getTvSourceType() {
        return Settings.Global.getInt(mContext.getContentResolver(), DB_ID_TV_SOURCE_TYPE, DEVICE_ID_ADTV);
    }

    public void setAudioOutputSpeakerDelay(int source, int delayMs) {
        if (source < AUDIO_OUTPUT_DELAY_SOURCE_ATV || source >= AUDIO_OUTPUT_DELAY_SOURCE_MAX) {
            Log.w(TAG, "setAudioOutputSpeakerDelay: unsupport delay source:" + source + ", min:" + AUDIO_OUTPUT_DELAY_SOURCE_ATV
                    + ", max:" + (AUDIO_OUTPUT_DELAY_SOURCE_MAX-1));
            return;
        }
        if (delayMs < HAL_AUDIO_OUT_DEV_DELAY_MIN || delayMs > HAL_AUDIO_OUT_DEV_DELAY_MAX) {
            Log.w(TAG, "unsupport speaker delay time:" + delayMs + "ms, min:" + HAL_AUDIO_OUT_DEV_DELAY_MIN + "ms, max:"
                    + HAL_AUDIO_OUT_DEV_DELAY_MAX + "ms, now use max value");
            delayMs = HAL_AUDIO_OUT_DEV_DELAY_MAX;
        }
        if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "setAudioOutputSpeakerDelay delay source:" + source + ", delayMs:" + delayMs);
        int currentTvSource = getTvSourceType();
        if (currentTvSource == source) {
            setAudioOutputDelayToHal(HAL_AUDIO_OUT_DEV_DELAY_SPEAKER, delayMs);
        } else {
            Log.i(TAG, "setAudioOutputSpeakerDelay current source:" + currentTvSource + " is not the same as the set source:" + source + ", only save to DB");
        }
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_AUDIO_OUTPUT_SPEAKER_DELAY_ARRAY[source], delayMs);
    }

    public int getAudioOutputSpeakerDelay(int source) {
        if (source < AUDIO_OUTPUT_DELAY_SOURCE_ATV || source >= AUDIO_OUTPUT_DELAY_SOURCE_MAX) {
            Log.w(TAG, "getAudioOutputSpeakerDelay unsupport delay source:" + source + ", min:" + AUDIO_OUTPUT_DELAY_SOURCE_ATV
                    + ", max:" + (AUDIO_OUTPUT_DELAY_SOURCE_MAX-1));
            return -1;
        }
        int delayMs = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_AUDIO_OUTPUT_SPEAKER_DELAY_ARRAY[source], HAL_AUDIO_OUT_DEV_DELAY_DEFAULT);
        if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "getAudioOutputSpeakerDelay source:" + source + ", delayMs:" + delayMs);
        return delayMs;
    }

    public void setAudioOutputSpdifDelay(int source, int delayMs) {
        if (source < AUDIO_OUTPUT_DELAY_SOURCE_ATV || source >= AUDIO_OUTPUT_DELAY_SOURCE_MAX) {
            Log.w(TAG, "setAudioOutputSpdifDelay unsupport delay source:" + source + ", min:" + AUDIO_OUTPUT_DELAY_SOURCE_ATV
                    + ", max:" + (AUDIO_OUTPUT_DELAY_SOURCE_MAX-1));
            return;
        }
        if (delayMs < HAL_AUDIO_OUT_DEV_DELAY_MIN || delayMs > HAL_AUDIO_OUT_DEV_DELAY_MAX) {
            Log.w(TAG, "unsupport spdif delay time:" + delayMs + "ms, min:" + HAL_AUDIO_OUT_DEV_DELAY_MIN + "ms, max:"
                    + HAL_AUDIO_OUT_DEV_DELAY_MAX + "ms, now use max value");
            delayMs = HAL_AUDIO_OUT_DEV_DELAY_MAX;
        }
        if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "setAudioOutputSpdifDelay delay source:" + source + ", delayMs:" + delayMs);
        int currentTvSource = getTvSourceType();
        if (currentTvSource == source) {
            setAudioOutputDelayToHal(HAL_AUDIO_OUT_DEV_DELAY_SPDIF, delayMs);
        } else {
            Log.i(TAG, "setAudioOutputSpdifDelay current source:" + currentTvSource + " is not the same as the set source:" + source + ", only save to DB");
        }
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_AUDIO_OUTPUT_SPDIF_DELAY_ARRAY[source], delayMs);
    }

    public int getAudioOutputSpdifDelay(int source) {
        if (source < AUDIO_OUTPUT_DELAY_SOURCE_ATV || source >= AUDIO_OUTPUT_DELAY_SOURCE_MAX) {
            Log.w(TAG, "getAudioOutputSpdifDelay unsupport delay source:" + source + ", min:" + AUDIO_OUTPUT_DELAY_SOURCE_ATV
                    + ", max:" + (AUDIO_OUTPUT_DELAY_SOURCE_MAX-1));
            return -1;
        }
        int delayMs = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_AUDIO_OUTPUT_SPDIF_DELAY_ARRAY[source], HAL_AUDIO_OUT_DEV_DELAY_DEFAULT);
        if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "getAudioOutputSpdifDelay source:" + source + ", delayMs:" + delayMs);
        return delayMs;
    }

    public void setAudioOutputHeadphoneDelay(int source, int delayMs) {
        if (source < AUDIO_OUTPUT_DELAY_SOURCE_ATV || source >= AUDIO_OUTPUT_DELAY_SOURCE_MAX) {
            Log.w(TAG, "setAudioOutputHeadphoneDelay unsupport delay source:" + source + ", min:" + AUDIO_OUTPUT_DELAY_SOURCE_ATV
                    + ", max:" + (AUDIO_OUTPUT_DELAY_SOURCE_MAX-1));
            return;
        }
        if (delayMs < HAL_AUDIO_OUT_DEV_DELAY_MIN || delayMs > HAL_AUDIO_OUT_DEV_DELAY_MAX) {
            Log.w(TAG, "unsupport spdif delay time:" + delayMs + "ms, min:" + HAL_AUDIO_OUT_DEV_DELAY_MIN + "ms, max:"
                    + HAL_AUDIO_OUT_DEV_DELAY_MAX + "ms, now use max value");
            delayMs = HAL_AUDIO_OUT_DEV_DELAY_MAX;
        }
        if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "setAudioOutputHeadphoneDelay delay source:" + source + ", delayMs:" + delayMs);
        int currentTvSource = getTvSourceType();
        if (currentTvSource == source) {
            setAudioOutputDelayToHal(HAL_AUDIO_OUT_DEV_DELAY_HEADPHONE, delayMs);
        } else {
            Log.i(TAG, "setAudioOutputHeadphoneDelay current source:" + currentTvSource + " is not the same as the set source:" + source + ", only save to DB");
        }
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_AUDIO_OUTPUT_HEADPHONE_DELAY_ARRAY[source], delayMs);
    }

    public int getAudioOutputHeadphoneDelay(int source) {
        if (source < AUDIO_OUTPUT_DELAY_SOURCE_ATV || source >= AUDIO_OUTPUT_DELAY_SOURCE_MAX) {
            Log.w(TAG, "getAudioOutputHeadphoneDelay unsupport delay source:" + source + ", min:" + AUDIO_OUTPUT_DELAY_SOURCE_ATV
                    + ", max:" + (AUDIO_OUTPUT_DELAY_SOURCE_MAX-1));
            return -1;
        }
        int delayMs = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_AUDIO_OUTPUT_HEADPHONE_DELAY_ARRAY[source], HAL_AUDIO_OUT_DEV_DELAY_DEFAULT);
        if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "getAudioOutputHeadphoneDelay source:" + source + ", delayMs:" + delayMs);
        return delayMs;
    }

    public void setAudioOutputAllDelay(int delayMs) {
        if (delayMs < HAL_AUDIO_OUT_DEV_DELAY_MIN || delayMs > HAL_AUDIO_OUT_DEV_DELAY_MAX) {
            Log.w(TAG, "unsupport delay time:" + delayMs + "ms, min:" + HAL_AUDIO_OUT_DEV_DELAY_MIN + "ms, max:"
                    + HAL_AUDIO_OUT_DEV_DELAY_MAX + "ms, now use max value");
            delayMs = HAL_AUDIO_OUT_DEV_DELAY_MAX;
        }
        if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "setAudioOutputAllDelay delay " + ", delayMs:" + delayMs);
        setAudioOutputDelayToHal(HAL_AUDIO_OUT_DEV_DELAY_ALL, delayMs);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_AUDIO_OUTPUT_ALL_DELAY, delayMs);
    }

    public int getAudioOutputAllDelay() {
        int delayMs = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_AUDIO_OUTPUT_ALL_DELAY, HAL_AUDIO_OUT_DEV_DELAY_DEFAULT);
        if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "getAudioOutputAllDelay, delayMs:" + delayMs);
        return delayMs;
    }

    public void setAudioPrescale(int source,int value) {
        if (source < AUDIO_OUTPUT_DELAY_SOURCE_ATV || source >= AUDIO_OUTPUT_DELAY_SOURCE_MAX) {
            Log.w(TAG, "setAudioPrescale unsupport delay source:" + source + ", min:" + AUDIO_OUTPUT_DELAY_SOURCE_ATV
                    + ", max:" + (AUDIO_OUTPUT_DELAY_SOURCE_MAX-1));
            return;
        }
        if (value < AUDIO_PRESCALE_MIN || value > AUDIO_PRESCALE_MAX) {
            Log.w(TAG, "unsupport audio prescale:" + value + ", min:" + AUDIO_PRESCALE_MIN + ", max:" + AUDIO_PRESCALE_MAX);
            return;
        }
        if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "setAudioPrescale source:" + source + ", value:" + value);
        try {
            StringBuffer parameter;
            String realValue = "";
            DecimalFormat decimalFormat = new DecimalFormat("0.0");
            Settings.Global.putInt(mContext.getContentResolver(), DB_ID_AUDIO_PRESCALE_ARRAY[source], value);

            // packgeing "SOURCE_GAIN=1.0 1.0 1.0 1.0 1.0" [atv,dtv,hdmi,av,media]
            parameter = new StringBuffer(PARAM_HAL_AUDIO_PRESCALE + "=");
            //UI -150 - 150, audio_hal -15 - 15 db
            int tempParamter = 1;
            tempParamter = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_AUDIO_PRESCALE_ARRAY[AUDIO_OUTPUT_DELAY_SOURCE_ATV],
                    AUDIO_PRESCALE_DEFAULT_ARRAY[AUDIO_OUTPUT_DELAY_SOURCE_ATV]);
            realValue = decimalFormat.format((float) tempParamter / 10);
            parameter.append(realValue + " ");

            tempParamter = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_AUDIO_PRESCALE_ARRAY[AUDIO_OUTPUT_DELAY_SOURCE_DTV],
                    AUDIO_PRESCALE_DEFAULT_ARRAY[AUDIO_OUTPUT_DELAY_SOURCE_DTV]);
            realValue = decimalFormat.format((float) tempParamter / 10);
            parameter.append(realValue + " ");

            tempParamter = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_AUDIO_PRESCALE_ARRAY[AUDIO_OUTPUT_DELAY_SOURCE_HDMI],
                    AUDIO_PRESCALE_DEFAULT_ARRAY[AUDIO_OUTPUT_DELAY_SOURCE_HDMI]);
            realValue = decimalFormat.format((float) tempParamter / 10);
            parameter.append(realValue + " ");

            tempParamter = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_AUDIO_PRESCALE_ARRAY[AUDIO_OUTPUT_DELAY_SOURCE_AV],
                    AUDIO_PRESCALE_DEFAULT_ARRAY[AUDIO_OUTPUT_DELAY_SOURCE_AV]);
            realValue = decimalFormat.format((float) tempParamter / 10);
            parameter.append(realValue + " ");

            tempParamter = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_AUDIO_PRESCALE_ARRAY[AUDIO_OUTPUT_DELAY_SOURCE_MEDIA],
                    AUDIO_PRESCALE_DEFAULT_ARRAY[AUDIO_OUTPUT_DELAY_SOURCE_MEDIA]);
            realValue = decimalFormat.format((float) tempParamter / 10);
            parameter.append(realValue + " ");
            if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "setAudioPrescale setParameters:" + parameter.toString());
            mAudioManager.setParameters(parameter.toString());
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public int getAudioPrescale(int source) {
        if (source < AUDIO_OUTPUT_DELAY_SOURCE_ATV || source >= AUDIO_OUTPUT_DELAY_SOURCE_MAX) {
            Log.w(TAG, "getAudioPrescaleStatus: unsupport delay source:" + source + ", min:" + AUDIO_OUTPUT_DELAY_SOURCE_ATV
                    + ", max:" + (AUDIO_OUTPUT_DELAY_SOURCE_MAX-1));
            return -1;
        }

        int saveResult = 0;
        BigDecimal mBigDecimalBase = new BigDecimal(Float.toString(10.00f));
        BigDecimal mBigDecimal = new BigDecimal(0.0f);
        String value = mAudioManager.getParameters("SOURCE_GAIN");//atv,dtv,hdmi,av,media
        value.trim();
        if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "getAudioPrescale hal param:" + value);
        saveResult = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_AUDIO_PRESCALE_ARRAY[source],
                AUDIO_PRESCALE_DEFAULT_ARRAY[AUDIO_OUTPUT_DELAY_SOURCE_ATV]);
        String[] subStrings = value.split(" ");//"source_gain = 1.0 1.0 1.0 1.0 1.0"
        if (subStrings.length == 7) {
            int driverValue = 0;
            mBigDecimal = new BigDecimal(subStrings[subStrings.length - 5].substring(0,3));
            driverValue = (int) mBigDecimal.multiply(mBigDecimalBase).floatValue();
            if (driverValue != saveResult) {
                Log.w(TAG, "getAudioPrescaleStatus driverValue:" + driverValue + ", saveResult:" + saveResult);
            }
        } else {
            Log.w(TAG, "getAudioPrescaleStatus param length:" + subStrings.length + " invalid");
        }
        if (DroidLogicUtils.getAudioDebugEnable()) Log.d(TAG, "getAudioPrescale source:" + source + ", value:" + saveResult);
        return saveResult;
    }

    public void initAudioConfigSettings() {
        // refresh db delay of media to hal
        setAudioOutputSpeakerDelay(AUDIO_OUTPUT_DELAY_SOURCE_MEDIA, getAudioOutputSpeakerDelay(AUDIO_OUTPUT_DELAY_SOURCE_MEDIA));
        setAudioOutputSpdifDelay(AUDIO_OUTPUT_DELAY_SOURCE_MEDIA, getAudioOutputSpdifDelay(AUDIO_OUTPUT_DELAY_SOURCE_MEDIA));
        setAudioOutputHeadphoneDelay(AUDIO_OUTPUT_DELAY_SOURCE_MEDIA, getAudioOutputHeadphoneDelay(AUDIO_OUTPUT_DELAY_SOURCE_MEDIA));
        setAudioOutputAllDelay(getAudioOutputAllDelay());
        // refresh db prescale of all source to hal (set one prescale, at the same time the others will be set)
        setAudioPrescale(AUDIO_OUTPUT_DELAY_SOURCE_ATV, getAudioPrescale(AUDIO_OUTPUT_DELAY_SOURCE_ATV));
    }
}

