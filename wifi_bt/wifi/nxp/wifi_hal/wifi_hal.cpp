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
#include <errno.h>

#include <linux/pkt_sched.h>
#include <netlink/object-api.h>
#include <netlink/netlink.h>
#include <netlink/socket.h>
#include <netlink/attr.h>
#include <netlink/msg.h>

#include <dirent.h>
#include <net/if.h>

#include "common.h"

#define LOG_TAG  "WifiHAL"

#include <utils/Log.h>

#include "wifi_hal.h"
#include "common.h"
#include "cpp_bindings.h"
#include "rtt.h"
#include "roam.h"
/*
 BUGBUG: normally, libnl allocates ports for all connections it makes; but
 being a static library, it doesn't really know how many other netlink connections
 are made by the same process, if connections come from different shared libraries.
 These port assignments exist to solve that problem - temporarily. We need to fix
 libnl to try and allocate ports across the entire process.
 */

#define WIFI_HAL_CMD_SOCK_PORT       644
#define WIFI_HAL_EVENT_SOCK_PORT     645

/*
* Defines for wifi_wait_for_driver_ready()
* Specify durations between polls and max wait time 
*/
#define POLL_DRIVER_DURATION_US (100000)  /*100ms*/
#define POLL_DRIVER_MAX_TIME_MS (20000)  /*20s*/


static void internal_event_handler(wifi_handle handle, int events);
static int internal_no_seq_check(nl_msg *msg, void *arg);
static int internal_valid_message_handler(nl_msg *msg, void *arg);
static int wifi_get_multicast_id(wifi_handle handle, const char *name, const char *group);
static int wifi_add_membership(wifi_handle handle, const char *group);
static wifi_error wifi_init_interfaces(wifi_handle handle);
wifi_error wifi_configure_nd_offload(wifi_interface_handle iface, u8 enable);
wifi_error wifi_start_rssi_monitoring(wifi_request_id id, wifi_interface_handle iface, s8 max_rssi,
                                           s8 min_rssi, wifi_rssi_event_handler eh);
wifi_error wifi_stop_rssi_monitoring(wifi_request_id id, wifi_interface_handle iface);

/**
* API to set packe filter
* @param program        pointer to the program byte-code
* @param len               length of the program byte_code 
*/
wifi_error wifi_set_packet_filter(wifi_interface_handle iface, const u8* program, u32 len);
wifi_error wifi_start_wake_reason_cnt(hal_info * info);

/**
* API to get packet filter capabilities. Returns the chipset's hardware filtering capabilities
* @param version         pointer to version of the packet filter interpreter supported, filled in upon return. 0 indicates no support
* @param max_len       pointer to maximum size of the filter bytecode, filled in upon return
*/
wifi_error wifi_get_packet_filter_capabilities(wifi_interface_handle handle, u32* version, u32* max_len);
wifi_error wifi_get_wake_reason_stats(wifi_interface_handle iface, 
                                            WLAN_DRIVER_WAKE_REASON_CNT *wifi_wake_reason_cnt);
wifi_error wifi_get_valid_channels(wifi_interface_handle handle, int band, int max_channels, 
                                        wifi_channel *channels, int *num_channels);

typedef enum wifi_attr {
    NXP_ATTR_WIFI_COMMON_INVALID = 0,
    NXP_ATTR_SET_SCAN_MAC_OUI,
    NXP_ATTR_SUPPORTED_FEATURE_SET,
    NXP_ATTR_NODFS,
    NXP_ATTR_COUNTRY_CODE,
    NXP_ATTR_VALID_CHANNELS_BAND,
    NXP_ATTR_VALID_CHANNELS_NUM_CHANNELS,
    NXP_ATTR_VALID_CHANNEL_LIST,
    NXP_ATTR_CONCURRENCY_MATRIX_SET_SIZE_MAX,
    NXP_ATTR_CONCURRENCY_MATRIX_SET_SIZE,
    NXP_ATTR_CONCURRENCY_MATRIX_SET,

    NXP_ATTR_WIFI_COMMON_AFTER_LAST,
    NXP_ATTR_WIFI_COMMON_MAX =
    NXP_ATTR_WIFI_COMMON_AFTER_LAST - 1,
} wifi_attr_t;

enum wifi_rssi_monitor_attr {
    NXP_ATTR_RSSI_MONITOR_INVALID = 0,
    NXP_ATTR_RSSI_MONITOR_CONTROL,
    NXP_ATTR_RSSI_MONITOR_MIN_RSSI,
    NXP_ATTR_RSSI_MONITOR_MAX_RSSI,
    NXP_ATTR_RSSI_MONITOR_CUR_BSSID,
    NXP_ATTR_RSSI_MONITOR_CUR_RSSI,

    /* keep last */
    NXP_ATTR_RSSI_MONITOR_AFTER_LAST,
    NXP_ATTR_RSSI_MONITOR_MAX =
    NXP_ATTR_RSSI_MONITOR_AFTER_LAST - 1,
};

enum wifi_attr_nd_offload
{
    NXP_WIFI_ATTR_ND_OFFLOAD_INVALID = 0,
    NXP_ATTR_ND_OFFLOAD_CONTROL,

    /* keep last */
    NXP_WIFI_ATTR_ND_OFFLOAD_AFTER_LAST,
    NXP_WIFI_ATTR_ND_OFFLOAD_MAX =
    NXP_WIFI_ATTR_ND_OFFLOAD_AFTER_LAST - 1,
};

enum wifi_attr_packet_filter 
{
    NXP_ATTR_PACKET_FILTER_INVALID = 0,
    NXP_ATTR_PACKET_FILTER_TOTAL_LENGTH,
    NXP_ATTR_PACKET_FILTER_PROGRAM,
    NXP_ATTR_PACKET_FILTER_VERSION,
    NXP_ATTR_PACKET_FILTER_MAX_LEN,
    NXP_ATTR_PACKET_FILTER_MAX,
};

/* Initialize/Cleanup */

wifi_interface_handle wifi_get_iface_handle(wifi_handle handle, char *name)
{
    hal_info *info = (hal_info *)handle;
    for (int i=0;i<info->num_interfaces;i++)
    {
        if (!strcmp(info->interfaces[i]->name, name))
        {
            return ((wifi_interface_handle )(info->interfaces)[i]);
        }
    }
    return NULL;
}

void wifi_socket_set_local_port(struct nl_sock *sock, uint32_t port)
{
    uint32_t pid = getpid() & 0x3FFFFF;
    ALOGV("pid = %d", pid);
    nl_socket_set_local_port(sock, pid + (port << 22));
}

static nl_sock * wifi_create_nl_socket(int port)
{
    ALOGV("create cmd socket, port = %d", port);
    struct nl_sock *sock = nl_socket_alloc();
    if (sock == NULL) {
        ALOGE("Could not create handle");
        return NULL;
    }

    wifi_socket_set_local_port(sock, port);

    struct sockaddr *addr = NULL;

    if (nl_connect(sock, NETLINK_GENERIC)) {
        ALOGE("Could not connect handle");
        nl_socket_free(sock);
        return NULL;
    }

    if (nl_socket_set_nonblocking(sock)) {
        ALOGE("Could not make socket non-blocking");
        nl_socket_free(sock);
        return NULL;
    }
    
    return sock;
}


