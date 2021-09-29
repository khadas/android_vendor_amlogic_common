package com.droidlogic.googletv.settings.sliceprovider;

import android.net.Uri;
import android.util.Log;
import androidx.slice.Slice;
import androidx.slice.SliceProvider;
import com.android.tv.twopanelsettings.slices.builders.PreferenceSliceBuilder;
import com.android.tv.twopanelsettings.slices.builders.PreferenceSliceBuilder.RowBuilder;
import com.droidlogic.googletv.settings.R;
import com.droidlogic.googletv.settings.sliceprovider.manager.HdmiCecContentManager;
import com.droidlogic.googletv.settings.sliceprovider.utils.MediaSliceUtil;
import com.droidlogic.googletv.settings.sliceprovider.broadcastreceiver.HdmiCecSliceBroadcastReceiver;

import android.content.ContentResolver;
import android.provider.Settings;


public class HdmiCecContentSliceProvider extends MediaSliceProvider {
  private static final String TAG = HdmiCecContentSliceProvider.class.getSimpleName();
  private static final boolean DEBUG = true;
  private HdmiCecContentManager mHdmiCecContentManager;

  @Override
  public boolean onCreateSliceProvider() {
    return true;
  }

  @Override
  public Slice onBindSlice(final Uri sliceUri) {
    if (MediaSliceUtil.CanDebug()) {
       Log.d(TAG, "onBindSlice: " + sliceUri);
    }
    switch (MediaSliceUtil.getFirstSegment(sliceUri)) {
      case MediaSliceConstants.HDMI_CEC_PATH:
        // fill in Netfilx Esn into general info purposely
        return createHdmiCecSlice(sliceUri);
      default:
        return null;
    }
  }

  @Override
  public void shutdown() {
    HdmiCecContentManager.shutdown(getContext());
    mHdmiCecContentManager = null;
    super.shutdown();
  }

  private Slice createHdmiCecSlice(Uri sliceUri) {
    final PreferenceSliceBuilder psb = new PreferenceSliceBuilder(getContext(), sliceUri);

    if (!HdmiCecContentManager.isInit()) {
      mHdmiCecContentManager = HdmiCecContentManager.getHdmiCecContentManager(getContext());
    }

    psb.addPreference(
        new RowBuilder()
            .setKey(getContext().getString(R.string.hdmi_cec_switch_key))
            .setTitle(getContext().getString(R.string.hdmi_cec_switch_title))
            .setSubtitle(mHdmiCecContentManager.getHdmiCecStatusName())
            .addSwitch(
                generatePendingIntent(
                    getContext(),
                    MediaSliceConstants.ACTION_HDMI_SWITCH_CEC_CHANGED,
                    HdmiCecSliceBroadcastReceiver.class),mHdmiCecContentManager.getHdmiCecStatus()));

    return psb.build();
  }
}
