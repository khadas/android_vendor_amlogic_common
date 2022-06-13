/*
 * Copyright (C) 2016 The Android Open Source Project
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
 *  @author   Tellen Yu
 *  @version  1.0
 *  @date     2018/1/12
 *  @par function description:
 *  - 1 droidlogic tv hwbinder client
 */

#define LOG_TAG "TvServerHidlClient"
#include <log/log.h>
#include "unistd.h"

#include "include/TvServerHidlClient.h"

namespace android {

Mutex TvServerHidlClient::mLock;

// establish binder interface to tv service
sp<ITvServer> TvServerHidlClient::getTvService()
{
    Mutex::Autolock _l(mLock);

#if 1//PLATFORM_SDK_VERSION >= 26
    sp<ITvServer> tvservice = ITvServer::tryGetService();
    while (tvservice == nullptr) {
         usleep(200*1000);//sleep 200ms
         tvservice = ITvServer::tryGetService();
         ALOGE("tryGet tvserver daemon Service");
    };
    mDeathRecipient = new TvServerDaemonDeathRecipient(this);
    Return<bool> linked = tvservice->linkToDeath(mDeathRecipient, /*cookie*/ 0);
    if (!linked.isOk()) {
        ALOGE("Transaction error in linking to tvserver daemon service death: %s", linked.description().c_str());
    } else if (!linked) {
        ALOGE("Unable to link to tvserver daemon service death notifications");
    } else {
        ALOGI("Link to tvserver daemon service death notification successful");
    }

#else
    Mutex::Autolock _l(mLock);
    if (mTvService.get() == 0) {
        sp<IServiceManager> sm = defaultServiceManager();
        sp<IBinder> binder;
        do {
            binder = sm->getService(String16("tvservice"));
            if (binder != 0)
                break;
            ALOGW("TvService not published, waiting...");
            usleep(500000); // 0.5 s
        } while (true);
        if (mDeathNotifier == NULL) {
            mDeathNotifier = new DeathNotifier();
        }
        binder->linkToDeath(mDeathNotifier);
        mTvService = interface_cast<ITvService>(binder);
    }
    ALOGE_IF(mTvService == 0, "no TvService!?");
    return mTvService;
#endif

    return tvservice;
}

TvServerHidlClient::TvServerHidlClient(tv_connect_type_t type): mType(type)
{
    mTvServer = getTvService();
    if (mTvServer != nullptr) {
        mTvServerHidlCallback = new TvServerHidlCallback(this);
        mTvServer->setCallback(mTvServerHidlCallback, static_cast<ConnectType>(type));
    }
}

TvServerHidlClient::~TvServerHidlClient()
{
    disconnect();
}

sp<TvServerHidlClient> TvServerHidlClient::connect(tv_connect_type_t type)
{
    return new TvServerHidlClient(type);
}

void TvServerHidlClient::reconnect()
{
    ALOGI("tvserver client type:%d reconnect", mType);
    mTvServer.clear();
    //reconnect to server
    mTvServer = getTvService();
    if (mTvServer != nullptr)
        mTvServer->setCallback(mTvServerHidlCallback, static_cast<ConnectType>(mType));
}

void TvServerHidlClient::disconnect()
{
    ALOGD("disconnect");
}

/*
status_t TvServerHidlClient::processCmd(const Parcel &p, Parcel *r __unused)
{
    int cmd = p.readInt32();

    ALOGD("processCmd cmd=%d", cmd);
    return 0;
#if 0
    sp<IAllocator> ashmemAllocator = IAllocator::getService("ashmem");
    if (ashmemAllocator == nullptr) {
        ALOGE("can not get ashmem service");
        return -1;
    }

    size_t size = p.dataSize();
    hidl_memory hidlMemory;
    auto res = ashmemAllocator->allocate(size, [&](bool success, const hidl_memory& memory) {
                if (!success) {
                    ALOGE("ashmem allocate size:%d fail", size);
                }
                hidlMemory = memory;
            });

    if (!res.isOk()) {
        ALOGE("ashmem allocate result fail");
        return -1;
    }

    sp<IMemory> memory = hardware::mapMemory(hidlMemory);
    void* data = memory->getPointer();
    memory->update();
    // update memory however you wish after calling update and before calling commit
    memcpy(data, p.data(), size);
    memory->commit();
    Return<int32_t> ret = mTvServer->processCmd(hidlMemory, (int)size);
    if (!ret.isOk()) {
        ALOGE("Failed to processCmd");
    }
    return ret;
#endif
}*/

void TvServerHidlClient::setListener(const sp<TvListener> &listener)
{
    mListener = listener;
}

int TvServerHidlClient::startTv() {
    return mTvServer->startTv();
}

int TvServerHidlClient::stopTv() {
    return mTvServer->stopTv();
}

int TvServerHidlClient::switchInputSrc(int32_t inputSrc) {
    return mTvServer->switchInputSrc(inputSrc);
}

int TvServerHidlClient::getInputSrcConnectStatus(int32_t inputSrc) {
    return mTvServer->getInputSrcConnectStatus(inputSrc);
}

int TvServerHidlClient::getCurrentInputSrc() {
    return mTvServer->getCurrentInputSrc();
}

int TvServerHidlClient::getHdmiAvHotplugStatus() {
    return mTvServer->getHdmiAvHotplugStatus();
}

std::string TvServerHidlClient::getSupportInputDevices() {
    int ret;
    std::string tvDevices;
    mTvServer->getSupportInputDevices([&](int32_t result, const ::android::hardware::hidl_string& devices) {
        ret = result;
        tvDevices = devices;
    });
    return tvDevices;
}

int TvServerHidlClient::getHdmiPorts(int32_t inputSrc) {
    return mTvServer->getHdmiPorts(inputSrc);
}

SignalInfo TvServerHidlClient::getCurSignalInfo() {
    SignalInfo signalInfo;
    mTvServer->getCurSignalInfo([&](const SignalInfo& info) {
        signalInfo.fmt = info.fmt;
        signalInfo.transFmt = info.transFmt;
        signalInfo.status = info.status;
        signalInfo.frameRate = info.frameRate;
    });
    return signalInfo;
}

int TvServerHidlClient::setMiscCfg(const std::string& key, const std::string& val) {
    return mTvServer->setMiscCfg(key, val);
}

std::string TvServerHidlClient::getMiscCfg(const std::string& key, const std::string& def) {
    std::string miscCfg;
    mTvServer->getMiscCfg(key, def, [&](const std::string& cfg) {
        miscCfg = cfg;
    });

    return miscCfg;
}

int TvServerHidlClient::loadEdidData(int32_t isNeedBlackScreen, int32_t isDolbyVisionEnable) {
    return mTvServer->loadEdidData(isNeedBlackScreen, isDolbyVisionEnable);
}

int TvServerHidlClient::updateEdidData(int32_t inputSrc, const std::string& edidData) {
    return mTvServer->updateEdidData(inputSrc, edidData);
}

int TvServerHidlClient::setHdmiEdidVersion(int32_t port_id, int32_t ver) {
    return mTvServer->setHdmiEdidVersion(port_id, ver);
}

int TvServerHidlClient::getHdmiEdidVersion(int32_t port_id) {
    return mTvServer->getHdmiEdidVersion(port_id);
}

int TvServerHidlClient::saveHdmiEdidVersion(int32_t port_id, int32_t ver) {
    return mTvServer->saveHdmiEdidVersion(port_id, ver);
}

int TvServerHidlClient::setHdmiColorRangeMode(int32_t range_mode) {
    return mTvServer->setHdmiColorRangeMode(range_mode);
}

int TvServerHidlClient::getHdmiColorRangeMode() {
    return mTvServer->getHdmiColorRangeMode();
}

FormatInfo TvServerHidlClient::getHdmiFormatInfo() {
    FormatInfo info;
    mTvServer->getHdmiFormatInfo([&](const FormatInfo formatInfo) {
        info.width     = formatInfo.width;
        info.height    = formatInfo.height;
        info.fps       = formatInfo.fps;
        info.interlace = formatInfo.interlace;
    });
    return info;
}

int TvServerHidlClient::handleGPIO(const std::string& key, int32_t is_out, int32_t edge) {
    return mTvServer->handleGPIO(key, is_out, edge);
}

int TvServerHidlClient::vdinUpdateForPQ(int32_t gameStatus, int32_t pcStatus, int32_t autoSwitchFlag) {
    return mTvServer->vdinUpdateForPQ(gameStatus, pcStatus, autoSwitchFlag);
}

int TvServerHidlClient::setWssStatus(int status) {
    return mTvServer->setWssStatus(status);
}

int TvServerHidlClient::setDeviceIdForCec(int DeviceId) {
    return mTvServer->setDeviceIdForCec(DeviceId);
}

int TvServerHidlClient::setScreenColorForSignalChange(int screenColor, int is_save) {
    return mTvServer->setScreenColorForSignalChange(screenColor, is_save);
}

int TvServerHidlClient::getScreenColorForSignalChange() {
    return mTvServer->getScreenColorForSignalChange();
}

int TvServerHidlClient::dtvGetSignalSNR() {
    return mTvServer->dtvGetSignalSNR();
}

// callback from tv service
Return<void> TvServerHidlClient::TvServerHidlCallback::notifyCallback(const TvHidlParcel& hidlParcel)
{
    ALOGI("notifyCallback event type:%d", hidlParcel.msgType);

#if 0
    Parcel p;

    sp<IMemory> memory = android::hardware::mapMemory(parcelMem);
    void* data = memory->getPointer();
    memory->update();
    // update memory however you wish after calling update and before calling commit
    p.setDataPosition(0);
    p.write(data, size);
    memory->commit();

#endif

    sp<TvListener> listener;
    {
        Mutex::Autolock _l(mLock);
        listener = tvserverClient->mListener;
    }

    tv_parcel_t parcel;
    parcel.msgType = hidlParcel.msgType;
    for (int i = 0; i < hidlParcel.bodyInt.size(); i++) {
        parcel.bodyInt.push_back(hidlParcel.bodyInt[i]);
    }

    for (int j = 0; j < hidlParcel.bodyString.size(); j++) {
        parcel.bodyString.push_back(hidlParcel.bodyString[j]);
    }

    if (listener != NULL) {
        listener->notify(parcel);
    }
    return Void();
}

void TvServerHidlClient::TvServerDaemonDeathRecipient::serviceDied(uint64_t cookie __unused,
        const ::android::wp<::android::hidl::base::V1_0::IBase>& who __unused)
{
    ALOGE("tvserver daemon died");

    usleep(200*1000);//sleep 200ms
    tvserverClient->reconnect();
}
}
