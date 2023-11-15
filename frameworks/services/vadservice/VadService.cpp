/*
 * Copyright (C) 2023 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *  @author   Hongchao Yin
 *  @version  2.0
 *  @date     2023/11/14
 *  @par function description:
 *  - 1 open vad related devices first before enter freeze mode
 */

#define LOG_TAG "vadservice"

#include <android-base/logging.h>
#include <cutils/properties.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>

extern "C" {
#include "aml_alsa_mixer.h"
#include "aml_vad_wakeup.h"
#include "alsa_device_parser.h"
}

#define ANDROID_RB_PROPERTY "sys.powerctl"

#define PATH_CMD_LINE "/proc/cmdline"
#define PATH_REBOOT_REASON "/sys/devices/platform/reboot/reboot_reason"
#define PATH_FREEZE "/sys/power/state"
#define PATH_HDCP_ESM "/sys/module/tvin_hdmirx/parameters/hdcp22_kill_esm"

#define BUF_LEN_MAX    (4096)
#define BUF_LEN_NORMAL    (32)

static int                     g_kernel_log_fd = -1;

static void VadAborter(const char* abort_message) {
    android::base::DefaultAborter(abort_message);
}

void VadKernelLogging(char* argv[]) {
    // Make stdin/stdout/stderr all point to /dev/null.
    g_kernel_log_fd = open("/sys/fs/selinux/null", O_RDWR);
    if (g_kernel_log_fd == -1) {
        int saved_errno = errno;
        android::base::InitLogging(argv, &android::base::KernelLogger, VadAborter);
        errno = saved_errno;
        PLOG(FATAL) << "Couldn't open /sys/fs/selinux/null";
        return;
    }
    dup2(g_kernel_log_fd, 0);
    dup2(g_kernel_log_fd, 1);
    dup2(g_kernel_log_fd, 2);
    if (g_kernel_log_fd > 2) close(g_kernel_log_fd);

    android::base::InitLogging(argv, &android::base::KernelLogger, VadAborter);
}

void writeSys(const char *path, const char *val) {
    int fd;

    if ((fd = open(path, O_RDWR)) < 0) {
        LOG(ERROR) << "writeSysFs, open fail:" << path;
        return;
    }
    write(fd, val, strlen(val));
    close(fd);
}

int readSys(const char *path, char *buf, int count) {
    int fd, len = -1;

    if (NULL == buf) {
        LOG(ERROR) << "readSys, buf is null";
        return len;
    }

    if ((fd = open(path, O_RDONLY)) < 0) {
        LOG(ERROR) << "readSys, open fail: " << path << ", error=" << strerror(errno);
        return len;
    }

    len = read(fd, buf, count - 1);
    if (len < 0 || len > count - 1) {
        LOG(ERROR) << "readSys, len:%d"<< len << ", read error: " << path << ", error=" << strerror(errno);
        len = -1;
    } else {
        buf[len] = '\0';
    }
    close(fd);
    return len;
}

int processBuffer(char *t_buf, char split_ch, char *t_name, char *t_value) {
    char *tmp_ptr = NULL, *tmp_name = NULL, *tmp_value = NULL;

    tmp_ptr = t_buf;
    while (tmp_ptr && *tmp_ptr) {
        char *x = strchr(tmp_ptr, split_ch);
        if (x != NULL) {
            *x++ = '\0';
        }
        tmp_name = tmp_ptr;
        if (tmp_name[0] != '\0') {
            tmp_value = strchr(tmp_ptr, '=');
            if (tmp_value != NULL) {
                *tmp_value++ = '\0';
                if (strncmp(tmp_name, t_name, strlen(t_name)) == 0) {
                    strncpy(t_value, tmp_value, BUF_LEN_NORMAL);
                    return 0;
                }
            }
        }
        tmp_ptr = x;
    }
    return -1;
}

int processReadFile(char *file_path, int offset, char *pBuf, int len) {
    int tmp_cnt = 0;
    int dev_fd = open(file_path, O_RDONLY);
    if (dev_fd >= 0) {
        off_t ret = lseek(dev_fd, offset, SEEK_SET);
        if (ret == -1) {
            LOG(ERROR) << "processReadFile lseek fail";
            close(dev_fd);
            return 0;
        }
        tmp_cnt = read(dev_fd, pBuf, len);
        if (tmp_cnt < 0)
            tmp_cnt = 0;
        /* get rid of trailing newline, it happens */
        if (tmp_cnt > 0 && pBuf[tmp_cnt - 1] == '\n')
            tmp_cnt--;
        pBuf[tmp_cnt] = 0;
        close(dev_fd);
    } else {
        pBuf[0] = 0;
    }

    return tmp_cnt;
}

