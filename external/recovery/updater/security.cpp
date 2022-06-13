/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
* *
This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
* *
Description:
*/

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <string.h>
#include <android-base/properties.h>
#include <ziparchive/zip_archive.h>
#include <android-base/logging.h>

#include "cutils/properties.h"
#include "security.h"
#include "cutils/android_reboot.h"
#include "uboot_env.h"

#define CMD_SECURE_CHECK _IO('d', 0x01)
#define CMD_DECRYPT_DTB  _IO('d', 0x02)

int SetDtbEncryptFlag(const char *flag) {
    int len = 0;
    int fd = open(DECRYPT_DTB, O_RDWR);
    if (fd < 0) {
        printf("open %s failed!\n", DECRYPT_DTB);
        return -1;
    }

    len = write(fd, flag, 1);
    if (len != 1) {
        printf("write %s failed!\n", DECRYPT_DTB);
        close(fd);
        fd = -1;
        return -1;
    }

    close(fd);

    return 0;
}

int SetDtbEncryptFlagByIoctl(const char *flag) {
    int ret = -1;
    unsigned long operation = 0;
    if (!strcmp(flag, "1") ) {
        operation = 1;
    } else {
        operation = 0;
    }

    int fd = open(DEFEND_KEY, O_RDWR);
    if (fd < 0) {
        printf("open %s failed!\n", DEFEND_KEY);
        return -1;
    }

    ret = ioctl(fd, CMD_DECRYPT_DTB, &operation);
    close(fd);

    return ret;
}

int GetSecureBootImageSize(const unsigned char *buffer, const int size) {

    int len = size;
    int encrypt_size = size;
    unsigned char *pstart = (unsigned char *)buffer;

    //"avbtool"
    const unsigned char avbtool[] = { 0x61, 0x76, 0x62, 0x74, 0x6F, 0x6F, 0x6C };

    while (len > 8) {
        if (!memcmp(pstart, avbtool, 7)) {
            printf("find avb0 flag,already enable avb function !\n");
            //encrypt image offset 148 bytes
            pstart += 148;
            //get encrypt image size
            encrypt_size = (pstart[0] << 24) + (pstart[1] << 16) + (pstart[2] << 8) + pstart[3];
            printf("encrypt_size = %d\n", encrypt_size);
            break;
        }
        pstart += 8;
        len -= 8;
    }

    return encrypt_size;
}

/**
  *  --- judge platform whether match with zip image or not
  *
  *  @imageName:   image name
  *  @imageBuffer: image data address
  *  @imageSize:   image data size
  *
  *  return value:
  *  <0: failed
  *  =0: not match
  *  >0: match
  */
static int IsPlatformMatchWithImage(
        const char *imageName,
        const unsigned char *imageBuffer,
        const int imageSize)
{
    int fd = -1;
    ssize_t result = -1;
    int encrypt_size = 0;
    int ret = SECURE_ERROR;

    fd = open(DEFEND_KEY, O_RDWR);
    if (fd < 0) {
        printf("open %s failed (%s)\n", DEFEND_KEY, strerror(errno));
        return ret;
    }

    //if enable avb, need get the real encrypt data size
    encrypt_size = GetSecureBootImageSize(imageBuffer, imageSize);

    //already enable avb2
    printf("imageSize=%d, encrypt_size=%d\n",imageSize, encrypt_size);
    if (encrypt_size != imageSize) {
        //if not bootloader.img, skip, because if enable avb2, just encrypted bootloader.img
        if (strcmp(imageName, BOOTLOADER_IMG)) {
            printf("enable avb2, skip %s defendkey check\n", imageName);
            return SECURE_MATCH;
        }
    }

    result = write(fd, imageBuffer, encrypt_size);// check rsa
    printf("write %s datas to %s. [imgsize:%d, result:%d]\n", imageName, DEFEND_KEY, encrypt_size, result);
    if (result == 1) {
        ret = SECURE_MATCH;
    } else if (result == -2){
        ret = SECURE_FAIL;
    }

    close(fd);
    fd = -1;

    return ret;

}


/**
  *  --- judge platform whether match with zip image or not
  *
  *  @platformEncryptStatus: 0: platform unencrypted, 1: platform encrypted
  *  @imageEncryptStatus:    0: image unencrypted, 1: image encrypted
  *  @imageName:   image name
  *  @imageBuffer: image data address
  *  @imageSize:   image data size
  *
  *  return value:
  *  <0: failed
  *  =0: not match
  *  >0: match
  */
