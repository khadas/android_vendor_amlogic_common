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
#include "nan_nxp.h"
#include <utils/String8.h>

using namespace android;

typedef enum{
    NXP_ATTR_NAN_INVALID = 0,
    NXP_ATTR_NAN_FAKE,
    NXP_ATTR_NAN_IND,
    NXP_ATTR_NAN_TRANSTIION_ID,
    NXP_ATTR_NAN_RSP_TYPE,
    NXP_ATTR_NAN_ENABLE_REQ,
    NXP_ATTR_NAN_PUBLISH_REQ,
    NXP_ATTR_NAN_PUBLISH_CANCEL_REQ,
    NXP_ATTR_NAN_SUBSCRIBE_REQ,
    NXP_ATTR_NAN_SUBSCRIBE_CANCEL_REQ,
    NXP_ATTR_NAN_TRANSMIT_FOLLOWUP_REQ,
    NXP_ATTR_NAN_STATS_REQ,
    NXP_ATTR_NAN_CONFIG_REQ,
    NXP_ATTR_NAN_TCA_REQ,
    NXP_ATTR_NAN_BEACON_SDF_PAYLOAD_REQ,
    NXP_ATTR_NAN_VERSION_REQ,
    NXP_ATTR_NAN_DATA_IF_REQ,
    NXP_ATTR_NAN_REQ_INITOR_REQ,
    NXP_ATTR_NAN_INDI_RESP_REQ,
    NXP_ATTR_NAN_DATA_END,
    NXP_ATTR_NOTIFY_RESPONSE,
    NXP_ATTR_PUBLISH_TERMINATED,
    NXP_ATTR_MATCH,
    NXP_ATTR_MATCH_EXPIRED,
    NXP_ATTR_SUBSCRIBE_TERMINATED,
    NXP_ATTR_FOLLOWUP,
    NXP_ATTR_DISC_ENG_EVENT,
    NXP_ATTR_DISABLED,
    NXP_ATTR_TCA,
    NXP_ATTR_BEACON_SDF_PAYLOAD,
    NXP_ATTR_DATA_REQUEST,
    NXP_ATTR_DATA_CONFIRM,
    NXP_ATTR_DATA_END,
    NXP_ATTR_TRANSMIT_FOLLOWUP,
    NXP_ATTR_RANGE_REQUEST,
    NXP_ATTR_RANGE_REPORT,

    NXP_ATTR_NAN_AFTER_LAST,
    NXP_ATTR_NAN_MAX = 
    NXP_ATTR_NAN_AFTER_LAST - 1,
} NAN_ATTRIBUTES;

typedef struct
{
    u16 MsgId;
    u16 MsgLen;
    u16 handle;
    u16 transactionId;
} NanHeader;

typedef struct 
{
    NanHeader header;
    u16 status;
    u16 value;
} NanRespHeader;

typedef enum{
    NXP_EVENT_NAN = 0x10010,
} NAN_EVENTS;

struct errorCode {
    NanStatusType frameworkError;
    NanInternalStatusType firmwareError;
    char nan_error[NAN_ERROR_STR_LEN];
};

struct errorCode errorCodeTranslation[] = {
    {NAN_STATUS_SUCCESS, NAN_I_STATUS_SUCCESS,
     "NAN status success"},

    {NAN_STATUS_INTERNAL_FAILURE, NAN_I_STATUS_DE_FAILURE,
     "NAN Discovery engine failure"},

    {NAN_STATUS_INVALID_PUBLISH_SUBSCRIBE_ID, NAN_I_STATUS_INVALID_HANDLE,
     "Invalid Publish/Subscribe ID"},

    {NAN_STATUS_NO_RESOURCE_AVAILABLE, NAN_I_STATUS_NO_SPACE_AVAILABLE,
     "No space available"},

    {NAN_STATUS_INVALID_PARAM, NAN_I_STATUS_INVALID_PUBLISH_TYPE,
     "Invalid Publish type, can be 0 or 1"},
    {NAN_STATUS_INVALID_PARAM, NAN_I_STATUS_INVALID_TX_TYPE,
     "Invalid Tx type"},
    {NAN_STATUS_INVALID_PARAM, NAN_I_STATUS_INVALID_MSG_VERSION,
     "Invalid internal message version"},
    {NAN_STATUS_INVALID_PARAM, NAN_I_STATUS_INVALID_MSG_LEN,
     "Invalid message length"},
    {NAN_STATUS_INVALID_PARAM, NAN_I_STATUS_INVALID_MSG_ID,
     "Invalid message ID"},
    {NAN_STATUS_INVALID_PARAM, NAN_I_STATUS_INVALID_MATCH_ALGORITHM,
     "Invalid matching algorithm, can be 0(match once), 1(match continuous) or 2(match never)"},
    {NAN_STATUS_INVALID_PARAM, NAN_I_STATUS_INVALID_TLV_LEN,
     "Invalid TLV length"},
    {NAN_STATUS_INVALID_PARAM, NAN_I_STATUS_INVALID_TLV_TYPE,
     "Invalid TLV type"},
    {NAN_STATUS_INVALID_PARAM, NAN_I_STATUS_MISSING_TLV_TYPE,
     "Missing TLV type"},
    {NAN_STATUS_INVALID_PARAM, NAN_I_STATUS_INVALID_TOTAL_TLVS_LEN,
     "Invalid total TLV length"},
    {NAN_STATUS_INVALID_PARAM, NAN_I_STATUS_INVALID_TLV_VALUE,
     "Invalid TLV value"},
    {NAN_STATUS_INVALID_PARAM, NAN_I_STATUS_INVALID_TX_PRIORITY,
     "Invalid Tx priority"},
    {NAN_STATUS_INVALID_PARAM, NAN_I_STATUS_INVALID_CONNECTION_MAP,
     "Invalid connection map"},
    {NAN_STATUS_INVALID_PARAM, NAN_I_STATUS_INVALID_THRESHOLD_CROSSING_ALERT_ID,
     "Invalid TCA-Threshold Crossing Alert ID"},
    {NAN_STATUS_INVALID_PARAM, NAN_I_STATUS_INVALID_STATS_ID,
     "Invalid STATS ID"},

    {NAN_STATUS_PROTOCOL_FAILURE, NAN_I_STATUS_TX_FAIL,
     "Tx Fail"},

    {NAN_STATUS_INVALID_PARAM, NAN_I_STATUS_INVALID_RSSI_CLOSE_VALUE,
     "Invalid RSSI close value range is 20dbm to 60dbm"},
    {NAN_STATUS_INVALID_PARAM, NAN_I_STATUS_INVALID_RSSI_MIDDLE_VALUE,
     "Invalid RSSI middle value range is 20dbm to 75dbm"},
    {NAN_STATUS_INVALID_PARAM, NAN_I_STATUS_INVALID_HOP_COUNT_LIMIT,
     "Invalid hop count limit, max hop count limit is 5"},
    {NAN_STATUS_INVALID_PARAM, NAN_I_STATUS_INVALID_HIGH_CLUSTER_ID_VALUE,
     "Invalid cluster ID value. Please set the cluster id high greater than the cluster id low"},
    {NAN_STATUS_INVALID_PARAM, NAN_I_STATUS_INVALID_BACKGROUND_SCAN_PERIOD,
     "Invalid background scan period. The range is 10 to 30 milliseconds"},
    {NAN_STATUS_INVALID_PARAM, NAN_I_STATUS_INVALID_SCAN_CHANNEL,
     "Invalid scan channel. Only valid channels are the NAN social channels"},
    {NAN_STATUS_INVALID_PARAM, NAN_I_STATUS_INVALID_POST_NAN_CONNECTIVITY_CAPABILITIES_BITMAP,
     "Invalid post nan connectivity bitmap"},
    {NAN_STATUS_INVALID_PARAM, NAN_I_STATUS_INVALID_FURTHER_AVAILABILITY_MAP_NUMCHAN_VALUE,
     "Invalid further availability map number of channel value"},
    {NAN_STATUS_INVALID_PARAM, NAN_I_STATUS_INVALID_FURTHER_AVAILABILITY_MAP_DURATION_VALUE,
     "Invalid further availability map duration value"},
    {NAN_STATUS_INVALID_PARAM, NAN_I_STATUS_INVALID_FURTHER_AVAILABILITY_MAP_CLASS_VALUE,
     "Invalid further availability map class value"},
    {NAN_STATUS_INVALID_PARAM, NAN_I_STATUS_INVALID_FURTHER_AVAILABILITY_MAP_CHANNEL_VALUE,
     "Invalid further availability map channel value"},
    {NAN_STATUS_INVALID_PARAM, NAN_I_STATUS_INVALID_FURTHER_AVAILABILITY_MAP_AVAILABILITY_INTERVAL_BITMAP_VALUE,
     "Invalid further availability map availability interval bitmap value"},
    {NAN_STATUS_INVALID_PARAM, NAN_I_STATUS_INVALID_FURTHER_AVAILABILITY_MAP_MAP_ID,
     "Invalid further availability map map ID"},
    {NAN_STATUS_INVALID_PARAM, NAN_I_STATUS_INVALID_POST_NAN_DISCOVERY_CONN_TYPE_VALUE,
     "Invalid post nan discovery connection type value"},
    {NAN_STATUS_INVALID_PARAM, NAN_I_STATUS_INVALID_POST_NAN_DISCOVERY_DEVICE_ROLE_VALUE,
     "Invalid post nan discovery device role value"},
    {NAN_STATUS_INVALID_PARAM, NAN_I_STATUS_INVALID_POST_NAN_DISCOVERY_DURATION_VALUE,
     "Invalid post nan discovery duration value"},
    {NAN_STATUS_INVALID_PARAM, NAN_I_STATUS_INVALID_POST_NAN_DISCOVERY_BITMAP_VALUE,
     "Invalid post nan discovery bitmap value"},
    {NAN_STATUS_INVALID_PARAM, NAN_I_STATUS_MISSING_FUTHER_AVAILABILITY_MAP,
     "Missing further availability map"},
    {NAN_STATUS_INVALID_PARAM, NAN_I_STATUS_INVALID_BAND_CONFIG_FLAGS,
     "Invalid band configuration flags"},
    {NAN_STATUS_INVALID_PARAM, NAN_I_STATUS_INVALID_RANDOM_FACTOR_UPDATE_TIME_VALUE,
     "Invalid random factor update time value"},
    {NAN_STATUS_INVALID_PARAM, NAN_I_STATUS_INVALID_ONGOING_SCAN_PERIOD,
     "Invalid ongoing scan period"},
    {NAN_STATUS_INVALID_PARAM, NAN_I_STATUS_INVALID_DW_INTERVAL_VALUE,
     "Invalid DW interval value"},
    {NAN_STATUS_INVALID_PARAM, NAN_I_STATUS_INVALID_DB_INTERVAL_VALUE,
     "Invalid DB interval value"},

