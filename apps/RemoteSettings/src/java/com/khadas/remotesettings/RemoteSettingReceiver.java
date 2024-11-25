package com.khadas.remotesettings;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

import com.google.gson.Gson;
import com.google.gson.reflect.TypeToken;

import java.util.ArrayList;
import java.util.List;

public class RemoteSettingReceiver extends BroadcastReceiver{
	private static final String TAG = "IRSettingReceiver";
	@Override
	public void onReceive(Context context, Intent intent) {
		// TODO Auto-generated method stub
		String action = intent.getAction();
		Gson gson = new Gson();
		List<RemoteKeyInfo> irkeyList = new ArrayList<>();
		String keyInfo = RemotePreference.getKeyInfo(context);
		Log.d(TAG, "keyInfo " + keyInfo);
		if(!keyInfo.equals(RemotePreference.DEF_KEY_INFO)) {
			irkeyList = gson.fromJson(keyInfo, new TypeToken<List<RemoteKeyInfo>>() {
			}.getType());
			for (int i = 0; i < irkeyList.size(); i++) {
				RemoteKeyUtils.updateIRKey(irkeyList.get(i).getScanCode(), irkeyList.get(i).getIrCode());
			}
		}
	}
}
