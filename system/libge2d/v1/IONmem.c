/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <ion/ion.h>
#include <linux/ion_4.12.h>
#include "IONmem.h"

static int cmem_fd = -2;
static int ref_count = 0;

static int validate_init()
{
    switch (cmem_fd) {
      case -3:
        __E("CMEM_exit() already called, check stderr output for earlier "
            "CMEM failure messages (possibly version mismatch).\n");

        return 0;

      case -2:
        __E("CMEM_init() not called, you must initialize CMEM before "
            "making API calls.\n");

        return 0;

      case -1:
        __E("CMEM file descriptor -1 (failed 'open()'), ensure CMEMK "
            "kernel module cmemk.ko has been installed with 'insmod'");

        return 0;

      default:
        return 1;
    }
}

int CMEM_init(void)
{
    __D("init: entered - ref_count %d, cmem_fd %d\n", ref_count, cmem_fd);

    if (cmem_fd >= 0) {
        ref_count++;
        __D("init: /dev/ion already opened, incremented ref_count %d\n",
            ref_count);
        return 0;
    }

    cmem_fd = ion_open();

    if (cmem_fd < 0) {
        __E("init: Failed to open /dev/ion: '%s'\n", strerror(errno));
        return -1;
    }

    ref_count++;

    __D("init: successfully opened /dev/ion...\n");

    __D("init: exiting, returning success\n");

    return 0;
}

int ion_mem_alloc_fd(int ion_fd, size_t size, IONMEM_AllocParams *params, unsigned int flag, unsigned int alloc_hmask)
{
    int ret = -1;
    int num_heaps = 0;
    unsigned int heap_mask = 0;

    if (ion_query_heap_cnt(ion_fd, &num_heaps) >= 0) {
        __D("num_heaps=%d\n", num_heaps);
        struct ion_heap_data *const heaps = (struct ion_heap_data *)malloc(num_heaps * sizeof(struct ion_heap_data));
        if (heaps != NULL && num_heaps) {
            if (ion_query_get_heaps(ion_fd, num_heaps, heaps) >= 0) {
                for (int i = 0; i != num_heaps; ++i) {
                    __D("heaps[%d].type=%d, heap_id=%d\n", i, heaps[i].type, heaps[i].heap_id);
                    if ((1 << heaps[i].type) == alloc_hmask) {
                        /* ion-fb heap is only for display */
                        if (!strncmp("ion-fb", heaps[i].name, 6))
                            continue;
                        heap_mask = 1 << heaps[i].heap_id;
                        __D("%d, m=%x, 1<<heap_id=%x, heap_mask=%x, name=%s, alloc_hmask=%x\n",
                            heaps[i].type, 1<<heaps[i].type, heaps[i].heap_id, heap_mask, heaps[i].name, alloc_hmask);
                        ret = ion_alloc_fd(ion_fd, size, 0, heap_mask, flag, &params->mImageFd);
                        if (ret >= 0)
                            break;
                    }
                }
            }
            free(heaps);
            if (!heap_mask)
                __E("don't find match heap!!\n");
        } else {
              if (heaps)
                  free(heaps);
            __E("heaps is NULL or no heaps,num_heaps=%d\n", num_heaps);
        }
    } else {
        __E("query_heap_cnt fail! no ion heaps for alloc!!!\n");
    }
    if (ret < 0) {
        __E("ion_alloc failed, errno=%d\n", errno);
        return -ENOMEM;
    }
    return ret;
}

unsigned long CMEM_alloc(size_t size, IONMEM_AllocParams *params)
{
    int ret = 0;
    int legacy_ion = 0;

    if (!validate_init()) {
        return 0;
    }

    legacy_ion = ion_is_legacy(cmem_fd);
    if (legacy_ion) {
        ret = ion_alloc(cmem_fd, size, 0, 1 << ION_HEAP_TYPE_CUSTOM, 0, &params->mIonHnd);
        if (ret < 0) {
            ret = ion_alloc(cmem_fd, size, 0, ION_HEAP_CARVEOUT_MASK, 0, &params->mIonHnd);
            if (ret < 0) {
                ret = ion_alloc(cmem_fd, size, 0, ION_HEAP_TYPE_DMA_MASK, 0, &params->mIonHnd);
                    if (ret < 0) {
                        ret = ion_close(cmem_fd);
                        if (ret < 0)
                            __E("ion_close failed, errno=%d", errno);
                        __E("ion_alloc failed, errno=%d", errno);
                        cmem_fd = -1;
                        return -ENOMEM;
                    }
            }
        }
        ret = ion_share(cmem_fd, params->mIonHnd, &params->mImageFd);
        if (ret < 0) {
            __E("ion_share failed, errno=%d", errno);
            ion_free(cmem_fd, params->mIonHnd);
            ret = ion_close(cmem_fd);
            if (ret < 0)
                __E("ion_close failed, errno=%d", errno);
            cmem_fd = -1;
            return -EINVAL;
        }
    } else {
        unsigned flag = 0;

        flag |= ION_FLAG_EXTEND_MESON_HEAP;
        ret = ion_mem_alloc_fd(cmem_fd, size, params, flag,
                                    ION_HEAP_TYPE_DMA_MASK);
        if (ret < 0)
            ret = ion_mem_alloc_fd(cmem_fd, size, params, flag,
                                        ION_HEAP_CARVEOUT_MASK);
        if (ret < 0)
                ret = ion_mem_alloc_fd(cmem_fd, size, params, flag,
                                            ION_HEAP_TYPE_CUSTOM);
        if (ret < 0) {
            __E("%s failed, errno=%d\n", __func__, errno);
            return -ENOMEM;
        }
    }

    return ret;
}

#if 0
void* CMEM_getUsrPtr(unsigned long PhyAdr, int size)
{
    void *userp = NULL;
    /* Map the physical address to user space */
    userp = mmap(0,                       // Preferred start address
                 size,                    // Length to be mapped
                 PROT_WRITE | PROT_READ,  // Read and write access
                 MAP_SHARED,              // Shared memory
                 cmem_fd,                 // File descriptor
                 PhyAdr);               // The byte offset from fd

    if (userp == MAP_FAILED) {
        __E("registerAlloc: Failed to mmap buffer at physical address %#lx\n",
            PhyAdr);
        return NULL;
    }
    __D("mmap succeeded, returning virt buffer %p\n", userp);

    return userp;
}
#endif

int CMEM_free(IONMEM_AllocParams *params)
{
    if (!validate_init()) {
        return -1;
    }
    __D("CMEM_free,mIonHnd=%x free\n", params->mIonHnd);

    ion_free(cmem_fd, params->mIonHnd);

    return 0;
}


int CMEM_exit(void)
{
    int result = 0;

    __D("exit: entered - ref_count %d, cmem_fd %d\n", ref_count, cmem_fd);

    if (!validate_init()) {
        return -1;
    }

    __D("exit: decrementing ref_count\n");

    ref_count--;
    if (ref_count == 0) {
        result = ion_close(cmem_fd);

        __D("exit: ref_count == 0, closed /dev/ion (%s)\n",
            result == -1 ? strerror(errno) : "succeeded");

        /* setting -3 allows to distinguish CMEM exit from CMEM failed */
        cmem_fd = -3;
    }

    __D("exit: exiting, returning %d\n", result);

    return result;
}

