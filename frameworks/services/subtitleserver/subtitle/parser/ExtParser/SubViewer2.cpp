#define LOG_TAG "SubViewer2"

#include "SubViewer2.h"

SubViewer2::SubViewer2(std::shared_ptr<DataSource> source): TextSubtitle(source) {
}

SubViewer2::~SubViewer2() {
}

/***************
* VobSub (paired with an .IDX file)--binary format.
* DVDSubtitle format:
{HEAD
DISCID=
DVDTITLE=Disney's Dinosaur
CODEPAGE=1250
FORMAT=ASCII
LANG=English
TITLE=1
ORIGINAL=ORIGINAL
AUTHOR=McPoodle
WEB=http://www.geocities.com/mcpoodle43/subs/
INFO=Extended Edition
LICENSE=
}
{T 00:00:50:28 This is the Earth at a time when the dinosaurs roamed...}
{T 00:00:54:08 a lush and fertile planet.}
****/
std::shared_ptr<ExtSubItem> SubViewer2::decodedItem() {
    char line[LINE_LEN + 1];
    char text[LINE_LEN + 1];
    int a1, a2, a3, a4;

    while (mReader->getLine(line)) {
        if (sscanf(line, "{T %d:%d:%d:%d %[^\n\r]", &a1, &a2, &a3, &a4, text) < 5) {
            continue;
        }

        std::shared_ptr<ExtSubItem> item = std::shared_ptr<ExtSubItem>(new ExtSubItem());
        item->start = a1 * 360000 + a2 * 6000 + a3 * 100 + a4 / 10;
        item->end = item->start + 200;
        item->lines.push_back(std::string(text));

        // TODO: multi line support
        return item;
    }

    return nullptr;
}


