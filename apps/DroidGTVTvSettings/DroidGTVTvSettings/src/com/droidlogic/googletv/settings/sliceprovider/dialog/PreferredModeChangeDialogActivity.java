package com.droidlogic.googletv.settings.sliceprovider.dialog;

import static com.android.tv.twopanelsettings.slices.SlicesConstants.EXTRA_PREFERENCE_KEY;

import android.app.Activity;
import android.os.Bundle;
import android.os.CountDownTimer;
import android.os.PowerManager;
import android.content.Context;
import android.view.WindowManager;
import android.widget.Button;
import androidx.appcompat.app.AlertDialog;
import com.droidlogic.googletv.settings.R;
import com.droidlogic.googletv.settings.sliceprovider.manager.DisplayCapabilityManager;
import com.droidlogic.googletv.settings.sliceprovider.manager.DisplayCapabilityManager.HdrFormat;
import com.droidlogic.googletv.settings.sliceprovider.utils.MediaSliceUtil;
import java.util.concurrent.TimeUnit;
import android.util.Log;

public class PreferredModeChangeDialogActivity extends Activity {
  private static String TAG = PreferredModeChangeDialogActivity.class.getSimpleName();
  private static final String COUNTDOWN_PLACEHOLDER = "COUNTDOWN_PLACEHOLDER";
  private static final String RESOLUTION_PLACEHOLDER = "RESOLUTION_PLACEHOLDER";
  private AlertDialog mAlertDialog;
  private Runnable mRestoreCallback = () -> {};
  private DisplayCapabilityManager mDisplayCapabilityManager;
  private static int DEFAULT_COUNTDOWN_SECONDS = 15;
  private CountDownTimer mCountDownTimer;
  private int countdownInSeconds = 0;
  private boolean mDisplayModeWasChanged;
  private boolean wasHdrPolicyChanged;
  HdrFormat targetPreferredFormat;
  HdrFormat currentPreferredFormat;

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    mDisplayCapabilityManager =
        DisplayCapabilityManager.getDisplayCapabilityManager(getApplicationContext());

    currentPreferredFormat = mDisplayCapabilityManager.getPreferredFormat();
    targetPreferredFormat =
        HdrFormat.fromKey(getIntent().getStringExtra(EXTRA_PREFERENCE_KEY));
    if (currentPreferredFormat == targetPreferredFormat) finish();
    String currentMode = mDisplayCapabilityManager.getCurrentMode();
    if (MediaSliceUtil.CanDebug()) Log.d(TAG,"onCreate currentPreferredFormat:"+currentPreferredFormat+" targetPreferredFormat:"+targetPreferredFormat+" currentMode:"+currentMode);
    //wasHdrPolicyChanged = changeHdrPolicyIfNeeded(currentPreferredFormat, targetPreferredFormat);
    //mDisplayModeWasChanged = changeDisplayModeIfNeeded(targetPreferredFormat, currentMode);
    mDisplayCapabilityManager.setPreferredFormat(targetPreferredFormat);

