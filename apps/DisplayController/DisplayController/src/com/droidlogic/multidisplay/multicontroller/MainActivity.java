package com.droidlogic.multidisplay.multicontroller;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.res.Configuration;
import android.graphics.Color;
import android.graphics.PorterDuff;
import android.hardware.display.DisplayManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.UserHandle;
import android.os.Message;
import android.util.Log;
import android.view.Display;
import android.view.View;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.ListView;
import android.widget.TextView;

import com.droidlogic.displaycontrollerservice.IMirrorDisplayInterface;

import java.util.ArrayList;
import java.util.List;


public class MainActivity extends Activity implements DisplayManager.DisplayListener, MirrorDisplayWrapper.UICallback {
    public static final String TAG = "MM-C";
    private static int EXP_TIME = 3*6000;//3min
    private MirrorDisplayWrapper mDisplayController;
    private ListView mMirrorList;
    private TextView mCurrentDisplayInfoTextView;
    private MirroredDeviceAdapter mAdapter;
    private List<MirroredData> mData;
    private boolean mConnected;
    private List<Button> mButtons = new ArrayList<>();
    private int mCurrentDisplayId;
    private Handler mHandler = new SwithHandler();
    private final ServiceConnection serviceConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            mDisplayController = new MirrorDisplayWrapper(IMirrorDisplayInterface.Stub.asInterface(service), MainActivity.this);
            mConnected = true;
            mAdapter.setController(mDisplayController);
            updateMirroredData();
            Log.d(TAG, "connected");
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            mDisplayController = null;
            mConnected = false;
        }
    };
    private DisplayManager mDisplayManager;

    private void initialUI() {
        mCurrentDisplayId = MainActivity.this.getDisplay().getDisplayId();
        Display[] ds = mDisplayManager.getDisplays();
        mData = new ArrayList<MirroredData>();
        for (Display d : ds) {
            if (d.getDisplayId() != mCurrentDisplayId) {
                mData.add(new MirroredData(d.getDisplayId(), isMirrored(d.getDisplayId())));
            }
        }
        mAdapter = new MirroredDeviceAdapter(MainActivity.this, mData);
        mMirrorList.setAdapter(mAdapter);

        mMirrorList.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {


            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                Log.d(TAG, "Clicked");
                updateViews();
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {

            }
        });
        Button mButton0 = findViewById(R.id.swith_display0);
        mButton0.setOnClickListener(mSwithDisplayClickController);
        Button mButton2 = findViewById(R.id.swith_display2);
        mButton2.setOnClickListener(mSwithDisplayClickController);
        Button mButton3 = findViewById(R.id.swith_display3);
        mButton3.setOnClickListener(mSwithDisplayClickController);
        mButtons.add(mButton0);
        mButtons.add(mButton2);
        mButtons.add(mButton3);
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);Log.d(TAG,"onConfigurationChanged"+getDisplay().getDisplayId());
        Log.d(TAG,"onConfigurationChanged"+newConfig);
    }

    private boolean isMirrored(int displayId) {
        return mConnected && mDisplayController.isMirrored(displayId);
    }

    private String currentDisplayInfoDump() {
        String builder = getResources().getString(R.string.display_device) + "\t" + mCurrentDisplayId + "\n" + getResources().getString(R.string.mirroring) + "\t" + isMirrored(mCurrentDisplayId) + "\n";
        return builder;
    }

    private void updateMirroredData() {
        for (MirroredData data : mData) {
            data.setMirrored(isMirrored(data.getDisplayId()));
        }
        mAdapter.notifyDataSetChanged();
        updateViews();
    }

    private void updateDisplayList(boolean add, int displayId) {
        if (add) {
            mData.add(new MirroredData(displayId, false));
        } else {
            for (MirroredData data : mData) {
                if (data.getDisplayId() == displayId) {
                    mData.remove(data);
                    break;
                }
            }
        }
        mAdapter.notifyDataSetChanged();
        updateViews();
    }

    private void updateViews() {
        mCurrentDisplayInfoTextView.setText(currentDisplayInfoDump());
        Log.d(TAG,"updateViews"+mConnected);
        if (mConnected) {
            mMirrorList.setVisibility(View.VISIBLE);
        } else {
            mMirrorList.setVisibility(View.INVISIBLE);
        }
        if (mDisplayManager.getDisplays().length < 3) {
            for (Button btn:mButtons) {
                btn.setEnabled(false);
            }
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.d(TAG,"onCreate");
        setContentView(R.layout.layout_main2);
        mDisplayManager = (DisplayManager) MainActivity.this.getSystemService(Context.DISPLAY_SERVICE);
        mCurrentDisplayInfoTextView = findViewById(R.id.current_display_info);
        mMirrorList = findViewById(R.id.mirror_list);
        initialUI();
    }


    @Override
    public void onResume() {
        super.onResume();
        Log.d(TAG,"onResume"+getDisplay().getDisplayId());
        updateViews();
        Intent intent = new Intent();
        intent.setComponent(new ComponentName("com.droidlogic.displaycontrollerservice", "com.droidlogic.displaycontrollerservice.MultiDisplayService"));
        bindService(intent, serviceConnection, Context.BIND_AUTO_CREATE);

        mDisplayManager.registerDisplayListener(MainActivity.this, null);
    }

    @Override
    protected void onPause() {
        super.onPause();Log.d(TAG,"onPause"+getDisplay().getDisplayId());
        mDisplayManager.unregisterDisplayListener(this);
    }

    @Override
    public void onDisplayAdded(int displayId) {
        updateDisplayList(true, displayId);
    }

    @Override
    public void onDisplayRemoved(int displayId) {
        updateDisplayList(false, displayId);
    }

    @Override
    public void onDisplayChanged(int displayId) {

    }

    @Override
    public void updateUI() {
        updateViews();
    }
    private View.OnClickListener mSwithDisplayClickController = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            v.setSelected(!v.isSelected());
            if (v.isSelected()) {
                v.getBackground().setColorFilter(Color.GREEN, PorterDuff.Mode.MULTIPLY);
            }else {
                v.getBackground().clearColorFilter();
            }
            setButtonClicked();
        }
    };
    private void setButtonClicked() {
        int formDisplayid = -1;
        int toDisplayid = -1;
        int count = 0;
        for (int i=0;i<mButtons.size();i++) {
            if (mButtons.get(i).isSelected()) {
                count++;
                if (formDisplayid == -1) {
                    formDisplayid = i;
                }else {
                    toDisplayid = i;
                }
            }
        }
        if (count == 2) {
            Display[] d = mDisplayManager.getDisplays();
            boolean succ = mDisplayController.swithDisplay(d[formDisplayid].getDisplayId(),d[toDisplayid].getDisplayId());
            if (succ) {
               // mMirrorList.setVisibility(View.INVISIBLE);
                Message msg = new Message();
                msg.what = 0;
                msg.arg1 = d[toDisplayid].getDisplayId();
                msg.arg2 = d[formDisplayid].getDisplayId();
                mHandler.sendMessageDelayed(msg,EXP_TIME);
                for (Button btn:mButtons) {
                    btn.setEnabled(false);
                }
            }
            for (Button btn:mButtons) {
                btn.getBackground().clearColorFilter();
            }
        }
    }
    private class SwithHandler extends  Handler{
        @Override
        public void handleMessage(Message msg) {
            mDisplayController.swithDisplay(msg.arg1,msg.arg2);
        mMirrorList.setVisibility(View.VISIBLE);
        for (Button btn:mButtons) {
        btn.setEnabled(false);
            }
            super.handleMessage(msg);
        }
    }
}
