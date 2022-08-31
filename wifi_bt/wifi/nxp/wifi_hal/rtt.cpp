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

#include "nl80211_copy.h"

#define LOG_TAG  "WifiHAL"

#include <utils/Log.h>
#include <utils/String8.h>

#include "wifi_hal.h"
#include "common.h"
#include "cpp_bindings.h"

using namespace android;
#define RTT_RESULT_SIZE (sizeof(wifi_rtt_result))

typedef enum {
    NXP_ATTR_RTT_INVALID = 0,
    NXP_ATTR_RTT_CAPA,
    NXP_ATTR_RTT_NUM_CONFIG,
    NXP_ATTR_RTT_CONFIG,
    NXP_ATTR_RTT_DEVICE_ADDR,
    NXP_ATTR_RTT_RESULT_COMPLETE,
    NXP_ATTR_RTT_RESULT_NUM,
    NXP_ATTR_RTT_RESULT_FULL,
    NXP_ATTR_RTT_CHANNEL_INFO,
    NXP_ATTR_RTT_MAX_DUR_SEC,
    NXP_ATTR_RTT_PREAMBLE,
    NXP_ATTR_RTT_LCI_INFO,
    NXP_ATTR_RTT_LCR_INFO,

    /* keep last */
    NXP_ATTR_RTT_AFTER_LAST,
    NXP_ATTR_RTT_MAX =
    NXP_ATTR_RTT_AFTER_LAST
} RTT_ATTRIBUTE;

typedef enum {
    RTT_SET_LCI,
    RTT_SET_LCR,
} ConfigureType;

#define DOT11_MEASURE_TYPE_LCI          8   /* d11 measurement LCI type */
#define DOT11_MEASURE_TYPE_CIVICLOC     11  /* d11 measurement location civic */

class GetRttCapa :public WifiCommand
{
private:
    wifi_rtt_capabilities *Capa;

public:
    GetRttCapa(wifi_interface_handle iface, wifi_rtt_capabilities *capabilities)
        : WifiCommand("GetRttCapa", iface, 0)
    {
        Capa = capabilities;
    }

    virtual int create() {
        int ret = 0;
        ret = mMsg.create(MARVELL_OUI, NXP_SUBCMD_RTT_GET_CAPA);
        if (ret < 0) {
            ALOGE("Failed to create message to get RTT capabilities - %d", ret);
            return ret;
        }
        return WIFI_SUCCESS;
    }

    virtual int handleResponse(WifiEvent& reply) {
        if (reply.get_cmd() != NL80211_CMD_VENDOR) {
            ALOGE("Ignoring reply with cmd = %d\n", reply.get_cmd());
            return NL_SKIP;
        }

        nlattr *tb = (nlattr*)(reply.get_data(NL80211_ATTR_VENDOR_DATA));
        int len = reply.get_vendor_data_len();
        if(tb == NULL || len == 0){
            ALOGE("ERROR! No data in command response\n");
            return NL_SKIP;
        }
        
        struct nlattr *tb_vendor[NXP_ATTR_RTT_MAX];
        nla_parse(tb_vendor, (NXP_ATTR_RTT_MAX - 1) ,tb, len, NULL);
        if (!tb_vendor[NXP_ATTR_RTT_CAPA]){
            ALOGE("NXP_ATTR_RTT_CAPA not found");
            return WIFI_ERROR_INVALID_ARGS;
        }
        memcpy(Capa, (wifi_rtt_capabilities *)nla_data(tb_vendor[NXP_ATTR_RTT_CAPA]),
                sizeof(wifi_rtt_capabilities));
        return NL_OK;
    }
};


class RttRangeControl : public WifiCommand
{
private:
    unsigned numConfig;
    int mCompleted;
    int Index;
    int totalCnt;
    static const int MAX_RESULTS = 1024;
    wifi_rtt_result *rttResults[MAX_RESULTS];
    wifi_rtt_config *rttConfig;
    wifi_rtt_event_handler Handler;

public:
    RttRangeControl(wifi_interface_handle iface, int id, unsigned num_rtt_config,
                         wifi_rtt_config rtt_config[], wifi_rtt_event_handler handler)
    : WifiCommand("RttRangeControl", iface, id)
    {
        numConfig = num_rtt_config;
        rttConfig = rtt_config;
        Handler = handler;
        memset(rttResults, 0, sizeof(wifi_rtt_result*) * MAX_RESULTS);
        Index = 0;
        mCompleted = 0;
        totalCnt = 0;
    }