    mRestoreCallback =
        () -> {
          if (MediaSliceUtil.CanDebug()) Log.d(TAG,"mRestoreCallback currentPreferredFormat:"+currentPreferredFormat);
          mDisplayCapabilityManager.setPreferredFormat(currentPreferredFormat);
          /*if (mDisplayModeWasChanged) {
            mDisplayCapabilityManager.setResolutionAndRefreshRateByMode(currentMode);
          }
          if (wasHdrPolicyChanged) {
            mDisplayCapabilityManager.setHdrPolicySource(
                !mDisplayCapabilityManager.isHdrPolicySource());
          }*/
        };
    initAlertDialog();
    showDialog();
  }

  private boolean changeDisplayModeIfNeeded(HdrFormat targetPreferredFormat, String currentMode) {
    if (targetPreferredFormat == HdrFormat.DOLBY_VISION
        && !mDisplayCapabilityManager.doesModeSupportDolbyVision(currentMode)) {
      mDisplayCapabilityManager.setModeSupportingDolbyVision();
      return true;
    }
    return false;
  }

  private boolean changeHdrPolicyIfNeeded(
      HdrFormat currentPreferredFormat, HdrFormat targetPreferredFormat) {
    if (HdrFormat.SDR == targetPreferredFormat && !mDisplayCapabilityManager.isHdrPolicySource()) {
      // set to follow source to make sure we don't force SDR content to HDR
      mDisplayCapabilityManager.setHdrPolicySource(true);
      return true;
    } else if (HdrFormat.SDR == currentPreferredFormat
        && mDisplayCapabilityManager.isHdrPolicySource()) {
      // set to "Always HDR/Dolby Vision" when changed from SDR
      mDisplayCapabilityManager.setHdrPolicySource(false);
      return true;
    }
    return false;
  }

  private void initAlertDialog() {
    AlertDialog.Builder builder = new AlertDialog.Builder(this);
    builder.setCancelable(false);
    builder.setPositiveButton(
        R.string.preferred_mode_change_dialog_ok_msg,
        (dialog, which) -> {
          if (mCountDownTimer != null) {
            mCountDownTimer.cancel();
          }
          finish();
          if (MediaSliceUtil.CanDebug()) Log.d(TAG,"initAlertDialog OK reboot");
          // click ok
          mDisplayCapabilityManager.setPreferredFormat(targetPreferredFormat);
          mDisplayCapabilityManager.setHdrPriority(targetPreferredFormat);
          reboot();
        });
    builder.setNegativeButton(
        R.string.preferred_mode_change_dialog_cancel_msg,
        (dialog, which) -> {
          if (mCountDownTimer != null) {
            mCountDownTimer.cancel();
          }
          // click cancel
          if (MediaSliceUtil.CanDebug()) Log.d(TAG,"initAlertDialog cancel");
          mRestoreCallback.run();
          finish();
        });

    builder.setTitle(getString(R.string.preferred_mode_change_dialog_title));
    builder.setMessage(
        mDisplayModeWasChanged
            ? getString(R.string.preferred_mode_change_and_display_mode_change_dialog_desc)
                .replace(
                    RESOLUTION_PLACEHOLDER,
                    mDisplayCapabilityManager.getTitleByMode(
                        mDisplayCapabilityManager.getCurrentMode()))
            : getString(R.string.preferred_mode_change_dialog_desc));

    mAlertDialog = builder.create();
    mAlertDialog.getWindow().setType(WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY);
  }

  private void showDialog() {
    mAlertDialog.show();
    mAlertDialog.getButton(AlertDialog.BUTTON_NEGATIVE).requestFocus();
    countdownInSeconds = DEFAULT_COUNTDOWN_SECONDS;
    mCountDownTimer =
        new CountDownTimer(
            TimeUnit.SECONDS.toMillis(DEFAULT_COUNTDOWN_SECONDS), TimeUnit.SECONDS.toMillis(1L)) {

          final Button cancelButton = mAlertDialog.getButton(AlertDialog.BUTTON_NEGATIVE);

          @Override
          public void onTick(long millisUntilFinished) {
            cancelButton.setText(
                getString(R.string.preferred_mode_change_dialog_cancel_msg)
                    .replace(COUNTDOWN_PLACEHOLDER, String.valueOf(countdownInSeconds)));
            if (countdownInSeconds != 0) {
              countdownInSeconds--;
            }
          }

          @Override
          public void onFinish() {
            mAlertDialog.dismiss();
            mRestoreCallback.run();
            finish();
            if (MediaSliceUtil.CanDebug()) Log.d(TAG,"showDialog timeout");
          }
        };
    mCountDownTimer.start();
  }

    private void reboot() {
        if (MediaSliceUtil.CanDebug()) Log.d(TAG,"reboot");
        Context context = (Context) (getApplicationContext());
        ((PowerManager) context.getSystemService(Context.POWER_SERVICE)).reboot(null);
    }

}
