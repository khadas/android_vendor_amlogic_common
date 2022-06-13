/******************************************************************
 *
 *Copyright (C) 2012  Amlogic, Inc.
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

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.view.WindowManager;
import android.widget.TextView;
import android.widget.Toast;
import com.droidlogic.updater.UpdateApplication;
import com.droidlogic.updater.UpdateManager;
import com.droidlogic.updater.util.PrefUtils;
import com.droidlogic.updater.R;
import android.util.Log;
import androidx.annotation.NonNull;
import com.droidlogic.updater.service.PrepareUpdateService;
public class EmptyActivity extends Activity {

    private static final int SHOW_TIME = 5000;
    private HandlerThread mWorkerThread = new HandlerThread("checker");
    private Handler mWorkerHandler;
    private TextView mTextView;
    private PrefUtils mPref;
    private UpdateManager mUpdateEngine;
    private static final int MSG_CHECK  = 0;
    @Override
    protected void onResume() {
        super.onResume();
        mTextView = (TextView)findViewById(R.id.text_result);
    }
    public void onAttachedToWindow() {

        this.getWindow().addFlags(WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE| WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE);
        super.onAttachedToWindow();
    }
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_empty);
        mUpdateEngine = ((UpdateApplication)getApplication()).getUpdateManager();
        mPref = new PrefUtils(this);
        mWorkerThread.start();
        Log.d("ABUpdate","create");
        mWorkerHandler = new Handler(mWorkerThread.getLooper()){
            @Override
            public void dispatchMessage(@NonNull Message msg) {
                switch (msg.what) {
                    case MSG_CHECK:
                        check();
                        break;
                }
            }
        };
        mWorkerHandler.sendEmptyMessage(MSG_CHECK);
    }

    @Override
    protected void onDestroy() {
        mWorkerHandler.removeMessages(MSG_CHECK);
        mWorkerThread.quitSafely();
        super.onDestroy();
    }
    private void check() {
        mPref.disableUI(true, this);
        int mergeResult = mUpdateEngine.cleanupAppliedPayload();
        Log.d("ABUpdate","mergeResult"+mergeResult);
        if (mPref.getBooleanVal(PrefUtils.key, false)) {
            if (mergeResult == PrepareUpdateService.RESULT_CODE_SUCCESS) {
                runOnUiThread(() -> {mTextView.setText(getString(R.string.update_success));});
                Toast.makeText(EmptyActivity.this, getString(R.string.update_success), SHOW_TIME).show();
            } else {
                runOnUiThread(() -> {mTextView.setText(getString(R.string.update_fail));});
                Toast.makeText(EmptyActivity.this, getString(R.string.update_fail), SHOW_TIME).show();
            }
            mPref.setBoolean(PrefUtils.key,false);
        }
        mPref.disableUI(false, this);
        this.finish();
    }

}
