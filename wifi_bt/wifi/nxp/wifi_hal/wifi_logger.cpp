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

#include "wifi_hal.h"
#include "common.h"
#include "cpp_bindings.h"
#include <utils/String8.h>

using namespace android;
#define MEM_DUMP_CHUNK_SIZE (4 * 1024)
#define MEM_DUMP_PATH_LENGTH      128
#define DEFAULT_CMD_EVENT_WAKE_CNT_SZ 32
#define DEFAULT_DRIVER_FW_LOCAL_WAKE_CNT_SZ 32

typedef enum {
    NXP_ATTR_LOGGER_INVALID = 0,
    NXP_ATTR_LOGGER_RING_ID,
    NXP_ATTR_LOGGER_FLAGS,
    NXP_ATTR_LOGGER_VERBOSE_LEVEL,
    NXP_ATTR_LOGGER_MIN_DATA_SIZE,
    NXP_ATTR_LOGGER_RING_BUFFER_STATUS,
    NXP_ATTR_LOGGER_NUM_RINGS,
    NXP_ATTR_LOGGER_FEATURE_SET,
    NXP_ATTR_LOGGER_MAX_INTERVAL_SEC,
    NXP_ATTR_LOGGER_RING_BUFFER,
    NXP_ATTR_LOGGER_NAME,
    NXP_ATTR_MEM_DUMP,
    NXP_ATTR_LOGGER_ERR_CODE,
    NXP_ATTR_LOGGER_RING_DATA,
    NXP_ATTR_LOGGER_WAKE_REASON_STAT,
    NXP_ATTR_LOGGER_TX_REPORT_BUFFS,
    NXP_ATTR_LOGGER_RX_REPORT_BUFFS,
    NXP_ATTR_LOGGER_PKT_FATE_FRAME_CONTENT,
    NXP_ATTR_LOGGER_FW_DUMP_PATH = 20,
    NXP_ATTR_LOGGER_DRV_DUMP_PATH = 21,

    NXP_ATTR_WIFI_LOGGER_AFTER_LAST,
    NXP_ATTR_WIFI_LOGGER_MAX = 
    NXP_ATTR_WIFI_LOGGER_AFTER_LAST - 1
} LOGGER_ATTRIBUTE;

typedef enum {
    NXP_EVENT_RING_BUFFER_DATA = 0x1000b,
    NXP_EVENT_ALERT,
    NXP_EVENT_START_PKT_FATE,
    NXP_EVENT_WAKE_REASON_REPORT,
} LOGGER_EVENT;

typedef enum {
    FW_VERSION,
    DRV_VERSION,
    RING_DATA,
    RING_STATUS,
    GET_FEATURE,
    START_LOGGING,
    FW_MEM_DUMP,
    DRV_MEM_DUMP,
} CmdType;

typedef enum
{
    NO_HSWAKEUP_REASON = 0,     //0.unknown 
    BCAST_DATA_MATCHED,         // 1. Broadcast data matched
    MCAST_DATA_MATCHED,         // 2. Multicast data matched
    UCAST_DATA_MATCHED,         // 3. Unicast data matched
    MASKTABLE_EVENT_MATCHED,    // 4. Maskable event matched
    NON_MASKABLE_EVENT_MATCHED, // 5. Non-maskable event matched
    NON_MASKABLE_CONDITION_MATCHED, // 6. Non-maskable condition matched (EAPoL rekey)
    MAGIC_PATTERN_MATCHED,      // 7. Magic pattern matched
    CONTROL_FRAME_MATCHED,      // 8. Control frame matched
    MANAGEMENT_FRAME_MATCHED,   // 9. Management frame matched
    GTK_REKEY_FAILURE,          //10. GTK rekey failure
    RESERVED                    // Others: reserved
} HSWakeupReason_t;

///////////////////////////////////////////////////////////////////////////////
class LoggerGetVersion : public WifiCommand
{
private:
    char *Buffer;
    int bufferSize;
    CmdType Type;

public:
    LoggerGetVersion(wifi_interface_handle iface, char *buffer, 
                           int buffer_size, CmdType type)
    : WifiCommand("LoggerGetVersion", iface, 0)
    {
        Buffer = buffer;
        bufferSize = buffer_size;
        Type = type;
        memset(Buffer, 0, bufferSize);
    }

    virtual int create(){
        int result = 0;
        switch(Type){
            case FW_VERSION:
            {
                result = mMsg.create(MARVELL_OUI, NXP_SUBCMD_GET_FW_VERSION);
                if (result < 0) {
                    ALOGE("Failed to create message to get fw version - %d", result);
                    return result;
                }
                 break;
             }
            case DRV_VERSION:
            {
                result = mMsg.create(MARVELL_OUI, NXP_SUBCMD_GET_DRIVER_VERSION);
                if (result < 0) {
                    ALOGE("Failed to create message to get driver version - %d", result);
                    return result;
                }
                 break;
            }
            default:
                ALOGE("Unknow command");
                return WIFI_ERROR_UNKNOWN;
        }
        return WIFI_SUCCESS;
    }

    virtual int handleResponse(WifiEvent& reply){
        if (reply.get_cmd() != NL80211_CMD_VENDOR) {
            ALOGW("Ignoring reply with cmd = %d", reply.get_cmd());
            return NL_SKIP;
        }

        nlattr *tb = (nlattr*)(reply.get_data(NL80211_ATTR_VENDOR_DATA));
        int len = reply.get_vendor_data_len();
        if(tb == NULL || len == 0){
            ALOGE("ERROR! No data in command response");
            return NL_SKIP;
        }

        struct nlattr *tb_vendor[NXP_ATTR_WIFI_LOGGER_MAX];
        int status = WIFI_ERROR_NONE;
        nla_parse(tb_vendor, (NXP_ATTR_WIFI_LOGGER_MAX-1),tb, len, NULL);
        if (bufferSize < len)
           return NL_SKIP;
        bufferSize = len;
        if (!tb_vendor[NXP_ATTR_LOGGER_NAME]){
            ALOGE("NXP_ATTR_NAME not found");
            status = WIFI_ERROR_INVALID_ARGS;
            return status;
        }
        memcpy(Buffer, nla_data(tb_vendor[NXP_ATTR_LOGGER_NAME]) ,bufferSize);
        return NL_OK;
    }
};

class LoggerCommand : public WifiCommand
{
private:
    u32 *numRings;
    wifi_ring_buffer_status *ringStatus;
    unsigned int *Support;
    u32 verboseLevel;
    u32 Flags;
    u32 maxIntervalSec;
    u32 minDataSize;
    char *ringName;
    CmdType Type;

public:
    // constructor for ring data
    LoggerCommand(wifi_interface_handle iface, char *ring_name, CmdType cmdType)
        : WifiCommand("LoggerCommand", iface, 0)
    {
        ringName = ring_name;
        Type = cmdType;
        numRings = NULL;
        ringStatus = NULL;
        Support = NULL;
        verboseLevel = 0;
        Flags = 0;
        maxIntervalSec = 0;
        minDataSize = 0;
    }

    // constructor for ring status
    LoggerCommand(wifi_interface_handle iface, u32 *num_rings,
                         wifi_ring_buffer_status *status, CmdType cmdType)
        : WifiCommand("LoggerCommand", iface, 0)
    {
        numRings = num_rings;
        ringStatus = status;
        Type = cmdType;
        Support = NULL;
        verboseLevel = 0;
        Flags = 0;
        maxIntervalSec = 0;
        minDataSize = 0;
        ringName = NULL;
    }

