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
 *  - 1 droidlogic hdmi cec real implementation
 */

#define LOG_TAG "hdmicecd"
#define LOG_CEE_TAG "HdmiCecControl"
#define LOG_UNIT_TAG "hdmicecd"

#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <log/log.h>
#include <cutils/properties.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include <HdmiCecControl.h>
#include <HdmiCecCompat.h>
#include <HdmiCecUtils.h>

namespace android {

HdmiCecControl::HdmiCecControl() : mMsgHandler(this)
{
    LOGI("[hcc] HdmiCecControl");
    init();
}

HdmiCecControl::~HdmiCecControl(){}

HdmiCecControl::MsgHandler::MsgHandler(HdmiCecControl *hdmiControl)
{
    mControl = hdmiControl;
}

HdmiCecControl::MsgHandler::~MsgHandler(){}

void HdmiCecControl::MsgHandler::handleMessage (CMessage &msg)
{
    LOGD("msg handler handle message type = %x", msg.mType);
    unsigned char body[CEC_MESSAGE_BODY_MAX_LENGTH] = {0};
    switch (msg.mType) {
        case HdmiCecControl::MsgHandler::MSG_GIVE_PHYSICAL_ADDRESS:
            if (mControl->mCecDevice.is_tv) {
                cec_message_t message;
                message.initiator = CEC_ADDR_TV;
                message.destination = (cec_logical_address_t)(msg.mpPara[0]);
                message.body[0] = CEC_MESSAGE_GIVE_PHYSICAL_ADDRESS;
                message.length = 1;
                mControl->send(&message);
            }
            break;
        case HdmiCecControl::MsgHandler::MSG_USER_CONTROL_PRESSED:
            if (mControl->mCecDevice.is_tv) {
                cec_message_t message;
                message.initiator = (cec_logical_address_t)((msg.mpPara[0]) & 0xff);
                message.destination = (cec_logical_address_t)((msg.mpPara[1]) & 0xff);
                message.body[0] = CEC_MESSAGE_USER_CONTROL_PRESSED;
                message.body[1] = (msg.mpPara[2]) & 0xff;
                message.length = 2;
                mControl->send(&message);
            }
            break;
        case HdmiCecControl::MsgHandler::MSG_ONE_TOUCH_PLAY:
            mControl->sendOneTouchPlay(msg.mpPara[0]);
            break;
        case HdmiCecControl::MsgHandler::MSG_GET_MENU_LANGUAGE:
            body[0] = CEC_MESSAGE_GET_MENU_LANGUAGE;
            mControl->sendMessage(mControl->mCecDevice.playback_logical_addr, CEC_ADDR_TV, 1, body);
            break;
        case HdmiCecControl::MsgHandler::MSG_GIVE_OSD_NAME:
            body[0] = CEC_MESSAGE_GIVE_OSD_NAME;
            mControl->sendMessage(mControl->mCecDevice.playback_logical_addr, CEC_ADDR_TV, 1, body);
            break;
        case HdmiCecControl::MsgHandler::MSG_GIVE_DEVICE_VENDOR_ID:
            body[0] = CEC_MESSAGE_GIVE_DEVICE_VENDOR_ID;
            mControl->sendMessage(mControl->mCecDevice.playback_logical_addr, CEC_ADDR_TV, 1, body);
            break;
        case HdmiCecControl::MsgHandler::MSG_PROCESS_CEC_WAKEUP:
            mControl->processCecWakeup();
        break;
    }
}

/**
 * initialize some cec flags before opening cec deivce.
 */
void HdmiCecControl::init()
{
    mCecDevice.is_tv = false;
    mCecDevice.is_playback = false;
    mCecDevice.device_types = NULL;
    mCecDevice.added_phy_addr = NULL;
    mCecDevice.total_device = 0;
    mCecDevice.phy_addr = INVALID_PHYSICAL_ADDRESS;
    mCecDevice.run = true;
    mCecDevice.exited = false;
    mCecDevice.total_port = 0;
    mCecDevice.cec_connect_status = 0;
    mCecDevice.port_data = NULL;
    mCecDevice.playback_logical_addr = CEC_ADDR_BROADCAST;
    mCecDevice.is_cec_enabled = true;
    mCecDevice.is_cec_controled = false;
    getDeviceTypes();

    int index = 0;
    mCecDevice.added_phy_addr = new int[CEC_ADDR_BROADCAST];
    for (index = 0; index < CEC_ADDR_BROADCAST; index++) {
        mCecDevice.added_phy_addr[index] = 0;
    }

    mCecDevice.vendor_ids = new int[CEC_ADDR_BROADCAST];
    for (index = 0; index < CEC_ADDR_BROADCAST; index++) {
        mCecDevice.vendor_ids[index] = 0;
    }

    mCecDevice.driver_fd = open(CEC_FILE, O_RDWR);
    if (mCecDevice.driver_fd < 0) {
        LOGE("[hcc] can't open device. fd < 0");
        return;
    }
    if (getLogLevel() > LOG_LEVEL_1) {
        ALOGD("openCecDevice debug open!");
        ioctl(mCecDevice.driver_fd, CEC_IOC_SET_DEBUG_EN, 1);
    }

    for (index = 0; index < mCecDevice.total_device; index++) {
        int deviceType =  mCecDevice.device_types[index];
        LOGD("[hcc] set device type index : %d, type: %d", index, deviceType);
        ioctl(mCecDevice.driver_fd, CEC_IOC_SET_DEV_TYPE, deviceType);
    }

    getBootConnectStatus();

    mHdmiCecEventHandler = new HdmiCecEventHandler(this);
    mMonitor = new HdmiCecBusMonitor(mHdmiCecEventHandler);
    mMonitor->run("HdmiCecBusMonitor");

    /*
    pthread_t thead;
    pthread_create(&thead, NULL, __threadLoop, this);
    pthread_setname_np(thead, "hdmi_cec_loop");*/
    mMsgHandler.startMsgQueue();
}

void HdmiCecControl::getDeviceTypes() {
    int index = 0;
    char value[PROPERTY_VALUE_MAX] = {0};
    const char * split = ",";
    char * type;
    mCecDevice.device_types = new int[DEV_TYPE_VIDEO_PROCESSOR];
    getProperty(PROPERTY_DEVICE_TYPE, value, "4");
    type = strtok(value, split);
    mCecDevice.device_types[index] = atoi(type);
    while (type != NULL) {
        type = strtok(NULL,split);
        if (type != NULL)
            mCecDevice.device_types[++index] = atoi(type);
    }
    mCecDevice.total_device = index + 1;
    index = 0;
    for (index = 0; index < mCecDevice.total_device; index++) {
        if (mCecDevice.device_types[index] == DEV_TYPE_TV) {
            mCecDevice.is_tv = true;
        } else if (mCecDevice.device_types[index] == DEV_TYPE_PLAYBACK) {
            mCecDevice.is_playback = true;
        }
        LOGI("[hcc] mCecDevice.device_types[%d]: %d", index, mCecDevice.device_types[index]);
    }
}
/**
 * close cec device, reset some cec flags, and recycle some resources.
 */
int HdmiCecControl::closeCecDevice()
{
    mCecDevice.run = false;
    while (!mCecDevice.exited) {
        usleep(100 * 1000);
    }

    close(mCecDevice.driver_fd);

    delete [] mCecDevice.port_data;
    delete [] mCecDevice.device_types;
    delete [] mCecDevice.added_phy_addr;
    delete [] mCecDevice.vendor_ids;
    LOGI("[hcc] %s, cec has closed.", __FUNCTION__);
    return 0;
}

/**
 * initialize all cec flags when open cec devices, get the {@code fd} of cec devices,
 * and create a thread for cec working.
 * {@Return}  fd of cec device.
 */
int HdmiCecControl::openCecDevice()
{
    return mCecDevice.driver_fd;
}

/**
 * get connect status when boot
 */
void HdmiCecControl::getBootConnectStatus()
{
    unsigned int total, i;
    int port, connect, ret;

    ret = ioctl(mCecDevice.driver_fd, CEC_IOC_GET_PORT_NUM, &total);
    LOGD("[hcc] total port:%d, ret:%d", total, ret);
    if (ret < 0)
        return ;

    if (total > MAX_PORT)
        total = MAX_PORT;
    hdmi_port_info_t  *portData = NULL;
    portData = new hdmi_port_info[total];
    if (!portData) {
        LOGE("[hcc] alloc port_data failed");
        return;
    }
    ioctl(mCecDevice.driver_fd, CEC_IOC_GET_PORT_INFO, portData);
    mCecDevice.cec_connect_status = 0;
    for (i = 0; i < total; i++) {
        port = portData[i].port_id;
        connect = port;
        ret = ioctl(mCecDevice.driver_fd, CEC_IOC_GET_CONNECT_STATUS, &connect);
        if (ret) {
            LOGD("[hcc] get port %d connected status failed, ret:%d", i, ret);
            continue;
        }
        mCecDevice.cec_connect_status |= ((connect ? 1 : 0) << port);
    }
    delete [] portData;
    LOGD("[hcc] cec_connect_status: %d", mCecDevice.cec_connect_status);
}

void* HdmiCecControl::__threadLoop(void *user)
{
    HdmiCecControl *const self = static_cast<HdmiCecControl *>(user);
    self->threadLoop();
    return 0;
}

void HdmiCecControl::threadLoop()
{
    unsigned char msgBuf[CEC_MESSAGE_BODY_MAX_LENGTH];
    hdmi_cec_event_t event;
    int r = -1;

    while (mCecDevice.driver_fd < 0) {
        usleep(1000 * 1000);
        mCecDevice.driver_fd = open(CEC_FILE, O_RDWR);
    }
    LOGI("[hcc] file open ok, fd = %d.", mCecDevice.driver_fd);

    while (mCecDevice.run) {
        if (!mCecDevice.is_cec_enabled) {
            usleep(1000 * 1000);
            continue;
        }
        checkConnectStatus();

        memset(msgBuf, 0, sizeof(msgBuf));
        //try to get a message from dev.
        r = readMessage(msgBuf, CEC_MESSAGE_BODY_MAX_LENGTH);
        if (r <= 1)//ignore received ping messages
            continue;

        printCecMsgBuf((const char*)msgBuf, r);

        memset(event.cec.body, 0, sizeof(event.cec.body));
        memcpy(event.cec.body, msgBuf + 1, r - 1);
        event.eventType = 0;
        event.cec.initiator = cec_logical_address_t((msgBuf[0] >> 4) & 0xf);
        event.cec.destination = cec_logical_address_t((msgBuf[0] >> 0) & 0xf);
        event.cec.length = r - 1;

        if (mCecDevice.is_cec_controled || transferableInSleep((char*)msgBuf)) {
             event.eventType |= HDMI_EVENT_CEC_MESSAGE;
        }

        messageValidateAndHandle(&event);
        handleOTPMsg(&event);
        if (mEventListener != NULL && event.eventType != 0) {
            mEventListener->onEventUpdate(&event);
        }
    }
    //LOGE("thread end.");
    mCecDevice.exited = true;
}

void HdmiCecControl::processCecWakeup()
{
    ALOGD("%s logical addr:%x, physical addr:%x", __FUNCTION__,
            mCecDevice.cec_wake_status.wake_device_logical_addr,
            mCecDevice.cec_wake_status.wake_device_phy_addr);
    if (isSourceDevice(mCecDevice.cec_wake_status.wake_device_logical_addr)) {
        hdmi_cec_event_t event;
        event.eventType = HDMI_EVENT_CEC_MESSAGE;
        event.cec.initiator = (cec_logical_address_t)mCecDevice.cec_wake_status.wake_device_logical_addr;
        event.cec.destination = CEC_ADDR_BROADCAST;
        event.cec.length = 3;
        event.cec.body[0] = CEC_MESSAGE_ACTIVE_SOURCE;
        event.cec.body[1] = (mCecDevice.cec_wake_status.wake_device_phy_addr >> 8) & 0xff;
        event.cec.body[2] = mCecDevice.cec_wake_status.wake_device_phy_addr & 0xff;
        if (mEventListener != NULL) {
            ALOGD("%s receives delayed active source message from uboot.", __FUNCTION__);
            mEventListener->onEventUpdate(&event);
        }
    }
    mCecDevice.cec_wake_status.processed = false;
}

void HdmiCecControl::initCecWakeupInfo()
{
    if (!mCecDevice.is_tv) {
        return;
    }

    int wakeupReason = 0;
    ioctl(mCecDevice.driver_fd, CEC_IOC_GET_BOOT_REASON, &wakeupReason);
    mCecDevice.cec_wake_status.processed = (wakeupReason & 0xff) == CEC_WAKEUP;

    ALOGD("%s wakeup reason:%d, cec wake up:%d", __FUNCTION__, wakeupReason,
                                        mCecDevice.cec_wake_status.processed);

    if (!mCecDevice.cec_wake_status.processed) {
        return;
    }

    int wakeAddr = 0;
    // wakeup address is 0xb1000.
    ioctl(mCecDevice.driver_fd, CEC_IOC_GET_BOOT_ADDR, &wakeAddr);
    mCecDevice.cec_wake_status.wake_device_logical_addr = (wakeAddr >> 16) & 0xf;
    mCecDevice.cec_wake_status.wake_device_phy_addr = (wakeAddr & 0xffff);
    ALOGD("%s wakeup %x logical addr:%x, physical addr:%x", __FUNCTION__, wakeAddr,
            mCecDevice.cec_wake_status.wake_device_logical_addr,
            mCecDevice.cec_wake_status.wake_device_phy_addr);
}

bool HdmiCecControl::isSourceDevice(int logicalAddress)
{
    bool res = false;
    switch (logicalAddress) {
        case CEC_ADDR_RECORDER_1:
            [[fallthrough]];
        case CEC_ADDR_RECORDER_2:
            [[fallthrough]];
        case CEC_ADDR_TUNER_1:
            [[fallthrough]];
        case CEC_ADDR_PLAYBACK_1:
            [[fallthrough]];
        case CEC_ADDR_TUNER_2:
            [[fallthrough]];
        case CEC_ADDR_TUNER_3:
            [[fallthrough]];
        case CEC_ADDR_PLAYBACK_2:
            [[fallthrough]];
        case CEC_ADDR_RECORDER_3:
            [[fallthrough]];
        case CEC_ADDR_TUNER_4:
            [[fallthrough]];
        case CEC_ADDR_PLAYBACK_3:{
            res = true;
            break;

        }
        default:
            break;
    }
    return res;
}

/**
 * Check if still transfer it when mCecDevice.is_cec_controled is false
*/

bool HdmiCecControl::transferableInSleep(char *msgBuf)
{
    switch (msgBuf[1]) {
        case CEC_MESSAGE_GIVE_DEVICE_VENDOR_ID:
            [[fallthrough]];
        case CEC_MESSAGE_GIVE_OSD_NAME:
            [[fallthrough]];
        case CEC_MESSAGE_GIVE_DEVICE_POWER_STATUS:
            [[fallthrough]];
        case CEC_MESSAGE_REPORT_POWER_STATUS:
            [[fallthrough]];
        case CEC_MESSAGE_GIVE_PHYSICAL_ADDRESS:
            [[fallthrough]];
        case CEC_MESSAGE_REPORT_PHYSICAL_ADDRESS:{ 
            return true;
        }
    }
    bool ret = false;
    int index = 0;
    for (index = 0; index < mCecDevice.total_device; index++) {
        if (mCecDevice.device_types[index] == DEV_TYPE_PLAYBACK) {
            switch (msgBuf[1]) {
                case CEC_MESSAGE_DECK_CONTROL:
                    [[fallthrough]];
                case CEC_MESSAGE_PLAY:
                    [[fallthrough]];
                case CEC_MESSAGE_SET_STREAM_PATH:
                    [[fallthrough]];
                case CEC_MESSAGE_ROUTING_CHANGE:
                    ret = true;
                    break;
                case CEC_MESSAGE_USER_CONTROL_PRESSED:
                    if (msgBuf[2] == CEC_KEYCODE_POWER
                    || msgBuf[2] == CEC_KEYCODE_POWER_TOGGLE_FUNCTION
                    || msgBuf[2] == CEC_KEYCODE_POWER_ON_FUNCTION) {
                        ret = true;
                    }
                    break;
                default:
                    break;
            }
        } else if (mCecDevice.device_types[index] == DEV_TYPE_TV) {
            switch (msgBuf[1]) {
                case CEC_MESSAGE_IMAGE_VIEW_ON:
                    [[fallthrough]];
                case CEC_MESSAGE_TEXT_VIEW_ON:
                    [[fallthrough]];
                case CEC_MESSAGE_ACTIVE_SOURCE:
                    ret = true;
                    break;
                default:
                    break;
            }
        } else if (mCecDevice.device_types[index] == DEV_TYPE_AUDIO_SYSTEM) {
            switch (msgBuf[1]) {
                case CEC_MESSAGE_SYSTEM_AUDIO_MODE_REQUEST:
                    [[fallthrough]];
                case CEC_MESSAGE_USER_CONTROL_PRESSED:
                    ret = true;
                    break;
                default:
                    break;
            }
        }
    }
    return ret;
}

/**
 * Check if received a valid message.
* @param msgBuf is a message Buf
*   msgBuf[1]: message type
*   msgBuf[2]-msgBuf[n]: message para
* @param len is message lenth
* @param deviceType is type of device
*/

void HdmiCecControl::messageValidateAndHandle(hdmi_cec_event_t* event)
{
    CMessage msg;
    int initiator = (int)(event->cec.initiator);
    int destination = (int)(event->cec.destination);
    int devPhyAddr = 0;
    int opcode = event->cec.body[0];

    switch (opcode) {
        case CEC_MESSAGE_DEVICE_VENDOR_ID:
            mCecDevice.vendor_ids[initiator] = ((event->cec.body[1] & 0xff) << 16)
                + ((event->cec.body[2] & 0xff) << 8) +  (event->cec.body[3] & 0xff);
            break;
        default:
            break;
    }
    if (mCecDevice.is_tv) {
        switch (opcode) {
            case CEC_MESSAGE_REPORT_PHYSICAL_ADDRESS:
                devPhyAddr = ((event->cec.body[1] & 0xff) << 8) +  (event->cec.body[2] & 0xff);
                // Compat code: not accpet a device other than tv takes physical address 0.
                if (event->cec.body[1] == 0) {
                    msg.mType = HdmiCecControl::MsgHandler::MSG_GIVE_PHYSICAL_ADDRESS;
                    msg.mDelayMs = DELAY_TIMEOUT_MS/5;
                    msg.mpPara[0] = initiator;
                    mMsgHandler.removeMsg(msg);
                    mMsgHandler.sendMsg(msg);
                    event->eventType = 0;
                    LOGE("[hcc] receviced message: %02x validate fail and drop", event->cec.body[0]);
                } else {
                    // Compat code: no tranfer the same <Report Physical Address> if it has been done.
                    // Preserve this code for projects like amazon fireos.
                    // Check if we have added the specific address, we should not allow the same
                    // devices reports too many messages so that tv might trigger so many NewDeviceActions.
                    // This works mostly in senarios where the connected box or audio system might trigger
                    // a hotplug out and in event when it goes to sleep.
                    AutoMutex _l(mLock);
                    if (mCecDevice.added_phy_addr[initiator] == devPhyAddr) {
                        LOGE("receive report physical address message but we have already added it.");
                        event->eventType = 0;
                        return;
                    }

                    mCecDevice.added_phy_addr[initiator] = devPhyAddr;
                    /*
                    // Compat code: Process uboot cec wake up and forge missed otp messages.
                    // aml cec driver forging messages may have hight risk.
                    if (mCecDevice.cec_wake_status.processed
                        && mCecDevice.cec_wake_status.wake_device_logical_addr == initiator
                        && mCecDevice.cec_wake_status.wake_device_phy_addr == devPhyAddr) {
                        LOGI("cec wake up device!");
                        msg.mType = HdmiCecControl::MsgHandler::MSG_PROCESS_CEC_WAKEUP;
                        mMsgHandler.sendMsg(msg);
                    }
                    */
                }
                break;
            case CEC_MESSAGE_ROUTING_CHANGE:
                [[fallthrough]];
            case CEC_MESSAGE_ROUTING_INFORMATION:
                if (destination != CEC_ADDR_BROADCAST) {
                    LOGD("[hcc] receviced message: %02x validate fail and drop", opcode);
                    event->eventType = 0;
                }
                break;
            default:
                break;
        }
    } else if (mCecDevice.is_playback) {
        switch (opcode) {
            #ifdef NO_USE_DROID_PATCH
            case CEC_MESSAGE_ROUTING_CHANGE:
                [[fallthrough]];
            case CEC_MESSAGE_ROUTING_INFORMATION:
                // build <set stream path> message for routing change message
                // in case java framwork dos not process this message
                event->cec.body[0] = CEC_MESSAGE_SET_STREAM_PATH;
                event->cec.body[1] = event->cec.body[event->cec.length - 2];
                event->cec.body[2] = event->cec.body[event->cec.length - 1];
                event->cec.length = event->cec.length - 2;
                printCecEvent(event);
                ALOGD("replace <Routing Change> with <Set Stream Path>");
                break;
            #endif
            case CEC_MESSAGE_SET_MENU_LANGUAGE:
                handleSetMenuLanguage(event);
                break;
        }
    }
}

void HdmiCecControl::handleOTPMsg(hdmi_cec_event_t* event)
{
    if (event->cec.body[0] == CEC_MESSAGE_ACTIVE_SOURCE) {
        AutoMutex _l(mLock);
        mCecDevice.active_logical_addr = (int)(event->cec.initiator);
        mCecDevice.active_routing_path = ((event->cec.body[1] & 0xff) << 8) + (event->cec.body[2] & 0xff);
    }
}

void HdmiCecControl::handleSetMenuLanguage(hdmi_cec_event_t* event)
{
    if (event->cec.initiator != CEC_ADDR_TV) {
        event->eventType = 0;
        LOGE("handleSetMenuLanguage message from no tv is not accpeted");
        return;
    }

    int language = ((event->cec.body[1] & 0xff) << 16) + ((event->cec.body[2] & 0xff) << 8)
                    + (event->cec.body[3] & 0xff);
    LOGD("handleSetMenuLanguage tv vendorId %x lang %x", mCecDevice.vendor_ids[CEC_ADDR_TV], language);
    // Compat for the vendors which use different code for "zho"
    compatLangIfneeded(mCecDevice.vendor_ids[CEC_ADDR_TV], language, event);
}

void HdmiCecControl::getDeviceExtraInfo(int flag)
{
    LOGD("get info from tv");
    CMessage msg;
    // Get tv osd name
    msg.mType = HdmiCecControl::MsgHandler::MSG_GIVE_OSD_NAME;
    msg.mDelayMs = DELAY_TIMEOUT_MS * flag;
    mMsgHandler.removeMsg(msg);
    mMsgHandler.sendMsg(msg);
    // Get tv vendor id
    msg.mType = HdmiCecControl::MsgHandler::MSG_GIVE_DEVICE_VENDOR_ID;
    msg.mDelayMs = DELAY_TIMEOUT_MS * flag;
    mMsgHandler.removeMsg(msg);
    mMsgHandler.sendMsg(msg);
    // Get tv menu language
    msg.mType = HdmiCecControl::MsgHandler::MSG_GET_MENU_LANGUAGE;
    msg.mDelayMs = DELAY_TIMEOUT_MS * flag;
    mMsgHandler.removeMsg(msg);
    mMsgHandler.sendMsg (msg);
}

void HdmiCecControl::sendOneTouchPlay(int logicalAddress) {
    LOGD("send one touch play message %x", logicalAddress);

    cec_message_t message;
    message.initiator = (cec_logical_address_t)logicalAddress;
    message.destination = CEC_ADDR_TV;
    message.length = 1;
    message.body[0] = CEC_MESSAGE_TEXT_VIEW_ON;
    send(&message);

    message.destination = CEC_ADDR_BROADCAST;
    message.length = 4;
    message.body[0] = CEC_MESSAGE_ACTIVE_SOURCE;
    message.body[1] = (mCecDevice.phy_addr >> 8) & 0xff;
    message.body[2] = mCecDevice.phy_addr & 0xff;
    message.body[3] = logicalAddress & 0xff;
    send(&message);
}

void HdmiCecControl::checkConnectStatus()
{
    unsigned int prevStatus, bit;
    int i, port, connect, ret;
    hdmi_cec_event_t event;

    prevStatus = mCecDevice.cec_connect_status;
    for (i = 0; i < mCecDevice.total_port && mCecDevice.port_data != NULL; i++) {
        port = mCecDevice.port_data[i].port_id;
        if (mCecDevice.total_port == 1 && mCecDevice.device_types[0] == DEV_TYPE_PLAYBACK) {
            //playback for tx hotplug para is always 0
            port = 0;
        }
        connect = port;
        ret = ioctl(mCecDevice.driver_fd, CEC_IOC_GET_CONNECT_STATUS, &connect);
        if (ret) {
            LOGE("[hcc] get port %d connected status failed, ret:%d\n", mCecDevice.port_data[i].port_id, ret);
            continue;
        }
        bit = prevStatus & (1 << port);
        if (bit ^ ((connect ? 1 : 0) << port)) {//connect status has changed
            LOGI("[hcc] port:%x, connect status changed, now:%x, prevStatus:%x\n",
                    mCecDevice.port_data[i].port_id, connect, prevStatus);
            if (mEventListener != NULL && mCecDevice.is_cec_enabled && mCecDevice.is_cec_controled) {
                event.eventType = HDMI_EVENT_HOT_PLUG;
                event.hotplug.connected = connect;
                event.hotplug.port_id = mCecDevice.port_data[i].port_id;
                mEventListener->onEventUpdate(&event);
            }
            prevStatus &= ~(bit);
            prevStatus |= ((connect ? 1 : 0) << port);
        }
    }
    mCecDevice.cec_connect_status = prevStatus;
}


HdmiCecControl::HdmiCecEventHandler::HdmiCecEventHandler(HdmiCecControl *hdmiControl)
{
    mControl = hdmiControl;
}

HdmiCecControl::HdmiCecEventHandler::~HdmiCecEventHandler()
{
}

void HdmiCecControl::HdmiCecEventHandler::onCecEvent(int type) {
    LOGD("onCecEvent type :%d", type);
    if (HDMI_EVENT_HOT_PLUG == type) {
        mControl->checkConnectStatus();
    } else if (HDMI_EVENT_CEC_MESSAGE == type) {
        mControl->readCecMessage();
    }
}

int HdmiCecControl::readCecMessage()
{
    unsigned char msgBuf[CEC_MESSAGE_BODY_MAX_LENGTH];
    int r = -1;

    memset(msgBuf, 0, sizeof(msgBuf));
    //try to get a message from dev.
    r = readMessage(msgBuf, CEC_MESSAGE_BODY_MAX_LENGTH);
    if (r <= 1) {
        //ignore received ping messages
        return 0;
    }

    printCecMsgBuf((const char*)msgBuf, r);

    if (!mCecDevice.is_cec_enabled) {
        return 0;
    }

    hdmi_cec_event_t event;
    memset(event.cec.body, 0, sizeof(event.cec.body));
    memcpy(event.cec.body, msgBuf + 1, r - 1);
    event.eventType = 0;
    event.cec.initiator = cec_logical_address_t((msgBuf[0] >> 4) & 0xf);
    event.cec.destination = cec_logical_address_t((msgBuf[0] >> 0) & 0xf);
    event.cec.length = r - 1;

    if (mCecDevice.is_cec_controled || transferableInSleep((char*)msgBuf)) {
         event.eventType |= HDMI_EVENT_CEC_MESSAGE;
    }

    messageValidateAndHandle(&event);
    handleOTPMsg(&event);
    if (mEventListener != NULL && event.eventType != 0) {
        mEventListener->onEventUpdate(&event);
    }
    return 0;
}

int HdmiCecControl::readMessage(unsigned char *buf, int msgCount)
{
    if (msgCount <= 0 || !buf) {
        return 0;
    }

    int ret = -1;
    /* maybe blocked at driver */
    ret = read(mCecDevice.driver_fd, buf, msgCount);
    if (ret < 0) {
        LOGE("[hcc] read :%s failed, ret:%d\n", CEC_FILE, ret);
        return -1;
    }

    return ret;
}

void HdmiCecControl::getPortInfos(hdmi_port_info_t* list[], int* total)
{
    if (assertHdmiCecDevice())
        return;

    ioctl(mCecDevice.driver_fd, CEC_IOC_GET_PORT_NUM, total);

    LOGD("[hcc] total port:%d", *total);
    if (*total > MAX_PORT)
        *total = MAX_PORT;

    if (NULL != mCecDevice.port_data)
        delete [] mCecDevice.port_data;
    mCecDevice.port_data = new hdmi_port_info[*total];
    if (!mCecDevice.port_data) {
        LOGE("[hcc] alloc port_data failed");
        *total = 0;
        return;
    }

    ioctl(mCecDevice.driver_fd, CEC_IOC_GET_PORT_INFO, mCecDevice.port_data);

    for (int i = 0; i < *total; i++) {
        LOGI("[hcc] portId: %d, type:%s, cec support:%d, arc support:%d, physical address:%x",
                mCecDevice.port_data[i].port_id,
                mCecDevice.port_data[i].type ? "output" : "input",
                mCecDevice.port_data[i].cec_supported,
                mCecDevice.port_data[i].arc_supported,
                mCecDevice.port_data[i].physical_address);
    }

    *list = mCecDevice.port_data;
    mCecDevice.total_port = *total;
}

int HdmiCecControl::addLogicalAddress(cec_logical_address_t address)
{
    if (assertHdmiCecDevice())
        return -EINVAL;

    if (isSourceDevice(address)) {
        mCecDevice.playback_logical_addr = address;
    }

    int res = ioctl(mCecDevice.driver_fd, CEC_IOC_ADD_LOGICAL_ADDR, address);
    LOGI("[hcc] addr:%x, allocate result:%d\n", mCecDevice.playback_logical_addr, res);

    onAddressAllocated(address);
    return res;
}

void HdmiCecControl::onAddressAllocated(int logicalAddress)
{
    LOGD("[hcc] onAddressAllocated:%d", logicalAddress);
    #ifdef NO_USE_DROID_PATCH
    getDeviceExtraInfo(1);
    #endif

    // Android has implemented the function of ONE TOUCH PLAY with keyevents Power and Home. In the previous version
    // like p we use an easy way which is doing this when logical address is allocated. This will make the playback
    // wake up tv if needed and gain the active source in senarios incluing boot, wake up without power key, and
    // hotplug in. This is not accepted by google and most of our cutomers do not care about it, thus we will disable
    // it by default from q. If the customer wants to do it, please do it in frameworks/base.
    // Remove the code from r. Don't produce any messages outside of android framework. Details is in SWPL-26388.
    /*
    if (mCecDevice.is_playback && isConnected(CEC_ADDR_TV)) {
        if (mCecDevice.mAutoOtp && getPropertyBoolean(PROPERTY_ONE_TOUCH_PLAY, true)) {
            CMessage message;
            message.mType = HdmiCecControl::MsgHandler::MSG_ONE_TOUCH_PLAY;
            message.mDelayMs = DELAY_TIMEOUT_MS/5;
            message.mpPara[0] = logicalAddress;
            mMsgHandler.removeMsg(message);
            mMsgHandler.sendMsg(message);
        }
    }
    */
}

void HdmiCecControl::clearLogicaladdress()
{
    LOGI("clearLogicaladdress");
    if (assertHdmiCecDevice())
        return;

    ioctl(mCecDevice.driver_fd, CEC_IOC_CLR_LOGICAL_ADDR, 0);
}

void HdmiCecControl::setOption(int flag, int value)
{
    if (assertHdmiCecDevice())
        return;

    int ret = -1;
    switch (flag) {
        case HDMI_OPTION_ENABLE_CEC:
            ret = ioctl(mCecDevice.driver_fd, CEC_IOC_SET_OPTION_ENALBE_CEC, value);
            mCecDevice.is_cec_enabled = (value == 1);
            if (mCecDevice.is_cec_enabled) {
                mCecDevice.is_cec_controled = true;
            }
            break;

        case HDMI_OPTION_WAKEUP:
            ret = ioctl(mCecDevice.driver_fd, CEC_IOC_SET_OPTION_WAKEUP, value);
            break;

        case HDMI_OPTION_SYSTEM_CEC_CONTROL:
            ret = ioctl(mCecDevice.driver_fd, CEC_IOC_SET_OPTION_SYS_CTRL, value);
            mCecDevice.is_cec_controled = (value == 1);
            /*
            if (mCecDevice.is_cec_controled) {
                initCecWakeupInfo();
            }
            */
            break;

        case HDMI_OPTION_SET_LANG:
            ret = ioctl(mCecDevice.driver_fd, CEC_IOC_SET_OPTION_SET_LANG, value);
            break;

        default:
            break;
    }
    LOGD("[hcc] %s, flag:0x%x, value:0x%x, ret:%d, is_cec_controled:%x", __FUNCTION__,
                        flag, value, ret, mCecDevice.is_cec_controled);
}

void HdmiCecControl::setAudioReturnChannel(int port, bool flag)
{
    if (assertHdmiCecDevice())
        return;

    int ret = ioctl(mCecDevice.driver_fd, CEC_IOC_SET_ARC_ENABLE, flag);
    LOGD("[hcc] %s, port id:%d, flag:%x, ret:%d\n", __FUNCTION__, port, flag, ret);
}

bool HdmiCecControl::isConnected(int port)
{
    if (assertHdmiCecDevice())
        return false;

    int status = -1, ret;
    /* use status pass port id */
    status = port;
    ret = ioctl(mCecDevice.driver_fd, CEC_IOC_GET_CONNECT_STATUS, &status);
    if (ret)
        return false;
    LOGD("[hcc] %s, port:%d, connected:%s", __FUNCTION__, port, status ? "yes" : "no");
    return status;
}

int HdmiCecControl::getVersion(int* version)
{
    if (assertHdmiCecDevice())
        return -1;

    int ret = ioctl(mCecDevice.driver_fd, CEC_IOC_GET_VERSION, version);
    LOGD("[hcc] %s, version:%x, ret = %d", __FUNCTION__, *version, ret);
    return ret;
}

int HdmiCecControl::getVendorId(uint32_t* vendorId)
{
    if (assertHdmiCecDevice())
        return -1;

    int ret = ioctl(mCecDevice.driver_fd, CEC_IOC_GET_VENDOR_ID, vendorId);
    LOGD("[hcc] %s, vendorId: %x, ret = %d", __FUNCTION__, *vendorId, ret);
    if (*vendorId == 0) {
        LOGD("use none zerio vendor id for cts");
        *vendorId = VENDOR_ID_CTS;
    }
    return ret;
}

int HdmiCecControl::getPhysicalAddress(uint16_t* addr)
{
    if (assertHdmiCecDevice())
        return -EINVAL;

    if (mCecDevice.is_tv) {
        // no need to update tv's address, it's persistent 0.
        *addr = 0;
        return 0;
    }

    int ret = ioctl(mCecDevice.driver_fd, CEC_IOC_GET_PHYSICAL_ADDR, addr);
    LOGD("[hcc] %s, physical addr: %x, last pa: %x, ret = %d", __FUNCTION__,
        *addr, mCecDevice.phy_addr, ret);

    // In cases in which device sleeps the physical address could be 0 for a playback.
    // It's not acceptable. We can use the last valid value in this case.
    if (*addr != 0 && *addr != INVALID_PHYSICAL_ADDRESS) {
        // Save the valid one
        mCecDevice.phy_addr = *addr;
    } else {
        *addr = mCecDevice.phy_addr;
    }
    return ret;
}

int HdmiCecControl::sendMessage(const cec_message_t* message, bool isExtend)
{
    int ret = -1;
    if (assertHdmiCecDevice()) {
        LOGE("sendMessage not valid cec device!");
        return -EINVAL;
    }

    if (!mCecDevice.is_cec_enabled) {
        LOGE("sendMessage cec not enabled!");
        return -EINVAL;
    }
    if (isExtend) {
        return sendExtMessage(message);
    }

    if (preHandleOfSend(message) != 0) {
        return ret;
    }

    ret = send(message);
    postHandleOfSend(message, ret);
    return ret;
}

int HdmiCecControl::sendMessage(int source, int destination, int length, unsigned char body[])
{
    cec_message_t message;
    message.initiator = (cec_logical_address_t)(source & 0xf);
    message.destination = (cec_logical_address_t)(destination & 0xf);
    message.length = length;
    memcpy(message.body, body, length);
    return send(&message);
}

int HdmiCecControl::preHandleOfSend(const cec_message_t* message)
{
    int ret = 0;
    int para = 0;
    int dest = message->destination;
    int opcode = message->body[0] & 0xff;
    switch (opcode) {
        case CEC_MESSAGE_GIVE_DEVICE_VENDOR_ID: {
            mCecDevice.vendor_ids[dest] = 0;
            break;
        }
        case CEC_MESSAGE_ROUTING_CHANGE:
        case CEC_MESSAGE_SET_STREAM_PATH: {
            para = ((message->body[message->length - 2] & 0xff) << 8) + (message->body[message->length - 1] & 0xff);
            /*
            // Filter all routing messages if cec wake up work is not finished.
            if (mCecDevice.cec_wake_status.processed && para != mCecDevice.cec_wake_status.wake_device_phy_addr) {
                ALOGE("%s filter routing message during cec wake up:0x%x para:0x%2x", __FUNCTION__, opcode, para);
                return -1;
            }
            */
            break;
        }
        case CEC_MESSAGE_GIVE_SYSTEM_AUDIO_MODE_STATUS: {
            if (mCecDevice.is_playback) {
                cec_message_t poll;
                poll.initiator = message->initiator;
                poll.destination = message->destination;
                poll.length = 0;
                int pollResult = send(&poll);
                if (pollResult != SUCCESS) {
                    LOGI("no need to query audio mode for no avr exists. ret=%d", pollResult);
                    ret = pollResult;
                }
            }
            break;
        }
        case CEC_MESSAGE_USER_CONTROL_PRESSED:
            para = (message->body[1] & 0xff);
            if (mCecDevice.is_tv
                && (para == CEC_KEYCODE_POWER_ON_FUNCTION)
                && needSendPlayKey(mCecDevice.vendor_ids[dest])) {
                LOGI("send play key for specific device vendor id 0x%2x", mCecDevice.vendor_ids[dest]);
                CMessage msg;
                msg.mType = HdmiCecControl::MsgHandler::MSG_USER_CONTROL_PRESSED;
                msg.mDelayMs = DELAY_TIMEOUT_MS/10;
                msg.mpPara[0] = message->initiator;
                msg.mpPara[1] = dest;
                msg.mpPara[2] = CEC_KEYCODE_PLAY;
                mMsgHandler.sendMsg(msg);
            }
            break;
        case CEC_MESSAGE_GIVE_PHYSICAL_ADDRESS: {
            // TV needs to reset the saved address if we are going to check it again. Or else if the device has
            // reported its address before tv tries to query it in DeviceDiscoveryAction, the filter logic may
            // filter it and cause the DeviceDiscoveryAction fails.
            AutoMutex _l(mLock);
            mCecDevice.added_phy_addr[dest] = 0;
            break;
        }
        case CEC_MESSAGE_TEXT_VIEW_ON:
            [[fallthrough]];
        case CEC_MESSAGE_IMAGE_VIEW_ON:
            [[fallthrough]];
        case CEC_MESSAGE_ACTIVE_SOURCE: {
            // The android framework has not taken this senario into consideration, we have to do the supplement
            // filter work in hal. It works when the playback powers down just after it wakes up.
            if (mCecDevice.is_playback && !mCecDevice.is_cec_controled) {
                ALOGD("filter One Touch Play message when playback goes to sleep.");
                ret = FAIL;
            }
            break;
        }
        default:
            break;
    }
    return ret;
}

int HdmiCecControl::postHandleOfSend(const cec_message_t* message, int result)
{
    int ret = 0;
    int dest, len;
    len = message->length;
    dest = message->destination;
    AutoMutex _l(mLock);
    if (len == 0 && result == NACK && mCecDevice.added_phy_addr[dest] != 0) {
        mCecDevice.added_phy_addr[dest] = 0;
        ALOGE("[hcc] Polling %d fail, reset mCecDevice.added_phy_addr[%d].", dest, dest);
    }
    return ret;
}

void HdmiCecControl::turnOnDevice(int logicalAddress)
{
    cec_message_t message;
    message.initiator = CEC_ADDR_TV;
    message.destination = (cec_logical_address_t)logicalAddress;
    // Power
    message.body[0] = CEC_MESSAGE_USER_CONTROL_PRESSED;
    message.body[1] = (CEC_KEYCODE_POWER_ON_FUNCTION & 0xff);
    message.length = 2;
    send(&message);
    message.body[0] = CEC_MESSAGE_USER_CONTROL_RELEASED;
    message.length = 1;
    send(&message);
    LOGD("[hcc] send wakeUp message.");
}

int HdmiCecControl::sendExtMessage(const cec_message_t* message)
{
    LOGV("sendExtMessage");
    return send(message);
}

int HdmiCecControl::send(const cec_message_t* message)
{
    unsigned char msgBuf[CEC_MESSAGE_BODY_MAX_LENGTH];
    int ret = -1;
    memset(msgBuf, 0, sizeof(msgBuf));
    msgBuf[0] = ((message->initiator & 0xf) << 4) | (message->destination & 0xf);
    memcpy(msgBuf + 1, message->body, message->length);
    ret = write(mCecDevice.driver_fd, msgBuf, message->length + 1);
    printCecMessage(message, ret);
    return ret;
}

bool HdmiCecControl::assertHdmiCecDevice()
{
    return mCecDevice.driver_fd < 0;
}

void HdmiCecControl::setEventObserver(const sp<HdmiCecEventListener> &eventListener)
{
    mEventListener = eventListener;
}

void HdmiCecControl::setHdmiCecActionCallback(const sp<HdmiCecActionCallback> &actionCallback)
{
    mHdmiCecActionCallback = actionCallback;
}

};//namespace android
