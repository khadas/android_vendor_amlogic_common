package com.droidlogic.updater.service;

import android.app.IntentService;
import android.content.Intent;
import android.content.Context;
import android.os.Bundle;
import android.os.Build;
import android.os.Handler;
import android.os.UpdateEngine;
import android.os.ResultReceiver;
import android.os.RecoverySystem;
import android.util.Log;
import android.util.Pair;
import static com.droidlogic.updater.util.PackageFiles.COMPATIBILITY_ZIP_FILE_NAME;
import static com.droidlogic.updater.util.PackageFiles.OTA_PACKAGE_DIR;
import static com.droidlogic.updater.util.PackageFiles.PAYLOAD_BINARY_FILE_NAME;
import static com.droidlogic.updater.util.PackageFiles.PAYLOAD_PROPERTIES_FILE_NAME;
import com.droidlogic.updater.PayloadSpec;
import com.droidlogic.updater.UpdateConfig;
import com.droidlogic.updater.util.UpdateEngineProperties;
import com.droidlogic.updater.util.FileDownloader;
import com.droidlogic.updater.util.PackageFiles;
import com.droidlogic.updater.util.PayloadSpecs;
import com.droidlogic.updater.util.UpdateConfigs;
import org.json.JSONException;
import com.google.common.collect.ImmutableSet;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.File;
import java.io.PrintWriter;
import java.util.Arrays;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.Files;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLConnection;
import java.util.Optional;
import java.util.ArrayList;

import com.droidlogic.updater.util.PermissionUtils;

public class PrepareUpdateService extends IntentService {
    /**
     * UpdateResultCallback result codes.
     */
    public static final int RESULT_CODE_SUCCESS = 0;
    public static final int RESULT_CODE_ERROR = 1;
    public static String ACTION_CHECK_UPDATE = "com.droidlogic.checkup.update";
    public static final String EXTRA_PARAM_RESULT_RECEIVER = "result-receiver";
    public static final String BUNDLE_PARAM_CONFIG = "param_config";
    public static final String EXTRA_RETURN_STATUS = "result-status";
    public static final String TAG = "ABUpdate";
    public static final String EXTRA_PARAM_CONFIG = "config";
    private static CHECKUP_STATUS mCurrentStatus = CHECKUP_STATUS.IDLE;
    private Handler mLocalHandler = new Handler();
    private static int CHECK_STATUS = 3000;
    public static boolean mStopService = false;
    private static ArrayList<FileDownloader> mDownloaders = new ArrayList<FileDownloader>(10);
    //check update task status
    public enum CHECKUP_STATUS{
        IDLE,CHECKING,NEWEST,UPDATE
    };
    private Runnable checkTime = new Runnable(){
        public void run() {
            if (mStopService) {
                PrepareUpdateService.this.stopSelf();
            }else {
                 mLocalHandler.postDelayed(checkTime,CHECK_STATUS);
            }
        }
    };
    public PrepareUpdateService() {
        super("PrepareUpdateService");
    }
    public interface CheckupResultCallback{
        public void onReceiveResult(int status, UpdateConfig config);
    }
    /*
    * start Service to checkup new version
    * */
    public static void startCheckup(Context context,Handler handler,CheckupResultCallback resultReceiver) {
        if (PermissionUtils.CanDebug()) Log.d(TAG,"startCheckup");
        mCurrentStatus = CHECKUP_STATUS.IDLE;
        CallbackCheckupReceiverWrapper wrapper = new CallbackCheckupReceiverWrapper(handler,resultReceiver);
        Intent intent = new Intent(context, PrepareUpdateService.class);
        intent.setAction(ACTION_CHECK_UPDATE);
        intent.putExtra(EXTRA_PARAM_RESULT_RECEIVER,wrapper);
        context.startService(intent);
    }
    public static void stopCheck(){
        mStopService = true;
        for (int i = 0; i < mDownloaders.size(); i++) {
            mDownloaders.get(i).stopDown();
        }
        mDownloaders.clear();
        mCurrentStatus = CHECKUP_STATUS.IDLE;
    }
    /**
     * This interface is used to send results from {@link PrepareUpdateService} to
     * {@code MainActivity}.
     */
    public interface UpdateResultCallback {
        /**
         * Invoked when files are downloaded and payload spec is constructed.
         *
         * @param resultCode  result code, values are defined in {@link PrepareUpdateService}
         * @param payloadSpec prepared payload spec for streaming update
         */
        void onReceiveResult(int resultCode, PayloadSpec payloadSpec);
    }