/*initialize function pointer table with Broadcom HHAL API*/
wifi_error init_wifi_vendor_hal_func_table(wifi_hal_fn *fn)
{
    ALOGV("***init_wifi_vendor_hal_func_table***");
    if (fn == NULL) {
        return WIFI_ERROR_UNKNOWN;
    }
    fn->wifi_initialize = wifi_initialize;
    fn->wifi_wait_for_driver_ready = wifi_wait_for_driver_ready;
    fn->wifi_cleanup = wifi_cleanup;
    fn->wifi_event_loop = wifi_event_loop;
    fn->wifi_get_supported_feature_set = wifi_get_supported_feature_set;
    fn->wifi_get_concurrency_matrix = wifi_get_concurrency_matrix;
    fn->wifi_set_scanning_mac_oui =  wifi_set_scanning_mac_oui;
    fn->wifi_get_ifaces = wifi_get_ifaces;
    fn->wifi_get_iface_name = wifi_get_iface_name;
    fn->wifi_set_link_stats = wifi_set_link_stats;
    fn->wifi_get_link_stats = wifi_get_link_stats;
    fn->wifi_clear_link_stats = wifi_clear_link_stats;
    fn->wifi_get_valid_channels = wifi_get_valid_channels;
    fn->wifi_rtt_range_request = wifi_rtt_range_request;
    fn->wifi_rtt_range_cancel = wifi_rtt_range_cancel;
    fn->wifi_get_rtt_capabilities = wifi_get_rtt_capabilities;
    fn->wifi_rtt_get_responder_info = wifi_rtt_get_responder_info;
    fn->wifi_enable_responder = wifi_enable_responder;
    fn->wifi_disable_responder = wifi_disable_responder;
    fn->wifi_set_lci = wifi_set_lci;
    fn->wifi_set_lcr = wifi_set_lcr;
    fn->wifi_set_nodfs_flag = wifi_set_nodfs_flag;
    fn->wifi_start_logging = wifi_start_logging;
    fn->wifi_set_country_code = wifi_set_country_code;
    fn->wifi_get_firmware_memory_dump = wifi_get_firmware_memory_dump;
    fn->wifi_get_driver_memory_dump = wifi_get_driver_memory_dump;
    fn->wifi_set_log_handler = wifi_set_log_handler;
    fn->wifi_reset_log_handler = wifi_reset_log_handler;
    fn->wifi_set_alert_handler = wifi_set_alert_handler;
    fn->wifi_reset_alert_handler = wifi_reset_alert_handler;
    fn->wifi_get_firmware_version = wifi_get_firmware_version;
    fn->wifi_get_ring_buffers_status = wifi_get_ring_buffers_status;
    fn->wifi_get_logger_supported_feature_set = wifi_get_logger_supported_feature_set;
    fn->wifi_get_ring_data = wifi_get_ring_data;
    fn->wifi_get_driver_version = wifi_get_driver_version;
    fn->wifi_start_rssi_monitoring = wifi_start_rssi_monitoring;
    fn->wifi_stop_rssi_monitoring = wifi_stop_rssi_monitoring;
    fn->wifi_get_wake_reason_stats = wifi_get_wake_reason_stats;
    fn->wifi_configure_nd_offload = wifi_configure_nd_offload;
    fn->wifi_start_pkt_fate_monitoring = wifi_start_pkt_fate_monitoring;
    fn->wifi_get_tx_pkt_fates = wifi_get_tx_pkt_fates;
    fn->wifi_get_rx_pkt_fates = wifi_get_rx_pkt_fates;
    fn->wifi_nan_enable_request = nan_enable_request;
    fn->wifi_nan_disable_request = nan_disable_request;
    fn->wifi_nan_publish_request = nan_publish_request;
    fn->wifi_nan_publish_cancel_request= nan_publish_cancel_request;
    fn->wifi_nan_subscribe_request = nan_subscribe_request;
    fn->wifi_nan_subscribe_cancel_request = nan_subscribe_cancel_request;
    fn->wifi_nan_transmit_followup_request = nan_transmit_followup_request;
    fn->wifi_nan_stats_request = nan_stats_request;
    fn->wifi_nan_config_request = nan_config_request;
    fn->wifi_nan_tca_request = nan_tca_request;
    fn->wifi_nan_beacon_sdf_payload_request = nan_beacon_sdf_payload_request;
    fn->wifi_nan_register_handler = nan_register_handler;
    fn->wifi_nan_get_version = nan_get_version;
    fn->wifi_nan_get_capabilities = nan_get_capabilities;
    fn->wifi_nan_data_interface_create = nan_data_interface_create;
    fn->wifi_nan_data_interface_delete = nan_data_interface_delete;
    fn->wifi_nan_data_request_initiator = nan_data_request_initiator;
    fn->wifi_nan_data_indication_response = nan_data_indication_response;
    fn->wifi_nan_data_end = nan_data_end;
    fn->wifi_set_packet_filter = wifi_set_packet_filter;
    fn->wifi_get_packet_filter_capabilities = wifi_get_packet_filter_capabilities;
    fn->wifi_get_roaming_capabilities = wifi_get_roaming_capabilities;
    fn->wifi_enable_firmware_roaming = wifi_enable_firmware_roaming;
    fn->wifi_configure_roaming = wifi_configure_roaming;
    fn->wifi_start_sending_offloaded_packet = wifi_start_sending_offloaded_packet;
    fn->wifi_stop_sending_offloaded_packet = wifi_stop_sending_offloaded_packet;
    ALOGV("***over init_wifi_vendor_hal_func_table***");
    return WIFI_SUCCESS;
}

