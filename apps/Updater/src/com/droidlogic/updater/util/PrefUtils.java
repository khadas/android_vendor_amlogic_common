/******************************************************************
 *
 *Copyright(C) 2012 Amlogic, Inc.
 *
 *Licensed under the Apache License, Version 2.0 (the "License");
 *you may not use this file except in compliance with the License.
 *You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing, software
 *distributed under the License is distributed on an "AS IS" BASIS,
 *WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *See the License for the specific language governing permissions and
 *limitations under the License.
 ******************************************************************/
package com.droidlogic.updater.util;

import android.content.Context;
import android.content.ComponentName;
import android.content.pm.PackageManager;
import android.content.SharedPreferences;
import android.os.Build;
import android.os.storage.StorageManager;
import android.util.Log;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.Enumeration;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;
import java.lang.reflect.Constructor;
import java.lang.reflect.Method;
import java.nio.channels.FileChannel;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Set;
import com.droidlogic.updater.ui.MainActivity;
import com.droidlogic.updater.util.PermissionUtils;

/**
 * @ClassName PrefUtils
 * @Description TODO
 * @Date 2013-7-16
 * @Email
 * @Author
 * @Version V1.0
 */
public class PrefUtils {
    public static Boolean DEBUG = false;
    public static final String TAG = "OTA";
    public static final String key = "updateby";
    public static final String EXTERNAL_STORAGE = "/external_storage/";
    private static final String PREFS_DOWNLOAD_FILELIST = "download_filelist";
    private static final String PREFS_UPDATE_FILEPATH = "update_file_path";
    private static final String PREFS_UPDATE_SCRIPT = "update_with_script";
    private static final String PREFS_UPDATE_FILESIZE = "update_file_size";
    private static final String PREFS_UPDATE_DESC = "update_desc";
    public static final String DEV_PATH = "/storage/external_storage";
    public static final String PREF_START_RESTORE = "retore_start";
    public static final String PREF_AUTO_CHECK = "auto_check";
    static final String FlagFile = ".wipe_record";
    public static final String DATA_UPDATE = "/data/droidota/update.zip";
    private Context mContext;
    private SharedPreferences mPrefs;
    private static final int FILE_COPY_BUFFER_SIZE = 1024;
    public PrefUtils(Context context) {
        mPrefs = context.getSharedPreferences("update", Context.MODE_PRIVATE);
        mContext = context;
    }

    private void setString(String key, String Str) {
        SharedPreferences.Editor mEditor = mPrefs.edit();
        mEditor.putString(key, Str);
        mEditor.commit();
    }

    private void setStringSet(String key, Set<String> downSet) {
        SharedPreferences.Editor mEditor = mPrefs.edit();
        mEditor.putStringSet(key, downSet);
        mEditor.commit();
    }

    private void setInt(String key, int Int) {
        SharedPreferences.Editor mEditor = mPrefs.edit();
        mEditor.putInt(key, Int);
        mEditor.commit();
    }

    public void setDescrib(String desc) {
        setString(PREFS_UPDATE_DESC, desc);
    }

    public void clearData() {
        String filePath = getUpdatePath();
        File file = null;
        if (filePath != null) {
            file = new File(filePath);
            if (file.exists()) {
                file.delete();
            }
        }
        file = new File(DATA_UPDATE);
        if (file.exists()) {
            file.delete();
        }
    }

    public String getDescri() {
        return mPrefs.getString(PREFS_UPDATE_DESC, "");
    }

    private void setLong(String key, long Long) {
        SharedPreferences.Editor mEditor = mPrefs.edit();
        mEditor.putLong(key, Long);
        mEditor.commit();
    }

    public void setBoolean(String key, Boolean bool) {
        SharedPreferences.Editor mEditor = mPrefs.edit();
        mEditor.putBoolean(key, bool);
        mEditor.commit();
    }

    public void setScriptAsk(boolean bool) {
        setBoolean(PREFS_UPDATE_SCRIPT, bool);
    }

    public boolean getScriptAsk() {
        return mPrefs.getBoolean(PREFS_UPDATE_SCRIPT, false);
    }

    void setDownFileList(Set<String> downlist) {
        if (downlist.size() > 0) {
            setStringSet(PREFS_DOWNLOAD_FILELIST, downlist);
        }
    }