    /**
     * Starts PrepareUpdateService.
     *
     * @param context        application context
     * @param config         update config
     * @param resultCallback callback that will be called when the update is ready to be installed
     */
    public static void startService(Context context,
            UpdateConfig config,
            Handler handler,
            UpdateResultCallback resultCallback) {
        ResultReceiver receiver = new CallbackResultReceiver(handler, resultCallback);
        Intent intent = new Intent(context, PrepareUpdateService.class);
        intent.putExtra(EXTRA_PARAM_CONFIG, config);
        intent.putExtra(EXTRA_PARAM_RESULT_RECEIVER, receiver);
        context.startService(intent);
    }
    public  static class CallbackCheckupReceiverWrapper extends ResultReceiver {
        private CheckupResultCallback mCheckupcallback;
        public CallbackCheckupReceiverWrapper(Handler handler,CheckupResultCallback checkupcallback) {
            super(handler);
            mCheckupcallback = checkupcallback;
        }

        @Override
        public void onReceiveResult(int status, Bundle bundle) {
            if (bundle != null) {
                UpdateConfig config = bundle.getParcelable(BUNDLE_PARAM_CONFIG);
                mCheckupcallback.onReceiveResult(status, config);
            }else {
                mCheckupcallback.onReceiveResult(status, null);
            }
        }
    }
    @Override
    protected void onHandleIntent(Intent intent) {
        if (PermissionUtils.CanDebug()) Log.d(TAG,"onHandleIntent");
        if (intent != null) {
            final String action = intent.getAction();
            if (ACTION_CHECK_UPDATE.equals(action)) {
                ResultReceiver resultReceiver = intent.getParcelableExtra(EXTRA_PARAM_RESULT_RECEIVER);
                Bundle extrabunder = intent.getExtras();
                Log.d(TAG,"onHandleIntent"+intent+"intent:"+extrabunder.getParcelable(EXTRA_PARAM_RESULT_RECEIVER)+"intent.getextra"+extrabunder.getShort("key"));
                Bundle bundle = new Bundle();
                try {
                    mStopService = false;
                    Pair<CHECKUP_STATUS,UpdateConfig>  config = checkCfg();
                    bundle.putParcelable(BUNDLE_PARAM_CONFIG,config.second);
                    resultReceiver.send(config.first.ordinal(),bundle);
                } catch (Exception e) {
                    e.printStackTrace();
                    if (PermissionUtils.CanDebug()) Log.e(TAG, "Failed to prepare streaming update", e);
                    bundle.putParcelable(BUNDLE_PARAM_CONFIG,null);
                    resultReceiver.send(CHECKUP_STATUS.NEWEST.ordinal(), null);
                }
            }else{
                if (PermissionUtils.CanDebug()) Log.d(TAG, "On handle intent is called");
                mLocalHandler.postDelayed(checkTime,CHECK_STATUS);
                UpdateConfig config = intent.getParcelableExtra(EXTRA_PARAM_CONFIG);
                ResultReceiver resultReceiver = intent.getParcelableExtra(EXTRA_PARAM_RESULT_RECEIVER);

                try {
                    PayloadSpec spec = execute(config);
                    resultReceiver.send(RESULT_CODE_SUCCESS, CallbackResultReceiver.createBundle(spec));
                } catch (Exception e) {
                    if (PermissionUtils.CanDebug()) Log.d(TAG,"mCurrentStatus:"+mCurrentStatus);
                    if (PermissionUtils.CanDebug()) Log.e(TAG, "Failed to prepare streaming update", e);
                    resultReceiver.send(RESULT_CODE_ERROR, null);
                }
            }
        }
    }
    public String createParams(){
        StringBuilder params = new StringBuilder();
        params.append("sn="+Build.getSerial());
        params.append("&bdate="+Build.TIME/1000);
        params.append("&os=android-"+Build.VERSION.RELEASE);
        params.append("&device="+Build.DEVICE);
        return params.toString();
    }
    private Pair<CHECKUP_STATUS,UpdateConfig> checkCfg()throws MalformedURLException, Exception {
        CHECKUP_STATUS status = CHECKUP_STATUS.IDLE;
        mCurrentStatus = CHECKUP_STATUS.CHECKING;
        String param ="/chk?"+createParams();
        if (PermissionUtils.CanDebug()) Log.d(TAG,"url: "+UpdateEngineProperties.SERVER_URI+param);
        URL url = new URL(UpdateEngineProperties.SERVER_URI+param);
        URLConnection connection = url.openConnection();
        connection.connect();

        status= CHECKUP_STATUS.CHECKING;
        BufferedReader reader = new BufferedReader(new InputStreamReader(connection.getInputStream()));
        StringBuilder result = new StringBuilder();
        String line;
        while ( (line = reader.readLine() ) != null ) {
            result.append(line);
            if (PermissionUtils.CanDebug()) Log.d(TAG,line);
        }
        UpdateConfig config  = UpdateConfig.fromJson(result.toString());
        if (config.getName() != null) {
            status = CHECKUP_STATUS.UPDATE;
        } else {
            status = CHECKUP_STATUS.NEWEST;
        }
        mCurrentStatus = status;
        Pair<CHECKUP_STATUS,UpdateConfig> ret = new Pair<CHECKUP_STATUS,UpdateConfig>(status,config);
        return ret;
    }
    public static int getStatus(){
        return mCurrentStatus.ordinal();
    }
    /**
     * 1. Downloads files for streaming updates.
     * 2. Makes sure required files are present.
     * 3. Checks OTA package compatibility with the device.
     * 4. Constructs {@link PayloadSpec} for streaming update.
     */
    private PayloadSpec execute(UpdateConfig config)
            throws IOException, PreparationFailedException {

        if (config.getAbConfig().getVerifyPayloadMetadata()) {
            if (PermissionUtils.CanDebug()) Log.i(TAG, "Verifying payload metadata with UpdateEngine.");
            if (!verifyPayloadMetadata(config)) {
                throw new PreparationFailedException("Payload metadata is not compatible");
            }
        }

        if (config.getInstallType() == UpdateConfig.AB_INSTALL_TYPE_NON_STREAMING) {
            return mPayloadSpecs.forNonStreaming(config.getUpdatePackageFile());
        }
        downloadPreStreamingFiles(config, PackageFiles.OTA_PACKAGE_DIR);
        Optional<UpdateConfig.PackageFile> payloadBinary =
                UpdateConfigs.getPropertyFile(PAYLOAD_BINARY_FILE_NAME, config);

        if (!payloadBinary.isPresent()) {
            throw new PreparationFailedException(
                    "Failed to find " + PAYLOAD_BINARY_FILE_NAME + " in config");
        }

        if (!UpdateConfigs.getPropertyFile(PackageFiles.PAYLOAD_PROPERTIES_FILE_NAME, config).isPresent()
                || !Paths.get(PackageFiles.OTA_PACKAGE_DIR, PackageFiles.PAYLOAD_PROPERTIES_FILE_NAME).toFile().exists()) {
            throw new IOException(PackageFiles.PAYLOAD_PROPERTIES_FILE_NAME + " not found");
        }

        File compatibilityFile = Paths.get(PackageFiles.OTA_PACKAGE_DIR, PackageFiles.COMPATIBILITY_ZIP_FILE_NAME).toFile();
        if (compatibilityFile.isFile()) {
            if (PermissionUtils.CanDebug()) Log.i(TAG, "Verifying OTA package for compatibility with the device");
            if (!verifyPackageCompatibility(compatibilityFile)) {
                throw new PreparationFailedException(
                        "OTA package is not compatible with this device");
            }
        }

        return mPayloadSpecs.forStreaming(config.getUrl(),
                payloadBinary.get().getOffset(),
                payloadBinary.get().getSize(),
                Paths.get(PackageFiles.OTA_PACKAGE_DIR, PackageFiles.PAYLOAD_PROPERTIES_FILE_NAME).toFile());
    }

