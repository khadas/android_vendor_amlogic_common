package com.droidlogic;

import android.app.Service;
import android.content.Context;
import android.content.BroadcastReceiver;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.wifi.WifiManager;
import android.os.IBinder;
import android.util.Log;

import com.droidlogic.app.SystemControlManager;

public class NetworkSwitchService extends Service {
    private static final String TAG = "NetworkSwitchService";

    private static final String UNNETCONDITION_LED = "/sys/class/leds/net_red/brightness";
    private static final String NETCONDITION_LED = "/sys/class/leds/net_green/brightness";
    private SystemControlManager mControlManager;

    private BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {

            mControlManager = SystemControlManager.getInstance();
            //SystemControlManager mControlManager = new SystemControlManager(context);
            String action = intent.getAction();
            Log.d(TAG, "action:" + action);

            if (ConnectivityManager.CONNECTIVITY_ACTION.equals(action)) {
                ConnectivityManager cm = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
                NetworkInfo networkInfo = cm.getActiveNetworkInfo();

                if (networkInfo != null && networkInfo.isAvailable()) {
                    if (UNNETCONDITION_LED != null && NETCONDITION_LED !=null) {
                        mControlManager.writeSysFs(UNNETCONDITION_LED, "0");
                        mControlManager.writeSysFs(NETCONDITION_LED, "0");
                    }
                } else {
                    if (UNNETCONDITION_LED != null && NETCONDITION_LED !=null) {
                        mControlManager.writeSysFs(UNNETCONDITION_LED, "1");
                        mControlManager.writeSysFs(NETCONDITION_LED, "1");
                    }
                }
            }
        }
    };

    @Override
    public void onCreate() {
        super.onCreate();
        IntentFilter filter = new IntentFilter();
        filter.addAction("android.net.conn.CONNECTIVITY_CHANGE");
        registerReceiver(mReceiver, filter);
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        return super.onStartCommand(intent, flags, startId);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        unregisterReceiver(mReceiver);
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }
}
