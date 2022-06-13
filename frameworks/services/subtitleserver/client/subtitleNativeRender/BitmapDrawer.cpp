
#include "BitmapDrawer.h"
BitmapDrawer& BitmapDrawer::GetInstance() {
    return _instance;
}
BitmapDrawer BitmapDrawer::_instance;

BitmapDrawer::BitmapDrawer() {

}
BitmapDrawer::~BitmapDrawer() {

}

bool BitmapDrawer::draw(SkCanvas &canvas, const char *data, int x, int y, int width, int height) {

/*
    1. clear canvas dirty
*/
    canvas.clear(0x00000000);


/* 2. draw new bitmap */
    SkBitmap bitmap;
    ssize_t bpr =width * 4;//bytesPerPixel(buf.format);
    bitmap.installPixels(SkImageInfo::MakeN32Premul(width, height), (void *)data, bpr);
    canvas.drawBitmap(bitmap, x, y);
    return true;
}

bool BitmapDrawer::drawScaled(SkCanvas &canvas, const char *data, int x, int y, int width, int height, int videoWidth, int videoHeight) {

    return draw(canvas, data, x, y, width, height);
}

