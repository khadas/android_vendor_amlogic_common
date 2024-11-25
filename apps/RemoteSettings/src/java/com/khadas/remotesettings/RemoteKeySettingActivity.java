package com.khadas.remotesettings;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;
import android.view.KeyEvent;
import android.widget.TextView;

import com.google.gson.Gson;
import com.google.gson.reflect.TypeToken;

import java.util.ArrayList;
import java.util.List;

public class RemoteKeySettingActivity extends Activity {
    private static final String TAG = "IRKeySettingActivity";
    private List<RemoteKeyInfo> irkeyList = new ArrayList<>();
    private int keySetIndex = 0;
    private TextView tvShowInfo;

    private int getNextIrKey(int start) {
        for(int i = start; i < irkeyList.size(); i++) {
            if(irkeyList.get(i).isSelect()) {
                return i;
            }
        }
        return irkeyList.size();
    }

    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_remotekey_setting);
        String keyStr = getIntent().getStringExtra("keyInfo");
        Log.e(TAG, "onCreate " + keyStr);
        if(!TextUtils.isEmpty(keyStr)) {
            Gson gson = new Gson();
            irkeyList = gson.fromJson(keyStr, new TypeToken<List<RemoteKeyInfo>>() {}.getType());
        }

        tvShowInfo = (TextView) findViewById(R.id.tv_showInfo);
        RemoteKeyUtils.enterSetKeyMode();
        keySetIndex = getNextIrKey(keySetIndex);
        tvShowInfo.setText(getString(R.string.press_key_prompt, irkeyList.get(keySetIndex).getName()));
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        int irCode = RemoteKeyUtils.readIRCode();
        irkeyList.get(keySetIndex).setIrCode(irCode);
        RemoteKeyUtils.updateIRKey(irkeyList.get(keySetIndex).getScanCode(), irkeyList.get(keySetIndex).getIrCode());
        keySetIndex++;
        keySetIndex = getNextIrKey(keySetIndex);
        if(keySetIndex >= irkeyList.size()) {
            Gson gson = new Gson();
            String gSonStr = gson.toJson(irkeyList);
            Log.e(TAG, "keyInfo:" + gSonStr);
            Intent resultIntent = new Intent();
            resultIntent.putExtra("keyInfo", gSonStr);
            setResult(Activity.RESULT_OK, resultIntent);
            finish();
            return true;
        }
        tvShowInfo.setText(getString(R.string.press_key_prompt, irkeyList.get(keySetIndex).getName()));
        return super.onKeyDown(keyCode, event);
    }

    @Override
    protected void onResume() {
        super.onResume();
        RemoteKeyUtils.enterSetKeyMode();
    }

    @Override
    protected void onPause() {
        super.onPause();
        RemoteKeyUtils.exitSetKeyMode();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        RemoteKeyUtils.exitSetKeyMode();
    }
}