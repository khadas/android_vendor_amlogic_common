/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: C++ file
 */

#define LOG_TAG "SystemControl"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <zlib.h>

#ifdef MTD_OLD
# include <linux/mtd/mtd.h>
#else
# define  __user	/* nothing */
# include <mtd/mtd-user.h>
#endif

#include "Ubootenv.h"
#include "common.h"


const char *PROFIX_UBOOTENV_VAR = "ubootenv.var.";

Ubootenv::Ubootenv() :
    mEnvLock(PTHREAD_MUTEX_INITIALIZER) {

    memset(mEnvPartitionName, 0, sizeof(mEnvPartitionName));
    init();
}

Ubootenv::~Ubootenv() {
    pthread_mutex_lock(&mEnvLock);

    if (mEnvData.image) {
        free(mEnvData.image);
        mEnvData.image = NULL;
        mEnvData.crc = NULL;
        mEnvData.data = NULL;
    }

    pthread_mutex_unlock(&mEnvLock);
}

int Ubootenv::updateValue(const char* name, const char* value) {
    if (!mEnvInitDone) {
        SYS_LOGE("[ubootenv] bootenv do not init\n");
        return -1;
    }

    if (!name || !value) {
        SYS_LOGE("[ubootenv] update value or name is null\n");
        return -1;
    }

    SYS_LOGI("[ubootenv] update value name [%s]: value [%s] \n", name, value);
    const char* envName = NULL;
    if (strcmp(name, "ubootenv.var.bootcmd") == 0) {
        envName = "bootcmd";
    } else {
        if (!isEnv(name)) {
            //should assert here.
            SYS_LOGE("[ubootenv] %s is not a ubootenv variable.\n", name);
            return -2;
        }
        envName = name + strlen(PROFIX_UBOOTENV_VAR);
    }

    pthread_mutex_lock(&mEnvLock);

    const char *envValue = get(envName);
    if (!envValue)
        envValue = "";

    if (!strcmp(value, envValue)) {
        pthread_mutex_unlock(&mEnvLock);
        return 0;
    }

    set(envName, value, true);

    int i = 0;
    int ret = -1;
    while (i < MAX_UBOOT_RWRETRY && ret < 0) {
        i ++;
        ret = save();
        if (ret < 0)
            SYS_LOGE("[ubootenv] Cannot write %s: %d.\n", mEnvPartitionName, ret);
    }

    if (i < MAX_UBOOT_RWRETRY) {
        SYS_LOGI("[ubootenv] Save ubootenv to %s succeed!\n", mEnvPartitionName);
    }

    pthread_mutex_unlock(&mEnvLock);

    return ret;
}

const char * Ubootenv::getValue(const char * key) {
    if (!isEnv(key)) {
        //should assert here.
        SYS_LOGE("[ubootenv] %s is not a ubootenv variable and need prefix:ubootenv.var.xxx\n", key);
        //print all env
        printValues();
        return NULL;
    }

    pthread_mutex_lock(&mEnvLock);
    const char* envName = key + strlen(PROFIX_UBOOTENV_VAR);
    const char* envValue = get(envName);
    pthread_mutex_unlock(&mEnvLock);
    return envValue;
}

void Ubootenv::printValues() {
    char *proc = mEnvData.data;
    char *nextProc;

    do {
        nextProc = proc + strlen(proc) + sizeof(char);
        SYS_LOGV("[ubootenv] printenv: %s\n", proc);

        if (!(*nextProc)) {
            SYS_LOGI("[ubootenv] printenv end\n");
            break;
        }
        proc = nextProc;
    } while(1);
}

void Ubootenv::dump(int fd) {
    char *proc = mEnvData.data;
    char *nextProc;
    do {
        dprintf(fd, "[ubootenv] env: [%s]\n", proc);
        nextProc = proc + strlen(proc) + sizeof(char);

        if (!(*nextProc)) {
            SYS_LOGI("[ubootenv] printenv end\n");
            break;
        }
        proc = nextProc;
    } while(1);
}

int Ubootenv::reInit() {
   pthread_mutex_lock(&mEnvLock);

   if (mEnvData.image) {
       free(mEnvData.image);
       mEnvData.image = NULL;
       mEnvData.crc = NULL;
       mEnvData.data = NULL;
   }

   pthread_mutex_unlock(&mEnvLock);

   init();

   return 0;
}

