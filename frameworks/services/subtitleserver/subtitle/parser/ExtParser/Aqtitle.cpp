#define LOG_TAG "Aqtitle"

#include "Aqtitle.h"

Aqtitle::Aqtitle(std::shared_ptr<DataSource> source) : TextSubtitle(source) {
    ALOGE("Aqtitle Subtitle");
}


Aqtitle::~Aqtitle() {

}

/**
**************************************************************
-->> 000004
Don't move

-->> 000028


-->> 000120
Look at that

-->> 000150


-->> 000153
Those are kids.

-->> 000182

*************************************************************

*/


std::shared_ptr<ExtSubItem> Aqtitle::decodedItem() {
    char line[LINE_LEN + 1] = {0};
    int startPts = 0;
    int endPts = 0;

    while (mReader->getLine(line)) {

        //ALOGD("%d %d %d", __LINE__, strlen(line), line[strlen(line)-1]);
        //ALOGD("%d %s is EmptyLine?%d", __LINE__, line, isEmptyLine(line));
        if (sscanf(line, "-->> %d", &startPts) < 1) {
            continue;
        }

        do {
            if (!mReader->getLine(line)) {
                return nullptr;
            }
        } while (strlen(line) == 0);

        if (isEmptyLine(line)) {
            ALOGD("?%d %d %d %d [%d]? why new line??", __LINE__, line[0], line[1], line[2], strlen(line));
            continue;
        }
        // Got Start Pts, Got Subtitle String...
        std::string sub(line);
        // poll all subtitles
        while (mReader->getLine(line)) {
            if (isEmptyLine(line)) {
                break;
            }
            sub.append(line);
        }

        do {
            if (!mReader->getLine(line)) {
                return nullptr;
            }
        } while (isEmptyLine(line));

        if (sscanf(line, "-->> %d", &endPts) < 1) {
            // invalid. try next
            continue;
        }

        std::shared_ptr<ExtSubItem> item = std::shared_ptr<ExtSubItem>(new ExtSubItem());
        item ->start = startPts;
        item ->end = endPts;
        item->lines.push_back(sub);
        ALOGD("Add Item: %d:%d %s", startPts, endPts, sub.c_str());
        return item;
    }

    return nullptr;
}
