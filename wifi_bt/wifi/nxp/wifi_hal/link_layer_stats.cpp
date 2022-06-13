/*
 * Copyright (C) 2017 The Android Open Source Project
 * Portions copyright (C) 2017 Broadcom Limited
 * Portions copyright 2015-2020 NXP
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

#define LOG_TAG  "WifiHAL"

#include <utils/Log.h>

#include "wifi_hal.h"
#include "common.h"
#include "cpp_bindings.h"

typedef enum {
    NXP_ATTR_LL_STATS_INVALID = 0,
    NXP_ATTR_MPDU_SIZE_THRESHOLD,
    NXP_ATTR_AGGRESSIVE_STATS_GATHERING,
    NXP_ATTR_LL_STATS_IFACE,
    NXP_ATTR_LL_STATS_NUM_RADIOS,
    NXP_ATTR_LL_STATS_RADIO,
    NXP_ATTR_LL_STATS_CLEAR_REQ_MASK,
    NXP_ATTR_LL_STATS_STOP_REQ,
    NXP_ATTR_LL_STATS_CLEAR_RSP_MASK,
    NXP_ATTR_LL_STATS_STOP_RSP,

    /* keep last */
    NXP_ATTR_LL_STATS_AFTER_LAST,
    NXP_ATTR_LL_STATS_MAX = 
    NXP_ATTR_LL_STATS_AFTER_LAST - 1

} LINK_LAYER_ATTRIBUTE;

/* radio statistics */
typedef struct {
   wifi_radio radio;                      // wifi radio (if multiple radio supported)
   u32 on_time;                           // msecs the radio is awake (32 bits number accruing over time)
   u32 tx_time;                           // msecs the radio is transmitting (32 bits number accruing over time)
   u32 num_tx_levels;                     // number of radio transmit power levels
   //u32 *tx_time_per_levels;               // pointer to an array of radio transmit per power levels in
                                          // msecs accured over time
   u32 rx_time;                           // msecs the radio is in active receive (32 bits number accruing over time)
   u32 on_time_scan;                      // msecs the radio is awake due to all scan (32 bits number accruing over time)
   u32 on_time_nbd;                       // msecs the radio is awake due to NAN (32 bits number accruing over time)
   u32 on_time_gscan;                     // msecs the radio is awake due to G?scan (32 bits number accruing over time)
   u32 on_time_roam_scan;                 // msecs the radio is awake due to roam?scan (32 bits number accruing over time)
   u32 on_time_pno_scan;                  // msecs the radio is awake due to PNO scan (32 bits number accruing over time)
   u32 on_time_hs20;                      // msecs the radio is awake due to HS2.0 scans and GAS exchange (32 bits number accruing over time)
   u32 num_channels;                      // number of channels
   wifi_channel_stat channels[];          // channel statistics
} wifi_radio_stat_nxp;

/* interface statistics */
typedef struct {
   //wifi_interface_handle iface;          // wifi interface
   wifi_interface_link_layer_info info;  // current state of the interface
   u32 beacon_rx;                        // access point beacon received count from connected AP
   u64 average_tsf_offset;               // average beacon offset encountered (beacon_TSF - TBTT)
                                         // The average_tsf_offset field is used so as to calculate the
                                         // typical beacon contention time on the channel as well may be
                                         // used to debug beacon synchronization and related power consumption issue
   u32 leaky_ap_detected;                // indicate that this AP typically leaks packets beyond the driver guard time.
   u32 leaky_ap_avg_num_frames_leaked;  // average number of frame leaked by AP after frame with PM bit set was ACK'ed by AP
   u32 leaky_ap_guard_time;              // guard time currently in force (when implementing IEEE power management based on
                                         // frame control PM bit), How long driver waits before shutting down the radio and
                                         // after receiving an ACK for a data frame with PM bit set)
   u32 mgmt_rx;                          // access point mgmt frames received count from connected AP (including Beacon)
   u32 mgmt_action_rx;                   // action frames received count
   u32 mgmt_action_tx;                   // action frames transmit count
   wifi_rssi rssi_mgmt;                  // access Point Beacon and Management frames RSSI (averaged)
   wifi_rssi rssi_data;                  // access Point Data Frames RSSI (averaged) from connected AP
   wifi_rssi rssi_ack;                   // access Point ACK RSSI (averaged) from connected AP
   wifi_wmm_ac_stat ac[WIFI_AC_MAX];     // per ac data packet statistics
   u32 num_peers;                        // number of peers
   wifi_peer_info peer_info[];           // per peer statistics
} wifi_iface_stat_nxp;

