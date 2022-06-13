#define LOG_TAG "SubtitleContext"
#include  "MyLog.h"
#include "SubtitleContext.h"

// need same as server side
#define TYPE_SUBTITLE_Q_TONE_DATA 0xAAAA


//AmlSubtitleStatus amlsub_RegistOnDataCB(AmlSubtitleHnd handle, AmlSubtitleDataCb listener);
// Currently, C callback has no better way to handle in c++
static void MyAmlSubtitleDataCb_1(const char *data,
                int size, AmlSubDataType type,
                int x, int y, int width, int height,
                int videoWidth, int videoHeight,
                int showing) {
    SubtitleContext::GetInstance().processSubtitleCallback(PLAYER_HANDLE_ID_MAIN,
        data, size, type, x, y, width, height, videoWidth, videoHeight, showing);

}
static void MyAmlSubtitleDataCb_2(const char *data,
                int size, AmlSubDataType type,
                int x, int y, int width, int height,
                int videoWidth, int videoHeight,
                int showing) {
    SubtitleContext::GetInstance().processSubtitleCallback(PLAYER_HANDLE_ID_PIP,
        data, size, type, x, y, width, height, videoWidth, videoHeight, showing);
}
static void MyAmlSubtitleDataCb_3(const char *data,
                int size, AmlSubDataType type,
                int x, int y, int width, int height,
                int videoWidth, int videoHeight,
                int showing) {
    SubtitleContext::GetInstance().processSubtitleCallback(PLAYER_HANDLE_ID_SUB1,
        data, size, type, x, y, width, height, videoWidth, videoHeight, showing);
}
static void MyAmlSubtitleDataCb_4(const char *data,
                int size, AmlSubDataType type,
                int x, int y, int width, int height,
                int videoWidth, int videoHeight,
                int showing) {
    SubtitleContext::GetInstance().processSubtitleCallback(PLAYER_HANDLE_ID_SUB2,
        data, size, type, x, y, width, height, videoWidth, videoHeight, showing);
}
static void MyAmlSubtitleDataCb_5(const char *data,
                int size, AmlSubDataType type,
                int x, int y, int width, int height,
                int videoWidth, int videoHeight,
                int showing) {
    SubtitleContext::GetInstance().processSubtitleCallback(PLAYER_HANDLE_ID_SUB3,
        data, size, type, x, y, width, height, videoWidth, videoHeight, showing);
}

static AmlSubtitleDataCb gOnDataCB[PLAYER_HANDLE_MAX] = {
        nullptr,
        MyAmlSubtitleDataCb_1,
        MyAmlSubtitleDataCb_2,
        MyAmlSubtitleDataCb_3,
        MyAmlSubtitleDataCb_4,
        MyAmlSubtitleDataCb_5,
};

SubtitleContext SubtitleContext::_instance;
SubtitleContext& SubtitleContext::GetInstance() {
    return _instance;
}

SubtitleContext::SubtitleContext() {
    ALOGD("SubtitleContext");

/*     AmlSubtitleParam param;
    param.subtitleType = TYPE_SUBTITLE_DVB;
    param.ioSource = E_SUBTITLE_FMQ;
    AmlSubtitleStatus r = amlsub_Open(mHandle, &param);
    */
}

SubtitleContext::~SubtitleContext() {
   ALOGD("~~SubtitleContext ");
}

bool SubtitleContext::processSubtitleCallback(int sessionId,
                const char *data,
                int size, AmlSubDataType type,
                int x, int y, int width, int height,
                int videoWidth, int videoHeight,
                int showing) {
    ALOGE("processSubtitleCallback id:%d data:%p, size=%d, type:%d x:%d y:%d, width:%d height:%d",
        sessionId, data, size, type, x, y, width, height);
    std::unique_lock<std::mutex> autolock(mMutex);

    if (type == TYPE_SUBTITLE_Q_TONE_DATA) {
        ALOGD("QToneDataReceived: %d %p %d", sessionId, data, size);

        if (mQToneCb[sessionId] != nullptr) {
            mQToneCb[sessionId](sessionId, data, size);
        }
        return true;
    }

    // TODO: Post to a Queue, and create a new thread to handle this.
    if (showing) {
        ALOGD("showing...");
        mSubtitle[sessionId].mRender->render(data, size, type, x, y, width, height, videoWidth, videoHeight);
    } else {
        ALOGD("clear...");
        mSubtitle[sessionId].mRender->clear();
    }
    return true;
}


