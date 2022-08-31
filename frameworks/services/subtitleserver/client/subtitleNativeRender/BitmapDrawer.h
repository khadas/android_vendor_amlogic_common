#pragma once
#include <SkBitmap.h>
//#include <SkImage.h>

#include <SkCanvas.h>
#include <SkPaint.h>
#include <SkTypeface.h>

// TODO: maybe a Drawer and inherited xxDrawer?

class BitmapDrawer {
public:
    static BitmapDrawer& GetInstance();
    bool draw(SkCanvas &canvas, const char *data, int x, int y, int width, int height);
    bool drawScaled(SkCanvas &canvas, const char *data, int x, int y, int width, int height, int videoWidth, int videoHeight);

protected:
    BitmapDrawer();
    ~BitmapDrawer();

private:
    static BitmapDrawer _instance;
};
