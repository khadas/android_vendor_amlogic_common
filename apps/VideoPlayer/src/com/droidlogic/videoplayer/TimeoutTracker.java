package com.droidlogic.videoplayer;

import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.util.Log;

public class TimeoutTracker implements Runnable {
    private final String mName;
    private final boolean mLogPrint;
    private final boolean mRunInCallingThread;

    private TimeoutListener mListener;
    private long mTimeoutVal;
    private Handler mHandler;

    public TimeoutTracker(String name) {
        this(name, false);
    }
    public TimeoutTracker(String name, boolean runInCallingThread) {
        this(name, true, false);
    }

    public TimeoutTracker(String name, boolean runInCallingThread, boolean logPrint) {
        mName = name;
        mRunInCallingThread = runInCallingThread;
        mLogPrint = logPrint;
    }

    public void startTrack() {
        startTrack(false);
    }

    public void startTrack(boolean reset) {
        logIfNeeded("startTrack");

        if (mHandler == null) {
            initHandler();
        }

        if (reset) {
            mHandler.removeCallbacks(this);
        }

        mHandler.postDelayed(this, mTimeoutVal);
    }

    private void initHandler() {
        Looper looper = Looper.myLooper();
        if (!mRunInCallingThread) {
            HandlerThread ht = new HandlerThread(mName);
            ht.start();
            looper = ht.getLooper();
        }

        if (looper == null) {
            looper = Looper.getMainLooper();
        }

        mHandler = new Handler(looper);
    }

    public void stopTrack() {
        logIfNeeded("stopTrack");
        if (mHandler != null) {
            mHandler.removeCallbacks(this);
            if (!mRunInCallingThread) {
                mHandler.getLooper().quit();
            }
            mHandler = null;
        }
    }

    public void setTimeoutValue(long ms) {
        mTimeoutVal = ms > 0 ? ms : 0;
    }

    public void setTrackListener(TimeoutListener l) {
        mListener = l;
    }

    private void logIfNeeded(String msg) {
        if (mLogPrint) {
            Log.d(mName, msg);
        }
    }

    private void notifyTimeout() {
        if (mListener != null) {
            mListener.onTimeout();;
        }
    }

    @Override
    public void run() {
        logIfNeeded("notifyTimeout start");
        notifyTimeout();
        logIfNeeded("notifyTimeout end");
    }

    public interface TimeoutListener {
        public void onTimeout();
    }

}