    {NAN_STATUS_SUCCESS, NAN_I_PUBLISH_SUBSCRIBE_TERMINATED_REASON_TIMEOUT,
     "Terminated Reason: Timeout"},
    {NAN_STATUS_SUCCESS, NAN_I_PUBLISH_SUBSCRIBE_TERMINATED_REASON_USER_REQUEST,
     "Terminated Reason: User Request"},
    {NAN_STATUS_SUCCESS, NAN_I_PUBLISH_SUBSCRIBE_TERMINATED_REASON_COUNT_REACHED,
     "Terminated Reason: Count Reached"},

    {NAN_STATUS_INVALID_REQUESTOR_INSTANCE_ID, NAN_I_STATUS_INVALID_REQUESTER_INSTANCE_ID,
     "Invalid match handle"},
    {NAN_STATUS_NAN_NOT_ALLOWED, NAN_I_STATUS_NAN_NOT_ALLOWED,
     "Nan not allowed"},
    {NAN_STATUS_NO_OTA_ACK, NAN_I_STATUS_NO_OTA_ACK,
     "No OTA ack"},
    {NAN_STATUS_ALREADY_ENABLED, NAN_I_STATUS_NAN_ALREADY_ENABLED,
     "NAN is Already enabled"},
    {NAN_STATUS_FOLLOWUP_QUEUE_FULL, NAN_I_STATUS_FOLLOWUP_QUEUE_FULL,
     "Follow-up queue full"},

    {NAN_STATUS_UNSUPPORTED_CONCURRENCY_NAN_DISABLED, NDP_I_UNSUPPORTED_CONCURRENCY,
     "Unsupported Concurrency"},

    {NAN_STATUS_INTERNAL_FAILURE, NDP_I_NAN_DATA_IFACE_CREATE_FAILED,
     "NAN data interface create failed"},
    {NAN_STATUS_INTERNAL_FAILURE, NDP_I_NAN_DATA_IFACE_DELETE_FAILED,
     "NAN data interface delete failed"},
    {NAN_STATUS_INTERNAL_FAILURE, NDP_I_DATA_INITIATOR_REQUEST_FAILED,
     "NAN data initiator request failed"},
    {NAN_STATUS_INTERNAL_FAILURE, NDP_I_DATA_RESPONDER_REQUEST_FAILED,
     "NAN data responder request failed"},

    {NAN_STATUS_INVALID_NDP_ID, NDP_I_INVALID_NDP_INSTANCE_ID,
     "Invalid NDP instance ID"},

    {NAN_STATUS_INVALID_PARAM, NDP_I_INVALID_RESPONSE_CODE,
     "Invalid response code"},
    {NAN_STATUS_INVALID_PARAM, NDP_I_INVALID_APP_INFO_LEN,
     "Invalid app info length"},

    {NAN_STATUS_PROTOCOL_FAILURE, NDP_I_MGMT_FRAME_REQUEST_FAILED,
     "Management frame request failed"},
    {NAN_STATUS_PROTOCOL_FAILURE, NDP_I_MGMT_FRAME_RESPONSE_FAILED,
     "Management frame response failed"},
    {NAN_STATUS_PROTOCOL_FAILURE, NDP_I_MGMT_FRAME_CONFIRM_FAILED,
     "Management frame confirm failed"},

    {NAN_STATUS_INTERNAL_FAILURE, NDP_I_END_FAILED,
     "NDP end failed"},

    {NAN_STATUS_PROTOCOL_FAILURE, NDP_I_MGMT_FRAME_END_REQUEST_FAILED,
     "Management frame end request failed"},

    {NAN_STATUS_INTERNAL_FAILURE, NDP_I_VENDOR_SPECIFIC_ERROR,
     "Vendor specific error"}
};

///////////////////////////////////////////////////////////////////////
class NanControlCommand : public WifiCommand
{
private:
    transaction_id mId;
    NanEnableRequest *Msg;
    NanHeader header;
    NanNxpRequestType mtype; 

public:
    // Constructor for nan enable
    NanControlCommand(transaction_id id, wifi_interface_handle iface, 
                              NanNxpRequestType type, NanEnableRequest *msg)
    : WifiCommand("NanControlCommand", iface, 0)
    {
        mId = id;
        Msg = msg;
        mtype = type;
        memset(&header, 0, sizeof(NanHeader));
    }

    // Constructor for nan disable
    NanControlCommand(transaction_id id, wifi_interface_handle iface,
                              NanNxpRequestType type)
    : WifiCommand("NanControlCommand", iface, 0)
    {
        mId = id;
        mtype = type;
        memset(&header, 0, sizeof(NanHeader));
    }

    int createRequest(WifiRequest& request) {
        int enable_len = sizeof(NanHeader);
        u32 type = 0;
        int result = request.create(MARVELL_OUI, NXP_SUBCMD_NAN_ENABLE_REQ);
        if (result < 0) {
            ALOGE("Failed to create message to enable nan\n");
            return result;
        }
        header.MsgId = NAN_MSG_ID_ENABLE;
        header.transactionId = mId;
        type = NAN_DE_MAC_ADDR;

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
        result = request.put(NXP_ATTR_NAN_FAKE, &header, enable_len);
        if (result != WIFI_SUCCESS) {
            ALOGE("Failed to put NanHeader, result = %d", result);
            return result;
        }

        if (mtype == NAN_REQUEST_ENABLED) {
            result = request.put_u32(NXP_ATTR_NAN_IND, type);
            if (result != WIFI_SUCCESS) {
                ALOGE("Failed to put IND type, result = %d", result);
                return result;
            }
            result = request.put_u32(NXP_ATTR_NAN_IND, type);
            if (result != WIFI_SUCCESS) {
                ALOGE("Failed to put IND type, result = %d", result);
                return result;
            }
        }
//        result = request.put(ATTR_NAN_ENABLE_REQ, (void *)Msg, len);
//        if (result < 0) {
//            ALOGE("Failed to put msg, result = %d", result);
//            return result;
//        }
        request.attr_end(data);
        return WIFI_SUCCESS;
    }

    int createTeardownRequest(WifiRequest &request) {

        int disable_len = sizeof(NanHeader);
        u32 type = 0;

        int result = request.create(MARVELL_OUI, NXP_SUBCMD_NAN_DISABLE_REQ);
        if(result < 0) {
            ALOGE("Failed to create message to enable nan\n");
            return result;
        }
        header.MsgId = NAN_MSG_ID_DISABLE;
        header.transactionId = mId;
        type = NAN_DE_MAC_ADDR;

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
        result = request.put(NXP_ATTR_NAN_FAKE, &header, disable_len);
        if (result != WIFI_SUCCESS) {
            ALOGE("Failed to put NanHeader, result = %d", result);
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
            ALOGE("Failed to enable nan, result = %d", result);
            return result;
        }
        return result;
    }

    virtual int cancel_specific() {

        WifiRequest request(familyId(), ifaceId());
        int result = createTeardownRequest(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("failed to create request; result = %d", result);
        } else {
            result = requestResponse(request);
            if (result != WIFI_SUCCESS) {
                ALOGE("failed to disable nan = %d", result);
            }
        }
        return result;
    }
};

class NanPublishReqCommand : public WifiCommand
{
private:
    transaction_id mId;
    NanPublishRequest* Preq;
    NanPublishCancelRequest* Pcancelreq;
    NanHeader publishHeader;

public:
    //Constructor for publish request
    NanPublishReqCommand(transaction_id id, wifi_interface_handle iface, 
                                   NanPublishRequest *msg)
    : WifiCommand("NanPublishReqCommand", iface, 0)
    {
        mId = id;
        Preq = msg;
        memset(&publishHeader, 0, sizeof(NanHeader));
        Pcancelreq = NULL;
    }