    Set<String> getDownFileSet() {
        return mPrefs.getStringSet(PREFS_DOWNLOAD_FILELIST, null);
    }

    public boolean getBooleanVal(String key, boolean def) {
        return mPrefs.getBoolean(key, def);
    }

    public void setUpdatePath(String path) {
        setString(PREFS_UPDATE_FILEPATH, path);
    }

    public String getUpdatePath() {
        String path = mPrefs.getString(PREFS_UPDATE_FILEPATH, null);
        if (path != null) {
            path = onExternalPathSwitch(path);
        }
        return path;
    }

    public long getFileSize() {
        return mPrefs.getLong(PREFS_UPDATE_FILESIZE, 0);
    }

    public void saveFileSize(long fileSize) {
        setLong(PREFS_UPDATE_FILESIZE, fileSize);
    }

    public static Object getProperties(String key, String def) {
        String defVal = def;
        try {
            Class properClass = Class.forName("android.os.SystemProperties");
            Method getMethod = properClass.getMethod("get", String.class, String.class);
            defVal = (String) getMethod.invoke(null, key, def);
        } catch (Exception ex) {
            ex.printStackTrace();
        } finally {
            if (PermissionUtils.CanDebug()) Log.d(TAG, "getProperty:" + key + " defVal:" + defVal);
            return defVal;
        }

    }

    static boolean isUserVer() {
        String userVer = (String) getProperties("ro.secure", null);//SystemProperties.get ( "ro.secure", "" );
        String userDebug = (String) getProperties("ro.debuggable", "0");//SystemProperties.get ( "ro.debuggable", "" );
        String hideLocalUp = (String) getProperties("ro.otaupdate.local", null);//SystemProperties.get ( "ro.otaupdate.local", "" );
        if ((hideLocalUp != null) && hideLocalUp.equals("1")) {
            if ((userVer != null) && (userVer.length() > 0)) {
                return (userVer.trim().equals("1")) && (userDebug.equals("0"));
            }
        }
        return false;
    }

    public static boolean getAutoCheck() {
        String auto = (String) getProperties("ro.product.update.autocheck", "false");
        return ("true").equals(auto);
    }

    private ArrayList<File> getExternalStorageListSystemAPI() {
        Class<?> volumeInfoC = null;
        Method getvolume = null;
        Method isMount = null;
        Method getType = null;
        Method getPath = null;
        List<?> mVolumes = null;
        StorageManager mStorageManager = (StorageManager) mContext.getSystemService(Context.STORAGE_SERVICE);
        ArrayList<File> devList = new ArrayList<File>();
        try {
            volumeInfoC = Class.forName("android.os.storage.VolumeInfo");
            getvolume = StorageManager.class.getMethod("getVolumes");
            isMount = volumeInfoC.getMethod("isMountedReadable");
            getType = volumeInfoC.getMethod("getType");
            getPath = volumeInfoC.getMethod("getPath");
            mVolumes = (List<?>) getvolume.invoke(mStorageManager);

            for (Object vol : mVolumes) {
                if (vol != null && (boolean) isMount.invoke(vol) && (int) getType.invoke(vol) == 0) {
                    devList.add((File) getPath.invoke(vol));
                    if (PermissionUtils.CanDebug()) Log.d(TAG, "path.getName():" + getPath.invoke(vol));
                }
            }
        } catch (Exception ex) {
            ex.printStackTrace();
        } finally {
            return devList;
        }
    }

    public ArrayList<File> getStorageList(boolean extern) {

        if (Build.VERSION.SDK_INT >= 26 && Build.VERSION.SDK_INT < 29) {
            Class<?> fileListClass = null;
            Method getdev = null;
            ArrayList<File> devList = new ArrayList<File>();
            try {
                fileListClass = Class.forName("com.droidlogic.app.FileListManager");
                getdev = fileListClass.getMethod("getDevices");
                Constructor constructor = fileListClass.getConstructor(new Class[]{Context.class});
                Object fileListObj = constructor.newInstance(mContext);
                ArrayList<HashMap<String, Object>> devices = new ArrayList<HashMap<String, Object>>();
                devices = (ArrayList<HashMap<String, Object>>) getdev.invoke(fileListObj);

                for (HashMap<String, Object> dev : devices) {
                    if (PermissionUtils.CanDebug()) Log.d(TAG, "getDevice:" + dev.get("key_name") + "getType:" + dev.get("key_type"));
                    String name = (String) dev.get("key_name");
                    if (name.equals("Local Disk") && extern) {
                        continue;
                    }
                    devList.add(new File((String) dev.get("key_path")));
                }
            } catch (Exception ex) {
                ex.printStackTrace();
            } finally {
                return devList;
            }
        } else if (extern) {
            return getExternalStorageListSystemAPI();
        } else {
            return getMainDeviceListSystemAPI();
        }
    }

