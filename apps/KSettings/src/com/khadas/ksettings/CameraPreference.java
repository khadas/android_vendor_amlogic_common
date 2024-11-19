package com.khadas.ksettings;

import android.annotation.SuppressLint;
import android.app.ActionBar;
import android.app.StatusBarManager;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.SystemProperties;
import android.preference.Preference;
import android.preference.ListPreference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceManager;
import android.preference.SwitchPreference;
import android.view.View;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.view.WindowManager;
import android.widget.Toast;
import android.util.Log;


public class CameraPreference extends PreferenceActivity implements Preference.OnPreferenceClickListener {

    private static final String TAG = "CameraPreference";

    private static final String CAMERA_FACEBACK_KEY = "CAMERA_FACEBACK_KEY";
    private static final String CAMERA_ORIENTATION_KEY = "CAMERA_ORIENTATION_KEY";
    private Context mContext;

    private ListPreference mCameraFaceBackPreference;
    private ListPreference mCameraOrientationPreference;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.camera_setting);
        getActionBar().setHomeButtonEnabled(true);
        getActionBar().setDisplayHomeAsUpEnabled(true);
        mContext = this;
        initPreference();
    }

    private void initPreference() {
        mCameraFaceBackPreference = (ListPreference) findPreference(CAMERA_FACEBACK_KEY);
        bindPreferenceSummaryToValue(mCameraFaceBackPreference);
        String faceback = SystemProperties.get("persist.sys.camera_usb_faceback", "2");
        mCameraFaceBackPreference.setValue(faceback);

        mCameraOrientationPreference = (ListPreference) findPreference(CAMERA_ORIENTATION_KEY);
        bindPreferenceSummaryToValue(mCameraOrientationPreference);
        String orientation = SystemProperties.get("persist.sys.camera_usb_orientation", "0");
        mCameraOrientationPreference.setValue(orientation);
        Log.e(TAG, "faceback " + faceback + " orientation " + orientation);
    }

    private Preference.OnPreferenceChangeListener sBindPreferenceSummaryToValueListener = new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange(Preference preference, Object value) {
            String stringValue = value.toString();
            String key = preference.getKey();
            Log.e(TAG, "stringValue " + stringValue + " key " + key);
            if (preference instanceof ListPreference) {
                // For list preferences, look up the correct display value in
                // the preference's 'entries' list.
                ListPreference listPreference = (ListPreference) preference;
                int index = listPreference.findIndexOfValue(stringValue);
                // Set the summary to reflect the new value.
                preference.setSummary(index >= 0 ? listPreference.getEntries()[index] : null);
            if (CAMERA_FACEBACK_KEY.equals(key)) {
                String faceback = SystemProperties.get("persist.sys.camera_usb_faceback", "2");
                if(!stringValue.equals(faceback)) {
                    SystemProperties.set("persist.sys.camera_usb_faceback", stringValue);
                    Toast.makeText(mContext, mContext.getString(R.string.reboot_take_effect), Toast.LENGTH_SHORT).show();
                }
            } else if(CAMERA_ORIENTATION_KEY.equals(key)) {
                String orientation = SystemProperties.get("persist.sys.camera_usb_orientation", "0");
                if (!stringValue.equals(orientation)) {
                    SystemProperties.set("persist.sys.camera_usb_orientation", stringValue);
                    Toast.makeText(mContext, mContext.getString(R.string.reboot_take_effect), Toast.LENGTH_SHORT).show();
                }
            }
            } else {
                // For all other preferences, set the summary to the value's
                // simple string representation.
                preference.setSummary(stringValue);
            }
            return true;
        }
    };

    private void bindPreferenceSummaryToValue(Preference preference) {
        // Set the listener to watch for value changes.
        preference.setOnPreferenceChangeListener(sBindPreferenceSummaryToValueListener);
        // Trigger the listener immediately with the preference's
        // current value.
        sBindPreferenceSummaryToValueListener.onPreferenceChange(preference, PreferenceManager.getDefaultSharedPreferences(preference.getContext()).getString(preference.getKey(), ""));
    }

    @Override
    public void onPointerCaptureChanged(boolean hasCapture) {
    }

    @Override
    public boolean onPreferenceClick(Preference preference) {
        return true;
    }

}
