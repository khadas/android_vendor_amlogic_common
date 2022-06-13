/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC SystemControlManager
 */

package com.droidlogic.app;

import android.content.Context;
import android.os.IBinder;
import android.os.HwBinder;
import android.os.Parcel;
import android.os.RemoteException;
import android.util.Log;

import java.util.ArrayList;
import java.util.NoSuchElementException;

//import android.hidl.manager.V1_0.IServiceManager;
//import android.hidl.manager.V1_0.IServiceNotification;
import vendor.amlogic.hardware.systemcontrol.V1_1.ISystemControl;
import vendor.amlogic.hardware.systemcontrol.V1_0.ISystemControlCallback;
import vendor.amlogic.hardware.systemcontrol.V1_0.Result;
import vendor.amlogic.hardware.systemcontrol.V1_0.DroidDisplayInfo;
import vendor.amlogic.hardware.systemcontrol.V1_0.SourceInputParam;
import vendor.amlogic.hardware.systemcontrol.V1_0.NolineParam;
import vendor.amlogic.hardware.systemcontrol.V1_0.OverScanParam;
import vendor.amlogic.hardware.systemcontrol.V1_0.WhiteBalanceParam;
import vendor.amlogic.hardware.systemcontrol.V1_0.PQDatabaseInfo;

public class SystemControlManager {
    private static final String TAG                 = "SysControlManager";

    //must sync with DisplayMode.h
    public static final boolean USE_BEST_MODE       = false;
    public static final int DISPLAY_TYPE_NONE       = 0;
    public static final int DISPLAY_TYPE_TABLET     = 1;
    public static final int DISPLAY_TYPE_MBOX       = 2;
    public static final int DISPLAY_TYPE_TV         = 3;
    public static final int KEY_TYPE_LEN_FIRST      = 4096;
    public static final int KEY_TYPE_LEN_SECOND     = 10240;

    public static final int PROVISION_KEY_VERSION_FIRST                         = 0x04;
    public static final int PROVISION_KEY_VERSION_SECOND                        = 0x05;
    public static final int PROVISION_KEY_TYPE_WIDEVINE                         = 0x11;
    public static final int PROVISION_KEY_TYPE_PLAYREADY_PRIVATE                = 0x21;
    public static final int PROVISION_KEY_TYPE_PLAYREADY_PUBLIC                 = 0x22;
    public static final int PROVISION_KEY_TYPE_HDCP_TX14                        = 0x31;
    public static final int PROVISION_KEY_TYPE_HDCP_TX22                        = 0x32;
    public static final int PROVISION_KEY_TYPE_HDCP_RX14                        = 0x33;
    public static final int PROVISION_KEY_TYPE_HDCP_RX22_WFD                    = 0x34;
    public static final int PROVISION_KEY_TYPE_HDCP_RX22_FW                     = 0x35;
    public static final int PROVISION_KEY_TYPE_HDCP_RX22_FW_PRIVATE             = 0x36;
    public static final int PROVISION_KEY_TYPE_KEYMASTER                        = 0x41;
    public static final int PROVISION_KEY_TYPE_KEYMASTER_3                      = 0x42;
    public static final int PROVISION_KEY_TYPE_KEYMASTER_3_ATTEST_DEV_ID_BOX    = 0x43;
    public static final int PROVISION_KEY_TYPE_EFUSE                            = 0x51;
    public static final int PROVISION_KEY_TYPE_CIPLUS                           = 0x61;
    public static final int PROVISION_KEY_TYPE_CIPLUS_ECP                       = 0x62;
    public static final int PROVISION_KEY_TYPE_NAGRA_DEV_UUID                   = 0x71;
    public static final int PROVISION_KEY_TYPE_NAGRA_DEV_SECRET                 = 0x72;
    public static final int PROVISION_KEY_TYPE_PFID                             = 0x81;
    public static final int PROVISION_KEY_TYPE_PFPK                             = 0x82;
    public static final int PROVISION_KEY_TYPE_YOUTUBE_SECRET                   = 0x91;
    public static final int PROVISION_KEY_TYPE_NETFLIX_MGKID                    = 0xA2;
    public static final int PROVISION_KEY_TYPE_WIDEVINE_CAS                     = 0xB1;
    public static final int PROVISION_KEY_TYPE_DOLBY_ID                         = 0xC1;
    public static final int PROVISION_KEY_TYPE_INVALID                          = 0xFFFFFFFF;

    private ISystemControl mProxy = null;
    private static final String BOOT_VIDEO                  =  "/data/bootvideo";
    private static final String LOGO_PNG                    = "/mnt/vendor/odm_ext/logo_files/bootup.bmp";
    // Notification object used to listen to the start of the system control daemon.
    //private final ServiceNotification mServiceNotification = new ServiceNotification();

    private static final int SYSTEM_CONTROL_DEATH_COOKIE = 1000;

    private static SystemControlManager mInstance;

    private IBinder mIBinder = null;

    // Mutex for all mutable shared state.
    private final Object mLock = new Object();

    private SystemControlManager() {
        /*
        try {
            boolean ret = IServiceManager.getService()
                    .registerForNotifications("vendor.amlogic.hardware.systemcontrol@1.0::ISystemControl", "", mServiceNotification);
            if (!ret) {
                Log.e(TAG, "Failed to register service start notification");
            }
        } catch (RemoteException e) {
            Log.e(TAG, "Failed to register service start notification", e);
            return;
        }*/
        connectToProxy();
    }

    private static class InstanceHolder {
        private static final SystemControlManager INSTANCE = new SystemControlManager();
    }

    public static SystemControlManager getInstance() {
         return InstanceHolder.INSTANCE;
     }

    private void connectToProxy() {
        synchronized (mLock) {
            if (mProxy != null) {
                return;
            }

            try {
                mProxy = ISystemControl.getService();
                mProxy.linkToDeath(new DeathRecipient(), SYSTEM_CONTROL_DEATH_COOKIE);
            } catch (NoSuchElementException e) {
                Log.e(TAG, "connectToProxy: system control service not found."
                        + " Did the service fail to start?", e);
            } catch (RemoteException e) {
                Log.e(TAG, "connectToProxy: system control service not responding", e);
            }
        }
    }

    public String getProperty(String prop) {
        synchronized (mLock) {
            Mutable<String> resultVal = new Mutable<>();
            try {
                mProxy.getProperty(prop, (int ret, String v) -> {
                                if (Result.OK == ret) {
                                    resultVal.value = v;
                                }
                            });
                return resultVal.value;
            } catch (RemoteException e) {
                Log.e(TAG, "getProperty:" + e);
            }
        }
        return "";
    }

    public String getPropertyString(String prop, String def) {
        synchronized (mLock) {
            Mutable<String> resultVal = new Mutable<>();
            try {
                mProxy.getPropertyString(prop, def, (int ret, String v) -> {
                                if (Result.OK == ret) {
                                    resultVal.value = v;
                                }
                            });
                return resultVal.value;
            } catch (RemoteException e) {
                Log.e(TAG, "getPropertyString:" + e);
            }
        }

        return "";
    }

    public int getPropertyInt(String prop, int def) {
        synchronized (mLock) {
            Mutable<Integer> resultVal = new Mutable<>();
            try {
                mProxy.getPropertyInt(prop, def, (int ret, int v) -> {
                                if (Result.OK == ret) {
                                    resultVal.value = v;
                                }
                            });
                return resultVal.value;
            } catch (RemoteException e) {
                Log.e(TAG, "getPropertyInt:" + e);
            }
        }

        return 0;
    }

    public long getPropertyLong(String prop, long def) {
        synchronized (mLock) {
            Mutable<Long> resultVal = new Mutable<>();
            try {
                mProxy.getPropertyLong(prop, def, (int ret, long v) -> {
                                if (Result.OK == ret) {
                                    resultVal.value = v;
                                }
                            });
                return resultVal.value;
            } catch (RemoteException e) {
                Log.e(TAG, "getPropertyLong:" + e);
            }
        }

        return 0;
    }

    public boolean getPropertyBoolean(String prop, boolean def) {
        synchronized (mLock) {
            Mutable<Boolean> resultVal = new Mutable<>();
            try {
                mProxy.getPropertyBoolean(prop, def, (int ret, boolean v) -> {
                                if (Result.OK == ret) {
                                    resultVal.value = v;
                                }
                            });
                return resultVal.value;
            } catch (RemoteException e) {
                Log.e(TAG, "getPropertyBoolean:" + e);
            }
        }

        return false;
    }

    public void setProperty(String prop, String val) {
        synchronized (mLock) {
            try {
                mProxy.setProperty(prop, val);
            } catch (RemoteException e) {
                Log.e(TAG, "setProperty:" + e);
            }
        }
    }

    public String readSysFs(String path) {
        synchronized (mLock) {
            Mutable<String> resultVal = new Mutable<>();
            try {
                mProxy.readSysfs(path, (int ret, String v) -> {
                                if (Result.OK == ret) {
                                    resultVal.value = v;
                                }
                            });
                return resultVal.value;
            } catch (RemoteException e) {
                Log.e(TAG, "readSysFs:" + e);
            }
        }

        return "";
    }

    public String readSysFsOri(String path) {
        synchronized (mLock) {
            Mutable<String> resultVal = new Mutable<>();
            try {
                mProxy.readSysfsOri(path, (int ret, String v) -> {
                                if (Result.OK == ret) {
                                    resultVal.value = v;
                                }
                            });
                return resultVal.value;
            } catch (RemoteException e) {
                Log.e(TAG, "readSysFs:" + e);
            }
        }

        return "";
    }

    public boolean writeSysFs(String path, String val) {
        synchronized (mLock) {
            try {
                mProxy.writeSysfs(path, val);
            } catch (RemoteException e) {
                Log.e(TAG, "writeSysFs:" + e);
            }
        }

        return true;
    }

    public boolean memcContrl(boolean on) {
        synchronized (mLock) {
            try {
                  return (mProxy.memcContrl(on) == Result.OK);
            } catch (RemoteException e) {
                Log.e(TAG, "memcContrl:" + e);
            }
        }
        return false;
    }


    public boolean setBootVideo(String filePath, boolean async) {
        synchronized (mLock) {
            try {
                  return (mProxy.scCpyFile(filePath,BOOT_VIDEO,async) == Result.OK);
            } catch (RemoteException e) {
                Log.e(TAG, "setbootvideo:" + e);
            }
        }
        return false;
    }

    public boolean setLogo(String filePath, boolean async) {
        synchronized (mLock) {
            try {
                  return (mProxy.scCpyFile(filePath,LOGO_PNG,async) == Result.OK);
            } catch (RemoteException e) {
                Log.e(TAG, "setlogo:" + e);
            }
        }
        return false;
    }

    public int[] paddingBuffer(int[] src, int def, int len) {
        int[] data;
        data = new int[len];
        synchronized (mLock) {
            try {
                int i;
                for (i = 0; i < def; ++i) {
                    data[i] = src[i];
                }
                for (; i < len; ++i) {
                    data[i] = 0;
                }
            } catch (Exception e) {
                Log.e(TAG, "padding_buffer:" + e);
            }
        }

        return data;
    }

    public boolean writeSysFs(String path, int[] val, int def) {
        synchronized (mLock) {
            try {
                int[] data;
                if (def > KEY_TYPE_LEN_FIRST) {
                    Log.e(TAG, "The data len is too long, it cannot exceed " + (String.format("%d", def)));
                    return false;
                }
                data = paddingBuffer(val, def, KEY_TYPE_LEN_FIRST);
                mProxy.writeSysfsBin(path, data, def);
            } catch (Exception e) {
                Log.e(TAG, "writeSysFs:" + e);
            }
        }

        return true;
    }

    //Provision key start

    /*
    *usage: writeUnifyKey("usid", val);
    * use it to write normal str keys
    */
    public void writeUnifyKey(String prop, String val) {
        synchronized (mLock) {
            try {
                mProxy.writeUnifyKey(prop, val);
            } catch (Exception e) {
                Log.e(TAG, "setBootenv:" + e);
            }
        }
    }

    /*
    *usage: writePlayreadyKey("widevinekeybox", keyContent, keyContent.length);
    * use it write keys that contain Special characters
    */
    public boolean writePlayreadyKey(int[] val, int size) {
        synchronized (mLock) {
            try {
                int[] data;
                if (size > KEY_TYPE_LEN_FIRST) {
                    Log.e(TAG, "The data len is too long, it cannot exceed " + (String.format("%d", size)));
                    return false;
                }
                data = paddingBuffer(val, size, KEY_TYPE_LEN_FIRST);
                int res = mProxy.writePlayreadyKey(data, size);
                return 0 == res;
            } catch (Exception e) {
                Log.e(TAG, "writeUnifyKey:" + e);
            }
        }
        return true;
    }

    public boolean writeWidevineKey(int[] val, int size) {
        Log.d(TAG, "writeWidevineKey size: " + size);
        synchronized (mLock) {
            try {
                int[] data;
                if (size > KEY_TYPE_LEN_FIRST) {
                    Log.e(TAG, "The data len is too long, it cannot exceed " + (String.format("%d", size)));
                    return false;
                }
                data = paddingBuffer(val, size, KEY_TYPE_LEN_FIRST);
                int res = mProxy.writeWidevineKey(data, size);
                return 0 == res;
            } catch (Exception e) {
                Log.e(TAG, "writeUnifyKey:" + e);
            }
        }
        return true;
    }

    public boolean writeNetflixKey(int[] val, int size) {
        Log.d(TAG, "writeNetflixKey size: " + size);
        synchronized (mLock) {
            try {
                int[] data;
                if (size > KEY_TYPE_LEN_FIRST) {
                    Log.e(TAG, "The data len is too long, it cannot exceed " + (String.format("%d", size)));
                    return false;
                }
                data = paddingBuffer(val, size, KEY_TYPE_LEN_FIRST);
                int res = mProxy.writeNetflixKey(data, size);
                return 0 == res;
            } catch (Exception e) {
                Log.e(TAG, "writeUnifyKey:" + e);
            }
        }
        return true;
    }

    /*
    *usage: writeAttestationKey("/dev/unifykeys", "attestationkeybox", keyContent, keyContent.length);
    * keyContent: the value of attestationkey
    * keyContent.length: the size of keyContent
    */
    public boolean writeAttestationKey(int[] val, int size) {
        synchronized (mLock) {
            try {
                int[] data;
                if (size > KEY_TYPE_LEN_SECOND) {
                    Log.e(TAG, "The data len is too long, it cannot exceed " + (String.format("%d", size)));
                    return false;
                }
                data = paddingBuffer(val, size, KEY_TYPE_LEN_SECOND);
                int res = mProxy.writeAttestationKey(data, size);
                return 0 == res;
            } catch (Exception e) {
                Log.e(TAG, "writeAttestationKey:" + e);
            }
        }

        return true;
    }

   /*
    *usage: writeHDCP14Key("widevinekeybox", keyContent, keyContent.length);
    * use it write keys that contain Special characters
    */
    public boolean writeHDCP14Key(int[] val, int size) {
        synchronized (mLock) {
            try {
                int[] data;
                if (size > KEY_TYPE_LEN_FIRST) {
                    Log.e(TAG, "The data len is too long, it cannot exceed " + (String.format("%d", size)));
                    return false;
                }
                data = paddingBuffer(val, size, KEY_TYPE_LEN_FIRST);
                int res = mProxy.writeHDCP14Key(data, size);
                return 0 == res;
            } catch (Exception e) {
                Log.e(TAG, "writeHDCP14Key:" + e);
            }
        }
        return true;
    }

    /*
    * usage: writeHdcpRX14Key(val, size);
    * val: the value of hdcp rx 22 key, the key must be 328/348 byte, if not, ask sale to apply license.
    * size: the size of val
    */
    public boolean writeHdcpRX14Key(int[] val, int size) {
        synchronized (mLock) {
            try {
                int[] data;
                if (size > KEY_TYPE_LEN_FIRST) {
                    Log.e(TAG, "The data len is too long, it cannot exceed " + (String.format("%d", size)));
                    return false;
                }
                data = paddingBuffer(val, size, KEY_TYPE_LEN_FIRST);
                int res = mProxy.writeHdcpRX14Key(data, size);
                return 0 == res;
            } catch (Exception e) {
                Log.e(TAG, "writeHdcpRX14Key:" + e);
            }
        }

        return true;
    }

   /*
    *usage: writeHDCP22Key("widevinekeybox", keyContent, keyContent.length);
    * use it write keys that contain Special characters
    */
    public boolean writeHDCP22Key(int[] val, int size) {
        synchronized (mLock) {
            try {
                int[] data;
                if (size > KEY_TYPE_LEN_FIRST) {
                    Log.e(TAG, "The data len is too long, it cannot exceed " + (String.format("%d", size)));
                    return false;
                }
                data = paddingBuffer(val, size, KEY_TYPE_LEN_FIRST);
                int res = mProxy.writeHDCP22Key(data, size);
                return 0 == res;
            } catch (Exception e) {
                Log.e(TAG, "writeHDCP22Key:" + e);
            }
        }
        return true;
    }

    /*
    * usage: writeHdcpRX22Key(val, size);
    * val: the value of key, such as 00000000_hdcp_key2.2.bin, ask sales to get it.
    * size: the size of val
    */
    public boolean writeHdcpRX22Key(int[] val, int size) {
        synchronized (mLock) {
            try {
                int[] data;
                if (size > KEY_TYPE_LEN_FIRST) {
                    Log.e(TAG, "The data len is too long, it cannot exceed " + (String.format("%d", size)));
                    return false;
                }
                data = paddingBuffer(val, size, KEY_TYPE_LEN_FIRST);
                int res = mProxy.writeHdcpRX22Key(data, size);
                return 0 == res;
            } catch (Exception e) {
                Log.e(TAG, "writeHdcpRX22Key:" + e);
            }
        }

        return true;
    }

    public boolean writePFIDKey(int[] val, int size) {
        synchronized (mLock) {
            try {
                int[] data;
                if (size > KEY_TYPE_LEN_FIRST) {
                    Log.e(TAG, "The data len is too long, it cannot exceed " + (String.format("%d", size)));
                    return false;
                }
                data = paddingBuffer(val, size, KEY_TYPE_LEN_FIRST);
                int res = mProxy.writePFIDKey(data, size);
                return 0 == res;
            } catch (RemoteException e) {
                Log.e(TAG, "writePFIDKey:" + e);
            }
        }
        return true;
    }

    public boolean writePFPKKey(int[] val, int size) {
        synchronized (mLock) {
            try {
                int[] data;
                if (size > KEY_TYPE_LEN_FIRST) {
                    Log.e(TAG, "The data len is too long, it cannot exceed " + (String.format("%d", size)));
                    return false;
                }
                data = paddingBuffer(val, size, KEY_TYPE_LEN_FIRST);
                int res = mProxy.writePFPKKey(data, size);
                return 0 == res;
            } catch (RemoteException e) {
                Log.e(TAG, "writePFPKKey:" + e);
            }
        }
        return true;
    }

    /*
     *usage: readUnifyKey("usid");
     * use it to read normal str keys
     */
    public String readUnifyKey(String path) {
        synchronized (mLock) {
            Mutable<String> resultVal = new Mutable<>();
            try {
                mProxy.readUnifyKey(path, (int ret, String v) -> {
                                if (Result.OK == ret) {
                                    resultVal.value = v;
                                }
                            });
                return resultVal.value;
            } catch (Exception e) {
                Log.e(TAG, "readUnifyKey:" + e);
            }
        }

        return "";
    }

