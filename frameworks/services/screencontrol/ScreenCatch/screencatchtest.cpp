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
#define LOG_TAG "screencatch"

#include <cutils/log.h>
#include "ScreenCatch.h"
#include "Bitmap.h"

using namespace android;


#define MAX_FILE_PATH_SIZE 128

/*****************************************************************************/
enum {
    SAVE_FILE_BMP,
    SAVE_FILE_BIN,
    SAVE_FILE_UNKNOWN
};
static const char* FILE_TYPE_STR_ARR[] = {
    "BMP", "BINARY"
};

static const char *opt_str = "hnmbc:t:p:";
static void help(char *appName)
{
    printf(
        "Usage:\n"
        "  %s [-h] [-m/-b/-n] [-c <counter>] [-t <type>] [left  top  right  bottom  outWidth  outHeight] \n"
        "\n"
        "Parameters:\n"
        "  -h  :  show this help \n"
        "  -m  :  save as bmp file (default) \n"
        "  -n  :  no save as file \n"
        "  -b  :  save as binary file \n"
        "  -c <counter> : continually save file with counter, default as 1\n"
        "  -t <type> : set capture type:\n"
        "             0 -- video only \n"
        "             1 -- video+osd (default) \n"
        "  -p <dir> : the dir to save the file, default as /data/temp\n"
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


int main(int argc, char **argv)
{
    using namespace android;

    uint32_t type = 1;
    int framecount = 0;
    size_t size = 0;
    int ch;
    int saveFileType = SAVE_FILE_BMP;
    int left=0, top=0, right=1280, bottom=720;
    int outWidth=1280, outHeight=720;
    int tmpArgIdx = 0;
    int counter = 1;
    char dump_path[128];
    char dump_dir[64] = "/data/temp";
    bool isSaveFile = true;




    while ((ch = getopt(argc, argv, opt_str)) != -1) {
        switch (ch) {
        case 'h': help(argv[0]); exit(0);
        case 'm': saveFileType = SAVE_FILE_BMP; break;
        case 'n': isSaveFile = false;break;
        case 'b': saveFileType = SAVE_FILE_BIN; break;
        case 'c': counter = atoi(optarg); break;
        case 't': type = atoi(optarg); break;
        case 'p': memset(dump_dir, 0, 64);
                  if (strlen(optarg) <= 64)
                        memcpy(dump_dir,optarg,strlen(optarg));
                  break;
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
    printf("type=%d(%s), file type:%s\n"
           "(left,top,right,bottom)=(%d,%d,%d,%d)\n"
           "out(width,height)=(%d,%d)\n"
           "isSaveFile     =%d\n"
           "counter=%d\n",
        type, type==0?"video only":"video+osd",
        FILE_TYPE_STR_ARR[saveFileType],
        left, top, right, bottom, outWidth, outHeight,isSaveFile,counter);


    for (int i = 0; i < counter; i++) {
        framecount++;
        std::unique_ptr<ScreenCatch> capture = std::make_unique<ScreenCatch>();
        auto size = std::make_unique<Size>(outWidth,outHeight);
        auto area = std::make_unique<Area>(left,top,right,bottom);
        auto parmeter = std::make_unique<InputParmeter>();
        parmeter->size = std::move(size);
        parmeter->area = std::move(area);
        parmeter->source_type = type;
        bool ret = capture->start(parmeter);
        if (!ret) {
            printf("screencath start fail !!!\n");
            return 0;
        }
        memset (dump_path, 0, MAX_FILE_PATH_SIZE);
        if (saveFileType == SAVE_FILE_BMP) {
            snprintf(dump_path, 128, "%s/%d.bmp", dump_dir, framecount);
        }else if (saveFileType == SAVE_FILE_BIN) {
            snprintf(dump_path, 128, "%s/%s-%dx%d-%d.bin", dump_dir,"argb8888",
                    outWidth, outHeight, framecount);
        }
        printf("Try save:%s\n", dump_path);
        /* coverity[path_manipulation_sink:SUPPRESS] */
        int32_t dump_fd = open(dump_path, O_CREAT | O_RDWR | O_TRUNC, 0644);
        if (dump_fd < 0) {
            printf("the path open %s fail,maybe don't have the dir !!!\n",dump_path);
            capture->stop();
            return 0;
        }
        if (outWidth <= 0 || outHeight <= 0 ) {
            printf("the outWidth or  outHeight is not legal !!\n");
            capture->stop();
            close(dump_fd);
            return 0;
        }
        uint8_t* buffer = new uint8_t[outWidth * outHeight * 4];
        if (!buffer) {
            printf("new buffer fail !!\n");
            capture->stop();
            close(dump_fd);
            return 0;
        }
        int32_t buffer_size = 0;
        while (1) {
            bool ret = capture->readBuffer(buffer,&buffer_size);
            if (ret)
                break;
            usleep(5 * 1000);//5ms
        }
        printf("read buffer from screencatch buffer_size=%d\n",buffer_size);
        if ( buffer_size <= 0  || !isSaveFile) {
            delete []buffer;
            capture->stop();
            close(dump_fd);
            continue;
        }
        if (saveFileType == SAVE_FILE_BMP) {
            uint8_t* rgb = new uint8_t[buffer_size];
            if (!rgb) {
                printf("new buffer fail !!!\n");
                capture->stop();
                return 0;
            }
            argb8888_to_bmp32(buffer, rgb, buffer_size);
            printf("argb8888_to_bmp32 over\n");
            Bitmap *bmp = new Bitmap((void *)rgb, outWidth, outHeight, 4);
            bmp->save(dump_fd);
            delete []rgb;
            delete bmp;
        }else if (saveFileType ==  SAVE_FILE_BIN) {
            write(dump_fd, buffer, buffer_size);
        }
        delete []buffer;
        capture->stop();
        close(dump_fd);
    }
    ALOGI("[%s %d] screencap finish", __FUNCTION__, __LINE__);
    return 1;
}
/*****************************************************************************/
