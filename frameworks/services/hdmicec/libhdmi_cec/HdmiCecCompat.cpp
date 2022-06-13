/* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: C++ file
*/

#define LOG_TAG "hdmicecd"

#include <HdmiCecBase.h>
#include "HdmiCecCompat.h"

namespace android {

void compatLangIfneeded(int vendorId, int language, hdmi_cec_event_t* event) {
    if (!event) {
        return;
    }

    // Replace the vendors' wrong code to simlified Chinese.
    if (isCompatVendor(vendorId, language)) {
        ALOGI("replace Simplified Chinese code for %x", vendorId);
        event->cec.body[1] = ZH_LANGUAGE[0];
        event->cec.body[2] = ZH_LANGUAGE[1];
        event->cec.body[3] = ZH_LANGUAGE[2];
        return;
    }
}

bool isCompatVendor(int vendorId, int language) {
    // For traditional chinese code
    if (CHI == language) {
        for (unsigned int i = 0; i < sizeof(COMPAT_TV) / sizeof(COMPAT_TV[0]); i++) {
            if (vendorId == COMPAT_TV[i]) {
                return true;
            }
        }
    }
    // For korean code
    if (KOR == language && LG == vendorId) {
        return true;
    }

    return false;
}

bool needSendPlayKey(int vendorId) {
    for (unsigned int i = 0; i < sizeof(COMPAT_PLAYBACK) / sizeof(COMPAT_PLAYBACK[0]); i++) {
        if (vendorId == COMPAT_PLAYBACK[i]) {
            return true;
        }
    }
    return false;
}

} //namespace android
