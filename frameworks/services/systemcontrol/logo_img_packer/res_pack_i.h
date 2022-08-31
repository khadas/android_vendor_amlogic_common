/*
 * Copyright (c) 2015 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 */
/*
 * \file        res_pack_i.h
 * \brief       include file for res_pack
 *
 * \version     1.0.0
 * \date        25/10/2013   11:33
 * \author      Sam.Wu <yihui.wu@amlgic.com>
 *
 * Copyright (c) 2013 Amlogic Inc.. All Rights Reserved.
 *
 */
#ifndef __RES_PACK_I_H__
#define __RES_PACK_I_H__

#include <unistd.h>
#include <dirent.h>
#define MAX_PATH    512
#define min(a, b)   ((a) > (b) ? (b) : (a))

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>

#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

//#include "res_pack.h" // avoid re-defined warning. use limit to image_packer module
#include "crc32.h"

typedef void* __hdle;

int img_pack(const char** const path_src, const char* const packedImg,
        const int totalFileNum);
int res_img_pack(const char* const szDir, const char* const outResImg);
int res_img_unpack(const char* const path_src, const char* const unPackDirPath, int needCheckCrc);
int res_img_pack_bmp(const char* path);

#endif // __RES_PACK_I$_H__