    // constructor for feature set
    LoggerCommand(wifi_interface_handle iface, unsigned int *support,
                          CmdType cmdType)
        : WifiCommand("LoggerCommand", iface, 0)
    {
        Support = support;
        Type = cmdType;
        numRings = NULL;
        ringStatus = NULL;
        verboseLevel = 0;
        Flags = 0;
        maxIntervalSec = 0;
        minDataSize = 0;
        ringName = NULL;
    }

    // constructor for start logging
    LoggerCommand(wifi_interface_handle iface, u32 verbose_level, u32 flags,
                          u32 max_interval_sec, u32 min_data_size, char *ring_name,
                          CmdType cmdType)
        : WifiCommand("LoggerCommand", iface, 0)
    {
        verboseLevel = verbose_level;
        Flags = flags;
        maxIntervalSec = max_interval_sec;
        minDataSize = min_data_size;
        ringName = ring_name;
        Type = cmdType;
        numRings = NULL;
        ringStatus = NULL;
        Support = NULL;
    }

    int createRequest(WifiRequest &request) {
        int result = 0;
        switch (Type) {
            case RING_DATA:
            {
                result = request.create(MARVELL_OUI, NXP_SUBCMD_GET_RING_DATA);
                if (result != WIFI_SUCCESS) {
                    ALOGE("Failed to create message to get ring data - %d", result);
                    return result;
                }

                nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
                result = request.put_string(NXP_ATTR_LOGGER_RING_ID, ringName);
                if (result != WIFI_SUCCESS)
                    return result;
                request.attr_end(data);
                break;
            }

            case RING_STATUS:
            {
                result = request.create(MARVELL_OUI, NXP_SUBCMD_GET_RING_STATUS);
                if (result != WIFI_SUCCESS) {
                    ALOGE("Failed to create message to get ring status - %d", result);
                    return result;
                }
                break;
            }

            case GET_FEATURE:
            {
                result = request.create(MARVELL_OUI, NXP_SUBCMD_GET_LOGGER_FEATURE_SET);
                if (result < 0) {
                    ALOGE("Failed to create message to get logger feature set - %d", result);
                    return result;
                }
                break;
            }

            case START_LOGGING:
            {
                result = request.create(MARVELL_OUI, NXP_SUBCMD_START_LOGGING);
                if (result != WIFI_SUCCESS) {
                    ALOGE("Failed to create request to start logging - %d", result);
                    return result;
                }
                nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
                result = request.put_u32(NXP_ATTR_LOGGER_VERBOSE_LEVEL, verboseLevel);
                if (result != WIFI_SUCCESS)
                    return result;
                result = request.put_u32(NXP_ATTR_LOGGER_FLAGS, Flags);
                if (result != WIFI_SUCCESS)
                    return result;
                result = request.put_u32(NXP_ATTR_LOGGER_MAX_INTERVAL_SEC, maxIntervalSec);
                if (result != WIFI_SUCCESS)
                    return result;
                result = request.put_u32(NXP_ATTR_LOGGER_MIN_DATA_SIZE, minDataSize);
                if (result != WIFI_SUCCESS)
                    return result;
                result = request.put_string(NXP_ATTR_LOGGER_RING_ID, ringName);
                if (result != WIFI_SUCCESS)
                    return result;
                request.attr_end(data);
                break;
            }
            default:
                ALOGE("Unknown Debug command");
                result = WIFI_ERROR_UNKNOWN;
        }
        return WIFI_SUCCESS;
    }

    int start() {
        WifiRequest request(familyId(), ifaceId());
        int result = createRequest(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("Failed to create debug request; result = %d", result);
            return result;
        }

        result = requestResponse(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("Failed to register debug response; result = %d", result);
        }
        return WIFI_SUCCESS;
    }

    virtual int handleResponse(WifiEvent& reply) {
        if (reply.get_cmd() != NL80211_CMD_VENDOR) {
            ALOGE("Ignoring reply with cmd = %d", reply.get_cmd());
            return NL_SKIP;
        }

        switch (Type) {
            case START_LOGGING:
            case RING_DATA:
                break;
            case RING_STATUS:
            {
                nlattr *vendor_data = (nlattr *)(reply.get_data(NL80211_ATTR_VENDOR_DATA));
                int len = reply.get_vendor_data_len();
                unsigned int num_rings = 0;
                if (vendor_data == NULL || len == 0) {
                    ALOGE("No Debug data found");
                    return NL_SKIP;
                }

                struct nlattr *tb_vendor[NXP_ATTR_WIFI_LOGGER_MAX + 1];
                nla_parse(tb_vendor, NXP_ATTR_WIFI_LOGGER_MAX, vendor_data, len, NULL);
                if(!tb_vendor[NXP_ATTR_LOGGER_NUM_RINGS]){
                    ALOGE("NXP_ATTR_LOGGER_NUM_RINGS not found");
                    return NL_SKIP;
                }
                num_rings = nla_get_u32(tb_vendor[NXP_ATTR_LOGGER_NUM_RINGS]);
                if(*numRings < num_rings){
                    ALOGE("Not enough status buffers provided, available: %d", (*numRings));
                    return NL_SKIP;
                }

                *numRings = num_rings;
                if(!tb_vendor[NXP_ATTR_LOGGER_RING_BUFFER_STATUS]){
                    ALOGE("NXP_ATTR_LOGGER_RING_BUFFER_STATUS not found");
                    return NL_SKIP;
                }
                int total_len = 0;
                total_len = (*numRings) * sizeof(wifi_ring_buffer_status);
                memset(ringStatus, 0, total_len);
                memcpy(ringStatus, (void *)nla_data(tb_vendor[NXP_ATTR_LOGGER_RING_BUFFER_STATUS]),
                       total_len);
                break;
            }
            case GET_FEATURE:
            {
                int len = reply.get_vendor_data_len();
                struct nlattr *vendor_data = (nlattr *)(reply.get_data(NL80211_ATTR_VENDOR_DATA));
                if(vendor_data == NULL || len == 0){
                    ALOGE("ERROR! No data in get feature command response\n");
                    return NL_SKIP;
                }
                struct nlattr *tb_vendor[NXP_ATTR_WIFI_LOGGER_MAX + 1];
                nla_parse(tb_vendor, NXP_ATTR_WIFI_LOGGER_MAX, vendor_data, len, NULL);
                if(!tb_vendor[NXP_ATTR_LOGGER_FEATURE_SET]){
                    ALOGE("NXP_ATTR_WIFI_LOGGER_FEATURE_SET not found\n");
                    return NL_SKIP;
                }
                *Support = nla_get_u32(tb_vendor[NXP_ATTR_LOGGER_FEATURE_SET]);
                ALOGE("get supported feature = 0x%x\n", (*Support));
                break;
            }
            default:
                ALOGW("Unknown Logger command");
        }
        return NL_OK;
    }
};

wifi_error wifi_get_firmware_version(wifi_interface_handle iface, 
                                            char *buffer, int buffer_size)
{
   int ret = 0;
    if(!buffer || (buffer_size < 0)){
        ALOGE("buffer is NULL or invalid buffer size\n");
        return WIFI_ERROR_INVALID_ARGS;
    }
    LoggerGetVersion FwVersion(iface, buffer, buffer_size, FW_VERSION);
    ret = FwVersion.requestResponse();
    return (wifi_error)ret;
}

wifi_error wifi_get_driver_version(wifi_interface_handle iface, 
                                        char *buffer, int buffer_size)
{
     int ret = 0;
    if(!buffer || (buffer_size < 0)){
        ALOGE("buffer is NULL or invalid buffer size");
        return WIFI_ERROR_INVALID_ARGS;
    }
    LoggerGetVersion DrvVersion(iface, buffer, buffer_size, DRV_VERSION);
    ret = DrvVersion.requestResponse();
    return (wifi_error)ret;
}

