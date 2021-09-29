/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.droidlogic.googletv.settings.sliceprovider.accessories;

import static com.droidlogic.googletv.settings.sliceprovider.accessories.ConnectedDevicesSliceBroadcastReceiver.ACTION_TOGGLE_CHANGED;
import static com.droidlogic.googletv.settings.sliceprovider.accessories.ConnectedDevicesSliceBroadcastReceiver.BLUETOOTH_ON;
import static com.droidlogic.googletv.settings.sliceprovider.accessories.ConnectedDevicesSliceBroadcastReceiver.EXTRA_TOGGLE_TYPE;
import static com.droidlogic.googletv.settings.sliceprovider.accessories.ConnectedDevicesSliceUtils.EXTRAS_SLICE_URI;

import android.app.PendingIntent;
import android.app.tvsettings.TvSettingsEnums;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattService;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.UserHandle;
import android.provider.Settings;
import android.text.TextUtils;
import android.util.ArrayMap;
import android.util.Log;

import androidx.annotation.IntegerRes;
import androidx.core.graphics.drawable.IconCompat;
import androidx.slice.Slice;
import androidx.slice.SliceProvider;

import com.android.settingslib.bluetooth.CachedBluetoothDevice;
import com.droidlogic.googletv.settings.R;
import com.android.tv.twopanelsettings.slices.builders.PreferenceSliceBuilder;
import com.android.tv.twopanelsettings.slices.builders.PreferenceSliceBuilder.RowBuilder;

import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.UUID;
import java.lang.Thread;
import java.lang.InterruptedException;

