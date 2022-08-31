#pragma once
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string>
#include <list>
#include <errno.h>

static inline bool ignoreCaseCompare(const std::string& a, const std::string& b) {
    if (a.length() == b.length()) {
        return std::equal(a.begin(), a.end(), b.begin(),
                          [](char a, char b) {
                              return tolower(a) == tolower(b);
                          });
    } else {
        return false;
    }
}

static inline int convertCcColor(int color) {
   return ((color&0x3)*85 |
        (((color&0xc)>>2)*85) << 8 |
        (((color&0x30)>>4)*85) << 16 |
        0xff<<24);
}


static inline std::string getApplicationPath() {
    int fd = open("/proc/self/cmdline", O_RDONLY | O_CLOEXEC);
    char procName[128];
    memset(procName, 0, sizeof(procName));
    read(fd, procName, sizeof(procName) - 1);

    if (strlen(procName) != 0) {
        if (access((std::string("/data/user/0/") + procName).c_str(), R_OK) == 0) {
            return std::string("/data/user/0/") + procName;
        }
    }

    return std::string("/data/user/0/android");
}