    RttRangeControl(wifi_interface_handle iface, int id)
    : WifiCommand("RttRangeControl", iface, id)
    {
        Index = 0;
        mCompleted = 0;
        totalCnt = 0;
        numConfig = 0;
    }

    int createSetupRequest(WifiRequest& request) {
        int result = 0;
        result = request.create(MARVELL_OUI, NXP_SUBCMD_RTT_RANGE_REQUEST);
        if (result < 0) {
            ALOGE("Failed to create message to request RTT range - %d", result);
            return result;
        }

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
        result = request.put_u8(NXP_ATTR_RTT_NUM_CONFIG, numConfig);
        if (result < 0) {
            ALOGE("Failed to put num of rtt params\n");
            return result;
        }

        unsigned len = numConfig * sizeof(wifi_rtt_config);
        result = request.put(NXP_ATTR_RTT_CONFIG, rttConfig, len);
        if(result < 0) {
            ALOGE("Failed to put rtt params\n");
            return result;
        }
        request.attr_end(data);
        return WIFI_SUCCESS;
    }

    int createTeardownRequest(WifiRequest& request, unsigned num_devices, mac_addr *addr) {
        int result = 0;
        result = request.create(MARVELL_OUI, NXP_SUBCMD_RTT_RANGE_CANCEL);
        if (result < 0) {
            ALOGE("Failed to create message to cancel RTT range");
            return result;
        }

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
        result = request.put_u8(NXP_ATTR_RTT_NUM_CONFIG, num_devices);
        if(result < 0)
            return result;
        result = request.put_addr(NXP_ATTR_RTT_DEVICE_ADDR, *addr);
        if(result < 0)
            return result;
        request.attr_end(data);
        return WIFI_SUCCESS;
    }

    int start() {
        WifiRequest request(familyId(), ifaceId());
        int result = createSetupRequest(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("failed to create setup request; result = %d", result);
            return result;
        }

        result = requestResponse(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("failed to configure RTT setup; result = %d", result);
            return result;
        }

        result = registerVendorHandler(MARVELL_OUI, NXP_EVENT_RTT_RANGE_RESULT);
        if (result < 0) {
            ALOGE("Failed to register handler");
            unregisterVendorHandler(MARVELL_OUI, NXP_EVENT_RTT_RANGE_RESULT);
            return result;
        }
        return WIFI_SUCCESS;
    }

    int cancel_specific(unsigned num_devices, mac_addr addr[]) {
        WifiRequest request(familyId(), ifaceId());
        int result = createTeardownRequest(request, num_devices, addr);
        if (result != WIFI_SUCCESS) {
            ALOGE("failed to create stop request; result = %d", result);
            return result;
        } else {
            result = requestResponse(request);
            if (result != WIFI_SUCCESS) {
                ALOGE("failed to stop RTT; result = %d", result);
                return result;
            }
        }
        unregisterVendorHandler(MARVELL_OUI, NXP_EVENT_RTT_RANGE_RESULT);
        return WIFI_SUCCESS;
    }

    virtual int handleResponse(WifiEvent& reply) {
        /* Nothing to do on response! */
        return NL_SKIP;
    }

