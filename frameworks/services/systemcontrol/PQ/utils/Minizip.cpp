/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 * Author:  Shoufu Zhao <shoufu.zhao@amlogic.com>
 */
#define LOG_TAG "SystemControl"
#define LOG_TV_TAG "Minizip"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

#include <utils/Log.h>

#include "Minizip.h"

extern "C" int mz_compress(unsigned char *pDest, unsigned long *pDest_len, const unsigned char *pSource, unsigned long source_len);
extern "C" int mz_compress2(unsigned char *pDest, unsigned long *pDest_len, const unsigned char *pSource, unsigned long source_len, int level);
extern "C" int mz_uncompress(unsigned char *pDest, unsigned long *pDest_len, const unsigned char *pSource, unsigned long source_len);

Minizip::Minizip()
{
    SYS_LOGD("%s, entering...\n", __FUNCTION__);

    mCompressHeadFlag = 1;     //default compress have head info
    mUnCompressTailFlag = 1;   //default uncompress have tail info
    mMaxUnCompressFileSize = CC_MAX_UNCOMPRESS_FILE_BUF_SIZE;

    mSrcBuf = NULL;
    mDstBuf = NULL;
}

Minizip::~Minizip()
{
    SYS_LOGD("%s, entering...\n", __FUNCTION__);

    freeAll();
}

unsigned char *Minizip::newBuf(const char *fun_name, int len)
{
    unsigned char *tmp_ptr = NULL;

    if (len <= 0)
    {
        return NULL;
    }

    tmp_ptr = new unsigned char[len];
    SYS_LOGD("%s, function %s new %d bytes ok (%p)\n", __FUNCTION__, fun_name, len, tmp_ptr);

    return tmp_ptr;
}

unsigned char *Minizip::newSrcBuf(int len)
{
    if (mSrcBuf == NULL)
    {
        mSrcBuf = newBuf(__FUNCTION__, len);
    }
    return mSrcBuf;
}

unsigned char *Minizip::newDstBuf(int len)
{
    if (mDstBuf == NULL)
    {
        mDstBuf = newBuf(__FUNCTION__, len);
    }
    return mDstBuf;
}

void Minizip::DeleteBuf(const char *fun_name, unsigned char **ptr)
{
    unsigned char *tmp_ptr = NULL;

    if (*ptr != NULL)
    {
        SYS_LOGD("%s, function %s delete buffer ok (%p)\n", __FUNCTION__, fun_name, *ptr);
        delete *ptr;
        *ptr = NULL;
    }
}

void Minizip::DeleteSrcBuf()
{
    DeleteBuf(__FUNCTION__, &mSrcBuf);
}

void Minizip::DeleteDstBuf()
{
    DeleteBuf(__FUNCTION__, &mDstBuf);
}

void Minizip::freeAll()
{
    DeleteSrcBuf();
    DeleteDstBuf();
}

int Minizip::setCompressHeadFlag(int flag_val)
{
    int oldVal = mCompressHeadFlag;
    mCompressHeadFlag = flag_val;
    return oldVal;
}

int Minizip::getCompressHeadFlag()
{
    return mCompressHeadFlag;
}

int Minizip::setMaxUnCompressFileSize(int max_size)
{
    int oldSize = mMaxUnCompressFileSize;
    mMaxUnCompressFileSize = max_size;
    return oldSize;
}

int Minizip::getMaxUnCompressFileSize()
{
    return mMaxUnCompressFileSize;
}

