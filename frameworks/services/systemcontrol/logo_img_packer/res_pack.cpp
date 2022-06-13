/*
 * Copyright (c) 2015 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 */
/*
 * Command for pack imgs.
 *
 * Copyright (C) 2012 Amlogic.
 * Elvis Yu <elvis.yu@amlogic.com>
 */
#include "res_pack_i.h"
#include "res_pack.h"
#define LOG_TAG "SystemControl"
#include "common.h"
#include <dirent.h>
#include <unistd.h>

#define COMPILE_TYPE_CHK(expr, t)       typedef char t[(expr) ? 1 : -1]

COMPILE_TYPE_CHK(AML_RES_IMG_HEAD_SZ == sizeof(AmlResImgHeadLogo_t), a);//assert the image header size 64
COMPILE_TYPE_CHK(AML_RES_ITEM_HEAD_SZ == sizeof(AmlResItemHeadLogo_t), b);//assert the item head size 64

#define IMG_HEAD_SZ     sizeof(AmlResImgHeadLogo_t)
#define ITEM_HEAD_SZ    sizeof(AmlResItemHeadLogo_t)
//#define ITEM_READ_BUF_SZ    (1U<<20)//1M
#define ITEM_READ_BUF_SZ    (64U<<10)//64K to test

#define LOGO_TEST_DIR "/mnt/vendor/param/logo/"
#define PIC_PRELOAD_SZ  (8U<<10) //Total read 4k at first to read the image header
#define BLOCK_LOGO "/dev/block/logo"
#define LOGO_TEST_SRC_FILE "/vendor/test_logo.bmp"
#define LOGO_TEST_DST_FILE "/mnt/vendor/param/logo/test_logo"


static size_t get_filesize(const char *fpath)
{
	struct stat buf;
	if (stat(fpath, &buf) < 0)
	{
		SYS_LOGE("Can't stat %s : %s\n", fpath, strerror(errno));
		return -1;
	}
	return buf.st_size;
}

#if 0
void dump_mem(char * buffer, int count)
{
    int i;
    if (NULL == buffer || count == 0)
    {
        SYS_LOGE("%s() %d: %p, %d", __func__, __LINE__, buffer, count);
        return;
    }
    for (i=0; i<count ; i++)
    {
        if (i % 16 == 0)
            SYS_LOGE("\n");
        SYS_LOGE("%02x ", buffer[i]);
    }
    SYS_LOGE("\n");
}
#endif

static  const char* get_filename(const char *fpath)
{
	int i;
	const char *filename = fpath;
	for (i = strlen(fpath)-1; i >= 0; i--)
	{
		if ('/' == fpath[i] || '\\' == fpath[i])
		{
			i++;
			filename = fpath + i;
			break;
		}
	}

	return filename;
}

// added to filter out .bmp files
static char* get_last_itemname(const char* itemName)
{
    char buf[256]={0};
    char* last_itemname = (char*)malloc(256);

    strcpy(last_itemname, itemName);
    const char* bmp_tag = ".bmp";
    const char* suffix_of_file;
    suffix_of_file = strstr(itemName, bmp_tag);
    int len = 0;

    if (suffix_of_file && strcmp(suffix_of_file, bmp_tag) == 0)
    {
        len = suffix_of_file-itemName;
        strncpy(buf, itemName, len);
        buf[len] = '\0';
        strcpy(last_itemname, buf);
    }
    return last_itemname;
}

int get_file_path_from_argv(const char** const argv, __hdle *hDir, char* fileName)
{
    long index = (long)hDir;
    const char* fileSrc = argv[index];

    strcpy(fileName, fileSrc);
    return 0;
}

int get_dir_filenums(const char *dir_path)
{
	int count = 0;
	DIR *d;
	struct dirent *de;
	d = opendir(dir_path);
    if (d == 0) {
        SYS_LOGE("opendir failed, %s\n", strerror(errno));
        return -1;
    }
	while ((de = readdir(d)) != 0) {
	        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;
	        if (de->d_name[0] == '.') continue;
		count++;
	}
    closedir(d);
	return count;
}

