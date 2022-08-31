/******************************************************************
 *
 *Copyright (C)2012 Amlogic, Inc.
 *
 *Licensed under the Apache License, Version 2.0 (the "License");
 *you may not use this file except in compliance with the License.
 *You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing, software
 *distributed under the License is distributed on an "AS IS" BASIS,
 *WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *See the License for the specific language governing permissions and
 *limitations under the License.
 ******************************************************************/
package com.droidlogic.updater.ui;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.os.UpdateEngine;
import android.os.PowerManager;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;
import androidx.annotation.NonNull;

import com.droidlogic.updater.R;
import com.droidlogic.updater.UpdateApplication;
import com.droidlogic.updater.UpdateConfig;
import com.droidlogic.updater.UpdateManager;
import com.droidlogic.updater.UpdaterState;
import com.droidlogic.updater.service.PrepareUpdateService;
import com.droidlogic.updater.service.AutoCheckService;
import com.droidlogic.updater.util.UpdateEngineErrorCodes;
import com.droidlogic.updater.util.UpdateEngineStatuses;
import com.droidlogic.updater.util.PrefUtils;
import com.droidlogic.updater.util.MD5;
import com.droidlogic.updater.util.PermissionUtils;
import java.io.IOException;
import java.io.File;

public class MainActivity extends Activity implements View.OnClickListener, PrefUtils.CallbackChecker{
    private static final String TAG = "ABUpdate";
    private static final int queryReturnOk = 0;
    private static final int queryUpdateFile = 1;
    private Button mBtnLocal;
    private Button updateBtn;
    private TextView mLocalPath;
    private TextView mFullPath;
    private Button mBtnUpdate;
    private Button mBtnReboot;
    private UpdateConfig currentConfig;
    private ViewGroup mUpdatePro;
    private TextView mRunningStatus;
    private ProgressBar mProgressBar;
    private ViewGroup mvOnline;
    private ViewGroup mvLocal;
    private Handler mHandler = new UIHandler();
    private HandlerThread mWorkerThread = new HandlerThread("worker");
    private Handler mWorkerHandler;
    private PrefUtils mPref;
    private static final int IDLE = 0;
    private static final int CHECKING = 1;
    private static final int UPDATE = 3;
    private static final int NEWEST = 2;
    private static final int MSG_ERR  = 5;
    private static final int MSG_SUCC = 6;
    private static final int MSG_INIT = 7;
    private static final int MSG_WAIT_FOR_MERGE = 8;
    private static final int REBOOT_WAIT = 5000;
    private static final int RESET_TIME = 10000;
    private static int mStatus = IDLE;
    private static int mProgress = 0;
    private static boolean quickStopCopy;
    private static boolean rebootRequest;
    private String filePath;
    private UpdateManager mUpdateManager;
    public static boolean mShowing;

