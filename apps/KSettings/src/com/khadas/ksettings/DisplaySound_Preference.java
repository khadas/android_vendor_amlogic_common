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
import android.preference.PreferenceScreen;
import android.preference.SwitchPreference;
import android.provider.Settings;
import android.widget.Toast;

import java.io.IOException;


public class DisplaySound_Preference extends PreferenceActivity implements Preference.OnPreferenceClickListener {

    private ListPreference Screen_rotation_Preference;
    private ListPreference Screen_density_Preference;

    private SwitchPreference Force_land_Preference;

    private MipiSeekBarPreference mipi_Preference;
    private VboSeekBarPreference vbo_Preference;

    private PreferenceScreen MORE_DISPLAY_Preference;

    private static final String MIPI_BRIGHTNESS_KEY = "mipi_Brightness";
    private static final String VBO_BRIGHTNESS_KEY = "vbo_Brightness";

    private static final String DISPLAY_ROTATION_KEY = "DISPLAY_ROTATION_KEY";
    private static final String DISPLAY_DENSITY_KEY = "DISPLAY_DENSITY_KEY";


    private static final String FORCE_LAN_FOR_APP_KEY = "FORCE_LAN_FOR_APP_KEY";

    private static final String MORE_DISPLAY_SETTINGS = "MORE_DISPLAY_SETTINGS";

    private String mipi,vbo;

    private static Context mContext;

    private static boolean status_flag = true;

    public static final int MSG_UI_BL = 101;

    private Handler uiHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_UI_BL:
                    //mipi -- /sys/class/backlight/aml-bl/brightness
                    //vbo -- /sys/class/backlight/aml-bl1/brightness
                    try {
                        mipi = ComApi.execCommand(new String[]{"sh", "-c", "cat /sys/class/backlight/aml-bl/brightness"});
                        if(mipi.equals("") || mipi.contains("No such file or directory")){
                            mipi_Preference.setSummary("No MIPI Panel");
                            mipi_Preference.setEnabled(false);
                        }else {
                            mipi_Preference.setSummary("" + mipi);
                            mipi_Preference.setEnabled(true);
                        }

                        vbo = ComApi.execCommand(new String[]{"sh", "-c", "cat /sys/class/backlight/aml-bl1/brightness"});
                        if(vbo.equals("") || vbo.contains("No such file or directory")){
                            vbo_Preference.setSummary("No VBO Panel");
                            vbo_Preference.setEnabled(false);
                        }else {
                            vbo_Preference.setSummary("" + vbo);
                            vbo_Preference.setEnabled(true);
                        }

                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                    break;
                default:
                    break;
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.display_sound);

        getActionBar().setHomeButtonEnabled(true);
        getActionBar().setDisplayHomeAsUpEnabled(true);

        mContext = this;

        mipi_Preference = (MipiSeekBarPreference) findPreference(MIPI_BRIGHTNESS_KEY);
        vbo_Preference = (VboSeekBarPreference) findPreference(VBO_BRIGHTNESS_KEY);

        Screen_rotation_Preference = (ListPreference) findPreference(DISPLAY_ROTATION_KEY);
        bindPreferenceSummaryToValue(Screen_rotation_Preference);

        Screen_density_Preference = (ListPreference) findPreference(DISPLAY_DENSITY_KEY);
        bindPreferenceSummaryToValue(Screen_density_Preference);

        Force_land_Preference = (SwitchPreference)findPreference(FORCE_LAN_FOR_APP_KEY);
        Force_land_Preference.setOnPreferenceClickListener(this);
        getPreferenceScreen().removePreference(Force_land_Preference);

        MORE_DISPLAY_Preference = (PreferenceScreen)findPreference(MORE_DISPLAY_SETTINGS);
        MORE_DISPLAY_Preference.setOnPreferenceClickListener(this);
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
                    //Message msg= uiHandler.obtainMessage(MSG_UI_BL,"HW Addr:"+hwaddr+"  IP Addr: "+ip);
                    //uiHandler.sendMessageDelayed(msg,100);

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

                if (DISPLAY_DENSITY_KEY.equals(key)){
                    String command = index >= 0 ? listPreference.getEntries()[index].toString() : "";
                    try {
                        ComApi.execCommand(new String[]{"sh", "-c", "wm density " + command});
                    } catch (IOException e) {
                        e.printStackTrace();
                    }

                }else if(DISPLAY_ROTATION_KEY.equals(key)){
                    Settings.System.putInt(mContext.getContentResolver(),"user_rotation",index);
                    SystemProperties.set("persist.sys.user_rotation",""+index);
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
        final String key = preference.getKey();
        if (FORCE_LAN_FOR_APP_KEY.equals(key)){
            if (Force_land_Preference.isChecked()) {
                Toast.makeText(this,"true",Toast.LENGTH_SHORT).show();
            }else {
                Toast.makeText(this,"false",Toast.LENGTH_SHORT).show();
            }
        }else if (MORE_DISPLAY_SETTINGS.equals(key)){
            SystemProperties.set("persist.sys.use.tv_settings","1");
            Intent intent = new Intent();
            intent.setClassName("com.android.tv.settings", "com.android.tv.settings.device.displaysound.DisplaySoundActivity");
            startActivity(intent);
            SystemProperties.set("persist.sys.use.tv_settings","0");
        }
        return true;
    }
}