int Ubootenv::init() {
    const char *NAND_ENV = "/dev/nand_env";
    const char *BLOCK_ENV = "/dev/block/env";//for update from P
    const char *BLOCK_ENV_BYNAME = "/dev/block/by-name/env";//normally use that
    const char *BLOCK_UBOOT_ENV = "/dev/block/by-name/ubootenv";
    struct stat st;

    //the nand env or block env is the same
    mEnvPartitionSize = CONFIG_ENV_SIZE;
//#if defined(MESON8_ENVSIZE) || defined(GXBABY_ENVSIZE) || defined(GXTVBB_ENVSIZE) || defined(GXL_ENVSIZE)
//    mEnvPartitionSize = 0x10000;
//#endif
    mEnvSize = mEnvPartitionSize - sizeof(uint32_t);

    if (!stat(NAND_ENV, &st)) {
        strcpy (mEnvPartitionName, NAND_ENV);
    }
    else if (!stat(BLOCK_ENV, &st)) {
        strcpy (mEnvPartitionName, BLOCK_ENV);
    }
    else if (!stat(BLOCK_ENV_BYNAME, &st)) {
        strcpy (mEnvPartitionName, BLOCK_ENV_BYNAME);
    }
    else if (!stat(BLOCK_UBOOT_ENV, &st)) {
        int fd;
        struct mtd_info_user info;

        strcpy (mEnvPartitionName, BLOCK_UBOOT_ENV);
        if ((fd = open(mEnvPartitionName, O_RDWR)) < 0) {
            SYS_LOGE("[ubootenv] open device(%s) error\n", mEnvPartitionName );
            return -2;
        }

        memset(&info, 0, sizeof(info));
        int err = ioctl(fd, MEMGETINFO, &info);
        if (err < 0) {
            SYS_LOGE("[ubootenv] get MTD info error\n" );
            close(fd);
            return -3;
        }
        close(fd);

        //mEnvEraseSize = info.erasesize;//0x20000;//128K
        mEnvPartitionSize = info.size;//0x8000;
        mEnvSize = mEnvPartitionSize - sizeof(long);
    }

    //the first four bytes are crc value, others are data
    SYS_LOGI("[ubootenv] using %s with size(%d) (%d)", mEnvPartitionName, mEnvPartitionSize, mEnvSize);

    int i = 0;
    int ret = -1;
    while (i < MAX_UBOOT_RWRETRY && ret < 0) {
        i ++;
        ret = readPartitionData();
        if (ret < 0)
            SYS_LOGE("[ubootenv] Cannot read %s: %d.\n", mEnvPartitionName, ret);
        if (ret < -2)
            free(mEnvData.image);
    }

    if (i >= MAX_UBOOT_RWRETRY) {
        SYS_LOGE("[ubootenv] read %s failed \n", mEnvPartitionName);
        return -2;
    }

    printValues();

    mEnvInitDone = true;
    return 0;
}

int Ubootenv::readPartitionData() {
    int fd;
    int flag = 0;
    int seeknum;
    if ((fd = open(mEnvPartitionName, O_RDONLY)) < 0) {
        SYS_LOGE("[ubootenv] open devices error: %s\n", strerror(errno));
        return -1;
    }

    char *addr = (char *)malloc(mEnvPartitionSize);
    if (addr == NULL) {
        SYS_LOGE("[ubootenv] Not enough memory for environment (%u bytes)\n", mEnvPartitionSize);
        close(fd);
        return -2;
    }

    pthread_mutex_lock(&mEnvLock);

    memset(addr, 0, mEnvPartitionSize);
    mEnvData.image = addr;
    struct env_image *image = (struct env_image *)addr;
    mEnvData.crc = &(image->crc);
    mEnvData.data = image->data;

    int ret = read(fd ,mEnvData.image, mEnvPartitionSize);
    if (ret == (int)mEnvPartitionSize) {
        uint32_t crcCalc = crc32(0, (uint8_t *)mEnvData.data, mEnvSize);
        if (crcCalc != *(mEnvData.crc)) {
            SYS_LOGE("[ubootenv] CRC Check fail save_crc=%08x, crcCalc = %08x \n",
                *mEnvData.crc, crcCalc);
            flag = -3;
        }
    } else {
        SYS_LOGE("[ubootenv] read error 0x%x \n",ret);
        flag = -5;
    }

    if (flag != 0) {
        SYS_LOGI("first env error, try second....\n");
        seeknum = lseek(fd, CONFIG_ENV_OFFSET_REDUND, SEEK_SET);
        if (seeknum != CONFIG_ENV_OFFSET_REDUND) {
            SYS_LOGE("[ubootenv] lseek error, seeknum = %d \n", seeknum);
        }
        int ret2 = read(fd ,mEnvData.image, CONFIG_ENV_OFFSET_REDUND);
        if (ret2 == (int)mEnvPartitionSize) {
            uint32_t crcCalc = crc32(0, (uint8_t *)mEnvData.data, mEnvSize);
            if (crcCalc != *(mEnvData.crc)) {
                SYS_LOGE("[ubootenv] CRC2 Check fail save_crc=%08x, crcCalc = %08x \n",
                    *mEnvData.crc, crcCalc);
                close(fd);
                return -3;
            }
        }
    }

    pthread_mutex_unlock(&mEnvLock);

exit:
    close(fd);
    return 0;
}