/** The SliceProvider for "connected devices" settings */
public class ConnectedDevicesSliceProvider extends SliceProvider implements
        BluetoothDeviceProvider.Listener {

    private static final String TAG = "ConnectedDevices";
    private static final boolean DEBUG = true;

    private static final UUID GATT_BATTERY_SERVICE_UUID =
            UUID.fromString("0000180f-0000-1000-8000-00805f9b34fb");
    private static final UUID GATT_BATTERY_LEVEL_CHARACTERISTIC_UUID =
            UUID.fromString("00002a19-0000-1000-8000-00805f9b34fb");
    public static final UUID GATT_DEVICE_INFORMATION_UUID =
            UUID.fromString("0000180a-0000-1000-8000-00805f9b34fb");
    public static final UUID GATT_VERSION_CHARACTERISTIC_UUID =
            UUID.fromString("00002A28-0000-1000-8000-00805f9b34fb");

    public static final String ACTION_CONNECT_INPUT =
            "com.google.android.intent.action.CONNECT_INPUT";

    public static boolean deviceGattFlag = false;
    public String deviceBatteryLevel = "-1";
    public String deviceFirmwareVersion = "";

    private BluetoothGattCharacteristic bleVersion;
    private BluetoothGatt mBluetoothGatt;

    private boolean versionRequest = false;

    private final Map<Uri, Integer> mPinnedUris = new ArrayMap<>();
    private final Handler mHandler = new Handler(Looper.getMainLooper());

    private boolean mBtDeviceServiceBound;
    private BluetoothDevicesService.LocalBinder mBtDeviceServiceBinder;

    private final BluetoothDeviceProvider mLocalBluetoothDeviceProvider =
            new LocalBluetoothDeviceProvider() {
        BluetoothDeviceProvider getHostBluetoothDeviceProvider() {
            return getBluetoothDeviceProvider();
        }
    };

    private final ServiceConnection mBtDeviceServiceConnection =
            new SimplifiedConnection() {

                @Override
                public void onServiceConnected(ComponentName className, IBinder service) {
                    mBtDeviceServiceBinder = (BluetoothDevicesService.LocalBinder) service;
                    mBtDeviceServiceBinder.addListener(ConnectedDevicesSliceProvider.this);
                    getContext().getContentResolver()
                            .notifyChange(ConnectedDevicesSliceUtils.GENERAL_SLICE_URI, null);
                }

                @Override
                protected void cleanUp() {
                    if (mBtDeviceServiceBinder != null) {
                        mBtDeviceServiceBinder.removeListener(ConnectedDevicesSliceProvider.this);
                    }
                    mBtDeviceServiceBinder = null;
                }
            };

    static final String KEY_BLUETOOTH_TOGGLE = "bluetooth_toggle";
    static final String KEY_PAIR_REMOTE = "pair_remote";
    static final String KEY_ACCESSORIES = "accessories";
    static final String KEY_OFFICIAL_REMOTE = "official_remote";
    static final String KEY_CONNECT = "connect";
    static final String KEY_DISCONNECT = "disconnect";
    static final String KEY_RENAME = "rename";
    static final String KEY_FORGET = "forget";
    static final String KEY_EXTRAS_DEVICE = "extra_devices";

    static final int YES = R.string.general_action_yes;
    static final int NO = R.string.general_action_no;
    static final int[] YES_NO_ARGS = {YES, NO};

    @Override
    public boolean onCreateSliceProvider() {
        return true;
    }

    @Override
    public PendingIntent onCreatePermissionRequest(Uri sliceUri, String callingPackage) {
        final Intent settingsIntent = new Intent(Settings.ACTION_SETTINGS);
        final PendingIntent noOpIntent =
                PendingIntent.getActivity(getContext(), 0, settingsIntent, 0);
        return noOpIntent;
    }

    @Override
    public void onSlicePinned(Uri sliceUri) {
        mHandler.post(() -> {
            if (DEBUG) {
                Log.d(TAG, "Slice pinned: " + sliceUri);
            }
            Context context = getContext();
            if (!mBtDeviceServiceBound && context.bindServiceAsUser(
                    new Intent(context, AccessoryUtils.getBluetoothDeviceServiceClass()),
                    mBtDeviceServiceConnection,
                    Context.BIND_AUTO_CREATE, UserHandle.SYSTEM)) {
                mBtDeviceServiceBound = true;
            }
            if (!mPinnedUris.containsKey(sliceUri)) {
                mPinnedUris.put(sliceUri, 0);
            }
            mPinnedUris.put(sliceUri, mPinnedUris.get(sliceUri) + 1);
        });
    }

    @Override
    public Slice onBindSlice(Uri sliceUri) {
        if (DEBUG) {
            Log.d(TAG, "onBindSlice: " + sliceUri);
        }

        if (ConnectedDevicesSliceUtils.isGeneralPath(sliceUri)) {
            return createGeneralSlice(sliceUri);
        } else if (ConnectedDevicesSliceUtils.isBluetoothDevicePath(sliceUri)) {
            deviceGattFlag =false;
            return createBluetoothDeviceSlice(sliceUri);
        }
        return null;
    }

    @Override
    public void onSliceUnpinned(Uri sliceUri) {
        mHandler.post(() -> {
            if (DEBUG) {
                Log.d(TAG, "Slice unpinned: " + sliceUri);
            }
            if (mBluetoothGatt != null) {
                mBluetoothGatt.disconnect();
                mBluetoothGatt.close();
            }
            Context context = getContext();
            // If at this point there is only one slice pinned, we need to unbind the service as
            // there won't be any slice pinned after handleSliceUnpinned is called.
            if (mPinnedUris.containsKey(sliceUri)) {
                int newCount = mPinnedUris.get(sliceUri) - 1;
                mPinnedUris.put(sliceUri, newCount);
                if (newCount == 0) {
                    mPinnedUris.remove(sliceUri);
                }
            }
            if (mPinnedUris.isEmpty() && mBtDeviceServiceBound) {
                context.unbindService(mBtDeviceServiceConnection);
                mBtDeviceServiceBound = false;
            }
        });
    }

    // BluetoothDeviceProvider.Listener implementation
    @Override
    public void onDeviceUpdated(BluetoothDevice device) {
        getContext().getContentResolver()
                .notifyChange(ConnectedDevicesSliceUtils.GENERAL_SLICE_URI, null);
        notifyDeviceSlice(device);
    }

    // The initial slice in the Connected Device flow.
    private Slice createGeneralSlice(Uri sliceUri) {
        PreferenceSliceBuilder psb = new PreferenceSliceBuilder(getContext(), sliceUri);
        psb.addScreenTitle(
                new RowBuilder()
                        .setTitle(getString(R.string.connected_devices_slice_pref_title))
                        .setPageId(TvSettingsEnums.CONNECTED_SLICE));
        updateBluetoothToggle(psb);
        updatePairingButton(psb);
        updateConnectedDevices(psb);
        updateOfficialRemoteSettings(psb);
        return psb.build();
    }

    // The slice page that shows detail information of a particular device.
    private Slice createBluetoothDeviceSlice(Uri sliceUri) {
        Context context = getContext();
        String deviceAddr = ConnectedDevicesSliceUtils.getDeviceAddr(sliceUri);
        BluetoothDevice device = BluetoothDevicesService.findDevice(deviceAddr);
        CachedBluetoothDevice cachedDevice =
                AccessoryUtils.getCachedBluetoothDevice(getContext(), device);
        String deviceName = "";
        if (device != null) {
            if (device.getType() == BluetoothDevice.DEVICE_TYPE_LE || device.getType() == BluetoothDevice.DEVICE_TYPE_DUAL) {
                Log.d(TAG,"device.connectGatt");
                deviceGattFlag = true;
                mBluetoothGatt = device.connectGatt(getContext(), true, new GattBatteryCallbacks());
                //mBluetoothGatt.discoverServices();
            }
            deviceName = AccessoryUtils.getLocalName(device);
        }

        PreferenceSliceBuilder psb = new PreferenceSliceBuilder(getContext(), sliceUri);
        psb.addScreenTitle(
                new RowBuilder()
                        .setTitle(deviceName)
                        .setPageId(TvSettingsEnums.CONNECTED_SLICE_DEVICE_ENTRY));

        Bundle extras;
        Intent i;
        // Update "connect/disconnect preference"
        if (cachedDevice != null && !cachedDevice.isBusy()) {
            // Whether the device is actually connected from CachedBluetoothDevice's perceptive.
            boolean isConnected =false;
            //if(device.getType() == BluetoothDevice.DEVICE_TYPE_LE || device.getType() == BluetoothDevice.DEVICE_TYPE_DUAL){
            //    isConnected = AccessoryUtils.isConnected(device) && cachedDevice.isConnected();
            //}else{
                   isConnected = AccessoryUtils.isConnected(device);
            //}
            RowBuilder connectionActionPref = new RowBuilder()
                    .setKey(isConnected ? KEY_DISCONNECT : KEY_CONNECT)
                    .setTitle(getString(isConnected
                            ? R.string.bluetooth_disconnect_action_title
                            : R.string.bluetooth_connect_action_title));
            extras = new Bundle();
            i = new Intent(context, BluetoothActionActivity.class);
            BluetoothActionFragment.prepareArgs(
                    extras,
                    isConnected ? KEY_DISCONNECT : KEY_CONNECT,
                    R.drawable.ic_baseline_bluetooth_searching_large,
                    isConnected
                            ? R.string.bluetooth_disconnect_confirm
                            : R.string.bluetooth_connect_confirm,
                    0,
                    YES_NO_ARGS,
                    deviceName,
                    isConnected ? 1 /* default to NO (index 1) */ : 0 /* default to YES */
            );
            i.putExtras(extras);
            i.putExtra(KEY_EXTRAS_DEVICE, device);
            PendingIntent pendingIntent = PendingIntent
                    .getActivity(context, 3, i, PendingIntent.FLAG_UPDATE_CURRENT);
            Intent followUpIntent =
                    new Intent(context, ConnectedDevicesSliceBroadcastReceiver.class);
            followUpIntent.putExtra(EXTRAS_SLICE_URI, sliceUri.toString());
            PendingIntent followupIntent = PendingIntent
                    .getBroadcast(context, 4, followUpIntent, PendingIntent.FLAG_UPDATE_CURRENT);
            connectionActionPref.setPendingIntent(pendingIntent);
            connectionActionPref.setFollowupPendingIntent(followupIntent);
            psb.addPreference(connectionActionPref);
        }

        // Update "rename preference".
        RowBuilder renamePref = new RowBuilder()
                .setKey(KEY_RENAME)
                .setTitle(getString(R.string.bluetooth_rename_action_title))
                .setActionId(TvSettingsEnums.CONNECTED_SLICE_DEVICE_ENTRY_RENAME);
        extras = new Bundle();
        BluetoothActionFragment.prepareArgs(
                extras,
                KEY_RENAME,
                R.drawable.ic_baseline_bluetooth_searching_large,
                R.string.bluetooth_rename,
                0,
                null,
                deviceName,
                BluetoothActionFragment.DEFAULT_CHOICE_UNDEFINED
        );
        i = new Intent(context, BluetoothActionActivity.class);
        i.putExtra(KEY_EXTRAS_DEVICE, device);
        i.putExtras(extras);
        PendingIntent renamePendingIntent = PendingIntent
                .getActivity(context, 5, i, PendingIntent.FLAG_UPDATE_CURRENT);

        Intent followUpIntent = new Intent(context, ConnectedDevicesSliceBroadcastReceiver.class);
        followUpIntent.putExtra(EXTRAS_SLICE_URI, sliceUri.toString());
        PendingIntent renameFollowupIntent = PendingIntent
                .getBroadcast(context, 6, followUpIntent, PendingIntent.FLAG_UPDATE_CURRENT);
        renamePref.setFollowupPendingIntent(renameFollowupIntent);
        renamePref.setPendingIntent(renamePendingIntent);
        psb.addPreference(renamePref);

        // Update "forget preference".
        RowBuilder forgetPref = new RowBuilder()
                .setKey(KEY_FORGET)
                .setTitle(getString(R.string.bluetooth_forget_action_title))
                .setActionId(TvSettingsEnums.CONNECTED_SLICE_DEVICE_ENTRY_FORGET);
        extras = new Bundle();
        i = new Intent(context, BluetoothActionActivity.class);
        BluetoothActionFragment.prepareArgs(
                extras,
                KEY_FORGET,
                R.drawable.ic_baseline_bluetooth_searching_large,
                R.string.bluetooth_forget_confirm,
                0,
                YES_NO_ARGS,
                deviceName,
                1 /* default to NO (index 1) */
        );
        i.putExtras(extras);
        i.putExtra(KEY_EXTRAS_DEVICE, device);
        PendingIntent disconnectPendingIntent = PendingIntent
                .getActivity(context, 7, i, PendingIntent.FLAG_UPDATE_CURRENT);
        followUpIntent = new Intent(context, ConnectedDevicesSliceBroadcastReceiver.class);
        followUpIntent.putExtra(EXTRAS_SLICE_URI, sliceUri.toString());
        PendingIntent forgetFollowupIntent = PendingIntent
                .getBroadcast(context, 8, followUpIntent, PendingIntent.FLAG_UPDATE_CURRENT);
        forgetPref.setPendingIntent(disconnectPendingIntent);
        forgetPref.setFollowupPendingIntent(forgetFollowupIntent);
        psb.addPreference(forgetPref);

        // Update "bluetooth device info preference".
        RowBuilder infoPref = new RowBuilder()
                .setIcon(IconCompat.createWithResource(context, R.drawable.ic_baseline_info_24dp));

        BluetoothDeviceProvider provider = mLocalBluetoothDeviceProvider;
        //sleep 100ms
        try {
             Thread.sleep(100);
        } catch (InterruptedException e) {
             e.printStackTrace();
        }
        if ( deviceGattFlag && "-1" != deviceBatteryLevel) infoPref.addInfoItem(getString(R.string.bluetooth_battery_label), deviceBatteryLevel);
        if ( deviceGattFlag && "" != deviceFirmwareVersion) infoPref.addInfoItem(getString(R.string.bluetooth_fireware_label), deviceFirmwareVersion);
        infoPref.addInfoItem(getString(R.string.bluetooth_serial_number_label), deviceAddr);
        psb.addPreference(infoPref);
        deviceGattFlag = false;
        return psb.build();
    }

    private void updateBluetoothToggle(PreferenceSliceBuilder psb) {
        if (showBluetoothToggle()) {
            Intent bluetoothToggleIntent;
            if (AccessoryUtils.isBluetoothEnabled()) {
                bluetoothToggleIntent = new Intent(getContext(), BluetoothActionActivity.class);
                Bundle extras = new Bundle();
                BluetoothActionFragment.prepareArgs(
                        extras,
                        KEY_BLUETOOTH_TOGGLE,
                        R.drawable.ic_baseline_bluetooth_searching_large,
                        R.string.bluetooth_toggle_confirmation_dialog_title,
                        R.string.bluetooth_toggle_confirmation_dialog_summary,
                        YES_NO_ARGS,
                        null,
                        0 /* default to YES */
                );
                bluetoothToggleIntent.putExtras(extras);
            } else {
                bluetoothToggleIntent = new Intent(ACTION_TOGGLE_CHANGED);
                bluetoothToggleIntent.setClass(
                        getContext(), ConnectedDevicesSliceBroadcastReceiver.class);
                bluetoothToggleIntent.putExtra(EXTRA_TOGGLE_TYPE, BLUETOOTH_ON);
            }
            psb.addPreference(
                    new RowBuilder()
                            .setKey(KEY_BLUETOOTH_TOGGLE)
                            .setIcon(IconCompat.createWithResource(
                                    getContext(), R.drawable.ic_bluetooth_raw))
                            .setIconNeedsToBeProcessed(true)
                            .setTitle(getString(R.string.bluetooth_toggle_title))
                            .addSwitch(
                                    AccessoryUtils.isBluetoothEnabled()
                                            ? PendingIntent.getActivity(
                                                    getContext(), 1, bluetoothToggleIntent, 0)
                                            : PendingIntent.getBroadcast(
                                                    getContext(), 2, bluetoothToggleIntent, 0),
                                    AccessoryUtils.isBluetoothEnabled())
            );
        }
    }

    private void updatePairingButton(PreferenceSliceBuilder psb) {
        if (AccessoryUtils.isBluetoothEnabled()) {
            Intent i = new Intent(ACTION_CONNECT_INPUT).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            PendingIntent pendingIntent = PendingIntent.getActivity(getContext(), 3, i, 0);
            psb.addPreference(new RowBuilder()
                    .setKey(KEY_PAIR_REMOTE)
                    .setTitle(getString(R.string.bluetooth_pair_accessory))
                    .setActionId(TvSettingsEnums.CONNECTED_SLICE_CONNECT_NEW_DEVICES)
                    .setIcon(IconCompat.createWithResource(getContext(),
                            R.drawable.ic_baseline_add_24dp))
                    .setIconNeedsToBeProcessed(true)
                    .setPendingIntent(pendingIntent)
            );
        }
    }

    private void updateConnectedDevices(PreferenceSliceBuilder psb) {
        // Overall BT devices maps
        HashMap<String, BluetoothDevice> addressToDevice = new HashMap<>();
        // Sets for BT devices that are not official remotes:
        // - activeAccessories: they are considered connected from both BluetoothDevice and
        //       CachedBluetoothDevice's perceptive.
        // - inactiveAccessories: they are considered connected from BluetoothDevice's perceptive
        //       but disconnected from CachedBluetoothDevice's perceptive. They can be easily
        //       reconnected.
        // - bondedAccessories: they are considered merely bonded but not connected from
        //       BluetoothDevice's perceptive.
        Set<String> activeAccessories = new HashSet<>();
        Set<String> inactiveAccessories = new HashSet<>();
        Set<String> bondedAccessories = new HashSet<>();

        // Bucketing all BT devices
        for (BluetoothDevice device : getBluetoothDevices()) {
            CachedBluetoothDevice cachedDevice =
                    AccessoryUtils.getCachedBluetoothDevice(getContext(), device);
            if (!AccessoryUtils.isKnownDevice(getContext(), device)) {
                if (AccessoryUtils.isConnected(device)) {
                    addressToDevice.put(device.getAddress(), device);
                    if (cachedDevice != null && cachedDevice.isConnected()) {
                        activeAccessories.add(device.getAddress());
                    } else {
                        inactiveAccessories.add(device.getAddress());
                    }
                } else if (AccessoryUtils.isBonded(device)) {
                    addressToDevice.put(device.getAddress(), device);
                    bondedAccessories.add(device.getAddress());
                }
            }
        }

        // "Accessories" category
        if (activeAccessories.size() + inactiveAccessories.size() + bondedAccessories.size()
                > 0) {
            psb.addPreferenceCategory(new RowBuilder()
                    .setTitle(getContext().getString(R.string.bluetooth_known_devices_category))
                    .setKey(KEY_ACCESSORIES));
            // Add accessories following the ranking of: active, inactive, bonded.
            createAndAddBtDeviceSlicePreferenceFromSet(psb, activeAccessories, addressToDevice);
            createAndAddBtDeviceSlicePreferenceFromSet(psb, inactiveAccessories, addressToDevice);
            createAndAddBtDeviceSlicePreferenceFromSet(psb, bondedAccessories, addressToDevice);
        }
    }

    private void updateOfficialRemoteSettings(PreferenceSliceBuilder psb) {
        String officialRemoteSettingsUri =
                getString(R.string.bluetooth_official_remote_entry_slice_uri);
        if (TextUtils.isEmpty(officialRemoteSettingsUri)) {
            return;
        }
        if (ConnectedDevicesSliceUtils.isSliceProviderValid(
                getContext(), officialRemoteSettingsUri)) {
            psb.addPreferenceCategory(new RowBuilder()
                    .setTitle(getContext().getString(R.string.bluetooth_official_remote_category)));
            psb.addPreference(new RowBuilder()
                    .setKey(KEY_OFFICIAL_REMOTE)
                    .setTitle(getContext().getString(
                            R.string.bluetooth_official_remote_entry_title))
                    .setTargetSliceUri(officialRemoteSettingsUri));
        }
    }

    private void createAndAddBtDeviceSlicePreferenceFromSet(
            PreferenceSliceBuilder psb,
            Set<String> addresses,
            HashMap<String, BluetoothDevice> addressesToBtDeviceMap) {
        if (psb == null || addresses == null || addresses.isEmpty()
                || addressesToBtDeviceMap == null || addressesToBtDeviceMap.isEmpty()) {
            return;
        }
        final List<String> devicesAddressesList = new ArrayList<>(addresses);
        Collections.sort(devicesAddressesList);
        for (String deviceAddr : devicesAddressesList) {
            psb.addPreference(
                    createBtDeviceSlicePreference(
                            getContext(),
                            addressesToBtDeviceMap.get(deviceAddr)));
        }
    }

    private PreferenceSliceBuilder.RowBuilder createBtDeviceSlicePreference(
            Context context, BluetoothDevice device) {
        PreferenceSliceBuilder.RowBuilder pref = new PreferenceSliceBuilder.RowBuilder();
        boolean isConnected =false;
        //if(device.getType() == BluetoothDevice.DEVICE_TYPE_LE || device.getType() == BluetoothDevice.DEVICE_TYPE_DUAL){
        //    isConnected = AccessoryUtils.isConnected(device)
        //        && AccessoryUtils.getCachedBluetoothDevice(getContext(), device) != null
         //        && AccessoryUtils.getCachedBluetoothDevice(getContext(), device).isConnected();
        //}else{
            isConnected = AccessoryUtils.isConnected(device)
                && AccessoryUtils.getCachedBluetoothDevice(getContext(), device) != null;
        //}
        Log.d(TAG,"AccessoryUtils.isConnected: "+(AccessoryUtils.isConnected(device))+" AccessoryUtils.getCachedBluetoothDevice:"+(AccessoryUtils.getCachedBluetoothDevice(getContext(), device) != null));
        pref.setKey(device.getAddress());
        pref.setTitle(AccessoryUtils.getLocalName(device));
        pref.setSubtitle(
                isConnected
                        ? getString(R.string.bluetooth_connected_status)
                        : getString(R.string.bluetooth_disconnected_status));
        pref.setIcon(IconCompat.createWithResource(
                context, AccessoryUtils.getImageIdForDevice(device, true)));
        pref.setIconNeedsToBeProcessed(true);
        Uri targetSliceUri = ConnectedDevicesSliceUtils
                .getDeviceUri(device.getAddress(), device.getAlias());
        pref.setTargetSliceUri(targetSliceUri.toString());
        return pref;
    }

    private List<BluetoothDevice> getBluetoothDevices() {
        if (mBtDeviceServiceBinder != null) {
            return mBtDeviceServiceBinder.getDevices();
        }
        return new ArrayList<>();
    }

    private BluetoothDeviceProvider getBluetoothDeviceProvider() {
        return mBtDeviceServiceBinder;
    }

    private void notifyDeviceSlice(BluetoothDevice device) {
        String addr = device.getAddress();
        mHandler.post(() -> {
            if (device != null) {
                getContext().getContentResolver().notifyChange(
                        ConnectedDevicesSliceUtils.getDeviceUri(addr, device.getAlias()), null);
            }
        });
    }

    private boolean showBluetoothToggle() {
        return getContext().getResources().getBoolean(R.bool.show_bluetooth_toggle);
    }

    private String getString(@IntegerRes int resId) {
        return getContext().getString(resId);
    }

    private class GattBatteryCallbacks extends BluetoothGattCallback {

        @Override
        public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
            if (DEBUG) {
                Log.d(TAG, "Connection status:" + status + " state:" + newState);
            }
            if (status == BluetoothGatt.GATT_SUCCESS && newState == BluetoothGatt.STATE_CONNECTED) {
                gatt.discoverServices();
            }
        }

        @Override
        public void onServicesDiscovered(BluetoothGatt gatt, int status) {
            if (status != BluetoothGatt.GATT_SUCCESS) {
                if (DEBUG) {
                    Log.e(TAG, "Service discovery failure on " + gatt);
                }
                return;
            }

            final BluetoothGattService battService = gatt.getService(GATT_BATTERY_SERVICE_UUID);
            if (battService == null) {
                if (DEBUG) {
                    Log.d(TAG, "No battery service");
                }
                return;
            }

            final BluetoothGattCharacteristic battLevel =
                    battService.getCharacteristic(GATT_BATTERY_LEVEL_CHARACTERISTIC_UUID);
            if (battLevel == null) {
                if (DEBUG) {
                    Log.d(TAG, "No battery level");
                }
                return;
            }
            gatt.readCharacteristic(battLevel);

            final BluetoothGattService infoService = gatt.getService(GATT_DEVICE_INFORMATION_UUID);
            if (infoService == null) {
                if (DEBUG) {
                    Log.d(TAG, "No device info service");
                }
                return;
            }

            bleVersion = infoService.getCharacteristic(GATT_VERSION_CHARACTERISTIC_UUID);
            if (bleVersion == null) {
                if (DEBUG) {
                    Log.d(TAG, "No ble version");
                }
                return;
            }
            //gatt.readCharacteristic(bleVersion);
        }

        @Override
        public void onCharacteristicRead(BluetoothGatt gatt,
                BluetoothGattCharacteristic characteristic, int status) {
            if (status != BluetoothGatt.GATT_SUCCESS) {
                if (DEBUG) {
                    Log.e(TAG, "Read characteristic failure on " + gatt + " " + characteristic);
                }
                return;
            }

            if (GATT_BATTERY_LEVEL_CHARACTERISTIC_UUID.equals(characteristic.getUuid())) {
                final int batteryLevel =
                        characteristic.getIntValue(BluetoothGattCharacteristic.FORMAT_UINT8, 0);
                mHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        Log.d(TAG,"onCharacteristicRead mBatteryPref:"+batteryLevel);
                        deviceGattFlag = true;
                        //if(deviceBatteryLevel!="Good" || deviceBatteryLevel!="Low"){
                            if (batteryLevel >= 50) {
                                deviceBatteryLevel = "Good";
                            }else{
                                deviceBatteryLevel = "Low";
                            }
                            //getContext().getContentResolver().notifyChange(ConnectedDevicesSliceUtils.BLUETOOTH_DEVICE_SLICE_URI, null);
                        //}
                        /*if (mBatteryPref != null && !mUnpairing) {
                            mBatteryPref.setTitle(getString(R.string.accessory_battery,
                                    batteryLevel));
                            mBatteryPref.setVisible(true);
                        }*/
                    }
                });
            }

            if (GATT_VERSION_CHARACTERISTIC_UUID.equals(characteristic.getUuid())) {
                final byte[] versionData = characteristic.getValue();
                final String version = transferVersionDataToString(versionData);
                if ( DEBUG ) {
                    Log.d(TAG, "version: "+version);
                }
                mHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        Log.d(TAG,"onCharacteristicRead mVersionPref:"+version);
                        deviceGattFlag = true;
                        //if(deviceFirmwareVersion != version){
                            deviceFirmwareVersion = version;
                            //getContext().getContentResolver().notifyChange(ConnectedDevicesSliceUtils.BLUETOOTH_DEVICE_SLICE_URI, null);
                        //}
                        /*if (mVersionPref != null && !mUnpairing) {
                            mVersionPref.setTitle(getString(R.string.accessory_version,
                                    version));
                            mVersionPref.setVisible(true);
                        }*/
                    }
                });
            }
            onNextRequestIfneeded(gatt);
        }
    }

    private String transferVersionDataToString(byte[] data) {
        try {
            return new String(data, "ISO-8859-1");
        } catch (UnsupportedEncodingException e) {
            e.printStackTrace();
        }
        return null;
    }

    private void onNextRequestIfneeded(BluetoothGatt gatt){
        if (!versionRequest && gatt != null && bleVersion != null) {
            versionRequest = gatt.readCharacteristic(bleVersion);
        }
    }

}