wifi_error wifi_initialize(wifi_handle *handle)
{
    struct nl_cb *cb = NULL;
    struct nl_sock *cmd_sock = NULL;
    struct nl_sock *event_sock = NULL;
    wifi_error ret = WIFI_SUCCESS;

    hal_info *info = (hal_info *)malloc(sizeof(hal_info));
    if (info == NULL) {
        ALOGE("Could not allocate hal_info");
        ret = WIFI_ERROR_OUT_OF_MEMORY;
        goto exit;
    }

    memset(info, 0, sizeof(*info));

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, info->cleanup_socks) == -1) {
        ALOGE("Could not create cleanup sockets");
        ret = WIFI_ERROR_UNKNOWN;
        goto exit;
    }

    cmd_sock = wifi_create_nl_socket(WIFI_HAL_CMD_SOCK_PORT);
    if (cmd_sock == NULL) {
        ALOGE("Could not create handle");
        ret = WIFI_ERROR_UNKNOWN;
        goto exit;
    }

    event_sock = wifi_create_nl_socket(WIFI_HAL_EVENT_SOCK_PORT);
    if (event_sock == NULL) {
        ALOGE("Could not create handle");
        ret = WIFI_ERROR_UNKNOWN;
        goto exit;
    }

    cb = nl_socket_get_cb(event_sock);
    if (cb == NULL) {
        ret = WIFI_ERROR_UNKNOWN;
        goto exit;
    }

    ALOGV("cb->refcnt = %d\n", cb->cb_refcnt);
    nl_cb_set(cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, internal_no_seq_check, info);
    nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, internal_valid_message_handler, info);
    nl_cb_put(cb);

    info->cmd_sock = cmd_sock;
    info->event_sock = event_sock;
    info->clean_up = false;
    info->in_event_loop = false;

    info->event_cb = (cb_info *)malloc(sizeof(cb_info) * DEFAULT_EVENT_CB_SIZE);
    if (info->event_cb == NULL) {
        ALOGE("Could not allocate event_cb");
        ret = WIFI_ERROR_OUT_OF_MEMORY;
        goto exit;
    }
    info->alloc_event_cb = DEFAULT_EVENT_CB_SIZE;
    info->num_event_cb = 0;

    info->cmd = (cmd_info *)malloc(sizeof(cmd_info) * DEFAULT_CMD_SIZE);
    if (info->cmd == NULL) {
        ALOGE("Could not allocate cmd info");
        ret = WIFI_ERROR_OUT_OF_MEMORY;
        goto exit;
    }
    info->alloc_cmd = DEFAULT_CMD_SIZE;
    info->num_cmd = 0;

    info->nl80211_family_id = genl_ctrl_resolve(cmd_sock, "nl80211");
    if (info->nl80211_family_id < 0) {
        ALOGE("Could not resolve nl80211 familty id");
        ret = WIFI_ERROR_UNKNOWN;
        goto exit;
    }

    pthread_mutex_init(&info->cb_lock, NULL);

    *handle = (wifi_handle) info;

    wifi_add_membership(*handle, "scan");
    wifi_add_membership(*handle, "mlme");
    wifi_add_membership(*handle, "regulatory");
    wifi_add_membership(*handle, "vendor");

    ret = wifi_init_interfaces(*handle);
    if (ret != WIFI_SUCCESS) {
        ALOGE("wifi_init_interfaces Failed");
        pthread_mutex_destroy(&info->cb_lock);
        ret = WIFI_ERROR_UNKNOWN;
        goto exit;
    }

    ret = wifi_start_wake_reason_cnt(info);
    if (ret != WIFI_SUCCESS) {
        ALOGE("Initializing Wake Reason Event Handler Failed");
        pthread_mutex_destroy(&info->cb_lock);
        ret = WIFI_ERROR_UNKNOWN;
        goto exit;
    }

    srand(getpid());

    ALOGV("Initialized Wifi HAL Successfully; vendor cmd = %d", NL80211_CMD_VENDOR);
exit:
    if (ret != WIFI_SUCCESS) {
        if (event_sock)
            nl_socket_free(event_sock);
        if (cmd_sock)
            nl_socket_free(cmd_sock);
        if (info) {
            if (info->cmd) free(info->cmd);
            if (info->event_cb) free(info->event_cb);
            free(info);
        }
    }

    return ret;
}

wifi_error wifi_wait_for_driver_ready(void)
{

    // This function will wait to make sure basic client netdev is created
    // Function times out after 20 seconds
    int count = (POLL_DRIVER_MAX_TIME_MS * 1000) / POLL_DRIVER_DURATION_US;
    FILE *fd;

    do {
        if ((fd = fopen("/sys/class/net/wlan0", "r")) != NULL) {
            fclose(fd);
            return WIFI_SUCCESS;
        }
        usleep(POLL_DRIVER_DURATION_US);
    } while(--count > 0);

    ALOGE("Timed out waiting on Driver ready ... ");
    return WIFI_ERROR_TIMED_OUT;
}



static int wifi_add_membership(wifi_handle handle, const char *group)
{
    hal_info *info = getHalInfo(handle);

    int id = wifi_get_multicast_id(handle, "nl80211", group);
    if (id < 0) {
        ALOGE("Could not find group %s", group);
        return id;
    }

    int ret = nl_socket_add_membership(info->event_sock, id);
    if (ret < 0) {
        ALOGE("Could not add membership to group %s", group);
    }

    ALOGV("Successfully added membership for group %s", group);
    return ret;
}

static void internal_cleaned_up_handler(wifi_handle handle)
{
    hal_info *info = getHalInfo(handle);
    wifi_cleaned_up_handler cleaned_up_handler = info->cleaned_up_handler;

    if (info->cmd_sock != 0) {
        close(info->cleanup_socks[0]);
        close(info->cleanup_socks[1]);
        nl_socket_free(info->cmd_sock);
        nl_socket_free(info->event_sock);
        info->cmd_sock = NULL;
        info->event_sock = NULL;
    }

    (*cleaned_up_handler)(handle);
    pthread_mutex_destroy(&info->cb_lock);
    free(info);

    ALOGV("Internal cleanup completed");
}

void wifi_cleanup(wifi_handle handle, wifi_cleaned_up_handler handler)
{
    hal_info *info = getHalInfo(handle);
    info->cleaned_up_handler = handler;
    info->clean_up = true;

    pthread_mutex_lock(&info->cb_lock);

    int bad_commands = 0;

    while (info->num_cmd > bad_commands) {
        int num_cmd = info->num_cmd;
        cmd_info *cmdi = &(info->cmd[bad_commands]);
        WifiCommand *cmd = cmdi->cmd;
        if (cmd != NULL) {
            pthread_mutex_unlock(&info->cb_lock);
            cmd->cancel();
            pthread_mutex_lock(&info->cb_lock);
            /* release reference added when command is saved */
            cmd->releaseRef();
            if (num_cmd == info->num_cmd) {
                bad_commands++;
            }
        }
    }

    for (int i = 0; i < info->num_event_cb; i++) {
        cb_info *cbi = &(info->event_cb[i]);
        WifiCommand *cmd = (WifiCommand *)cbi->cb_arg;
        ALOGE("Leaked command %p", cmd);
    }

    pthread_mutex_unlock(&info->cb_lock);
    WLAN_DRIVER_WAKE_REASON_CNT *wake_reason_stat = NULL;
    wake_reason_stat = info->wifi_wake_reason_cnt;
    free(wake_reason_stat->cmd_event_wake_cnt);
    wake_reason_stat->cmd_event_wake_cnt = NULL;
    free(wake_reason_stat->driver_fw_local_wake_cnt);
    wake_reason_stat->driver_fw_local_wake_cnt = NULL;
    free(info->wifi_wake_reason_cnt);
    info->wifi_wake_reason_cnt = NULL;
	
    if(info->pkt_fate_stats) {
        free(info->pkt_fate_stats);
        info->pkt_fate_stats = NULL;
    }
		

    if (write(info->cleanup_socks[0], "T", 1) < 1) {
        ALOGE("could not write to cleanup socket");
    } else {
        ALOGV("Wifi cleanup completed\n");
    }
}

static int internal_pollin_handler(wifi_handle handle)
{
    hal_info *info = getHalInfo(handle);
    struct nl_cb *cb = nl_socket_get_cb(info->event_sock);
    int res = nl_recvmsgs(info->event_sock, cb);
    nl_cb_put(cb);
    return res;
}

