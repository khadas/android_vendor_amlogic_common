package cn.com.factorytest;

import java.io.File;
import java.io.IOException;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Environment;
import android.os.Bundle;
import android.util.Log;

import java.io.IOException;

import android.widget.Toast;
import android.provider.Settings;

import cn.com.factorytest.MainActivity;

public class FactoryReceiver extends BroadcastReceiver {
    private static final String TAG = Tools.TAG;

    private static final String udiskfile = "khadas_test.xml";

    @Override
    public void onReceive(Context context, Intent intent) {
        // TODO Auto-generated method stub
        String action = intent.getAction();

        if (action.equals(Intent.ACTION_BOOT_COMPLETED)) {
            String mac = Tools.getMac();
            if (mac.equals("00:00:00:00:00:00")) {
                mac = Tools.getSharedPreference(context);
                if (!mac.equals("00:00:00:00:00:00")) {
                    String cmd = String.format("setbootenv ubootenv.var.factory_mac %s", mac);
                    try {
                        Process exeCmd = Runtime.getRuntime().exec(cmd);
                    } catch (IOException e) {
                        Log.e(TAG, "Excute exception: " + e.getMessage());
                    }
                }
            }
            Log.d(TAG, "Factory action=" + action);

            try {
                Thread.sleep(8 * 1000);
            } catch (Exception e) {
                e.printStackTrace();
            }

            try {
                String rec = Tools.execCommand(new String[]{"sh", "-c", "ls /mnt/media_rw/"});
                Log.e(TAG, "rec=" + rec);
                if (rec == null || rec.equals("")) {
                    return;
                }

                for (int i = 0; i < rec.length(); i = i + 9) {
                    String bootpath = "/storage/" + rec.substring(i, i + 9) + "/" + udiskfile;
                    Log.e(TAG, "bootpath=" + bootpath);
                    File file = new File(bootpath);
                    if (file.exists() && file.isFile()) {
                        goto_factorytest(context, bootpath);
                        return;
                    }
                }

            } catch (IOException e) {
                e.printStackTrace();
            }

            return;
        }

        Log.d(TAG, "Factory action=" + action);
        Uri uri = intent.getData();

        if (uri.getScheme().equals("file")) {
            String path = uri.getPath();
            String externalStoragePath = Environment.getExternalStorageDirectory().getPath();
            String legacyPath = Environment.getLegacyExternalStorageDirectory().getPath();


            try {
                path = new File(path).getCanonicalPath();
            } catch (IOException e) {
                Log.e(TAG, "couldn't canonicalize " + path);
                return;
            }
            if (path.startsWith(legacyPath)) {
                path = externalStoragePath + path.substring(legacyPath.length());
            }

            String fullpath = path + "/" + udiskfile;
            goto_factorytest(context, fullpath);
        }
    }

