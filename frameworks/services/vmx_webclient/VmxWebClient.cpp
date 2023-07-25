/*
 * Copyright (c) 2023 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#include <aidlcommonsupport/NativeHandle.h>
#include <utils/Log.h>
#include <dlfcn.h>
#include "VmxWebClient.h"
#include "AmVmxWebClientAdaptor.h"

typedef void *(*WebClientAllocContextFunc)(void);

typedef int (*WebClientDecryptFunc)(const void *, struct amVmxWebClientDecryptParam *);

typedef int (*WebClientFreeContextFunc)(void *);

static WebClientAllocContextFunc webclient_alloc = NULL;
static WebClientDecryptFunc webclient_decrypt = NULL;
static WebClientFreeContextFunc webclient_free = NULL;

namespace aidl::vendor::amlogic::hardware::vmx_webclient::implementation {

VmxWebClient::VmxWebClient()
{
    mLibHandle = dlopen("libverimatrixadaptor.so", RTLD_NOW);
    if (mLibHandle == NULL) {
        ALOGE("Unable to locate libverimatrixadaptor.so %s ", dlerror());
        return;
    }

    webclient_alloc =
         (WebClientAllocContextFunc)dlsym(mLibHandle, "amVmxWebClientAllocContext");
    webclient_decrypt =
        (WebClientDecryptFunc)dlsym(mLibHandle, "amVmxWebClientDecrypt");
    webclient_free =
        (WebClientFreeContextFunc)dlsym(mLibHandle, "amVmxWebClientFreeContext");

    if (webclient_alloc)
        mWebClientObj = webclient_alloc();

    ALOGI("Create mWebClientObj is %p L%d", mWebClientObj, __LINE__);
}

VmxWebClient::~VmxWebClient() {
    ::android::Mutex::Autolock autoLock(mLock);

    if (mWebClientObj) {
        if (webclient_free) {
            webclient_free(mWebClientObj);
            mWebClientObj = NULL;
        }
    }

    if (mLibHandle != NULL) {
        dlclose(mLibHandle);
        mLibHandle = NULL;
    }
}

::ndk::ScopedAStatus VmxWebClient::createInstance(int32_t* _aidl_return) {
    (void)_aidl_return;
    ::android::Mutex::Autolock autoLock(mLock);

    if (webclient_alloc && !mWebClientObj)
        mWebClientObj = webclient_alloc();

    ALOGI("Create mWebClientObj is %p L%d", mWebClientObj, __LINE__);
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus VmxWebClient::destroyInstance(int32_t* _aidl_return) {
    (void)_aidl_return;
    ::android::Mutex::Autolock autoLock(mLock);

    ALOGI("Destroy mWebClientObj is %p", mWebClientObj);
    if (mWebClientObj) {
        if (webclient_free) {
            webclient_free(mWebClientObj);
            mWebClientObj = NULL;
        }
    }
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus VmxWebClient::decrypt(const VmxWebClientDecryptParam& para,
        std::vector<uint8_t>* outData, int32_t* _aidl_return) {
    (void)para;
    (void)outData;
    (void)_aidl_return;
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus VmxWebClient::decryptSecure(const VmxWebClientDecryptParam& para,
        int32_t* _aidl_return) {
    (void)_aidl_return;
    ::android::Mutex::Autolock autoLock(mLock);

    struct amVmxWebClientDecryptParam amPara;
    int ret = 0;
    const char *detailedError = "";

    memset(&amPara, 0, sizeof(amPara));

    if (mWebClientObj && webclient_decrypt) {
        amPara.mSecure = para.secure;
        amPara.mSampleAES = para.sampleAES;
        amPara.mKeySeq = para.keySeq;

        if (para.mode == Mode::UNENCRYPTED) {
            amPara.mMode = kMode_Unencrypted;
        } else if (para.mode == Mode::AES_CTR) {
            amPara.mMode = kMode_AES_CTR;
        } else if (para.mode == Mode::AES_CBC) {
            amPara.mMode = kMode_AES_CBC;
        } else {
            ALOGE("Can't support decrypt mode");
            detailedError = "Can't support decrypt mode";
            return toNdkScopedAStatus(Status::ERROR_DRM_DECRYPT, detailedError);
        }

        amPara.mPattern.mEncryptBlocks = para.pattern.encryptBlocks;
        amPara.mPattern.mSkipBlocks = para.pattern.skipBlocks;

        if (para.key.size() > 0) {
            amPara.mKeyUrl = (const char *)para.key.data();
            amPara.mKeyUrlLen = para.key.size();
        }

        if (para.iv.size() > 0) {
            amPara.mIv = para.iv.data();
            amPara.mIvLen = para.iv.size();
        }

        if (para.subSamples.size() > 0) {
            amPara.mSubSamples = reinterpret_cast<const struct SubSamples *>(para.subSamples.data());
            amPara.mNumSubSamples = para.subSamples.size();
        }

        amPara.mSourceHandle = ::android::makeFromAidl(para.sourceDesc);
        amPara.mSecureHandle = ::android::makeFromAidl(para.secureDesc);
        amPara.mSrcOffset = para.srcOffset;
        amPara.mOffset = para.offset;

        ret = webclient_decrypt(mWebClientObj, &amPara);
        if (ret) {
            ALOGE("decrypt failed 0x%x", ret);
            detailedError = "decrypt failed";
            return toNdkScopedAStatus(Status::ERROR_DRM_DECRYPT, detailedError);
        }
    } else {
        ALOGE("Invalid obj or decrypt interface %p %p", mWebClientObj, webclient_decrypt);
        detailedError = "Invalid obj or decrypt interface";
        return toNdkScopedAStatus(Status::ERROR_DRM_UNKNOWN, detailedError);
    }

    return toNdkScopedAStatus(Status::OK);
}

}  // namespace vendor::amlogic::hardware::vmx_webclient::implementation