    /**
     * Downloads only payload_metadata.bin and verifies with
     * {@link UpdateEngine#verifyPayloadMetadata}.
     * Returns {@code true} if the payload is verified or the result is unknown because of
     * exception from UpdateEngine.
     * By downloading only small portion of the package, it allows to verify if UpdateEngine
     * will install the update.
     */
    private boolean verifyPayloadMetadata(UpdateConfig config) {
        Optional<UpdateConfig.PackageFile> metadataPackageFile =
                Arrays.stream(config.getAbConfig().getPropertyFiles())
                        .filter(p -> p.getFilename().equals(
                                PackageFiles.PAYLOAD_METADATA_FILE_NAME))
                        .findFirst();
        if (!metadataPackageFile.isPresent()) {
            if (PermissionUtils.CanDebug()) Log.w(TAG, String.format("ab_config.property_files doesn't contain %s",
                    PackageFiles.PAYLOAD_METADATA_FILE_NAME));
            return true;
        }
        Path metadataPath = Paths.get(OTA_PACKAGE_DIR, PackageFiles.PAYLOAD_METADATA_FILE_NAME);
        try {
            Files.deleteIfExists(metadataPath);
            FileDownloader d = new FileDownloader(
                    config.getUrl(),
                    metadataPackageFile.get().getOffset(),
                    metadataPackageFile.get().getSize(),
                    metadataPath.toFile());
            mDownloaders.add(d);
            d.download();
        } catch (IOException e) {
            if (PermissionUtils.CanDebug()) Log.w(TAG, String.format("Downloading %s from %s failed",
                    PackageFiles.PAYLOAD_METADATA_FILE_NAME,
                    config.getUrl()), e);
            return true;
        }
        try {
            return mUpdateEngine.verifyPayloadMetadata(metadataPath.toAbsolutePath().toString());
        } catch (Exception e) {
            if (PermissionUtils.CanDebug()) Log.w(TAG, "UpdateEngine#verifyPayloadMetadata failed", e);
            return true;
        }
    }

