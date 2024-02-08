/*
 * Copyright (C) 2009 The Android Open Source Project
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

package com.droidlogic.displaycontrollerservice;

import android.content.Context;
import android.hardware.display.DisplayManager;
import android.util.Log;
import android.view.Display;
import android.view.DisplayAddress;
import android.provider.Settings;

import java.util.HashMap;
import java.util.List;
import java.util.ArrayList;

public class MultiDisplayController implements DisplayManager.DisplayListener {
    private static final String TAG = "MM-Service";
    private static final String MMC_CONTROL = "mmc_control";
    private static final HashMap<Integer, Long> mSurfaceRecord = new HashMap<Integer, Long>();

    static {
        System.loadLibrary("displaycontroller");
    }

    private final ArrayList<DisplayMapping> mDisplayMappings = new ArrayList();
    private final Context mContext;

    private final DisplayManager mDisplayManager;

    public MultiDisplayController(Context context) {
        mContext = context;

        mDisplayManager = (DisplayManager) mContext.getSystemService(Context.DISPLAY_SERVICE);

        mDisplayManager.registerDisplayListener(MultiDisplayController.this, null);
        initialDisplayMapping();
        saveMirroringDataBase(mSurfaceRecord.size());
    }

    private static native long nativeMirror(int displayId, int toDisplayId, boolean control);

    private static native boolean nativeSwitch(int displayId, int toDisplayId);

    private static native void nativeRelease(long surfaceNativeObject);

    @Override
    public void onDisplayAdded(int displayId) {
        removeMapping(displayId);
        addMapping(displayId);
    }

    @Override
    public void onDisplayRemoved(int displayId) {
        if (mSurfaceRecord.get(displayId) != null) {
            Log.d(TAG, "record release for displayId ");
            nativeRelease(mSurfaceRecord.get(displayId));
        }
        removeMapping(displayId);
        saveMirroringDataBase(mSurfaceRecord.size());
    }

    @Override
    public void onDisplayChanged(int displayId) {

    }

    public void stopMirrorIfMirrored() {
        for (DisplayMapping map : mDisplayMappings) {
            stopMapping(map.mDisplay.getDisplayId());
        }
    }

    private void initialDisplayMapping() {
        for (Display d : mDisplayManager.getDisplays()) {
            DisplayMapping map = new DisplayMapping(d);
            mDisplayMappings.add(map);
        }
    }

    private synchronized void removeMapping(int displayId) {
        DisplayMapping mapping = getMapping(displayId);
        if (mapping != null) {
            mDisplayMappings.remove(mapping);
        }
    }

    private synchronized void addMapping(int displayId) {
        DisplayMapping map = new DisplayMapping(mDisplayManager.getDisplay(displayId));
        mDisplayMappings.add(map);
    }

    public boolean switchDisplay(int fromDisplayId, int toDisplayId) {
        int tPort = findDisplay2phyPort(toDisplayId);
        int fPort = findDisplay2phyPort(fromDisplayId);
        if (tPort == -1 || fPort == -1) return false;
        nativeSwitch(fPort, tPort);
        return true;
    }

    public boolean mirroringDisplay(int fromDisplayId, int toDisplayId, boolean control) {
        DisplayMapping mirrorDisplay = getMapping(toDisplayId);
        if (mirrorDisplay == null) return false;
        if (getMapping(toDisplayId).isMapping()) {
            Log.d(TAG, "---- strage case " + Log.getStackTraceString(new Throwable()));
            return true;
        }
        int tPort = findDisplay2phyPort(toDisplayId);
        int fPort = findDisplay2phyPort(fromDisplayId);
        if (tPort == -1 || fPort == -1) return false;
        long surfaceNativeObject = nativeMirror(fPort, tPort, control);
        Log.d(TAG, "MirroringDisplay " + fromDisplayId + " - " + toDisplayId);
        if (surfaceNativeObject != 0) {
            if (mSurfaceRecord.get(toDisplayId) != null) {
                Log.d(TAG, "record release");
                nativeRelease(mSurfaceRecord.get(toDisplayId));
            }
            mSurfaceRecord.put(toDisplayId, surfaceNativeObject);
            Log.d(TAG, "MirroringDisplay Info" + surfaceNativeObject);
            mirrorDisplay.saveData(surfaceNativeObject,fromDisplayId,control);

        }
        saveMirroringDataBase(mSurfaceRecord.size());
        Log.d(TAG, "updated info" + getMapping(fromDisplayId) + "::::" + getMapping(toDisplayId));
        return true;
    }

    private void saveMirroringDataBase(int mirroring) {
        Settings.Global.putInt(mContext.getContentResolver(), MMC_CONTROL, mirroring);
    }

    public void stopMapping(int displayId) {
        if (!isMapping(displayId)) {
            Log.d(TAG, "stopMapping but not mirrored");
            return;
        }
        releaseMirrorDisplay(displayId);
        saveMirroringDataBase(mSurfaceRecord.size());
    }

    public boolean isControlled(int displayId) {
        DisplayMapping mapping = getMapping(displayId);
        if (mapping == null) return false;
        return mapping.isControlled();
    }

    public int getPhyPort(int displayId) {
        return findDisplay2phyPort(displayId);
    }

    public int getMirroredId(int displayId) {
        DisplayMapping mapping = getMapping(displayId);
        if (mapping == null) return -1;
        return mapping.getMirroredDisplay();
    }

    public int findDisplay2phyPort(int displayId) {
        Display display = mDisplayManager.getDisplay(displayId);
        DisplayAddress address = display.getAddress();
        if (!(address instanceof DisplayAddress.Physical)) {
            return -1;
        }
        DisplayAddress.Physical physical = (DisplayAddress.Physical) address;
        return physical.getPort();
    }

    public boolean isMapping(int displayId) {
        DisplayMapping mapping = getMapping(displayId);
        if (mapping == null) return false;
        Log.d(TAG, "mapping current DisplayId" + displayId + "Mirrored displayId" + mapping.mMirroredDisplayId + " mapping.mMirroringSurface" + mapping.mMirroringSurface);
        boolean ret = (mapping.mMirroredDisplayId != -1 && mapping.mMirroredDisplayId != mapping.mDisplay.getDisplayId()) || (mapping.mMirroringSurface != 0);
        Log.d(TAG, "isMapping " + displayId + " value:" + ret);
        return ret;
    }

    public boolean beMirroring(int displayId) {
        Log.d(TAG, "beMirroring " + displayId);
        for (DisplayMapping map : mDisplayMappings) {
            Log.d(TAG, "map " + map.mMirroredDisplayId);
            if (map.getMirroredDisplay() == displayId) {
                return true;
            }
        }
        return false;
    }

    /*
     * release display Listener
     * */
    public void releaseListener() {
        mDisplayManager.unregisterDisplayListener(this);
    }

    private DisplayMapping getMapping(int displayId) {
        for (DisplayMapping map : mDisplayMappings) {
            if (map.mDisplay.getDisplayId() == displayId) {
                return map;
            }
        }
        return null;
    }

    /*
     * release Mirror Display which has Mirror.
     * */
    private void releaseMirrorDisplay(int displayId) {
        Log.d(TAG, "releaseMirrorDisplay " + displayId);
        DisplayMapping map = getMapping(displayId);
        if (map == null) return;
        if (mSurfaceRecord.get(displayId) != null && map.mMirroringSurface != mSurfaceRecord.get(displayId)) {
            nativeRelease(mSurfaceRecord.get(displayId));
        }
        map.releaseMirror();
        mSurfaceRecord.remove(displayId);
    }

    class DisplayMapping {
        private final Display mDisplay;
        private long mMirroringSurface;
        private int mMirroredDisplayId;
        private boolean mIsControlled;

        private DisplayMapping(Display display) {
            mDisplay = display;
            mMirroredDisplayId = -1;//display.getDisplayId();
            mMirroringSurface = 0;
        }

        public int getMirroredDisplay() {
            if (isMapping()) {
                return mMirroredDisplayId;
            }
            return -1;
        }

        public void saveData(long surfaceNativeObject,int fromDisplayId, boolean control) {
            mMirroringSurface = surfaceNativeObject;
            mMirroredDisplayId = fromDisplayId;
            mIsControlled = control;
        }

        public boolean isControlled() {
            return mIsControlled;
        }

        public void releaseMirror() {
            Log.d(TAG, "releaseMirror " + mMirroringSurface);
            if (mMirroringSurface != 0) {
                nativeRelease(mMirroringSurface);
                mMirroringSurface = 0;
            }
            if (mMirroredDisplayId != -1) {
                mMirroredDisplayId = -1;
            }
            mIsControlled = false;
        }

        @Override
        public String toString() {
            return "MirroringData{" + "mDisplay=" + mDisplay.getDisplayId() + ", mMirroringSurface=" + mMirroringSurface + ", mMirroredDisplayId=" + mMirroredDisplayId + '}';
        }

        public boolean isMapping() {
            //be mirrored
            if (mMirroredDisplayId != -1) {
                return true;
            }
            //mirrored
            return mMirroringSurface != 0;
        }
    }
}
