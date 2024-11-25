package com.khadas.remotesettings;

public class RemoteKeyInfo {

    private String name;
    private String key;
    private boolean isSelect;
    private int scanCode;
    private int irCode;

    public RemoteKeyInfo(String key, String name, boolean isSelect, int scanCode, int irCode) {
        this.key = key;
        this.name = name;
        this.isSelect = isSelect;
        this.scanCode = scanCode;
        this.irCode = irCode;
    }

    public String getKey() {
        return key;
    }

    public void setKey(String key) {
        this.key = key;
    }

    public int getScanCode() {
        return scanCode;
    }

    public void setScanCode(int scanCode) {
        this.scanCode = scanCode;
    }

    public int getIrCode() {
        return irCode;
    }

    public void setIrCode(int irCode) {
        this.irCode = irCode;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public boolean isSelect() {
        return isSelect;
    }

    public void setSelect(boolean select) {
        isSelect = select;
    }

}
