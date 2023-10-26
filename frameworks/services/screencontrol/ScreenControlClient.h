/** @file ScreenControlService.h
 *  @par Copyright:
 *  - Copyright 2011 Amlogic Inc as unpublished work
 *  All Rights Reserved
 *  - The information contained herein is the confidential property
 *  of Amlogic.  The use, copying, transfer or disclosure of such information
 *  is prohibited except by express written agreement with Amlogic Inc.
 *  @author   huijie huang
 *  @version  1.0
 *  @date     2019/05/06
 *  @par function description:
 *  - screen capture
 *  - screen record
 *  @warning This class may explode in your face.
 *  @note If you inherit anything from this class, you're doomed.
 */

#ifndef ANDROID_GUI_SCREENCONTROLCLIENT_H
#define ANDROID_GUI_SCREENCONTROLCLIENT_H

#include <utils/Errors.h>
#include <string>
#include <vector>
#include <vendor/amlogic/hardware/screencontrol/1.0/IScreenControl.h>
#include <vendor/amlogic/hardware/screencontrol/1.0/IScreenControlCallback.h>
#include <vendor/amlogic/hardware/screencontrol/1.0/types.h>
#include <utils/Mutex.h>

using ::vendor::amlogic::hardware::screencontrol::V1_0::IScreenControl;
using ::vendor::amlogic::hardware::screencontrol::V1_0::IScreenControlCallback;

using ::android::hardware::Return;
using ::android::hardware::hidl_memory;

namespace android {

enum record_type
{
    RECORD_TYPE_TS,
	RECORD_TYPE_AVC,
    RECORD_TYPE_YUV
};

class ScreenControlClient  : public IScreenControlCallback {
public:
    ScreenControlClient();

    ~ScreenControlClient();
    class AvcRecordCallback : public virtual RefBase {
    public:
        AvcRecordCallback() = default;
        virtual ~AvcRecordCallback() = default;
        virtual void onAvcDataArouse(void* data, int32_t size, int32_t frame_type, int64_t pts) = 0;
    };

    class YuvRecordCallback : public virtual RefBase {
    public:
        YuvRecordCallback() = default;
        virtual ~YuvRecordCallback() = default;
        virtual void onYuvDataArouse(void* data, int32_t size) = 0;
    };

    class MicroDimCallback : public virtual RefBase {
    public:
        MicroDimCallback() = default;
        virtual ~MicroDimCallback() = default;
        virtual void onMicroDimArouse(void* data, int32_t size) = 0;
    };
    void setAvcCallback(const sp<AvcRecordCallback>&f);
    void setYuvCallback(const sp<YuvRecordCallback>&f);
    void setMicroDimCallback(const sp<MicroDimCallback>&f);

    static ScreenControlClient *getInstance();
    /* start capturing screen to get rgb data
    *
    * left,top,right,bottom: the coordinates of rect which you want to capture the screen.
    *              if you want to capture full screen,its should be set to (0,0,width,height).
    *
    * width,height: it's the resloution of output buffer.
    *
    * sourceType: the layer of screen, 0:video only 1: osd+video 2:osd only
    *
    * buffer:the address of output data,the format of buffer is RGBA888,
    *       it must to be free after call the function,eg: delete []buffer;
    *
    * bufSize: the size of output buffer
    *
    * return true if success, false otherwise
    */
    int32_t startScreenCapBuffer(int32_t left, int32_t top, int32_t right, int32_t bottom,
        int32_t width, int32_t height, int32_t sourceType, void **buffer, int32_t *bufSize);
    /* start record screen to get ts file
    *
    * left,top,right,bottom: the coordinates of rect which you want to capture the screen.
    *              if you want to capture full screen,its should be set to (0,0,width,height).
    *
    * width,height: it's the resloution of output data.
    *
    * sourceType: the layer of screen, 0:video only 1: osd+video 2:osd only
    *
    * frameRate: the frame rate of the data
    *
    * bitRate: the bit rate of the output data.
    *          The unit of this variable is seconds.
    *
    * limitTimeSec: the time you want to record the screen.
    *               The unit of this variable is seconds.
    *
    * filename : the ts file name which you want to save.
    *
    * return true if success, false otherwise
    */
    int32_t startScreenRecord(int32_t left, int32_t top, int32_t right, int32_t bottom, int32_t width, int32_t height, int32_t frameRate,
        int32_t bitRate, int32_t limitTimeSec, int32_t sourceType, const char* filename);
    // if you want to call the function to record full screen and get the TS file.
    int32_t startScreenRecord(int32_t width, int32_t height, int32_t frameRate,
        int32_t bitRate, int32_t limitTimeSec, int32_t sourceType, const char* filename);

    /* start to screen record to get the es data,and the format is H264.
    *
    * left,top,right,bottom: the coordinates of rect which you want to capture the screen.
    *              if you want to capture full screen,its should be set to (0,0,width,height).
    *
    * width,height: it's the resloution of output data.
    *
    * sourceType: the layer of screen, 0:video only 1: osd+video 2:osd only
    *
    * frameRate: the frame rate of the data
    *
    * bitRate: the bit rate of the output data.
    *          The unit of this variable is seconds.
    *
    *
    * return true if success, false otherwise
    */

    int32_t startAvcScreenRecord(int32_t left, int32_t top, int32_t right, int32_t bottom, int32_t width,
                                int32_t height, int32_t frameRate,int32_t bitRate, int32_t sourceType);
    // if you want to call the function to record full screen and get the ES data
    int32_t startAvcScreenRecord(int32_t width, int32_t height, int32_t frameRate,int32_t bitRate, int32_t sourceType);


    /* start to screen record to get the yuv data,and the format is NV21.
    *
    * left,top,right,bottom: the coordinates of rect which you want to capture the screen.
    *              if you want to capture full screen,its should be set to (0,0,width,height).
    *
    * width,height: it's the resloution of output data.
    *
    * sourceType: the layer of screen, 0:video only 1: osd+video 2:osd only
    *
    * frameRate: the frame rate of the data
    *
    * return true if success, false otherwise
    */
    int32_t startYuvScreenRecord(int32_t left, int32_t top, int32_t right, int32_t bottom, int32_t width,
                                int32_t height, int32_t frameRate, int32_t sourceType);
    // if you want to call the function to record full screen and get the yuv data
    int32_t startYuvScreenRecord(int32_t width, int32_t height, int32_t frameRate, int32_t sourceType);

    int32_t startMicroDim(int32_t width, int32_t height);

    Return<void> onAvcDataArouse(const hidl_memory &mem,int32_t size, int32_t frame_type,int64_t pts);

    Return<void> onYuvDataArouse(const hidl_memory &mem,int32_t size);

    Return<void> onMicroDimArouse(const hidl_memory &mem,int32_t size);



    void forceStop();

private:
    static ScreenControlClient *mInstance;
    sp<IScreenControl> mScreenCtrl;
    wp<AvcRecordCallback> mAvcCb;
    wp<YuvRecordCallback> mYuvCb;
    wp<MicroDimCallback> mMicroDimCb;
    Mutex mLock;
};

}
#endif

