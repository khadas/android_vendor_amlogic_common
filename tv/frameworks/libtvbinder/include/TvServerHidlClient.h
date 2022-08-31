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

#ifndef _ANDROID_TV_SERVER_HIDL_CLIENT_H_
#define _ANDROID_TV_SERVER_HIDL_CLIENT_H_

#include <utils/Timers.h>
#include <utils/threads.h>
#include <utils/RefBase.h>
#include <utils/Mutex.h>

#include <vendor/amlogic/hardware/tvserver/1.0/ITvServer.h>

namespace android {

using ::vendor::amlogic::hardware::tvserver::V1_0::ITvServer;
using ::vendor::amlogic::hardware::tvserver::V1_0::ITvServerCallback;
using ::vendor::amlogic::hardware::tvserver::V1_0::ConnectType;
using ::vendor::amlogic::hardware::tvserver::V1_0::SignalInfo;
using ::vendor::amlogic::hardware::tvserver::V1_0::TvHidlParcel;
using ::vendor::amlogic::hardware::tvserver::V1_0::FormatInfo;

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

typedef enum {
    CONNECT_TYPE_HAL            = 0,
    CONNECT_TYPE_EXTEND         = 1
} tv_connect_type_t;

typedef struct tv_parcel_s {
    int msgType;
    std::vector<int> bodyInt;
    std::vector<std::string> bodyString;
} tv_parcel_t;

class TvListener : virtual public RefBase {
public:
    virtual void notify(const tv_parcel_t &parcel) = 0;
};

class TvServerHidlClient : virtual public RefBase {
public:
    static sp<TvServerHidlClient> connect(tv_connect_type_t type);
    TvServerHidlClient(tv_connect_type_t type);
    ~TvServerHidlClient();

    void reconnect();
    void disconnect();
    //status_t processCmd(const Parcel &p, Parcel *r);
    void setListener(const sp<TvListener> &listener);

    int startTv();
    int stopTv();
    int switchInputSrc(int32_t inputSrc);
    int getInputSrcConnectStatus(int32_t inputSrc);
    int getCurrentInputSrc();
    int getHdmiAvHotplugStatus();
    std::string getSupportInputDevices();
    int getHdmiPorts(int32_t inputSrc);

    SignalInfo getCurSignalInfo();
    int setMiscCfg(const std::string& key, const std::string& val);
    std::string getMiscCfg(const std::string& key, const std::string& def);
    int loadEdidData(int32_t isNeedBlackScreen, int32_t isDolbyVisionEnable);
    int updateEdidData(int32_t inputSrc, const std::string& edidData);
    int setHdmiEdidVersion(int32_t port_id, int32_t ver);
    int getHdmiEdidVersion(int32_t port_id);
    int saveHdmiEdidVersion(int32_t port_id, int32_t ver);
    int setHdmiColorRangeMode(int32_t range_mode);
    int getHdmiColorRangeMode();
    FormatInfo getHdmiFormatInfo();
    int handleGPIO(const std::string& key, int32_t is_out, int32_t edge);
    int vdinUpdateForPQ(int32_t gameStatus, int32_t pcStatus, int32_t autoSwitchFlag);
    int setWssStatus(int status);
    int setDeviceIdForCec(int DeviceId);
    int setScreenColorForSignalChange(int screenColor, int is_save);
    int getScreenColorForSignalChange();
    int dtvGetSignalSNR();
private:
    class TvServerHidlCallback : public ITvServerCallback {
    public:
        TvServerHidlCallback(TvServerHidlClient *client): tvserverClient(client) {};
        Return<void> notifyCallback(const TvHidlParcel& parcel) override;

    private:
        TvServerHidlClient *tvserverClient;
    };

    struct TvServerDaemonDeathRecipient : public android::hardware::hidl_death_recipient  {
        TvServerDaemonDeathRecipient(TvServerHidlClient *client): tvserverClient(client) {};

        // hidl_death_recipient interface
        virtual void serviceDied(uint64_t cookie,
            const ::android::wp<::android::hidl::base::V1_0::IBase>& who) override;
    private:
        TvServerHidlClient *tvserverClient;
    };
    sp<TvServerDaemonDeathRecipient> mDeathRecipient = nullptr;

    static Mutex mLock;
    tv_connect_type_t mType;
    // helper function to obtain tv service handle
    sp<ITvServer> getTvService();

    sp<TvListener> mListener;
    sp<ITvServer> mTvServer;
    sp<TvServerHidlCallback> mTvServerHidlCallback = nullptr;
};

}//namespace android

#endif/*_ANDROID_TV_SERVER_HIDL_CLIENT_H_*/