int processFile(char *t_fname, char split, char *t_name, char *t_value) {
    char line_buf[BUF_LEN_MAX];

    processReadFile(t_fname, 0, line_buf, BUF_LEN_MAX);
    return processBuffer(line_buf, split, t_name, t_value);
}

void vadReboot(void) {
    LOG(ERROR) << "ffv normal reboot";
    if (true) {
        writeSys(PATH_HDCP_ESM, "1");
        usleep(50 * 1000);
        syscall(__NR_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2,
                LINUX_REBOOT_CMD_RESTART2, "normal");
    } else {
        property_set(ANDROID_RB_PROPERTY, "normal");
    }
}

bool isCoolboot() {
    char buf[BUF_LEN_NORMAL] = { 0 };

    int ret = readSys(PATH_REBOOT_REASON, buf, BUF_LEN_NORMAL);
    if (ret < 0) {
        LOG(ERROR) << "isCoolboot: readSys fail.";
        return false;
    }
    LOG(ERROR) << "reboot reason is " << buf;
    if (strncmp(buf, "0", 1) == 0) {    // cold boot
        return true;
    } else if (strncmp(buf, "15", 2) == 0) { // ffv reboot
        return true;
    }
    return false;
}

bool isFFVFreezeMode() {
    char buf[BUF_LEN_NORMAL] = { 0 };

    memset(buf, 0, BUF_LEN_NORMAL);
    if (processFile((char*)PATH_CMD_LINE, ' ', (char*)"ffv_freeze", buf) == 0) {
        if (strncmp(buf, "on", 2) == 0)
            return true;
    }
    return false;
}

int32_t aml_vad_check_sound_card() {
    int card = -1;
    bool is_snd_dev_ok = false;
    int tmp_cnt = 0;
    while (1) {
        if (card == -1) {
            card = alsa_device_get_card_index();
        } else {
            if (is_snd_dev_ok == false) {
                char fn[256];
                snprintf(fn, sizeof(fn), "/dev/snd/controlC%u", card);
                if (access(fn, R_OK | W_OK) == 0) {
                    is_snd_dev_ok = true;
                } else {
                    LOG(ERROR) << "aml_vad_check_sound_card dev: " << fn << " not found.";
                }
            }
        }
        if (card != -1 && is_snd_dev_ok) {
            break;
        }
        LOG(android::base::INFO) << "aml_vad_check_sound_card get card again ...";
        if (tmp_cnt++ >= 100) {
            LOG(ERROR) << "aml_vad_check_sound_card try get card timeout 10s ...";
            return -1;
        }
        usleep(100 * 1000);
    }
    LOG(ERROR) << "aml_vad_check_sound_card card id: " << card;
    return 0;
}

int main(int argc __attribute__((unused)), char **argv __attribute__((unused))) {
    int32_t ret = 0;

    VadKernelLogging(argv);
    LOG(ERROR) << "VadService starting...";
    if (/*isCoolboot() && */isFFVFreezeMode()) {
        LOG(ERROR) << "VadService ffv freeze mode";
        ret = aml_vad_check_sound_card();
        if (ret != 0) {
            LOG(ERROR) << "VadService check sound card fail";
            return -1;
        }
        struct aml_mixer_handle alsa_mixer;
        memset(&alsa_mixer, 0, sizeof(struct aml_mixer_handle));
        open_mixer_handle(&alsa_mixer);
        aml_vad_suspend(&alsa_mixer);

        LOG(ERROR) << "VadService enter the freeze mode.";
        writeSys(PATH_FREEZE, "freeze"); // block here
        aml_vad_resume(&alsa_mixer);
        LOG(ERROR) << "VadService starting dump data";

        // mount /mnt/vendor/param to /data/
        ret = system("mount -t ext4 /dev/block/by-name/param /data/");
        if (ret != 0) {
            LOG(ERROR) << "VadService mount /mnt/vendor/param fail. ret:" << ret;
        } else {
            aml_vad_dump(true/* block */);
            ret = system("sync; umount /data");
            if (ret != 0) {
                LOG(ERROR) << "VadService umount /mnt/vendor/param fail. ret:" << ret;
            }
        }
        LOG(ERROR) << "VadService exit, and reboot";
        vadReboot();
    } else {
        LOG(ERROR) << "VadService exit. Normal startup.";
    }
    if (g_kernel_log_fd != -1) {
        close(g_kernel_log_fd);
    }
    return 0;
}

