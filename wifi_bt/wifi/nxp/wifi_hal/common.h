/*
 * Copyright (C) 2017 The Android Open Source Project
 * Portions copyright (C) 2017 Broadcom Limited
 * Portions copyright 2015-2021 NXP
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

#include "wifi_hal.h"

#ifndef __WIFI_HAL_COMMON_H__
#define __WIFI_HAL_COMMON_H__

#define LOG_TAG  "WifiHAL"

#include <pthread.h>
#include "nl80211_copy.h"
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
#include <utils/Log.h>

#include <linux/pkt_sched.h>
#include <netlink/object-api.h>
#include <netlink/netlink.h>
#include <netlink/socket.h>
#include <netlink-private/object-api.h>
#include <netlink-private/types.h>
#include <stddef.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <iostream>
#include <new>
#include <stdarg.h>
#include "pkt_stats.h"

#define WIFI_HAL_VERSION      "008.001"

// some common macros
#define min(x, y)       ((x) < (y) ? (x) : (y))
#define max(x, y)       ((x) > (y) ? (x) : (y))

/*
  Mutest define
*/
class Mutex
{
private:
    pthread_mutex_t mMutex;
public:
    Mutex() {
        pthread_mutex_init(&mMutex, NULL);
    }
    ~Mutex() {
        pthread_mutex_destroy(&mMutex);
    }
    int tryLock() {
        return pthread_mutex_trylock(&mMutex);
    }
    int lock() {
        return pthread_mutex_lock(&mMutex);
    }
    void unlock() {
        pthread_mutex_unlock(&mMutex);
    }
};

/*
  Define Condition
*/
class Condition
{
private:
    pthread_cond_t mCondition;
    pthread_mutex_t mMutex;

public:
    Condition() {
        pthread_mutex_init(&mMutex, NULL);
        pthread_cond_init(&mCondition, NULL);
    }
    ~Condition() {
        pthread_cond_destroy(&mCondition);
        pthread_mutex_destroy(&mMutex);
    }

    int wait() {
        return pthread_cond_wait(&mCondition, &mMutex);
    }

    void signal() {
        pthread_cond_signal(&mCondition);
    }
};

#define SOCKET_BUFFER_SIZE      (32768U)
#define RECV_BUF_SIZE           (4096)
#define DEFAULT_EVENT_CB_SIZE   (64)
#define DEFAULT_CMD_SIZE        (64)
#define MAC_OUI_LEN             3


/* libnl 2.0 compatibility code */
#define nl_handle nl_sock
#define nl80211_handle_alloc nl_socket_alloc_cb
#define nl80211_handle_destroy nl_socket_free

const uint32_t MARVELL_OUI = 0x005043;

