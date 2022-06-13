/** @file mfgdebug.c
 *
 *  @brief This file contains MFG bridge debug code.
 *
 * (C) Copyright 2011-2017 Marvell International Ltd. All Rights Reserved
 *
 * MARVELL CONFIDENTIAL
 * The source code contained or described herein and all documents related to
 * the source code ("Material") are owned by Marvell International Ltd or its
 * suppliers or licensors. Title to the Material remains with Marvell International Ltd
 * or its suppliers and licensors. The Material contains trade secrets and
 * proprietary and confidential information of Marvell or its suppliers and
 * licensors. The Material is protected by worldwide copyright and trade secret
 * laws and treaty provisions. No part of the Material may be used, copied,
 * reproduced, modified, published, uploaded, posted, transmitted, distributed,
 * or disclosed in any way without Marvell's prior express written permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery
 * of the Materials, either expressly, by implication, inducement, estoppel or
 * otherwise. Any license under such intellectual property rights must be
 * express and approved by Marvell in writing.
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include "mfgdebug.h"

static unsigned char mfg_debug = DBG_MASK;

void
mfg_dprint(int dbg_level, char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    if (dbg_level & mfg_debug) {
        vprintf(fmt, args);
    }
    va_end(args);
}

void
mfg_hexdump(int dbg_level, const char *title, const unsigned char *buf, int len)
{
    int i, j;
    unsigned char *offset;

    if (!(dbg_level & mfg_debug))
        return;

    offset = (unsigned char *) buf;
    printf("%s - hexdump(len=%lu):\n", title, (unsigned long) len);

    for (i = 0; i < len / 16; i++) {
        for (j = 0; j < 16; j++)
            printf("%02x  ", offset[j]);
        printf("\n");
        offset += 16;
    }
    i = len % 16;
    for (j = 0; j < i; j++)
        printf("%02x  ", offset[j]);
    printf("\n");
}
