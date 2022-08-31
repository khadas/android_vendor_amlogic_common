package com.droidlogic.updater.util;

/**
 * Utility class for properties that will be passed to {@code UpdateEngine#applyPayload}.
 */
public final class UpdateEngineProperties {

    /*service */
    public static final String SERVER_URI = "http://aats.amlogic.com/amlota";
    /**
     * The property indicating that the update engine should not switch slot
     * when the device reboots.
     */
    public static final String PROPERTY_DISABLE_SWITCH_SLOT_ON_REBOOT = "SWITCH_SLOT_ON_REBOOT=0";

    /**
     * The property to skip post-installation.
     * https://source.android.com/devices/tech/ota/ab/#post-installation
     */
    public static final String PROPERTY_SKIP_POST_INSTALL = "RUN_POST_INSTALL=0";

    private UpdateEngineProperties() {}
}