/* vendor commands define */
typedef enum {
    /* don't use 0 as a valid subcommand */
    VENDOR_NL80211_SUBCMD_UNSPECIFIED,

    /* define all vendor startup commands between 0x0 and 0x0FFF */
    VENDOR_NL80211_SUBCMD_RANGE_START  = 0x0001,
    VENDOR_NL80211_SUBCMD_RANGE_END    = 0x0FFF,
    NXP_OFFLOAD_START_SENDING_PACKET  = 0x0003,
    NXP_OFFLOAD_STOP_SENDING_PACKET   = 0x0004,
    NXP_SUBCMD_SCAN_MAC_OUI           = 0x0007,
    NXP_SUBCMD_SET_PACKET_FILTER      = 0x0011,
    NXP_SUBCMD_GET_PACKET_FILTER_CAPA = 0x0012,
    NXP_SUBCMD_CONFIGURE_ND_OFFLOAD   = 0x0100,

    /* define all GScan related commands between 0x1000 and 0x10FF */
    NXP_SUBCMD_GSCAN_RANGE_START      = 0x1000,
    GSCAN_SUBCMD_GET_CAPABILITIES      = NXP_SUBCMD_GSCAN_RANGE_START,
    GSCAN_SUBCMD_SET_CONFIG,                            /* 0x1001 */
    GSCAN_SUBCMD_SET_SCAN_CONFIG,                       /* 0x1002 */
    GSCAN_SUBCMD_ENABLE_GSCAN,                          /* 0x1003 */
    GSCAN_SUBCMD_GET_SCAN_RESULTS,                      /* 0x1004 */
    GSCAN_SUBCMD_SCAN_RESULTS,                          /* 0x1005 */
    GSCAN_SUBCMD_SET_HOTLIST,                           /* 0x1006 */
    GSCAN_SUBCMD_SET_SIGNIFICANT_CHANGE_CONFIG,         /* 0x1007 */
    GSCAN_SUBCMD_ENABLE_FULL_SCAN_RESULTS,              /* 0x1008 */
    NXP_SUBCMD_GET_VALID_CHANNELS,                     /* 0x1009 */
    NXP_SUBCMD_GET_SUPPORTED_FEATURE_SET,               /* 0x100A */
    NXP_SUBCMD_GET_CONCURRENCY_MATRIX,                  /* 0x100B */
    NXP_SUBCMD_SET_NODFS_FLAG,                          /* 0x100C */
    NXP_SUBCMD_SET_COUNTRY_CODE,                        /* 0x100D */
    GSCAN_SUBCMD_SET_EPNO_SSID,                          /* 0x100E */
    WIFI_SUBCMD_SET_SSID_WHITE_LIST,                    /* 0x100F */
    WIFI_SUBCMD_SET_ROAM_PARAMS,                        /* 0x1010 */
    WIFI_SUBCMD_ENABLE_LAZY_ROAM,                       /* 0x1011 */
    WIFI_SUBCMD_SET_BSSID_PREF,                         /* 0x1012 */
    WIFI_SUBCMD_SET_BSSID_BLACKLIST,                     /* 0x1013 */
    GSCAN_SUBCMD_ANQPO_CONFIG,                          /* 0x1014 */
    /* Add more sub commands here */
    GSCAN_SUBCMD_MAX,
    
    /* define all RTT related commands between 0x1100 and 0x11FF */
    NXP_SUBCMD_RTT_GET_CAPA = 0x1100,
    NXP_SUBCMD_RTT_RANGE_REQUEST,
    NXP_SUBCMD_RTT_RANGE_CANCEL,
    NXP_SUBCMD_RTT_GET_RESPONDER_INFO,
    NXP_SUBCMD_RTT_ENABLE_RESPONDER,
    NXP_SUBCMD_RTT_DISABLE_RESPONDER,
    NXP_SUBCMD_RTT_SET_LCI,
    NXP_SUBCMD_RTT_SET_LCR,
    
    /* define all link layer related commands between 0x1200 and 0x12FF */
    NXP_SUBCMD_LL_STATS_SET = 0x1200,
    NXP_SUBCMD_LL_STATS_GET,
    NXP_SUBCMD_LL_STATS_CLR,
    
    /* define all Logger related commands between 0x1400 and 0x14FF */
    NXP_SUBCMD_START_LOGGING = 0x1400,
    NXP_SUBCMD_GET_LOGGER_FEATURE_SET,
    NXP_SUBCMD_GET_RING_DATA,
    NXP_SUBCMD_GET_RING_STATUS,
    NXP_SUBCMD_GET_FW_VERSION,
    NXP_SUBCMD_GET_FW_MEM_DUMP,
    NXP_SUBCMD_GET_DRIVER_VERSION,
    NXP_SUBCMD_GET_DRIVER_MEM_DUMP,
    NXP_SUBCMD_START_PKT_FATE_MONITORING,
    
    /* define all RSSI related commands between 0x1500 and 0x15FF */
    NXP_SUBCMD_RSSI_MONITOR = 0x1500,
    NXP_EVENT_RSSI_MONITOR,
    
    /* define all roaming control commands between 0x1700 and 0x17FF */
    NXP_SUBCMD_GET_ROAMING_CAPA = 0x1700,
    NXP_SUBCMD_ENABLE_FW_ROAMING,
    NXP_SUBCMD_ROAMING_CONFIG,

    /* define all nan related commands between 0x1800 and 0x18FF */
    NXP_SUBCMD_NAN_ENABLE_REQ = 0x1800,
    NXP_SUBCMD_NAN_DISABLE_REQ,
    NXP_SUBCMD_NAN_PUBLISH_REQ,
    NXP_SUBCMD_NAN_PUBLISH_CANCEL,
    NXP_SUBCMD_NAN_SUBSCRIBE_REQ,
    NXP_SUBCMD_NAN_SUBSCRIBE_CANCEL,
    NXP_SUBCMD_NAN_TRANSMIT_FOLLOWUP,
    NXP_SUBCMD_NAN_STATS_REQ,
    NXP_SUBCMD_NAN_CONFIG_REQ,
    NXP_SUBCMD_NAN_TCA_REQ,
    NXP_SUBCMD_NAN_BEACON_SDF_PAYLOAD,
    NXP_SUBCMD_NAN_GET_CAPA,
    NXP_SUBCMD_NAN_DATA_IF_CREATE,
    NXP_SUBCMD_NAN_DATA_IF_DELETE,
    NXP_SUBCMD_NAN_DATA_REQ_INITOR,
    NXP_SUBCMD_NAN_DATA_INDI_RESP,
    NXP_SUBCMD_NAN_DATA_END,
    
    /* This is reserved for future usage */
} ANDROID_VENDOR_SUB_COMMAND;

typedef enum {
    RESERVED1,
    RESERVED2,
    GSCAN_EVENT_SIGNIFICANT_CHANGE_RESULTS ,
    GSCAN_EVENT_HOTLIST_RESULTS_FOUND,
    GSCAN_EVENT_SCAN_RESULTS_AVAILABLE,
    GSCAN_EVENT_FULL_SCAN_RESULTS,
    RTT_EVENT_COMPLETE,
    NXP_EVENT_RTT_RANGE_RESULT,
    GSCAN_EVENT_COMPLETE_SCAN,
    GSCAN_EVENT_HOTLIST_RESULTS_LOST,
    GSCAN_EVENT_EPNO_EVENT,
    GSCAN_EVENT_ANQPO_HOTSPOT_MATCH,
    GOOGLE_RSSI_MONITOR_EVENT
} WIFI_EVENT;

