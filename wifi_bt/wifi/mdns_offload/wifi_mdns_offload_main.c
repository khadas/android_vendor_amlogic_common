/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef __cplusplus
extern "C"
{
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "wifi_mdns_offload.h"

#define ARRAY_SIZE(X) (unsigned int)(sizeof(X) / sizeof((X)[0]))

static unsigned char rawOffloadPacket[] = {
0x00, 0x00, 0x84, 0x00, 0x00, 0x00, 0x00, 0x04,
0x00, 0x00, 0x00, 0x03, 0x02, 0x36, 0x30, 0x03,
0x31, 0x38, 0x33, 0x03, 0x31, 0x36, 0x38, 0x03,
0x31, 0x39, 0x32, 0x07, 0x69, 0x6e, 0x2d, 0x61,
0x64, 0x64, 0x72, 0x04, 0x61, 0x72, 0x70, 0x61,
0x00, 0x00, 0x0c, 0x80, 0x01, 0x00, 0x00, 0x00,
0x78, 0x00, 0x0f, 0x07, 0x41, 0x6e, 0x64, 0x72,
0x6f, 0x69, 0x64, 0x05, 0x6c, 0x6f, 0x63, 0x61,
0x6c, 0x00, 0x01, 0x39, 0x01, 0x35, 0x01, 0x43,
0x01, 0x31, 0x01, 0x31, 0x01, 0x41, 0x01, 0x45,
0x01, 0x46, 0x01, 0x46, 0x01, 0x46, 0x01, 0x35,
0x01, 0x37, 0x01, 0x42, 0x01, 0x44, 0x01, 0x34,
0x01, 0x44, 0x01, 0x30, 0x01, 0x30, 0x01, 0x30,
0x01, 0x30, 0x01, 0x30, 0x01, 0x30, 0x01, 0x30,
0x01, 0x30, 0x01, 0x30, 0x01, 0x30, 0x01, 0x30,
0x01, 0x30, 0x01, 0x30, 0x01, 0x38, 0x01, 0x45,
0x01, 0x46, 0x03, 0x69, 0x70, 0x36, 0xc0, 0x23,
0x00, 0x0c, 0x80, 0x01, 0x00, 0x00, 0x00, 0x78,
0x00, 0x02, 0xc0, 0x33, 0xc0, 0x33, 0x00, 0x01,
0x80, 0x01, 0x00, 0x00, 0x00, 0x78, 0x00, 0x04,
0xc0, 0xa8, 0xb7, 0x3c, 0xc0, 0x33, 0x00, 0x1c,
0x80, 0x01, 0x00, 0x00, 0x00, 0x78, 0x00, 0x10,
0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0xd4, 0xdb, 0x75, 0xff, 0xfe, 0xa1, 0x1c, 0x59,
0xc0, 0x0c, 0x00, 0x2f, 0x80, 0x01, 0x00, 0x00,
0x00, 0x78, 0x00, 0x06, 0xc0, 0x0c, 0x00, 0x02,
0x00, 0x08, 0xc0, 0x42, 0x00, 0x2f, 0x80, 0x01,
0x00, 0x00, 0x00, 0x78, 0xc0, 0x06, 0xc0, 0x42,
0x00, 0x02, 0x00, 0x08, 0xc0, 0x33, 0x00, 0x2f,
0x80, 0x01, 0x00, 0x00, 0x00, 0x78, 0x00, 0x08,
0xc0, 0x33, 0x00, 0x04, 0x40, 0x00, 0x00, 0x08,
};

static matchCriteria criteriaList[] = {
    {
        .type = 1,
        .nameOffset = 52,
    },
    {
        .type = 255,
        .nameOffset = 52,
    },
};

static int read_file(FILE *file, unsigned char *buf, size_t size,
    unsigned char isHex)
{
    size_t count = 0;
    int c;
    char data[3];

    if (isHex) {
        memset(data, 0, sizeof(data));
        while (!feof(file) && (fscanf(file, "%2s", data) == 1)) {
            if (count >= size)
                return -1;
            if (!strcasecmp(data, "0x"))
                continue;
            long int value = strtol(data, NULL, 16);
            if (value > 0xff)
                return -1;
            buf[count] = value;
            memset(data, 0, sizeof(data));
            count++;
        }
        return count;
    }

    while (!feof(file) && ((c = fgetc(file)) != EOF)) {
        if (count >= size)
            return -1;
        buf[count] = (unsigned char)c;
        count++;
    }

    return count;
}

static int read_hex(unsigned int argc, char **argv,
    unsigned char *buf, size_t size)
{
    unsigned int i, data;
    int res;

    if (argc > size)
        return -1;

    for (i = 0; i < argc; i++) {
        if (strstr(argv[i], "0x"))
            res = sscanf(argv[i], "0x%x", &data);
        else
            res = sscanf(argv[i], "%x", &data);
        if (res != 1 || data > 0xff)
            return -1;
        buf[i] = data;
    }

    return argc;
}

static void dump_buf(unsigned char *buf, uint32_t len)
{
    int line = 16, i = 0, j = 0;
	uint32_t n = 0;
    char *dump = NULL;

    dump = (char *)malloc(256);
    if (!dump) {
        LOGE("alloc failed!\n");
        return;
    }
    for (i = 0; i < len; i++) {
        memset(dump, 0, 256);
        n = 0;
        n += sprintf(dump + n, "%04x|", i);
        for (j = i; j < i + line; j++) {
            if (j < len)
                n += sprintf(dump + n, "%02x", buf[j]);
            else
                n += sprintf(dump + n, "  ");
            if (j == i + line - 1)
                n += sprintf(dump + n, "|");
            else
                n += sprintf(dump + n, " ");
        }
        for (j = i; j < i + line && j < len; j++) {
            if (buf[j] > 32 && buf[j] < 127)
                n += sprintf(dump + n, "%c", buf[j]);
            else
                n += sprintf(dump + n, ".");
        }
        LOGV("%s\n", dump);
        i = i + line - 1;
    }
    free(dump);
}

static void exec_setOffloadState(int argc, char **argv)
{
    if (strcmp(argv[1], "setOffloadState")
        && strcmp(argv[1], "--test"))
        return;
    LOGV("%s:\n", __func__);
    int i = 0;
    uint32_t enable = 0;
    for (i = 2; i < argc; i++) {
        if (strstr(argv[i], "--enable") && argv[i + 1]) {
            enable = atoi(argv[i + 1]);
            break;
        }
    }
    LOGV("%s: enable:%d\n", __func__, enable);
    uint32_t ret = setOffloadState(enable);
    LOGI("%s: done!ret:%d\n", __func__, ret);
}

static void exec_resetAll(int argc, char **argv)
{
    if (strcmp(argv[1], "resetAll")
        && strcmp(argv[1], "--test"))
        return;
    LOGV("%s:\n", __func__);
    resetAll();
    LOGI("%s: done!\n", __func__);
}

static void exec_addProtocolResponses(int argc, char **argv)
{
    if (strcmp(argv[1], "addProtocolResponses")
        && strcmp(argv[1], "--test"))
        return;
    LOGV("%s:\n", __func__);
    int i = 0, j = 0;
    char *ifname = "wlan0";
    mdnsProtocolData offloadData;
    offloadData.rawOffloadPacket = rawOffloadPacket;
    offloadData.rawOffloadPacketLen = ARRAY_SIZE(rawOffloadPacket);
    offloadData.matchCriteriaList = criteriaList;
    offloadData.matchCriteriaListNum = ARRAY_SIZE(criteriaList);
    uint32_t matchCriteriaListNum = 0;
    matchCriteria matchCriteriaList[16];
    unsigned char rawOffloadBuf[1024] = {};
    FILE *file = NULL;
    char *fileName = NULL;
    char **hexData =  NULL;
    unsigned char hexDataFile = 1;
    int count = 0, n = argc - 2;
    memset(&matchCriteriaList, -1, sizeof(matchCriteria));
    for (i = 2; i < argc; i++) {
        n--;
        if (strstr(argv[i], "--ifname") && argv[i + 1])
            ifname = argv[i + 1];
        if (strstr(argv[i], "--criteria") && argv[i + 1]) {
            if (2 == sscanf(argv[i + 1], "%d,%d", &matchCriteriaList[j].type,
                &matchCriteriaList[j].nameOffset)) {
                matchCriteriaListNum++;
                j++;
            }
        }
        if (strstr(argv[i], "--rawOffloadpacket") && argv[i + 1]) {
            fileName = NULL;
            file = NULL;
            hexData = NULL;
            count = 0;
            if (!strcmp(argv[i + 1], "./hexdata_file")) {
                fileName =  "./hexdata_file";
                hexDataFile = 1;
            } else if (!strcmp(argv[i + 1], "./rawdata_file")) {
                fileName =  "./rawdata_file";
                hexDataFile = 0;
            } else
                hexData = &argv[i + 1];
            if (fileName)
                file = fopen(fileName, "r");
            memset(rawOffloadBuf, 0, sizeof(rawOffloadBuf));
            if (file) {
                count = read_file(file, rawOffloadBuf,
                    sizeof(rawOffloadBuf), hexDataFile);
                if (file != stdin)
                    fclose(file);
            } else if (hexData)
                count = read_hex(n, hexData, rawOffloadBuf,
                    sizeof(rawOffloadBuf));
            if (count > 0) {
                offloadData.rawOffloadPacketLen = count;
                offloadData.rawOffloadPacket = rawOffloadBuf;
            }
        }
    }
    if (matchCriteriaListNum > 0) {
        offloadData.matchCriteriaListNum = matchCriteriaListNum;
        offloadData.matchCriteriaList = matchCriteriaList;
    }
    LOGV("%s: ifname:%s\n", __func__, ifname);
    LOGV("%s: criteria list:\n", __func__);
    for (i = 0; i < offloadData.matchCriteriaListNum; i++) {
        LOGV("%d. type:%d\tnameOffset:%d\n", i + 1,
            offloadData.matchCriteriaList[i].type,
            offloadData.matchCriteriaList[i].nameOffset);
    }
    LOGV("%s: rawOffloadpacket:\n", __func__);
    dump_buf(offloadData.rawOffloadPacket, offloadData.rawOffloadPacketLen);
    int ret = addProtocolResponses(ifname, &offloadData);
    LOGI("%s: done!ret:%d\n", __func__, ret);
}

static void exec_removeProtocolResponses(int argc, char **argv)
{
    if (strcmp(argv[1], "removeProtocolResponses")
        && strcmp(argv[1], "--test"))
        return;
    LOGV("%s:\n", __func__);
    int i = 0;
    uint32_t recordkey = 0;
    for (i = 2; i < argc; i++) {
        if (strstr(argv[i], "--recordkey") && argv[i + 1]) {
            recordkey = atoi(argv[i + 1]);
            break;
        }
    }
    LOGV("%s: recordkey:%d\n", __func__, recordkey);
    removeProtocolResponses(recordkey);
    LOGI("%s: done!\n", __func__);
}

static void exec_getAndResetHitCounter(int argc, char **argv)
{
    if (strcmp(argv[1], "getAndResetHitCounter")
        && strcmp(argv[1], "--test"))
        return;
    LOGV("%s:\n", __func__);
    int i = 0;
    uint32_t recordkey = 0;
    for (i = 2; i < argc; i++) {
        if (strstr(argv[i], "--recordkey") && argv[i + 1]) {
            recordkey = atoi(argv[i + 1]);
            break;
        }
    }
    LOGV("%s: recordkey:%d\n", __func__, recordkey);
    int ret = getAndResetHitCounter(recordkey);
    LOGI("%s: done!ret:%d\n", __func__, ret);
}

static void exec_getAndResetMissCounter(int argc, char **argv)
{
    if (strcmp(argv[1], "getAndResetMissCounter")
        && strcmp(argv[1], "--test"))
        return;
    LOGV("%s:\n", __func__);
    int ret = getAndResetMissCounter();
    LOGI("%s: done!ret:%d\n", __func__, ret);
}

static void exec_addToPassthroughList(int argc, char **argv)
{
    if (strcmp(argv[1], "addToPassthroughList")
        && strcmp(argv[1], "--test"))
        return;
    LOGV("%s:\n", __func__);
    int i = 0;
    char *ifname = "wlan0";
    char *qname = "Android.local";
    for (i = 2; i < argc; i++) {
        if (strstr(argv[i], "--ifname") && argv[i + 1])
            ifname = argv[i + 1];
        if (strstr(argv[i], "--qname") && argv[i + 1])
            qname = argv[i + 1];
    }
    LOGV("%s: ifname:%s,qname:%s\n", __func__, ifname, qname);
    uint32_t ret = addToPassthroughList(ifname, qname);
    LOGI("%s: done!ret:%d\n", __func__, ret);
}

static void exec_removeFromPassthroughList(int argc, char **argv)
{
    if (strcmp(argv[1], "removeFromPassthroughList")
        && strcmp(argv[1], "--test"))
        return;
    LOGV("%s:\n", __func__);
    int i = 0;
    char *ifname = "wlan0";
    char *qname = "Android.local";
    for (i = 2; i < argc; i++) {
        if (strstr(argv[i], "--ifname") && argv[i + 1])
            ifname = argv[i + 1];
        if (strstr(argv[i], "--qname") && argv[i + 1])
            qname = argv[i + 1];
    }
    LOGV("%s: ifname:%s,qname:%s\n", __func__, ifname, qname);
    removeFromPassthroughList(ifname, qname);
    LOGI("%s: done!\n", __func__);
}

static void exec_setPassthroughBehavior(int argc, char **argv)
{
    if (strcmp(argv[1], "setPassthroughBehavior")
        && strcmp(argv[1], "--test"))
        return;
    LOGV("%s:\n", __func__);
    int i = 0;
    char *ifname = "wlan0";
    passthroughBehavior behavior = PASSTHROUGH_LIST;
    for (i = 2; i < argc; i++) {
        if (strstr(argv[i], "--ifname") && argv[i + 1])
            ifname = argv[i + 1];
        if (strstr(argv[i], "--behavior") && argv[i + 1])
            behavior = (passthroughBehavior)atoi(argv[i + 1]);
    }
    LOGV("%s: ifname:%s,behavior:%d\n", __func__, ifname, behavior);
    setPassthroughBehavior(ifname, behavior);
    LOGI("%s: done!\n", __func__);
}

static void usage(char *name)
{
    PRINT("\n");
    PRINT("%s --help|-h\n",
          name);
    PRINT("%s [-d|--debug] --test\n",
          name);
    PRINT("%s [-d|--debug] setOffloadState\n"\
          "\t --enable 0|1\n",
          name);
    PRINT("%s [-d|--debug] resetAll\n",
          name);
    PRINT("%s [-d|--debug] addProtocolResponses\n"\
          "\t --ifname wlan0|wlan1|...\n"\
          "\t --criteria type,nameoffset [--criteria type,nameoffset]...\n"\
          "\t --rawOffloadpacket hexdata|./rawdata_file|./hexdata_file\n",
          name);
    PRINT("%s [-d|--debug] removeProtocolResponses\n"\
          "\t --recordkey value\n",
          name);
    PRINT("%s [-d|--debug] getAndResetHitCounter\n"\
          "\t --recordkey value\n",
          name);
    PRINT("%s [-d|--debug] getAndResetMissCounter\n",
          name);
    PRINT("%s [-d|--debug] addToPassthroughList\n"\
          "\t --ifname wlan0|wlan1|...\n"\
          "\t --qname value\n",
          name);
    PRINT("%s [-d|--debug] removeFromPassthroughList\n"\
          "\t --ifname wlan0|wlan1|...\n"\
          "\t --qname value\n",
          name);
    PRINT("%s [-d|--debug] setPassthroughBehavior\n"\
          "\t --ifname wlan0|wlan1|...\n"\
          "\t --behavior value\n",
          name);
    PRINT("\n");
    PRINT("e.g.:\n");
    PRINT("%s setOffloadState --enable 1\n",
          name);
    PRINT("%s --debug setOffloadState --enable 1\n",
          name);
    PRINT("%s addProtocolResponses --ifname wlan0 --criteria 1,52 "\
          "--criteria 255,52 --rawOffloadpacket 01 02 03 04\n",
          name);
    PRINT("%s addProtocolResponses --ifname wlan0 --criteria 1,52 "\
          "--criteria 255,52 --rawOffloadpacket ./rawdata_file\n",
          name);
    PRINT("%s addProtocolResponses --ifname wlan0 --criteria 1,52 "\
          "--criteria 255,52 --rawOffloadpacket ./hexdata_file\n",
          name);
    PRINT("\n");
    PRINT("Note:\n");
    PRINT("when a param is not specified, the default "\
          "param will be used.\n");
    PRINT("\n");
}

int main(int argc, char **argv)
{
    int _argc = argc;
    char **_argv = argv;

    wifi_mdns_offload_set_log(LOG_STYLE_CONSOLE, LOG_INFO_MASK);
    if (_argc == 1) {
        usage(argv[0]);
        return 0;
    } else if (!strcmp(_argv[1], "-h") || !strcmp(_argv[1], "--help")) {
        usage(argv[0]);
        return 0;
    } else if (!strcmp(_argv[1], "-d") || !strcmp(_argv[1], "--debug")) {
        wifi_mdns_offload_set_log(LOG_STYLE_CONSOLE, LOG_DEBUG_MASK);
        _argc--;
        _argv++;
        if (_argc == 1) {
            usage(argv[0]);
            return 0;
        }
    }
    wifi_mdns_offload_init();
    exec_setOffloadState(_argc, _argv);
    exec_resetAll(_argc, _argv);
    exec_addProtocolResponses(_argc, _argv);
    exec_removeProtocolResponses(_argc, _argv);
    exec_getAndResetHitCounter(_argc, _argv);
    exec_getAndResetMissCounter(_argc, _argv);
    exec_addToPassthroughList(_argc, _argv);
    exec_removeFromPassthroughList(_argc, _argv);
    exec_setPassthroughBehavior(_argc, _argv);
    wifi_mdns_offload_deinit();
    return 0;
}

#ifdef __cplusplus
}
#endif

