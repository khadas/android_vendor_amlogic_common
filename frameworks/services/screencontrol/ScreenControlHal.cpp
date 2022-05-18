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
    using android::Mutex;

    ScreenControlHal::ScreenControlHal() {
        mScreenControl = new ScreenControlService();
        mYuvRecordWidth = 0;
        mYuvRecordHeight = 0;
    }

    ScreenControlHal::~ScreenControlHal() {
        delete mScreenControl;
    }

    Return<Result> ScreenControlHal::startScreenRecord(int32_t width, int32_t height, int32_t frameRate, int32_t bitRate, int32_t limitTimeSec, int32_t sourceType, const hidl_string& filename) {
        Mutex::Autolock autoLock(mLock);
        if ( NULL != mScreenControl) {
            std::string filenamestr = filename;
            if (android::OK == mScreenControl->startScreenRecord(width, height, frameRate, bitRate, limitTimeSec, sourceType, filenamestr.c_str()))
                return Result::OK;
        }
        return Result::FAIL;
    }
    Return<Result> ScreenControlHal::startScreenRecordByCrop(int32_t left, int32_t top, int32_t right, int32_t bottom, int32_t width, int32_t height, int32_t frameRate, int32_t bitRate, int32_t limitTimeSec, int32_t sourceType, const hidl_string& filename) {
        Mutex::Autolock autoLock(mLock);
        if ( NULL != mScreenControl) {
            std::string filenamestr = filename;
            if (android::OK != mScreenControl->setScreenRecordCropArea(left, top, right, bottom ) )
                return Result::FAIL;
            if (android::OK == mScreenControl->startScreenRecord(width, height, frameRate, bitRate, limitTimeSec, sourceType, filenamestr.c_str()))
                return Result::OK;
        }
        return Result::FAIL;
    }

    Return<Result>  ScreenControlHal::startScreenCap(int32_t left, int32_t top, int32_t right, int32_t bottom, int32_t width, int32_t height, int32_t sourceType, const hidl_string& filename) {
        Mutex::Autolock autoLock(mLock);
        if ( NULL != mScreenControl) {
            std::string filenamestr = filename;
            if (android::OK == mScreenControl->startScreenCap(left, top, right, bottom, width, height, sourceType, filenamestr.c_str()))
                return Result::OK;
        }
        return Result::FAIL;
    }

    Return<void> ScreenControlHal::startScreenCapBuffer(int32_t left, int32_t top, int32_t right, int32_t bottom, int32_t width, int32_t height, int32_t sourceType, startScreenCapBuffer_cb _cb) {
        Mutex::Autolock autoLock(mLock);
        sp<IAllocator> allocator = IAllocator::getService("ashmem");
        allocator->allocate(width*height*4, [&](bool success, const hidl_memory& mem) {
            int bufSize = 0;
            int ret = android::OK;
            if (success) {
                sp<IMemory> memory = mapMemory(mem);
                void* data = memory->getPointer();
                memory->update();
                ret = mScreenControl->startScreenCapBuffer(left, top, right, bottom,
                        width, height, sourceType, data, &bufSize);
                memory->commit();
                if (android::OK == ret) {
                    _cb(Result::OK, mem);
                } else {
                    _cb(Result::FAIL, mem);
                }
            } else {
                ALOGI("alloc memory Fail");
                _cb(Result::FAIL, mem);
            }
        });
        return Void();
    }

    Return<void> ScreenControlHal::forceStop() {
        if (NULL != mScreenControl) {
            mScreenControl->forceStop();
        }
        return Void();
    }

    Return<Result> ScreenControlHal::startAvcRecord(int32_t width, int32_t height, int32_t frameRate, int32_t bitRate, int32_t sourceType) {
        Mutex::Autolock autoLock(mLock);
        if (NULL != mScreenControl) {
            if (android::OK == mScreenControl->startAvcRecord(width, height, frameRate, bitRate, sourceType)) {
                mAvcRecordWidth = width;
                mAvcRecordHeight = height;
                mAvcRecordFramerate = frameRate;
                mAvcRecordBitrate = bitRate;
                mAvcRecordSourceType = sourceType;
                return Result::OK;
            }
        }
        return Result::FAIL;
    }
    Return<void> ScreenControlHal::getAvcRecordData(getAvcRecordData_cb _hidl_cb) {
        Mutex::Autolock autoLock(mLock);
        if (mAvcRecordWidth > 0 && mAvcRecordHeight > 0 && mScreenControl->isHaveAvcDate()) {
            sp<IAllocator> allocator = IAllocator::getService("ashmem");
            allocator->allocate(mAvcRecordWidth*mAvcRecordHeight*4, [&](bool success, const hidl_memory& mem) {
                int bufSize = 0;
                uint8_t frameType = AVC_TYPE_FRAME_TYPE_UNKNOWN;
                int64_t nowtime = 0;
                int ret = android::OK;
                if (success) {
                    sp<IMemory> memory = mapMemory(mem);
                    void* data = memory->getPointer();
                    memory->update();
                    ret = mScreenControl->getAvcRecordData(data, &bufSize,&nowtime);
                    memory->commit();
                    if (android::OK == ret) {
                        uint8_t *prt =new uint8_t[bufSize];
                        memcpy(prt, memory->getPointer(), bufSize);
                        uint8_t naltype = (*(prt+4)) & 0x1F;
                        ALOGD("ScreenControlHal::getAvcRecordData naltype= %d",naltype);
                        switch (naltype) {
                            case NAL_SLICE: {
                                NALU_t nal[1];
                                if (prt[0]== 0x00 && prt[1]== 0x00 && prt[2]== 0x00 && prt[3]== 0x01 ) {
                                    nal->startcodeprefix_len = 4;
                                    nal->buf = prt + 4;
                                    nal->len = bufSize - 4;
                                }else if (prt[0]== 0x00 && prt[1]== 0x00 && prt[2]== 0x01) {
                                    nal->startcodeprefix_len = 3;
                                    nal->buf = prt + 3;
                                    nal->len = bufSize - 3;
                                }
                                nal->nal_unit_type = naltype;
                                int ret =GetFrameType(nal);
                                if (ret < 1) {
                                    frameType = AVC_TYPE_FRAME_TYPE_UNKNOWN;
                                    ALOGE(" find frame type error !!");
                                    break;
                                }
                                switch (nal->Frametype) {
                                    case FRAME_I:
                                        frameType = AVC_TYPE_FRAME_TYPE_FRAME_I;
                                        break;
                                    case FRAME_P:
                                        frameType = AVC_TYPE_FRAME_TYPE_FRAME_P;
                                        break;
                                    case FRAME_B:
                                        frameType = AVC_TYPE_FRAME_TYPE_FRAME_B;
                                        break;
                                    default:
                                        frameType = AVC_TYPE_FRAME_TYPE_UNKNOWN;
                                        break;
                                }
                                break;
                            }
                            case NAL_SLICE_DPA :
                                frameType = AVC_TYPE_FRAME_TYPE_SLICE_A;
                                break;
                            case NAL_SLICE_DPB:
                                frameType = AVC_TYPE_FRAME_TYPE_SLICE_B;
                                break;
                            case NAL_SLICE_DPC:
                                frameType = AVC_TYPE_FRAME_TYPE_SLICE_C;
                                break;
                            case NAL_SLICE_IDR:
                                frameType = AVC_TYPE_FRAME_TYPE_IDR;
                                break;
                            case NAL_SEI:
                                frameType = AVC_TYPE_FRAME_TYPE_SEI;
                                break;
                            case NAL_SPS:
                                frameType = AVC_TYPE_FRAME_TYPE_SPS;
                                break;
                            case NAL_PPS:
                                frameType = AVC_TYPE_FRAME_TYPE_PPS;
                                break;
                            case NAL_DELIMITER:
                                frameType = AVC_TYPE_FRAME_TYPE_DELIMITER;
                                break;
                            case NAL_SEQUENCE_END:
                                frameType = AVC_TYPE_FRAME_TYPE_SEQUENCE_END;
                                break;
                            case NAL_STEAM_END:
                                frameType = AVC_TYPE_FRAME_TYPE_STEAM_END;
                                break;
                            default:
                                frameType = AVC_TYPE_FRAME_TYPE_UNKNOWN;
                                break;
                        }
                        delete [] prt;
                        _hidl_cb(Result::OK, mem, bufSize,frameType, nowtime);
                    } else {
                        _hidl_cb(Result::FAIL, mem, bufSize,frameType, nowtime);
                    }
                } else {
                    ALOGE("alloc memory Fail");
                    _hidl_cb(Result::FAIL, mem, bufSize,frameType, nowtime);
                }
            });
        } else {
             hidl_memory m;
             const hidl_memory& m1= m;
//            ALOGE("Error width(%d) or height(%d)", mAvcRecordWidth, mAvcRecordHeight);
                  _hidl_cb(Result::FAIL, m1, 0, 0, 0);
        }
        return Void();
    }
    Return<Result> ScreenControlHal::checkAvcRecordDone() {
        Mutex::Autolock autoLock(mLock);
        if (NULL != mScreenControl) {
            if (android::OK == mScreenControl->checkAvcRecordDone())
                return Result::OK;
        }
        return Result::FAIL;
    }

    Return<Result> ScreenControlHal::startYuvRecord(int32_t width, int32_t height, int32_t frameRate,int32_t sourceType) {
        Mutex::Autolock autoLock(mLock);
        if (NULL != mScreenControl) {
            if (android::OK == mScreenControl->startYuvRecord(width, height, frameRate,sourceType)) {
                mYuvRecordWidth = width;
                mYuvRecordHeight = height;
                return Result::OK;
            }
        }
        return Result::FAIL;
    }

    Return<Result> ScreenControlHal::checkYuvRecordDone() {
      Mutex::Autolock autoLock(mLock);
        if (NULL != mScreenControl) {
       //ALOGI("enter %s", __func__);
            if (android::OK == mScreenControl->checkYuvRecordDone())
                return Result::OK;
        }
        return Result::FAIL;
    }

    Return<void> ScreenControlHal::getYuvRecordData(getYuvRecordData_cb _hidl_cb) {
        Mutex::Autolock autoLock(mLock);
        if (mYuvRecordWidth > 0 && mYuvRecordHeight > 0 && mScreenControl->isHaveYuvDate()) {
            sp<IAllocator> allocator = IAllocator::getService("ashmem");
            int bufSize =mYuvRecordWidth*mYuvRecordHeight*3/2;
            allocator->allocate(bufSize, [&](bool success, const hidl_memory& mem) {
                int ret = android::OK;
                int64_t nowtime = 0;
                if (success) {
                    sp<IMemory> memory = mapMemory(mem);
                    void* data = memory->getPointer();
                    memory->update();
                    ret = mScreenControl->getYuvRecordData(data,bufSize);
                    memory->commit();
                    if (android::OK == ret) {
                        _hidl_cb(Result::OK, mem, bufSize);
                    } else {
                        _hidl_cb(Result::FAIL, mem, 0);
                    }
                } else {
                    ALOGE("alloc memory Fail");
                    _hidl_cb(Result::FAIL, mem, 0);
                }
            });
        } else {
            hidl_memory m;
            const hidl_memory& m1= m;
            _hidl_cb(Result::FAIL, m1, 0);
        }
        return Void();
    }

    void ScreenControlHal::handleServiceDeath(uint32_t cookie) {

    }

    ScreenControlHal::DeathRecipient::DeathRecipient(sp<ScreenControlHal> sch):mScreenControlHal(sch) {}
    void ScreenControlHal::DeathRecipient::serviceDied(uint64_t cookie,
                    const ::android::wp<::android::hidl::base::V1_0::IBase>& who) {
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