    /*
     *usage: readPlayreadyKey("widevinekeybox", keyContent, keyContent.length);
     * use it read keys that contain Special characters, If key is secure, it only return the hash data
     * by now the secure key contain: hdcp/secure_boot_set/widevinekeybox/hdcp22_fw_private/
     *       hdcp22_rx_private/prpubkeybox/prprivkeybox/attestationkeybox/attestationdevidbox
     */
    public boolean readPlayreadyKey(String path, int key_type, int size) {
        synchronized (mLock) {
            try {
                if (size > KEY_TYPE_LEN_FIRST) {
                    Log.e(TAG, "The data len is too long, it cannot exceed " + (String.format("%d", size)));
                    return false;
                }
                int res = mProxy.readPlayreadyKey(path, key_type, size);
                Log.d(TAG, "readPlayreadyKey result " + res);
                return 0 == res;
            } catch (Exception e) {
                Log.e(TAG, "readPlayreadyKey:" + e);
            }
        }
        return false;
    }

    public boolean readWidevineKey(int key_type, int size) {
        Log.d(TAG, "readWidevineKey key_type: "+ key_type + ";size: " + size);
        synchronized (mLock) {
            try {
                if (size > KEY_TYPE_LEN_FIRST) {
                    Log.e(TAG, "The data len is too long, it cannot exceed " + (String.format("%d", size)));
                    return false;
                }
                int res = mProxy.readWidevineKey(key_type, size);
                Log.d(TAG, "readWidevineKey result " + res);
                return 0 == res;
            } catch (Exception e) {
                Log.e(TAG, "readWidevineKey:" + e);
            }
        }
        return false;
    }

    public boolean readNetflixKey(int key_type, int size) {
        Log.d(TAG, "readNetflixKey key_type: "+ key_type + ";size: " + size);
        synchronized (mLock) {
            try {
                if (size > KEY_TYPE_LEN_FIRST) {
                    Log.e(TAG, "The data len is too long, it cannot exceed " + (String.format("%d", size)));
                    return false;
                }
                int res = mProxy.readNetflixKey(key_type, size);
                Log.d(TAG, "readNetflixKey result " + res);
                return 0 == res;
            } catch (Exception e) {
                Log.e(TAG, "readNetflixKey:" + e);
            }
        }
        return false;
    }

    /*
     *usage: readAttestationKey("/dev/unifykeys", "attestationkeybox", keyContent, keyContent.length);
     * attestationkeybox is secure key, so it only return the hash data
     */
    public boolean readAttestationKey(int key_type, int size) {
        synchronized (mLock) {
            try {
                if (size > KEY_TYPE_LEN_SECOND) {
                    Log.e(TAG, "The data len is too long, it cannot exceed " + (String.format("%d", size)));
                    return false;
                }
                int res = mProxy.readAttestationKey(key_type, size);
                Log.d(TAG, "readAttestationKey result " + res);
                return 0 == res;
            } catch (Exception e) {
                Log.e(TAG, "readAttestationKey:" + e);
            }
        }

        return false;
    }

   /*
    *usage: readHDCP14Key("widevinekeybox", keyContent, keyContent.length);
    * use it read keys that contain Special characters, If key is secure, it only return the hash data
    * by now the secure key contain: hdcp/secure_boot_set/widevinekeybox/hdcp22_fw_private/
    *       hdcp22_rx_private/prpubkeybox/prprivkeybox/attestationkeybox/attestationdevidbox
    */
    public boolean readHDCP14Key(int key_type, int size) {
        synchronized (mLock) {
            try {
                if (size > KEY_TYPE_LEN_FIRST) {
                    Log.e(TAG, "The data len is too long, it cannot exceed " + (String.format("%d", size)));
                    return false;
                }
                int res = mProxy.readHDCP14Key(key_type, size);
                Log.d(TAG, "readHDCP14Key result " + res);
                return 0 == res;
            } catch (Exception e) {
                Log.e(TAG, "readHDCP14Key:" + e);
            }
        }
        return false;
    }


    public boolean readHdcpRX14Key(int key_type, int size) {
        synchronized (mLock) {
            try {
                if (size > KEY_TYPE_LEN_FIRST) {
                    Log.e(TAG, "The data len is too long, it cannot exceed " + (String.format("%d", size)));
                    return false;
                }
                int res = mProxy.readHdcpRX14Key(key_type, size);
                Log.d(TAG, "readHdcpRX14Key result " + res);
                return 0 == res;
            } catch (Exception e) {
                Log.e(TAG, "readHdcpRX14Key:" + e);
            }
        }
        return false;
    }

   /*
    *usage: readHDCP22Key("widevinekeybox", keyContent, keyContent.length);
    * use it read keys that contain Special characters, If key is secure, it only return the hash data
    * by now the secure key contain: hdcp/secure_boot_set/widevinekeybox/hdcp22_fw_private/
    *       hdcp22_rx_private/prpubkeybox/prprivkeybox/attestationkeybox/attestationdevidbox
    */
    public boolean readHDCP22Key(int key_type, int size) {
        synchronized (mLock) {
            try {
                if (size > KEY_TYPE_LEN_FIRST) {
                    Log.e(TAG, "The data len is too long, it cannot exceed " + (String.format("%d", size)));
                    return false;
                }
                int res = mProxy.readHDCP22Key(key_type, size);
                Log.d(TAG, "readHDCP22Key result " + res);
                return 0 == res;
            } catch (Exception e) {
                Log.e(TAG, "readHDCP22Key:" + e);
            }
        }
        return false;
    }

    public boolean readHdcpRX22Key(int key_type, int size) {
        synchronized (mLock) {
            try {
                if (size > KEY_TYPE_LEN_FIRST) {
                    Log.e(TAG, "The data len is too long, it cannot exceed " + (String.format("%d", size)));
                    return false;
                }
                int res = mProxy.readHdcpRX22Key(key_type, size);
                Log.d(TAG, "readHdcpRX22Key result " + res);
                return 0 == res;
            } catch (Exception e) {
                Log.e(TAG, "readHdcpRX22Key:" + e);
            }
        }
        return false;
    }


    public boolean checkPlayreadyKey(String path, int[] val, int key_type, int size) {
        synchronized (mLock) {
            try {
                int[] data;
                if (size > KEY_TYPE_LEN_FIRST) {
                    Log.e(TAG, "The data len is too long, it cannot exceed " + (String.format("%d", size)));
                    return false;
                }
                data = paddingBuffer(val, size, KEY_TYPE_LEN_FIRST);
                int res = mProxy.checkPlayreadyKey(path, data, key_type, size);
                Log.d(TAG, "checkPlayreadyKey result " + res);
                return 0 == res;
            } catch (Exception e) {
                Log.e(TAG, "checkPlayreadyKey:" + e);
            }
        }
        return false;
    }

    public boolean checkWidevineKey(int[] val, int key_type, int size) {
        synchronized (mLock) {
            try {
                int[] data;
                if (size > KEY_TYPE_LEN_FIRST) {
                    Log.e(TAG, "The data len is too long, it cannot exceed " + (String.format("%d", size)));
                    return false;
                }
                data = paddingBuffer(val, size, KEY_TYPE_LEN_FIRST);
                int res = mProxy.checkWidevineKey(data, key_type, size);
                Log.d(TAG, "checkWidevineKey result " + res);
                return 0 == res;
            } catch (Exception e) {
                Log.e(TAG, "checkWidevineKey:" + e);
            }
        }
        return false;
    }

    public boolean checkNetflixKey(int[] val, int key_type, int size) {
        synchronized (mLock) {
            try {
                int[] data;
                if (size > KEY_TYPE_LEN_FIRST) {
                    Log.e(TAG, "The data len is too long, it cannot exceed " + (String.format("%d", size)));
                    return false;
                }
                data = paddingBuffer(val, size, KEY_TYPE_LEN_FIRST);
                int res = mProxy.checkNetflixKey(data, key_type, size);
                Log.d(TAG, "checkNetflixKey result " + res);
                return 0 == res;
            } catch (Exception e) {
                Log.e(TAG, "checkNetflixKey:" + e);
            }
        }
        return false;
    }

    /* check does AttestationKey ok*/
    public boolean checkAttestationKey(int[] val, int key_type, int size) {
        synchronized (mLock) {
            try {
                int[] data;
                if (size > KEY_TYPE_LEN_SECOND) {
                    Log.e(TAG, "The data len is too long, it cannot exceed " + (String.format("%d", size)));
                    return false;
                }
                data = paddingBuffer(val, size, KEY_TYPE_LEN_SECOND);
                int res = mProxy.checkAttestationKey(data,key_type,size);
                Log.d(TAG, "checkAttestationKey result " + res);
                return 0 == res;
            } catch (Exception e) {
                Log.e(TAG, "checkAttestationKey:" + e);
            }
        }
        return false;
    }

    public boolean checkHDCP14Key(int[] val, int key_type, int size) {
        synchronized (mLock) {
            try {
                int[] data;
                if (size > KEY_TYPE_LEN_FIRST) {
                    Log.e(TAG, "The data len is too long, it cannot exceed " + (String.format("%d", size)));
                    return false;
                }
                data = paddingBuffer(val, size, KEY_TYPE_LEN_FIRST);
                int res = mProxy.checkHDCP14Key(data, key_type, size);
                Log.d(TAG, "checkHDCP14Key result " + res);
                return 0 == res;
            } catch (Exception e) {
                Log.e(TAG, "checkHDCP14Key:" + e);
            }
        }
        return false;
    }

    public boolean checkHDCP14KeyIsExist(int key_type) {
        synchronized (mLock) {
            try {
                int res = mProxy.checkHDCP14KeyIsExist(key_type);
                return 0 == res;
            } catch (Exception e) {
                Log.e(TAG, "checkHDCP14KeyIsExist:" + e);
            }
        }

        return false;
    }

    public boolean checkHDCP22Key(String path, int[] val, int key_type, int size) {
        synchronized (mLock) {
            try {
                int[] data;
                if (size > KEY_TYPE_LEN_FIRST) {
                    Log.e(TAG, "The data len is too long, it cannot exceed " + (String.format("%d", size)));
                    return false;
                }
                data = paddingBuffer(val, size, KEY_TYPE_LEN_FIRST);
                int res = mProxy.checkHDCP22Key(path, data, key_type, size);
                Log.d(TAG, "checkHDCP22Key result " + res);
                return 0 == res;
            } catch (Exception e) {
                Log.e(TAG, "checkHDCP22Key:" + e);
            }
        }
        return false;
    }

    public boolean checkHDCP22KeyIsExist(int key_type_first, int key_type_second) {
        synchronized (mLock) {
            try {
                int res = mProxy.checkHDCP22KeyIsExist(key_type_first, key_type_second);
                return 0 == res;
            } catch (Exception e) {
                Log.e(TAG, "checkHDCP22KeyIsExist:" + e);
            }
        }

        return false;
    }

    public boolean checkPFIDKeyIsExist(int key_type) {
        synchronized (mLock) {
            try {
                int res = mProxy.checkPFIDKeyIsExist(key_type);
                return 0 == res;
            } catch (RemoteException e) {
                Log.e(TAG, "checkPFIDKeyIsExist:" + e);
            }
        }
        return false;
    }

    public boolean checkPFPKKeyIsExist(int key_type) {
        synchronized (mLock) {
            try {
                int res = mProxy.checkPFPKKeyIsExist(key_type);
                return 0 == res;
            } catch (RemoteException e) {
                Log.e(TAG, "checkPFPKKeyIsExist:" + e);
            }
        }
        return false;
    }

    //Provision key end

    /*
    * usage: writeHdcpRXImg(path);
    * path: the path of key, such as 00000000_hdcp_key2.2.bin, ask sales to get it.
    */
    public boolean writeHdcpRXImg(String path) {
        synchronized (mLock) {
            try {
                mProxy.writeHdcpRXImg(path);
            } catch (RemoteException e) {
                Log.e(TAG, "writeHdcpRXImg:" + e);
            }
        }

        return true;
    }

    public boolean updataLogoBmp(String path) {
        Log.i(TAG, "updataLogoBmp: " + path);
        synchronized (mLock) {
            try {
                mProxy.updataLogoBmp(path);
            } catch (RemoteException e) {
                Log.e(TAG, "updataLogoBmp:" + e);
            }
        }

        return true;
    }

    public String getBootenv(String prop, String def) {
        synchronized (mLock) {
            Mutable<String> resultVal = new Mutable<>();
            try {
                mProxy.getBootEnv(prop, (int ret, String v) -> {
                                if (Result.OK == ret) {
                                    resultVal.value = v;
                                } else {
                                    resultVal.value = def;
                                }
                            });
                return resultVal.value;
            } catch (RemoteException e) {
                Log.e(TAG, "getBootenv:" + e);
            }
        }

        return "";
    }

    public void setBootenv(String prop, String val) {
        synchronized (mLock) {
            try {
                mProxy.setBootEnv(prop, val);
            } catch (RemoteException e) {
                Log.e(TAG, "setBootenv:" + e);
            }
        }
    }
    public boolean setHdrStrategy(String type) {
        synchronized (mLock) {
           try {
               mProxy.setHdrStrategy(type);
           } catch (RemoteException e) {
               Log.e(TAG, "setHdrStrategy:" + e);
           }
        }
            return false;
    }
    public boolean setHdrPriority(String type) {
        synchronized (mLock) {
           try {
               mProxy.setHdrPriority(type);
           } catch (RemoteException e) {
               Log.e(TAG, "setHdrPriority:" + e);
           }
        }
            return false;
    }
    public boolean GetModeSupportDeepColorAttr(String mode, String value) {
        synchronized (mLock) {
            try {
                return (mProxy.getModeSupportDeepColorAttr(mode, value) == Result.OK)?true:false;
            } catch (RemoteException e) {
                Log.e(TAG, "setBootenv:" + e);
            }
        }
        return false;
    }

    public DisplayInfo getDisplayInfo() {
        /*DisplayInfo info = new DisplayInfo();
        synchronized (mLock) {
            Mutable<DroidDisplayInfo> resultInfo = new Mutable<>();
            try {
                mProxy.getDroidDisplayInfo((int ret, DroidDisplayInfo v) -> {
                                if (Result.OK == ret) {
                                    resultInfo.value = v;
                                }
                            });
                info.type = resultInfo.value.type;
                info.socType = resultInfo.value.socType;
                info.defaultUI = resultInfo.value.defaultUI;
                info.fb0Width = resultInfo.value.fb0w;
                info.fb0Height = resultInfo.value.fb0h;
                info.fb0FbBits = resultInfo.value.fb0bits;
                info.fb0TripleEnable = (1==resultInfo.value.fb0trip)?true:false;

                info.fb1Width = resultInfo.value.fb1w;
                info.fb1Height = resultInfo.value.fb1h;
                info.fb1FbBits = resultInfo.value.fb1bits;
                info.fb1TripleEnable = (1==resultInfo.value.fb1trip)?true:false;
            } catch (RemoteException e) {
                Log.e(TAG, "getDisplayInfo:" + e);
            }
        }*/

        return null;
    }

    public void loopMountUnmount(boolean isMount, String path){
        synchronized (mLock) {
            try {
                mProxy.loopMountUnmount(isMount?1:0, path);
            } catch (RemoteException e) {
                Log.e(TAG, "loopMountUnmount:" + e);
            }
        }
    }

    public String getActiveDispMode() {
        synchronized (mLock) {
            Mutable<String> resultVal = new Mutable<>();
            try {
                mProxy.getActiveDispMode((int ret, String v) -> {
                                if (Result.OK == ret) {
                                    resultVal.value = v;
                                }
                            });
                return resultVal.value;
            } catch (RemoteException e) {
                Log.e(TAG, "getActiveDispMode:" + e);
            }
        }
        return "";
    }

    public void setMboxOutputMode(String mode) {
        synchronized (mLock) {
            try {
                mProxy.setSourceOutputMode(mode);
            } catch (RemoteException e) {
                Log.e(TAG, "setMboxOutputMode:" + e);
            }
        }
    }

    public void setDigitalMode(String mode) {
        synchronized (mLock) {
            try {
                mProxy.setDigitalMode(mode);
            } catch (RemoteException e) {
                Log.e(TAG, "setDigitalMode:" + e);
            }
        }
    }

    public void setOsdMouseMode(String mode) {
        synchronized (mLock) {
            try {
                mProxy.setOsdMouseMode(mode);
            } catch (RemoteException e) {
                Log.e(TAG, "setOsdMouseMode:" + e);
            }
        }
    }

    public void setOsdMousePara(int x, int y, int w, int h) {
        synchronized (mLock) {
            try {
                mProxy.setOsdMousePara(x, y, w, h);
            } catch (RemoteException e) {
                Log.e(TAG, "setOsdMousePara:" + e);
            }
        }
    }

    public void setPosition(int x, int y, int w, int h) {
        synchronized (mLock) {
            try {
                mProxy.setPosition(x, y, w, h);
            } catch (RemoteException e) {
                Log.e(TAG, "setPosition:" + e);
            }
        }
    }

    public int[] getPosition(String mode) {
        int[] curPosition = { 0, 0, 1280, 720 };
        synchronized (mLock) {
            Mutable<Integer> left = new Mutable<>();
            Mutable<Integer> top = new Mutable<>();
            Mutable<Integer> width = new Mutable<>();
            Mutable<Integer> height = new Mutable<>();
            try {
                mProxy.getPosition(mode, (int ret, int x, int y, int w, int h) -> {
                                if (Result.OK == ret) {
                                    left.value = x;
                                    top.value = y;
                                    width.value = w;
                                    height.value = h;
                                }
                            });
                curPosition[0] = left.value;
                curPosition[1] = top.value;
                curPosition[2] = width.value;
                curPosition[3] = height.value;
                return curPosition;
            } catch (RemoteException e) {
                Log.e(TAG, "getPosition:" + e);
            }
        }
        return curPosition;
    }

    public String getDeepColorAttr(String mode) {
        synchronized (mLock) {
            Mutable<String> resultVal = new Mutable<>();
            try {
                mProxy.getDeepColorAttr(mode, (int ret, String v) -> {
                                if (Result.OK == ret) {
                                    resultVal.value = v;
                                }
                            });
                return resultVal.value;
            } catch (RemoteException e) {
                Log.e(TAG, "getDeepColorAttr:" + e);
            }
        }
        return "";
    }

    public long resolveResolutionValue(String mode) {
        synchronized (mLock) {
            Mutable<Long> resultVal = new Mutable<>();
            try {
                mProxy.resolveResolutionValue(mode, (int ret, long v) -> {
                                if (Result.OK == ret) {
                                    resultVal.value = v;
                                }
                            });
                return resultVal.value;
            } catch (RemoteException e) {
                Log.e(TAG, "resolveResolutionValue:" + e);
            }
        }
        return -1;
    }

    public String isTvSupportDolbyVision() {
        synchronized (mLock) {
            Mutable<String> resultVal = new Mutable<>();
            try {
                mProxy.sinkSupportDolbyVision((int ret, String v, boolean support) -> {
                                if (Result.OK == ret) {
                                    resultVal.value = v;
                                }
                            });
                return resultVal.value;
            } catch (RemoteException e) {
                Log.e(TAG, "isTvSupportDolbyVision:" + e);
            }
        }
        return "";
    }

    public void setDolbyVisionEnable(int state) {
        synchronized (mLock) {
            try {
                mProxy.setDolbyVisionState(state);
            } catch (RemoteException e) {
                Log.e(TAG, "setDolbyVisionEnable:" + e);
            }
        }
    }

    public void setALLMMode(int state) {
        synchronized (mLock) {
            try {
                mProxy.setALLMState(state);
            } catch (RemoteException e) {
                Log.e(TAG, "setALLMState:" + e);
            }
        }
    }

    public boolean getAllmSupport() {
        synchronized (mLock) {
            Mutable<String> resultVal = new Mutable<>();
            try {
                return mProxy.getAllmSupport();
            } catch (RemoteException e) {
                Log.e(TAG, "getAllmSupport:" + e);
            }
        }
        return false;
    }

