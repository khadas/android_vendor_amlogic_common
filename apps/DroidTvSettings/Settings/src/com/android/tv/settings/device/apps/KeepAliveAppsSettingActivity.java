package com.android.tv.settings.device.apps;

import android.app.ActivityManager;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.Window;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.ListView;
import android.widget.Toast;
import android.app.Activity;
import android.provider.Settings;
import com.android.tv.settings.R;
import android.view.Window;
import android.view.WindowManager;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;

public class KeepAliveAppsSettingActivity extends Activity {
    private static final String TAG = "KeepAliveAppsSetting";
    private ListView lvApps;
    private List<AppInfo> AppInfoList = new ArrayList<>();
    private AppInfoAdapter adapter;
    private PackageManager packageManager;
    private int mIconDpi;

    private List<String> keepAliveAppsList = new ArrayList<>();

    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                WindowManager.LayoutParams.FLAG_FULLSCREEN);

        setContentView(R.layout.activity_selectapps);

        lvApps = (ListView)findViewById(R.id.lv_apps);

        ActivityManager activityManager =  (ActivityManager)getSystemService(Context.ACTIVITY_SERVICE);
        packageManager = getPackageManager();
        mIconDpi = activityManager.getLauncherLargeIconDensity();

        adapter = new AppInfoAdapter(KeepAliveAppsSettingActivity.this, AppInfoList, true);
        lvApps.setAdapter(adapter);
        lvApps.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> adapterView, View view, int i, long l) {
                Log.d(TAG, "onItemClick " + i);
            }
        });

        adapter.setOnItemCheckedChangedListener(new AppInfoAdapter.onItemCheckedChangedListener() {
            @Override
            public void onCheckedChanged(int i, boolean isChecked) {
                AppInfoList.get(i).setSelect(isChecked);
                String packageName = AppInfoList.get(i).getPkgName();
                Log.d(TAG, "onCheckedChanged packageName " + packageName + " isChecked " + isChecked);
                saveSelectApps();
            }
        });

        String alwaysAliveApps = Settings.Global.getString(getContentResolver(), Settings.Global.KEEP_ALIVE_APP);;
        Log.e(TAG, "onCreate strKidsApps " + alwaysAliveApps);
        if(!TextUtils.isEmpty(alwaysAliveApps)) {
            keepAliveAppsList = Arrays.asList(alwaysAliveApps.split(";"));
        }

        Intent filterIntent = new Intent(Intent.ACTION_MAIN, null);
        filterIntent.addCategory(Intent.CATEGORY_LAUNCHER);
        List<ResolveInfo> apps = packageManager.queryIntentActivities(filterIntent, 0);
        for (ResolveInfo resolveInfo : apps) {
            AppInfo appInfo = new AppInfo();
            appInfo.setPkgName(resolveInfo.activityInfo.applicationInfo.packageName);
            appInfo.setAppName(resolveInfo.loadLabel(packageManager).toString());
            appInfo.setIcon(getResIconFormActyInfo(resolveInfo.activityInfo));
            if(keepAliveAppsList != null && keepAliveAppsList.contains(resolveInfo.activityInfo.applicationInfo.packageName)) {
                appInfo.setSelect(true);
            }
            AppInfoList.add(appInfo);
        }
    }

    private void saveSelectApps() {
        String apps = "";
        for(int i = 0; i < AppInfoList.size(); i++) {
            if(AppInfoList.get(i).isSelect()) {
                apps += AppInfoList.get(i).getPkgName() + ";";
            }
        }
        Settings.Global.putString(getContentResolver(), Settings.Global.KEEP_ALIVE_APP, apps);
        String alwaysAliveApps = Settings.Global.getString(getContentResolver(), Settings.Global.KEEP_ALIVE_APP);
        Log.d(TAG, "alwaysAliveApps " + alwaysAliveApps);
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
        Log.e(TAG, "onDestroy");
        saveSelectApps();
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        return super.onKeyDown(keyCode, event);
    }

    private Drawable getResIconFormActyInfo(ActivityInfo info) {
        Resources resources;
        try {
            resources = packageManager.getResourcesForApplication(info.applicationInfo);
        } catch (PackageManager.NameNotFoundException e) {
            resources = null;
        }
        if (resources != null) {
            int iconId = info.getIconResource();
            if (iconId != 0) {
                return getResIconFormActyInfo(resources, iconId);
            }
        }
        return getDefaultIcon();
    }

    private Drawable getResIconFormActyInfo(Resources resources, int iconId) {
        Drawable drawable;
        try {
            drawable = resources.getDrawableForDensity(iconId, mIconDpi);
        } catch (Resources.NotFoundException e) {
            drawable = null;
        }
        return (drawable != null) ? drawable : getDefaultIcon();
    }

    private Drawable getDefaultIcon() {
        return getResIconFormActyInfo(Resources.getSystem(),
                android.R.mipmap.sym_def_app_icon);
    }

}