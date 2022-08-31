#define LOG_TAG "Mircodvd"

#include "Mircodvd.h"

Mircodvd::Mircodvd(std::shared_ptr<DataSource> source): TextSubtitle(source) {
    // default rate
    mPtsRate = 15;
}

Mircodvd::~Mircodvd() {
}

std::shared_ptr<ExtSubItem> Mircodvd::decodedItem() {
    char *line = (char *)MALLOC(LINE_LEN+1);
    char *line2 = (char *)MALLOC(LINE_LEN);
    memset(line, 0, LINE_LEN+1);
    memset(line2, 0, LINE_LEN);
    while (mReader->getLine(line)) {
        int start =0, end = 0;
        if (sscanf (line, "{%d}{%d}%[^\r\n]", &start, &end, line2) < 3) {
            if (sscanf(line, "{%d}{}%[^\r\n]", &start, line2) < 2) {
                continue;
            }
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

