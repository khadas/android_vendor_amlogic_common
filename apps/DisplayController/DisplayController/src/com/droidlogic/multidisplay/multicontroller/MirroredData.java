package com.droidlogic.multidisplay.multicontroller;

import android.util.Log;

public class MirroredData {
    private int mFromDisplay;
    private int mDisplayId;
    private boolean mirrored;
    private String mDeviceName;
    private static final String TAG = "MM";

    public MirroredData(int displayId, boolean mirrored) {
        this.mDisplayId = displayId;
        this.mirrored = mirrored;
    }

    public int getDisplayId() {
        return mDisplayId;
    }

    public void setDisplayId(int displayId) {
        this.mDisplayId = displayId;
    }

    public void setFromDisplayId(int displayId) {
        Log.d(TAG,"setFromDisplayId"+displayId+" mDisplayId"+mDisplayId+" oldFrom"+mFromDisplay+" ,mirrored"+mirrored);
        this.mFromDisplay = displayId;
    }

    public int getFromDisplayId() {
        return this.mFromDisplay;
    }

    public String getName(boolean isMirrored) {
        if (isMirrored) {
            return "IS MIRRORING " + mDisplayId;
        }
        return mFromDisplay+" MIRROR TO DEVICE"+mDisplayId;
    }

    public boolean isMirrored() {
        return mirrored;
    }

    public void setMirrored(boolean mirrored) {
        Log.d(TAG,"setMirrored"+mirrored+" mDisplayId"+mDisplayId);
        this.mirrored = mirrored;
    }
}