    public void sendHDMIContentType(int state) {
        synchronized (mLock) {
            try {
                mProxy.sendHDMIContentType(state);
            } catch (RemoteException e) {
                Log.e(TAG, "sendGameContentType:" + e);
            }
        }
    }

    public boolean getGameContentTypeSupport() {
        synchronized (mLock) {
            Mutable<String> resultVal = new Mutable<>();
            try {
                return mProxy.getGameContentTypeSupport();
            } catch (RemoteException e) {
                Log.e(TAG, "getGameContentTypeSupport:" + e);
            }
        }
        return false;
    }

    public void saveDeepColorAttr(String mode, String dcValue) {
        synchronized (mLock) {
            try {
                mProxy.saveDeepColorAttr(mode, dcValue);
            } catch (RemoteException e) {
                Log.e(TAG, "saveDeepColorAttr:" + e);
            }
        }
    }

    public void setHdrMode(String mode) {
        synchronized (mLock) {
            try {
                mProxy.setHdrMode(mode);
            } catch (RemoteException e) {
                Log.e(TAG, "setHdrMode:" + e);
            }
        }
    }

    public void setSdrMode(String mode) {
        synchronized (mLock) {
            try {
                mProxy.setSdrMode(mode);
            } catch (RemoteException e) {
                Log.e(TAG, "setSdrMode:" + e);
            }
        }
    }
    public int getDolbyVisionType() {
        synchronized (mLock) {
            Mutable<Integer> resultVal = new Mutable<>();
            try {
                mProxy.getDolbyVisionType((int ret, int v) -> {
                    if (Result.OK == ret) {
                        resultVal.value = v;
                    }
                });
                return resultVal.value;
            } catch (RemoteException e) {
                Log.e(TAG, "getDolbyVisionType:" + e);
            }
        }

        return 0;
    }

    public void setGraphicsPriority(String mode) {
        synchronized (mLock) {
            try {
                mProxy.setGraphicsPriority(mode);
            } catch (RemoteException e) {
                Log.e(TAG, "setGraphicsPriority:" + e);
            }
        }
    }

    public String getGraphicsPriority() {
        synchronized (mLock) {
            Mutable<String> resultVal = new Mutable<>();
            try {
                mProxy.getGraphicsPriority((int ret, String v) -> {
                    if (Result.OK == ret) {
                        resultVal.value = v;
                    }
                });
                return resultVal.value;
            } catch (RemoteException e) {
                Log.e(TAG, "getGraphicsPriority:" + e);
            }
        }
        return "";
    }

    /**
     * that use by droidlogic-res.apk only, because need have one callback only
     *
     * @hide
     */
    private boolean mListenerFlag = false;
    public void setListener(ISystemControlCallback listener) {
        Log.i(TAG, "setListener");
        synchronized (mLock) {
            try {
                if (mListenerFlag)
                    Log.d(TAG, "already setListener");
                else {
                    mProxy.setCallback(listener);
                    mListenerFlag = true;
                }
            } catch (RemoteException e) {
                Log.e(TAG, "setCallback:" + e);
            }
        }
    }

    public void setAppInfo(String pkg, String cls, ArrayList<String> proc) {
        synchronized (mLock) {
            try {
                mProxy.setAppInfo(pkg, cls, proc);
            } catch (RemoteException e) {
                Log.e(TAG, "setAppInfo:" + e);
            }
        }
    }

    public int set3DMode(String mode3d) {
        Log.i(TAG, "[set3DMode]mode3d:" + mode3d);
        synchronized (mLock) {
            try {
                mProxy.set3DMode(mode3d);
            } catch (RemoteException e) {
                Log.e(TAG, "set3DMode:" + e);
            }
        }

        return 0;
    }

    /**
     * Close 3D mode, include 3D setting and OSD display setting.
     */
    public void init3DSettings() {
        synchronized (mLock) {
            try {
                mProxy.init3DSetting();
            } catch (RemoteException e) {
                Log.e(TAG, "init3DSettings:" + e);
            }
        }
    }

    /**
     * Get 3D format for current playing video, include local, streaming and HDMI input.
     * return format is setted by video parser, such as libplayer for amlogic
     *
     * @return 3D format
     * FORMAT_3D_OFF
     * FORMAT_3D_AUTO
     * FORMAT_3D_SIDE_BY_SIDE
     * FORMAT_3D_TOP_AND_BOTTOM
     */
    public int getVideo3DFormat() {
        synchronized (mLock) {
            Mutable<Integer> resultVal = new Mutable<>();
            try {
                mProxy.getVideo3DFormat((int ret, int v) -> {
                                if (Result.OK == ret) {
                                    resultVal.value = v;
                                }
                            });
                return resultVal.value;
            } catch (RemoteException e) {
                Log.e(TAG, "getVideo3DFormat:" + e);
            }
        }
        return -1;
    }

    /**
     * Get display 3D format setted by setDisplay3DTo2DFormat.
     *
     * @return 3D format
     * FORMAT_3D_OFF
     * FORMAT_3D_AUTO
     * FORMAT_3D_SIDE_BY_SIDE
     * FORMAT_3D_TOP_AND_BOTTOM
     */
    public int getDisplay3DTo2DFormat() {
        synchronized (mLock) {
            Mutable<Integer> resultVal = new Mutable<>();
            try {
                mProxy.getDisplay3DTo2DFormat((int ret, int v) -> {
                                if (Result.OK == ret) {
                                    resultVal.value = v;
                                }
                            });
                return resultVal.value;
            } catch (RemoteException e) {
                Log.e(TAG, "getDisplay3DTo2DFormat:" + e);
            }
        }
        return -1;
    }

    /**
     * Set 3D format for video, this format is decided by user,
     * if LCD isn't support 3D and you wanna play a 3D file, use the api to show part picture of the video,
     * such as the left side of the 3D video source or the top side one.
     *
     * @param 3D format
     * FORMAT_3D_OFF
     * FORMAT_3D_AUTO
     * FORMAT_3D_SIDE_BY_SIDE
     * FORMAT_3D_TOP_AND_BOTTOM
     *
     * @return set status
     */
    public boolean setDisplay3DTo2DFormat(int format) {
        synchronized (mLock) {
            Mutable<Integer> resultVal = new Mutable<>();
            try {
                mProxy.setDisplay3DTo2DFormat(format);
            } catch (RemoteException e) {
                Log.e(TAG, "setDisplay3DTo2DFormat:" + e);
            }
        }

        return true;
    }

    /**
     * Set 3D format for OSD and video, this format is decided by user,
     * if LCD is support 3D, use the api to set OSD and video 3D format.
     *
     * @param 3D format
     * FORMAT_3D_OFF
     * FORMAT_3D_AUTO
     * FORMAT_3D_SIDE_BY_SIDE
     * FORMAT_3D_TOP_AND_BOTTOM
     *
     * @return set status
     */
    public boolean setDisplay3DFormat(int format) {
        synchronized (mLock) {
            Mutable<Integer> resultVal = new Mutable<>();
            try {
                mProxy.setDisplay3DFormat(format);
            } catch (RemoteException e) {
                Log.e(TAG, "setDisplay3DFormat:" + e);
            }
        }

        return true;
    }

    /**
     * Get display 3D format setted by setDisplay3DFormat.
     *
     * @return 3D format
     * FORMAT_3D_OFF
     * FORMAT_3D_AUTO
     * FORMAT_3D_SIDE_BY_SIDE
     * FORMAT_3D_TOP_AND_BOTTOM
     */
    public int getDisplay3DFormat() {
        synchronized (mLock) {
            Mutable<Integer> resultVal = new Mutable<>();
            try {
                mProxy.getDisplay3DFormat((int ret, int v) -> {
                                if (Result.OK == ret) {
                                    resultVal.value = v;
                                }
                            });
                return resultVal.value;
            } catch (RemoteException e) {
                Log.e(TAG, "getDisplay3DFormat:" + e);
            }
        }
        return -1;
    }

    /**
     * for subtitle, maybe unnecessary
     */
    public boolean setOsd3DFormat(int format) {
        synchronized (mLock) {
            Mutable<Integer> resultVal = new Mutable<>();
            try {
                mProxy.setOsd3DFormat(format);
            } catch (RemoteException e) {
                Log.e(TAG, "setOsd3DFormat:" + e);
            }
        }

        return true;
    }

    /**
     * Switch 3D to 2D for video, this api is used for tv platform if user wanna watch movie part of 3D files,
     * take left side or top side for example
     *
     * @param 3D format
     * FORMAT_3D_TO_2D_LEFT_EYE
     * FORMAT_3D_TO_2D_RIGHT_EYE
     *
     * @return set status
     */
    public boolean switch3DTo2D(int format) {
        synchronized (mLock) {
            Mutable<Integer> resultVal = new Mutable<>();
            try {
                mProxy.switch3DTo2D(format);
            } catch (RemoteException e) {
                Log.e(TAG, "switch3DTo2D:" + e);
            }
        }

        return true;
    }

    /**
     * // TODO: haven't implemented yet
     */
    public boolean switch2DTo3D(int format) {
        synchronized (mLock) {
            Mutable<Integer> resultVal = new Mutable<>();
            try {
                mProxy.switch2DTo3D(format);
            } catch (RemoteException e) {
                Log.e(TAG, "switch2DTo3D:" + e);
            }
        }

        return true;
    }

    //PQ moudle
    public enum SourceInput {
        TV(0),
        AV1(1),
        AV2(2),
        YPBPR1(3),
        YPBPR2(4),
        HDMI1(5),
        HDMI2(6),
        HDMI3(7),
        HDMI4(8),
        VGA(9),
        XXXX(10),//not use MPEG source
        DTV(11),
        SVIDEO(12),
        IPTV(13),
        DUMMY(14),
        SOURCE_SPDIF(15),
        ADTV(16),
        AUX(17),
        ARC(18),
        MAX(19);
        private int val;

        SourceInput(int val) {
            this.val = val;
        }

        public static SourceInput valueOf(int value) {
            for (SourceInput sinput : SourceInput.values()) {
                if (sinput.toInt() == value) {
                    return sinput;
                }
            }
          return XXXX;
        }

        public int toInt() {
            return this.val;
        }
    }

   public enum PQMode {
        PQ_MODE_STANDARD(0),
        PQ_MODE_BRIGHT(1),
        PQ_MODE_SOFTNESS(2),
        PQ_MODE_USER(3),
        PQ_MODE_MOVIE(4),
        PQ_MODE_COLORFUL(5),
        PQ_MODE_MONITOR(6),
        PQ_MODE_GAME(7),
        PQ_MODE_SPORTS(8),
        PQ_MODE_SONY(9),
        PQ_MODE_SAMSUNG(10),
        PQ_MODE_SHARP(11);

        private int val;

        PQMode(int val) {
            this.val = val;
        }

        public int toInt() {
            return this.val;
        }
    }

    public int LoadPQSettings(SourceInputParam srcInputParam) {
        synchronized (mLock) {
            try {
                return mProxy.loadPQSettings(srcInputParam);
            } catch (RemoteException e) {
                Log.e(TAG, "LoadPQSettings:" + e);
            }
        }

        return -1;

    }

      /**
      * @Function: SetPQMode
      * @Description: Set current source picture mode
      * @Param: value mode refer to enum Pq_Mode, source refer to enum SourceInput, is_save 1 to save
      * @Return: 0 success, -1 fail
      */
     public int SetPQMode(int pq_mode, int is_save, int is_autoswitch) {
         synchronized (mLock) {
             try {
                 return mProxy.setPQmode(pq_mode, is_save, is_autoswitch);
             } catch (RemoteException e) {
                 Log.e(TAG, "SetPQMode:" + e);
             }
         }

        return -1;

    }

        /**
     * @Function: GetPQMode
     * @Description: Get current source picture mode
     * @Param: source refer to enum SourceInput
     * @Return: picture mode refer to enum Pq_Mode
     */
    public int GetPQMode() {
        synchronized (mLock) {
            try {
                return mProxy.getPQmode();
            } catch (RemoteException e) {
                Log.e(TAG, "getDisplay3DFormat:" + e);
            }
        }
        return -1;
    }

        /**
     * @Function: SavePQMode
     * @Description: Save current source picture mode
     * @Param: picture mode refer to enum Pq_Mode, source refer to enum SourceInput
     * @Return: 0 success, -1 fail
     */
    public int SavePQMode(int pq_mode) {
        synchronized (mLock) {
            try {
                return mProxy.savePQmode(pq_mode);
            } catch (RemoteException e) {
                Log.e(TAG, "SavePQMode:" + e);
            }
        }
        return -1;
    }

   public enum color_temperature {
        COLOR_TEMP_STANDARD(0),
        COLOR_TEMP_WARM(1),
        COLOR_TEMP_COLD(2),
        COLOR_TEMP_USER(3),
        COLOR_TEMP_MAX(4);
        private int val;
        color_temperature(int val) {
            this.val = val;
        }
        public int toInt() {
            return this.val;
        }
    }

    public enum rgb_type{
        TYPE_INVALID(-1),
        R_GAIN(0),
        G_GAIN(1),
        B_GAIN(2),
        R_POST_OFFSET(3),
        G_POST_OFFSET(4),
        B_POST_OFFSET(5),
        RGB_TYPE_MAX(6);
        private int val;

        rgb_type(int val) {
            this.val = val;
        }

        public int toInt() {
            return this.val;
        }
    }

    /**
     * @Function: SetColorTemperature
     * @Description: Set current source color temperature mode
     * @Param: value mode refer to enum color_temperature, source refer to enum SourceInput, is_save 1 to save
     * @Return: 0 success, -1 fail
     */
    public int SetColorTemperature(int mode, int is_save) {
       synchronized (mLock) {
           try {
               return mProxy.setColorTemperature(mode, is_save);
           } catch (RemoteException e) {
                Log.e(TAG, "SetColorTemperature:" + e);
           }
       }
       return -1;

    }

        /**
     * @Function: GetColorTemperature
     * @Description: Get current source color temperature mode
     * @Param: source refer to enum SourceInput
     * @Return: color temperature refer to enum color_temperature
     */
    public int GetColorTemperature() {
        synchronized (mLock) {
            try {
                return mProxy.getColorTemperature();
            } catch (RemoteException e) {
                Log.e(TAG, "GetColorTemperature:" + e);
            }
        }
        return -1;
    }

        /**
     * @Function: SaveColorTemperature
     * @Description: Save current source color temperature mode
     * @Param: color temperature mode refer to enum color_temperature, source refer to enum SourceInput
     * @Return: 0 success, -1 fail
     */
    public int SaveColorTemperature(int mode) {
          synchronized (mLock) {
            try {
                return mProxy.saveColorTemperature(mode);
            } catch (RemoteException e) {
                Log.e(TAG, "SaveColorTemperature:" + e);
            }
        }
        return -1;
    }

    /**
     * @Function: SetColorTemperatureUserParam
     * @Description: Set current source color temperature mode for user mode
     * @Param: value mode refer to enum color_temperature, is_save 1 to save
     * @param: type refer to enum rgb_type, value between -1024 to 2047
     * @Return: 0 success, -1 fail
     */
    public int SetColorTemperatureUserParam(color_temperature mode, int is_save, rgb_type type, int value) {
        synchronized (mLock) {
            try {
                return mProxy.setColorTemperatureUserParam(mode.toInt(), is_save, type.toInt(), value);
            } catch (Exception e) {
                 Log.e(TAG, "SetColorTemperatureUserParam:" + e);
            }
        }
        return -1;

    }

    /**
     * @Function: SetColorTemperatureUserParam
     * @Description: Get the params of user color temperature mode
     * @Return: color temperature params refer to class WhiteBalanceParams
     */
    public WhiteBalanceParams GetColorTemperatureUserParam() {
        WhiteBalanceParams params = new WhiteBalanceParams();
        synchronized (mLock) {
            try {
                WhiteBalanceParam tempParam = mProxy.getColorTemperatureUserParam();
                params.r_gain = tempParam.r_gain;
                params.g_gain = tempParam.g_gain;
                params.b_gain = tempParam.b_gain;
                params.r_offset = tempParam.r_post_offset;
                params.g_offset = tempParam.g_post_offset;
                params.b_offset = tempParam.b_post_offset;
             } catch (Exception e) {
                  Log.e(TAG, "GetColorTemperatureUserParam:" + e);
             }
         }

         return params;
    }
    /**
     * @Function: SetBrightness
     * @Description: Set current source brightness value
     * @Param: value brightness, source refer to enum SourceInput, is_save 1 to save
     * @Return: 0 success, -1 fail
     */
    public int SetBrightness(int value, int is_save) {
          synchronized (mLock) {
            try {
                return mProxy.setBrightness(value, is_save);
            } catch (RemoteException e) {
                Log.e(TAG, "SetBrightness:" + e);
            }
        }
        return -1;
    }

    /**
     * @Function: GetBrightness
     * @Description: Get current source brightness value
     * @Param: source refer to enum SourceInput
     * @Return: value brightness
     */
    public int GetBrightness() {
          synchronized (mLock) {
          try {
              return mProxy.getBrightness();
          } catch (RemoteException e) {
              Log.e(TAG, "GetBrightness:" + e);
          }
      }
      return -1;

    }

    /**
     * @Function: SaveBrightness
     * @Description: Save current source brightness value
     * @Param: value brightness, source refer to enum SourceInput
     * @Return: 0 success, -1 fail
     */
    public int SaveBrightness(int value) {
          synchronized (mLock) {
            try {
                return mProxy.saveBrightness(value);
            } catch (RemoteException e) {
                Log.e(TAG, "SaveBrightness:" + e);
            }
        }
        return -1;
    }

    /**
     * @Function: SetContrast
     * @Description: Set current source contrast value
     * @Param: value contrast, source refer to enum SourceInput, is_save 1 to save
     * @Return: 0 success, -1 fail
     */
    public int SetContrast(int value, int is_save) {
          synchronized (mLock) {
            try {
                return mProxy.setContrast(value, is_save);
            } catch (RemoteException e) {
                Log.e(TAG, "SetContrast:" + e);
            }
        }
        return -1;

    }

    /**
     * @Function: GetContrast
     * @Description: Get current source contrast value
     * @Param: source refer to enum SourceInput
     * @Return: value contrast
     */
    public int GetContrast() {
        synchronized (mLock) {
            try {
                return mProxy.getContrast();
            } catch (RemoteException e) {
                Log.e(TAG, "GetContrast:" + e);
            }
        }
        return -1;

    }

    /**
     * @Function: SaveContrast
     * @Description: Save current source contrast value
     * @Param: value contrast, source refer to enum SourceInput
     * @Return: 0 success, -1 fail
     */
    public int SaveContrast(int value) {
          synchronized (mLock) {
            try {
                return mProxy.saveContrast(value);
            } catch (RemoteException e) {
                Log.e(TAG, "SaveContrast:" + e);
            }
        }
        return -1;

    }

        /**
     * @Function: SetSatuation
     * @Description: Set current source saturation value
     * @Param: value saturation, source refer to enum SourceInput, fmt current fmt refer to tvin_sig_fmt_e, is_save 1 to save
     * @Return: 0 success, -1 fail
     */
    public int SetSaturation(int value, int is_save) {
          synchronized (mLock) {
            try {
                return mProxy.setSaturation(value, is_save);
            } catch (RemoteException e) {
                Log.e(TAG, "SetBrightness:" + e);
            }
        }
        return -1;

    }

    /**
       * @Function: GetSatuation
       * @Description: Get current source saturation value
       * @Param: source refer to enum SourceInput
       * @Return: value saturation
       */
      public int GetSaturation() {
        synchronized (mLock) {
            try {
                return mProxy.getSaturation();
            } catch (RemoteException e) {
                Log.e(TAG, "GetSaturation:" + e);
            }
        }
        return -1;

      }

