/*
 * Copyright (C) 2013 The Android Open Source Project
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
#define LOG_TAG "ScreenCatchTest"
#include <limits.h>
#include <fcntl.h>
#include <cutils/log.h>
#include "Bitmap.h"
#include "../ScreenManager.h"
#include "../ScreenControlClient.h"


using namespace android;
#define MAX_FILE_PATH_SIZE 128

enum {  // index for FILE_TYPE_STR_ARR
    SAVE_FILE_BMP = 0,
    SAVE_FILE_BIN = 1,
};
static const char* FILE_TYPE_STR_ARR[] = {
    "BMP", "BINARY"
};
static const char* CAPTURE_TYPE_STR_ARR[] = {
    "video only", "video+osd","osd only"
};

static const char *opt_str = "hlnbc:t:";

static void help(char *appName)
{
    printf(
        "Usage:\n"
        "  %s [-h] [-p/-l/-n/-b] [-c <counter>] [-t <type>] [left  top  right  bottom  outWidth  outHeight] \n"
        "\n"
        "Parameters:\n"
        "  -h  :  show this help \n"
        "  -l  :  specify loop mode \n"
        "  -n  :  no save as file \n"
        "  -b  :  save as binary file ,default as bmp file \n"
        "  -c <counter> : continually save file with counter, default as 1\n"
        "  -t <type> : set capture type:\n"
        "             0 -- video only \n"
        "             1 -- video+osd (default) \n"
        "             2 -- osd only \n"
        "  left  top  right  bottom : capture area, default as 720P (0,0,1280,720) \n"
        "  outWidth  outHeight : output size, default as 720P (1280,720) \n"
        "\n"
        "---NOTICE---\n"
        "Pls run following commands before use:\n"
        "      mkdir -p /data/temp; chmod 777 /data/temp \n"
        , appName);
}
static void argb8888_to_bmp32(void *src, void *dst, size_t size) {
    char *argb8888 = (char *)src;
    char *rgb32 = (char *)dst;
    for (int i=0; i<size; i+=4) {
        rgb32[i]   = argb8888[i+2]; //B
        rgb32[i+1] = argb8888[i+1]; //G
        rgb32[i+2] = argb8888[i];   //R
        rgb32[i+3] = argb8888[i+3]; //Alpha
    }
}

int main(int argc, char **argv) {
    int counter = 1;
    int nowCounter = 0;
    int type = 1;
    int ch;
    int tmpArgIdx = 0;
    int saveFileType = SAVE_FILE_BMP;
    int left=0, top=0, right=1280, bottom=720;
    int outWidth=1280, outHeight=720;
    char dump_dir[64] = "/data/temp";
    char dump_path[MAX_FILE_PATH_SIZE];
    bool isSaveFile = true;
    while ((ch = getopt(argc, argv, opt_str)) != -1) {
        switch (ch) {
        case 'h': help(argv[0]); exit(0);
        case 'l': counter = INT_MAX; break;
        case 'n': isSaveFile = false; break;
        case 'b': saveFileType = SAVE_FILE_BIN; break;
        case 'c': counter = atoi(optarg); break;
        case 't': type = atoi(optarg); break;
        default: break;
        }
    }
    tmpArgIdx = optind;
    if ((tmpArgIdx+1) < argc) {
        if ((argc-tmpArgIdx) == 6) {
            left = atoi(argv[tmpArgIdx++]);
            top = atoi(argv[tmpArgIdx++]);
            right = atoi(argv[tmpArgIdx++]);
            bottom = atoi(argv[tmpArgIdx++]);
            outWidth = atoi(argv[tmpArgIdx++]);
            outHeight = atoi(argv[tmpArgIdx++]);
        } else if ((argc-tmpArgIdx) == 2) {
            left = 0;
            top = 0;
            right = atoi(argv[tmpArgIdx++]);
            bottom = atoi(argv[tmpArgIdx++]);
            outWidth = right;
            outHeight = bottom;
        }
    }
    printf("type=%d(%s), file type:%s\n"
        "(left,top,right,bottom)=(%d,%d,%d,%d)\n"
        "out(width,height)=(%d,%d)\n"
        "counter=%d\n",
        type, CAPTURE_TYPE_STR_ARR[type],
        FILE_TYPE_STR_ARR[saveFileType],
        left, top, right, bottom, outWidth, outHeight, counter);
    while (1) {
        ScreenControlClient* client = ScreenControlClient::getInstance();
        void *buffer = nullptr;
        int bufferSize = 0;
        ALOGD("start screen cap ");
        printf("start screen cap \n");
        int64_t firstTimeUs = getNowTimesUs();
        int ret = client->startScreenCapBuffer(left, top, right, bottom, outWidth, outHeight, type, (void **)&buffer,&bufferSize);
        if (ret != 0 || buffer == nullptr || bufferSize <= 0 ) {
            printf("client start screencap fail !!\n");
            ALOGE("client start screencap fail !!");
            return 0;
        }
        if (isSaveFile) {
            memset (dump_path, 0, MAX_FILE_PATH_SIZE);
            if (saveFileType == SAVE_FILE_BMP) {
                snprintf(dump_path, 128, "%s/%dx%d-%d.bmp", dump_dir, outWidth, outHeight,nowCounter);
            }else if (saveFileType == SAVE_FILE_BIN) {
                snprintf(dump_path, 128, "%s/%s-%dx%d-%d.bin", dump_dir,"argb8888",
                        outWidth, outHeight, nowCounter);
            }
            printf("Try save:%s\n", dump_path);
            int32_t dump_fd = open(dump_path, O_CREAT | O_RDWR | O_TRUNC, 0644);
            if (dump_fd < 0) {
                printf("the path open %s fail,maybe don't have the dir !!!\n",dump_path);
                delete [] buffer;
                return 0;
            }
            if (saveFileType == SAVE_FILE_BMP) {
                uint8_t* rgb = new uint8_t[bufferSize];
                if (!rgb) {
                    printf("new buffer fail !!!\n");
                    close(dump_fd);
                    delete [] buffer;
                    return 0;
                }
                argb8888_to_bmp32(buffer, rgb, bufferSize);
                printf("argb8888_to_bmp32 over\n");
                Bitmap *bmp = new Bitmap((void *)rgb, outWidth, outHeight, 4);
                bmp->save(dump_fd);
                delete []rgb;
                delete bmp;
            }else if (saveFileType ==  SAVE_FILE_BIN) {
                write(dump_fd, buffer, bufferSize);
            }
        }
        delete [] buffer;
        nowCounter++;
        int64_t endTimeUs = getNowTimesUs();
        ALOGD("finish screen cap nowCounter = %d, duration = %lld ms",nowCounter,(endTimeUs-firstTimeUs)/1000);
        printf("finish screen cap nowCounter = %d, duration = %lld ms\n",nowCounter,(endTimeUs-firstTimeUs)/1000);
        if (nowCounter >= counter)
            break;

    }
    printf("finish screencap count =%d\n",nowCounter);
    return 0;

}