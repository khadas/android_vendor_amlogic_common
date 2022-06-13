/*
 * Copyright (c) 2015 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 */
/*
 * crc32.c
 *
 *  Created on: 2013-5-31
 *      Author: binsheng.xu@amlogic.com
 */
#include "res_pack_i.h"
#include "res_pack.h"
#define LOG_TAG "SystemControl"
#include "common.h"

#define BUFSIZE     1024*16

static unsigned int crc_table[256];


static void init_crc_table(void)
{
    unsigned int c;
    unsigned int i, j;

    for (i = 0; i < 256; i++) {
        c = (unsigned int)i;
        for (j = 0; j < 8; j++) {
            if (c & 1)
                c = 0xedb88320L ^ (c >> 1);
            else
                c = c >> 1;
        }
        crc_table[i] = c;
    }
}

//generate crc32 with buffer data
unsigned int crc32(unsigned int crc,unsigned char *buffer, unsigned int size)
{
    unsigned int i;
    for (i = 0; i < size; i++) {
        crc = crc_table[(crc ^ buffer[i]) & 0xff] ^ (crc >> 8);
    }
    return crc ;
}

//Generate crc32 value with file steam, which from 'offset' to end if checkSz==0
unsigned calc_logoimg_crc(int fd, off_t offset, unsigned checkSz)
{

    unsigned char buf[BUFSIZE];

    unsigned int crc = 0xffffffff;
    unsigned MaxCheckLen = 0;
    unsigned totalLenToCheck = 0;
	int off_set = 0;

    if (fd < 0) {
        SYS_LOGE("bad param!!\n");
        return 0;
    }

    MaxCheckLen = lseek(fd,0,SEEK_END);
    SYS_LOGI("MaxCheckLen = 0x%x\n", MaxCheckLen);
    MaxCheckLen -= offset;
    if (!checkSz) {
        checkSz = MaxCheckLen;
    }
    else if (checkSz > MaxCheckLen) {
        SYS_LOGE("checkSz %u > max %u\n", checkSz, MaxCheckLen);
        return 0;
    }

    init_crc_table();

    while (totalLenToCheck < checkSz)
    {
        int nread;
        unsigned leftLen = checkSz - totalLenToCheck;
        int thisReadSz = leftLen > BUFSIZE ? BUFSIZE : leftLen;

        off_set = offset + totalLenToCheck;
        lseek(fd,off_set,SEEK_SET);
        nread = read(fd, buf, thisReadSz);
        if (nread < 0) {
            SYS_LOGE("%d:read %s.\n", __LINE__, strerror(errno));
            return 0;
        }
        crc = crc32(crc, buf, thisReadSz);
        totalLenToCheck += thisReadSz;
    }

    return crc;
}

int check_img_crc(int fd, off_t offset, const unsigned orgCrc, unsigned checkSz)
{
    const unsigned genCrc = calc_logoimg_crc(fd, offset, checkSz);

    if (genCrc != orgCrc)
    {
        SYS_LOGE("%d:genCrc 0x%x != orgCrc 0x%x, error(%s).\n", __LINE__, genCrc, orgCrc, strerror(errno));
        return -1;
    }

    return 0;
}