          /**
     * @Function: SaveSaturation
     * @Description: Save current source saturation value
     * @Param: value saturation, source refer to enum SourceInput
     * @Return: 0 success, -1 fail
     */
    public int SaveSaturation(int value) {
            synchronized (mLock) {
              try {
                  return mProxy.saveSaturation(value);
              } catch (RemoteException e) {
                  Log.e(TAG, "SaveSaturation:" + e);
              }
          }
          return -1;

    }

    /**
     * @Function: SetHue
     * @Description: Set current source hue value
     * @Param: value saturation, source refer to enum SourceInput, fmt current fmt refer to tvin_sig_fmt_e, is_save 1 to save
     * @Return: 0 success, -1 fail
     */
    public int SetHue(int value, int is_save) {
          synchronized (mLock) {
            try {
                return mProxy.setHue(value, is_save);
            } catch (RemoteException e) {
                Log.e(TAG, "SetHue:" + e);
            }
        }
        return -1;

    }

    /**
     * @Function: GetHue
     * @Description: Get current source hue value
     * @Param: source refer to enum SourceInput
     * @Return: value hue
     */
    public int GetHue() {
        synchronized (mLock) {
            try {
                return mProxy.getHue();
            } catch (RemoteException e) {
                Log.e(TAG, "GetHue:" + e);
            }
        }
        return -1;

    }

    /**
     * @Function: SaveHue
     * @Description: Save current source hue value
     * @Param: value hue, source refer to enum SourceInput
     * @Return: 0 success, -1 fail
     */
    public int SaveHue(int value) {
          synchronized (mLock) {
            try {
                return mProxy.saveHue(value);
            } catch (RemoteException e) {
                Log.e(TAG, "SaveHue:" + e);
            }
        }
        return -1;


    }

    /**
     * @Function: SetSharpness
     * @Description: Set current source sharpness value
     * @Param: value saturation, source_type refer to enum SourceInput, is_enable set 1 as default
     * @Param: status_3d refer to enum Tvin_3d_Status, is_save 1 to save
     * @Return: 0 success, -1 fail
     */
    public int SetSharpness(int value, int is_enable, int is_save) {
          synchronized (mLock) {
            try {
                return mProxy.setSharpness(value, is_enable, is_save);
            } catch (RemoteException e) {
                Log.e(TAG, "SetSharpness:" + e);
            }
        }
        return -1;

    }

    /**
     * @Function: GetSharpness
     * @Description: Get current source sharpness value
     * @Param: source refer to enum SourceInput
     * @Return: value sharpness
     */
    public int GetSharpness() {
        synchronized (mLock) {
            try {
                return mProxy.getSharpness();
            } catch (RemoteException e) {
                Log.e(TAG, "GetSharpness:" + e);
            }
        }
        return -1;

    }

    /**
     * @Function: SaveSharpness
     * @Description: Save current source sharpness value
     * @Param: value sharpness, source refer to enum SourceInput, isEnable set 1 enable as default
     * @Return: 0 success, -1 fail
     */
    public int SaveSharpness(int value, int isEnable) {
          synchronized (mLock) {
            try {
                return mProxy.saveSharpness(value);
            } catch (RemoteException e) {
                Log.e(TAG, "SaveHue:" + e);
            }
        }
        return -1;

    }

    public enum Noise_Reduction_Mode {
        REDUCE_NOISE_CLOSE(0),
        REDUCE_NOISE_WEAK(1),
        REDUCE_NOISE_MID(2),
        REDUCE_NOISE_STRONG(3),
        REDUCTION_MODE_AUTO(4);

        private int val;

        Noise_Reduction_Mode(int val) {
            this.val = val;
        }

        public int toInt() {
            return this.val;
        }
    }

    /**
     * @Function: SetNoiseReductionMode
     * @Description: Set current source noise reduction mode
     * @Param: noise reduction mode refer to enum Noise_Reduction_Mode, source refer to enum SourceInput, is_save 1 to save
     * @Return: 0 success, -1 fail
     */
    public int SetNoiseReductionMode(int nr_mode, int is_save) {
          synchronized (mLock) {
            try {
                return mProxy.setNoiseReductionMode(nr_mode, is_save);
            } catch (RemoteException e) {
                Log.e(TAG, "SetNoiseReductionMode:" + e);
            }
        }
        return -1;

    }

    /**
     * @Function: GetNoiseReductionMode
     * @Description: Get current source noise reduction mode
     * @Param: source refer to enum SourceInput
     * @Return: noise reduction mode refer to enum Noise_Reduction_Mode
     */
    public int GetNoiseReductionMode() {
        synchronized (mLock) {
            try {
                return mProxy.getNoiseReductionMode();
            } catch (RemoteException e) {
                Log.e(TAG, "GetNoiseReductionMode:" + e);
            }
        }
        return -1;
    }

        /**
     * @Function: SaveNoiseReductionMode
     * @Description: Save current source noise reduction mode
     * @Param: noise reduction mode refer to enum Noise_Reduction_Mode, source refer to enum SourceInput
     * @Return: 0 success, -1 fail
     */
    public int SaveNoiseReductionMode(int nr_mode) {
          synchronized (mLock) {
            try {
                return mProxy.saveNoiseReductionMode(nr_mode);
            } catch (RemoteException e) {
                Log.e(TAG, "SaveNoiseReductionMode:" + e);
            }
        }
        return -1;
    }

    /**
     * @Function: SetSmoothPlusMode
     * @Description: Set current source smooth plus mode
     * @Param: smooth plus mode refer to enum vpp_smooth_plus_mode_e, source refer to enum SourceInput, is_save 1 to save
     * @Return: 0 success, -1 fail
     */
    public int SetSmoothPlusMode(int smoothplus_mode, int is_save) {
          synchronized (mLock) {
            try {
                return mProxy.setSmoothPlusMode(smoothplus_mode, is_save);
            } catch (Exception e) {
                Log.e(TAG, "setSmoothPlusMode:" + e);
            }
        }
        return -1;
    }

    /**
     * @Function: GetSmoothPlusMode
     * @Description: Get current source smooth plus mode
     * @Param: source refer to enum SourceInput
     * @Return: smooth plus mode refer to enum Smooth_Plus_Mode
     */
    public int GetSmoothPlusMode() {
        synchronized (mLock) {
            try {
                return mProxy.getSmoothPlusMode();
            } catch (Exception e) {
                Log.e(TAG, "getSmoothPlusMode:" + e);
            }
        }
        return -1;
    }

    public enum HDR_TMO_Mode {
        HDR_TMO_OFF(0),
        HDR_TMO_DYNAMIC(1),
        HDR_TMO_STATIC(2);

        private int val;

        HDR_TMO_Mode(int val) {
            this.val = val;
        }

        public int toInt() {
            return this.val;
        }
    }

    /**
     * @Function: SetHDRTMOMode
     * @Description: Set current source hdr tmo mode
     * @Param: hdr_tmo_mode refer to enum HDR_TMO_Mode, is_save 1 to save
     * @Return: 0 success, -1 fail
     */
    public int SetHDRTMOMode(int hdr_tmo_mode, int is_save) {
          synchronized (mLock) {
            try {
                return mProxy.setHDRTMOMode(hdr_tmo_mode, is_save);
            } catch (Exception e) {
                Log.e(TAG, "setHDRTMOMode:" + e);
            }
        }
        return -1;
    }

    /**
     * @Function: GetHDRTMOMode
     * @Description: Get current source hdr tmo mode
     * @Param:
     * @Return: hdr tmo mode refer to enum HDR_TMO_Mode
     */
    public int GetHDRTMOMode() {
        synchronized (mLock) {
            try {
                return mProxy.getHDRTMOMode();
            } catch (Exception e) {
                Log.e(TAG, "getHDRTMOMode:" + e);
            }
        }
        return -1;
    }

    public int SetEyeProtectionMode(int inputtSrc, int enable, int isSave) {
          synchronized (mLock) {
            try {
                return mProxy.setEyeProtectionMode(inputtSrc, enable, isSave);
            } catch (RemoteException e) {
                Log.e(TAG, "SetEyeProtectionMode:" + e);
            }
        }
        return -1;
    }

    public int GetEyeProtectionMode(int inputtSrc) {
         synchronized (mLock) {
            try {
                return mProxy.getEyeProtectionMode(inputtSrc);
            } catch (RemoteException e) {
                Log.e(TAG, "GetEyeProtectionMode:" + e);
            }
        }
        return -1;
    }

    public enum gamma_curve {
        GAMMA_CURVE_DEFAULT(0),
        GAMMA_CURVE_1(1),
        GAMMA_CURVE_2(2),
        GAMMA_CURVE_3(3),
        GAMMA_CURVE_4(4),
        GAMMA_CURVE_5(5),
        GAMMA_CURVE_6(6),
        GAMMA_CURVE_7(7),
        GAMMA_CURVE_8(8),
        GAMMA_CURVE_9(9),
        GAMMA_CURVE_10(10),
        GAMMA_CURVE_11(11);

        private int val;

        gamma_curve(int val) {
            this.val = val;
        }

        public int toInt() {
            return this.val;
        }
    }

    /**
     * @Function: SetGammaValue
     * @Description: Set gamma curve
     * @Param: curve refer to enum gamma_curve, is_save 1 to save
     * @Return: 0 success, -1 fail
     */
    public int SetGammaValue(int curve, int isSave) {
          synchronized (mLock) {
            try {
                return mProxy.setGammaValue(curve, isSave);
            } catch (RemoteException e) {
                Log.e(TAG, "SetGammaValue:" + e);
            }
        }
        return -1;

    }

    /**
     * @Function: GetGammaValue
     * @Description: Get current gamma curve
     * @Param:
     * @Return: gamma curve refer to enum gamma_curve
     */
    public int GetGammaValue() {
          synchronized (mLock) {
            try {
                return mProxy.getGammaValue();
            } catch (RemoteException e) {
                Log.e(TAG, "GetGammaValue:" + e);
            }
        }
        return -1;

    }

    public enum Display_Mode {
        DISPLAY_MODE_169(0),
        DISPLAY_MODE_PERSON(1),
        DISPLAY_MODE_MOVIE(2),
        DISPLAY_MODE_CAPTION(3),
        DISPLAY_MODE_MODE43(4),
        DISPLAY_MODE_FULL(5),
        DISPLAY_MODE_NORMAL(6),
        DISPLAY_MODE_NOSCALEUP(7),
        DISPLAY_MODE_CROP_FULL(8),
        DISPLAY_MODE_CROP(9),
        DISPLAY_MODE_ZOOM(10),
        DISPLAY_MODE_MAX(11);
        private int val;

        Display_Mode(int val) {
            this.val = val;
        }

        public int toInt() {
            return this.val;
        }
    }

    public boolean hasMemcFunc() {
          synchronized (mLock) {
            try {
                return (mProxy.hasMemcFunc() == Result.OK);
            } catch (RemoteException e) {
                Log.e(TAG, "hasMemcFunc:" + e);
            }
        }
        return false;
    }

    public int SetMemcMode(int mode, int isSave) {
          synchronized (mLock) {
            try {
                return mProxy.setMemcMode(mode, isSave);
            } catch (RemoteException e) {
                Log.e(TAG, "SetMemcMode:" + e);
            }
        }
        return -1;
    }

    public int GetMemcMode() {
          synchronized (mLock) {
            try {
                return mProxy.getMemcMode();
            } catch (RemoteException e) {
                Log.e(TAG, "GetMemcMode:" + e);
            }
        }
        return -1;
    }

    public int SetMemcDeBlurLevel(int level, int isSave) {
          synchronized (mLock) {
            try {
                return mProxy.setMemcDeBlurLevel(level, isSave);
            } catch (RemoteException e) {
                Log.e(TAG, "SetMemcDeBlurLevel:" + e);
            }
        }
        return -1;
    }

    public int GetMemcDeBlurLevel() {
          synchronized (mLock) {
            try {
                return mProxy.getMemcDeBlurLevel();
            } catch (RemoteException e) {
                Log.e(TAG, "GetMemcDeBlurLevel:" + e);
            }
        }
        return -1;
    }

    public int SetMemcDeJudderLevel(int level, int isSave) {
          synchronized (mLock) {
            try {
                return mProxy.setMemcDeJudderLevel(level, isSave);
            } catch (RemoteException e) {
                Log.e(TAG, "SetMemcDeJudderLevel:" + e);
            }
        }
        return -1;
    }

    public int GetMemcDeJudderLevel() {
          synchronized (mLock) {
            try {
                return mProxy.getMemcDeJudderLevel();
            } catch (RemoteException e) {
                Log.e(TAG, "GetMemcDeJudderLevel:" + e);
            }
        }
        return -1;
    }

    public int SetDisplayMode(int inputtSrc, Display_Mode mode, int isSave) {
          synchronized (mLock) {
            try {
                return mProxy.setDisplayMode(inputtSrc, mode.toInt(), isSave);
            } catch (RemoteException e) {
                Log.e(TAG, "SetDisplayMode:" + e);
            }
        }
        return -1;
    }

    public int GetDisplayMode(int inputtSrc) {
          synchronized (mLock) {
            try {
                return mProxy.getDisplayMode(inputtSrc);
            } catch (RemoteException e) {
                Log.e(TAG, "GetDisplayMode:" + e);
            }
        }
        return -1;
    }

    public int SaveDisplayMode(int inputtSrc, Display_Mode mode) {
          synchronized (mLock) {
            try {
                return mProxy.saveDisplayMode(inputtSrc, mode.toInt());
            } catch (RemoteException e) {
                Log.e(TAG, "SaveDisplayMode:" + e);
            }
        }
        return -1;
    }

     public int SetBacklight(int value, int isSave) {
           synchronized (mLock) {
             try {
                 return mProxy.setBacklight(value, isSave);
             } catch (RemoteException e) {
                 Log.e(TAG, "SetBacklight:" + e);
             }
         }
         return -1;
     }

     public int GetBacklight() {
           synchronized (mLock) {
             try {
                 return mProxy.getBacklight();
             } catch (RemoteException e) {
                 Log.e(TAG, "GetBacklight:" + e);
             }
         }
         return -1;
     }

     public int SaveBacklight(int value) {
           synchronized (mLock) {
             try {
                 return mProxy.saveBacklight(value);
             } catch (RemoteException e) {
                 Log.e(TAG, "SaveBacklight:" + e);
             }
         }
         return -1;
     }

     /**
      * @Function: CheckLdimExist
      * @Description: check local diming moudle exist or not
      * @Param:
      * @Return: true: exist, false: don't exist
      */
     public boolean CheckLdimExist() {
           synchronized (mLock) {
             try {
                 int ret = mProxy.checkLdimExist();
                 if (ret == 0) {
                     return false;
                 } else {
                     return true;
                 }
             } catch (RemoteException e) {
                 Log.e(TAG, "CheckLdimExist:" + e);
             }
         }
         return false;
     }

     public enum Dynamic_Backlight_Mode {
         DYNAMIC_BACKLIGHT_OFF(0),
         DYNAMIC_BACKLIGHT_LOW(1),
         DYNAMIC_BACKLIGHT_HIGH(2);

         private int val;

         Dynamic_Backlight_Mode(int val) {
             this.val = val;
         }

         public static Dynamic_Backlight_Mode valueOf(int value) {
             for (Dynamic_Backlight_Mode dbm : Dynamic_Backlight_Mode.values()) {
                 if (dbm.toInt() == value) {
                     return dbm;
                 }
             }
           return DYNAMIC_BACKLIGHT_OFF;
         }

         public int toInt() {
             return this.val;
         }
     }

     /**
      * @Function: SetDynamicBacklight
      * @Description: Set current source dynamic backlight mode
      * @Param: dynamic backlight mode refer to enum Dynamic_Backlight_Mode, source refer to enum SourceInput, is_save 1 to save
      * @Return: 0 success, -1 fail
      */
     public int SetDynamicBacklight(Dynamic_Backlight_Mode mode, int isSave) {
           synchronized (mLock) {
             try {
                 return mProxy.setDynamicBacklight(mode.toInt(), isSave);
             } catch (RemoteException e) {
                 Log.e(TAG, "SetDynamicBacklight:" + e);
             }
         }
         return -1;
     }

     /**
      * @Function: GetDynamicBacklight
      * @Description: Get current source dynamic backlight mode
      * @Param: source refer to enum SourceInput
      * @Return: dynamic backlight mode refer to enum Dynamic_Backlight_Mode
      */
     public int GetDynamicBacklight() {
           synchronized (mLock) {
             try {
                 return mProxy.getDynamicBacklight();
             } catch (RemoteException e) {
                 Log.e(TAG, "GetDynamicBacklight:" + e);
             }
         }
         return -1;
     }

     public enum Local_Contrast_Mode {
         LOCAL_CONTRAST_MODE_OFF(0),
         LOCAL_CONTRAST_MODE_LOW(1),
         LOCAL_CONTRAST_MODE_MID(2),
         LOCAL_CONTRAST_MODE_HIGH(3);

         private int val;

         Local_Contrast_Mode(int val) {
             this.val = val;
         }

         public int toInt() {
             return this.val;
         }
     }
     /**
      * @Function: SetLocalContrastMode
      * @Description: Get current source Local Contrast Mode
      * @Param: mode refer to enum Local_Contrast_Mode, isSave whether need save set value
      * @Return: 0 success, -1 fail
      */
     public int SetLocalContrastMode(Local_Contrast_Mode mode, int isSave) {
           synchronized (mLock) {
             try {
                 return mProxy.setLocalContrastMode(mode.toInt(), isSave);
             } catch (Exception e) {
                 Log.e(TAG, "SetLocalContrastMode:" + e);
             }
         }
         return -1;
     }

     /**
      * @Function: GetLocalContrastMode
      * @Description: Get current source Local Contrast Mode
      * @Param:
      * @Return: Local Contrast Mode refer to enum Local_Contrast_Mode
      */
     public int GetLocalContrastMode() {
           synchronized (mLock) {
             try {
                 return mProxy.getLocalContrastMode();
             } catch (Exception e) {
                 Log.e(TAG, "GetLocalContrastMode:" + e);
             }
         }
         return -1;
     }

     public enum Black_Extension_Mode {
         BLACK_EXTENSION_MODE_OFF(0),
         BLACK_EXTENSION_MODE_LOW(1),
         BLACK_EXTENSION_MODE_MID(2),
         BLACK_EXTENSION_MODE_HIGH(3);

         private int val;

         Black_Extension_Mode(int val) {
             this.val = val;
         }

         public static Black_Extension_Mode valueOf(int value) {
             for (Black_Extension_Mode dem : Black_Extension_Mode.values()) {
                 if (dem.toInt() == value) {
                     return dem;
                 }
             }
           return BLACK_EXTENSION_MODE_OFF;
         }

         public int toInt() {
             return this.val;
         }
     }
     /**
      * @Function: SetBlackExtensionMode
      * @Description: Set current source Black Extension Mode
      * @Param: mode refer to enum Black_Extension_Mode, isSave whether need save set value
      * @Return: 0 success, -1 fail
      */
     public int SetBlackExtensionMode(Black_Extension_Mode mode, int isSave) {
           synchronized (mLock) {
             try {
                 return mProxy.setBlackExtensionMode(mode.toInt(), isSave);
             } catch (Exception e) {
                 Log.e(TAG, "SetBlackExtensionMode:" + e);
             }
         }
         return -1;
     }

     /**
      * @Function: GetBlackExtensionMode
      * @Description: Get current source Black Extension Mode
      * @Param:
      * @Return: Black Extension Mode refer to enum Black_Extension_Mode
      */
     public int GetBlackExtensionMode() {
           synchronized (mLock) {
             try {
                 return mProxy.getBlackExtensionMode();
             } catch (Exception e) {
                 Log.e(TAG, "GetBlackExtensionMode:" + e);
             }
         }
         return -1;
     }

