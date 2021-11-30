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

package com.android.tv.settings.display.screen;

import android.os.Bundle;
import android.os.Handler;
import android.os.PowerManager;
import android.os.Looper;
import android.provider.Settings;
import android.provider.Settings.System;
import android.content.ContentResolver;
import android.content.Context;
import android.database.ContentObserver;
import androidx.leanback.preference.LeanbackPreferenceFragment;
import androidx.preference.Preference;
import androidx.preference.PreferenceCategory;
import android.util.Log;
import android.text.TextUtils;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;

import com.android.tv.settings.R;

public class BrightnessFragment extends LeanbackPreferenceFragment implements SeekBar.OnSeekBarChangeListener {

    private static final String TAG = "BrightnessFragment";
    private static final int DEFAULT_BRIGHTNESS = 102;


    private SeekBar seekbar_brightness;

    private PowerManager powerManager;
    private ContentResolver mContentResolver;
    private Context mContext;
    private int mMinBrightness;
    private int mMaxBrightness;
    private int mCurrentBrightness;


    private boolean isSeekBarInited = false;


    private ContentObserver mBrightnessObserver =
        new ContentObserver(new Handler(Looper.getMainLooper())) {
            @Override
            public void onChange(boolean selfChange) {
                 updatedState();
            }
        };

    public static BrightnessFragment newInstance() {
        return new BrightnessFragment();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mContext = getContext();
        mContentResolver = mContext.getContentResolver();
    }

    @Override
    public void onResume() {
        super.onResume();
        mContentResolver.registerContentObserver(System.getUriFor(System.SCREEN_BRIGHTNESS), false, mBrightnessObserver);
    }

    @Override
    public void onPause() {
        super.onPause();
        mContentResolver.unregisterContentObserver(mBrightnessObserver);
    }


    private void updatedState() {
        int progress = System.getInt(mContentResolver, System.SCREEN_BRIGHTNESS, DEFAULT_BRIGHTNESS);
        seekbar_brightness.setProgress(progress);
    }

    @Override
    public View onCreateView (LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = inflater.inflate(R.xml.screen_brightness, container, false);
        return view;
    }

    @Override
    public void onViewCreated (View view, Bundle savedInstanceState) {
        powerManager = (PowerManager) mContext.getSystemService(Context.POWER_SERVICE);
        mMinBrightness = powerManager.getMinimumScreenBrightnessSetting();
        mMaxBrightness = powerManager.getMaximumScreenBrightnessSetting();
        mCurrentBrightness = System.getInt(mContentResolver, System.SCREEN_BRIGHTNESS, DEFAULT_BRIGHTNESS);
        initSeekBar(view);
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {

    }

    private void initSeekBar(View view) {
        int status = -1;
        seekbar_brightness = (SeekBar) view.findViewById(R.id.seekbar_brightness);
        seekbar_brightness.setEnabled(true);
        seekbar_brightness.setMax(mMaxBrightness-mMinBrightness);
        seekbar_brightness.setProgress(mCurrentBrightness+mMinBrightness);
        seekbar_brightness.requestFocus();
        seekbar_brightness.setOnSeekBarChangeListener(this);
        isSeekBarInited = true;
    }

    @Override
    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
        if (!isSeekBarInited) {
            return;
        }
        switch (seekBar.getId()) {
            case R.id.seekbar_brightness:{
                if (fromUser) {
                    System.putInt(mContentResolver, System.SCREEN_BRIGHTNESS, progress+mMinBrightness);
                }
                break;
            }
            default:
                break;
        }
    }

    @Override
    public void onStopTrackingTouch(SeekBar seekBar) {

    }

    @Override
    public void onStartTrackingTouch(SeekBar seekBar) {

    }

}
