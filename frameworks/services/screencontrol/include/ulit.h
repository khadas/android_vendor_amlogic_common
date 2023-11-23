#ifndef ANDROID_SCREENCONTROL_ULIT_H
#define ANDROID_SCREENCONTROL_ULIT_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include<time.h>
namespace android {

class DataDumper {

public:

    DataDumper(const char* path) {
        fd = open(path, O_CREAT|O_RDWR, 0666);

    }
    void dump(uint8_t * data,int32_t size) {
        if (data && size > 0 && fd >0) {
            write(fd,data,size);
        }


    }
    ~DataDumper() {
        if (fd > 0)
            close(fd);
    }

private:
    int fd = 0;
};
inline int64_t getNowTimesUs() {
    struct timespec now;
    clock_gettime(CLOCK_BOOTTIME, &now);
    int64_t now_time = (int64_t)now.tv_sec * 1000 * 1000 + (int64_t)now.tv_nsec / 1000;
    return now_time;
}


};
#endif // ANDROID_SCREENCONTROL_ULIT_H