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
 */

#include <cutils/properties.h>
#include <DisplayModeMgr.h>

#include "SysTokenizer.h"
#include "SysWrite.h"
#include "common.h"

#define CHECK_DISPLAY_SERVICE() \
    if (!mDisplayAdapter) { \
        SYS_LOGI("getDisplayMode displayAdapter not ready"); \
        return false; \
    } \

#define DISPLAY_CFG_FILE                "/vendor/etc/mesondisplay.cfg"
#define DISPLAY_HDMI_USED               "/sys/class/amhdmitx/amhdmitx0/hdmi_used"
#define DEVICE_STR_MID                  "MID"
#define DEVICE_STR_MBOX                 "MBOX"
#define DEVICE_STR_TV                   "TV"
#define PROP_TVSOC_AS_MBOX              "vendor.tv.soc.as.mbox"
#define HDMI_ONLY                       "persist.vendor.sys.hdmionly"

enum {
    DISPLAY_TYPE_NONE                   = 0,
    DISPLAY_TYPE_TABLET                 = 1,
    DISPLAY_TYPE_MBOX                   = 2,
    DISPLAY_TYPE_TV                     = 3,
    DISPLAY_TYPE_REPEATER               = 4
};

ANDROID_SINGLETON_STATIC_INSTANCE(DisplayModeMgr)

DisplayModeMgr::DisplayModeMgr() {
#ifndef RECOVERY_MODE
    mDisplayAdapter = meson::DisplayAdapterCreateRemote();
#else
    mDisplayAdapter = meson::DisplayAdapterCreateLocal(meson::DisplayAdapter::BackendType::DISPLAY_TYPE_FBDEV);
#endif
    init();
}

DisplayModeMgr::~DisplayModeMgr() {
}

bool DisplayModeMgr::init() {
    if (mDisplayAdapter) {
        while (true) {
            SYS_LOGI("Wait DisplayAdapter ready");
            if (mDisplayAdapter->isReady()) {
                break;
            }
            usleep(1000);
        }
    }
    initConnectType();

    return true;
}

bool DisplayModeMgr::initConnectType() {
    const char* WHITESPACE = " \t\r";
    SysTokenizer* tokenizer;
    int status = SysTokenizer::open(DISPLAY_CFG_FILE, &tokenizer);
    mConnType = ConnectorType::CONN_TYPE_HDMI;
    SysWrite *pSysWrite = new SysWrite();

    if (status) {
        SYS_LOGE("Error %d opening display config file %s.", status, DISPLAY_CFG_FILE);
    } else {
        while (!tokenizer->isEof()) {
            tokenizer->skipDelimiters(WHITESPACE);
            if (!tokenizer->isEol() && tokenizer->peekChar() != '#') {
                char *token = tokenizer->nextToken(WHITESPACE);

                if (!strcmp(token, DEVICE_STR_MBOX)) {
                    mDisplayType = DISPLAY_TYPE_MBOX;
                } else if (!strcmp(token, DEVICE_STR_TV)) {
                    mDisplayType = DISPLAY_TYPE_TV;
                } else if (!strcmp(token, DEVICE_STR_MID)) {
                    mDisplayType = DISPLAY_TYPE_TABLET;
                } else {
                    SYS_LOGE("%s: Expected keyword, got '%s'.", tokenizer->getLocation(), token);
                    break;
                }
            }
            tokenizer->nextLine();
        }
        delete tokenizer;
    }

    //if TVSOC as Mbox, change mDisplayType to DISPLAY_TYPE_REPEATER. and it will be in REPEATER process.
    if ((DISPLAY_TYPE_TV == mDisplayType) && (pSysWrite->getPropertyBoolean(PROP_TVSOC_AS_MBOX, false))) {
        mDisplayType = DISPLAY_TYPE_REPEATER;
    }

    // detect connector Type
    if (mDisplayType == DISPLAY_TYPE_TV) {
        mConnType = ConnectorType::CONN_TYPE_PANEL;
    } else {
        if (!access(DISPLAY_HDMI_USED, F_OK)) {
            char hdmi_state[MODE_LEN] = {0};
            pSysWrite->readSysfs(DISPLAY_HDMI_USED, hdmi_state);
            if (strstr(hdmi_state, "1") || pSysWrite->getPropertyBoolean(HDMI_ONLY, false))
                mConnType = ConnectorType::CONN_TYPE_HDMI;
            else
                mConnType = ConnectorType::CONN_TYPE_CVBS;
        } else {
            mConnType = ConnectorType::CONN_TYPE_HDMI;
        }
    }

    delete pSysWrite;

    return status;
}

bool DisplayModeMgr::getDisplayMode(char *mode, int len) {
    bool ret = false;
    std::string curMode = "null";
    ret = mDisplayAdapter->getDisplayMode(curMode, mConnType);
    strncpy(mode, curMode.c_str(), len-1);
    return ret;
}

bool DisplayModeMgr::getDisplayMode(std::string& mode) {
    return getDisplayMode(mode, mConnType);
}

bool DisplayModeMgr::getDisplayMode(std::string& mode, ConnectorType display) {
    CHECK_DISPLAY_SERVICE();
    return mDisplayAdapter->getDisplayMode(mode, display);
}

bool DisplayModeMgr::setDisplayMode(std::string mode) {
    return setDisplayMode(mode, mConnType);
}

bool DisplayModeMgr::setDisplayMode(std::string mode, ConnectorType display) {
    CHECK_DISPLAY_SERVICE();
    return mDisplayAdapter->setDisplayMode(mode, display);
}

bool DisplayModeMgr::setDisplayAttribute(std::string name, std::string value) {
    return setDisplayAttribute(name, value, mConnType);
}

bool DisplayModeMgr::setDisplayAttribute(std::string name, std::string value, ConnectorType display) {
    CHECK_DISPLAY_SERVICE();
    return mDisplayAdapter->setDisplayAttribute(name, value, display);
}

bool DisplayModeMgr::getDisplayAttribute(std::string name, std::string& value) {
    return getDisplayAttribute(name, value, mConnType);
}

bool DisplayModeMgr::getDisplayAttribute(std::string name, std::string& value, ConnectorType display) {
    CHECK_DISPLAY_SERVICE();
    return mDisplayAdapter->getDisplayAttribute(name, value, display);
}
bool DisplayModeMgr::setDisplayRect(const meson::Rect rect) {
    return setDisplayRect(rect, mConnType);
}

bool DisplayModeMgr::setDisplayRect(const meson::Rect rect, ConnectorType display) {
    CHECK_DISPLAY_SERVICE();
    return mDisplayAdapter->setDisplayRect(rect, display);
}

bool DisplayModeMgr::setDisplayRect(int left,  int top, int width, int height) {
    return setDisplayRect({left, top, width, height}, mConnType);
}

bool DisplayModeMgr::updateConnectorType() {
    // For MBox, if connected hdmi, never switch back to cvbs
    if (mDisplayType == DISPLAY_TYPE_MBOX && !access(DISPLAY_HDMI_USED, F_OK)) {
        char hdmi_state[MODE_LEN] = {0};
        SysWrite *pSysWrite = new SysWrite();

        pSysWrite->readSysfs(DISPLAY_HDMI_USED, hdmi_state);
        if (strstr(hdmi_state, "1") || pSysWrite->getPropertyBoolean(HDMI_ONLY, false))
            mConnType = ConnectorType::CONN_TYPE_HDMI;
        else
            mConnType = ConnectorType::CONN_TYPE_CVBS;
        delete pSysWrite;
    }

    return true;
}
