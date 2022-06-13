#include "SkTextBox.h"
#include "SkBlurMaskFilter.h"
#include "TextDrawer.h"
TextDrawer& TextDrawer::GetInstance() {
    return _instance;
}
TextDrawer TextDrawer::_instance;

TextDrawer::TextDrawer() {
    // TODO: initialize CJK fonts.
    mTypeFace =SkTypeface::MakeFromFile("/system/fonts/NotoSansCJK-Regular.ttc");
    if (mTypeFace == nullptr) {
        ALOGE("Error! cannot initialize typeface");
    } else {
        ALOGD("initialized typeface");
    }
}

TextDrawer::~TextDrawer() {

}

bool TextDrawer::drawLine(SkCanvas &canvas, const char *data) {

    /*
        1. clear canvas dirty
    */
    canvas.clear(0x00000000);

    SkTextBox box;

// Draw
    SkPaint paint;
    paint.setARGB(255, 255, 255, 255);
    paint.setTextSize(60);
    paint.setAntiAlias(true);
    paint.setSubpixelText(true);
    paint.setLCDRenderText(true);
    paint.setTypeface(mTypeFace);

    paint.setTextAlign(SkPaint::kCenter_Align);


    SkImageInfo info = canvas.imageInfo();

    // Skia can have auto line break and text wrapper.
    box.setBox(0, 0, info.width(), info.height());
    box.setSpacingAlign(SkTextBox::SpacingAlign::kEnd_SpacingAlign);



    const SkScalar radius = 3.0f;
    //const SkScalar xDrop = 5.0f;
    //const SkScalar yDrop = 5.0f;
    const uint8_t blurAlpha = 200;

    SkPaint blur(paint);
    blur.setAlpha(blurAlpha);
    blur.setARGB(255, 0, 0, 0);
    blur.setMaskFilter(SkBlurMaskFilter::Make(
        kNormal_SkBlurStyle,
        SkBlurMaskFilter::ConvertRadiusToSigma(radius), 0));

    box.setText(data, strlen(data), blur);
    box.draw(&canvas);

    box.setText(data, strlen(data), paint);
    box.draw(&canvas);

    SkRect bounds;
    box.getBox(&bounds);
    ALOGD("%d [%f %f %f %f] [%d %d] %s",
        box.countLines(), bounds.fLeft, bounds.fTop, bounds.fRight, bounds.fBottom, info.width(), info.height(), data);

    return true;
}

bool TextDrawer::drawLineAt(SkCanvas &canvas, const char *data, int x, int y) {
    return true;
}

