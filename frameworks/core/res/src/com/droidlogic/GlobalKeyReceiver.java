/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC GlobalKeyReceiver
 */

package com.droidlogic;

import android.app.ActivityManager;
import android.os.UserManager;
import android.content.pm.UserInfo;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.ComponentName;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.hardware.hdmi.HdmiControlManager;
import android.hardware.hdmi.HdmiPlaybackClient;
import android.hardware.hdmi.HdmiPlaybackClient.OneTouchPlayCallback;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.net.Uri;
import android.os.PowerManager;
import android.os.SystemClock;
import android.os.UserHandle;
import android.provider.Settings;
import android.util.Log;
import android.view.KeyEvent;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.database.Cursor;
import android.net.Uri;



import com.droidlogic.app.UsbCameraManager;
import java.util.List;
import java.lang.reflect.InvocationTargetException;
import com.droidlogic.app.SystemControlManager;

import com.droidlogic.app.DroidLogicUtils;

public class GlobalKeyReceiver extends BroadcastReceiver {
    private static final String TAG = "GlobalKeyReceiver";

    private static final String PACKAGE_NAME_LIVETV = "com.droidlogic.android.tv";
    private static final String ACTIVITY_NAME_LIVETV = "com.android.tv.MainActivity";
    private static final String PACKAGE_NAME_NETFLIX = "com.netflix.ninja";
    private static final String PACKAGE_NAME_YOUTUBE = "com.google.android.youtube.tv";
    private static final String PACKAGE_NAME_PLAYMOVIE = "com.google.android.videos";
    private static final String NETFLIX_PERMISSION = "com.netflix.ninja.permission.NETFLIX_KEY";
    private static final String NETFLIX_ACTION = "com.netflix.ninja.intent.action.NETFLIX_KEY";
    private static final int NETFLIX_SOURCE_TYPE_NETFLIX_BUTTON = 1;
    private static final int NETFLIX_SOURCE_TYPE_POWER_ON_FROM_NETFLIX_BUTTON = 19;
    private static final String URI_NETFLIX = "nflx://www.netflix.com/";
    private static final String REMOTE_YT_BUTTON = "yt_remote_button";
    private static final int  PENDING_KEY_NULL = -1;
    private static final String EXTRA_BEGAN_FROM_NON_INTERACTIVE =
            "EXTRA_BEGAN_FROM_NON_INTERACTIVE";
    private boolean mIsBootCompleted = false;
    private static final String HOTWORDMIC_AUTH = "atv.hotwordmic";
    //table name
    private static final String TOGGLESTATE = "togglestate";
    public static final Uri HOTWORDMIC_URI = new Uri.Builder().scheme("content")
                                                  .authority(HOTWORDMIC_AUTH)
                                                  .appendPath(TOGGLESTATE)
                                                  .build();
    //colume name
    public static final String COLUME_ID = "state" ;

    private static boolean isTvSetupComplete(Context context) {
        return Settings.Secure
                .getInt(context.getContentResolver(), Settings.Secure.USER_SETUP_COMPLETE,
                        0) != 0;
    }

    private boolean isInteractive(Context context) {
        PowerManager powerManager = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
        if (powerManager == null) return true;
        return powerManager.isInteractive();
    }