static int IsPlatformMachWithZipArchiveImage(
        const int platformEncryptStatus,
        const int imageEncryptStatus,
        const char *imageName,
        const unsigned char *imageBuffer,
        const int imageSize)
{
    int fd = -1, ret = -1;
    ssize_t result = -1;
    int encrypt_size = 0;

    if (strcmp(imageName, BOOT_IMG) &&
        strcmp(imageName, RECOVERY_IMG) &&
        strcmp(imageName, BOOTLOADER_IMG)) {
        printf("can't support %s at present\n",
            imageName);
        return -1;
    }

    if (imageBuffer == NULL) {
        printf("havn't malloc space for %s\n",
            imageName);
        return -1;
    }

    if (imageSize <= 0) {
        printf("%s size is %d\n",
            imageName, imageSize);
        return -1;
    }

    switch (platformEncryptStatus) {
        case 0: {
            if (!imageEncryptStatus) {
                ret = 1;
            } else {
                ret = 0;
            }
            break;
        }

        case 1: {
            if (!imageEncryptStatus) {
                ret = 0;
            } else {
                fd = open(DEFEND_KEY, O_RDWR);
                if (fd < 0) {
                    printf("open %s failed (%s)\n",
                        DEFEND_KEY, strerror(errno));
                    return -1;
                }

                //if enable avb, need get the real encrypt data size
                encrypt_size = GetSecureBootImageSize(imageBuffer, imageSize);

                result = write(fd, imageBuffer, encrypt_size);// check rsa
                printf("write %s datas to %s. [imgsize:%d, result:%d, %s]\n",
                    imageName, DEFEND_KEY, imageSize, result,
                    (result == 1) ? "match" :
                    (result == -2) ? "not match" : "failed or not support");
                if (result == 1) {
                    ret = 1;
                } else if(result == -2) {
                    ret = 0;
                } else {    // failed or not support
                    ret = -1;
                }
                close(fd);
                fd = -1;
            }
            break;
        }
    }

    return ret;
}

/**
  *  --- check bootloader.img whether encrypt or not
  *
  *  @imageName:   bootloader.img
  *  @imageBuffer: bootloader.img data address
  *
  *  return value:
  *  <0: failed
  *  =0: unencrypted
  *  >0: encrypted
  */
static int IsBootloaderImageEncrypted(
        const char *imageName,
        const unsigned char *imageBuffer)
{
    int step0=1;
    int step1=0;
    int index=0;
    const unsigned char *pstart = NULL;
    const unsigned char *pImageAddr = imageBuffer;
    const unsigned char *pEncryptedBootloaderInfoBufAddr = NULL;

    // Don't modify. unencrypt bootloader info, for kernel version 3.14
    const int newbootloaderEncryptInfoOffset = 0x10;
    const int newbootloaderEncryptInfoOffset1 = 0x70;
    const unsigned char newunencryptedBootloaderInfoBuf[] = { 0x40, 0x41, 0x4D, 0x4C};

    if (strcmp(imageName, BOOTLOADER_IMG)) {
        printf("this image must be %s,but it is %s\n",
            BOOTLOADER_IMG, imageName);
        return -1;
    }

    if (imageBuffer == NULL) {
        printf("havn't malloc space for %s\n",
            imageName);
        return -1;
    }

    //check image whether encrypted for kernel 3.14
    pEncryptedBootloaderInfoBufAddr = pImageAddr + newbootloaderEncryptInfoOffset;
    if (!memcmp(newunencryptedBootloaderInfoBuf, pEncryptedBootloaderInfoBufAddr,
        ARRAY_SIZE(newunencryptedBootloaderInfoBuf))) {
        step0 = 0;
    }

    pstart = pImageAddr + newbootloaderEncryptInfoOffset1;
    for (index=0;index<16;index++) {
        if (pstart[index] != 0) {
            step1 = 1;
            break;
        }
    }

    if ((step0 == 1) && (step1 == 1)) {
        return 1;  // encrypted
    }

    return 0;//unencrypted
}

/*  return value:
  *  <0: failed
  *  =0: decrypt success
  */
