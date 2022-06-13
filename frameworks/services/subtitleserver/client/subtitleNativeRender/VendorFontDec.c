#include "VendorFontDec.h"
#include <syslog.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <openssl/evp.h>
#include <openssl/x509.h>

//const char *ttf_dir = "/system/etc/vendorfont/";
void copy_file(const char*src_path, const char*des_path) {
    int fd,fd2;
    unsigned char buff[1024];
    int len = 0;
    fd = open(src_path, O_RDONLY);
    fd2 = open(des_path, O_RDWR|O_CREAT, 0666);
    //syslog(LOG_ERR, "copy_file %s %d %d",des_path,fd2,fd);
    if (fd2 < 0) {
        syslog(LOG_ERR, "%d", errno);
    }
    chmod(des_path,0644);
    while ((len = read(fd, buff, 1024)) > 0) {
        write(fd2, buff, len);
       // syslog(LOG_ERR, "copy_file %d ",len);
    }
    close(fd);
    close(fd2);
}

int vendorFontExtractTo(const char*ttf_dir, const char* path) {
    OpenSSL_add_all_algorithms();
    DIR *d;
    int fd;
    struct dirent *dp;
    //syslog(LOG_ERR, "vendor_font_release enter");
    if (!(d = opendir(ttf_dir))) {
        //syslog(LOG_ERR, "vendor_font_release enterxxxxx");
        return -1;
    }

    syslog(LOG_ERR, "%d", errno);
    while ((dp = readdir(d))) {
        if ((strcmp(dp->d_name, ".") == 0) || (strcmp(dp->d_name, "..") == 0)) {
            continue;
        }

        char name[255];
        char destname[255];
        memset(name, 0, 255);
        strcpy(name, ttf_dir);
        strcpy(destname, path);
        strcat(name, dp->d_name);
        strcat(destname, dp->d_name);
        //syslog(LOG_ERR, "copy_file %s %s",name,destname);
        //copy_file(name, destname);
        if (strcmp(dp->d_name, "fonts") == 0) {
            strcat(destname, ".xml");
        }
        decrypt_file(name, destname);
        if (strcmp(dp->d_name, "fonts") == 0) {
            chown(destname, 1000, 1000);
            //syslog(LOG_ERR,"fopen----");
            //chmod(destname,644);
        }
    }
    closedir(d);
    return 0;
}

int encry_file(const char*src_path,const char *des_path) {
    unsigned char key[EVP_MAX_KEY_LENGTH];
    unsigned char iv[EVP_MAX_KEY_LENGTH];
    EVP_CIPHER_CTX ctx;
    unsigned char out[1024];
    int outl;
    unsigned char in[1024];
    int inl;
    int rv;
    int i;
    FILE *fpIn;
    FILE *fpOut;

    fpIn = fopen(src_path, "rb");
    if (fpIn == NULL) {
        return -1;
    }

    fpOut = fopen(des_path, "wb");
    if (fpOut == NULL) {
        fclose(fpIn);
        return -1;
    }

    for (i=0; i<24; i++) key[i]=i;

    for (i=0; i<8; i++) iv[i]=i;

    EVP_CIPHER_CTX_init(&ctx);

    rv = EVP_EncryptInit_ex(&ctx, EVP_des_ede3_cbc(), NULL, key, iv);
    if (rv != 1) {
        syslog(LOG_ERR, "EVP_EncryptInit_ex error");
        return -1;
    }

    for (;;) {
        inl = fread(in, 1, 1024, fpIn);
        if(inl <= 0) break;

        rv = EVP_EncryptUpdate(&ctx, out, &outl, in, inl);
        if (rv != 1) {
            fclose(fpIn);
            fclose(fpOut);
            EVP_CIPHER_CTX_cleanup(&ctx);
            return -1;
        }

        fwrite(out, 1, outl, fpOut);
    }

    rv = EVP_EncryptFinal_ex(&ctx, out, &outl);
    if (rv != 1) {
        fclose(fpIn);
        fclose(fpOut);
        EVP_CIPHER_CTX_cleanup(&ctx);
        return -1;
    }

    fwrite(out, 1, outl, fpOut);
    fclose(fpIn);
    fclose(fpOut);
    EVP_CIPHER_CTX_cleanup(&ctx);
    syslog(LOG_ERR, "encry_file end");
    return 1;
}