     public enum Deblock_Mode {
         DI_DEBLOCK_MODE_OFF(0),
         DI_DEBLOCK_MODE_LOW(1),
         DI_DEBLOCK_MODE_MIDDLE(2),
         DI_DEBLOCK_MODE_HIGH(3),
         DI_DEBLOCK_MODE_AUTO(4);

         private int val;

         Deblock_Mode(int val) {
             this.val = val;
         }

         public static Deblock_Mode valueOf(int value) {
             for (Deblock_Mode dm : Deblock_Mode.values()) {
                 if (dm.toInt() == value) {
                     return dm;
                 }
             }
           return DI_DEBLOCK_MODE_OFF;
         }

         public int toInt() {
             return this.val;
         }
     }
     /**
      * @Function: SetDeblockMode
      * @Description: Set current source deblock Mode
      * @Param: mode refer to enum Deblock_Mode, isSave whether need save set value
      * @Return: 0 success, -1 fail
      */
     public int SetDeblockMode(Deblock_Mode mode, int isSave) {
           synchronized (mLock) {
             try {
                 return mProxy.setDeblockMode(mode.toInt(), isSave);
             } catch (Exception e) {
                 Log.e(TAG, "SetDeblockMode:" + e);
             }
         }
         return -1;
     }

     /**
      * @Function: GetDeblockMode
      * @Description: Get current source deblock Mode
      * @Param:
      * @Return: deblock Mode refer to enum Deblock_Mode
      */
     public int GetDeblockMode() {
           synchronized (mLock) {
             try {
                 return mProxy.getDeblockMode();
             } catch (Exception e) {
                 Log.e(TAG, "GetDeblockMode:" + e);
             }
         }
         return -1;
     }

     public enum DemoSquito_Mode {
         DI_DEMOSQUITO_MODE_OFF(0),
         DI_DEMOSQUITO_MODE_LOW(1),
         DI_DEMOSQUITO_MODE_MIDDLE(2),
         DI_DEMOSQUITO_MODE_HIGH(3),
         DI_DEMOSQUITO_MODE_AUTO(4);

         private int val;

         DemoSquito_Mode(int val) {
             this.val = val;
         }

         public static DemoSquito_Mode valueOf(int value) {
             for (DemoSquito_Mode dsm : DemoSquito_Mode.values()) {
                 if (dsm.toInt() == value) {
                     return dsm;
                 }
             }
           return DI_DEMOSQUITO_MODE_OFF;
         }

         public int toInt() {
             return this.val;
         }
     }
     /**
      * @Function: SetDemoSquitoMode
      * @Description: Set current source demosquito Mode
      * @Param: mode refer to enum DemoSquito_Mode, isSave whether need save set value
      * @Return: 0 success, -1 fail
      */
     public int SetDemoSquitoMode(DemoSquito_Mode mode, int isSave) {
           synchronized (mLock) {
             try {
                 return mProxy.setDemoSquitoMode(mode.toInt(), isSave);
             } catch (Exception e) {
                 Log.e(TAG, "SetDemoSquitoMode:" + e);
             }
         }
         return -1;
     }

     /**
      * @Function: GetDemoSquitoMode
      * @Description: Get current source demosquito Mode
      * @Param:
      * @Return: demosquito Mode refer to enum DemoSquito_Mode
      */
     public int GetDemoSquitoMode() {
           synchronized (mLock) {
             try {
                 return mProxy.getDemoSquitoMode();
             } catch (Exception e) {
                 Log.e(TAG, "GetDemoSquitoMode:" + e);
             }
         }
         return -1;
     }

     public enum ColorBaseMode {
         COLOR_BASE_MODE_OFF(0),
         COLOR_BASE_MODE_OPTIMIZE(1),
         COLOR_BASE_MODE_ENHANCE(2),
         COLOR_BASE_MODE_DEMO(3),
         COLOR_BASE_MODE_MAX(4);
         private int val;

         ColorBaseMode(int val) {
             this.val = val;
         }

         public int toInt() {
             return this.val;
         }
     }
     /**
      * @Function: SetColorBaseMode
      * @Description:  Set Color Base Mode of the current source
      * @Param source_input: refer to enum SourceInput
      * @Param mode: refer to enum ColorBaseMode
      * @Param is_save: refer to whether need save
      * @Return: 0 success, -1 fail
      */
     public int SetColorBaseMode(ColorBaseMode mode, int is_save) {
         synchronized (mLock) {
             try {
                 return mProxy.setColorBaseMode(mode.toInt(), is_save);
             } catch (Exception e) {
                 Log.e(TAG, "SetColorBaseMode:" + e);
             }
         }
         return -1;
     }

     /**
      * @Function: GetColorBaseMode
      * @Description: Get Color Base Mode of current source
      * @Return: mode of the special source
      */
     public int GetColorBaseMode() {
         synchronized (mLock) {
             try {
                 return mProxy.getColorBaseMode();
             } catch (Exception e) {
                 Log.e(TAG, "GetColorBaseMode:" + e);
             }
         }
         return -1;
     }

     public enum SourceHdrType {
          SOURCE_HDR_TYPE_NONE(0),
          SOURCE_HDR_TYPE_HDR10(1),
          SOURCE_HDR_TYPE_HDR10PLUS(2),
          SOURCE_HDR_TYPE_DOVI(3),
          SOURCE_HDR_TYPE_PRIMESL(4),
          SOURCE_HDR_TYPE_HLG(5),
          SOURCE_HDR_TYPE_SDR(6),
          SOURCE_HDR_TYPE_MVC(7);
          private int val;

          SourceHdrType(int val) {
              this.val = val;
          }

          public int toInt() {
              return this.val;
          }
      }

      /**
       * @Function: GetSourceHdrType
       * @Description: Get hdr type of current source
       * @Return: hdr type of the special source
       */
      public int GetSourceHdrType() {
          synchronized (mLock) {
              try {
                  return mProxy.getSourceHdrType();
              } catch (Exception e) {
                  Log.e(TAG, "getSourceHdrType:" + e);
              }
          }
          return -1;
      }

     /**
      * @Function: FactorySetPQMode_Brightness
      * @Description: Adjust brightness value in corresponding pq mode for factory menu conctrol
      * @Param: source_type refer to enum SourceInput_Type, PQMode refer to enum Pq_Mode, brightness brightness value
      * @Return: 0 success, -1 fail
      */
     public int FactorySetPQMode_Brightness(SourceInput source_input, SignalFmt sig_fmt, TransFmt trans_fmt, PQMode pq_mode, int brightness) {
           synchronized (mLock) {
             try {
                 return mProxy.factorySetPQMode_Brightness(source_input.toInt(), sig_fmt.toInt(), trans_fmt.toInt(), pq_mode.toInt(), brightness);
             } catch (RemoteException e) {
                 Log.e(TAG, "factorySetPQMode_Brightness:" + e);
             }
         }
         return -1;
     }

     /**
      * @Function: FactoryGetPQMode_Brightness
      * @Description: Get brightness value in corresponding pq mode for factory menu conctrol
      * @Param: source_type refer to enum SourceInput_Type, PQMode refer to enum Pq_Mode
      * @Return: 0 success, -1 fail
      */
     public int FactoryGetPQMode_Brightness(SourceInput source_input, SignalFmt sig_fmt, TransFmt trans_fmt, PQMode pq_mode) {
           synchronized (mLock) {
             try {
                 return mProxy.factoryGetPQMode_Brightness(source_input.toInt(), sig_fmt.toInt(), trans_fmt.toInt(), pq_mode.toInt());
             } catch (RemoteException e) {
                 Log.e(TAG, "factoryGetPQMode_Brightness:" + e);
             }
         }
         return -1;
     }

     /**
      * @Function: FactorySetPQMode_Contrast
      * @Description: Adjust contrast value in corresponding pq mode for factory menu conctrol
      * @Param: source_type refer to enum SourceInput_Type, PQMode refer to enum Pq_Mode, contrast contrast value
      * @Return: contrast value
      */
     public int FactorySetPQMode_Contrast(SourceInput source_input, SignalFmt sig_fmt, TransFmt trans_fmt, PQMode pq_mode, int contrast) {
           synchronized (mLock) {
             try {
                 return mProxy.factorySetPQMode_Contrast(source_input.toInt(), sig_fmt.toInt(), trans_fmt.toInt(), pq_mode.toInt(), contrast);
             } catch (RemoteException e) {
                 Log.e(TAG, "factorySetPQMode_Contrast:" + e);
             }
         }
         return -1;
     }

     /**
      * @Function: FactoryGetPQMode_Contrast
      * @Description: Get contrast value in corresponding pq mode for factory menu conctrol
      * @Param: source_type refer to enum SourceInput_Type, PQMode refer to enum Pq_Mode
      * @Return: 0 success, -1 fail
      */
     public int FactoryGetPQMode_Contrast(SourceInput source_input, SignalFmt sig_fmt, TransFmt trans_fmt, PQMode pq_mode) {
           synchronized (mLock) {
             try {
                 return mProxy.factoryGetPQMode_Contrast(source_input.toInt(), sig_fmt.toInt(), trans_fmt.toInt(), pq_mode.toInt());
             } catch (RemoteException e) {
                 Log.e(TAG, "factoryGetPQMode_Contrast:" + e);
             }
         }
         return -1;
     }

     /**
      * @Function: FactorySetPQMode_Saturation
      * @Description: Adjust saturation value in corresponding pq mode for factory menu conctrol
      * @Param: source_type refer to enum SourceInput_Type, PQMode refer to enum Pq_Mode, saturation saturation value
      * @Return: 0 success, -1 fail
      */
     public int FactorySetPQMode_Saturation(SourceInput source_input, SignalFmt sig_fmt, TransFmt trans_fmt, PQMode pq_mode, int saturation) {
           synchronized (mLock) {
             try {
                 return mProxy.factorySetPQMode_Saturation(source_input.toInt(), sig_fmt.toInt(), trans_fmt.toInt(), pq_mode.toInt(), saturation);
             } catch (RemoteException e) {
                 Log.e(TAG, "factorySetPQMode_Saturation:" + e);
             }
         }
         return -1;
     }

     /**
      * @Function: FactoryGetPQMode_Saturation
      * @Description: Get saturation value in corresponding pq mode for factory menu conctrol
      * @Param: source_type refer to enum SourceInput_Type, PQMode refer to enum Pq_Mode
      * @Return: saturation value
      */
     public int FactoryGetPQMode_Saturation(SourceInput source_input, SignalFmt sig_fmt, TransFmt trans_fmt, PQMode pq_mode) {
           synchronized (mLock) {
             try {
                 return mProxy.factoryGetPQMode_Saturation(source_input.toInt(), sig_fmt.toInt(), trans_fmt.toInt(), pq_mode.toInt());
             } catch (RemoteException e) {
                 Log.e(TAG, "factoryGetPQMode_Saturation:" + e);
             }
         }
         return -1;
     }

     /**
      * @Function: FactorySetPQMode_Hue
      * @Description: Adjust hue value in corresponding pq mode for factory menu conctrol
      * @Param: source_type refer to enum SourceInput_Type, PQMode refer to enum Pq_Mode, hue hue value
      * @Return: 0 success, -1 fail
      */
     public int FactorySetPQMode_Hue(SourceInput source_input, SignalFmt sig_fmt, TransFmt trans_fmt, PQMode pq_mode, int hue) {
           synchronized (mLock) {
             try {
                 return mProxy.factorySetPQMode_Hue(source_input.toInt(), sig_fmt.toInt(), trans_fmt.toInt(), pq_mode.toInt(), hue);
             } catch (RemoteException e) {
                 Log.e(TAG, "factorySetPQMode_Hue:" + e);
             }
         }
         return -1;
     }

     /**
      * @Function: FactoryGetPQMode_Hue
      * @Description: Get hue value in corresponding pq mode for factory menu conctrol
      * @Param: source_type refer to enum SourceInput_Type, PQMode refer to enum Pq_Mode
      * @Return: hue value
      */
     public int FactoryGetPQMode_Hue(SourceInput source_input, SignalFmt sig_fmt, TransFmt trans_fmt, PQMode pq_mode) {
           synchronized (mLock) {
             try {
                 return mProxy.factoryGetPQMode_Hue(source_input.toInt(), sig_fmt.toInt(), trans_fmt.toInt(), pq_mode.toInt());
             } catch (RemoteException e) {
                 Log.e(TAG, "factoryGetPQMode_Hue:" + e);
             }
         }
         return -1;
     }

     /**
      * @Function: FactorySetPQMode_Sharpness
      * @Description: Adjust sharpness value in corresponding pq mode for factory menu conctrol
      * @Param: source_type refer to enum SourceInput_Type, PQMode refer to enum Pq_Mode, sharpness sharpness value
      * @Return: 0 success, -1 fail
      */
     public int FactorySetPQMode_Sharpness(SourceInput source_input, SignalFmt sig_fmt, TransFmt trans_fmt, PQMode pq_mode, int sharpness) {
           synchronized (mLock) {
             try {
                 return mProxy.factorySetPQMode_Sharpness(source_input.toInt(), sig_fmt.toInt(), trans_fmt.toInt(), pq_mode.toInt(), sharpness);
             } catch (RemoteException e) {
                 Log.e(TAG, "factorySetPQMode_Sharpness:" + e);
             }
         }
         return -1;
     }

     /**
      * @Function: FactoryGetPQMode_Sharpness
      * @Description: Get sharpness value in corresponding pq mode for factory menu conctrol
      * @Param: source_type refer to enum SourceInput_Type, PQMode refer to enum Pq_Mode
      * @Return: sharpness value
      */
     public int FactoryGetPQMode_Sharpness(SourceInput source_input, SignalFmt sig_fmt, TransFmt trans_fmt, PQMode pq_mode) {
           synchronized (mLock) {
             try {
                 return mProxy.factoryGetPQMode_Sharpness(source_input.toInt(), sig_fmt.toInt(), trans_fmt.toInt(), pq_mode.toInt());
             } catch (RemoteException e) {
                 Log.e(TAG, "factoryGetPQMode_Sharpness:" + e);
             }
         }
         return -1;
     }

     /**
      * @Function: FactoryResetPQMode
      * @Description: Reset all values of PQ mode for factory menu conctrol
      * @Param:
      * @Return: 0 success, -1 fail
      */
     public int FactoryResetPQMode() {
           synchronized (mLock) {
             try {
                 return mProxy.factoryResetPQMode();
             } catch (RemoteException e) {
                 Log.e(TAG, "FactoryResetPQMode:" + e);
             }
         }
         return -1;
     }

        /**
     * @Function: FactoryResetColorTemp
     * @Description: Reset all values of color temperature mode for factory menu conctrol
     * @Param:
     * @Return: 0 success, -1 fail
     */
    public int FactoryResetColorTemp() {
          synchronized (mLock) {
            try {
                return mProxy.factoryResetColorTemp();
            } catch (RemoteException e) {
                Log.e(TAG, "FactoryResetColorTemp:" + e);
            }
        }
        return -1;
    }

     /**
      * @Function: FactorySetParamsDefault
      * @Description: Reset all values of pq mode and color temperature mode for factory menu conctrol
      * @Param:
      * @Return: 0 success, -1 fail
      */
     public int FactorySetParamsDefault() {
           synchronized (mLock) {
             try {
                 return mProxy.factorySetParamsDefault();
             } catch (RemoteException e) {
                 Log.e(TAG, "FactorySetParamsDefault:" + e);
             }
         }
         return -1;
     }

   public class noline_params_t {
       public int osd0;
       public int osd25;
       public int osd50;
       public int osd75;
       public int osd100;
   }

   public enum NOLINE_PARAMS_TYPE {
       NOLINE_PARAMS_TYPE_BRIGHTNESS(0),
       NOLINE_PARAMS_TYPE_CONTRAST(1),
       NOLINE_PARAMS_TYPE_SATURATION(2),
       NOLINE_PARAMS_TYPE_HUE(3),
       NOLINE_PARAMS_TYPE_SHARPNESS(4),
       NOLINE_PARAMS_TYPE_VOLUME(5),
       NOLINE_PARAMS_TYPE_MAX(6);

       private int val;

       NOLINE_PARAMS_TYPE(int val) {
           this.val = val;
       }

       public int toInt() {
           return this.val;
       }
   }

     /**
      * @Function: FactorySetNolineParams
      * @Description: Nonlinearize the params of corresponding nolinear param type for factory menu conctrol
      * @Param: noline_params_type refer to enum NOLINE_PARAMS_TYPE, source_type refer to SourceInput_Type, params params value refer to class noline_params_t
      * @Return: 0 success, -1 fail
      */
     public int FactorySetNolineParams(NOLINE_PARAMS_TYPE noline_params_type, SourceInput source_input, SignalFmt sig_fmt, TransFmt trans_fmt, noline_params_t params) {
         synchronized (mLock) {
             try {
                 return mProxy.factorySetNolineParams(source_input.toInt(), sig_fmt.toInt(), trans_fmt.toInt(), noline_params_type.toInt(), params.osd0,
                                                      params.osd25, params.osd50, params.osd75, params.osd100);
             } catch (RemoteException e) {
                 Log.e(TAG, "FactorySetNolineParams:" + e);
             }
         }
         return -1;
     }

     /**
      * @Function: FactoryGetNolineParams
      * @Description: Nonlinearize the params of corresponding nolinear param type for factory menu conctrol
      * @Param: noline_params_type refer to enum NOLINE_PARAMS_TYPE, source_type refer to SourceInput_Type
      * @Return: params value refer to class noline_params_t
      */
     public noline_params_t FactoryGetNolineParams(NOLINE_PARAMS_TYPE noline_params_type, SourceInput source_input, SignalFmt sig_fmt, TransFmt trans_fmt) {
         noline_params_t noline_params = new noline_params_t();
         synchronized (mLock) {
             try {
                 NolineParam param = mProxy.factoryGetNolineParams(source_input.toInt(), sig_fmt.toInt(), trans_fmt.toInt(), noline_params_type.toInt());
                 noline_params.osd0 = param.osd0;
                 noline_params.osd25 = param.osd25;
                 noline_params.osd50 = param.osd50;
                 noline_params.osd75 = param.osd75;
                 noline_params.osd100 = param.osd100;
             } catch (RemoteException e) {
                 Log.e(TAG, "FactoryGetNolineParams:" + e);
             }
         }

         return noline_params;
     }

     public class tvin_cutwin_t {
         public int hs;
         public int he;
         public int vs;
         public int ve;
     }

      /* tvin signal format table */
     public enum TransFmt {
         TVIN_TFMT_2D(0),
         TVIN_TFMT_3D_LRH_OLOR(1),
         TVIN_TFMT_3D_LRH_OLER(2),
         TVIN_TFMT_3D_LRH_ELOR(3),
         TVIN_TFMT_3D_LRH_ELER(4),
         TVIN_TFMT_3D_TB(5),
         TVIN_TFMT_3D_FP(6),
         TVIN_TFMT_3D_FA(7),
         TVIN_TFMT_3D_LA(8),
         TVIN_TFMT_3D_LRF(9),
         TVIN_TFMT_3D_LD(10),
         TVIN_TFMT_3D_LDGD(11),
         TVIN_TFMT_3D_DET_TB(12),
         TVIN_TFMT_3D_DET_LR(13),
         TVIN_TFMT_3D_DET_INTERLACE(14),
         TVIN_TFMT_3D_DET_CHESSBOARD(15),
         TVIN_TFMT_3D_MAX(16);

         private int val;

         TransFmt(int val) {
           this.val = val;
         }

         public static TransFmt valueOf(int value) {
             for (TransFmt tft : TransFmt.values()) {
                 if (tft.toInt() == value) {
                     return tft;
                 }
             }
           return TVIN_TFMT_3D_MAX;
         }

