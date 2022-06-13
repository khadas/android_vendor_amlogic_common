package com.droidlogic.updater;

import android.os.Parcel;
import android.os.Parcelable;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.Serializable;
import java.util.ArrayList;
import java.util.Optional;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

import com.droidlogic.updater.util.PermissionUtils;

/**
 * An update description. It will be parsed from JSON, which is intended to
 * be sent from server to the update app, but in this sample app it will be stored on the device.
 */
public class UpdateConfig implements Parcelable {

    public static final String TARTEPATH = "/data/ota_package/update.zip";
    public static final int AB_INSTALL_TYPE_NON_STREAMING = 0;
    public static final int AB_INSTALL_TYPE_STREAMING = 1;

    public static final Parcelable.Creator<UpdateConfig> CREATOR =
            new Parcelable.Creator<UpdateConfig>() {
                @Override
                public UpdateConfig createFromParcel(Parcel source) {
                    return new UpdateConfig(source);
                }

                @Override
                public UpdateConfig[] newArray(int size) {
                    return new UpdateConfig[size];
                }
            };

    public static UpdateConfig readDefCfg(String filepath) throws IOException {
        ArrayList<String> list = new ArrayList<>(6);
        list.add("payload_properties.txt");
        list.add("payload.bin");
        list.add("payload_metadata.bin");
        list.add("care_map.txt");
        list.add("compatibility.zip");
        list.add("metadata");
        UpdateConfig c = new UpdateConfig();
        c.mName = "LocalUpdate";

        c.mAbInstallType = AB_INSTALL_TYPE_NON_STREAMING;
        ArrayList<PackageFile> propertyFiles = new ArrayList<>();
        File f = new File(filepath);
        c.mUrl = "file://" + TARTEPATH;
        ZipInputStream zipInput = new ZipInputStream(new FileInputStream(f));
        ZipEntry zipEntry = null;
        long startPos = 0;
        boolean verifyPayloadMetadata = false;
        while ((zipEntry = zipInput.getNextEntry()) != null) {
            if (list.contains(zipEntry.getName())) {
                propertyFiles.add(new PackageFile(
                        zipEntry.getName(),
                        startPos,
                        zipEntry.getCompressedSize()));
            }
            if (zipEntry.getName().equals("payload_metadata.bin")) {
                verifyPayloadMetadata = true;
            }
            startPos += zipEntry.getCompressedSize();
        }
        if (propertyFiles.size() == 0) {
            //error zip file
            throw new IOException("error file");
        }
        c.mAbConfig = new AbConfig(
                false,
                verifyPayloadMetadata,
                propertyFiles.toArray(new PackageFile[0]),
                null);
        return c;

    }

    /**
     * parse update config from json
     */
    public static UpdateConfig fromJson(String json) {
        UpdateConfig c = new UpdateConfig();
        try {
            JSONObject o = new JSONObject(json);
            c.mName = o.getString("name");
            c.mUrl = o.getString("url");
            switch (o.getString("ab_install_type")) {
                case AB_INSTALL_TYPE_NON_STREAMING_JSON:
                    c.mAbInstallType = AB_INSTALL_TYPE_NON_STREAMING;
                    break;
                case AB_INSTALL_TYPE_STREAMING_JSON:
                    c.mAbInstallType = AB_INSTALL_TYPE_STREAMING;
                    break;
                default:
                    throw new JSONException("Invalid type, expected either "
                            + "NON_STREAMING or STREAMING, got " + o.getString("ab_install_type"));
            }

            // TODO: parse only for A/B updates when non-A/B is implemented
            JSONObject ab = o.getJSONObject("ab_config");
            boolean forceSwitchSlot = false;// ab.getBoolean("force_switch_slot");
            boolean verifyPayloadMetadata = ab.getBoolean("verify_payload_metadata");
            ArrayList<PackageFile> propertyFiles = new ArrayList<>();
            if (ab.has("property_files")) {
                JSONArray propertyFilesJson = ab.getJSONArray("property_files");
                for (int i = 0; i < propertyFilesJson.length(); i++) {
                    JSONObject p = propertyFilesJson.getJSONObject(i);
                    propertyFiles.add(new PackageFile(
                            p.getString("filename"),
                            p.getLong("offset"),
                            p.getLong("size")));
                }
            }
            String authorization = ab.optString("authorization", null);
            c.mAbConfig = new AbConfig(
                    false,
                    verifyPayloadMetadata,
                    propertyFiles.toArray(new PackageFile[0]),
                    authorization);

            c.mRawJson = json;
        } catch (JSONException ex) {
           // ex.printStackTrace();
        } finally {
            return c;
        }

    }

    /**
     * these strings are represent types in JSON config files
     */
    private static final String AB_INSTALL_TYPE_NON_STREAMING_JSON = "NON_STREAMING";
    private static final String AB_INSTALL_TYPE_STREAMING_JSON = "STREAMING";

    /**
     * name will be visible on UI
     */
    private String mName;

