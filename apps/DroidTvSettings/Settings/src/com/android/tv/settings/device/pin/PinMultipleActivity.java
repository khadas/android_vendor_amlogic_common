package com.android.tv.settings.device.pin;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.Switch;
import android.widget.TextView;
import com.android.tv.settings.R;
import com.droidlogic.app.SystemControlManager;
import android.view.Window;
import android.view.WindowManager;
import java.io.IOException;
import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Timer;
import java.util.TimerTask;
import android.text.TextUtils;

public class PinMultipleActivity extends Activity implements CompoundButton.OnCheckedChangeListener {

    private static final String TAG = "PinMultipleActivity";

    private Switch swPin15V, swPin25V;
    private Switch swPin3USBDM, swPin4USBDP;
    private Switch swPin5GND;
    private Switch swPin6VCCMCU;
    private Switch swPin7MCUSWCLK, swPin8MCUSWDIO;
    private Switch swPin9GND;
    private Switch swPin10ADCCH6;
    private Switch swPin111V8;
    private Switch swPin12ADCCH3;
    private Switch swPin13SPDIFOUT;
    private Switch swPin14GND;
    private Switch swPin15UARTRX, swPin16UARTTX;
    private Switch swPin17GND;
    private Switch swPin18LINUXRX, swPin19LINUXTX;
    private Switch swPin203V3;

    private Switch swPin21GND;
    private Switch swPin22I2CCMFSCL, swPin23I2CCMFSDL;
    private Switch swPin24GND;
    private Switch swPin25I2CCMASCL, swPin26I2CCMASDL;
    private Switch swPin273V3;
    private Switch swPin28GND;
    private Switch swPin29I2SSCLK1, swPin30I2SMCLK1, swPin31I2SSD01, swPin32I2SLRCLK1, swPin33I2SSDI1;

    private Switch swPin34GND;

    private Switch swPin35PWMF;
    private Switch swPin36GPIOT18, swPin37GPIOT19, swPin38PWR, swPin39IRIN;

    private Switch swPin40GND;

    private SystemControlManager mSystemControlManager;

    private static final String UARTC = "ubootenv.var.khadas_multiple_uartc_id";

    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mSystemControlManager = SystemControlManager.getInstance();

        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                WindowManager.LayoutParams.FLAG_FULLSCREEN);

        setContentView(R.layout.activity_pin_multiple);

        swPin15V = (Switch) findViewById(R.id.sw_pin1_5V);
        swPin15V.setChecked(true);
        swPin15V.setEnabled(false);
        swPin25V = (Switch) findViewById(R.id.sw_pin2_5V);
        swPin25V.setChecked(true);
        swPin25V.setEnabled(false);
        swPin3USBDM = (Switch)findViewById(R.id.sw_pin3_USB_DM);
        swPin3USBDM.setOnCheckedChangeListener(this);
        swPin3USBDM.setChecked(true);
        swPin3USBDM.setEnabled(false);
        swPin4USBDP = (Switch)findViewById(R.id.sw_pin4_USB_DP);
        swPin4USBDP.setOnCheckedChangeListener(this);
        swPin4USBDP.setChecked(true);
        swPin4USBDP.setEnabled(false);
        swPin5GND = (Switch) findViewById(R.id.sw_pin5_GND);
        swPin5GND.setChecked(true);
        swPin5GND.setEnabled(false);
        swPin6VCCMCU = (Switch) findViewById(R.id.sw_pin6_VCC_MCU);
        swPin6VCCMCU.setChecked(true);
        swPin6VCCMCU.setEnabled(false);
        swPin7MCUSWCLK = (Switch) findViewById(R.id.sw_pin7_MCU_SWCLK);
        swPin7MCUSWCLK.setChecked(true);
        swPin7MCUSWCLK.setEnabled(false);
        swPin8MCUSWDIO = (Switch) findViewById(R.id.sw_pin8_MCU_SWDIO);
        swPin8MCUSWDIO.setChecked(true);
        swPin8MCUSWDIO.setEnabled(false);
        swPin9GND = (Switch)findViewById(R.id.sw_pin9_GND);
        swPin9GND.setChecked(true);
        swPin9GND.setEnabled(false);
        swPin10ADCCH6 = (Switch)findViewById(R.id.sw_pin10_ADC_CH6);
        swPin10ADCCH6.setOnCheckedChangeListener(this);
        swPin10ADCCH6.setChecked(true);
        swPin10ADCCH6.setEnabled(false);
        swPin111V8 = (Switch) findViewById(R.id.sw_pin11_1V8);
        swPin111V8.setChecked(true);
        swPin111V8.setEnabled(false);
        swPin12ADCCH3 = (Switch)findViewById(R.id.sw_pin12_ADC_CH3);
        swPin12ADCCH3.setOnCheckedChangeListener(this);
        swPin12ADCCH3.setChecked(true);
        swPin12ADCCH3.setEnabled(false);
        swPin13SPDIFOUT = (Switch)findViewById(R.id.sw_pin13_SPDIFOUT);
        swPin13SPDIFOUT.setOnCheckedChangeListener(this);
        swPin13SPDIFOUT.setChecked(true);
        swPin13SPDIFOUT.setEnabled(false);
        swPin14GND = (Switch) findViewById(R.id.sw_pin14_GND);
        swPin14GND.setChecked(true);
        swPin14GND.setEnabled(false);
        swPin15UARTRX = (Switch)findViewById(R.id.sw_pin15_UART_EN_RX);
        swPin15UARTRX.setOnCheckedChangeListener(this);
 //       swPin15UARTRX.setChecked(true);
 //       swPin15UARTRX.setEnabled(false);
        swPin16UARTTX = (Switch)findViewById(R.id.sw_pin16_UART_EN_TX);
        swPin16UARTTX.setOnCheckedChangeListener(this);
