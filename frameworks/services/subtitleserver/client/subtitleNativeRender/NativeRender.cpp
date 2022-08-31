#define LOG_TAG "SubtitleControl-api"
#include "MyNativeWindowApi.h"
#include "NativeRender.h"

#include <SkBitmap.h>
//#include <SkImage.h>

#include <SkCanvas.h>
#include <SkPaint.h>
#include <SkTypeface.h>

#include "MyLog.h"
#include "TextDrawer.h"
#include "BitmapDrawer.h"
#include "CloseCaptionDrawer.h"

NativeRender::NativeRender(SubNativeRenderHnd win) {
    mWin = win;
}

NativeRender::~NativeRender() {
}

bool NativeRender::clear() {
    SubNativeRenderBuffer buf;
    NativeWindowLock(mWin, &buf);
    memset(buf.bits, 0, buf.stride*buf.height*4);
    NativeWindowUnlockAndPost(mWin);

    return true;
}

bool NativeRender::clearDirty() {
    return clear();
}


bool NativeRender::render(const char *data,
                int size, AmlSubDataType type,
                int x, int y, int width, int height,
                int videoWidth, int videoHeight) {

    SubNativeRenderBuffer buf;
    NativeWindowLock(mWin, &buf);

    SkBitmap surfaceBitmap;
    ssize_t bpr = buf.stride * 4;//bytesPerPixel(buf.format);
    surfaceBitmap.installPixels(SkImageInfo::MakeN32Premul(buf.width, buf.height), buf.bits, bpr);
    SkCanvas surfaceCanvas(surfaceBitmap);

    switch (type) {
        case SUB_DATA_TYPE_STRING:
            TextDrawer::GetInstance().drawLine(surfaceCanvas, data);
            break;
        case SUB_DATA_TYPE_CC_JSON:
            CloseCaptionDrawer::GetInstance().draw(surfaceCanvas, data);
            break;
        case SUB_DATA_TYPE_BITMAP:
            BitmapDrawer::GetInstance().draw(surfaceCanvas, data, x, y, width, height);
            break;
        case SUB_DATA_TYPE_POSITON_BITMAP:
            BitmapDrawer::GetInstance().drawScaled(surfaceCanvas, data, x, y, width, height, videoWidth, videoHeight);
            break;
    }

    NativeWindowUnlockAndPost(mWin);

    ////////////
    return true;
}