         public int toInt() {
           return this.val;
         }
     }

     /* tvin signal format table */
     public enum SignalFmt {
         TVIN_SIG_FMT_NULL(0),
         //VGA Formats
         TVIN_SIG_FMT_VGA_512X384P_60HZ_D147    (0x001),
         TVIN_SIG_FMT_VGA_560X384P_60HZ_D147    (0x002),
         TVIN_SIG_FMT_VGA_640X200P_59HZ_D924    (0x003),
         TVIN_SIG_FMT_VGA_640X350P_85HZ_D080    (0x004),
         TVIN_SIG_FMT_VGA_640X400P_59HZ_D940    (0x005),
         TVIN_SIG_FMT_VGA_640X400P_85HZ_D080    (0x006),
         TVIN_SIG_FMT_VGA_640X400P_59HZ_D638    (0x007),
         TVIN_SIG_FMT_VGA_640X400P_56HZ_D416    (0x008),
         TVIN_SIG_FMT_VGA_640X480P_66HZ_D619    (0x009),
         TVIN_SIG_FMT_VGA_640X480P_66HZ_D667    (0x00a),
         TVIN_SIG_FMT_VGA_640X480P_59HZ_D940    (0x00b),
         TVIN_SIG_FMT_VGA_640X480P_60HZ_D000    (0x00c),
         TVIN_SIG_FMT_VGA_640X480P_72HZ_D809    (0x00d),
         TVIN_SIG_FMT_VGA_640X480P_75HZ_D000_A  (0x00e),
         TVIN_SIG_FMT_VGA_640X480P_85HZ_D008    (0x00f),
         TVIN_SIG_FMT_VGA_640X480P_59HZ_D638    (0x010),
         TVIN_SIG_FMT_VGA_640X480P_75HZ_D000_B  (0x011),
         TVIN_SIG_FMT_VGA_640X870P_75HZ_D000    (0x012),
         TVIN_SIG_FMT_VGA_720X350P_70HZ_D086    (0x013),
         TVIN_SIG_FMT_VGA_720X400P_85HZ_D039    (0x014),
         TVIN_SIG_FMT_VGA_720X400P_70HZ_D086    (0x015),
         TVIN_SIG_FMT_VGA_720X400P_87HZ_D849    (0x016),
         TVIN_SIG_FMT_VGA_720X400P_59HZ_D940    (0x017),
         TVIN_SIG_FMT_VGA_720X480P_59HZ_D940    (0x018),
         TVIN_SIG_FMT_VGA_768X480P_59HZ_D896    (0x019),
         TVIN_SIG_FMT_VGA_800X600P_56HZ_D250    (0x01a),
         TVIN_SIG_FMT_VGA_800X600P_60HZ_D000    (0x01b),
         TVIN_SIG_FMT_VGA_800X600P_60HZ_D000_A  (0x01c),
         TVIN_SIG_FMT_VGA_800X600P_60HZ_D317    (0x01d),
         TVIN_SIG_FMT_VGA_800X600P_72HZ_D188    (0x01e),
         TVIN_SIG_FMT_VGA_800X600P_75HZ_D000    (0x01f),
         TVIN_SIG_FMT_VGA_800X600P_85HZ_D061    (0x020),
         TVIN_SIG_FMT_VGA_832X624P_75HZ_D087    (0x021),
         TVIN_SIG_FMT_VGA_848X480P_84HZ_D751    (0x022),
         TVIN_SIG_FMT_VGA_960X600P_59HZ_D635    (0x023),
         TVIN_SIG_FMT_VGA_1024X768P_59HZ_D278   (0x024),
         TVIN_SIG_FMT_VGA_1024X768P_60HZ_D000   (0x025),
         TVIN_SIG_FMT_VGA_1024X768P_60HZ_D000_A (0x026),
         TVIN_SIG_FMT_VGA_1024X768P_60HZ_D000_B (0x027),
         TVIN_SIG_FMT_VGA_1024X768P_74HZ_D927   (0x028),
         TVIN_SIG_FMT_VGA_1024X768P_60HZ_D004   (0x029),
         TVIN_SIG_FMT_VGA_1024X768P_70HZ_D069   (0x02a),
         TVIN_SIG_FMT_VGA_1024X768P_75HZ_D029   (0x02b),
         TVIN_SIG_FMT_VGA_1024X768P_84HZ_D997   (0x02c),
         TVIN_SIG_FMT_VGA_1024X768P_74HZ_D925   (0x02d),
         TVIN_SIG_FMT_VGA_1024X768P_60HZ_D020   (0x02e),
         TVIN_SIG_FMT_VGA_1024X768P_70HZ_D008   (0x02f),
         TVIN_SIG_FMT_VGA_1024X768P_75HZ_D782   (0x030),
         TVIN_SIG_FMT_VGA_1024X768P_77HZ_D069   (0x031),
         TVIN_SIG_FMT_VGA_1024X768P_71HZ_D799   (0x032),
         TVIN_SIG_FMT_VGA_1024X1024P_60HZ_D000  (0x033),
         TVIN_SIG_FMT_VGA_1152X864P_60HZ_D000   (0x034),
         TVIN_SIG_FMT_VGA_1152X864P_70HZ_D012   (0x035),
         TVIN_SIG_FMT_VGA_1152X864P_75HZ_D000   (0x036),
         TVIN_SIG_FMT_VGA_1152X864P_84HZ_D999   (0x037),
         TVIN_SIG_FMT_VGA_1152X870P_75HZ_D062   (0x038),
         TVIN_SIG_FMT_VGA_1152X900P_65HZ_D950   (0x039),
         TVIN_SIG_FMT_VGA_1152X900P_66HZ_D004   (0x03a),
         TVIN_SIG_FMT_VGA_1152X900P_76HZ_D047   (0x03b),
         TVIN_SIG_FMT_VGA_1152X900P_76HZ_D149   (0x03c),
         TVIN_SIG_FMT_VGA_1280X720P_59HZ_D855   (0x03d),
         TVIN_SIG_FMT_VGA_1280X720P_60HZ_D000_A (0x03e),
         TVIN_SIG_FMT_VGA_1280X720P_60HZ_D000_B (0x03f),
         TVIN_SIG_FMT_VGA_1280X720P_60HZ_D000_C (0x040),
         TVIN_SIG_FMT_VGA_1280X720P_60HZ_D000_D (0x041),
         TVIN_SIG_FMT_VGA_1280X768P_59HZ_D870   (0x042),
         TVIN_SIG_FMT_VGA_1280X768P_59HZ_D995   (0x043),
         TVIN_SIG_FMT_VGA_1280X768P_60HZ_D100   (0x044),
         TVIN_SIG_FMT_VGA_1280X768P_85HZ_D000   (0x045),
         TVIN_SIG_FMT_VGA_1280X768P_74HZ_D893   (0x046),
         TVIN_SIG_FMT_VGA_1280X768P_84HZ_D837   (0x047),
         TVIN_SIG_FMT_VGA_1280X800P_59HZ_D810   (0x048),
         TVIN_SIG_FMT_VGA_1280X800P_59HZ_D810_A (0x049),
         TVIN_SIG_FMT_VGA_1280X800P_60HZ_D000   (0x04a),
         TVIN_SIG_FMT_VGA_1280X800P_85HZ_D000   (0x04b),
         TVIN_SIG_FMT_VGA_1280X960P_60HZ_D000   (0x04c),
         TVIN_SIG_FMT_VGA_1280X960P_60HZ_D000_A (0x04d),
         TVIN_SIG_FMT_VGA_1280X960P_75HZ_D000   (0x04e),
         TVIN_SIG_FMT_VGA_1280X960P_85HZ_D002   (0x04f),
         TVIN_SIG_FMT_VGA_1280X1024P_60HZ_D020  (0x050),
         TVIN_SIG_FMT_VGA_1280X1024P_60HZ_D020_A(0x051),
         TVIN_SIG_FMT_VGA_1280X1024P_75HZ_D025  (0x052),
         TVIN_SIG_FMT_VGA_1280X1024P_85HZ_D024  (0x053),
         TVIN_SIG_FMT_VGA_1280X1024P_59HZ_D979  (0x054),
         TVIN_SIG_FMT_VGA_1280X1024P_72HZ_D005  (0x055),
         TVIN_SIG_FMT_VGA_1280X1024P_60HZ_D002  (0x056),
         TVIN_SIG_FMT_VGA_1280X1024P_67HZ_D003  (0x057),
         TVIN_SIG_FMT_VGA_1280X1024P_74HZ_D112  (0x058),
         TVIN_SIG_FMT_VGA_1280X1024P_76HZ_D179  (0x059),
         TVIN_SIG_FMT_VGA_1280X1024P_66HZ_D718  (0x05a),
         TVIN_SIG_FMT_VGA_1280X1024P_66HZ_D677  (0x05b),
         TVIN_SIG_FMT_VGA_1280X1024P_76HZ_D107  (0x05c),
         TVIN_SIG_FMT_VGA_1280X1024P_59HZ_D996  (0x05d),
         TVIN_SIG_FMT_VGA_1280X1024P_60HZ_D000  (0x05e),
         TVIN_SIG_FMT_VGA_1360X768P_59HZ_D799   (0x05f),
         TVIN_SIG_FMT_VGA_1360X768P_60HZ_D015   (0x060),
         TVIN_SIG_FMT_VGA_1360X768P_60HZ_D015_A (0x061),
         TVIN_SIG_FMT_VGA_1360X850P_60HZ_D000   (0x062),
         TVIN_SIG_FMT_VGA_1360X1024P_60HZ_D000  (0x063),
         TVIN_SIG_FMT_VGA_1366X768P_59HZ_D790   (0x064),
         TVIN_SIG_FMT_VGA_1366X768P_60HZ_D000   (0x065),
         TVIN_SIG_FMT_VGA_1400X1050P_59HZ_D978  (0x066),
         TVIN_SIG_FMT_VGA_1440X900P_59HZ_D887   (0x067),
         TVIN_SIG_FMT_VGA_1440X1080P_60HZ_D000  (0x068),
         TVIN_SIG_FMT_VGA_1600X900P_60HZ_D000   (0x069),
         TVIN_SIG_FMT_VGA_1600X1024P_60HZ_D000  (0x06a),
         TVIN_SIG_FMT_VGA_1600X1200P_59HZ_D869  (0x06b),
         TVIN_SIG_FMT_VGA_1600X1200P_60HZ_D000  (0x06c),
         TVIN_SIG_FMT_VGA_1600X1200P_65HZ_D000  (0x06d),
         TVIN_SIG_FMT_VGA_1600X1200P_70HZ_D000  (0x06e),
         TVIN_SIG_FMT_VGA_1680X1050P_59HZ_D954  (0x06f),
         TVIN_SIG_FMT_VGA_1680X1080P_60HZ_D000  (0x070),
         TVIN_SIG_FMT_VGA_1920X1080P_49HZ_D929  (0x071),
         TVIN_SIG_FMT_VGA_1920X1080P_59HZ_D963_A(0x072),
         TVIN_SIG_FMT_VGA_1920X1080P_59HZ_D963  (0x073),
         TVIN_SIG_FMT_VGA_1920X1080P_60HZ_D000  (0x074),
         TVIN_SIG_FMT_VGA_1920X1200P_59HZ_D950  (0x075),
         TVIN_SIG_FMT_VGA_1024X768P_60HZ_D000_C (0x076),
         TVIN_SIG_FMT_VGA_1024X768P_60HZ_D000_D (0x077),
         TVIN_SIG_FMT_VGA_1920X1200P_59HZ_D988  (0x078),
         TVIN_SIG_FMT_VGA_1400X900P_60HZ_D000   (0x079),
         TVIN_SIG_FMT_VGA_1680X1050P_60HZ_D000  (0x07a),
         TVIN_SIG_FMT_VGA_800X600P_60HZ_D062    (0x07b),
         TVIN_SIG_FMT_VGA_800X600P_60HZ_317_B   (0x07c),
         TVIN_SIG_FMT_VGA_RESERVE8              (0x07d),
         TVIN_SIG_FMT_VGA_RESERVE9              (0x07e),
         TVIN_SIG_FMT_VGA_RESERVE10             (0x07f),
         TVIN_SIG_FMT_VGA_RESERVE11             (0x080),
         TVIN_SIG_FMT_VGA_RESERVE12             (0x081),
         TVIN_SIG_FMT_VGA_MAX                   (0x082),
         TVIN_SIG_FMT_VGA_THRESHOLD             (0x200),
         //Component Formats
         TVIN_SIG_FMT_COMP_480P_60HZ_D000       (0x201),
         TVIN_SIG_FMT_COMP_480I_59HZ_D940       (0x202),
         TVIN_SIG_FMT_COMP_576P_50HZ_D000       (0x203),
         TVIN_SIG_FMT_COMP_576I_50HZ_D000       (0x204),
         TVIN_SIG_FMT_COMP_720P_59HZ_D940       (0x205),
         TVIN_SIG_FMT_COMP_720P_50HZ_D000       (0x206),
         TVIN_SIG_FMT_COMP_1080P_23HZ_D976      (0x207),
         TVIN_SIG_FMT_COMP_1080P_24HZ_D000      (0x208),
         TVIN_SIG_FMT_COMP_1080P_25HZ_D000      (0x209),
         TVIN_SIG_FMT_COMP_1080P_30HZ_D000      (0x20a),
         TVIN_SIG_FMT_COMP_1080P_50HZ_D000      (0x20b),
         TVIN_SIG_FMT_COMP_1080P_60HZ_D000      (0x20c),
         TVIN_SIG_FMT_COMP_1080I_47HZ_D952      (0x20d),
         TVIN_SIG_FMT_COMP_1080I_48HZ_D000      (0x20e),
         TVIN_SIG_FMT_COMP_1080I_50HZ_D000_A    (0x20f),
         TVIN_SIG_FMT_COMP_1080I_50HZ_D000_B    (0x210),
         TVIN_SIG_FMT_COMP_1080I_50HZ_D000_C    (0x211),
         TVIN_SIG_FMT_COMP_1080I_60HZ_D000      (0x212),
         TVIN_SIG_FMT_COMP_MAX                  (0x213),
         TVIN_SIG_FMT_COMP_THRESHOLD            (0x400),
         //HDMI Formats
         TVIN_SIG_FMT_HDMI_640X480P_60HZ        (0x401),
         TVIN_SIG_FMT_HDMI_720X480P_60HZ        (0x402),
         TVIN_SIG_FMT_HDMI_1280X720P_60HZ       (0x403),
         TVIN_SIG_FMT_HDMI_1920X1080I_60HZ      (0x404),
         TVIN_SIG_FMT_HDMI_1440X480I_60HZ       (0x405),
         TVIN_SIG_FMT_HDMI_1440X240P_60HZ       (0x406),
         TVIN_SIG_FMT_HDMI_2880X480I_60HZ       (0x407),
         TVIN_SIG_FMT_HDMI_2880X240P_60HZ       (0x408),
         TVIN_SIG_FMT_HDMI_1440X480P_60HZ       (0x409),
         TVIN_SIG_FMT_HDMI_1920X1080P_60HZ      (0x40a),
         TVIN_SIG_FMT_HDMI_720X576P_50HZ        (0x40b),
         TVIN_SIG_FMT_HDMI_1280X720P_50HZ       (0x40c),
         TVIN_SIG_FMT_HDMI_1920X1080I_50HZ_A    (0x40d),
         TVIN_SIG_FMT_HDMI_1440X576I_50HZ       (0x40e),
         TVIN_SIG_FMT_HDMI_1440X288P_50HZ       (0x40f),
         TVIN_SIG_FMT_HDMI_2880X576I_50HZ       (0x410),
         TVIN_SIG_FMT_HDMI_2880X288P_50HZ       (0x411),
         TVIN_SIG_FMT_HDMI_1440X576P_50HZ       (0x412),
         TVIN_SIG_FMT_HDMI_1920X1080P_50HZ      (0x413),
         TVIN_SIG_FMT_HDMI_1920X1080P_24HZ      (0x414),
         TVIN_SIG_FMT_HDMI_1920X1080P_25HZ      (0x415),
         TVIN_SIG_FMT_HDMI_1920X1080P_30HZ      (0x416),
         TVIN_SIG_FMT_HDMI_2880X480P_60HZ       (0x417),
         TVIN_SIG_FMT_HDMI_2880X576P_60HZ       (0x418),
         TVIN_SIG_FMT_HDMI_1920X1080I_50HZ_B    (0x419),
         TVIN_SIG_FMT_HDMI_1920X1080I_100HZ     (0x41a),
         TVIN_SIG_FMT_HDMI_1280X720P_100HZ      (0x41b),
         TVIN_SIG_FMT_HDMI_720X576P_100HZ       (0x41c),
         TVIN_SIG_FMT_HDMI_1440X576I_100HZ      (0x41d),
         TVIN_SIG_FMT_HDMI_1920X1080I_120HZ     (0x41e),
         TVIN_SIG_FMT_HDMI_1280X720P_120HZ      (0x41f),
         TVIN_SIG_FMT_HDMI_720X480P_120HZ       (0x420),
         TVIN_SIG_FMT_HDMI_1440X480I_120HZ      (0x421),
         TVIN_SIG_FMT_HDMI_720X576P_200HZ       (0x422),
         TVIN_SIG_FMT_HDMI_1440X576I_200HZ      (0x423),
         TVIN_SIG_FMT_HDMI_720X480P_240HZ       (0x424),
         TVIN_SIG_FMT_HDMI_1440X480I_240HZ      (0x425),
         TVIN_SIG_FMT_HDMI_1280X720P_24HZ       (0x426),
         TVIN_SIG_FMT_HDMI_1280X720P_25HZ       (0x427),
         TVIN_SIG_FMT_HDMI_1280X720P_30HZ       (0x428),
         TVIN_SIG_FMT_HDMI_1920X1080P_120HZ     (0x429),
         TVIN_SIG_FMT_HDMI_1920X1080P_100HZ     (0x42a),
         TVIN_SIG_FMT_HDMI_1280X720P_60HZ_FRAME_PACKING  (0x42b),
         TVIN_SIG_FMT_HDMI_1280X720P_50HZ_FRAME_PACKING  (0x42c),
         TVIN_SIG_FMT_HDMI_1280X720P_24HZ_FRAME_PACKING  (0x42d),
         TVIN_SIG_FMT_HDMI_1280X720P_30HZ_FRAME_PACKING  (0x42e),
         TVIN_SIG_FMT_HDMI_1920X1080I_60HZ_FRAME_PACKING (0x42f),
         TVIN_SIG_FMT_HDMI_1920X1080I_50HZ_FRAME_PACKING (0x430),
         TVIN_SIG_FMT_HDMI_1920X1080P_24HZ_FRAME_PACKING (0x431),
         TVIN_SIG_FMT_HDMI_1920X1080P_30HZ_FRAME_PACKING (0x432),
         TVIN_SIG_FMT_HDMI_800X600_00HZ                  (0x433),
         TVIN_SIG_FMT_HDMI_1024X768_00HZ                 (0x434),
         TVIN_SIG_FMT_HDMI_720X400_00HZ                  (0x435),
         TVIN_SIG_FMT_HDMI_1280X768_00HZ                 (0x436),
         TVIN_SIG_FMT_HDMI_1280X800_00HZ                 (0x437),
         TVIN_SIG_FMT_HDMI_1280X960_00HZ                 (0x438),
         TVIN_SIG_FMT_HDMI_1280X1024_00HZ                (0x439),
         TVIN_SIG_FMT_HDMI_1360X768_00HZ                 (0x43a),
         TVIN_SIG_FMT_HDMI_1366X768_00HZ                 (0x43b),
         TVIN_SIG_FMT_HDMI_1600X1200_00HZ                (0x43c),
         TVIN_SIG_FMT_HDMI_1920X1200_00HZ                (0x43d),
         TVIN_SIG_FMT_HDMI_1440X900_00HZ                 (0x43e),
         TVIN_SIG_FMT_HDMI_1400X1050_00HZ                (0x43f),
         TVIN_SIG_FMT_HDMI_1680X1050_00HZ                (0x440),
         TVIN_SIG_FMT_HDMI_1920X1080I_60HZ_ALTERNATIVE   (0x441),
         TVIN_SIG_FMT_HDMI_1920X1080I_50HZ_ALTERNATIVE   (0x442),
         TVIN_SIG_FMT_HDMI_1920X1080P_24HZ_ALTERNATIVE   (0x443),
         TVIN_SIG_FMT_HDMI_1920X1080P_30HZ_ALTERNATIVE   (0x444),
         TVIN_SIG_FMT_HDMI_3840X2160_00HZ                (0x445),
         TVIN_SIG_FMT_HDMI_4096X2160_00HZ                (0x446),
         TVIN_SIG_FMT_HDMI_RESERVE7                      (0x447),
         TVIN_SIG_FMT_HDMI_RESERVE8                      (0x448),
         TVIN_SIG_FMT_HDMI_RESERVE9                      (0x449),
         TVIN_SIG_FMT_HDMI_RESERVE10                     (0x44a),
         TVIN_SIG_FMT_HDMI_RESERVE11                     (0x44b),
         TVIN_SIG_FMT_HDMI_720X480P_60HZ_FRAME_PACKING   (0x44c),
         TVIN_SIG_FMT_HDMI_720X576P_50HZ_FRAME_PACKING   (0x44d),
         TVIN_SIG_FMT_HDMI_MAX                           (0x44e),
         TVIN_SIG_FMT_HDMI_THRESHOLD                     (0x600),
         //Video Formats
         TVIN_SIG_FMT_CVBS_NTSC_M                        (0x601),
         TVIN_SIG_FMT_CVBS_NTSC_443                      (0x602),
         TVIN_SIG_FMT_CVBS_PAL_I                         (0x603),
         TVIN_SIG_FMT_CVBS_PAL_M                         (0x604),
         TVIN_SIG_FMT_CVBS_PAL_60                        (0x605),
         TVIN_SIG_FMT_CVBS_PAL_CN                        (0x606),
         TVIN_SIG_FMT_CVBS_SECAM                         (0x607),
         TVIN_SIG_FMT_CVBS_MAX                           (0x608),
         TVIN_SIG_FMT_CVBS_THRESHOLD                     (0x800),
         //656 Formats
         TVIN_SIG_FMT_BT656IN_576I_50HZ                  (0x801),
         TVIN_SIG_FMT_BT656IN_480I_60HZ                  (0x802),
         //601 Formats
         TVIN_SIG_FMT_BT601IN_576I_50HZ                  (0x803),
         TVIN_SIG_FMT_BT601IN_480I_60HZ                  (0x804),
         //Camera Formats
         TVIN_SIG_FMT_CAMERA_640X480P_30HZ               (0x805),
         TVIN_SIG_FMT_CAMERA_800X600P_30HZ               (0x806),
         TVIN_SIG_FMT_CAMERA_1024X768P_30HZ              (0x807),
         TVIN_SIG_FMT_CAMERA_1920X1080P_30HZ             (0x808),
         TVIN_SIG_FMT_CAMERA_1280X720P_30HZ              (0x809),
         TVIN_SIG_FMT_BT601_MAX                          (0x80a),
         TVIN_SIG_FMT_BT601_THRESHOLD                    (0xa00),
         TVIN_SIG_FMT_MAX(0xFFFFFFFF);