wifi_error wifi_get_ring_data(wifi_interface_handle iface, char *ring_name)
{
    int ret = 0;
    if(!ring_name){
        ALOGE("ring_name is NULL");
        return WIFI_ERROR_INVALID_ARGS;
    }
    LoggerCommand *RingData = new LoggerCommand(iface, ring_name, RING_DATA);
    ret = RingData->start();
    return (wifi_error)ret;
}

wifi_error wifi_get_ring_buffers_status(wifi_interface_handle iface, u32 *num_rings,
                                              wifi_ring_buffer_status *status)
{
    int ret = 0;
    if(!status || !(*num_rings)){
        ALOGE("Ring status buffer NULL or num_rings is zero");
        return  WIFI_ERROR_INVALID_ARGS;
    }
    LoggerCommand *RingStatus = new LoggerCommand(iface, num_rings, status, RING_STATUS);
    ret = RingStatus->start();
        return (wifi_error)ret;
}

/* API to get supportable feature */
wifi_error wifi_get_logger_supported_feature_set(wifi_interface_handle iface,
                                                          unsigned int *support)
{
    int ret = 0;
    if(!support){
        ALOGE("Logger support feature set buffer is NULL");
        return  WIFI_ERROR_INVALID_ARGS;
    }
    LoggerCommand *FeatureSet = new LoggerCommand(iface, support, GET_FEATURE);
    ret = FeatureSet->start();
    return (wifi_error)ret;
}

wifi_error wifi_start_logging(wifi_interface_handle iface, u32 verbose_level,
                                  u32 flags, u32 max_interval_sec,
                                  u32 min_data_size, char *ring_name)
{
    int ret = 0;
    if(!ring_name){
        ALOGE("Ring name is NULL");
        return  WIFI_ERROR_INVALID_ARGS;
    }
    LoggerCommand *Start = new LoggerCommand(iface, verbose_level, flags, max_interval_sec,
                                             min_data_size, ring_name, START_LOGGING);
    ret = Start->start();
    return (wifi_error)ret;
}
class StartPktFateCommand : public WifiCommand
{
public:
    StartPktFateCommand(wifi_interface_handle handle)
    : WifiCommand("StartPktFateCommand", handle, 0)
    { }

    int createRequest(WifiRequest& request) {
        int result = request.create(MARVELL_OUI, NXP_SUBCMD_START_PKT_FATE_MONITORING);
        if (result < 0) {
            ALOGE("Failed to create message to start packet fate monitor");
            return result;
        }
        return WIFI_SUCCESS;
    }

    virtual int start(){
        WifiRequest request(familyId(), ifaceId());
        int result = createRequest(request);
        if (result < 0)
            return result;
        result = requestResponse(request);
        if (result < 0) {
            ALOGE("Failed to start packet fate, result = %d", result);
            return result;
        }
        registerVendorHandler(MARVELL_OUI, NXP_EVENT_START_PKT_FATE);
        return WIFI_SUCCESS;
    }

    virtual int handleEvent(WifiEvent& event) {
       hal_info *info = mInfo;
       if (event.get_cmd() != NL80211_CMD_VENDOR) {
           ALOGE("Ignoring reply with cmd = %d", event.get_cmd());
       return NL_SKIP;
       }

       nlattr *vendor_data =(nlattr *) (event.get_data(NL80211_ATTR_VENDOR_DATA));
       int len = event.get_vendor_data_len();
       if (vendor_data == NULL || len == 0) {
            ALOGE("Start packet fate: No data");
            return NL_SKIP;
        }

        struct nlattr *tb_vendor[NXP_ATTR_WIFI_LOGGER_MAX + 1];
        struct nlattr *nla = NULL;
        nla_parse(tb_vendor, NXP_ATTR_WIFI_LOGGER_MAX, vendor_data, len, NULL);
        nla = tb_vendor[NXP_ATTR_LOGGER_TX_REPORT_BUFFS];
        if(nla){
            memcpy(&info->pkt_fate_stats->tx_fate_stats[info->pkt_fate_stats->n_tx_stats_collected],
                    (wifi_tx_report_i *)nla_data(nla), nla->nla_len);
            info->pkt_fate_stats->tx_fate_stats[info->pkt_fate_stats->n_tx_stats_collected].frame_inf.frame_content = NULL;
            if(tb_vendor[NXP_ATTR_LOGGER_PKT_FATE_FRAME_CONTENT]){
                info->pkt_fate_stats->tx_fate_stats[info->pkt_fate_stats->n_tx_stats_collected].frame_inf.frame_content\
                = (char *)nla_data(tb_vendor[NXP_ATTR_LOGGER_PKT_FATE_FRAME_CONTENT]);
                int ftypetx =(int)(info->pkt_fate_stats->tx_fate_stats[info->pkt_fate_stats->n_tx_stats_collected].frame_inf.payload_type);
                if ((ftypetx != FRAME_TYPE_ETHERNET_II) && (ftypetx != FRAME_TYPE_80211_MGMT)) {
                     info->pkt_fate_stats->tx_fate_stats[info->pkt_fate_stats->n_tx_stats_collected].frame_inf.payload_type = FRAME_TYPE_UNKNOWN;
                }
                info->pkt_fate_stats->n_tx_stats_collected ++;
                info->pkt_fate_stats->n_tx_stats_collected %= MAX_FATE_LOG_LEN;
            }
        }
        nla = tb_vendor[NXP_ATTR_LOGGER_RX_REPORT_BUFFS];
        if(nla){
            memcpy(&info->pkt_fate_stats->rx_fate_stats[info->pkt_fate_stats->n_rx_stats_collected],
                    (wifi_rx_report_i *)nla_data(nla), nla->nla_len);
            info->pkt_fate_stats->rx_fate_stats[info->pkt_fate_stats->n_rx_stats_collected].frame_inf.frame_content = NULL;
            if(tb_vendor[NXP_ATTR_LOGGER_PKT_FATE_FRAME_CONTENT]){
                info->pkt_fate_stats->rx_fate_stats[info->pkt_fate_stats->n_rx_stats_collected].frame_inf.frame_content\
                = (char *)nla_data(tb_vendor[NXP_ATTR_LOGGER_PKT_FATE_FRAME_CONTENT]);
                int ftyperx =(int)(info->pkt_fate_stats->rx_fate_stats[info->pkt_fate_stats->n_rx_stats_collected].frame_inf.payload_type);
                if ((ftyperx != FRAME_TYPE_ETHERNET_II) && (ftyperx != FRAME_TYPE_80211_MGMT)) {
                     info->pkt_fate_stats->rx_fate_stats[info->pkt_fate_stats->n_rx_stats_collected].frame_inf.payload_type = FRAME_TYPE_UNKNOWN;
                }
                info->pkt_fate_stats->n_rx_stats_collected ++;
                info->pkt_fate_stats->n_rx_stats_collected %= MAX_FATE_LOG_LEN; 
            }
        }
        return NL_OK;
    }
};

