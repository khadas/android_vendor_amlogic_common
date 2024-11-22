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


public class PinMultiplePreference extends PreferenceActivity implements Preference.OnPreferenceClickListener {

    private PreferenceScreen PIN_MULTIPLE_Preference;

    private static final String PIN_MULTIPLE = "PIN_MULTIPLE";

    private static Context mContext;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.pin_multiple);

        getActionBar().setHomeButtonEnabled(true);
        getActionBar().setDisplayHomeAsUpEnabled(true);

        mContext = this;

        PIN_MULTIPLE_Preference = (PreferenceScreen)findPreference(PIN_MULTIPLE);
        PIN_MULTIPLE_Preference.setOnPreferenceClickListener(this);
    }

    @Override
    protected void onResume() {
        super.onResume();
    }


    @Override
    protected void onStop() {
        super.onStop();
    }

    @Override
    public void onPointerCaptureChanged(boolean hasCapture) {

    }

    @Override
    public boolean onPreferenceClick(Preference preference) {
        final String key = preference.getKey();
        if (PIN_MULTIPLE.equals(key)){
            SystemProperties.set("persist.sys.use.tv_settings","1");
            Intent intent = new Intent();
            intent.setClassName("com.android.tv.settings", "com.android.tv.settings.device.pin.PinMultipleActivity");
            startActivity(intent);
            SystemProperties.set("persist.sys.use.tv_settings","0");
        }
        return true;
    }
}
