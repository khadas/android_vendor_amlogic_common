/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License
 */
package com.droidlogic.launcher.provider;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.UriMatcher;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;
import android.os.Bundle;
import android.os.ParcelFileDescriptor;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.util.Log;
import com.droidlogic.launcher.provider.R;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

/**
 * Content provider that allows the TV Launcher to read configuration files.
 */
public class ConfigurationProvider extends ContentProvider {

    private static final String TAG = "ConfigurationProvider";

    private static final String CONFIG_AUTHORITY = "tvlauncher.config";
    private static final String WIDGET_AUTHORITY = "tvlauncher.widget";
    private static final String CONFIGURATION_DATA = "configuration";
    private static final String WIDGET_DATA = "widget";

    private static final String CONFIG_FILE_PREFIX = "configuration_";

    private static final int MATCH_CONFIGURATION = 1;
    private static final int MATCH_WIDGET = 3;

    private static final UriMatcher URI_MATCHER;

    static {
        URI_MATCHER = new UriMatcher(UriMatcher.NO_MATCH);
        URI_MATCHER.addURI(CONFIG_AUTHORITY, CONFIGURATION_DATA, MATCH_CONFIGURATION);
        URI_MATCHER.addURI(WIDGET_AUTHORITY, WIDGET_DATA, MATCH_WIDGET);
    }

    @Nullable
    @Override
    public ParcelFileDescriptor openFile(@NonNull Uri uri, @NonNull String mode)
            throws FileNotFoundException {
        final InputStream stream;
        PipeDataWriter writer;
        switch (URI_MATCHER.match(uri)) {
            case MATCH_CONFIGURATION:
                stream = createConfigurationStream();
                break;
            default:
                throw new IllegalArgumentException("Unknown URI: " + uri);
        }
        writer = new PipeDataWriter() {
            @Override
            public void writeDataToPipe(@NonNull ParcelFileDescriptor output, @NonNull Uri uri,
                    @NonNull String mimeType, @Nullable Bundle opts, @Nullable Object args) {
                try (FileOutputStream out = new FileOutputStream(output.getFileDescriptor())) {
                    byte[] buffer = new byte[8192];
                    int count;
                    while ((count = stream.read(buffer)) != -1) {
                        out.write(buffer, 0, count);
                    }
                } catch (IOException e) {
                    Log.e(TAG, "Failed to send file " + uri, e);
                }
            }
        };
        return openPipeHelper(uri, "text/xml", null, null, writer);
    }

    /**
     * Returns customization config stream.
     *
     * <p>It loads config file based on country code of the device.</p>
     */
    private InputStream createConfigurationStream() {
        Context context = getContext().getApplicationContext();

        return context.getResources().openRawResource(R.raw.configuration);
    }

    @Override
    public Cursor query(Uri uri, String[] projection, String selection,
            String[] selectionArgs, String sortOrder) {
         Log.d(TAG,"query:"+uri+",projection:"+projection);
        switch (URI_MATCHER.match(uri)) {
            case MATCH_WIDGET:
                String packageName = getContext().getApplicationContext().getPackageName();
                //Intent intent = new Intent(getContext(), InputsActivity.class);
                Intent intent = new Intent("com.android.tv.action.VIEW_INPUTS");
                MatrixCursor cursor = new MatrixCursor(new String[]{"icon", "title", "action"});

                String icon = "android.resource://" + packageName + "/raw/round_input_white_36dp";
                //String title = getContext().getString(R.string.inputs);
                String title = new String("Inputs");
                String action = intent.toUri(Intent.URI_INTENT_SCHEME);

                cursor.addRow(new String[]{icon, title, action});
                return cursor;
            default:
                return null;
        }
    }

    @Override
    public boolean onCreate() {
        return true;
    }

    @Nullable
    @Override
    public Uri insert(@NonNull Uri uri, ContentValues values) {
        throw new UnsupportedOperationException();
    }

    @Override
    public int delete(@NonNull Uri uri, String selection, String[] selectionArgs) {
        throw new UnsupportedOperationException();
    }

    @Override
    public int update(@NonNull Uri uri, ContentValues values, String selection,
            String[] selectionArgs) {
        throw new UnsupportedOperationException();
    }

    @Override
    public String getType(@NonNull Uri uri) {
        throw new UnsupportedOperationException();
    }

}
