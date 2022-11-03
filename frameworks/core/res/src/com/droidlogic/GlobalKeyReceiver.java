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
import android.content.ActivityNotFoundException;
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
import android.text.TextUtils;
import android.util.Log;
import android.view.KeyEvent;
import android.os.SystemProperties;

import com.droidlogic.app.UsbCameraManager;
import java.util.List;
import java.lang.reflect.InvocationTargetException;

public class GlobalKeyReceiver extends BroadcastReceiver {
    private static final String TAG = "GlobalKeyReceiver";

    private static final String PACKAGE_NAME_LIVETV = "com.droidlogic.android.tv";
    private static final String ACTIVITY_NAME_LIVETV = "com.android.tv.MainActivity";
    private static final String PACKAGE_NAME_DROIDTVSETTINGS = "com.droidlogic.tv.settings";
    private static final String ACTIVITY_NAME_TVSOURCE = "com.droidlogic.tv.settings.TvSourceActivity";
    private static final String PACKAGE_NAME_NETFLIX = "com.netflix.ninja";
    private static final String PACKAGE_NAME_YOUTUBE = "com.google.android.youtube.tv";
    private static final String ACTIVITY_NAME_YOUTUBE = "com.google.android.apps.youtube.tv.activity.MainActivity";
    private static final String PACKAGE_NAME_PLAYMOVIE = "com.google.android.videos";
    private static final String PACKAGE_NAME_PRIMEVIDEO = "com.amazon.amazonvideo.livingroom";
    private static final String PACKAGE_NAME_GOOGLEPLAY = "com.android.vending";
    private static final String ACTIVITY_NAME_GOOGLEPLAY = "com.google.android.finsky.tvmainactivity.TvMainActivity";
    private static final String PACKAGE_NAME_DISNEY = "com.disney.disneyplus";
    private static final String PACKAGE_NAME_PARAMOUNT = "com.cbs.ott";
    private static final String NETFLIX_PERMISSION = "com.netflix.ninja.permission.NETFLIX_KEY";
    private static final String NETFLIX_ACTION = "com.netflix.ninja.intent.action.NETFLIX_KEY";
    private static final int NETFLIX_SOURCE_TYPE_NETFLIX_BUTTON = 1;
    private static final int NETFLIX_SOURCE_TYPE_POWER_ON_FROM_NETFLIX_BUTTON = 19;
    private static final String URI_NETFLIX = "nflx://www.netflix.com/";
    private static final String REMOTE_BUTTON_START = "remote_button";
    private static final String REMOTE_YT_BUTTON = "yt_remote_button";
    private static final int  PENDING_KEY_NULL = -1;
    private static final String EXTRA_BEGAN_FROM_NON_INTERACTIVE =
            "EXTRA_BEGAN_FROM_NON_INTERACTIVE";
    private static final String NETFLIX_KEY_POWER_MODE = "power_on";
    private static final String ACTION_LAUNCH_APP = "com.google.global_button.ACTION_LAUNCH_APP";
    private static final String EXTRA_PACKAGE_NAME = "launchPackageName";
    private static final String EXTRA_LAUNCH_INTENT = "launchIntent";
    private static final String NETFLIX_INTENT = "com.netflix.action.NETFLIX_KEY_START";

    private static boolean isTvSetupComplete(Context context) {
        return Settings.Secure
                .getInt(context.getContentResolver(), Settings.Secure.USER_SETUP_COMPLETE,
                        0) != 0;
    }

