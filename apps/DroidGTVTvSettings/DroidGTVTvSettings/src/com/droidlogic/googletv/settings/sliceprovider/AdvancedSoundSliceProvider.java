package com.droidlogic.googletv.settings.sliceprovider;

import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Handler;
import android.util.Log;
import androidx.slice.Slice;
import com.android.tv.twopanelsettings.slices.builders.PreferenceSliceBuilder;
import com.android.tv.twopanelsettings.slices.builders.PreferenceSliceBuilder.RowBuilder;
import com.droidlogic.googletv.settings.R;
import com.droidlogic.googletv.settings.sliceprovider.broadcastreceiver.AdvancedSoundSliceBroadcastReceiver;
import com.droidlogic.googletv.settings.sliceprovider.dialog.EnableUnsupportedSoundDialogActivity;
import com.droidlogic.googletv.settings.sliceprovider.manager.AdvancedSoundManager;
import com.droidlogic.googletv.settings.sliceprovider.utils.MediaSliceUtil;
import java.util.Map;

public class AdvancedSoundSliceProvider extends MediaSliceProvider {
  private static final String TAG = AdvancedSoundSliceProvider.class.getSimpleName();
  private static final boolean DEBUG = true;

  private AdvancedSoundManager mAdvancedSoundManager;
  private Handler mHandler = new Handler();

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
      case MediaSliceConstants.SURROUND_SOUND_TOGGLE_PATH:
        return createSurroundSoundToggleSlice(sliceUri);
      case MediaSliceConstants.ADVANCED_SOUND_SETTINGS_PATH:
        return createAdvancedSoundSettingsSlice(sliceUri);
      case MediaSliceConstants.ADVANCED_SOUND_SETTINGS_FORMAT_SELECTION_PATH:
        return createAdvancedSoundSettingsFormatSlice(sliceUri);
      default:
        return null;
    }
  }

  private boolean initAdvancedSoundManager(final Uri callbackUri) {
    if (mAdvancedSoundManager != null) {
      return true;
    }
    if (!AdvancedSoundManager.isInit()) {
      final Context context = getContext();
      // Hander is uesd to avoid executing IO operation in UI thread.
      mHandler.post(
          () -> {
            AdvancedSoundManager.getAdvancedSoundManager(context);
            context.getContentResolver().notifyChange(callbackUri, null);
          });
      return false;
    } else {
      mAdvancedSoundManager = AdvancedSoundManager.getAdvancedSoundManager(getContext());
      return true;
    }
  }

  private void tryRefreshAdvancedSoundManager(final Uri callbackUri) {
    if (AdvancedSoundManager.isInit()) {
      final Context context = getContext();
      mHandler.post(
          () -> {
            if (mAdvancedSoundManager.refresh()) {
              context.getContentResolver().notifyChange(callbackUri, null);
            }
          });
    }
  }

  private Slice createSurroundSoundToggleSlice(Uri sliceUri) {
    if (!initAdvancedSoundManager(sliceUri)) {
      return null;
    } else {
      tryRefreshAdvancedSoundManager(sliceUri);
    }

    final PreferenceSliceBuilder psb = new PreferenceSliceBuilder(getContext(), sliceUri);
    String surrondSoundPassthrough = mAdvancedSoundManager.getSurroundPassthroughSetting();

    psb.setEmbeddedPreference(
        new RowBuilder()
            .setTitle(getContext().getString(R.string.surround_sound_switch_title))
            .addSwitch(
                generatePendingIntent(
                    getContext(),
                    MediaSliceConstants.ACTION_SURROUND_SOUND_ENABLED,
                    AdvancedSoundSliceBroadcastReceiver.class),
                !surrondSoundPassthrough.equals(AdvancedSoundManager.VAL_SURROUND_SOUND_NEVER)));

    return psb.build();
  }

  private Slice createAdvancedSoundSettingsSlice(Uri sliceUri) {
    final PreferenceSliceBuilder psb = new PreferenceSliceBuilder(getContext(), sliceUri);

    psb.addScreenTitle(
        new RowBuilder().setTitle(getContext().getString(R.string.advanced_sound_settings_title)));
    psb.setEmbeddedPreference(
        new RowBuilder().setTitle(getContext().getString(R.string.advanced_sound_settings_title)));

    if (!initAdvancedSoundManager(sliceUri)) {
      return psb.build();
    } else {
      tryRefreshAdvancedSoundManager(sliceUri);
    }

    String surrondSoundPassthrough = mAdvancedSoundManager.getSurroundPassthroughSetting();
    String subtitle = null;
    if (surrondSoundPassthrough.equals(AdvancedSoundManager.VAL_SURROUND_SOUND_AUTO)) {
      subtitle = getContext().getString(R.string.sound_auto_select_format_title);
    } else if (surrondSoundPassthrough.equals(AdvancedSoundManager.VAL_SURROUND_SOUND_MANUAL)) {
      subtitle = getContext().getString(R.string.sound_manual_select_format_title);
    } else if (surrondSoundPassthrough.equals(AdvancedSoundManager.VAL_SURROUND_SOUND_NEVER)) {
      subtitle = getContext().getString(R.string.sound_disabled_title);
    }

    psb.addPreference(
        new RowBuilder()
            .setTitle(getContext().getString(R.string.surround_sound_format_selection_title))
            .setSubtitle(subtitle)
            .setEnabled(
                !surrondSoundPassthrough.equals(AdvancedSoundManager.VAL_SURROUND_SOUND_NEVER))
            .setTargetSliceUri(
                MediaSliceUtil.generateTargetSliceUri(
                    MediaSliceConstants.ADVANCED_SOUND_SETTINGS_FORMAT_SELECTION_PATH)));

    return psb.build();
  }

  private Slice createAdvancedSoundSettingsFormatSlice(Uri sliceUri) {
    final PreferenceSliceBuilder psb = new PreferenceSliceBuilder(getContext(), sliceUri);

    if (!initAdvancedSoundManager(sliceUri)) {
      return psb.build();
    } else {
      tryRefreshAdvancedSoundManager(sliceUri);
    }

    String surrondSoundPassthrough = mAdvancedSoundManager.getSurroundPassthroughSetting();
    if (surrondSoundPassthrough.equals(AdvancedSoundManager.VAL_SURROUND_SOUND_NEVER)) {
      return psb.build();
    }

    psb.addScreenTitle(
        new RowBuilder().setTitle(getContext().getString(R.string.format_selection_title)));

    psb.addPreference(
        new RowBuilder()
            .setKey(getContext().getString(R.string.sound_auto_select_format_key))
            .setTitle(getContext().getString(R.string.sound_auto_select_format_title))
            .setSubtitle(getContext().getString(R.string.sound_auto_select_format_summary))
            .setInfoSummary(getContext().getString(R.string.sound_auto_select_format_help_text))
            .addRadioButton(
                generatePendingIntent(
                    getContext(),
                    MediaSliceConstants.ACTION_SOUND_FORMAT_CONTROL_POLICY_CHANGED,
                    AdvancedSoundSliceBroadcastReceiver.class),
                AdvancedSoundManager.VAL_SURROUND_SOUND_AUTO.equals(surrondSoundPassthrough),
                getContext().getString(R.string.sound_format_select_type_radio_group_name)));
    psb.addPreference(
        new RowBuilder()
            .setKey(getContext().getString(R.string.sound_manual_select_format_key))
            .setTitle(getContext().getString(R.string.sound_manual_select_format_title))
            .setSubtitle(getContext().getString(R.string.sound_manual_select_format_summary))
            .setInfoSummary(getContext().getString(R.string.sound_manual_select_format_help_text))
            .addRadioButton(
                generatePendingIntent(
                    getContext(),
                    MediaSliceConstants.ACTION_SOUND_FORMAT_CONTROL_POLICY_CHANGED,
                    AdvancedSoundSliceBroadcastReceiver.class),
                AdvancedSoundManager.VAL_SURROUND_SOUND_MANUAL.equals(surrondSoundPassthrough),
                getContext().getString(R.string.sound_format_select_type_radio_group_name)));

    if (AdvancedSoundManager.VAL_SURROUND_SOUND_MANUAL.equals(surrondSoundPassthrough)) {
      updateSoundFormatDetails(psb);
    }
    return psb.build();
  }

  private void updateSoundFormatDetails(PreferenceSliceBuilder psb) {
    Map<Integer, Boolean> allSoundFormats = mAdvancedSoundManager.getAllSoundFormats();
    Map<Integer, Boolean> allReportedSoundFormats =
        mAdvancedSoundManager.getAllReportedSoundFormats();
    Map<Integer, Boolean> allSoundFormatStatuses =
        mAdvancedSoundManager.getAllSoundFormatStatuses();
    boolean isManualMode =
        mAdvancedSoundManager
            .getSurroundPassthroughSetting()
            .equals(AdvancedSoundManager.VAL_SURROUND_SOUND_MANUAL);

    if (MediaSliceUtil.CanDebug()) Log.d(TAG,"allReportedSoundFormats.isEmpty():"+allReportedSoundFormats.isEmpty());
    if (!allReportedSoundFormats.isEmpty()) {
      psb.addPreferenceCategory(
          new RowBuilder()
              .setTitle(getContext().getString(R.string.sound_supported_format_category_title)));
      for (Map.Entry<Integer, Boolean> format : allReportedSoundFormats.entrySet()) {
        int formatId = format.getKey();
        psb.addPreference(
            new RowBuilder()
                .setKey(
                    getContext().getString(R.string.surround_sound_format_key_prefix) + formatId)
                .setTitle(mAdvancedSoundManager.getFormatDisplayName(formatId))
                .setEnabled(isManualMode)
                .addSwitch(
                    generatePendingIntent(
                        getContext(),
                        MediaSliceConstants.ACTION_SET_SOUND_FORMAT,
                        AdvancedSoundSliceBroadcastReceiver.class),
                    allSoundFormatStatuses.get(formatId)));
      }
    }
    if (!allSoundFormats.equals(allReportedSoundFormats)) {
      psb.addPreferenceCategory(
          new RowBuilder()
              .setTitle(getContext().getString(R.string.sound_unsupported_format_category_title)));
      for (Map.Entry<Integer, Boolean> format : allSoundFormats.entrySet()) {
        int formatId = format.getKey();
        if (allReportedSoundFormats.containsKey(formatId)) continue;
        psb.addPreference(
            new RowBuilder()
                .setKey(MediaSliceUtil.generateKeyFromSurroundSoundFormatId(getContext(), formatId))
                .setTitle(mAdvancedSoundManager.getFormatDisplayName(formatId))
                .setEnabled(isManualMode)
                .addSwitch(
                    generateEnableUnsupportedSoundDialogPendingIntent(
                        MediaSliceConstants.SHOW_UNSUPPORTED_FORMAT_CHNAGE_WARNING),
                    allSoundFormatStatuses.get(formatId)));
      }
    }
  }

  private PendingIntent generateEnableUnsupportedSoundDialogPendingIntent(String action) {
    final Intent intent = new Intent(action);
    intent.setClass(getContext(), EnableUnsupportedSoundDialogActivity.class);
    return PendingIntent.getActivity(getContext(), 0, intent, PendingIntent.FLAG_UPDATE_CURRENT);
  }
}
