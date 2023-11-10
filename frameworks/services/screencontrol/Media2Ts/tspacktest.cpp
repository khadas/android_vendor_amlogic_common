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

#include <utils/Log.h>
#include "tspack.h"


using namespace android;

static const char *opt_str = "hc:f:b:t:s:p:";
const char *filename = "/data/temp/video.ts";
static void help(char *appName)
{
    printf(
        "Usage:\n"
        "  %s [-h] [-c <counter>] [-f <framerate>] [-b <bitrate>] [-t <type>] [-s <second>] [<left>  <top>  <right>  <bottom> <width> <height>]\n"
        "\n"
        "Parameters:\n"
        "  -h            : show this help\n"
        "  -c <counter> : continually save file with counter, default as 1\n"
        "  -f <framerate>: frame per second, unit bps, default as 30\n"
        "  -b <bitrate>  : bits per second, unit bit, default as 4000000\n"
        "  -t <type>     : select video-only(%d) or video+osd(%d), default as video+osd\n"
        "  -s <second>   : record times, unit second(s), default as 30\n"
        "  -p <dir> : the dir to save the file, default as /data/temp\n"
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
    int ch;
    int framerate=30, bitrate=4000000, type=AML_CAPTURE_OSD_VIDEO, timeSecond=30;
    int left=0, top=0, right=1280, bottom=720;
    int outWidth=1280, outHeight=720;
    int tmpArgIdx = 0;
    int64_t mFirstPts = 0;
    int counter = 1;
    int framecount = 0;
    char dump_path[128];
    char dump_dir[64] = "/data/temp";
    while ((ch = getopt(argc, argv, opt_str)) != -1) {
        switch (ch) {
        case 'h': help(argv[0]); exit(0);
        case 'c': counter = atoi(optarg); break;
        case 'f': framerate = atoi(optarg); break;
        case 'b': bitrate = atoi(optarg); break;
        case 't': type = atoi(optarg); break;
        case 's': timeSecond = atoi(optarg); break;
        case 'p': memset(dump_dir, 0, 64);
                  if (strlen(optarg) <= 64)
                    memcpy(dump_dir,optarg,strlen(optarg));
                  break;
        default: break;
        }
    }
    tmpArgIdx = optind;

    if ((tmpArgIdx+1) < argc && (argc-tmpArgIdx) >= 2) {
        if ((argc-tmpArgIdx) == 2) {
            outWidth = atoi(argv[tmpArgIdx++]);
            outHeight = atoi(argv[tmpArgIdx++]);
            left = 0;
            top = 0;
            right = outWidth;
            bottom = outHeight;
        }else {
            left = atoi(argv[tmpArgIdx++]);
            top = atoi(argv[tmpArgIdx++]);
            right = atoi(argv[tmpArgIdx++]);
            bottom = atoi(argv[tmpArgIdx++]);
            outWidth = atoi(argv[tmpArgIdx++]);
            outHeight = atoi(argv[tmpArgIdx++]);
        }

    }


    printf("size     =[%dX%d]\n"
           "(left,top,right,bottom)=(%d,%d,%d,%d)\n"
           "framerate=%dbps\n"
           "bitrate  =%d\n"
           "type     =%s\n"
           "time     =%ds\n"
           "counter     =%d\n",
           outWidth, outHeight,left, top,right,bottom, framerate, bitrate,
           type==AML_CAPTURE_OSD_VIDEO?"video+osd":type==AML_CAPTURE_VIDEO?"video only":"unknown",
           timeSecond, counter);
    for (int i = 0; i < counter; i++) {
        framecount++;
        mFirstPts = 0;
        std::unique_ptr<TSPacker> tspacker = std::make_unique<TSPacker>();
        auto parmeter = std::make_unique<ESConvertorParmeter>();
        parmeter->size = std::make_unique<Size>(outWidth,outHeight);
        parmeter->area = std::make_unique<Area>(left,top,right,bottom);
        parmeter->source_type = type;
        parmeter->frame_rate = framerate;
        parmeter->bit_rate_ = bitrate;

        if (!tspacker->start(parmeter)) {
            printf("the tspacker start fail!!\n");
            return 0;
        }
        memset (dump_path, 0, 128);
        snprintf(dump_path, 128, "%s/%dx%d-%d.ts",dump_dir,outWidth, outHeight, framecount);
        printf("Try save:%s\n", dump_path);
        /* coverity[path_manipulation_sink:SUPPRESS] */
        int32_t fd = open(dump_path, O_CREAT | O_RDWR, 0666);
        if (fd < 0 )
            return 0;

        while (1) {
            uint8_t * buffer = nullptr;
            int32_t size = 0;
            int64_t pts = 0;
            bool ret = tspacker->readBuffer(&buffer,&size,&pts);
            if (!ret || !buffer || size <= 0 || pts <= 0) {
                usleep(5 * 1000);//5ms
                continue;
            }
            if (mFirstPts == 0)
                mFirstPts = pts;
            int64_t diff = timeSecond * 1000 * 1000;
            int64_t diffPts = pts - mFirstPts;
            write(fd, buffer, size);
            delete []buffer;
            printf("[%s %d] video dump_size = %d,pts = %lld,diffPts=%lld\n", __FUNCTION__, __LINE__,size,pts,diffPts);
            if (diffPts >= diff)
                break;

        }
        tspacker->stop();
        close(fd);
        printf("TSPackerTest stop count =%d \n",framecount);
    }


    printf("TSPackerTest finish\n");
    return 0;
}