    /**
     * Downloads files defined in {@link UpdateConfig#getAbConfig()}
     * and exists in {@code PRE_STREAMING_FILES_SET}, and put them
     * in directory {@code dir}.
     *
     * @throws IOException when can't download a file
     */
    private void downloadPreStreamingFiles(UpdateConfig config, String dir)
            throws IOException {
        if (PermissionUtils.CanDebug()) Log.d(TAG, "Deleting existing files from " + dir);
        for (String file : PRE_STREAMING_FILES_SET) {
            try {
                Files.deleteIfExists(Paths.get(OTA_PACKAGE_DIR, file));
            }catch(Exception e) {
                if (PermissionUtils.CanDebug()) Log.w(TAG, "UpdateEngine#deleteIfExists failed", e);
            }
        }
        if (PermissionUtils.CanDebug()) Log.d(TAG, "Downloading files to " + dir);
        for (UpdateConfig.PackageFile file : config.getAbConfig().getPropertyFiles()) {
            if (PRE_STREAMING_FILES_SET.contains(file.getFilename()) && mCurrentStatus.ordinal() > 2) {
                if (PermissionUtils.CanDebug()) Log.d(TAG, "Downloading file " + file.getFilename());
                FileDownloader downloader = new FileDownloader(
                        config.getUrl(),
                        file.getOffset(),
                        file.getSize(),
                        Paths.get(dir, file.getFilename()).toFile());
                mDownloaders.add(downloader);
                downloader.download();
            }
        }
    }

    /**
     * @param file physical location of {@link PackageFiles#COMPATIBILITY_ZIP_FILE_NAME}
     * @return true if OTA package is compatible with this device
     */
    private boolean verifyPackageCompatibility(File file) {
        try {
            return RecoverySystem.verifyPackageCompatibility(file);
        } catch (IOException e) {
            if (PermissionUtils.CanDebug()) Log.e(TAG, "Failed to verify package compatibility", e);
            return false;
        }
    }

    /**
     * Used by {@link PrepareUpdateService} to pass {@link PayloadSpec}
     * to {@link UpdateResultCallback#onReceiveResult}.
     */
    private static class CallbackResultReceiver extends ResultReceiver {

        static Bundle createBundle(PayloadSpec payloadSpec) {
            Bundle b = new Bundle();
            b.putSerializable(BUNDLE_PARAM_PAYLOAD_SPEC, payloadSpec);
            return b;
        }

        private static final String BUNDLE_PARAM_PAYLOAD_SPEC = "payload-spec";

        private UpdateResultCallback mUpdateResultCallback;

        CallbackResultReceiver(Handler handler, UpdateResultCallback updateResultCallback) {
            super(handler);
            this.mUpdateResultCallback = updateResultCallback;
        }

        @Override
        protected void onReceiveResult(int resultCode, Bundle resultData) {
            PayloadSpec payloadSpec = null;
            if (resultCode == RESULT_CODE_SUCCESS) {
                payloadSpec = (PayloadSpec) resultData.getSerializable(BUNDLE_PARAM_PAYLOAD_SPEC);
            }
            mUpdateResultCallback.onReceiveResult(resultCode, payloadSpec);
        }
    }
    /**
     * The files that should be downloaded before streaming.
     */
    private static final ImmutableSet<String> PRE_STREAMING_FILES_SET =
            ImmutableSet.of(
                    PackageFiles.CARE_MAP_FILE_NAME,
                    PackageFiles.COMPATIBILITY_ZIP_FILE_NAME,
                    PackageFiles.METADATA_FILE_NAME,
                    PackageFiles.PAYLOAD_PROPERTIES_FILE_NAME
            );

    private final PayloadSpecs mPayloadSpecs = new PayloadSpecs();
    private final UpdateEngine mUpdateEngine = new UpdateEngine();

    private static class PreparationFailedException extends Exception {
        PreparationFailedException(String message) {
            super(message);
        }
    }
}
