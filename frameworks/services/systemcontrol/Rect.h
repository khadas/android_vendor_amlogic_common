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
 *  @date     2014/10/23
 *  @par function description:
 *  - 1 set display mode
 */
#ifndef ANDROID_DISPLAY_RECT_H
#define ANDROID_DISPLAY_RECT_H

namespace droidlogic{
class Rect
{
public:
    Rect(unsigned int left,unsigned int top, unsigned int right, unsigned int bottom);
    Rect();
    bool inside(Rect& rect);
    bool inscreen();
    unsigned int left;
    unsigned int top;
    unsigned int right;
    unsigned int bottom;
};
};

#endif