//        swPin16UARTTX.setChecked(true);
//        swPin16UARTTX.setEnabled(false);
        swPin17GND = (Switch) findViewById(R.id.sw_pin17_GND);
        swPin17GND.setChecked(true);
        swPin17GND.setEnabled(false);
        swPin18LINUXRX = (Switch)findViewById(R.id.sw_pin18_LINUX_RX);
        swPin18LINUXRX.setOnCheckedChangeListener(this);
        swPin18LINUXRX.setChecked(true);
        swPin18LINUXRX.setEnabled(false);
        swPin19LINUXTX = (Switch)findViewById(R.id.sw_pin19_LINUX_TX);
        swPin19LINUXTX.setOnCheckedChangeListener(this);
        swPin19LINUXTX.setChecked(true);
        swPin19LINUXTX.setEnabled(false);
        swPin203V3 = (Switch) findViewById(R.id.sw_pin20_3V3);
        swPin203V3.setChecked(true);
        swPin203V3.setEnabled(false);

        swPin21GND = (Switch) findViewById(R.id.sw_pin21_GND);
        swPin21GND.setChecked(true);
        swPin21GND.setEnabled(false);
        swPin22I2CCMFSCL = (Switch)findViewById(R.id.sw_pin22_I2CM_F_SCL);
        swPin22I2CCMFSCL.setOnCheckedChangeListener(this);
        swPin22I2CCMFSCL.setChecked(true);
        swPin22I2CCMFSCL.setEnabled(false);
        swPin23I2CCMFSDL = (Switch)findViewById(R.id.sw_pin23_I2CM_F_SDA);
        swPin23I2CCMFSDL.setOnCheckedChangeListener(this);
        swPin23I2CCMFSDL.setChecked(true);
        swPin23I2CCMFSDL.setEnabled(false);
        swPin24GND = (Switch) findViewById(R.id.sw_pin24_GND);
        swPin24GND.setChecked(true);
        swPin24GND.setEnabled(false);
        swPin25I2CCMASCL = (Switch)findViewById(R.id.sw_pin25_I2CM_A_SCL);
        swPin25I2CCMASCL.setOnCheckedChangeListener(this);
        swPin25I2CCMASCL.setChecked(true);
        swPin25I2CCMASCL.setEnabled(false);
        swPin26I2CCMASDL = (Switch)findViewById(R.id.sw_pin26_I2CM_A_SDA);
        swPin26I2CCMASDL.setOnCheckedChangeListener(this);
        swPin26I2CCMASDL.setChecked(true);
        swPin26I2CCMASDL.setEnabled(false);
        swPin273V3 = (Switch) findViewById(R.id.sw_pin27_3V3);
        swPin273V3.setChecked(true);
        swPin273V3.setEnabled(false);
        swPin28GND = (Switch) findViewById(R.id.sw_pin28_GND);
        swPin28GND.setChecked(true);
        swPin28GND.setEnabled(false);
        swPin29I2SSCLK1 = (Switch)findViewById(R.id.sw_pin29_I2S_SCLK1);
        swPin29I2SSCLK1.setOnCheckedChangeListener(this);
        swPin29I2SSCLK1.setChecked(true);
        swPin29I2SSCLK1.setEnabled(false);
        swPin30I2SMCLK1 = (Switch)findViewById(R.id.sw_pin30_I2S_MCLK1);
        swPin30I2SMCLK1.setOnCheckedChangeListener(this);
        swPin30I2SMCLK1.setChecked(true);
        swPin30I2SMCLK1.setEnabled(false);
        swPin31I2SSD01 = (Switch)findViewById(R.id.sw_pin31_I2S_SDO1);
        swPin31I2SSD01.setOnCheckedChangeListener(this);
        swPin31I2SSD01.setChecked(true);
        swPin31I2SSD01.setEnabled(false);
        swPin32I2SLRCLK1 = (Switch)findViewById(R.id.sw_pin32_I2S_LRCLK1);
        swPin32I2SLRCLK1.setOnCheckedChangeListener(this);
        swPin32I2SLRCLK1.setChecked(true);
        swPin32I2SLRCLK1.setEnabled(false);
        swPin33I2SSDI1 = (Switch)findViewById(R.id.sw_pin33_I2S_SDI1);
        swPin33I2SSDI1.setOnCheckedChangeListener(this);
        swPin33I2SSDI1.setChecked(true);
        swPin33I2SSDI1.setEnabled(false);
        swPin34GND = (Switch) findViewById(R.id.sw_pin34_GND);
        swPin34GND.setChecked(true);
        swPin34GND.setEnabled(false);
        swPin35PWMF = (Switch)findViewById(R.id.sw_pin35_PWM_F);
        swPin35PWMF.setOnCheckedChangeListener(this);
        swPin35PWMF.setChecked(true);
        swPin35PWMF.setEnabled(false);
        swPin36GPIOT18 = (Switch)findViewById(R.id.sw_pin36_GPIOT_18);
        swPin36GPIOT18.setOnCheckedChangeListener(this);
        swPin36GPIOT18.setChecked(true);
        swPin36GPIOT18.setEnabled(false);
        swPin37GPIOT19 = (Switch)findViewById(R.id.sw_pin37_GPIOT_19);
        swPin37GPIOT19.setOnCheckedChangeListener(this);
        swPin37GPIOT19.setChecked(true);
        swPin37GPIOT19.setEnabled(false);
        swPin38PWR = (Switch)findViewById(R.id.sw_pin38_PWR_HOLD_EXT);
        swPin38PWR.setOnCheckedChangeListener(this);
        swPin38PWR.setChecked(true);
        swPin38PWR.setEnabled(false);
        swPin39IRIN = (Switch)findViewById(R.id.sw_pin39_GPIOD_15_IR_IN);
        swPin39IRIN.setOnCheckedChangeListener(this);
        swPin39IRIN.setChecked(true);
        swPin39IRIN.setEnabled(false);
        swPin40GND = (Switch) findViewById(R.id.sw_pin40_GND);
        swPin40GND.setChecked(true);
        swPin40GND.setEnabled(false);

        initAllSwitch();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

    private void initAllSwitch() {
        swPin15UARTRX.setChecked(getUartCStatus());
        swPin16UARTTX.setChecked(getUartCStatus());
    }

    private boolean getUartCStatus() {
        String status = mSystemControlManager.getBootenv(UARTC, String.valueOf(1));
        Log.d(TAG, "getUartCStatus " + status);
        if (!TextUtils.isEmpty(status) && status.equals("0")) {
            return false;
        }
        return true;
    }

    private void setUartCStatus(boolean status) {
        Log.d(TAG, "setUartCStatus " + status);

        if(getUartCStatus() == status) {
            return;
        }

        if (status) {
            mSystemControlManager.setBootenv(UARTC, String.valueOf(1));
        } else {
            mSystemControlManager.setBootenv(UARTC, String.valueOf(0));
        }

        swPin15UARTRX.setChecked(status);
        swPin16UARTTX.setChecked(status);
    }

    @Override
    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
        int id = buttonView.getId();
        Log.e(TAG, "id " + id + " isChecked " + isChecked);
        if (id == R.id.sw_pin15_UART_EN_RX ||
                   id == R.id.sw_pin16_UART_EN_TX) {
            setUartCStatus(isChecked);
        }
    }
}