    /**
     * update zip file URI, can be https:// or file://
     */
    private String mUrl;

    /**
     * non-streaming (first saves locally) OR streaming (on the fly)
     */
    private int mAbInstallType;

    /**
     * A/B update configurations
     */
    private AbConfig mAbConfig;

    private String mRawJson;

    protected UpdateConfig() {
    }

    protected UpdateConfig(Parcel in) {
        this.mName = in.readString();
        this.mUrl = in.readString();
        this.mAbInstallType = in.readInt();
        this.mAbConfig = (AbConfig) in.readSerializable();
        this.mRawJson = in.readString();
    }

    public UpdateConfig(String name, String url, int installType) {
        this.mName = name;
        this.mUrl = url;
        this.mAbInstallType = installType;
    }

    public String getName() {
        return mName;
    }

    public String getUrl() {
        return mUrl;
    }

    public String getRawJson() {
        return mRawJson;
    }

    public int getInstallType() {
        return mAbInstallType;
    }

    public AbConfig getAbConfig() {
        return mAbConfig;
    }

    /**
     * @return File object for given url
     */
    public File getUpdatePackageFile() {
        if (mAbInstallType != AB_INSTALL_TYPE_NON_STREAMING) {
            throw new RuntimeException("Expected non-streaming install type");
        }
        if (!mUrl.startsWith("file://")) {
            throw new RuntimeException("url is expected to start with file://");
        }
        return new File(mUrl.substring(7, mUrl.length()));
    }

    @Override
    public String toString() {
        return "UpdateConfig{" +
                "mName='" + mName + '\'' +
                ", mUrl='" + mUrl + '\'' +
                ", mAbInstallType=" + mAbInstallType +
                ", mAbConfig=" + mAbConfig.toString() +
                ", mRawJson='" + mRawJson + '\'' +
                '}';
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeString(mName);
        dest.writeString(mUrl);
        dest.writeInt(mAbInstallType);
        dest.writeSerializable(mAbConfig);
        dest.writeString(mRawJson);
    }

    /**
     * Description of a file in an OTA package zip file.
     */
    public static class PackageFile implements Serializable {

        private static final long serialVersionUID = 31043L;

        /**
         * filename in an archive
         */
        private String mFilename;

        /**
         * defines beginning of update data in archive
         */
        private long mOffset;

        /**
         * size of the update data in archive
         */
        private long mSize;

        public PackageFile(String filename, long offset, long size) {
            this.mFilename = filename;
            this.mOffset = offset;
            this.mSize = size;
        }

        public String getFilename() {
            return mFilename;
        }

        public long getOffset() {
            return mOffset;
        }

        public long getSize() {
            return mSize;
        }

        public String toString() {
            return "{ mFilename: " + mFilename + " ,mOffset: " + mOffset + " ,mSize: " + mSize + " }";
        }
    }

    /**
     * A/B (seamless) update configurations.
     */
    public static class AbConfig implements Serializable {

        private static final long serialVersionUID = 31044L;

        /**
         * if set true device will boot to new slot, otherwise user manually
         * switches slot on the screen.
         */
        private boolean mForceSwitchSlot;

        /**
         * if set true device will boot to new slot, otherwise user manually
         * switches slot on the screen.
         */
        private boolean mVerifyPayloadMetadata;

        /**
         * defines beginning of update data in archive
         */
        private PackageFile[] mPropertyFiles;

        /**
         * SystemUpdaterSample receives the authorization token from the OTA server, in addition
         * to the package URL. It passes on the info to update_engine, so that the latter can
         * fetch the data from the package server directly with the token.
         */
        private String mAuthorization;

        public AbConfig(
                boolean forceSwitchSlot,
                boolean verifyPayloadMetadata,
                PackageFile[] propertyFiles,
                String authorization) {
            this.mForceSwitchSlot = forceSwitchSlot;
            this.mVerifyPayloadMetadata = verifyPayloadMetadata;
            this.mPropertyFiles = propertyFiles;
            this.mAuthorization = authorization;
        }

        public boolean getForceSwitchSlot() {
            return mForceSwitchSlot;
        }

        public boolean getVerifyPayloadMetadata() {
            return mVerifyPayloadMetadata;
        }

        public PackageFile[] getPropertyFiles() {
            return mPropertyFiles;
        }

        public Optional<String> getAuthorization() {
            return mAuthorization == null ? Optional.empty() : Optional.of(mAuthorization);
        }

        public String toString() {
            StringBuilder builder = new StringBuilder();
            for (PackageFile f : mPropertyFiles) {
                builder.append(f.toString());
            }
            String propstr = builder.toString();
            return "AbConfig{" +
                    "mForceSwitchSlot='" + mForceSwitchSlot + '\'' +
                    ", mVerifyPayloadMetadata='" + mVerifyPayloadMetadata + '\'' +
                    ", mPropertyFiles=" + propstr +
                    ", mAuthorization=" + mAuthorization +
                    '}';
        }
    }

}
