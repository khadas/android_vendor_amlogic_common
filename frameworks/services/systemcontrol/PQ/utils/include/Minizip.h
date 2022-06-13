/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 * Author:  Shoufu Zhao <shoufu.zhao@amlogic.com>
 */

#ifndef __MINI_ZIP_H__
#define __MINI_ZIP_H__

#include "CPQLog.h"

#define CC_MAX_UNCOMPRESS_FILE_BUF_SIZE                (0x600000)  //6MB

#define CC_TAG_STR_MAX_LEN                             (16)
#define CS_TAG_STR_CONTENT                             "Minizip format"

#define CC_UNCOMPRESS_CHECK_OK                         (0)
#define CC_UNCOMPRESS_CHECK_ERROR_SRC_FILE             (-1)
#define CC_UNCOMPRESS_CHECK_ERROR_DST_FILE             (-2)

// total 64 bytes
struct minizip_comp_header_s {
    unsigned int self_crc32;
    unsigned int src_crc32;
    unsigned int self_len;
    unsigned int src_len;
    unsigned char tag_str[CC_TAG_STR_MAX_LEN];
    unsigned int version;
    unsigned char rev[28];
};

class Minizip
{
public:
    Minizip();
    ~Minizip();

    int compress_file(const char *dst_name, const char *src_name);
    int uncompress_file(const char *dst_name, const char *src_name);
    int setCompressHeadFlag(int flag_val);
    int getCompressHeadFlag();
    int setMaxUnCompressFileSize(int max_size);
    int getMaxUnCompressFileSize();
    void freeAll();
    int checkCompressHeader(struct minizip_comp_header_s *ptrHead, const char *src_name);
    int checkCompressHeader(struct minizip_comp_header_s *tmpHead, unsigned char* data_buf, int data_len);
    int checkUncompressFile(const char *dst_name, const char *src_name);
    int CheckAndUpdateUncompressFile(const char *dst_name, const char *src_name);
    unsigned int CalCRC32(unsigned int crc, const unsigned char *ptr, int buf_len);
    unsigned int CalCRC32(const char *file_path, int file_len);
    unsigned int CalCRC32(const char *file_path);
    int getFileSize(const char *file_path);
    int testCompressFile(const char *dst_path, const char *src_name);

private:
    unsigned char* newBuf(const char *fun_name, int len);
    unsigned char* newSrcBuf(int len);
    unsigned char* newDstBuf(int len);
    void DeleteBuf(const char *fun_name, unsigned char** ptr);
    void DeleteSrcBuf();
    void DeleteDstBuf();
    int readFileToBuffer(const char *file_path, int skip_len, int avilia_len, unsigned char data_buf[]);
    int writeFileFromBuffer(const char *file_path, int wr_size, unsigned char data_buf[]);

private:
    unsigned char* mSrcBuf;
    unsigned char* mDstBuf;

    int mCompressHeadFlag;
    int mUnCompressTailFlag;
    int mMaxUnCompressFileSize;
};

#endif //__MINI_ZIP_H__

