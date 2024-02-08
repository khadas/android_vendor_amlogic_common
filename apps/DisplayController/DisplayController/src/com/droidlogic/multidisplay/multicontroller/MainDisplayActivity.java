package com.droidlogic.multidisplay.multicontroller;

import android.app.Activity;
import android.content.res.Configuration;
import android.graphics.Color;
import android.hardware.display.DisplayManager;
import android.os.Bundle;
import android.util.Log;
import android.view.Display;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageButton;
import android.widget.LinearLayout;

import java.util.HashMap;

public class MainDisplayActivity extends Activity implements View.OnClickListener,
                                  DisplayView.FullScreenChangeListener, DisplayView.DisplayMoveListener {
    private static final String TAG = "SSS";
    private final HashMap<Integer, DisplayView> mControlViews = new HashMap<>();
    private DisplayManager mDisplayManager;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mDisplayManager = getSystemService(DisplayManager.class);
        setContentView(R.layout.activity_main);
        initialViews();
        ImageButton btn = findViewById(R.id.back_button);
        btn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                MainDisplayActivity.this.finish();
            }
        });
    }

    private void initialViews() {
        LinearLayout rootView = findViewById(R.id.rootview);
        Display[] ds = mDisplayManager.getDisplays();
        int i = 0;
        for (Display d : ds) {
            if (d.getDisplayId() != 0) {
                Log.d(TAG, "d.getDisplayId" + d.getDisplayId());
                DisplayView view = new DisplayView(MainDisplayActivity.this);
                view.setMirrorDisplayId(d.getDisplayId());
                view.setMoveListener(this);
                mControlViews.put(d.getDisplayId(), view);
                LinearLayout.LayoutParams param = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
                param.setMargins(1,1,1,1);
                param.weight = 1;
                rootView.addView(view, i, param);
                i++;
                view.setFullScreenListener(this);
            }
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    @Override
    public void onAttachedToWindow() {
        super.onAttachedToWindow();
    }

    @Override
    public void onDetachedFromWindow() {
        super.onDetachedFromWindow();
    }


    @Override
    public void onClick(View v) {
        if (v instanceof DisplayView) {
            Log.d(TAG, "clicked" + ((DisplayView) v).getMirroredDisplayId());
        }
    }

    @Override
    public void onFullScreenChanged(int displayId, boolean full) {
        if (full) {
            for (Integer i : mControlViews.keySet()) {
                if (i != displayId) {
                    Log.d(TAG, "displayid:" + i);
                    mControlViews.get(i).setSmall();
                }
            }
        } else {
            for (Integer i : mControlViews.keySet()) {
                if (i != displayId) {
                    Log.d(TAG, "displayid:" + i);
                    mControlViews.get(i).setDefault();
                }
            }
        }
    }

    @Override
    public void onMovedToDisplay(int displayId, Configuration config) {
        if (displayId != 0) {
            MainDisplayActivity.this.finish();
        }
    }
}
