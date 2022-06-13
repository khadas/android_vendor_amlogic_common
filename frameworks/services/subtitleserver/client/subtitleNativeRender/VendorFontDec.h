#ifndef __LIBS_VENDORUTIL_H
#define __LIBS_VENDORUTIL_H

#ifdef __cplusplus
extern "C" {
#endif

void vendor_font_release(const char*ttf_dir,const char* path);
void copy_file (const char*src_path,const char*des_path);
int  encry_file(const char*src_path,const char *des_path);
int decrypt_file(const char*src_path,const char *des_path);

#ifdef __cplusplus
};
#endif


#endif