/* Run event handler */
void wifi_event_loop(wifi_handle handle)
{
    hal_info *info = getHalInfo(handle);
    if (info->in_event_loop) {
        return;
    } else {
        info->in_event_loop = true;
    }

    pollfd pfd[2];
    memset(&pfd[0], 0, sizeof(pollfd) * 2);

    pfd[0].fd = nl_socket_get_fd(info->event_sock);	
    pfd[0].events = POLLIN;
    pfd[1].fd = info->cleanup_socks[1];
    pfd[1].events = POLLIN;
    char buf[2048];
    /* TODO: Add support for timeouts */

    do {
        int timeout = -1;                   /* Infinite timeout */
        pfd[0].revents = 0;
        pfd[1].revents = 0;
        int result = poll(pfd, 2, timeout);
        if (result < 0) {
            ALOGE("Error polling socket");
        } else if (pfd[0].revents & POLLERR) {
            ALOGE("POLL Error; error no = %d", errno);
            int result2 = read(pfd[0].fd, buf, sizeof(buf));
            ALOGE("Read after POLL returned %d, error no = %d", result2, errno);
        } else if (pfd[0].revents & POLLHUP) {
            ALOGE("Remote side hung up");
            break;
        } else if (pfd[0].revents & POLLIN) {
            internal_pollin_handler(handle);
        } else if (pfd[1].revents & POLLIN) {
            ALOGV("Got a signal to exit!!!");
            int result2 = read(pfd[1].fd, buf, sizeof(buf));
            ALOGV("Read after POLL returned %d, error no = %d", result2, errno);
        } else {
            ALOGE("Unknown event - %0x, %0x", pfd[0].revents, pfd[1].revents);
        }
    } while (!info->clean_up);

    internal_cleaned_up_handler(handle);
}

///////////////////////////////////////////////////////////////////////////////////////

static int internal_no_seq_check(struct nl_msg *msg, void *arg)
{
    return NL_OK;
}

static int internal_valid_message_handler(nl_msg *msg, void *arg)
{

    wifi_handle handle = (wifi_handle)arg;
    hal_info *info = getHalInfo(handle);
    bool dispatched = false;
    WifiEvent event(msg);
    int res = event.parse();
    if (res < 0) {
        ALOGE("Failed to parse event: %d", res);
        return NL_SKIP;
    }

    int cmd = event.get_cmd();
    uint32_t vendor_id = 0;
    int subcmd = 0;
    if (cmd == NL80211_CMD_VENDOR) {
        vendor_id = event.get_u32(NL80211_ATTR_VENDOR_ID);
        subcmd = event.get_u32(NL80211_ATTR_VENDOR_SUBCMD);
        ALOGV("event received %s, vendor_id = 0x%0x, subcmd = 0x%0x",
                event.get_cmdString(), vendor_id, subcmd);
    } else {
        ALOGV("event received %s", event.get_cmdString());
    }

    pthread_mutex_lock(&info->cb_lock);

    for (int i = 0; i < info->num_event_cb; i++) {
        if (cmd == info->event_cb[i].nl_cmd) {
            if (cmd == NL80211_CMD_VENDOR
                && ((vendor_id != info->event_cb[i].vendor_id)
                || (subcmd != info->event_cb[i].vendor_subcmd)))
            {
                /* event for a different vendor, ignore it */
                continue;
            }
            cb_info *cbi = &(info->event_cb[i]);
            nl_recvmsg_msg_cb_t cb_func = cbi->cb_func;
            void *cb_arg = cbi->cb_arg;
            WifiCommand *cmd = (WifiCommand *)cbi->cb_arg;
            if (cmd != NULL) {
                cmd->addRef();
            }
            pthread_mutex_unlock(&info->cb_lock);
            (*cb_func)(msg, cb_arg);
            if (cmd != NULL) {
                cmd->releaseRef();
            }
            return NL_OK;
        }
    }

    pthread_mutex_unlock(&info->cb_lock);
    return NL_OK;
}

///////////////////////////////////////////////////////////////////////////////////////

class GetMulticastIdCommand : public WifiCommand
{
private:
    const char *mName;
    const char *mGroup;
    int   mId;
public:
    GetMulticastIdCommand(wifi_handle handle, const char *name, const char *group)
        : WifiCommand("GetMulticastIdCommand", handle, 0)
    {
        mName = name;
        mGroup = group;
        mId = -1;
    }

    int getId() {
        return mId;
    }

    virtual int create() {
        int nlctrlFamily = genl_ctrl_resolve(mInfo->cmd_sock, "nlctrl");
        ALOGV("ctrl family = %d", nlctrlFamily);
        int ret = mMsg.create(nlctrlFamily, CTRL_CMD_GETFAMILY, 0, 0);
        if (ret < 0) {
            return ret;
        }
        ret = mMsg.put_string(CTRL_ATTR_FAMILY_NAME, mName);
        return ret;
    }

    virtual int handleResponse(WifiEvent& reply) {

        struct nlattr **tb = reply.attributes();
        struct genlmsghdr *gnlh = reply.header();
        struct nlattr *mcgrp = NULL;
        int i;

        if (!tb[CTRL_ATTR_MCAST_GROUPS]) {
            ALOGE("No multicast groups found");
            return NL_SKIP;
        } else {
            ALOGV("Multicast groups attr size = %d", nla_len(tb[CTRL_ATTR_MCAST_GROUPS]));
        }

        for_each_attr(mcgrp, tb[CTRL_ATTR_MCAST_GROUPS], i) {

            struct nlattr *tb2[CTRL_ATTR_MCAST_GRP_MAX + 1];
            nla_parse(tb2, CTRL_ATTR_MCAST_GRP_MAX, (nlattr *)nla_data(mcgrp),
                nla_len(mcgrp), NULL);
            if (!tb2[CTRL_ATTR_MCAST_GRP_NAME] || !tb2[CTRL_ATTR_MCAST_GRP_ID]) {
                continue;
            }

            char *grpName = (char *)nla_data(tb2[CTRL_ATTR_MCAST_GRP_NAME]);
            int grpNameLen = nla_len(tb2[CTRL_ATTR_MCAST_GRP_NAME]);

            if (strncmp(grpName, mGroup, grpNameLen) != 0)
                continue;

            mId = nla_get_u32(tb2[CTRL_ATTR_MCAST_GRP_ID]);
            break;
        }

        return NL_SKIP;
    }

};

class SetScanningMacOui : public WifiCommand {
private:
    byte *Oui;

public:
    SetScanningMacOui(wifi_interface_handle handle, oui scan_oui)
        : WifiCommand("SetScanningMacOui", handle, 0)
    {
        Oui = scan_oui;
    }

    virtual int create() {
        int ret = 0;
        ret = mMsg.create(MARVELL_OUI, NXP_SUBCMD_SCAN_MAC_OUI);
        if (ret < 0) {
            ALOGE("Failed to create message to set scanning mac oui - %d", ret);
            return ret;
        }
        nlattr *data = mMsg.attr_start(NL80211_ATTR_VENDOR_DATA);
        ret = mMsg.put(NXP_ATTR_SET_SCAN_MAC_OUI, Oui, MAC_OUI_LEN);
        if (ret < 0) {
            return ret;
        }
        mMsg.attr_end(data);
        return WIFI_SUCCESS;
    }

