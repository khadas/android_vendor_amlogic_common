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



#include <cutils/properties.h>

#define LOG_TAG "TSPackerTest"

#include <utils/Log.h>
#include <utils/String8.h>

#include "tspack.h"

#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <linux/videodev2.h>
#include <hardware/hardware.h>

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include "../ScreenControlService.h"
#include "../ScreenManager.h"

using namespace android;

static const char *opt_str = "hf:b:t:s:";
static void help(char *appName)
{
    printf(
        "Usage:\n"
        "  %s [-h] [-f <framerate>] [-b <bitrate>] [-t <type>] [-s <second>] [<left>  <top>  <right>  <bottom> <width> <height>]\n"
        "\n"
        "Parameters:\n"
        "  -h            : show this help\n"
        "  -f <framerate>: frame per second, unit bps, default as 30\n"
        "  -b <bitrate>  : bits per second, unit bit, default as 4000000\n"
        "  -t <type>     : select video-only(%d) or video+osd(%d), default as video+osd\n"
        "  -s <second>   : record times, unit second(s), default as 30\n"
        "  left  top  right  bottom : capture area, default as 720P (0,0,1280,720) \n"
        "  width height  : output size, default as 720P (1280X720)\n"
        "\n"
        "\n"
        "---NOTICE---\n"
        "Default save [es] files to /data/temp/ \n"
        "Pls run following commands before use:\n"
        "      mkdir -p /data/temp; chmod 777 /data/temp \n"
        , appName
        , AML_CAPTURE_VIDEO
        , AML_CAPTURE_OSD_VIDEO);
}

int main(int argc, char **argv) {
    int err;
    int isAudio;
    int dump_time = 0, video_dump_size = 0, audio_dump_size = 0;
    MediaBufferBase *tVideoBuffer, *tAudioBuffer;
    char *filename = "/data/temp/video.ts";
    int ch;
    int framerate=30, bitrate=4000000, type=AML_CAPTURE_OSD_VIDEO, timeSecond=30;
    int left=0, top=0, right=1280, bottom=720;
    int outWidth=1280, outHeight=720;
    int tmpArgIdx = 0;
    int needDumpFrame = 0;

    while ((ch = getopt(argc, argv, opt_str)) != -1) {
        switch (ch) {
        case 'h': help(argv[0]); exit(0);
        case 'f': framerate = atoi(optarg); break;
        case 'b': bitrate = atoi(optarg); break;
        case 't': type = atoi(optarg); break;
        case 's': timeSecond = atoi(optarg); break;
        default: break;
        }
    }
    tmpArgIdx = optind;

    if ((tmpArgIdx+1) < argc && (argc-tmpArgIdx) >= 6) {
        left = atoi(argv[tmpArgIdx++]);
        top = atoi(argv[tmpArgIdx++]);
        right = atoi(argv[tmpArgIdx++]);
        bottom = atoi(argv[tmpArgIdx++]);
        outWidth = atoi(argv[tmpArgIdx++]);
        outHeight = atoi(argv[tmpArgIdx++]);
    }

    needDumpFrame = framerate * timeSecond;

    printf("size     =[%dX%d]\n"
           "(left,top,right,bottom)=(%d,%d,%d,%d)\n"
           "framerate=%dbps\n"
           "bitrate  =%d\n"
           "type     =%s\n"
           "time     =%ds\n"
           "save as [%s]\n",
           outWidth, outHeight,left, top,right,bottom, framerate, bitrate,
           type==AML_CAPTURE_OSD_VIDEO?"video+osd":type==AML_CAPTURE_VIDEO?"video only":"unknow",
           timeSecond, filename);

    ProcessState::self()->startThreadPool();

    int  video_file = open(filename, O_CREAT | O_RDWR, 0666);
    if (video_file < 0) {
        printf("open file [%s] error: %s", filename, strerror(errno));
    }

    sp<TSPacker> mTSPacker = new TSPacker(outWidth, outHeight, framerate, bitrate, type, 0);
    mTSPacker->setVideoCrop(left, top,right,bottom );
    err = mTSPacker->start();
    if (err != OK) {
        printf("[%s %d] start TSPacker error, exit...\n", __FUNCTION__, __LINE__);
        return OK;
    }

    while (1) {
        tVideoBuffer = NULL;
        err = mTSPacker->read(&tVideoBuffer);

        if (err != OK) {
            usleep(1);
            continue;
        }

        dump_time++;

        err = write(video_file, tVideoBuffer->data(), tVideoBuffer->range_length());
        if (err < 0) {
            printf("write file [%s] error:%s, dump time:%d", filename, strerror(errno), dump_time);
        }
        video_dump_size += tVideoBuffer->range_length();
        printf("[%s %d] video dump_time:%d size:%d dump_size:%d\n", __FUNCTION__, __LINE__, dump_time, tVideoBuffer->range_length(), video_dump_size);

        tVideoBuffer->release();
        tVideoBuffer = NULL;

        if (dump_time > needDumpFrame)
            break;
    }

    printf("mH264Convertor stop\n");

    mTSPacker->stop();
    close(video_file);

    printf("file close\n");
    return 0;
}