int Minizip::compress_file(const char *dst_name, const char *src_name)
{
    int tmp_ret = 0, header_len = 0;
    unsigned long src_len = 0, dst_len = 0;

    header_len = 0;
    if (getCompressHeadFlag() == 1)
    {
        header_len = sizeof(struct minizip_comp_header_s);
    }

    tmp_ret = readFileToBuffer(src_name, 0, -1, NULL);
    if (tmp_ret <= header_len)
    {
        SYS_LOGE("%s, read file to buffer error!!!\n", __FUNCTION__);
        return -1;
    }

    src_len = tmp_ret;

    newDstBuf(mMaxUnCompressFileSize);

    if (mDstBuf != NULL && mSrcBuf != NULL)
    {
        // dst_len input is avaliable buffer size, output is compress size.
        dst_len = mMaxUnCompressFileSize;
        tmp_ret = mz_compress(mDstBuf + header_len, &dst_len, mSrcBuf, src_len);
        if (tmp_ret < 0)
        {
            SYS_LOGE("%s, compress file to buffer error(%d, %lu, %lu)!!!\n", __FUNCTION__, tmp_ret, src_len, dst_len);
            return -1;
        }

        SYS_LOGD("%s, compress file to buffer ok (%d, %lu, %lu)\n", __FUNCTION__, tmp_ret, src_len, dst_len);

        if (getCompressHeadFlag() == 1)
        {
            struct minizip_comp_header_s tmpHead;

            memset((void *)&tmpHead, 0, header_len);

            tmpHead.version = 1;
            tmpHead.src_len = src_len;
            tmpHead.src_crc32 = CalCRC32(0, mSrcBuf, src_len);
            strncpy((char *)tmpHead.tag_str, CS_TAG_STR_CONTENT, CC_TAG_STR_MAX_LEN);
            tmpHead.self_len = dst_len + header_len;

            memcpy(mDstBuf, (void *)&tmpHead, header_len);
            tmpHead.self_crc32 = CalCRC32(0, mDstBuf + 4, dst_len + header_len - 4);
            memcpy(mDstBuf, (void *) & (tmpHead.self_crc32), 4);

            writeFileFromBuffer(dst_name, dst_len + header_len, mDstBuf);
        }
        else
        {
            writeFileFromBuffer(dst_name, dst_len + header_len, mDstBuf);
        }

        DeleteSrcBuf();
        DeleteDstBuf();

        return 0;
    }

    DeleteSrcBuf();

    SYS_LOGE("%s, new dst buffer error!!!\n", __FUNCTION__);
    return -1;
}

int Minizip::uncompress_file(const char *dst_name, const char *src_name)
{
    int tmp_ret = 0, header_len = 0;
    unsigned long src_len = 0, dst_len = 0;

    header_len = 0;
    if (getCompressHeadFlag() == 1)
    {
        header_len = sizeof(struct minizip_comp_header_s);
    }

    tmp_ret = readFileToBuffer(src_name, 0, -1, NULL);
    if (tmp_ret <= header_len)
    {
        SYS_LOGE("%s, read file to buffer error!!!\n", __FUNCTION__);
        return -1;
    }

    src_len = tmp_ret;

    if (mSrcBuf != NULL && checkCompressHeader(NULL, mSrcBuf, src_len) < 0)
    {
        DeleteSrcBuf();
        return -1;
    }

    newDstBuf(mMaxUnCompressFileSize);

    if (mDstBuf != NULL && mSrcBuf != NULL)
    {
        // dst_len input is avaliable buffer size, output is compress size.
        dst_len = mMaxUnCompressFileSize;
        tmp_ret = mz_uncompress(mDstBuf, &dst_len, mSrcBuf + header_len, src_len - header_len);
        if (tmp_ret < 0)
        {
            SYS_LOGE("%s, uncompress file to buffer error (%d, %lu, %lu) !!!\n", __FUNCTION__, tmp_ret, src_len, dst_len);
            return -1;
        }

        SYS_LOGD("%s, uncompress file to buffer ok (%d, %lu, %lu)\n", __FUNCTION__, tmp_ret, src_len, dst_len);

        writeFileFromBuffer(dst_name, dst_len, mDstBuf);

        DeleteSrcBuf();
        DeleteDstBuf();

        return 0;
    }

    DeleteSrcBuf();

    SYS_LOGE("%s, new dst buffer error!!!\n", __FUNCTION__);
    return -1;
}

int Minizip::getFileSize(const char *file_path)
{
    int file_size = 0;
    int dev_fd = -1;

    dev_fd = open(file_path, O_RDONLY);
    if (dev_fd < 0)
    {
        SYS_LOGE("%s, open \"%s\" ERROR(%s)!!\n", __FUNCTION__,
              file_path, strerror(errno));
        return 0;
    }

    file_size = lseek(dev_fd, 0L, SEEK_END);
    lseek(dev_fd, 0L, SEEK_SET);

    return file_size;
}

