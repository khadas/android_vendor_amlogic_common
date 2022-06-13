#define LOG_TAG "SubViewer3"

#include "SubViewer3.h"

SubViewer3::SubViewer3(std::shared_ptr<DataSource> source): TextSubtitle(source) {
}

SubViewer3::~SubViewer3() {
}

std::shared_ptr<ExtSubItem> SubViewer3::decodedItem() {
    char line[LINE_LEN + 1];
    while (mReader->getLine(line)) {
        std::shared_ptr<ExtSubItem> item = std::shared_ptr<ExtSubItem>(new ExtSubItem());

        int a0, a1, a2, a3, a4, b1, b2, b3, b4;
        if (sscanf(line, "%d  %d:%d:%d,%d  %d:%d:%d,%d",
                   &a0, &a1, &a2, &a3, &a4, &b1, &b2, &b3, &b4) < 9) {
            continue;
        }

        item->start = a1 * 360000 + a2 * 6000 + a3 * 100 + a4;
        item->end = b1 * 360000 + b2 * 6000 + b3 * 100 + b4;

        while (mReader->getLine(line)) {
            if (isEmptyLine(line)) {
                break;
            }

            std::string sub(line);
            //TODO: post process sub

            item->lines.push_back(std::string(line));
            return item;
        }
    }
    return nullptr;
}

