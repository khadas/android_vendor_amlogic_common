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

#define LOG_TAG "ExtCamPrvdr"
// #define LOG_NDEBUG 0

#include "AmlogicCameraProvider.h"

#include <AmlogicCameraDevice.h>
#include <aidl/android/hardware/camera/common/Status.h>
#include <convert.h>
#include <cutils/properties.h>
#include <linux/videodev2.h>
#include <log/log.h>
#include <sys/inotify.h>
#include <regex>

namespace android {
namespace hardware {
namespace camera {
namespace provider {
namespace implementation {

using ::aidl::android::hardware::camera::common::Status;
using ::android::hardware::camera::device::implementation::AmlogicCameraDevice;
using ::android::hardware::camera::device::implementation::fromStatus;
//using ::android::hardware::camera::external::common::ExternalCameraConfig;

namespace {
// "device@<version>/external/<id>"
const std::regex kDeviceNameRE("device@([0-9]+\\.[0-9]+)/internal/(.+)");
const int kMaxCameraDeviceNameLen = 128;
const int kMaxCameraIdLen = 16;

bool matchDeviceName(const std::string& deviceName, std::string* deviceVersion,
                     std::string* cameraId) {
    std::smatch sm;
    if (std::regex_match(deviceName, sm, kDeviceNameRE)) {
        if (deviceVersion != nullptr) {
            *deviceVersion = sm[1];
        }
        if (cameraId != nullptr) {
            *cameraId = sm[2];
        }
        return true;
    }
    return false;
}
}  // namespace


void AmlogicCameraProvider::removeDeviceNames(int camera_id)
{
    std::string cameraIdStr = std::to_string(camera_id);

    mCameraIds.remove(cameraIdStr);

    int deviceVersion = mModule->getDeviceVersion(camera_id);
    auto deviceNamePair = std::make_pair(cameraIdStr,
                                         getAidlDeviceName(cameraIdStr, deviceVersion));
    mCameraDeviceNames.remove(deviceNamePair);
    mCallbacks->cameraDeviceStatusChange(deviceNamePair.second, CameraDeviceStatus::NOT_PRESENT);
    if (deviceVersion >= CAMERA_DEVICE_API_VERSION_3_2 &&
        mModule->isOpenLegacyDefined() && mOpenLegacySupported[cameraIdStr]) {

        deviceNamePair = std::make_pair(cameraIdStr,
                            getAidlDeviceName(cameraIdStr, CAMERA_DEVICE_API_VERSION_1_0));
        mCameraDeviceNames.remove(deviceNamePair);
        mCallbacks->cameraDeviceStatusChange(deviceNamePair.second,
                                             CameraDeviceStatus::NOT_PRESENT);
    }

    mModule->removeCamera(camera_id);
}

void AmlogicCameraProvider::addDeviceNames(int camera_id, CameraDeviceStatus status, bool cam_new){
    char cameraId[kMaxCameraIdLen];
    snprintf(cameraId, sizeof(cameraId), "%d", camera_id);
    std::string cameraIdStr(cameraId);

    mCameraIds.add(cameraIdStr);
    mOpenLegacySupported[cameraIdStr] = false;
    int deviceVersion = mModule->getDeviceVersion(camera_id);
    auto deviceNamePair = std::make_pair(cameraIdStr,
                                         getAidlDeviceName(cameraIdStr, deviceVersion));
    mCameraDeviceNames.add(deviceNamePair);
    if (cam_new)
    {
        mCallbacks->cameraDeviceStatusChange(deviceNamePair.second, status);
    }
    if (deviceVersion >= CAMERA_DEVICE_API_VERSION_3_2 && mModule->isOpenLegacyDefined()) {
        // try open_legacy to see if it actually works
        struct hw_device_t* halDev = nullptr;
        int ret = mModule->openLegacy(cameraId, CAMERA_DEVICE_API_VERSION_1_0, &halDev);
        if (ret == 0) {
            mOpenLegacySupported[cameraIdStr] = true;
            halDev->close(halDev);
            deviceNamePair = std::make_pair(cameraIdStr,
                            getAidlDeviceName(cameraIdStr, CAMERA_DEVICE_API_VERSION_1_0));
            mCameraDeviceNames.add(deviceNamePair);
            if (cam_new) {
                mCallbacks->cameraDeviceStatusChange(deviceNamePair.second, status);
            }
        } else if (ret == -EBUSY || ret == -EUSERS) {
            // Looks like this provider instance is not initialized during
            // system startup and there are other camera users already.
            // Not a good sign but not fatal.
            ALOGW("%s: open_legacy try failed!", __FUNCTION__);
        }
    }

}
AmlogicCameraProvider::AmlogicCameraProvider() :
    camera_module_callbacks_t({sCameraDeviceStatusChange,
                                   sTorchModeStatusChange}) {
    mInitFailed = initialize();
}

AmlogicCameraProvider::~AmlogicCameraProvider() {}

bool AmlogicCameraProvider::initialize() {
    camera_module_t* rawModule;
    int err = hw_get_module(CAMERA_HARDWARE_MODULE_ID, ((const hw_module_t **)&rawModule));
    if (err < 0) {
        ALOGE("Could not load camera HAL module: %d (%s)", err, strerror(-err));
        return true;
    }
    mModule.reset(new CameraModule(rawModule));
    err = mModule->init();
    if (err != OK) {
        ALOGE("Could not initialize camera HAL module: %d (%s)", err, strerror(-err));
        mModule.reset();
        return true;
    }
    // ALOGI("Loaded \"%s\" camera module", mModule->getModuleName());
    // VendorTagDescriptor::clearGlobalVendorTagDescriptor();
    // if (!setUpVendorTags()) {
    //     ALOGE("%s: Vendor tag setup failed, will not be available.", __FUNCTION__);
    // }
    err = mModule->setCallbacks(this);
    if (err != OK) {
        ALOGE("Could not set camera module callback: %d (%s)", err, strerror(-err));
        mModule.reset();
        return true;
    }
    mNumberOfLegacyCameras = mModule->getNumberOfCameras();
    for (int i = 0; i < mNumberOfLegacyCameras; i++) {
        struct camera_info info;
        auto rc = mModule->getCameraInfo(i, &info);
        if (rc != NO_ERROR) {
            ALOGE("%s: Camera info query failed!", __func__);
            mModule.reset();
            return true;
        }

        if (checkCameraVersion(i, info) != OK) {
            ALOGE("%s: Camera version check failed!", __func__);
            mModule.reset();
            return true;
        }

        char cameraId[kMaxCameraIdLen];
        snprintf(cameraId, sizeof(cameraId), "%d", i);
        std::string cameraIdStr(cameraId);
        mCameraStatusMap[cameraIdStr] = CameraDeviceStatus::PRESENT;

        addDeviceNames(i);
    }
    return false; // mInitFailed
}
/**
 * Check that the device HAL version is still in supported.
 */
int AmlogicCameraProvider::checkCameraVersion(int id, camera_info info) {
    if (mModule == nullptr) {
        return NO_INIT;
    }

    // device_version undefined in CAMERA_MODULE_API_VERSION_1_0,
    // All CAMERA_MODULE_API_VERSION_1_0 devices are backward-compatible
    uint16_t moduleVersion = mModule->getModuleApiVersion();
    if (moduleVersion >= CAMERA_MODULE_API_VERSION_2_0) {
        // Verify the device version is in the supported range
        switch (info.device_version) {
            case CAMERA_DEVICE_API_VERSION_1_0:
            case CAMERA_DEVICE_API_VERSION_3_2:
            case CAMERA_DEVICE_API_VERSION_3_3:
            case CAMERA_DEVICE_API_VERSION_3_4:
            case CAMERA_DEVICE_API_VERSION_3_5:
                // in support
                break;
            case CAMERA_DEVICE_API_VERSION_3_6:
                /**
                 * ICameraDevice@3.5 contains APIs from both
                 * CAMERA_DEVICE_API_VERSION_3_6 and CAMERA_MODULE_API_VERSION_2_5
                 * so we require HALs to uprev both for simplified supported combinations.
                 * HAL can still opt in individual new APIs independently.
                 */
                if (moduleVersion < CAMERA_MODULE_API_VERSION_2_5) {
                    ALOGE("%s: Device %d has unsupported version combination:"
                            "HAL version %x and module version %x",
                            __FUNCTION__, id, info.device_version, moduleVersion);
                    return NO_INIT;
                }
                break;
            case CAMERA_DEVICE_API_VERSION_2_0:
            case CAMERA_DEVICE_API_VERSION_2_1:
            case CAMERA_DEVICE_API_VERSION_3_0:
            case CAMERA_DEVICE_API_VERSION_3_1:
                // no longer supported
            default:
                ALOGE("%s: Device %d has HAL version %x, which is not supported",
                        __FUNCTION__, id, info.device_version);
                return NO_INIT;
        }
    }

    return OK;
}

// bool AmlogicCameraProvider::setUpVendorTags() {
//     ATRACE_CALL();
//     vendor_tag_ops_t vOps = vendor_tag_ops_t();

//      // Check if vendor operations have been implemented
//     if (!mModule->isVendorTagDefined()) {
//         ALOGI("%s: No vendor tags defined for this device.", __FUNCTION__);
//         return true;
//     }

//     mModule->getVendorTagOps(&vOps);

//     if (vOps.get_tag_count == nullptr || vOps.get_all_tags == nullptr ||
//             vOps.get_section_name == nullptr || vOps.get_tag_name == nullptr ||
//             vOps.get_tag_type == nullptr) {
//         ALOGE("%s: Vendor tag operations not fully defined. Ignoring definitions."
//                , __FUNCTION__);
//         return false;
//     }

//     std::shared_ptr<VendorTagDescriptor> desc;
//     status_t res;
//     if ((res = VendorTagDescriptor::createDescriptorFromOps(&vOps, /*out*/desc))
//             != OK) {
//         ALOGE("%s: Could not generate descriptor from vendor tag operations,"
//               "received error %s (%d). Camera clients will not be able to use"
//               "vendor tags", __FUNCTION__, strerror(res), res);
//         return false;
//     }
//     // Set the global descriptor to use with camera metadata
//     VendorTagDescriptor::setAsGlobalVendorTagDescriptor(desc);
//     const SortedVector<String8>* sectionNames = desc->getAllSectionNames();
//     size_t numSections = sectionNames->size();
//     std::vector<std::vector<VendorTag>> tagsBySection(numSections);
//     int tagCount = desc->getTagCount();
//     std::vector<uint32_t> tags(tagCount);
//     desc->getTagArray(tags.data());
//     for (int i = 0; i < tagCount; i++) {
//         VendorTag vt;
//         vt.tagId = tags[i];
//         vt.tagName = desc->getTagName(tags[i]);
//         vt.tagType = (CameraMetadataType) desc->getTagType(tags[i]);
//         ssize_t sectionIdx = desc->getSectionIndex(tags[i]);
//         tagsBySection[sectionIdx].push_back(vt);
//     }
//     mVendorTagSections.resize(numSections);
//     for (size_t s = 0; s < numSections; s++) {
//         mVendorTagSections[s].sectionName = (*sectionNames)[s].string();
//         mVendorTagSections[s].tags = tagsBySection[s];
//     }
//     return true;
// }


void AmlogicCameraProvider::sCameraDeviceStatusChange(
        const struct camera_module_callbacks* callbacks,
        int camera_id,
        int new_status) {
    AmlogicCameraProvider* cp = const_cast<AmlogicCameraProvider*>(
        static_cast<const AmlogicCameraProvider*>(callbacks));
    if (cp == nullptr)
    {
        ALOGE("%s: callback ops is null", __FUNCTION__);
        return;
    }

    Mutex::Autolock _l(cp->mLock);
    char cameraId[kMaxCameraIdLen];
    snprintf(cameraId, sizeof(cameraId), "%d", camera_id);
    std::string cameraIdStr(cameraId);
    cp->mCameraStatusMap[cameraIdStr] = (CameraDeviceStatus) new_status;

    if (cp->mCallbacks == nullptr) {
        // For camera connected before mCallbacks is set, the corresponding
        // addDeviceNames() would be called later in setCallbacks().
        return;
    }

    bool found = false;
    CameraDeviceStatus status = (CameraDeviceStatus) new_status;
    for (auto const& deviceNamePair : cp->mCameraDeviceNames) {
        if (cameraIdStr.compare(deviceNamePair.first) == 0) {
            cp->mCallbacks->cameraDeviceStatusChange(deviceNamePair.second, status);
            found = true;
        }
    }

    switch (status) {
        case CameraDeviceStatus::PRESENT:
        case CameraDeviceStatus::ENUMERATING:
            if (!found) {
                cp->addDeviceNames(camera_id, status, true);
            }
            break;
        case CameraDeviceStatus::NOT_PRESENT:
            if (found) {
                cp->removeDeviceNames(camera_id);
            }
    }

}
void AmlogicCameraProvider::sTorchModeStatusChange(
        const struct camera_module_callbacks* callbacks,
        const char* camera_id,
        int new_status) {
    AmlogicCameraProvider* cp = const_cast<AmlogicCameraProvider*>(
        static_cast<const AmlogicCameraProvider*>(callbacks));

    Mutex::Autolock _l(cp->mLock);
    if (cp->mCallbacks != nullptr) {
        std::string cameraIdStr(camera_id);
        TorchModeStatus status = (TorchModeStatus) new_status;
        for (auto const& deviceNamePair : cp->mCameraDeviceNames) {
            if (cameraIdStr.compare(deviceNamePair.first) == 0) {
                cp->mCallbacks->torchModeStatusChange(
                        deviceNamePair.second, status);
            }
        }
    }
}


std::string AmlogicCameraProvider::getLegacyCameraId(const std::string& deviceName) {
    std::string cameraId;
    matchDeviceName(deviceName, nullptr, &cameraId);
    return cameraId;
}

std::string AmlogicCameraProvider::getAidlDeviceName(std::string cameraId, int deviceVersion) {
    // Maybe consider create a version check method and SortedVec to speed up?
    // if (deviceVersion != CAMERA_DEVICE_API_VERSION_1_0 &&
    //         deviceVersion != CAMERA_DEVICE_API_VERSION_3_2 &&
    //         deviceVersion != CAMERA_DEVICE_API_VERSION_3_3 &&
    //         deviceVersion != CAMERA_DEVICE_API_VERSION_3_4 &&
    //         deviceVersion != CAMERA_DEVICE_API_VERSION_3_5 &&
    //         deviceVersion != CAMERA_DEVICE_API_VERSION_3_6) {
    //     return std::string("");
    // }

    // // Supported combinations:
    // // CAMERA_DEVICE_API_VERSION_1_0 -> ICameraDevice@1.0
    // // CAMERA_DEVICE_API_VERSION_3_[2-4] -> ICameraDevice@[3.2|3.3]
    // // CAMERA_DEVICE_API_VERSION_3_5 + CAMERA_MODULE_API_VERSION_2_4 -> ICameraDevice@3.4
    // // CAMERA_DEVICE_API_VERSION_3_[5-6] + CAMERA_MODULE_API_VERSION_2_5 -> ICameraDevice@3.5
    // bool isV1 = deviceVersion == CAMERA_DEVICE_API_VERSION_1_0;
    // int versionMajor = isV1 ? 1 : 3;
    // int versionMinor = isV1 ? 0 : mPreferredHal3MinorVersion;
    // if (deviceVersion == CAMERA_DEVICE_API_VERSION_3_5) {
    //     if (mModule->getModuleApiVersion() == CAMERA_MODULE_API_VERSION_2_5) {
    //         versionMinor = 5;
    //     } else {
    //         versionMinor = 4;
    //     }
    // } else if (deviceVersion == CAMERA_DEVICE_API_VERSION_3_6) {
    //     versionMinor = 5;
    // }
    ALOGI("get Aidl device name of id=%s, deviceVersion=%d", cameraId.c_str(), deviceVersion);
    char deviceName[kMaxCameraDeviceNameLen];
    int versionMajor = 1;
    int versionMinor = 1;
    snprintf(deviceName, sizeof(deviceName), "device@%d.%d/internal/%s",
            versionMajor, versionMinor, cameraId.c_str());
    return deviceName;
}

ndk::ScopedAStatus AmlogicCameraProvider::setCallback(
        const std::shared_ptr<ICameraProviderCallback>& in_callback) {
    Mutex::Autolock _l(mLock);
    mCallbacks = in_callback;

    if (mCallbacks == nullptr) {
        return fromStatus(Status::ILLEGAL_ARGUMENT);
    }

    for (const auto& statusPair : mCameraStatusMap) {
        //mCallback->cameraDeviceStatusChange(pair.first, pair.second);
        int id = std::stoi(statusPair.first);
        auto status = static_cast<CameraDeviceStatus>(statusPair.second);
        if (id >= mNumberOfLegacyCameras && status != CameraDeviceStatus::NOT_PRESENT) {
            addDeviceNames(id, status, true);
        }
    }
    return fromStatus(Status::OK);
}

ndk::ScopedAStatus AmlogicCameraProvider::getVendorTags(
        std::vector<VendorTagSection>* _aidl_return) {
    if (_aidl_return == nullptr) {
        return fromStatus(Status::ILLEGAL_ARGUMENT);
    }
    // No vendor tag support for USB camera
    *_aidl_return = {};
    return fromStatus(Status::OK);
}

ndk::ScopedAStatus AmlogicCameraProvider::getCameraIdList(std::vector<std::string>* _aidl_return) {
    if (_aidl_return == nullptr) {
        return fromStatus(Status::ILLEGAL_ARGUMENT);
    }
    // External camera HAL always report 0 camera, and extra cameras
    // are just reported via cameraDeviceStatusChange callbacks
    std::vector<std::string> deviceNameList;
    for (auto const& deviceNamePair : mCameraDeviceNames) {
        if (std::stoi(deviceNamePair.first) >= mNumberOfLegacyCameras) {
            // External camera devices must be reported through the device status change callback,
            // not in this list.
            continue;
        }
        if (mCameraStatusMap[deviceNamePair.first] == CameraDeviceStatus::PRESENT) {
            _aidl_return->push_back(deviceNamePair.second);
        }
    }
    return fromStatus(Status::OK);
}

ndk::ScopedAStatus AmlogicCameraProvider::getCameraDeviceInterface(
        const std::string& in_cameraDeviceName, std::shared_ptr<ICameraDevice>* _aidl_return) {
    if (_aidl_return == nullptr) {
        return fromStatus(Status::ILLEGAL_ARGUMENT);
    }
    std::string cameraId, deviceVersion;
    bool match = matchDeviceName(in_cameraDeviceName, &deviceVersion, &cameraId);

    if (!match) {
        *_aidl_return = nullptr;
        return fromStatus(Status::ILLEGAL_ARGUMENT);
    }

    ssize_t index = mCameraDeviceNames.indexOf(std::make_pair(cameraId, in_cameraDeviceName));
    if (index == NAME_NOT_FOUND)
    {
        Status status = Status::OK;
        ssize_t idx = mCameraIds.indexOf(cameraId);
        if (idx == NAME_NOT_FOUND) {
            ALOGE("%s: cannot find camera %s!", __FUNCTION__, cameraId.c_str());
            status = Status::ILLEGAL_ARGUMENT;
        } else { // invalid version
            ALOGE("%s: camera device %s does not support version %s!",
                    __FUNCTION__, cameraId.c_str(), deviceVersion.c_str());
            status = Status::OPERATION_NOT_SUPPORTED;
        }
        *_aidl_return = nullptr;
        return fromStatus(status);
    }

    if (mCameraStatusMap.count(cameraId) == 0 ||
        mCameraStatusMap[cameraId] != CameraDeviceStatus::PRESENT) {
        *_aidl_return = nullptr;
        return fromStatus(Status::ILLEGAL_ARGUMENT);
    }

    std::shared_ptr<AmlogicCameraDevice> deviceImpl =
            ndk::SharedRefBase::make<AmlogicCameraDevice>(mModule, cameraId, mCameraDeviceNames);
    if (deviceImpl == nullptr) {
        ALOGE("%s: cannot allocate camera device for id %s", __FUNCTION__, cameraId.c_str());
        *_aidl_return = nullptr;
        return fromStatus(Status::INTERNAL_ERROR);
    }

    if (deviceImpl->isInitFailed()) {
        ALOGE("%s: camera device %s init failed!", __FUNCTION__, cameraId.c_str());
        deviceImpl = nullptr;
        *_aidl_return = nullptr;
        return fromStatus(Status::INTERNAL_ERROR);
    }

    *_aidl_return = deviceImpl;
    return fromStatus(Status::OK);
}

ndk::ScopedAStatus AmlogicCameraProvider::notifyDeviceStateChange(int64_t) {
    return fromStatus(Status::OK);
}

ndk::ScopedAStatus AmlogicCameraProvider::getConcurrentCameraIds(
        std::vector<ConcurrentCameraIdCombination>* _aidl_return) {
    if (_aidl_return == nullptr) {
        return fromStatus(Status::ILLEGAL_ARGUMENT);
    }
    *_aidl_return = {};
    return fromStatus(Status::OK);
}

ndk::ScopedAStatus AmlogicCameraProvider::isConcurrentStreamCombinationSupported(
        const std::vector<CameraIdAndStreamCombination>&, bool* _aidl_return) {
    if (_aidl_return == nullptr) {
        return fromStatus(Status::ILLEGAL_ARGUMENT);
    }
    // No concurrent stream combinations are supported
    *_aidl_return = false;
    return fromStatus(Status::OK);
}



// Start AmlogicCameraProvider::HotplugThread function

// End AmlogicCameraProvider::HotplugThread functions

}  // namespace implementation
}  // namespace provider
}  // namespace camera
}  // namespace hardware
}  // namespace android