class GetLinkStats : public WifiCommand
{
private:
    wifi_stats_result_handler Handler;

public:
    GetLinkStats(wifi_interface_handle iface, wifi_stats_result_handler handler)
    : WifiCommand("GetLinkStats", iface, 0)
    {
        Handler = handler;
    }

    virtual int create() {
        int ret = 0;
        ret = mMsg.create(MARVELL_OUI, NXP_SUBCMD_LL_STATS_GET);
        if (ret < 0) {
            ALOGE("Failed to create message to get link layer stats - %d", ret);
            return ret;
        }
        return WIFI_SUCCESS;
    }

    virtual int handleResponse(WifiEvent& reply) {
        struct nlattr *tb_vendor[NXP_ATTR_LL_STATS_MAX + 1];
        struct nlattr *vendor_data = NULL;
        u32 num_radios = 0;
        wifi_iface_stat *iface_stat = NULL;
        wifi_radio_stat *radio_stat = NULL;
        int id = 0, len = 0;
        u32 i = 0;
        wifi_iface_stat_nxp *iface_stat_nxp = NULL;
        u32 iface_len = 0;
        wifi_radio_stat *radio_stat_tmp = NULL;
        u32 radio_stat_len = 0;
        wifi_radio_stat_nxp *radio_stat_nxp = NULL;
        wifi_radio_stat_nxp *radio_stat_nxp_tmp = NULL;
        u32 each_len = 0;
        u32 total_len = 0;

        if (reply.get_cmd() != NL80211_CMD_VENDOR) {
            ALOGE("Ignoring reply with cmd = %d", reply.get_cmd());
            return NL_SKIP;
        }
        vendor_data = (nlattr *)(reply.get_data(NL80211_ATTR_VENDOR_DATA));
        len = reply.get_vendor_data_len();
        if (vendor_data == NULL || len == 0) {
            ALOGE("Error! No data in command response\n");
            return NL_SKIP;
        }
        id = reply.get_vendor_id();
        nla_parse(tb_vendor, NXP_ATTR_LL_STATS_MAX, vendor_data, len, NULL);

        /** Process IFACE */
        if(!tb_vendor[NXP_ATTR_LL_STATS_IFACE]){
            ALOGE("NXP_ATTR_LL_STATS_IFACE not found\n");
            return NL_SKIP;
        }
        iface_stat_nxp = (wifi_iface_stat_nxp *)nla_data(tb_vendor[NXP_ATTR_LL_STATS_IFACE]);
        iface_len = nla_len(tb_vendor[NXP_ATTR_LL_STATS_IFACE]) + sizeof(wifi_interface_handle);
        iface_stat = (wifi_iface_stat *)malloc(iface_len);
        if (iface_stat == NULL) {
            ALOGE("Could not allocate iface_stat");
            return NL_SKIP;
        }
        memset(iface_stat, 0, iface_len);
        iface_stat->iface = NULL;
        memcpy((u8 *)iface_stat + sizeof(wifi_interface_handle), iface_stat_nxp, nla_len(tb_vendor[NXP_ATTR_LL_STATS_IFACE]));

        /** Process Num of RADIO */
        if(!tb_vendor[NXP_ATTR_LL_STATS_NUM_RADIOS]){
            ALOGE("NXP_ATTR_LL_STATS_NUM_RADIOS not found\n");
            free(iface_stat);
            return NL_SKIP;
        }
        num_radios = nla_get_u32(tb_vendor[NXP_ATTR_LL_STATS_NUM_RADIOS]);

        /** Process RADIO */
        if(!tb_vendor[NXP_ATTR_LL_STATS_RADIO]){
            ALOGE("NXP_ATTR_LL_STATS_RADIO not found\n");
            free(iface_stat);
            return NL_SKIP;
        }
        radio_stat_nxp = (wifi_radio_stat_nxp *)nla_data(tb_vendor[NXP_ATTR_LL_STATS_RADIO]);
        /* Check len and Count total len need to malloc */
        radio_stat_nxp_tmp = radio_stat_nxp;
        for (i=0; i<num_radios; i++) {
            each_len = sizeof(wifi_radio_stat_nxp) + radio_stat_nxp_tmp->num_channels * sizeof(wifi_channel_stat);
            total_len += each_len;
            radio_stat_len += sizeof(wifi_radio_stat) + radio_stat_nxp_tmp->num_channels * sizeof(wifi_channel_stat);
            radio_stat_nxp_tmp = (wifi_radio_stat_nxp *)((u8 *)radio_stat_nxp_tmp + each_len);
        }
        if (!radio_stat_len || !total_len || total_len != (u32)nla_len(tb_vendor[NXP_ATTR_LL_STATS_RADIO])) {
            ALOGE("NXP_ATTR_LL_STATS_RADIO len not right: radio_stat_len=%d total_len=%d attrlen=%d\n",
                radio_stat_len, total_len, nla_len(tb_vendor[NXP_ATTR_LL_STATS_RADIO]));
            free(iface_stat);
            return NL_SKIP;
        }
        /* malloc radio stat and copy the data from driver to HAL */
        radio_stat = (wifi_radio_stat *)malloc(radio_stat_len);
        if (radio_stat == NULL) {
            ALOGE("Could not allocate radio_stat");
            free(iface_stat);
            return NL_SKIP;
        }
        memset(radio_stat, 0, radio_stat_len);
        radio_stat_tmp= radio_stat;
        radio_stat_nxp_tmp = radio_stat_nxp;
        for (i=0; i<num_radios; i++) {
            radio_stat_tmp->radio = radio_stat_nxp_tmp->radio;
            radio_stat_tmp->on_time = radio_stat_nxp_tmp->on_time;
            radio_stat_tmp->tx_time = radio_stat_nxp_tmp->tx_time;
            radio_stat_tmp->num_tx_levels = radio_stat_nxp_tmp->num_tx_levels;
            radio_stat_tmp->tx_time_per_levels = NULL;
            radio_stat_tmp->rx_time = radio_stat_nxp_tmp->rx_time;
            radio_stat_tmp->on_time_scan = radio_stat_nxp_tmp->on_time_scan;
            radio_stat_tmp->on_time_nbd = radio_stat_nxp_tmp->on_time_nbd;
            radio_stat_tmp->on_time_gscan = radio_stat_nxp_tmp->on_time_gscan;
            radio_stat_tmp->on_time_roam_scan = radio_stat_nxp_tmp->on_time_roam_scan;
            radio_stat_tmp->on_time_pno_scan = radio_stat_nxp_tmp->on_time_pno_scan;
            radio_stat_tmp->on_time_hs20 = radio_stat_nxp_tmp->on_time_hs20;
            radio_stat_tmp->num_channels = radio_stat_nxp_tmp->num_channels;
            memcpy(radio_stat_tmp->channels, radio_stat_nxp_tmp->channels, radio_stat_nxp_tmp->num_channels * sizeof(wifi_channel_stat));
            radio_stat_tmp = (wifi_radio_stat *)((u8 *)radio_stat_tmp + sizeof(wifi_radio_stat) + radio_stat_nxp_tmp->num_channels * sizeof(wifi_channel_stat));
            radio_stat_nxp_tmp = (wifi_radio_stat_nxp *)((u8 *)radio_stat_nxp_tmp +sizeof(wifi_radio_stat_nxp) + radio_stat_nxp_tmp->num_channels * sizeof(wifi_channel_stat));
        }

        if (Handler.on_link_stats_results) {
            (Handler.on_link_stats_results)(id, iface_stat, num_radios, radio_stat);
        } else {
            ALOGW("No linklayer stats handler registered");
        }
        free(iface_stat);
        free(radio_stat);
        return NL_OK;
    }
};

