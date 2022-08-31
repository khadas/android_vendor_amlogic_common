/*
 * Copyright (C) 2006 The Android Open Source Project
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
 */

#ifndef ANDROID_DROIDLOGIC_SCREENCONTROL_DEBUG_H
#define ANDROID_DROIDLOGIC_SCREENCONTROL_DEBUG_H

namespace android {

class ScreenControlDebug {
public:
    ScreenControlDebug();
    ~ScreenControlDebug();

    static bool initDebug();
    static bool canDebug();

private:
    static bool mIsInit;
    static bool mCanDebug;
};

}

#endif

