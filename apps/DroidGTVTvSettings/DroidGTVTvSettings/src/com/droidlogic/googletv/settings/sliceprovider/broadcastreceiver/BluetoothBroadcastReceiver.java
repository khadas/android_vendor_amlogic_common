package com.droidlogic.googletv.settings.sliceprovider.broadcastreceiver;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import com.droidlogic.googletv.settings.R;

import com.droidlogic.googletv.settings.sliceprovider.accessories.BluetoothDevicesService;

/** The {@BroadcastReceiver} for performing actions upon device boot. */
public class BluetoothBroadcastReceiver extends BroadcastReceiver {

    private static final String TAG = "BluetoothBroadcastReceiver";
    private static final boolean DEBUG = false;

    private static final String NATIVE_CONNECTED_DEVICE_SLICE_PROVIDER_URI =
            "content://com.google.android.tv.btservices.settings.sliceprovider/general";

    @Override
    public void onReceive(Context context, Intent intent) {
        if (DEBUG) {
            Log.i(TAG, "onReceive");
        }
        // Start the Service that supports ConnectedDevicesSliceProvider only if the URI is not
        // overlaid.
        if (context != null
                && NATIVE_CONNECTED_DEVICE_SLICE_PROVIDER_URI.equals(
                        context.getResources().getString(R.string.connected_devices_slice_uri))) {
            Intent mainIntent = new Intent(context, BluetoothDevicesService.class);
            context.startService(mainIntent);
        }
    }
}
