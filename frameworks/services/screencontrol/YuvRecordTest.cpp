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
#define LOG_TAG "YuvRecordTest"
#include <limits.h>
#include <fcntl.h>
#include <cutils/log.h>
#include<time.h>
#include "ScreenControlClient.h"

using namespace android;
#define MAX_FILE_PATH_SIZE 128

inline int64_t getNowTimesUs() {
    struct timespec now;
    clock_gettime(CLOCK_BOOTTIME, &now);
    int64_t now_time = now.tv_sec * 1000 * 1000 + now.tv_nsec / 1000;
    return now_time;
}

static const char* CAPTURE_TYPE_STR_ARR[] = {
    "video only", "video+osd","osd only"
};
static const char *opt_str = "hlnf:t:s:c:";
static void help(char *appName)
{
    printf(
        "Usage:\n"
        "  %s [-h/-n] [-f <framerate>] [-t <type>] [-s <second>] [<left>  <top>  <right>  <bottom> <width> <height>]\n"
        "\n"
        "Parameters:\n"
        "  -h            : show this help\n"
        "  -n            :  no save as file \n"
        "  -l            :  specify loop mode \n"
        "  -f <framerate>: frame per second, unit bps, default as 30\n"
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
        "      mkdir -p /data/temp; chmod 777 /data/temp \n"
        , appName);

}

class YuvRecorder : public ScreenControlClient::YuvRecordCallback {
public:
    YuvRecorder(char *fileName) {
        mFirstPts = 0;
        mLastPts = 0;
        mFileName = fileName;
        if (mFileName != nullptr) {
            fd = open(mFileName, O_CREAT | O_RDWR, 0666);
        }
        client = ScreenControlClient::getInstance();
    }
    ~YuvRecorder() {
        if (fd > 0)
            close(fd);
    }
    bool start(int left, int top,int right,int bottom,int width,int height,int source_type,
                int32_t frame_rate) {
        int ret = client->startYuvScreenRecord(width,height, frame_rate,source_type);
        client->setYuvCallback(this);
        return ret == 0?true:false;
    }
    void stop() {
        client->forceStop();
    }
    void onYuvDataArouse(void *data, int32_t size) {
        int64_t pts = getNowTimesUs();
        printf("onYuvDataArouse mFirstPts = %lld,pts =%lld,diff =%lld,pid=%d\n",mFirstPts,pts,(pts-mFirstPts),getpid());
        if (mFirstPts == 0)
            mFirstPts = pts;
        if (fd > 0) {
            write(fd, data, size);
        }
        mLastPts = pts;
    }
    int64_t getLastPts(){return mLastPts;}
    int64_t getFirstPts(){return mFirstPts;}
    int64_t getDiffPts(){return (mFirstPts == 0||mLastPts == 0)?0:(mLastPts - mFirstPts);}
private:
    char *mFileName;
    int64_t mFirstPts;
    int64_t mLastPts;
    int32_t fd = -1;
    ScreenControlClient* client;

};

int main(int argc, char **argv) {

    int counter = 1;
    int nowCounter = 0;
    int type = 1;
    int ch;
    int tmpArgIdx = 0;
    int left=0, top=0, right=1280, bottom=720;
    int timeSecond = 30;
    int framerate = 30;
    int outWidth=1280, outHeight=720;
    char dump_dir[64] = "/data/temp";
    char dump_path[MAX_FILE_PATH_SIZE];
    bool isSaveFile = true;
    while ((ch = getopt(argc, argv, opt_str)) != -1) {
        switch (ch) {
        case 'h': help(argv[0]); exit(0);
        case 'n': isSaveFile = false; break;
        case 'l': counter = INT_MAX; break;
        case 'f': framerate = atoi(optarg); break;
        case 'c': counter = atoi(optarg); break;
        case 's': timeSecond = atoi(optarg); break;
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
    printf("size     =[%dX%d]\n"
           "(left,top,right,bottom)=(%d,%d,%d,%d)\n"
           "framerate=%dbps\n"
           "type     =%s\n"
           "isSaveFile     =%d\n"
           "counter     =%d\n"
           "time     =%ds\n",
           outWidth, outHeight,left, top,right,bottom, framerate,
           CAPTURE_TYPE_STR_ARR[type],
           isSaveFile, counter, timeSecond);
    while (1) {
        char * realFile = nullptr;
        memset (dump_path, 0, 128);
        if (isSaveFile) {
            snprintf(dump_path, 128, "%s/%dx%d-%d.es",dump_dir,outWidth, outHeight, nowCounter);
            printf("Try save:%s\n", dump_path);
            realFile = dump_path;
        }
        sp<YuvRecorder> recorder= new YuvRecorder(realFile);
        if (!recorder->start(left,top,right,bottom,outWidth, outHeight,type,framerate)) {
            printf("YuvRecorder start fail\n");
            return 0;
        }
        int64_t diff = (int64_t)timeSecond * 1000 * 1000;
        printf("YuvRecorder diff=%lld\n",diff);
        while (1) {
            int64_t diffpts = recorder->getDiffPts();
            int64_t firstPts = recorder->getFirstPts();
            int64_t lastPts = recorder->getLastPts();
            if (diffpts >= diff ) {
                printf("YuvRecorder firstPts =%lld,lastPts=%lld,diffpts=%lld,pid=%d\n",firstPts,lastPts,diffpts,getpid());
                break;
            }
            usleep(5*1000);//5ms
        }
        recorder->stop();
        nowCounter++;
        printf("YuvRecorder stop nowCounter=%d\n",nowCounter);
        if (nowCounter >= counter)
            break;
    }
    printf("finish yuv screen record count =%d\n",nowCounter);

    return 0;
}
