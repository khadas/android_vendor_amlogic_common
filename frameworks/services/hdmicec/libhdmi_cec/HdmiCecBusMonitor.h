/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef HDMICECBUSMONITOR_H
#define HDMICECBUSMONITOR_H

#include <utils/Thread.h>
#include <utils/StrongPointer.h>
#include <sys/epoll.h>

namespace android {

static const uint32_t EPOLL_ID_CEC = 1;

#define  EPOLL_MAX_EVENTS  16
#define  INPUT_MAX_EVENTS  128
#define  NO_TIMEOUT  -1

#define UEVENT_MSG_LEN 2048

#define UEVENT_TYPE_HOTPLUG "hdmi_conn="
#define UEVENT_TYPE_MESSAGE "cec_rx_msg=1"


static const char* CEC_FILE =  "/dev/cec";

class HdmiCecListener: virtual public RefBase {
public:
    virtual void onCecEvent(int eventType) = 0;
};

class HdmiCecBusMonitor: public Thread {
public:
    HdmiCecBusMonitor(sp<HdmiCecListener> listener);
    virtual ~HdmiCecBusMonitor();

private:
    bool threadLoop();

    void processUevent();

    // The array of pending epoll events and the index of the next event to be handled.
    struct epoll_event mPendingEventItems[EPOLL_MAX_EVENTS];

    sp<HdmiCecListener> mListener;

    int mEpollFd;
    int mUeventFd;;

};
} //namespace android
#endif