int DtbImgDecrypt(
        const char *imageName,
        const unsigned char *imageBuffer,
        const int imageSize,
        const char *flag,
        unsigned char *encryptedbuf)
{
    ssize_t result = -1;
    ssize_t readlen = -1;
    int fd = -1, ret = -1;

     if ((imageBuffer == NULL) || (imageName == NULL)) {
        printf("imageBuffer is null!\n");
        return -1;
     }

    if (access(DECRYPT_DTB, F_OK) ||access(DEFEND_KEY, F_OK)) {
        printf("doesn't support dtb secure check\n");
        return -1;   // kernel doesn't support
    }

    ret = SetDtbEncryptFlag(flag);
    if (ret < 0) {
        printf("set dtb encrypt flag by %s failed!, try ioctl\n", DECRYPT_DTB);
        ret = SetDtbEncryptFlagByIoctl(flag);
        if (ret < 0) {
            printf("set dtb encrypt flag by ioctl failed!\n");
            return -1;
        }
    }

    fd = open(DEFEND_KEY, O_RDWR);
    if (fd < 0) {
        printf("open %s failed (%s)\n",DEFEND_KEY, strerror(errno));
        return -1;
    }

    result = write(fd, imageBuffer, imageSize);// check rsa
    printf("write %s datas to %s. [imgsize:%d, result:%d, %s]\n",
        imageName, DEFEND_KEY, imageSize, result,
        (result == 1) ? "match" :
        (result == -2) ? "not match" : "failed or not support");

    if (!strcmp(flag, "1")) {
        printf("dtb.img need to encrypted!\n");
        readlen = read(fd, encryptedbuf, imageSize);
        if (readlen  < 0) {
            printf("read %s error!\n", DEFEND_KEY);
            close(fd);
            return -1;
        }
    }

    close(fd);
    fd = -1;

    return 0;
}

/**
  *  --- check zip archive image whether encrypt or not
  *   image is bootloader.img/boot.img/recovery.img
  *
  *  @imageName:   image name
  *  @imageBuffer: image data address
  *  @imageSize:   image data size
  *
  *  return value:
  *  <0: failed
  *  =0: unencrypted
  *  >0: encrypted
  */
static int IsZipArchiveImageEncrypted(
        const char *imageName,
        const unsigned char *imageBuffer,
        const int imageSize)
{
    int ret = -1;
    const unsigned char *pImageAddr = imageBuffer;

    if (strcmp(imageName, BOOT_IMG) &&
        strcmp(imageName, RECOVERY_IMG) &&
        strcmp(imageName, BOOTLOADER_IMG)) {
        printf("can't support %s at present\n",
            imageName);
        return -1;
    }

    if (imageBuffer == NULL) {
        printf("havn't malloc space for %s\n",
            imageName);
        return -1;
    }

    if (imageSize <= 0) {
        printf("%s size is %d\n",
            imageName, imageSize);
        return -1;
    }

    if (!strcmp(imageName, BOOTLOADER_IMG)) {
        return IsBootloaderImageEncrypted(imageName, imageBuffer);
    }

    //check image whether encrypted for kernel 3.14
    AmlSecureBootImgHeader encryptSecureBootImgHeader =
        (const AmlSecureBootImgHeader)pImageAddr;
    p_AmlEncryptBootImgInfo encryptBootImgHeader =
        &encryptSecureBootImgHeader->encrypteImgInfo;

    secureDbg("magic:%s, version:0x%04x\n",
        encryptBootImgHeader->magic, encryptBootImgHeader->version);

    ret = memcmp(encryptBootImgHeader->magic, SECUREBOOT_MAGIC,
        strlen(SECUREBOOT_MAGIC));

    //for android P, 2048 offset
    if (ret) {
        encryptSecureBootImgHeader = (AmlSecureBootImgHeader)(pImageAddr+1024);
        encryptBootImgHeader = &encryptSecureBootImgHeader->encrypteImgInfo;
        ret = memcmp(encryptBootImgHeader->magic, SECUREBOOT_MAGIC, strlen(SECUREBOOT_MAGIC));
    }

    if (!ret && encryptBootImgHeader->version != 0x0) {
        return 1;   // encrypted
    }

    //for v3 secureboot image encrypted
    if (ret) {
        const unsigned char V3SecureBootInfoBuf[] = { 0x40, 0x41, 0x4D, 0x4C};
        if (!memcmp(V3SecureBootInfoBuf, pImageAddr, ARRAY_SIZE(V3SecureBootInfoBuf))) {
            return 1;   // encrypted
        }

        //find offset 2048 byte
        if (!memcmp(V3SecureBootInfoBuf, pImageAddr+2048, ARRAY_SIZE(V3SecureBootInfoBuf))) {
            return 1;   // encrypted
        }
    }

    return 0;       // unencrypted
 }

