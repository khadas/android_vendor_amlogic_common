/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC BootComplete
 */

package com.droidlogic.displaycontrollerservice;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

public class BootComplete extends BroadcastReceiver {
    private static final String TAG             = "BootComplete";

    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();
        Log.i(TAG, "action: " + action);
        context.startService(new Intent(context,MultiDisplayService.class));
	}
}