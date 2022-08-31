/******************************************************************
*
*Copyright (C) 2012 Amlogic, Inc.
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

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.math.BigInteger;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

import android.R.string;
import android.util.Log;

public class MD5 {
    static String TAG = "MD5";

    private static String createMd5(File file) {
        MessageDigest mMDigest;
        FileInputStream Input = null;
        byte buffer[] = new byte[1024];
        int len;
        if (!file.exists())
            return null;
        try {
            mMDigest = MessageDigest.getInstance("MD5");
            Input = new FileInputStream(file);
            while ((len = Input.read(buffer, 0, 1024)) != -1) {
                mMDigest.update(buffer, 0, len);
            }
            Input.close();
            Input=null;
        } catch (NoSuchAlgorithmException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            return null;
        } catch (FileNotFoundException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            return null;
        } catch (IOException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            try {
                Input.close();
            } catch (IOException e1) {
                // TODO Auto-generated catch block
                e1.printStackTrace();
            }
            return null;
        }
        BigInteger mBInteger = new BigInteger(1, mMDigest.digest());
        Log.v(TAG, "create_MD5=" + mBInteger.toString(16));
        return mBInteger.toString(16);

    }
    public static boolean checkMd5(String Md5,File file){
        String str = createMd5(file);
        String mServMd5 = (new BigInteger(Md5,16)).toString(16);
        if (mServMd5.equalsIgnoreCase(str)) {
            Log.d(TAG,"md5sum = " + str+"Md5="+Md5);
            return true;
        } else {
            Log.d(TAG," not equals md5sum = " + str+"Md5="+Md5);
            return false;
        }
    }
    public static boolean checkMd5(String Md5, String strfile) {

        File file = new File(strfile);
        return checkMd5(Md5,file);
    }
    public static boolean checkMd5Files(File file1,File file2){
        String str1 = createMd5(file1);
        String str2 = createMd5(file2);
        if (str1.equalsIgnoreCase(str2)) {
            Log.d(TAG,"copy varify md5sum = " + str1+"Md5="+str2);
            return true;
        } else {
            Log.d(TAG," copy varify  not equals md5sum = " + str1+"Md5="+str2);
            return false;
        }
    }
    public static void main(String args[]){
        String Md5 = "003ad4693194e5012f03f33606220d80";
        String servMd5 = "3ad4693194e5012f03f33606220d80";
        BigInteger mBInteger = new BigInteger(Md5,16);
        System.out.println("BInteger="+(new BigInteger(Md5,16)).toString(16));
        BigInteger mServ = new BigInteger(servMd5,16);
        System.out.println("servMd5="+mServ+"?"+(servMd5.equals(mBInteger.toString(16))));
    }
}