bool SubtitleContext::registerQtoneDataCb(int sessionId, QToneDataCallback cb) {
    std::unique_lock<std::mutex> autolock(mMutex);

    if (sessionId != PLAYER_HANDLE_ID_MAIN) {
        ALOGE("currently, only support main stream");
    }
    mQToneCb[sessionId] = cb;

    return true;
}


bool SubtitleContext::addSubWindow(int sessionId, SubNativeRenderHnd win) {
    // currently, we only support play 1 session.
    std::unique_lock<std::mutex> autolock(mMutex);

    if (sessionId != PLAYER_HANDLE_ID_MAIN) {
        ALOGE("currently, only support main stream");
    }
    mSubtitle[sessionId].mWin = win;

    return true;
}

// TODO: multithread support

bool SubtitleContext::startPlaySubtitle(int sessionId, const char *lang) {
    std::unique_lock<std::mutex> autolock(mMutex);
    if (sessionId != PLAYER_HANDLE_ID_MAIN) {
        ALOGE("currently, only support main stream");
    }
    ALOGD("mSubtitle[%d].mExtFd=%d", sessionId, mSubtitle[sessionId].mExtFd);

    AmlSubtitleHnd hnd = amlsub_Create();
    mSubtitle[sessionId].mHandle = hnd;

    // TODO: make more robust
    mSubtitle[sessionId].mRender = std::shared_ptr<NativeRender>(new NativeRender(mSubtitle[sessionId].mWin));

    // set smiURL before, use external subtitle
    AmlSubtitleParam param;
    memset(&param, 0, sizeof(param));
    param.lang = lang;
    if (mSubtitle[sessionId].mExtFd > 0) {
        param.fd = mSubtitle[sessionId].mExtFd;
        param.ioSource = E_SUBTITLE_FILE;
    } else {
        param.ioSource = E_SUBTITLE_FMQ;
    }
    AmlSubtitleStatus r = amlsub_Open(mSubtitle[sessionId].mHandle, &param);

    // TODO: render to this session.
    amlsub_RegistOnDataCB(mSubtitle[sessionId].mHandle, gOnDataCB[sessionId]);

    mSubtitle[sessionId].mSubStarted = true;

    ALOGD("mSubtitle[%d].mExtFd=%d", sessionId, mSubtitle[sessionId].mExtFd);
    if (mSubtitle[sessionId].mExtFd > 0) {
        close(mSubtitle[sessionId].mExtFd);
        mSubtitle[sessionId].mExtFd = -1;
        mPtsThread[sessionId]._thread = new std::thread(&SubtitleContext::pollPts, this, &mPtsThread[sessionId], sessionId);
    }
    return true;
}

bool SubtitleContext::stopPlaySubtitle(int sessionId) {
    std::unique_lock<std::mutex> autolock(mMutex);
    amlsub_Close( mSubtitle[sessionId].mHandle);
    if (mSubtitle[sessionId].mSubStarted) {
        mSubtitle[sessionId].mRender->clear();
    }
    AmlSubtitleStatus r = amlsub_Destroy(mSubtitle[sessionId].mHandle);
    mSubtitle[sessionId].mSubStarted = false;

    if (mSubtitle[sessionId].mExtFd > 0) {
        close(mSubtitle[sessionId].mExtFd);
        mSubtitle[sessionId].mExtFd = -1;
    }
    if (mPtsThread[sessionId]._thread != nullptr) {
        // TODO: stop
        mPtsThread[sessionId].requestExit();
        delete mPtsThread[sessionId]._thread;
        mPtsThread[sessionId]._thread = nullptr;
    }
    return true;
}

bool SubtitleContext::setupExternalFd(int sessionId, int fd) {
    std::unique_lock<std::mutex> autolock(mMutex);
    if (fd <= 0) {
        ALOGE("Error! invalid file descriptor:%d", fd);
        return false;
    }

    if (mSubtitle[sessionId].mExtFd > 0) {
        ALOGD("previous fd not use and close! close it");
        close(mSubtitle[sessionId].mExtFd);
    }

    mSubtitle[sessionId].mExtFd = fd;
    return true;
}

extern int64_t NativeWindowGetPts(int sessionId);
void SubtitleContext::pollPts(struct PtsThreadData *data, int sessionId) {
    ALOGD("pollPts for %d", sessionId);
    const int dvbTimeMultiply = 90; // convert ms to DVB time.
    while (!data->_exited) {
        std::unique_lock<std::mutex> autolock(data->_mutex);
        data->_cv.wait_for(autolock, std::chrono::milliseconds(100));

        int64_t pos = NativeWindowGetPts(sessionId);
        amlsub_UpdateVideoPos(mSubtitle[sessionId].mHandle, pos*dvbTimeMultiply);
    }
}


