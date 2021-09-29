package com.droidlogic.googletv.settings.sliceprovider;

import android.app.Activity;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import androidx.slice.SliceProvider;

abstract class MediaSliceProvider extends SliceProvider {
  protected static PendingIntent generatePendingIntent(
      Context context, String action, Class<?> receiverClass) {
    final Intent intent = new Intent(action);
    intent.setClass(context, receiverClass);
    if (Activity.class.isAssignableFrom(receiverClass)) {
      return PendingIntent.getActivity(context, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT);
    }
    return PendingIntent.getBroadcast(context, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT);
  }
}
