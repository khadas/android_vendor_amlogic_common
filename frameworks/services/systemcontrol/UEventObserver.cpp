/*
 * Copyright (C) 2011 The Android Open Source Project
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
 *  @date     2016/09/06
 *  @par function description:
 *  - 1 process uevent for system control
 */

#define LOG_TAG "SystemControl"
//#define LOG_NDEBUG 0

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <linux/netlink.h>

#include <log/log.h>
#include "common.h"
#include "UEventObserver.h"
#include "DisplayMode.h"

UEventObserver::UEventObserver()
    :mFd(-1),
    mLogLevel(LOG_LEVEL_DEFAULT) {

    mFd = ueventInit();
    mMatchStr.num = 0;
    mMatchStr.strList.buf = NULL;
    mMatchStr.strList.next = NULL;
}

UEventObserver::~UEventObserver() {
    if (mFd >= 0) {
        close(mFd);
        mFd = -1;
    }
}

int UEventObserver::ueventInit() {
    struct sockaddr_nl addr;
    int sz = 64*1024;
    int s;

    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    //addr.nl_pid = pthread_self() << 16 | getpid();
    addr.nl_pid = gettid();
    addr.nl_groups = 0xffffffff;

    s = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    if (s < 0)
        return 0;

    setsockopt(s, SOL_SOCKET, SO_RCVBUFFORCE, &sz, sizeof(sz));

    if (bind(s, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        close(s);
        return 0;
    }

    return s;
}

int UEventObserver::ueventGetFd() {
    return mFd;
}

int UEventObserver::ueventNextEvent(char* buffer, int buffer_length) {
    while (1) {
        struct pollfd fds;
        int nr;

        fds.fd = mFd;
        fds.events = POLLIN;
        fds.revents = 0;
        nr = poll(&fds, 1, -1);

        if (nr > 0 && (fds.revents & POLLIN)) {
            int count = recv(mFd, buffer, buffer_length, 0);
            if (count > 0) {
                return count;
            }
        }
    }

    // won't get here
    return 0;
}

std::vector<std::string> UEventObserver::strSplit(const std::string& s, const std::string& delim)
{
    std::vector<std::string> elems;
    size_t pos = 0;
    size_t len = s.length();
    size_t delim_len = delim.length();

    if (delim_len == 0) {
        return elems;
    }

    while (pos < len) {
        int find_pos = s.find(delim, pos);
        if (find_pos < 0) {
            elems.push_back(s.substr(pos, len - pos));
            break;
        }
        elems.push_back(s.substr(pos, find_pos - pos));
        pos = find_pos + delim_len;
    }
    return elems;
}

bool UEventObserver::isMatch(const char* buffer, size_t length,
    uevent_data_t* ueventData, const char *matchStr) {
    bool matched = false;
    // Consider all zero-delimited fields of the buffer
    const char* field = buffer;
    const char* end = buffer + length + 1;
    do {
        if (!strcmp(field, matchStr)) {

            strcpy(ueventData->matchName, matchStr);
            matched = true;
        }
        // for new uevent tx
        if (matched && (std::string(field).substr(0, 7) == std::string("hdmitx_"))) {
            std::vector<std::string> values = strSplit(std::string(field), "=");
            if (2 != values.size()) {
                SYS_LOGE("the field is wrong: %s", field);
                continue;
            }
            strcpy(ueventData->switchName, values[0].c_str());
            strcpy(ueventData->switchState, values[1].c_str());
        } else if (matched && (std::string(field).find("rx22=") != std::string(field).npos)) { // for new uevent rx
            std::vector<std::string> values = strSplit(std::string(field), "=");
            if (2 != values.size()) {
                SYS_LOGE("the rx22 field is wrong: %s", field);
                continue;
            }
            strcpy(ueventData->switchName, values[0].c_str());
            strcpy(ueventData->switchState, values[1].c_str());
        }

        field += strlen(field) + 1;
    } while (field != end);

    if (matched) {
        ueventData->len = length;
        memcpy(ueventData->buf, buffer, length);
        return matched;
    }

    return matched;
}

bool UEventObserver::isMatch(const char* buffer, size_t length, uevent_data_t* ueventData) {
    bool matched = false;
#ifdef RECOVERY_MODE
    match_node_t *strItem = &mMatchStr.strList;
    //the first node is null
    for (size_t i = 0; i < (unsigned int)mMatchStr.num; i++) {
        const char *matchStr = strItem->buf;
        strItem = strItem->next;
        matched = isMatch(buffer, length, ueventData, matchStr);
        if (matched)
            break;
    }
#else
    AutoMutex _l(gMatchesMutex);
    for (size_t i = 0; i < gMatches.size(); i++) {
        const char *matchStr = gMatches.itemAt(i).string();
        matched = isMatch(buffer, length, ueventData, matchStr);
        if (matched)
            break;
    }
#endif

    return matched;
}

void UEventObserver::waitForNextEvent(uevent_data_t* ueventData) {
    char buffer[1024];

    for (;;) {
        int length = ueventNextEvent(buffer, sizeof(buffer) - 1);
        if (length <= 0) {
            SYS_LOGE("Received uevent message length: %d", length);
            return;
        }
        buffer[length] = '\0';

        ueventPrint(buffer, length);
        if (isMatch(buffer, length, ueventData))
            return;
    }
}

void UEventObserver::addMatch(const char *matchStr) {
#ifdef RECOVERY_MODE
    match_node_t *strItem = &mMatchStr.strList;
    //the first node is null
    if (NULL == strItem->buf) {
        strItem->buf = (char *)malloc(strlen(matchStr) + 1);
        strcpy(strItem->buf, matchStr);
        strItem->next = NULL;

        mMatchStr.num++;
        return;
    }

    while (NULL != strItem->buf) {
        if (!strcmp(strItem->buf, matchStr)) {
            SYS_LOGE("have added uevent : %s before", matchStr);
            return;
        }

        //the last node
        if (NULL == strItem->next) {
            SYS_LOGI("no one match the uevent : %s, add it to list", matchStr);
            break;
        }
        strItem = strItem->next;
    }

    match_node_t *newNode = (match_node_t *)malloc(sizeof(match_node_t));
    newNode->buf = (char *)malloc(strlen(matchStr) + 1);
    strcpy(newNode->buf, matchStr);
    newNode->next = NULL;

    //add the new node to the list
    strItem->next = newNode;

    mMatchStr.num++;

#else
    AutoMutex _l(gMatchesMutex);
    gMatches.add(String8(matchStr));
#endif
}

void UEventObserver::removeMatch(const char *matchStr) {
#ifdef RECOVERY_MODE
    match_node_t *headItem = &mMatchStr.strList;
    match_node_t *curItem = headItem;
    match_node_t *preItem = curItem;
    while (NULL != curItem->buf) {
        if (!strcmp(curItem->buf, matchStr)) {
            SYS_LOGI("find the match uevent : %s, remove it", matchStr);
            match_node_t *tmpNode = curItem->next;
            free(curItem->buf);
            curItem->buf = NULL;
            //head item do not need free
            if (curItem != headItem) {
                free(curItem);
                curItem = NULL;
            }

            preItem->next = tmpNode;
            mMatchStr.num--;
            return;
        }

        //the last node
        if (NULL == curItem->next) {
            SYS_LOGE("can not find the match uevent : %s", matchStr);
            return;
        }
        preItem = curItem;
        curItem = curItem->next;
    }

#else
    AutoMutex _l(gMatchesMutex);
    for (size_t i = 0; i < gMatches.size(); i++) {
        if (gMatches.itemAt(i) == matchStr) {
            gMatches.removeAt(i);
            break; // only remove first occurrence
        }
    }
#endif
}

void UEventObserver::setLogLevel(int level) {
    mLogLevel = level;
}

void UEventObserver::ueventPrint(char* ueventBuf, int len) {
    if (mLogLevel > LOG_LEVEL_1) {
        //change@/devices/virtual/switch/hdmi ACTION=change DEVPATH=/devices/virtual/switch/hdmi
        //SUBSYSTEM=switch SWITCH_NAME=hdmi SWITCH_STATE=0 SEQNUM=2791
        //for new uevent:
        //change@/devices/virtual/amhdmitx/amhdmitx0 ACTION=change
        //DEVPATH=/devices/virtual/amhdmitx/amhdmitx0 SUBSYSTEM=amhdmitx hdmitx_hpd=0
        //MAJOR=509 MINOR=0 DEVNAME=amhdmitx0 SEQNUM=2753

        char printBuf[1024] = {0};
        memcpy(printBuf, ueventBuf, len);
        for (int i = 0; i < len; i++) {
            if (printBuf[i] == 0x0)
                printBuf[i] = ' ';
        }

        SYS_LOGI("Received uevent message: %s", printBuf);
    }
}

void UEventObserver::setUevntCallback (HDMITxUevntCallbak *cb) {
    pmHDMITxUevntCallbak = cb;
}

void UEventObserver::setFRAutoAdpt(FrameRateAutoAdaption *mFRAutoAdpt) {
   pmFrameRateAutoAdaption = mFRAutoAdpt;
}

void UEventObserver::setHDCPTxAuth(HDCPTxAuth *cb) {
   pmHDCPTxAuth = cb;
}

void UEventObserver::setSuspendResume(bool status) {
   mSuspendResume = status;
}

bool UEventObserver::getSuspendResume(void) {
   return mSuspendResume;
}

void UEventObserver::setSysCtrlReady(bool status) {
    if (status == true) {
        mSysWrite.writeSysfs(DISPLAY_HDMI_SYSCTRL_READY, "1");
    } else {
        mSysWrite.writeSysfs(DISPLAY_HDMI_SYSCTRL_READY, "0");
    }
}

//start HDMI TX UEVENT prcessed thread
int UEventObserver::start_hdmitxuevent_thread() {
    int ret;
    pthread_t thread_id;

    setSuspendResume(false);

    addMatch(HDMI_TX_UEVENT);

    ret = pthread_create(&thread_id, NULL, HDMITxUenventThreadLoop, this);
    if (ret != 0) {
        SYS_LOGE("Create HDMITxUenventThreadLoop error :%d!\n", ret);
    }

    return 0;
}
// HDMI TX uevent prcessed in this loop
void* UEventObserver::HDMITxUenventThreadLoop(void* data) {
    UEventObserver *pThiz = (UEventObserver*)data;

    uevent_data_t ueventData;

    //systemcontrol ready for driver
    pThiz->setSysCtrlReady(true);

    while (true) {
        memset(&ueventData, 0, sizeof(uevent_data_t));
        pThiz->waitForNextEvent(&ueventData);

        SYS_LOGI("HDMI TX switch_name:%s, switch_state: %s\n", ueventData.switchName, ueventData.switchState);

        // for new uevent handle
        if (!strcmp(ueventData.matchName, HDMI_TX_UEVENT)) {
            if (pThiz->getSuspendResume()) {
                // hdmi audio uevent
                if (!strcmp(ueventData.switchName, UEVENT_HDMI_AUDIO)  && (NULL != pThiz->pmHDMITxUevntCallbak)) {
                    pThiz->pmHDMITxUevntCallbak->onTxEvent(ueventData.switchName, ueventData.switchState, OUPUT_MODE_STATE_POWER);
                } else if ((!strcmp(ueventData.switchName, UEVENT_HDMI_POWER)) &&
                    (!strcmp(ueventData.switchState, HDMI_TX_RESUME)) &&
                    (NULL != pThiz->pmHDMITxUevntCallbak)) {
                    pThiz->setSuspendResume(false);
                    pThiz->pmHDMITxUevntCallbak->onTxEvent(ueventData.switchName, ueventData.switchState, OUPUT_MODE_STATE_POWER);
                }
            } else {
                //hdmi plugin/plugout uevent
                if (!strcmp(ueventData.switchName, UEVENT_HPD) && (NULL != pThiz->pmHDMITxUevntCallbak)) {
                    pThiz->pmHDMITxUevntCallbak->onTxEvent(ueventData.switchName, ueventData.switchState, OUPUT_MODE_STATE_POWER);
                }
                //hdmi suspend and resume uevent
                else if (!strcmp(ueventData.switchName, UEVENT_HDMI_POWER)) {
                    //0: hdmi suspend  1: hdmi resume
                    if (!strcmp(ueventData.switchState, HDMI_TX_RESUME) && (NULL != pThiz->pmHDMITxUevntCallbak)) {
                        pThiz->setSuspendResume(false);
                        pThiz->pmHDMITxUevntCallbak->onTxEvent(ueventData.switchName, ueventData.switchState, OUPUT_MODE_STATE_POWER);
                    }
                    else if (!strcmp(ueventData.switchState, HDMI_TX_SUSPEND) && (NULL != pThiz->pmHDMITxUevntCallbak)) {
                        pThiz->setSuspendResume(true);
                        pThiz->mSysWrite.writeSysfs(DISPLAY_HDMI_HDCP_POWER, "1");
                        pThiz->pmHDMITxUevntCallbak->onTxEvent(ueventData.switchName, ueventData.switchState, OUPUT_MODE_STATE_POWER);
                    }
                }
                // hdmi audio uevent
                else if (!strcmp(ueventData.switchName, UEVENT_HDMI_AUDIO)  && (NULL != pThiz->pmHDMITxUevntCallbak)) {
                    pThiz->pmHDMITxUevntCallbak->onTxEvent(ueventData.switchName, ueventData.switchState, OUPUT_MODE_STATE_POWER);
                }
                //hdmi hdr uevent
                else if (!strcmp(ueventData.switchName, UEVENT_HDMI_HDR)) {
                    //0: exit hdr mode  1: enter hdr mode
                    //mark this for flash black when Always HDR to Adaptive HDR
                    if (!strcmp(ueventData.switchState, "0")) {
                        //pThiz->mSysWrite.writeSysfs(DISPLAY_HDMI_AVMUTE_SYSFS, "1");
                        //usleep(100000);//100ms
                        //pThiz->pmHDCPTxAuth->stopVerAll();
                        //pThiz->pmHDCPTxAuth->stop();
                        //pThiz->mSysWrite.writeSysfs(DISPLAY_HDMI_PHY, "0"); /* Turn off TMDS PHY */
                        //usleep(200000);//200ms
                        //pThiz->mSysWrite.writeSysfs(DISPLAY_HDMI_PHY, "1"); /* Turn on TMDS PHY */
                        //pThiz->pmHDCPTxAuth->start();
                    }
                }
                //hdcp uevent
                else if (!strcmp(ueventData.switchName, UEVENT_HDCP)) {

                }
            }
        }
        //old extcon uevent handle
        //hot plug string is the hdmi audio, hdmi power, hdmi hdr substring
        else if (!strcmp(ueventData.matchName, HDMI_TX_PLUG_UEVENT) && !strcmp(ueventData.switchName, HDMI_UEVENT_HDMI) && (NULL != pThiz->pmHDMITxUevntCallbak)) {
            pThiz->pmHDMITxUevntCallbak->onTxEvent(ueventData.switchName, ueventData.switchState, OUPUT_MODE_STATE_POWER);
        }
        else if (!strcmp(ueventData.matchName, HDMI_TX_POWER_UEVENT) && !strcmp(ueventData.switchName, HDMI_UEVENT_HDMI_POWER)) {
            //0: hdmi suspend  1: hdmi resume
            if (!strcmp(ueventData.switchState, HDMI_TX_RESUME) && (NULL != pThiz->pmHDMITxUevntCallbak)) {
                pThiz->pmHDMITxUevntCallbak->onTxEvent(ueventData.switchName, ueventData.switchState, OUPUT_MODE_STATE_POWER);
            }
            else if (!strcmp(ueventData.switchState, HDMI_TX_SUSPEND)) {
                pThiz->mSysWrite.writeSysfs(DISPLAY_HDMI_HDCP_POWER, "1");
            }
        }
        else if (!strcmp(ueventData.matchName, HDMI_TX_HDR_UEVENT) && !strcmp(ueventData.switchName, HDMI_UEVENT_HDMI_HDR)) {
            //0: exit hdr mode  1: enter hdr mode
            //mark this for flash black when Always HDR to Adaptive HDR
            char hdrState[MODE_LEN] = {0};
            pThiz->mSysWrite.readSysfs(HDMI_TX_SWITCH_HDR, hdrState);
            if (!strcmp(hdrState, "0")) {
                //pThiz->mSysWrite.writeSysfs(DISPLAY_HDMI_AVMUTE_SYSFS, "1");
                //usleep(100000);//100ms
                //pThiz->pmHDCPTxAuth->stopVerAll();
                //pThiz->pmHDCPTxAuth->stop();
                //pThiz->mSysWrite.writeSysfs(DISPLAY_HDMI_PHY, "0"); /* Turn off TMDS PHY */
                //usleep(200000);//200ms
                //pThiz->mSysWrite.writeSysfs(DISPLAY_HDMI_PHY, "1"); /* Turn on TMDS PHY */
                //pThiz->pmHDCPTxAuth->start();
            }
        }
        else if (!strcmp(ueventData.matchName, HDMI_TX_HDCP_UEVENT) && !strcmp(ueventData.switchName, HDMI_UEVENT_HDCP)) {

        }
        else if (!strcmp(ueventData.matchName, HDMI_TX_HDCP14_LOG_UEVENT) && !strcmp(ueventData.switchName, HDMI_UEVENT_HDCP_LOG)) {

        }
        else if (!strcmp(ueventData.matchName, HDMI_TVOUT_FRAME_RATE_UEVENT)) {
               pThiz->pmFrameRateAutoAdaption->onTxUeventReceived(&ueventData);
        }
        else if (!strcmp(ueventData.matchName, HDMI_TX_HDMI_AUDIO_UEVENT) && !strcmp(ueventData.switchName, HDMI_UEVENT_HDMI_AUDIO)  && (NULL != pThiz->pmHDMITxUevntCallbak)) {
               pThiz->pmHDMITxUevntCallbak->onTxEvent(ueventData.switchName, ueventData.switchState, OUPUT_MODE_STATE_POWER);
        }
    }

    return NULL;
}
