#define LOG_TAG "SubtitleServerClientSDK"
#include <android-base/logging.h>
#include <fcntl.h>
#include "SubtitleServerClient.h"

using android::Mutex;
namespace amlogic {
#define EVENT_ONSUBTITLEDATA_CALLBACK       0xA00000
#define EVENT_ONSUBTITLEAVAILABLE_CALLBACK  0xA00001
#define EVENT_ONVIDEOAFDCHANGE_CALLBACK     0xA00002
#define EVENT_ONMIXVIDEOEVENT_CALLBACK      0xA00003
#define EVENT_ONSUBTITLE_DIMESION_CALLBACK  0xA00004
#define EVENT_ONSUBTITLE_LANGUAGE_CALLBACK  0xA00005
#define EVENT_ONSUBTITLE_INFO_CALLBACK      0xA00006


void SubtitleServerClient::SubtitleDeathRecipient::serviceDied(
    uint64_t, const android::wp<::android::hidl::base::V1_0::IBase>&) {
    LOG(ERROR) << "SubtitleServer died. Cleanup instance!";
    // delete Instance

    sp<SubtitleServerClient> owner = mOwner.promote();
    if (owner != nullptr) {
        Mutex::Autolock _l(owner->mLock);
        if (owner->mListener != nullptr) owner->mListener->onServerDied();
        owner->mRemote = nullptr;
        owner->initRemoteLocked();

    }
}

Return<void> SubtitleServerClient::SubtitleCallback::notifyDataCallback(const SubtitleHidlParcel& parcel) {
    hidl_memory mem = parcel.mem;
    int type = parcel.msgType;
    sp<IMemory> memory = mapMemory(mem);
    int width = parcel.bodyInt[0];
    int height =  parcel.bodyInt[1];
    int size = parcel.bodyInt[2];
    int cmd = parcel.bodyInt[3];
    int cordinateX = parcel.bodyInt[4];
    int cordinateY = parcel.bodyInt[5];
    int videoWidth = parcel.bodyInt[6];
    int videoHeight = parcel.bodyInt[7];
    ALOGD("processSubtitleData! %d %d", type, parcel.msgType);
    if (memory ==  nullptr) {
        ALOGD("Error! mapMemory cannot get memory!");
        return Void();
    }
    char* data = static_cast<char *>(static_cast<void*>(memory->getPointer()));

    if (mListener != nullptr) {
        mListener->onSubtitleEvent(data, size, type, cordinateX, cordinateY, width, height, videoWidth, videoHeight, cmd);
    } else {
        ALOGD("error, no handle for this event!");
    }
    return Void();
}

Return<void> SubtitleServerClient::SubtitleCallback::eventNotify(const SubtitleHidlParcel& parcel) {
    switch (parcel.msgType) {
        case EVENT_ONSUBTITLEDATA_CALLBACK: {
            int event = parcel.bodyInt[0];
            int id =  parcel.bodyInt[1];
            ALOGI("onSubtitleDataEvent cc event:%d, channel id:%d", event, id);
            if (mListener != nullptr) {
                mListener->onSubtitleDataEvent(event, id);
            } else {
                ALOGD("error, no handle for this event!");
            }
        }
        break;

        case EVENT_ONSUBTITLEAVAILABLE_CALLBACK: {
            int avail = parcel.bodyInt[0];
            ALOGI("onSubtitleAvail avail:%d", avail);

            if (mListener != nullptr) {
                mListener->onSubtitleAvail(avail);
            } else {
                ALOGD("error, no handle for this event!");
            }
        }
        break;

        case EVENT_ONVIDEOAFDCHANGE_CALLBACK: {
            int afdEvent = parcel.bodyInt[0];
            int playerid = parcel.bodyInt[1];
            ALOGI("onSubtitleAfdEvent bbc event:%d, playerid:%d", afdEvent, playerid);

            if (mListener != nullptr) {
                mListener->onSubtitleAfdEvent(playerid, afdEvent);
            } else {
                ALOGD("error, no handle for this event!");
            }
        }
        break;

        case EVENT_ONMIXVIDEOEVENT_CALLBACK:{
            int val = parcel.bodyInt[0];
            ALOGI("processSubtitle mix video event:%d", val);

            if (mListener != nullptr) {
                mListener->onMixVideoEvent(val);
            } else {
                ALOGD("error, no handle for this event!");
            }
        }
        break;

        case EVENT_ONSUBTITLE_DIMESION_CALLBACK: {
            int width = parcel.bodyInt[0];
            int height = parcel.bodyInt[1];
            ALOGI("onSubtitleDimension width:%d height:%d", width, height);

            if (mListener != nullptr) {
                mListener->onSubtitleDimension(width, height);
            } else {
                ALOGD("error, no handle for this event!");
            }
        }
        break;
        case EVENT_ONSUBTITLE_LANGUAGE_CALLBACK: {
            std::string lang = parcel.bodyString[0];

            if (mListener != nullptr) {
                ALOGI("onSubtitleLanguage lang:%s", lang.c_str());
                mListener->onSubtitleLanguage(lang);
            } else {
                ALOGD("error, no handle for this event!");
            }
        }

        break;
        case EVENT_ONSUBTITLE_INFO_CALLBACK: {
            int what = parcel.bodyInt[0];
            int extra =  parcel.bodyInt[1];
            ALOGI("onSubtitleInfoEvent what:%d, extra:%d", what, extra);
            if (mListener != nullptr) {
                mListener->onSubtitleInfo(what, extra);
            } else {
                ALOGD("error, no handle for this event!");
            }
        }
        break;

    }
    return Void();
}

Return<void> SubtitleServerClient::SubtitleCallback::uiCommandCallback(const SubtitleHidlParcel &parcel) {
    ALOGD("uiCommandCallback: cmd=%d", parcel.msgType);

    if (mListener != nullptr) {
        mListener->onSubtitleUIEvent(parcel.msgType, parcel.bodyInt);
    } else {
        ALOGD("error, no handle for this event!");
    }
    return Void();
}



SubtitleServerClient::SubtitleServerClient(bool isFallback, sp<SubtitleListener> listener, OpenType openType) {
    android::base::SetMinimumLogSeverity(android::base::VERBOSE);

    LOG(ERROR) <<"SubtitleServerHidlClient getSubtitleService ...";
    Mutex::Autolock _l(mLock);
    mIsFallback = isFallback;
    mListener = listener;
    mOpenType = openType;
    initRemoteLocked();
}

void SubtitleServerClient::initRemoteLocked() {
    mRemote = ISubtitleServer::tryGetService();
    while (mRemote == nullptr) {
        mLock.unlock();
        usleep(200*1000);//sleep 200ms
        mLock.lock();
        mRemote = ISubtitleServer::tryGetService();
        LOG(ERROR) <<"tryGet ISubtitleServer daemon Service" << ISubtitleServer::descriptor;
    }

    mCallback = new SubtitleCallback(mListener);

    // fallback mode, only used for fallback display daemon
    if (mIsFallback) {
        ALOGD("regist fallback subtitle");
        mRemote->setFallbackCallback(mCallback, static_cast<ConnectType>(1));
    } else {
        mRemote->setCallback(mCallback, static_cast<ConnectType>(1)/*ConnectType:TYPE_EXTEND*/);
    }

    mDeathRecipient = new SubtitleDeathRecipient(this);

    Return<bool> linked = mRemote->linkToDeath(mDeathRecipient, /*cookie*/ 0XABDADA);
    if (!linked.isOk()) {
        LOG(ERROR) << "Transaction error in linking to system service death:%s"<<linked.description();
    } else if (!linked) {
        LOG(ERROR) << "Unable to link to death notifications";
    } else {
        LOG(ERROR) << "Link to service death notification successful";
    }

    bool result;
    mRemote->openConnection( [&] (const Result &ret, const int& v) {
            if (ret == Result::OK) {
                mSessionId = v;
            } else {
                mSessionId = -1;
            }
            result = (ret == Result::OK);
        });

    if (!result) {
        LOG(ERROR) << "Cannot open connections!";
        return;
    }

    LOG(INFO) << "Created open session: " << mSessionId;
}

template<typename T>
void SubtitleServerClient::checkRemoteResultLocked(Return<T> &r) {
    if (!r.isOk()) {
        //mRemote = nullptr;
        ALOGE("hidl communication failed! server died?? %s", r.description().c_str());
    }
}

SubtitleServerClient::~SubtitleServerClient() {
    Mutex::Autolock _l(mLock);
    LOG(INFO) << "~SubtitleServerClient " << mSessionId;
    if (mRemote != nullptr && mDeathRecipient != nullptr) {
        auto g = mRemote->removeCallback(mCallback);
        checkRemoteResultLocked(g);
        auto r = mRemote->close(mSessionId);
        checkRemoteResultLocked(r);
        auto b = mRemote->unlinkToDeath(mDeathRecipient);
        checkRemoteResultLocked(b);

        auto c= mRemote->closeConnection(mSessionId);
        checkRemoteResultLocked(c);
    }
    mListener = nullptr;
    mDeathRecipient = nullptr;
    mRemote = nullptr;
}


bool SubtitleServerClient::open(int fd, int fdData, int trackId, int ioType) {
    if (fd <=0 || fdData <= 0)
        return open(-1, ioType);

    Mutex::Autolock _l(mLock);
    LOG(INFO) << "open session:" << mSessionId << " ioType="<<ioType;
    if (mRemote == nullptr) {
        initRemoteLocked();
    }

    native_handle_t* nativeHandle = nullptr;

    ::lseek(fd, 0, SEEK_SET);
    ::lseek(fd, 0, SEEK_SET);
    LOG(INFO) << "open session:" << mSessionId << " fd:" << fd << " fdData:" << fdData;
    nativeHandle = native_handle_create(2, 1);
    nativeHandle->data[0] = fd;
    nativeHandle->data[1] = fdData;
    nativeHandle->data[2] = trackId;

    ::android::hardware::hidl_handle handle;
    handle.setTo(nativeHandle, false /* shouldOwn */);

    auto r = mRemote->open(mSessionId, handle, ioType, mOpenType);
    checkRemoteResultLocked(r);
    native_handle_delete(nativeHandle);

    return true;
}

bool SubtitleServerClient::open(int fd, int ioType) {
    Mutex::Autolock _l(mLock);
    LOG(INFO) << "open session:" << mSessionId << " ioType="<<ioType;
    if (mRemote == nullptr) {
        initRemoteLocked();
    }

    native_handle_t* nativeHandle = nullptr;

    if (fd > 0) {
        ::lseek(fd, 0, SEEK_SET);
        LOG(INFO) << "open session:" << mSessionId << " fd:" << fd;
        nativeHandle = native_handle_create(1, 0);
        nativeHandle->data[0] = fd;
    } else {
        nativeHandle = native_handle_create(0, 0);
    }


    ::android::hardware::hidl_handle handle;
    handle.setTo(nativeHandle, false /* shouldOwn */);
    ::android::hardware::hidl_vec<hidl_handle> out; // null handles for this.

    auto r = mRemote->open(mSessionId, handle, ioType, mOpenType);
    checkRemoteResultLocked(r);
    native_handle_delete(nativeHandle);

    return true;
}

bool SubtitleServerClient::open(const char *path, int ioType) {
    int fd = -1;
    bool res = false;
    if ((path != nullptr) && ((fd = ::open(path, O_RDONLY)) > 0)) {
        res = open(fd, ioType);
    } else {
        res = open(-1, ioType);
    }

    if (fd >= 0) ::close(fd);
    return res;
}


bool SubtitleServerClient::close() {
    LOG(INFO) << "close session: " << mSessionId;
    Mutex::Autolock _l(mLock);
    if (mRemote == nullptr) {
        initRemoteLocked();
    }
    auto r = mRemote->close(mSessionId);
    checkRemoteResultLocked(r);
    return r.isOk();
}

bool SubtitleServerClient::updateVideoPos(int pos) {
    Mutex::Autolock _l(mLock);
    if (mRemote == nullptr) {
        initRemoteLocked();
    }
    auto r = mRemote->updateVideoPos(mSessionId, pos);
    checkRemoteResultLocked(r);
    return true;
}


int SubtitleServerClient::totalTracks() {
    Mutex::Autolock _l(mLock);
    if (mRemote == nullptr) {
        initRemoteLocked();
    }

    int track = -1;
    auto r = mRemote->getTotalTracks(mSessionId, [&] (const Result &ret, const int& v) {
            if (ret == Result::OK) {
                track = v;
                LOG(INFO) << "Get Total tracks:" << track;
            }
        });
    checkRemoteResultLocked(r);
    return track;
}

int SubtitleServerClient::getSubType() {
    Mutex::Autolock _l(mLock);
    if (mRemote == nullptr) {
        initRemoteLocked();
    }
    int subType = -1;
    auto r = mRemote->getType(mSessionId, [&] (const Result &ret, const int& v) {
            if (ret == Result::OK) {
                subType = v;
                LOG(INFO) << "Get subType:" << subType;
            }
        });
    checkRemoteResultLocked(r);
    return subType;
}

bool SubtitleServerClient::resetForSeek() {
    Mutex::Autolock _l(mLock);
    if (mRemote == nullptr) {
        initRemoteLocked();
    }

    auto r = mRemote->resetForSeek(mSessionId);
    checkRemoteResultLocked(r);
    return r.isOk();
}

bool SubtitleServerClient::setSubType(int type) {
    Mutex::Autolock _l(mLock);
    if (mRemote == nullptr) {
        initRemoteLocked();
    }

    auto r = mRemote->setSubType(mSessionId, type);
    checkRemoteResultLocked(r);
    return r.isOk();
}

//    mode: 1 for the player id setting;
//             2 for the media id setting.
bool SubtitleServerClient::setPipId(int mode, int id) {
    Mutex::Autolock _l(mLock);
    if (mRemote == nullptr) {
        initRemoteLocked();
    }

    auto r = mRemote->setPipId(mSessionId, mode, id);
    checkRemoteResultLocked(r);
    return r.isOk();
}

bool SubtitleServerClient::setSubPid(int pid) {
    Mutex::Autolock _l(mLock);
    if (mRemote == nullptr) {
        initRemoteLocked();
    }

    auto r = mRemote->setSubPid(mSessionId, pid);
    checkRemoteResultLocked(r);
    return r.isOk();
}

bool SubtitleServerClient::setClosedCaptionLang(const char *lang) {
    LOG(INFO) << "select session:" << mSessionId << ",lang=" << lang;
    Mutex::Autolock _l(mLock);
    if (mRemote == nullptr) {
        initRemoteLocked();
    }
    std::string clang;
    clang = lang;
    auto r = mRemote->setClosedCaptionLang(mSessionId, clang);
    checkRemoteResultLocked(r);
    return r.isOk();
}

bool SubtitleServerClient::selectCcChannel(int ch, const char *lang) {
    Mutex::Autolock _l(mLock);
    LOG(INFO) << "select session:" << mSessionId << ",channel="<< ch << ",lang=" << lang;
    if (mRemote == nullptr) {
        initRemoteLocked();
    }
    std::string clang;
    if (lang != nullptr) {
        clang = lang;
    }

    Return<Result> r = mRemote->resetForSeek(mSessionId);
    checkRemoteResultLocked(r);
    r = mRemote->resetForSeek(mSessionId);
    checkRemoteResultLocked(r);
    r = mRemote->close(mSessionId);
    checkRemoteResultLocked(r);
    r = mRemote->setChannelId(mSessionId, ch);
    checkRemoteResultLocked(r);
    r = mRemote->setClosedCaptionLang(mSessionId, clang);
    checkRemoteResultLocked(r);
    r = mRemote->setSubType(mSessionId, DTV_SUB_CC);
    checkRemoteResultLocked(r);
    r = mRemote->open(mSessionId, nullptr, 0, mOpenType); // CC do not care io type
    checkRemoteResultLocked(r);
    return r.isOk();
}

bool SubtitleServerClient::selectCcChannel(int ch) {
    char* clang = "";
    return selectCcChannel(ch, clang);
}

bool SubtitleServerClient::setClosedCaptionId(int id) {
    Mutex::Autolock _l(mLock);
    if (mRemote == nullptr) {
        initRemoteLocked();
    }


    auto r = mRemote->setChannelId(mSessionId, id);
    checkRemoteResultLocked(r);
    return r.isOk();
}

bool SubtitleServerClient::setClosedCaptionVfmt(int fmt) {
    Mutex::Autolock _l(mLock);
    if (mRemote == nullptr) {
        initRemoteLocked();
    }

    auto r = mRemote->setClosedCaptionVfmt(mSessionId, fmt);
    checkRemoteResultLocked(r);
    return r.isOk();
}

bool SubtitleServerClient::setCompositionPageId(int pageId) {
    Mutex::Autolock _l(mLock);
    if (mRemote == nullptr) {
        initRemoteLocked();
    }

    auto r = mRemote->setPageId(mSessionId, pageId);
    checkRemoteResultLocked(r);
    return r.isOk();
}

bool SubtitleServerClient::setAncillaryPageId(int ancPageId){
    Mutex::Autolock _l(mLock);
    if (mRemote == nullptr) {
        initRemoteLocked();
    }

    auto r = mRemote->setAncPageId(mSessionId, ancPageId);
    checkRemoteResultLocked(r);
    return r.isOk();
}

bool SubtitleServerClient::ttControl(int cmd, int magazine, int page, int regionId, int param) {
    Mutex::Autolock _l(mLock);
    if (mRemote == nullptr) {
        initRemoteLocked();
    }

    auto r = mRemote->ttControl(mSessionId, cmd, magazine, page, regionId, param);
    checkRemoteResultLocked(r);
    return r.isOk();

}


bool SubtitleServerClient::userDataOpen() {
    Mutex::Autolock _l(mLock);
    if (mRemote == nullptr) {
        initRemoteLocked();
    }
    auto r = mRemote->userDataOpen(mSessionId);
    checkRemoteResultLocked(r);
    return r.isOk();
}

bool SubtitleServerClient::userDataClose() {
    Mutex::Autolock _l(mLock);
    if (mRemote == nullptr) {
        initRemoteLocked();
    }
    auto r = mRemote->userDataClose(mSessionId);
    checkRemoteResultLocked(r);

    return r.isOk();
}


// ui related.
// Below api only used for control standalone UI.
// Thes UI is not recomendated, only for some native app/middleware
// that cannot Android (Java) UI hierarchy.
bool SubtitleServerClient::uiShow() {
    Mutex::Autolock _l(mLock);
    if (mRemote == nullptr) {
        initRemoteLocked();
    }
    auto r = mRemote->show(mSessionId);
    checkRemoteResultLocked(r);

    return r.isOk();
}

bool SubtitleServerClient::uiHide() {
    Mutex::Autolock _l(mLock);
    if (mRemote == nullptr) {
        initRemoteLocked();
    }
    auto r = mRemote->hide(mSessionId);
    checkRemoteResultLocked(r);

    return r.isOk();
}

bool SubtitleServerClient::uiSetTextColor(int color) {
    Mutex::Autolock _l(mLock);
    if (mRemote == nullptr) {
        initRemoteLocked();
    }
    auto r = mRemote->setTextColor(mSessionId, color);
    checkRemoteResultLocked(r);
    return r.isOk();
}

bool SubtitleServerClient::uiSetTextSize(int size) {
    Mutex::Autolock _l(mLock);
    if (mRemote == nullptr) {
        initRemoteLocked();
    }
    auto r = mRemote->setTextSize(mSessionId, size);
    checkRemoteResultLocked(r);
    return r.isOk();
}

bool SubtitleServerClient::uiSetGravity(int gravity) {
    Mutex::Autolock _l(mLock);
    if (mRemote == nullptr) {
        initRemoteLocked();
    }
    auto r = mRemote->setGravity(mSessionId, gravity);
    checkRemoteResultLocked(r);
    return r.isOk();
}

bool SubtitleServerClient::uiSetTextStyle(int style) {
    Mutex::Autolock _l(mLock);
    if (mRemote == nullptr) {
        initRemoteLocked();
    }
    auto r = mRemote->setTextStyle(mSessionId, style);
    checkRemoteResultLocked(r);
    return r.isOk();
}

bool SubtitleServerClient::uiSetYOffset(int yOffset) {
    Mutex::Autolock _l(mLock);
    if (mRemote == nullptr) {
        initRemoteLocked();
    }
    auto r = mRemote->setPosHeight(mSessionId, yOffset);
    checkRemoteResultLocked(r);
    return r.isOk();
}

bool SubtitleServerClient::uiSetImageRatio(float ratioW, float ratioH, int32_t maxW, int32_t maxH) {
    Mutex::Autolock _l(mLock);
    if (mRemote == nullptr) {
        initRemoteLocked();
    }
    auto r = mRemote->setImgRatio(mSessionId, ratioW, ratioH, maxW, maxH);
    checkRemoteResultLocked(r);
    return r.isOk();
}

bool SubtitleServerClient::uiGetSubDemision(int *pWidth, int *pHeight) {
    Mutex::Autolock _l(mLock);
    if (mRemote == nullptr) {
        initRemoteLocked();
    }
    auto r = mRemote->getSubDemision(mSessionId, [&] (const Result &ret, const int& width, const int&height) {
            if (ret == Result::OK) {
                *pWidth = width;
                *pHeight = height;
                LOG(INFO) << "Get getSubDemision:" << width << "x" <<height;
            }
        });
    checkRemoteResultLocked(r);
    return r.isOk();
}

bool SubtitleServerClient::uiSetSurfaceViewRect(int x, int y, int width, int height) {
    Mutex::Autolock _l(mLock);
    if (mRemote == nullptr) {
        initRemoteLocked();
    }
    auto r = mRemote->setSurfaceViewRect(mSessionId, x, y, width, height);
    checkRemoteResultLocked(r);
    return r.isOk();
}


}
