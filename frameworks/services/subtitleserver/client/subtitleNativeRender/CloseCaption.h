#pragma once
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string>
#include <list>

#include <SkCanvas.h>
#include <SkPaint.h>
#include <SkTypeface.h>
#include <json/json.h>
#include "CloseCaptionFont.h"

#include "utils_funcs.h"
namespace Amlogic {
namespace NativeRender {
namespace CloseCaption {



//////// TODO: currently, we all use the global singleton.
//////// move to normal use later
class CloseCaption;
class Window;
class Rows;

enum CaptionVersion {
    CC_VER_CEA608,
    CC_VER_CEA708,
};

// This normally should got from java CaptioningManager
// However,
struct Settings {
    float mFontScale;
    bool mIsEnabled;
    sk_sp<SkTypeface> mTypeFace;
    bool mHasBackgroundColor;
    bool mHasEdgeColor;
    bool mHasEdgeType;
    bool mHasForegroundColor;
    bool mHasWindowColor;

    int mForegroundColor;
    int mForegroundOpacity;
    int mWindowColor;
    int mWindowOpcity;
    int mBackgroundColor;
    int mBackgroundOpcity;
    int mEdgeColor;
    int mEdgeType;
    int mStrockeWidth;

};

// TODO: integrate to window.

struct CaptionScreen {
    int mWidth;
    int mHeight;


    double mSafeTitleLeft;
    double mSafeTitleRight;
    double mSafeTitleTop;
    double mSafeTitleButton;
    double mSafeTitleWidth;
    double mSafeTitleHeight;
    double mSafeTitleWPercent;
    double mSafeTitleHPercent;

    int mVideoLeft;
    int mVideoRight;
    int mVideoTop;
    int mVideoBottom;
    int mVideoHeight;
    int mVideoWidth;

    double mVideohvRateOnScreen;
    int mVideohvRateOrigin;
    double mHvRate;

    double mScreenLeft;
    double mScreenRight;

    int mAnchorVertical;
    int mAnchorHorizon;
    double mAnchorVerticalDensity;
    double mAnchorHorizonDensity;
    double mMaxFontHeight;
    double mMaxFontWidth;
    double mMaxFontSize;
    double mFixedCharWidth;
    float mWindowBorderWidth;
    int mCcRowCount;
    int mCcColCount;

    CaptionScreen() {
        memset(this, 0, sizeof(*this));
        mSafeTitleWPercent = 0.98;
        mSafeTitleHPercent = 0.98;

        mCcRowCount = 15;
        mCcColCount = 32;
    }
    void updateScreen(int w, int h);
    double getWindowLeftTopX(
            bool anchorRelative, int anchorH, int anchorPoint, double rowLength);
    double getWindowLeftTopY(bool anchorRelative, int anchorV, int anchorPoint, int rowCount);

    //static CaptionScreen& Instance();

    //private:
    //    static CaptionScreen _instance;
};

class Configure {
public:
    Configure();
    ~Configure() {}
    void constructFontManager() {
        // TODO: no need this
        //mFontManager = std::shared_ptr<FontManager>(&FontManager::Instance());
    }
    void constructCaptionScreen() {
        mCaptionScreen = std::shared_ptr<CaptionScreen>(new CaptionScreen());
    }

    std::shared_ptr<FontManager> getFontManager() {
        return mFontManager;
    }

    std::shared_ptr<CaptionScreen> getScreen() {
        return mCaptionScreen;
    }
    double mWindowMaxFontSize;
    int mDefaultFillOpacity;

private:
    std::shared_ptr<FontManager> mFontManager;
    std::shared_ptr<CaptionScreen> mCaptionScreen;
};

class RowString {
public:
    RowString(CaptionVersion ver, std::shared_ptr<Configure> config, Json::Value &root);
    bool draw(SkCanvas &canvas, Window &win, Rows &r);
    void dump(std::string prefix);

    void updateRowStartX(double x) {mStrStartX = x;}
    int getStringCharCount(){return mStrCharactersCount;}
    double getStringLengthOnPaint() {return mStringLengthOnPaint;}
    double getMaxSingleFontWidth() {return mMaxSingleFontWidth; }

private:
    CaptionVersion mVersion;
    /* For parse json use */
    bool mItalics;
    bool mUnderline;
    int mEdgeColor;
    int mFgColor;
    int mBgColor;
    std::string mPenSize;
    std::string mFontStyle;
    std::string mOffset;
    std::string mEdgeType;
    std::string mFgOpacity;
    std::string mBgOpacity;
    std::string mData;
    bool mIsMultiByteStr;
    double mStringLengthOnPaint;