wifi_error wifi_start_pkt_fate_monitoring(wifi_interface_handle handle)
{
    wifi_handle wifiHandle = getWifiHandle(handle);
    hal_info *info = getHalInfo(wifiHandle);

    if (info->fate_monitoring_enabled == true) {
        ALOGW("Packet monitoring is already enabled");
        return WIFI_SUCCESS;
    }
    StartPktFateCommand *cmd = new StartPktFateCommand(handle);
    if (cmd) {
        wifi_register_cmd(wifiHandle, 0, cmd);
        wifi_error result = (wifi_error)cmd->start();
        if (result != WIFI_SUCCESS)
            wifi_unregister_cmd(wifiHandle, 0);
        info->pkt_fate_stats = (packet_fate_monitor_info *) malloc (
                                              sizeof(packet_fate_monitor_info));
        if (info->pkt_fate_stats == NULL) {
            ALOGE("Failed to allocate memory for : %zu bytes",
                  sizeof(packet_fate_monitor_info));
            return WIFI_ERROR_OUT_OF_MEMORY;
        }
        memset(info->pkt_fate_stats, 0, sizeof(packet_fate_monitor_info));
        pthread_mutex_lock(&info->pkt_fate_stats_lock);
        info->fate_monitoring_enabled = true;
        pthread_mutex_unlock(&info->pkt_fate_stats_lock);
        return result;
    } else {
        ALOGE("Out of memory");
        return WIFI_ERROR_OUT_OF_MEMORY;
    }
}

class StartWakeReasonCnt : public WifiCommand
{
public:
    StartWakeReasonCnt(wifi_handle handle)
        : WifiCommand("StartWakeReasonCnt", handle, 0)
    {
        mInfo = getHalInfo(handle);
    }

    virtual int start(){
        registerVendorHandler(MARVELL_OUI, NXP_EVENT_WAKE_REASON_REPORT);
        return WIFI_SUCCESS;
    }

    virtual int handleEvent(WifiEvent &event){
        u16 reason = 0;
        WLAN_DRIVER_WAKE_REASON_CNT *mWifiWakeReasonCnt = mInfo->wifi_wake_reason_cnt;
        nlattr *vendor_data = (nlattr *)event.get_data(NL80211_ATTR_VENDOR_DATA);
        int len = event.get_vendor_data_len();
        if (vendor_data == NULL || len == 0) {
            ALOGE("No data found");
            return NL_SKIP;
        }

        struct nlattr* tb_vendor[NXP_ATTR_WIFI_LOGGER_MAX + 1];
        nla_parse(tb_vendor, NXP_ATTR_WIFI_LOGGER_MAX, vendor_data, len, NULL);

        if (!tb_vendor[NXP_ATTR_LOGGER_WAKE_REASON_STAT]){
            ALOGE("NXP_ATTR_LOGGER_WAKE_REASON_STAT not found");
            return WIFI_ERROR_INVALID_ARGS;
        }
        reason = nla_get_u16(tb_vendor[NXP_ATTR_LOGGER_WAKE_REASON_STAT]);
        switch (reason) {
            case BCAST_DATA_MATCHED:
                mWifiWakeReasonCnt->total_rx_data_wake++;
                mWifiWakeReasonCnt->rx_wake_details.rx_broadcast_cnt++;
                break;
            case MCAST_DATA_MATCHED:
                mWifiWakeReasonCnt->total_rx_data_wake++;
                mWifiWakeReasonCnt->rx_wake_details.rx_multicast_cnt++;
                break;
            case UCAST_DATA_MATCHED:
                mWifiWakeReasonCnt->total_rx_data_wake++;
                mWifiWakeReasonCnt->rx_wake_details.rx_unicast_cnt++;
                break;
            case MASKTABLE_EVENT_MATCHED:
            case NON_MASKABLE_EVENT_MATCHED:
                mWifiWakeReasonCnt->total_cmd_event_wake++;
                if (mWifiWakeReasonCnt->cmd_event_wake_cnt_used < mWifiWakeReasonCnt->cmd_event_wake_cnt_sz - 1) {
                    mWifiWakeReasonCnt->cmd_event_wake_cnt[mWifiWakeReasonCnt->cmd_event_wake_cnt_used++] =
                         reason;
                } else {
                    ALOGE("cmd_event_wake_cnt_used reached its size at %d",
                           mWifiWakeReasonCnt->cmd_event_wake_cnt_used);
                }
                break;
            case NON_MASKABLE_CONDITION_MATCHED:
            case MAGIC_PATTERN_MATCHED:
            case CONTROL_FRAME_MATCHED:
            case MANAGEMENT_FRAME_MATCHED:
            case GTK_REKEY_FAILURE:
            case NO_HSWAKEUP_REASON:
                mWifiWakeReasonCnt->total_driver_fw_local_wake++;
                if (mWifiWakeReasonCnt->driver_fw_local_wake_cnt_used <
                    mWifiWakeReasonCnt->driver_fw_local_wake_cnt_sz - 1) {
                    mWifiWakeReasonCnt
                        ->driver_fw_local_wake_cnt[mWifiWakeReasonCnt->driver_fw_local_wake_cnt_used++] =
                        reason;
                } else {
                    ALOGE("driver_fw_local_wake_cnt_used reached its size at %d",
                           mWifiWakeReasonCnt->driver_fw_local_wake_cnt_used);
                }
                break;
            default:
                break;
        }
        return NL_OK;
    }
};

wifi_error wifi_start_wake_reason_cnt(hal_info *info)
{
    WLAN_DRIVER_WAKE_REASON_CNT *mWifiWakeReasonCnt = NULL;

    mWifiWakeReasonCnt = (WLAN_DRIVER_WAKE_REASON_CNT *)malloc(sizeof(WLAN_DRIVER_WAKE_REASON_CNT));
    if (!(mWifiWakeReasonCnt)) {
        ALOGE("WiFi Logger: wake_reason_stat alloc failed");
        return WIFI_ERROR_OUT_OF_MEMORY;
    }
    memset(mWifiWakeReasonCnt, 0, sizeof(WLAN_DRIVER_WAKE_REASON_CNT));

    mWifiWakeReasonCnt->cmd_event_wake_cnt = (int*)malloc(sizeof(int) * DEFAULT_CMD_EVENT_WAKE_CNT_SZ);
    if (mWifiWakeReasonCnt->cmd_event_wake_cnt == NULL) {
        ALOGE("WiFi Logger: wake_reason_stat cmd_event_wake_cnt alloc failed");
        free(mWifiWakeReasonCnt);
        return WIFI_ERROR_OUT_OF_MEMORY;
    }
    memset(mWifiWakeReasonCnt->cmd_event_wake_cnt, 0, sizeof(int) * DEFAULT_CMD_EVENT_WAKE_CNT_SZ);
    mWifiWakeReasonCnt->cmd_event_wake_cnt_sz = DEFAULT_CMD_EVENT_WAKE_CNT_SZ;

    mWifiWakeReasonCnt->driver_fw_local_wake_cnt = (int*)malloc(sizeof(int) * DEFAULT_DRIVER_FW_LOCAL_WAKE_CNT_SZ);
    if (!(mWifiWakeReasonCnt->driver_fw_local_wake_cnt)) {
        ALOGE("WiFi Logger: wake_reason_stat driver_fw_local_wake_cnt alloc failed");
        free(mWifiWakeReasonCnt->cmd_event_wake_cnt);
        free(mWifiWakeReasonCnt);
        return WIFI_ERROR_OUT_OF_MEMORY;
    }
    memset(mWifiWakeReasonCnt->driver_fw_local_wake_cnt, 0, 
           sizeof(int) * DEFAULT_DRIVER_FW_LOCAL_WAKE_CNT_SZ);
    mWifiWakeReasonCnt->driver_fw_local_wake_cnt_sz = DEFAULT_DRIVER_FW_LOCAL_WAKE_CNT_SZ;
    info->wifi_wake_reason_cnt = mWifiWakeReasonCnt;

    wifi_handle handle = getWifiHandle(info);
    StartWakeReasonCnt *cmd = new StartWakeReasonCnt(handle);
    if (cmd) {
        wifi_register_cmd(handle, 0, cmd);
        wifi_error result = (wifi_error)cmd->start();
        if (result != WIFI_SUCCESS)
            wifi_unregister_cmd(handle, 0);
        return result;
    } else {
        ALOGE("Out of memory");
        free(mWifiWakeReasonCnt->driver_fw_local_wake_cnt);
        free(mWifiWakeReasonCnt->cmd_event_wake_cnt);
        free(mWifiWakeReasonCnt);
        return WIFI_ERROR_OUT_OF_MEMORY;
    }
}

