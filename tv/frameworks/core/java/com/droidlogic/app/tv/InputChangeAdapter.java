/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.app.tv;

import android.content.Context;
import android.content.Intent;
import android.hardware.hdmi.HdmiControlManager;
import android.hardware.hdmi.HdmiDeviceInfo;
import android.hardware.hdmi.HdmiTvClient;
import android.media.tv.TvInputInfo;
import android.media.tv.TvInputManager;
import android.util.Log;
import android.os.SystemProperties;
import android.text.TextUtils;

import java.util.List;

/**
 * Hdmi adapter for InputChangeListener
 */

public class InputChangeAdapter {
    private static final String TAG = "InputChangeAdapter";

    private static final String ACTION_OTP_INPUT_SOURCE_CHANGE = "droidlogic.tv.action.OTP_INPUT_SOURCE_CHANGED";
    // For test use. Please reboot if you want to change this property.
    private static final String PROP_OTP_INPUT_CHANGE = "persist.vendor.tv.otp.inputchange";

    private volatile static InputChangeAdapter sIntance;

    private Intent mBootOtp;
    private Context mContext;
    private boolean mBootComplete = true;

    private InputChangeAdapter() {}

    private InputChangeAdapter(Context context) {
        mContext = context;
        if (SystemProperties.getBoolean(PROP_OTP_INPUT_CHANGE, true)) {
            registerInputChangeListener(context);
        }
    }

    public void sendBootOtpIntent() {
        mBootComplete = true;
        if (mBootOtp != null && mContext != null) {
            Log.d(TAG, "send boot otp intent");
            mContext.sendBroadcast(mBootOtp);
        }
    }

    private void registerInputChangeListener(Context context) {
        HdmiControlManager hdmiControlManager = (HdmiControlManager) context.getSystemService(Context.HDMI_CONTROL_SERVICE);
        if (null == hdmiControlManager) {
            Log.e(TAG, "failed to get HdmiControlManager");
            return;
        }

        HdmiTvClient tvClient = hdmiControlManager.getTvClient();
        if (null == tvClient) {
            Log.e(TAG, "failed to get HdmiTvClient");
            return;
        }

        tvClient.setInputChangeListener(new HdmiTvClient.InputChangeListener() {
            @Override
            public void onChanged(HdmiDeviceInfo info) {
                Log.d(TAG, "onChanged: " + info);

                TvInputManager manager = (TvInputManager) context.getSystemService(Context.TV_INPUT_SERVICE);
                if (null == manager) {
                    Log.e(TAG, "TvInputManager null!");
                    return;
                }

                List<TvInputInfo> tvInputList = manager.getTvInputList();
                String inputId = "";
                 String parentInputId = "";
                for (TvInputInfo tvInputInfo : tvInputList) {
                    HdmiDeviceInfo hdmiInfo = tvInputInfo.getHdmiDeviceInfo();
                    if (hdmiInfo != null && hdmiInfo.getLogicalAddress() == info.getLogicalAddress()) {
                        inputId = tvInputInfo.getId();
                        parentInputId = tvInputInfo.getParentId();
                        break;
                    }
                }

                if (TextUtils.isEmpty(inputId)) {
                    Log.d(TAG, "no input id found for " + info);
                    return;
                }

                String currentSelectInput = DroidLogicHdmiCecManager.getInstance(context).getCurrentInput();

                Log.d(TAG, "input id:" + inputId + " parent:" + parentInputId + " current:" + currentSelectInput);
                if (currentSelectInput.equals(inputId)
                    || currentSelectInput.equals(parentInputId)) {
                    Log.d(TAG, "same input id no need to broadcast");
                    return;
                }

                Intent intent = new Intent(ACTION_OTP_INPUT_SOURCE_CHANGE);
                intent.putExtra(TvInputInfo.EXTRA_INPUT_ID, inputId);

                if (!mBootComplete) {
                    Log.d(TAG, "One Touch Play event, but not boot yet");
                    mBootOtp = intent;
                } else {
                    Log.d(TAG, "One Touch Play event, send broadcast intent " + intent);
                    context.sendBroadcast(intent);
                }
            }
        });
    }

    public static InputChangeAdapter getInstance(Context context) {
        if (null == sIntance) {
            synchronized(InputChangeAdapter.class) {
                if (null == sIntance) {
                    sIntance = new InputChangeAdapter(context);
                }
            }
        }
        return sIntance;
    }
}