int IsPlatformEncryptedByIoctl(void) {
    int ret = -1;
    unsigned int operation = 0;
    char prop[PROPERTY_VALUE_MAX+1] = {0};

    property_get("ro.boot.aml_secure_check", prop, "true");
    printf("ro.boot.aml_secure_check=%s\n", prop);
    if (strcmp(prop, "false") == 0 ) {
        printf("ignore aml secure check\n");
        return SECURE_SKIP;
    }

    printf("ignore aml secure check in R \n");
    return 0;

    if (access(DEFEND_KEY, F_OK)) {
        printf("kernel doesn't support secure check\n");
        return SECURE_SKIP;   // kernel doesn't support
    }

    int fd = open(DEFEND_KEY, O_RDWR);
    if (fd < 0) {
        printf("open %s failed!\n", DEFEND_KEY);
        return ret;
    }

    ret = ioctl(fd, CMD_SECURE_CHECK, &operation);
    close(fd);

    if (ret == 0) {
        printf("check platform: unencrypted\n");
    } else if (ret > 0) {
        printf("check platform: encrypted\n");
    } else {
        printf("check platform: failed\n");
    }

    return ret;
}

/**
  *  --- check platform whether encrypt or not
  *
  *  return value:
  *  <0: failed
  *  =0: unencrypted
  *  >0: encrypted
  */
int IsPlatformEncrypted(void)
{
    int fd = -1, ret = -1;
    ssize_t count = 0;
    char rBuf[128] = {0};
    char platform[PROPERTY_VALUE_MAX+1] = {0};

    printf("ignore aml secure check in R \n");
    return 0;

    //Q is ro.boot.avb_version
    std::string avb_version = android::base::GetProperty("ro.boot.vbmeta.avb_version", "default");
    if (!strcmp("default", avb_version.c_str())) {
        //R is ro.boot.avb_version
        avb_version = android::base::GetProperty("ro.boot.avb_version", "default");
    }
    printf("avb_version: %s\n", avb_version.c_str());
    if (strcmp("default", avb_version.c_str())) {
        printf("skip check in avb mode\n");
        return SECURE_SKIP;
    }

    if (access(SECURE_CHECK, F_OK) || access(DEFEND_KEY, F_OK)) {
        printf("kernel doesn't support secure check\n");
        return ret;   // kernel doesn't support
    }

    fd = open(SECURE_CHECK, O_RDONLY);
    if (fd < 0) {
        printf("open %s failed (%s)\n", SECURE_CHECK, strerror(errno));
        return ret;
    }

    property_get("ro.build.product", platform, "unknow");
    count = read(fd, rBuf, sizeof(rBuf) - 1);
    if (count <= 0) {
        printf("read %s failed (count:%d)\n", SECURE_CHECK, count);
        close(fd);
        return ret;
    }
    rBuf[count] = '\0';

    if (!strcmp(rBuf, s_pStatus[UNENCRYPT])) {
        printf("check platform(%s): unencrypted\n", platform);
        ret = 0;
    } else if (!strcmp(rBuf, s_pStatus[ENCRYPT])) {
        printf("check platform(%s): encrypted\n", platform);
        ret = 1;
    } else if (!strcmp(rBuf, s_pStatus[FAIL])) {
        printf("check platform(%s): failed\n", platform);
    } else {
        printf("check platform(%s): %s\n", platform, rBuf);
    }

    if (fd >= 0) {
        close(fd);
        fd = -1;
    }

    return ret;
}

/**
  *  --- get upgrade package image data
  *
  *  @zipArchive: zip archive object
  *  @imageName:  upgrade package image's name
  *  @imageSize:  upgrade package image's size
  *
  *  return value:
  *  <0: failed
  *  =0: can't find image
  *  >0: get image data successful
  */
