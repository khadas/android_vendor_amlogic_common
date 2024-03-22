/*
 * Copyright (C) 2022 The Android Open Source Project
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

#define LOG_TAG "ExtCamDev"
// #define LOG_NDEBUG 0
#include <log/log.h>

#include "AmlogicCameraDevice.h"

#include <aidl/android/hardware/camera/common/Status.h>
#include <convert.h>
#include <linux/videodev2.h>
#include <regex>
#include <set>

namespace android {
namespace hardware {
namespace camera {
namespace device {
namespace implementation {

using ::aidl::android::hardware::camera::common::Status;

AmlogicCameraDevice::AmlogicCameraDevice(std::shared_ptr<CameraModule> module, const std::string& cameraId,
                              const SortedVector<std::pair<std::string, std::string>>& cameraDeviceNames)
    :   mModule(module),
        mCameraId(cameraId),
        mDisconnected(false),
        mCameraDeviceNames(cameraDeviceNames){
    mCameraIdInt = atoi(mCameraId.c_str());
    if (mCameraIdInt < 0) {
        ALOGE("%s: Invalid camera id: %s", __FUNCTION__, mCameraId.c_str());
        mInitFail = true;
    } else if (mCameraIdInt >= mModule->getNumberOfCameras()) {
        ALOGE("%s: Adding a new camera id: %s", __FUNCTION__, mCameraId.c_str());
    }

    mDeviceVersion = mModule->getDeviceVersion(mCameraIdInt);
    if (mDeviceVersion < CAMERA_DEVICE_API_VERSION_3_2) {
        ALOGE("%s: Camera id %s does not support HAL3.2+",
                __FUNCTION__, mCameraId.c_str());
        mInitFail = true;
    }

    // mAshmemAllocator = IAllocator::getService("ashmem");
    // if (mAshmemAllocator == nullptr) {
    //     ALOGI("%s: cannot get ashmemAllocator", __FUNCTION__);
    //     mInitFail = true;
    // }
}

AmlogicCameraDevice::~AmlogicCameraDevice() {}

Status AmlogicCameraDevice::initStatus() const {
    Mutex::Autolock _l(mLock);
    Status status = Status::OK;
    if (mInitFail) {
        status = Status::INTERNAL_ERROR;
    } else if (mDisconnected) {
        status = Status::CAMERA_DISCONNECTED;
    }
    return status;
}

Status AmlogicCameraDevice::getAidlStatus(int status) {

    switch (status) {
        case 0: return Status::OK;
        case -ENOSYS: return Status::OPERATION_NOT_SUPPORTED;
        case -EBUSY : return Status::CAMERA_IN_USE;
        case -EUSERS: return Status::MAX_CAMERAS_IN_USE;
        case -ENODEV: return Status::INTERNAL_ERROR;
        case -EINVAL: return Status::ILLEGAL_ARGUMENT;
        default:
            ALOGE("%s: unknown HAL status code %d", __FUNCTION__, status);
            return Status::INTERNAL_ERROR;
    }
}

/***
 * Override the method to be implemented
*/

ndk::ScopedAStatus AmlogicCameraDevice::getCameraCharacteristics(CameraMetadata* _aidl_return) {
    if (_aidl_return == nullptr) {
        return fromStatus(Status::ILLEGAL_ARGUMENT);
    }

    Status status = initStatus();
    CameraMetadata cameraCharacteristics;
    if (status == Status::OK) {
        struct camera_info info;
        int ret = mModule->getCameraInfo(mCameraIdInt, &info);
        if (ret == OK)
        {
            ALOGI("convert metadata to aidl");
            convertToAidl(info.static_camera_characteristics, _aidl_return);
        } else {
            ALOGE("%s: get camera info failed!", __FUNCTION__);
            status = Status::INTERNAL_ERROR;
        }
    }
    //const camera_metadata_t* rawMetadata = mCameraCharacteristics.getAndLock();
    //convertToAidl(rawMetadata, _aidl_return);
    //mCameraCharacteristics.unlock(rawMetadata);
    return fromStatus(status);
}

