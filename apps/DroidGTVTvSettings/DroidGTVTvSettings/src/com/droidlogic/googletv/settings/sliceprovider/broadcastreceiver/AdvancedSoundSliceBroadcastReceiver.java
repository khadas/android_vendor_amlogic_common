package com.droidlogic.googletv.settings.sliceprovider.broadcastreceiver;

import static android.app.slice.Slice.EXTRA_TOGGLE_STATE;
import static com.android.tv.twopanelsettings.slices.SlicesConstants.EXTRA_PREFERENCE_KEY;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import com.droidlogic.googletv.settings.R;
import com.droidlogic.googletv.settings.sliceprovider.MediaSliceConstants;
import com.droidlogic.googletv.settings.sliceprovider.manager.AdvancedSoundManager;
import com.droidlogic.googletv.settings.sliceprovider.utils.MediaSliceUtil;

public class AdvancedSoundSliceBroadcastReceiver extends BroadcastReceiver {

  private static final String TAG = AdvancedSoundSliceBroadcastReceiver.class.getSimpleName();
  private static final long REFRESH_DELAY_MS = 100L;
  private final Handler mHandler = new Handler();

  @Override
  public void onReceive(Context context, Intent intent) {
    final String action = intent.getAction();
    boolean isChecked;
    switch (action) {
      case MediaSliceConstants.ACTION_SURROUND_SOUND_ENABLED:
        isChecked = intent.getBooleanExtra(EXTRA_TOGGLE_STATE, true);
        if (isChecked) {
          getAdvancedSoundManager(context).restoreSurroundPassthroughSetting();
        } else {
          getAdvancedSoundManager(context)
              .setSurroundPassthroughSetting(AdvancedSoundManager.VAL_SURROUND_SOUND_NEVER);
        }
        mHandler.postDelayed(
            () -> {
              context
                  .getContentResolver()
                  .notifyChange(MediaSliceConstants.SURROUND_SOUND_TOGGLE_URI, null);
              context
                  .getContentResolver()
                  .notifyChange(MediaSliceConstants.ADVANCED_SOUND_SETTINGS_URI, null);
              context
                  .getContentResolver()
                  .notifyChange(
                      MediaSliceConstants.ADVANCED_SOUND_SETTINGS_FORMAT_SELECTION_URI, null);
            },
            REFRESH_DELAY_MS);
        break;
      case MediaSliceConstants.ACTION_SOUND_FORMAT_CONTROL_POLICY_CHANGED:
        String key = intent.getStringExtra(EXTRA_PREFERENCE_KEY);
        if (context.getString(R.string.sound_auto_select_format_key).equals(key)) {
          getAdvancedSoundManager(context)
              .setSurroundPassthroughSetting(AdvancedSoundManager.VAL_SURROUND_SOUND_AUTO);
        } else if (context.getString(R.string.sound_manual_select_format_key).equals(key)) {
          getAdvancedSoundManager(context)
              .setSurroundPassthroughSetting(AdvancedSoundManager.VAL_SURROUND_SOUND_MANUAL);
        }
        // Change on surround sound policy takes some time to take effect
        mHandler.postDelayed(
            () -> {
              context
                  .getContentResolver()
                  .notifyChange(
                      MediaSliceConstants.ADVANCED_SOUND_SETTINGS_FORMAT_SELECTION_URI, null);
            },
            REFRESH_DELAY_MS);

        break;
      case MediaSliceConstants.ACTION_SET_SOUND_FORMAT:
        isChecked = intent.getBooleanExtra(EXTRA_TOGGLE_STATE, true);
        int formatId =
            MediaSliceUtil.fetchSurroundSoundFormatIdFromKey(
                context, intent.getStringExtra(EXTRA_PREFERENCE_KEY));
        getAdvancedSoundManager(context).setSurroundManualFormatsSetting(isChecked, formatId);
        break;
    }
  }

  private AdvancedSoundManager getAdvancedSoundManager(Context context) {
    return AdvancedSoundManager.getAdvancedSoundManager(context);
  }
}
