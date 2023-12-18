#include <fcntl.h>
#include <string>
#include <iostream>

#include <vector>

#include <thread>

#include <android-base/file.h>
#include <android-base/logging.h>
//#include <android-base/properties.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>
#include <android-base/threads.h>
#include <cutils/properties.h>

#include <cutils/android_filesystem_config.h>

#include <json/reader.h>
#include <json/value.h>

#include <filesystem>

#include "UserLogConfig.h"

#define LOG_TAG "logging"

using android::base::StringPrintf;
using android::base::StringReplace;
using android::base::unique_fd;
using android::base::WriteStringToFile;

static bool writeToFile(const char *path, const char *val) {
    const int SIZE = 20;
    int ret, fd, len;
    char value[SIZE];

    if (!path)
        return false;

    fd = open(path, O_WRONLY, 0);
    if (fd < 0) {
        ALOGE("writeToFile: Could not open '%s' err: %d\n", path, errno);
        return false;
    }

    ret = write(fd, val, strlen(val)+1);
    ret = write(fd, "\n", 2);

    close(fd);
    return true;
}

UserLogConfig::UserLogConfig() {
    ALOGD("%s\n", __func__);
}

UserLogConfig::~UserLogConfig() {

}


pid_t UserLogConfig::startPersistLogcat() {
    pid_t pid = -1;

    // clear last pid:
    property_set("ro.logd.kernel", "true");

    pid = fork();
    if (pid == 0) {
        std::vector<char*> c_strings;
        c_strings.push_back(const_cast<char*>(mLogcatConfig.binPath.size()>0 ? mLogcatConfig.binPath.c_str() : "/system/bin/logcat"));
        //c_strings.push_back("-L");  // has last?
        c_strings.push_back("-b");
        c_strings.push_back(const_cast<char*>(mLogcatConfig.logBuffers.size()>0 ? mLogcatConfig.logBuffers.c_str() : "all"));  // args ${logd.logpersistd.buffer:-all}
        c_strings.push_back("-v");
        c_strings.push_back(const_cast<char*>(mLogcatConfig.format.size()>0 ? mLogcatConfig.format.c_str() : "threadtime,usec,printable"));
        c_strings.push_back("-D");
        c_strings.push_back("-f");
        c_strings.push_back(const_cast<char*>(mLogcatConfig.savePath.c_str()));//data/media/0/logd_ext/logcat");  // args
        c_strings.push_back("-r");
        c_strings.push_back(const_cast<char*>(mLogcatConfig.rotateKBytes.size()>0 ? mLogcatConfig.rotateKBytes.c_str() : "2048")); // args${logd.logpersistd.rotate_kbytes:-2048}
        c_strings.push_back("-n");
        c_strings.push_back(const_cast<char*>(mLogcatConfig.rotateCounts.size()>0 ? mLogcatConfig.rotateCounts.c_str() : "256"));
        c_strings.push_back(nullptr);

        for (std::size_t i = 0; i < c_strings.size(); i++) {
             ALOGD ("%s ", c_strings[i]);
        }
        ALOGD ("\n");

        if (setgid(AID_MEDIA_RW) != 0) {
             ALOGE("setgid failed: %s\n", strerror(errno));
        }

        std::vector<gid_t> supp_gids_;
        supp_gids_.emplace_back(AID_SYSTEM);
        supp_gids_.emplace_back(AID_LOG);
        supp_gids_.emplace_back(AID_LOGD);
        supp_gids_.emplace_back(AID_SDCARD_RW);
        supp_gids_.emplace_back(AID_MEDIA_RW);
        supp_gids_.emplace_back(AID_EXTERNAL_STORAGE);

        if (setgroups(supp_gids_.size(), &supp_gids_[0]) != 0) {
            ALOGE("setgroups failed: %s\n", strerror(errno));
        }
        execv(c_strings[0], c_strings.data());
    }
    mLogcatConfig.logcat_pid = pid;
    ALOGD("create logcat process: %d\n", pid);
    return pid;
}
bool UserLogConfig::stopLogging() {
   return kill(mLogcatConfig.logcat_pid, 9) >= 0;
}


bool UserLogConfig::drainAllSysfsWriteAction() {
    for (auto action : mSysfsWriteActions) {
        writeToFile(action.path.c_str(), action.value.c_str());

    }
    mSysfsWriteActions.clear();
    return true;
}

bool UserLogConfig::drainAllPropertyAction() {
    for (auto action : mPropertyActions) {
        if (property_set(action.property.c_str(), action.value.c_str()) < 0) {
            ALOGE("property_set %s %s failed\n", action.property.c_str(), action.value.c_str());
        }
    }
    mSysfsWriteActions.clear();
    return true;
}


bool UserLogConfig::Load(const std::string& file_name) {
    std::string json_doc;

    if (!android::base::ReadFileToString(file_name, &json_doc)) {
       std::cout << "Failed to read task profiles from " << file_name << std::endl;
        return false;
    }

    Json::CharReaderBuilder builder;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    Json::Value root;
    std::string errorMessage;
    if (!reader->parse(&*json_doc.begin(), &*json_doc.end(), &root, &errorMessage)) {
       std::cout << "Failed to parse task profiles: " << errorMessage << std::endl;
        return false;
    }


    const Json::Value& logcatVal = root["logcat"];
    mLogcatConfig.binPath = logcatVal["path"].asString();
    mLogcatConfig.logBuffers = logcatVal["logBuffers"].asString();
    mLogcatConfig.format = logcatVal["format"].asString();

    std::filesystem::path p = file_name;
    mLogcatConfig.savePath = logcatVal["saveName"].asString();
    mLogcatConfig.savePath = p.parent_path().string() + std::string("/") + mLogcatConfig.savePath;
    //mLogcatConfig.binPath = profile_val["path"].asString();
    mLogcatConfig.rotateKBytes = logcatVal["rotateKBytes"].asString();
    mLogcatConfig.rotateCounts = logcatVal["rotateCounts"].asString();

    mLogcatConfig.dump();

    const Json::Value& sysfsWriterVal = root["sysfs_write"];
    for (auto member : sysfsWriterVal.getMemberNames()) {
        SysfsWriteAction action;
        action.path = member;
        action.value = sysfsWriterVal[member].asString();
        std::cout << member << ":" <<sysfsWriterVal[member].asString() <<std::endl;
        mSysfsWriteActions.push_back(action);
    }

    const Json::Value& propertyVal = root["property"];
    for (auto member : propertyVal.getMemberNames()) {
        PropertyAction action;
        action.property = member;
        action.value = propertyVal[member].asString();
        std::cout << member << ":" <<propertyVal[member].asString() <<std::endl;
        mPropertyActions.push_back(action);
    }
    return true;
}

UserLogConfig& UserLogConfig::GetInstance() {

    // Deliberately leak this object to avoid a race between destruction on
    // process exit and concurrent access from another thread.
    static auto* instance = new UserLogConfig();
    return *instance;
}
