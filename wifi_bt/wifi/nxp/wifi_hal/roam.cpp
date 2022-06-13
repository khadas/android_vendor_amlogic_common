/*
* Copyright 2017-2020 NXP
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
* limitations under the License.
*/
#include <stdint.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <linux/rtnetlink.h>
#include <netpacket/packet.h>
#include <linux/filter.h>
#include <linux/errqueue.h>

#include <linux/pkt_sched.h>
#include <netlink/object-api.h>
#include <netlink/netlink.h>
#include <netlink/socket.h>
//#include <netlink/handlers.h>

#define LOG_TAG  "WifiHAL"

#include <utils/Log.h>
#include <utils/String8.h>

#include "wifi_hal.h"
#include "common.h"
#include "cpp_bindings.h"

using namespace android;

typedef enum {
    NXP_ATTR_ROAMING_INVALID = 0,
    NXP_ATTR_ROAMING_CAPA,
    NXP_ATTR_FW_ROAMING_CONTROL,
    NXP_ATTR_ROAMING_CONFIG_BSSID,
    NXP_ATTR_ROAMING_CONFIG_SSID,

    /* keep last */
    NXP_ATTR_ROAMING_AFTER_LAST,
    NXP_ATTR_ROAMING_MAX =
    NXP_ATTR_ROAMING_AFTER_LAST - 1

} ROAMING_ATTRIBUTE;

/* BSSID blacklist */
typedef struct {
    int num_bssid;                           // number of blacklisted BSSIDs
    mac_addr bssids[MAX_BLACKLIST_BSSID];    // blacklisted BSSIDs
} wifi_bssid_params;

typedef struct {
    u32 num_ssid;
    ssid_t *ssids;
} wifi_ssid_params;

class GetRoamingCapa : public WifiCommand
{
private:
    wifi_roaming_capabilities *Capa;

public:
    GetRoamingCapa(wifi_interface_handle handle, wifi_roaming_capabilities *caps)
    : WifiCommand("GetRoamingCapa", handle, 0)
    {
        Capa = caps;
        memset(Capa, 0, sizeof(wifi_roaming_capabilities));
    }

    virtual int create(){
        int ret = 0;
        ret = mMsg.create(MARVELL_OUI, NXP_SUBCMD_GET_ROAMING_CAPA);
        if (ret < 0) {
            ALOGE("Failed to create message to get roaming capabilities\n");
            return ret;
        }
        return WIFI_SUCCESS;
    }

    virtual int handleResponse(WifiEvent& reply){
        if (reply.get_cmd() != NL80211_CMD_VENDOR) {
            ALOGE("Ignoring reply with cmd = %d", reply.get_cmd());
        return NL_SKIP;
        }

        nlattr *vendor_data = (nlattr *)(reply.get_data(NL80211_ATTR_VENDOR_DATA));
        int len = reply.get_vendor_data_len();
        if (vendor_data == NULL || len == 0) {
            ALOGE("Error! No data in command response");
            return NL_SKIP;
        }

        struct nlattr *tb_vendor[NXP_ATTR_ROAMING_MAX];
        nla_parse(tb_vendor, NXP_ATTR_ROAMING_MAX - 1, vendor_data, len, NULL);
        if(!tb_vendor[NXP_ATTR_ROAMING_CAPA]){
            ALOGE("NXP_ATTR_ROAMING_CAPA not found\n");
            return NL_SKIP;
        }
        memcpy(Capa, (wifi_roaming_capabilities *)nla_data(tb_vendor[NXP_ATTR_ROAMING_CAPA]), 
               sizeof(wifi_roaming_capabilities));
        return NL_OK;
    }
};

wifi_error wifi_get_roaming_capabilities(wifi_interface_handle handle, 
                                                wifi_roaming_capabilities *caps)
{
    int ret = 0;
    GetRoamingCapa Capa(handle, caps);
    ret = Capa.requestResponse();
    return (wifi_error) ret;
}

class EnableFirmwareRoaming : public WifiCommand
{
private:
    fw_roaming_state_t roamingState;

public:
    EnableFirmwareRoaming(wifi_interface_handle handle, fw_roaming_state_t state)
    : WifiCommand("EnableFirmwareRoaming", handle, 0)
    {
        roamingState = state;
    }

    virtual int create(){
        int ret = 0;
        ret = mMsg.create(MARVELL_OUI, NXP_SUBCMD_ENABLE_FW_ROAMING);
        if (ret < 0) {
            ALOGE("Failed to create message to enable firmware roaming\n");
            return ret;
        }

        nlattr *data = mMsg.attr_start(NL80211_ATTR_VENDOR_DATA);
        ret = mMsg.put_u32(NXP_ATTR_FW_ROAMING_CONTROL, roamingState);
        if (ret < 0)
            return ret;
        mMsg.attr_end(data);
        return WIFI_SUCCESS;
    }

    virtual int handleResponse(WifiEvent& reply) {
        /* Nothing to do on response! */
        return NL_SKIP;
    }
};

wifi_error wifi_enable_firmware_roaming(wifi_interface_handle handle, 
                                                 fw_roaming_state_t state)
{
    int ret = 0;
    EnableFirmwareRoaming Roaming(handle, state);
    ret = Roaming.requestResponse();
    return (wifi_error) ret;
}

class ConfigureRoaming : public WifiCommand
{
private:
    wifi_roaming_config *roamingConfig;
    wifi_bssid_params Bssid;
    wifi_ssid_params Ssid;

public:
    ConfigureRoaming(wifi_interface_handle handle, wifi_roaming_config *roaming_config)
    : WifiCommand("ConfigureRoaming", handle, 0)
    {
        roamingConfig = roaming_config;
        Bssid.num_bssid = roamingConfig->num_blacklist_bssid;
        memcpy(Bssid.bssids, roamingConfig->blacklist_bssid, (Bssid.num_bssid * sizeof(mac_addr)));
        Ssid.num_ssid = roamingConfig->num_whitelist_ssid;
        Ssid.ssids = roamingConfig->whitelist_ssid;
   }

    virtual int create(){
        int ret = 0;
        ret = mMsg.create(MARVELL_OUI, NXP_SUBCMD_ROAMING_CONFIG);
        if (ret < 0) {
            ALOGE("Failed to create message to configure roaming\n");
            return ret;
        }

        nlattr *data = mMsg.attr_start(NL80211_ATTR_VENDOR_DATA);
        ret = mMsg.put(NXP_ATTR_ROAMING_CONFIG_BSSID,(void *)(&Bssid), sizeof(wifi_bssid_params));
        if (ret < 0)
            return ret;
        ret = mMsg.put(NXP_ATTR_ROAMING_CONFIG_SSID, (void *)(&Ssid), sizeof(wifi_ssid_params));
        if(ret < 0)
            return ret;
        mMsg.attr_end(data);
        return WIFI_SUCCESS;
    }

    virtual int handleResponse(WifiEvent& reply){
        /* Nothing to do on response! */
        return NL_SKIP;
    }
};

wifi_error wifi_configure_roaming(wifi_interface_handle handle, 
                                        wifi_roaming_config *roaming_config)
{
    int ret = 0;
    ConfigureRoaming Config(handle, roaming_config );
    ret = Config.requestResponse();
    return (wifi_error) ret;
}