    //Constructor for publish cancel request
    NanPublishReqCommand(transaction_id id, wifi_interface_handle iface, 
                                   NanPublishCancelRequest *msg)
    : WifiCommand("NanPublishReqCommand", iface, 0)
    {
        mId = id;
        Pcancelreq = msg;
        memset(&publishHeader, 0, sizeof(NanHeader));
    }

    int createRequest(WifiRequest& request) {
        int len = sizeof(NanHeader);
        int result = request.create(MARVELL_OUI, NXP_SUBCMD_NAN_PUBLISH_REQ);
        if (result < 0)
            return result;
        publishHeader.MsgId = NAN_MSG_ID_PUBLISH_SERVICE;
        publishHeader.transactionId = mId;
        publishHeader.handle = Preq->publish_id;

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
        result = request.put(NXP_ATTR_NAN_FAKE, &publishHeader, len);
        if (result < 0) {
            ALOGE("Failed to create publish request message");
            return result;
        }

        if (Preq->ttl > 0) {
            ALOGV("publish limit ttl = %d", Preq->ttl);
            result = request.put_u32(NXP_ATTR_NAN_IND, Preq->ttl);
            if (result != WIFI_SUCCESS) {
                ALOGE("Failed to put IND type, result = %d", result);
                return result;
            }
        }
        request.attr_end(data);
        return WIFI_SUCCESS;
    }

    int createTeardownRequest(WifiRequest &request) {
        int len = sizeof(NanHeader);
        int result = request.create(MARVELL_OUI, NXP_SUBCMD_NAN_PUBLISH_CANCEL);
        if(result < 0) {
            ALOGE("Failed to create publish cancel message");
            return result;
        }
        publishHeader.MsgId = NAN_MSG_ID_PUBLISH_SERVICE_CANCEL;
        publishHeader.transactionId = mId;
        publishHeader.handle = Pcancelreq->publish_id;

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
        result = request.put(NXP_ATTR_NAN_FAKE, &publishHeader, len);
        if (result < 0)
            return result;
        request.attr_end(data);
        return WIFI_SUCCESS;
    }

    virtual int start(){
        WifiRequest request(familyId(), ifaceId());
        int result = createRequest(request);
        if (result < 0) {
            return result;
        }
        result = requestResponse(request);
        if (result < 0) {
            ALOGE("Failed to request publish, result = %d", result);
            return result;
        }
        return result;
    }

    virtual int cancel_specific() {
        WifiRequest request(familyId(), ifaceId());
        int result = createTeardownRequest(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("failed to create request; result = %d", result);
        } else {
            result = requestResponse(request);
            if (result != WIFI_SUCCESS) {
                ALOGE("failed to cancel publish = %d", result);
            }
        }
        return result;
    }
};

class SubscribeReqCommand : public WifiCommand
{
private:
    transaction_id mId;
    NanSubscribeRequest* Msg;
    NanSubscribeCancelRequest* cMsg;
    NanHeader subscribeHeader;

public:
    //Constructor for subscribe request
    SubscribeReqCommand(transaction_id id, wifi_interface_handle iface, 
                                 NanSubscribeRequest* msg)
    : WifiCommand("SubscribeReqCommand", iface, 0)
    {
        mId = id;
        Msg = msg;
        memset(&subscribeHeader, 0, sizeof(NanHeader));
    }

    //Constructor for subscribe cancel request
    SubscribeReqCommand(transaction_id id, wifi_interface_handle iface, 
                                 NanSubscribeCancelRequest* msg)
    : WifiCommand("SubscribeReqCommand", iface, 0)
    {
        mId = id;
        cMsg = msg;
        memset(&subscribeHeader, 0, sizeof(NanHeader));
    }

    int createRequest(WifiRequest& request) {
        int len = sizeof(NanHeader);
        int result = request.create(MARVELL_OUI, NXP_SUBCMD_NAN_SUBSCRIBE_REQ);
        if (result < 0)
            return result;
        subscribeHeader.MsgId = NAN_MSG_ID_SUBSCRIBE_SERVICE;
        subscribeHeader.transactionId = mId;
        subscribeHeader.handle = Msg->subscribe_id;

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
        result = request.put(NXP_ATTR_NAN_FAKE, &subscribeHeader, len);
        if (result < 0)
            return result;
        if (Msg->ttl > 0) {
            ALOGV("subscribe limit ttl = %d", Msg->ttl);
            result = request.put_u32(NXP_ATTR_NAN_IND, Msg->ttl);
            if (result != WIFI_SUCCESS) {
                ALOGE("Failed to put IND type, result = %d", result);
                return result;
            }
        }
        request.attr_end(data);
        return result;
    }

    int createTeardownRequest(WifiRequest &request) {
        int len = sizeof(NanHeader);
        int result = request.create(MARVELL_OUI, NXP_SUBCMD_NAN_SUBSCRIBE_CANCEL);
        if(result < 0)
            return result;
        subscribeHeader.MsgId = NAN_MSG_ID_SUBSCRIBE_SERVICE_CANCEL;
        subscribeHeader.transactionId = mId;
        subscribeHeader.handle = cMsg->subscribe_id;

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
        result = request.put(NXP_ATTR_NAN_FAKE, &subscribeHeader, len);
        if (result < 0)
            return result;
        request.attr_end(data);
        return result;
    }

    virtual int start(){
        WifiRequest request(familyId(), ifaceId());
        int result = createRequest(request);
        if (result < 0)
            return result;
        result = requestResponse(request);
        if (result < 0) {
            ALOGE("Failed to request subscribe, result = %d", result);
            return result;
        }
        return WIFI_SUCCESS;
    }

    virtual int cancel_specific() {
        WifiRequest request(familyId(), ifaceId());
        int result = createTeardownRequest(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("failed to create request; result = %d", result);
            return result;
        } else {
            result = requestResponse(request);
            if (result != WIFI_SUCCESS) {
                ALOGE("failed to cancel subscribe = %d", result);
                return result;
            }
        }
        return WIFI_SUCCESS;
    }
};

class TransmitFollowupCommand : public WifiCommand
{
private:
    transaction_id mId;
    NanTransmitFollowupRequest* Msg;
    NanHeader transmitHeader;

public:
    TransmitFollowupCommand(transaction_id id, wifi_interface_handle iface, 
                                      NanTransmitFollowupRequest *msg)
    : WifiCommand("TransmitFollowupCommand", iface, 0)
    {
        mId = id;
        Msg = msg;
        memset(&transmitHeader, 0, sizeof(NanHeader));
    }

    virtual int create(){
        int ret = mMsg.create(MARVELL_OUI, NXP_SUBCMD_NAN_TRANSMIT_FOLLOWUP);
        int len = sizeof(NanHeader);
        if (ret < 0) {
            ALOGE("Failed to create message to request transmit followup\n");
            return ret;
        }
        transmitHeader.MsgId = NAN_MSG_ID_TRANSMIT_FOLLOWUP;
        transmitHeader.transactionId = mId;
        transmitHeader.handle = Msg->publish_subscribe_id;

        nlattr *data = mMsg.attr_start(NL80211_ATTR_VENDOR_DATA);
        ret = mMsg.put(NXP_ATTR_NAN_FAKE, &transmitHeader, len);
        if (ret < 0)
            return ret;
        mMsg.attr_end(data);
        return WIFI_SUCCESS;
    }
};

class StatsRequestCommand : public WifiCommand
{
private:
    transaction_id mId;
    NanStatsRequest *Msg;
    NanHeader statsHeader;

public:
    StatsRequestCommand(transaction_id id, wifi_interface_handle iface, 
                                NanStatsRequest *msg)
    : WifiCommand("StatsRequestCommand", iface, 0)
    {
        mId = id;
        Msg = msg;
        memset(&statsHeader, 0, sizeof(NanHeader));
    }

    virtual int create(){
        int ret = mMsg.create(MARVELL_OUI, NXP_SUBCMD_NAN_STATS_REQ);
        int len = sizeof(NanHeader);
        if (ret < 0) {
            ALOGE("Failed to create message to request nan statistics from Discovery Engine\n");
            return ret;
        }
        statsHeader.MsgId = NAN_MSG_ID_STATS;
        statsHeader.transactionId = mId;
        nlattr *data = mMsg.attr_start(NL80211_ATTR_VENDOR_DATA);
        ret = mMsg.put(NXP_ATTR_NAN_FAKE, &statsHeader, len);
        if (ret < 0)
            return ret;
        mMsg.attr_end(data);
        return WIFI_SUCCESS;
    }
};

class ConfigRequestCommand : public WifiCommand
{
private:
    transaction_id mId;
    NanConfigRequest *Msg;
    NanHeader configHeader;

public:
    ConfigRequestCommand(transaction_id id, wifi_interface_handle iface, 
                                  NanNxpRequestType type, NanConfigRequest *msg)
    : WifiCommand("ConfigRequestCommand", iface, 0)
    {
        mId = id;
        Msg = msg;
        memset(&configHeader, 0, sizeof(NanHeader));
    }