    private void wakeUp(Context context) {
        PowerManager powerManager = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
        if (powerManager == null) return;
        Log.i(TAG, "isInteractive : " + powerManager.isInteractive() + "wake up");
        //powerManager.wakeUp(SystemClock.uptimeMillis(), "android.policy:KEY");
        try {
            Class[] typeArgs = new Class[2];
            typeArgs[0] = long.class;
            typeArgs[1] = String.class;

            Object[] valueArgs = new Object[2];
            valueArgs[0] = SystemClock.uptimeMillis();
            valueArgs[1] = "android.policy:KEY";

            powerManager.getClass().getMethod("wakeUp", long.class,String.class)
                    .invoke(powerManager, valueArgs);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        if (Intent.ACTION_BOOT_COMPLETED.equals(intent.getAction())) {
            mIsBootCompleted = true;
        }

        if ("android.intent.action.GLOBAL_BUTTON".equals(intent.getAction())) {
            String component ;
            Intent intent1 = new Intent();

            KeyEvent event = intent.getParcelableExtra(Intent.EXTRA_KEY_EVENT);
            boolean fromNonInteractive =
                intent.getBooleanExtra(EXTRA_BEGAN_FROM_NON_INTERACTIVE, false);
            int keyCode = event.getKeyCode();
            int keyAction = event.getAction();
            Log.i(TAG, "onReceive:"+"keyAction<" + keyAction+"> keyCode: " + keyCode );
            if (!mIsBootCompleted || !isTvSetupComplete(context)) {
                  Log.i(TAG, "Set up incomplete. Ignoring KeyEvent: " + keyCode);
                  return;
            }

/*             switch (keyCode) {
                case KeyEvent.KEYCODE_F5:
                    if (keyAction == KeyEvent.ACTION_UP) {
                        intent1.setComponent(new ComponentName("com.google.android.music", "com.google.android.music.tv.HomeActivity"));
                    }
                    break;

                case KeyEvent.KEYCODE_F1:
                    if (keyAction == KeyEvent.ACTION_UP) {
                        oneTouchPlay(context);
                        intent1.setComponent(new ComponentName(PACKAGE_NAME_YOUTUBE, "com.google.android.apps.youtube.tv.activity.MainActivity"))
                               .putExtra(REMOTE_YT_BUTTON, true);
                        wakeUp(context);
                    }
                    break;

                case KeyEvent.KEYCODE_F3:
                    if (keyAction == KeyEvent.ACTION_UP) {
                        intent1.setComponent(new ComponentName("com.android.vending", "com.google.android.finsky.tvmainactivity.TvMainActivity"));
                    }
                    break;

                case KeyEvent.KEYCODE_F4:
                    if (keyAction == KeyEvent.ACTION_UP) {
                        intent1.setComponent(new ComponentName(PACKAGE_NAME_PLAYMOVIE, "com.google.android.apps.play.movies.tv.usecase.home.TvHomeActivity"));
                    }
                    break;

                case KeyEvent.KEYCODE_F2:
                    if (keyAction == KeyEvent.ACTION_UP) {
                        Log.i(TAG, "onReceive:  receive from interactive " + fromNonInteractive );
                        oneTouchPlay(context);
                        launchNetflix(context,fromNonInteractive);
                    }
                    return;//netflix button is a special case, all things are processed in launchNetflix()

                case KeyEvent.KEYCODE_SETTINGS:
                    if (keyAction == KeyEvent.ACTION_UP) {
                        intent1.setComponent(new ComponentName("com.android.tv.settings", "com.android.tv.settings.MainSettings"));
                    }
                    break;
                case KeyEvent.KEYCODE_F6:
                    if (keyAction == KeyEvent.ACTION_UP) {
                        intent1.setComponent(new ComponentName(PACKAGE_NAME_LIVETV, ACTIVITY_NAME_LIVETV));
                    }
                    break;
                case KeyEvent.KEYCODE_PAIRING:
                    if (keyAction == KeyEvent.ACTION_UP) {
                        intent1.setComponent(new ComponentName("com.android.tv.settings", "com.android.tv.settings.accessories.AddAccessoryActivity"));
                        intent1.putExtra("no_input_mode", true);
                    }
                    break;
                case KeyEvent.KEYCODE_MUTE:
                    case KeyEvent.KEYCODE_F7:
                        if (keyAction == KeyEvent.ACTION_UP) {
                            updateMicToggleState(context);
                        }
                    return;
                default:
                    Log.e(TAG, "Unhandled KeyEvent: " + keyCode);
                    intent1.setComponent(new ComponentName("com.nes.blerc","com.nes.blerc.MainActivity"));
                    break;
            }

            intent1.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            if (keyAction == KeyEvent.ACTION_UP && isIntentAvailable(context,intent1)) {
                Log.i(TAG, "startActivity" );
                context.startActivity(intent1);
            } */
        }
    }

    private void oneTouchPlay(Context context) {
        Log.d(TAG, "oneTouchPlay");
        HdmiControlManager manager = (HdmiControlManager)context.getSystemService(Context.HDMI_CONTROL_SERVICE);
        HdmiPlaybackClient playback = null;
        if (manager != null) {
            playback = manager.getPlaybackClient();
        }
        if (playback == null) {
            Log.e(TAG, "oneTouchPlay client null!");
            return;
        }
        playback.oneTouchPlay(new OneTouchPlayCallback() {
            @Override
            public void onComplete(int result) {
                if (result != HdmiControlManager.RESULT_SUCCESS) {
                    Log.w(TAG, "One touch play failed: " + result);
                }
            }
        });
    }

    private boolean isNetflixRunning(Context context) {
        boolean isNetflixRunning = false;
        try {
            ActivityManager am = (ActivityManager) context.getSystemService (Context.ACTIVITY_SERVICE);
            List<ActivityManager.RunningTaskInfo> tasks = am.getRunningTasks(1);
            ComponentName componentInfo = tasks.get(0).topActivity;
            if (componentInfo.getPackageName().equals(PACKAGE_NAME_NETFLIX)) {
                isNetflixRunning = true;
            }
        } catch (Exception e) {}

        return isNetflixRunning;
    }

    private UserHandle getCurrentUser(Context context) {
        final ActivityManager activityManager = (ActivityManager)context.getSystemService(Context.ACTIVITY_SERVICE);
        final UserManager userManager = (UserManager)context.getSystemService(Context.USER_SERVICE);

        List<UserInfo> userList = userManager.getUsers();
        for (UserInfo user : userList) {
            if (user.id == 0) {
                continue;
            }
            boolean isUserRunning = activityManager.isUserRunning(user.id);
            Log.d(TAG, "userid = " + user.id + " isUserRunning = " + isUserRunning);

            if (isUserRunning) {
                return new UserHandle(user.id);
            }

        }

        return new UserHandle(0);

    }

    private void launchNetflix(Context context , boolean isInteractive) {
        PackageManager packageManager = context.getPackageManager();
        if (packageManager.getLaunchIntentForPackage(PACKAGE_NAME_NETFLIX) == null) {
            Log.e(TAG, "Cannot find intent for Netlix package: " + PACKAGE_NAME_NETFLIX);
            return;
        }

        Log.i(TAG, "launchNetflix: isInteractive: " + isInteractive);
        //changed for Ninja 7.0.0 and Later
        //https://nrd.netflix.com/docs/development/atv/integrating-netflix
        Intent netflixIntent = new Intent();
        netflixIntent.setAction("com.netflix.action.NETFLIX_KEY_START");
        netflixIntent.setPackage(PACKAGE_NAME_NETFLIX);
        netflixIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_REORDER_TO_FRONT);
        netflixIntent.putExtra("power_on", isInteractive); //false for netflixButton, true for powerOnFromNetflixButton
        context.startActivityAsUser(netflixIntent, getCurrentUser(context));
       //netflix key always need wake up, if it is in interactive , wakeUp do noting
        wakeUp(context);
    }

    public boolean isIntentAvailable(Context context, Intent intent) {
         final PackageManager packageManager = context.getPackageManager();
         List<ResolveInfo> list = packageManager.queryIntentActivities(intent,
                    PackageManager.MATCH_DEFAULT_ONLY);
         return list.size() > 0;
    }


   private void updateMicToggleState (Context context)
   {
        ContentResolver resolver =  context.getContentResolver();
        ContentValues value = new ContentValues();

        boolean mic_enable = DroidLogicUtils.getMicToggleState();
        if (mic_enable)
            value.put(COLUME_ID, 1);
        else
            value.put(COLUME_ID, 0);
        int ret = resolver.update(HOTWORDMIC_URI, value, "", null);
    }


}