char * Ubootenv::get(const char * key) {
    char *proc = mEnvData.data;
    char *nextProc = NULL;
    char *envValue = NULL;
    char envKey[128];

    if (!mEnvInitDone) {
        SYS_LOGE("[ubootenv] don't init done\n");
        return NULL;
    }

    do {
        //SYS_LOGV("[ubootenv] proc: %s \n", proc);
        envValue = strchr(proc, '=');
        if (envValue) {
            int offset = envValue - proc;
            memset(envKey, 0 , sizeof(envKey));
            if (offset*sizeof(char) < sizeof(envKey)) {
                memcpy(envKey, proc, offset*sizeof(char));
            } else {
                SYS_LOGE("[ubootenv] env key :%s size is larger than 128 bytes and only copy 127 bytes\n", proc);
                memcpy(envKey, proc, sizeof(envKey) - 1);
            }

            if (!strcmp(envKey, key)) {
                envValue += sizeof(char);
                SYS_LOGI("[ubootenv]  get key:%s envValue: %s\n", key, envValue);
                break;
            } else {
                envValue = NULL;
            }
        }

        nextProc = proc + strlen(proc) + sizeof(char);
        if (!(*nextProc)) {
            SYS_LOGI("[ubootenv] search end and not find env:%s\n", key);
            break;
        }
        proc = nextProc;
    } while(1);

    return envValue;
}

/*
creat_args_flag : if true , if envvalue don't exists Creat it .
              if false , if envvalue don't exists just exit .
*/
int Ubootenv::set(const char * key,  const char * value, bool createNew) {
    //malloc size > the active env size and keep the '\0' end
    char *mEnvData_Backup = (char *)malloc(mEnvPartitionSize);
    if (mEnvData_Backup == NULL) {
        SYS_LOGE("[ubootenv] Not enough memory for environment (%u bytes)\n", mEnvSize);
        return -1;
    }

    if (strlen(key)>= 128 || strlen(value) >= 4096) {
        SYS_LOGE("[ubootenv] Invalid env data key:%s, value:%s size is larger\n", key, value);
        return -1;
    }

    memset(mEnvData_Backup, 0, mEnvPartitionSize);
    memcpy(mEnvData_Backup, mEnvData.data, mEnvSize);
    memset(mEnvData.data, 0, mEnvSize);

    int len = 0;
    bool find = false;
    char *data = mEnvData.data;
    char *proc = mEnvData_Backup;
    char *nextProc;
    char envKey[128];
    char envValue[4096];

    //parse key and value and if key have and replace value
    do {
        //SYS_LOGV("[ubootenv] set proc: %s \n", proc);
        nextProc = proc + strlen(proc) + sizeof(char);
        char *del = strchr(proc, (int)'=');
        if (del != NULL) {
            memset(envKey, 0, sizeof(envKey));
            memset(envValue, 0, sizeof(envValue));
            *del=0;
            if (strlen(proc) < sizeof(envKey)) {
                strcpy(envKey, proc);
            } else {
                SYS_LOGE("[ubootenv] env key :%s size is larger than 128 bytes and only copy 127 bytes\n", proc);
                strncpy(envKey, proc, sizeof(envKey) - 1);
            }

            if (!strcmp(envKey, key)) {
                strcpy(envValue, value);
                find = true;
            } else {
                char *value = del + sizeof(char);
                if (strlen(value) < sizeof(envValue)) {
                    strcpy(envValue, del + sizeof(char));
                } else {
                    SYS_LOGE("[ubootenv] env data :%s size is larger than 4096 bytes and only copy 4095 bytes\n", value);
                    strncpy(envValue, del + sizeof(char), sizeof(envValue) - 1);
                }
            }

            len = sprintf(data, "%s=%s", envKey, envValue);
            if (len < (int)(sizeof(char)*3)) {
                SYS_LOGE("[ubootenv] Invalid env data key:%s, value:%s\n", envKey, envValue);
            } else {
                data += len + sizeof(char);
            }
        }

        if (!(*nextProc)) {
            break;
        }
        proc = nextProc;
    } while(1);

    //append the key and value at end of env data if not find the key
    if (!find && createNew) {
        len = sprintf(data, "%s=%s", key, value);
        if (len < (int)(sizeof(char)*3)) {
            SYS_LOGE("[ubootenv] Invalid env data key:%s, value:%s\n", key, value);
        } else {
            data += len + sizeof(char);
        }
    }

    printValues();

    free(mEnvData_Backup);
    return 0;
}