    virtual int create(){
        int ret = mMsg.create(MARVELL_OUI, NXP_SUBCMD_NAN_CONFIG_REQ);
        int len = sizeof(NanHeader);
        if (ret < 0) {
            ALOGE("Failed to create message to request nan configuration\n");
            return ret;
        }
        configHeader.MsgId = NAN_MSG_ID_CONFIGURATION;
        configHeader.transactionId = mId;

        nlattr *data = mMsg.attr_start(NL80211_ATTR_VENDOR_DATA);
        ret = mMsg.put(NXP_ATTR_NAN_FAKE, &configHeader, len);
        if (ret != WIFI_SUCCESS) {
            ALOGE("Failed to put config_header, result = %d", ret);
            return ret;
        }
//        ret = mMsg.put(ATTR_NAN_CONFIG_REQ, (void *)Msg, len);
//        if (ret < 0) {
//            return ret;
//        }
        mMsg.attr_end(data);
        return WIFI_SUCCESS;
    }
};

class TcaRequestCommand : public WifiCommand
{
private:
    transaction_id mId;
    NanTCARequest *Msg;
    NanHeader tcaHeader;

public:
    TcaRequestCommand(transaction_id id, wifi_interface_handle iface, 
                               NanTCARequest *msg)
    : WifiCommand("TcaRequestCommand", iface, 0)
    {
        mId =  id;
        Msg = msg;
        memset(&tcaHeader, 0, sizeof(NanHeader));
    }

    virtual int create(){
        int ret = mMsg.create(MARVELL_OUI, NXP_SUBCMD_NAN_TCA_REQ);
        int len = sizeof(NanHeader);
        if (ret < 0) {
            ALOGE("Failed to create message to configure the various threshold crossing alerts\n");
            return ret;
        }
        tcaHeader.MsgId = NAN_MSG_ID_TCA;
        tcaHeader.transactionId = mId;

        nlattr *data = mMsg.attr_start(NL80211_ATTR_VENDOR_DATA);
        ret = mMsg.put(NXP_ATTR_NAN_FAKE, &tcaHeader, len);
        if (ret < 0)
            return ret;
        mMsg.attr_end(data);
        return WIFI_SUCCESS;
    }
};

class BeaconSDFPayload : public WifiCommand
{
private:
    transaction_id mId;
    NanBeaconSdfPayloadRequest* Msg;
    NanHeader SDFHeader;

public:
    BeaconSDFPayload(transaction_id id, wifi_interface_handle iface, 
                            NanBeaconSdfPayloadRequest* msg)
    : WifiCommand("BeaconSDFPayload", iface, 0)
    {
        mId = id;
        Msg = msg;
        memset(&SDFHeader, 0, sizeof(NanHeader));
    }

    virtual int create(){
        int ret = mMsg.create(MARVELL_OUI, NXP_SUBCMD_NAN_BEACON_SDF_PAYLOAD);
        int len = sizeof(NanHeader);
        if (ret < 0) {
            ALOGE("Failed to create message to set beacon or sdf payload to DE\n");
            return ret;
        }
        SDFHeader.MsgId = NAN_MSG_ID_BEACON_SDF;
        SDFHeader.transactionId = mId;
        nlattr *data = mMsg.attr_start(NL80211_ATTR_VENDOR_DATA);
        ret = mMsg.put(NXP_ATTR_NAN_FAKE, &SDFHeader, len);
        if (ret < 0)
            return ret;
        mMsg.attr_end(data);
        return WIFI_SUCCESS;
    }
};

static int enable_num = 0;

class SetRegisterHandlerCommand : public WifiCommand
{
private:
    NanCallbackHandler mHandler;
    NanPublishTerminatedInd mNptInd;
//    NanMatchInd *mNmInd;
//    NanMatchExpiredInd *mNmeInd;
    NanSubscribeTerminatedInd mNstInd;
//    NanFollowupInd *mNfpInd;
    NanDiscEngEventInd mNDEeventInd;
//    NanDisabledInd *mNdisabledInd;
//    NanTCAInd *mNTCAInd;
//    NanBeaconSdfPayloadInd *mNbspInd;
//    NanDataPathRequestInd *mNDPreqInd;
//    NanDataPathConfirmInd *mNDPconfirmInd;
//    NanDataPathEndInd *mNDPendInd;
//    NanTransmitFollowupInd *mNtransfpInd;
//    NanRangeRequestInd *mNRangereqInd;
//    NanRangeReportInd *mNRangerepInd;

public:
    SetRegisterHandlerCommand(wifi_interface_handle iface, 
                                        NanCallbackHandler handlers)
    : WifiCommand("SetRegisterHandlerCommand", iface, 0)
    {
        mHandler = handlers;
    }

    int start(){
        ALOGV("Register Nan handler");
        registerVendorHandler(MARVELL_OUI, NXP_EVENT_NAN);
        return WIFI_SUCCESS;
    }

