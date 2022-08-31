/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef IONMEM_H
#define IONMEM_H
#include <ion/ion.h>
#include <stdbool.h>
#include <log/log.h>
#if defined (__cplusplus)
extern "C" {
#endif

#ifdef __DEBUG
#define __D(fmt, args...) ALOGD("ionmem debug: " fmt, ## args)
#else
#define __D(fmt, args...)
#endif

#define __E(fmt, args...) ALOGE("ionmem error: " fmt, ## args)

#define ION_FLAG_EXTEND_MESON_HEAP          (1 << 30)

typedef struct IONMEM_AllocParams {
    ion_user_handle_t   mIonHnd;
    int                 mImageFd;
    size_t size;
    unsigned char *usr_ptr;
} IONMEM_AllocParams;

int ion_mem_init(void);
unsigned long ion_mem_alloc(int ion_fd, size_t size, IONMEM_AllocParams *params, bool cache_flag);
int ion_mem_invalid_cache(int ion_fd, int shared_fd);
void ion_mem_exit(int ion_fd);

#define ION_IOC_MAGIC 'I'
#define ION_IOC_INVALID_CACHE _IOWR(ION_IOC_MAGIC, 9, struct ion_fd_data)

#if defined (__cplusplus)
}
#endif

#endif

