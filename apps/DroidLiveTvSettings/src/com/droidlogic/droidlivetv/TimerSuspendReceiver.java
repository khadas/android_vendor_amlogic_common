/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC TimerSuspendReceiver
 */

package com.droidlogic.droidlivetv;

import android.app.AlarmManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.SystemProperties;
import android.util.Log;
import android.os.PowerManager;
import android.view.KeyEvent;
import android.view.InputEvent;
import android.os.SystemClock;

import com.droidlogic.app.tv.DroidLogicTvUtils;
import com.droidlogic.app.SystemControlManager;
import com.droidlogic.app.tv.DroidLogicTvUtils;
import com.droidlogic.app.DroidLogicKeyEvent;
import com.droidlogic.app.DataProviderManager;

import java.lang.reflect.Method;

public class TimerSuspendReceiver extends BroadcastReceiver {
    private static final String TAG = "TimerSuspendReceiver";
    private static final int INJECT_INPUT_EVENT_MODE_WAIT_FOR_FINISH = 2;

    private Context mContext = null;
    private SystemControlManager mSystemControlManager = null;

    @Override
    public void onReceive(Context context, Intent intent) {
        Log.d(TAG, "onReceive: " + intent);

        mContext = context;
        if (intent != null) {
            //add for third party application that can't rasise power key
            //action:droidlogic.intent.action.TIMER_SUSPEND key:command_suspend value:true
            if (intent.getBooleanExtra(DroidLogicTvUtils.KEY_COMMAND_REQUEST_SUSPEND, false)) {
                pressPowerKey();
            } else {
                mSystemControlManager = SystemControlManager.getInstance();
                if (intent.getBooleanExtra(DroidLogicTvUtils.KEY_ENABLE_SUSPEND_TIMEOUT, false)) {
                    DataProviderManager.putIntValue(mContext, DroidLogicTvUtils.PROP_DROID_TV_SLEEP_TIME, 0);//clear it as acted
                }
                startSleepTimer(intent);
            }
        }
    }

    public void startSleepTimer (Intent intent) {
        Log.d(TAG, "startSleepTimer");
        Intent intentservice = new Intent(mContext, TimerSuspendService.class );
        intentservice.putExtra(DroidLogicTvUtils.KEY_ENABLE_NOSIGNAL_TIMEOUT, intent.getBooleanExtra(DroidLogicTvUtils.KEY_ENABLE_NOSIGNAL_TIMEOUT, false));
        intentservice.putExtra(DroidLogicTvUtils.KEY_ENABLE_SUSPEND_TIMEOUT, intent.getBooleanExtra(DroidLogicTvUtils.KEY_ENABLE_SUSPEND_TIMEOUT, false));
        mContext.startService (intentservice);
    }

    private void pressPowerKey () {
        if (!isSystemScreenOn()) {
            Log.d(TAG, "pressPowerKey screen is off already");
            return;
        }
        long now = SystemClock.uptimeMillis();
        KeyEvent down = new KeyEvent(now, now, KeyEvent.ACTION_DOWN, DroidLogicKeyEvent.KEYCODE_POWER, 0);
        KeyEvent up = new KeyEvent(now, now, KeyEvent.ACTION_UP, DroidLogicKeyEvent.KEYCODE_POWER, 0);
        GetinjectInputEvent(down, INJECT_INPUT_EVENT_MODE_WAIT_FOR_FINISH);
        GetinjectInputEvent(up, INJECT_INPUT_EVENT_MODE_WAIT_FOR_FINISH);
    }

    private boolean isSystemScreenOn() {
        PowerManager powerManager = mContext.getSystemService(PowerManager.class);
        boolean isScreenOpen = powerManager.isScreenOn();
        Log.d(TAG, "isSystemScreenOn isScreenOpen = " + isScreenOpen);
        return isScreenOpen;
    }

    private void GetinjectInputEvent(InputEvent keyevent, int mode) {
        try {
            Class<?> cls = Class.forName("android.hardware.input.InputManager");
            Method constructor = cls.getMethod("getInstance");
            Method method = cls.getMethod("injectInputEvent", InputEvent.class, int.class);
            method.invoke(constructor.invoke(null), keyevent, mode);
        } catch(Exception e) {
            Log.d(TAG, "GetinjectInputEvent Exception = " + e.getMessage());
        }
    }
}