static unsigned char *s_pImageBuffer = NULL;
static int GetZipArchiveImage(
        const ZipArchiveHandle za,
        const char *imageName,
        int *imageSize)
{
    std::string_view zip_path(imageName);
    ZipEntry entry;
    if (FindEntry(za, zip_path, &entry) != 0) {
      printf("no %s in package!\n", imageName);
      return 0;
    }

    *imageSize = entry.uncompressed_length;
    if (*imageSize <= 0) {
        printf("can't get package entry uncomp len(%d) (%s)\n",
            *imageSize, strerror(errno));
        return -1;
    }

    if (s_pImageBuffer != NULL) {
        free(s_pImageBuffer);
        s_pImageBuffer = NULL;
    }

    s_pImageBuffer = (unsigned char *)calloc(*imageSize, sizeof(unsigned char));
    if (!s_pImageBuffer) {
        printf("can't malloc %d size space (%s)\n",
            *imageSize, strerror(errno));
        return -1;
    }

    int32_t ret = ExtractToMemory(za, &entry, s_pImageBuffer, entry.uncompressed_length);
    if (ret != 0) {
        printf("can't extract package entry to image buffer\n");
        goto FREE_IMAGE_MEM;
    }

    return 1;


FREE_IMAGE_MEM:
    if (s_pImageBuffer != NULL) {
        free(s_pImageBuffer);
        s_pImageBuffer = NULL;
    }

    return -1;
}

int ImageWrite(char *dst, unsigned char *buffer, int size, int offset){

    int fd = open(dst, O_RDWR | O_CREAT, 00777);
    if (fd < 0) {
        printf("open %s failed\n", dst);
        return -1;
    }

    lseek(fd, offset, SEEK_SET);

    int len = write(fd, buffer, size);
    if (len != size) {
        printf("write %d failed\n", size);
        close(fd);
        return -1;
    }

    fsync(fd);
    close(fd);
    return 0;
}

int RecoveryPreUpdate(const ZipArchiveHandle zipArchive){
    int ret = 0;
    int imageSize = 0;
    char bootdevice[128]= {0};
    char expectindex[32] = {0};

    char *CurrentIndex = get_bootloader_env("forUpgrade_bootloaderIndex");
    if ((!CurrentIndex) || (!strcmp(CurrentIndex, "0"))) {
        sprintf(bootdevice, "%s", "/dev/block/mmcblk0boot0");
        sprintf(expectindex, "%s", "1");
    } else if (!strcmp(CurrentIndex, "1")) {
        sprintf(bootdevice, "%s", "/dev/block/mmcblk0boot1");
        sprintf(expectindex, "%s", "2");
    } else {
        sprintf(bootdevice, "%s", "/dev/block/bootloader");
        sprintf(expectindex, "%s", "0");
    }

    //update expect bootloader block device
    ret = GetZipArchiveImage(zipArchive, BOOTLOADER_IMG, &imageSize);
    if (ret > 0) {
        printf("Find bootloader.img, start to update %s\n", bootdevice);
        ImageWrite(bootdevice, s_pImageBuffer, imageSize, 512);
    }

    //update dt.img to cache
    ret = GetZipArchiveImage(zipArchive, DTB_IMG, &imageSize);
    if (ret > 0) {
        printf("Find dt.img, start to update /cache/recovery/dtb.img\n");
        ImageWrite("/cache/recovery/dtb.img",  s_pImageBuffer, imageSize, 0);
    }

    //update recovery.img to cache
    ret = GetZipArchiveImage(zipArchive, RECOVERY_IMG, &imageSize);
    if (ret > 0) {
        printf("Find recovery.img, start to update /cache/recovery/recovery.img\n");
        ImageWrite("/cache/recovery/recovery.img",  s_pImageBuffer, imageSize, 0);
    }

    //update vendor_boot.img to cache
    ret = GetZipArchiveImage(zipArchive, VENDOR_BOOT_IMG, &imageSize);
    if (ret > 0) {
        printf("Find vendor_boot.img, start to update /cache/recovery/vendor_boot.img\n");
        ImageWrite("/cache/recovery/vendor_boot.img",  s_pImageBuffer, imageSize, 0);
    }

    set_bootloader_env("reboot_status", "reboot_next");
    set_bootloader_env("expect_index", expectindex);
    set_bootloader_env("upgrade_step", "3");
    sleep(2);

    if (android::base::GetBoolProperty("ro.boot.quiescent", false))
        property_set(ANDROID_RB_PROPERTY, "reboot,quiescent");
    else
        property_set(ANDROID_RB_PROPERTY, "reboot");

    sleep(5);
    return 0;
}

