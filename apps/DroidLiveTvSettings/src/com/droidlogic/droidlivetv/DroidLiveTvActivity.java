/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC DroidLiveTvActivity
 */

package com.droidlogic.droidlivetv;

import com.droidlogic.droidlivetv.ui.MultiOptionFragment;
import com.droidlogic.droidlivetv.ui.OverlayRootView;
import com.droidlogic.droidlivetv.ui.SideFragmentManager;
import com.droidlogic.droidlivetvsettings.R;

import com.droidlogic.app.DroidLogicKeyEvent;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.graphics.PixelFormat;
import android.os.Bundle;
import android.util.Log;
import android.view.Display;
import android.view.KeyEvent;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.os.Handler;
import android.os.Message;
import android.provider.Settings;
import android.content.IntentFilter;
import android.content.Intent;
import android.content.BroadcastReceiver;

import com.droidlogic.app.tv.DroidLogicTvUtils;

public class DroidLiveTvActivity extends Activity {
    private final static String TAG = "DroidLiveTvActivity";
    private SideFragmentManager mSideFragmentManager;
    private OverlayRootView mOverlayRootView;
    private Context mContext;
    private static final String KEY_MENU_TIME = DroidLogicTvUtils.KEY_MENU_TIME;
    private static final int DEFAULT_MENU_TIME = DroidLogicTvUtils.DEFAULT_MENU_TIME;

