/**
 * Copyright 2020 Google Inc. All Rights Reserved.
 */
package com.sample.googletvcustomizationsample;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.content.UriMatcher;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.os.ParcelFileDescriptor;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

/**
 * Content provider that provides sample customization for Google TV
 */
public class GoogleTVCustomizationSampleProvider extends ContentProvider {
    private static final String TAG = "GoogleTVCustomizationSampleProvider";
    private static final String CONFIG_AUTHORITY = "tvlauncher.config";

    private static final String CONFIGURATION_DATA = "configuration";

    private static final int MATCH_CONFIGURATION = 1;


    private static final UriMatcher sUriMatcher;

    static {
        sUriMatcher = new UriMatcher(UriMatcher.NO_MATCH);
        sUriMatcher.addURI(CONFIG_AUTHORITY, CONFIGURATION_DATA, MATCH_CONFIGURATION);
    }

    @Nullable
    @Override
    public ParcelFileDescriptor openFile(Uri uri, String mode) throws
            FileNotFoundException {
        final InputStream stream;
        PipeDataWriter writer;

        switch (sUriMatcher.match(uri)) {
            case MATCH_CONFIGURATION:
                // In this example, the configuration file is static and resides in
                // the res/raw folder of the app.
                stream = getContext().getResources().openRawResource(
                        R.raw.config);
                break;

            default:
                throw new UnsupportedOperationException("Unknown uri: " + uri);
        }
        writer = new PipeDataWriter() {
            @Override
            public void writeDataToPipe(@NonNull ParcelFileDescriptor output,
                                        @NonNull Uri uri, @NonNull String mimeType,
                                        @Nullable Bundle opts, @Nullable Object args) {
                try (FileOutputStream out = new FileOutputStream(
                        output.getFileDescriptor())) {
                    byte[] buffer = new byte[8192];
                    int count;
                    while ((count = stream.read(buffer)) != -1) {
                        out.write(buffer, 0, count);
                    }
                } catch (IOException e) {
                    Log.e(TAG,"Failed to send file " + uri, e);
                }
            }
        };
        return openPipeHelper(uri, "text/xml", null, null, writer);
    }

    @Override
    public boolean onCreate() {
        return true;
    }

    @Nullable
    @Override
    public Cursor query(Uri uri, String[] projection, String selection,
                        String[] selectionArgs, String sortOrder) {
        throw new UnsupportedOperationException();
    }

    @Nullable
    @Override
    public Uri insert(Uri uri, ContentValues values) {
        throw new UnsupportedOperationException();
    }

    @Override
    public int delete(Uri uri, String selection, String[] selectionArgs) {
        throw new UnsupportedOperationException();
    }

    @Override
    public int update(Uri uri, ContentValues values, String selection,
                      String[] selectionArgs) {
        throw new UnsupportedOperationException();
    }

    @Override
    public String getType(@NonNull Uri uri) {
        throw new UnsupportedOperationException();
    }
}
