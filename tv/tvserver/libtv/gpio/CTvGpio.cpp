/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_NDEBUG 0

#define LOG_TAG "tvserver"
#define LOG_TV_TAG "CTvGpio"

#include <CTvLog.h>
#include <stdlib.h>
#include <string.h>
#include <tvutils.h>
#include <assert.h>

#include "CTvGpio.h"


CTvGpio::CTvGpio()
{
    mGpioPinNum = 0;
    memset(mGpioName, 0, 64);
}

CTvGpio::~CTvGpio()
{
    if (mGpioPinNum > 0)
        tvWriteSysfs(GPIO_UNEXPORT, mGpioPinNum);
}

char* CTvGpio::convertPortName(char* port_id, const char* port_name, const char* path){
    char line[128] = {0};
    char* search_result = NULL;
    char* result = port_id;

    assert(port_id && port_name && path);

    FILE* fp = fopen(path, "r+");
    if (NULL == fp) {
        LOGV("%s, Error open file", __FUNCTION__);
        return result;
    }

    while (fgets(line, 128, fp)) {
        search_result = strstr(line, port_name);
        if (search_result) {
            //LOGV("%s, line=[%s]", __FUNCTION__, line);
            strcpy(port_id, strtok(line, " gpio-"));
            LOGV("%s, result=[%s]", __FUNCTION__, result);
            break;
        }
    }

    fclose(fp);
    return result;
}

int CTvGpio::processCommand(const char *port_name, bool is_out, int edge)
{
    LOGV("%s, port_name=[%s], is_out=[%d], edge=[%d], gpio_pin=[%d]", __FUNCTION__, port_name, is_out, edge, mGpioPinNum);
    if (strncmp(port_name, "GPIO", 4) != 0)
        return -1;

    char pin_value[10] = {0};
    char* convertPortId = convertPortName(pin_value, port_name, GPIO_KERNEL_DEBUG);
    if (NULL == convertPortId) {
        return -1;
    }
    LOGV("%s, pin_value=[%s]", __FUNCTION__, pin_value);

    if (strcmp(mGpioName, port_name) != 0) {
        strcpy(mGpioName, port_name);
        mGpioPinNum = atoi(pin_value);
    }
    LOGV("%s, port_name=[%s], is_out=[%d], edge=[%d], gpio_pin=[%d]", __FUNCTION__, port_name, is_out, edge, mGpioPinNum);

    int ret = -1;
    if (mGpioPinNum > 0) {
        if (is_out) {
            ret = setGpioOutEdge(edge);
        } else {
            ret = getGpioInEdge();
        }
    }

    return ret;
}

int CTvGpio::setGpioOutEdge(int edge)
{
    LOGD("%s, gpio_pin=[%d], edge=[%d]", __FUNCTION__, mGpioPinNum, edge);

    char direction[128] = {0};
    char value[128] = {0};
    GPIO_DIRECTION(direction, mGpioPinNum);
    GPIO_VALUE(value, mGpioPinNum);
    LOGV("dirction path:[%s]", direction);
    LOGV("value path:[%s]", value);

    if (needExportAgain(direction)) {
        tvWriteSysfs(GPIO_EXPORT, mGpioPinNum);
    }
    tvWriteSysfs(direction, "out");
    tvWriteSysfs(value, edge);

    return 0;
}

int CTvGpio::getGpioInEdge()
{
    LOGD("%s, gpio_pin=[%d]", __FUNCTION__, mGpioPinNum);

    char direction[128] = {0};
    char value[128] = {0};
    char edge[128] = {0};
    GPIO_DIRECTION(direction, mGpioPinNum);
    GPIO_VALUE(value, mGpioPinNum);
    LOGV("dirction path:[%s]", direction);
    LOGV("value path:[%s]", value);

    if (needExportAgain(direction)) {
        tvWriteSysfs(GPIO_EXPORT, mGpioPinNum);
    }
    tvWriteSysfs(direction, "in");
    tvReadSysfs(value, edge);
    LOGD("edge:[%s]", edge);

    return atoi(edge);
}

bool CTvGpio::needExportAgain(char *path) {
    return !isFileExist(path);
}

