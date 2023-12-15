package com.droidlogic.multidisplay.multicontroller;


public class MirroredData {
    private int displayId;
    private boolean mirrored;
    private String mDeviceName;

    public MirroredData(int displayId, boolean mirrored) {
        this.displayId = displayId;
        this.mirrored = mirrored;
    }

    public int getDisplayId() {
        return displayId;
    }

    public void setDisplayId(int displayId) {
        this.displayId = displayId;
    }
    public String getName() {
        return "DEVICE"+displayId;
    }
    public boolean isMirrored() {
        return mirrored;
    }

    public void setMirrored(boolean mirrored) {
        this.mirrored = mirrored;
    }
}
