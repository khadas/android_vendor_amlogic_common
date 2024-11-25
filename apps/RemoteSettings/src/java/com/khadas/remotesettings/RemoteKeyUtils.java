package com.khadas.remotesettings;

import android.text.TextUtils;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;

public class RemoteKeyUtils {

    private static final String TAG = "IRKeyUtils";

    public static int readIRCode() {
        int irCode = 0;
        final String irCodePath = "/sys/class/remote/amremote/receive_scancode";
        File file = new File(irCodePath);
        BufferedReader reader = null;
        String irCodeString = null;
        try  {
            reader = new BufferedReader(new FileReader(file));
            irCodeString = reader.readLine();
            if(!TextUtils.isEmpty(irCodeString)) {
                irCode = Integer.decode(irCodeString);
            }
            reader.close();
        } catch (IOException e) {
            e.printStackTrace();
        }  finally  {
            if (reader !=  null ) {
                try {
                    reader.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
        return irCode;
    }

    private static void writeFile(int cmd, int val1, int val2) {
        final String KEYMAP_PATH = "/sys/class/remote/amremote/custom_keymap";
        File file = new File(KEYMAP_PATH);
        if (!file.exists()) {
            return;
        }

        byte []  data = {(byte) cmd, (byte) val1, (byte)val2};
        FileOutputStream fos = null;
        try {
            fos = new FileOutputStream(KEYMAP_PATH);
            fos.write(data);
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            if (fos != null) {
                try {
                    fos.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    public static void updateIRKey(int scanCode, int irCode) {
        writeFile(0x02, scanCode, irCode);
    }

    public static void enterSetKeyMode() {
        writeFile(0x00, 0x00, 0x01);
    }

    public static void exitSetKeyMode() {
        writeFile(0x00, 0x00, 0x00);
    }

}