///////////////////////////////////////////////////////////////////////////////
class SetLogHandler : public WifiCommand
{
private:
    wifi_ring_buffer_data_handler Handler;
    wifi_ring_buffer_status buffStatus;
    char *Buffer;
    int bufferSize;

public:
    SetLogHandler(wifi_interface_handle iface, int id, 
                       wifi_ring_buffer_data_handler handler)
    : WifiCommand("SetLogHandler", iface, id)
    {
        Handler = handler;
        memset(&buffStatus, 0, sizeof(wifi_ring_buffer_status));
        Buffer = NULL;
        bufferSize = 0;
    }

    int start() {
        int ret = 0;
        ALOGV("Register loghandler");
        ret = registerVendorHandler(MARVELL_OUI, NXP_EVENT_RING_BUFFER_DATA);
        if(ret < 0){
            ALOGE("Failed to register vendor handler for NXP_EVENT_RING_BUFFER_DATA");
            return ret;
        }
        return WIFI_SUCCESS;
    }

    virtual int cancel() {
        /* Send a command to driver to stop generating logging events */
        ALOGV("Clear loghandler");
        /* unregister event handler */
        unregisterVendorHandler(MARVELL_OUI, NXP_EVENT_RING_BUFFER_DATA);
        return WIFI_SUCCESS;
    }

    virtual int handleEvent(WifiEvent& event) {
        if (event.get_cmd() != NL80211_CMD_VENDOR) {
           ALOGD("Ignoring reply with cmd = %d", event.get_cmd());
           return NL_SKIP;
        }

        nlattr *vendor_data = (nlattr *)event.get_data(NL80211_ATTR_VENDOR_DATA);
        int len = event.get_vendor_data_len();
        int event_id = event.get_vendor_subcmd();

        if (vendor_data == NULL || len == 0) {
            ALOGE("Error! No data in event");
            return NL_SKIP;
        }

        if(event_id == NXP_EVENT_RING_BUFFER_DATA) {
            struct nlattr *tb_vendor[NXP_ATTR_WIFI_LOGGER_MAX + 1];
            nla_parse(tb_vendor, NXP_ATTR_WIFI_LOGGER_MAX, vendor_data, len, NULL);

            if(!tb_vendor[NXP_ATTR_LOGGER_RING_BUFFER_STATUS]){
                ALOGE("NXP_ATTR_LOGGER_RING_BUFFER_STATUS not found");
                return NL_SKIP;
            }
            memcpy(&buffStatus, (void *)nla_data(tb_vendor[NXP_ATTR_LOGGER_RING_BUFFER_STATUS]),
                    sizeof(buffStatus));
            if(!tb_vendor[NXP_ATTR_LOGGER_RING_BUFFER]){
                ALOGE("NXP_ATTR_LOGGER_RING_BUFFER not found");
                return NL_SKIP;
            }
            bufferSize = nla_len(tb_vendor[NXP_ATTR_LOGGER_RING_BUFFER]);
            Buffer = (char *)nla_data(tb_vendor[NXP_ATTR_LOGGER_RING_BUFFER]);
            if (Handler.on_ring_buffer_data)
                (*Handler.on_ring_buffer_data)((char *)buffStatus.name, Buffer, bufferSize, &buffStatus);
            } else {
                ALOGE("Unknown Event");
                return NL_SKIP;
            }
        return NL_OK;
    }
};

wifi_error wifi_set_log_handler(wifi_request_id id, 
                                     wifi_interface_handle iface,
                                     wifi_ring_buffer_data_handler handler)
{
    wifi_handle handle = getWifiHandle(iface);
    int ret = 0;

    SetLogHandler *SetHandler = new SetLogHandler(iface, id, handler);
    if (!SetHandler){
        ALOGE("Out of memory");
        return WIFI_ERROR_OUT_OF_MEMORY;
    }
    wifi_register_cmd(handle, id, SetHandler);
    ret = SetHandler->start();
    if (ret != WIFI_SUCCESS)
        wifi_unregister_cmd(handle, id);
    return (wifi_error)ret;
}

wifi_error wifi_reset_log_handler(wifi_request_id id, wifi_interface_handle iface)
{
    wifi_handle handle = getWifiHandle(iface);
    ALOGV("Loghandler reset, wifi_request_id = %d, handle = %p", id, handle);
    if (id == -1) {
        wifi_ring_buffer_data_handler handler;
        memset(&handler, 0, sizeof(handler));
        SetLogHandler *Reset = new SetLogHandler(iface, id, handler);
        Reset->cancel();
        Reset->releaseRef();
        return WIFI_SUCCESS;
    }
    return wifi_cancel_cmd(id, iface);
}

///////////////////////////////////////////////////////////////////////////////
class SetAlertHandler : public WifiCommand
{
private:
    wifi_alert_handler Handler;
    int buffSize;
    char *Buff;
    int errCode;

public:
    SetAlertHandler(wifi_interface_handle iface, int id, wifi_alert_handler handler)
    : WifiCommand("SetAlertHandler", iface, id)
    {
        Handler = handler;
        buffSize = 0;
        Buff = NULL;
        errCode = 0;
    }

    int start() {
        int ret = 0;
        ret = registerVendorHandler(MARVELL_OUI, NXP_EVENT_ALERT);
        if(ret < 0){
            ALOGE("Failed to register vendor handler for NXP_EVENT_ALERT");
            return ret;
        }
        return WIFI_SUCCESS;
    }

    virtual int cancel() {
        /* unregister alert handler */
        unregisterVendorHandler(MARVELL_OUI, NXP_EVENT_ALERT);
        wifi_unregister_cmd(wifiHandle(), id());
        ALOGV("Success to clear alerthandler");
        return WIFI_SUCCESS;
    }

    virtual int handleEvent(WifiEvent& event) {

        if (event.get_cmd() != NL80211_CMD_VENDOR) {
            ALOGE("Ignoring reply with cmd = %d", event.get_cmd());
            return NL_SKIP;
        }

        nlattr *vendor_data = (nlattr *)event.get_data(NL80211_ATTR_VENDOR_DATA);
        int len = event.get_vendor_data_len();
        int event_id = event.get_vendor_subcmd();

        ALOGV("len = %d", len);
        if (vendor_data == NULL || len == 0) {
            ALOGE("Error! No data in event");
            return NL_SKIP;
        }

        if (event_id == NXP_EVENT_ALERT) {
            struct nlattr *tb_vendor[NXP_ATTR_WIFI_LOGGER_MAX + 1];
            nla_parse(tb_vendor, NXP_ATTR_WIFI_LOGGER_MAX, vendor_data, len, NULL);

            if(!tb_vendor[NXP_ATTR_LOGGER_ERR_CODE]){
                ALOGE("NXP_ATTR_LOGGER_ERR_CODE not found");
                return NL_SKIP;
            }
            errCode = nla_get_u32(tb_vendor[NXP_ATTR_LOGGER_ERR_CODE]);
            if(!tb_vendor[NXP_ATTR_LOGGER_RING_DATA]){
                ALOGE("NXP_ATTR_LOGGER_RING_DATA not found");
                return NL_SKIP;
            }
            buffSize = nla_len(tb_vendor[NXP_ATTR_LOGGER_RING_DATA]);
            Buff = (char *)nla_data(tb_vendor[NXP_ATTR_LOGGER_RING_DATA]);
            if (Handler.on_alert) {
                 (*Handler.on_alert)(id(), Buff, buffSize, errCode);
             }
        }
        return NL_OK;
    }
};

