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

public class LogoLEDBarPreference extends DialogPreference implements OnSeekBarChangeListener{

    private SeekBar seekBar;
    private TextView textView;

    private int value;


    private final int MSG_WHAT_SET_BACKLIGHT  = 0;

    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_WHAT_SET_BACKLIGHT:
                    SystemProperties.set("persist.sys.logoled.brightness", ""+msg.arg1);
                    break;
                default:
                    break;
            }
        }
    };

    public LogoLEDBarPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        // TODO Auto-generated constructor stub
    }

    @Override
    protected void onBindDialogView(View view) {
        // TODO Auto-generated method stub
        super.onBindDialogView(view);
        seekBar = (SeekBar) view.findViewById(R.id.seekBar1);
        seekBar.setMin(0);
        seekBar.setMax(31);
        textView = (TextView) view.findViewById(R.id.textView1);
        seekBar.setOnSeekBarChangeListener(this);

        value = SystemProperties.getInt("persist.sys.logoled.brightness", 15);
        textView.setText("" + value);
        seekBar.setProgress(value);
    }

    @Override
    protected void onDialogClosed(boolean positiveResult) {
        // TODO Auto-generated method stub
        if (positiveResult) {
            Log.i("Dialog closed", "You click positive button");
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