    virtual int handleEvent(WifiEvent & event){
        ALOGV("***Got a NAN event***");
        if (event.get_cmd() != NL80211_CMD_VENDOR) {
            ALOGE("Ignoring reply with cmd = %d", event.get_cmd());
            return NL_SKIP;
        }

        nlattr *vendor_data =(nlattr *) (event.get_data(NL80211_ATTR_VENDOR_DATA));
        int len = event.get_vendor_data_len();
        ALOGV("len = %d\n", len);
//        hexdump((void *)vendor_data, len);

        if (vendor_data == NULL || len == 0) {
           ALOGE("NAN: No data");
           return NL_SKIP;
        }

        struct nlattr *tb_vendor[NXP_ATTR_NAN_MAX + 1];
        NanRespHeader respHeader;
        NanResponseMsg rsp_data;
//        rsp_data = (NanResponseMsg *)malloc(sizeof(NanResponseMsg));
        memset(&rsp_data, 0, sizeof(NanResponseMsg));
        memset(&respHeader, 0, sizeof(NanRespHeader));
        nla_parse(tb_vendor, NXP_ATTR_NAN_MAX, vendor_data, len, NULL);

        ALOGV("handle Event: start parser tb_vendor");

        if (tb_vendor[NXP_ATTR_NAN_FAKE]){
            ALOGV("handle Event:ATTR_NAN_FAKE");
            memcpy(&respHeader, (NanRespHeader *)nla_data(tb_vendor[NXP_ATTR_NAN_FAKE]), sizeof(NanRespHeader));
            switch(respHeader.header.MsgId){
                case NAN_MSG_ID_ENABLE:
                {
                    ALOGV("handle Event:NAN_MSG_ID_ENABLE");
                    enable_num++;
                    rsp_data.status = NAN_STATUS_SUCCESS;
                    strlcpy(rsp_data.nan_error, errorCodeTranslation[0].nan_error, NAN_ERROR_STR_LEN);
                    rsp_data.response_type = NAN_RESPONSE_ENABLED;
                    break;
                }
                case NAN_MSG_ID_DISABLE:
                {
                    ALOGV("handle Event:NAN_MSG_ID_DISABLE");
                    rsp_data.status = NAN_STATUS_SUCCESS;
                    strlcpy(rsp_data.nan_error, errorCodeTranslation[0].nan_error, NAN_ERROR_STR_LEN);
                    rsp_data.response_type = NAN_RESPONSE_DISABLED;
                    break;
                }
                case NAN_MSG_ID_CONFIGURATION:
                {
                    rsp_data.status = NAN_STATUS_SUCCESS;
                    strlcpy(rsp_data.nan_error, errorCodeTranslation[0].nan_error, NAN_ERROR_STR_LEN);
                    rsp_data.response_type = NAN_RESPONSE_CONFIG;
                    break;
                }
                case NAN_MSG_ID_TCA:
                {
                    rsp_data.status = NAN_STATUS_SUCCESS;
                    strlcpy(rsp_data.nan_error, errorCodeTranslation[0].nan_error, NAN_ERROR_STR_LEN);
                    rsp_data.response_type = NAN_RESPONSE_TCA;
                    break;
                }
                case NAN_MSG_ID_PUBLISH_SERVICE:
                {
                    rsp_data.status = NAN_STATUS_SUCCESS;
                    strlcpy(rsp_data.nan_error, errorCodeTranslation[0].nan_error, NAN_ERROR_STR_LEN);
                    rsp_data.response_type = NAN_RESPONSE_PUBLISH;
                    rsp_data.body.publish_response.publish_id= respHeader.header.handle;
                    break;
                }
                case NAN_MSG_ID_PUBLISH_SERVICE_CANCEL:
                {
                    rsp_data.status = NAN_STATUS_SUCCESS;
                    strlcpy(rsp_data.nan_error, errorCodeTranslation[0].nan_error, NAN_ERROR_STR_LEN);
                    rsp_data.response_type = NAN_RESPONSE_PUBLISH_CANCEL;
                    rsp_data.body.publish_response.publish_id= respHeader.header.handle;
                    break;
                }
                case NAN_MSG_ID_SUBSCRIBE_SERVICE:
                {
                    rsp_data.status = NAN_STATUS_SUCCESS;
                    strlcpy(rsp_data.nan_error, errorCodeTranslation[0].nan_error, NAN_ERROR_STR_LEN);
                    rsp_data.response_type = NAN_RESPONSE_SUBSCRIBE;
                    rsp_data.body.subscribe_response.subscribe_id= respHeader.header.handle;
                    break;
                }
                case NAN_MSG_ID_SUBSCRIBE_SERVICE_CANCEL:
                {
                    rsp_data.status = NAN_STATUS_SUCCESS;
                    strlcpy(rsp_data.nan_error, errorCodeTranslation[0].nan_error, NAN_ERROR_STR_LEN);
                    rsp_data.response_type = NAN_RESPONSE_SUBSCRIBE_CANCEL;
                    rsp_data.body.subscribe_response.subscribe_id= respHeader.header.handle;
                    break;
                }
                case NAN_MSG_ID_TRANSMIT_FOLLOWUP:
                {
                    rsp_data.status = NAN_STATUS_SUCCESS;
                    strlcpy(rsp_data.nan_error, errorCodeTranslation[0].nan_error, NAN_ERROR_STR_LEN);
                    rsp_data.response_type = NAN_RESPONSE_TRANSMIT_FOLLOWUP;
                    break;
                }
                case NAN_MSG_ID_STATS:
                {
                    rsp_data.status = NAN_STATUS_SUCCESS;
                    strlcpy(rsp_data.nan_error, errorCodeTranslation[0].nan_error, NAN_ERROR_STR_LEN);
                    rsp_data.response_type = NAN_RESPONSE_STATS;
                    rsp_data.body.stats_response.stats_type = NAN_STATS_ID_DE_SUBSCRIBE;
                    break;
                }
                case NAN_MSG_ID_BEACON_SDF:
                {
                    rsp_data.status = NAN_STATUS_SUCCESS;
                    strlcpy(rsp_data.nan_error, errorCodeTranslation[0].nan_error, NAN_ERROR_STR_LEN);
                    rsp_data.response_type = NAN_RESPONSE_BEACON_SDF_PAYLOAD;
                    break;
                }
                case NAN_MSG_ID_CAPABILITIES:
                {
                    rsp_data.status = NAN_STATUS_SUCCESS;
                    strlcpy(rsp_data.nan_error, errorCodeTranslation[0].nan_error, NAN_ERROR_STR_LEN);
                    rsp_data.response_type = NAN_GET_CAPABILITIES;
//                    rsp_data.body.nan_capabilities.max_concurrent_nan_clusters = \
//                                pFwRsp->max_concurrent_nan_clusters;
//                    rsp_data.body.nan_capabilities.max_publishes = \
//                                pFwRsp->max_publishes;
//                    rsp_data.body.nan_capabilities.max_subscribes = \
//                                pFwRsp->max_subscribes;
                    rsp_data.body.nan_capabilities.max_service_name_len = 255;
//                                pFwRsp->max_service_name_len;
                    rsp_data.body.nan_capabilities.max_match_filter_len = 255;
//                                pFwRsp->max_match_filter_len;
                    rsp_data.body.nan_capabilities.max_total_match_filter_len = 255; 
//                                pFwRsp->max_total_match_filter_len;
                    rsp_data.body.nan_capabilities.max_service_specific_info_len = 255;
//                                pFwRsp->max_service_specific_info_len;
//                    rsp_data.body.nan_capabilities.max_vsa_data_len = \
//                                pFwRsp->max_vsa_data_len;
//                    rsp_data.body.nan_capabilities.max_mesh_data_len = \
//                                pFwRsp->max_mesh_data_len;
//                    rsp_data.body.nan_capabilities.max_ndi_interfaces = \
//                               pFwRsp->max_ndi_interfaces;
//                    rsp_data.body.nan_capabilities.max_ndp_sessions = \
//                               pFwRsp->max_ndp_sessions;
//                    rsp_data.body.nan_capabilities.max_app_info_len = \
//                               pFwRsp->max_app_info_len;
//                    rsp_data.body.nan_capabilities.max_queued_transmit_followup_msgs = \
//                               pFwRsp->max_queued_transmit_followup_msgs;
//                    rsp_data.body.nan_capabilities.ndp_supported_bands = \
//                               pFwRsp->ndp_supported_bands;
//                    rsp_data.body.nan_capabilities.cipher_suites_supported = \
//                               pFwRsp->cipher_suites_supported;
//                    rsp_data.body.nan_capabilities.max_scid_len = \
//                               pFwRsp->max_scid_len;
//                    rsp_data.body.nan_capabilities.is_ndp_security_supported = \
//                               pFwRsp->is_ndp_security_supported;
//                    rsp_data.body.nan_capabilities.max_sdea_service_specific_info_len = \
//                               pFwRsp->max_sdea_service_specific_info_len;
//                    rsp_data.body.nan_capabilities.max_subscribe_address = \
//                               pFwRsp->max_subscribe_address;
                    break;
                }
                case NAN_MSG_ID_NDP_IF_CREATE:
                {
                    rsp_data.status = NAN_STATUS_SUCCESS;
                    strlcpy(rsp_data.nan_error, errorCodeTranslation[0].nan_error, NAN_ERROR_STR_LEN);
                    rsp_data.response_type = NAN_DP_INTERFACE_CREATE;
                    break;
                }
                case NAN_MSG_ID_NDP_IF_DELETE:
                {
                    rsp_data.status = NAN_STATUS_SUCCESS;
                    strlcpy(rsp_data.nan_error, errorCodeTranslation[0].nan_error, NAN_ERROR_STR_LEN);
                    rsp_data.response_type = NAN_DP_INTERFACE_DELETE;
                    break;
                }
                case NAN_MSG_ID_NDP_REQ_INITIATOR:
                {
                    rsp_data.status = NAN_STATUS_SUCCESS;
                    strlcpy(rsp_data.nan_error, errorCodeTranslation[0].nan_error, NAN_ERROR_STR_LEN);
                    rsp_data.response_type = NAN_DP_INITIATOR_RESPONSE;
                    rsp_data.body.data_request_response.ndp_instance_id = respHeader.header.handle;
                    break;
                }
                case NAN_MSG_ID_NDP_IND_RESP:
                {
                    rsp_data.status = NAN_STATUS_SUCCESS;
                    strlcpy(rsp_data.nan_error, errorCodeTranslation[0].nan_error, NAN_ERROR_STR_LEN);
                    rsp_data.response_type = NAN_DP_RESPONDER_RESPONSE;
                    break;
                }
                case NAN_MSG_ID_NDP_END:
                {
                    rsp_data.status = NAN_STATUS_SUCCESS;
                    strlcpy(rsp_data.nan_error, errorCodeTranslation[0].nan_error, NAN_ERROR_STR_LEN);
                    rsp_data.response_type = NAN_DP_END;
                    break;
                }
                default:
                {
                    ALOGE("Error. MsgId not found");
                    //return NL_SKIP;
                }
            }
            ALOGV("handle Event: rsp type = %d", rsp_data.response_type);
            ALOGV("handle Event: rsp status = %d", rsp_data.status);
            ALOGV("handle Event: rsp nan error = %s", rsp_data.nan_error);
            ALOGV("handle Event: rsp transtionId = %d", respHeader.header.transactionId);
            /*handle enable and disable request*/
            if ((respHeader.header.MsgId == NAN_MSG_ID_ENABLE || respHeader.header.MsgId == NAN_MSG_ID_DISABLE) && (enable_num == 1)) {
                if(mHandler.NotifyResponse && (respHeader.header.transactionId != 0xffff)){
                    ALOGE("handle enable or disable Event: ATTR_NOTIFY_RESPONSE");
                    mHandler.NotifyResponse(respHeader.header.transactionId, &rsp_data);
                }
            }
            if (enable_num == 3)
                enable_num = 0;

            /*handle enable and disable request*/
            if ((respHeader.header.MsgId == NAN_MSG_ID_PUBLISH_SERVICE || respHeader.header.MsgId == NAN_MSG_ID_PUBLISH_SERVICE_CANCEL)
                    || (respHeader.header.MsgId == NAN_MSG_ID_SUBSCRIBE_SERVICE || respHeader.header.MsgId == NAN_MSG_ID_PUBLISH_SERVICE_CANCEL)
                    || (respHeader.header.MsgId == NAN_MSG_ID_CAPABILITIES)){
                if(mHandler.NotifyResponse && (respHeader.header.transactionId != 0xffff)){
                    ALOGV("handle publish  Event: ATTR_NOTIFY_RESPONSE");
                    mHandler.NotifyResponse(respHeader.header.transactionId, &rsp_data);
                }
            }
        }

        usleep(500000);
        if(tb_vendor[NXP_ATTR_NAN_IND]){
            ALOGV("handle Event:ATTR_NAN_IND");
            if (respHeader.header.MsgId == NAN_MSG_ID_ENABLE) {
              u32 ind_type = 0;
              memset(&mNDEeventInd, 0, sizeof(NanDiscEngEventInd));
              ind_type = nla_get_u32(tb_vendor[NXP_ATTR_NAN_IND]);
              switch(ind_type)
              {
                  case NAN_DE_MAC_ADDR:
                  {
                      ALOGV("handle Event:NAN_DE_MAC_ADDR");
                      mNDEeventInd.event_type = NAN_EVENT_ID_DISC_MAC_ADDR;
                      for(int i=0; i<6; i++) {
                          mNDEeventInd.data.mac_addr.addr[i] = (unsigned char)rand()%125;
                          ALOGE("handle event: mac_addr[i] = 0x%c", mNDEeventInd.data.mac_addr.addr[i]);
                      }
                      break;
                  }
                  case NAN_DE_START_CLUSTER:
                  {
                      ALOGV("handle Event:NAN_DE_START_CLUSTER");
                      mNDEeventInd.event_type = NAN_EVENT_ID_STARTED_CLUSTER;
                      for(int i=0; i<6; i++) {
                          mNDEeventInd.data.mac_addr.addr[i] = (unsigned char)rand()%125;
                      }
                      break;
                  }
                  case NAN_DE_JOIN_CLUSTER:
                  {
                      ALOGV("handle Event:NAN_DE_JOIN_CLUSTER");
                      mNDEeventInd.event_type = NAN_EVENT_ID_JOINED_CLUSTER;
                      for(int i=0; i<6; i++) {
                          mNDEeventInd.data.mac_addr.addr[i] = (unsigned char)rand()%125;
                      }
                      break;
                  }
                  default:
                      ALOGE("Error. nan indication not found");
                      //return NL_SKIP;
              }
              if(mHandler.EventDiscEngEvent){
                  ALOGV("handle Event: EventDiscEngEvent");
                  mHandler.EventDiscEngEvent(&mNDEeventInd);
              }
            }

            if (respHeader.header.MsgId == NAN_MSG_ID_PUBLISH_SERVICE) {
              u32 ind_ttl = 0;
              memset(&mNptInd, 0, sizeof(NanPublishTerminatedInd));
              ind_ttl = nla_get_u32(tb_vendor[NXP_ATTR_NAN_IND]);
              ALOGV("handle Event:publish ttl = %d", ind_ttl);
              if (ind_ttl > 0)
                  usleep(ind_ttl*1000*1000);

              mNptInd.reason = NAN_STATUS_SUCCESS;
              strlcpy(mNptInd.nan_reason, errorCodeTranslation[0].nan_error, NAN_ERROR_STR_LEN);
              mNptInd.publish_id= respHeader.header.handle;

              if (mHandler.EventPublishTerminated) {
                  ALOGV("handle Event: EventPublishTerminated");
                  mHandler.EventPublishTerminated(&mNptInd);
              }
            }

            if (respHeader.header.MsgId == NAN_MSG_ID_SUBSCRIBE_SERVICE) {
              u32 ind_ttl = 0;
              memset(&mNstInd, 0, sizeof(NanSubscribeTerminatedInd));
              ind_ttl = nla_get_u32(tb_vendor[NXP_ATTR_NAN_IND]);
              ALOGV("handle Event:subscribe ttl = %d", ind_ttl);
              if (ind_ttl > 0)
                  usleep(ind_ttl*1000*1000);

              mNstInd.reason = NAN_STATUS_SUCCESS;
              strlcpy(mNstInd.nan_reason, errorCodeTranslation[0].nan_error, NAN_ERROR_STR_LEN);
              mNstInd.subscribe_id = respHeader.header.handle;
               
              if (mHandler.EventSubscribeTerminated) {
                  ALOGV("handle Event: mHandler.EventSubscribeTerminated");
                  mHandler.EventSubscribeTerminated(&mNstInd);
              }
            }
        }
/*        if(tb_vendor[ATTR_NAN_TRANSTIION_ID]){
           transaction_Id = nla_get_u32(tb_vendor[ATTR_NAN_TRANSTIION_ID]); 
           ALOGE("handle Event: transactionId = %d", transaction_Id);
        }

        ALOGE("handle Event: start response type");
        if(tb_vendor[ATTR_NAN_RSP_TYPE]){
            mType = (NanNxpRequestType)nla_get_u32(tb_vendor[ATTR_NAN_RSP_TYPE]); 
            ALOGE("handle Event: nan rsp type = 0x%x", mType);
            if (mType == NAN_REQUEST_MAX) {
                ALOGE("RSP type is invalid");
                return NL_SKIP;
            }

            switch(mType){
                case NAN_REQUEST_ENABLED:
                {
                    ALOGE("handle Event: NAN_REQUEST_ENABLED");
                    rsp_data.status = errorCodeTranslation[0].frameworkError;
                    strlcpy(rsp_data.nan_error, errorCodeTranslation[0].nan_error, NAN_ERROR_STR_LEN);
                    rsp_data.response_type = NAN_RESPONSE_ENABLED;
                    break;
                }
                case NAN_REQUEST_DISABLED:
                {
                    ALOGE("handle Event: NAN_REQUEST_DISABLED");
                    rsp_data.status = errorCodeTranslation[0].frameworkError;
                    strlcpy(rsp_data.nan_error, errorCodeTranslation[0].nan_error, NAN_ERROR_STR_LEN);
                    rsp_data.response_type = NAN_RESPONSE_DISABLED;
                    break;
                }
                case NAN_REQUEST_CONFIG:
                {
                    ALOGE("handle Event: NAN_REQUEST_CONFIG");
                    rsp_data.status = errorCodeTranslation[0].frameworkError;
                    strlcpy(rsp_data.nan_error, errorCodeTranslation[0].nan_error, NAN_ERROR_STR_LEN);
                    rsp_data.response_type = NAN_RESPONSE_CONFIG;
                    break;
                }
                default:
                {
                    return NL_SKIP;
                }
            }
            ALOGE("handle Event: rsp type = %d", rsp_data.response_type);
            ALOGE("handle Event: rsp status = %d", rsp_data.status);
            ALOGE("handle Event: rsp nan error = %s", rsp_data.nan_error);
            ALOGE("handle Event: rsp transtionId = %d", transaction_Id);
            if(mHandler.NotifyResponse && (transaction_Id != 0xffff)){
                ALOGE("handle Event: ATTR_NOTIFY_RESPONSE");
                mHandler.NotifyResponse(transaction_Id, &rsp_data);
            }
        }*/
///_________________________________________________________________________________________
/*
        if(tb_vendor[ATTR_PUBLISH_TERMINATED]){
            memset(mNptInd, 0, sizeof(NanPublishTerminatedInd));
            mNptInd = (NanPublishTerminatedInd *)tb_vendor[ATTR_PUBLISH_TERMINATED];
            if(mHandler.EventPublishTerminated){
                mHandler.EventPublishTerminated(mNptInd);
            }
        }
        if(tb_vendor[ATTR_MATCH]){
            memset(mNmInd, 0, sizeof(NanMatchInd));
            mNmInd = (NanMatchInd *)tb_vendor[ATTR_MATCH];
            if(mHandler.EventMatch){
                mHandler.EventMatch(mNmInd);
            }
        }
        if(tb_vendor[ATTR_MATCH_EXPIRED]){
            memset(mNmeInd, 0, sizeof(NanMatchExpiredInd));
            mNmeInd = (NanMatchExpiredInd *)tb_vendor[ATTR_MATCH_EXPIRED];
            if(mHandler.EventMatchExpired){
                mHandler.EventMatchExpired(mNmeInd);
            }
        }
        if(tb_vendor[ATTR_SUBSCRIBE_TERMINATED]){
           memset(mNstInd, 0, sizeof(NanSubscribeTerminatedInd));
           mNstInd = (NanSubscribeTerminatedInd *)tb_vendor[ATTR_SUBSCRIBE_TERMINATED];
           if(mHandler.EventSubscribeTerminated){
                mHandler.EventSubscribeTerminated(mNstInd);
            }
        }
        if(tb_vendor[ATTR_FOLLOWUP]){
            memset(mNfpInd, 0, sizeof(NanFollowupInd));
            mNfpInd = (NanFollowupInd *)tb_vendor[ATTR_FOLLOWUP];
            if(mHandler.EventFollowup){
                mHandler.EventFollowup(mNfpInd);
            }
        }
        if(tb_vendor[ATTR_DISC_ENG_EVENT]){
            memset(mNDEeventInd, 0, sizeof(NanDiscEngEventInd));
            mNDEeventInd = (NanDiscEngEventInd *)tb_vendor[ATTR_DISC_ENG_EVENT];
            if(mHandler.EventDiscEngEvent){
                mHandler.EventDiscEngEvent(mNDEeventInd);
            }
        }
        if(tb_vendor[ATTR_DISABLED]){
            memset(mNdisabledInd, 0, sizeof(NanDisabledInd));
            mNdisabledInd = (NanDisabledInd*)tb_vendor[ATTR_DISABLED];
            if(mHandler.EventDisabled){
                mHandler.EventDisabled(mNdisabledInd);
            }
        }
        if(tb_vendor[ATTR_TCA]){
            memset(mNTCAInd, 0, sizeof(NanTCAInd));
            mNTCAInd = (NanTCAInd *)tb_vendor[ATTR_TCA];
            if(mHandler.EventTca){
                mHandler.EventTca(mNTCAInd);
            }
        }
        if(tb_vendor[ATTR_BEACON_SDF_PAYLOAD]){
            memset(mNbspInd, 0, sizeof(NanBeaconSdfPayloadInd));
            mNbspInd = (NanBeaconSdfPayloadInd *)tb_vendor[ATTR_BEACON_SDF_PAYLOAD];
            if(mHandler.EventBeaconSdfPayload){
                mHandler.EventBeaconSdfPayload(mNbspInd);
            }
        }
        if(tb_vendor[ATTR_DATA_REQUEST]){
            memset(mNDPreqInd, 0, sizeof(NanDataPathRequestInd));
            mNDPreqInd = (NanDataPathRequestInd *)tb_vendor[ATTR_DATA_REQUEST];
            if(mHandler.EventDataRequest){
                mHandler.EventDataRequest(mNDPreqInd);
            }
        }
        if(tb_vendor[ATTR_DATA_CONFIRM]){
            memset(mNDPconfirmInd, 0, sizeof(NanDataPathConfirmInd));
            mNDPconfirmInd = (NanDataPathConfirmInd *)tb_vendor[ATTR_DATA_CONFIRM];
            if(mHandler.EventDataConfirm){
                mHandler.EventDataConfirm(mNDPconfirmInd);
            }
        }
        if(tb_vendor[ATTR_DATA_END]){
            memset(mNDPendInd, 0, sizeof(NanDataPathEndInd));
            mNDPendInd = (NanDataPathEndInd *)tb_vendor[ATTR_DATA_END];
            if(mHandler.EventDataEnd){
                mHandler.EventDataEnd(mNDPendInd);
            }
        }
        if(tb_vendor[ATTR_TRANSMIT_FOLLOWUP]){
            memset(mNtransfpInd, 0, sizeof(NanTransmitFollowupInd));
            mNtransfpInd = (NanTransmitFollowupInd *)tb_vendor[ATTR_TRANSMIT_FOLLOWUP];
            if(mHandler.EventTransmitFollowup){
                mHandler.EventTransmitFollowup(mNtransfpInd);
            }
        }
        if(tb_vendor[ATTR_RANGE_REQUEST]){
            memset(mNRangereqInd, 0, sizeof(NanRangeRequestInd));
            mNRangereqInd = (NanRangeRequestInd *)tb_vendor[ATTR_RANGE_REQUEST];
            if(mHandler.EventRangeRequest){
                mHandler.EventRangeRequest(mNRangereqInd);
            }
        }
        if(tb_vendor[ATTR_RANGE_REPORT]){
            memset(mNRangerepInd, 0, sizeof(NanRangeReportInd));
            mNRangerepInd = (NanRangeReportInd *)tb_vendor[ATTR_RANGE_REPORT];
            if(mHandler.EventRangeReport){
                mHandler.EventRangeReport(mNRangerepInd);
            }
        }*/
        else{
            ALOGE("Unknown attribute, ignore\n");
        }
        return NL_OK;
    }
};

