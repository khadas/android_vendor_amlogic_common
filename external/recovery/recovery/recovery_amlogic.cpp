/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
* *
This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
* *
Description:
*/

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <dirent.h>
#include "recovery_amlogic.h"

#include "set_display_mode.h"
extern "C" {
#include "uboot_env.h"
}
#define LOGE(...) fprintf(stdout, "E:" __VA_ARGS__)
#define LOGW(...) fprintf(stdout, "W:" __VA_ARGS__)
#define LOGI(...) fprintf(stdout, "I:" __VA_ARGS__)

#define NUM_OF_BLKDEVICE_TO_ENUM    3
#define NUM_OF_PARTITION_TO_ENUM    6

static const char *UDISK_COMMAND_FILE = "/udisk/factory_update_param.aml";
static const char *SDCARD_COMMAND_FILE = "/sdcard/factory_update_param.aml";


// write /proc/sys/vm/watermark_scale_factor 30
//write /proc/sys/vm/min_free_kbytes 12288
int write_sys(const char *path, const char *value, int len) {
    int fd = open(path, O_RDWR);
    if (fd < 0) {
        printf("open %s failed!\n", path);
        return -1;
    }

    int size = write(fd, value, len);
    if (size != len) {
        printf("write %s failed!\n", path);
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

void set_watermark_scale() {
    write_sys("/proc/sys/vm/watermark_scale_factor", "30", strlen("30"));
    write_sys("/proc/sys/vm/min_free_kbytes", "12288", strlen("12288"));
}

std::vector<char*> StringVectorToNullTerminatedArrayAml(const std::vector<std::string>& args) {
    std::vector<char*> result(args.size());
    std::transform(args.cbegin(), args.cend(), result.begin(),
                 [](const std::string& arg) { return const_cast<char*>(arg.c_str()); });
    result.push_back(nullptr);
    return result;
}

static int exec_cmd_aml(const std::vector<std::string>& args) {
    CHECK(!args.empty());
    auto argv = StringVectorToNullTerminatedArrayAml(args);

    pid_t child;
    if ((child = fork()) == 0) {
        execv(argv[0], argv.data());
        _exit(EXIT_FAILURE);
    }

    int status;
    waitpid(child, &status, 0);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        LOG(ERROR) << args[0] << " failed with status " << WEXITSTATUS(status);
    }
    return WEXITSTATUS(status);
}

int wipe_param(void) {
    //system/bin/mke2fs -F -t ext4 -b 4096 /dev/block/cache
    std::vector<std::string> mke2fs_args = {"/system/bin/mke2fs", "-F", "-t", "ext4", "-b", "4096", "/dev/block/param",};

    int result = exec_cmd_aml(mke2fs_args);
    return result;
}

static int mount_fs_rdonly(char *device_name, const char *mount_point, const char *fs_type) {
    std::string locale = "";
    if (!mount(device_name, mount_point, fs_type,
        MS_NOSUID | MS_NODEV | MS_RDONLY | MS_NOEXEC, locale.c_str())) {
        LOGW("successful to mount %s on %s by read-only\n",
            device_name, mount_point);
        return 0;
    } else {
        LOGE("failed to mount %s on %s by read-only (%s)\n",
            device_name, mount_point, strerror(errno));
    }

    return -1;
}

int auto_mount_fs(char *device_name, const char* mount_point, const char *fs_type) {
    if (access(device_name, F_OK)) {
        return -1;
    }
    std::string locale = "";

    if (!strcmp(fs_type, "auto")) {
        if (!mount(device_name, mount_point, "vfat",
            MS_NOATIME | MS_NODEV | MS_NODIRATIME, locale.c_str())) {
            goto auto_mounted;
        } else {
            if (strstr(mount_point, "sdcard")) {
                LOGW("failed to mount %s on %s (%s).try read-only ...\n",
                    device_name, mount_point, strerror(errno));
                if (!mount_fs_rdonly(device_name, mount_point, "vfat")) {
                    goto auto_mounted;
                }
            }
        }

        if (!mount(device_name, mount_point, "ntfs",
            MS_NOATIME | MS_NODEV | MS_NODIRATIME, locale.c_str())) {
            goto auto_mounted;
        } else {
            if (strstr(mount_point, "sdcard")) {
                LOGW("failed to mount %s on %s (%s).try read-only ...\n",
                    device_name, mount_point, strerror(errno));
                if (!mount_fs_rdonly(device_name, mount_point, "ntfs")) {
                    goto auto_mounted;
                }
            }
        }

        if (!mount(device_name, mount_point, "exfat",
            MS_NOATIME | MS_NODEV | MS_NODIRATIME, locale.c_str())) {
            goto auto_mounted;
        } else {
            if (strstr(mount_point, "sdcard")) {
                LOGW("failed to mount %s on %s (%s).try read-only ...\n",
                    device_name, mount_point, strerror(errno));
                if (!mount_fs_rdonly(device_name, mount_point, "exfat")) {
                    goto auto_mounted;
                }
            }
        }
    } else {
        if(!mount(device_name, mount_point,fs_type,
            MS_NOATIME | MS_NODEV | MS_NODIRATIME, locale.c_str())) {
            goto auto_mounted;
        } else {
            if (strstr(mount_point, "sdcard")) {
                LOGW("failed to mount %s on %s (%s).try read-only ...\n",
                    device_name, mount_point, strerror(errno));
                if (!mount_fs_rdonly(device_name, mount_point, fs_type)) {
                    goto auto_mounted;
                }
            }
        }
    }

    return -1;

auto_mounted:
    return 0;
}

int customize_smart_device_mounted(const char*blk_device, const char *mount_point,
    const char *fs_type) {
    int i = 0, j = 0;
    int first_position = 0;
    int second_position = 0;
    char * tmp = NULL;
    char *mounted_device = NULL;
    char device_name[256] = {0};
    char device_boot[256] = {0};
    const char *usb_device = "/dev/block/sd";
    const char *sdcard_device = "/dev/block/mmcblk";

    if (blk_device != NULL) {
        int num = 0;
        const char *blk_device_t = blk_device;
        for (; *blk_device_t != '\0'; blk_device_t ++) {
            if (*blk_device_t == '#') {
                num ++;
            }
        }

        /*
        * Contain two '#' for blk_device name in recovery.fstab
        * such as /dev/block/sd## (udisk)
        * such as /dev/block/mmcblk#p# (sdcard)
        */
        if (num != 2) {
            return 1;   // Don't contain two '#'
        }

        if (access(mount_point, F_OK)) {
            mkdir(mount_point, 0755);
        }

        // find '#' position
        if (strchr(blk_device, '#')) {
            tmp = strchr((char *)blk_device, '#');
            first_position = tmp - blk_device;
            if (strlen(tmp+1) > 0 && strchr(tmp+1, '#')) {
                tmp = strchr(tmp+1, '#');
                second_position = tmp - blk_device;
            }
        }

        if (!first_position || !second_position) {
            LOGW("decompose blk_device error(%s) in recovery.fstab\n",
                blk_device);
            return -1;
        }

        int copy_len = (strlen(blk_device) < sizeof(device_name)) ?
            strlen(blk_device) : sizeof(device_name);

        for (i = 0; i < NUM_OF_BLKDEVICE_TO_ENUM; i ++) {
            memset(device_name, '\0', sizeof(device_name));
            strncpy(device_name, blk_device, copy_len);

            if (!strncmp(device_name, sdcard_device, strlen(sdcard_device))) {
                // start from '0' for mmcblk0p#
                device_name[first_position] = '0' + i;
            } else if (!strncmp(device_name, usb_device, strlen(usb_device))) {
                // start from 'a' for sda#
                device_name[first_position] = 'a' + i;
            }

            for (j = 1; j <= NUM_OF_PARTITION_TO_ENUM; j ++) {
                device_name[second_position] = '0' + j;
                if (!access(device_name, F_OK)) {
                    LOGW("try mount %s ...\n", device_name);
                    if (!auto_mount_fs(device_name, mount_point, fs_type)) {
                        mounted_device = device_name;
                        LOGW("successful to mount %s\n", device_name);
                        goto mounted;
                    }
                }
            }

            if (!strncmp(device_name, sdcard_device, strlen(sdcard_device))) {
                // mmcblk0p1->mmcblk0
                device_name[strlen(device_name) - 2] = '\0';
                sprintf(device_boot, "%s%s", device_name, "boot0");
                // TODO: Here,need to distinguish between cards and flash at best
            } else if (!strncmp(device_name, usb_device, strlen(usb_device))) {
                // sda1->sda
                device_name[strlen(device_name) - 1] = '\0';
            }

            if (!access(device_name, F_OK)) {
                if (strlen(device_boot) && (!access(device_boot, F_OK))) {
                    continue;
                }

                LOGW("try mount %s ...\n", device_name);
                if (!auto_mount_fs(device_name, mount_point, fs_type)) {
                    mounted_device = device_name;
                    LOGW("successful to mount %s\n", device_name);
                    goto mounted;
                }
            }
        }
    } else {
        LOGE("Can't get blk_device\n");
    }

    return -1;

mounted:
    return 0;
}

int smart_device_mounted(const char *blk_device, const char *mount_point,
    const char *fs_type) {
    int i = 0, len = 0;
    char * tmp = NULL;
    char device_name[256] = {0};
    char *mounted_device = NULL;

    mkdir(mount_point, 0755);

    if (blk_device != NULL) {
        int ret = customize_smart_device_mounted(blk_device, mount_point, fs_type);
        if (ret <= 0) {
            return ret;
        }
    }

    if (blk_device != NULL) {
        tmp = strchr((char *)blk_device, '#');
        len = tmp - blk_device;
        if (tmp && len < 255) {
            strncpy(device_name, blk_device, len);
            for (i = 1; i <= NUM_OF_PARTITION_TO_ENUM; i++) {
                device_name[len] = '0' + i;
                device_name[len + 1] = '\0';
                LOGW("try mount %s ...\n", device_name);
                if (!access(device_name, F_OK)) {
                    if (!auto_mount_fs(device_name, mount_point, fs_type)) {
                        mounted_device = device_name;
                        LOGW("successful to mount %s\n", device_name);
                        goto mounted;
                    }
                }
            }

            const char *mmcblk = "/dev/block/mmcblk";
            if (!strncmp(device_name, mmcblk, strlen(mmcblk))) {
                device_name[len - 1] = '\0';
            } else {
                device_name[len] = '\0';
            }

            LOGW("try mount %s ...\n", device_name);
            if (!access(device_name, F_OK)) {
                if (!auto_mount_fs(device_name, mount_point, fs_type)) {
                    mounted_device = device_name;
                    LOGW("successful to mount %s\n", device_name);
                    goto mounted;
                }
            }
        } else {
            LOGW("try mount %s ...\n", blk_device);
            strncpy(device_name, blk_device, sizeof(device_name));
            if (!access(device_name, F_OK)) {
                if (!auto_mount_fs(device_name, mount_point, fs_type)) {
                    mounted_device = device_name;
                    LOGW("successful to mount %s\n", device_name);
                    goto mounted;
                }
            }
        }
    }

    return -1;

mounted:
    return 0;
}

int ensure_storage_mounted(const char* device, const char* mount_point) {
    int time_out = 2000000;
    while (time_out) {
        if (!smart_device_mounted(device, mount_point, "auto")) {
            return 0;
        }
        usleep(100000);
        time_out -= 100000;
    }

    return -1;
}

void ensure_external_mounted(const char* mount_point) {
    if (!strcmp(mount_point, AML_UDISK_ROOT)) {
        ensure_storage_mounted(UDISK_DEVICE, AML_UDISK_ROOT);
    } else if (!strcmp(mount_point, AML_SDCARD_ROOT)) {
        ensure_storage_mounted(SDCARD_DEVICE, AML_SDCARD_ROOT);
    } else {
        LOG(INFO) << "Just support for udisk or sdcard";
    }
}


void amlogic_get_args(std::vector<std::string>& args) {

    if (args.size() == 1) {
        std::string content;
        if (ensure_storage_mounted(UDISK_DEVICE, AML_UDISK_ROOT) == 0 &&
            android::base::ReadFileToString(UDISK_COMMAND_FILE, &content)) {

            std::vector<std::string> tokens = android::base::Split(content, "\n");
            for (auto it = tokens.begin(); it != tokens.end(); it++) {
                // Skip empty and '\0'-filled tokens.
                if (!it->empty() && (*it)[0] != '\0') {
                    int size = it->size();
                    if ((*it)[size-1] == '\r') {
                        (*it)[size-1] = '\0';
                    }
                    args.push_back(std::move(*it));
                }
            }
            LOG(INFO) << "Got " << args.size() << " arguments from " << UDISK_COMMAND_FILE;
        }
    }

    if (args.size() == 1) {
        std::string content;
        if (ensure_storage_mounted(SDCARD_DEVICE, AML_SDCARD_ROOT) == 0 &&
            android::base::ReadFileToString(SDCARD_COMMAND_FILE, &content)) {

            std::vector<std::string> tokens = android::base::Split(content, "\n");
            for (auto it = tokens.begin(); it != tokens.end(); it++) {
                // Skip empty and '\0'-filled tokens.
                if (!it->empty() && (*it)[0] != '\0') {
                    int size = it->size();
                    if ((*it)[size-1] == '\r') {
                        (*it)[size-1] = '\0';
                    }
                    args.push_back(std::move(*it));
                }
            }
            LOG(INFO) << "Got " << args.size() << " arguments from " << SDCARD_COMMAND_FILE;
        }
    }

    if (args.size() == 1) {
        args.push_back(std::move("--show_text"));
    }

}

void amlogic_init() {
    set_watermark_scale();
}

