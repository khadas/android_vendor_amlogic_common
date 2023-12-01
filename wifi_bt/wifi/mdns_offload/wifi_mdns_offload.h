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

#ifndef __WIFI_MDNS_OFFLOAD_H__
#define __WIFI_MDNS_OFFLOAD_H__

#ifdef __cplusplus
extern "C"
{
#endif
#include <stdint.h>
typedef uint32_t u32_boolean;

#define GOOGLE_VENDOR_OUI 0x1A11

typedef enum {
    WIFI_MDNS_OFFLOAD_SET_STATE = 0x1664,
    WIFI_MDNS_OFFLOAD_RESET_ALL,
    WIFI_MDNS_OFFLOAD_ADD_PROTOCOL_RESPONSES,
    WIFI_MDNS_OFFLOAD_REMOVE_PROTOCOL_RESPONSES,
    WIFI_MDNS_OFFLOAD_GET_AND_RESET_HIT_COUNTER,
    WIFI_MDNS_OFFLOAD_GET_AND_RESET_MISS_COUNTER,
    WIFI_MDNS_OFFLOAD_ADD_TO_PASSTHROUGH_LIST,
    WIFI_MDNS_OFFLOAD_REMOVE_FROM_PASSTHROUGH_LIST,
    WIFI_MDNS_OFFLOAD_SET_PASSTHROUGH_BEHAVIOR,
} wifi_mdns_offload_subcmd_t;

typedef enum {
    WIFI_MDNS_OFFLOAD_ATTRIBUTE_NONE,
    WIFI_MDNS_OFFLOAD_ATTRIBUTE_STATE,
    WIFI_MDNS_OFFLOAD_ATTRIBUTE_NETWORK_INTERFACE,
    WIFI_MDNS_OFFLOAD_ATTRIBUTE_OFFLOAD_PKT_LEN,
    WIFI_MDNS_OFFLOAD_ATTRIBUTE_OFFLOAD_PKT_DATA,
    WIFI_MDNS_OFFLOAD_ATTRIBUTE_MATCH_CRITERIA_NUM,
    WIFI_MDNS_OFFLOAD_ATTRIBUTE_MATCH_CRITERIA_DATA,
    WIFI_MDNS_OFFLOAD_ATTRIBUTE_RECORD_KEY,
    WIFI_MDNS_OFFLOAD_ATTRIBUTE_QNAME,
    WIFI_MDNS_OFFLOAD_ATTRIBUTE_PASSTHROUGH_BEHAVIOR,
    WIFI_MDNS_OFFLOAD_ATTRIBUTE_MAX,
} wifi_mdns_offload_attr_t;

typedef struct {
    /* QTYPE RRTYPE */
    int type;
    /* RRNAME offset in the rawOffloadPacket */
    int nameOffset;
} matchCriteria;

typedef struct {
    unsigned char *rawOffloadPacket;
    uint32_t rawOffloadPacketLen;
    matchCriteria *matchCriteriaList;
    uint32_t matchCriteriaListNum;
} mdnsProtocolData;

typedef enum {
    /* All the queries are forwarded to the system without any modification */
    FORWARD_ALL,
    /* All the queries are dropped.*/
    DROP_ALL,
    /* Only the queries present in the passthrough list are forwarded
     * to the system without any modification.
    */
    PASSTHROUGH_LIST,
} passthroughBehavior;

typedef enum {
    LOG_STYLE_LOGCAT,
    LOG_STYLE_CONSOLE,
} log_style_t;

typedef enum {
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_VERBOSE,
} log_level_t;

#define LOG_ERROR_MASK (1 << LOG_LEVEL_ERROR)
#define LOG_WARNING_MASK ((1 << LOG_LEVEL_WARNING) | LOG_ERROR_MASK)
#define LOG_INFO_MASK ((1 << LOG_LEVEL_INFO) | LOG_WARNING_MASK)
#define LOG_DEBUG_MASK ((1 << LOG_LEVEL_DEBUG) | LOG_INFO_MASK)
#define LOG_VERBOSE_MASK ((1 << LOG_LEVEL_VERBOSE) | LOG_DEBUG_MASK)

#define LOGE(fmt, ...) \
    wifi_mdns_offload_log_out(LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) \
    wifi_mdns_offload_log_out(LOG_LEVEL_WARNING, fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...) \
    wifi_mdns_offload_log_out(LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define LOGD(fmt, ...) \
    wifi_mdns_offload_log_out(LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define LOGV(fmt, ...) \
    wifi_mdns_offload_log_out(LOG_LEVEL_VERBOSE, fmt, ##__VA_ARGS__)

#define PRINT printf

int wifi_mdns_offload_log_out(uint8_t level, const char *fmt, ...);
void wifi_mdns_offload_set_log(uint8_t style, uint32_t mask);

int wifi_mdns_offload_init();
int wifi_mdns_offload_deinit();

u32_boolean setOffloadState(u32_boolean enabled);
void resetAll();
int addProtocolResponses(char *networkInterface,
    mdnsProtocolData *offloadData);
void removeProtocolResponses(int recordKey);
int getAndResetHitCounter(int recordKey);
int getAndResetMissCounter();
u32_boolean addToPassthroughList(char *networkInterface, char *qname);
void removeFromPassthroughList(char *networkInterface, char *qname);
void setPassthroughBehavior(char *networkInterface,
    passthroughBehavior behavior);

#ifdef __cplusplus
}
#endif

#endif /* __WIFI_MDNS_OFFLOAD_H__ */

