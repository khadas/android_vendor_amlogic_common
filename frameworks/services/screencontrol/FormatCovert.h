/*
**
** Copyright 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef ANDROID_SCREENCONTR_FORMAT_COVERT_H
#define ANDROID_SCREENCONTR_FORMAT_COVERT_H

#include <IONmem.h>
#include <aml_ge2d.h>
#include <ge2d_port.h>
#include <ScreenManager.h>

namespace android {


class FormatCovert {

public:

    FormatCovert() {};

    virtual ~FormatCovert() {};

    virtual bool covert(const char* src_buff,unsigned int src_fmt, size_t src_w, size_t src_h,
                            char* dst_buff,unsigned int dst_fmt, size_t dst_w, size_t dst_h) = 0;

};

class SoftWareFormatCovert : public FormatCovert {

public:
    SoftWareFormatCovert();
    virtual ~SoftWareFormatCovert();
    bool covert(const char* src_buff,unsigned int src_fmt, size_t src_w, size_t src_h,
                            char* dst_buff,unsigned int dst_fmt, size_t dst_w, size_t dst_h);
};

class HardWareFormatCovert : public FormatCovert {

public:
    HardWareFormatCovert();
    virtual ~HardWareFormatCovert();
    bool covert(const char* src_buff,unsigned int src_fmt, size_t src_w, size_t src_h,
                            char* dst_buff,unsigned int dst_fmt, size_t dst_w, size_t dst_h);
private:
    aml_ge2d_t m_amlge2d;
    int32_t ge2DFmtConvert(int32_t dst_fd, int32_t dst_fmt, size_t dst_w, size_t dst_h,
                                 int32_t src_fd, int32_t src_fmt, size_t src_w, size_t src_h);
};



};//namespace android


#endif
