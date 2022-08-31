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
import android.content.ContentProvider;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.database.sqlite.SQLiteOpenHelper;
import android.database.sqlite.SQLiteDatabase;
import android.database.Cursor;
import android.net.Uri;
import android.content.UriMatcher;
import android.util.Log;

public class TvControlDataManager {
    private static final String TAG = "TvControlDataManager";
    private static final boolean DEBUG = Log.isLoggable("DataProvider", Log.DEBUG);

    public static final String DB_NAME = "tv_control_data.db";
    public static final String TABLE_SCAN_NAME = "tv_control_scan";
    public static final String TABLE_SOUND_NAME = "tv_control_sound";
    public static final String TABLE_PPPOE_NAME = "tv_control_pppoe";
    public static final String TABLE_OTHERS_NAME = "tv_control_others";

    public static final int DB_VERSION = 1;
    public static final String KEY_INIT = "tv_control_init";

    public static final int TABLE_SCAN_CODE = 1;
    public static final int TABLE_SOUND_CODE = 2;
    public static final int TABLE_PPPOE_CODE = 3;
    public static final int TABLE_OTHERS_CODE = 4;

    public static final String TABLE_STRING_NAME = "string_table";
    public static final String PROPERTY = "property";
    public static final String VALUE = "value";
    public static final String DESCRIP = "description";
    public static final String TABLE_PROPERTY = "(_id INTEGER PRIMARY KEY AUTOINCREMENT, " + PROPERTY + " TEXT UNIQUE ON CONFLICT REPLACE, " + VALUE + " TEXT, " + DESCRIP + " TEXT);";

    public static final String AUTHORITY = "com.droidlogic.database";
    public static final String CONTENT_URI = "content://" + AUTHORITY + "/";

    private ContentResolver mContentResolver = null;
    private Context mContext = null;

    private static TvControlDataManager mInstance = null;

    private TvControlDataManager(Context context) {
        mContext = context;
        mContentResolver = mContext.getContentResolver();
    }

    public synchronized static TvControlDataManager getInstance(Context context) {
        if (null == mInstance) {
            mInstance = new TvControlDataManager(context);
        }

        return mInstance;
    }

    public boolean putString(ContentResolver resolver, String name, String value) {
        return putStringValue(mContext, name, value);
    }

    public String getString(ContentResolver resolver, String name) {
        return getStringValue(mContext, name, null);
    }

    public boolean putInt(ContentResolver resolver, String name, int value) {
        return putIntValue(mContext, name, value);
    }

    public int getInt(ContentResolver resolver, String name, int def) {
        return getIntValue(mContext, name, def);
    }

    public boolean putLong(ContentResolver resolver, String name, long value) {
        return putLongValue(mContext, name, value);
    }

    public long getLong(ContentResolver resolver, String name, long def) {
        return getLongValue(mContext, name, def);
    }

    public boolean putFloat(ContentResolver resolver, String name, float value) {
        return putFloatValue(mContext, name, value);
    }

    public float getFloat(ContentResolver resolver, String name, float def) {
        return getFloatValue(mContext, name, def);
    }

    public boolean putBoolean(ContentResolver resolver, String name, boolean value) {
        return putIntValue(mContext, name, value ? 1 : 0);
    }

    public boolean getBoolean(ContentResolver resolver, String name, boolean def) {
        int result = getIntValue(mContext, name, def ? 1 : 0);
        return result == 1 ;
    }

    public static boolean putStringValue(Context context, String name, String value) {
        return putStringValueToTable(context, TABLE_STRING_NAME, name, value);
    }

    public static String getStringValue(Context context, String name, String def) {
        return getStringValueFromTable(context, TABLE_STRING_NAME, name, def);
    }

