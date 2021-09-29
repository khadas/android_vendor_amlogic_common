package com.droidlogic.googletv.settings.sliceprovider.manager;

import static android.provider.Settings.Global.HDMI_CONTROL_ENABLED;

import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.UserHandle;
import android.util.Log;
import android.provider.Settings;
import android.content.ContentResolver;
import com.droidlogic.googletv.settings.R;

import com.droidlogic.googletv.settings.sliceprovider.utils.MediaSliceUtil;
import com.droidlogic.googletv.settings.sliceprovider.MediaSliceConstants;


public class HdmiCecContentManager {
    private static final String TAG = HdmiCecContentManager.class.getSimpleName();
    private static volatile HdmiCecContentManager mHdmiCecContentManager;
    private Context mContext;

    public static boolean isInit() {
        return mHdmiCecContentManager != null;
    }

    public static HdmiCecContentManager getHdmiCecContentManager(final Context context) {
        if (mHdmiCecContentManager == null) {
            synchronized (HdmiCecContentManager.class) {
            if (mHdmiCecContentManager == null)
                mHdmiCecContentManager = new HdmiCecContentManager(context);
            }
        }
        return mHdmiCecContentManager;
    }

    public static void shutdown(final Context context) {
        if (mHdmiCecContentManager != null) {
            synchronized (HdmiCecContentManager.class) {
                if (mHdmiCecContentManager != null) {
                    mHdmiCecContentManager = null;
                }
            }
        }
    }

    private HdmiCecContentManager(final Context context) {
        mContext = context;
    }

    public boolean getHdmiCecStatus() {
        ContentResolver resolver = mContext.getContentResolver();
        return Settings.Global.getInt(resolver, Settings.Global.HDMI_CONTROL_ENABLED, 1) != 0;
    }

    public void setHdmiCecStatus(int state) {
        ContentResolver resolver = mContext.getContentResolver();
        Settings.Global.putInt(resolver, Settings.Global.HDMI_CONTROL_ENABLED, state);
        resolver.notifyChange(MediaSliceConstants.DISPLAYSOUND_HDMI_CEC_URI, null);
        if (MediaSliceUtil.CanDebug()) Log.d(TAG, "setHdmiCecStatus cec swtich:" + state);
    }


    public String getHdmiCecStatusName() {
        ContentResolver resolver = mContext.getContentResolver();
        // Note that default CEC is enabled. You'll find similar retrieval of property in
        // HdmiControlService.
        boolean cecEnabled =
                Settings.Global.getInt(resolver, Settings.Global.HDMI_CONTROL_ENABLED, 1) != 0;
        return cecEnabled ? "Enabled" : "Disabled" ;
    }

}
