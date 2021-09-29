package com.droidlogic.googletv.settings.sliceprovider.broadcastreceiver;

import static com.android.tv.twopanelsettings.slices.SlicesConstants.EXTRA_PREFERENCE_KEY;
import static android.app.slice.Slice.EXTRA_TOGGLE_STATE;

import android.app.ProgressDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.provider.Settings;
import android.view.WindowManager;


import com.droidlogic.googletv.settings.R;
import com.droidlogic.googletv.settings.sliceprovider.MediaSliceConstants;
import com.droidlogic.googletv.settings.sliceprovider.utils.MediaSliceUtil;
import com.droidlogic.googletv.settings.sliceprovider.manager.HdmiCecContentManager;


public class HdmiCecSliceBroadcastReceiver extends BroadcastReceiver {
  private static final String TAG = HdmiCecSliceBroadcastReceiver.class.getSimpleName();
  private ProgressDialog mProgress;
  private static final int MSG_ENABLE_CEC_SWITCH = 0;
  private static final int TIME_DELAYED = 5000;//ms
  private Handler mHandler = new Handler() {
      @Override
      public void handleMessage(Message msg) {
          switch (msg.what) {
              case MSG_ENABLE_CEC_SWITCH:
                  if (MediaSliceUtil.CanDebug()) Log.d(TAG, "enable cec switch");
                  if (mProgress != null && mProgress.isShowing()) {
                      mProgress.dismiss();
                  }
                  break;
              default:
                  break;
          }
      }
  };

  @Override
  public void onReceive(Context context, Intent intent) {
    final String action = intent.getAction();
    boolean isChecked;
    mProgress = new ProgressDialog(context);
    mProgress.setMessage("It takes a few seconds to update cec status, please wait...");
    mProgress.setIndeterminate(false);
    mProgress.getWindow().setType(WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY);
    String key;
    switch (action) {
      case MediaSliceConstants.ACTION_HDMI_SWITCH_CEC_CHANGED:
        key = intent.getStringExtra(EXTRA_PREFERENCE_KEY);
        isChecked = intent.getBooleanExtra(EXTRA_TOGGLE_STATE, true);
        getHdmiCecContentManager(context).setHdmiCecStatus(isChecked? 1 : 0);
        if (mProgress != null && !mProgress.isShowing()) {
            if (MediaSliceUtil.CanDebug()) Log.d(TAG, "check enable show dialog");
            mProgress.show();
        }
        mHandler.sendEmptyMessageDelayed(MSG_ENABLE_CEC_SWITCH, TIME_DELAYED);
        break;
    }
  }

  private HdmiCecContentManager getHdmiCecContentManager(Context context) {
    return HdmiCecContentManager.getHdmiCecContentManager(context);
  }

}
