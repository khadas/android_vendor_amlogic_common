// IMirrorDisplayInterface.aidl
package com.droidlogic.displaycontrollerservice;

// Declare any non-default types here with import statements

interface IMirrorDisplayInterface {
    /**
     * Demonstrates some basic types that you can use as parameters
     * and return values in AIDL.
     */
    boolean startMirror (int displayId, int tDisplayId, boolean control);
    boolean isControlled(int tDisplayId);
    boolean isMirrored (int displayId);
    boolean isMirroring (int displayId);
    void stopMirror(int displayId);
    boolean swithDisplay(int displayId,int toDisplayId);
    int getPhyPort(int displayId);
    int getMirroredId(int displayId);
}