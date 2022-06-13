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

#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <map>
#include <cmath>
#include <string>
#include <pthread.h>
#include <linux/fb.h>
#include <semaphore.h>
#include <utils/Mutex.h>

using namespace android;

class FileUtils{
public:
    static bool cpyFileWithThraed(const char* src, const char* dest);
    static bool cpyFileDirect(const char* src, const char* dest);
};
#endif // FILE_UTILS_H
