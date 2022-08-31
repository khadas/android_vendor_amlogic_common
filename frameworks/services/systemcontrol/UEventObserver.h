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

 #ifndef _SYSTEM_CONTROL_UEVENT_OBSERVER_H
#define _SYSTEM_CONTROL_UEVENT_OBSERVER_H

#include "FrameRateAutoAdaption.h"
#include "HDCP/HDCPTxAuth.h"

#include "SysWrite.h"

#ifndef RECOVERY_MODE
#include <utils/Vector.h>
#include <utils/String8.h>
#include <utils/Mutex.h>

using namespace android;
#endif

#define HDMI_TX_UEVENT    "change@/devices/virtual/amhdmitx/amhdmitx0"   //hdmi uevent

#define UEVENT_HPD                   "hdmitx_hpd"
#define UEVENT_HDCP                  "hdmitx_hdcp"
#define UEVENT_HDMI_AUDIO            "hdmitx_audio"
#define UEVENT_HDMI_POWER            "hdmitx_hdcppwr"
#define UEVENT_HDMI_HDR              "hdmitx_hdr"
#define UEVENT_RXSENSE               "hdmitx_rxsense"
#define UEVENT_CEDST                 "hdmitx_cedst"

typedef struct _match_node{
    char *buf;
	struct _match_node *next;
} match_node_t;

typedef struct {
    int num; //match string item number
    match_node_t strList;
} match_item_t;

// ----------------------------------------------------------------------------
class UEventObserver
{
public:
    class HDMITxUevntCallbak {
    public:
        HDMITxUevntCallbak() {};
        virtual ~HDMITxUevntCallbak() {};
        virtual void onTxEvent (char* switchName, char* hpdstate, int outputState) = 0;
    };

    UEventObserver();
    ~UEventObserver();

    std::vector<std::string> strSplit(const std::string& s, const std::string& delim="=");
    void setUevntCallback (HDMITxUevntCallbak *cb);
    void setFRAutoAdpt (FrameRateAutoAdaption *mFRAutoAdpt);
    void setHDCPTxAuth(HDCPTxAuth *cb);
    void addMatch(const char *matchStr);
    void removeMatch(const char *matchStr);
    void waitForNextEvent(uevent_data_t* ueventData);
    int ueventGetFd();
    void setLogLevel(int level);
    void setSuspendResume(bool status);
    bool getSuspendResume(void);
    void setSysCtrlReady(bool status);
    int start_hdmitxuevent_thread();

private:
    int ueventInit();
    bool isMatch(const char* buffer, size_t length, uevent_data_t* ueventData, const char *matchStr);
    bool isMatch(const char* buffer, size_t length, uevent_data_t* ueventData);
    int ueventNextEvent(char* buffer, int buffer_length);
    void ueventPrint(char* ueventBuf, int len);
    static void* HDMITxUenventThreadLoop(void* data);

    int mFd;
    int mLogLevel;
    bool mSuspendResume;

    HDMITxUevntCallbak *pmHDMITxUevntCallbak = NULL;
    FrameRateAutoAdaption *pmFrameRateAutoAdaption = NULL;
    HDCPTxAuth *pmHDCPTxAuth = NULL;

    SysWrite mSysWrite;

    match_item_t mMatchStr;//for none recovery mode

#ifndef RECOVERY_MODE
    Mutex gMatchesMutex;
    Vector<String8> gMatches;
#endif
};
// ----------------------------------------------------------------------------
#endif /*_SYSTEM_CONTROL_UEVENT_OBSERVER_H*/
