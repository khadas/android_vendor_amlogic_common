#pragma once
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string>
#include <list>

#include <SkTypeface.h>
#include "utils_funcs.h"


namespace Amlogic {
namespace NativeRender {
namespace CloseCaption {

class FontManager {
public:
    FontManager();
    sk_sp<SkTypeface> typeFaceFromName(std::string typeName, bool isIitalic, bool isMutlByte = false);
    bool isMonoFont(std::string font);
    static FontManager& Instance();

private:
    static FontManager _instance;
    bool uncryptFontTo(const char*srcPath, const char*destPath);
    bool initFontResource();
    sk_sp<SkTypeface> getFallbackFonts(std::string typeName, bool isIitalic);

    sk_sp<SkTypeface> mMonoSerifTf;
    sk_sp<SkTypeface> mMonoSerifItTf;
    sk_sp<SkTypeface> mCasualTf;
    sk_sp<SkTypeface> mCasualItTf;
    sk_sp<SkTypeface> mPropSansTf;
    sk_sp<SkTypeface> mPropSansItTf;
    sk_sp<SkTypeface> mSmallCapitalTf;
    sk_sp<SkTypeface> mSmallCapitalItTf;
    sk_sp<SkTypeface> mCursiveTf;
    sk_sp<SkTypeface> mCursiveItTf;

    sk_sp<SkTypeface> mFallbackCJK;
};

} // namespace CloseCaption
} // namespace NativeRender
} //namespace Amlogic