class GetNANCapaCommand : public WifiCommand
{
private:
    transaction_id mId;
    NanHeader capaHeader;

public:
    GetNANCapaCommand(transaction_id id, wifi_interface_handle iface)
    : WifiCommand("GetNANCapaCommand", iface, 0)
    {
        mId = id;
        memset(&capaHeader, 0, sizeof(NanHeader));
    }

    virtual int create(){
        int ret = mMsg.create(MARVELL_OUI, NXP_SUBCMD_NAN_GET_CAPA);
        int len = sizeof(NanHeader);
        if (ret < 0) {
            ALOGE("Failed to create message to get NAN capabilities\n");
            return ret;
        }
        capaHeader.MsgId = NAN_MSG_ID_CAPABILITIES;
        capaHeader.transactionId = mId;

        nlattr *data = mMsg.attr_start(NL80211_ATTR_VENDOR_DATA);
        ret = mMsg.put(NXP_ATTR_NAN_FAKE, &capaHeader, len);
        if (ret < 0)
            return ret;
        mMsg.attr_end(data);
        return WIFI_SUCCESS;
    }
};

class DataIfCreateCommand : public WifiCommand
{
private:
    transaction_id mId;
    char *mIface_name;
    NanHeader NDPHeader;

public:
    DataIfCreateCommand(transaction_id id, wifi_interface_handle iface,
                                char *iface_name)
    : WifiCommand("DataIfCreateCommand", iface, 0)
    {
        mId = id;
        mIface_name = iface_name;
        memset(&NDPHeader, 0, sizeof(NanHeader));
    }