    virtual int handleResponse(WifiEvent& reply) {
        return NL_SKIP;
    }
};

class SetNodfsFlag : public WifiCommand {
private:
    u32 nodFs;

public:
    SetNodfsFlag(wifi_interface_handle handle, u32 nodfs)
        : WifiCommand("SetNodfsFlag", handle, 0) 
    {
        nodFs = nodfs;
    }
    virtual int create() {
        int ret = 0;
        ret = mMsg.create(MARVELL_OUI, NXP_SUBCMD_SET_NODFS_FLAG);
        if (ret < 0) {
            ALOGE("Failed to create message to set nodfs flag - %d", ret);
            return ret;
        }

        nlattr *data = mMsg.attr_start(NL80211_ATTR_VENDOR_DATA);
        ret = mMsg.put_u32(NXP_ATTR_NODFS, nodFs);
        if (ret < 0) {
             ALOGE("Failed to put nodfs flag");
             return ret;
        }

        mMsg.attr_end(data);
        return WIFI_SUCCESS;
    }
};


class SetCountryCodeCommand : public WifiCommand {
private:
    const char *mCountryCode;

public:
    SetCountryCodeCommand(wifi_interface_handle handle, const char *country_code)
        : WifiCommand("SetCountryCodeCommand", handle, 0) 
    {
        mCountryCode = country_code;
    }
    virtual int create() {
        int ret;
        ret = mMsg.create(NL80211_CMD_REQ_SET_REG);
        if (ret < 0) {
             ALOGE("Can't create message to send to driver - %d", ret);
             return ret;
        }
        ret = mMsg.put_string(NL80211_ATTR_REG_ALPHA2, mCountryCode);
        if (ret < 0) {
            return ret;
        }
        return WIFI_SUCCESS;
    }
};

class RSSIMonitorControl : public WifiCommand {
private:
    s8 maxRSSI;
    s8 minRSSI;
    wifi_rssi_event_handler handler;
    s8 currRSSI;
    u8 BSSID[6];

public:
    RSSIMonitorControl(wifi_request_id id, wifi_interface_handle handle,
                             s8 max_rssi, s8 min_rssi, wifi_rssi_event_handler eh)
        : WifiCommand("RSSIMonitorControl", handle, id)
    {
        maxRSSI = max_rssi;
        minRSSI = min_rssi;
        handler = eh;
        currRSSI = 0;
        memset(BSSID, 0, sizeof(BSSID));
    }

    RSSIMonitorControl(wifi_request_id id, wifi_interface_handle handle)
        : WifiCommand("RSSIMonitorControl", handle, id)
    {
        maxRSSI = 0;
        minRSSI = 0;
        //handler.on_rssi_threshold_breached = NULL;
        memset(&handler, 0, sizeof(handler));
        currRSSI = 0;
        memset(BSSID, 0, sizeof(BSSID));
    }

    int createRequest(WifiRequest& request) {
        u32 enable = 1;
        int result = request.create(MARVELL_OUI, NXP_SUBCMD_RSSI_MONITOR);
        if (result < 0) {
            ALOGE("Failed to create message to start RSSI monitor - %d", result);
            return result;
        }

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
        result = request.put_s8(NXP_ATTR_RSSI_MONITOR_MAX_RSSI, maxRSSI);
        if (result < 0) {
            return result;
        }
        result = request.put_s8(NXP_ATTR_RSSI_MONITOR_MIN_RSSI, minRSSI);
        if (result < 0) {
            return result;
        }
        result = request.put_u32(NXP_ATTR_RSSI_MONITOR_CONTROL, enable);
        if (result < 0) {
            return result;
        }
        request.attr_end(data);
        return WIFI_SUCCESS;
    }

    int createTeardownRequest(WifiRequest &request, s8 max_rssi, s8 min_rssi) {
        u32 disable = 0;
        int result = request.create(MARVELL_OUI, NXP_SUBCMD_RSSI_MONITOR);
        if(result < 0) {
            return result;
        }

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
        result = request.put_s8(NXP_ATTR_RSSI_MONITOR_MAX_RSSI, maxRSSI);
        if (result < 0) {
            return result;
        }
        result = request.put_s8(NXP_ATTR_RSSI_MONITOR_MIN_RSSI, minRSSI);
        if (result < 0) {
            return result;
        }
        result = request.put_u32(NXP_ATTR_RSSI_MONITOR_CONTROL, disable);
        if (result < 0) {
            return result;
        }
        request.attr_end(data);
        return WIFI_SUCCESS;

    }
    int start() {
        WifiRequest request(familyId(), ifaceId());
        int result = createRequest(request);
        if (result < 0) {
            return result;
        }
        result = requestResponse(request);
        if (result < 0) {
            ALOGE("Failed to set RSSI Monitor, result = %d", result);
            return result;
        }
        result = registerVendorHandler(MARVELL_OUI, NXP_EVENT_RSSI_MONITOR);
        if (result < 0) {
            ALOGE("Failed to register handler");
            unregisterVendorHandler(MARVELL_OUI, NXP_EVENT_RSSI_MONITOR);
            return result;
        }
        return WIFI_SUCCESS;
    }

    virtual int cancel_specific(s8 max_rssi, s8 min_rssi) {

        WifiRequest request(familyId(), ifaceId());
        int result = createTeardownRequest(request, 0, 0);
        if (result != WIFI_SUCCESS) {
            ALOGE("failed to create request; result = %d", result);
        } else {
            result = requestResponse(request);
            if (result != WIFI_SUCCESS) {
                ALOGE("failed to stop RSSI monitoring = %d", result);
            }
        }
        unregisterVendorHandler(MARVELL_OUI, NXP_EVENT_RSSI_MONITOR);
        return NL_OK;
    }

    virtual int handleResponse(WifiEvent& reply) {
        /* Nothing to do on response! */
        return NL_SKIP;
    }

