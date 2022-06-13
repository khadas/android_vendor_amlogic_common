#define LOG_TAG "SubViewer"

#include "SubViewer.h"

SubViewer::SubViewer(std::shared_ptr<DataSource> source): TextSubtitle(source) {
}

SubViewer::~SubViewer() {

}


std::shared_ptr<ExtSubItem> SubViewer::decodedItem() {
    char line[LINE_LEN + 1];
    char *p = NULL;
    int i, len;

    while (mReader->getLine(line)) {

        std::shared_ptr<ExtSubItem> item = std::shared_ptr<ExtSubItem>(new ExtSubItem());

        int a1, a2, a3, a4, b1, b2, b3, b4;
        if (sscanf(line, "%d:%d:%d%[,.:]%d --> %d:%d:%d%[,.:]%d",
                 &a1, &a2, &a3, (char *)&i, &a4, &b1, &b2, &b3, (char *)&i, &b4) < 10) {
            continue;
        }

        item->start = a1 * 360000 + a2 * 6000 + a3 * 100 + a4 / 10;
        item->end = b1 * 360000 + b2 * 6000 + b3 * 100 + b4 / 10;

        bool isNewLine = false;
        do {
            if (!mReader->getLine(line)) {
                break;
            }

            isNewLine = isEmptyLine(line);
            if (!isNewLine) {
                item->lines.push_back(std::string(line));
            }
        } while (!isNewLine);

        return item;
    }
    return nullptr;
}
