package com.khadas.ksettings;


import android.app.Activity;
import android.content.Context;
import android.hardware.Sensor;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.hardware.SensorEvent;
import android.os.Bundle;
import android.view.MenuItem;
import android.widget.TextView;


public class Gsensor_Preference extends Activity implements SensorEventListener {

    private SensorManager mSensorManager;
    private TextView mTextView_x;
    private TextView mTextView_y;
    private TextView mTextView_z;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.gsensor);
        getActionBar().setHomeButtonEnabled(true);
        getActionBar().setDisplayHomeAsUpEnabled(true);

        //获取Sensor服务
        mSensorManager = (SensorManager) getSystemService(Context.SENSOR_SERVICE);
        //获取gsensor的对象
        Sensor gsensor = mSensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
        //注册数据监听器，当有数据时会回调onSensorChanged方法
        mSensorManager.registerListener(this, gsensor, SensorManager.SENSOR_DELAY_NORMAL);

        mTextView_x = findViewById(R.id.textView_x);
        mTextView_y = findViewById(R.id.textView_y);
        mTextView_z = findViewById(R.id.textView_z);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        mSensorManager.unregisterListener(this);
    }

    @Override
    public void onSensorChanged(SensorEvent event) {
        if(event.sensor == null)
            return ;
        //判断获取的数据类型是不是gsensor
        if(event.sensor.getType() == Sensor.TYPE_ACCELEROMETER){
            //获得数据为float类型的数据
            mTextView_x.setText("X: " + event.values[0] + " m/s²");
            mTextView_y.setText("Y: " + event.values[1] + " m/s²");
            mTextView_z.setText("Z: " + event.values[2] + " m/s²");
        }
    }

    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy) {

    }

    public boolean onOptionsItemSelected(MenuItem item) {
        // TODO Auto-generated method stub
        if(item.getItemId() == android.R.id.home) {
            finish();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }
}
