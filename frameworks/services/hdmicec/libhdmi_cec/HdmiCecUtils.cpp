/* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: c++ file
*/

#define LOG_TAG "hdmicecd"

#include <log/log.h>
#include <string.h>
#include <cutils/properties.h>

namespace android {

void getProperty(const char *key, char *value, const char *def) {
    property_get(key, value, def);
}

bool getPropertyBoolean(const char *key, bool def) {
    int len;
    char buf[PROPERTY_VALUE_MAX] = {0};
    bool result = def;

    len = property_get(key, buf, "");

    if (len == 1) {
        char ch = buf[0];
        if (ch == '0' || ch == 'n') {
            result = false;
        } else if (ch == '1' || ch == 'y') {
            result = true;
        }
    } else if (len > 1) {
        if (!strcmp(buf, "no") || !strcmp(buf, "false") || !strcmp(buf, "off")) {
            result = false;
        } else if (!strcmp(buf, "yes") || !strcmp(buf, "true") || !strcmp(buf, "on")) {
            result = true;
        }
    }
    return result;
}

void setProperty(const char *key, const char *value) {
    int err;
    err = property_set(key, value);

    if (err < 0) {
        ALOGE("failed to set system property %s\n", key);
    }
}

} // namespace android
