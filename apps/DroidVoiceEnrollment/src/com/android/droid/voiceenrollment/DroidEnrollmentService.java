/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.droid.voiceenrollment;

import android.app.Activity;
import android.hardware.soundtrigger.SoundTrigger;
import android.hardware.soundtrigger.SoundTrigger.Keyphrase;
import android.hardware.soundtrigger.SoundTrigger.KeyphraseSoundModel;
import android.os.Bundle;
import android.os.UserManager;
import android.util.Log;
import android.view.View;
import android.widget.Toast;
import android.app.Service;
import android.content.Context;
import android.os.IBinder;
import android.content.Intent;


import java.util.Locale;
import java.util.Random;
import java.util.UUID;

/**
 * TODO: must be transitioned to a service.
 */

public class DroidEnrollmentService extends Service {

    private static final String TAG = "DroidEnrollmentService";
    private Context mContext;


    private static final boolean DBG = false;

    /** Keyphrase related constants, must match those defined in enrollment_application.xml */
    private static final int KEYPHRASE_ID = 101;
    private static final int RECOGNITION_MODES = SoundTrigger.RECOGNITION_MODE_VOICE_TRIGGER;
    private static final String BCP47_LOCALE = "fr-FR";
    private static final String TEXT = "Hello There";

    private EnrollmentUtil mEnrollmentUtil;
    private Random mRandom;

    @Override
    public void onCreate() {
        super.onCreate();
        mContext = this;
        Log.d(TAG, "--onCreate");
    }

    @Override
    public IBinder onBind(Intent intent) {
         Log.d(TAG, "--onBind");
         return null;
     }

     @Override
     public int onStartCommand(Intent intent, int flags, int startId) {
         Log.d(TAG, "--onStartCommand,intent:" + intent);
         Log.d(TAG, "    flags:" + flags);
         Log.d(TAG, "    startId:" + startId);
         return super.onStartCommand(intent, flags, startId);
     }

    /**
     * Called when the user clicks the enroll button.
     * Performs a fresh enrollment.
     */
    public void onEnrollButtonClicked(View v) {
        Keyphrase kp = new Keyphrase(KEYPHRASE_ID, RECOGNITION_MODES,
                Locale.forLanguageTag(BCP47_LOCALE), TEXT,
                new int[] { UserManager.get(this).getProcessUserId() /* current user */});
        UUID modelUuid = UUID.randomUUID();
        // Generate a fake model to push.
        byte[] data = new byte[1024];
        mRandom.nextBytes(data);
        KeyphraseSoundModel soundModel = new KeyphraseSoundModel(modelUuid, null, data,
                new Keyphrase[] { kp });
        boolean status = mEnrollmentUtil.addOrUpdateSoundModel(soundModel);
        if (status) {
            Toast.makeText(
                    this, "Successfully enrolled, model UUID=" + modelUuid, Toast.LENGTH_SHORT)
                    .show();
        } else {
            Toast.makeText(this, "Failed to enroll!!!" + modelUuid, Toast.LENGTH_SHORT).show();
        }
    }

    /**
     * Called when the user clicks the un-enroll button.
     * Clears the enrollment information for the user.
     */
    public void onUnEnrollButtonClicked(View v) {
        KeyphraseSoundModel soundModel = mEnrollmentUtil.getSoundModel(KEYPHRASE_ID, BCP47_LOCALE);
        if (soundModel == null) {
            Toast.makeText(this, "Sound model not found!!!", Toast.LENGTH_SHORT).show();
            return;
        }
        boolean status = mEnrollmentUtil.deleteSoundModel(KEYPHRASE_ID, BCP47_LOCALE);
        if (status) {
            Toast.makeText(this, "Successfully un-enrolled, model UUID=" + soundModel.getUuid(),
                    Toast.LENGTH_SHORT)
                    .show();
        } else {
            Toast.makeText(this, "Failed to un-enroll!!!", Toast.LENGTH_SHORT).show();
        }
    }

    /**
     * Called when the user clicks the re-enroll button.
     * Uses the previously enrolled sound model and makes changes to it before pushing it back.
     */
    public void onReEnrollButtonClicked(View v) {
        KeyphraseSoundModel soundModel = mEnrollmentUtil.getSoundModel(KEYPHRASE_ID, BCP47_LOCALE);
        if (soundModel == null) {
            Toast.makeText(this, "Sound model not found!!!", Toast.LENGTH_SHORT).show();
            return;
        }
        // Generate a fake model to push.
        byte[] data = new byte[2048];
        mRandom.nextBytes(data);
        KeyphraseSoundModel updated = new KeyphraseSoundModel(soundModel.getUuid(),
                soundModel.getVendorUuid(), data, soundModel.getKeyphrases());
        boolean status = mEnrollmentUtil.addOrUpdateSoundModel(updated);
        if (status) {
            Toast.makeText(this, "Successfully re-enrolled, model UUID=" + updated.getUuid(),
                    Toast.LENGTH_SHORT)
                    .show();
        } else {
            Toast.makeText(this, "Failed to re-enroll!!!", Toast.LENGTH_SHORT).show();
        }
    }
}