    private ArrayList<File> getMainDeviceListSystemAPI() {
        Class<?> volumeInfoC = null;
        Method getvolume = null;
        Method isMount = null;
        Method getType = null;
        Method getPath = null;
        List<?> mVolumes = null;
        StorageManager mStorageManager = (StorageManager) mContext.getSystemService(Context.STORAGE_SERVICE);
        ArrayList<File> devList = new ArrayList<File>();
        try {
            volumeInfoC = Class.forName("android.os.storage.VolumeInfo");
            getvolume = StorageManager.class.getMethod("getVolumes");
            isMount = volumeInfoC.getMethod("isMountedReadable");
            getType = volumeInfoC.getMethod("getType");
            getPath = volumeInfoC.getMethod("getPathForUser", int.class);
            mVolumes = (List<?>) getvolume.invoke(mStorageManager);

            for (Object vol : mVolumes) {
                if (vol != null && (boolean) isMount.invoke(vol) && ((int) getType.invoke(vol) == 0 || (int) getType.invoke(vol) == 2)) {
                    devList.add((File) getPath.invoke(vol, 0));
                    if (PermissionUtils.CanDebug()) Log.d(TAG, "path.getName():" + getPath.invoke(vol, 0));
                }
            }
        } catch (Exception ex) {
            ex.printStackTrace();
        } finally {
            return devList;
        }
    }

    public Object getDiskInfo(String filePath) {
        StorageManager mStorageManager = (StorageManager) mContext.getSystemService(Context.STORAGE_SERVICE);
        Class<?> volumeInfoC = null;
        Class<?> deskInfoC = null;
        Method getvolume = null;
        Method getDisk = null;
        Method isMount = null;
        Method getPath = null;
        Method getType = null;
        List<?> mVolumes = null;
        try {
            volumeInfoC = Class.forName("android.os.storage.VolumeInfo");
            deskInfoC = Class.forName("android.os.storage.DiskInfo");
            getvolume = StorageManager.class.getMethod("getVolumes");
            mVolumes = (List<?>) getvolume.invoke(mStorageManager);//mStorageManager.getVolumes();
            isMount = volumeInfoC.getMethod("isMountedReadable");
            getDisk = volumeInfoC.getMethod("getDisk");
            getPath = volumeInfoC.getMethod("getPath");
            getType = volumeInfoC.getMethod("getType");
            for (Object vol : mVolumes) {
                if (vol != null && (boolean) isMount.invoke(vol) && ((int) getType.invoke(vol) == 0)) {
                    Object info = getDisk.invoke(vol);
                    if (PermissionUtils.CanDebug()) Log.d(TAG, "getDiskInfo" + ((File) getPath.invoke(vol)).getAbsolutePath());
                    if (info != null && filePath.contains(((File) getPath.invoke(vol)).getAbsolutePath())) {
                        if (PermissionUtils.CanDebug()) Log.d(TAG, "getDiskInfo path.getName():" + ((File) getPath.invoke(vol)).getAbsolutePath());
                        return info;
                    }
                }
            }
        } catch (Exception ex) {
            ex.printStackTrace();
        }
        return null;

    }

    public String getTransPath(String inPath) {
        if (Build.VERSION.SDK_INT >= 26 && Build.VERSION.SDK_INT < 29) {
            Class<?> fileListClass = null;
            Method getdev = null;
            String outPath = inPath;
            try {
                fileListClass = Class.forName("com.droidlogic.app.FileListManager");
                getdev = fileListClass.getMethod("getDevices");
                Constructor constructor = fileListClass.getConstructor(new Class[]{Context.class});
                Object fileListObj = constructor.newInstance(mContext);
                ArrayList<HashMap<String, Object>> devices = new ArrayList<HashMap<String, Object>>();
                devices = (ArrayList<HashMap<String, Object>>) getdev.invoke(fileListObj);

                for (HashMap<String, Object> dev : devices) {
                    String name = (String) dev.get("key_name");
                    String volName = (String) dev.get("key_path");
                    if (outPath.contains(volName)) {
                        outPath = outPath.replace(volName, name);
                    }
                }
            } catch (Exception ex) {
                ex.printStackTrace();
            } finally {
                return outPath;
            }
        } else {
            return getNickName(inPath);
        }
    }