int img_res_check_log_header(const AmlResImgHeadLogo_t* pResImgHead)
{
    int rc = 0;

    rc = memcmp(pResImgHead->magic, AML_RES_IMG_V1_MAGIC, AML_RES_IMG_V1_MAGIC_LEN);
    if (rc) {
        SYS_LOGE("Magic error for res, pResImgHead->magic %s != %s\n", pResImgHead->magic, AML_RES_IMG_V1_MAGIC);
        return 1;
    }
    if (AML_RES_IMG_VERSION_V2 != pResImgHead->version) {
        SYS_LOGE("res version 0x%x != 0x%x\n", pResImgHead->version, AML_RES_IMG_VERSION_V2);
        return 2;
    }

    return 0;
}

int res_img_unpack(const char* const path_src, const char* const unPackDirPath, int needCheckCrc)
{
        char* itemReadBuf = NULL;
        int fdResImg;
        int ret = 0;
        AmlResImgHeadLogo_t* pImgHead = NULL;
        int imgVersion = -1;
        ssize_t ImgFileSz = 0;
        unsigned itemIndex = 0;

        fdResImg = open(path_src, O_RDONLY);
        if (fdResImg < 0) {
                SYS_LOGE("Fail to open res image at path %s\n", path_src);
                return __LINE__;
        }

        itemReadBuf = new char[ITEM_READ_BUF_SZ * 2];
        if (!itemReadBuf) {
                SYS_LOGE("Fail to new buffer at size 0x%x\n", ITEM_READ_BUF_SZ * 2);
                close(fdResImg);
                return __LINE__;
        }
        pImgHead = (AmlResImgHeadLogo_t*)(itemReadBuf + ITEM_READ_BUF_SZ);

        ImgFileSz = read(fdResImg, pImgHead, ITEM_READ_BUF_SZ);
        if (ImgFileSz <= IMG_HEAD_SZ) {
                SYS_LOGE("file size 0x%zx too small\n", ImgFileSz);
                if (itemReadBuf) delete[] itemReadBuf, itemReadBuf = NULL;
                close(fdResImg);
                return __LINE__;
        }

        if (img_res_check_log_header(pImgHead)) {
            SYS_LOGE("Logo header err.\n");
            if (itemReadBuf) delete[] itemReadBuf, itemReadBuf = NULL;
            close(fdResImg);
            return __LINE__;
        }


        //for each loop:
        //    1, read item body and save as file;
        //    2, get next item head
        const unsigned   itemAlignSz  = AML_RES_IMG_ITEM_ALIGN_SZ;
        const unsigned   itemAlignMod = itemAlignSz - 1;
        const unsigned   itemSzAlignMask = ~itemAlignMod;
        unsigned totalReadItemNum = 0;
        const AmlResItemHeadLogo_t* pItemHead = NULL;

        SYS_LOGI("imgItemNum: %d\n", pImgHead->imgItemNum);

        pItemHead = (AmlResItemHeadLogo_t*)(pImgHead + 1);
        for (itemIndex = 0; itemIndex < pImgHead->imgItemNum; ++itemIndex, ++pItemHead) {
            char itemFullPath[MAX_PATH*2];
            FILE* fp_item = NULL;
            if (IH_MAGIC != pItemHead->magic) {
                SYS_LOGE("item magic 0x%x != 0x%x\n", pItemHead->magic, IH_MAGIC);
                ret = __LINE__;
                goto _exit;
            }

            sprintf(itemFullPath, "%s/%s", unPackDirPath, pItemHead->name);
            SYS_LOGI("item %s\n", itemFullPath);

            fp_item = fopen(itemFullPath, "wb");
            if (!fp_item) {
                SYS_LOGE("Fail to create file %s, strerror(%s)\n", itemFullPath, strerror(errno));
                ret = __LINE__; goto _exit;
            }

            const unsigned thisItemBodySz = pItemHead->size;
            const unsigned thisItemBodyOccupySz =  (thisItemBodySz & itemSzAlignMask) + itemAlignSz;
            const unsigned stuffLen       = thisItemBodyOccupySz - thisItemBodySz;
            unsigned itemTotalReadLen;
            unsigned long rdOff;

            //SYS_LOGI("thisItemBodySz = 0x%x, thisItemBodyOccupySz = 0x%x, stuffLen = %d\n", thisItemBodySz, thisItemBodyOccupySz, stuffLen);

            for (itemTotalReadLen = 0; itemTotalReadLen < thisItemBodyOccupySz; )
            {
                const unsigned leftLen = thisItemBodyOccupySz - itemTotalReadLen;
                const unsigned thisReadSz = min(leftLen, ITEM_READ_BUF_SZ);
                ssize_t actualReadSz = 0;
                rdOff = pItemHead->start + itemTotalReadLen;

                //SYS_LOGI("leftLen = 0x%x, thisReadSz = 0x%x, rdOff = 0x%x\n", leftLen, thisReadSz, rdOff);

                lseek(fdResImg, rdOff, SEEK_SET);
                actualReadSz = read(fdResImg, itemReadBuf, thisReadSz);
                if (thisReadSz != actualReadSz) {
                    SYS_LOGE("thisReadSz 0x%x != actualReadSz 0x%zx\n", thisReadSz, actualReadSz);
                    ret = __LINE__;goto _exit;
                }

                itemTotalReadLen += thisReadSz;
                const unsigned thisWriteSz = itemTotalReadLen < thisItemBodySz ? thisReadSz : (thisReadSz - stuffLen);
                actualReadSz = fwrite(itemReadBuf, 1, thisWriteSz, fp_item);
                if (thisWriteSz != actualReadSz) {
                    SYS_LOGE("want write 0x%x, but 0x%zx\n", thisWriteSz, actualReadSz);
                    ret = __LINE__;goto _exit;
                }

            }
            fclose(fp_item), fp_item = NULL;
        }

_exit:
        if (itemReadBuf) delete[] itemReadBuf, itemReadBuf = NULL;
        close(fdResImg);
        return ret;
}