    public static boolean putStringValueToTable(Context context, String table, String name, String value) {
        if (DEBUG) Log.d(TAG, "putStringValueToTable  table = " + table + ", name = " + name + ", value = " + value);
        boolean result = false;
        if (context == null) {
            //Log.d(TAG, "putStringValueToTable null context");
            return result;
        }
        Uri uri = Uri.parse(CONTENT_URI + table);
        ContentValues values = new ContentValues();
        values.put(PROPERTY, name);
        values.put(VALUE, value);
        Cursor cursor = null;
        try {
            cursor = context.getContentResolver().query(uri, new String[] { PROPERTY, VALUE}, PROPERTY + " = ?",
                    new String[]{ name }, null, null);
            if (cursor != null && cursor.moveToFirst()) {
                int row = context.getContentResolver().update(uri, values, PROPERTY + " = ?", new String[] { name });
                if (row != -1) {
                    result = true;
                }
                if (DEBUG) {
                    Log.d(TAG, "putStringValueToTable update row = " + row);
                }
            } else {
                Uri resultUri = context.getContentResolver().insert(uri, values);
                if (resultUri != null) {
                    result = true;
                }
                if (DEBUG) {
                    Log.d(TAG, "putStringValueToTable insert resultUri = " + resultUri);
                }
            }
        } catch (Exception e) {
            Log.e(TAG,"putStringValueToTable putString Exception  = " + e.getMessage());
            e.printStackTrace();
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
        if (DEBUG) {
            Log.d(TAG, "putStringValueToTable result = " + result);
        }
        return result;
    }

    public static String getStringValueFromTable(Context context, String table, String name, String def) {
        String result = def;
        if (context == null) {
            Log.d(TAG, "getStringValueFromTable null context");
            return result;
        }
        Uri uri = Uri.parse(CONTENT_URI + table);
        Cursor cursor = null;
        try {
            cursor = context.getContentResolver().query(uri, new String[] { PROPERTY, VALUE}, PROPERTY + " = ?",
                    new String[]{ name }, null, null);
            if (cursor != null && cursor.moveToFirst()) {
                result = cursor.getString(cursor.getColumnIndex(VALUE));
            }
        } catch (Exception e) {
            Log.e(TAG,"getStringValueFromTable Exception  = " + e.getMessage());
            e.printStackTrace();
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
        if (DEBUG) Log.d(TAG,"getStringValueFromTable table = " + table + ", name = " + name + ", value = " + result);
        return result;
    }

    public static boolean putBooleanValue(Context context, String name, boolean value) {
        boolean result = false;
        result = putStringValue(context, name, value ? "true" : "false");
        return result;
    }

    public static boolean getBooleanValue(Context context, String name, boolean def) {
        boolean result = def;
        String stringValue = getStringValue(context, name, def ? "true" : "false");
        if ("true".equals(stringValue)) {
            result = true;
        } else {
            result = false;
        }
        return result;
    }

    public static boolean putIntValue(Context context, String name, int value) {
        boolean result = false;
        result = putStringValue(context, name, String.valueOf(value));
        return result;
    }

    public static int getIntValue(Context context, String name, int def) {
        int result = def;
        String stringValue = getStringValue(context, name, String.valueOf(def));
        try {
            result = Integer.valueOf(stringValue);
        } catch (Exception e) {
            Log.d(TAG, "getIntValue Exception = " + e.getMessage());
        }
        return result;
    }

    public static boolean putLongValue(Context context, String name, long value) {
        boolean result = false;
        result = putStringValue(context, name, String.valueOf(value));
        return result;
    }

    public static long getLongValue(Context context, String name, long def) {
        long result = def;
        String stringValue = getStringValue(context, name, String.valueOf(def));
        try {
            result = Long.valueOf(stringValue);
        } catch (Exception e) {
            Log.d(TAG, "getLongValue Exception = " + e.getMessage());
        }
        return result;
    }

    public static boolean putFloatValue(Context context, String name, float value) {
        boolean result = false;
        result = putStringValue(context, name, String.valueOf(value));
        return result;
    }

    public static float getFloatValue(Context context, String name, float def) {
        float result = def;
        String stringValue = getStringValue(context, name, String.valueOf(def));
        try {
            result = Float.valueOf(stringValue);
        } catch (Exception e) {
            Log.d(TAG, "getFloatValue Exception = " + e.getMessage());
        }
        return result;
    }
}
