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

#include <errno.h>
#include <cutils/log.h>
#include <cutils/atomic.h>

//#include <media/stagefright/MediaSource.h>
//#include <media/stagefright/MediaBuffer.h>
#include <OMX_IVCommon.h>

#include <utils/List.h>
#include <utils/RefBase.h>
#include <utils/threads.h>

#include <media/stagefright/MetaData.h>

#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <ui/PixelFormat.h>
//#include <ui/DisplayInfo.h>

#include <system/graphics.h>

//#include <SkBitmap.h>
//#include <SkDocument.h>
//#include <SkStream.h>

// TODO: Fix Skia.

#include "ScreenCatch.h"
#include "Bitmap.h"
#include "../ScreenManager.h"

using namespace android;

/*****************************************************************************/
enum {  // index for FILE_TYPE_STR_ARR
    SAVE_FILE_PNG = 0,
    SAVE_FILE_JPEG,
    SAVE_FILE_BMP,
    SAVE_FILE_BIN,
};
static const char* FILE_TYPE_STR_ARR[] = {
    "PNG","JPEG", "BMP", "BINARY"
};

static const char *opt_str = "hpjmbc:t:";
static void help(char *appName)
{
    printf(
        "Usage:\n"
        "  %s [-h] [-p/-j/-m/-b] [-c <counter>] [-t <type>] [left  top  right  bottom  outWidth  outHeight] \n"
        "\n"
        "Parameters:\n"
        "  -h  :  show this help \n"
        "  -p  :  save as png file (default) \n"
        "  -j  :  save as jpeg file \n"
        "  -m  :  save as bmp file \n"
        "  -b  :  save as binary file \n"
        "  -c <counter> : continually save file with counter, default as 1\n"
        "  -t <type> : set capture type:\n"
        "             0 -- video only \n"
        "             1 -- video+osd (default) \n"
        "  left  top  right  bottom : capture area, default as 720P (0,0,1280,720) \n"
        "  outWidth  outHeight : output size, default as 720P (1280,720) \n"
        "\n"
        "---NOTICE---\n"
        "Pls run following commands before use:\n"
        "      mkdir -p /data/temp; chmod 777 /data/temp \n"
        , appName);
}

#if 0
static SkColorType flinger2skia(PixelFormat f) {
    switch (f) {
        case PIXEL_FORMAT_RGB_565:
            return kRGB_565_SkColorType;
        default:
            return kN32_SkColorType;
    }
}
#endif

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

static void rgb888_to_bmp24(void *src, void *dst, size_t size) {
    char *rgb888 = (char *)src;
    char *rgb24 = (char *)dst;
    for (int i=0; i<size; i+=3) {
        rgb24[i] = rgb888[i+2];   //B
        rgb24[i+1] = rgb888[i+1]; //G
        rgb24[i+2] = rgb888[i];   //R
    }
}

static int getColorFormatSize(int32_t clrFormat, int32_t width, int32_t height) {
    int size = 0;
    switch (clrFormat) {
    case OMX_COLOR_Format32bitARGB8888: size = width * height * 4; break;
    case OMX_COLOR_Format24bitRGB888: size = width * height * 3; break;
    case OMX_COLOR_FormatYUV420SemiPlanar: size = width * height * 3 / 2; break;
    default: break;
    }
    return size;
}

