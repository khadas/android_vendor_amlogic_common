#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <thread>
#include <mutex>
#include <SubtitleNativeAPI.h>
#include "AmlNativeSubRender.h"
#include "NativeRender.h"

// this same as customer defined.
typedef enum {
    PLAYER_HANDLE_ID_MAIN = 1,    // MAIN
    PLAYER_HANDLE_ID_PIP =  2,    // PIP
    PLAYER_HANDLE_ID_SUB1 = 3,    // SUB1  // PIP and SUB1 have different ID value, but they will use same player.
    PLAYER_HANDLE_ID_SUB2 = 4,    // SUB2
    PLAYER_HANDLE_ID_SUB3 = 5,   // SUB3
    PLAYER_HANDLE_MAX,
} BPNI_PLAYER_HANDLE_ID;

struct Subtitle {
    AmlSubtitleHnd mHandle;
    SubNativeRenderHnd mWin;
    std::shared_ptr<NativeRender> mRender;
    bool mSubStarted;
    int mExtFd;
    Subtitle() : mHandle(nullptr), mWin(nullptr),
        mRender(nullptr), mSubStarted(false), mExtFd(-1) {
    }
};

class SubtitleContext {

public:
    static SubtitleContext& GetInstance();

    bool addSubWindow(int sessionId, SubNativeRenderHnd win);
    bool startPlaySubtitle(int sessionId, const char *lang);
    bool stopPlaySubtitle(int sessionId);
    bool setupExternalFd(int sessionId, int fd);

    bool registerQtoneDataCb(int sessionId, QToneDataCallback cb);

    bool processSubtitleCallback(int sessionId,
                const char *data,
                int size, AmlSubDataType type,
                int x, int y, int width, int height,
                int videoWidth, int videoHeight,
                int showing);

protected:
    SubtitleContext();
    ~SubtitleContext();

private:
    static SubtitleContext _instance;

    QToneDataCallback mQToneCb[PLAYER_HANDLE_MAX];
    Subtitle mSubtitle[PLAYER_HANDLE_MAX];
    std::mutex mMutex;

    struct PtsThreadData {
        std::thread *_thread;
        std::mutex _mutex;
        std::condition_variable _cv;
        bool _exited;
        PtsThreadData() {
            _thread = nullptr;
            _exited = false;
        }

        void requestExit() {
            _exited = true;
            _cv.notify_all();
            if (_thread !=  nullptr) _thread->join();
        }
    } mPtsThread[PLAYER_HANDLE_MAX];

    //std::thread *mPtsThread[PLAYER_HANDLE_MAX];
    void pollPts(struct PtsThreadData *data, int sessionId);
};