    //use exist keycode to redefine tv related keyevent
    public static final int KEYCODE_TV_SHORTCUTKEY_GLOBALSETUP    = KeyEvent.KEYCODE_BUTTON_1;
    public static final int KEYCODE_TV_SHORTCUTKEY_SOURCE_LIST    = KeyEvent.KEYCODE_BUTTON_2;
    public static final int KEYCODE_TV_SHORTCUTKEY_3DMODE         = KeyEvent.KEYCODE_BUTTON_3;
    public static final int KEYCODE_TV_SHORTCUTKEY_DISPAYMODE     = KeyEvent.KEYCODE_BUTTON_4;
    public static final int KEYCODE_TV_SHORTCUTKEY_VIEWMODE       = KeyEvent.KEYCODE_BUTTON_5;
    public static final int KEYCODE_TV_SHORTCUTKEY_VOICEMODE      = KeyEvent.KEYCODE_BUTTON_6;
    public static final int KEYCODE_TV_SHORTCUTKEY_TVINFO         = KeyEvent.KEYCODE_BUTTON_7;
    public static final int KEYCODE_EARLY_POWER                   = KeyEvent.KEYCODE_BUTTON_8;
    public static final int KEYCODE_TV_SLEEP                      = KeyEvent.KEYCODE_BUTTON_9;
    public static final int KEYCODE_TV_SOUND_CHANNEL              = KeyEvent.KEYCODE_BUTTON_10;
    public static final int KEYCODE_TV_REPEAT                     = KeyEvent.KEYCODE_BUTTON_11;
    public static final int KEYCODE_TV_SUBTITLE                   = KeyEvent.KEYCODE_BUTTON_12;
    public static final int KEYCODE_TV_SWITCH                     = KeyEvent.KEYCODE_BUTTON_13;
    public static final int KEYCODE_TV_WASU                       = KeyEvent.KEYCODE_BUTTON_14;
    public static final int KEYCODE_TV_VTION                      = KeyEvent.KEYCODE_BUTTON_15;
    public static final int KEYCODE_TV_BROWSER                    = KeyEvent.KEYCODE_BUTTON_16;
    public static final int KEYCODE_TV_ALTERNATE                  = KeyEvent.KEYCODE_BUTTON_X;
    public static final int KEYCODE_FAV                           = KeyEvent.KEYCODE_BUTTON_Y;
    public static final int KEYCODE_LIST                          = KeyEvent.KEYCODE_BUTTON_Z;
    public static final int KEYCODE_MEDIA_AUDIO_CONTROL           = KeyEvent.KEYCODE_BUTTON_L1;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        registerSomeReceivers();
        mContext = this;
        Intent intent = getIntent();
        Bundle bundle= intent.getExtras();
        if (bundle != null) {
            requestWindowFeature(Window.FEATURE_NO_TITLE);
            setContentView(R.layout.activity_droid_live_tv);
            mOverlayRootView = (OverlayRootView) getLayoutInflater().inflate(R.layout.overlay_root_view, null, false);
            mSideFragmentManager = new SideFragmentManager(this);
            Display display = getWindowManager().getDefaultDisplay();

            int keyvalue = bundle.getInt("eventkey");
            int deviceid = bundle.getInt("deviceid");
            Log.d(TAG, "GETKEY: " + keyvalue);
            mSideFragmentManager.show(new MultiOptionFragment(bundle, mContext));
            startShowActivityTimer();
        } else {
            finish();
        }
    }

    @Override
    public void onResume() {
        Log.d(TAG, "onResume" );
        registerSomeReceivers();
        startShowActivityTimer();
        super.onResume();
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        Log.d(TAG, "onKeyDown(" + keyCode + ", " + event + ")");
        startShowActivityTimer();
        switch (keyCode) {
            case KeyEvent.KEYCODE_BACK:
                //mSideFragmentManager.popSideFragment();
                handler.sendEmptyMessage(0);
                return true;
            case KeyEvent.KEYCODE_DPAD_UP:
            case KeyEvent.KEYCODE_DPAD_DOWN:
            case KeyEvent.KEYCODE_ENTER:
            case KeyEvent.KEYCODE_DPAD_CENTER:
                startShowActivityTimer();
                break;
            default:
                // pass through
        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        Log.d(TAG, "onKeyUp(" + keyCode + ", " + event + ")");
        switch (keyCode) {
            case DroidLogicKeyEvent.KEYCODE_TV_SHORTCUTKEY_VIEWMODE:
            case DroidLogicKeyEvent.KEYCODE_TV_SHORTCUTKEY_VOICEMODE:
            case DroidLogicKeyEvent.KEYCODE_TV_SHORTCUTKEY_DISPAYMODE:
            case DroidLogicKeyEvent.KEYCODE_TV_SLEEP:
            case DroidLogicKeyEvent.KEYCODE_FAV:
            case DroidLogicKeyEvent.KEYCODE_LIST:
            case KEYCODE_TV_SHORTCUTKEY_VIEWMODE:
            case KEYCODE_TV_SHORTCUTKEY_VOICEMODE:
            case KEYCODE_TV_SHORTCUTKEY_DISPAYMODE:
            case KEYCODE_TV_SLEEP:
            case KEYCODE_FAV:
            case KEYCODE_LIST:
                handler.sendEmptyMessage(0);
                return true;
            default:
                // pass through
        }
        return super.onKeyUp(keyCode, event);
    }

    public BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            Log.d(TAG, "intent = " + intent);
            switch (intent.getAction()) {
                case Intent.ACTION_CLOSE_SYSTEM_DIALOGS:
                    finish();
                    break;
                case Intent.ACTION_SCREEN_OFF:
                    finish();
                    break;
            }
        }
    };
    public void registerSomeReceivers() {
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(Intent.ACTION_SCREEN_OFF);
        intentFilter.addAction(Intent.ACTION_CLOSE_SYSTEM_DIALOGS);
        registerReceiver(mReceiver, intentFilter);
    }

    private void startShowActivityTimer () {
        handler.removeMessages(0);
        int seconds = Settings.System.getInt(getContentResolver(), KEY_MENU_TIME, DEFAULT_MENU_TIME);
        if (seconds == 1) {
            seconds = 15;
        } else if (seconds == 2) {
            seconds = 30;
        } else if (seconds == 3) {
            seconds = 60;
        } else if (seconds == 4) {
            seconds = 120;
        } else if (seconds == 5) {
            seconds = 240;
        } else {
            seconds = 0;
        }
        Log.d(TAG, "[startShowActivityTimer] seconds = " + seconds);
        if (seconds > 0) {
            handler.sendEmptyMessageDelayed(0, seconds * 1000);
        } else {
            handler.removeMessages(0);
        }
    }

    Handler handler = new Handler() {
        public void handleMessage(Message msg) {
            finish();
        }
    };

    @Override
    public void onAttachedToWindow() {
        super.onAttachedToWindow();
        WindowManager.LayoutParams windowParams = new WindowManager.LayoutParams(
                WindowManager.LayoutParams.TYPE_APPLICATION_SUB_PANEL, 0, PixelFormat.TRANSPARENT);
        windowParams.token = getWindow().getDecorView().getWindowToken();
        ((WindowManager) getSystemService(Context.WINDOW_SERVICE)).addView(mOverlayRootView,
                windowParams);
    }

    @Override
    public void onDetachedFromWindow() {
        super.onDetachedFromWindow();
        ((WindowManager) getSystemService(Context.WINDOW_SERVICE)).removeView(mOverlayRootView);
    }

    @Override
    public View findViewById(int id) {
        // In order to locate fragments in non-application window, we should override findViewById.
        // Internally, Activity.findViewById is called to attach a view of a fragment into its
        // container. Without the override, we'll get crash during the fragment attachment.
        View v = mOverlayRootView != null ? mOverlayRootView.findViewById(id) : null;
        return v == null ? super.findViewById(id) : v;
    }

    public SideFragmentManager getSideFragmentManager() {
        return mSideFragmentManager;
    }

    public void onDestroy() {
        super.onDestroy();
        handler.removeMessages(0);
    }
}