int Minizip::readFileToBuffer(const char *file_path, int skip_len, int avilia_len, unsigned char data_buf[])
{
    int rd_cnt = 0, file_size = 0;
    int dev_fd = -1;
    unsigned char *tmp_ptr = data_buf;

    dev_fd = open(file_path, O_RDONLY);
    if (dev_fd < 0)
    {
        SYS_LOGE("%s, open \"%s\" ERROR(%s)!!\n", __FUNCTION__,
              file_path, strerror(errno));
        return -1;
    }

    file_size = lseek(dev_fd, 0L, SEEK_END);

    if (file_size <= 0)
    {
        SYS_LOGE("%s, file \"%s\" size ERROR(%d)!!!!\n", __FUNCTION__,
              file_path, file_size);
        close(dev_fd);
        dev_fd = -1;
        return -1;
    }

    if (tmp_ptr != NULL && file_size > avilia_len)
    {
        SYS_LOGE("%s, file \"%s\" size over flow buffer ERROR(%d, %d)!!!!\n", __FUNCTION__,
              file_path, file_size, avilia_len);
        close(dev_fd);
        dev_fd = -1;
        return -1;
    }

    if (file_size < skip_len)
    {
        SYS_LOGE("%s, file \"%s\" size skip length ERROR(%d, %d)!!!!\n", __FUNCTION__,
              file_path, file_size, skip_len);
        close(dev_fd);
        dev_fd = -1;
        return -1;
    }

    if (tmp_ptr == NULL)
    {
        tmp_ptr = newSrcBuf(file_size);
        skip_len = 0;
    }

    lseek(dev_fd, skip_len, SEEK_SET);

    if (tmp_ptr != NULL)
    {
        rd_cnt = read(dev_fd, tmp_ptr, file_size);

        close(dev_fd);
        dev_fd = -1;

        if (rd_cnt != file_size)
        {
            SYS_LOGE("%s, read file \"%s\" ERROR(%d, %d)!!!!\n", __FUNCTION__,
                  file_path, rd_cnt, file_size);
            DeleteSrcBuf();
            return -1;
        }

        return rd_cnt;
    }

    SYS_LOGE("%s, new src buffer error!!!\n", __FUNCTION__);
    return -1;
}

int Minizip::writeFileFromBuffer(const char *file_path, int wr_size, unsigned char data_buf[])
{
    int wr_cnt = 0;
    int dev_fd = -1;

    if (data_buf == NULL)
    {
        SYS_LOGE("%s, data_buf buffer is NULL\n", __FUNCTION__);
        return -1;
    }

    dev_fd = open(file_path, O_WRONLY | O_SYNC | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);

    if (dev_fd < 0)
    {
        SYS_LOGE("%s, open %s ERROR(%s)!!\n", __FUNCTION__, file_path, strerror(errno));
        return -1;
    }

    wr_cnt = write(dev_fd, data_buf, wr_size);

    fsync(dev_fd);

    close(dev_fd);
    dev_fd = -1;

    return wr_cnt;
}

int Minizip::checkCompressHeader(struct minizip_comp_header_s *ptrHead, const char *src_name)
{
    int tmp_ret = 0, header_len = 0;

    header_len = 0;
    if (getCompressHeadFlag() == 1)
    {
        header_len = sizeof(struct minizip_comp_header_s);
    }

    tmp_ret = readFileToBuffer(src_name, 0, -1, NULL);
    if (tmp_ret <= header_len)
    {
        SYS_LOGE("%s, read file to buffer error!!!\n", __FUNCTION__);
        return -1;
    }

    if (checkCompressHeader(ptrHead, mSrcBuf, tmp_ret) < 0)
    {
        SYS_LOGE("%s, check bin header error!!!\n", __FUNCTION__);
        DeleteSrcBuf();
        return -1;
    }

    SYS_LOGD("%s, check compress file header ok.\n", __FUNCTION__);
    return 0;
}