    private String getNickName(String inPath) {
        String outPath = inPath;
        String pathLast;
        String pathVol;
        int idx = -1;
        int len;
        Class<?> volumeInfoC = null;
        Method getBestVolumeDescription = null;
        Method getVolumes = null;
        Method getType = null;
        Method isMount = null;
        Method getPath = null;
        List<?> volumes = null;
        StorageManager storageManager = (StorageManager) mContext.getSystemService(Context.STORAGE_SERVICE);
        try {
            volumeInfoC = Class.forName("android.os.storage.VolumeInfo");
            getVolumes = StorageManager.class.getMethod("getVolumes");
            volumes = (List) getVolumes.invoke(storageManager);
            isMount = volumeInfoC.getMethod("isMountedReadable");
            getType = volumeInfoC.getMethod("getType");
            getPath = volumeInfoC.getMethod("getPath");
            for (Object vol : volumes) {
                if (vol != null && (boolean) isMount.invoke(vol) && (int) getType.invoke(vol) == 0) {
                    pathVol = ((File) getPath.invoke(vol)).getAbsolutePath();
                    idx = inPath.indexOf(pathVol);
                    if (idx != -1) {
                        len = pathVol.length();
                        pathLast = inPath.substring(idx + len);
                        getBestVolumeDescription = StorageManager.class.getMethod("getBestVolumeDescription", volumeInfoC);

                        outPath = ((String) getBestVolumeDescription.invoke(storageManager, vol)) + pathLast;
                    }
                }
            }
        } catch (Exception ex) {
            ex.printStackTrace();
        } finally {
            return outPath;
        }

    }

    private String getCanWritePath() {
        ArrayList<File> externalDevs = getStorageList(true);
        String filePath = "";
        for (int j = 0; (externalDevs != null) && j < externalDevs.size(); j++) {
            File dir = externalDevs.get(j);
            if (dir.isDirectory() && dir.canWrite()) {
                filePath = dir.getAbsolutePath();
                filePath += "/";
                break;
            }
        }
        return filePath;
    }

    private String getAttribute(String inPath) {
        if (Build.VERSION.SDK_INT >= 26) {
            Class<?> fileListClass = null;
            Method getdev = null;
            try {
                fileListClass = Class.forName("com.droidlogic.app.FileListManager");
                getdev = fileListClass.getMethod("getDevices");
                Constructor constructor = fileListClass.getConstructor(new Class[]{Context.class});
                Object fileListObj = constructor.newInstance(mContext);
                ArrayList<HashMap<String, Object>> devices = new ArrayList<HashMap<String, Object>>();
                devices = (ArrayList<HashMap<String, Object>>) getdev.invoke(fileListObj);

                for (HashMap<String, Object> dev : devices) {
                    String name = (String) dev.get("key_name");
                    String volName = (String) dev.get("key_path");
                    if (inPath.contains(volName)) {
                        String type = (String) dev.get("key_type");
                        if (type == "type_udisk") return "/udisk/";
                        else if (type == "type_sdcard") return "/sdcard/";
                    }
                }
            } catch (Exception ex) {
                ex.printStackTrace();
            }
            return "/cache/";
        } else {
            return getAttributeonN(inPath);
        }
    }

    private String getAttributeonN(String inPath) {
        String res = "";
        Class<?> deskInfoClass = null;
        Method isSd = null;
        Method isUsb = null;
        Object info = getDiskInfo(inPath);
        if (info == null) {
            res += "/cache/";
        } else {
            try {
                deskInfoClass = Class.forName("android.os.storage.DiskInfo");
                isSd = deskInfoClass.getMethod("isSd");
                isUsb = deskInfoClass.getMethod("isUsb");
                if (info != null) {
                    if ((boolean) isSd.invoke(info)) {
                        res += "/sdcard/";
                    } else if ((boolean) isUsb.invoke(info)) {
                        res += "/udisk/";
                    } else {
                        res += "/cache/";
                    }
                } else {
                    res += "/cache/";
                }

            } catch (Exception ex) {
                ex.printStackTrace();
                res += "/cache/";
            }
        }
        return res;
    }

