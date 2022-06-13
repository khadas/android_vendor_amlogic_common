/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
* *
This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
* *
Description:
*/

#ifndef _RECOVERY_INSTALL_AMLOGIC_H_
#define _RECOVERY_INSTALL_AMLOGIC_H_

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/parseint.h>
#include <android-base/properties.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>
#include <android-base/unique_fd.h>

#define AML_SDCARD_ROOT    "/sdcard"
#define AML_UDISK_ROOT      "/udisk"
#define UDISK_DEVICE            "/dev/block/sd##"
#define SDCARD_DEVICE         "/dev/block/mmcblk#p#"

void amlogic_init();
int wipe_param(void);
void amlogic_get_args(std::vector<std::string>& args);
int ensure_storage_mounted(const char* device, const char* mount_point);

#endif

