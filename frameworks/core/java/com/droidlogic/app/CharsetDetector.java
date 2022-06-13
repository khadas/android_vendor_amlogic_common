/*
 *  Copyright 2010 Georgios Migdos <cyberpython@gmail.com>.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *  under the License.
 */

package com.droidlogic.app;

import java.io.BufferedInputStream;
import java.io.EOFException;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.nio.ByteBuffer;
import java.nio.charset.CharacterCodingException;
import java.nio.charset.Charset;
import java.nio.charset.CharsetDecoder;

import android.util.Log;

/**
 *
 * @author
 */
public class CharsetDetector {
    private static final String TAG = "CharsetDetector";

    String[] mCharsetsToBeTested = {
        "UTF8",
        "GBK",
        "BIG5",
        "cp932",
        "cp949",
        "Windows-1255",
        "cp1250",
        "cp1254",
        "cp1098",
        "iso 8859-2",
        "UTF-16LE",
        "CP1256",
        "cp1252",
        "iso 8859-8",
        "UTF-16BE",
        "iso-8859-1"
    };


    public Charset detectCharset(File f) {

        Charset charset = null;
        int len = 0;
        String fileDetect = null;

        fileDetect = detectCharsetFromFilePath(f);
        if (fileDetect != null) {
            return Charset.forName(fileDetect);
        }

        len = getFileSize(f);
        //Log.i(TAG,"file size:" + len);
        if (len <= 0) {
            Log.e(TAG,"file is not valid!");
            return null;
        }

        for (String charsetName : mCharsetsToBeTested) {
            charset = detectCharset(f, Charset.forName(charsetName), len);
            if (charset != null) {
                break;
            }
        }

        return charset;
    }

    public static int getFileSize(File file) {
        if (!file.exists() || !file.isFile()) {
            Log.e(TAG,"file is not exist!!");
            return 0;
        }
        return (int)file.length();

    }

    public static String detectCharsetFromFilePath(File file){
        if (!file.exists() || !file.isFile()) {
            Log.e(TAG,"file is not exist!!");
            return null;
        }

        String pathName = file.getName();
        if (pathName == null) {
            return null;
        }

        Log.i(TAG,"detectCharsetFromFilePath pathName:" + pathName);

        if (pathName.contains("8859-1") || pathName.contains("8859_1")) {
            return "iso-8859-1";
        }
        if (pathName.contains("Windows-1250") || pathName.contains("1250")) {
            return "Windows-1250";
        }

        return null;
    }


    private Charset detectCharset(File f, Charset charset, int len) {
        try {
            BufferedInputStream input = new BufferedInputStream(new FileInputStream(f));

            Log.i (TAG,"[detectCharset]len:" + len);
            CharsetDecoder decoder = charset.newDecoder();
            decoder.reset();

            byte[] buffer = new byte[len];
            boolean identified = false;
            while ((input.read(buffer) != -1) && (!identified)) {
                identified = identify(buffer, decoder);
            }

            input.close();

            if (identified) {
                return charset;
            } else {
                return null;
            }

        } catch (Exception e) {
            return null;
        }
    }

    private boolean identify(byte[] bytes, CharsetDecoder decoder) {
        try {
            decoder.decode(ByteBuffer.wrap(bytes));
        } catch (CharacterCodingException e) {
            return false;
        }
        return true;
    }
}

