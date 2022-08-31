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

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "Bitmap.h"

//#define NDEBUG  //disable assert

#define BITMAT_FILE_TYPE_MAGIC 0x4d42 //bm

namespace android {

Bitmap::Bitmap(int inFd) {
    initPriv();
    assert(inFd >= 0);
    assert(readHeader(inFd));
    assert(readData(fd));
}

Bitmap::Bitmap(FILE *inFile) {
    initPriv();
    assert(inFile != NULL);
    assert(readHeader(fileno(inFile)));
    assert(readData(fd));
}

Bitmap::Bitmap(const char *inFilePath) {
    initPriv();
    assert(inFilePath != NULL);
    int fd = open(inFilePath, O_RDONLY, 0666);
    if (fd >= 0) {
        assert(readHeader(fd));
        assert(readData(fd));
        close(fd);
    }
}

bool Bitmap::readHeader(int fd) {
    if (read(fd, &mBfh, sizeof(mBfh)) <= 0) {
        return false;
    }
    if (BITMAT_FILE_TYPE_MAGIC != mBfh.bfType) {
        return false;
    }
    if (read(fd, &mBih, sizeof(mBih)) <= 0) {
        return false;
    }

    return true;
}

bool Bitmap::readData(int fd) {
    int dataLen = abs(mBih.biWidth * mBih.biHeight) * mBih.biBitCount / 8;
    int ret = -1;
    int readedLen = 0;
    if (mData == NULL) {
        mData = calloc(1, dataLen);
        if (mData == NULL) {
            return false;
        }
        isDataAlloc = true;
    }
    do {
        ret = read(fd, mData, dataLen-readedLen);
        if (ret > 0) {
            readedLen += ret;
        }
    } while (readedLen < dataLen);
    return (readedLen >= dataLen);
}

void Bitmap::initPriv() {
    memset(&mBfh, 0, sizeof(mBfh));
    memset(&mBih, 0, sizeof(mBih));
    mData = NULL;
    isDataAlloc = false;
}

Bitmap::Bitmap(void *rgb, int width, int height, int bytePerPixel) {
    initPriv();
    // fill BmpFileHeader
    mBfh.bfType = BITMAT_FILE_TYPE_MAGIC;
    mBfh.bfReserved1 = 0;
    mBfh.bfReserved2 = 0;
    mBfh.bfSize = sizeof(BmpFileHeader) + sizeof(BmpInfoHeader) +
                 width * height * bytePerPixel;
    mBfh.bfOffBits = sizeof(BmpFileHeader) + sizeof(BmpInfoHeader);

    // fill BmpInfoHeader
    mBih.biSize = sizeof(BmpInfoHeader);
    mBih.biWidth = width;
    mBih.biHeight = -height;
    mBih.biPlanes = 1;
    mBih.biBitCount = 8 * bytePerPixel;
    mBih.biCompression = 0;
    mBih.biSizeImage = 0;
    mBih.biXPelsPerMeter = 0;
    mBih.biYPelsPerMeter = 0;
    mBih.biClrUsed = 0;
    mBih.biClrImportant = 0;

    // save rgb pointer
    this->mData = rgb;
}

Bitmap::~Bitmap() {
    if (isDataAlloc && (mData != NULL)) {
        free (mData);
    }
}

bool Bitmap::getHeader(BmpFileHeader &bfh, BmpInfoHeader &bih) {
    memcpy(&bfh, &mBfh, sizeof(mBfh));
    memcpy(&bih, &mBih, sizeof(mBih));
    return true;
}

bool Bitmap::getData(void *dst, int dstLength) {
    if (dst == NULL || dstLength <= 0) {
        return false;
    }
    int dataLen = abs(mBih.biWidth * mBih.biHeight) * mBih.biBitCount / 8;
    memcpy(dst, mData, (dstLength<dataLen?dstLength:dataLen));
    return true;
}

bool Bitmap::save(int fd) {
    if (fd >= 0) {
        write(fd, &mBfh, sizeof(mBfh));
        write(fd, &mBih, sizeof(mBih));
        write(fd, mData, abs(mBih.biWidth * mBih.biHeight) * mBih.biBitCount / 8);
        return true;
    }
    return false;
}

bool Bitmap::save(FILE *file) {
    int fd = fileno(file);
    return save(fd);
}

bool Bitmap::save(const char *filepath) {
    bool success = false;
    int fd = open(filepath, O_CREAT|O_RDWR, 0666);
    if (fd < 0) {
        return false;
    }
    success = save(fd);
    close(fd);
    return success;
}

} // end of namespace android

