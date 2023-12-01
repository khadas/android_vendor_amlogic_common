/*
 * Copyright (C) 2017 The Android Open Source Project
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

#define LOG_TAG  "wifimdnsoffload"
#ifdef __cplusplus
extern "C"
{
#endif
#ifdef ANDROID
#include <utils/Log.h>
#endif
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <net/if.h>

#include <netlink/socket.h>
#include <netlink/genl/ctrl.h>
#include <netlink/genl/genl.h>

#include "nl80211_copy.h"
#include "wifi_mdns_offload.h"

#ifndef NETLINK_EXT_ACK
#define NETLINK_EXT_ACK 11
enum nlmsgerr_attrs
{
    NLMSGERR_ATTR_UNUSED,
    NLMSGERR_ATTR_MSG,
    NLMSGERR_ATTR_OFFS,
    NLMSGERR_ATTR_COOKIE,

    __NLMSGERR_ATTR_MAX,
    NLMSGERR_ATTR_MAX = __NLMSGERR_ATTR_MAX - 1
};
#endif

#define WIFI_MDNS_OFFLOAD_CMD_SOCK_PORT 888
#define NL80211_ATTR_MAX_INTERNAL 256

typedef struct {
    struct nl_sock *sock;
    int nl80211_family_id;
} nl_socket_handler;

static nl_socket_handler socket_handler;
static const uint32_t vendor_oui = GOOGLE_VENDOR_OUI;
static uint32_t log_style = LOG_STYLE_LOGCAT;
static uint32_t log_mask = LOG_DEBUG_MASK;

int wifi_mdns_offload_log_out(uint8_t level, const char *fmt, ...)
{
    uint32_t n = 0;

    if (!(log_mask & (1 << level)))
        return n;

    if (log_style == LOG_STYLE_CONSOLE) {
        va_list args;
        va_start(args, fmt);
        n = vprintf(fmt, args);
        va_end(args);
        return n;
    } else if (log_style == LOG_STYLE_LOGCAT) {
#ifdef ANDROID
        int android_log_level;
        if (level == LOG_LEVEL_ERROR)
            android_log_level = ANDROID_LOG_ERROR;
        else if (level == LOG_LEVEL_WARNING)
            android_log_level = ANDROID_LOG_WARN;
        else if (level == LOG_LEVEL_INFO)
            android_log_level = ANDROID_LOG_INFO;
        else if (level == LOG_LEVEL_DEBUG)
            android_log_level = ANDROID_LOG_DEBUG;
        else if (level == LOG_LEVEL_VERBOSE)
            android_log_level = ANDROID_LOG_VERBOSE;
        else
            android_log_level = ANDROID_LOG_DEBUG;
        va_list args;
        va_start(args, fmt);
        n = __android_log_vprint(android_log_level, LOG_TAG, fmt, args);
        va_end(args);
        return n;
#else
        return n;
#endif
    }

    return n;
}

void wifi_mdns_offload_set_log(uint8_t style, uint32_t mask)
{
    log_style = style;
    log_mask = mask;
}

int wifi_mdns_offload_init()
{
    LOGD("wifi_mdns_offload_init\n");
    struct nl_sock *sock = nl_socket_alloc();
    if (sock == NULL) {
        LOGE("nl_socket_alloc failed!\n");
        return -1;
    }
    uint32_t pid = getpid() & 0x3FFFFF;
    nl_socket_set_local_port(sock,
		pid + (WIFI_MDNS_OFFLOAD_CMD_SOCK_PORT << 22));
    if (nl_connect(sock, NETLINK_GENERIC)) {
        LOGE("nl_connect failed!\n");
        nl_socket_free(sock);
        return -1;
    }
    nl_socket_set_buffer_size(sock, 8192, 8192);
    int err = 1;
    setsockopt(nl_socket_get_fd(sock), SOL_NETLINK,
        NETLINK_EXT_ACK, &err, sizeof(err));
    int nl80211_family_id = genl_ctrl_resolve(sock, "nl80211");
    if (nl80211_family_id < 0) {
        LOGE("Could not resolve nl80211 family id\n");
        nl_socket_free(sock);
        return -1;
    }
    socket_handler.sock = sock;
    socket_handler.nl80211_family_id = nl80211_family_id;
    LOGD("wifi_mdns_offload_init Successfully\n");
    return 0;
}

int wifi_mdns_offload_deinit()
{
    LOGD("wifi_mdns_offload_deinit\n");
    struct nl_sock *sock = socket_handler.sock;
    if (sock) {
        nl_socket_free(sock);
    }
    LOGD("wifi_mdns_offload_deinit Successfully\n");
    return 0;
}

static struct nl_msg *nlmsg_create(uint32_t cmd)
{
    int ret = 0;
    struct nl_msg *msg = nlmsg_alloc();
    if (msg == NULL) {
        LOGE("nlmsg_alloc failed!\n");
        return NULL;
    }
    if (genlmsg_put(msg, 0, 0, socket_handler.nl80211_family_id,
		0, 0, NL80211_CMD_VENDOR, 0) == NULL) {
        LOGE("genlmsg_put failed!\n");
        goto error;
    }
    ret = nla_put(msg, NL80211_ATTR_VENDOR_ID,
          sizeof(vendor_oui), &vendor_oui);
    if (ret < 0) {
        LOGE("line:%d,nla_put failed!ret=%d\n", __LINE__, ret);
        goto error;
    }
    ret = nla_put(msg, NL80211_ATTR_VENDOR_SUBCMD, sizeof(cmd), &cmd);
    if (ret < 0) {
        LOGE("line:%d,nla_put failed!ret=%d\n", __LINE__, ret);
        goto error;
    }
    uint32_t if_idx = if_nametoindex("wlan0");
    if (!if_idx) {
        LOGE("wlan0 does not exist\n");
        goto error;
    }
    ret = nla_put(msg, NL80211_ATTR_IFINDEX, sizeof(if_idx), &if_idx);
    if (ret < 0) {
        LOGE("line:%d,nla_put failed!ret=%d\n", __LINE__, ret);
        goto error;
    }
    return msg;
error:
    nlmsg_free(msg);
    return NULL;
}

static int put_data(struct nl_msg *msg, int attr,
    void *data, uint32_t len)
{
    int ret = 0;
    struct nlattr *start = nla_nest_start(msg, NL80211_ATTR_VENDOR_DATA);
    if (start == NULL) {
        LOGE("nla_nest_start failed!\n");
        goto error;
    }
    ret = nla_put(msg, attr, len, data);
    if (ret < 0) {
        LOGE("line:%d,nla_put failed!ret=%d\n", __LINE__, ret);
        goto error;
    }
    ret = nla_nest_end(msg, start);
    if (ret < 0) {
        LOGE("nla_nest_end failed!ret=%d\n", ret);
        goto error;
    }
    return ret;
error:
    nlmsg_free(msg);
    return ret;
}

static int no_seq_check(struct nl_msg *msg, void *arg)
{
    //LOGD("no_seq_check called\n");
    return NL_OK;
}

static int valid_handler(struct nl_msg *msg, void *arg)
{
    //LOGD("valid_handler called\n");
    int *err = (int *)arg;
    *err = 0;
    return NL_SKIP;
}

static int ack_handler(struct nl_msg *msg, void *arg)
{
    //LOGD("ack_handler called\n");
    int *err = (int *)arg;
    *err = 0;
    return NL_STOP;
}

static int finish_handler(struct nl_msg *msg, void *arg)
{
    //LOGD("finish_handler called\n");
    int *ret = (int *)arg;
    *ret = 0;
    return NL_SKIP;
}

static int error_handler(struct sockaddr_nl *nla,
    struct nlmsgerr *err, void *arg)
{
    int *ret = (int *)arg;
    *ret = err->error;
    //LOGD("error_handler called: %d\n", err->error);
    return NL_SKIP;
}

static uint32_t get_u32(struct nlattr **attributes, int attribute)
{
    return attributes[attribute] ? nla_get_u32(attributes[attribute]) : 0;
}

static int get_len(struct nlattr **attributes, int attribute)
{
    return attributes[attribute] ? nla_len(attributes[attribute]) : 0;
}

static void *get_data(struct nlattr **attributes, int attribute)
{
    return attributes[attribute] ? nla_data(attributes[attribute]) : NULL;
}

static int get_vendor_id(struct nlattr **attributes)
{
    return get_u32(attributes, NL80211_ATTR_VENDOR_ID);
}

static int get_vendor_subcmd(struct nlattr **attributes)
{
    return get_u32(attributes, NL80211_ATTR_VENDOR_SUBCMD);
}

static void *get_vendor_data(struct nlattr **attributes)
{
    return get_data(attributes, NL80211_ATTR_VENDOR_DATA);
}

static int get_vendor_data_len(struct nlattr **attributes)
{
    return get_len(attributes, NL80211_ATTR_VENDOR_DATA);
}

static char *decode_qname(unsigned char *buf,
    uint32_t buf_len, uint32_t offset)
{
    char *qname = NULL;
    unsigned char *p = NULL, *c = NULL;
    uint32_t n = 0, i = 0;

    if (!buf || buf_len < 1 || offset < 1 || offset > buf_len)
        goto err;
    p = buf + offset - 1;
    if (*p == 0)
        goto err;
    qname = (char *)malloc(256);
    if (!qname) {
        LOGD("alloc failed!\n");
        return NULL;
    }
    memset(qname, 0, 256);
    c = (unsigned char *)qname;
    while (*p) {
        if ((*p >> 6) == 0x03) {
            n = (((*p << 8) | *(p + 1)) & 0x3fff) - 1;
            if (n > (buf_len - 1))
                goto err;
            p = buf + n;
            continue;
        }
        n = *p;
        if (p + 1 + n > buf + buf_len - 1)
            goto err;
        p++;
        for (i = 0; i < n; i++) {
            if (*p > 32 && *p < 127)
                *c++ = *p++;
            else
                goto err;
        }
        if (*p != 0)
            *c++ = '.';
    }
    return qname;
err:
    LOGD("decode qname failed!\n");
    if (qname)
        free(qname);
    return NULL;
}

static void dump_msg(unsigned char *buf, uint32_t len)
{
    int line = 16, i = 0, j = 0;
	uint32_t n = 0;
    char *dump = NULL;

    dump = (char *)malloc(256);
    if (!dump) {
        LOGD("alloc failed!\n");
        return;
    }
    for (i = 0; i < len; i++) {
        memset(dump, 0, 256);
        n = 0;
        n += sprintf(dump + n, "%04x|", i);
        for (j = i; j < i + line; j++) {
            if (j < len)
                n += sprintf(dump + n, "%02x", buf[j]);
            else
                n += sprintf(dump + n, "  ");
            if (j == i + line - 1)
                n += sprintf(dump + n, "|");
            else
                n += sprintf(dump + n, " ");
        }
        for (j = i; j < i + line && j < len; j++) {
            if (buf[j] > 32 && buf[j] < 127)
                n += sprintf(dump + n, "%c", buf[j]);
            else
                n += sprintf(dump + n, ".");
        }
        LOGD("%s\n", dump);
        i = i + line - 1;
    }
    free(dump);
    dump = NULL;
}

int response_handler(struct nl_msg *msg, void *arg) {
    //LOGD("response_handler called");
    struct genlmsghdr *header;
    struct nlattr *attributes[NL80211_ATTR_MAX_INTERNAL + 1];
    header = (struct genlmsghdr *)nlmsg_data(nlmsg_hdr(msg));
    if (header->cmd != NL80211_CMD_VENDOR) {
        LOGD("ignoring response with cmd = %d\n", header->cmd);
        return NL_SKIP;
    }
    LOGD("response msg len = %d,dump msg:\n", nlmsg_hdr(msg)->nlmsg_len);
    dump_msg((unsigned char *)(nlmsg_hdr(msg)), nlmsg_hdr(msg)->nlmsg_len);
    int ret = nla_parse(attributes, NL80211_ATTR_MAX_INTERNAL,
        genlmsg_attrdata(header, 0), genlmsg_attrlen(header, 0), NULL);
    if (ret < 0) {
        LOGE("nla_parse: failed to parse reply message = %d\n", ret);
        return NL_SKIP;
    }
    ret = nla_parse_nested(attributes, NL80211_ATTR_MAX_INTERNAL,
        attributes[NL80211_ATTR_VENDOR_DATA], NULL);
    if (ret < 0) {
        LOGE("nla_parse_nested: failed to parse reply message = %d\n", ret);
        return NL_SKIP;
    }
    uint32_t vendor_id = get_vendor_id(attributes);
    uint32_t vendor_subcmd = get_vendor_subcmd(attributes);
    uint32_t vendor_data_len = get_vendor_data_len(attributes);
    void *vendor_data = get_vendor_data(attributes);
    LOGD("vendor id: 0x%04x\n", vendor_id);
    LOGD("vendor subcmd: 0x%04x\n", vendor_subcmd);
    LOGD("vendor data len: %d\n", vendor_data_len);
    if (vendor_subcmd == WIFI_MDNS_OFFLOAD_SET_STATE
        || vendor_subcmd == WIFI_MDNS_OFFLOAD_ADD_TO_PASSTHROUGH_LIST) {
        *((u32_boolean *)arg) = *((u32_boolean *)vendor_data);
        return NL_OK;
    } else if ((vendor_subcmd == WIFI_MDNS_OFFLOAD_ADD_PROTOCOL_RESPONSES
        || vendor_subcmd == WIFI_MDNS_OFFLOAD_GET_AND_RESET_HIT_COUNTER
        || vendor_subcmd == WIFI_MDNS_OFFLOAD_GET_AND_RESET_MISS_COUNTER)
        && vendor_data) {
        *((int *)arg) = *((int *)vendor_data);
        return NL_OK;
    }
    return NL_SKIP;
}

static int requestResponse(struct nl_msg *msg, void *arg)
{
	struct nl_cb *cb = NULL;
    int err = 0;
    struct nl_sock *sock = socket_handler.sock;
    if (sock == NULL) {
        LOGE("sock == NULL!\n");
        err = -1;
        goto out;
    }
    cb = nl_cb_alloc(NL_CB_DEFAULT);
    if (!cb) {
        LOGE("nl_cb_alloc failed!\n");
        err = -1;
        goto out;
    }
    err = nl_send_auto_complete(sock, msg);
    if (err < 0) {
		LOGE("nl_send_auto_complete failed!err=%d\n", err);
        goto out;
    }
    err = 1;
    nl_cb_set(cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, no_seq_check, NULL);
    nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &err);
    nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
    nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);
    nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, response_handler, arg);
    while (err > 0) {
        int ret = nl_recvmsgs(sock, cb);
        if (ret) {
            LOGE("nl80211: %s->nl_recvmsgs failed: %d\n", __func__, ret);
        }
    }
out:
    if (cb)
        nl_cb_put(cb);
    if (msg)
        nlmsg_free(msg);
    return err;
}

u32_boolean setOffloadState(u32_boolean enabled)
{
    LOGD("%s: enabled:%d\n", __func__, enabled);
    struct nl_msg *msg = NULL;
    int ret = 0;
    msg = nlmsg_create(WIFI_MDNS_OFFLOAD_SET_STATE);
    if (msg == NULL) {
        LOGE("nlmsg_create failed!\n");
        return 0;
    }
    struct nlattr *start = nla_nest_start(msg, NL80211_ATTR_VENDOR_DATA);
    if (start == NULL) {
        LOGE("nla_nest_start failed!\n");
        goto error;
    }
    ret = nla_put(msg, WIFI_MDNS_OFFLOAD_ATTRIBUTE_STATE,
        sizeof(enabled), &enabled);
    if (ret < 0) {
        LOGE("line:%d,nla_put failed!ret=%d\n", __LINE__, ret);
        goto error;
    }
    ret = nla_nest_end(msg, start);
    if (ret < 0) {
        LOGE("nla_nest_end failed!ret=%d\n", ret);
        goto error;
    }
    u32_boolean result = 0;
    ret = requestResponse(msg, &result);
    LOGD("%s: exit:%d,result:%d\n", __func__, ret, result);
    return result;
error:
    nlmsg_free(msg);
    return 0;
}

void resetAll()
{
    LOGD("%s:\n", __func__);
    struct nl_msg *msg = NULL;
    int ret = 0;
    msg = nlmsg_create(WIFI_MDNS_OFFLOAD_RESET_ALL);
    if (msg == NULL) {
        LOGE("nlmsg_create failed!\n");
        return;
    }
    ret = requestResponse(msg, NULL);
    LOGD("%s: exit:%d\n", __func__, ret);
}

int addProtocolResponses(char *networkInterface,
    mdnsProtocolData *offloadData)
{
    LOGD("%s:\n", __func__);
    if (networkInterface)
        LOGD("%s: networkInterface:%s\n", __func__, networkInterface);
    if (offloadData) {
        LOGD("%s: rawOffloadPacketLen:%u\n", __func__,
            offloadData->rawOffloadPacketLen);
        LOGD("%s: criteriaListNum:%u\n", __func__,
            offloadData->matchCriteriaListNum);
        if (offloadData->matchCriteriaList) {
            LOGD("%s: dump:\n", __func__);
            LOGD("criteria list:\n");
            int i = 0;
            char *qname = NULL;
            for (i = 0; i < offloadData->matchCriteriaListNum; i++) {
                qname = decode_qname(offloadData->rawOffloadPacket,
                    offloadData->rawOffloadPacketLen,
                    offloadData->matchCriteriaList[i].nameOffset);
                LOGD("%d. type:%d\tnameOffset:%d\tname:%s\n", i + 1,
                    offloadData->matchCriteriaList[i].type,
                    offloadData->matchCriteriaList[i].nameOffset,
                    (qname && strlen(qname) > 0) ? qname : "none");
                if (qname) {
                    free(qname);
                    qname = NULL;
                }
            }
        }
        if (offloadData->rawOffloadPacket) {
            LOGD("rawOffloadPacket:\n");
            dump_msg(offloadData->rawOffloadPacket,
                offloadData->rawOffloadPacketLen);
        }
    }
    struct nl_msg *msg = NULL;
    int ret = 0;
    msg = nlmsg_create(WIFI_MDNS_OFFLOAD_ADD_PROTOCOL_RESPONSES);
    if (msg == NULL) {
        LOGE("nlmsg_create failed!\n");
        return -1;
    }
    struct nlattr *start = nla_nest_start(msg, NL80211_ATTR_VENDOR_DATA);
    if (start == NULL) {
        LOGE("nla_nest_start failed!\n");
        goto error;
    }
    if (networkInterface) {
        ret = nla_put(msg, WIFI_MDNS_OFFLOAD_ATTRIBUTE_NETWORK_INTERFACE,
            strlen(networkInterface) + 1, networkInterface);
        if (ret < 0) {
            LOGE("line:%d,nla_put failed!ret=%d\n", __LINE__, ret);
            goto error;
        }
    }
    if (offloadData) {
        ret = nla_put(msg, WIFI_MDNS_OFFLOAD_ATTRIBUTE_OFFLOAD_PKT_LEN,
            sizeof(offloadData->rawOffloadPacketLen),
            &offloadData->rawOffloadPacketLen);
        if (ret < 0) {
            LOGE("line:%d,nla_put failed!ret=%d\n", __LINE__, ret);
            goto error;
        }
        if (offloadData->rawOffloadPacket) {
            ret = nla_put(msg, WIFI_MDNS_OFFLOAD_ATTRIBUTE_OFFLOAD_PKT_DATA,
                offloadData->rawOffloadPacketLen,
                offloadData->rawOffloadPacket);
            if (ret < 0) {
                LOGE("line:%d,nla_put failed!ret=%d\n", __LINE__, ret);
                goto error;
            }
        }
        ret = nla_put(msg, WIFI_MDNS_OFFLOAD_ATTRIBUTE_MATCH_CRITERIA_NUM,
            sizeof(offloadData->matchCriteriaListNum),
            &offloadData->matchCriteriaListNum);
        if (ret < 0) {
            LOGE("line:%d,nla_put failed!ret=%d\n", __LINE__, ret);
            goto error;
        }
        if (offloadData->matchCriteriaList) {
            ret = nla_put(msg,
                WIFI_MDNS_OFFLOAD_ATTRIBUTE_MATCH_CRITERIA_DATA,
                sizeof(matchCriteria) * offloadData->matchCriteriaListNum,
                offloadData->matchCriteriaList);
            if (ret < 0) {
                LOGE("line:%d,nla_put failed!ret=%d\n", __LINE__, ret);
                goto error;
            }
        }
    }
    ret = nla_nest_end(msg, start);
    if (ret < 0) {
        LOGE("nla_nest_end failed!ret=%d\n", ret);
        goto error;
    }
    int result = -1;
    ret = requestResponse(msg, &result);
    LOGD("%s: exit:%d,result:%d\n", __func__, ret, result);
    return result;
error:
    nlmsg_free(msg);
    return ret;
}

void removeProtocolResponses(int recordKey)
{
    LOGD("%s: recordKey:%d\n", __func__, recordKey);
    struct nl_msg *msg = NULL;
    int ret = 0;
    msg = nlmsg_create(WIFI_MDNS_OFFLOAD_REMOVE_PROTOCOL_RESPONSES);
    if (msg == NULL) {
        LOGE("nlmsg_create failed!\n");
        return;
    }
    struct nlattr *start = nla_nest_start(msg, NL80211_ATTR_VENDOR_DATA);
    if (start == NULL) {
        LOGE("nla_nest_start failed!\n");
        goto error;
    }
    ret = nla_put(msg, WIFI_MDNS_OFFLOAD_ATTRIBUTE_RECORD_KEY,
        sizeof(recordKey), &recordKey);
    if (ret < 0) {
        LOGE("line:%d,nla_put failed!ret=%d\n", __LINE__, ret);
        goto error;
    }
    ret = nla_nest_end(msg, start);
    if (ret < 0) {
        LOGE("nla_nest_end failed!ret=%d\n", ret);
        goto error;
    }
    ret = requestResponse(msg, NULL);
    LOGD("%s: exit:%d\n", __func__, ret);
    return;
error:
    nlmsg_free(msg);
}

int getAndResetHitCounter(int recordKey)
{
    LOGD("%s: recordKey:%d\n", __func__, recordKey);
    struct nl_msg *msg = NULL;
    int ret = 0;
    msg = nlmsg_create(WIFI_MDNS_OFFLOAD_GET_AND_RESET_HIT_COUNTER);
    if (msg == NULL) {
        LOGE("nlmsg_create failed!\n");
        return -1;
    }
    struct nlattr *start = nla_nest_start(msg, NL80211_ATTR_VENDOR_DATA);
    if (start == NULL) {
        LOGE("nla_nest_start failed!\n");
        goto error;
    }
    ret = nla_put(msg, WIFI_MDNS_OFFLOAD_ATTRIBUTE_RECORD_KEY,
        sizeof(recordKey), &recordKey);
    if (ret < 0) {
        LOGE("line:%d,nla_put failed!ret=%d\n", __LINE__, ret);
        goto error;
    }
    ret = nla_nest_end(msg, start);
    if (ret < 0) {
        LOGE("nla_nest_end failed!ret=%d\n", ret);
        goto error;
    }
    int result = -1;
    ret = requestResponse(msg, &result);
    LOGD("%s: exit:%d,result:%d\n", __func__, ret, result);
    return result;
error:
    nlmsg_free(msg);
    return ret;
}

int getAndResetMissCounter()
{
    LOGD("%s:\n", __func__);
    struct nl_msg *msg = NULL;
    int ret = 0;
    msg = nlmsg_create(WIFI_MDNS_OFFLOAD_GET_AND_RESET_MISS_COUNTER);
    if (msg == NULL) {
        LOGE("nlmsg_create failed!\n");
        return -1;
    }
    int result = -1;
    ret = requestResponse(msg, &result);
    LOGD("%s: exit:%d,result:%d\n", __func__, ret, result);
    return result;
}

u32_boolean addToPassthroughList(char *networkInterface, char *qname)
{
    LOGD("%s:\n", __func__);
    if (networkInterface)
        LOGD("%s: networkInterface:%s\n", __func__, networkInterface);
    if (qname)
        LOGD("%s: qname:%s\n", __func__, qname);
    struct nl_msg *msg = NULL;
    int ret = 0;
    msg = nlmsg_create(WIFI_MDNS_OFFLOAD_ADD_TO_PASSTHROUGH_LIST);
    if (msg == NULL) {
        LOGE("nlmsg_create failed!\n");
        return 0;
    }
    struct nlattr *start = nla_nest_start(msg, NL80211_ATTR_VENDOR_DATA);
    if (start == NULL) {
        LOGE("nla_nest_start failed!\n");
        goto error;
    }
    if (networkInterface) {
        ret = nla_put(msg, WIFI_MDNS_OFFLOAD_ATTRIBUTE_NETWORK_INTERFACE,
            strlen(networkInterface) + 1, networkInterface);
        if (ret < 0) {
            LOGE("line:%d,nla_put failed!ret=%d\n", __LINE__, ret);
            goto error;
        }
    }
    if (qname) {
        ret = nla_put(msg, WIFI_MDNS_OFFLOAD_ATTRIBUTE_QNAME,
            strlen(qname) + 1, qname);
        if (ret < 0) {
            LOGE("line:%d,nla_put failed!ret=%d\n", __LINE__, ret);
            goto error;
        }
    }
    ret = nla_nest_end(msg, start);
    if (ret < 0) {
        LOGE("nla_nest_end failed!ret=%d\n", ret);
        goto error;
    }
    u32_boolean result = 0;
    ret = requestResponse(msg, &result);
    LOGD("%s: exit:%d,result:%d\n", __func__, ret, result);
    return result;
error:
    nlmsg_free(msg);
    return 0;
}

void removeFromPassthroughList(char *networkInterface, char *qname)
{
    LOGD("%s:\n", __func__);
    if (networkInterface)
        LOGD("%s: networkInterface:%s\n", __func__, networkInterface);
    if (qname)
        LOGD("%s: qname:%s\n", __func__, qname);
    struct nl_msg *msg = NULL;
    int ret = 0;
    msg = nlmsg_create(WIFI_MDNS_OFFLOAD_REMOVE_FROM_PASSTHROUGH_LIST);
    if (msg == NULL) {
        LOGE("nlmsg_create failed!\n");
        return;
    }
    struct nlattr *start = nla_nest_start(msg, NL80211_ATTR_VENDOR_DATA);
    if (start == NULL) {
        LOGE("nla_nest_start failed!\n");
        goto error;
    }
    if (networkInterface) {
        ret = nla_put(msg, WIFI_MDNS_OFFLOAD_ATTRIBUTE_NETWORK_INTERFACE,
            strlen(networkInterface) + 1, networkInterface);
        if (ret < 0) {
            LOGE("line:%d,nla_put failed!ret=%d\n", __LINE__, ret);
            goto error;
        }
    }
    if (qname) {
        ret = nla_put(msg, WIFI_MDNS_OFFLOAD_ATTRIBUTE_QNAME,
            strlen(qname) + 1, qname);
        if (ret < 0) {
            LOGE("line:%d,nla_put failed!ret=%d\n", __LINE__, ret);
            goto error;
        }
    }
    ret = nla_nest_end(msg, start);
    if (ret < 0) {
        LOGE("nla_nest_end failed!ret=%d\n", ret);
        goto error;
    }
    ret = requestResponse(msg, NULL);
    LOGD("%s: exit:%d\n", __func__, ret);
    return;
error:
    nlmsg_free(msg);
}

void setPassthroughBehavior(char *networkInterface,
    passthroughBehavior behavior)
{
    if (networkInterface)
        LOGD("%s: networkInterface:%s\n", __func__, networkInterface);
    LOGD("%s: behavior:%d\n", __func__, behavior);
    struct nl_msg *msg = NULL;
    int ret = 0;
    msg = nlmsg_create(WIFI_MDNS_OFFLOAD_SET_PASSTHROUGH_BEHAVIOR);
    if (msg == NULL) {
        LOGE("nlmsg_create failed!\n");
        return;
    }
    struct nlattr *start = nla_nest_start(msg, NL80211_ATTR_VENDOR_DATA);
    if (start == NULL) {
        LOGE("nla_nest_start failed!\n");
        goto error;
    }
    if (networkInterface) {
        ret = nla_put(msg, WIFI_MDNS_OFFLOAD_ATTRIBUTE_NETWORK_INTERFACE,
            strlen(networkInterface) + 1, networkInterface);
        if (ret < 0) {
            LOGE("line:%d,nla_put failed!ret=%d\n", __LINE__, ret);
            goto error;
        }
    }
    ret = nla_put(msg, WIFI_MDNS_OFFLOAD_ATTRIBUTE_PASSTHROUGH_BEHAVIOR,
        sizeof(behavior), &behavior);
    if (ret < 0) {
        LOGE("line:%d,nla_put failed!ret=%d\n", __LINE__, ret);
        goto error;
    }
    ret = nla_nest_end(msg, start);
    if (ret < 0) {
        LOGE("nla_nest_end failed!ret=%d\n", ret);
        goto error;
    }
    ret = requestResponse(msg, NULL);
    LOGD("%s: exit:%d\n", __func__, ret);
    return;
error:
    nlmsg_free(msg);
}

#ifdef __cplusplus
}
#endif