wifi_error wifi_set_alert_handler(wifi_request_id id, 
                                      wifi_interface_handle iface,
                                      wifi_alert_handler handler)
{
    int ret = 0;
    wifi_handle handle = getWifiHandle(iface);
    ALOGV("Alerthandler start, handle = %p", handle);
    SetAlertHandler *SetAlert = new SetAlertHandler(iface, id, handler);
    if(!SetAlert){
        ALOGE("Out of memory");
        return WIFI_ERROR_OUT_OF_MEMORY;
    }
    wifi_register_cmd(handle, id, SetAlert);
    ret = SetAlert->start();
    if (ret != WIFI_SUCCESS)
        wifi_unregister_cmd(handle, id);
    return (wifi_error)ret;
}

wifi_error wifi_reset_alert_handler(wifi_request_id id, wifi_interface_handle iface)
{
    wifi_handle handle = getWifiHandle(iface);
    ALOGV("Alerthandler reset, wifi_request_id = %d, handle = %p", id, handle);

    if (id == -1) {
        wifi_alert_handler handler;
        memset(&handler, 0, sizeof(handler));

        SetAlertHandler *ResetAlert = new SetAlertHandler(iface, id, handler);
        ResetAlert->cancel();
        ResetAlert->releaseRef();
        return WIFI_SUCCESS;
    }

    return wifi_cancel_cmd(id, iface);
}

///////////////////////////////////////////////////////////////////////////////
class MemoryDumpCommand: public WifiCommand
{
private:
    wifi_firmware_memory_dump_handler Handler;
    wifi_driver_memory_dump_callbacks Callbacks;
    CmdType Type;
    u32 buffSize;
    char *Buff;
    u32 mStatus;

public:
    //Constructor for get firmware memory dumps
    MemoryDumpCommand(wifi_interface_handle iface, 
                                 wifi_firmware_memory_dump_handler handler,
                                 CmdType cmdType)
        : WifiCommand("MemoryDumpCommand", iface, 0)
    {
        Handler = handler;
        buffSize = 0;
        Buff = NULL;
        Type = cmdType;
        // Callbacks.on_driver_memory_dump = NULL;
        memset(&Callbacks, 0, sizeof(Callbacks));
        mStatus = 0;
    }

    //Constructor for get driver memory dumps
    MemoryDumpCommand(wifi_interface_handle iface, 
                                 wifi_driver_memory_dump_callbacks callbacks,
                                 CmdType cmdType)
        : WifiCommand("MemoryDumpCommand", iface, 0)
    {
        Callbacks = callbacks;
        buffSize = 0;
        Buff = NULL;
        Type = cmdType;
        // Handler.on_firmware_memory_dump = NULL;
        memset(&Handler, 0, sizeof(Handler));
        mStatus = 0;
    }

    int createRequest(WifiRequest &request) {
        int result = 0;
        switch (Type) {
            case FW_MEM_DUMP:
            {
                result = request.create(MARVELL_OUI, NXP_SUBCMD_GET_FW_MEM_DUMP);
                if (result != WIFI_SUCCESS) {
                    ALOGE("Failed to create request to get firmware memory dump - %d\n", result);
                    return result;
                }
                break;
            }
            case DRV_MEM_DUMP:
            {
                result = request.create(MARVELL_OUI, NXP_SUBCMD_GET_DRIVER_MEM_DUMP);
                if (result != WIFI_SUCCESS) {
                    ALOGE("Failed to create request to get driver memory dump - %d\n", result);
                    return result;
                }
                break;
            }
            default:
                ALOGE("Unknown Debug command");
                result = WIFI_ERROR_UNKNOWN;
        }
        return WIFI_SUCCESS;
    }

    int start() {
        WifiRequest request(familyId(), ifaceId());
        int result = createRequest(request);
        if (result != WIFI_SUCCESS) {
            return NL_SKIP;
        }

        result = requestResponse(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("Failed to register debug response; result = %d", result);
            return NL_SKIP;
        }
        return WIFI_SUCCESS;
    }

