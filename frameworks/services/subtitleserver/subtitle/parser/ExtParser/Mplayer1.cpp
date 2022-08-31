#define LOG_TAG "Mplayer1"

#include "Mplayer1.h"

Mplayer1::Mplayer1(std::shared_ptr<DataSource> source): TextSubtitle(source) {
    // default rate
    mPtsRate = 15;
}

Mplayer1::~Mplayer1() {
}

std::shared_ptr<ExtSubItem> Mplayer1::decodedItem() {
    char *line = (char *)MALLOC(LINE_LEN+1);
    char *line2 = (char *)MALLOC(LINE_LEN);
    memset(line, 0, LINE_LEN+1);
    memset(line2, 0, LINE_LEN);
    while (mReader->getLine(line)) {
        int start =0, end = 0, tmp;
        ALOGD(" read: %s", line);
        if (sscanf(line, "%d,%d,%d,%[^\r\n]", &start, &end, &tmp, line2) < 4) {
                continue;
        }

        if (start == 1) {
            if (atoi(line2) > 0) {
                mPtsRate = atoi(line2);
            }
            continue;
        }

        std::shared_ptr<ExtSubItem> item = std::shared_ptr<ExtSubItem>(new ExtSubItem());
        item->start = start*100/mPtsRate;
        item->end = end*100/mPtsRate;
        std::string s(line2);
        item->lines.push_back(s);
        free(line);
        free(line2);
        return item;
    }
    free(line);
    free(line2);
    return nullptr;
}