    virtual int handleEvent(WifiEvent& event) {
        u8 *data = NULL;
        u8 *pHead = NULL;
        u8 *pEnd = NULL;
        int result_cnt = 0;

        nlattr *vendor_data = (nlattr*)(event.get_attribute(NL80211_ATTR_VENDOR_DATA));
        int len = event.get_vendor_data_len();
        if (vendor_data == NULL || len == 0) {
            ALOGE("Error! No rtt results found");
            return NL_SKIP;
        }

        for (nl_iterator it(vendor_data); it.has_next(); it.next()) {
            if (it.get_type() == NXP_ATTR_RTT_RESULT_COMPLETE) {
                /* 1: result event is complete  0: resultes event is not complete */
                mCompleted = it.get_u32();
                ALOGV("result event is %d\n", mCompleted);
                ALOGV("retrieved completed flag : %d\n", mCompleted);
            } else if (it.get_type() == NXP_ATTR_RTT_RESULT_NUM) {
                result_cnt = it.get_u32();
                ALOGV("number of result is %d\n", result_cnt);
                ALOGV("Number of RTT results: %d\n", result_cnt);
            } else if(it.get_type() == NXP_ATTR_RTT_RESULT_FULL) {
                unsigned long result_len = 0;

                result_len = it.get_len();
                data = (u8 *)(it.get_data());
                pHead = data;
                pEnd = data;
                pEnd += result_len;
                if(result_len < RTT_RESULT_SIZE){
                    ALOGE("Event buffer size is too small\n");
                    return NL_SKIP;
                } else {
                    rttResults[Index] = (wifi_rtt_result *)malloc(RTT_RESULT_SIZE);
                    memset(rttResults[Index], 0, RTT_RESULT_SIZE);
                    memcpy(rttResults[Index], data, RTT_RESULT_SIZE);
                    rttResults[Index]->LCI = NULL;
                    rttResults[Index]->LCR = NULL;
                    data += RTT_RESULT_SIZE;
                    while(data < pEnd){
                        u8 *type = data;
                        u8 *element_len = data;
                        element_len += 1;
                        type += 4;
                        switch(*type){
                            case DOT11_MEASURE_TYPE_LCI:
                            {
                                rttResults[Index]->LCI = (wifi_information_element *)data;
                                data += (*element_len + 2);
                                break;
                            }
                            case DOT11_MEASURE_TYPE_CIVICLOC:
                            {
                                rttResults[Index]->LCR = (wifi_information_element *)data;
                                data += (*element_len + 2);
                                break;
                            }
                            default:
                            {
                                ALOGE("Unknow attribute data\n");
                                return NL_SKIP;
                            }
                        }
                    }
                }
                Index ++;
            }
        }
        totalCnt = Index;
        if (mCompleted) {
            unregisterVendorHandler(MARVELL_OUI, NXP_EVENT_RTT_RANGE_RESULT);
            if(Handler.on_rtt_results){
                 (Handler.on_rtt_results)(id(), totalCnt, rttResults);
            }
            for(int n = 0; n < totalCnt; n++){
                 ALOGV("Addr %02x:%02x:%02x:%02x:%02x:%02x\n", 
                         rttResults[n]->addr[0], rttResults[n]->addr[1], rttResults[n]->addr[2], 
                         rttResults[n]->addr[3], rttResults[n]->addr[4], rttResults[n]->addr[5]);
                 ALOGV("burst_num = %02x\tmeasurement_num = %02x\nsuccess_num = %02x\tnumber_per_burst_peer = %02x\n",
                         rttResults[n]->burst_num, rttResults[n]->measurement_number, 
                         rttResults[n]->success_number, rttResults[n]->number_per_burst_peer);
                 ALOGV("status = %02x\tretry_after_duration = %02x\ntype = %02x\trssi = %02x\n",
                         rttResults[n]->status, rttResults[n]->retry_after_duration, rttResults[n]->type,
                         rttResults[n]->rssi);
//                 ALOGV("rssi_spread = %02x\trtt = %ld\nrtt_sd = %lld\trtt_spread = %lld\n", 
//                         rttResults[n]->rssi_spread, rttResults[n]->rtt, rttResults[n]->rtt_sd, 
//                         rttResults[n]->rtt_spread);
//                 ALOGV("distance_mm = %02x\tdistance_sd_mm = %02x\ndistance_spread_mm = %02x\tts = %lld\n",
//                         rttResults[n]->distance_mm, rttResults[n]->distance_sd_mm, 
//                         rttResults[n]->distance_spread_mm, rttResults[n]->ts);
                 ALOGV("burst_duration = %02x\tnegotiated_burst_num = %02x\n", 
                         rttResults[n]->burst_duration, rttResults[n]->negotiated_burst_num);
                 if(rttResults[n]->LCI){
                     ALOGV("lci_id = %d\tlci_len = %d\n", rttResults[n]->LCI->id, rttResults[n]->LCI->len);
                     ALOGV("data :\n");
                     for(int lci_index = 0; lci_index < rttResults[n]->LCI->len; lci_index ++){
                         ALOGV("%02x ", rttResults[n]->LCI->data[lci_index]);
                     }
                     ALOGV("\n");
                 }
                 if(rttResults[n]->LCR){
                     ALOGV("lcr_id = %d\tlcr_len = %d\n", rttResults[n]->LCR->id, rttResults[n]->LCR->len);
                     for (int lcr_index = 0; lcr_index < rttResults[n]->LCR->len; lcr_index ++){
                         ALOGV("%02x ", rttResults[n]->LCR->data[lcr_index]);
                     }
                     ALOGV("\n");
                 }
            }
            for (int m = 0; m < totalCnt; m++) {
                free(rttResults[m]);
                rttResults[m] = NULL;
            }
            totalCnt = Index = 0;
            WifiCommand *cmd = wifi_unregister_cmd(wifiHandle(), id());
            if (cmd)
                cmd->releaseRef();
        }
        return NL_SKIP;
    }
};

