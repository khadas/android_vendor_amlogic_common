package com.droidlogic.updater.service;

import android.app.AlarmManager;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.PendingIntent;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.Message;
import android.os.SystemClock;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.util.Log;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;
import com.droidlogic.updater.R;
import com.droidlogic.updater.UpdateApplication;
import com.droidlogic.updater.UpdateConfig;
import com.droidlogic.updater.UpdateManager;
import com.droidlogic.updater.UpdaterState;
import com.droidlogic.updater.ui.RemoteView;
import com.droidlogic.updater.ui.MainActivity;
import com.droidlogic.updater.ui.FragmentAlertDialog;
import com.droidlogic.updater.util.PermissionUtils;
import com.droidlogic.updater.util.UpdateEngineErrorCodes;
import com.droidlogic.updater.util.UpdateEngineStatuses;
import java.util.Calendar;
import org.json.JSONObject;

public class AutoCheckService extends Service {
    private static final int UPDATE = 3;
    public static final int SHOWING = 1;
    public static final int CHECK = 2;
    public static final int RUNNING = 3;
    public static final int HIDE = 4;
    public static final int FRUSH = 5;
    private static final long HALFHOUR = 1000*60*30;
    public static final String SHOWING_ACTION = "com.droidlogic.updater.autocheck";
    public static final String HIDE_UI = "com.droidlogic.updater.hideui";
    public static final String SHOW_UI = "com.droidlogic.updater.showui";
    private static final String TAG = "AutoCheckService";
    private Handler mWorkHandler;
    private Handler mUIHandler = new UIHandler();
    private AlarmManager aManager;
    private UpdateConfig mCurrentConfig;
    private PendingIntent pIntent;
    private static RemoteView mRemoteView;
    private UpdateManager mUpdateManager;

    public AutoCheckService() {
    }

    @Override
    public void onCreate() {
        super.onCreate();
        Log.d(TAG,"oNnCreate");
        mRemoteView = RemoteView.getInstance();
        mRemoteView.createView(AutoCheckService.this);
        IntentFilter intentfilter = new IntentFilter(SHOWING_ACTION);
        intentfilter.addAction(HIDE_UI);
        intentfilter.addAction(SHOW_UI);
        registerReceiver(actionReceiver, intentfilter);
        mUpdateManager = ((UpdateApplication)getApplication()).getUpdateManager();
    }
    private BroadcastReceiver actionReceiver =new BroadcastReceiver(){

        @Override
        public void onReceive(Context context, Intent intent) {
            if (intent.getAction().equals(SHOWING_ACTION)) {
                mUIHandler.sendEmptyMessage(CHECK);
            }else if (intent.getAction().equals(HIDE_UI)) {
                hideUI();
            }else{
                mUpdateManager.setOnStateChangeCallback(AutoCheckService.this::onUpdaterStateChange);
                mUpdateManager.setOnEngineCompleteCallback(AutoCheckService.this::onEnginePayloadApplicationComplete);
                mUpdateManager.setOnProgressUpdateCallback(AutoCheckService.this::onProgressUpdate);
                showUI();
            }
        }
    };

    class UIHandler extends Handler {
        @Override
        public void dispatchMessage( Message msg) {
           switch (msg.what) {
               case CHECK:
               if (!MainActivity.mShowing) {
                    update();
               }
               break;
               case SHOWING:
               if (!MainActivity.mShowing) {
                    showDialog();
               }
               break;
               case HIDE:
               hideUI();
               break;
               case FRUSH:
               updateInfo((String)msg.obj,true,msg.arg1);
               break;
           }
        }
    };

    /**
     * Invoked when SystemUpdaterSample app state changes.
     * Value of {@code state} will be one of the
     * values from {@link UpdaterState}.
     */
    private void onUpdaterStateChange(int state) {
        Log.d(TAG,"onUpdaterStateChange"+state);
        if (state == UpdaterState.SLOT_SWITCH_REQUIRED) {
            mUpdateManager.setSwitchSlotOnReboot();
        }
    }

    /**
     * Invoked when the payload has been applied, whether successfully or
     * unsuccessfully. The value of {@code errorCode} will be one of the
     * values from {@link UpdateEngine.ErrorCodeConstants}.
     */
    private void onEnginePayloadApplicationComplete(int errorCode) {
        Log.d(TAG,"onEnginePayloadApplicationComplete"+errorCode);
        mUIHandler.post(() ->{
            boolean updateSuccess = UpdateEngineErrorCodes.isUpdateSucceeded(errorCode);
            final String completionState = updateSuccess? getString(R.string.update_success):getString(R.string.update_fail);
            Message msg = mUIHandler.obtainMessage();
            msg.what = FRUSH;
            msg.arg1 = 1;
            msg.obj = completionState;
            mUIHandler.sendMessage(msg);
        });
    }
    /**
     * Invoked when update progress changes.
     */
    private void onProgressUpdate(double progress) {
        int val = (int)(100*progress);
        if (val == 0) {
            return;
        }
        Message msg = mUIHandler.obtainMessage();
        msg.what = FRUSH;
        msg.arg1 = val;
        msg.obj = getString(R.string.running)+(((int)(val*100))/100.0f)+"%";
        mUIHandler.sendMessage(msg);
    }

