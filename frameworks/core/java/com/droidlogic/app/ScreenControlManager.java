/*
**
** Copyright 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

package com.droidlogic.app;

import android.content.Context;
import android.os.HwBinder;
import android.os.RemoteException;
import android.graphics.Rect;
import android.graphics.Bitmap;
import java.util.NoSuchElementException;
import android.util.Log;
import java.io.File;
import java.lang.reflect.Method;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Arrays;
import android.os.Build;
import android.content.Intent;
import android.content.BroadcastReceiver;
import android.content.IntentFilter;
import java.lang.ref.WeakReference;

public class ScreenControlManager {
    private static final String TAG = "ScreenControlManager";
    private static final int SCREEN_CONTROL_DEATH_COOKIE = 1001;
    public static final int REMOTE_EXCEPTION = -0xffff;
    private static ScreenControlManager mInstance = null;
    private Context mContext = null;
    private BroadcastReceiver mStrReceiver = null;
    private YuvCallbackListener mYuvCbl = null;
    private AvcCallbackListener mAvcCbl = null;

    static final int MSG_DATA_TYPE_YUV = 0;
    static final int MSG_DATA_TYPE_AVC = 1;

    // Mutex for all mutable shared state.
    private final Object mLock = new Object();

    static {
        System.loadLibrary("screencontrol_jni");
    }

    private native void native_ConnectScreenControl();
    private native int native_ScreenCap(int left, int top, int right, int bottom, int width, int height, int sourceType, String filename);
    private native byte[] native_ScreenCapBuffer(int left, int top, int right, int bottom, int width, int height, int sourceType);
    private native int native_ScreenRecord(int left, int top, int right, int bottom, int width, int height, int frameRate, int bitRate, int limitTimeSec, int sourceType, String filename);
    private native void native_StartReceiver(WeakReference<ScreenControlManager> wo);
    //avc
    private native int native_startAvcRecord(int left, int top, int right, int bottom, int width, int height, int frameRate, int bitRate, int sourceType);

    //yuv
    private native int native_startYuvRecord(int left, int top, int right, int bottom, int width, int height, int frameRate, int sourceType);


    private native void native_ForceStop();

    public ScreenControlManager(Context context) {
        mContext = context;
        native_ConnectScreenControl();
        registerReceiver();
    }

    public ScreenControlManager() {
        native_ConnectScreenControl();
        registerReceiver();
    }

    public void setYuvCallbackListener(YuvCallbackListener l) {
        mYuvCbl = l;
    }
    public void setAvcCallbackListener(AvcCallbackListener l) {
        mAvcCbl = l;
    }


    public static ScreenControlManager getInstance() {
         if (null == mInstance) mInstance = new ScreenControlManager();
         return mInstance;
    }

    public static ScreenControlManager getInstance(Context ctx) {
         if (null == mInstance) mInstance = new ScreenControlManager(ctx);
         return mInstance;
    }

    private void connectToProxy() {
    }

    private void registerReceiver() {
        if (null == mContext) {
            Log.w(TAG, "Context=null, fail to register receiver");
            return;
        }
        final IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_SCREEN_ON);
        filter.addAction(Intent.ACTION_SCREEN_OFF);
        filter.addAction(Intent.ACTION_SHUTDOWN);

        mStrReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                switch (intent.getAction()) {
                    case Intent.ACTION_SCREEN_ON:
                        Log.i(TAG, "Get action [Screen on]");
                        break;
                    case Intent.ACTION_SCREEN_OFF:
                        Log.i(TAG, "Get action [Screen off]");
                        native_ForceStop();
                        break;
                    case Intent.ACTION_SHUTDOWN:
                        Log.i(TAG, "Get action [Shutdown]");
                        native_ForceStop();
                        break;
                    default: break;
                }
            }
        };
        mContext.registerReceiver(mStrReceiver, filter);
    }

    private void unregisterReceiver() {
        if (null == mContext) {
            Log.w(TAG, "Context=null, fail to unregister receiver");
            return;
        }
        if (Build.VERSION.SDK_INT >= 7) {
            try {
                mContext.unregisterReceiver(mStrReceiver);
            } catch (Exception e) {
                e.printStackTrace();
            }
        } else {
            mContext.unregisterReceiver(mStrReceiver);
        }
        mStrReceiver = null;
    }

    public int startScreenCap(int left, int top, int right, int bottom, int width, int height, int sourceType, String filename) {
        Log.d(TAG, "startScreenCap left:" + left + ",top:"+ top + ",right:" + right + ",bottom:" + bottom + ",width:" + width + ",height:" + height + ",sourceType:" + sourceType + ",filename:" + filename);
        int result = 0;
        synchronized (mLock) {
            byte[] byteArr = startScreenCapBuffer(left, top, right, bottom, width, height, sourceType);
            if (byteArr != null) {
                Bitmap bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
                bitmap.copyPixelsFromBuffer(ByteBuffer.wrap(byteArr));
                saveBitmapAsPicture(bitmap, new File(filename));
            } else {
                result = REMOTE_EXCEPTION;
            }
        }
        return result;
    }

    public byte[] startScreenCapBuffer(int left, int top, int right, int bottom, int width, int height, int sourceType) {
        Log.d(TAG, "startScreenCapBuffer left:" + left + ",top:"+ top + ",right:" + right + ",bottom:" + bottom + ",width:" + width + ",height:" + height + ",sourceType:" + sourceType);
        ByteBuffer byteBuffer;
        byte[] byteArray = null;
        synchronized (mLock) {
            try {
                return native_ScreenCapBuffer(left, top, right, bottom, width, height, sourceType);
            } catch (Exception e) {
                Log.e(TAG, "startScreenCapBuffer: ScreenControlService is dead!:" + e);
            }
        }
        return null;
    }

    public int startScreenRecord(int width, int height, int frameRate, int bitRate, int limitTimeSec, int sourceType, String filename) {
        return startScreenRecord(0, 0, width, height, width, height, frameRate, bitRate, limitTimeSec, sourceType, filename);
    }

    public int startScreenRecord(int left, int top, int right, int bottom, int width, int height, int frameRate, int bitRate, int limitTimeSec, int sourceType, String filename) {
        Log.d(TAG, "startScreenRecord left:" + left + ",top:"+ top + ",right:" + right + ",bottom:" + bottom + ",width:" + width + ",height:" + height + ",sourceType:" + sourceType + ",frameRate:" + frameRate + ",bitRate:" + bitRate + ",limitTimeSec:" + limitTimeSec + ",filename:" + filename);
        synchronized (mLock) {
            try {
                return native_ScreenRecord(left, top, right, bottom, width, height, frameRate, bitRate, limitTimeSec, sourceType, filename);
            } catch (Exception e) {
                Log.e(TAG, "startScreenRecord: ScreenControlService is dead!:" + e);
            }
        }
        return REMOTE_EXCEPTION;
    }

    public int startAvcScreenRecord(int width, int height, int frameRate, int bitRate, int sourceType) {
        return startAvcScreenRecord(0,0,width,height,width, height, frameRate, bitRate,sourceType);
    }

    public int startAvcScreenRecord(int left, int top, int right, int bottom, int width, int height, int frameRate, int bitRate, int sourceType) {
        Log.d(TAG, "startAvcScreenRecord left:" + left + ",top:"+ top + ",right:" + right + ",bottom:" + bottom + ",width:" + width +
                    ",height:"+ height + ",frameRate:" + frameRate + ",bitRate:" + bitRate + ",sourceType:" + sourceType );
        synchronized (mLock) {
            try {
                if (mAvcCbl == null)
                    return REMOTE_EXCEPTION;
                native_StartReceiver(new WeakReference<ScreenControlManager>(this));
                return native_startAvcRecord(left, top, right, bottom, width, height, frameRate, bitRate,sourceType);
            } catch (Exception e) {
                Log.e(TAG, "startAvcScreenRecord: ScreenControlService is dead!:" + e);
            }
        }
        return REMOTE_EXCEPTION;
    }

    public int startYuvScreenRecord(int width, int height, int frameRate, int sourceType) {
        return startYuvScreenRecord(0,0,width,height,width, height, frameRate,sourceType);
    }

    public int startYuvScreenRecord(int left, int top, int right, int bottom, int width, int height, int frameRate, int sourceType) {
        Log.d(TAG, "startYuvScreenRecord left:" + left + ",top:"+ top + ",right:" + right + ",bottom:" + bottom + ",width:" + width + ",height:"+ height +
                ",frameRate:" + frameRate  + ",sourceType:" + sourceType );
        synchronized (mLock) {
            try {
                if (mYuvCbl == null)
                    return REMOTE_EXCEPTION;
                native_StartReceiver(new WeakReference<ScreenControlManager>(this));
                return native_startYuvRecord(left, top, right, bottom, width, height, frameRate,sourceType);
            } catch (Exception e) {
                Log.e(TAG, "startYuvScreenRecord: ScreenControlService is dead!:" + e);
            }
        }
        return REMOTE_EXCEPTION;
    }

    public void stopRecord() {
        native_ForceStop();
    }

    public void release()
    {
        Log.d(TAG, "release()");
        unregisterReceiver();
    }

    /* save as jpeg or png picture */
    private boolean saveBitmapAsPicture(Bitmap bmp, File f) {
        boolean success = false;
        try {
            FileOutputStream out = new FileOutputStream(f);
            String filename = f.getName();
            if (filename.contains(".jpeg") || filename.contains(".jpg"))
                success = bmp.compress(Bitmap.CompressFormat.JPEG, 100, out);
            else if (filename.contains(".png"))
                success = bmp.compress(Bitmap.CompressFormat.PNG, 100, out);
            out.flush();
            out.close();
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }
        return success;
    }

    public static interface YuvCallbackListener {

        void onYuvAvailable(byte[] data);
    }
    public static interface AvcCallbackListener {
        void onAvcAvailable(byte[] data, int frameType, long pts);
    }

    private int onMessage(int type, int frameType, long pts , Object x){
        YuvCallbackListener yuvCb = mYuvCbl;
        AvcCallbackListener avcCb = mAvcCbl;
        Log.i(TAG, "onMessage type=" + type +",frameType="+frameType + ",pts=" + pts);
        if (type == MSG_DATA_TYPE_AVC && avcCb != null) {
            avcCb.onAvcAvailable((byte[]) x, frameType, pts);
        }else if (type == MSG_DATA_TYPE_YUV && mYuvCbl != null ) {
            yuvCb.onYuvAvailable((byte[]) x);
        }
        return 0;

    }

    static int native_proc(Object o, int type,int frameType, long pts, Object x){
        WeakReference<ScreenControlManager> wo;
        ScreenControlManager f;
        if (o == null)
          return 0;
        try {
          wo = (WeakReference<ScreenControlManager>) o;
          f = wo.get();
          if (f == null )
            return 0;
          return f.onMessage(type, frameType, pts, x);
        }catch (Throwable e) {
          e.printStackTrace();
          return 0;
        }

    }


    final class DeathRecipient implements HwBinder.DeathRecipient {
        DeathRecipient() {
        }

        @Override
        public void serviceDied(long cookie) {
            if (SCREEN_CONTROL_DEATH_COOKIE == cookie) {
                Log.e(TAG, "screen control service died cookie: " + cookie);
            }
        }
    }
}