typedef void (*wifi_internal_event_handler) (wifi_handle handle, int events);

class WifiCommand;

typedef struct {
    int nl_cmd;
    uint32_t vendor_id;
    int vendor_subcmd;
    nl_recvmsg_msg_cb_t cb_func;
    void *cb_arg;
} cb_info;

typedef struct {
    wifi_request_id id;
    WifiCommand *cmd;
} cmd_info;

typedef struct {
    wifi_handle handle;                             // handle to wifi data
    char name[8+1];                                 // interface name + trailing null
    int  id;                                        // id to use when talking to driver
} interface_info;

typedef struct {

    struct nl_sock *cmd_sock;                       // command socket object
    struct nl_sock *event_sock;                     // event socket object
    int nl80211_family_id;                          // family id for 80211 driver
    int cleanup_socks[2];                           // sockets used to implement wifi_cleanup

    bool in_event_loop;                             // Indicates that event loop is active
    bool clean_up;                                  // Indication to clean up the socket

    wifi_internal_event_handler event_handler;      // default event handler
    wifi_cleaned_up_handler cleaned_up_handler;     // socket cleaned up handler

    cb_info *event_cb;                              // event callbacks
    int num_event_cb;                               // number of event callbacks
    int alloc_event_cb;                             // number of allocated callback objects
    pthread_mutex_t cb_lock;                        // mutex for the event_cb access

    cmd_info *cmd;                                  // Outstanding commands
    int num_cmd;                                    // number of commands
    int alloc_cmd;                                  // number of commands allocated

    bool fate_monitoring_enabled;
    packet_fate_monitor_info *pkt_fate_stats;
    pthread_mutex_t pkt_fate_stats_lock;

    interface_info **interfaces;                    // array of interfaces
    int num_interfaces;                             // number of interfaces
    WLAN_DRIVER_WAKE_REASON_CNT *wifi_wake_reason_cnt;
} hal_info;

//#define PNO_SSID_FOUND  0x1
//#define PNO_SSID_LOST    0x2

/*typedef struct wifi_pno_result {
    unsigned char ssid[32];
    unsigned char ssid_len;
    signed char rssi;
    u16 channel;
    u16 flags;
    mac_addr  bssid;
} wifi_pno_result_t;*/

wifi_error wifi_register_handler(wifi_handle handle, int cmd, nl_recvmsg_msg_cb_t func, void *arg);
wifi_error wifi_register_vendor_handler(wifi_handle handle,
            uint32_t id, int subcmd, nl_recvmsg_msg_cb_t func, void *arg);
void wifi_unregister_handler(wifi_handle handle, int cmd);
void wifi_unregister_vendor_handler(wifi_handle handle, uint32_t id, int subcmd);
wifi_error wifi_register_cmd(wifi_handle handle, int id, WifiCommand *cmd);
WifiCommand *wifi_unregister_cmd(wifi_handle handle, int id);
WifiCommand *wifi_get_cmd(wifi_handle handle, int id);
void wifi_unregister_cmd(wifi_handle handle, WifiCommand *cmd);
interface_info *getIfaceInfo(wifi_interface_handle);
wifi_handle getWifiHandle(wifi_interface_handle handle);
hal_info *getHalInfo(wifi_handle handle);
hal_info *getHalInfo(wifi_interface_handle handle);
wifi_handle getWifiHandle(hal_info *info);
wifi_interface_handle getIfaceHandle(interface_info *info);
wifi_error wifi_cancel_cmd(wifi_request_id id, wifi_interface_handle iface);
extern wifi_error wifi_configure_nd_offload(wifi_interface_handle iface, u8 enable);
extern wifi_error wifi_start_rssi_monitoring(wifi_request_id id, wifi_interface_handle
                        iface, s8 max_rssi, s8 min_rssi, wifi_rssi_event_handler eh);
extern wifi_error wifi_stop_rssi_monitoring(wifi_request_id id, wifi_interface_handle iface);
/**
* API to set packe filter
* @param program        pointer to the program byte-code
* @param len               length of the program byte_code 
*/
extern wifi_error wifi_set_packet_filter(wifi_interface_handle iface, const u8* program, u32 len);

/**
* API to get packet filter capabilities. Returns the chipset's hardware filtering capabilities
* @param version         pointer to version of the packet filter interpreter supported, filled in upon return. 0 indicates no support
* @param max_len       pointer to maximum size of the filter bytecode, filled in upon return
*/
extern wifi_error wifi_get_packet_filter_capabilities(wifi_interface_handle handle, u32* version, u32* max_len);
extern wifi_error wifi_get_wake_reason_stats(wifi_interface_handle iface, 
                                            WLAN_DRIVER_WAKE_REASON_CNT *wifi_wake_reason_cnt);

void hexdump(void *bytes, byte len);

#endif

