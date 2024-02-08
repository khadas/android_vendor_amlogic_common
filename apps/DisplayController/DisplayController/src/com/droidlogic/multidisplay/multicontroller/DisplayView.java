package com.droidlogic.multidisplay.multicontroller;


import android.content.Context;
import android.content.res.Configuration;
import android.graphics.Rect;
import android.hardware.display.DisplayManager;
import android.os.Build;
import android.os.RemoteException;
import android.util.Log;
import android.view.Display;
import android.view.Surface;
import android.view.SurfaceControl;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

public class DisplayView extends SurfaceView implements View.OnClickListener {

    private static final String TAG = "SSS";
    private final DisplayManager mDisplayManager;
    private final SurfaceControl mSurfaceControl;
    private final SurfaceControl.Transaction mTransaction = new SurfaceControl.Transaction();
    public DisplayMoveListener mMoveListener;
    boolean mFull;
    private FullScreenChangeListener mListener;
    private int mMirroredDisplayId;
    private boolean mHasMirror;
    private Rect mDisplayRect;
    private Rect mFullScreenRect;
    SurfaceHolder.Callback mSHCallback = new SurfaceHolder.Callback() {

        @Override
        public void surfaceCreated(SurfaceHolder holder) {
            mDisplayRect = new Rect(DisplayView.this.getLeft(), DisplayView.this.getTop(), DisplayView.this.getRight(), DisplayView.this.getBottom());
            Log.d(TAG, "-->surfaceCreated" + mDisplayRect + ":" + DisplayView.this.getTop() + ":" + DisplayView.this.getBottom());
            if (!mHasMirror) {
                createMirror();
            }

        }

        @Override
        public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
            Rect currentRect = new Rect(0, 0, DisplayView.this.getWidth(), DisplayView.this.getHeight());
            Log.d(TAG, "surfaceChanged " + currentRect + " on Display " + mMirroredDisplayId);
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                mTransaction.setVisibility(mSurfaceControl, true).setGeometry(mSurfaceControl, mFullScreenRect, currentRect, Surface.ROTATION_0).apply();
            }
        }

        @Override
        public void surfaceDestroyed(SurfaceHolder holder) {
            if (mHasMirror) {
                removeMirror();
            }
        }
    };
    private int mClickTimes;

    public DisplayView(Context context) {
        super(context);
        mFull = false;
        mDisplayManager = context.getSystemService(DisplayManager.class);
        mSurfaceControl = new SurfaceControl.Builder().setName("mirrorSurfaceControl").build();
        mMirroredDisplayId = -1;
        getHolder().addCallback(mSHCallback);
        setOnClickListener(this);
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();
        mClickTimes = 0;
    }

    @Override
    protected void onDetachedFromWindow() {
        super.onDetachedFromWindow();
        mClickTimes = 0;
    }

    public void setFullscreen(boolean full) {
        if (mListener != null) {
            mListener.onFullScreenChanged(mMirroredDisplayId, full);
        }
        // Rect fullScreenRect = new Rect(0, 0, getDisplay().getWidth(), getDisplay().getHeight());
        if (full) {
            setFull();
            Log.d(TAG, "full " + mFullScreenRect);
        } else {
            Log.d(TAG, "mDisplayRect" + mDisplayRect);
            setDefault();
        }
    }

    public int getMirroredDisplayId() {
        return mMirroredDisplayId;
    }

    public void setMirrorDisplayId(int id) {
        this.mMirroredDisplayId = id;
        Display d = mDisplayManager.getDisplay(id);
        mFullScreenRect = new Rect(0, 0, d.getWidth(), d.getHeight());
        Log.d(TAG, "setDisplay get Rect " + mFullScreenRect);
    }

    public void setFullScreenListener(FullScreenChangeListener l) {
        this.mListener = l;
    }

    private void createMirror() {
        Log.d(TAG, "createMirror --" + mMirroredDisplayId);
        if (mMirroredDisplayId != -1) {
            boolean success = false;
            try {
                Class globalclass = Class.forName("android.view.WindowManagerGlobal");
                Method getWmServiceMethod = globalclass.getDeclaredMethod("getWindowManagerService");
                getWmServiceMethod.setAccessible(true);
                Object iWindowManager = getWmServiceMethod.invoke(null);
                Method mirrorDisplay = iWindowManager.getClass().getMethod("mirrorDisplay", int.class, SurfaceControl.class);
                mirrorDisplay.setAccessible(true);
                mirrorDisplay.invoke(iWindowManager, mMirroredDisplayId, mSurfaceControl);
                success = true;
            } catch (NoSuchMethodException e) {
                e.printStackTrace();
            } catch (ClassNotFoundException e) {
                e.printStackTrace();
            } catch (IllegalAccessException e) {
                e.printStackTrace();
            } catch (InvocationTargetException e) {
                e.printStackTrace();
            }
            Log.d(TAG, "Mirrored success " + success);
            if (!success) {
                return;
            }

            if (!mSurfaceControl.isValid()) {
                Log.d(TAG, "Mirrored failed " + mMirroredDisplayId);
                return;
            }

            mHasMirror = true;
            Rect rect = new Rect(0, 0, mDisplayRect.width(), mDisplayRect.height());
            Log.d(TAG, "mDisplayRect.left,mDisplayRect.top" + mDisplayRect + "---" + mFullScreenRect);
            mTransaction.setVisibility(mSurfaceControl, true).reparent(mSurfaceControl, getSurfaceControl()).setGeometry(mSurfaceControl, mFullScreenRect, rect, Surface.ROTATION_0).apply();
        }

    }

    public void removeMirror() {
        if (mSurfaceControl.isValid()) {
            mTransaction.setVisibility(mSurfaceControl, false).apply();
        }
        mHasMirror = false;
    }

    public void setFull() {
        Log.d(TAG, "mDisplay " + mMirroredDisplayId + " setFull ");
        setLeftTopRightBottom(0, 0, getDisplay().getWidth(), getDisplay().getHeight());
        mFull = true;
    }

    public void setSmall() {
        if (!mHasMirror) {
            return;
        }
        Log.d(TAG, "mDisplay " + mMirroredDisplayId + " setSmall ");
        setLeftTopRightBottom(0, 0, 1, 1);
        mFull = false;
    }

    public void setDefault() {
        Log.d(TAG, "mDisplay " + mMirroredDisplayId + " setDefault ");
        setLeftTopRightBottom(mDisplayRect.left, mDisplayRect.top, mDisplayRect.right, mDisplayRect.bottom);
        mFull = false;
    }

    @Override
    public void onClick(View v) {
        mClickTimes = ( (++mClickTimes) % 3);
        Log.d(TAG,"mClickTimes"+mClickTimes);
        if (mClickTimes == 2) {
            setFullscreen(!mFull);
        }
    }

    public void setMoveListener(DisplayMoveListener listener) {
        mMoveListener = listener;
    }

    // @Override
    public void onMovedToDisplay(int displayId, Configuration config) {
        if (mMoveListener != null) {
            mMoveListener.onMovedToDisplay(displayId, config);
        }
    }

    interface FullScreenChangeListener {
        void onFullScreenChanged(int displaySize, boolean full);
    }

    public interface DisplayMoveListener {
        void onMovedToDisplay(int displayId, Configuration config);
    }

}
