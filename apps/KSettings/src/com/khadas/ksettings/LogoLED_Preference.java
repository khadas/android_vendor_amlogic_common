package com.khadas.ksettings;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.SystemProperties;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceManager;


public class LogoLED_Preference extends PreferenceActivity implements Preference.OnPreferenceClickListener {

    private ListPreference logoled_Preference;
    private LogoLEDBarPreference logoledbar_Preference;


    private static final String LOGELED_BRIGHTNESS_KEY = "logoled_Brightness";
    private static final String LOGO_LED_KEY = "LOGO_LED_KEY";

    private static Context mContext;

    private static boolean status_flag = true;

    public static final int MSG_UI_BL = 101;

    private Handler uiHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_UI_BL:
                    int value = SystemProperties.getInt("persist.sys.logoled.brightness", 15);
                    logoledbar_Preference.setSummary("" + value);
                    break;
                default:
                    break;
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.logoled_control);

        getActionBar().setHomeButtonEnabled(true);
        getActionBar().setDisplayHomeAsUpEnabled(true);

        mContext = this;

        logoledbar_Preference = (LogoLEDBarPreference) findPreference(LOGELED_BRIGHTNESS_KEY);

        logoled_Preference = (ListPreference) findPreference(LOGO_LED_KEY);
        bindPreferenceSummaryToValue(logoled_Preference);

    }

    @Override
    protected void onResume() {
        super.onResume();
        status_flag = true;
        new Thread (new Runnable() {
            @Override
            public void run() {
                // do ui operate
                while (status_flag){
                    uiHandler.removeMessages(MSG_UI_BL);
                    uiHandler.sendEmptyMessageDelayed(MSG_UI_BL,100);
                    try {
                        Thread.sleep(1 * 1000);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }
            }
        }).start();



    }


    @Override
    protected void onStop() {
        super.onStop();
        status_flag = false;
    }

    /**
     * bindPreferenceSummaryToValue 拷贝至as自动生成的preferences的代码，用于绑定显示实时值
     */
    private static Preference.OnPreferenceChangeListener sBindPreferenceSummaryToValueListener = new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange(Preference preference, Object value) {
            String stringValue = value.toString();
            String key = preference.getKey();
            if (preference instanceof ListPreference) {
                // For list preferences, look up the correct display value in
                // the preference's 'entries' list.
                ListPreference listPreference = (ListPreference) preference;
                int index = listPreference.findIndexOfValue(stringValue);
                // Set the summary to reflect the new value.
                preference.setSummary(index >= 0 ? listPreference.getEntries()[index] : null);

                if (LOGO_LED_KEY.equals(key)){
                    //Log.d("wjh","2===" + index);
                    SystemProperties.set("persist.sys.logo_led_control", "" + index);

                }

            }  else {
                // For all other preferences, set the summary to the value's
                // simple string representation.
                preference.setSummary(stringValue);
            }
            return true;
        }
    };
    private static void bindPreferenceSummaryToValue(Preference preference) {
        // Set the listener to watch for value changes.
        preference.setOnPreferenceChangeListener(sBindPreferenceSummaryToValueListener);

        // Trigger the listener immediately with the preference's
        // current value.
        sBindPreferenceSummaryToValueListener.onPreferenceChange(preference,
                PreferenceManager.getDefaultSharedPreferences(preference.getContext()).getString(preference.getKey(), ""));
    }


    @Override
    public void onPointerCaptureChanged(boolean hasCapture) {

    }

    @Override
    public boolean onPreferenceClick(Preference preference) {
        return false;
    }
}
