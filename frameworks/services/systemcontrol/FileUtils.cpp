/*
 * Copyright (C) 2011 The Android Open Source Project
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
 *  @author   Tellen Yu
 *  @version  2.0
 *  @date     2014/09/09
 *  @par function description:
 *  - 1 write property or sysfs in daemon
 */
#include "FileUtils.h"
#include "PQ/utils/include/CFile.h"
#include "common.h"
#ifndef RECOVERY_MODE
using namespace android;
#endif
struct CyParam
{
    const char* src;
    const char* dest;
};
void * filepy(void *arg) {
    CyParam *pstru;
    int fd;
    CFile srcFile;
    pstru = ( struct CyParam *) (arg);
    umask(0000);
    fd = creat(pstru->dest,0644);
    ALOGE("copy file from %s to %s",pstru->src,pstru->dest);
    if (srcFile.openFile(pstru->src) > 0) {
        srcFile.copyTo(pstru->dest);
    }
    return 0;
}
bool FileUtils::cpyFileWithThraed(const char* src, const char* dest) {
    pthread_t id;
    struct CyParam param;
    param.src = src;
    param.dest = dest;
    int ret = pthread_create(&id, 0, filepy, &param);
    if (ret != 0) {
        SYS_LOGE("Create params file copy error!\n");
        return false;
    }
    return true;
}
bool FileUtils::cpyFileDirect(const char* src, const char* dest) {
    int fd;
    umask(0000);
    fd = creat(dest,0644);
    CFile srcFile;
    if (srcFile.openFile(src) > 0) {
        srcFile.copyTo(dest);
        return true;
    }
    return false;
}
