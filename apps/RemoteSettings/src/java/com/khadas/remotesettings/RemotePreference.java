package com.khadas.remotesettings;

import android.content.Context;
import android.content.SharedPreferences;

public class RemotePreference {

	public static final String DEF_KEY_INFO = "[" +
			"{\n" +
			"\t\"irCode\": 20,\n" +
			"\t\"isSelect\": true,\n" +
			"\t\"key\": \"POWER\",\n" +
			"\t\"name\": \"POWER\",\n" +
			"\t\"scanCode\": 116\n" +
			"}, " +

			"{\n" +
			"\t\"irCode\": 3,\n" +
			"\t\"isSelect\": true,\n" +
			"\t\"key\": \"UP\",\n" +
			"\t\"name\": \"UP\",\n" +
			"\t\"scanCode\": 103\n" +
			"}, "+

			"{\n" +
			"\t\"irCode\": 2,\n" +
			"\t\"isSelect\": true,\n" +
			"\t\"key\": \"DOWN\",\n" +
			"\t\"name\": \"DOWN\",\n" +
			"\t\"scanCode\": 108\n" +
			"}, " +

			"{\n" +
			"\t\"irCode\": 14,\n" +
			"\t\"isSelect\": true,\n" +
			"\t\"key\": \"LEFT\",\n" +
			"\t\"name\": \"LEFT\",\n" +
			"\t\"scanCode\": 105\n" +
			"}, " +

			"{\n" +
			"\t\"irCode\": 26,\n" +
			"\t\"isSelect\": true,\n" +
			"\t\"key\": \"RIGHT\",\n" +
			"\t\"name\": \"RIGHT\",\n" +
			"\t\"scanCode\": 106\n" +
			"}, " +

			"{\n" +
			"\t\"irCode\": 7,\n" +
			"\t\"isSelect\": true,\n" +
			"\t\"key\": \"OK\",\n" +
			"\t\"name\": \"OK\",\n" +
			"\t\"scanCode\": 232\n" +
			"}, " +

			"{\n" +
			"\t\"irCode\": 1,\n" +
			"\t\"isSelect\": true,\n" +
			"\t\"key\": \"BACK\",\n" +
			"\t\"name\": \"BACK\",\n" +
			"\t\"scanCode\": 158\n" +
			"}, " +

			"{\n" +
			"\t\"irCode\": 91,\n" +
			"\t\"isSelect\": true,\n" +
			"\t\"key\": \"MOUSE\",\n" +
			"\t\"name\": \"MOUSE\",\n" +
			"\t\"scanCode\": 63\n" +
			"}, " +

			"{\n" +
			"\t\"irCode\": 13,\n" +
			"\t\"isSelect\": true,\n" +
			"\t\"key\": \"MENU\",\n" +
			"\t\"name\": \"MENU\",\n" +
			"\t\"scanCode\": 139\n" +
			"}, " +

			" {\n" +
			"\t\"irCode\": 88,\n" +
			"\t\"isSelect\": true,\n" +
			"\t\"key\": \"Volume Down\",\n" +
			"\t\"name\": \"Volume Down\",\n" +
			"\t\"scanCode\": 114\n" +
			"}, " +

			"{\n" +
			"\t\"irCode\": 11,\n" +
			"\t\"isSelect\": true,\n" +
			"\t\"key\": \"Volume UP\",\n" +
			"\t\"name\": \"Volume UP\",\n" +
			"\t\"scanCode\": 115\n" +
			"}," +

			"{\n" +
			"\t\"irCode\": 72,\n" +
			"\t\"isSelect\": true,\n" +
			"\t\"key\": \"HOME\",\n" +
			"\t\"name\": \"HOME\",\n" +
			"\t\"scanCode\": 102\n" +
			"}" +

			"]";

	private static SharedPreferences getSP(Context context) {
		return context.getSharedPreferences(context.getString(R.string.app_name), Context.MODE_PRIVATE);
	}

	public static String getKeyInfo(Context context) {
		return getSP(context).getString("KeyInfo", DEF_KEY_INFO);
	}

	public static void setKeyInfo(Context context, String value) {
		getSP(context).edit().putString("KeyInfo", value).commit();
	}

}

