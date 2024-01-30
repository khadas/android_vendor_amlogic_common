package com.droidlogic.multidisplay.multicontroller;

import android.os.RemoteException;

import com.droidlogic.displaycontrollerservice.IMirrorDisplayInterface;

public class MirrorDisplayWrapper {
    IMirrorDisplayInterface mStub;
    UICallback mCallback;

    public MirrorDisplayWrapper(IMirrorDisplayInterface stub, UICallback callback) {
        mStub = stub;
        mCallback = callback;
    }

    boolean startMirror(int displayId, int tDisplayId) {
        boolean ret = false;
        try {
            ret = mStub.startMirror(displayId, tDisplayId);
            if (ret) mCallback.updateUI();
        } catch (RemoteException ex) {

        }
        return ret;
    }

    boolean isMirroring(int displayId) {
        try {
            return mStub.isMirroring(displayId);
        } catch (RemoteException ex) {

        }
        return false;
    }

    boolean isMirrored(int displayId) {
        try {
            return mStub.isMirrored(displayId);
        } catch (RemoteException ex) {

        }
        return false;
    }

    void stopMirror(int displayId) {
        try {
            mStub.stopMirror(displayId);
            mCallback.updateUI();
        } catch (RemoteException ex) {

        }
    }

    boolean swithDisplay(int displayId, int toDisplayId) {
        try {
            return mStub.swithDisplay(displayId, toDisplayId);
        } catch (RemoteException ex) {

        }
        return false;
    }

    public interface UICallback {
        void updateUI();
    }
}
