/*
 * Copyright (C) 2017 The Android Open Source Project
 * Portions copyright (C) 2017 Broadcom Limited
 * Portions copyright 2020 NXP
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

typedef enum {
    NXP_ATTR_OFFLOAD_INVALID = 0,
    NXP_ATTR_OFFLOAD_PACKET_ID,
    NXP_ATTR_OFFLOAD_PACKET_ETHER_TYPE,
    NXP_ATTR_OFFLOAD_PACKET_IP_PKT,
    NXP_ATTR_OFFLOAD_PACKET_IP_PKT_LEN,
    NXP_ATTR_OFFLOAD_PACKET_SRC_MAC_ADDR,
    NXP_ATTR_OFFLOAD_PACKET_DST_MAC_ADDR,
    NXP_ATTR_OFFLOAD_PACKET_PERIOD_MSEC,
} WIFI_OFFLOAD_ATTRIBUTE;

typedef enum {
    START_SENDING_OFFLOAD_PACKET,
    STOP_SENDING_OFFLOAD_PACKET,
} CmdType;

///////////////////////////////////////////////////////////////////////////////
class SendOffloadPacket : public WifiCommand
{
private:
    u8 Id;
    u16 etherType;
    u8 *ipPacket;
    u16 packetLen;
    u8 *srcMacAddr;
    u8 *dstMacAddr;
    u32 periodMsec;
    CmdType Type;

public:
    SendOffloadPacket(wifi_request_id id, wifi_interface_handle iface, u16 ether_type,
                            u8 *ip_packet, u16 ip_packet_len, u8 *src_mac_addr, 
                            u8 *dst_mac_addr, u32 period_msec, CmdType cmdType)
    : WifiCommand("SendOffloadPacket", iface, 0)
    {
        Id = id;
        etherType = ether_type;
        ipPacket = ip_packet;
        packetLen = ip_packet_len;
        srcMacAddr = src_mac_addr;
        dstMacAddr = dst_mac_addr;
        periodMsec = period_msec;
        Type = cmdType;
    }

    SendOffloadPacket(wifi_request_id id, wifi_interface_handle iface, CmdType cmdType)
    : WifiCommand("SendOffloadPacket", iface, 0)
    {
        Id = id;
        Type = cmdType;
        etherType = 0;
        ipPacket = NULL;
        packetLen = 0;
        srcMacAddr = NULL;
        dstMacAddr = NULL;
        periodMsec = 0;
    }

    virtual int create(){
        int ret = 0;
        switch(Type){
            case START_SENDING_OFFLOAD_PACKET:
            {
                ret = mMsg.create(MARVELL_OUI, NXP_OFFLOAD_START_SENDING_PACKET);
                if (ret!= WIFI_SUCCESS) {
                    ALOGE("Failed to create message start sending offload packet - %d", ret);
                    return ret;
                }
                nlattr *data = mMsg.attr_start(NL80211_ATTR_VENDOR_DATA);
                ret = mMsg.put_u8(NXP_ATTR_OFFLOAD_PACKET_ID, Id);
                if (ret < 0)
                    return ret;
                if (etherType){
                    ret = mMsg.put_u16(NXP_ATTR_OFFLOAD_PACKET_ETHER_TYPE, etherType);
                    if(ret < 0)
                        return ret;
                }
                ret = mMsg.put_u16(NXP_ATTR_OFFLOAD_PACKET_IP_PKT_LEN, packetLen);
                if (ret < 0)
                    return ret;
                ret = mMsg.put(NXP_ATTR_OFFLOAD_PACKET_IP_PKT, ipPacket, packetLen);
                if(ret < 0)
                    return ret;
                ret = mMsg.put_addr(NXP_ATTR_OFFLOAD_PACKET_SRC_MAC_ADDR, srcMacAddr);
                if(ret < 0)
                    return ret;
                ret = mMsg.put_addr(NXP_ATTR_OFFLOAD_PACKET_DST_MAC_ADDR, dstMacAddr);
                if(ret < 0)
                    return ret;
                ret = mMsg.put_u32(NXP_ATTR_OFFLOAD_PACKET_PERIOD_MSEC, periodMsec);
                if(ret < 0)
                    return ret;
                mMsg.attr_end(data);
                break;
            }
            case STOP_SENDING_OFFLOAD_PACKET:
            {
                ret = mMsg.create(MARVELL_OUI, NXP_OFFLOAD_STOP_SENDING_PACKET);
                if (ret!= WIFI_SUCCESS) {
                    ALOGE("Failed to create message to stop sending offload packet - %d", ret);
                    return ret;
                }
                nlattr *data = mMsg.attr_start(NL80211_ATTR_VENDOR_DATA);
                ret = mMsg.put_u8(NXP_ATTR_OFFLOAD_PACKET_ID, Id);
                if (ret < 0)
                    return ret;
                mMsg.attr_end(data);
                break;
            }
            default:
                ALOGE("Unknown offload packet command");
                return WIFI_ERROR_UNKNOWN;
        }
    return WIFI_SUCCESS;
    }

    virtual int handleResponse(WifiEvent& reply) {
        return NL_OK;
    }
};


/* API to send specified mkeep_alive packet periodically. */
wifi_error wifi_start_sending_offloaded_packet(wifi_request_id id, wifi_interface_handle iface,
                                                        u16 ether_type, u8 *ip_packet, u16 ip_packet_len, 
                                                        u8 *src_mac_addr, u8 *dst_mac_addr, 
                                                        u32 period_msec)
{
    int ret = 0;
    if(!ip_packet || !src_mac_addr || !dst_mac_addr){
        ALOGE("The ip_packet/src_mac_addr/dst_mac_addr is NULL");
        return WIFI_ERROR_INVALID_ARGS;
    }
    SendOffloadPacket Start(id, iface, ether_type, ip_packet, ip_packet_len, src_mac_addr, 
                            dst_mac_addr, period_msec, START_SENDING_OFFLOAD_PACKET);
    ret = Start.requestResponse();
    return (wifi_error)ret;
}

/* API to stop sending mkeep_alive packet. */
wifi_error wifi_stop_sending_offloaded_packet(wifi_request_id id, wifi_interface_handle iface)
{
    int ret = 0;
    SendOffloadPacket Stop(id, iface, STOP_SENDING_OFFLOAD_PACKET);
    ret = Stop.requestResponse();
    return (wifi_error)ret;
}