         private int val;
         SignalFmt(int val) {
             this.val = val;
         }

         public static SignalFmt valueOf(int value) {
            for (SignalFmt fmt : SignalFmt.values()) {
               if (fmt.toInt() == value) {
                  return fmt;
               }
            }
            return TVIN_SIG_FMT_MAX;
         }

         public int toInt() {
              return this.val;
         }
     }

     /**
      * @Function: FactorySetOverscanParams
      * @Description: Set overscan params of corresponding source type and fmt for factory menu conctrol
      * @Param: source_type refer to enum SourceInput_Type, fmt refer to enum tvin_sig_fmt_e
      * @Param: trans_fmt refer to enum tvin_trans_fmt, cutwin_t refer to class tvin_cutwin_t
      * @Return: 0 success, -1 fail
      */
     public int FactorySetOverscanParams(SourceInput source_input, SignalFmt fmt,
                                                  TransFmt trans_fmt, tvin_cutwin_t cutwin_t) {
          synchronized (mLock) {
              try {
                  return mProxy.factorySetOverscan(source_input.toInt(), fmt.toInt(), trans_fmt.toInt(),
                                                   cutwin_t.he, cutwin_t.hs, cutwin_t.ve, cutwin_t.vs);
              } catch (RemoteException e) {
                  Log.e(TAG, "FactorySetOverscanParams:" + e);
              }
          }
          return -1;
     }

     /**
      * @Function: FactoryGetOverscanParams
      * @Description: Get overscan params of corresponding source type and fmt for factory menu conctrol
      * @Param: source_type refer to enum SourceInput_Type, fmt refer to enum tvin_sig_fmt_e
      * @Param: trans_fmt refer to enum tvin_trans_fmt
      * @Return: cutwin_t value for overscan refer to class tvin_cutwin_t
      */
     public tvin_cutwin_t FactoryGetOverscanParams(SourceInput source_input, SignalFmt fmt, TransFmt trans_fmt) {
         tvin_cutwin_t cutwin_t = new tvin_cutwin_t();
         synchronized (mLock) {
             try {
                 OverScanParam param = mProxy.factoryGetOverscan(source_input.toInt(), fmt.toInt(), trans_fmt.toInt());
                 cutwin_t.hs = param.he;
                 cutwin_t.he = param.hs;
                 cutwin_t.vs = param.ve;
                 cutwin_t.ve = param.vs;
             } catch (RemoteException e) {
                 Log.e(TAG, "FactoryGetOverscanParams:" + e);
             }
         }

         return cutwin_t;
     }

     /**
      * @Function: Read the red gain with specified souce and color temperature
      * @Param:
      * @ Return value: the red gain value
      * */
     public int FactoryWhiteBalanceSetRedGain(SourceInput source_input, SignalFmt fmt, TransFmt trans_fmt, color_temperature colorTemp_mode, int value) {
         synchronized (mLock) {
             try {
                 return mProxy.setwhiteBalanceGainRed(source_input.toInt(), fmt.toInt(), trans_fmt.toInt(), colorTemp_mode.toInt(), value);
             } catch (RemoteException e) {
                 Log.e(TAG, "FactoryWhiteBalanceSetRedGain:" + e);
             }
         }
         return -1;
     }

     public int FactoryWhiteBalanceSetGreenGain(SourceInput source_input, SignalFmt fmt, TransFmt trans_fmt, color_temperature colorTemp_mode, int value) {
           synchronized (mLock) {
             try {
                 return mProxy.setwhiteBalanceGainGreen(source_input.toInt(), fmt.toInt(), trans_fmt.toInt(), colorTemp_mode.toInt(), value);
             } catch (RemoteException e) {
                 Log.e(TAG, "FactoryWhiteBalanceSetGreenGain:" + e);
             }
         }
         return -1;
     }

     public int FactoryWhiteBalanceSetBlueGain(SourceInput source_input, SignalFmt fmt, TransFmt trans_fmt, color_temperature colorTemp_mode, int value) {
           synchronized (mLock) {
             try {
                 return mProxy.setwhiteBalanceGainBlue(source_input.toInt(), fmt.toInt(), trans_fmt.toInt(), colorTemp_mode.toInt(), value);
             } catch (RemoteException e) {
                 Log.e(TAG, "FactoryWhiteBalanceSetBlueGain:" + e);
             }
         }
         return -1;
     }

     public int FactoryWhiteBalanceGetRedGain(SourceInput source_input, SignalFmt fmt, TransFmt trans_fmt, color_temperature colorTemp_mode) {
           synchronized (mLock) {
             try {
                 return mProxy.getwhiteBalanceGainRed(source_input.toInt(), fmt.toInt(), trans_fmt.toInt(), colorTemp_mode.toInt());
             } catch (RemoteException e) {
                 Log.e(TAG, "FactoryWhiteBalanceGetRedGain:" + e);
             }
         }
         return -1;
     }

     public int FactoryWhiteBalanceGetGreenGain(SourceInput source_input, SignalFmt fmt, TransFmt trans_fmt, color_temperature colorTemp_mode) {
           synchronized (mLock) {
             try {
                 return mProxy.getwhiteBalanceGainGreen(source_input.toInt(), fmt.toInt(), trans_fmt.toInt(), colorTemp_mode.toInt());
             } catch (RemoteException e) {
                 Log.e(TAG, "FactoryWhiteBalanceGetGreenGain:" + e);
             }
         }
         return -1;
     }

     public int FactoryWhiteBalanceGetBlueGain(SourceInput source_input, SignalFmt fmt, TransFmt trans_fmt, color_temperature colorTemp_mode) {
           synchronized (mLock) {
             try {
                 return mProxy.getwhiteBalanceGainBlue(source_input.toInt(), fmt.toInt(), trans_fmt.toInt(), colorTemp_mode.toInt());
             } catch (RemoteException e) {
                 Log.e(TAG, "FactoryWhiteBalanceGetGreenGain:" + e);
             }
         }
         return -1;
     }

     public int FactoryWhiteBalanceSetRedOffset(SourceInput source_input, SignalFmt fmt, TransFmt trans_fmt, color_temperature colorTemp_mode, int value) {
         synchronized (mLock) {
             try {
                 return mProxy.setwhiteBalanceOffsetRed(source_input.toInt(), fmt.toInt(), trans_fmt.toInt(), colorTemp_mode.toInt(), value);
             } catch (RemoteException e) {
                   Log.e(TAG, "FactoryWhiteBalanceSetRedOffset:" + e);
             }
         }
         return -1;
     }

     public int FactoryWhiteBalanceSetGreenOffset(SourceInput source_input, SignalFmt fmt, TransFmt trans_fmt, color_temperature colorTemp_mode, int value) {
         synchronized (mLock) {
             try {
                 return mProxy.setwhiteBalanceOffsetGreen(source_input.toInt(), fmt.toInt(), trans_fmt.toInt(), colorTemp_mode.toInt(), value);
             } catch (RemoteException e) {
                   Log.e(TAG, "FactoryWhiteBalanceSetGreenOffset:" + e);
             }
         }
         return -1;
     }

     public int FactoryWhiteBalanceSetBlueOffset(SourceInput source_input, SignalFmt fmt, TransFmt trans_fmt, color_temperature colorTemp_mode, int value) {
         synchronized (mLock) {
             try {
                 return mProxy.setwhiteBalanceOffsetBlue(source_input.toInt(), fmt.toInt(), trans_fmt.toInt(), colorTemp_mode.toInt(), value);
             } catch (RemoteException e) {
                   Log.e(TAG, "FactoryWhiteBalanceSetBlueOffset:" + e);
             }
         }
         return -1;
     }

     public int FactoryWhiteBalanceGetRedOffset(SourceInput source_input, SignalFmt fmt, TransFmt trans_fmt, color_temperature colorTemp_mode) {
         synchronized (mLock) {
             try {
                 return mProxy.getwhiteBalanceOffsetRed(source_input.toInt(), fmt.toInt(), trans_fmt.toInt(), colorTemp_mode.toInt());
             } catch (RemoteException e) {
                   Log.e(TAG, "FactoryWhiteBalanceGetRedOffset:" + e);
             }
         }
         return -1;
     }

     public int FactoryWhiteBalanceGetGreenOffset(SourceInput source_input, SignalFmt fmt, TransFmt trans_fmt, color_temperature colorTemp_mode) {
         synchronized (mLock) {
             try {
                 return mProxy.getwhiteBalanceOffsetGreen(source_input.toInt(), fmt.toInt(), trans_fmt.toInt(), colorTemp_mode.toInt());
             } catch (RemoteException e) {
                   Log.e(TAG, "FactoryWhiteBalanceGetGreenOffset:" + e);
             }
         }
         return -1;
     }

     public int FactoryWhiteBalanceGetBlueOffset(SourceInput source_input, SignalFmt fmt, TransFmt trans_fmt, color_temperature colorTemp_mode) {
         synchronized (mLock) {
             try {
                 return mProxy.getwhiteBalanceOffsetBlue(source_input.toInt(), fmt.toInt(), trans_fmt.toInt(), colorTemp_mode.toInt());
             } catch (RemoteException e) {
                   Log.e(TAG, "FactoryWhiteBalanceGetBlueOffset:" + e);
             }
         }
         return -1;
     }

     public int FactoryWhiteBalanceSetColorTemperature(SourceInput source_input, SignalFmt fmt, TransFmt trans_fmt, color_temperature colorTemp_mode, int is_save) {
         synchronized (mLock) {
             try {
                 return mProxy.setColorTemperature(colorTemp_mode.toInt(), is_save);
             } catch (RemoteException e) {
                 Log.e(TAG, "FactoryWhiteBalanceSetColorTemperature:" + e);
             }
         }
         return -1;
     }

     public int FactoryWhiteBalanceGetColorTemperature(SourceInput source_input, SignalFmt fmt, TransFmt trans_fmt) {
         synchronized (mLock) {
             try {
                 return mProxy.getColorTemperature();
             } catch (RemoteException e) {
                 Log.e(TAG, "FactoryWhiteBalanceGetColorTemperature:" + e);
             }
         }
         return -1;
     }

     /**
      * @Function: Save the white balance data to fbc or g9
      * @Param:
      * @Return value: save OK: 0 , else -1
      *
      * */
     public int FactoryWhiteBalanceSaveParameters(SourceInput source_input, SignalFmt fmt, TransFmt trans_fmt, color_temperature colorTemp_mode, int r_gain, int g_gain, int b_gain, int r_offset, int g_offset, int b_offset) {
         synchronized (mLock) {
             try {
                 return mProxy.saveWhiteBalancePara(source_input.toInt(), fmt.toInt(), trans_fmt.toInt(), colorTemp_mode.toInt(), r_gain, g_gain, b_gain,r_offset, g_offset, b_offset);
             } catch (RemoteException e) {
                   Log.e(TAG, "FactoryWhiteBalanceSaveParameters:" + e);
             }
         }
         return -1;
     }

     public class WhiteBalanceParams {
         public int r_gain;        // u1.10, range 0~2047, default is 1024 (1.0x)
         public int g_gain;        // u1.10, range 0~2047, default is 1024 (1.0x)
         public int b_gain;        // u1.10, range 0~2047, default is 1024 (1.0x)
         public int r_offset;      // s11.0, range -1024~+1023, default is 0
         public int g_offset;      // s11.0, range -1024~+1023, default is 0
         public int b_offset;      // s11.0, range -1024~+1023, default is 0
     }

     public WhiteBalanceParams FactoryWhiteBalanceGetAllParams(int colorTemp_mode) {
         WhiteBalanceParams params = new WhiteBalanceParams();
         synchronized (mLock) {
             try {
                 int ret = mProxy.factoryfactoryGetColorTemperatureParams(colorTemp_mode);
                 if (ret == 0) {
                     params.r_gain = 0;
                     params.g_gain = 0;
                     params.b_gain = 0;
                     params.r_offset = 0;
                     params.g_offset = 0;
                     params.b_offset = 0;
                 }
             } catch (RemoteException e) {
                 Log.e(TAG, "FactoryWhiteBalanceGetAllParams:" + e);
             }
         }

         return params;
     }

   public int FactorySSMRestore() {
         synchronized (mLock) {
           try {
               return mProxy.factorySSMRestore();
           } catch (RemoteException e) {
               Log.e(TAG, "FactorySSMRestore:" + e);
           }
       }
       return -1;

   }

   public int FactoryResetNonlinear() {
         synchronized (mLock) {
           try {
               return mProxy.factoryResetNonlinear();
           } catch (RemoteException e) {
               Log.e(TAG, "FactoryResetNonlinear:" + e);
           }
       }
       return -1;
    }

   public int FactorySetGamma(int gamma_r, int gamma_g, int gamma_b) {
         synchronized (mLock) {
           try {
               return mProxy.factorySetGamma(gamma_r, gamma_g, gamma_b);
           } catch (RemoteException e) {
               Log.e(TAG, "FactorySetGamma:" + e);
           }
       }
       return -1;
    }

    public int SysSSMReadNTypes(int id, int data_len, int offset) {
         synchronized (mLock) {
           try {
               return mProxy.sysSSMReadNTypes(id, data_len, offset);
           } catch (RemoteException e) {
               Log.e(TAG, "SysSSMReadNTypes:" + e);
           }
       }
       return -1;

    }

    public int SysSSMWriteNTypes(int id, int data_len, int data_buf, int offset) {
          synchronized (mLock) {
            try {
                return mProxy.sysSSMWriteNTypes(id, data_len, data_buf, offset);
            } catch (RemoteException e) {
                Log.e(TAG, "SysSSMWriteNTypes:" + e);
            }
        }
        return -1;

    }

    public int GetActualAddr(int id) {
          synchronized (mLock) {
          Mutable<Integer> resultVal = new Mutable<>();
            try {
                return mProxy.getActualAddr(id);
            } catch (RemoteException e) {
                Log.e(TAG, "GetActualAddr:" + e);
            }
        }
        return -1;

    }

    public int GetActualSize(int id) {
          synchronized (mLock) {
            try {
                return mProxy.getActualSize(id);
            } catch (RemoteException e) {
                Log.e(TAG, "GetActualSize:" + e);
            }
        }
        return -1;

    }

    public int SSMRecovery() {
          synchronized (mLock) {
            try {
                return mProxy.SSMRecovery();
            } catch (RemoteException e) {
                Log.e(TAG, "SSMRecovery:" + e);
            }
        }
        return -1;

    }

    public int SetPLLValues(SourceInputParam srcInputParam) {
          synchronized (mLock) {
            try {
                return mProxy.setPLLValues(srcInputParam);
            } catch (RemoteException e) {
                Log.e(TAG, "SetPLLValues:" + e);
            }
        }
        return -1;

    }

     public int SetCVD2Values() {
           synchronized (mLock) {
             try {
                 return mProxy.setCVD2Values();
             } catch (RemoteException e) {
                 Log.e(TAG, "SetCVD2Values:" + e);
             }
         }
         return -1;

     }

    public int GetSSMStatus() {
         synchronized (mLock) {
           try {
               return mProxy.getSSMStatus();
           } catch (RemoteException e) {
               Log.e(TAG, "GetSSMStatus:" + e);
           }
       }
       return -1;
    }

     public int SetCurrentSourceInfo(SourceInput source, int sig_fmt, int trans_fmt) {
           synchronized (mLock) {
             try {
                 return mProxy.setCurrentSourceInfo(source.toInt(), sig_fmt, trans_fmt);
             } catch (RemoteException e) {
                 Log.e(TAG, "SetCurrentSourceInfo:" + e);
             }
         }
         return -1;
     }

     public int[] GetCurrentSourceInfo() {
           int CurrentSourceInfo[] = {0, 0, 0};
           synchronized (mLock) {
               Mutable<SourceInputParam> srcInputParam = new Mutable<>();
               try {
                   mProxy.getCurrentSourceInfo((int ret, SourceInputParam tmpSrcInputParam)-> {
                                                  if (Result.OK == ret) {
                                                      srcInputParam.value = tmpSrcInputParam;
                                                  }
                                              });
                 CurrentSourceInfo[0] = srcInputParam.value.sourceInput;
                 CurrentSourceInfo[1] = srcInputParam.value.sigFmt;
                 CurrentSourceInfo[2] = srcInputParam.value.transFmt;
                 return CurrentSourceInfo;
             } catch (RemoteException e) {
                 Log.e(TAG, "GetCurrentSourceInfo:" + e);
             }
         }
         return CurrentSourceInfo;
     }

