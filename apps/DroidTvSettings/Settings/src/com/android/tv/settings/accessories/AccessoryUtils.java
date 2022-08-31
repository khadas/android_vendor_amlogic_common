/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.tv.settings.accessories;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothClass;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothProfile;
import android.content.Context;
import android.util.Log;

import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;

import com.android.settingslib.bluetooth.CachedBluetoothDevice;
import com.android.settingslib.bluetooth.LocalBluetoothManager;
import com.android.tv.settings.R;

import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.FutureTask;

/** Provide utilities for Remote & Accessories. */
final class AccessoryUtils {

    public static final String TAG = "AccessoryUtils";

    private static final int MINOR_MASK = 0b11111100;
    // Includes any generic keyboards or pointers, and any joystick, game pad, or remote subtypes.
    private static final int MINOR_REMOTE_MASK = 0b11001100;
    private static List<String> sKnownDeviceLabels = null;

    /** This allows OEM to easily override the main Service if desired. */
    public static Class getBluetoothDeviceServiceClass() {
        return BluetoothDevicesService.class;
    }

    public static LocalBluetoothManager getLocalBluetoothManager(Context context) {
        final FutureTask<LocalBluetoothManager> localBluetoothManagerFutureTask =
                new FutureTask<>(
                        // Avoid StrictMode ThreadPolicy violation
                        () -> LocalBluetoothManager.getInstance(
                                context, (c, bluetoothManager) -> {
                                })
                );
        try {
            localBluetoothManagerFutureTask.run();
            return localBluetoothManagerFutureTask.get();
        } catch (InterruptedException | ExecutionException e) {
            Log.w(TAG, "Error getting LocalBluetoothManager.", e);
            return null;
        }
    }

    public static CachedBluetoothDevice getCachedBluetoothDevice(
            Context context, BluetoothDevice device) {
        LocalBluetoothManager localBluetoothManager = getLocalBluetoothManager(context);
        if (localBluetoothManager != null) {
            return localBluetoothManager.getCachedDeviceManager().findDevice(device);
        }
        return null;
    }

    public static BluetoothAdapter getDefaultBluetoothAdapter() {
        final FutureTask<BluetoothAdapter> defaultBluetoothAdapterFutureTask =
                new FutureTask<>(
                        // Avoid StrictMode ThreadPolicy violation
                        BluetoothAdapter::getDefaultAdapter);
        try {
            defaultBluetoothAdapterFutureTask.run();
            return defaultBluetoothAdapterFutureTask.get();
        } catch (InterruptedException | ExecutionException e) {
            Log.w(TAG, "Error getting default BluetoothAdapter.", e);
            return null;
        }
    }

    public static String getLocalName(BluetoothDevice device) {
        if (device == null) {
            return null;
        }
        return device.getAlias();
    }

    public static boolean isBluetoothEnabled() {
        return getDefaultBluetoothAdapter() != null && getDefaultBluetoothAdapter().isEnabled();
    }

    public static boolean isConnected(BluetoothDevice device) {
        if (device == null) {
            return false;
        }
        return device.getBondState() == BluetoothDevice.BOND_BONDED && device.isConnected();
    }

    public static boolean isBonded(BluetoothDevice device) {
        if (device == null) {
            return false;
        }
        return device.getBondState() == BluetoothDevice.BOND_BONDED && !device.isConnected();
    }

    public static boolean isRemoteClass(BluetoothDevice device) {
        if (device == null || device.getBluetoothClass() == null) {
            return false;
        }
        int major = device.getBluetoothClass().getMajorDeviceClass();
        int minor = device.getBluetoothClass().getDeviceClass() & MINOR_MASK;
        return BluetoothClass.Device.Major.PERIPHERAL == major
                && (minor & ~MINOR_REMOTE_MASK) == 0;
    }

    // For partner, this will be used to identify official device to omit it in the generic
    // accessories section since the device's settings will be displayed in partner-implemented
    // Slice.
    public static boolean isKnownDevice(Context context, BluetoothDevice device) {
        if (device == null || device.getName() == null) {
            return false;
        }
        if (sKnownDeviceLabels == null) {
            if (context == null) {
                return false;
            } else {
                sKnownDeviceLabels =
                        Collections.unmodifiableList(
                                Arrays.asList(context.getResources().getStringArray(
                                        R.array.known_bluetooth_device_labels)));
            }
        }

        final String name = device.getName().toLowerCase();
        for (String knownLabel : sKnownDeviceLabels) {
            if (name.contains(knownLabel.toLowerCase())) {
                return true;
            }
        }
        return false;
    }

    public static boolean isA2dpSource(BluetoothDevice device) {
        return device != null && device.getBluetoothClass() != null
                && device.getBluetoothClass().doesClassMatch(BluetoothProfile.A2DP);
    }

    /**
     * @param dev             the bluetooth device
     * @param willBeProcessed whether the icon will be processed by Slice
     * @return the resource id for a bluetooth device's icon based on its device type
     */
    public static @DrawableRes int getImageIdForDevice(
            @NonNull BluetoothDevice dev, boolean willBeProcessed) {
        if (dev == null || dev.getBluetoothClass() == null) {
            return willBeProcessed ? R.drawable.ic_bluetooth_raw : R.drawable.ic_bluetooth;
        }

        final int devClass = dev.getBluetoothClass().getDeviceClass();
        // The order is critical
        if (devClass == BluetoothClass.Device.AUDIO_VIDEO_WEARABLE_HEADSET) {
            return willBeProcessed ? R.drawable.ic_headset_mic_raw : R.drawable.ic_headset_mic;
        } else if (devClass == BluetoothClass.Device.AUDIO_VIDEO_HEADPHONES
                || devClass == BluetoothClass.Device.AUDIO_VIDEO_LOUDSPEAKER
                || devClass == BluetoothClass.Device.AUDIO_VIDEO_PORTABLE_AUDIO
                || devClass == BluetoothClass.Device.AUDIO_VIDEO_HIFI_AUDIO) {
            return willBeProcessed ? R.drawable.ic_headset_raw : R.drawable.ic_headset;
        } else if ((devClass & InputDeviceCriteria.MINOR_DEVICE_CLASS_POINTING) != 0) {
            return willBeProcessed ? R.drawable.ic_mouse_raw : R.drawable.ic_mouse;
        } else if (isA2dpSource(dev) && willBeProcessed) {
            return R.drawable.ic_a2dp_raw;
        } else if ((devClass & InputDeviceCriteria.MINOR_DEVICE_CLASS_REMOTE) != 0) {
            return willBeProcessed ? R.drawable.ic_games_raw : R.drawable.ic_games;
        } else if ((devClass & InputDeviceCriteria.MINOR_DEVICE_CLASS_JOYSTICK) != 0) {
            return willBeProcessed ? R.drawable.ic_games_raw : R.drawable.ic_games;
        } else if ((devClass & InputDeviceCriteria.MINOR_DEVICE_CLASS_GAMEPAD) != 0) {
            return willBeProcessed ? R.drawable.ic_games_raw : R.drawable.ic_games;
        } else if ((devClass & InputDeviceCriteria.MINOR_DEVICE_CLASS_KEYBOARD) != 0) {
            return willBeProcessed ? R.drawable.ic_keyboard_raw : R.drawable.ic_keyboard;
        }

        // Default for now
        return willBeProcessed ? R.drawable.ic_bluetooth_raw : R.drawable.ic_bluetooth;
    }

    private AccessoryUtils() {
        // do not allow instantiation
    }
}
