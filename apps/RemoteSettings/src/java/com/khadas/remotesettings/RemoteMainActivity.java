package com.khadas.remotesettings;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.ListView;

import com.google.gson.Gson;
import com.google.gson.reflect.TypeToken;

import java.util.ArrayList;
import java.util.List;

public class RemoteMainActivity extends Activity {
    private static final String TAG = "MainActivity";
    private ListView lvIRKey;
    private List<RemoteKeyInfo> irkeyList = new ArrayList<>();
    private RemoteKeyAdapter adapter;
    private Button btnNext;
    private Button btnReset;
    private int REQUEST_CODE = 1;

    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_remote_main);

        lvIRKey = (ListView)findViewById(R.id.lv_irkey);
        String keyInfo = RemotePreference.getKeyInfo(RemoteMainActivity.this);
        Log.e(TAG, "onCreate keyInfo:" + keyInfo);
        Gson gson = new Gson();
        irkeyList = gson.fromJson(keyInfo, new TypeToken<List<RemoteKeyInfo>>() {}.getType());

        irkeyList.get(0).setName(getString(R.string.power));
        irkeyList.get(1).setName(getString(R.string.up));
        irkeyList.get(2).setName(getString(R.string.down));
        irkeyList.get(3).setName(getString(R.string.left));
        irkeyList.get(4).setName(getString(R.string.right));
        irkeyList.get(5).setName(getString(R.string.ok));
        irkeyList.get(6).setName(getString(R.string.back));
        irkeyList.get(7).setName(getString(R.string.mouse));
        irkeyList.get(8).setName(getString(R.string.menu));
        irkeyList.get(9).setName(getString(R.string.volume_down));
        irkeyList.get(10).setName(getString(R.string.volume_up));
        irkeyList.get(11).setName(getString(R.string.home));

        adapter = new RemoteKeyAdapter(RemoteMainActivity.this, irkeyList);
        lvIRKey.setAdapter(adapter);
        lvIRKey.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> adapterView, View view, int i, long l) {
                irkeyList.get(i).setSelect(!irkeyList.get(i).isSelect());
                adapter.notifyDataSetChanged();
            }
        });

        btnNext = (Button) findViewById(R.id.btn_next);
        btnNext.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                int selectNum = 0;
                for(int i = 0; i < irkeyList.size(); i++) {
                    if(irkeyList.get(i).isSelect()) {
                        selectNum++;
                    }
                }
                Log.d(TAG, "selectNum:" + selectNum);
                if(selectNum == 0) {
                    return;
                }

                Gson gson = new Gson();
                String gSonStr = gson.toJson(irkeyList);
                Intent irKeySettingsIntent = new Intent(RemoteMainActivity.this, RemoteKeySettingActivity.class);
                irKeySettingsIntent.putExtra("keyInfo", gSonStr);
                startActivityForResult(irKeySettingsIntent, REQUEST_CODE);
            }
        });

        btnReset = (Button)findViewById(R.id.btn_reset);
        btnReset.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                RemotePreference.setKeyInfo(RemoteMainActivity.this, RemotePreference.DEF_KEY_INFO);
                Gson gson2 = new Gson();
                irkeyList = gson2.fromJson(RemotePreference.DEF_KEY_INFO, new TypeToken<List<RemoteKeyInfo>>() {}.getType());
                for(int i = 0; i < irkeyList.size(); i++) {
                    RemoteKeyUtils.updateIRKey(irkeyList.get(i).getScanCode(), irkeyList.get(i).getIrCode());
                }
                RemoteKeyUtils.exitSetKeyMode();
                finish();
            }
        });

    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        Log.e(TAG, "onActivityResult requestCode " + requestCode + " resultCode " + resultCode);
        if (requestCode == REQUEST_CODE && resultCode == Activity.RESULT_OK) {
            if (data != null) {
                String keyInfo = data.getStringExtra("keyInfo");
                Log.e(TAG, "onActivityResult keyInfo:" + keyInfo);
                RemotePreference.setKeyInfo(RemoteMainActivity.this, keyInfo);
            }
            finish();
        }
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        Log.d(TAG, "onKeyDown keyCode " + keyCode + " scanCode " + event.getScanCode() + " readIRCode " + RemoteKeyUtils.readIRCode());
        return super.onKeyDown(keyCode, event);
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    @Override
    protected void onPause() {
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

}