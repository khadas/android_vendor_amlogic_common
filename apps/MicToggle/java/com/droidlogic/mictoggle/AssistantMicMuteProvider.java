/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC AssistantMicMuteProvider
 */

package com.droidlogic.mictoggle;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.content.Context;
import android.content.UriMatcher;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.net.Uri;

import android.util.Log;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.text.TextUtils;

import android.database.sqlite.SQLiteOpenHelper;
import com.droidlogic.app.DroidLogicUtils;
import com.droidlogic.app.SystemControlManager;

public class AssistantMicMuteProvider extends ContentProvider {

    private Context mContext;
    DBHelper mDbHelper = null;
    SQLiteDatabase db = null;

    private static final String HOTWORDMIC_AUTH = "atv.hotwordmic";
    //table name
    private static final String TOGGLESTATE = "togglestate";
    public static final Uri HOTWORDMIC_URI = new Uri.Builder().scheme("content")
                                                  .authority(HOTWORDMIC_AUTH)
                                                  .appendPath(TOGGLESTATE)
                                                  .build();
    //colume name
    public static final String COLUME_ID = "state" ;


    private static final String TAG = "AssistantMicMuteProvider";

    public static final int Toggle_Code = 1;

    private static final UriMatcher mMatcher;
    static{
        mMatcher = new UriMatcher(UriMatcher.NO_MATCH);
        mMatcher.addURI(HOTWORDMIC_AUTH, TOGGLESTATE, Toggle_Code);
    }


    @Override
    public boolean onCreate() {

        mContext = getContext();

        mDbHelper = new DBHelper(getContext());
        db = mDbHelper.getWritableDatabase();

        boolean mic_enable = getMicToggleState();
        String mic_state = mic_enable ? "1" : "0";
        Log.d(TAG, "mic_state:" + mic_state);

        db.execSQL("delete from " + TOGGLESTATE);
        db.execSQL("insert into " + TOGGLESTATE + " values(" + mic_state + ");");

        return true;
    }

    @Override
    public Uri insert(Uri uri, ContentValues values) {

        String table = getTableName(uri);

        if (isAllowedPackage()) {
            db.insert(table, null, values);
            mContext.getContentResolver().notifyChange(uri, null);
        }
        return uri;
   }


    @Override
    public Cursor query(Uri uri, String[] projection, String selection,
                        String[] selectionArgs, String sortOrder) {

        String table = getTableName(uri);

        if (isAllowedPackage())
            return db.query(table,projection,selection,selectionArgs,null,null,sortOrder,null);

        return null;
    }


    @Override
    public int update(Uri uri, ContentValues values, String selection,
                      String[] selectionArgs) {
        int ret = 0;
        String table = getTableName(uri);
        if (isAllowedPackage()) {
            ret = db.update(table, values, selection, selectionArgs );
            mContext.getContentResolver().notifyChange(uri, null);
        }
        return ret;
    }

    @Override
    public int delete(Uri uri, String selection, String[] selectionArgs) {
        return 0;
    }

    @Override
    public String getType(Uri uri) {
        return null;
    }

    private String getTableName(Uri uri){
        String tableName = null;
        switch (mMatcher.match(uri)) {
            case Toggle_Code:
                tableName = TOGGLESTATE;
                break;
        }
        return tableName;
   }

   private boolean isAllowedPackage() {
       String pkg = getCallingPackage();
       ApplicationInfo info = null;
       try {
            info = getContext().getPackageManager().getApplicationInfo(pkg, 0);
       } catch (PackageManager.NameNotFoundException e) {
       }
       boolean ret = !TextUtils.isEmpty(pkg) && info != null &&
              ((info.flags & ApplicationInfo.FLAG_SYSTEM) != 0 ||
              (info.flags & ApplicationInfo.FLAG_UPDATED_SYSTEM_APP) != 0);
       return ret;
  }


    private class DBHelper extends SQLiteOpenHelper {
        private static final String TAG = "DBHelper";

        //name of database
        private static final String DATABASE_NAME = "mic.db";

        private static final int DATABASE_VERSION = 1;
        //database version

        public DBHelper(Context context) {
            super(context, DATABASE_NAME, null, DATABASE_VERSION);
        }

        @Override
        public void onCreate(SQLiteDatabase db) {
            db.execSQL("CREATE TABLE IF NOT EXISTS " + TOGGLESTATE + "(" +COLUME_ID + " INTEGER PRIMARY KEY AUTOINCREMENT)");
        }

        @Override
        public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion)   {
            Log.d(TAG, "DBHelper.onUpgrade");
        }


    }


    public static SystemControlManager getSystemControlManager() {
        return SystemControlManager.getInstance();
    }

    public static boolean getMicToggleState() {
        String ret = getSystemControlManager().readSysFs("/sys/class/gpio_keypad/table").replaceAll("\n", "");
        Log.d(TAG, "getMicToggleState:"+ret);

        int a = ret.indexOf("name = mute");
        String item_mute = ret.substring(a);
        int pos_on = item_mute.indexOf("1");
        int pos_off = item_mute.indexOf("0");
        if (pos_on < 0 && pos_off < 0)
            return false;

        if (pos_on > 0 && pos_off > 0)
            return pos_on > pos_off ? false : true;

        return pos_on > 0 ? true : false;

    }

}


