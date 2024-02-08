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

    boolean startMirror(int displayId, int tDisplayId, boolean control) {
        boolean ret = false;
        try {
            ret = mStub.startMirror(displayId, tDisplayId, control);
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

    boolean isControlled(int displayId) {
        try {
            return mStub.isControlled(displayId);
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

    int getMirroredId(int displayId) {
        try {
            return mStub.getMirroredId(displayId);
        } catch (RemoteException ex) {

        }return 0;
    }

    int getPhyPort(int displayId) {
        try {
            return mStub.getPhyPort(displayId);
        } catch (RemoteException ex) {

        }return 0;
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