class GetRttResponderInfo :public WifiCommand
{
private:
    wifi_rtt_responder *responderInfo;

public:
    GetRttResponderInfo (wifi_interface_handle iface, wifi_rtt_responder *responder_info)
    : WifiCommand("GetRttResponderInfo", iface, 0)
    {
        responderInfo = responder_info;
        memset(responderInfo, 0, sizeof(wifi_rtt_responder));
    }
    virtual int create() {
        int ret = 0;
        ret = mMsg.create(MARVELL_OUI, NXP_SUBCMD_RTT_GET_RESPONDER_INFO);
        if (ret < 0) {
            ALOGE("Failed to create message to get responder info\n");
            return ret;
        }
        return WIFI_SUCCESS;
    }

    virtual int handleResponse(WifiEvent& reply) {
        if (reply.get_cmd() != NL80211_CMD_VENDOR) {
            ALOGE("Ignoring reply with cmd = %d", reply.get_cmd());
            return NL_SKIP;
        }
        nlattr *tb = (nlattr*)(reply.get_data(NL80211_ATTR_VENDOR_DATA));
        int len = reply.get_vendor_data_len();
        if(tb == NULL || len == 0){
            ALOGE("ERROR\n");
            return NL_SKIP;
        }
        struct nlattr *tb_vendor[NXP_ATTR_RTT_MAX];
        nla_parse(tb_vendor, (NXP_ATTR_RTT_MAX - 1), tb, len, NULL);
        if (!tb_vendor[NXP_ATTR_RTT_CHANNEL_INFO]){
            ALOGE("NXP_ATTR_RTT_CHANNEL_INFO not found");
            return WIFI_ERROR_INVALID_ARGS;
        }
        memcpy(&(responderInfo->channel),(wifi_channel_info *)nla_data(tb_vendor[NXP_ATTR_RTT_CHANNEL_INFO]), 
               sizeof(wifi_channel_info));
        if (!tb_vendor[NXP_ATTR_RTT_PREAMBLE]){
            ALOGE("NXP_ATTR_RTT_PREAMBLE not found");
            return WIFI_ERROR_INVALID_ARGS;
        }
        memcpy(&(responderInfo->preamble), (wifi_rtt_preamble *)nla_data(tb_vendor[NXP_ATTR_RTT_PREAMBLE]),
                sizeof(wifi_rtt_preamble));
        ALOGV("channel width = %d\n", responderInfo->channel.width);
        ALOGV("center freq = %d\n", responderInfo->channel.center_freq);
        ALOGV("preamble = %d\n", responderInfo->preamble);
        return NL_OK;
    }
};


/* API to request RTT measurement */
wifi_error wifi_rtt_range_request(wifi_request_id id, wifi_interface_handle iface,
                                       unsigned num_rtt_config, wifi_rtt_config rtt_config[], 
                                       wifi_rtt_event_handler handler)
{
    int ret = 0;
    wifi_handle handle = getWifiHandle(iface);
    RttRangeControl *RangeReq = new RttRangeControl(iface, id, num_rtt_config, 
                                                     rtt_config, handler);
    wifi_register_cmd(handle, id, RangeReq);
    ret = RangeReq->start();
    return (wifi_error) ret;
}

/* API to cancel RTT measurements */
wifi_error wifi_rtt_range_cancel(wifi_request_id id,  wifi_interface_handle iface,
                                      unsigned num_devices, mac_addr *addr)
{
    RttRangeControl *RangeCancel = new RttRangeControl(iface, id);
    if (RangeCancel) {
        RangeCancel->cancel_specific(num_devices, addr);
        RangeCancel->releaseRef();
        return WIFI_SUCCESS;
    }
    return WIFI_ERROR_INVALID_ARGS;
}