    private static boolean isPackageRunning(String packageName) {
        try {
            List<ActivityManager.RunningTaskInfo> tasks = ActivityManager.getService().getTasks(1);
            ComponentName componentInfo = tasks.get(0).topActivity;
            return componentInfo.getPackageName().equals(packageName);
        } catch (Exception e) {
        }
        return false;
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
        if ("android.intent.action.GLOBAL_BUTTON".equals(intent.getAction())) {
            String component ;
            Intent intent1 = new Intent();

            KeyEvent event = intent.getParcelableExtra(Intent.EXTRA_KEY_EVENT);
            boolean fromNonInteractive =
                intent.getBooleanExtra(EXTRA_BEGAN_FROM_NON_INTERACTIVE, false);
            int keyCode = event.getKeyCode();
            int keyAction = event.getAction();
            Log.i(TAG, "onReceive:"+"keyAction<" + keyAction+"> keyCode: " + keyCode );
            if (!isTvSetupComplete(context)) {
                  Log.i(TAG, "Set up incomplete. Ignoring KeyEvent: " + keyCode);
                  return;
            }

            switch (keyCode) {
                case KeyEvent.KEYCODE_F5:
                    if (keyAction == KeyEvent.ACTION_UP) {
                        intent1.setComponent(new ComponentName("com.google.android.music", "com.google.android.music.tv.HomeActivity"));
                    }
                    break;

                case KeyEvent.KEYCODE_F1:
                case KeyEvent.KEYCODE_BUTTON_3:
                    if (keyAction == KeyEvent.ACTION_UP) {
                        oneTouchPlay(context);
                        intent1.setComponent(new ComponentName(PACKAGE_NAME_YOUTUBE, ACTIVITY_NAME_YOUTUBE))
                               .putExtra(REMOTE_YT_BUTTON, true);
                        wakeUp(context);
                    }
                    break;

                case KeyEvent.KEYCODE_F3:
                case KeyEvent.KEYCODE_BUTTON_6:
                    if (keyAction == KeyEvent.ACTION_UP) {
                        oneTouchPlay(context);
                        launchAppByPackageName(context, PACKAGE_NAME_PRIMEVIDEO);
                        wakeUp(context);
                    }
                    return;


                case KeyEvent.KEYCODE_F4:
                case KeyEvent.KEYCODE_BUTTON_7:
                    if (keyAction == KeyEvent.ACTION_UP) {
                        intent1.setComponent(new ComponentName(PACKAGE_NAME_GOOGLEPLAY, ACTIVITY_NAME_GOOGLEPLAY));
                    }
                    break;

                case KeyEvent.KEYCODE_F2:
                case KeyEvent.KEYCODE_BUTTON_4:
                    if (keyAction == KeyEvent.ACTION_UP) {
                        Log.i(TAG, "onReceive:  receive from interactive " + fromNonInteractive );
                        oneTouchPlay(context);
                        launchNetflix(context,fromNonInteractive);
                    }
                    return;//netflix button is a special case, all things are processed in launchNetflix()

                case KeyEvent.KEYCODE_SETTINGS:
                case KeyEvent.KEYCODE_NOTIFICATION:
                    if (keyAction == KeyEvent.ACTION_UP) {
                        if (SystemProperties.get("sys.vendor.global.settingskey").equals("dashboard")) {
                            intent1.setComponent(new ComponentName("com.google.android.apps.tv.launcherx", "com.google.android.apps.tv.launcherx.dashboard.DashboardActivity"));
                        } else {
                            intent1.setComponent(new ComponentName("com.android.tv.settings", "com.android.tv.settings.MainSettings"));
                        }
                    }
                    break;
                case KeyEvent.KEYCODE_F6:
                    if (keyAction == KeyEvent.ACTION_UP) {
                        intent1.setComponent(new ComponentName(PACKAGE_NAME_LIVETV, ACTIVITY_NAME_LIVETV));
                    }
                    break;
                case KeyEvent.KEYCODE_TV_INPUT:
                    if (keyAction == KeyEvent.ACTION_UP) {
                        if (isTargetPackageRunningOnTop(PACKAGE_NAME_LIVETV)) {
                            intent1.putExtra("from_live_tv", 1);
                        }
                        intent1.setAction("com.android.tv.action.VIEW_INPUTS");
                        intent1.setComponent(new ComponentName(PACKAGE_NAME_DROIDTVSETTINGS, ACTIVITY_NAME_TVSOURCE));
                    }
                    break;
                case KeyEvent.KEYCODE_BUTTON_9:
                    if (keyAction == KeyEvent.ACTION_UP) {
                        oneTouchPlay(context);
                        launchAppByPackageName(context, PACKAGE_NAME_DISNEY);
                        wakeUp(context);
                    }
                    return;
                case KeyEvent.KEYCODE_BUTTON_10:
                    if (keyAction == KeyEvent.ACTION_UP) {
                        oneTouchPlay(context);
                        launchAppByPackageName(context, PACKAGE_NAME_PARAMOUNT);
                        wakeUp(context);
                    }
                    return;
                case KeyEvent.KEYCODE_PAIRING:
                    if (keyAction == KeyEvent.ACTION_UP) {
                        intent1.setComponent(new ComponentName("com.android.tv.settings", "com.android.tv.settings.accessories.AddAccessoryActivity"));
                        intent1.putExtra("no_input_mode", true);
                    }
                    break;
                default:
                    Log.e(TAG, "Unhandled KeyEvent: " + keyCode);
                    intent1.setComponent(new ComponentName("com.nes.blerc","com.nes.blerc.MainActivity"));
                    break;
            }

            intent1.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            if (keyAction == KeyEvent.ACTION_UP && isIntentAvailable(context,intent1)) {
                Log.i(TAG, "startActivity" );
                context.startActivity(intent1);
            }
        }
    }

    private void oneTouchPlay(Context context) {
        Log.d(TAG, "oneTouchPlay");
        HdmiControlManager manager = context.getSystemService(HdmiControlManager.class);
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

    private void launchNetflixAtv(Context context , boolean isInteractive) {
        Log.i(TAG, "launchNetflix atv: isInteractive: " + isInteractive);
        //changed for Ninja 7.0.0 and Later
        //https://nrd.netflix.com/docs/development/atv/integrating-netflix
        Intent netflixIntent = new Intent();
        netflixIntent.setAction(NETFLIX_INTENT);
        netflixIntent.setPackage(PACKAGE_NAME_NETFLIX);
        netflixIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_REORDER_TO_FRONT);
        netflixIntent.putExtra(NETFLIX_KEY_POWER_MODE, isInteractive); //false for netflixButton, true for powerOnFromNetflixButton
        context.startActivity(netflixIntent);
    }

