/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC ArcVolumeController
 */

package com.droidlogic;

import android.annotation.NonNull;
import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.media.AudioManager;
import android.media.AudioSystem;
import android.hardware.hdmi.HdmiControlManager;
import android.hardware.hdmi.HdmiTvClient;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.ImageView;
import android.widget.Toast;

import com.droidlogic.R;

/**
 * Process the business of volume bar when avr is connected on TV device.
 */
public class ArcVolumeController {
    private static final String TAG = ArcVolumeController.class.getSimpleName();

    private static final int ENABLED = 1;
    private static final int DISABLED = 0;

    private static final int ADJUST_MUTE = 1;
    private static final int ADJUST_OTHERS = 2;

    private static final int MSG_VOLUME_BAR_DISMISS = 1;
    private static final int MSG_SHOW_VOLUME_BAR = 2;
    private static final int MSG_SHOW_VOLUME_BAR_AVR_MUTE = 3;
    private static final int MSG_SHOW_ARC_PORT_WARNING = 4;
    private static final int MSG_VOLUME_MUTE_SELF_ADJUST = 5;

    private static final long DELAY_VOLUME_BAR_DISMISS = 1000;
    private static final long DELAY_AVR_MUTE = 2000;
    private static final long DELAY_ARC_PORT_WARNING = 2000;
    private static final long DELAY_VOLUME_MUTE_SELF_ADJUST = 2000;

    // Related with HdmiControlManager.OSD_MESSAGE_AVR_VOLUME_CHANGED = 2
    private static final int OSD_NAME_VOLUME_KEY = 3;

    private Context mContext;

    private HdmiTvClient mTvClient;
    private HdmiControlManager mHdmiControlManager;
    private AudioManager mAudioManager;

    // Use a dialog as the volume bar for avr volume events.
    private AlertDialog mVolumeBar;
    private View mVolumeView;

    private ImageView mViewVolumeKey;

    // when connected with avr, tv could always be initiated with unmute.
    private boolean mMute = false;

    private boolean mMuteSelfAdjust = false;

    private Handler mHandler = new Handler() {
        public void handleMessage(@NonNull Message msg) {
            switch (msg.what) {
                case MSG_VOLUME_BAR_DISMISS:
                    mVolumeBar.dismiss();
                    break;
                case MSG_SHOW_VOLUME_BAR:
                    showVolumeBar(ADJUST_MUTE == msg.arg1, ENABLED == msg.arg2);
                    break;
                case MSG_SHOW_VOLUME_BAR_AVR_MUTE:
                    showVolumeBar(true, ENABLED == (int)msg.obj);
                    break;
                case MSG_SHOW_ARC_PORT_WARNING:
                    Log.d(TAG, "Audio system is not connected to arc port!");
                    Toast.makeText(mContext, R.string.arc_port_warning, Toast.LENGTH_LONG).show();
                    break;
                case MSG_VOLUME_MUTE_SELF_ADJUST:
                    mMuteSelfAdjust = false;
                    break;
            }
        }
    };


