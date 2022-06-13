/******************************************************************
 *
 *Copyright (C)2012 Amlogic, Inc.
 *
 *Licensed under the Apache License, Version 2.0 (the "License");
 *you may not use this file except in compliance with the License.
 *You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing, software
 *distributed under the License is distributed on an "AS IS" BASIS,
 *WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *See the License for the specific language governing permissions and
 *limitations under the License.
 ******************************************************************/
package com.droidlogic.updater.ui;

import android.content.Context;
import android.graphics.PixelFormat;
import android.os.Handler;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.WindowManager;
import android.widget.TextView;
import android.widget.ProgressBar;
import com.droidlogic.updater.R;

public class RemoteView {
    private View mFloatView;
    private WindowManager wm;
    private WindowManager.LayoutParams mParams;
    private boolean isShowing;
    private Handler mHandler;
    private static RemoteView mInstance;

    public synchronized static RemoteView getInstance() {
        if (mInstance == null) {
            mInstance = new RemoteView();
        }
        return mInstance;
    }

    private RemoteView() {
    }

    public void createView(Context context) {
        LayoutInflater inflater = LayoutInflater.from(context);
        mFloatView = inflater.inflate(R.layout.scene_layout, null, false);
        wm = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
        mParams = new WindowManager.LayoutParams();
        mParams.type = WindowManager.LayoutParams.TYPE_SYSTEM_OVERLAY;
        mParams.format = PixelFormat.TRANSLUCENT;
        mParams.gravity = Gravity.BOTTOM|Gravity.RIGHT;
        mParams.flags = WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL | WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE | WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE;
        mParams.width = 120;
        // mParams.height = 30;
        mParams.width = WindowManager.LayoutParams.WRAP_CONTENT;
        mParams.height = WindowManager.LayoutParams.WRAP_CONTENT;
     //   mParams.x = 100;
       // mParams.y = 100;
        isShowing = false;
    }

    public void adjustParam() {
       wm.updateViewLayout(mFloatView, mParams);
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


    public void updateUI(String value, boolean visible, int progress) {
        Log.d("V","updateUI"+value+" visible:"+(mFloatView == null)+" "+progress);
        if (mFloatView == null) return;
        TextView status =
                (TextView) mFloatView.findViewById(R.id.progress_status);
        status.setText(value);
        if (visible) {
            ProgressBar mProgressbar =(ProgressBar) mFloatView.findViewById(R.id.progress);
            mProgressbar.setIndeterminate(false);
            mProgressbar.setProgress(progress);
        }
    }

    public boolean isShow() {
        return isShowing;
    }

    public boolean isCreated() {
        return !(mFloatView == null);
    }

    public View getRootView() {
        return mFloatView;
    }
}