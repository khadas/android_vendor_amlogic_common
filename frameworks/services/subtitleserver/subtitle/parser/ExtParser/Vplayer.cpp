#define LOG_TAG "Vplayer"

#include "Vplayer.h"

// Parse similar as Lyrics.
Vplayer::Vplayer(std::shared_ptr<DataSource> source): TextSubtitle(source) {
    mBuffer = new char[LINE_LEN + 1]();
    mReuseBuffer = false;
}

Vplayer::~Vplayer() {
    delete[] mBuffer;
}
std::shared_ptr<ExtSubItem> Vplayer::decodedItem() {
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
        if (sscanf(mBuffer, "%d:%d:%d:%[^\n\r]", &a1, &a2, &a3, text) < 4) {
            mReuseBuffer = false;
            // fail, check again.
            continue;
        }

        std::shared_ptr<ExtSubItem> item = std::shared_ptr<ExtSubItem>(new ExtSubItem());
        item->start =  a1 * 360000 + a2 * 6000 + a3 * 100;
        item->end = item->start + 200;
        item->lines.push_back(std::string(text));

        // get time End, maybe has end time, maybe not, handle this case.
        if (mReader->getLine(mBuffer) == nullptr) {
            return item;
        }
        // has end??
        pattenLen = sscanf(mBuffer, "%d:%d:%d:%[^\n\r]", &a1, &a2, &a3, text);
        if (pattenLen == 4) {
            mReuseBuffer = true;
            return item;
        } else if (pattenLen == 3) {
            item->end = a1 * 360000 + a2 * 6000 + a3 * 100;
        }

        mReuseBuffer = false;
        return item;
    }
    return nullptr;
}

