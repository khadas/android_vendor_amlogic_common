/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_TAG "aml_vicp"

#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <log/log.h>
#include "vicp_func.h"

int open_vicp(void)
{
    int fd = -1;

    fd = open(VICP_DEV, O_RDWR);
    if (fd < 0) {
        ALOGE("open %s failed!error no %d\n", VICP_DEV, errno);
    }

    return fd;
}

int close_vicp(int fd)
{
    int ret = -1;

    ret = close(fd);
    if (ret < 0)
        return -errno;
    return ret;
}

int process_vicp(int fd, vicp_data_info_t *vicp_data)
{
    int ret = -1;

    if (!vicp_data) {
        ALOGE("%s: NULL param!\n", __func__);
        return -1;
    }

    ret = ioctl(fd, VICP_PROCESS, vicp_data);
    if (ret < 0)
        ALOGE("%s failed(%s).\n", __func__, strerror(errno));

    return ret;
}