/**
  *  --- check platform and upgrade package whether
  *  encrypted,if all encrypted,rsa whether all the same
  *
  *  @ziparchive: Archive of  Zip Package
  *
  *  return value:
  *  =-1: failed; not allow upgrade
  *  = 0: check not match; not allow upgrade
  *  = 1: check match; allow upgrade
  *  = 2: kernel not support secure check; allow upgrade
  */
int RecoverySecureCheck(const ZipArchiveHandle zipArchive)
{
    int i = 0;
    int num = 0;
    int flag_old_new = 0;
    std::string avb_version;
    int imageSize = 0;
    int ret = SECURE_MATCH;
    int platformEncryptStatus = 0;
    const char *pImageName_normal[] = {
            DTB_IMG,
            BOOT_IMG,
            RECOVERY_IMG };
    char **pImageName;

    //if not android 9, need upgrade for two step
    std::string android_version = android::base::GetProperty("ro.build.version.sdk", "");
    if (strcmp(ANDROID_VERSION_R, android_version.c_str())) {
        printf("now upgrade from android %s to R\n", android_version.c_str());
        flag_old_new = 1;
    }

    platformEncryptStatus = IsPlatformEncrypted();
    if (platformEncryptStatus == SECURE_SKIP) {
        return SECURE_SKIP;
    } else if (platformEncryptStatus < 0) {
        printf("get platform encrypted by /sys/class/defendkey/secure_check failed, try ioctl!\n");
        platformEncryptStatus = IsPlatformEncryptedByIoctl();
    }

    /*defendkey maybe not exist or some failed, no need do securecheck*/
    if (platformEncryptStatus < 0 || platformEncryptStatus == SECURE_SKIP) {
        if (flag_old_new == 1) {
            return SECURE_OLD_NEW;//old to R
        } else {
            return SECURE_SKIP;//Skip check
        }
    }

    //if encrypted board, bootloader.img must be encrypted
    ret = GetZipArchiveImage(zipArchive, BOOTLOADER_IMG, &imageSize);
    if (ret < 0) {
        printf("get %s datas failed\n", BOOTLOADER_IMG);
        goto ERR1;
    } else if (ret == 0) {
        printf("check %s: not find,skiping...\n", BOOTLOADER_IMG);
    } else {
        ret = IsPlatformMatchWithImage(BOOTLOADER_IMG, s_pImageBuffer, imageSize);
        if (ret == SECURE_FAIL) {
            printf("%s doesn't match platform\n", BOOTLOADER_IMG);
            goto ERR1;
        }
        printf("%s match with platform\n", BOOTLOADER_IMG);
    }

    if (flag_old_new == 1) {
        return SECURE_OLD_NEW;//Q to R
    }

    //Q is ro.boot.avb_version
    avb_version = android::base::GetProperty("ro.boot.vbmeta.avb_version", "default");
    if (!strcmp("default", avb_version.c_str())) {
        //R is ro.boot.avb_version
        avb_version = android::base::GetProperty("ro.boot.avb_version", "default");
    }
    printf("avb_version: %s\n", avb_version.c_str());
    //if non-avb mode, need check dt.img boot.img and recovery.img
    if (!strcmp("default", avb_version.c_str())) {
        printf("null avb\n");
        pImageName = (char **)pImageName_normal;
        num = ARRAY_SIZE(pImageName_normal);
        for (i = 0; i < num; i++) {
            printf("pImageName[%d]: %s\n", i, pImageName[i]);
            ret = GetZipArchiveImage(zipArchive, pImageName[i], &imageSize);
            if (ret < 0) {
                printf("get %s datas failed\n", pImageName[i]);
                goto ERR1;
            } else if (ret == 0) {
                printf("check %s: not find,skiping...\n", pImageName[i]);
                continue;
            } else {
                ret = IsPlatformMatchWithImage(pImageName[i], s_pImageBuffer, imageSize);
                if (ret != SECURE_MATCH) {
                    printf("%s doesn't match platform\n", pImageName[i]);
                    goto ERR1;
                }
                printf("%s match with platform\n", pImageName[i]);
            }
        }
    }

    ret = SECURE_MATCH;
ERR1:
    if (s_pImageBuffer != NULL) {
        free(s_pImageBuffer);
        s_pImageBuffer = NULL;
    }

    return ret;
}
