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

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.Display;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.Window;
import android.widget.Button;
import android.widget.TextView;

import com.droidlogic.updater.R;
import com.droidlogic.updater.service.AutoCheckService;
/**
 * Demonstrates how to show an AlertDialog that is managed by a Fragment.
 */
public class FragmentAlertDialog extends Activity {
    private Button mButton;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Display display = getWindowManager().getDefaultDisplay();
        Window window = getWindow();
        setTitle("RunningRunningRunningRunningRunning");
        android.view.WindowManager.LayoutParams windowLayoutParams = window.getAttributes();
        windowLayoutParams.width = (int) (display.getWidth() * 0.3);
        windowLayoutParams.height = (int) (display.getHeight() * 0.3);
        getWindow().setAttributes(windowLayoutParams);
        setContentView(R.layout.fragment_dialog);
        mButton = (Button)findViewById(R.id.show);
        mButton.setOnClickListener(new View.OnClickListener() {

            @Override
            public void onClick(View v) {
                Intent intent = new Intent();
                intent.setAction(AutoCheckService.SHOWING_ACTION);
                FragmentAlertDialog.this.sendBroadcast(intent);
                FragmentAlertDialog.this.finish();
            }
        });
    }

}