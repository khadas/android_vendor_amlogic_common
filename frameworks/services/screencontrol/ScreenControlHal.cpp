/*
 * Copyright (C) 2006 The Android Open Source Project
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
#define LOG_TAG "ScreenControlHal"
#include <log/log.h>
#include <string>
#include <inttypes.h>
#include <utils/String8.h>
#include <android/hidl/allocator/1.0/IAllocator.h>
#include <android/hidl/memory/1.0/IMemory.h>
#include <hidlmemory/mapping.h>
#include "ScreenControlHal.h"
namespace vendor {
namespace amlogic {
namespace hardware {
namespace screencontrol {
namespace V1_0 {
namespace implementation {
//    using ::android::hidl::memory::V1_0::IMapper;
using ::android::hidl::allocator::V1_0::IAllocator;
using ::android::hardware::hidl_memory;
using ::android::hidl::memory::V1_0::IMemory;
using ::android::hardware::mapMemory;
using ::android::Mutex;

ScreenControlHal::ScreenControlHal(ScreenControlService * control):
mScreenControl(control),
mDeathRecipient(new DeathRecipient(this)) {
    ALOGI("ScreenControlHal setListener ");
    control->setListener(this);
}

ScreenControlHal::~ScreenControlHal() {
}
Return<void> ScreenControlHal::setCallback(const sp<IScreenControlCallback>& callback)  {
    if (callback != nullptr) {
        ALOGI("setCallback ");
        mCallBack = callback;
    }
    return Void();
}

Return<void> ScreenControlHal::startScreenCapBuffer(int32_t left, int32_t top, int32_t right, int32_t bottom, int32_t width, int32_t height, int32_t sourceType, startScreenCapBuffer_cb _cb) {
    sp<IAllocator> allocator = IAllocator::getService("ashmem");
    allocator->allocate((uint64_t)width*(uint64_t)height*4, [&](bool success, const hidl_memory& mem) {
        int bufSize = 0;
        int ret = android::OK;
        if (success) {
            sp<IMemory> memory = mapMemory(mem);
            void* data = memory->getPointer();
            memory->update();
            ret = mScreenControl->startScreenCapBuffer(left, top, right, bottom,
                                                 width, height, sourceType, data, &bufSize);
            memory->commit();
            _cb(ret,mem);
        } else {
            ALOGI("alloc memory Fail");
            _cb(android::AML_ERROR_CODE_OTHER, mem);
        }
    });
    return Void();
}

Return<Result> ScreenControlHal::startScreenRecord(int32_t left, int32_t top, int32_t right, int32_t bottom, int32_t width,
                                            int32_t height, int32_t frameRate, int32_t bitRate, int32_t limitTimeSec,
                                            int32_t sourceType, const hidl_string& filename) {
    Mutex::Autolock autoLock(mLock);
    if ( NULL != mScreenControl) {
        std::string filenamestr = filename;
        if (android::OK == mScreenControl->startScreenRecord(left, top, right, bottom, width, height, frameRate, bitRate, limitTimeSec, sourceType, filenamestr.c_str()))
            return Result::OK;
    }
    return Result::FAIL;
}

Return<Result> ScreenControlHal::startAvcRecord(int32_t left, int32_t top, int32_t right, int32_t bottom, int32_t width, int32_t height,
                                        int32_t frameRate, int32_t bitRate, int32_t sourceType)
{
    Mutex::Autolock autoLock(mLock);
    if ( NULL != mScreenControl) {
        if (android::OK == mScreenControl->startAvcRecord(left, top, right, bottom, width, height, frameRate, bitRate, sourceType))
            return Result::OK;
    }
    return Result::FAIL;
}

Return<void> ScreenControlHal::forceStop() {
    if (NULL != mScreenControl) {
        mScreenControl->forceStop();
    }
    mCallBack = nullptr;
    return Void();
}

void ScreenControlHal::onEsBufferAvailable(void* data, int32_t size, int32_t frame_type, int64_t pts) {
    if (mCallBack) {
        sp<IAllocator> allocator = IAllocator::getService("ashmem");
        allocator->allocate(size, [&](bool success, const hidl_memory& mem) {
            int bufSize = 0;
            int ret = android::OK;
            if (success) {
                sp<IMemory> memory = mapMemory(mem);
                void* pointer = memory->getPointer();
                memcpy(pointer,data,size);
                memory->update();
                mCallBack->onAvcDataArouse(mem,size,frame_type,pts);
                memory->commit();
            } else {
                ALOGI("alloc memory Fail");
            }
        });
    }

}

Return<Result> ScreenControlHal::startYuvRecord(int32_t left, int32_t top, int32_t right, int32_t bottom,
                                    int32_t width, int32_t height,int32_t frameRate, int32_t sourceType) {
    Mutex::Autolock autoLock(mLock);
     if ( NULL != mScreenControl) {
        if (android::OK == mScreenControl->startYuvRecord(left, top, right, bottom, width, height, frameRate, sourceType))
            return Result::OK;
    }
    return Result::FAIL;
}

void ScreenControlHal::onYuvBufferAvailable(void* data, int32_t size) {
    if (mCallBack) {
        sp<IAllocator> allocator = IAllocator::getService("ashmem");
        allocator->allocate(size, [&](bool success, const hidl_memory& mem) {
            int bufSize = 0;
            int ret = android::OK;
            if (success) {
                sp<IMemory> memory = mapMemory(mem);
                void* pointer = memory->getPointer();
                memcpy(pointer,data,size);
                memory->update();
                mCallBack->onYuvDataArouse(mem,size);
                memory->commit();
            } else {
                ALOGI("alloc memory Fail");
            }
        });
    }

}

Return<Result> ScreenControlHal::startMicroDim(int32_t width, int32_t height) {
    Mutex::Autolock autoLock(mLock);
    if ( NULL != mScreenControl) {
        if (android::OK == mScreenControl->startMicroDim( width, height))
            return Result::OK;
    }
    return Result::FAIL;
}

void ScreenControlHal::onMicroDimAvailable(void* data, int32_t size) {
    if (mCallBack) {
        sp<IAllocator> allocator = IAllocator::getService("ashmem");
        allocator->allocate(size, [&](bool success, const hidl_memory& mem) {
            int bufSize = 0;
            int ret = android::OK;
            if (success) {
                sp<IMemory> memory = mapMemory(mem);
                void* pointer = memory->getPointer();
                memcpy(pointer,data,size);
                memory->update();
                mCallBack->onMicroDimArouse(mem,size);
                memory->commit();
            } else {
                ALOGI("alloc memory Fail");
            }
        });
    }

}


void ScreenControlHal::handleServiceDeath(uint32_t cookie) {
    Mutex::Autolock autoLock(mLock);
    ALOGE("screencontrolservice handleServiceDeath cookie:%d",(int)cookie);
    mCallBack = nullptr;
}
ScreenControlHal::DeathRecipient::DeathRecipient(sp<ScreenControlHal> sch):mScreenControlHal(std::move(sch)) {}

void ScreenControlHal::DeathRecipient::serviceDied(uint64_t cookie,
                const ::android::wp<::android::hidl::base::V1_0::IBase>& ) {
    ALOGE("screencontrolservice daemon client died cookie:%d",(int)cookie);
    uint32_t type = static_cast<uint32_t>(cookie);
    mScreenControlHal->handleServiceDeath(type);
}

} //namespace implementation
}//namespace V1_0
} //namespace screencontrol
}//namespace hardware
} //namespace android
} //namespace vendor
