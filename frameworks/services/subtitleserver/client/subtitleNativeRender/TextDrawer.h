#pragma once
#include <SkBitmap.h>
//#include <SkImage.h>

#include <SkCanvas.h>
#include <SkPaint.h>
#include <SkTypeface.h>

#include "MyLog.h"
// TODO: maybe a Drawer and inherited xxDrawer?

class TextDrawer {
public:
    static TextDrawer& GetInstance();
    bool drawLine(SkCanvas &canvas, const char *data);
    bool drawLineAt(SkCanvas &canvas, const char *data, int x, int y);

protected:
    TextDrawer();
    ~TextDrawer();

private:
    static TextDrawer _instance;
    sk_sp<SkTypeface> mTypeFace;
};