/* API to get RTT capabilities */
wifi_error wifi_get_rtt_capabilities(wifi_interface_handle iface,
                                        wifi_rtt_capabilities *capabilities)
{
    int ret = 0;
    GetRttCapa RttCapa(iface, capabilities);
    ret = RttCapa.requestResponse();
    return (wifi_error) ret;
}

/* API to get RTT responder info */
wifi_error wifi_rtt_get_responder_info(wifi_interface_handle iface, 
                                             wifi_rtt_responder *responder_info)
{
    int ret = 0;
    GetRttResponderInfo Info(iface, responder_info);
    ret = Info.requestResponse();
    return (wifi_error) ret;
}

class RttEnableResponder : public WifiCommand
{
private:
    wifi_channel_info channelHint;
    unsigned maxDurSec;
    wifi_rtt_responder * responderInfo;

public:
    RttEnableResponder(wifi_request_id id, wifi_interface_handle iface, 
                             wifi_channel_info channel_hint, unsigned max_duration_seconds,
                              wifi_rtt_responder *responder_info)
    : WifiCommand("RttEnableResponder", iface, id)
    {
        channelHint = channel_hint;
        maxDurSec = max_duration_seconds;
        responderInfo = responder_info;
        memset(responderInfo, 0, sizeof(wifi_rtt_responder));
    }

    virtual int create() {
        int result = 0;
        result = mMsg.create(MARVELL_OUI, NXP_SUBCMD_RTT_ENABLE_RESPONDER);
        if (result < 0) {
            ALOGE("Failed to create message to enable RTT responder");
            return result;
        }

        nlattr *data = mMsg.attr_start(NL80211_ATTR_VENDOR_DATA);
        result = mMsg.put(NXP_ATTR_RTT_CHANNEL_INFO, &channelHint, sizeof(wifi_channel_info));
        if (result < 0) {
            ALOGE("Failed to set channel hint\n");
            return result;
        }

        result = mMsg.put_u32(NXP_ATTR_RTT_MAX_DUR_SEC, maxDurSec);
        if(result < 0) {
            ALOGE("Failed to set max duration seconds\n");
            return result;
        }
        mMsg.attr_end(data);
        return WIFI_SUCCESS;
    }
    virtual int handleResponse(WifiEvent& reply){

        if (reply.get_cmd() != NL80211_CMD_VENDOR) {
            ALOGE("Ignoring reply with cmd = %d", reply.get_cmd());
            return NL_SKIP;
        }

        nlattr *tb = (nlattr*)(reply.get_data(NL80211_ATTR_VENDOR_DATA));
        int len = reply.get_vendor_data_len();
        if(tb == NULL || len == 0){
            ALOGE("ERROR! No data in command response\n");
            return NL_SKIP;
        }

        struct nlattr *tb_vendor[NXP_ATTR_RTT_MAX];
        int status = WIFI_ERROR_NONE;
        nla_parse(tb_vendor, (NXP_ATTR_RTT_MAX - 1), tb, len, NULL);
        if(!tb_vendor[NXP_ATTR_RTT_CHANNEL_INFO]){
            ALOGE("NXP_ATTR_RTT_CHANNEL not found");
            status = WIFI_ERROR_INVALID_ARGS;
            return status;
        }
        memcpy(&(responderInfo->channel), (wifi_channel_info *)nla_data(tb_vendor[NXP_ATTR_RTT_CHANNEL_INFO]),
                sizeof(wifi_channel_info));
        if(!tb_vendor[NXP_ATTR_RTT_PREAMBLE]){
            ALOGE("NXP_ATTR_RTT_PREAMBLE not found");
            status = WIFI_ERROR_INVALID_ARGS;
            return status;
        }
        memcpy(&(responderInfo->preamble), (wifi_rtt_preamble *)nla_data(tb_vendor[NXP_ATTR_RTT_PREAMBLE]),
                sizeof(wifi_rtt_preamble));
        return NL_OK;
    }
};

/** 
* Enable RTT responder mode.
* channel_hint - hint of the channel information where RTT responder should be enabled on.
* max_duration_seconds - timeout of responder mode.
* channel_used - channel used for RTT responder, NULL if responder is not enabled.
*/
wifi_error wifi_enable_responder(wifi_request_id id, wifi_interface_handle iface,
                                       wifi_channel_info channel_hint, 
                                       unsigned max_duration_seconds,
                                       wifi_rtt_responder *responder_info)
{
    int ret = 0;
    RttEnableResponder Responder(id, iface, channel_hint, max_duration_seconds,
                                 responder_info);
    ret = Responder.requestResponse();
    return (wifi_error)ret;
}

