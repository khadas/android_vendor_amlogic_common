#define LOG_TAG "UserDataAfd"
#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <utils/Log.h>

#include <UserDataAfd.h>
#include "VideoInfo.h"




#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define  TRACE()    LOGI("[%s::%d]\n",__FUNCTION__,__LINE__)

UserDataAfd *UserDataAfd::sInstance = nullptr;
int UserDataAfd::sNewAfdValue = -1;
void UserDataAfd::notifyCallerAfdChange(int afd) {
    //LOGI("afd_evt_callback, afdValue:%x", afd);
    if (mNotifier != nullptr) {
        mNotifier->onVideoAfdChange((afd&0x7), mPlayerId);
    }
}

void UserDataAfd:: setPlayerId(int id) {
    LOGI("setPlayerId id=%d",id);
    if (sInstance == nullptr) {
       LOGI("Error: setPlayerId sInstance is null");
       return;
    }
    if (id == mPlayerId) {
        return;
    }
    if (-1 == id) return;

    mPlayerId = id;
    if (mThread != nullptr) {
        stop();
        mThread->join();
        mThread = nullptr;
    }
    start(mNotifier);
}

UserDataAfd *UserDataAfd::getCurrentInstance() {
    return UserDataAfd::sInstance;
}


//afd callback
void afd_evt_callback(long devno, int eventType, void *param, void *userdata) {
    (void)devno;
    (void)eventType;
    (void)userdata;
    int afdValue;
    AM_USERDATA_AFD_t *afd = (AM_USERDATA_AFD_t *)param;
    afdValue = afd->af;
    UserDataAfd *instance = UserDataAfd::getCurrentInstance();
    if (instance != nullptr && afdValue != UserDataAfd::sNewAfdValue) {
        instance->notifyCallerAfdChange(afdValue);
        UserDataAfd::sNewAfdValue = afdValue;
        LOGI("AFD callback, value:0x%x", UserDataAfd::sNewAfdValue);
    }
}

UserDataAfd::UserDataAfd() {
    mNotifier = nullptr;
    mPlayerId = -1;
    LOGI("creat UserDataAfd");
    sInstance = this;
    mThread = nullptr;

}

UserDataAfd::~UserDataAfd() {
    LOGI("~UserDataAfd");
    sInstance = nullptr;
    mPlayerId = -1;
    if (mThread != nullptr) {
        stop();
        mThread->join();
        mThread = nullptr;
    }

}
int UserDataAfd::start(ParserEventNotifier *notify)
{
    LOGI("startUserData mPlayerId = %d", mPlayerId);

    // TODO: should impl a real status/notify manner
    std::unique_lock<std::mutex> autolock(mMutex);
    mNotifier = notify;
    if (-1 == mPlayerId) {
        return 1;
    }
    //get Video Format need some time, will block main thread, so start a thread to open userdata.
    mThread = std::shared_ptr<std::thread>(new std::thread(&UserDataAfd::run, this));
    return 1;
}

void UserDataAfd::run() {
    //int mode;
    AM_USERDATA_OpenPara_t para;
    memset(&para, 0, sizeof(para));
    para.vfmt = VideoInfo::Instance()->getVideoFormat();

    if (mPlayerId != -1) {
        para.playerid = mPlayerId;
    }
    UserDataAfd::sNewAfdValue = -1;
    if (AM_USERDATA_Open(USERDATA_DEVICE_NUM, &para) != AM_SUCCESS) {
         LOGI("Cannot open userdata device %d", USERDATA_DEVICE_NUM);
         return;
    }

    //add notify afd change
    LOGI("start afd running mPlayerId = %d",mPlayerId);
    AM_USERDATA_GetMode(USERDATA_DEVICE_NUM, &mMode);
    AM_USERDATA_SetMode(USERDATA_DEVICE_NUM, mMode | AM_USERDATA_MODE_AFD);
    AM_EVT_Subscribe(USERDATA_DEVICE_NUM, AM_USERDATA_EVT_AFD, afd_evt_callback, NULL);
}


int UserDataAfd::stop() {
    //LOGI("stopUserData");
    // TODO: should impl a real status/notify manner
    // this is tooo simple...
    {
        std::unique_lock<std::mutex> autolock(mMutex);
        if (mThread == nullptr) {
            return -1;
        }
    }
    AM_EVT_Unsubscribe(USERDATA_DEVICE_NUM, AM_USERDATA_EVT_AFD, afd_evt_callback, NULL);
    if ((mMode && AM_USERDATA_MODE_CC) ==  AM_USERDATA_MODE_CC)
        AM_USERDATA_Close(USERDATA_DEVICE_NUM);
    return 0;
}

