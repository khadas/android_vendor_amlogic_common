#pragma once

#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <functional>
#include <map>
#include <mutex>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <android-base/unique_fd.h>
#include <cutils/android_filesystem_config.h>

#include <utils/Log.h>
#include <utils/Looper.h>

#define LOG_TAG "logging"

// Configurations for save persist logcat log.
typedef struct logcat_config {
    std::string binPath;
    std::string logBuffers;
    std::string format;
    std::string savePath;
    std::string rotateKBytes;
    std::string rotateCounts;
    int logcat_pid;
    logcat_config():logcat_pid(-1) {
         ALOGD("%s\n", __func__);
    }
    void dump() {
        ALOGD("logcat config:\n");
        ALOGD("binPath      %s\n", binPath.c_str());
        ALOGD("logBuffers   %s\n", logBuffers.c_str());
        ALOGD("format       %s\n", format.c_str());
        ALOGD("savePath     %s\n", savePath.c_str());
        ALOGD("rotateKBytes %s\n", rotateKBytes.c_str());
        ALOGD("rotateCounts %s\n", rotateCounts.c_str());
       // ALOGD("rotateCounts %s\n", rotateCounts.c_str());
        ALOGD("\n");

    }
} LogcatConfig;

// Configuration list for write some sysfs or other filepath
typedef struct {
    std::string path;
    std::string value;
} SysfsWriteAction;

// Configuration for set property, (maybe prevent for selinux and useless)
typedef struct {
    std::string property;
    std::string value;
} PropertyAction;

// TODO: configuration for periodically polling file path and save to file. for example: vmstat, proc/meminfo

class UserLogConfig {
public:
    bool Load(const std::string& file_name);
    void updateLogPath(const std::string&path) {
        mUdiskLogPath = path;
    }
    static UserLogConfig& GetInstance();

    pid_t startPersistLogcat();
    bool stopLogging();
    bool drainAllSysfsWriteAction();
    bool drainAllPropertyAction();
private:
    UserLogConfig();
    ~UserLogConfig();

    std::string mUdiskLogPath;
    LogcatConfig mLogcatConfig;
    std::vector<SysfsWriteAction> mSysfsWriteActions;
    std::vector<PropertyAction> mPropertyActions;
};