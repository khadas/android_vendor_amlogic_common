package com.droidlogic.multidisplay.multicontroller;

import android.util.Log;

public class MirroredData {
    private static final String TAG = "MM";
    private int mFromDisplay;
    private int mDisplayId;
    private boolean mirrored;
    private boolean mControl;
    private String mDeviceName;
    private Callback mCallback;

    public MirroredData(int displayId, boolean mirrored) {
        this.mDisplayId = displayId;
        this.mirrored = mirrored;
    }

    public void setCallback(Callback callback) {
        this.mCallback = callback;
    }

    public int getDisplayId() {
        return mDisplayId;
    }

    public void setDisplayId(int displayId) {
        this.mDisplayId = displayId;
    }

    public int getFromDisplayId() {
        return this.mFromDisplay;
    }

    public void setFromDisplayId(int displayId) {
        Log.d(TAG, "setFromDisplayId" + displayId + " mDisplayId" + mDisplayId + " oldFrom" + mFromDisplay + " ,mirrored" + mirrored);
        this.mFromDisplay = displayId;
    }

    public String getName(int displayId) {
        if (mCallback != null) {
            Log.d(TAG, "getName" + displayId + " /" + mDisplayId);
            if (displayId == mDisplayId) displayId = -1;
            if (displayId != -1) {
                return mCallback.getName(displayId) + " IS MIRRORING " + mCallback.getName(mDisplayId);
            }
            return mCallback.getName(mFromDisplay) + " MIRROR TO " + mCallback.getName(mDisplayId);
        } else {
            if (displayId != -1) {
                return displayId + " IS MIRRORING " + mDisplayId;
            }
            return mFromDisplay + " MIRROR TO " + mDisplayId;
        }

    }

    public boolean isMirrored() {
        return mirrored;
    }

    public void setMirrored(boolean mirrored) {
        Log.d(TAG, "setMirrored" + mirrored + " mDisplayId" + mDisplayId);
        this.mirrored = mirrored;
    }

    public boolean getControl() {
        return mControl;
    }

    public void setControl(boolean control) {
        Log.d(TAG, "setMirrored" + control + " mDisplayId" + mDisplayId);
        this.mControl = control;
    }


    public interface Callback {

        String getName(int displayId);
    }
}
