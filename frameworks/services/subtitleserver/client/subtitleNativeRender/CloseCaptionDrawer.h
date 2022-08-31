#pragma once
#include <SkBitmap.h>
//#include <SkImage.h>

#include <SkCanvas.h>
#include <SkPaint.h>
#include <SkTypeface.h>
#include "CloseCaption.h"

// TODO: maybe a Drawer and inherited xxDrawer?
using Amlogic::NativeRender::CloseCaption::Configure;
class CloseCaptionDrawer {
public:
    static CloseCaptionDrawer& GetInstance();

    void draw(SkCanvas &canvas, const char *str);

protected:
    CloseCaptionDrawer();
    ~CloseCaptionDrawer();

private:
    static CloseCaptionDrawer _instance;

    std::shared_ptr<Configure> mConfig;
};