    virtual int create(){
        int ret = mMsg.create(MARVELL_OUI, NXP_SUBCMD_NAN_DATA_IF_CREATE);
        int len = sizeof(NanHeader);
        if (ret < 0) {
            ALOGE("Failed to create message to create data interface\n");
            return ret;
        }
        NDPHeader.MsgId = NAN_MSG_ID_NDP_IF_CREATE;
        NDPHeader.transactionId = mId;

        nlattr *data = mMsg.attr_start(NL80211_ATTR_VENDOR_DATA);
        ret = mMsg.put(NXP_ATTR_NAN_FAKE, &NDPHeader, len);
        if (ret < 0)
            return ret;
        mMsg.attr_end(data);
        return WIFI_SUCCESS;
    }
};

class DataIfDeleteCommand : public WifiCommand
{
private:
    transaction_id mId;
    char *mIface_name;
    NanHeader NDPHeader;

public:
    DataIfDeleteCommand(transaction_id id, wifi_interface_handle iface, char *iface_name)
    : WifiCommand("DataIfDeleteCommand", iface, 0)
    {
        mId = id;
        mIface_name = iface_name;
        memset(&NDPHeader, 0, sizeof(NanHeader));
    }

    virtual int create(){
        int ret = mMsg.create(MARVELL_OUI, NXP_SUBCMD_NAN_DATA_IF_DELETE);
        int len = sizeof(NanHeader);
        if (ret < 0) {
            ALOGE("Failed to create message to delete data interface\n");
            return ret;
        }
        NDPHeader.MsgId = NAN_MSG_ID_NDP_IF_DELETE;
        NDPHeader.transactionId = mId;

        nlattr *data = mMsg.attr_start(NL80211_ATTR_VENDOR_DATA);
        ret = mMsg.put(NXP_ATTR_NAN_FAKE, &NDPHeader, len);
        if (ret < 0)
            return ret;
        mMsg.attr_end(data);
        return WIFI_SUCCESS;
    }
};

class DataReqInitiatorCommand : public WifiCommand
{
private:
    transaction_id mId;
    NanDataPathInitiatorRequest* Msg;
    NanHeader NDPHeader;

public:
    DataReqInitiatorCommand(transaction_id id, wifi_interface_handle iface,
                                    NanDataPathInitiatorRequest *msg)
    : WifiCommand("DataReqInitiatorCommand", iface, 0)
    {
        mId = id;
        Msg = msg;
        memset(&NDPHeader, 0, sizeof(NanHeader));
    }

    virtual int create(){
        int ret = mMsg.create(MARVELL_OUI, NXP_SUBCMD_NAN_DATA_REQ_INITOR);
        int len = sizeof(NanHeader);
        if (ret < 0) {
            ALOGE("Failed to create message to request initiator\n");
            return ret;
        }
        NDPHeader.MsgId = NAN_MSG_ID_NDP_REQ_INITIATOR;
        NDPHeader.transactionId = mId;
        NDPHeader.handle = Msg->requestor_instance_id;

        nlattr *data = mMsg.attr_start(NL80211_ATTR_VENDOR_DATA);
        ret = mMsg.put(NXP_ATTR_NAN_FAKE, &NDPHeader, len);
        if (ret < 0)
            return ret;
        mMsg.attr_end(data);
        return WIFI_SUCCESS;
    }
};

class DataIndicationRspCommand : public WifiCommand
{
private:
    transaction_id mId;
    NanDataPathIndicationResponse* Msg;
    NanHeader NDPHeader;

public:
    DataIndicationRspCommand(transaction_id id, wifi_interface_handle iface,
                                       NanDataPathIndicationResponse *msg)
    : WifiCommand("DataIndicationRspCommand", iface, 0)
    {
        mId = id;
        Msg = msg;
        memset(&NDPHeader, 0, sizeof(NanHeader));
    }
    virtual int create(){
        int ret = mMsg.create(MARVELL_OUI, NXP_SUBCMD_NAN_DATA_INDI_RESP);
        int len = sizeof(NanHeader);
        if (ret < 0) {
            ALOGE("Failed to create message to config data indication response\n");
            return ret;
        }
        NDPHeader.MsgId = NAN_MSG_ID_NDP_IND_RESP;
        NDPHeader.transactionId = mId;

        nlattr *data = mMsg.attr_start(NL80211_ATTR_VENDOR_DATA);
        ret = mMsg.put(NXP_ATTR_NAN_FAKE, &NDPHeader, len);
        if (ret < 0)
            return ret;
        mMsg.attr_end(data);
        return WIFI_SUCCESS;
    }
};

class DataEndCommand : public WifiCommand
{
private:
    transaction_id mId;
    NanDataPathEndRequest* Msg;
    NanHeader NDPHeader;

public:
    DataEndCommand(transaction_id id, wifi_interface_handle iface, 
                           NanDataPathEndRequest *msg)
    : WifiCommand("DataEndCommand", iface, 0)
    {
        mId = id;
        Msg = msg;
        memset(&NDPHeader, 0, sizeof(NanHeader));
    }