class RttDisableResponder : public WifiCommand
{
public:
    RttDisableResponder(wifi_request_id id, wifi_interface_handle iface)
    : WifiCommand("RttDisableResponder", iface, id)
    { }

    virtual int create(){
        int result = 0;
        result = mMsg.create(MARVELL_OUI, NXP_SUBCMD_RTT_DISABLE_RESPONDER);
        if (result < 0) {
            ALOGE("Failed to create message to disable rtt responder");
            return result;
        }
        return WIFI_SUCCESS;
    }

    virtual int handleResponse(WifiEvent& reply){
        /* Nothing to do in response! */
        return NL_SKIP;
    }
};

/* Disable RTT responder mode */
wifi_error wifi_disable_responder(wifi_request_id id, wifi_interface_handle iface)
{
    int ret = 0;
    RttDisableResponder Responder(id, iface);
    ret = Responder.requestResponse();
    return (wifi_error)ret;
}

class RttSetCommand : public WifiCommand
{
private:
    wifi_lci_information *LCI;
    wifi_lcr_information *LCR;
    ConfigureType Config;

public:
    RttSetCommand(wifi_request_id id, wifi_interface_handle iface, 
                         wifi_lci_information *lci, ConfigureType cmdType)
    : WifiCommand("RttSetCommand", iface, id)
    {
        LCI = lci;
        Config = cmdType;
    }

    RttSetCommand(wifi_request_id id, wifi_interface_handle iface,
                         wifi_lcr_information *lcr, ConfigureType cmdType)
    : WifiCommand("RttSetCommand", iface, id)
    {
        LCR = lcr;
        Config = cmdType;
    }

    virtual int create(){
        int result = 0;
        switch(Config){
            case RTT_SET_LCI:
            {
                result = mMsg.create(MARVELL_OUI, NXP_SUBCMD_RTT_SET_LCI);
                if (result != WIFI_SUCCESS) {
                    ALOGE("Failed to create message to set LCI - %d", result);
                    return result;
                }
                
                nlattr *data = mMsg.attr_start(NL80211_ATTR_VENDOR_DATA);
                result = mMsg.put(NXP_ATTR_RTT_LCI_INFO, LCI, sizeof(wifi_lci_information));
                if (result < 0) {
                    ALOGE("Failed to set channel hint\n");
                    return result;
                }
                mMsg.attr_end(data);
                break;
            }
            case RTT_SET_LCR:
            {
                result = mMsg.create(MARVELL_OUI, NXP_SUBCMD_RTT_SET_LCR);
                if (result != WIFI_SUCCESS) {
                    ALOGE("Failed to create message to set LCR - %d", result);
                    return result;
                }
                nlattr *data = mMsg.attr_start(NL80211_ATTR_VENDOR_DATA);
                result = mMsg.put(NXP_ATTR_RTT_LCR_INFO, LCR, sizeof(wifi_lcr_information));
                if (result < 0) {
                    ALOGE("Failed to set channel hint\n");
                    return result;
                }
                mMsg.attr_end(data);
                break;
            }
            default:
                ALOGE("Unknow command");
                return WIFI_ERROR_UNKNOWN;
        }
        return WIFI_SUCCESS;
    }

    virtual int handleResponse(WifiEvent& reply){
        /* Nothing to do in response! */
        return NL_SKIP;
    }
};

/* API to configure the LCI */
wifi_error wifi_set_lci(wifi_request_id id, wifi_interface_handle iface, 
                          wifi_lci_information *lci)
{
    int ret = 0;
    RttSetCommand SetLci(id, iface, lci, RTT_SET_LCI);
    ret = SetLci.requestResponse();
    return (wifi_error)ret;
}

/* API to configure the LCR */
wifi_error wifi_set_lcr(wifi_request_id id, wifi_interface_handle iface, 
                          wifi_lcr_information *lcr)
{
    int ret = 0;
    RttSetCommand SetLcr(id, iface, lcr, RTT_SET_LCR);
    ret = SetLcr.requestResponse();
    return (wifi_error) ret;
}