ndk::ScopedAStatus AmlogicCameraDevice::getPhysicalCameraCharacteristics(const std::string&,
                                                                          CameraMetadata*) {
    ALOGE("%s: Physical camera functions are not supported for external cameras.", __FUNCTION__);
    return fromStatus(Status::ILLEGAL_ARGUMENT);
}

ndk::ScopedAStatus AmlogicCameraDevice::getResourceCost(CameraResourceCost* _aidl_return) {
    if (_aidl_return == nullptr) {
        return fromStatus(Status::ILLEGAL_ARGUMENT);
    }

    Status status = initStatus();
    CameraResourceCost resCost;
    if (status == Status::OK) {
        int cost = 100;
        std::vector<std::string> conflicting_devices;
        struct camera_info info;
        if (mModule->getModuleApiVersion() >= CAMERA_MODULE_API_VERSION_2_4) {
            int ret = mModule->getCameraInfo(mCameraIdInt, &info);
            if (ret == OK) {
                cost = info.resource_cost;
                for (size_t i = 0; i < info.conflicting_devices_length; i++) {
                    std::string cameraId(info.conflicting_devices[i]);
                    for (const auto& pair : mCameraDeviceNames) {
                        if (cameraId == pair.first) {
                            conflicting_devices.push_back(pair.second);
                        }
                    }
                }
            } else {
                status = Status::INTERNAL_ERROR;
            }
        }

         if (status == Status::OK) {
            resCost.resourceCost = cost;
            resCost.conflictingDevices.resize(conflicting_devices.size());
            for (size_t i = 0; i < conflicting_devices.size(); i++) {
                resCost.conflictingDevices[i] = conflicting_devices[i];
                ALOGV("CamDevice %s is conflicting with camDevice %s",
                        mCameraId.c_str(), resCost.conflictingDevices[i].c_str());
            }
        }
    }

    _aidl_return->resourceCost = resCost.resourceCost;
    return fromStatus(Status::OK);
}

ndk::ScopedAStatus AmlogicCameraDevice::isStreamCombinationSupported(
        const StreamConfiguration& in_streams, bool* _aidl_return) {
    ALOGE("%d", in_streams.streamConfigCounter);
    Status status;
    status_t res;
    camera_stream_combination_t streamComb{};
    streamComb.operation_mode = static_cast<uint32_t> (in_streams.operationMode);
    streamComb.num_streams = in_streams.streams.size();
    camera_stream_t *streamBuffer  = new camera_stream_t[streamComb.num_streams];
    size_t i = 0;
    for (const auto &it : in_streams.streams) {

        if (it.useCase != aidl::android::hardware::camera::metadata::ScalerAvailableStreamUseCases::ANDROID_SCALER_AVAILABLE_STREAM_USE_CASES_DEFAULT) {
            *_aidl_return = false;
            return fromStatus(Status::OK);
        }

        streamBuffer[i].stream_type = static_cast<int> (it.streamType);
        streamBuffer[i].width = it.width;
        streamBuffer[i].height = it.height;
        streamBuffer[i].format = static_cast<int> (it.format);
        streamBuffer[i].data_space = static_cast<android_dataspace_t> (it.dataSpace);
        streamBuffer[i].usage = static_cast<uint32_t> (it.usage);
        streamBuffer[i].physical_camera_id = it.physicalCameraId.c_str();
        streamBuffer[i++].rotation = static_cast<int> (it.rotation);
    }
    streamComb.streams = streamBuffer;
    res = mModule->isStreamCombinationSupported(mCameraIdInt, &streamComb);

    status = getAidlStatus(res);
    delete [] streamBuffer;
    *_aidl_return = status == Status::OK;
    return fromStatus(Status::OK);
}