   virtual int handleEvent(WifiEvent& event) {

       if (event.get_cmd() != NL80211_CMD_VENDOR) {
           ALOGE("Ignoring reply with cmd = %d", event.get_cmd());
           return NL_SKIP;
       }

       nlattr *vendor_data =(nlattr *) (event.get_data(NL80211_ATTR_VENDOR_DATA));
       int len = event.get_vendor_data_len();
       int event_id = event.get_vendor_subcmd();
       struct nlattr *nl_attr = NULL;
       s8 tmp_rssi = 0;

       if (vendor_data == NULL || len == 0) {
           ALOGE("RSSI monitor: No data");
           return NL_SKIP;
       }
       
       if(event_id == NXP_EVENT_RSSI_MONITOR){
           struct nlattr *tb_vendor[NXP_ATTR_RSSI_MONITOR_MAX + 1];
           nla_parse(tb_vendor, NXP_ATTR_RSSI_MONITOR_MAX, vendor_data, len, NULL);
           if(!tb_vendor[NXP_ATTR_RSSI_MONITOR_CUR_RSSI]){
               ALOGE("NXP_ATTR_RSSI_MONITOR_CUR_RSSI not found\n");
               return NL_SKIP;
           }
           nl_attr = tb_vendor[NXP_ATTR_RSSI_MONITOR_CUR_RSSI];
           tmp_rssi = *((s8 *)(void *)nl_attr + 4);
           currRSSI = *(s8 *)(tb_vendor[NXP_ATTR_RSSI_MONITOR_CUR_RSSI]);
           if(!tb_vendor[NXP_ATTR_RSSI_MONITOR_CUR_BSSID]){
               ALOGE("NXP_ATTR_RSSI_MONITOR_CUR_BSSID not found\n");
               return NL_SKIP;
           }
           if(nla_len(tb_vendor[NXP_ATTR_RSSI_MONITOR_CUR_BSSID]) != sizeof(*BSSID)){
               ALOGE("Invalid mac addr length\n");
               return NL_SKIP;
           }
           memcpy(BSSID,(u8*)nla_data(tb_vendor[NXP_ATTR_RSSI_MONITOR_CUR_BSSID]), sizeof(*BSSID));
           if (*handler.on_rssi_threshold_breached) {
               (*handler.on_rssi_threshold_breached)(id(), BSSID, currRSSI);
           } else {
               ALOGE("No RSSI monitor handler registered");
           }
          return NL_OK;
       }
       return NL_OK;
    }

};

class GetSupportedFeatureSet : public WifiCommand {
private:
    feature_set *featureSet;

public:
    GetSupportedFeatureSet(wifi_interface_handle handle, feature_set *set)
    : WifiCommand("GetSupportedFeatureSet", handle, 0)
    {
        featureSet = set;
    }

    virtual int create() {
        int ret = 0;
        ret = mMsg.create(MARVELL_OUI, NXP_SUBCMD_GET_SUPPORTED_FEATURE_SET);
        if (ret < 0){
            ALOGE("Failed to create message to get supported feature set - %d", ret);
            return ret;
        }
        return WIFI_SUCCESS;
    }

    virtual int handleResponse(WifiEvent& reply) {
        if (reply.get_cmd() != NL80211_CMD_VENDOR) {
            ALOGE("Ignoring reply with cmd = %d", reply.get_cmd());
            return NL_SKIP;
        }
        nlattr *vendor_data = (nlattr *)reply.get_data(NL80211_ATTR_VENDOR_DATA);
        int len = reply.get_vendor_data_len();
        if (vendor_data == NULL || len == 0) {
            ALOGE("Error! No data in command response");
            return NL_SKIP;
        }
        struct nlattr *tb_vendor[NXP_ATTR_WIFI_COMMON_MAX + 1];
        nla_parse(tb_vendor, NXP_ATTR_WIFI_COMMON_MAX, vendor_data, len, NULL);
        if(!tb_vendor[NXP_ATTR_SUPPORTED_FEATURE_SET]){
            ALOGE("NXP_ATTR_SUPPORTED_FEATURE_SET not found");
            return WIFI_ERROR_INVALID_ARGS;
        }
        *featureSet = nla_get_u32(tb_vendor[NXP_ATTR_SUPPORTED_FEATURE_SET]);
        return NL_OK;
    }
};

class GetConcurrencyMatrix : public WifiCommand 
{
private:
    int setMax;
    feature_set *Set;
    int *setSize;

public:
    GetConcurrencyMatrix(wifi_interface_handle handle, int set_size_max,
                                feature_set *set, int *set_size)
    : WifiCommand("GetConcurrencyMatrix", handle,0)
    {
        setMax = set_size_max;
        Set = set;
        setSize = set_size;
    }

    virtual int create() {
        int ret = 0;
        ret = mMsg.create(MARVELL_OUI, NXP_SUBCMD_GET_CONCURRENCY_MATRIX);
        if (ret < 0) {
            ALOGE("Failed to create message to get concurrency matrix - %d", ret);
            return ret;
        }
        nlattr *data = mMsg.attr_start(NL80211_ATTR_VENDOR_DATA);
        ret = mMsg.put_u32(NXP_ATTR_CONCURRENCY_MATRIX_SET_SIZE_MAX, setMax);
        if (ret < 0) {
            return ret;
        }
        mMsg.attr_end(data);
        return WIFI_SUCCESS;
    }

    virtual int handleResponse(WifiEvent& reply) {
        if (reply.get_cmd() != NL80211_CMD_VENDOR) {
            ALOGE("Ignoring reply with cmd = %d", reply.get_cmd());
            return NL_SKIP;
        }
        nlattr *vendor_data = (nlattr *)reply.get_data(NL80211_ATTR_VENDOR_DATA);
        int len = reply.get_vendor_data_len();
        if (vendor_data == NULL || len == 0) {
            ALOGE("Error! No data in command response");
            return NL_SKIP;
        }
        struct nlattr *tb_vendor[NXP_ATTR_WIFI_COMMON_MAX + 1];
        nla_parse(tb_vendor, NXP_ATTR_WIFI_COMMON_MAX, vendor_data, len, NULL);
        if(!tb_vendor[NXP_ATTR_CONCURRENCY_MATRIX_SET_SIZE]){
            ALOGE("NXP_ATTR_CONCURRENCY_MATRIX_SET_SIZE not found");
            return WIFI_ERROR_INVALID_ARGS;
        }
        *setSize = nla_get_u32(tb_vendor[NXP_ATTR_CONCURRENCY_MATRIX_SET_SIZE]);
        if(!tb_vendor[NXP_ATTR_CONCURRENCY_MATRIX_SET]){
            ALOGE("NXP_ATTR_CONCURRENCY_MATRIX_SET not found");
            return WIFI_ERROR_INVALID_ARGS;
        }
        memcpy(Set,(nlattr *) tb_vendor[NXP_ATTR_CONCURRENCY_MATRIX_SET], *setSize);
        return NL_OK;
    }
};

class ConfigureNdOffLoad : public WifiCommand {
private:
    u8 Enable;

public:
    ConfigureNdOffLoad(wifi_interface_handle iface, u8 enable)
    : WifiCommand("ConfigureNdOffLoad", iface, 0) 
    {
        Enable = enable;
    }
    virtual int create() {
        int ret = 0;
        ret = mMsg.create(MARVELL_OUI, NXP_SUBCMD_CONFIGURE_ND_OFFLOAD);
        if (ret < 0) {
            ALOGE("Failed to create message to configure nd offload - %d", ret);
            return ret;
        }
        nlattr *data = mMsg.attr_start(NL80211_ATTR_VENDOR_DATA);
        ret = mMsg.put_u8(NXP_ATTR_ND_OFFLOAD_CONTROL, Enable);
        if (ret < 0) {
            return ret;
        }
        mMsg.attr_end(data);
        return WIFI_SUCCESS;
    }
};