    private void launchNetflix(Context context, boolean fromNonInteractive) {
        PackageManager packageManager = context.getPackageManager();
        if (packageManager.getLaunchIntentForPackage(PACKAGE_NAME_NETFLIX) == null) {
            Log.e(TAG, "Cannot find intent for Netlix package: " + PACKAGE_NAME_NETFLIX);
            return;
        }
        String globalButtonLaunch = context.getString(R.string.config_globalButtonLaunch);
        Log.d(TAG, " globalButtonLaunch component: " + globalButtonLaunch);

        Intent intent = new Intent(ACTION_LAUNCH_APP);
        intent.setComponent(ComponentName.unflattenFromString(globalButtonLaunch));
        intent.putExtra(EXTRA_PACKAGE_NAME, PACKAGE_NAME_NETFLIX);
        intent.addFlags(Intent.FLAG_INCLUDE_STOPPED_PACKAGES | Intent.FLAG_RECEIVER_FOREGROUND);

        Intent launchIntent  = new Intent(NETFLIX_INTENT);
        launchIntent.setPackage(PACKAGE_NAME_NETFLIX);
        launchIntent.putExtra(NETFLIX_KEY_POWER_MODE, fromNonInteractive);
        launchIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_INCLUDE_STOPPED_PACKAGES
            | Intent.FLAG_RECEIVER_FOREGROUND | Intent.FLAG_ACTIVITY_REORDER_TO_FRONT);
        intent.putExtra(EXTRA_LAUNCH_INTENT, launchIntent);

        if (canIntentBeHandled(context, intent)) {
        Log.d(TAG, "launchNetflix gtv: isInteractive: " + fromNonInteractive);
            context.sendBroadcast(intent);
        } else {
            launchNetflixAtv(context,fromNonInteractive);
        }
        //netflix key always need wake up, if it is in interactive , wakeUp do noting
        wakeUp(context);
    }

    private  boolean canIntentBeHandled(Context context, Intent intent) {
        List<ResolveInfo> receivers = context.getPackageManager().queryBroadcastReceivers(
            intent, PackageManager.MATCH_ALL);
        Log.d(TAG, "receivers " + receivers);
        if (receivers != null && receivers.size() > 0) {
            return true;
        }
        return false;
    }

    public boolean isIntentAvailable(Context context, Intent intent) {
         final PackageManager packageManager = context.getPackageManager();
         List<ResolveInfo> list = packageManager.queryIntentActivities(intent,
                    PackageManager.MATCH_DEFAULT_ONLY);
         return list.size() > 0;
    }

    private boolean isTargetPackageRunningOnTop(String targetPackageName) {
        try {
            List<ActivityManager.RunningTaskInfo> tasks = ActivityManager.getService().getTasks(1);
            ComponentName componentInfo = tasks.get(0).topActivity;
            Log.d(TAG, "getTopActivityPackageName = " + componentInfo.getPackageName());
            return componentInfo.getPackageName().equals(targetPackageName);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return false;
    }
    private boolean launchAppByPackageName(Context context, String packageName) {
        Intent globalButtonIntent = getGlobalButtonLaunchReceiver(context);
        if (globalButtonIntent != null) {
            globalButtonIntent.putExtra(EXTRA_PACKAGE_NAME, packageName);
            context.sendBroadcast(globalButtonIntent);
            return true;
        }

        PackageManager packageManager = context.getPackageManager();
        Intent launchIntent = packageManager.getLaunchIntentForPackage(packageName);
        if (launchIntent == null) {
            Log.e(TAG, "Cannot find intent for package: " + packageName);
            String uri = "https://play.google.com/store/apps/details?id=" + packageName;
            Intent installIntent = new Intent(Intent.ACTION_VIEW, Uri.parse(uri));
            installIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            try {
                context.startActivity(installIntent);
            } catch (ActivityNotFoundException e) {
                Log.e(TAG, "Failed to launch Play Store for package: " + packageName, e);
                return false;
            }
            return false;
            }

        if (isPackageRunning(packageName)) {
            Log.d(TAG, "Package already running: " + packageName);
            return true;
        }
        if (packageName.equals(PACKAGE_NAME_DISNEY)
                || packageName.equals(PACKAGE_NAME_PARAMOUNT)) {
            launchIntent.putExtra(REMOTE_BUTTON_START, true);
        }
        context.startActivity(launchIntent);
        return true;
    }

    // Return Global button launch receiver or null if not exists.
    private static Intent getGlobalButtonLaunchReceiver(Context context) {
        String receiverComponent = context.getString(R.string.config_globalButtonLaunch);
        if (TextUtils.isEmpty(receiverComponent)) {
            return null;
        }
        Intent intent = new Intent(ACTION_LAUNCH_APP);
        intent.setFlags(Intent.FLAG_RECEIVER_FOREGROUND);
        intent.setComponent(ComponentName.unflattenFromString(receiverComponent));
        List<ResolveInfo> resolved =
                context.getPackageManager().queryBroadcastReceivers(intent, 0);
        if (resolved != null && resolved.size() > 0) {
            return intent;
        }
        return null;
    }

}
