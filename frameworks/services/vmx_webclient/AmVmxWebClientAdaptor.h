/*
 * Copyright (c) 2023 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef AM_VMX_ADAPTOR_H_
#define AM_VMX_ADAPTOR_H_

#include <cutils/native_handle.h>

#ifdef __cplusplus
extern "C" {
#endif

struct SubSamples {
    int mNumBytesOfClearData;
    int mNumBytesOfEncryptedData;
};

enum Mode {
    kMode_Unencrypted = 0,
    kMode_AES_CTR     = 1,
    kMode_AES_WV      = 2,
    kMode_AES_CBC     = 3,
};

struct Pattern {
    int mEncryptBlocks;
    int mSkipBlocks;
};

struct amVmxWebClientDecryptParam
{
    int mSecure;
    int mSampleAES;
    const char *mKeyUrl;
    int mKeyUrlLen;
    int mKeySeq;
    const unsigned char *mIv;
    int mIvLen;
    enum Mode mMode;
    struct Pattern mPattern;
    const struct SubSamples *mSubSamples;
    int mNumSubSamples;
    const native_handle_t *mSourceHandle;
    const native_handle_t *mSecureHandle;
    uint64_t mSrcOffset;
    uint64_t mOffset;
};

void *amVmxWebClientAllocContext(void);

int amVmxWebClientDecrypt(const void *context, struct amVmxWebClientDecryptParam *para);

int amVmxWebClientFreeContext(void *context);

#ifdef __cplusplus
}
#endif

#endif
