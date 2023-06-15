package com.khadas.ksettings;

import android.content.Context;
import android.os.Handler;
import android.os.Message;
import android.os.SystemProperties;
import android.preference.DialogPreference;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;

import java.io.IOException;

public class VboSeekBarPreference extends DialogPreference implements OnSeekBarChangeListener{

    private SeekBar seekBar;
    private TextView textView;

    private String value;

    private final int MSG_WHAT_SET_BACKLIGHT  = 0;

    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_WHAT_SET_BACKLIGHT:
                    //mipi -- /sys/class/backlight/aml-bl/brightness
                    //vbo -- /sys/class/backlight/aml-bl1/brightness
                    try {
                        ComApi.execCommand(new String[]{"sh", "-c", "echo " + msg.arg1 +  " > /sys/class/backlight/aml-bl1/brightness"});
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                    break;
                default:
                    break;
            }
        }
    };

    public VboSeekBarPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        // TODO Auto-generated constructor stub
    }

    @Override
    protected void onBindDialogView(View view) {
        // TODO Auto-generated method stub
        super.onBindDialogView(view);
        seekBar = (SeekBar) view.findViewById(R.id.seekBar1);
        textView = (TextView) view.findViewById(R.id.textView1);
        seekBar.setOnSeekBarChangeListener(this);

        //mipi -- /sys/class/backlight/aml-bl/brightness
        //vbo -- /sys/class/backlight/aml-bl1/brightness
        try {
            value = ComApi.execCommand(new String[]{"sh", "-c", "cat /sys/class/backlight/aml-bl1/brightness"});
            //Log.d("wjh","Vbo=" + value);
            if(value.equals("") || value.contains("No such file or directory")){
                textView.setText("No VBO Panel");
            }else {
                textView.setText(value);
                if(Integer.parseInt(value) >= 0 && Integer.parseInt(value) <= 255) {
                    seekBar.setProgress(Integer.parseInt(value));
                }
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    @Override
    protected void onDialogClosed(boolean positiveResult) {
        // TODO Auto-generated method stub
        if (positiveResult) {
            Log.i("Dialog closed", "You click positive button");
            try {
                value = ComApi.execCommand(new String[]{"sh", "-c", "cat /sys/class/backlight/aml-bl1/brightness"});
                Log.d("hlm1","Vbo=" + value);
                SystemProperties.set("persist.sys.vbo_bl_value", value);
            } catch (IOException e) {
                e.printStackTrace();
            }
        } else {
            Log.i("Dialog closed", "You click negative button");
        }
    }

    @Override
    public void onProgressChanged(SeekBar seekBar, int progress,
                                  boolean fromUser) {
        textView.setText("" + progress);
        mHandler.removeMessages(MSG_WHAT_SET_BACKLIGHT);
        Message msg = new Message();
        msg.what = MSG_WHAT_SET_BACKLIGHT;
        msg.arg1 = progress;
        mHandler.sendMessageDelayed(msg,100);
    }

    @Override
    public void onStartTrackingTouch(SeekBar seekBar) {
        // TODO Auto-generated method stub

    }

    @Override
    public void onStopTrackingTouch(SeekBar seekBar) {
        // TODO Auto-generated method stub

    }

}
