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

package com.droidlogic;

import android.content.Context;
import android.graphics.PixelFormat;
import android.os.Handler;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.WindowManager;
import android.widget.TextView;

import com.droidlogic.app.SystemControlManager;

public class AIRemoteView {

    private View mFloatView;
    private WindowManager wm;
    private WindowManager.LayoutParams mParams;
    private boolean isShowing;
    private Handler mHandler;
    private TextView mShowOTTInfo;
    private TextView mShowTVInfo;
    private static AIRemoteView mInstance;

    public synchronized static AIRemoteView getInstance() {
        if (mInstance == null) {
            mInstance = new AIRemoteView();
        }
        return mInstance;
    }

    private AIRemoteView() {
    }

    public void createView(Context context) {
        LayoutInflater inflater = LayoutInflater.from(context);
        mFloatView = inflater.inflate(R.layout.scene_layout, null, false);
        wm = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
        mParams = new WindowManager.LayoutParams();
        mParams.type = WindowManager.LayoutParams.TYPE_SYSTEM_OVERLAY;
        mParams.gravity = Gravity.LEFT | Gravity.TOP;
        mParams.format = PixelFormat.TRANSLUCENT;
        mParams.flags = WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL
                | WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE
                | WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE;

        mShowOTTInfo = (TextView) mFloatView.findViewById(R.id.show_info_ott);
        mShowTVInfo = (TextView) mFloatView.findViewById(R.id.show_info_tv);
        updateUI("AI PQ invalid value...");

        isShowing = false;

    }

    public void show() {
        if (!isShowing) {
            wm.addView(mFloatView, mParams);
            isShowing = true;
        }
    }

    public void hide() {
        if (isShowing) {
            wm.removeViewImmediate(mFloatView);
            isShowing = false;
        }
    }

    public void updateUI(String value) {
        if (isTvFeature()) {
            updateUI(value, mShowTVInfo);
        } else {
            updateUI(value, mShowOTTInfo);
        }
    }

    private void updateUI(String value, TextView textView) {
        if (mFloatView == null) return;
        if (value.equals(textView.getText().toString())) {
            return;
        }
        textView.setText(value);
    }

    public boolean isShow() {
        return isShowing;
    }

    public boolean isCreated() {
        return !(mFloatView == null);
    }

    public static boolean isTvFeature() {
        SystemControlManager sm = SystemControlManager.getInstance();
        return ("1".equals(sm.getPropertyString("ro.vendor.platform.is.tv", "")));
    }

}
