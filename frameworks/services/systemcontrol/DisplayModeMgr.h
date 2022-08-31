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

#include <utils/Singleton.h>
#include <DisplayAdapter.h>

namespace meson {
    class DisplayAdapter;
}

using namespace android;
using ConnectorType = meson::DisplayAdapter::ConnectorType;

/*
enum {
    DISPLAY_TYPE_NONE                   = 0,
    DISPLAY_TYPE_TABLET                 = 1,
    DISPLAY_TYPE_MBOX                   = 2,
    DISPLAY_TYPE_TV                     = 3,
    DISPLAY_TYPE_REPEATER               = 4
};
*/

class DisplayModeMgr : public android::Singleton<DisplayModeMgr> {
public:
    DisplayModeMgr();
    ~DisplayModeMgr();

    bool getDisplayMode(char *mode, int len);
    bool getDisplayMode(std::string& mode);
    bool getDisplayMode(std::string& mode, ConnectorType display);
    bool setDisplayMode(std::string mode);
    bool setDisplayMode(std::string mode, ConnectorType display);

    bool setDisplayAttribute(std::string cmd, std::string attribute);
    bool setDisplayAttribute(std::string cmd, std::string attribute, ConnectorType display);
    bool getDisplayAttribute(std::string cmd, std::string& attribute);
    bool getDisplayAttribute(std::string cmd, std::string& attribute, ConnectorType display);

    bool setDisplayRect(int left, int top, int width, int height);
    bool setDisplayRect(const meson::Rect rect);
    bool setDisplayRect(const meson::Rect rect, ConnectorType display);

    bool updateConnectorType();

private:
    bool init();
    bool initConnectType();

    ConnectorType mConnType;
    int mDisplayType;

    std::unique_ptr<meson::DisplayAdapter> mDisplayAdapter;
};
