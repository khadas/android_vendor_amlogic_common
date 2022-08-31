/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.app.tv;

import android.content.Context;
import android.provider.Settings;
import android.os.SystemClock;
import android.util.Log;

import java.util.Date;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

import com.droidlogic.app.DaylightSavingTime;
import com.droidlogic.app.tv.TvControlDataManager;

public class TvTime{
    private final static String TAG = "TvTime";
    private static final boolean DEBUG = true;
    private long diff = 0;
    private Context mContext;

    private final static String TV_KEY_TVTIME = "dtvtime";
    private final static String PROP_SET_SYSTIME_ENABLED = "persist.tv.getdtvtime.isneed";
    private TvControlDataManager mTvControlDataManager = null;
    private final static String TV_STREAM_TIME = "vendor.sys.tv.stream.realtime";//to fit for dtvkit

    public TvTime(Context context){
        mContext = context;
        mTvControlDataManager = TvControlDataManager.getInstance(mContext);
    }

    public synchronized void setTime(long time){
        Date sys = new Date();

        diff = time - sys.getTime();
        /*
        SystemControlManager SM = SystemControlManager.getInstance();
        if (SM.getPropertyBoolean(PROP_SET_SYSTIME_ENABLED, false)
                && (Math.abs(diff) > 1000)) {
            SystemClock.setCurrentTimeMillis(time);
            diff = 0;
            DaylightSavingTime daylightSavingTime = DaylightSavingTime.getInstance();
            daylightSavingTime.updateDaylightSavingTimeForce();
        }*/

        //mTvControlDataManager.putLong(mContext.getContentResolver(), TV_KEY_TVTIME, diff);
        setProp(TV_STREAM_TIME, String.valueOf(diff));
    }


    public synchronized long getTime(){
        Date sys = new Date();
        //diff = mTvControlDataManager.getLong(mContext.getContentResolver(), TV_KEY_TVTIME, 0);
        diff = getLong(TV_STREAM_TIME, 0);

        return sys.getTime() + diff;
    }


    public synchronized long getDiffTime(){
        //return mTvControlDataManager.getLong(mContext.getContentResolver(), TV_KEY_TVTIME, 0);
        return getLong(TV_STREAM_TIME, 0);
    }

    public synchronized void setDiffTime(long diff){
        this.diff = diff;
        //mTvControlDataManager.putLong(mContext.getContentResolver(), TV_KEY_TVTIME, this.diff);
        setProp(TV_STREAM_TIME, String.valueOf(this.diff));
    }

    public String getProp(String key, String def) throws IllegalArgumentException {
        String result = def;
        try {
            Class SystemPropertiesClass = Class.forName("android.os.SystemProperties");
            Method getIntMethod =
                    SystemPropertiesClass.getDeclaredMethod("get", String.class, String.class);
            getIntMethod.setAccessible(true);
            result = (String) getIntMethod.invoke(SystemPropertiesClass, key, def);
        } catch (InvocationTargetException
                | IllegalAccessException
                | NoSuchMethodException
                | ClassNotFoundException e) {
            Log.e(TAG, "Failed to invoke SystemProperties.get()", e);
        }
        if (DEBUG) {
            Log.d(TAG, "getProp key = " + key + ", result = " + result);
        }
        return result;
    }

    public boolean setProp(String key, String def) throws IllegalArgumentException {
        boolean result = false;
        try {
            Class SystemPropertiesClass = Class.forName("android.os.SystemProperties");
            Method getIntMethod =
                    SystemPropertiesClass.getDeclaredMethod("set", String.class, String.class);
            getIntMethod.setAccessible(true);
            getIntMethod.invoke(SystemPropertiesClass, key, def);
            result = true;
        } catch (InvocationTargetException
                | IllegalAccessException
                | NoSuchMethodException
                | ClassNotFoundException e) {
            Log.e(TAG, "Failed to invoke SystemProperties.set()", e);
        }
        if (DEBUG) {
            Log.d(TAG, "setProp key = " + key + ", def = " + def + ", result = " + result);
        }
        return result;
    }

    public long getLong(String key, long def) throws IllegalArgumentException {
        long result = def;
        try {
            Class SystemPropertiesClass = Class.forName("android.os.SystemProperties");
            Method getIntMethod =
                    SystemPropertiesClass.getDeclaredMethod("getLong", String.class, long.class);
            getIntMethod.setAccessible(true);
            result = (long) getIntMethod.invoke(SystemPropertiesClass, key, def);
        } catch (InvocationTargetException
                | IllegalAccessException
                | NoSuchMethodException
                | ClassNotFoundException e) {
            Log.e(TAG, "Failed to invoke SystemProperties.getLong()", e);
        }
        if (DEBUG) {
            Log.d(TAG, "getLong key = " + key + ", result = " + result);
        }
        return result;
    }
}

