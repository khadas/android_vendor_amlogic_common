/* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#define LOG_TAG "hdmicecd"

#include <log/log.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <cutils/uevent.h>

#include <hardware/hdmi_cec.h>
#include <CMsgQueue.h>
#include "HdmiCecBusMonitor.h"

namespace android {

HdmiCecBusMonitor::HdmiCecBusMonitor(sp<HdmiCecListener> listener) {
    mListener = listener;

    //uevent
    mUeventFd = uevent_open_socket(64 * 1024, true);
    if (mUeventFd < 0) {
        ALOGE("uevent_open_socket failed.");
        return;
    }
    fcntl(mUeventFd, F_SETFL, O_NONBLOCK);

    //epoll
    mEpollFd = epoll_create1(EPOLL_CLOEXEC);
    if (mEpollFd < 0) {
        ALOGE("epoll_create failed.");
        return;
    }
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;
    ev.data.fd = mUeventFd;
    if (epoll_ctl(mEpollFd, EPOLL_CTL_ADD, mUeventFd, &ev) == -1) {
        ALOGE("epoll_ctl failed.");
        return;
    }
}

HdmiCecBusMonitor::~HdmiCecBusMonitor() {

    close(mUeventFd);
    close(mEpollFd);
}

bool HdmiCecBusMonitor::threadLoop() {
    ALOGI("Cec monitor thread start.");
    while (!exitPending()) {
        int eventNum = epoll_wait(mEpollFd, mPendingEventItems, EPOLL_MAX_EVENTS, NO_TIMEOUT);
        if (eventNum <= 0) {
            ALOGE("epoll_wait fails.");
            continue;
        }
        for (int i = 0; i < eventNum; i++) {
            if (mPendingEventItems[i].events & EPOLLIN) {
                processUevent();
            }
        }
    }
    ALOGI("HdmiCecBusMonitor exit.");
    return false;
}

void HdmiCecBusMonitor::processUevent() {
    char msg[UEVENT_MSG_LEN + 2];
    char* cp;
    int n;

    n = uevent_kernel_multicast_recv(mUeventFd, msg, UEVENT_MSG_LEN);
    if (n <= 0) return;
    if (n >= UEVENT_MSG_LEN) /* overflow -- discard */
        return;

    msg[n] = '\0';
    msg[n + 1] = '\0';
    cp = msg;

    while (*cp) {
        if (strstr(cp, UEVENT_TYPE_HOTPLUG)) {
            // hotplug event
            //ALOGD("receive uevent hotplug %s", cp);
            if (mListener != NULL) {
                mListener->onCecEvent(HDMI_EVENT_HOT_PLUG);
            }
            break;
        } else if (!strcmp(cp, UEVENT_TYPE_MESSAGE)) {
            // message event
            //ALOGD("receive uevent message %s", cp);
            if (mListener != NULL) {
                mListener->onCecEvent(HDMI_EVENT_CEC_MESSAGE);
            }
            break;
        }
        /* advance to after the next \0 */
        while (*cp++);
    }

}

} // namespace android
