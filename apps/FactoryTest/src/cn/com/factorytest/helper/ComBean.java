package cn.com.factorytest.helper;

import java.text.SimpleDateFormat;

public class ComBean {
    public byte[] bRec = null;

    public ComBean(String sPort, byte[] buffer, int size) {
        bRec = new byte[size];
        for (int i = 0; i < size; i++) {
            bRec[i] = buffer[i];
        }
    }
}