class SetPacketFilterCommand : public WifiCommand {
private:
    u32 mLen;
    char* mProgram;

public:
    SetPacketFilterCommand(wifi_interface_handle iface, const u8* program, 
                                   u32 len)
    : WifiCommand("SetPacketFilterCommand", iface, 0)
    {
        mProgram = (char *)program;
        mLen = len;
    }

    virtual int create() {
        int ret = 0;
        ret = mMsg.create(MARVELL_OUI, NXP_SUBCMD_SET_PACKET_FILTER);
        if(ret < 0){
            ALOGE("Can't create message to send to driver - %d", ret);
            return ret;
        }
        nlattr *data = mMsg.attr_start(NL80211_ATTR_VENDOR_DATA);
        ret = mMsg.put_u32(NXP_ATTR_PACKET_FILTER_TOTAL_LENGTH, mLen);
        if(ret < 0) {
            ALOGE("Failed to put attribute len");
            return ret;
        }
        ret = mMsg.put_string(NXP_ATTR_PACKET_FILTER_PROGRAM, mProgram);
        if(ret < 0) {
            ALOGE("Failed to put attribute program");
            return ret;
        }
        mMsg.attr_end(data);
        return WIFI_SUCCESS;
    }
};

class GetPacketFilterCapaCommand : public WifiCommand {
private: 
    u32 *mVersion;
    u32 *mMax_len;

public:
    GetPacketFilterCapaCommand(wifi_interface_handle handle,
                                         u32 *version, u32 *max_len)
    : WifiCommand("GetPacketFilterCapaCommand", handle, 0)
    {
        mVersion = version;
        mMax_len = max_len;
    }

    virtual int createSetupRequest(WifiRequest &request) {
        int ret = 0;
        ret = request.create(MARVELL_OUI,NXP_SUBCMD_GET_PACKET_FILTER_CAPA);
        if(ret < 0){
            ALOGE("Can't create message to send to driver - %d", ret);
            return ret;
        }
        return WIFI_SUCCESS;
    }

    int start() {
        WifiRequest request(familyId(), ifaceId());
        int result = 0;
        result = createSetupRequest(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("failed to create request; result = %d", result);
            return result;
        }

        result = requestResponse(request);
        if (result < 0) {
            ALOGE("%s: requestResponse() error: %d", __FUNCTION__, result);
            if (result == -ENOTSUP) {
                /* Packet filtering is not supported currently, so return version
                            * and length as 0
                            */
                ALOGE("Packet filtering is not supported");
                *mVersion = 0;
                *mMax_len = 0;
                result = WIFI_SUCCESS;
            } else {
                ALOGE("Failed to get packet filter capa, result = %d", result);
                return result;
            }
        }
        return WIFI_SUCCESS;
    }

    virtual int handleResponse(WifiEvent &reply){

        if (reply.get_cmd() != NL80211_CMD_VENDOR) {
        ALOGE("Ignoring reply with cmd = %d\n", reply.get_cmd());
        return NL_SKIP;
        }

        nlattr *tb = (nlattr*)(reply.get_data(NL80211_ATTR_VENDOR_DATA));
        int len = reply.get_vendor_data_len();
        if(tb == NULL || len == 0){
            ALOGE("ERROR\n");
            return NL_SKIP;
        }

        struct nlattr *tb_vendor[NXP_ATTR_PACKET_FILTER_MAX + 1];
        nla_parse(tb_vendor, NXP_ATTR_PACKET_FILTER_MAX, tb, len, NULL);
        if (!tb_vendor[NXP_ATTR_PACKET_FILTER_VERSION]){
            ALOGE("%s: NXP_ATTR_PACKET_FILTER_VERSION not found", __FUNCTION__);
            return WIFI_ERROR_INVALID_ARGS;
        }
        *mVersion = nla_get_u32(tb_vendor[NXP_ATTR_PACKET_FILTER_VERSION]);
        if (!tb_vendor[NXP_ATTR_PACKET_FILTER_MAX_LEN]) {
            ALOGE("%s: NXP_ATTR_PACKET_FILTER_MAX_LEN not found", __FUNCTION__);
            return WIFI_ERROR_INVALID_ARGS;
        }
        *mMax_len = nla_get_u32(tb_vendor[NXP_ATTR_PACKET_FILTER_MAX_LEN]);
        return NL_OK;
    }
};

class GetValidChannels : public WifiCommand
{
private:
    int Band;
    int maxChannels;
    wifi_channel *Channels;
    int *numChannels;

public:
    GetValidChannels(wifi_interface_handle iface, int band, int max_channels,
                           wifi_channel *channels, int *num_channels)
    : WifiCommand("GetValidChannels", iface, 0)
{
    Band = band;
    maxChannels = max_channels;
    numChannels = num_channels;
    Channels = channels;
    memset(Channels, 0, sizeof(wifi_channel) * maxChannels);
}
    virtual int create() {
        int ret = 0;
        ret = mMsg.create(MARVELL_OUI, NXP_SUBCMD_GET_VALID_CHANNELS);
        if (ret < 0) {
            ALOGE("Failed to create message to get valid channels - %d", ret);
            return ret;
        }
        nlattr *data = mMsg.attr_start(NL80211_ATTR_VENDOR_DATA);
        ret = mMsg.put_u32(NXP_ATTR_VALID_CHANNELS_BAND, Band);
        if (ret < 0) {
            return ret;
        }
        mMsg.attr_end(data);
        return WIFI_SUCCESS;
    }

    virtual int handleResponse(WifiEvent& reply) {
        if (reply.get_cmd() != NL80211_CMD_VENDOR) {
            ALOGE("Ignoring reply with cmd = %d", reply.get_cmd());
            return NL_SKIP;
        }
        nlattr *vendor_data = (nlattr *)reply.get_data(NL80211_ATTR_VENDOR_DATA);
        int len = reply.get_vendor_data_len();
        if (vendor_data == NULL || len == 0) {
            ALOGE("Error! No data in command response");
            return NL_SKIP;
        }
        struct nlattr *tb_vendor[NXP_ATTR_WIFI_COMMON_MAX + 1];
        nla_parse(tb_vendor, NXP_ATTR_WIFI_COMMON_MAX, vendor_data, len, NULL);
        if(!tb_vendor[NXP_ATTR_VALID_CHANNELS_NUM_CHANNELS]){
            ALOGE("NXP_ATTR_VALID_CHANNELS_NUM_CHANNELS not found");
            return WIFI_ERROR_INVALID_ARGS;
        }
        *numChannels= nla_get_u32(tb_vendor[NXP_ATTR_VALID_CHANNELS_NUM_CHANNELS]);
        if(*numChannels > maxChannels)
            *numChannels = maxChannels;
        if(!tb_vendor[NXP_ATTR_VALID_CHANNEL_LIST]){
            ALOGE("NXP_ATTR_VALID_CHANNEL_LIST not found");
            return WIFI_ERROR_INVALID_ARGS;
        }
        memcpy(Channels, (wifi_channel *)nla_data(tb_vendor[NXP_ATTR_VALID_CHANNEL_LIST]), 
                sizeof(wifi_channel) * (*numChannels));
        return NL_OK;
    }
};

