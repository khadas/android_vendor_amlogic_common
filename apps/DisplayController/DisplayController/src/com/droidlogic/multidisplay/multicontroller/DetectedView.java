package com.droidlogic.multidisplay.multicontroller;

import android.content.Context;
import android.content.res.Configuration;
import android.view.View;

public class DetectedView extends View {

    public DisplayMoveListener mListener;

    public DetectedView(Context context) {
        super(context);
    }

    public void setMoveListener(DisplayMoveListener listener) {
        mListener = listener;
    }

   // @Override
    public void onMovedToDisplay(int displayId, Configuration config) {
        if (mListener != null) {
            mListener.onMovedToDisplay(displayId, config);
        }
        //super.onMovedToDisplay(displayId, config);
    }
    public interface DisplayMoveListener{
        void onMovedToDisplay(int displayId, Configuration config);
    }
}
