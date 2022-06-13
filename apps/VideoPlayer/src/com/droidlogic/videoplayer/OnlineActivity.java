/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description:
*/
package com.droidlogic.videoplayer;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;


public class OnlineActivity extends Activity implements View.OnClickListener {
	private static String TAG = "OnlineActivity";
	private boolean DEBUG = true;

    private ExtEditText urlEditText;
    private Button playBtn;
    private SharedPreferences mPreference;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_online);
        initSharedPreference();
        initView();
    }

    private void initSharedPreference() {
        mPreference = getSharedPreferences("pref_candidate_list", Context.MODE_PRIVATE);
    }

    private void LOGI (String tag, String msg) {
        if (DEBUG) { Log.i (tag, msg); }
    }

    private void initView(){
        urlEditText = (ExtEditText) findViewById(R.id.et_url);
        playBtn = (Button) findViewById(R.id.btn_play);
        playBtn.setOnClickListener(this);

        CandidateListView candidateListView = new CandidateListView(getApplicationContext());
        candidateListView.setPreference(mPreference);
        urlEditText.setCandidateListView(candidateListView);
    }

    private boolean isValidUrl(String url){
        if (url != null) {
            if (url.startsWith("http:")
             || url.startsWith("udp:")
             || url.startsWith("rtsp:")
             || url.startsWith("rtp:")
             || url.startsWith("https:")) {
                 return true;
            }
        }
        return false;
    }

    private void makeTextToast(String text){
        Toast toast = Toast.makeText (OnlineActivity.this, text, Toast.LENGTH_SHORT);
        toast.setGravity (Gravity.BOTTOM,0, 0);
        toast.setDuration (0x00000001);
        toast.show();
    }

    @Override
    public void onClick(View view) {
        int id = view.getId();
        LOGI(TAG, "onClick");
        switch (id) {
            case R.id.btn_play:
                String defaultUri = urlEditText.getText().toString();
                if (isValidUrl(defaultUri)) {
                    saveUrl(defaultUri);

                    List<String> paths = new ArrayList<String>();
                    paths.add (defaultUri);
                    PlayList.getinstance().setlist (paths, 0);
                    Intent intent = new Intent();
                    intent.setClass (OnlineActivity.this, VideoPlayer.class);
                    startActivity(intent);
                } else {
                    makeTextToast("Invalid Url");
                }
                break;
            default:
      }
  }

    private void saveUrl(String uri) {
        Set<String> candidateUrls =
                mPreference.getStringSet(CandidateListView.KEY_CANDIDATES_URLS, null);
        if (candidateUrls == null) {
            candidateUrls = new HashSet<String>();
        }

        if (candidateUrls.contains(uri)) {
            return;
        }

        candidateUrls.add(uri);
        SharedPreferences.Editor editor = mPreference.edit();
        editor.putStringSet(CandidateListView.KEY_CANDIDATES_URLS, null).apply();
        editor.putStringSet(CandidateListView.KEY_CANDIDATES_URLS,
                new HashSet<String>(candidateUrls))
                .apply();
    }
}
