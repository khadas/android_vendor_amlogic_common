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

#define LOG_NDEBUG 0
#define LOG_TAG "ScreenControlDebug"

#include <strings.h>
#include <stdlib.h>
#include <limits.h>
#include <cutils/properties.h>
#include <utils/Log.h>

#include "ScreenControlDebug.h"

namespace android {

#define SCREENCONTROL_DEBUG_PROP "ro.vendor.screencontrol.debug"

ScreenControlDebug::ScreenControlDebug() {
}

ScreenControlDebug::~ScreenControlDebug() {
}

bool ScreenControlDebug::mIsInit = false;
bool ScreenControlDebug::mCanDebug = false;

bool ScreenControlDebug::initDebug() {
    if (mIsInit) {
        return true;
    }

    char prop[PROPERTY_VALUE_MAX] = {0};
    ALOGD("Initial debug property");
    if (property_get(SCREENCONTROL_DEBUG_PROP, prop, "0") > 0) {
        bool result = false;
        ALOGD("Prop [%s]=%s", SCREENCONTROL_DEBUG_PROP, prop);
        if (!strcasecmp(prop, "true")) {
            result = true;
        } else {
            // try convert to number value
            char *tmp = NULL;
            long int propValue = strtol(prop, &tmp, 0);
            if (LONG_MIN != propValue && LONG_MAX != propValue && 0 != propValue) {
                result = true;
            }
        }
        mCanDebug = result;
    }
    mIsInit = true;
    return true;
}

bool ScreenControlDebug::canDebug() {
    return mCanDebug;
}

}

