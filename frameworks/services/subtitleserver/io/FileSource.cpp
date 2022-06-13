#define LOG_TAG "FileSource"

#include <unistd.h>
#include <fcntl.h>
#include <string>
#include "FileSource.h"
#include <utils/Log.h>
#include <utils/CallStack.h>
#include "IpcDataTypes.h"


FileSource::FileSource(int fd) {
    ALOGD("%s fd:%d", __func__, fd);
    mFd = fd;
    if (mFd > 0) {
        ::lseek(mFd, 0, SEEK_SET);
    }
}

FileSource::~FileSource() {
    ALOGD("%s mFd:%d", __func__, mFd);
    if (mFd > 0) {
        ::close(mFd);
        mFd = -1;
    }

}

static inline unsigned int make32bitValue(const char *buffer) {
    return buffer[0] | (buffer[1]<<8) | (buffer[2]<<16) | (buffer[3]<<24);
}

int FileSource::onData(const char *buffer, int len) {
    int type = make32bitValue(buffer);
    ALOGD("subtype=%x eTypeSubtitleRenderTime=%x size=%d", type, eTypeSubtitleRenderTime, len);
    switch (type) {
        case eTypeSubtitleTotal: {
            int totalSubtitle = make32bitValue(buffer + 4);
            ALOGD("eTypeSubtitleTotal:%x total:%d", type, totalSubtitle);
            for (auto it = mInfoListeners.begin(); it != mInfoListeners.end(); it++) {
                auto wk_lstner = (*it);
                if (auto lstn = wk_lstner.lock()) {
                    if (lstn == nullptr) return -1;
                    lstn->onSubtitleChanged(totalSubtitle);

                }
            }
            return 0;
        }

        default: {
            ALOGD("!!!!!!!!!FileSource:onData(subtitleData): %d", /*buffer,*/ len);
            break;
        }
    }

    return 0;
}

bool FileSource::start() {
    ALOGD("%s", __func__);
    return true;
}

bool FileSource::stop() {
    ALOGD("%s mFd:%d", __func__, mFd);
    return true;
}

SubtitleIOType FileSource::type() {
    return E_SUBTITLE_FILE;
}
bool FileSource::isFileAvailble() {
    ALOGD("%s", __func__);
    return (mDumpFd > 0);
}

size_t FileSource::lseek(int offSet, int whence) {
    ALOGD("%s", __func__);
    if (mFd > 0) {
        return ::lseek(mFd, offSet, whence);
    } else {
        return 0;
    }
}


size_t FileSource::availableDataSize() {
    int len = 0;
    if (mFd > 0) {
        len = ::lseek(mFd, 0L, SEEK_END);
        lseek(0, SEEK_SET);
    }
    return len;
}


size_t FileSource::read(void *buffer, size_t size) {
    int data_size = size, r = 0, read_done = 0;
    char *buf = (char *)buffer;
    do {
        errno = 0;
        r = ::read(mFd, buf + read_done, data_size);
    } while (r <= 0 && (errno == EINTR || errno == EAGAIN));
    ALOGD("have readed r=%d, mRdFd:%d, size:%d errno:%d(%s)", r, mFd, size, errno,strerror(errno));
    return r;
}

void FileSource::dump(int fd, const char *prefix) {
    dprintf(fd, "\nFileSource:\n");
}