ndk::ScopedAStatus AmlogicCameraDevice::open(
        const std::shared_ptr<ICameraDeviceCallback>& in_callback,
        std::shared_ptr<ICameraDeviceSession>* _aidl_return) {
    if (_aidl_return == nullptr) {
        ALOGE("%s: cannot open camera %s. return session ptr is null!", __FUNCTION__,
              mCameraId.c_str());
        return fromStatus(Status::ILLEGAL_ARGUMENT);
    }
    if (in_callback == nullptr) {
        ALOGE("%s: cannot open camera %s. callback is null!",
                __FUNCTION__, mCameraId.c_str());
        return fromStatus(Status::ILLEGAL_ARGUMENT);
    }

    Status status = initStatus();

    if (status != Status::OK) {
        ALOGE("%s: cannot open camera %s. camera is disconnected!",
                __FUNCTION__, mCameraId.c_str());
        return fromStatus(Status::INTERNAL_ERROR);
    }
    mLock.lock();
    // if (isInitFailedLocked()) {
    //     ALOGE("%s: cannot open camera %s. camera init failed!", __FUNCTION__, mCameraId.c_str());
    //     return fromStatus(Status::INTERNAL_ERROR);
    // }

    std::shared_ptr<AmlogicCameraDeviceSession> session;
    ALOGV("%s: Initializing device for camera %s", __FUNCTION__, mCameraId.c_str());
    session = mSession;

    if (session != nullptr && !session->isClosed()) {
        ALOGE("%s: cannot open an already opened camera!", __FUNCTION__);
        mLock.unlock();
        return fromStatus(Status::CAMERA_IN_USE);
    }

    status_t res;
    aml_camera_device_t *device;

    res = mModule->open(mCameraId.c_str(),
                reinterpret_cast<hw_device_t**>(&device));

    if (res != OK) {
        ALOGE("%s: cannot open camera %s!", __FUNCTION__, mCameraId.c_str());
        mLock.unlock();
        return fromStatus(getAidlStatus(res));
    }

    if (device->common.version < CAMERA_DEVICE_API_VERSION_3_2) {
        ALOGE("%s: Could not open camera: "
                "Camera device should be at least %x, reports %x instead",
                __FUNCTION__,
                CAMERA_DEVICE_API_VERSION_3_2,
                device->common.version);
        device->common.close(&device->common);
        mLock.unlock();
        return fromStatus(Status::ILLEGAL_ARGUMENT);
    }

    struct camera_info info;
    res = mModule->getCameraInfo(mCameraIdInt, &info);
    if (res != OK) {
        ALOGE("%s: Could not open camera: getCameraInfo failed", __FUNCTION__);
        device->common.close(&device->common);
        mLock.unlock();
        return fromStatus(Status::ILLEGAL_ARGUMENT);
    }

    session = createSession(
                device, info.static_camera_characteristics, in_callback);
    if (session == nullptr) {
        ALOGE("%s: camera device session allocation failed", __FUNCTION__);
        mLock.unlock();
        return fromStatus(Status::INTERNAL_ERROR);
    }

    if (session->isInitFailed()) {
        ALOGE("%s: camera device session init failed", __FUNCTION__);
        session = nullptr;
        mLock.unlock();
        return fromStatus(Status::INTERNAL_ERROR);
    }

    mSession = session;
    // IF_ALOGV() {
    //     session->getInterface()->interfaceChain([](
    //         ::android::hardware::hidl_vec<::android::hardware::hidl_string> interfaceChain) {
    //             ALOGV("Session interface chain:");
    //             for (const auto& iface : interfaceChain) {
    //                 ALOGV("  %s", iface.c_str());
    //             }
    //         });
    // }
    mLock.unlock();

    *_aidl_return = session;
    return fromStatus(Status::OK);
}

ndk::ScopedAStatus AmlogicCameraDevice::openInjectionSession(
        const std::shared_ptr<ICameraDeviceCallback>&, std::shared_ptr<ICameraInjectionSession>*) {
    return fromStatus(Status::OPERATION_NOT_SUPPORTED);
}

ndk::ScopedAStatus AmlogicCameraDevice::setTorchMode(bool) {
    return fromStatus(Status::OPERATION_NOT_SUPPORTED);
}

ndk::ScopedAStatus AmlogicCameraDevice::turnOnTorchWithStrengthLevel(int32_t) {
    return fromStatus(Status::OPERATION_NOT_SUPPORTED);
}

ndk::ScopedAStatus AmlogicCameraDevice::getTorchStrengthLevel(int32_t*) {
    return fromStatus(Status::OPERATION_NOT_SUPPORTED);
}



}  // namespace implementation
}  // namespace device
}  // namespace camera
}  // namespace hardware
}  // namespace android