    virtual int create(){
        int ret = mMsg.create(MARVELL_OUI, NXP_SUBCMD_NAN_DATA_END);
        int len = sizeof(NanHeader);
        if (ret < 0) {
            ALOGE("Failed to create message to end data path\n");
            return ret;
        }
        NDPHeader.MsgId = NAN_MSG_ID_NDP_END;
        NDPHeader.transactionId = mId;

        nlattr *data = mMsg.attr_start(NL80211_ATTR_VENDOR_DATA);
        ret = mMsg.put(NXP_ATTR_NAN_FAKE, &NDPHeader, len);
        if (ret < 0)
            return ret;
        mMsg.attr_end(data);
        return WIFI_SUCCESS;
    }
};

///////////////////////////////////////////////////////////////////////////////////////
wifi_error nan_enable_request(transaction_id id, wifi_interface_handle iface, 
                                   NanEnableRequest *msg)
{
    ALOGV("***nan_enable_request***");
    wifi_error ret;
    wifi_handle handle = getWifiHandle(iface);
    NanControlCommand *cmd = new NanControlCommand(id, iface, NAN_REQUEST_ENABLED, msg);
    wifi_register_cmd(handle, id, cmd);
    ret = (wifi_error)cmd->start();
    ALOGV("nan_enable_request, ret = %d", ret);
    ALOGV("nan_enable_request, transaction id = %d", id);
    return WIFI_SUCCESS;
}

wifi_error nan_disable_request(transaction_id id, wifi_interface_handle iface)
{
    ALOGV("***nan_disable_request***");
    wifi_error ret;
    wifi_handle handle = getWifiHandle(iface);
    NanControlCommand *cmd = new NanControlCommand(id, iface, NAN_REQUEST_DISABLED);
    wifi_register_cmd(handle, id, cmd);
    ret = (wifi_error)cmd->cancel_specific();
    ALOGV("nan_disable_request, ret = %d", ret);
    ALOGV("nan_disable_request, transaction id = %d", id);
    return WIFI_SUCCESS;
}

wifi_error nan_publish_request(transaction_id id, wifi_interface_handle iface, 
                                    NanPublishRequest *msg)
{
    ALOGV("***nan_publish_request***");
    wifi_error ret;
    wifi_handle handle = getWifiHandle(iface);
    NanPublishReqCommand *cmd = new NanPublishReqCommand(id, iface, msg);
    wifi_register_cmd(handle, id, cmd);
    ret = (wifi_error)cmd->start();
    ALOGV("nan_publish_request, ret = %d", ret);
    ALOGV("nan_publish_request, transaction id = %d", id);
    return WIFI_SUCCESS;
}

wifi_error nan_publish_cancel_request(transaction_id id, wifi_interface_handle iface, 
                                             NanPublishCancelRequest *msg)
{
    ALOGV("***nan_publish_cancel_request***");
    wifi_error ret;
    wifi_handle handle = getWifiHandle(iface);
    NanPublishReqCommand *cmd = new NanPublishReqCommand(id, iface, msg);
    wifi_register_cmd(handle, id, cmd);
    ret = (wifi_error)cmd->cancel_specific();
    ALOGV("nan_publish_cancel_request, ret = %d", ret);
    ALOGV("nan_publish_cancel_request, transaction id = %d", id);
    return WIFI_SUCCESS;
}

wifi_error nan_subscribe_request(transaction_id id, wifi_interface_handle iface, 
                                       NanSubscribeRequest *msg)
{
    ALOGV("***nan_subscribe_request***");
    wifi_error ret;
    wifi_handle handle = getWifiHandle(iface);
    SubscribeReqCommand *cmd = new SubscribeReqCommand(id, iface, msg);
    wifi_register_cmd(handle, id, cmd);
    ret = (wifi_error)cmd->start();
    ALOGV("nan_subscribe_request, ret = %d", ret);
    ALOGV("nan_subscribe_request, transaction id = %d", id);
    return WIFI_SUCCESS;
}

wifi_error nan_subscribe_cancel_request(transaction_id id, wifi_interface_handle iface, 
                                                NanSubscribeCancelRequest *msg)
{
    ALOGV("***nan_subscribe_cancel_request***");
    wifi_error ret;
    wifi_handle handle = getWifiHandle(iface);
    SubscribeReqCommand *cmd = new SubscribeReqCommand(id, iface, msg);
    wifi_register_cmd(handle, id, cmd);
    ret = (wifi_error)cmd->cancel_specific();
    ALOGV("nan_subscribe_cancel_request, ret = %d", ret);
    ALOGV("nan_subscribe_cancel_request, transaction id = %d", id);
    return WIFI_SUCCESS;
}

wifi_error nan_transmit_followup_request(transaction_id id, wifi_interface_handle iface, 
                                                NanTransmitFollowupRequest * msg)
{
   // TransmitFollowupCommand command(id, iface, msg);
   // return (wifi_error) command.requestResponse();
    return WIFI_SUCCESS;
}

wifi_error nan_stats_request(transaction_id id, wifi_interface_handle iface, 
                                 NanStatsRequest *msg)
{
   // StatsRequestCommand command(id, iface, msg);
   // return (wifi_error) command.requestResponse();
    return WIFI_SUCCESS;
}

wifi_error nan_config_request(transaction_id id, wifi_interface_handle iface,
                                   NanConfigRequest *msg)
{
    ALOGV("***nan_config_request*****");
    wifi_error ret;
    ConfigRequestCommand command(id, iface, NAN_REQUEST_CONFIG, msg);
    ret = (wifi_error) command.requestResponse();
    ALOGV("nan_config_request, ret = %d", ret);
    return WIFI_SUCCESS;
}

wifi_error nan_tca_request(transaction_id id, wifi_interface_handle iface,
                               NanTCARequest *msg)
{
   // TcaRequestCommand command(id, iface, msg);
   // return (wifi_error) command.requestResponse();
    return WIFI_SUCCESS;
}

wifi_error nan_beacon_sdf_payload_request(transaction_id id,wifi_interface_handle iface,
                                                   NanBeaconSdfPayloadRequest * msg)
{
   // BeaconSDFPayload command(id, iface, msg);
   // return (wifi_error) command.requestResponse();
    return WIFI_SUCCESS;
}

wifi_error nan_get_version(wifi_handle handle, NanVersion *version)
{
    *version = (NAN_MAJOR_VERSION <<16 | NAN_MINOR_VERSION << 8 | NAN_MICRO_VERSION);
    return WIFI_SUCCESS;
}

wifi_error nan_register_handler(wifi_interface_handle iface, NanCallbackHandler handlers)
{
    ALOGV("***wifi_nan_register_handler***");

    wifi_handle handle = getWifiHandle(iface);
    ALOGV("nanhandler start, handle = %p", handle);

    SetRegisterHandlerCommand *cmd = new SetRegisterHandlerCommand(iface, handlers);
    if (cmd) {
        wifi_register_cmd(handle, 0, cmd);
        wifi_error result = (wifi_error)cmd->start();
        if (result != WIFI_SUCCESS)
            wifi_unregister_cmd(handle, 0);
        return result;
    } else {
        ALOGE("Out of memory");
        return WIFI_ERROR_OUT_OF_MEMORY;
    }

    return WIFI_SUCCESS;
}

wifi_error nan_get_capabilities(transaction_id id, wifi_interface_handle iface)
{
    ALOGV("***nan_get_capabilities*****");
    wifi_error ret;
    GetNANCapaCommand command(id, iface);
    ret = (wifi_error) command.requestResponse();
    ALOGV("nan_get_capabilities, ret = %d", ret);
    ALOGV("nan_get_capabilities, transaction id = %d", id);
    return WIFI_SUCCESS;
}

wifi_error nan_data_interface_create(transaction_id id,wifi_interface_handle iface,char * iface_name)
{
   // DataIfCreateCommand command(id, iface, iface_name);
   // return (wifi_error) command.requestResponse();
    return WIFI_SUCCESS;
}

wifi_error nan_data_interface_delete(transaction_id id, wifi_interface_handle iface, char * iface_name)
{
   // DataIfDeleteCommand command(id, iface, iface_name);
   // return (wifi_error) command.requestResponse();
    return WIFI_SUCCESS;
}

wifi_error nan_data_request_initiator(transaction_id id, wifi_interface_handle iface,
                                           NanDataPathInitiatorRequest * msg)
{
   // DataReqInitiatorCommand command(id, iface, msg);
   // return (wifi_error) command.requestResponse();
    return WIFI_SUCCESS;
}

wifi_error nan_data_indication_response(transaction_id id, wifi_interface_handle iface,
                                        NanDataPathIndicationResponse * msg)
{
   // DataIndicationRspCommand command(id, iface, msg);
   // return (wifi_error) command.requestResponse();
    return WIFI_SUCCESS;
}

wifi_error nan_data_end(transaction_id id, wifi_interface_handle iface, NanDataPathEndRequest * msg)
{
   // DataEndCommand command(id, iface, msg);
   // return (wifi_error) command.requestResponse();
    return WIFI_SUCCESS;
}
