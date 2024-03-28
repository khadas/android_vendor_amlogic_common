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
import android.os.Message;
import android.os.SystemProperties;
import android.util.Log;
import android.view.Display;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.ImageButton;

import com.droidlogic.displaycontrollerservice.IMirrorDisplayInterface;

import java.util.ArrayList;
import java.util.List;
import java.util.HashMap;
import java.lang.reflect.*;

public class MainActivity extends Activity implements DisplayManager.DisplayListener, MirrorDisplayWrapper.UICallback, DetectedView.DisplayMoveListener, MirroredData.Callback {
    public static final String TAG = "MM-C";
    private static final int EXP_TIME = 3 * 6000;//3min
    private static final int SWITCH_DISPLAY = 1;
    private static final String RVC_PROP = "debug.display.using";
    private final List<Button> mButtons = new ArrayList<>();
    private final Handler mHandler = new SwithHandler();
    private final HashMap<Integer, String> mDevicesMap = new HashMap<Integer, String>();
    private final View.OnClickListener mStartController = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            Intent i = new Intent(MainActivity.this, MainDisplayActivity.class);
            MainActivity.this.startActivity(i);
        }
    };
    private MirrorDisplayWrapper mDisplayController;
    private ListView mMirrorList;
    private TextView mCurrentDisplayInfoTextView;
    private MirroredDeviceAdapter mAdapter;
    private List<MirroredData> mData;
    private boolean mConnected;
    private int mCurrentDisplayId;
    private DisplayManager mDisplayManager;
    private final View.OnClickListener mSwithDisplayClickController = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            v.setSelected(!v.isSelected());
            if (v.isSelected()) {
                v.getBackground().setColorFilter(Color.GREEN, PorterDuff.Mode.MULTIPLY);
            } else {
                v.getBackground().clearColorFilter();
            }
            setButtonClicked();
        }
    };
    private ImageButton mDisplayButton;
    private final ServiceConnection serviceConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            mDisplayController = new MirrorDisplayWrapper(IMirrorDisplayInterface.Stub.asInterface(service), MainActivity.this);
            mConnected = true;
            initialDeviceList();
            mAdapter.setController(mDisplayController);
            updateMirroredData();
            checkEnable();
            Log.d(TAG, "connected mmc-s");
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            mDisplayController = null;
            mConnected = false;
        }
    };

    private void initialUI() {
        View detectedView = new DetectedView(MainActivity.this);
        ViewGroup group = findViewById(R.id.rootview);
        group.addView(detectedView);
        mCurrentDisplayId = MainActivity.this.getDisplay().getDisplayId();
        Display[] ds = mDisplayManager.getDisplays();
        mData = new ArrayList<MirroredData>();
        for (Display d : ds) {
            if (d.getDisplayId() != mCurrentDisplayId) {
                Log.d(TAG, "update mData" + d.getDisplayId() + ":::" + mCurrentDisplayId);
                MirroredData mirrorData = new MirroredData(d.getDisplayId(), isMirrored(d.getDisplayId()));
                mirrorData.setFromDisplayId(mCurrentDisplayId);
                mirrorData.setCallback(MainActivity.this);
                mData.add(mirrorData);
            }
        }
        mAdapter = new MirroredDeviceAdapter(MainActivity.this);
        mAdapter.setData(mData);
        mAdapter.setDisplayId(mCurrentDisplayId);
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
        mDisplayButton = findViewById(R.id.monitor_display);
        mDisplayButton.setOnClickListener(mStartController);
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        Log.d(TAG, "onConfigurationChanged" + newConfig);
    }

    private boolean isMirrored(int displayId) {
        return mConnected && mDisplayController.isMirrored(displayId);
    }

    private boolean beMirrored(int displayId) {
        if (mConnected) {
            Log.d(TAG, "isMirroring " + mDisplayController.isMirroring(displayId));
        } else {
            Log.d(TAG, "not connected");
        }
        return mConnected && mDisplayController.isMirroring(displayId);
    }

    private String currentDisplayInfoDump() {
        String builder = getResources().getString(R.string.display_device) + "\t" + mCurrentDisplayId + "\n" + getResources().getString(R.string.mirroring) + "\t" + beMirrored(mCurrentDisplayId) + "\n";
        return builder;
    }

    private void updateMirroredData() {
        for (MirroredData data : mData) {
            data.setMirrored(isMirrored(data.getDisplayId()));
        }
        mAdapter.notifyDataSetChanged();
        updateViews();
    }

    private void displayButtonVisibleCheck() {
        int visible = mCurrentDisplayId == 0 ? View.VISIBLE : View.INVISIBLE;
        if (mCurrentDisplayId == 0) {
            for (Display d : mDisplayManager.getDisplays()) {
                if (isMirrored(d.getDisplayId())) {
                    visible = View.INVISIBLE;
                    break;
                }
            }
        }
        mDisplayButton.setVisibility(visible);
        Log.d(TAG, "updateDisplayChange" + visible);
    }

    private void updateDisplayChange(int fromDisplayId, int toDisplayId) throws Exception {
        updateViews();
        int size = mAdapter.getCount();
        for (int i = 0; i < size; i++) {
            MirroredData data = (MirroredData) mAdapter.getItem(i);
            if (data.getDisplayId() == toDisplayId) {
                data.setDisplayId(fromDisplayId);
                data.setMirrored(isMirrored(fromDisplayId));
            }
            Log.d(TAG, "isMirrored(fromDisplayId)" + fromDisplayId + " mCurrentDisplayId" + mCurrentDisplayId);
            if (!isMirrored(fromDisplayId)) {
                data.setFromDisplayId(mCurrentDisplayId);
            }
        }
        mAdapter.notifyDataSetChanged();
    }

    @Override
    public void onMovedToDisplay(int displayId, Configuration config) {
        Log.d(TAG, "updateDisplay" + displayId + " " + getDisplay().getDisplayId());
        int oldDisplayId = mCurrentDisplayId;
        mCurrentDisplayId = displayId;
        mAdapter.setDisplayId(mCurrentDisplayId);

        try {
            updateDisplayChange(oldDisplayId, mCurrentDisplayId);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void updateDisplayList(boolean add, int displayId) {
        Log.d(TAG, "updateDisplayList" + displayId);
        if (add) {
            MirroredData mirrorData = new MirroredData(displayId, false);
            mirrorData.setCallback(MainActivity.this);
            mirrorData.setFromDisplayId(mCurrentDisplayId);
            mData.add(mirrorData);
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

    private void hideViews(boolean enable) {
        if (!enable && mConnected && mDisplayController != null) {
            for (Display d : mDisplayManager.getDisplays()) {
                if (d.getDisplayId() == mCurrentDisplayId) continue;
                mDisplayController.stopMirror(d.getDisplayId());
            }
        }
        String text = enable ? currentDisplayInfoDump() : getResources().getString(R.string.disable_info);
        mCurrentDisplayInfoTextView.setText(text);
        mMirrorList.setVisibility(enable ? View.VISIBLE : View.INVISIBLE);
        for (Button btn : mButtons) {
            btn.setEnabled(enable);
        }
    }

    private void updateViews() {
        displayButtonVisibleCheck();
        mCurrentDisplayInfoTextView.setText(currentDisplayInfoDump());
        Log.d(TAG, "updateViews" + mConnected);
        if (mConnected) {
            mMirrorList.setVisibility(View.VISIBLE);
        } else {
            mMirrorList.setVisibility(View.INVISIBLE);
        }
        if (mDisplayManager.getDisplays().length < 3) {
            for (Button btn : mButtons) {
                btn.setEnabled(false);
            }
        } else {
            for (Button btn : mButtons) {
                btn.setEnabled(true);
            }
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.d(TAG, "onCreate");
        setContentView(R.layout.layout_main2);
        mDisplayManager = (DisplayManager) MainActivity.this.getSystemService(Context.DISPLAY_SERVICE);
        initialDeviceList();
        mCurrentDisplayInfoTextView = findViewById(R.id.current_display_info);
        mMirrorList = findViewById(R.id.mirror_list);
        initialUI();
    }

    private void initialDeviceList() {
        if (mDisplayController == null) return;
        mDevicesMap.clear();
        //port 32:mipi 1:lvds 2:hdmi
        for (Display display : mDisplayManager.getDisplays()) {
            int port = mDisplayController.getPhyPort(display.getDisplayId());
            if (port == 32) {
                mDevicesMap.put(display.getDisplayId(), "MIPI");
            } else if (port == 11) {
                mDevicesMap.put(display.getDisplayId(), "HDMI");
            } else if (port == 1) {
                mDevicesMap.put(display.getDisplayId(), "LVDS");
            }
        }
    }

    @Override
    public String getName(int displayId) {
        String name = mDevicesMap.get(displayId);
        if (name == null) return "DISPLAY" + displayId;
        return name;
    }

    @Override
    public void onResume() {
        super.onResume();
        int visible = mCurrentDisplayId == 0 ? View.VISIBLE : View.INVISIBLE;
        Log.d(TAG, "onResume" + mCurrentDisplayId);
        updateViews();
        Intent intent = new Intent();
        intent.setComponent(new ComponentName("com.droidlogic.displaycontrollerservice", "com.droidlogic.displaycontrollerservice.MultiDisplayService"));
        bindService(intent, serviceConnection, Context.BIND_AUTO_CREATE);
        mDisplayManager.registerDisplayListener(MainActivity.this, null);
        checkEnable();
    }

    private boolean checkEnable() {
        int result = SystemProperties.getInt(RVC_PROP, 0);
        Log.d(TAG, "check rvc " + result);
        boolean enable = result == 0;
        hideViews(enable);
        return enable;
    }

    @Override
    protected void onPause() {
        super.onPause();
        Log.d(TAG, "onPause" + mCurrentDisplayId);
        mDisplayManager.unregisterDisplayListener(this);
        unbindService(serviceConnection);
    }

    @Override
    public void onDisplayAdded(int displayId) {
        initialDeviceList();
        updateDisplayList(true, displayId);
    }

    @Override
    public void onDisplayRemoved(int displayId) {
        updateDisplayList(false, displayId);
    }

    @Override
    public void onDisplayChanged(int displayId) {
        Log.d(TAG, "onDisplayChanged" + displayId);
        updateUI();
    }

    @Override
    public void updateUI() {
        updateMirroredData();
        //updateViews();
    }
    @Override
    public boolean checkDisplay() {
        return checkEnable();
    }
    private void setButtonClicked() {
        int formDisplayid = -1;
        int toDisplayid = -1;
        int count = 0;
        for (int i = 0; i < mButtons.size(); i++) {
            if (mButtons.get(i).isSelected()) {
                count++;
                if (formDisplayid == -1) {
                    formDisplayid = i;
                } else {
                    toDisplayid = i;
                }
            }
        }
        if (count == 2) {
            Display[] d = mDisplayManager.getDisplays();
            boolean succ = mDisplayController.swithDisplay(d[formDisplayid].getDisplayId(), d[toDisplayid].getDisplayId());
            if (succ) {
                // mMirrorList.setVisibility(View.INVISIBLE);
                Message msg = new Message();
                msg.what = SWITCH_DISPLAY;
                msg.arg1 = d[toDisplayid].getDisplayId();
                msg.arg2 = d[formDisplayid].getDisplayId();
                mHandler.sendMessageDelayed(msg, EXP_TIME);
                for (Button btn : mButtons) {
                    btn.setEnabled(false);
                }
            }
            for (Button btn : mButtons) {
                btn.getBackground().clearColorFilter();
            }
        }
    }

    private class SwithHandler extends Handler {
        @Override
        public void handleMessage(Message msg) {
            if (msg.what == SWITCH_DISPLAY) {
                mDisplayController.swithDisplay(msg.arg1, msg.arg2);
                mMirrorList.setVisibility(View.VISIBLE);
                for (Button btn : mButtons) {
                    btn.setEnabled(true);
                }
            }
        }
    }

}
