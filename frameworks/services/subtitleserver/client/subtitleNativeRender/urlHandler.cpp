#define LOG_TAG "UrlDownload"

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <cutils/atomic.h>


#include "utils_funcs.h"
#include "MyLog.h"
#include "WebTask.h"



static inline const char *triml(const char *str, const char *whitespace) {
    const char *p = str;

    while (*p && (strchr(whitespace, *p) != NULL))
        p++;

    return p;
}


static int handleLocalFile(const char *path) {
    int fd = open(path, O_RDONLY);
    if (fd <= 0) {
        ALOGE("Error! cannot open file:%s %d", path, errno);
    }
    ALOGD("openfile:%s %d", path, fd);
    return fd;
}

static bool initDownloadPath(std::string fPath) {
    if (::access(fPath.c_str(), R_OK) != 0) {
        if (::mkdir(fPath.c_str(), 0775) != 0) {
            ALOGE("Error, cannot create path!:%s %d", fPath.c_str(), errno);
            return false;
        }
    }

    return true;
}

static int handleHttpFile(const char *url) {
    static int g_indicator = 0;
    std::stringstream ss;
    int i = android_atomic_inc(&g_indicator);
    int fd = -1;

    ALOGD("start:");

    std::string path = getApplicationPath() + "/tmp";
    ALOGD("save to:%s", path.c_str());
    if (!initDownloadPath(path)) {
        ALOGE("Error! cannot initialize path:%s", path.c_str());
        return -1;
    }

    ss << path << "/TMP" << i;
    path = ss.str();
    WebTask task;
    task.SetUrl(url);
    task.SetConnectTimeout(5);
    task.DoGetFile(path.c_str());

    if (task.WaitTaskDone() == 0) {
        fd = open(path.c_str(), O_RDONLY);
        unlink(path.c_str());
    } else {
        ALOGE("Error! cannot download file %s", url);
    }

    ALOGD("end");
    return fd;
}



int AmlUrl2Fd(const char *url) {
#define URL_TYPE_FILE 1
#define URL_TYPE_HTTP 2
#define URL_TYPE_FTP 3
    int fd = -1;
    int urlType = URL_TYPE_FILE;

    const char *p  = triml(url, "\t ");
    if (strncmp(p, "file://", 7) == 0) {
        urlType = URL_TYPE_FILE;
        p += 7;
    } else if ((strncmp(p, "http://", 7) == 0)
        || (strncmp(p, "https://", 8) == 0)) {
        urlType = URL_TYPE_HTTP;
    } else if (strncmp(p, "ftp:///", 8) == 0) {
        urlType = URL_TYPE_FTP;
    }


    switch (urlType) {
        case URL_TYPE_HTTP:
            fd = handleHttpFile(url);
            break;

        case URL_TYPE_FTP:
            ALOGE("Not support ftp yet");
            break;

        default:
            fd = handleLocalFile(p);
            break;
    }

    return fd;
}

