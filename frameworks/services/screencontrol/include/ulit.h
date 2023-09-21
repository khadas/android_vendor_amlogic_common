#ifndef ANDROID_SCREENCONTROL_ULIT_H
#define ANDROID_SCREENCONTROL_ULIT_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
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


};
#endif // ANDROID_SCREENCONTROL_ULIT_H