/*
 * 1,
 */
int img_pack(const char* const path_src, const char* const packedImg,
        const int totalFileNum)
{
        FILE *fd_src = NULL;
        int fd_dest = -1;
        unsigned int pos = 0;
        char file_path[MAX_PATH];
        const char *filename = NULL;
        unsigned imageSz = 0;
        const unsigned BufSz = ITEM_READ_BUF_SZ;
        char* itemBuf = NULL;
        unsigned thisWriteLen = 0;
        unsigned actualWriteLen = 0;
        int ret = 0;
        DIR *dir;
        struct dirent *ptr;
        unsigned itemIndex = 0;
        int offset = 0;
        const unsigned   itemAlignSz  = AML_RES_IMG_ITEM_ALIGN_SZ;
        const unsigned   itemAlignMod = itemAlignSz - 1;
        const unsigned   itemSzAlignMask = ~itemAlignMod;
        const unsigned   totalItemNum = totalFileNum ? totalFileNum : get_dir_filenums(path_src);
        const unsigned   HeadLen    = IMG_HEAD_SZ + ITEM_HEAD_SZ * totalItemNum;

        if (HeadLen > BufSz) {
                SYS_LOGE("head size 0x%x > max(0x%x)\n", HeadLen, BufSz); return __LINE__;
        }

        fd_dest = open(packedImg, O_RDWR);
        if (fd_dest < 0) {
            SYS_LOGE("open %s failed: %s\n", packedImg, strerror(errno));
            return -1;
        }

        itemBuf = new char[BufSz * 2];
        if (!itemBuf) {
                SYS_LOGE("Exception: fail to alloc buuffer\n");
                return __LINE__;
        }
        memset(itemBuf, 0, BufSz * 2);
        AmlResImgHeadLogo_t* const aAmlResImgHead = (AmlResImgHeadLogo_t*)(itemBuf + BufSz);


        if (write(fd_dest, aAmlResImgHead, HeadLen) != HeadLen) {
            SYS_LOGE("fail to write head, want 0x%x, but 0x%x\n", HeadLen, actualWriteLen);
            delete[] itemBuf;
            return __LINE__;
        }

        imageSz += HeadLen; //Increase imageSz after pack each item
        AmlResItemHeadLogo_t*       pItemHeadInfo           = (AmlResItemHeadLogo_t*)(aAmlResImgHead + 1);
        AmlResItemHeadLogo_t* const pFirstItemHeadInfo      = pItemHeadInfo;
        size_t itemOffsetLen = 0;

        SYS_LOGI("item num %d\n", totalItemNum);
        //for each loop: first create item header and pack it, second pack the item data
        //Fill the item head, 1) magic, 2)data offset, 3)next head start offset
        dir = opendir(path_src);
        while ((ptr=readdir(dir)) != NULL)
        {
                char filePath[MAX_PATH * 2];
                if (strcmp(ptr->d_name,".") == 0 || strcmp(ptr->d_name,"..") == 0)    ///current dir OR parrent dir
                    continue;
                else if (ptr->d_type == 8)
                {
                    SYS_LOGI("d_name:%s/%s\n", path_src, ptr->d_name);
                    sprintf(filePath, "%s/%s", path_src, ptr->d_name);
                }
                const size_t itemSz = get_filesize(filePath);
                const char*  itemName = get_filename(filePath);
                const unsigned itemBodyOccupySz =  (itemSz & itemSzAlignMask) + itemAlignSz;
                const unsigned itemStuffSz      = itemBodyOccupySz - itemSz;
                size_t itemWriteLen;

                if (IH_NMLEN - 1 < strlen(itemName)) {
                        SYS_LOGE("Item name %s len %d > max(%d)\n", itemName, (int)strlen(itemName), IH_NMLEN - 1);
                        ret = __LINE__; goto _exit;
                }

                SYS_LOGI("Item name: %s, item size: %zu\n", itemName, itemSz);

                pItemHeadInfo->magic = IH_MAGIC;
                pItemHeadInfo->size = itemSz;
                //imageSz += ITEM_HEAD_SZ;//not needed yet as all item_head moves to image header
                pItemHeadInfo->start = imageSz;
                pItemHeadInfo->next  =  (char*)(pItemHeadInfo + 1) - (char*)aAmlResImgHead;
                pItemHeadInfo->index = itemIndex;
                imageSz   += itemBodyOccupySz;
                pItemHeadInfo->nums   = totalItemNum;

                char* last_itemname = get_last_itemname(itemName);

                memcpy(pItemHeadInfo->name, last_itemname, strlen(last_itemname));
                //SYS_LOGI("last_itemname [%s]\n", last_itemname);
                ++pItemHeadInfo;//prepare for next item
                if (last_itemname != itemName) {
                    free(last_itemname);
                }
				SYS_LOGI("filePath: %s\n", filePath);
                fd_src = fopen(filePath, "rb");
                if (!fd_src) {
                        SYS_LOGE("Fail to open file [%s], strerror[%s]\n", filePath, strerror(errno));
                        ret = __LINE__; goto _exit;
                }
                for (itemWriteLen = 0; itemWriteLen < itemSz; itemWriteLen += thisWriteLen, itemOffsetLen += thisWriteLen)
                {
                        size_t leftLen = itemSz - itemWriteLen;
                        thisWriteLen = leftLen > BufSz ? BufSz : leftLen;
                        offset = HeadLen + itemOffsetLen;
                        actualWriteLen = fread(itemBuf, 1, thisWriteLen, fd_src);
                        if (actualWriteLen != thisWriteLen) {
                                SYS_LOGE("Want to read 0x%x but actual 0x%x, at itemWriteLen 0x%x, leftLen 0x%x\n",
                                                thisWriteLen, actualWriteLen, (unsigned)itemWriteLen, (unsigned)leftLen);
                                ret = __LINE__; goto _exit;
                        }

                        lseek(fd_dest, offset, SEEK_SET);
                        if (write(fd_dest, itemBuf, thisWriteLen) != thisWriteLen) {
                                SYS_LOGE("Want to write 0x%x but actual 0x%x\n", thisWriteLen, actualWriteLen);
                                ret = __LINE__; goto _exit;
                        }
                }
                fclose(fd_src), fd_src = NULL;

                offset = HeadLen + itemOffsetLen;
                memset(itemBuf, 0, itemStuffSz);
                thisWriteLen = itemStuffSz;
                lseek(fd_dest, offset, SEEK_SET);
                if (write(fd_dest, itemBuf, thisWriteLen) != thisWriteLen) {
                    SYS_LOGE("Want to write 0x%x but actual 0x%x\n", thisWriteLen, actualWriteLen);
                    ret = __LINE__; goto _exit;
                }
                itemOffsetLen += thisWriteLen;
				++itemIndex;
        }
        (--pItemHeadInfo)->next = 0;

        //Create the header
        aAmlResImgHead->version = AML_RES_IMG_VERSION_V2;
        memcpy(&aAmlResImgHead->magic[0], AML_RES_IMG_V1_MAGIC, AML_RES_IMG_V1_MAGIC_LEN);
        aAmlResImgHead->imgSz       = imageSz;
        aAmlResImgHead->imgItemNum  = totalItemNum;
        aAmlResImgHead->alignSz     = itemAlignSz;
        aAmlResImgHead->crc         = 0;

        lseek(fd_dest, 0, SEEK_SET);
        if (write(fd_dest, aAmlResImgHead, HeadLen) != HeadLen) {
            SYS_LOGE("Want to write 0x%x but actual 0x%x\n", thisWriteLen, actualWriteLen);
            ret = __LINE__; goto _exit;
        }

        aAmlResImgHead->crc = calc_logoimg_crc(fd_dest, 4, 0);//Gen crc32

        SYS_LOGI("aAmlResImgHead->crc 0x%x \n", aAmlResImgHead->crc);

        lseek(fd_dest, 0, SEEK_SET);
        if (write(fd_dest, aAmlResImgHead, HeadLen) != HeadLen) {
            SYS_LOGE("Want to write 0x%x but actual 0x%x\n", thisWriteLen, actualWriteLen);
            ret = __LINE__; goto _exit;
        }
_exit:
        if (itemBuf) delete[] itemBuf, itemBuf = NULL;
        if (fd_src) fclose(fd_src), fd_src = NULL;
        closedir(dir);
        return ret;
}

