package com.droidlogic.googletv.settings.sliceprovider.ueventobserver;


import android.os.UEventObserver;

public class SetModeUEventObserver {
  private static final String TAG = SetModeUEventObserver.class.getSimpleName();
  private UEventObserver mUEventObserver;
  private boolean mIsObserved = false;
  Runnable mRunnable = () -> {};

  public SetModeUEventObserver() {
    mUEventObserver =
        new UEventObserver() {
          @Override
          public void onUEvent(UEvent uEvent) {
            //Observe set mode completed event
            if (uEvent.get("STATE").equals("ACA=0")) {
              mRunnable.run();
              mIsObserved = true;
            }
          }
        };
  }

  public void setOnUEventRunnable(Runnable runnable) {
    mRunnable = runnable;
  }

  public void startObserving() {
    resetObserved();
    mUEventObserver.startObserving("DEVPATH=/devices/platform/vout/extcon/setmode");
  }

  public void stopObserving() {
    mUEventObserver.stopObserving();
  }

  public boolean isObserved() {
    return mIsObserved;
  }

  public void resetObserved() {
    mIsObserved = false;
  }
}