    public boolean inLocal(String fullpath) {
        String updateFilePath = getAttribute(fullpath);
        if (updateFilePath.startsWith("/data") || updateFilePath.startsWith("/cache")
                || updateFilePath.startsWith("/sdcard")) {
            return true;
        }
        return false;
    }

    public String onExternalPathSwitch(String filePath) {
        if (filePath.contains(EXTERNAL_STORAGE) || filePath.contains(EXTERNAL_STORAGE.toUpperCase())) {
            String path = getCanWritePath();
            if (path != null && !path.isEmpty()) {
                filePath = filePath.replace(EXTERNAL_STORAGE, path);
            }
        }
        return filePath;
    }


    public static void copyFile(String fileFromPath, String fileToPath) throws Exception {
        copyFile (fileFromPath, fileToPath, null);

    }

    private static boolean checkZip(String file) {
        boolean checkResult = false;
        String myDevice = Build.DEVICE;
        String myType = Build.TYPE;
        String myTags = Build.TAGS;
        ZipFile zip = null;
        try {
            zip = new ZipFile(new File(file));
            for (Enumeration entries = zip.entries(); entries.hasMoreElements(); ) {
                ZipEntry entry = (ZipEntry) entries.nextElement();
                String zipEntryName = entry.getName();
                if (zipEntryName.equals("META-INF/com/android/metadata")) {
                    InputStream in = zip.getInputStream(entry);
                    BufferedReader bf =
                            new BufferedReader(new InputStreamReader(in));
                    String lines = bf.readLine();
                    while (lines != null) {
                        if (lines.startsWith("post-build=") && lines.contains(myDevice) && lines.contains(myType) && lines.contains(myTags)) {
                            checkResult = true;
                            break;
                        }
                        lines = bf.readLine();
                    }
                    in.close();
                    break;
                }
            }
            zip.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
        return checkResult;
    }

    public static void copyFile(String fileFromPath, String fileToPath, CallbackChecker callback) throws Exception {
        if (!checkZip(fileFromPath)) {
            throw new RuntimeException("File Error");
        }
        FileInputStream fi = null;
        FileOutputStream fo = null;
        FileChannel in = null;
        FileChannel out = null;
        if (PermissionUtils.CanDebug()) Log.d(TAG, "copyFile from " + fileFromPath + " to " + fileToPath);
        try {
            File outFile = new File(fileToPath);
            if (outFile.exists()) {
                outFile.delete();
            }
            outFile.createNewFile();
            outFile.setReadable(true,false);
            outFile.setWritable(true,false);
            fi = new FileInputStream(new File(fileFromPath));
            in = fi.getChannel();
            fo = new FileOutputStream(new File(fileToPath));
            out = fo.getChannel();
            long size = in.size();
            long pos = 0;
            long count = 0;
            while ( pos < size ) {
                if (callback == null || !callback.checkrunning()) {
                    count = size - pos > FILE_COPY_BUFFER_SIZE ? FILE_COPY_BUFFER_SIZE : size - pos;
                    pos += out.transferFrom(in, pos, count);
                }else {
                    Log.d("Update","copy file interrupt");
                    break;
                }
            }
            Log.d("Update","already copy "+pos+" from "+size);
            //in.transferTo(0, in.size(), out);
        } finally {
            try {
                fi.close();
                fo.close();
                in.close();
                out.close();
            } catch (Exception ex) {
            }
        }
    }
    public interface CallbackChecker {
        public boolean checkrunning();
    }
    public void disableUI(boolean disable,Context cxt) {
        PackageManager pmg=cxt.getPackageManager();
        ComponentName component=new ComponentName(cxt,MainActivity.class);
        int res = pmg.getComponentEnabledSetting(component);
        if ( disable ) {
            pmg.setComponentEnabledSetting(component, PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
                    PackageManager.DONT_KILL_APP);
        }else{
            pmg.setComponentEnabledSetting(component, PackageManager.COMPONENT_ENABLED_STATE_DEFAULT,
                    PackageManager.DONT_KILL_APP);
        }
    }
}