    /* TODO: maybe there is more efficient way to do this */
    double mMaxSingleFontWidth;
    double mStrStartX;
    double mStrLeft;
    double mStrTop;
    double mStrRight;
    double mStrBottom;
    double mFontSize;
    int mStrCharactersCount;

    int mDecent;

//TODO:    Paint.FontMetricsInt mFontMetrics;

    /* below is the actual parameters we used */
    int mFgOpacityInt = 0xff;
    int mBgOpacityInt = 0xff;
    double mFontScale;
    sk_sp<SkTypeface> mFontFace;
    double mEdgeWidth;
    bool mUseCaptionManagerStyle;

private:
    void drawString(SkCanvas &canvas, std::string str, SkScalar left, SkScalar bottom, SkPaint &paint);
    void drawText(SkCanvas &canvas,  std::string str,
                       sk_sp<SkTypeface> face, double fontSize,
                       float left, float bottom, int fgColor, std::string opacity, int opacityInt,
                       bool underline, int edgeColor, float edgeWidth, std::string  edgeType);
    void updateDrawableConfig();
    SkPaint mPaint;
    std::shared_ptr<Configure> mConfig;
};

class Rows {
public:
    Rows(CaptionVersion ver, std::shared_ptr<Configure> config, Json::Value &root);
    bool draw(SkCanvas &canvas, Window &win);
    void dump(std::string prefix);

//private:
    CaptionVersion mVersion;
    int mStrCount;//mStrCount;
    //RowStr mRowStrs[];//mRowStrs[];
    //JSONArray mRowArray;//mRowArray;
    Json::Value mRowArray;
    /* Row length is sum of each string */
    double mRowLengthOnPaint;//mRowLengthOnPaint;
    double mRowStartX;//mRowStartX;
    double mRowStartY;//mRowStartY;
    int mRowNumberInWindow;//mRowNumberInWindow;
    int mRowCharactersCount;//mRowCharactersCount;
    double mPriorStrPositionForDraw;//mPriorStrPositionForDraw;
    /* This is for full justification use */
    double mCharacterGap;//mCharacterGap;
    double mRowMaxFontSize;//mRowMaxFontSize;


    std::list<RowString> mRowStrs;
    private:
        std::shared_ptr<Configure> mConfig;

};


class Window {
public:
    Window(CaptionVersion ver, std::shared_ptr<Configure> config, Json::Value &root);
    bool draw(SkCanvas &canvas);
    void dump(std::string prefix);

//private:
    void drawBorder(SkCanvas &canvas, SkPaint borderPaint, SkPaint shadowPaint,
        std::string type, SkRect rect, int color);

    void updateValue(int winWidth, int winHeight);

    CaptionVersion mVersion;
    int mAnchorPointer;
    int mAnchorV;
    int mAnchorH;
    bool mAnchorRelative;
    int mRowCount;
    int mColCount;

    bool mRowLock;
    bool mColoumnLock;
    std::string mJustify;
    std::string mPrintDirection;
    std::string mScrollDirection;
    bool mWordwrap;
    int mEffectSpeed;
    std::string mFillOpacity;
    int mFillColor;
    std::string mBorderType;
    int mBorderColor;
    double mPensizeWindowDepend;

    std::string mDisplayEffect;
    std::string mEffectDirection;
    std::string mEffectStatus;
    int mEffectPercent;

    //final double mWindowEdgeRate = 0.15;
    double mWindowEdgeWidth;

    double mWindowWidth;
    double mWindowLeftMost;
    double mWindowStartX;
    double mWindowStartY;

    SkScalar mWindowLeft;
    SkScalar mWindowTop;
    SkScalar mWindowButtom;
    SkScalar mWindowRight;
    SkScalar mRowLength;
    int mHeartBeat;

    double mWindowMaxFontSize;

    int mFillOpcityInt;
    Json::Value mRowJson;
    std::list<Rows> mRows;

private:
    std::shared_ptr<Configure> mConfig;
};

class CloseCaption {
public:
    CloseCaption(std::shared_ptr<Configure> config);
    bool parserJson(const char*str);
    void dump();
    bool draw(SkCanvas &canvas);

private:
    std::shared_ptr<Configure> mConfig;
    CaptionVersion mVersion;
    std::string mVerString;
    std::list<Window> mWindows;

};

} // namespace CloseCaption
} // namespace NativeRender
} //namespace Amlogic
