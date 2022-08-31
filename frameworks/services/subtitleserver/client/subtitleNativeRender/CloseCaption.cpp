#include "CloseCaption.h"

#include <json/json.h>

#include <SkColor.h>
#include <SkPaint.h>
#include <SkPath.h>
#include "SkTextBox.h"

#include "MyLog.h"

namespace Amlogic {
namespace NativeRender {
namespace CloseCaption {

static inline int getFontMetricsInt(SkPaint &paint) {
    SkPaint::FontMetrics metrics;

    paint.getFontMetrics(&metrics);
    int ascent = SkScalarRoundToInt(metrics.fAscent);
    int descent = SkScalarRoundToInt(metrics.fDescent);
    int leading = SkScalarRoundToInt(metrics.fLeading);

    return descent - ascent + leading;
}


static inline int getFontMetricsDecent(SkPaint &paint) {
    SkPaint::FontMetrics metrics;

    paint.getFontMetrics(&metrics);
    int descent = SkScalarRoundToInt(metrics.fDescent);

    return descent;
}


static inline bool isMultiByteUtf8String(std::string s) {
    for (int i=0; i<s.length(); ++i) {
      if ((0x80 & s[i]) != 0) return true;
    }
    return false;
}


Configure::Configure() {
    constructFontManager();
    constructCaptionScreen();

    // disable default opacity, set to larger than 0xFF
    mDefaultFillOpacity = 0;
}

static bool gDebug = true;
static bool gDump = true;

void CaptionScreen::updateScreen(int w, int h) {
    //Safe title must be calculated using video width and height.
    mWidth = w;
    mHeight = h;
    mSafeTitleHeight = mHeight * mSafeTitleHPercent;

//    if (mVideohvRateOnScreen > 1.7) {
        mSafeTitleWidth = mWidth * mSafeTitleWPercent;
//    } else {
//        mSafeTitleWidth = mWidth * 12 / 16 * mSafeTitleWPercent;
//    }
    //Font height is relative with safe title height, but now ratio of width and height can not be changed.
    mMaxFontHeight = mSafeTitleHeight / mCcRowCount;

//    if (mVideohvRateOnScreen > 1.7) {
        mMaxFontWidth = mMaxFontHeight * 0.8;
//    } else {
//        mMaxFontWidth = mMaxFontHeight * 0.6;
//    }
    mMaxFontSize = mMaxFontHeight;

    ALOGD("=============updateScreen: mMaxFontSize:%f mMaxFontHeight:%f, mSafeTitleHeight:%f, mCcRowCount:%d",
        mMaxFontSize, mMaxFontHeight, mSafeTitleHeight, mCcRowCount);

    //This is used for postioning character in 608 mode.
    mFixedCharWidth = mSafeTitleWidth / (mCcColCount + 1);

    mAnchorHorizon = ((mVideohvRateOrigin & 1) == 0)?210:160; //16:9 or 4:3
    mAnchorVertical = 75;

    //This is used for calculate coordinate in non-relative anchor mode
    mAnchorHorizonDensity = mSafeTitleWidth / mAnchorHorizon;
    mAnchorVerticalDensity = mSafeTitleHeight / mAnchorVertical;

    mWindowBorderWidth = (float)(mMaxFontHeight/6);
    mSafeTitleLeft = (mWidth - mSafeTitleWidth)/2;
    mSafeTitleRight = mSafeTitleLeft + mSafeTitleWidth;
    mSafeTitleTop = (mHeight - mSafeTitleHeight)/2;
    mSafeTitleButton = mSafeTitleTop + mSafeTitleHeight;
    mScreenLeft = getWindowLeftTopX(true, 0, 0, 0);
    mScreenRight = getWindowLeftTopY(true, 0, 0, 0);
}

double CaptionScreen::getWindowLeftTopX(
        bool anchorRelative, int anchorH, int anchorPoint, double rowLength) {
    double offset;
    /* Get anchor coordinate x */
    if (!anchorRelative) {
        /* mAnchorH is horizontal steps */
        offset = mSafeTitleWidth * anchorH / mAnchorHorizon + mSafeTitleLeft;
    } else {
        /* mAnchorH is percentage */
        offset = mSafeTitleWidth * anchorH / 100 + mSafeTitleLeft;
    }
    switch (anchorPoint) {
        case 0:
        case 3:
        case 6:
            return offset;
        case 1:
        case 4:
        case 7:
            return offset - rowLength/2 + mSafeTitleWidth / 2;
        case 2:
        case 5:
        case 8:
            return offset - rowLength + mSafeTitleWidth;
        default:
            return -1;
    }
}

double CaptionScreen::getWindowLeftTopY(bool anchorRelative, int anchorV, int anchorPoint, int rowCount) {
    double offset;
    double position;

    if (!anchorRelative) {
        /* mAnchorV is vertical steps */
        offset = mSafeTitleHeight * anchorV / mAnchorVertical + mSafeTitleTop;
    } else {
        /* mAnchorV is percentage */
        offset = mSafeTitleHeight * anchorV / 100 + mSafeTitleTop;
    }

    switch (anchorPoint) {
        case 0:
        case 1:
        case 2:
            position = offset;
            break;
        case 3:
        case 4:
        case 5:
            position = offset - (rowCount * mMaxFontHeight)/2;
            break;
        case 6:
        case 7:
        case 8:
            position = offset - rowCount * mMaxFontHeight;
            break;
        default:
            position = mSafeTitleTop - rowCount * mMaxFontHeight;
            break;
    }

    if ((position) < mSafeTitleTop) {
        position = mSafeTitleTop;
    } else if ((position + rowCount * mMaxFontHeight) > mSafeTitleButton) {
        position = mSafeTitleButton - rowCount * mMaxFontHeight;
    }
    return position;
}


RowString::RowString(CaptionVersion ver, std::shared_ptr<Configure> config, Json::Value &root) {
    mStringLengthOnPaint = 0.0f;
    mVersion = ver;
    mConfig = config;

    // default, none
    mEdgeType.assign("none");
    mFgOpacityInt = 0xff;
    mBgOpacityInt = 0xff;

    mData = root["data"].asString();
    mIsMultiByteStr = isMultiByteUtf8String(mData);

    mUseCaptionManagerStyle = false;
    mPenSize = root["pen_size"].asString();
    if (mPenSize == "") mPenSize = "standard";
    if (ignoreCaseCompare(mPenSize, "small")) {
        mFontScale = 0.5;
    } else if (ignoreCaseCompare(mPenSize, "large")) {
        mFontScale = 0.8;
    } else if (ignoreCaseCompare(mPenSize, "standard")) {
        mFontScale = 0.65;
    } else {
        ALOGE("Font scale not supported: %s",  mPenSize.c_str());
        mFontScale = 0.8;
    }
    ALOGE("mPenSize: %s mFontScale:%f",  mPenSize.c_str(), mFontScale);

    if ((root["font_style"].asString()).compare("default") != 0) {
        mFontStyle = root["font_style"].asString();
        if (mFontStyle.compare("") == 0) {
            //mFontStyle.assign("mono_serif");
            mFontStyle.assign("fallback");
        }
    }

    mOffset = root["offset"].asString();
    if (mOffset.compare("") == 0) mFontStyle = "normal";

    mItalics = root["italics"].asBool();
    mUnderline = root["underline"].asBool();
    mFgColor = root["fg_color"].asInt();
    mFgOpacity = root["fg_opacity"].asString();
    if (mFgOpacity.compare("") == 0) mFgOpacity.assign("transparent");
    mBgColor = root["bg_color"].asInt();
    mBgOpacity = root["bg_opacity"].asString();
    if (mBgOpacity.compare("") == 0) mBgOpacity.assign("transparent");

    if (ver == CC_VER_CEA708) {
        mEdgeType = root["edge_type"].asString();
        if (mEdgeType.compare("") == 0) mEdgeType.assign("none");
        mEdgeColor = root["edge_color"].asInt();
    }


    mFontSize = mConfig->getScreen()->mMaxFontSize;// * mFontScale;
    mFgColor = convertCcColor(mFgColor);
    mBgColor = convertCcColor(mBgColor);
    mEdgeColor = convertCcColor(mEdgeColor);

//    1. Solid --> opacity = 100
//    2. Transparent --> opacity = 0
//    3. Translucent --> opacity = 50
//    4. flashing

    if (ignoreCaseCompare(mFgOpacity, "solid")) {
        mFgOpacityInt = 0xff;
    } else if (ignoreCaseCompare(mFgOpacity, "transparent")) {
        mFgOpacityInt = 0;
    } else if (ignoreCaseCompare(mFgOpacity, "translucent")) {
        mFgOpacityInt = 0x80;
    } else {
        ALOGE("Fg opacity Not supported yet %s", mFgOpacity.c_str());
    }

    /* --------------------Background----------------- */
    if (ignoreCaseCompare(mBgOpacity, "solid")) {
        mBgOpacityInt = 0xff;
    } else if (ignoreCaseCompare(mBgOpacity, "transparent")){
        mBgOpacityInt = 0x0;
    } else if (ignoreCaseCompare(mBgOpacity, "translucent")){
        mBgOpacityInt = 0x80;
    } else if (ignoreCaseCompare(mBgOpacity, "flash")) {
        mBgOpacityInt = 0xff;
    }

    updateDrawableConfig();

}

void RowString::updateDrawableConfig() {
    /* Convert font scale to a logical range */
    mFontSize = mFontSize * mFontScale;
    bool isMonospace = false;

    if (mFontStyle == "") {
        mFontStyle.assign("not set");
    }
    /* Typeface handle:
     * Temporarily leave caption manager's config, although it is lack of some characters
     * Now, only switch typeface for stream
     */

    mFontFace = FontManager::Instance().typeFaceFromName(mFontStyle, mItalics, mIsMultiByteStr);
    isMonospace = false;//FontManager::Instance().isMonoFont(mFontStyle);


    SkRect bounds;
    mPaint.setTypeface(mFontFace);
    mPaint.setTextSize(mFontSize);
    // mWindowPaint.setLetterSpacing((float) 0.05);
    mMaxSingleFontWidth = mPaint.measureText("_", strlen("_"), &bounds);
    if (mVersion == CC_VER_CEA708) {
        if (isMonospace) {
            mStringLengthOnPaint = (mData.length() + 1) * mMaxSingleFontWidth;
        } else {
            mStringLengthOnPaint = mPaint.measureText(mData.c_str(), mData.length(), &bounds) + mMaxSingleFontWidth;
        }
    } else {
        mStringLengthOnPaint = mPaint.measureText(mData.c_str(), mData.length(), &bounds) + mMaxSingleFontWidth;
    }


    /* Get a largest metric to get the baseline */
    mPaint.setTextSize(mConfig->getScreen()->mMaxFontHeight);
    mDecent = getFontMetricsInt(mPaint);

    /* Return to normal */
    mPaint.setTextSize((float)(mFontSize * mFontScale));
}

bool RowString::draw(SkCanvas &canvas, Window &win, Rows &row) {

    mEdgeWidth = mFontSize/15;//EDGE_SIZE_PERCENT;
    if (win.mPensizeWindowDepend == 0) {
        SkRect bounds;
        win.mPensizeWindowDepend = mPaint.measureText("H", strlen("H"), &bounds);
    }
    mStrCharactersCount = mData.length();

    //row space
    int rowSpace = 7;
    // Get This row boundary
    mStrTop = win.mWindowStartY + row.mRowNumberInWindow * mConfig->getScreen()->mMaxFontHeight + 2 * rowSpace;
    mStrBottom = win.mWindowStartY + (row.mRowNumberInWindow + 1) * mConfig->getScreen()->mMaxFontHeight + rowSpace;

    /* Handle mJustify here */
    ALOGD("%s[r:%d] mJustify=%s [T:%f B:%f] mMaxFontHeight:%f", __func__,
    row.mRowNumberInWindow, win.mJustify.c_str(), mStrTop, mStrBottom, mConfig->getScreen()->mMaxFontHeight);


    ALOGD("win.mWindowRight:%f, mStringLengthOnPaint:%f", win.mWindowRight, mStringLengthOnPaint);

    if (ignoreCaseCompare(win.mJustify, "left")) {
        if (row.mPriorStrPositionForDraw == -1) {
            row.mPriorStrPositionForDraw = (win.mWindowRight - mStringLengthOnPaint)/2;
        }
        ALOGD("%s mPriorStrPositionForDraw=%f winStartX:%f, strStartX:%f", __func__, row.mPriorStrPositionForDraw, win.mWindowStartX, mStrStartX);
        mStrLeft = row.mPriorStrPositionForDraw;
        mStrRight = mStrLeft + mStringLengthOnPaint;
        row.mPriorStrPositionForDraw = mStrRight;
    } else if (ignoreCaseCompare(win.mJustify, "right")) {
        if (row.mPriorStrPositionForDraw == -1) {
            row.mPriorStrPositionForDraw = win.mWindowStartX + win.mWindowWidth;
        }
        mStrRight = row.mPriorStrPositionForDraw;
        mStrLeft = mStrRight - mStringLengthOnPaint;
        row.mPriorStrPositionForDraw = mStrLeft;
    } else if (ignoreCaseCompare(win.mJustify, "full")) {
        if (row.mPriorStrPositionForDraw == -1) {
            row.mPriorStrPositionForDraw = win.mWindowStartX;
        }

        mStrLeft = row.mPriorStrPositionForDraw;
        mStrRight = mStrLeft + row.mCharacterGap * mStrCharactersCount;
        row.mPriorStrPositionForDraw = mStrRight;


    } else if (ignoreCaseCompare(win.mJustify, "center")) {
        if (row.mPriorStrPositionForDraw == -1) {
            row.mPriorStrPositionForDraw = (win.mWindowWidth - row.mRowLengthOnPaint)/2 + win.mWindowStartX;
        }
        mStrLeft = row.mPriorStrPositionForDraw;
        mStrRight = mStrLeft + mStringLengthOnPaint;
        row.mPriorStrPositionForDraw = mStrRight;
    } else {
        /* default using left justfication */
        if (row.mPriorStrPositionForDraw == -1) {
            row.mPriorStrPositionForDraw = win.mWindowStartX + mStrStartX;
        }
        mStrLeft = row.mPriorStrPositionForDraw;
        mStrRight = mStrLeft + mStringLengthOnPaint;
        row.mPriorStrPositionForDraw = mStrRight;
    }

    /* Draw background, a rect, if opacity == 0, skip it */
    if (mStrBottom < mConfig->getScreen()->mSafeTitleTop + mConfig->getScreen()->mMaxFontHeight) {
        ALOGD("FAILED: %f < %f (safe top:%f) strtop:%f", mStrBottom,
            mConfig->getScreen()->mSafeTitleTop + mConfig->getScreen()->mMaxFontHeight,
            mConfig->getScreen()->mSafeTitleTop, mStrTop);
        mStrBottom = mConfig->getScreen()->mSafeTitleTop + mConfig->getScreen()->mMaxFontHeight;
    }

    if (mStrTop < mConfig->getScreen()->mSafeTitleTop) {
        mStrTop = mConfig->getScreen()->mSafeTitleTop;
    }

    if (mBgOpacityInt != 0) {
        SkPaint paint;
        paint.setColor(mBgColor);
        paint.setAlpha(mBgOpacityInt);
        canvas.drawRect(SkRect::MakeLTRB(mStrLeft, mStrTop, mStrRight, mStrBottom), paint);
        ALOGD("Row String draw rect [%f %f %f %f] mFillColor:%x, mFileeOpcityInt:%x",
            mStrLeft, mStrTop, mStrRight, mStrBottom, mBgColor, mBgOpacityInt);
    }

    if (!ignoreCaseCompare(win.mJustify, "full")) {
        drawText(canvas, mData, mFontFace, mFontSize,
                (float) mStrLeft, (float) (mStrBottom ),
                mFgColor, mFgOpacity, mFgOpacityInt,
                mUnderline,
                mEdgeColor, (float) mEdgeWidth, mEdgeType);
    } else {
        double priorCharacterPosition = mStrLeft;
        for (int i=0; i < mData.length(); i++) {
            drawText(canvas, std::string("") + mData[i], mFontFace, mFontSize,
                    (float) priorCharacterPosition, (float) (mStrBottom ),
                    mFgColor, mFgOpacity, mFgOpacityInt,
                    mUnderline,
                    mEdgeColor, (float) mEdgeWidth, mEdgeType);

            priorCharacterPosition += row.mCharacterGap;
        }
    }

    ALOGD("   StrStartX:%f Rect[%f %f %f %f]",
         mStrStartX, mStrLeft, mStrTop, mStrRight, mStrBottom);

    return true;
}

void RowString::drawString(SkCanvas &canvas, std::string str, SkScalar left, SkScalar bottom, SkPaint &paint) {

    //paint.setBlendMode(SkBlendMode::kPlus);
    canvas.drawText(str.c_str(), str.length(), left, bottom, paint);

    ALOGD("RowString::drawString  %s at[%f %f] color[%08X] alpha:%08x",
        str.c_str(), left, bottom, paint.getColor(), paint.getAlpha());

}

void RowString::drawText(SkCanvas &canvas,  std::string str,
                       sk_sp<SkTypeface> face, double fontSize,
                       float left, float bottom, int fgColor, std::string opacity, int opacityInt,
                       bool underline, int edgeColor, float edgeWidth, std::string  edgeType) {

    if (ignoreCaseCompare(opacity, "flash")) {
        opacityInt = 0xff;
    }

    mPaint.setSubpixelText(true);
    mPaint.setTypeface(face);
    if (opacityInt != 0xff) {
        mPaint.setBlendMode(SkBlendMode::kSrc);
    }
    mPaint.setAntiAlias(true);
    mPaint.setTextSize((float)fontSize);

    bottom -= getFontMetricsDecent(mPaint);

    if (ignoreCaseCompare(mEdgeType, "none")) {
        mPaint.setColor(fgColor);
        mPaint.setAlpha(opacityInt);
        drawString(canvas, str, left, bottom, mPaint);
    } else if (ignoreCaseCompare(mEdgeType, "uniform")) {
        //paint.setStrokeJoin(Paint.Join.ROUND);
        mPaint.setStrokeWidth((float)(edgeWidth*1.5));
        mPaint.setColor(edgeColor);
        mPaint.setStyle(SkPaint::Style::kStroke_Style);
        drawString(canvas, str, left, bottom, mPaint);
        mPaint.setColor(fgColor);
        mPaint.setAlpha(opacityInt);
        mPaint.setStyle(SkPaint::Style::kFill_Style);
        drawString(canvas, str, left, bottom, mPaint);
    } else if (ignoreCaseCompare(mEdgeType, "shadow_right")) {
//TODO  paint.setShadowLayer((float)edgeWidth, (float) edgeWidth, (float) edgeWidth, edgeColor);
        mPaint.setColor(fgColor);
        mPaint.setAlpha(opacityInt);
        drawString(canvas, str, left, bottom, mPaint);
    } else if (ignoreCaseCompare(mEdgeType, "shadow_left")) {
//TODO  paint.setShadowLayer((float)edgeWidth, (float) -edgeWidth, (float) -edgeWidth, edgeColor);
        mPaint.setColor(fgColor);
        mPaint.setAlpha(opacityInt);
        drawString(canvas, str, left, bottom, mPaint);
    } else if (ignoreCaseCompare(edgeType, "raised") ||
            ignoreCaseCompare(edgeType, "depressed")) {
        bool raised;
        if (ignoreCaseCompare(edgeType, "depressed")) {
            raised = false;
        } else {
            raised = true;
        }
        int colorUp = raised ? fgColor : mEdgeColor;
        int colorDown = raised ? edgeColor : fgColor;
        float offset = (float)edgeWidth;
        mPaint.setColor(fgColor);
        mPaint.setStyle(SkPaint::Style::kFill_Style);
//TODO                paint.setShadowLayer(edgeWidth, -offset, -offset, colorUp);
        drawString(canvas, str, left, bottom, mPaint);
//TODO                paint.setShadowLayer(edgeWidth, offset, offset, colorDown);
        drawString(canvas, str, left, bottom, mPaint);
    } else if (ignoreCaseCompare(edgeType, "none")) {
        mPaint.setColor(fgColor);
        mPaint.setAlpha(opacityInt);
        drawString(canvas, str, left, bottom, mPaint);
    }

    if (underline) {
        mPaint.setStrokeWidth((float)(2.0));
        mPaint.setColor(fgColor);
        canvas.drawLine(left, (float)(bottom + edgeWidth * 2),
                (float) (left + mStringLengthOnPaint),
                (float)(bottom + edgeWidth * 2), mPaint);
    }
}

void RowString::dump(std :: string prefix) {
    if (mData.length() <= 0) return;
    ALOGD("%s ==> Subtitle: %s", prefix.c_str(), mData.c_str());
    ALOGD("%s   Italics:%d Underline:%d Color[Edge:%08x Fg:%08x, Bg:%08x]",
        prefix.c_str(), mItalics, mUnderline, mEdgeColor, mFgColor, mBgColor);
    ALOGD("%s   FontStyle:%s PenSize:%s offset:%s, edgeType:%s Opacity[fg:%s bg:%s]",
        prefix.c_str(), mFontStyle.c_str(), mPenSize.c_str(), mOffset.c_str(),
        mEdgeType.c_str(), mFgOpacity.c_str(), mBgOpacity.c_str());

    ALOGD("%s   face:%p FontSize:%f StrCharactersCount:%d StringLenOnPaint:%f MaxSingleFontWidth:%f",
        prefix.c_str(), mFontFace.get(), mFontSize, mStrCharactersCount, mStringLengthOnPaint, mMaxSingleFontWidth);
    ALOGD("%s   StrStartX:%f Rect[%f %f %f %f]",
        prefix.c_str(), mStrStartX, mStrLeft, mStrTop, mStrRight, mStrBottom);
}

Rows::Rows(CaptionVersion ver, std::shared_ptr<Configure> config, Json::Value &root) {
    mVersion = ver;
    mConfig = config;

    // init
    mPriorStrPositionForDraw = -1;

    mRowArray = root["content"];
    mRowStartX = root["row_start"].asInt();
    mStrCount = mRowArray.size();
    ALOGD_IF(gDebug, "Row: Row==> mRowStartX:%f,%s mStrCount:%d", mRowStartX, root["row_start"].asString().c_str(), mStrCount);

    double singleCharWidth = (mVersion==CC_VER_CEA708) ?
            mConfig->mWindowMaxFontSize : mConfig->getScreen()->mFixedCharWidth;

    for (int i=0; i<mStrCount; i++) {
        RowString row(ver, mConfig, mRowArray[i]);
        //Every string starts at prior string's tail
        row.updateRowStartX(mRowLengthOnPaint + mRowStartX * singleCharWidth);
        //row.mStrStartX = mRowLengthOnPaint + mRowStartX * singleCharWidth;
        mRowCharactersCount += row.getStringCharCount();//row.mStrCharactersCount;
        mRowLengthOnPaint += row.getStringLengthOnPaint();
        double strMaxFontSize = row.getMaxSingleFontWidth();
        mRowMaxFontSize = (strMaxFontSize > mRowMaxFontSize)
            ?strMaxFontSize:mRowMaxFontSize;

        mRowStrs.push_back(row);
    }
//    mCharacterGap = mWindowWidth/mRowCharactersCount;

}

bool Rows::draw(SkCanvas &canvas, Window &win) {
    // nothing need to draw in this row ...

    ALOGD("mRowLengthOnPaint=%f mStrCount:%d", mRowLengthOnPaint, mStrCount);
    if (/*mRowLengthOnPaint == 0 || */mStrCount == 0) {
        return false;
    }

    ALOGD("Rows::draw mRowStartX:%f", mRowStartX);

    Rows &r = *this;
    std::for_each(mRowStrs.begin(), mRowStrs.end(), [&](RowString &rs) {
        rs.draw(canvas, win, r);
    });
    return true;
}

void Rows::dump(std::string prefix) {
    if (mStrCount <= 0) {
        return;
    }

    ALOGD("%s RowNum:%d StringCount:%d mRowStartX:%f mRowStartY:%f",
        prefix.c_str(), mRowNumberInWindow, mStrCount, mRowStartX, mRowStartY);
    ALOGD("%s RowLengthOnPaint:%f RowCharactersCount:%d PriorStrPositionForDra:%f CharacterGap:%f mRowMaxFontSize:%f",
        prefix.c_str(), mRowLengthOnPaint, mRowCharactersCount, mPriorStrPositionForDraw, mCharacterGap, mRowMaxFontSize);

    std::for_each(mRowStrs.begin(), mRowStrs.end(), [&](RowString &rs) {
        rs.dump(prefix+"    ");
    });
}

Window::Window(CaptionVersion ver, std::shared_ptr<Configure> config, Json::Value &root) {
    mVersion = ver;
    mConfig = config;

    mAnchorPointer = root["anchor_point"].asInt();
    mAnchorV = root["anchor_vertical"].asInt();
    mAnchorH = root["anchor_horizontal"].asInt();
    mAnchorRelative = root["anchor_relative"].asBool();
    ALOGD_IF(gDebug, "Window: anchor_point=%d  mAnchor[%d %d] anchor_relative:%d",
        mAnchorPointer, mAnchorV, mAnchorH, mAnchorRelative);

    mRowCount = root["row_count"].asInt();
    mColCount = root["column_count"].asInt();
    mRowLock = root["row_lock"].asBool();
    mColoumnLock = root["column_lock"].asBool();
    ALOGD_IF(gDebug, "Window: Row==> row:%d (lock?%d)  column:%d (lock?%d)",
        mRowCount, mRowLock, mColCount, mColoumnLock);


    mJustify = root["justify"].asString();
    mPrintDirection = root["print_direction"].asString();
    mScrollDirection = root["scroll_direction"].asString();
    mWordwrap = root["wordwrap"].asBool();
    ALOGD_IF(gDebug, "Window: justify==> %s print_dir:%s scroll_dir:%s wordwrap?%d",
        mJustify.c_str(), mPrintDirection.c_str(), mScrollDirection.c_str(), mWordwrap);

    mDisplayEffect = root["display_effect"].asString();
    mEffectDirection = root["effect_direction"].asString();
    mEffectSpeed = root["effect_speed"].asInt();


    if (mVersion == CC_VER_CEA708) {
        mEffectPercent = root["effect_percent"].asInt();
        mEffectStatus = root["effect_status"].asString();
        if (mEffectPercent <= 0) {
            mEffectPercent = 100;
        }
    }

    ALOGD_IF(gDebug, "Window: effect==> display:%s direction:%s speed:%d percent:%d status:%s",
        mDisplayEffect.c_str(), mEffectDirection.c_str(), mEffectSpeed, mEffectPercent, mEffectStatus.c_str());

    mFillOpacity = root["fill_opacity"].asString();
    mFillColor = root["fill_color"].asInt();
    mBorderType = root["border_type"].asString();
    mBorderColor = root["border_color"].asInt();

        /* Value from stream need to be converted */
    mFillColor = convertCcColor(mFillColor);
    mBorderColor = convertCcColor(mBorderColor);

    if (ignoreCaseCompare(mFillOpacity, "solid")) {
        mFillOpcityInt = 0xff;
    } else if (ignoreCaseCompare(mFillOpacity, "transparent")){
        mFillOpcityInt = 0;
    } else if (ignoreCaseCompare(mFillOpacity, "translucent")){
        mFillOpcityInt = 0x80;
    } else if (ignoreCaseCompare(mFillOpacity, "flash")) {
        mFillOpcityInt = 0xff;
    } else {
        mFillOpcityInt = 0xff;
    }

    if ((mConfig->mDefaultFillOpacity & 0xFF) == 0) {
        // has valid default fill opacity, use this!
        mFillOpcityInt = mConfig->mDefaultFillOpacity;
    }

    ALOGD_IF(gDebug, "Window: fill==> mFillOpacity:%s mFillColor:%x mBorderType:%s mBorderColor:%x mFillOpcityInt:%x ",
        mFillOpacity.c_str(), mFillColor,  mBorderType.c_str(), mBorderColor, mFillOpcityInt);

    // initialze window
    mWindowWidth = 0;
    mWindowLeftMost = 10000;
    mWindowMaxFontSize = 0;
    mWindowEdgeWidth = (float)(mConfig->getScreen()->mMaxFontHeight * 0.15);//windowEdgeRate = 0.15;

    mWindowMaxFontSize = mConfig->getScreen()->mSafeTitleWidth / 32;
    // TODO: revise
    mConfig->mWindowMaxFontSize = mWindowMaxFontSize;
    mWindowWidth = mColCount * mWindowMaxFontSize;

    mRowJson = root["rows"];
    if (mRowCount > mRowJson.size()) {
        ALOGE("window loses %d rows!", (mRowCount - mRowJson.size()));
    }
    for (int i=0; i<mRowJson.size(); i++) {
        Rows row(mVersion, mConfig, mRowJson[i]);
        row.mRowNumberInWindow = i;
        // TODO: revise
        mRowLength = row.mRowLengthOnPaint;
        mWindowLeftMost = row.mRowStartX < mWindowLeftMost ?
                    row.mRowStartX : mWindowLeftMost;
        mRows.push_back(row);
    }

    // TODO: resolve the size
    mWindowLeftMost *= mPensizeWindowDepend;
    mWindowStartX = mConfig->getScreen()->getWindowLeftTopX(mAnchorRelative, mAnchorH, mAnchorPointer, mWindowWidth);

    mWindowStartY = mConfig->getScreen()->getWindowLeftTopY(mAnchorRelative, mAnchorV, mAnchorPointer, mRowCount);
}

void Window::updateValue(int winWidth, int winHeight) {

}
bool Window::draw(SkCanvas &canvas) {
    SkPaint paint;
    SkPaint fadePaint;
    SkPaint wipePaint;
    SkImageInfo info = canvas.imageInfo();
    updateValue(info.width(), info.height());

    if (mVersion == CC_VER_CEA708) {

        double columnsWidth;
        columnsWidth = mColCount * mConfig->getScreen()->mMaxFontWidth;

        mWindowLeft = 0;
        mWindowTop = 0;
        mWindowRight = info.width();
        mWindowButtom =   mConfig->getScreen()->mMaxFontHeight * mRowCount;

        paint.setBlendMode(SkBlendMode::kSrc);
        /* Draw border */
        /* Draw border color */
        drawBorder(canvas, paint, fadePaint, mBorderType,
                SkRect::MakeLTRB(mWindowLeft, mWindowTop, mWindowRight, mWindowButtom),
                mBorderColor);

        /* Draw window */
    } else {
        // TODO: we may get same safe region...
        // TODO: calculate boundray.
        mWindowLeft = 0;
        mWindowRight = info.width();
        mWindowTop = 0;
        mWindowButtom = info.height();
    }

    // already clear, no need do agin.
    if (mFillOpcityInt != 0x0) {
        paint.setColor(mFillColor);
        paint.setAlpha(mFillOpcityInt);
        ALOGD("Draw window rect [%f %f %f %f] mFillColor:%x, mFileeOpcityInt:%x",
            mWindowLeft, mWindowRight, mWindowTop, mWindowButtom, mFillColor, mFillOpcityInt);
        canvas.drawRect(SkRect::MakeLTRB(mWindowLeft, mWindowTop, mWindowRight, mWindowButtom), paint);
    }

    Window &win = *this;
    std::for_each(mRows.begin(), mRows.end(), [&](Rows &row) {
        row.draw(canvas, win);
    });

    if (mVersion == CC_VER_CEA708) {
        double rectLeft, rectRight, rectTop, rectBottom;
        float border = mConfig->getScreen()->mWindowBorderWidth;
        if (ignoreCaseCompare(mDisplayEffect, "fade")) {
            fadePaint.setColor(SK_ColorWHITE);
            fadePaint.setAlpha(mEffectPercent*255/100);
            fadePaint.setBlendMode(SkBlendMode::kScreen);
            canvas.drawRect(
                SkRect::MakeLTRB(mWindowLeft-border, mWindowTop-border, mWindowRight+border, mWindowButtom+border),
                fadePaint);

        } else if (ignoreCaseCompare(mDisplayEffect, "wipe")) {
            if (ignoreCaseCompare(mEffectDirection, "left_right")) {
                rectLeft = mWindowStartX - border;
                rectRight = mWindowStartX + mWindowWidth * mEffectPercent /100 + border;
                rectTop = mWindowTop - border;
                rectBottom = mWindowButtom + border;
            } else if (ignoreCaseCompare(mEffectDirection, "right_left")) {
                rectLeft = mWindowStartX + mWindowWidth * (100 - mEffectPercent)/100 - border;
                rectRight = mWindowStartX + mWindowWidth + border;
                rectTop = mWindowTop - border;
                rectBottom = mWindowButtom + border;
            } else if (ignoreCaseCompare(mEffectDirection, "top_bottom")) {
                rectLeft = mWindowStartX - border;
                rectRight = mWindowStartX + mWindowWidth + border;
                rectTop = mWindowTop - border;
                rectBottom = mWindowTop + (mWindowButtom - mWindowTop) * mEffectPercent/100 + border;
            } else if (ignoreCaseCompare(mEffectDirection, "bottom_top")) {
                rectLeft = mWindowStartX - border;
                rectRight = mWindowStartX + mWindowWidth + border;
                rectTop = mWindowTop + (mWindowButtom - mWindowTop) * (100 - mEffectPercent)/100 - border;
                rectBottom = mWindowButtom + border;
            } else {
                rectLeft = 0;
                rectBottom = 0;
                rectTop = 0;
                rectRight = 0;
            }
            wipePaint.setColor(SK_ColorWHITE);
            wipePaint.setAlpha(0);
            fadePaint.setBlendMode(SkBlendMode::kClear);
            canvas.drawRect(SkRect::MakeLTRB(rectLeft, rectTop, rectRight, rectBottom), wipePaint);
        }
    }


    return true;
}

//TODO how to handle these 2 paint?
void Window::drawBorder(SkCanvas &canvas, SkPaint borderPaint, SkPaint shadowPaint,
    std::string type, SkRect rect, int color) {

    SkPath path1;
    SkPath path2;

    float gap = 0.1f; //(mCaptionScreen.mWindowBorderWidth);

    shadowPaint.setColor(SK_ColorGRAY);
    shadowPaint.setAlpha(0x90); // half +

    if (ignoreCaseCompare(type, "none")) {
        // none do what?
    }

    if (ignoreCaseCompare(type, "raised") ||
        ignoreCaseCompare(type, "depressed")) {
        SkScalar og = (SkScalar)(gap * 0.6);

        borderPaint.setStyle(SkPaint::kFill_Style);
        borderPaint.setColor(color);

        if (ignoreCaseCompare(type, "raised")) {
            //Right
            path1.moveTo(rect.fRight - 1, rect.fTop - 1);
            path1.lineTo(rect.fRight + og, rect.fTop - og);
            path1.lineTo(rect.fRight + og, rect.fBottom - og);
            path1.lineTo(rect.fRight - 1, rect.fBottom);
            path1.close();

            //Left
            path2.moveTo(rect.fLeft + 1, rect.fBottom);
            path2.lineTo(rect.fLeft - og, rect.fBottom - og);
            path2.lineTo(rect.fLeft - og, rect.fTop - og);
            path2.lineTo(rect.fLeft + 1, rect.fTop);
            path2.close();

            canvas.drawPath(path1, borderPaint);
            canvas.drawPath(path2, borderPaint);
            //Top
            canvas.drawRect(SkRect::MakeLTRB(rect.fLeft-og, rect.fTop-og, rect.fRight+og, rect.fTop), borderPaint);
        } else if (ignoreCaseCompare(type, "depressed")) {
            //Right
            path1.moveTo(rect.fRight - 1, rect.fTop);
            path1.lineTo(rect.fRight + og, rect.fTop + og);
            path1.lineTo(rect.fRight + og, rect.fBottom + og);
            path1.lineTo(rect.fRight - 1, rect.fBottom);
            path1.close();

            //Left
            path2.moveTo(rect.fLeft + 1, rect.fBottom);
            path2.lineTo(rect.fLeft - og, rect.fBottom + og);
            path2.lineTo(rect.fLeft - og, rect.fTop + og);
            path2.lineTo(rect.fLeft + 1, rect.fTop);
            path2.close();

            canvas.drawPath(path1, borderPaint);
            canvas.drawPath(path2, borderPaint);
            canvas.drawRect(SkRect::MakeLTRB(rect.fLeft-og, rect.fBottom, rect.fRight+og, rect.fBottom+og), borderPaint);
        }
    }

    borderPaint.setColor(color);
    if (ignoreCaseCompare(type, "uniform")) {
        canvas.drawRect(SkRect::MakeLTRB(rect.fLeft-gap, rect.fTop-gap, rect.fRight+gap, rect.fBottom+gap), borderPaint);
    } else if (ignoreCaseCompare(type, "shadow_left")) {
        canvas.drawRect(SkRect::MakeLTRB(rect.fLeft-gap, rect.fTop+gap, rect.fRight-gap, rect.fBottom+gap), borderPaint);
    } else if (ignoreCaseCompare(type, "shadow_right")) {
        canvas.drawRect(SkRect::MakeLTRB(rect.fLeft+gap, rect.fTop+gap, rect.fRight+gap, rect.fBottom+gap), borderPaint);
    }
}

void Window::dump(std :: string prefix) {
    ALOGD("%s row:%d column:%d rowLock:%d colLock:%d", prefix.c_str(), mRowCount, mColCount, mRowLock, mColoumnLock);
    ALOGD("%s anchorPoint:%d AnchorVH[%d %d] AnchorRelative:%d",
        prefix.c_str(), mAnchorPointer, mAnchorV, mAnchorH, mAnchorRelative);
    ALOGD("%s Justify:%s direction:%s scroll_direction:%s mWordwrap:%d",
        prefix.c_str(), mJustify.c_str(), mPrintDirection.c_str(), mScrollDirection.c_str(), mWordwrap);
    ALOGD("%s fill[%s %08X] border[%s %08X] penWindowDepend:%f",
        prefix.c_str(), mFillOpacity.c_str(), mFillColor, mBorderType.c_str(), mBorderColor, mPensizeWindowDepend);
    ALOGD("%s Effect[%s Speed:%d direction:%s status:%s percent:%d]",
        prefix.c_str(), mDisplayEffect.c_str(), mEffectSpeed,mEffectDirection.c_str(), mEffectStatus.c_str(), mEffectPercent);


    ALOGD("%s [==Only Dump Has Valid Content Row==]", prefix.c_str());
    std::for_each(mRows.begin(), mRows.end(), [&](Rows &r) {
        r.dump(prefix+"    ");
    });
}

CloseCaption::CloseCaption(std::shared_ptr<Configure> config) {
    mConfig = config;
}

bool CloseCaption::parserJson(const char*str) {
    std::string json(str);

    Json::Reader reader;
    Json::Value root;
    reader.parse(json, root);

    if (0) {
        std::string ds(str);
        while (ds.length() >= 512) {
            std::string tmp = ds.substr(0, 512);
            ds = ds.substr(512);
            ALOGD("%s", tmp.c_str());
        }
        ALOGD("%s", ds.c_str());
    }

    mVerString = root["type"].asString();
    if (mVerString == "cea608") {
        mVersion = CC_VER_CEA608;
    } else if (mVerString == "cea708") {
        mVersion = CC_VER_CEA708;
    } else {
        ALOGE("Error! not supported CloseCaption Version:%s", mVerString.c_str());
    }

    Json::Value windowRoots = root["windows"];

    ALOGD_IF(gDebug, "CloseCaption: type=%s windows[%d]", mVerString.c_str(), windowRoots.size());

    for (int i=0; i<windowRoots.size(); i++) {
        Window w(mVersion, mConfig, windowRoots[i]);
        mWindows.push_back(w);
    }

    return false;
}

bool CloseCaption::draw(SkCanvas &canvas) {
    canvas.clear(0x00000000);

    if (0) {
        SkPaint paint;
        // Text
        paint.setARGB(255, 255, 255, 255);
        paint.setTextSize(36);
        paint.setAntiAlias(true);
        paint.setSubpixelText(true);
        paint.setLCDRenderText(true);
        paint.setTypeface(FontManager::Instance().typeFaceFromName("mono_serif", false));

        SkTextBox box;
        SkImageInfo info = canvas.imageInfo();
        box.setBox(0, 0, info.width(), info.height());
        box.setSpacingAlign(SkTextBox::SpacingAlign::kEnd_SpacingAlign);
        char data[256] = "abcdefghijklmkopqrstuvwxmzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890`~!@#$%^&*()_+-=,./;'[]\\<>?:\"{}|";
        box.setText(data, strlen(data), paint);
        box.draw(&canvas);

        paint.setTypeface(FontManager::Instance().typeFaceFromName("sans-serif", false));
        box.setSpacingAlign(SkTextBox::SpacingAlign::kStart_SpacingAlign);
        box.setText(data, strlen(data), paint);
        box.draw(&canvas);

    }

    if (mConfig == nullptr || mConfig->getScreen() == nullptr) {
         ALOGE("Error! no configuration defined");
    } else {
        SkImageInfo info = canvas.imageInfo();
        mConfig->getScreen()->updateScreen(info.width(), info.height());
    }
    ALOGD("|         ================ DRAW START ====================");
    std::for_each(mWindows.begin(), mWindows.end(), [&](Window &w) {
        w.draw(canvas);
    });
    ALOGD("|         ================  DRAW END  ====================");
    if (gDump) dump();
    return true;
}

void CloseCaption::dump() {
    std::string prefix("    ");

    ALOGD("CloseCation Item: version:%s", mVerString.c_str());

    std::for_each(mWindows.begin(), mWindows.end(), [&](Window &w) {
        w.dump(prefix);
    });

}

} // namespace CloseCaption
} // namespace NativeRender
} //namespace Amlogic