int copyLogoFiles(const char *srcPath, const char *dstPath)
{
    int mFd;
    int dstFd;

    if ((mFd = open(srcPath, O_RDONLY)) == -1) {
        SYS_LOGE("Open %s Error:%s/n", srcPath, strerror(errno));
        return -1;
    }

    if ((dstFd = open(dstPath, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR)) == -1) {
        SYS_LOGE("Open %s Error:%s/n", dstPath, strerror(errno));
    }

    int bytes_read, bytes_write;
    char buffer[ITEM_READ_BUF_SZ];
    char *ptr;
    int ret = 0;
    while ((bytes_read = read(mFd, buffer, ITEM_READ_BUF_SZ))) {
        /* read error */
        if ((bytes_read == -1) && (errno != EINTR)) {
            ret = -1;
            break;
        } else if (bytes_read > 0) {
            ptr = buffer;
            while ((bytes_write = write(dstFd, ptr, bytes_read))) {
                /* failed to write halfway */
                if ((bytes_write == -1) && (errno != EINTR)) {
                    ret = -1;
                    break;
                }
                /* write finish */
                else if (bytes_write == bytes_read) {
                    ret = 0;
                    break;
                }
                /* write continue */
                else if (bytes_write > 0) {
                    ptr += bytes_write;
                    bytes_read -= bytes_write;
                }
            }
            /* failed to write at first */
            if (bytes_write == -1) {
                ret = -1;
                break;
            }
        }
    }
    fsync(dstFd);
    close(dstFd);
    return ret;
}

int res_img_pack_bmp(const char* path)
{
    int ret = 0;
    ret = res_img_unpack(BLOCK_LOGO, LOGO_TEST_DIR, 1);
    if (ret != 0) {
        SYS_LOGE("res_img_unpack error/n");
        return ret;
    }
    ret = copyLogoFiles(path, LOGO_TEST_DST_FILE);
    if (ret != 0) {
        SYS_LOGE("copy logo files error/n");
        return ret;
    }
    ret = img_pack(LOGO_TEST_DIR, BLOCK_LOGO, 0);
    if (ret != 0) {
        SYS_LOGE("img_pack error/n");
        return ret;
    }
    return ret;
}



