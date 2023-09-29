/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC HdmiCecActiveness
 */

package com.droidlogic.hdmi;

import android.os.IBinder;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.ServiceSpecificException;
import android.hardware.tv.hdmi.cec.CecMessage;
import android.hardware.tv.hdmi.cec.IHdmiCec;
import android.hardware.tv.hdmi.cec.IHdmiCecCallback;
import android.util.Log;

public class HdmiCecAidlClient  implements IBinder.DeathRecipient {
    private static final String TAG = "HdmiCecService";

    private IHdmiCec mHdmiCec;
    private HdmiCecCallbackAidl mAidlCallback;

    boolean connectToHal() {
        mHdmiCec =
                IHdmiCec.Stub.asInterface(
                        ServiceManager.getService(IHdmiCec.DESCRIPTOR + "/default"));
        if (mHdmiCec == null) {
            Log.e(TAG, "Could not initialize HDMI CEC AIDL HAL");
            return false;
        }
        try {
            mHdmiCec.asBinder().linkToDeath(this, 0);
        } catch (RemoteException e) {
            Log.e(TAG, "Couldn't link to death : ", e);
        }
        return true;
    }

    @Override
    public void binderDied() {
        // One of the services died, try to reconnect to both.
        mHdmiCec.asBinder().unlinkToDeath(this, 0);
        Log.e(TAG, "HDMI CEC service died, reconnecting");
        if (!connectToHal()) {
            Log.e(TAG, "It can't be connectted to cec hal");
            return;
        }
        // Reconnect the callback
        if (mAidlCallback != null) {
            setCallback(mAidlCallback.getCallback());
        }
    }

    public void setCallback(CecCallback callback) {
        mAidlCallback = new HdmiCecCallbackAidl(callback);
        try {
            // Create an AIDL callback that can callback onCecMessage
            mHdmiCec.setCallback(mAidlCallback);
        } catch (RemoteException e) {
            Log.e(TAG, "Couldn't initialise tv.cec callback : ", e);
        }
    }


    public void setLanguage(String language) {
        try {
            mHdmiCec.setLanguage(language);
        } catch (RemoteException e) {
            Log.e(TAG, "Failed to set language : ", e);
        }
    }


    public interface CecCallback {
        public void onCecMessage(int initiator, int destination, byte[] body);
    }

    private static final class HdmiCecCallbackAidl extends IHdmiCecCallback.Stub {
        private final CecCallback mCecCallback;

        public HdmiCecCallbackAidl(CecCallback cecCallback) {
            mCecCallback = cecCallback;
        }

        public CecCallback getCallback() {
            return mCecCallback;
        }

        @Override
        public void onCecMessage(CecMessage message) throws RemoteException {
            mCecCallback.onCecMessage(message.initiator, message.destination, message.body);
        }

        @Override
        public synchronized String getInterfaceHash() throws RemoteException {
            return IHdmiCecCallback.Stub.HASH;
        }

        @Override
        public int getInterfaceVersion() throws RemoteException {
            return IHdmiCecCallback.Stub.VERSION;
        }
    }

}

