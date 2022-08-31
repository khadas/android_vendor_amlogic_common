/* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: Header file
*/

#ifndef HDMICECUTILS_H
#define HDMICECUTILS_H

namespace android {

void getProperty(const char *key, char *value, const char *def);

bool getPropertyBoolean(const char *key, bool def);

void setProperty(const char *key, const char *value);

} // namespace android

#endif // HDMICECUTILS_H