int upzipBuffer2File(const char*inBuf, size_t size, const char*destPath) {
    unsigned char key[EVP_MAX_KEY_LENGTH];
    unsigned char iv[EVP_MAX_KEY_LENGTH];
    EVP_CIPHER_CTX ctx;
    unsigned char out[1024+EVP_MAX_KEY_LENGTH];
    int outl;
    int inl;
    int rv;
    int i;
    FILE *fpOut;

    OpenSSL_add_all_algorithms();
    fpOut = fopen(destPath,"wb");
    if (fpOut == NULL) {
        syslog(LOG_ERR, "fopen %s error fd:%d errno:%d %d %d", destPath, fpOut, errno, geteuid(), getegid());
        return -1;
    }

    for (i=0; i<24; i++) key[i]=i;

    for (i=0; i<8; i++) iv[i]=i;

    EVP_CIPHER_CTX_init(&ctx);

    rv = EVP_DecryptInit_ex(&ctx ,EVP_des_ede3_cbc(), NULL, key, iv);
    if (rv != 1) {
        EVP_CIPHER_CTX_cleanup(&ctx);
        return -1;
    }

    int remained = size;
    int readed = 0;
    while (remained > 0) {

        int compSize = remained > 1024? 1024: remained;
        remained -= compSize;
        rv = EVP_DecryptUpdate(&ctx, out, &outl, inBuf+readed, compSize);
        readed += compSize;
        if (rv != 1) {
            fclose(fpOut);
            EVP_CIPHER_CTX_cleanup(&ctx);
            return -1;
        }
        fwrite(out ,1, outl, fpOut);
    }

    rv = EVP_DecryptFinal_ex(&ctx, out, &outl);
 /*   if (rv != 1) {
        fclose(fpOut);
        EVP_CIPHER_CTX_cleanup(&ctx);
        return -1;
    }
*/
    fwrite(out, 1, outl, fpOut);
    fclose(fpOut);
    EVP_CIPHER_CTX_cleanup(&ctx);
    syslog(LOG_ERR,"dencode end rv %d\n", rv);
    return 1;

}

int decrypt_file(const char*src_path,const char *des_path){
    unsigned char key[EVP_MAX_KEY_LENGTH];
    unsigned char iv[EVP_MAX_KEY_LENGTH];
    EVP_CIPHER_CTX ctx;
    unsigned char out[1024+EVP_MAX_KEY_LENGTH];
    int outl;
    unsigned char in[1024];
    int inl;
    int rv;
    int i;
    FILE *fpIn;
    FILE *fpOut;

    //syslog(LOG_ERR,"fopen %s",des_path);
    //seteuid(0);
    //setegid(0);
    //syslog(LOG_ERR,"fopen %d %d",geteuid(),getegid());
    fpIn = fopen(src_path, "rb");
    if (fpIn == NULL) {
        return -1;
    }

    fpOut = fopen(des_path,"wb");
    if (fpOut == NULL) {
        syslog(LOG_ERR, "fopen %s error fd:%d errno:%d %d %d", des_path, fpOut, errno, geteuid(), getegid());
        fclose(fpIn);
        return -1;
    }

    for (i=0; i<24; i++) key[i]=i;

    for (i=0; i<8; i++) iv[i]=i;

    EVP_CIPHER_CTX_init(&ctx);

    rv = EVP_DecryptInit_ex(&ctx ,EVP_des_ede3_cbc(), NULL, key, iv);
    if (rv != 1) {
        EVP_CIPHER_CTX_cleanup(&ctx);
        return -1;
    }

    for (; ;) {
        inl = fread(in, 1, 1024, fpIn);
        if (inl <= 0) break;

        rv = EVP_DecryptUpdate(&ctx, out, &outl, in, inl);
        if (rv != 1) {
            fclose(fpIn);
            fclose(fpOut);
            EVP_CIPHER_CTX_cleanup(&ctx);
            return -1;
        }
        fwrite(out ,1, outl, fpOut);
    }

    rv = EVP_DecryptFinal_ex(&ctx, out, &outl);
    if (rv != 1) {
        fclose(fpIn);
        fclose(fpOut);
        EVP_CIPHER_CTX_cleanup(&ctx);
        return -1;
    }

    fwrite(out, 1, outl, fpOut);
    fclose(fpIn);
    fclose(fpOut);
    EVP_CIPHER_CTX_cleanup(&ctx);
    //syslog(LOG_ERR,"dencode end\n");
    return 1;
}
