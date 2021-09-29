package com.droidlogic.googletv.settings.sliceprovider.broadcastreceiver;

import static android.app.slice.Slice.EXTRA_TOGGLE_STATE;
import static com.android.tv.twopanelsettings.slices.SlicesConstants.EXTRA_PREFERENCE_KEY;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import com.droidlogic.googletv.settings.R;
import com.droidlogic.googletv.settings.sliceprovider.MediaSliceConstants;
import com.droidlogic.googletv.settings.sliceprovider.manager.DisplayCapabilityManager;
import com.droidlogic.googletv.settings.sliceprovider.utils.MediaSliceUtil;

public class HdrSliceBroadcastReceiver extends BroadcastReceiver {
  private static final String TAG = HdrSliceBroadcastReceiver.class.getSimpleName();
  private static final String ACTION_HDMI_PLUGGED= "android.intent.action.HDMI_PLUGGED";

  @Override
  public void onReceive(Context context, Intent intent) {
    final String action = intent.getAction();
    boolean isChecked;
    String key;
    switch (action) {
      case MediaSliceConstants.ACTION_MATCH_CONTENT_POLICY_CHANGED:
        key = intent.getStringExtra(EXTRA_PREFERENCE_KEY);
        boolean isSource = context.getString(R.string.hdr_match_content_source_key).equals(key);
        getDisplayCapabilityManager(context).setHdrPolicySource(isSource);
        context
            .getContentResolver()
            .notifyChange(MediaSliceConstants.HDR_AND_COLOR_FORMAT_URI, null);
        context.getContentResolver().notifyChange(MediaSliceConstants.MATCH_CONTENT_URI, null);
        break;
      case MediaSliceConstants.ACTION_SET_DOLBY_VISION_MODE:
        key = intent.getStringExtra(EXTRA_PREFERENCE_KEY);
        boolean isModeLL =
            context.getString(R.string.dolby_vision_mode_low_latency_key).equals(key);
        getDisplayCapabilityManager(context).setDolbyVisionModeLLPreferred(isModeLL);
        context
            .getContentResolver()
            .notifyChange(MediaSliceConstants.HDR_AND_COLOR_FORMAT_URI, null);
        context.getContentResolver().notifyChange(MediaSliceConstants.DOLBY_VISION_MODE_URI, null);
        break;
      case ACTION_HDMI_PLUGGED:
        if (MediaSliceUtil.CanDebug()) Log.d(TAG, "ACTION_HDMI_PLUGGED");
        context
            .getContentResolver()
            .notifyChange(MediaSliceConstants.RESOLUTION_URI, null);
        context
            .getContentResolver()
            .notifyChange(MediaSliceConstants.HDR_AND_COLOR_FORMAT_URI, null);
        context
            .getContentResolver()
            .notifyChange(MediaSliceConstants.MATCH_CONTENT_URI, null);
        context
            .getContentResolver()
            .notifyChange(MediaSliceConstants.DOLBY_VISION_MODE_URI, null);
        break;
      case MediaSliceConstants.ACTION_AUTO_BEST_RESOLUTIONS_ENABLED:
        isChecked = intent.getBooleanExtra(EXTRA_TOGGLE_STATE, true);
        if (MediaSliceUtil.CanDebug()) Log.d(TAG, "ACTION_AUTO_BEST_RESOLUTIONS_ENABLED isChecked: " + isChecked);
        String currentMode = getDisplayCapabilityManager(context).getCurrentMode();
        if (!isChecked) {
            getDisplayCapabilityManager(context).change2BestMode(currentMode);
        }else{
            getDisplayCapabilityManager(context).change2BestMode();
        }
        context
            .getContentResolver()
            .notifyChange(MediaSliceConstants.RESOLUTION_URI, null);
        break;
    }
  }

  private DisplayCapabilityManager getDisplayCapabilityManager(Context context) {
    return DisplayCapabilityManager.getDisplayCapabilityManager(context);
  }
}
