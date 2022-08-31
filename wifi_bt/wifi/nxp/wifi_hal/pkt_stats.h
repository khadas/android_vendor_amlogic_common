/*
* Copyright (C) 2014 The Android Open Source Project
* Portion copyright 2017-2020 NXP
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
#include "common.h"

#ifndef _PKT_STATS_H_
#define _PKT_STATS_H_

typedef struct {
    frame_type payload_type;
    u32 driver_timestamp_usec;
    u32 firmware_timestamp_usec;
    u32 frame_len;
    char *frame_content;
} frame_info_i;

typedef struct {
    // Prefix of MD5 hash of |frame_inf.frame_content|. If frame
    // content is not provided, prefix of MD5 hash over the same data
    // that would be in frame_content, if frame content were provided.
    char md5_prefix[MD5_PREFIX_LEN];  // Prefix of MD5 hash of packet bytes
    wifi_tx_packet_fate fate;
    frame_info_i frame_inf;
} wifi_tx_report_i;

typedef struct {
    // Prefix of MD5 hash of |frame_inf.frame_content|. If frame
    // content is not provided, prefix of MD5 hash over the same data
    // that would be in frame_content, if frame content were provided.
    char md5_prefix[MD5_PREFIX_LEN];
    wifi_rx_packet_fate fate;
    frame_info_i frame_inf;
} wifi_rx_report_i;

typedef struct {
    wifi_tx_report_i tx_fate_stats[MAX_FATE_LOG_LEN];
    size_t n_tx_stats_collected;
    wifi_rx_report_i rx_fate_stats[MAX_FATE_LOG_LEN];
    size_t n_rx_stats_collected;
} packet_fate_monitor_info;

#endif