    virtual int handleResponse(WifiEvent& reply) {
        FILE *dumpPtr = NULL;
        u32 dumpRead = 0;
        int remained = 0;
        int path_len = 0;
        char *buffer = NULL;
        char full_path[MEM_DUMP_PATH_LENGTH];
        long int ftell_ret_val = 0;

        if (reply.get_cmd() != NL80211_CMD_VENDOR) {
            ALOGE("Ignoring reply with cmd = %d", reply.get_cmd());
            return NL_SKIP;
        }

        nlattr *vendor_data = (nlattr *)(reply.get_data(NL80211_ATTR_VENDOR_DATA));
        int len = reply.get_vendor_data_len();
        if (vendor_data == NULL) {
            ALOGE("Error! No data in command response");
            return NL_SKIP;
        }

        switch(Type){
            case FW_MEM_DUMP:
            {
                struct nlattr *tb_vendor[NXP_ATTR_WIFI_LOGGER_MAX + 1];
                nla_parse(tb_vendor, NXP_ATTR_WIFI_LOGGER_MAX, vendor_data, len, NULL);
                if(!tb_vendor[NXP_ATTR_LOGGER_FW_DUMP_PATH]){
                    ALOGE("NXP_ATTR_LOGGER_FW_DUMP_PATH not found");
                    mStatus = WIFI_ERROR_NOT_SUPPORTED;
                    return NL_SKIP;
                }
                memset(full_path, 0, MEM_DUMP_PATH_LENGTH);
                path_len = strlen((char *)nla_data(tb_vendor[NXP_ATTR_LOGGER_FW_DUMP_PATH]));
                if(path_len >= MEM_DUMP_PATH_LENGTH)
                    return NL_SKIP;
                strcpy(full_path, (char *)nla_data(tb_vendor[NXP_ATTR_LOGGER_FW_DUMP_PATH]));
                dumpPtr = fopen(full_path, "r");
                if(!dumpPtr){
                    ALOGE("Failed to open firmware dump file");
                    mStatus = WIFI_ERROR_NOT_SUPPORTED;
                    return NL_SKIP;
                }
                fseek(dumpPtr, 0, SEEK_END);
                ftell_ret_val = ftell(dumpPtr);
                if(ftell_ret_val < 0) {
                    ALOGE("failed to get current position of file pointer");
                    if(dumpPtr) {
                       fclose(dumpPtr);
                       dumpPtr = NULL;
                    }
                    mStatus = WIFI_ERROR_NOT_SUPPORTED;
                    return NL_SKIP;
                }
                else {
                    buffSize = ftell_ret_val;
                }
                Buff = (char *)malloc(sizeof(char) * buffSize);
                if(!Buff){
                    ALOGE("Failed to allocate buffer for firmware memory dump");
                    if(dumpPtr) {
                       fclose(dumpPtr);
                       dumpPtr = NULL;
                    }
                    mStatus = WIFI_ERROR_NOT_SUPPORTED;
                    return NL_SKIP;
                }
                memset(Buff, 0, buffSize);
                dumpRead = fread(Buff, 1, buffSize, dumpPtr);
                if(dumpRead <=0 || dumpRead != buffSize){
                    ALOGE("Failed to read firmware dump file at once, dumpRead = %d", dumpRead);
                    rewind(dumpPtr);
                    remained = (int)buffSize;
                    buffer = Buff;
                    while(remained){
                        u32 readBytes = 0;
                        if(remained >= MEM_DUMP_CHUNK_SIZE)
                            readBytes = MEM_DUMP_CHUNK_SIZE;
                        else
                            readBytes = remained;
                        dumpRead = fread(buffer, 1, readBytes, dumpPtr);
                        if(dumpRead){
                            remained -= readBytes;
                            buffer += readBytes;
                            ALOGE("Read successfully, readBytes = %d, remained = %d", readBytes, remained);
                        } else {
                            ALOGE("Chunk read failed");
                            mStatus = WIFI_ERROR_NOT_SUPPORTED;
                        }
                    }
                }
                if (Handler.on_firmware_memory_dump) {
                    (*Handler.on_firmware_memory_dump)(Buff, buffSize);
                }
                break;
            }
            case DRV_MEM_DUMP:
            {
                struct nlattr *tb_vendor[NXP_ATTR_WIFI_LOGGER_MAX + 1];
                nla_parse(tb_vendor, NXP_ATTR_WIFI_LOGGER_MAX, vendor_data, len, NULL);
                if(!tb_vendor[NXP_ATTR_LOGGER_DRV_DUMP_PATH]){
                    ALOGE("NXP_ATTR_LOGGER_DRV_DUMP_PATH not found");
                    mStatus = WIFI_ERROR_NOT_SUPPORTED;
                    return NL_SKIP;
                }
                memset(full_path, 0, MEM_DUMP_PATH_LENGTH);
                path_len = strlen((char *)nla_data(tb_vendor[NXP_ATTR_LOGGER_DRV_DUMP_PATH]));
                if(path_len >= MEM_DUMP_PATH_LENGTH)
                    return NL_SKIP;
                strcpy(full_path, (char *)nla_data(tb_vendor[NXP_ATTR_LOGGER_DRV_DUMP_PATH]));
                dumpPtr = fopen(full_path, "r");
                if(!dumpPtr){
                    ALOGE("Failed to open driver memory dump file");
                    mStatus = WIFI_ERROR_NOT_SUPPORTED;
                    return NL_SKIP;
                }
                fseek(dumpPtr, 0, SEEK_END);
                ftell_ret_val = ftell(dumpPtr);
                if(ftell_ret_val < 0) {
                    ALOGE("failed to get current position of file pointer");
                    if(dumpPtr) {
                       fclose(dumpPtr);
                       dumpPtr = NULL;
                    }
                    mStatus = WIFI_ERROR_NOT_SUPPORTED;
                    return NL_SKIP;
                }
                else {
                    buffSize = ftell_ret_val;
                }
                Buff = (char *)malloc(sizeof(char) * buffSize);
                if(!Buff){
                    ALOGE("Failed to allocate buffer for driver memory dump");
                    if(dumpPtr){
                       fclose(dumpPtr);
                       dumpPtr = NULL;
                    }
                    mStatus = WIFI_ERROR_NOT_SUPPORTED;
                    return NL_SKIP;
                }
                memset(Buff, 0, sizeof(char) * buffSize);
                dumpRead = fread(Buff, 1, buffSize, dumpPtr);
                if(dumpRead <=0 || dumpRead != buffSize){
                    ALOGE("Failed to read driver dump file at once, dumpRead = %d", dumpRead);
                    rewind(dumpPtr);
                    remained = (int)buffSize;
                    buffer = Buff;
                    while(remained){
                        u32 readBytes = 0;
                        if(remained >= MEM_DUMP_CHUNK_SIZE)
                            readBytes = MEM_DUMP_CHUNK_SIZE;
                        else
                            readBytes = remained;
                        dumpRead = fread(buffer, 1, readBytes, dumpPtr);
                        if(dumpRead){
                            remained -= readBytes;
                            buffer += readBytes;
                            ALOGE("Read successfully, readBytes = %d, remained = %d", readBytes, remained);
                        } else {
                            ALOGE("Chunk read failed");
                            mStatus = WIFI_ERROR_NOT_SUPPORTED;
                        }
                    }
                }
                if(Callbacks.on_driver_memory_dump) {
                    (*Callbacks.on_driver_memory_dump)(Buff, buffSize);
                }
                break;
            }
            case FW_VERSION:
            case DRV_VERSION:
            case RING_DATA:
            case RING_STATUS:
            case GET_FEATURE:
            case START_LOGGING:
                ALOGE("Not supported type");
                break;
        }
        if(Buff)
            free(Buff);
        if(dumpPtr) {
           fclose(dumpPtr);
           dumpPtr = NULL;
        }
        return NL_OK;
    }

    int getStatus() {
        return mStatus;
    }

    virtual int handleEvent(WifiEvent& event) {
        /* NO events! */
        return NL_SKIP;
    }
};

/////////////////////////////////////////////////////////////////////////////////

wifi_error wifi_get_firmware_memory_dump(wifi_interface_handle iface,
                                                    wifi_firmware_memory_dump_handler handler)
{
    int ret;
    u32 status = 0;
    MemoryDumpCommand *FwDump = new MemoryDumpCommand(iface, handler, FW_MEM_DUMP);
    ret = FwDump->start();
    status = FwDump->getStatus();
    ALOGV("get firmware dump ret = %d", ret);
    ALOGV("get firmware status = %d", status);
    if (ret != WIFI_SUCCESS || status != 0){
        ret = WIFI_ERROR_NOT_SUPPORTED;
        ALOGE("get firmware dump not supported.");   
    }
    return (wifi_error)ret;
}

wifi_error wifi_get_driver_memory_dump(wifi_interface_handle iface,
                                                 wifi_driver_memory_dump_callbacks callbacks)
{
     int ret;
     u32 status = 0;
     MemoryDumpCommand *DrvDump = new MemoryDumpCommand(iface, callbacks, DRV_MEM_DUMP);
     ret = DrvDump->start();
     status = DrvDump->getStatus();
     ALOGV("get driver ret = %d", ret);
     ALOGV("get driver status = %d", status);
     if (ret != WIFI_SUCCESS || status != 0){
         ret = WIFI_ERROR_NOT_SUPPORTED;
         ALOGE("get driver dump not supported.");
     }
     return (wifi_error)ret;
}

