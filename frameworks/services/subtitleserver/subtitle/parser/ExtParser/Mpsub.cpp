#define LOG_TAG "Mpsub"

#include "Mpsub.h"

Mpsub::Mpsub(std::shared_ptr<DataSource> source): TextSubtitle(source) {
    mPosition = 0.0f;
}

Mpsub::~Mpsub() {
}

std::shared_ptr<ExtSubItem> Mpsub::decodedItem() {
    char line[LINE_LEN+1];
    float a, b;

    while (mReader->getLine(line)) {
        ALOGD(" read: %s", line);
        if (sscanf(line, "%f %f", &a, &b) != 2) {
                continue;
        }

        std::shared_ptr<ExtSubItem> item = std::shared_ptr<ExtSubItem>(new ExtSubItem());

        mPosition += a*100;
        item->start = mPosition;
        mPosition += b*100;
        item->end = mPosition;

        while (mReader->getLine(line)) {
            if (isEmptyLine(line)) break;

            std::string s(line);
            item->lines.push_back(s);
        }

        return item;
    }
    return nullptr;
}


