package com.droidlogic.googletv.settings.sliceprovider.dialog;

import static android.app.slice.Slice.EXTRA_TOGGLE_STATE;
import static com.android.tv.twopanelsettings.slices.SlicesConstants.EXTRA_PREFERENCE_KEY;

import android.app.Activity;
import android.os.Bundle;
import android.view.WindowManager;
import androidx.appcompat.app.AlertDialog;
import com.droidlogic.googletv.settings.R;
import com.droidlogic.googletv.settings.sliceprovider.manager.AdvancedSoundManager;
import com.droidlogic.googletv.settings.sliceprovider.utils.MediaSliceUtil;

public class EnableUnsupportedSoundDialogActivity extends Activity {
  private static String TAG = EnableUnsupportedSoundDialogActivity.class.getSimpleName();
  private AlertDialog mAlertDialog;
  private Runnable mRestoreCallback = () -> {};
  private int mFormatId;

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    mFormatId =
        MediaSliceUtil.fetchSurroundSoundFormatIdFromKey(
            getApplicationContext(), getIntent().getStringExtra(EXTRA_PREFERENCE_KEY));
    boolean isChecked = getIntent().getBooleanExtra(EXTRA_TOGGLE_STATE, true);
    AdvancedSoundManager.getAdvancedSoundManager(getApplicationContext())
        .setSurroundManualFormatsSetting(isChecked, mFormatId);

    if (isChecked == false) {
      finish();
      return;
    }

    mRestoreCallback =
        () -> {
          AdvancedSoundManager.getAdvancedSoundManager(getApplicationContext())
              .setSurroundManualFormatsSetting(false, mFormatId);
        };
    initAlertDialog();

    mAlertDialog.show();
    mAlertDialog.getButton(AlertDialog.BUTTON_NEGATIVE).requestFocus();
  }

  private void initAlertDialog() {
    AlertDialog.Builder builder = new AlertDialog.Builder(this);
    builder.setCancelable(false);
    builder.setPositiveButton(
        R.string.enable_unsupported_sound_format_dialog_ok_msg,
        (dialog, which) -> {
          finish();
        });
    builder.setNegativeButton(
        R.string.enable_unsupported_sound_format_dialog_cancel_msg,
        (dialog, which) -> {
          mRestoreCallback.run();
          finish();
        });

    builder.setTitle(getString(R.string.enable_unsupported_sound_format_dialog_title));
    builder.setMessage(getString(R.string.enable_unsupported_sound_format_dialog_desc));

    mAlertDialog = builder.create();
    mAlertDialog.getWindow().setType(WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY);
  }
}
