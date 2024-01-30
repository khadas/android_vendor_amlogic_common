// IMirrorDisplayInterface.aidl
package com.droidlogic.displaycontrollerservice;

// Declare any non-default types here with import statements

interface IMirrorDisplayInterface {

    boolean startMirror (int displayId, int tDisplayId);
    boolean isMirrored (int displayId);
    boolean isMirroring (int displayId);
    void stopMirror(int displayId);
    boolean swithDisplay(int displayId,int toDisplayId);

}