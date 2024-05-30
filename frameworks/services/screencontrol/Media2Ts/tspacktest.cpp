/*
 * Copyright (C) 2010 Amlogic Corporation.
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
#define LOG_TAG "TSPackerTest"
#include <cutils/log.h>
#include <fcntl.h>
#include <limits.h>
#include "../ScreenControlClient.h"
#include "../ScreenManager.h"

using namespace android;
#define MAX_FILE_PATH_SIZE 128

static const char* CAPTURE_TYPE_STR_ARR[] = {"video only", "video+osd", "osd only"};

static const char* opt_str = "hlnf:b:t:s:c:";
static void help(char* appName) {
    printf(
        "Usage:\n"
        "  %s [-h/-n] [-f <framerate>] [-b <bitrate>] [-t <type>] [-s <second>] [<left>  <top>  <right>  <bottom> <width> <height>]\n"
        "\n"
        "Parameters:\n"
        "  -h            : show this help\n"
        "  -n            :  no save as file \n"
        "  -l            :  specify loop mode \n"
        "  -f <framerate>: frame per second, unit bps, default as 30\n"
        "  -b <bitrate>  : bits per second, unit bit, default as 4000000\n"
        "  -t <type> : set capture type:\n"
        "             0 -- video only \n"
        "             1 -- video+osd (default) \n"
        "             2 -- osd only \n"
        "  -s <second>   : record times, unit second(s), default as 30\n"
        "  -c <counter>     : continually save file with counter, default as 1\n"
        "  left  top  right  bottom : capture area, default as 720P (0,0,1280,720) \n"
        "  width height  : output size, default as 720P (1280X720)\n"
        "\n"
        "\n"
        "---NOTICE---\n"
        "Default save [es] files to /data/temp/ \n"
        "Pls run following commands before use:\n"
        "      mkdir -p /data/temp; chmod 777 /data/temp \n",
        appName);
}

int main(int argc, char** argv) {
    int counter = 1;
    int nowCounter = 0;
    int type = 1;
    int ch;
    int tmpArgIdx = 0;
    int left = 0, top = 0, right = 1280, bottom = 720;
    int timeSecond = 30;
    int framerate = 30, bitrate = 4000000;
    int outWidth = 1280, outHeight = 720;
    char dump_dir[64] = "/data/temp";
    char dump_path[MAX_FILE_PATH_SIZE];
    bool isSaveFile = true;
    while ((ch = getopt(argc, argv, opt_str)) != -1) {
        switch (ch) {
            case 'h':
                help(argv[0]);
                exit(0);
            case 'n':
                isSaveFile = false;
                break;
            case 'l':
                counter = INT_MAX;
                break;
            case 'f':
                framerate = atoi(optarg);
                break;
            case 'b':
                bitrate = atoi(optarg);
                break;
            case 'c':
                counter = atoi(optarg);
                break;
            case 's':
                timeSecond = atoi(optarg);
                break;
            case 't':
                type = atoi(optarg);
                break;
            default:
                break;
        }
    }
    tmpArgIdx = optind;
    if ((tmpArgIdx + 1) < argc) {
        if ((argc - tmpArgIdx) == 6) {
            left = atoi(argv[tmpArgIdx++]);
            top = atoi(argv[tmpArgIdx++]);
            right = atoi(argv[tmpArgIdx++]);
            bottom = atoi(argv[tmpArgIdx++]);
            outWidth = atoi(argv[tmpArgIdx++]);
            outHeight = atoi(argv[tmpArgIdx++]);
        } else if ((argc - tmpArgIdx) == 2) {
            left = 0;
            top = 0;
            right = atoi(argv[tmpArgIdx++]);
            bottom = atoi(argv[tmpArgIdx++]);
            outWidth = right;
            outHeight = bottom;
        }
    }
    printf(
        "size     =[%dX%d]\n"
        "(left,top,right,bottom)=(%d,%d,%d,%d)\n"
        "framerate=%dbps\n"
        "bitrate  =%d\n"
        "type     =%s\n"
        "isSaveFile     =%d\n"
        "counter     =%d\n"
        "time     =%ds\n",
        outWidth, outHeight, left, top, right, bottom, framerate, bitrate, CAPTURE_TYPE_STR_ARR[type], isSaveFile,
        counter, timeSecond);
    while (1) {
        ScreenControlClient* client = ScreenControlClient::getInstance();
        memset(dump_path, 0, MAX_FILE_PATH_SIZE);
        if (isSaveFile) {
            snprintf(dump_path, 128, "%s/%dx%d-%d.ts", dump_dir, outWidth, outHeight, nowCounter);
        } else {
            snprintf(dump_path, 128, "%s/%dx%d-1.ts", dump_dir, outWidth, outHeight);
        }
        int64_t firstTimeUs = getNowTimesUs();
        ALOGD("start ts screen record dump_path=%s", dump_path);
        printf("start ts screen  record dump_path=%s\n", dump_path);
        int ret = client->startScreenRecord(left, top, right, bottom, outWidth, outHeight, framerate, bitrate,
                                            timeSecond, type, dump_path);
        if (ret != 0) {
            printf("client start ScreenRecord fail !!\n");
            ALOGE("client start ScreenRecord fail !!");
            return 0;
        }
        nowCounter++;
        int64_t endTimeUs = getNowTimesUs();
        ALOGD("finish ts screen record nowCounter = %d, duration = %lld ms", nowCounter,
              (endTimeUs - firstTimeUs) / 1000);
        printf("finish ts screen record nowCounter = %d, duration = %lld ms\n", nowCounter,
               (endTimeUs - firstTimeUs) / 1000);
        if (nowCounter >= counter)
            break;
    }
    printf("finish ts screen record count =%d\n", nowCounter);
    return 0;
}