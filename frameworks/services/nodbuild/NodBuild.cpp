/*
** Copyright 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#include "logtag.h"
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define AMLSEC_MKL_MAJOR "/sys/module/amlsec_mkl/parameters/mkl_drv_major"
#define AMLSEC_MKL_MINOR "/sys/module/amlsec_mkl/parameters/mkl_drv_num_minors"
#define CACERT_DRV_MAJOR "/sys/module/aml_ca_cert/parameters/cacert_drv_major"
#define CACERT_DRV_MINOR "/sys/module/aml_ca_cert/parameters/cacert_drv_num_minors"
#define AML_FILE_CACERT "/vendor/lib/modules/amlsec_mkl.ko"
#define KO_FILE_CACERT "/vendor/lib/modules/aml-ca-cert.ko"


enum NOD_CMC {
    amlsec = 0,
    cacert,
    cmd_max
};

static const char *NOD_CMD_NAME[cmd_max] = {
    "amlsec",
    "cacert",
};
unsigned int check_cmd(const char *cmd_name)
{
    for (int i = 0; i < cmd_max; i++) {
        if (strcmp(cmd_name, NOD_CMD_NAME[i]) == 0)
            return i;
    }
    return -1;
}

int readSys(const char *path, char *buf, int count) {
    int fd, len = -1;

    if ( NULL == buf ) {
        SYS_LOGE("buf is NULL");
        return len;
    }

    if ((fd = open(path, O_RDONLY)) < 0) {
        SYS_LOGE("readSys, open %s fail. Error info [%s]", path, strerror(errno));
        return len;
    }

    len = read(fd, buf, count);
    if (len < 0) {
        SYS_LOGE("read error: %s, %s\n", path, strerror(errno));
    }

    close(fd);
    return len;
}
int readSysfsInt(const char *path) {
    char buf[MAX_STR_LEN] = {0};
    readSys(path, (char*)buf, MAX_STR_LEN);
    return atoi(buf);
}
int mknodbuild(const char* nod_pref,size_t major,size_t minor) {
    if (major < 0 || minor < 0 || major < minor) {
        return -1;
    }
    int ret = -1;
    char *path = (char*)malloc(strlen(nod_pref) + 1);
    if (path == NULL) return -1;
    for (int i = 0; i < minor; i++) {
        sprintf(path, "%s%d", nod_pref, i);
        ret = unlink(path);
        SYS_LOGE("unlink %s, get %d", path, ret);
        dev_t dev = makedev(major, i);
        ret = mknod(path, S_IFCHR |0600, dev);
        SYS_LOGE("mknod %s, get %d", path, ret);
    }
    free(path);
    return ret;
}
int runningDev(const char* argv) {
   int major = -1;
   int minor = -1;
   switch (check_cmd(argv)) {
        case amlsec:
            major = readSysfsInt(AMLSEC_MKL_MAJOR);
            minor = readSysfsInt(AMLSEC_MKL_MINOR);
            return mknodbuild("/dev/amlsec-mkl", major, minor);
            break;
        case cacert:
            major = readSysfsInt(CACERT_DRV_MAJOR);
            minor = readSysfsInt(CACERT_DRV_MINOR);
            return mknodbuild("/dev/nocs-ca-cert", major, minor);
            break;
        default:
            return -1;
            break;
   }
    return 0;
}

int main(int argc, char **argv)
{
    int ret = 0;
    ret = runningDev("cacert");
    SYS_LOGE("create cacert ret %d", ret);
    ret = runningDev("amlsec");
    SYS_LOGE("create amlsec ret %d", ret);
    return 0;
}