     /**
      * @Function: FactoryGetRGBScreen
      * @Description: get rgb screen pattern
      * @Return: rgb(0xrrggbb)
      */
     public int FactoryGetRGBScreen() {
           synchronized (mLock) {
             try {
                 return mProxy.getRGBPattern();
             } catch (RemoteException e) {
                 Log.e(TAG, "FactoryGetRGBScreen:" + e);
             }
         }
         return -1;
     }

    /**
     * @Function: FactorySetRGBScreen
     * @Description: set test pattern with rgb.
     * @Param r,g,b int 0~255
     * @Return: -1 failed, otherwise success
     */
    public int FactorySetRGBScreen(int r, int g, int b) {
          synchronized (mLock) {
            try {
                return mProxy.setRGBPattern(r, g, b);
            } catch (RemoteException e) {
                Log.e(TAG, "FactorySetRGBScreen:" + e);
            }
        }
        return -1;

    }

    /**
     * @Function: FactorySetDDRSSC
     * @Description: Set ddr ssc level for factory menu conctrol
     * @Param: step ddr ssc level
     * @Return: 0 success, -1 fail
     */
    public int FactorySetDDRSSC(int step) {
          synchronized (mLock) {
            try {
                return mProxy.factorySetDDRSSC(step);
            } catch (RemoteException e) {
                Log.e(TAG, "FactorySetDDRSSC:" + e);
            }
        }
        return -1;
    }

    /**
     * @Function: FactoryGetDDRSSC
     * @Description: Get ddr ssc level for factory menu conctrol
     * @Param:
     * @Return: ddr ssc level
     */
    public int FactoryGetDDRSSC() {
          synchronized (mLock) {
            try {
                return mProxy.factoryGetDDRSSC();
            } catch (RemoteException e) {
                Log.e(TAG, "FactoryGetDDRSSC:" + e);
            }
        }
        return -1;
    }

    /**
     * @Function: FactorySetLVDSSSC
     * @Description: Set lvds ssc level for factory menu conctrol
     * @Param: step lvds ssc level
     * @Return: 0 success, -1 fail
     */
    public int FactorySetLVDSSSC(int step) {
          synchronized (mLock) {
            try {
                return mProxy.factorySetLVDSSSC(step);
            } catch (RemoteException e) {
                Log.e(TAG, "FactorySetLVDSSSC:" + e);
            }
        }
        return -1;
    }

    /**
     * @Function: FactoryGetLVDSSSC
     * @Description: Get lvds ssc level for factory menu conctrol
     * @Param:
     * @Return: lvds ssc level
     */
    public int FactoryGetLVDSSSC() {
          synchronized (mLock) {
            try {
                return mProxy.factoryGetLVDSSSC();
            } catch (RemoteException e) {
                Log.e(TAG, "FactoryGetLVDSSSC:" + e);
            }
        }
        return -1;
    }

    public int FactoryWhiteBalanceOpenGrayPattern() {
          synchronized (mLock) {
            try {
                return mProxy.whiteBalanceGrayPatternOpen();
            } catch (RemoteException e) {
                Log.e(TAG, "FactoryWhiteBalanceOpenGrayPattern:" + e);
            }
        }
        return -1;
    }

    public int FactoryWhiteBalanceCloseGrayPattern() {
          synchronized (mLock) {
            try {
                return mProxy.whiteBalanceGrayPatternClose();
            } catch (RemoteException e) {
                Log.e(TAG, "FactoryWhiteBalanceCloseGrayPattern:" + e);
            }
        }
        return -1;
    }

    public int FactoryWhiteBalanceSetGrayPattern(int value) {
          synchronized (mLock) {
            try {
                return mProxy.whiteBalanceGrayPatternSet(value);
            } catch (RemoteException e) {
                Log.e(TAG, "FactoryWhiteBalanceSetGrayPattern:" + e);
            }
        }
        return -1;
    }

    public int FactoryWhiteBalanceGetGrayPattern() {
          synchronized (mLock) {
            try {
                return mProxy.whiteBalanceGrayPatternGet();
            } catch (RemoteException e) {
                Log.e(TAG, "FactoryWhiteBalanceGetGrayPattern:" + e);
            }
        }
        return -1;
    }

     public int FactorySetHdrIsEnable(int mode) {
         synchronized (mLock) {
             try {
                 return mProxy.factorySetHdrMode(mode);
             } catch (RemoteException e) {
                 Log.e(TAG, "FactorySetHdrIsEnable:" + e);
             }
         }
         return -1;
     }

     public int FactoryGetHdrIsEnable() {
         synchronized (mLock) {
             try {
                 return mProxy.factoryGetHdrMode();
             } catch (RemoteException e) {
                 Log.e(TAG, "FactoryGetHdrIsEnable:" + e);
             }
         }
         return -1;
     }

     public int setDNLPCurveParams(SourceInput source, SignalFmt sig_fmt, TransFmt trans_fmt, int level) {
         synchronized (mLock) {
             try {
                 return mProxy.setDnlpParams(source.toInt(), sig_fmt.toInt(), trans_fmt.toInt(), level);
             } catch (RemoteException e) {
                 Log.e(TAG, "setDNLPCurveParams:" + e);
             }
         }
         return -1;
     }

     public int getDNLPCurveParams(SourceInput source, SignalFmt sig_fmt, TransFmt trans_fmt) {
         synchronized (mLock) {
             try {
                 return mProxy.getDnlpParams(source.toInt(), sig_fmt.toInt(), trans_fmt.toInt());
             } catch (RemoteException e) {
                 Log.e(TAG, "getDNLPCurveParams:" + e);
             }
         }
         return -1;
     }

     public int FactorySetDNLPCurveParams(SourceInput source, SignalFmt sig_fmt, TransFmt trans_fmt, int level, int final_gain) {
         synchronized (mLock) {
             try {
                 return mProxy.factorySetDnlpParams(source.toInt(), sig_fmt.toInt(), trans_fmt.toInt(), level, final_gain);
             } catch (RemoteException e) {
                 Log.e(TAG, "FactorySetDNLPCurveParams:" + e);
             }
         }
         return -1;
     }

     public int FactoryGetDNLPCurveParams(SourceInput source, SignalFmt sig_fmt, TransFmt trans_fmt, int level) {
         synchronized (mLock) {
             try {
                 return mProxy.factoryGetDnlpParams(source.toInt(), sig_fmt.toInt(), trans_fmt.toInt(), level);
             } catch (RemoteException e) {
                 Log.e(TAG, "FactoryGetDNLPCurveParams:" + e);
             }
         }
         return -1;
     }

     public int FactorysetBlackExtRegParams(SourceInput source, SignalFmt sig_fmt, TransFmt trans_fmt, int val) {
         synchronized (mLock) {
             try {
                 return mProxy.factorySetBlackExtRegParams(source.toInt(), sig_fmt.toInt(), trans_fmt.toInt(), val);
             } catch (RemoteException e) {
                 Log.e(TAG, "FactorysetBlackExtRegParams:" + e);
             }
         }
         return -1;
     }

     public int FactorygetBlackExtRegParams(SourceInput source, SignalFmt sig_fmt, TransFmt trans_fmt) {
         synchronized (mLock) {
             try {
                 return mProxy.factoryGetBlackExtRegParams(source.toInt(), sig_fmt.toInt(), trans_fmt.toInt());
             } catch (RemoteException e) {
                 Log.e(TAG, "FactorygetBlackExtRegParams:" + e);
             }
         }
         return -1;
     }

     public int FactorySetColorParams(SourceInput source, SignalFmt sig_fmt, TransFmt trans_fmt, int color_type, int color_param, int val) {
         synchronized (mLock) {
             try {
                 return mProxy.factorySetColorParams(source.toInt(), sig_fmt.toInt(), trans_fmt.toInt(), color_type, color_param, val);
             } catch (RemoteException e) {
                 Log.e(TAG, "FactorySetColorParams:" + e);
             }
         }
         return -1;
     }

     public int FactoryGetColorParams(SourceInput source, SignalFmt sig_fmt, TransFmt trans_fmt, int color_type, int color_param) {
         synchronized (mLock) {
             try {
                 return mProxy.factoryGetColorParams(source.toInt(), sig_fmt.toInt(), trans_fmt.toInt(), color_type, color_param);
             } catch (RemoteException e) {
                 Log.e(TAG, "FactoryGetColorParams:" + e);
             }
         }
         return -1;
     }

     public int FactorySetNoiseReductionParams(SourceInput source, SignalFmt sig_fmt, TransFmt trans_fmt, Noise_Reduction_Mode mode, int param_type, int val) {
         synchronized (mLock) {
             try {
                 return mProxy.factorySetNoiseReductionParams(source.toInt(), sig_fmt.toInt(), trans_fmt.toInt(), mode.toInt(), param_type, val);
             } catch (RemoteException e) {
                 Log.e(TAG, "FactorySetNoiseReductionParams:" + e);
             }
         }
         return -1;
     }

     public int FactoryGetNoiseReductionParams(SourceInput source, SignalFmt sig_fmt, TransFmt trans_fmt, Noise_Reduction_Mode mode, int param_type) {
         synchronized (mLock) {
             try {
                 return mProxy.factoryGetNoiseReductionParams(source.toInt(), sig_fmt.toInt(), trans_fmt.toInt(), mode.toInt(), param_type);
             } catch (RemoteException e) {
                 Log.e(TAG, "FactoryGetNoiseReductionParams:" + e);
             }
         }
         return -1;
     }

     public int FactorySetCTIParams(SourceInput source, SignalFmt sig_fmt, TransFmt trans_fmt, int param_type, int val) {
         synchronized (mLock) {
             try {
                 return mProxy.factorySetCTIParams(source.toInt(), sig_fmt.toInt(), trans_fmt.toInt(), param_type, val);
             } catch (RemoteException e) {
                 Log.e(TAG, "FactorySetCTIParams:" + e);
             }
         }
         return -1;
     }

     public int FactoryGetCTIParams(SourceInput source, SignalFmt sig_fmt, TransFmt trans_fmt, int param_type) {
         synchronized (mLock) {
             try {
                 return mProxy.factoryGetCTIParams(source.toInt(), sig_fmt.toInt(), trans_fmt.toInt(), param_type);
             } catch (RemoteException e) {
                 Log.e(TAG, "FactoryGetCTIParams:" + e);
             }
         }
         return -1;
     }

     public int FactorySetDecodeLumaParams(SourceInput source, SignalFmt sig_fmt, TransFmt trans_fmt, int param_type, int val) {
         synchronized (mLock) {
             try {
                 return mProxy.factorySetDecodeLumaParams(source.toInt(), sig_fmt.toInt(), trans_fmt.toInt(), param_type, val);
             } catch (RemoteException e) {
                 Log.e(TAG, "FactorySetDecodeLumaParams:" + e);
             }
         }
         return -1;
     }

     public int FactoryGetDecodeLumaParams(SourceInput source, SignalFmt sig_fmt, TransFmt trans_fmt, int param_type) {
         synchronized (mLock) {
             try {
                 return mProxy.factoryGetDecodeLumaParams(source.toInt(), sig_fmt.toInt(), trans_fmt.toInt(), param_type);
             } catch (RemoteException e) {
                 Log.e(TAG, "FactoryGetDecodeLumaParams:" + e);
             }
         }
         return -1;
     }

     public int FactorySetSharpnessHDParams(SourceInput source, SignalFmt sig_fmt, TransFmt trans_fmt, int isHD, int param_type, int val) {
         synchronized (mLock) {
             try {
                 return mProxy.factorySetSharpnessParams(source.toInt(), sig_fmt.toInt(), trans_fmt.toInt(), isHD, param_type, val);
             } catch (RemoteException e) {
                 Log.e(TAG, "FactorySetSharpnessHDParams:" + e);
             }
         }
         return -1;
     }

     public int FactoryGetSharpnessHDParams(SourceInput source, SignalFmt sig_fmt, TransFmt trans_fmt, int isHD, int param_type) {
         synchronized (mLock) {
             try {
                 return mProxy.factoryGetSharpnessParams(source.toInt(), sig_fmt.toInt(), trans_fmt.toInt(), isHD, param_type);
             } catch (RemoteException e) {
                 Log.e(TAG, "FactoryGetSharpnessHDParams:" + e);
             }
         }
         return -1;
     }

     public int SetDtvKitSourceEnable(int isEnable) {
         synchronized (mLock) {
             try {
                 return mProxy.setDtvKitSourceEnable(isEnable);
             } catch (Exception e) {
                 Log.e(TAG, "SetDtvKitSourceEnable:" + e);
             }
         }
         return -1;
     }

     public int setAipqEnable(boolean isEnable) {
         synchronized (mLock) {
             try {
                 return mProxy.setAipqEnable(isEnable ? 1 : 0);
             } catch (Exception e) {
                 Log.e(TAG, "setAipqEnable:" + e);
             }
         }
         return -1;
     }

     public boolean getAipqEnable() {
         synchronized (mLock) {
             try {
                 return mProxy.getAipqEnable() == 1;
             } catch (Exception e) {
                 Log.e(TAG, "getAipqEnable:" + e);
             }
         }
         return false;
     }

     public boolean aisrContrl(boolean on) {
         synchronized (mLock) {
             try {
                 return (mProxy.aisrContrl(on) == Result.OK);
             } catch (Exception e) {
                 Log.e(TAG, "aisrContrl:" + e);
             }
         }
         return false;
     }

     public boolean hasAisrFunc() {
         synchronized (mLock) {
             try {
                 return (mProxy.hasAisrFunc() == Result.OK);
             } catch (Exception e) {
                 Log.e(TAG, "hasAisrFunc:" + e);
             }
         }
         return false;
     }

     public boolean getAisr() {
         synchronized (mLock) {
             try {
                 return (mProxy.getAisr() == Result.OK);
             } catch (Exception e) {
                 Log.e(TAG, "getAisr:" + e);
             }
         }
         return false;
     }

     public int SetColorGamutMode(int mode, int isSave) {
           synchronized (mLock) {
             try {
                 return mProxy.setColorGamutMode(mode, isSave);
             } catch (RemoteException e) {
                 Log.e(TAG, "SetColorGamutMode:" + e);
             }
         }
         return -1;
     }

     public int GetColorGamutMode() {
           synchronized (mLock) {
             try {
                 return mProxy.getColorGamutMode();
             } catch (RemoteException e) {
                 Log.e(TAG, "GetColorGamutMode:" + e);
             }
         }
         return -1;
     }

     /**
      * @Function: getChipVersionInfo
      * @Description: Get chip version info
      * @Param:
      * @Return: chip type or NULL
      */
     public String getChipVersionInfo() {
         synchronized (mLock) {
             Mutable<String> resultVal = new Mutable<>();
             try {
                 mProxy.getChipVersionInfo((int ret, String v) -> {
                     if (Result.OK == ret) {
                         resultVal.value = v;
                     }
                 });
                 return resultVal.value;
             } catch (RemoteException e) {
                 Log.e(TAG, "getChipVersionInfo:" + e);
             }
         }
         return "";
     }

     public enum DataBase_Name {
         DATABASE_NAME_PQ(0),
         DATABASE_NAME_OVERSCAN(1),
         DATABASE_NAME_MAX(2);
         private int val;

         DataBase_Name(int val) {
             this.val = val;
         }

         public int toInt() {
             return this.val;
         }
     }

    public class DatabaseInfo {
        public String ToolVersion;
        public String ProjectVersion;
        public String GenerateTime;
    }
     /**
      * @Function: GetPQDatabaseInfo
      * @Description: Get database toolversion projectversion and generateTime
      * @Param:databaseName the name of database which you want get info
      * @Return: 0 success, -1 fail
      */
     public DatabaseInfo GetPQDatabaseInfo(DataBase_Name databaseName) {
         DatabaseInfo dataBaseInfo = new DatabaseInfo();
         synchronized (mLock) {
             try {
                 PQDatabaseInfo info = mProxy.getPQDatabaseInfo(databaseName.toInt());
                 dataBaseInfo.ToolVersion = info.ToolVersion;
                 dataBaseInfo.ProjectVersion = info.ProjectVersion;
                 dataBaseInfo.GenerateTime = info.GenerateTime;
                 return dataBaseInfo;
             } catch (Exception e) {
                 Log.e(TAG, "GetPQDatabaseInfo:" + e);
             }
         }
         return dataBaseInfo;
     }

    /**
    * @Function: setScreenColorForSignalChange
    * @Description: set screen color for switch source/switch channal/no signal
    * @Param: screenColor:screen color:0 is black screen; 1 is blue screen
    * @Param: isSave:wthether want to save setting
    * @Return: 0 ok or -1 error
    */
    public int setScreenColorForSignalChange(int screenColor, int isSave) {
        synchronized (mLock) {
            try {
                return mProxy.setScreenColorForSignalChange(screenColor, isSave);
            } catch (Exception e) {
                Log.e(TAG, "setScreenColorForSignalChange:" + e);
            }
        }
        return -1;
    }

    /**
    * @Function: getScreenColorForSignalChange
    * @Description: get screen color for switch source/switch channal/no signal
    * @Return: 0 is balck acreen; 1 is blue screen;
    */
    public int getScreenColorForSignalChange() {
        synchronized (mLock) {
            try {
                return mProxy.getScreenColorForSignalChange();
            } catch (Exception e) {
                Log.e(TAG, "getScreenColorForSignalChange:" + e);
            }
        }
        return -1;
    }

    /**
    * @Function: setStaticFrameStatus
    * @Description: update status of static frame:when enable it,
                    can black video for switching program
    * @Param: status: 0 is disable, 1 is enable
    * @Param: isSave:wthether want to save setting
    * @Return: 0 ok or -1 error
    */
    public int setStaticFrameStatus(int status, int isSave){
        synchronized (mLock) {
            try {
                return mProxy.setStaticFrameEnable(status, isSave);
            } catch (Exception e) {
                Log.e(TAG, "setStaticFrameStatus:" + e);
            }
        }
        return -1;
    }

    /**
    * @Function: getStaticFrameStatus
    * @Description: get status of static frame
    * @Return: 0 is disable, 1 is enable
    */
    public int getStaticFrameStatus() {
        synchronized (mLock) {
            try {
                return mProxy.getStaticFrameEnable();
            } catch (Exception e) {
                Log.e(TAG, "getStaticFrameStatus:" + e);
            }
        }
        return 0;
    }

    private static class Mutable<E> {
        public E value;

        Mutable() {
            value = null;
        }

        Mutable(E value) {
            this.value = value;
        }
    }

    final class DeathRecipient implements HwBinder.DeathRecipient {
        DeathRecipient() {
        }

        @Override
        public void serviceDied(long cookie) {
            if (SYSTEM_CONTROL_DEATH_COOKIE == cookie) {
                Log.e(TAG, "system control service died cookie: " + cookie);
                synchronized (mLock) {
                    mProxy = null;
                }
            }
        }
    }

    /*
    final class ServiceNotification extends IServiceNotification.Stub {
        @Override
        public void onRegistration(String fqName, String name, boolean preexisting) {
            Log.i(TAG, "system control service started " + fqName + " " + name);
            connectToProxy();
        }
    }
    */

    public static class DisplayInfo{
        /*//1:tablet 2:MBOX 3:TV
        public int type;
        public String socType;
        public String defaultUI;
        public int fb0Width;
        public int fb0Height;
        public int fb0FbBits;
        public boolean fb0TripleEnable;//Triple Buffer enable or not

        public int fb1Width;
        public int fb1Height;
        public int fb1FbBits;
        public boolean fb1TripleEnable;//Triple Buffer enable or not*/
    }
}