    private PrepareUpdateService.CheckupResultCallback mCallback = (int status, UpdateConfig config) -> {
        if (PermissionUtils.CanDebug()) Log.d(TAG,"checkcall back status:"+status);
        if (status == UPDATE) {
            Message msg = new Message();
            msg.obj = config.getName();
            msg.what = UPDATE;
            currentConfig = config;
            mHandler.sendMessage(msg);
        } else {
            mHandler.sendEmptyMessage(status);
        }

    };
    public boolean checkrunning() {
        return quickStopCopy;
    }
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        mUpdateManager = ((UpdateApplication)getApplication()).getUpdateManager();
        mBtnLocal = findViewById(R.id.update_local);
        updateBtn = findViewById(R.id.btn_online);
        mLocalPath = findViewById(R.id.logcal_zip_path);
        mFullPath = findViewById(R.id.file_path);
        mBtnUpdate = findViewById(R.id.update_cmd);
        mProgressBar = findViewById(R.id.progressBar);
        mRunningStatus = findViewById(R.id.running_status);
        mvOnline = findViewById(R.id.layer_online);
        mvLocal = findViewById(R.id.layer_local);
        mBtnReboot = findViewById(R.id.rebootbtn);
        mBtnLocal.setOnClickListener(this);
        mBtnUpdate.setOnClickListener(this);
        mBtnReboot.setOnClickListener(this);
        mWorkerThread.start();
        Log.d("Update","onCreate");
        mWorkerHandler = new Handler(mWorkerThread.getLooper());
        mPref = new PrefUtils(this);
        rebootRequest = false;
    }
    private Runnable cfgRunning = new cfgRunnable();
    class cfgRunnable implements Runnable {
        public void run() {
            try {
                Log.d("Update","start cp");
                try {
                    mUpdateManager.cancelRunningUpdate();
                    mUpdateManager.resetUpdate();
                }catch(Exception ex) {

                }
                quickStopCopy = false;
                filePath = mFullPath.getText().toString();
                runOnUiThread(()->{
                    mRunningStatus.setText(R.string.prepare);
                    mProgressBar.setIndeterminate(true);
                    mProgressBar.setVisibility(View.VISIBLE);
                });
                PrefUtils.copyFile(filePath,UpdateConfig.TARTEPATH,MainActivity.this);
                if (quickStopCopy) {
                    Log.d("Update","interrup by lastfilepath");
                    return;
                }else {
                   if (!MD5.checkMd5Files(new File(filePath),new File(UpdateConfig.TARTEPATH))) {
                       Log.d("Update","File copy failed");
                       mHandler.sendEmptyMessage(MSG_ERR);
                       return;
                   }
                }
                if (mStatus == UpdaterState.RUNNING || mStatus == UpdaterState.SLOT_SWITCH_REQUIRED
                || mStatus == UpdaterState.REBOOT_REQUIRED) {
                    return;
                }
                currentConfig = UpdateConfig.readDefCfg(mFullPath.getText().toString());
                Log.d("Update","stop cp");
                if (mBtnLocal.getVisibility() == View.VISIBLE &&
                            mFullPath.getText() != null
                            && filePath.equals(mFullPath.getText().toString())) {
                    applyUpdate(currentConfig);
                }
            } catch (Exception ex) {
                ex.printStackTrace();
                currentConfig = null;
                mHandler.sendEmptyMessage(MSG_ERR);
            }
        }
    };

    @Override
    protected void onResume() {
        super.onResume();
        mShowing = true;
        if (mPref.getBooleanVal(PrefUtils.key, false)) {
            if (rebootRequest) {
                //already requestBoot,disabled but still enter
                mRunningStatus.setText(getResources().getString(R.string.reboot_cmd));
                mvOnline.setVisibility(View.INVISIBLE);
                mvLocal.setVisibility(View.INVISIBLE);
                mBtnReboot.setVisibility(View.VISIBLE);
                mBtnReboot.requestFocus();
                Log.d(TAG,"rebootRequest is true");
                return;
            } else {
                mRunningStatus.setText(getResources().getString(R.string.wait_for_merge));
                mvOnline.setVisibility(View.INVISIBLE);
                mvLocal.setVisibility(View.INVISIBLE);
                mHandler.sendEmptyMessageDelayed(MSG_WAIT_FOR_MERGE,1000);
                Log.d(TAG,"rebootRequest is false");
                return;
            }
        } else {
            if (((UpdateApplication)getApplication()).mRunningUpgrade) {
                Intent i = new Intent(AutoCheckService.HIDE_UI);
                sendBroadcast(i);
            }
            initialView();
        }
    }

    private void initialView() {
        Log.d(TAG,"initialView");
        if (mvOnline != null)
            mvOnline.setVisibility(View.VISIBLE);
        if (mvLocal != null)
            mvLocal.setVisibility(View.VISIBLE);
        if (filePath != null && mFullPath.getText() == null) {
            mFullPath.setText(filePath);
            mLocalPath.setText(filePath.substring(filePath.lastIndexOf("/") + 1));
        }
        this.mUpdateManager.setOnStateChangeCallback(this::onUpdaterStateChange);
        this.mUpdateManager.setOnEngineCompleteCallback(this::onEnginePayloadApplicationComplete);
        this.mUpdateManager.setOnProgressUpdateCallback(this::onProgressUpdate);
        if (PermissionUtils.CanDebug()) Log.d(TAG,"Service getStatus"+PrepareUpdateService.getStatus());
        if (!checkLowerNetwork(MainActivity.this)) {
            updateBtn.setEnabled(false);
        }else if (PrepareUpdateService.getStatus() == IDLE && mStatus == IDLE) {
            updateBtn.setEnabled(true);
            updateBtn.setText(R.string.btn_check);
        }else if (PrepareUpdateService.getStatus() == 3 && mStatus == IDLE) {
            updateBtn.setEnabled(true);
            updateBtn.setText(R.string.btn_update);
        }
        mStatus = mUpdateManager.getUpdaterState();
        onUpdaterStateChange(mStatus);
        updateBtn.setOnClickListener(new View.OnClickListener() {

            @Override
            public void onClick(View v) {
                if (updateBtn.getText().equals(getString(R.string.btn_check))) {
                    mHandler.sendEmptyMessage(CHECKING);
                    checkFromService();
                } else if (updateBtn.getText().equals(getString(R.string.btn_update))) {
                    updateBtn.setEnabled(false);
                    mBtnUpdate.setEnabled(false);
                    mProgressBar.setIndeterminate(true);
                    mProgressBar.setVisibility(View.VISIBLE);
                    if (mvOnline != null) mvOnline.setVisibility(View.INVISIBLE);
                    if (mvLocal != null) mvLocal.setVisibility(View.INVISIBLE);
                    mRunningStatus.setText(getString(R.string.prepare));

                   ((UpdateApplication)getApplication()).getWorkHandler().post(()->{
                        try {
                            mUpdateManager.cancelRunningUpdate();
                            mUpdateManager.resetUpdate();
                        } catch (Exception ex){
                           // ex.printStackTrace();
                        }
                        applyUpdate(currentConfig);
                    });;
                }
            }
        });
    }

    private void checkFromService() {
        PrepareUpdateService.startCheckup(this, mHandler, mCallback);
    }

    @Override
    protected void onPause() {
        super.onPause();
        mShowing = false;
        this.mUpdateManager.setOnStateChangeCallback(null);
        this.mUpdateManager.setOnEngineCompleteCallback(null);
        this.mUpdateManager.setOnProgressUpdateCallback(null);
    }

    @Override
    public void onBackPressed() {
        if (mPref.getBooleanVal(PrefUtils.key, false) && rebootRequest) {
            Toast.makeText(MainActivity.this, getString(R.string.reboot_cmd), 1000).show();
        } else {
            super.onBackPressed();
        }
    }

    private void applyUpdate(UpdateConfig config) {
        try {
            mProgress = 0;
            mUpdateManager.applyUpdate(this, config);
        } catch (UpdaterState.InvalidTransitionException e) {
            if (PermissionUtils.CanDebug()) Log.e(TAG, "Failed to apply update " + config.getName(), e);
        }
    }


    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        if (data != null) {
            if ((requestCode == queryUpdateFile) &&
                    (resultCode == queryReturnOk)) {
                Bundle bundle = data.getExtras();
                String file = bundle.getString(FileSelector.FILE);
                if (file != null) {
                    mFullPath.setText(file);
                    mLocalPath.setText(file.substring(file.lastIndexOf("/") + 1
                    ));
                    mBtnUpdate.setEnabled(true);
                }
            }
        }
    }

    /**
     * @param status update engine status code
     */
    private void setUiEngineStatus(int status) {

        if (status > UpdaterState.RUNNING) {
            mProgressBar.setVisibility(View.INVISIBLE);
        }
        switch ( status ) {
            case UpdaterState.IDLE:
                break;
            case  UpdaterState.REBOOT_REQUIRED:
            case  UpdateEngine.UpdateStatusConstants.UPDATED_NEED_REBOOT:

                String rebootCmd = getResources().getString(R.string.reboot_cmd);
                if (PermissionUtils.CanDebug()) Log.d(TAG,"mRunningStatus.getText()"+mRunningStatus.getText()+" rebootcmd"+rebootCmd);
                if (!mRunningStatus.getText().equals(rebootCmd)) {
                    mRunningStatus.setText(getResources().getString(R.string.reboot_cmd_prepare));
                    mHandler.removeMessages(MSG_SUCC);
                    mHandler.sendEmptyMessageDelayed(MSG_SUCC,REBOOT_WAIT);
                }
                break;
            default:
                String statusText = UpdateEngineStatuses.getStatusText(status);
                mRunningStatus.setText(statusText + "/" + status);
        }

    }

    /**
     * @param errorCode update engine error code
     */
    private void setUiEngineErrorCode(int errorCode) {
        String errorText = UpdateEngineErrorCodes.getCodeName(errorCode);
        String errPref = getResources().getString(R.string.error_failed);
        String suffix = getResources().getString(R.string.error_suffix);
        if (errorCode == 0 || errorCode == 52/*UPDATE_BUT_NOT_ACTIVE*/) {
        }else {
            if (PermissionUtils.CanDebug()) Log.d(TAG, "setUiEngineErrorCode" + errorCode);
            mRunningStatus.setText(errPref + ":" + errorText + suffix);
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (((UpdateApplication)getApplication()).mRunningUpgrade) {
            Intent i = new Intent(AutoCheckService.SHOW_UI);
            sendBroadcast(i);
        }else {
            quickStopCopy = true;
            mWorkerHandler.removeCallbacks(cfgRunning);
            mWorkerThread.quitSafely();
            ((UpdateApplication)getApplication()).getWorkHandler().post(()->{
                try {
                    mUpdateManager.cancelRunningUpdate();
                    mUpdateManager.resetUpdate();
                } catch (Exception ex){}
            });

        }
    }

    /**
     * Invoked when update progress changes.
     */
    private void onProgressUpdate(double progress) {
        if (progress <= 1.0f) {
            runOnUiThread(() -> {
                Log.d(TAG,"mStatus"+progress+"/"+progress);
                if (mStatus == UpdaterState.RUNNING) {
                    if (mProgress == 0 && (int)(100*progress) == 1)
                        return;
                    if (((int)(100*progress)) > 1 && mProgressBar.isIndeterminate()) {
                        mProgressBar.setIndeterminate(false);
                    }
                    if (((int)(100*progress)) > mProgress) {
                        mProgress = ((int)(100*progress));
                    }
                    mProgressBar.setProgress( mProgress);
                }
            });
        }
    }

    /**
     * Invoked when the payload has been applied, whether successfully or
     * unsuccessfully. The value of {@code errorCode} will be one of the
     * values from {@link UpdateEngine.ErrorCodeConstants}.
     */
    private void onEnginePayloadApplicationComplete(int errorCode) {
        final String completionState = UpdateEngineErrorCodes.isUpdateSucceeded(errorCode)
                ? "SUCCESS"
                : "FAILURE";
        if (PermissionUtils.CanDebug()) Log.i(TAG,
                "PayloadApplicationCompleted - errorCode="
                        + UpdateEngineErrorCodes.getCodeName(errorCode) + "/" + errorCode
                        + " " + completionState);
        if (errorCode == 52) {
            return;
        }
        runOnUiThread(() -> {
            setUiEngineErrorCode(errorCode);
        });
        if (errorCode != 0) {
            mHandler.sendEmptyMessageDelayed(MSG_INIT,RESET_TIME);
        }

    }
    private void resetUI(){
         mProgressBar.setVisibility(View.INVISIBLE);
         if (mvOnline != null && mvOnline.getVisibility() == View.INVISIBLE) {
            mvOnline.setVisibility(View.VISIBLE);
         }
        if (mvLocal != null && mvLocal.getVisibility() == View.INVISIBLE) {
            mvLocal.setVisibility(View.VISIBLE);
        }
        if (checkLowerNetwork(MainActivity.this)) {
            updateBtn.setEnabled(true);
         }else {
            updateBtn.setEnabled(false);
         }
         updateBtn.setText(R.string.btn_check);
         mBtnLocal.setEnabled(true);
         String path = mFullPath.getText().toString();
         if (PermissionUtils.CanDebug()) Log.d(TAG,"path"+path);
         if (path != null && path.contains("/")) {
             mBtnUpdate.setEnabled(true);
         }else {
             mBtnUpdate.setEnabled(false);
         }
    }
    /**
     * Invoked when SystemUpdaterSample app state changes.
     * Value of {@code state} will be one of the
     * values from {@link UpdaterState}.
     */
    private void onUpdaterStateChange(int state) {
        if (PermissionUtils.CanDebug()) Log.i(TAG, "UpdaterStateChange state="
                + UpdaterState.getStateText(state));
        mStatus = state;
        runOnUiThread(() -> {
            if (state > UpdaterState.RUNNING) {
                mProgressBar.setVisibility(View.INVISIBLE);
            }else if (state == UpdaterState.RUNNING && mProgressBar.getVisibility() == View.INVISIBLE) {
                mProgressBar.setIndeterminate(true);
                mProgressBar.setVisibility(View.VISIBLE);
            }
            if ( state == UpdaterState.RUNNING || state == UpdaterState.SLOT_SWITCH_REQUIRED || state == UpdaterState.REBOOT_REQUIRED) {
                if (mvOnline != null) mvOnline.setVisibility(View.INVISIBLE);
                if (mvLocal != null) mvLocal.setVisibility(View.INVISIBLE);
            }
            String rebootCmd = getResources().getString(R.string.reboot_cmd);
            if (!mRunningStatus.getText().equals(rebootCmd)) {
                if (state == UpdaterState.PAUSED || state == UpdaterState.ERROR || state == IDLE) {
                    Log.d(TAG,"adjust ui---->state"+state);
                    resetUI();
                    if (state == UpdaterState.ERROR || state == UpdaterState.PAUSED ) {
                        mRunningStatus.setText(UpdaterState.getStateText(state)+getString(R.string.retry));
                    }else {
                        mRunningStatus.setText("");
                    }
                    return;
                }else if (state == UpdaterState.SLOT_SWITCH_REQUIRED) {
                    mUpdateManager.setSwitchSlotOnReboot();
                }
                if (PermissionUtils.CanDebug()) Log.d(TAG,"adjust ui");
                if (state == UpdaterState.SLOT_SWITCH_REQUIRED) {
                    if (PermissionUtils.CanDebug()) Log.d(TAG,"rebootCmd"+rebootCmd+" "+mRunningStatus.getText());
                    if (!mRunningStatus.getText().equals(rebootCmd)) {
                        mRunningStatus.setText(getResources().getString(R.string.reboot_cmd_prepare));
                        mHandler.removeMessages(MSG_SUCC);
                    }
                } else if (state == UpdaterState.REBOOT_REQUIRED) {
                    if (mRunningStatus.getText().toString().isEmpty()) {
                        mRunningStatus.setText(getResources().getString(R.string.reboot_cmd));
                        mBtnReboot.setVisibility(View.VISIBLE);
                        mBtnReboot.requestFocus();
                        mPref.setBoolean(PrefUtils.key,true);
                        rebootRequest = true;
                    }else {
                        mHandler.removeMessages(MSG_SUCC);
                        mHandler.sendEmptyMessageDelayed(MSG_SUCC,REBOOT_WAIT);
                    }

                } else{
                    setUiUpdaterState(state);
                }
            }
        });
    }

    private static boolean checkLowerNetwork(Context cxt) {
        ConnectivityManager connMgr = (ConnectivityManager) cxt.getSystemService(Context.CONNECTIVITY_SERVICE);
        Network net = connMgr.getActiveNetwork();
        NetworkCapabilities capabilities = connMgr.getNetworkCapabilities(net);
        return capabilities != null && capabilities.hasCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET);
    }

    /**
     * @param state updater sample state
     */
    private void setUiUpdaterState(int state) {
        String stateText = getResources().getString(R.string.running_pre) + UpdaterState.getStateText(state);
        if (PermissionUtils.CanDebug()) Log.d(TAG, "setUiUpdaterState--->"+stateText + "/" + state);

        mRunningStatus.setText(stateText);
    }

    @Override
    public void onClick(View view) {
        switch (view.getId()) {
            case R.id.update_local:
                Intent intent0 = new Intent(this, FileSelector.class);
                Activity activity = this;
                startActivityForResult(intent0, queryUpdateFile);
                break;
            case R.id.update_cmd:
                Log.d(TAG,"filePath"+filePath+"vs"+mFullPath.getText().toString());
                quickStopCopy = true;
                mWorkerHandler.removeCallbacks(cfgRunning);
                mWorkerHandler.post(cfgRunning);
                mBtnLocal.setEnabled(false);
                mBtnUpdate.setEnabled(false);
                updateBtn.setEnabled(false);

                break;
            case R.id.rebootbtn:
                mPref.disableUI(true, MainActivity.this);
                ((PowerManager)MainActivity.this.getSystemService(Context.POWER_SERVICE)).reboot("");
                break;

        }
    }

    class UIHandler extends Handler {
        @Override
        public void handleMessage(@NonNull Message msg) {
            int status = msg.what;
            switch (status) {
                case CHECKING:
                    StringBuilder builder = new StringBuilder();
                    mRunningStatus.setText(getResources().getString(R.string.online_pre)
                                                +getResources().getString(R.string.update_check));
                    break;
                case NEWEST:
                    mRunningStatus.setText(getResources().getString(R.string.online_pre)
                                                +getResources().getString(R.string.update_newest));
                    break;
                case UPDATE:
                    mRunningStatus.setText(getResources().getString(R.string.online_pre)
                                                +getResources().getString(R.string.update_update) + msg.obj);
                    updateBtn.setText(R.string.btn_update);
                    break;
                case MSG_ERR:
                    mRunningStatus.setText(getResources().getString(R.string.online_pre)
                                                +getResources().getString(R.string.error_ver));
                    Log.d(TAG,"current status err");
                    if (mProgressBar.getVisibility() == View.VISIBLE) {
                        mProgressBar.setVisibility(View.INVISIBLE);
                    }
                    if (mvOnline != null)
                        mvOnline.setVisibility(View.VISIBLE);
                    if (mvLocal != null)
                        mvLocal.setVisibility(View.VISIBLE);
                    mBtnLocal.setEnabled(true);
                    break;
                case MSG_SUCC:
                    String preparecmd = getResources().getString(R.string.reboot_cmd_prepare);
                    if (mRunningStatus.getText().equals(preparecmd)) {
                        mRunningStatus.setText(getResources().getString(R.string.reboot_cmd));
                        mBtnReboot.setVisibility(View.VISIBLE);
                        mBtnReboot.requestFocus();
                    }
                    mPref.setBoolean(PrefUtils.key,true);
                    rebootRequest = true;
                    break;
                case MSG_INIT:
                    try {
                        mUpdateManager.resetUpdate();
                    } catch (Exception ex){}
                    break;
                case MSG_WAIT_FOR_MERGE:
                    if (mPref.getBooleanVal(PrefUtils.key, false)) {
                        mHandler.sendEmptyMessageDelayed(MSG_WAIT_FOR_MERGE,1000);
                    }else {
                        initialView();
                    }
                    break;
            }
        }
    }

}
