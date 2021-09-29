package com.droidlogic.googletv.settings.sliceprovider;

import android.net.Uri;
import android.util.Log;
import androidx.slice.Slice;
import androidx.slice.SliceProvider;
import com.android.tv.twopanelsettings.slices.builders.PreferenceSliceBuilder;
import com.android.tv.twopanelsettings.slices.builders.PreferenceSliceBuilder.RowBuilder;
import com.droidlogic.googletv.settings.R;
import com.droidlogic.googletv.settings.sliceprovider.manager.GeneralContentManager;
import com.droidlogic.googletv.settings.sliceprovider.utils.MediaSliceUtil;

public class GeneralContentSliceProvider extends SliceProvider {
  private static final String TAG = GeneralContentSliceProvider.class.getSimpleName();
  private static final boolean DEBUG = true;
  private GeneralContentManager mGeneralContentManager;

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
      case MediaSliceConstants.GENERAL_INFO_PATH:
        // fill in Netfilx Esn into general info purposely
        return createNetFlixEsnSlice(sliceUri);
      default:
        return null;
    }
  }

  @Override
  public void shutdown() {
    GeneralContentManager.shutdown(getContext());
    mGeneralContentManager = null;
    super.shutdown();
  }

  private Slice createNetFlixEsnSlice(Uri sliceUri) {
    final PreferenceSliceBuilder psb = new PreferenceSliceBuilder(getContext(), sliceUri);

    if (!GeneralContentManager.isInit()) {
      mGeneralContentManager = GeneralContentManager.getGeneralContentManager(getContext());
    }

    if (mGeneralContentManager.getNetflixEsn() == null) {
      mGeneralContentManager.refresh();
      return null;
    }

    psb.setEmbeddedPreference(
        new RowBuilder()
            .setTitle(getContext().getString(R.string.netflix_esn_title))
            .setSubtitle(mGeneralContentManager.getNetflixEsn()));

    // force to refresh Netflix ESN next time
    mGeneralContentManager.setNetflixEsn(null);
    return psb.build();
  }
}