int main(int argc, char **argv)
{
    using namespace android;

    status_t ret = NO_ERROR;
    int status;
    int dumpfd;
    uint32_t type = 1;
    int framecount = 0;
    int32_t clrFormat = OMX_COLOR_Format32bitARGB8888;
    size_t size = 0;
    int ch;
    int saveFileType = SAVE_FILE_PNG;
    int left=0, top=0, right=1280, bottom=720;
    int outWidth=1280, outHeight=720;
    int tmpArgIdx = 0;
    int counter = 1;

    ScreenCatch* mScreenCatch;

    while ((ch = getopt(argc, argv, opt_str)) != -1) {
        switch (ch) {
        case 'h': help(argv[0]); exit(0);
        case 'p': saveFileType = SAVE_FILE_PNG; break;
        case 'j': saveFileType = SAVE_FILE_JPEG; break;
        case 'm': saveFileType = SAVE_FILE_BMP; break;
        case 'b': saveFileType = SAVE_FILE_BIN; break;
        case 'c': counter = atoi(optarg); break;
        case 't': type = atoi(optarg); break;
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
           "counter=%d\n",
        type, type==0?"video only":"video+osd",
        FILE_TYPE_STR_ARR[saveFileType],
        left, top, right, bottom, outWidth, outHeight, counter);
    size = getColorFormatSize(clrFormat, outWidth, outHeight);
    sp<MemoryHeapBase> memoryBase(new MemoryHeapBase(size, 0, "screen-capture"));
    void* const base = memoryBase->getBase();

    if (base != MAP_FAILED) {
        fprintf(stderr, "start screencap\n");
        mScreenCatch = new ScreenCatch(outWidth, outHeight, 32, type);
        mScreenCatch->setVideoCrop(left, top, right, bottom);

        MetaData* pMeta;
        pMeta = new MetaData();
        pMeta->setInt32(kKeyColorFormat, clrFormat);
        mScreenCatch->start(pMeta);
        char dump_path[128];
        char dump_dir[64] = "/data/temp";

        MediaBuffer *buffer;
        while (framecount < counter) {
            status = mScreenCatch->read(&buffer);
            if (status != OK) {
                usleep(100);
                continue;
            }

            framecount++;
            if (SAVE_FILE_PNG == saveFileType) {
                sprintf(dump_path, "%s/%d.png", dump_dir, framecount);
            } else if (SAVE_FILE_JPEG == saveFileType) {
                sprintf(dump_path, "%s/%d.jpeg", dump_dir, framecount);
            } else if (SAVE_FILE_BMP == saveFileType) {
                sprintf(dump_path, "%s/%d.bmp", dump_dir, framecount);
            } else {
                sprintf(dump_path, "%s/%s-%dx%d-%d.bin", dump_dir,
                    (clrFormat==OMX_COLOR_Format32bitARGB8888?"argb8888":
                        (clrFormat==OMX_COLOR_Format24bitRGB888?"rgb888":
                        (clrFormat==OMX_COLOR_FormatYUV420SemiPlanar?"yuv420":"unknow"))),
                    outWidth, outHeight, framecount);
            }
            printf("Try save:%s, size=%d\n", dump_path, buffer->size());

            dumpfd = open(dump_path, O_CREAT | O_RDWR | O_TRUNC, 0644);

            if (SAVE_FILE_PNG == saveFileType || SAVE_FILE_JPEG == saveFileType) {
#if 0
                memcpy(base, buffer->data(), buffer->size());
                const SkImageInfo info = SkImageInfo::Make(outWidth, outHeight, flinger2skia(f), kPremul_SkAlphaType, nullptr);
                SkPixmap pixmap(info, base, outWidth * bytesPerPixel(f));
                struct FDWStream final : public SkWStream {
                    size_t fBytesWritten = 0;
                    int fFd;
                    FDWStream(int f) : fFd(f) {}
                    size_t bytesWritten() const override {
                        return fBytesWritten;
                    }
                    bool write(const void* buffer, size_t size) override {
                        fBytesWritten += size;
                        return size == 0 || ::write(fFd, buffer, size) > 0;
                    }
                } fdStream(dumpfd);
                if (SAVE_FILE_PNG == saveFileType) {
                    (void)SkEncodeImage(&fdStream, pixmap, SkEncodedImageFormat::kPNG, 100);
                } else {
                    (void)SkEncodeImage(&fdStream, pixmap, SkEncodedImageFormat::kJPEG, 100);
                }
#else
                //TODO: save as jpeg/png file
                fprintf(stderr, "TYPE: [%s] - Not Support!!!\n", saveFileType==SAVE_FILE_JPEG?"JPG/JPEG":"PNG");
#endif
            } else if (SAVE_FILE_BMP == saveFileType) {
                //save bmp
                int bytePerPixel = 4;
                bool success = false;
                void *rgb = calloc(1, buffer->size());
                if (rgb != NULL) {
                    switch (clrFormat) {
                    case OMX_COLOR_Format32bitARGB8888:
                        /*
                         * argb8888 need to change Red and Blue order to adapter to BMP rgb32
                         * RGB32    : B  G  R  A
                         * ARGB8888 : R  G  B  A
                         *          <low......high>
                         */
                        argb8888_to_bmp32(buffer->data(), rgb, buffer->size());
                        bytePerPixel = 4;
                        success = true;
                        break;
                    case OMX_COLOR_Format24bitRGB888:
                        /*
                         * rgb888 need to change Red and Blue order to adapter to BMP rgb24
                         * RGB24  : B  G  R
                         * RGB888 : R  G  B
                         *        <low...high>
                         */
                        rgb888_to_bmp24(buffer->data(), rgb, buffer->size());
                        bytePerPixel = 3;
                        success = true;
                        break;
                    default: success = false; break;
                    }
                    if (success) {
                        Bitmap *bmp = new Bitmap((void *)rgb, outWidth, outHeight, bytePerPixel);
                        success = bmp->save(dumpfd);
                        delete bmp;
                    }
                    free(rgb);
                }
                printf("Save: %s %s\n", dump_path, success?"Ok":"Fail");
            } else {
                //save binary
                write(dumpfd, buffer->data(), buffer->size());
            }
            fprintf(stderr, "Transform finish!\n");

            close(dumpfd);
            buffer->release();
            buffer = NULL;
        }

        memoryBase.clear();
        mScreenCatch->stop();
        pMeta->clear();
        delete mScreenCatch;
    } else {
        ret = UNKNOWN_ERROR;
    }
    ALOGI("[%s %d] screencap finish", __FUNCTION__, __LINE__);
    return ret;
}
/*****************************************************************************/