int Minizip::checkCompressHeader(struct minizip_comp_header_s *ptrHead, unsigned char *data_buf, int data_len)
{
    int header_len = sizeof(struct minizip_comp_header_s);
    unsigned int cal_crc32 = 0;
    struct minizip_comp_header_s tmpHead;
    struct minizip_comp_header_s *pHead = ptrHead;

    if (pHead == NULL)
    {
        pHead = &tmpHead;
    }

    if (data_buf == NULL)
    {
        SYS_LOGE("%s, data_buf is NULL!!!\n", __FUNCTION__);
        return -1;
    }

    memset((void *)pHead, 0, header_len);

    if (getCompressHeadFlag() == 0)
    {
        return 0;
    }

    memcpy((void *)pHead, data_buf, header_len);
    if (strncmp((char *)pHead->tag_str, CS_TAG_STR_CONTENT, CC_TAG_STR_MAX_LEN))
    {
        SYS_LOGE("%s, Header file tag str error!!!\n", __FUNCTION__);
        return -1;
    }

    if ((int)pHead->self_len != (int)data_len)
    {
        SYS_LOGE("%s, Header file len error (0x%08x, 0x%08x)!!!\n", __FUNCTION__, pHead->self_len, data_len);
        return -1;
    }

    cal_crc32 = CalCRC32(0, data_buf + 4, data_len - 4);
    if (pHead->self_crc32 != cal_crc32)
    {
        SYS_LOGE("%s, Header file crc error (0x%08x, 0x%08x)!!!\n", __FUNCTION__, pHead->self_crc32, cal_crc32);
        return -1;
    }

    return 0;
}

int Minizip::checkUncompressFile(const char *dst_name, const char *src_name)
{
    int tmp_len = 0, error_code = 0, tmp_dst_err = 0;
    struct minizip_comp_header_s tmpHead;
    unsigned int cal_crc32 = 0;

    if (dst_name == NULL)
    {
        SYS_LOGE("%s, dst file name ptr is NULL.\n", __FUNCTION__);
        return CC_UNCOMPRESS_CHECK_ERROR_DST_FILE;
    }

    if (src_name == NULL)
    {
        SYS_LOGE("%s, src file name ptr is NULL.\n", __FUNCTION__);
        return CC_UNCOMPRESS_CHECK_ERROR_SRC_FILE;
    }

    if (access(dst_name, 0) < 0)
    {
        SYS_LOGE("%s, file name \"%s\" not exist.\n", __FUNCTION__, dst_name);
        return CC_UNCOMPRESS_CHECK_ERROR_DST_FILE;
    }

    if (access(src_name, 0) < 0)
    {
        SYS_LOGE("%s, file name \"%s\" not exist.\n", __FUNCTION__, src_name);
        return CC_UNCOMPRESS_CHECK_ERROR_SRC_FILE;
    }

    error_code = CC_UNCOMPRESS_CHECK_OK;

    if (checkCompressHeader(&tmpHead, src_name) == 0)
    {
        tmp_dst_err = 0;

        // start check file length is equal to src bin file
        tmp_len = getFileSize(dst_name);
        if ((int)tmp_len != (int)tmpHead.src_len)
        {
            tmp_dst_err = 1;
            SYS_LOGE("%s, dst file len error (%d, %d)!!!\n", __FUNCTION__, tmp_len, tmpHead.src_len);
            error_code = CC_UNCOMPRESS_CHECK_ERROR_DST_FILE;
        }

        // start check file crc is equal to src bin src crc
        if (tmp_dst_err == 0)
        {
            cal_crc32 = CalCRC32(dst_name, tmp_len);
            if (cal_crc32 != 0 && cal_crc32 != tmpHead.src_crc32)
            {
                SYS_LOGE("%s, dst file crc error (0x%08x, 0x%08x)!!!\n", __FUNCTION__, tmpHead.src_crc32, cal_crc32);
                error_code = CC_UNCOMPRESS_CHECK_ERROR_DST_FILE;
            }
        }
    }

    if (error_code == CC_UNCOMPRESS_CHECK_OK)
    {
        SYS_LOGD("%s, check uncompress file ok\n", __FUNCTION__);
    }

    return error_code;
}

