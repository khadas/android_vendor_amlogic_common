/*
 * Copyright (C) 2011 The Android Open Source Project
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

#ifndef ANDROID_DROIDLOGIC_BITMAP_H
#define ANDROID_DROIDLOGIC_BITMAP_H
#include <stdio.h>
namespace android {

#pragma pack(push, 1)
    typedef struct BmpFileHeader {       /**** BMP file header structure ****/
        unsigned short bfType;           /* File type: bm(0x4d42) */
        unsigned int   bfSize;           /* Size of file */
        unsigned short bfReserved1;      /* Reserved */
        unsigned short bfReserved2;      /* ... */
        unsigned int   bfOffBits;        /* Offset to bitmap data */
    } BmpFileHeader;

    typedef struct BmpInfoHeader {       /**** BMP file info structure ****/
        unsigned int   biSize;           /* Size of info header */
        int            biWidth;          /* Width of image */
        int            biHeight;         /* Height of image */
        unsigned short biPlanes;         /* Number of color planes */
        unsigned short biBitCount;       /* Number of bits per pixel */
        unsigned int   biCompression;    /* Type of compression to use */
        unsigned int   biSizeImage;      /* Size of image data */
        int            biXPelsPerMeter;  /* X pixels per meter */
        int            biYPelsPerMeter;  /* Y pixels per meter */
        unsigned int   biClrUsed;        /* Number of colors used */
        unsigned int   biClrImportant;   /* Number of important colors */
    } BmpInfoHeader;
#pragma pack(pop)

class Bitmap {
public:
    // create from file
    Bitmap(int inFd);
    Bitmap(FILE *inFile);
    Bitmap(const char *inFilePath);
    // create from buffer
    Bitmap(void *rgb, int width, int height, int bytePerPixel);
    ~Bitmap();

    bool getHeader(BmpFileHeader &bfh, BmpInfoHeader &bih);
    bool getData(void *dst, int dstLength);

    // save to file
    bool save(int outFd);
    bool save(FILE *outFile);
    bool save(const char *outFilePath);

private:
    void initPriv();
    bool readHeader(int fd);
    bool readData(int fd);

    BmpFileHeader mBfh;
    BmpInfoHeader mBih;
    void *mData;
    bool isDataAlloc;
};

}; // namespace android

#endif // ANDROID_DROIDLOGIC_BITMAP_H

