/*
 * Copyright 2016, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <wifi_info.h>

static const dongle_info dongle_registerd[]={\
    {"a962","0000","dhd","/vendor/lib/modules/dhd.ko","firmware_path=/vendor/etc/wifi/40181/fw_bcm40181a2.bin nvram_path=/vendor/etc/wifi/40181/nvram.txt","bcm6210",0x0,"/vendor/etc/wifi/40181/fw_bcm40181a2","",true,false},\
    {"0000","0000","wlan_mt76x8_usb","/vendor/lib/modules/wlan_mt76x8_usb.ko","sta=wlan ap=wlan p2p=p2p","mtk7668u",0x7668,"","btmtk_usb",true,true},\
    {"4335","0000","dhd","/vendor/lib/modules/dhd.ko","firmware_path=/vendor/etc/wifi/6335/fw_bcm4339a0_ag.bin nvram_path=/vendor/etc/wifi/6335/nvram.txt","bcm6335",0x0,"/vendor/etc/wifi/6335/fw_bcm4339a0_ag","",true,false},\
    {"0000","43c5","dhdpci","/vendor/lib/modules/dhdpci.ko","firmware_path=/vendor/etc/wifi/4336/fw_bcm4336_ag.bin nvram_path=/vendor/etc/wifi/4336/nvram.txt","bcm4336",0x0,"/vendor/etc/wifi/4336/fw_bcm4336_ag","",true,false},\
    {"a94d","0000","dhd","/vendor/lib/modules/dhd.ko","firmware_path=/vendor/etc/wifi/6234/fw_bcm43341b0_ag.bin nvram_path=/vendor/etc/wifi/6234/nvram.txt","bcm6234",0x0,"/vendor/etc/wifi/6234/fw_bcm43341b0_ag","",true,false},\
    {"a9bf","0000","dhd","/vendor/lib/modules/dhd.ko","firmware_path=/vendor/etc/wifi/6255/fw_bcm43455c0_ag.bin nvram_path=/vendor/etc/wifi/6255/nvram.txt","bcm6255",0x0,"/vendor/etc/wifi/6255/fw_bcm43455c0_ag","",true,false},\
    {"aae7","0000","dhd","/vendor/lib/modules/dhd.ko","firmware_path=/vendor/etc/wifi/AP6271/fw_bcm43751a1_ag.bin nvram_path=/vendor/etc/wifi/AP6271/nvram_ap6271s.txt","bcm6271",0x0,"/vendor/etc/wifi/AP6271/fw_bcm43751a1_ag","",true,false},\
    {"a9a6","0000","dhd","/vendor/lib/modules/dhd.ko","firmware_path=/vendor/etc/wifi/6212/fw_bcm43438a0.bin nvram_path=/vendor/etc/wifi/6212/nvram.txt","bcm6212",0x0,"/vendor/etc/wifi/6212/fw_bcm43438a0","",true,false},\
    {"4362","0000","dhd","/vendor/lib/modules/dhd.ko","firmware_path=/vendor/etc/wifi/43751/fw_bcm43751_ag.bin nvram_path=/vendor/etc/wifi/43751/nvram.txt","bcm43751",0x0,"/vendor/etc/wifi/43751/fw_bcm43751_ag","",true,false},\
    {"4345","0000","dhd","/vendor/lib/modules/dhd.ko","firmware_path=/vendor/etc/wifi/43458/fw_bcm43455c0_ag.bin nvram_path=/vendor/etc/wifi/43458/nvram_43458.txt","bcm43458",0x0,"/vendor/etc/wifi/43458/fw_bcm43455c0_ag","",true,false},\
    {"4354","0000","dhd","/vendor/lib/modules/dhd.ko","firmware_path=/vendor/etc/wifi/4354/fw_bcm4354a1_ag.bin nvram_path=/vendor/etc/wifi/4354/nvram_ap6354.txt","bcm6354",0x0,"/vendor/etc/wifi/4354/fw_bcm4354a1_ag","",true,false},\
    {"4356","0000","dhd","/vendor/lib/modules/dhd.ko","firmware_path=/vendor/etc/wifi/4356/fw_bcm4356a2_ag.bin nvram_path=/vendor/etc/wifi/4356/nvram_ap6356.txt","bcm6356",0x0,"/vendor/etc/wifi/4356/fw_bcm4356a2_ag","",true,false},\
    {"4359","0000","dhd","/vendor/lib/modules/dhd.ko","firmware_path=/vendor/etc/wifi/4359/fw_bcm4359c0_ag.bin nvram_path=/vendor/etc/wifi/4359/nvram.txt","bcm4359",0x0,"/vendor/etc/wifi/4359/fw_bcm4359c0_ag","",true,false},\
    {"0000","4415","dhdpci","/vendor/lib/modules/dhdpci.ko","firmware_path=/vendor/etc/wifi/4359/fw_bcm4359c0_ag.bin nvram_path=/vendor/etc/wifi/4359/nvram.txt","bcm4359",0x0,"/vendor/etc/wifi/4359/fw_bcm4359c0_ag","",true,false},\
    {"aa31","0000","dhd","/vendor/lib/modules/dhd.ko","firmware_path=/vendor/etc/wifi/4358/fw_bcm4358_ag.bin nvram_path=/vendor/etc/wifi/4358/nvram_4358.txt","bcm4358",0x0,"/vendor/etc/wifi/4358/fw_bcm4358_ag","",true,false},\
    {"8888","0000","vlsicomm","/vendor/lib/modules/vlsicomm.ko","","amlwifi",0x0,"","",true,true},\
    {"8179","0000","8189es","/vendor/lib/modules/8189es.ko","ifname=wlan0 if2name=p2p0","rtl8189es",0x0,"","",false,false},\
    {"b723","0000","8723bs","/vendor/lib/modules/8723bs.ko","ifname=wlan0 if2name=p2p0","rtl8723bs",0x0,"","",true,false},\
    {"c723","0000","8723cs","/vendor/lib/modules/8723cs.ko","ifname=wlan0 if2name=p2p0","rtl8723cs",0x0,"","",true,false},\
    {"f179","0000","8189fs","/vendor/lib/modules/8189fs.ko","ifname=wlan0 if2name=p2p0","rtl8189ftv",0x0,"","",false,false},\
    {"818b","0000","8192es","/vendor/lib/modules/8192es.ko","ifname=wlan0 if2name=p2p0","rtl8192es",0x0,"","",false,false},\
    {"0000","0000","8188eu","/vendor/lib/modules/8188eu.ko","ifname=wlan0 if2name=p2p0","rtl8188eu",0x8179,"","",false,false},\
    {"0000","0000","8188gtvu","/vendor/lib/modules/8188gtvu.ko","ifname=wlan0 if2name=p2p0","rtl8188gtv",0x018c,"","",false,false},\
    {"0000","0000","atbm602x_usb","/vendor/lib/modules/atbm602x_usb.ko","","atbm602x",0x8888,"","",false,false},\
    {"0000","0000","8188eu","/vendor/lib/modules/8188eu.ko","ifname=wlan0 if2name=p2p0","rtl8188eu",0x0179,"","",false,false},\
    {"0000","0000","mt7601usta","/vendor/lib/modules/mt7601usta.ko","","mtk7601",0x7601,"","",false,false},\
    {"0000","0000","mt7603usta","/vendor/lib/modules/mt7603usta.ko","","mtk7603",0x7603,"","",false,false},\
    {"0000","0000","mt7662u_sta","/vendor/lib/modules/mt7662u_sta.ko","","mtk7632",0x76a0,"","",false,false},\
    {"0000","0000","mt7662u_sta","/vendor/lib/modules/mt7662u_sta.ko","","mtk7632",0x76a1,"","",false,false},\
    {"c821","0000","8821cs","/vendor/lib/modules/8821cs.ko","ifname=wlan0 if2name=p2p0","rtl8821cs",0x0,"","",true,false},\
    {"b821","0000","8821cs","/vendor/lib/modules/8821cs.ko","ifname=wlan0 if2name=p2p0","rtl8821cs",0x0,"","",true,false},\
    {"0000","0000","8723du","/vendor/lib/modules/8723du.ko","ifname=wlan0 if2name=p2p0","rtl8723du",0xd723,"","rtk_btusb",true,true},\
    {"d723","0000","8723ds","/vendor/lib/modules/8723ds.ko","ifname=wlan0 if2name=p2p0","rtl8723ds",0x0,"","",true,false},\
    {"0000","0000","8821au","/vendor/lib/modules/8821au.ko","ifname=wlan0 if2name=p2p0","rtl8821au",0x0823,"","rtk_btusb",true,true},\
    {"0000","0000","8821au","/vendor/lib/modules/8821au.ko","ifname=wlan0 if2name=p2p0","rtl8821au",0x0821,"","rtk_btusb",true,true},\
    {"0000","0000","8821au","/vendor/lib/modules/8821au.ko","ifname=wlan0 if2name=p2p0","rtl8821au",0x0811,"","",false,false},\
    {"0000","0000","8812au","/vendor/lib/modules/8812au.ko","ifname=wlan0 if2name=p2p0","rtl8812au",0x881a,"","",false,false},\
    {"c822","0000","8822cs","/vendor/lib/modules/8822cs.ko","ifname=wlan0 if2name=p2p0","rtl8822cs",0x0,"","",true,false}, \
    {"0000","0000","8188fu","/vendor/lib/modules/8188fu.ko","ifname=wlan0 if2name=p2p0","rtl8188ftv",0xf179,"","",false,false},\
    {"0000","0000","8192eu","/vendor/lib/modules/8192eu.ko","ifname=wlan0 if2name=p2p0","rtl8192eu",0x818b,"","",false,false},\
    {"0000","0000","8192fu","/vendor/lib/modules/8192fu.ko","ifname=wlan0 if2name=p2p0","rtl8192fu",0xf192,"","",false,false},\
    {"b822","0000","8822bs","/vendor/lib/modules/8822bs.ko","ifname=wlan0 if2name=p2p0","rtl8822bs",0x0,"","",true,false},\
    {"0000","0000","8733bu","/vendor/lib/modules/8733bu.ko","ifname=wlan0 if2name=p2p0","rtl8733bu",0xb733,"","rtk_btusb",true,true},\
    {"0000","0000","8852au","/vendor/lib/modules/8852au.ko","ifname=wlan0 if2name=p2p0","rtl8852au",0x885c,"","rtk_btusb",true,true},\
    {"0000","0000","8852au","/vendor/lib/modules/8852au.ko","ifname=wlan0 if2name=p2p0","rtl8852au",0x885a,"","rtk_btusb",true,true},\
    {"0000","0000","aic8800_fdrv","/vendor/lib/modules/aic8800_fdrv.ko","","aic8800",0x8800,"","",true,false},\
    {"0701","0000","wlan","/vendor/lib/modules/wlan_9377.ko","","qca9377",0x0,"","",true,false},\
    {"050a","0000","wlan","/vendor/lib/modules/wlan_6174.ko","country_code=CN","qca6174",0x0,"","",true,false},\
    {"0801","0000","wlan","/vendor/lib/modules/wlan_9379.ko","","qca9379",0x0,"","bt_usb_qcom",true,false},\
    {"0000","0000","wlan","/vendor/lib/modules/wlan_9379.ko","","qca9379",0x9378,"","bt_usb_qcom",true,false},\
    {"0000","0000","wlan","/vendor/lib/modules/wlan_9379.ko","","qca9379",0x7a85,"","bt_usb_qcom",true,false},\
    {"7608","0000","wlan_mt76x8_sdio","/vendor/lib/modules/wlan_mt76x8_sdio.ko","sta=wlan ap=wlan p2p=p2p","mtk7668s",0x0,"","btmtksdio",true,true},\
    {"7603","0000","wlan_mt7663_sdio","/vendor/lib/modules/wlan_mt7663_sdio.ko","","mtk7661s",0x0,"","btmtksdio",true,true},\
    {"0000","0000","wlan_mt7663_usb","/vendor/lib/modules/wlan_mt7663_usb.ko","","mtk7663u",0x7663,"","btmtk_usb",true,true},\
    {"0000","0000","bcmdhd","/vendor/lib/modules/bcmdhd.ko","firmware_path=/vendor/etc/wifi/43569/fw_bcm4358u_ag.bin nvram_path=/vendor/etc/wifi/43569/nvram_ap62x8.txt","bcm43569",0xbd27,"/vendor/etc/wifi/43569/fw_bcm4358u_ag","btusb",true,false}, \
    {"0000","0000","bcmdhd","/vendor/lib/modules/bcmdhd.ko","firmware_path=/vendor/etc/wifi/43569/fw_bcm4358u_ag.bin nvram_path=/vendor/etc/wifi/43569/nvram_ap62x8.txt","bcm43569",0x0bdc,"/vendor/etc/wifi/43569/fw_bcm4358u_ag","btusb",true,false}, \
    {"0000","0000","8723bu","/vendor/lib/modules/8723bu.ko","ifname=wlan0 if2name=p2p0","rtl8723bu",0xb720,"","rtk_btusb",true,true}, \
    {"0000","0000","8822bu","/vendor/lib/modules/8822bu.ko","ifname=wlan0 if2name=p2p0","rtl8822bu",0xb82c,"","rtk_btusb",true,true}, \
    {"0000","0000","88x2cu","/vendor/lib/modules/88x2cu.ko","ifname=wlan0 if2name=p2p0","rtl88x2cu",0xc82c,"","rtk_btusb",true,true}, \
    {"0000","0000","8821cu","/vendor/lib/modules/8821cu.ko","ifname=wlan0 if2name=p2p0","rtl8821cu",0xc820,"","rtk_btusb",true,false}, \
    {"0000","0000","8821cu","/vendor/lib/modules/8821cu.ko","ifname=wlan0 if2name=p2p0","rtl8821cu",0xc811,"","rtk_btusb",true,true}, \
    {"3030","0000","ssv_hwif_ctrl","/vendor/lib/modules/ssv_hwif_ctrl.ko","","ssv6051",0x0,"","",false,false}, \
    {"0000","449d","dhdpci","/vendor/lib/modules/dhdpci.ko","firmware_path=/vendor/etc/wifi/43752a2/fw_bcm43752a2_pcie_ag.bin nvram_path=/vendor/etc/wifi/43752a2/nvram_ap6275p.txt","bcm43752a2p",0x0,"/vendor/etc/wifi/43752a2/fw_bcm43752a2_pcie_ag","",true,false},\
    {"0000","4475","dhdpci","/vendor/lib/modules/dhdpci.ko","firmware_path=/vendor/etc/wifi/43752a2/fw_bcm4375b4_pcie_ag.bin nvram_path=/vendor/etc/wifi/43752a2/nvram_ap6275hh3.txt","bcm43752a2p",0x0,"/vendor/etc/wifi/43752a2/fw_bcm4375b4_pcie_ag","",true,false},\
    {"aae8","0000","dhd","/vendor/lib/modules/dhd.ko","firmware_path=/vendor/etc/wifi/43752a2/fw_bcm43752a2_ag.bin nvram_path=/vendor/etc/wifi/43752a2/nvram_ap6275s.txt","bcm43752a2s",0x0,"/vendor/etc/wifi/43752a2/fw_bcm43752a2_ag","",true,false},\
    {"1101","0000","wlan","/vendor/lib/modules/wlan_6391.ko","","qca6391",0x0,"","",true,false},\
    {"1103","0000","wlan","/vendor/lib/modules/wlan_206x.ko","","qca206x",0x0,"","",true,false},\
    {"8852","0000","8852ae","/vendor/lib/modules/8852ae.ko","ifname=wlan0 if2name=p2p0","rtl8852ae",0x0,"","",true,false},\
    {"9141","0000","moal","/vendor/lib/modules/moal.ko","fw_name=nxp/sdiouart8997_combo_v4.bin fw_serial=1 ps_mode=2 auto_ds=2 cfg80211_wext=0xf sta_name=wlan uap_name=wlan wfd_name=p2p cal_data_cfg=none","nxpw8997",0x0,"","",true,true},\
    {"0000","0000","sprdwl_ng","/vendor/lib/modules/sprdwl_ng.ko","","uwe5621ds",0x0,"","sprdbt_tty",true,true},\
    {"0000","0000","ssv6x5x","/vendor/lib/modules/ssv6x5x.ko","tu_stacfgpath=/vendor/etc/wifi/ssv6x5x/ssv6x5x-wifi.cfg","ssv6155",0x6000,"","",false,false}};

static const bt_dongle_info bt_dongle_registerd[]={\
       {"rtk_btusb","/vendor/lib/modules/rtk_btusb.ko","rtl8761u",0xb761}};

int get_wifi_info (dongle_info *ext_info)
{
    int i;
    for (i = 0; i <(int)(sizeof(dongle_registerd)/sizeof(dongle_info)); i++)
    {
        ext_info[i] = dongle_registerd[i];
    }
    return (int)(sizeof(dongle_registerd)/sizeof(dongle_info));
}

int get_bt_info (bt_dongle_info *ext_info)
{
    int i;
    for (i = 0; i <(int)(sizeof(bt_dongle_registerd)/sizeof(bt_dongle_info)); i++)
    {
        ext_info[i] = bt_dongle_registerd[i];
    }
    return (int)(sizeof(bt_dongle_registerd)/sizeof(bt_dongle_info));
}