    private void goto_factorytest(Context context, String fullpath) {
        File file = new File(fullpath);
        if (file.exists() && file.isFile()) {
            try {
                Thread.sleep(1000);
            } catch (Exception e) {
                e.printStackTrace();
            }

            try {
                String rec = Tools.execCommand(new String[]{"sh", "-c", "cat " + fullpath});
                if (rec.contains("reboot_test=1")) {

                    int reboot_num = Settings.System.getInt(context.getContentResolver(), "Khadas_reboot_test_num", 0);

                    Toast.makeText(context, "Reboot Test : " + reboot_num, Toast.LENGTH_LONG).show();
                    try {
                        Thread.sleep(10 * 1000);
                        Settings.System.putInt(context.getContentResolver(), "Khadas_reboot_test_num", reboot_num + 1);
                        Thread.sleep(1 * 1000);
                        Process proc = Runtime.getRuntime().exec(new String[]{"reboot"});
                        proc.waitFor();
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                    return;
                } else {
                    //test_board
                    if(rec.contains("test_board=VIM1")){
                        MainActivity.test_board = "VIM1";
                    }else if(rec.contains("test_board=VIM2")){
                        MainActivity.test_board = "VIM2";
                    }else if(rec.contains("test_board=VIM3")){
                        MainActivity.test_board = "VIM3";
                    }else if(rec.contains("test_board=VIM4")){
                        MainActivity.test_board = "VIM4";
                    }else{
                        MainActivity.test_board = "VIM4";
                    }
                    //tfcard_test
                    MainActivity.tfcard_test = rec.contains("tfcard_test=1");
                    //usb20_test
                    MainActivity.usb20_test = rec.contains("usb20_test=1");
                    //usb30_test
                    MainActivity.usb30_test = rec.contains("usb30_test=1");
                    //spi_test
                    MainActivity.spi_test = rec.contains("spi_test=1");
                    //gsensor_test
                    MainActivity.gsensor_test = rec.contains("gsensor_test=1");
                    //mcu_test
                    MainActivity.mcu_test = rec.contains("mcu_test=1");
                    //hdmi_test
                    MainActivity.hdmi_test = rec.contains("hdmi_test=1");

                    //fusb302_test
                    MainActivity.fusb302_test = rec.contains("fusb302_test=1");
                    //gigabit_test
                    MainActivity.gigabit_test = rec.contains("gigabit_test=1");
                    //lan_test
                    MainActivity.lan_test = rec.contains("lan_test=1");
                    //wifi_test
                    MainActivity.wifi_test = rec.contains("wifi_test=1");
                    //bt_test
                    MainActivity.bt_test = rec.contains("bt_test=1");
                    //rtc_test
                    MainActivity.rtc_test = rec.contains("rtc_test=1");
                    //ageing_test
                    MainActivity.ageing_test = rec.contains("ageing_test=1");
                    if (MainActivity.ageing_test) {
                        MainActivity.ageing_flag = 1;
                    } else {
                        MainActivity.ageing_flag = 0;
                    }
                    //ageing_test_time
                    if (MainActivity.ageing_test) {
                        if (rec.contains("ageing_test_time=8")) {
                            MainActivity.ageing_time = 8;
                        } else if (rec.contains("ageing_test_time=12")) {
                            MainActivity.ageing_time = 12;
                        } else if (rec.contains("ageing_test_time=24")) {
                            MainActivity.ageing_time = 24;
                        } else {
                            MainActivity.ageing_time = 4;
                        }
                    }
                    //ageing ageing_cpu_max
                    if (MainActivity.ageing_test) {
                        if (rec.contains("ageing_cpu_max=1")) {
                            MainActivity.ageing_cpu_max = 1;
                        } else if (rec.contains("ageing_cpu_max=2")) {
                            MainActivity.ageing_cpu_max = 2;
                        } else if (rec.contains("ageing_cpu_max=3")) {
                            MainActivity.ageing_cpu_max = 3;
                        } else if (rec.contains("ageing_cpu_max=4")) {
                            MainActivity.ageing_cpu_max = 4;
                        } else if (rec.contains("ageing_cpu_max=5")) {
                            MainActivity.ageing_cpu_max = 5;
                        } else if (rec.contains("ageing_cpu_max=6")) {
                            MainActivity.ageing_cpu_max = 6;
                        } else {
                            MainActivity.ageing_cpu_max = 0;
                        }
                    }
                    Log.d(TAG, "ageing_flag=" + MainActivity.ageing_flag);
                    Log.d(TAG, "ageing_time=" + MainActivity.ageing_time);
                    Log.d(TAG, "ageing_cpu_max=" + MainActivity.ageing_cpu_max);
                    //power_led_test
                    MainActivity.power_led_test = rec.contains("power_led_test=1");
                    MainActivity.power_led_test = false;//remove power_led_test
                    //irkey_test
                    MainActivity.irkey_test = rec.contains("irkey_test=1");
                    //wol_enable
                    MainActivity.wol_enable = rec.contains("wol_enable=1");
                    //mic_test
                    MainActivity.mic_test = rec.contains("mic_test=1");
                    //mipi_camera_test
                    MainActivity.mipi_camera_test = rec.contains("mipi_camera_test=1");
                    //board_key_test
                    MainActivity.board_key_test = rec.contains("board_key_test=1");
                    //reset_mcu
                    MainActivity.reset_mcu = rec.contains("reset_mcu=1");
                    //wirte_mac
                    MainActivity.wirte_mac = rec.contains("wirte_mac=1");
                    Intent i = new Intent();
                    i.setClassName("cn.com.factorytest", "cn.com.factorytest.MainActivity");
                    i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                    context.startActivity(i);
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

}
