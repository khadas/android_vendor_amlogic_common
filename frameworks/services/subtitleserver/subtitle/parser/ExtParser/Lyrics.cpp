#define LOG_TAG "Lyrics"

#include "Lyrics.h"

Lyrics::Lyrics(std::shared_ptr<DataSource> source): TextSubtitle(source) {
    mBuffer = new char[LINE_LEN + 1]();
    mReuseBuffer = false;
}

Lyrics::~Lyrics() {
    delete[] mBuffer;
}

std::shared_ptr<ExtSubItem> Lyrics::decodedItem() {
    int a1, a2, a3;
    char text[LINE_LEN + 1];
    int pattenLen;

    while (true) {
        if (!mReuseBuffer) {
            if (mReader->getLine(mBuffer) == nullptr) {
                return nullptr;
            }
        }

        // parse start and text
        if (sscanf(mBuffer, "[%d:%d.%d]%[^\n\r]", &a1, &a2, &a3, text) < 4) {
            mReuseBuffer = false;
            // fail, check again.
            continue;
        }

        std::shared_ptr<ExtSubItem> item = std::shared_ptr<ExtSubItem>(new ExtSubItem());
        item->start = a1 * 6000 + a2 * 100 + a3;
        item->end = item->start + 200;
        item->lines.push_back(std::string(text));

        // get time End, maybe has end time, maybe not, handle this case.
        if (mReader->getLine(mBuffer) == nullptr) {
            return item;
        }
        // has end??
        pattenLen = sscanf(mBuffer, "[%d:%d.%d]%[^\n\r]", &a1, &a2, &a3, text);
        if (pattenLen == 4) {
            mReuseBuffer = true;
            return item;
        } else if (pattenLen == 3) {
            item->end = a1 * 6000 + a2 * 100 + a3;
        }

        mReuseBuffer = false;
        return item;
    }
    return nullptr;
}