static int wifi_get_multicast_id(wifi_handle handle, const char *name, const char *group)
{
    GetMulticastIdCommand cmd(handle, name, group);
    int res = cmd.requestResponse();
    if (res < 0)
        return res;
    else
        return cmd.getId();
}

/////////////////////////////////////////////////////////////////////////

static bool is_wifi_interface(const char *name)
{
    if (strncmp(name, "wlan", 4) != 0 && strncmp(name, "p2p", 3) != 0 &&
          strncmp(name, "mlan", 4) != 0 && strncmp(name, "nan", 3) != 0 &&
	  strncmp(name, "uap", 3) != 0) {
        /* not a wifi interface; ignore it */
        return false;
    } else {
        return true;
    }
}

static int get_interface(const char *name, interface_info *info)
{
    strcpy(info->name, name);
    info->id = if_nametoindex(name);
    return WIFI_SUCCESS;
}

wifi_error wifi_init_interfaces(wifi_handle handle)
{
    hal_info *info = (hal_info *)handle;

    struct dirent *de;

    DIR *d = opendir("/sys/class/net");
    if (d == 0)
        return WIFI_ERROR_UNKNOWN;

    int n = 0;
    while ((de = readdir(d))) {
        if (de->d_name[0] == '.')
            continue;
        if (is_wifi_interface(de->d_name) ) {
            n++;
        }
    }

    closedir(d);

    if (n == 0)
        return WIFI_ERROR_NOT_AVAILABLE;

    d = opendir("/sys/class/net");
    if (d == 0)
        return WIFI_ERROR_UNKNOWN;

    info->interfaces = (interface_info **)malloc(sizeof(interface_info *) * n);

    int i = 0;
    while ((de = readdir(d))) {
        if (de->d_name[0] == '.')
            continue;
        if (is_wifi_interface(de->d_name)) {
            interface_info *ifinfo = (interface_info *)malloc(sizeof(interface_info));
            if (get_interface(de->d_name, ifinfo) != WIFI_SUCCESS) {
                free(ifinfo);
                continue;
            }
            ifinfo->handle = handle;
            info->interfaces[i] = ifinfo;
            i++;
        }
    }

    closedir(d);
    ALOGV("Found %d interfaces", n);
    info->num_interfaces = n;
    return WIFI_SUCCESS;
}

wifi_error wifi_get_ifaces(wifi_handle handle, int *num, wifi_interface_handle **interfaces)
{
    hal_info *info = (hal_info *)handle;

    *interfaces = (wifi_interface_handle *)info->interfaces;
    *num = info->num_interfaces;

    return WIFI_SUCCESS;
}

wifi_error wifi_get_iface_name(wifi_interface_handle handle, char *name, size_t size)
{
    interface_info *info = (interface_info *)handle;
    strcpy(name, info->name);
    return WIFI_SUCCESS;
}

wifi_error wifi_get_supported_feature_set(wifi_interface_handle handle, feature_set *set)
{
    int ret = 0;
    GetSupportedFeatureSet getFeatureSet(handle, set);
    ret = getFeatureSet.requestResponse();
    return (wifi_error)ret;
}

wifi_error wifi_get_concurrency_matrix(wifi_interface_handle handle, int set_size_max,
       feature_set set[], int *set_size)
{
    int ret = 0;
    GetConcurrencyMatrix getConcurrencyMatrix(handle, set_size_max, set, set_size);
    ret = getConcurrencyMatrix.requestResponse();
    return (wifi_error)ret;
}

wifi_error wifi_set_scanning_mac_oui(wifi_interface_handle handle, oui scan_oui)
{
    int ret = 0;
    SetScanningMacOui SetMacOui(handle, scan_oui);
    ret = SetMacOui.requestResponse();
    return (wifi_error) ret;

}

wifi_error wifi_set_nodfs_flag(wifi_interface_handle handle, u32 nodfs)
{
    int ret = 0;
    SetNodfsFlag NodsFlag(handle, nodfs);
    ret = NodsFlag.requestResponse();
    return (wifi_error) ret;
}

wifi_error wifi_configure_nd_offload(wifi_interface_handle iface, u8 enable)
{
    int ret = 0;
    ConfigureNdOffLoad NdOffload(iface, enable);
    ret = NdOffload.requestResponse();
    return (wifi_error) ret;
}

wifi_error wifi_set_country_code(wifi_interface_handle handle, const char *country_code)
{
    if(!country_code){
        ALOGE("country_code is NULL");
        return WIFI_ERROR_INVALID_ARGS;
    }
    SetCountryCodeCommand command(handle, country_code);
    return (wifi_error) command.requestResponse();
}

wifi_error wifi_start_rssi_monitoring(wifi_request_id id, wifi_interface_handle
                        iface, s8 max_rssi, s8 min_rssi, wifi_rssi_event_handler eh)
{
    int ret = 0;
    wifi_handle handle = getWifiHandle(iface);
    RSSIMonitorControl *StartRSSI = new RSSIMonitorControl(id, iface, max_rssi, 
                                                           min_rssi, eh);
    wifi_register_cmd(handle, id, StartRSSI);
    ret = StartRSSI->start();
    return (wifi_error)ret;
}


wifi_error wifi_stop_rssi_monitoring(wifi_request_id id, wifi_interface_handle iface)
{
    wifi_handle handle = getWifiHandle(iface);
    wifi_rssi_event_handler handler;
    memset(&handler, 0, sizeof(handler));
    RSSIMonitorControl *StopRSSI = new RSSIMonitorControl(id, iface);
    StopRSSI->cancel_specific(0, 0);
    StopRSSI->releaseRef();
    return WIFI_SUCCESS;
}

wifi_error wifi_set_packet_filter(wifi_interface_handle iface, const u8* program, u32 len)
{
    int ret = 0;
    /* len=0 clears the filters in driver/firmware */
    if(len != 0 && program == NULL){
        ALOGE("No valid program provided");
        return WIFI_ERROR_INVALID_ARGS;
    }
    SetPacketFilterCommand SetFilter (iface, program, len);
    ret = SetFilter.requestResponse();
    return (wifi_error)ret;
}

wifi_error wifi_get_packet_filter_capabilities(wifi_interface_handle handle, u32 *version, u32 *max_len)
{
    int ret = 0;
    if(version == NULL || max_len == NULL) {
        ALOGE("Error! The input version/max_len is NULL");
        return WIFI_ERROR_INVALID_ARGS;
    }
    GetPacketFilterCapaCommand *GetCapa= new GetPacketFilterCapaCommand(handle, version, max_len);
    ret = GetCapa->start();
    return (wifi_error)ret;
}

wifi_error wifi_get_valid_channels(wifi_interface_handle handle,
        int band, int max_channels, wifi_channel *channels, int *num_channels)
{
    int ret = 0;
    if(!max_channels || channels == NULL){
        ALOGE("Error! The input channels is NULL or max_channel is zero");
        return WIFI_ERROR_INVALID_ARGS;
    }
    GetValidChannels ValidChannels(handle, band, max_channels, channels, num_channels);
    ret = ValidChannels.requestResponse();
    return (wifi_error) ret;
}

/////////////////////////////////////////////////////////////////////////////