wifi_error wifi_get_wake_reason_stats(wifi_interface_handle iface, 
                                      WLAN_DRIVER_WAKE_REASON_CNT *wifi_wake_reason_cnt)
{
    wifi_handle handle = getWifiHandle(iface);
    hal_info *info = getHalInfo(handle);
    WLAN_DRIVER_WAKE_REASON_CNT *pos = info->wifi_wake_reason_cnt;
    wifi_wake_reason_cnt->total_cmd_event_wake = pos->total_cmd_event_wake;
    wifi_wake_reason_cnt->cmd_event_wake_cnt = pos->cmd_event_wake_cnt;
    wifi_wake_reason_cnt->cmd_event_wake_cnt_sz = pos->cmd_event_wake_cnt_sz;
    wifi_wake_reason_cnt->cmd_event_wake_cnt_used = pos->cmd_event_wake_cnt_used;
    wifi_wake_reason_cnt->total_driver_fw_local_wake = pos->total_driver_fw_local_wake;
    wifi_wake_reason_cnt->driver_fw_local_wake_cnt = pos->driver_fw_local_wake_cnt;
    wifi_wake_reason_cnt->driver_fw_local_wake_cnt_sz = pos->driver_fw_local_wake_cnt_sz;
    wifi_wake_reason_cnt->driver_fw_local_wake_cnt_used = pos->driver_fw_local_wake_cnt_used;
    wifi_wake_reason_cnt->total_rx_data_wake = pos->total_rx_data_wake;
    wifi_wake_reason_cnt->rx_wake_details.rx_unicast_cnt = pos->rx_wake_details.rx_unicast_cnt;
    wifi_wake_reason_cnt->rx_wake_details.rx_multicast_cnt = pos->rx_wake_details.rx_multicast_cnt;
    wifi_wake_reason_cnt->rx_wake_details.rx_broadcast_cnt = pos->rx_wake_details.rx_broadcast_cnt;
    wifi_wake_reason_cnt->rx_wake_pkt_classification_info.icmp_pkt = pos->rx_wake_pkt_classification_info.icmp_pkt;
    wifi_wake_reason_cnt->rx_wake_pkt_classification_info.icmp6_pkt = pos->rx_wake_pkt_classification_info.icmp6_pkt;
    wifi_wake_reason_cnt->rx_wake_pkt_classification_info.icmp6_ra = pos->rx_wake_pkt_classification_info.icmp6_ra;
    wifi_wake_reason_cnt->rx_wake_pkt_classification_info.icmp6_na = pos->rx_wake_pkt_classification_info.icmp6_na;
    wifi_wake_reason_cnt->rx_wake_pkt_classification_info.icmp6_ns = pos->rx_wake_pkt_classification_info.icmp6_ns;
    wifi_wake_reason_cnt->rx_multicast_wake_pkt_info.ipv4_rx_multicast_addr_cnt = 
                        pos->rx_multicast_wake_pkt_info.ipv4_rx_multicast_addr_cnt;
    wifi_wake_reason_cnt->rx_multicast_wake_pkt_info.ipv6_rx_multicast_addr_cnt = 
                        pos->rx_multicast_wake_pkt_info.ipv6_rx_multicast_addr_cnt;
    wifi_wake_reason_cnt->rx_multicast_wake_pkt_info.other_rx_multicast_addr_cnt = 
                        pos->rx_multicast_wake_pkt_info.other_rx_multicast_addr_cnt;
    return WIFI_SUCCESS;
}

wifi_error wifi_get_tx_pkt_fates(wifi_interface_handle iface,
                                 wifi_tx_report *tx_report_bufs,
                                 size_t n_requested_fates,
                                 size_t *n_provided_fates)
{
    wifi_handle wifiHandle = getWifiHandle(iface);
    hal_info *info = getHalInfo(wifiHandle);
    wifi_tx_report_i *tx_fate_stats;
    size_t i;

    if (info->fate_monitoring_enabled != true) {
        ALOGE("Packet monitoring is not yet triggered");
        return WIFI_ERROR_UNINITIALIZED;
    }
    pthread_mutex_lock(&info->pkt_fate_stats_lock);

    tx_fate_stats = &info->pkt_fate_stats->tx_fate_stats[0];

    *n_provided_fates = min(n_requested_fates,
                            info->pkt_fate_stats->n_tx_stats_collected);

    for (i=0; i < *n_provided_fates; i++) {
        memcpy(tx_report_bufs[i].md5_prefix,
                    tx_fate_stats[i].md5_prefix, MD5_PREFIX_LEN);
        tx_report_bufs[i].fate = tx_fate_stats[i].fate;
        tx_report_bufs[i].frame_inf.payload_type =
            tx_fate_stats[i].frame_inf.payload_type;
        tx_report_bufs[i].frame_inf.driver_timestamp_usec =
            tx_fate_stats[i].frame_inf.driver_timestamp_usec;
        tx_report_bufs[i].frame_inf.firmware_timestamp_usec =
            tx_fate_stats[i].frame_inf.firmware_timestamp_usec;
        tx_report_bufs[i].frame_inf.frame_len =
            tx_fate_stats[i].frame_inf.frame_len;

        if (tx_report_bufs[i].frame_inf.payload_type == FRAME_TYPE_ETHERNET_II)
            memcpy(tx_report_bufs[i].frame_inf.frame_content.ethernet_ii_bytes,
                   tx_fate_stats[i].frame_inf.frame_content,
                   min(tx_fate_stats[i].frame_inf.frame_len,
                       MAX_FRAME_LEN_ETHERNET));
        else if (tx_report_bufs[i].frame_inf.payload_type ==
                                                         FRAME_TYPE_80211_MGMT)
            memcpy(
                tx_report_bufs[i].frame_inf.frame_content.ieee_80211_mgmt_bytes,
                tx_fate_stats[i].frame_inf.frame_content,
                min(tx_fate_stats[i].frame_inf.frame_len,
                    MAX_FRAME_LEN_80211_MGMT));
        else
            /* Currently framework is interested only two types(
             * FRAME_TYPE_ETHERNET_II and FRAME_TYPE_80211_MGMT) of packets, so
             * ignore the all other types of packets received from driver */

            ALOGE("Unknown format packet");
    }
    pthread_mutex_unlock(&info->pkt_fate_stats_lock);

    return WIFI_SUCCESS;
}

wifi_error wifi_get_rx_pkt_fates(wifi_interface_handle iface,
                                 wifi_rx_report *rx_report_bufs,
                                 size_t n_requested_fates,
                                 size_t *n_provided_fates)
{
    wifi_handle wifiHandle = getWifiHandle(iface);
    hal_info *info = getHalInfo(wifiHandle);
    wifi_rx_report_i *rx_fate_stats;
    size_t i;

    if (info->fate_monitoring_enabled != true) {
        ALOGE("Packet monitoring is not yet triggered");
        return WIFI_ERROR_UNINITIALIZED;
    }
    pthread_mutex_lock(&info->pkt_fate_stats_lock);

    rx_fate_stats = &info->pkt_fate_stats->rx_fate_stats[0];

    *n_provided_fates = min(n_requested_fates,
                            info->pkt_fate_stats->n_rx_stats_collected);

    for (i=0; i < *n_provided_fates; i++) {
        memcpy(rx_report_bufs[i].md5_prefix,
                    rx_fate_stats[i].md5_prefix, MD5_PREFIX_LEN);
        rx_report_bufs[i].fate = rx_fate_stats[i].fate;
        rx_report_bufs[i].frame_inf.payload_type =
            rx_fate_stats[i].frame_inf.payload_type;
        rx_report_bufs[i].frame_inf.driver_timestamp_usec =
            rx_fate_stats[i].frame_inf.driver_timestamp_usec;
        rx_report_bufs[i].frame_inf.firmware_timestamp_usec =
            rx_fate_stats[i].frame_inf.firmware_timestamp_usec;
        rx_report_bufs[i].frame_inf.frame_len =
            rx_fate_stats[i].frame_inf.frame_len;

        if (rx_report_bufs[i].frame_inf.payload_type == FRAME_TYPE_ETHERNET_II)
            memcpy(rx_report_bufs[i].frame_inf.frame_content.ethernet_ii_bytes,
                   rx_fate_stats[i].frame_inf.frame_content,
                   min(rx_fate_stats[i].frame_inf.frame_len,
                   MAX_FRAME_LEN_ETHERNET));
        else if (rx_report_bufs[i].frame_inf.payload_type ==
                                                         FRAME_TYPE_80211_MGMT)
            memcpy(
                rx_report_bufs[i].frame_inf.frame_content.ieee_80211_mgmt_bytes,
                rx_fate_stats[i].frame_inf.frame_content,
                min(rx_fate_stats[i].frame_inf.frame_len,
                    MAX_FRAME_LEN_80211_MGMT));
        else
            /* Currently framework is interested only two types(
             * FRAME_TYPE_ETHERNET_II and FRAME_TYPE_80211_MGMT) of packets, so
             * ignore the all other types of packets received from driver */

            ALOGE("Unknown format packet");
    }
    pthread_mutex_unlock(&info->pkt_fate_stats_lock);
    return WIFI_SUCCESS;
}

