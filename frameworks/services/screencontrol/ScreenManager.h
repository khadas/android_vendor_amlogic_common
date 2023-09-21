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

#ifndef ANDROID_SCREENCONTROL_SCREENMANAGER_H
#define ANDROID_SCREENCONTROL_SCREENMANAGER_H
#include "area.h"
#include "size.h"
#include "ulit.h"
#include <mutex>
#include <list>
#include<map>
#include "../../../../../hardware/amlogic/screen_source/aml_screen.h"
#include <hardware/hardware.h>
#include "ScreenControlDebug.h"
namespace android {


typedef enum{
    SCREENCONTROL_PIX_FMT_NV21,
    SCREENCONTROL_PIX_FMT_NV12,
    SCREENCONTROL_PIX_FMT_RGBA888,
    SCREENCONTROL_PIX_FMT_RGB565,
    SCREENCONTROL_PIX_FMT_UNKNOWN,
}aml_screencontrol_format;

typedef enum {
    AML_CAPTURE_VIDEO = 0,
    AML_CAPTURE_OSD_VIDEO,
    AML_CAPTURE_OSD_ONLY,
    SCAML_CAPTURE_UNKNOWN
}aml_source_type;


struct InputParmeter {
    InputParmeter(): source_type(SCAML_CAPTURE_UNKNOWN),
                    frame_rate(0),format(SCREENCONTROL_PIX_FMT_UNKNOWN){};
    InputParmeter(InputParmeter&&) = default;
    ~InputParmeter() = default;
    std::unique_ptr<Size> size;
    std::unique_ptr<Area> area;
    int32_t source_type;
    int32_t frame_rate;
    aml_screencontrol_format format;
};


// Record for output buffers.
struct OutputRecord {
    OutputRecord(): index(0), tv_usec(0), raw_buffer(nullptr),  canvas_buffer(nullptr){};
    OutputRecord(int32_t _index, int32_t _raw_buffer_size, uint64_t _tv_usec,uint8_t* _raw_buffer,void* _canvas_buffer,aml_screencontrol_format _format) :
                index(_index), raw_buffer_size(_raw_buffer_size), tv_usec(_tv_usec), raw_buffer(_raw_buffer),  canvas_buffer(_canvas_buffer),format(_format) {}
    OutputRecord(OutputRecord&&) = default;
    ~OutputRecord() = default;

    int32_t index;
    int32_t raw_buffer_size;
    int64_t tv_usec;
    uint8_t*   raw_buffer;
    void*   canvas_buffer;
    aml_screencontrol_format format;
};

class ScreenManager {
public:
    class ScreenMangerCallback {
    public:
        ScreenMangerCallback() = default;
        virtual ~ScreenMangerCallback() = default;
        virtual void PictureReady(const OutputRecord &output) = 0;

    };
    /* if other client want to use the screen manger at the same time,
       the info of client will be save to it.
    */
    struct MultiClientInfo {
        MultiClientInfo(aml_screencontrol_format f,ScreenMangerCallback *c): format(f),
                        cb(c){};
        MultiClientInfo(MultiClientInfo&&) = default;
        ~MultiClientInfo() = default;
        std::unique_ptr<Size> size;
        aml_screencontrol_format format;
        ScreenMangerCallback *cb;
    };
    static ScreenManager* getInstance() {
        static ScreenManager value;
        return &value;
    }
    bool start(std::unique_ptr<InputParmeter>& input, ScreenMangerCallback *client,int32_t *id,bool multi_acquire = true);
    void stop(int32_t client_id);
    bool realseBuffer(int32_t client_id,int32_t index);
    // the callback from screen source
    int32_t dataCallBack(aml_screen_buffer_info_t *buffer);



private:
    ScreenManager();
    ScreenManager(const ScreenManager& other) = delete;
    ScreenManager& operator = (const ScreenManager&) = delete;
    virtual ~ScreenManager();
    bool startMoreClient(std::unique_ptr<InputParmeter>& input, ScreenMangerCallback *client,int32_t *id);
    bool setFormat2Device();
    // get the new buffer by changing format.
    bool getBufferWithFormat(uint8_t *src ,int32_t src_size, uint8_t *dst,std::unique_ptr<InputParmeter>& src_parmeter,
                                                                        std::unique_ptr<MultiClientInfo>& dst_parmeter );
    int32_t getBufferSize(std::unique_ptr<Size>& size,aml_screencontrol_format format);
    bool isSupportFormat();
    bool setVideoRotation(int32_t degree);
    ScreenMangerCallback* mScreenMangerCallback;
    std::unique_ptr<InputParmeter> mInputParmeter;
    aml_screen_module_t* mScreenModule;
    aml_screen_device_t* mScreenDev;
    std::mutex mLock;
    std::list<std::unique_ptr<OutputRecord>> mOutputRecordQueue;
    std::map<int32_t,std::unique_ptr<MultiClientInfo>> mMultiClientMap;
    int32_t mBufferSize;
    int32_t mFormat;
    int32_t mPortType;
    int32_t mClientNum;
    //This is true if the client wants to fetch the data more than once, and false otherwise
    bool mIsMultiAcquire;
    bool mStart;
};



};// namespace android

#endif // ANDROID_SCREENCONTROL_SCREENMANAGER_H