    private final BroadcastReceiver mVolumeReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            handleVolumeChange(context, intent);
        }
    };

    public ArcVolumeController (Context context) {
        if (null == context) {
            return;
        }
        mContext = context;

        mHdmiControlManager = mContext.getSystemService(HdmiControlManager.class);
        if (null == mHdmiControlManager) {
            Log.e(TAG, "Can't get hdmi control manager!");
            return;
        }

        mTvClient = mHdmiControlManager.getTvClient();
        if (null == mTvClient) {
            Log.d(TAG, "Can't get hdmi tv client!");
            return;
        }
        mAudioManager = mContext.getSystemService(AudioManager.class);
        if (null == mAudioManager) {
            Log.d(TAG, "Can't get audio manager!");
            return;
        }

        Log.d(TAG, "create ArcVolumeController");
        createVolumeBar();

        registerRegisters();
    }

    private void createVolumeBar() {
        AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
        mVolumeBar = builder.create();
        mVolumeBar.getWindow().addFlags(WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE
            | WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL
            | WindowManager.LayoutParams.FLAG_SHOW_WHEN_LOCKED
            | WindowManager.LayoutParams.FLAG_WATCH_OUTSIDE_TOUCH
            | WindowManager.LayoutParams.FLAG_HARDWARE_ACCELERATED);

        mVolumeView = View.inflate(mContext, R.layout.tv_arc_volume_dialog, null);
        mViewVolumeKey = mVolumeView.findViewById(R.id.volume_key);

        mVolumeBar.getWindow().requestFeature(Window.FEATURE_NO_TITLE);
        mVolumeBar.getWindow().setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
        mVolumeBar.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_DIM_BEHIND
                | WindowManager.LayoutParams.FLAG_LAYOUT_INSET_DECOR);
        mVolumeBar.getWindow().setLayout(WindowManager.LayoutParams.WRAP_CONTENT,
                WindowManager.LayoutParams.WRAP_CONTENT);
        mVolumeBar.getWindow().setType(WindowManager.LayoutParams.TYPE_VOLUME_OVERLAY);
        WindowManager.LayoutParams params = mVolumeBar.getWindow().getAttributes();
        params.gravity = Gravity.CENTER_HORIZONTAL | Gravity.BOTTOM;
        mVolumeBar.getWindow().setAttributes(params);
    }

    private void registerRegisters() {
        IntentFilter filter = new IntentFilter();
        filter.addAction(AudioManager.VOLUME_CHANGED_ACTION);
        filter.addAction(AudioManager.STREAM_MUTE_CHANGED_ACTION);
        filter.addAction(HdmiControlManager.ACTION_OSD_MESSAGE);
        mContext.registerReceiver(mVolumeReceiver, filter, mContext.RECEIVER_EXPORTED);
    }

    private void handleVolumeChange(Context context, Intent intent) {
        String action = intent.getAction();
        boolean audioMode = mHdmiControlManager.getSystemAudioMode();
        if (!audioMode) {
            return;
        }
        int devices = mAudioManager.getDevicesForStream(AudioManager.STREAM_MUSIC);
        if ((devices & AudioSystem.DEVICE_OUT_HDMI_ARC) == 0) {
            // current device is not hdmi_arc.
            return;
        }

        switch (action) {
            case AudioManager.VOLUME_CHANGED_ACTION: {
                int streamType = intent.getIntExtra(AudioManager.EXTRA_VOLUME_STREAM_TYPE, -1);
                if (streamType != AudioManager.STREAM_MUSIC) {
                    return;
                }
                int volume = intent.getIntExtra(AudioManager.EXTRA_VOLUME_STREAM_VALUE, 0);
                int old = intent.getIntExtra(AudioManager.EXTRA_PREV_VOLUME_STREAM_VALUE, 0);
                boolean volumeUp = (volume - old) >= 0;
                Log.d(TAG, "handleVolumeChange old:" + old + " new:" + volume);
                //mHandler.sendMessage(Message.obtain(mHandler, MSG_SHOW_VOLUME_BAR, ADJUST_OTHERS, volumeUp ? ENABLED : DISABLED));
                break;
            }
            case AudioManager.STREAM_MUTE_CHANGED_ACTION: {
                int streamType = intent.getIntExtra(AudioManager.EXTRA_VOLUME_STREAM_TYPE, -1);
                if (streamType != AudioManager.STREAM_MUSIC) {
                    return;
                }
                boolean mute = intent.getBooleanExtra(AudioManager.EXTRA_STREAM_VOLUME_MUTED, false);
                Log.d(TAG, "handleVolumeChange mute:" + mute + " device mute:" + mMute);
                //mHandler.sendMessage(Message.obtain(mHandler, MSG_SHOW_VOLUME_BAR, ADJUST_MUTE, mute ? ENABLED : DISABLED));
                mMute = mute;
                break;
            }
            case HdmiControlManager.ACTION_OSD_MESSAGE: {
                int messageId = intent.getIntExtra(HdmiControlManager.EXTRA_MESSAGE_ID, -1);
                int extra = intent.getIntExtra(HdmiControlManager.EXTRA_MESSAGE_EXTRA_PARAM1, -1);
                Log.d(TAG, "handleVolumeChange message id:" + messageId + " extra:" + extra);
                switch (messageId) {
                    case HdmiControlManager.OSD_MESSAGE_AVR_VOLUME_CHANGED:
                        boolean mute = extra == HdmiControlManager.AVR_VOLUME_MUTED;
                        if (mute != mMute) {
                            Log.d(TAG, "avr mute:" + mute + " device mute:" + mMute);
                            mHandler.removeMessages(MSG_SHOW_VOLUME_BAR_AVR_MUTE, null);
                            mHandler.sendMessageDelayed(Message.obtain(mHandler, MSG_SHOW_VOLUME_BAR_AVR_MUTE,
                                mute ? ENABLED : DISABLED), mMuteSelfAdjust ? DELAY_AVR_MUTE : 0);
                        } else {
                            Log.d(TAG, "remove avr's previous audio status if exist");
                            if (mHandler.hasMessages(MSG_SHOW_VOLUME_BAR_AVR_MUTE)) {
                                mHandler.removeMessages(MSG_SHOW_VOLUME_BAR_AVR_MUTE, null);
                            }
                        }
                        break;
                    case HdmiControlManager.OSD_MESSAGE_ARC_CONNECTED_INVALID_PORT:
                        mHandler.removeMessages(MSG_SHOW_ARC_PORT_WARNING);
                        mHandler.sendEmptyMessageDelayed(MSG_SHOW_ARC_PORT_WARNING, DELAY_ARC_PORT_WARNING);
                        break;
                    case OSD_NAME_VOLUME_KEY:
                        int scene = 0;
                        int value = 0;
                        switch (extra) {
                            case KeyEvent.KEYCODE_VOLUME_DOWN:
                                scene = ADJUST_OTHERS;
                                value = DISABLED;
                                break;
                            case KeyEvent.KEYCODE_VOLUME_UP:
                                scene = ADJUST_OTHERS;
                                value = ENABLED;
                                break;
                            case KeyEvent.KEYCODE_VOLUME_MUTE:
                                scene = ADJUST_MUTE;
                                Log.d(TAG, "mute key with current mute:" + mMute);
                                if (mMute) {
                                    value = DISABLED;
                                } else {
                                    value = ENABLED;
                                }
                                mMuteSelfAdjust = true;
                                mHandler.removeMessages(MSG_VOLUME_MUTE_SELF_ADJUST);
                                mHandler.sendEmptyMessageDelayed(MSG_VOLUME_MUTE_SELF_ADJUST, DELAY_VOLUME_MUTE_SELF_ADJUST);
                                break;
                            default:
                                Log.e(TAG, "unknown key event:" + extra);
                                return;
                        }
                        mHandler.sendMessage(Message.obtain(mHandler, MSG_SHOW_VOLUME_BAR, scene, value));
                        break;
                }
                break;
            }
        }
    }

    public void showVolumeBar(boolean isMute, boolean on) {
        Log.d(TAG, "showVolumeBar mute:" + isMute + " " + on);
        // It has to dismiss and show again to get an animation.
        mVolumeBar.dismiss();
        if (isMute) {
            mMute = on;
            if (on) {
                mViewVolumeKey.setImageResource(R.drawable.tv_ic_arc_volume_mute);
            } else {
                mViewVolumeKey.setImageResource(R.drawable.tv_ic_arc_volume_unmute);
            }
        } else {
            if (on) {
                mViewVolumeKey.setImageResource(R.drawable.tv_ic_arc_volume_add);
            } else {
                mViewVolumeKey.setImageResource(R.drawable.tv_ic_arc_volume_sub);
            }
        }
        mVolumeBar.setView(mVolumeView);
        mVolumeBar.show();
        mHandler.removeMessages(MSG_VOLUME_BAR_DISMISS);
        mHandler.sendMessageDelayed(Message.obtain(mHandler, MSG_VOLUME_BAR_DISMISS), DELAY_VOLUME_BAR_DISMISS);
    }

}