int Minizip::CheckAndUpdateUncompressFile(const char *dst_name, const char *src_name)
{
    int tmp_err_code = 0;
    int ret = 0;
    SYS_LOGD("%s, entering...\n", __FUNCTION__);

    tmp_err_code = checkUncompressFile(dst_name, src_name);
    if (tmp_err_code == CC_UNCOMPRESS_CHECK_ERROR_DST_FILE)
    {
        SYS_LOGD("%s, start to uncompress bin file \"%s\" to dst file \"%s\"\n", __FUNCTION__, src_name, dst_name);
        remove(dst_name);
        if ( uncompress_file(dst_name, src_name) != 0 )
        {
            ALOGD("%s, uncompress %s failed!!!!\n", __FUNCTION__, src_name);
            return -1;
        }

    }
    else if ( tmp_err_code == CC_UNCOMPRESS_CHECK_ERROR_SRC_FILE )
    {
        SYS_LOGD("%s, %s is not found!!!!\n", __FUNCTION__, src_name);
        return -1;
    }

    SYS_LOGD("%s, exiting...\n", __FUNCTION__);
    return 0;
}

unsigned int Minizip::CalCRC32(const char *file_path)
{
    return CalCRC32(file_path, -1);
}

unsigned int Minizip::CalCRC32(const char *file_path, int file_len)
{
    int tmp_ret = 0, tmp_len = 0;
    unsigned int cal_crc32 = 0;
    unsigned char *tmp_buf = NULL;

    if (file_path == NULL)
    {
        SYS_LOGE("%s, file path is NULL!!!\n", __FUNCTION__);
        return 0;
    }

    if (file_len <= 0)
    {
        tmp_len = getFileSize(file_path);
        if (tmp_len <= 0)
        {
            SYS_LOGE("%s, file size (%d) error!!!\n", __FUNCTION__, tmp_ret);
            return 0;
        }
    }
    else
    {
        tmp_len = file_len;
    }

    tmp_buf = new unsigned char[tmp_len];
    tmp_ret = readFileToBuffer(file_path, 0, tmp_len, tmp_buf);
    if (tmp_ret <= 0)
    {
        SYS_LOGE("%s, read file to buffer error!!!\n", __FUNCTION__);

        delete tmp_buf;
        tmp_buf = NULL;
        return -1;
    }

    cal_crc32 = CalCRC32(0, tmp_buf, tmp_len);

    delete tmp_buf;
    tmp_buf = NULL;
    return cal_crc32;
}

unsigned int Minizip::CalCRC32(unsigned int crc, const unsigned char *ptr, int buf_len)
{
    static const unsigned int s_crc32[16] =
    {
        0, 0x1db71064, 0x3b6e20c8, 0x26d930ac, 0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
        0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c, 0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
    };

    unsigned int crcu32 = crc;
    if (buf_len < 0)
    {
        return 0;
    }

    if (!ptr)
    {
        return 0;
    }

    crcu32 = ~crcu32;
    while (buf_len--)
    {
        unsigned char b = *ptr++;
        crcu32 = (crcu32 >> 4) ^ s_crc32[(crcu32 & 0xF) ^ (b & 0xF)];
        crcu32 = (crcu32 >> 4) ^ s_crc32[(crcu32 & 0xF) ^ (b >> 4)];
    }

    return ~crcu32;
}

int Minizip::testCompressFile(const char *dst_path, const char *src_name)
{
    char tmp_path[256] = {'\0'};
    int i = 0, tmp_len = 0;
    int cur_file_cnt = 0, total_file_cnt = 0x7FFFFFFF;

    if (dst_path == NULL || src_name == NULL)
    {
        SYS_LOGE("dst_path or src_name is NULL!!!\n");
        return -1;
    }

    if (access(dst_path, 0) < 0)
    {
        SYS_LOGD("folder \"%s\"is not exist, create it.\n", dst_path);
        mkdir(dst_path, 0777);
    }

    strcpy(tmp_path, dst_path);
    tmp_len = strlen(tmp_path);

    for (i = 0; i < total_file_cnt; i++)
    {
        sprintf(tmp_path + tmp_len, "/pq_%010d.db", i);
        if (access(tmp_path, 0) < 0)
        {
            SYS_LOGD("find one avaliable file name \"%s\"\n", tmp_path);
            break;
        }
    }

    return uncompress_file(tmp_path, src_name);
}
