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

#define LOG_TAG "MediaConvertorTest"

#include <utils/Log.h>

#include "esconvertor.h"


using namespace android;

const char *filename = "/data/temp/video.es";

static const char *opt_str = "hf:b:t:s:";
static void help(char *appName)
{
    printf(
        "Usage:\n"
        "  %s [-h] [-f <framerate>] [-b <bitrate>] [-t <type>] [-s <second>] [<left>  <top>  <right>  <bottom>  <width> <height>]\n"
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

class EsConvertorTest : public ESConvertor::ESConvertorCallback {
public:

    EsConvertorTest() {
        mFirstPts = 0;
        mLastPts = 0;
        fd = -1;
    }

    ~EsConvertorTest() {
        if (fd > 0)
            close(fd);
    }


    bool start(int left, int top,int right,int bottom,int width,int height,int source_type,int32_t frame_rate,int32_t bit_rate) {
        convertor = std::make_unique<ESConvertor>();
        auto parmeter = std::make_unique<ESConvertorParmeter>();
        parmeter->size = std::make_unique<Size>(width,height);
        parmeter->area = std::make_unique<Area>(left,top,right,bottom);
        parmeter->source_type = source_type;
        parmeter->frame_rate = frame_rate;
        parmeter->bit_rate_ = bit_rate;
        fd = open(filename, O_CREAT | O_RDWR, 0666);
        if (fd <= 0 )
            return false;

        return convertor->start(parmeter,this);


    }
    bool stop() {
        if (fd > 0) {
            close(fd);
            fd = -1;
        }
        return convertor?convertor->stop():false;

    }
    void onEsBufferAvailable(void* const data, int32_t size, int32_t frame_type, int64_t pts) {
        printf("onEsBufferAvailable frame_type=%d,mFirstPts = %ld,pts =%ld,diff =%ld\n",frame_type,mFirstPts,pts,(pts-mFirstPts));
        if (mFirstPts == 0)
            mFirstPts = pts;
        if (fd > 0) {
            ALOGD("write in");
            write(fd, data, size);
            ALOGD("write out");

        }
        mLastPts = pts;
    }
    int64_t getLastPts(){return mLastPts;}
    int64_t getFirstPts(){return mFirstPts;}
    int64_t getDiffPts(){return (mFirstPts == 0||mLastPts == 0)?0:(mLastPts - mFirstPts);}

private:
    int64_t mFirstPts;
    int64_t mLastPts;
    int32_t fd;
    std::unique_ptr<ESConvertor> convertor;
};


int main(int argc, char **argv) {
    int err;
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

    needDumpFrame = framerate * timeSecond;

    printf("size     =[%dX%d]\n"
           "framerate=%dbps\n"
           "left  =%d\n"
           "top  =%d\n"
           "right  =%d\n"
           "bottom  =%d\n"
           "bitrate  =%d\n"
           "type     =%s\n"
           "time     =%ds\n"
           "save as [%s]\n",
           outWidth, outHeight, framerate, left,top,right,bottom,bitrate,
           type==AML_CAPTURE_OSD_VIDEO?"video+osd":(type==AML_CAPTURE_VIDEO?"video only":"unknown"),
           timeSecond, filename);

    auto test = std::make_unique<EsConvertorTest>();
    if (!test->start(left,top,right,bottom,outWidth, outHeight,type,framerate,bitrate)) {
        printf("EsConvertorTest start fail\n");
        return 0;
    }
    int64_t diff = timeSecond * 1000 * 1000;
    printf("EsConvertorTest diff=%ld\n",diff);
    while (1) {
        int64_t diffpts = test->getDiffPts();
        int64_t firstPts = test->getFirstPts();
        int64_t lastPts = test->getLastPts();
        if (diffpts >= diff ) {
            printf("EsConvertorTest firstPts =%ld,lastPts=%ld,diffpts=%ld\n",firstPts,lastPts,diffpts);
            break;
        }
        usleep(5*1000);//5ms
    }
    test->stop();
    printf("mH264Convertor stop\n");
    return 0;
}
