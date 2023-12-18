#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>

#include <thread>
#include <string>
#include <sysutils/NetlinkEvent.h>

#include <cutils/android_filesystem_config.h>


#include <utils/Log.h>
#include <utils/Looper.h>

#include "NetlinkManager.h"
#include "UserLogConfig.h"
#include "signalHandler.h"

#define LOG_TAG "logging"

using android::Looper;
using android::sp;
using namespace android;

#define MSG_USB_DISK_PLUG_IN    1
#define MSG_USB_DISK_PLUG_OUT   2
#define MSG_POLLING_JSON_CONFIG 3
#define MSG_CONFIG_AND_LOGGING  4
#define MSG_STOP_LOGGING        5


#define UDISK_ROOT_PATH "/mnt/media_rw/"

static std::string getUserLogUdisk();

sp<Looper> gLooper;

class BlockEventHandler: public MessageHandler {
public:
    BlockEventHandler() {};
    static sp<BlockEventHandler> GetInstance() {
        static sp<BlockEventHandler> sinstance = new BlockEventHandler();
        return sinstance;
    }

    void handleMessage(const Message& message) {
        //ALOGD("msg.what=%d\n", message.what);
        static int checkCount = 0;
        switch (message.what) {
            case MSG_USB_DISK_PLUG_IN:
                gLooper->removeMessages(this, MSG_USB_DISK_PLUG_IN);
                if (getUserLogUdisk().size() < 1) {
                    // no files, polling later[TODO: with timeout...]
                    if (checkCount ++ < 120) {
                        gLooper->sendMessageDelayed(1000000000LL, this, message);
                    } else {
                       checkCount = 0;
                       ALOGE("not found!\n");
                    }
                } else {
                    checkCount = 0;
                    gLooper->sendMessage(this, MSG_POLLING_JSON_CONFIG);
                }
                break;

            case MSG_POLLING_JSON_CONFIG: {
                std::string p = getUserLogUdisk();
                if (p.size() < 1) {
                    ALOGE("error! when  try to read config, but not found! plugout the disk?\n");
                } else {
                    std::string configpath = std::string(UDISK_ROOT_PATH)+p+std::string("/userlog/log_config.json");
                    //UserLogConfig::GetInstance().updateLogPath(std::string configpath = std::string(UDISK_ROOT_PATH)+p+std::string("/userlog/");
                    bool result = UserLogConfig::GetInstance().Load(configpath);
                    ALOGD("load config ready res=%d\n", result);

                    gLooper->sendMessage(this, MSG_CONFIG_AND_LOGGING);
                }
                break;
            }
            case MSG_USB_DISK_PLUG_OUT:
                if (getUserLogUdisk().size() < 1) {
                    UserLogConfig::GetInstance().stopLogging();
                }
                break;

            case MSG_CONFIG_AND_LOGGING: {
                UserLogConfig::GetInstance().drainAllSysfsWriteAction();
                UserLogConfig::GetInstance().drainAllPropertyAction();
                int pid = UserLogConfig::GetInstance().startPersistLogcat();
                break;
                }

            case MSG_STOP_LOGGING:
                break;
        }

    }
};





static void testStorageOpenable() {
    FILE *fp = fopen("/storage/C884-45DC/log.json", "r");
    if (fp == nullptr) {
        ALOGE("fopen /storage/C884-45DC/log.json failed: %s\n", strerror(errno));
    } else {
        ALOGD("fopen /storage/C884-45DC/log.json success\n");
        fclose(fp);
    }

    fp = fopen("/mnt/media_rw/C884-45DC/log.json", "r");
    if (fp == nullptr) {
        ALOGE("fopen //mnt/media_rw/C884-45DC/log.json failed: %s\n", strerror(errno));
    } else {
        ALOGD("fopen //mnt/media_rw/C884-45DC/log.json success\n");
        fclose(fp);
    }

    fp = fopen("/mnt/androidwritable/0/C884-45DC/log.json", "r");
    if (fp == nullptr) {
        ALOGE("fopen //mnt/androidwritable/0/C884-45DC/log.json failed: %s\n", strerror(errno));
    } else {
        ALOGD("fopen //mnt/media_rw/C884-45DC/log.json success\n");
        fclose(fp);
    }
}



static std::string getUserLogUdisk() {
    DIR* d = opendir(UDISK_ROOT_PATH);
    if (d) {
        struct dirent* de;
        int dfd = dirfd(d);

        while ((de = readdir(d))) {
            DIR* d2;
            if (de->d_name[0] == '.') continue;
            ALOGD("UDISK %s\n", de->d_name);

            std::string jpath = std::string(UDISK_ROOT_PATH) + std::string(de->d_name) + std::string("/userlog/log_config.json");

            if (::access(jpath.c_str(), F_OK) == 0) {
                ALOGD("OK %s\n", jpath.c_str());

                closedir(d);
                return std::string(de->d_name);
            } else {
                ALOGE("falied access file %d\n", errno);
            }
        }
        closedir(d);
    }
    return std::string("");
}




int main(int argc, char** argv) {

    //fprintf (stderr, "start...\n");
    ALOGD("start...\n");
    signalHandlerInit();


    /* Test Code, test can read storage or not */
    //testStorageOpenable();

    sp<Looper> looper = new Looper(false);
    if (looper == NULL) {
        fprintf(stderr, "Error, cannot create looper for event handling");
        exit(-1);
    }

    gLooper = looper;


    NetlinkManager* nm;
    if (!(nm = NetlinkManager::Instance())) {
        fprintf(stderr, "Unable to create NetlinkManager");
        exit(1);
    }
    auto handler = [] (NetlinkEvent* evt) {
        ALOGD("%s -%s,%s\n", evt->getSubsystem(),
            evt->findParam("DEVPATH") ? evt->findParam("DEVPATH") : "",
            evt->findParam("DEVTYPE") ? evt->findParam("DEVTYPE") : "");

//        evt->dump();

        // TODO: blkid check label and uuid, handle properly
        switch (evt->getAction()) {
            case NetlinkEvent::Action::kAdd:
            case NetlinkEvent::Action::kChange:
                gLooper->sendMessage(BlockEventHandler::GetInstance(), MSG_USB_DISK_PLUG_IN);
                break;

            case NetlinkEvent::Action::kRemove:
                gLooper->sendMessage(BlockEventHandler::GetInstance(), MSG_USB_DISK_PLUG_OUT);
                break;
        }

    };

    if (nm->start(handler)) {
        fprintf(stderr, "Unable to start NetlinkManager");
        exit(-1);
    }



    // Do coldboot. Udisk may plugin before we
    Message msg;
    msg.what = MSG_USB_DISK_PLUG_IN;
    looper->sendMessage(BlockEventHandler::GetInstance(), msg);


    while (true) {
        int32_t ret = looper->pollOnce(-1);
    }
}

