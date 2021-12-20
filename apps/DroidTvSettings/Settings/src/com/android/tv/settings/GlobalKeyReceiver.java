/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC GlobalKeyReceiver
 */

package com.android.tv.settings;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.view.KeyEvent;
import android.util.Log;
import java.lang.Class;
import java.lang.Exception;
import java.lang.Package;

public class GlobalKeyReceiver extends BroadcastReceiver {
    private static final String TAG = "DroidTvGlobalKeyReceiver";
    public static final  String HDMICEC_ACTIVITY_NAME = "com.android.tv.settings.tvoption.HdmiCecActivity";

    @Override
    public void onReceive(Context context, Intent intent) {
        if ("android.intent.action.GLOBAL_BUTTON".equals(intent.getAction())) {
            KeyEvent event = intent.getParcelableExtra(Intent.EXTRA_KEY_EVENT);
            int keyCode = event.getKeyCode();
            int keyAction = event.getAction();
            Log.i(TAG, "onReceive:"+"keyAction<" + keyAction+"> keyCode: " + keyCode );

            switch (keyCode) {
                case KeyEvent.KEYCODE_EXPLORER:
                    startHdmiCecActivity(context);
                    break;
                default:
                    Log.e(TAG, "unHandled KeyEvent: " + keyCode);
                    break;
            }
        }
    }

    private int startHdmiCecActivity(Context context) {
        Log.i(TAG, "to start HdmiCecActivity");
        try {
            Class activityClass = Class.forName(HDMICEC_ACTIVITY_NAME);
            Intent intent = new Intent(context, activityClass);
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            context.startActivity(intent);
        } catch (Exception e) {
            Log.e(TAG, e.getMessage());
        }
        return 0;
    }
}
