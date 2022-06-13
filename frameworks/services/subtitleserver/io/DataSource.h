#ifndef __SUBTITLE_DATASOURCE_H__
#define __SUBTITLE_DATASOURCE_H__
#include <list>
#include <mutex>
#include "InfoChangeListener.h"
#include <utils/Log.h>


typedef enum {
    E_SUBTITLE_FMQ = 0,
    E_SUBTITLE_DEV,
    E_SUBTITLE_FILE,
    E_SUBTITLE_SOCK, /*deprecated*/
    E_SUBTITLE_DEMUX,
    E_SUBTITLE_VBI,
} SubtitleIOType;

typedef enum {
    E_SOURCE_INV,
    E_SOURCE_STARTED,
    E_SOURCE_STOPED,
} SubtitleIOState;


class DataListener {
public:
    virtual int onData(const char*buffer, int len) = 0;
    virtual ~DataListener() {};
};

class DataSource : public DataListener {

public:
    DataSource() = default;
    DataSource& operator=(const DataSource&) = delete;
    virtual ~DataSource() {
        ALOGD("%s", __func__);
}

    virtual SubtitleIOType type() = 0;
    virtual bool start() = 0;
    virtual bool stop() = 0;

    virtual size_t availableDataSize() = 0;
    virtual size_t read(void *buffer, size_t bufSize) = 0;
    virtual size_t lseek(int offSet, int whence) = 0;

    virtual void updateParameter(int type, void *data) {
        (void) data;
        return;
    }

    virtual void setPipId (int mode, int id) {
        (void)mode;
        (void)id;
        return;
     }
    virtual bool isFileAvailble() {
        return false;
    }

    virtual int64_t getSyncTime() {
        return mSyncPts;
    }


    // If we got information from the data source, we notify these info to listner
    virtual void registerInfoListener(std::shared_ptr<InfoChangeListener>listener) {
        std::unique_lock<std::mutex> autolock(mLock);
        mInfoListeners.push_back(listener);
    }

    virtual void unregisterInfoListener(std::shared_ptr<InfoChangeListener>listener) {
        std::unique_lock<std::mutex> autolock(mLock);
        for (auto it = mInfoListeners.begin(); it != mInfoListeners.end(); it++) {
            auto wk_lstner = (*it);
            if (wk_lstner.expired()) {
                mInfoListeners.erase(it);
                return;
            }
            auto lsn = wk_lstner.lock();
            if (lsn == listener) {
                mInfoListeners.erase(it);
                return;
            }
        }
    }

    std::list<std::weak_ptr<InfoChangeListener>> mInfoListeners;

    int64_t mSyncPts;

    // TODO: dump interface.

    void enableSourceDump(bool enable) {
        mNeedDumpSource = enable;
    }

    virtual void dump(int fd, const char *prefix) = 0;
protected:

    std::mutex mLock;
    bool mNeedDumpSource;
    int mDumpFd;
    int mPlayerId;
    int mMediaSyncId;
};

#endif
