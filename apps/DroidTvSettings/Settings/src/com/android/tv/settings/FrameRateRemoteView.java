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

package com.android.tv.settings;

import android.content.Context;
import android.graphics.PixelFormat;
import android.os.Handler;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.WindowManager;
import android.widget.TextView;

public class FrameRateRemoteView {
    private View mFloatView;
    private WindowManager wm;
    private WindowManager.LayoutParams mParams;
    private boolean isShowing;
    private Handler mHandler;
    private static FrameRateRemoteView mInstance;

    public synchronized static FrameRateRemoteView getInstance() {
        if (mInstance == null) {
            mInstance = new FrameRateRemoteView();
        }
        return mInstance;
    }

    public void createView(Context context) {
        LayoutInflater inflater = LayoutInflater.from(context);
        mFloatView = inflater.inflate(R.layout.framerate_layout, null, false);
        wm = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
        mParams = new WindowManager.LayoutParams();
        mParams.type = WindowManager.LayoutParams.TYPE_SYSTEM_OVERLAY;
        mParams.gravity = Gravity.RIGHT | Gravity.TOP;
        mParams.format = PixelFormat.TRANSLUCENT;
        mParams.flags = WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL
                | WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE
                | WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE;

        mParams.width = WindowManager.LayoutParams.WRAP_CONTENT;
        mParams.height = WindowManager.LayoutParams.WRAP_CONTENT;
        mParams.x = 200;
        mParams.y = 100;
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
        if (mFloatView == null) return;
        TextView ShowINFO = (TextView) mFloatView.findViewById(R.id.show_framerate);
        ShowINFO.setText(value);
    }

    public boolean isShow() {
        return isShowing;
    }

    public boolean isCreated() {
        return !(mFloatView == null);
    }
}

