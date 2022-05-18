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
#include <vendor/amlogic/hardware/screencontrol/1.0/types.h>
#include <utils/Mutex.h>

using ::vendor::amlogic::hardware::screencontrol::V1_0::IScreenControl;
using ::vendor::amlogic::hardware::screencontrol::V1_0::Result;
using ::android::hardware::Return;


namespace android {

enum record_type
{
    RECORD_TYPE_TS,
	RECORD_TYPE_AVC,
    RECORD_TYPE_YUV
};

class ScreenControlClient  : virtual public RefBase {
public:
    ScreenControlClient();

    ~ScreenControlClient();

    static ScreenControlClient *getInstance();

    class AvcCallback: public virtual RefBase{
    public:
        virtual void onAvcDataArouse(void *data, int32_t size, uint8_t frameType, int64_t pts) = 0;
        virtual void onAvcDataOver() = 0;
    };
    class YuvCallback: public virtual RefBase{
    public:
        virtual void onYuvDataArouse(void *data, int32_t size) = 0;
        virtual void onYuvDataOver() = 0;
    };
    virtual void setAvcCallback(const sp<AvcCallback>&f);
    virtual void setYuvCallback(const sp<YuvCallback>&f);

    int startScreenRecord(int32_t width, int32_t height, int32_t frameRate,
        int32_t bitRate, int32_t limitTimeSec, int32_t sourceType, const char* filename);
    int startScreenRecord(int32_t left, int32_t top, int32_t right, int32_t bottom, int32_t width, int32_t height, int32_t frameRate,
        int32_t bitRate, int32_t limitTimeSec, int32_t sourceType, const char* filename);

    int startAvcScreenRecord(int32_t width, int32_t height, int32_t frameRate,
        int32_t bitRate, int32_t sourceType);

    bool getAvcRecordData(void **buffer, int *bufSize, uint8_t *frameType , int64_t *pts);

    bool checkAvcRecordDone();

    int startYuvScreenRecord(int32_t width, int32_t height,int32_t frameRate,int32_t sourceType);
    bool checkYuvRecordDone();
    bool getYuvRecordData(void **buffer, int *bufSize);

    int startScreenCap(int32_t left, int32_t top, int32_t right, int32_t bottom,
        int32_t width, int32_t height, int32_t sourceType, const char* filename);

    int startScreenCapBuffer(int32_t left, int32_t top, int32_t right, int32_t bottom,
        int32_t width, int32_t height, int32_t sourceType, void **buffer, int *bufSize);

    void forceStop();

private:
    wp<AvcCallback> mAvcCb;
    wp<YuvCallback> mYuvCb;
    static ScreenControlClient *mInstance;
    sp<IScreenControl> mScreenCtrl;
    int mRecordType;
    static void *ThreadWrapper(void *me);
    void threadFunc();
    Mutex mLock;
};

}
#endif