    private void update(){
        ((UpdateApplication)getApplication()).mRunningUpgrade = true;
        showUI();
        Log.d(TAG,"update");
        mWorkHandler.post(()->{
            try {
                mUpdateManager.cancelRunningUpdate();
                mUpdateManager.resetUpdate();
            }catch (Exception ex) {}
            try {
                mUpdateManager.setOnStateChangeCallback(this::onUpdaterStateChange);
                mUpdateManager.setOnEngineCompleteCallback(this::onEnginePayloadApplicationComplete);
                mUpdateManager.setOnProgressUpdateCallback(this::onProgressUpdate);
                mUpdateManager.applyUpdate(this, mCurrentConfig);
            }catch( Exception ex){
                ex.printStackTrace();
            }
        });
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        aManager = (AlarmManager)getSystemService(ALARM_SERVICE);
        mWorkHandler = ((UpdateApplication)this.getApplication()).getWorkHandler();
        mWorkHandler.postDelayed(checkRunner, 30000);
        return START_NOT_STICKY;
    }

    private CheckRunnable checkRunner = new CheckRunnable();
    class CheckRunnable implements Runnable{
        public void run() {
            if (checkLowerNetwork(AutoCheckService.this)) {
                startCheck();
                PrepareUpdateService.startCheckup(AutoCheckService.this,mWorkHandler,mCallback);
            }else {
                mWorkHandler.postDelayed(checkRunner, 30000);
            }
        }
    }

    @Override
    public void onDestroy() {
        Log.d(TAG,"onDestroy");
        unregisterReceiver(actionReceiver);
        mUpdateManager.setOnStateChangeCallback(null);
        mUpdateManager.setOnEngineCompleteCallback(null);
        mUpdateManager.setOnProgressUpdateCallback(null);
        ((UpdateApplication)getApplication()).mRunningUpgrade = false;
        super.onDestroy();
    }

    private boolean checkForceUpdate(String json) {
        boolean checkForce = false;
        try {
            JSONObject o = new JSONObject(json);
            checkForce = o.getBoolean("is_force_upgrade");
            Log.d(TAG,"checkForceUpdate"+checkForce);
        }catch(Exception ex){
            ex.printStackTrace();
        }finally {
            return checkForce;
        }
    }

    private PrepareUpdateService.CheckupResultCallback mCallback = (int status, UpdateConfig config) -> {
        if (PermissionUtils.CanDebug()) Log.d(TAG,"checkcall back status:"+status+"@"+AutoCheckService.this);
        if (status == UPDATE) {
            stopCheck();
            mCurrentConfig = config;
            Message msg = new Message();
            msg.obj = config.getName();
            UpdateApplication.mRunningForce = checkForceUpdate(config.getRawJson())?true:false;
            if (UpdateApplication.mRunningForce) {
                msg.what = CHECK;
                mUIHandler.sendMessage(msg);
            }/*else {
                msg.what = SHOWING;
                mUIHandler.sendMessage(msg);
            }*/
        }else {
            UpdateApplication.mRunningForce = false;
        }

    };

    protected void showDialog() {
        if (MainActivity.mShowing) {
            return;
        }
        Intent intent=new Intent(this,FragmentAlertDialog.class);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        startActivity(intent);
    }

    private void startCheck() {
       /* PrepareUpdateService.CallbackCheckupReceiverWrapper wrapper = new PrepareUpdateService.CallbackCheckupReceiverWrapper(mWorkHandler,mCallback);
        long triggerAtTime=SystemClock.elapsedRealtime()+3000;//+HALFHOUR/10;
        Intent i=new Intent(this, PrepareUpdateService.class);
        i.setAction(PrepareUpdateService.ACTION_CHECK_UPDATE);
        i.putExtra("key",33);
        i.putExtra(PrepareUpdateService.EXTRA_PARAM_RESULT_RECEIVER,wrapper);Log.d(TAG,"startCheck"+mWorkHandler+"/."+wrapper+"---"+i.getIntExtra("key",0));
        pIntent = PendingIntent.getService(this, 0, i, PendingIntent.FLAG_UPDATE_CURRENT);*/
        long triggerAtTime=SystemClock.elapsedRealtime()+HALFHOUR;
        Intent i=new Intent(this, AutoCheckService.class);
        pIntent = PendingIntent.getService(this, 0, i, PendingIntent.FLAG_UPDATE_CURRENT);
        aManager.setRepeating(AlarmManager.ELAPSED_REALTIME_WAKEUP,triggerAtTime,HALFHOUR,pIntent);
    }

    private void stopCheck() {
        aManager.cancel(pIntent);
    }

    public void showUI() {
        mRemoteView.show();
    }
    private static boolean checkLowerNetwork(Context cxt) {
        ConnectivityManager connMgr = (ConnectivityManager)cxt.getSystemService(Context.CONNECTIVITY_SERVICE);
        Network net = connMgr.getActiveNetwork();
        NetworkCapabilities capabilities = connMgr.getNetworkCapabilities(net);
        return capabilities != null && capabilities.hasCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET);
    }

    public void hideUI() {
        Log.d(TAG,"hideUI");
        mRemoteView.hide();
    }


    public void updateInfo(String str, boolean visible, int progress) {
        mRemoteView.updateUI(str, visible, progress);
        if (progress == 100) {
            mUIHandler.sendEmptyMessageDelayed(HIDE,6000);
        }
    }

    @Override
    public IBinder onBind(Intent intent) {
        // TODO: Return the communication channel to the service.
        throw new UnsupportedOperationException("Not yet implemented");
    }
}