class SetLinkStats : public WifiCommand
{
private:
    wifi_link_layer_params Params;

public:
    SetLinkStats(wifi_interface_handle iface, wifi_link_layer_params params)
    :WifiCommand("SetLinkStats", iface, 0)
    {
        Params = params;
    }
    virtual int create() {
        int ret = 0;
        ret = mMsg.create(MARVELL_OUI, NXP_SUBCMD_LL_STATS_SET);
        if (ret < 0) {
            ALOGE("Failed to create message to set link layer stats - %d", ret);
            return ret;
        }
        nlattr *data = mMsg.attr_start(NL80211_ATTR_VENDOR_DATA);
        ret = mMsg.put_u32(NXP_ATTR_MPDU_SIZE_THRESHOLD, Params.mpdu_size_threshold);
        if (ret < 0)
            return ret;
        ret = mMsg.put_u32(NXP_ATTR_AGGRESSIVE_STATS_GATHERING,Params.aggressive_statistics_gathering);
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

wifi_error wifi_get_link_stats(wifi_request_id id, wifi_interface_handle iface, 
                                  wifi_stats_result_handler handler)
{
    int ret = 0;
    GetLinkStats GetStats(iface, handler);
    ret = GetStats.requestResponse();
    return (wifi_error) ret;
}

wifi_error wifi_set_link_stats(wifi_interface_handle iface, wifi_link_layer_params params)
{
    int ret = 0;
    SetLinkStats SetStats(iface, params);
    ret = SetStats.requestResponse();
    return (wifi_error)ret;
}

class ClearLinkStats : public WifiCommand
{
private:
    u32 clearReqMask;
    u32 *clearRspMask;
    u8 stopReq;
    u8 *stopRsp;

public:
    ClearLinkStats (wifi_interface_handle iface, u32 stats_clear_req_mask,
                        u32 *stats_clear_rsp_mask, u8 stop_req, u8 *stop_rsp)
    : WifiCommand("ClearLinkStats", iface, 0)
    {
        clearReqMask = stats_clear_req_mask;
        clearRspMask = stats_clear_rsp_mask;
        stopReq = stop_req;
        stopRsp = stop_rsp;
    }

    virtual int create() {
        int ret = 0;
        ret = mMsg.create(MARVELL_OUI, NXP_SUBCMD_LL_STATS_CLR);
        if(ret < 0){
            ALOGE("Failed to create message to clear link stats - %d", ret);
            return ret;
        }
        nlattr *data = mMsg.attr_start(NL80211_ATTR_VENDOR_DATA);
        ret = mMsg.put_u32(NXP_ATTR_LL_STATS_CLEAR_REQ_MASK, clearReqMask);
        if (ret < 0)
            return ret;
        ret = mMsg.put_u8(NXP_ATTR_LL_STATS_STOP_REQ, stopReq);
        if (ret < 0)
            return ret;
        mMsg.attr_end(data);
        return WIFI_SUCCESS;
    }

    virtual int handleResponse(WifiEvent& reply){
        if (reply.get_cmd() != NL80211_CMD_VENDOR) {
            ALOGE("Ignoring reply with cmd = %d", reply.get_cmd());
            return NL_SKIP;
        }
        nlattr *vendor_data =(nlattr *)(reply.get_data(NL80211_ATTR_VENDOR_DATA));
        int len = reply.get_vendor_data_len();
        if (vendor_data == NULL || len == 0) {
            ALOGE("Error! No data in command response");
            return NL_SKIP;
        }
        struct nlattr *tb_vendor[NXP_ATTR_LL_STATS_MAX + 1];
        nla_parse(tb_vendor, NXP_ATTR_LL_STATS_MAX, vendor_data, len, NULL);
        if(!tb_vendor[NXP_ATTR_LL_STATS_CLEAR_RSP_MASK]){
            ALOGE("NXP_ATTR_LL_STATS_CLEAR_RSP_MASK not found\n");
            return NL_SKIP;
        }
        clearRspMask = (u32 *)nla_data(tb_vendor[NXP_ATTR_LL_STATS_CLEAR_RSP_MASK]);
        if(!tb_vendor[NXP_ATTR_LL_STATS_STOP_RSP]){
            ALOGE("NXP_ATTR_LL_STATS_STOP_RSP not found\n");
            return NL_SKIP;
        }
        stopRsp = (u8 *)nla_data(tb_vendor[NXP_ATTR_LL_STATS_STOP_RSP]);
        return NL_OK;
    }
};

wifi_error wifi_clear_link_stats(wifi_interface_handle iface, u32 stats_clear_req_mask, 
                                     u32 *stats_clear_rsp_mask, u8 stop_req, u8 *stop_rsp)
{
    int ret = 0;
    ClearLinkStats ClearStats(iface, stats_clear_req_mask, stats_clear_rsp_mask, 
                              stop_req, stop_rsp);
    ret = ClearStats.requestResponse();
    return (wifi_error) ret;
}
