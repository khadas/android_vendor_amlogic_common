/** @file ScreenControlService.h
 *  @par Copyright:
 *  - Copyright 2011 Amlogic Inc as unpublished work
 *  All Rights Reserved
 *  - The information contained herein is the confidential property
 *  of Amlogic.  The use, copying, transfer or disclosure of such information
 *  is prohibited except by express written agreement with Amlogic Inc.
 *  @author   liangzhuo xie
 *  @version  1.0
 *  @date     2018/08/18
 *  @par function description:
 *  - screen capture
 *  - screen record
 *  @warning This class may explode in your face.
 *  @note If you inherit anything from this class, you're doomed.
 */

#ifndef ANDROID_GUI_SCREENCONTROLSERVICE_H
#define ANDROID_GUI_SCREENCONTROLSERVICE_H

#include <cutils/compiler.h>
#include <stdint.h>
#include <binder/Binder.h>
#include <sys/types.h>
#include <utils/String16.h>

#include <media/stagefright/MediaSource.h>
#include <media/stagefright/MediaBuffer.h>

#include <utils/Errors.h>  // for status_t
#include <utils/KeyedVector.h>
#include <utils/String8.h>

#include <binder/BinderService.h>
#include <binder/MemoryDealer.h>

//#include <android/native_window.h>
#include <hardware/hardware.h>
#include "../../../../../hardware/amlogic/screen_source/aml_screen.h"
#include <utils/List.h>
#include <utils/RefBase.h>
#include <utils/threads.h>
#include <ui/PixelFormat.h>

#include "IScreenControlService.h"
#include "ScreenManager.h"
#include <Media2Ts/esconvertor.h>


namespace android {

#define SCREENCONTROL_GRALLOC_USAGE  ( GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_SW_READ_RARELY | GRALLOC_USAGE_SW_WRITE_NEVER )

class DeathNotifier;

class ScreenControlService :
            public BinderService<ScreenControlService>,
            public BnScreenControlService {

    friend class BinderService<ScreenControlService>;
public:

    ScreenControlService();

    virtual ~ScreenControlService();

    virtual int startScreenRecord(int32_t width, int32_t height, int32_t frameRate, int32_t bitRate, int32_t limitTimeSec, int32_t sourceType, const char* filename);

    virtual int startScreenCap(int32_t left, int32_t top, int32_t right, int32_t bottom, int32_t width, int32_t height, int32_t sourceType, const char* filename);

    virtual int startScreenCapBuffer(int32_t left, int32_t top, int32_t right, int32_t bottom, int32_t width, int32_t height, int32_t sourceType, void *buffer, int32_t *bufSize);
    virtual int setScreenRecordCropArea(int32_t left, int32_t top, int32_t right, int32_t bottom);
    virtual void forceStop();

    virtual int startYuvRecord(int32_t width, int32_t height, int32_t frameRate, int32_t sourceType);

    virtual int getYuvRecordData(void *dstBuffer,int32_t bufSize);


    virtual bool isHaveYuvDate();

    virtual int checkYuvRecordDone();

    // H264/AVC record
    virtual int startAvcRecord(int32_t width, int32_t height, int32_t frameRate, int32_t bitRate, int32_t sourceType);
    virtual int getAvcRecordData(void *dstBuffer, int32_t *dstBufferSize, int64_t *nowtime);
    virtual bool isHaveAvcDate();
    virtual int checkAvcRecordDone();

    virtual int release();
    static void instantiate(bool lazyMode=false);
    static ScreenControlService* getInstance();
    virtual int notifyProcessDied(const sp<IBinder> &binder);
//    virtual SkColorType flinger2skia(PixelFormat f);

private:
    sp<DeathNotifier> mDeathNotifier;
    bool mNeedStop;
    int mPicFd;  // use for save picture
    sp<ESConvertor> mVideoConvertor;
    int32_t mRecordCorpX;
    int32_t mRecordCorpY;
    int32_t mRecordCorpWidth;
    int32_t mRecordCorpHeight;
    ScreenManager* mScreenManager;
    int mYuvClientId;
    Mutex mLock;
};

// ----------------------------------------------------------------------------
}; // namespace android
#endif
