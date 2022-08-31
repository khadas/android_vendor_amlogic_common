/*
 * Copyright (C) 2016 The Android Open Source Project
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
 *  @author   Tellen Yu
 *  @version  1.0
 *  @date     2017/9/25
 *  @par function description:
 *  - 1 droidlogic hdmi cec hwbinder service
 */

#define LOG_TAG "hdmicecd"

#include <inttypes.h>
#include <string>
#include <cutils/properties.h>
#include "HdmiCecBase.h"
#include "DroidHdmiCec.h"

namespace vendor {
namespace amlogic {
namespace hardware {
namespace hdmicec {
namespace V1_0 {
namespace implementation {

DroidHdmiCec::DroidHdmiCec() : mDeathRecipient(new DeathRecipient(this))
{
    ALOGI("DroidHdmiCec init");
    mHdmiCecControl = new HdmiCecControl();
    mHdmiCecControl->setEventObserver(this);
    mHdmiCecControl->setHdmiCecActionCallback(this);
}

DroidHdmiCec::~DroidHdmiCec()
{
    delete mHdmiCecControl;
}

Return<void> DroidHdmiCec::openCecDevice(openCecDevice_cb _hidl_cb)
{
    int32_t fd = -1;
    if (NULL != mHdmiCecControl) {
        fd = mHdmiCecControl->openCecDevice();
    }

    _hidl_cb(Result::SUCCESS, fd);

    return Void();
}

Return<void> DroidHdmiCec::closeCecDevice()
{
    if (NULL != mHdmiCecControl) {
        mHdmiCecControl->closeCecDevice();
    }

    return Void();
}

Return<int32_t> DroidHdmiCec::getCecVersion()
{
    int version = 0;
    if (NULL != mHdmiCecControl) {
        mHdmiCecControl->getVersion(&version);
    }

    return version;
}

Return<uint32_t> DroidHdmiCec::getVendorId()
{
    uint32_t vendorId = 0;
    if (NULL != mHdmiCecControl) {
        mHdmiCecControl->getVendorId(&vendorId);
    }

    return vendorId;
}

Return<void> DroidHdmiCec::getPhysicalAddress(getPhysicalAddress_cb _hidl_cb)
{
    uint16_t addr = 0;
    if (NULL != mHdmiCecControl) {
        mHdmiCecControl->getPhysicalAddress(&addr);
    }

    _hidl_cb(Result::SUCCESS, addr);
    return Void();
}

Return<SendMessageResult> DroidHdmiCec::sendMessage(const CecMessage& message, bool isExtend)
{
    if (message.body.size() > CEC_MESSAGE_BODY_MAX_LENGTH) {
        ALOGE("sendMessage body size > %d", CEC_MESSAGE_BODY_MAX_LENGTH);
        return static_cast<SendMessageResult>(HDMI_RESULT_FAIL);
    }
    if (NULL != mHdmiCecControl) {
        //change message from hwbinder data structure to needed data structure
        cec_message_t msg;
        msg.initiator = static_cast<cec_logical_address_t>(message.initiator & 0xf);
        msg.destination = static_cast<cec_logical_address_t>(message.destination & 0xf);

        for (size_t i = 0; i < message.body.size(); ++i) {
            msg.body[i] = message.body[i];
        }
        msg.length = message.body.size();

        return static_cast<SendMessageResult>(mHdmiCecControl->sendMessage(&msg, isExtend));
    }
    return static_cast<SendMessageResult>(HDMI_RESULT_FAIL);
}

Return<void> DroidHdmiCec::getPortInfo(getPortInfo_cb _hidl_cb)
{
    hdmi_port_info_t *legacyPorts;
    int numPorts;
    hidl_vec<HdmiPortInfo> portInfos;
    if (NULL != mHdmiCecControl) {

        mHdmiCecControl->getPortInfos(&legacyPorts, &numPorts);
        portInfos.resize(numPorts);
        for (int i = 0; i < numPorts; ++i) {
            portInfos[i] = {
                .type = static_cast<HdmiPortType>(legacyPorts[i].type),
                .portId = static_cast<uint32_t>(legacyPorts[i].port_id),
                .cecSupported = legacyPorts[i].cec_supported != 0,
                .arcSupported = legacyPorts[i].arc_supported != 0,
                .physicalAddress = legacyPorts[i].physical_address
            };
        }

        _hidl_cb(portInfos);
    }
    return Void();

}

Return<Result> DroidHdmiCec::addLogicalAddress(CecLogicalAddress addr)
{
    if (NULL != mHdmiCecControl) {
        mHdmiCecControl->addLogicalAddress(static_cast<cec_logical_address_t>(addr));
        return Result::SUCCESS;
    }
    return Result::FAILURE_UNKNOWN;
}

Return<void> DroidHdmiCec::clearLogicalAddress()
{
    if (NULL != mHdmiCecControl) {
        mHdmiCecControl->clearLogicaladdress();
    }
    return Void();
}

Return<void> DroidHdmiCec::setOption(OptionKey key, bool value)
{
    if (NULL != mHdmiCecControl) {
        mHdmiCecControl->setOption((int)key, value);
    }
    return Void();
}

Return<void> DroidHdmiCec::enableAudioReturnChannel(int32_t portId, bool enable)
{
    if (NULL != mHdmiCecControl) {
        mHdmiCecControl->setAudioReturnChannel(portId, enable);
    }
    return Void();
}

Return<bool> DroidHdmiCec::isConnected(int32_t portId)
{
    if (NULL != mHdmiCecControl) {
        return mHdmiCecControl->isConnected(portId);
    }
    return false;
}

Return<void> DroidHdmiCec::setCallback(const sp<IDroidHdmiCecCallback>& callback, ConnectType type)
{
    if ((int)type > (int)ConnectType::TYPE_TOTAL - 1) {
        ALOGE("%s don't support type:%d", __FUNCTION__, (int)type);
        return Void();
    }

    if (callback != nullptr) {
        if (mClients[(int)type] != nullptr) {
            ALOGW("%s this type:%s had a callback, cover it", __FUNCTION__, getConnectTypeStr(type));
            mClients[(int)type]->unlinkToDeath(mDeathRecipient);
        }

        mClients[(int)type] = callback;
        Return<bool> linkResult = callback->linkToDeath(mDeathRecipient, (int)type);
        bool linkSuccess = linkResult.isOk() ? static_cast<bool>(linkResult) : false;
        if (!linkSuccess) {
            ALOGW("Couldn't link death recipient for type: %s", getConnectTypeStr(type));
        }
    }
    return Void();
}

void DroidHdmiCec::onEventUpdate(const hdmi_cec_event_t* event)
{
    //change native data structure to hwbinder data structure
    CecEvent hidlEvent;
    hidlEvent.eventType = event->eventType;
    if (HDMI_EVENT_HOT_PLUG == event->eventType) {
        hidlEvent.hotplug.connected = (0 == event->hotplug.connected)?false:true;
        hidlEvent.hotplug.portId = event->hotplug.port_id;
    }
    //HDMI_EVENT_CEC_MESSAGE
    else if (0 != event->eventType) {
        hidlEvent.cec.initiator = static_cast<CecLogicalAddress>(event->cec.initiator);
        hidlEvent.cec.destination = static_cast<CecLogicalAddress>(event->cec.destination);
        hidlEvent.cec.body.resize(event->cec.length);
        for (size_t i = 0; i < event->cec.length; i++) {
            hidlEvent.cec.body[i] = event->cec.body[i];
        }
    }

    int clientSize = mClients.size();
    for (int i = 0; i < clientSize; i++) {
        if (mClients[i] != nullptr) {
            Return<void> ret = mClients[i]->notifyCallback(hidlEvent);
            if (!ret.isOk()) {
                ALOGE("%s, client index:%d, connect type:%s, event:%s", __FUNCTION__, i,
                    getConnectTypeStr((ConnectType)i), getEventTypeStr(event->eventType));
            }
        }
    }
}

bool DroidHdmiCec::onAction(hdmi_cec_action_type action, const hdmi_cec_event_t* event)
{
    CecEvent hidlEvent;
    if (event != NULL) {
        hidlEvent.logicalAddress = event->logicalAddress;

        if (action == HDMI_ACTION_SET_MENU_LANGUAGE) {
            hidlEvent.cec.initiator = static_cast<CecLogicalAddress>(event->cec.initiator);
            hidlEvent.cec.destination = static_cast<CecLogicalAddress>(event->cec.destination);
            hidlEvent.cec.body.resize(event->cec.length);
            for (size_t i = 0; i < event->cec.length; i++) {
                hidlEvent.cec.body[i] = event->cec.body[i];
            }
        }
    }

    if (mClients[(int)ConnectType::TYPE_EXTEND] != nullptr) {
        Return<bool> ret = mClients[(int)ConnectType::TYPE_EXTEND]->notifyAction(hidlEvent, (int32_t)action);
        if (!ret.isOk()) {
            ALOGE("%s, action%d", __FUNCTION__, (int)action);
            return false;
        }
        return ret;
    }
    return false;
}


const char* DroidHdmiCec::getEventTypeStr(int eventType)
{
    switch (eventType) {
        case HDMI_EVENT_CEC_MESSAGE:
            return "cec message";
        case HDMI_EVENT_HOT_PLUG:
            return "hotplug message";
        default:
            return "unknown message";
    }
}

const char* DroidHdmiCec::getConnectTypeStr(ConnectType type)
{
    switch (type) {
        case ConnectType::TYPE_HAL:
            return "HAL";
        case ConnectType::TYPE_EXTEND:
            return "EXTEND";
        default:
            return "unknown type";
    }
}

void DroidHdmiCec::handleServiceDeath(uint32_t type) {
    ALOGI("hdmicec daemon client:%s died", getConnectTypeStr((ConnectType)type));
    mClients[type].clear();
}

DroidHdmiCec::DeathRecipient::DeathRecipient(sp<DroidHdmiCec> cec)
        : mDroidHdmiCec(cec) {}

void DroidHdmiCec::DeathRecipient::serviceDied(
        uint64_t cookie,
        const wp<::android::hidl::base::V1_0::IBase>& /*who*/) {
    ALOGE("droid hdmi cec daemon a client died cookie:%d", (int)cookie);

    uint32_t type = static_cast<uint32_t>(cookie);
    mDroidHdmiCec->handleServiceDeath(type);
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace hdmicec
}  // namespace hardware
}  // namespace amlogic
}  // namespace vendor