//save value to storage flash
int Ubootenv::save() {
    int fd;
    int err;
    int lseeknum;

    *(mEnvData.crc) = crc32(0, (uint8_t *)mEnvData.data, mEnvSize);

    if ((fd = open (mEnvPartitionName, O_RDWR)) < 0) {
        SYS_LOGE("[ubootenv] open devices error\n");
        return -1;
    }

    if (strstr (mEnvPartitionName, "mtd")) {
        struct erase_info_user erase;
        struct mtd_info_user info;
        unsigned char *data = NULL;

        memset(&info, 0, sizeof(info));
        err = ioctl(fd, MEMGETINFO, &info);
        if (err < 0) {
            SYS_LOGE("[ubootenv] Get MTD info error\n");
            close(fd);
            return -4;
        }

        erase.start = 0;
        if (info.erasesize > ((unsigned int)mEnvPartitionSize * 2)) {
            data = (unsigned char*)malloc(info.erasesize);
            if (data == NULL) {
                SYS_LOGE("[ubootenv] Out of memory!!!\n");
                close(fd);
                return -5;
            }
            memset(data, 0, info.erasesize);
            err = read(fd, (void*)data, info.erasesize);
            if (err != (int)info.erasesize) {
                SYS_LOGE("[ubootenv] Read access failed !!!\n");
                free(data);
                close(fd);
                return -6;
            }
            memcpy(data, mEnvData.image, mEnvPartitionSize);
            memcpy(data + CONFIG_ENV_OFFSET_REDUND, mEnvData.image, mEnvPartitionSize);
            erase.length = info.erasesize;
        }
        else {
            erase.length = mEnvPartitionSize * 2;
        }

        err = ioctl (fd, MEMERASE,&erase);
        if (err < 0) {
            SYS_LOGE ("[ubootenv] MEMERASE SYS_LOGE %d\n",err);
            free(data);
            close(fd);
            return  -2;
        }

        if (info.erasesize > (unsigned int)mEnvPartitionSize) {
            err = lseek(fd, 0L, SEEK_SET);
            if (err < 0) {
                SYS_LOGE("%s() %d: err is %d\n", __func__, __LINE__, err);
                free(data);
                close(fd);
                return -7;
            }
            if (data != NULL)
                err = write(fd , data, info.erasesize);
            else
                SYS_LOGE("data is NULL\n");
            free(data);
        }
        else {
            err = write(fd ,mEnvData.image, mEnvPartitionSize);
            lseeknum = lseek(fd, CONFIG_ENV_OFFSET_REDUND, SEEK_SET);
            if (lseeknum != CONFIG_ENV_OFFSET_REDUND) {
                SYS_LOGE("[ubootenv] can not lseek, seek num = %d \n", lseeknum);
            }
            err = write(fd ,mEnvData.image, mEnvPartitionSize);
            free(data);
        }

    } else {
        //emmc and nand needn't erase
        lseek(fd, 0L, SEEK_SET);
        err = write(fd, mEnvData.image, mEnvPartitionSize);
        lseeknum = lseek(fd, CONFIG_ENV_OFFSET_REDUND, SEEK_SET);
        if (lseeknum != CONFIG_ENV_OFFSET_REDUND) {
            SYS_LOGE("[ubootenv] lseek error, seek num = %d \n", lseeknum);
        }
        err = write(fd ,mEnvData.image, mEnvPartitionSize);
    }

    close(fd);
    if (err < 0) {
        SYS_LOGE ("[ubootenv] SYS_LOGE write, size %d \n", mEnvPartitionSize);
        return -3;
    }
    return 0;
}

int Ubootenv::isEnv(const char* prop_name) {
    if (!prop_name || !(*prop_name))
        return 0;

    if (!(*PROFIX_UBOOTENV_VAR))
        return 0;

    if (strncmp(prop_name, PROFIX_UBOOTENV_VAR, strlen(PROFIX_UBOOTENV_VAR)) == 0
        && strlen(prop_name) > strlen(PROFIX_UBOOTENV_VAR) )
        return 1;

    return 0;
}
