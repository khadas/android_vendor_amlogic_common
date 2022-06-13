#define LOG_TAG "WebVtt"
#include <regex>

#include "WebVtt.h"



SimpleWebVtt::SimpleWebVtt(std::shared_ptr<DataSource> source): TextSubtitle(source) {
    ALOGE("SimpleWebVtt Subtitle");
}

SimpleWebVtt::~SimpleWebVtt() {
}

/**
********************* Web VTT as bellow ***********************
WEBVTT

00:01.000 --> 00:04.000
Never drink liquid nitrogen.

00:05.000 --> 00:09.000
- It will perforate your stomach.
- You could die.

NOTE This is the last line in the file
***************************************************************
*/
std::shared_ptr<ExtSubItem> SimpleWebVtt::decodedItem() {
    char line[LINE_LEN + 1];
    int len = 0;

    while (mReader->getLine(line)) {

        int a1=0, a2=0, a3=0, a4=0, b1=0, b2=0, b3=0, b4=0;
        if ((len = sscanf(line, "%d:%d:%d.%d --> %d:%d:%d.%d", &a1, &a2, &a3, &a4, &b1, &b2, &b3, &b4)) < 8) {
            if ((len = sscanf(line, "%d:%d.%d --> %d:%d.%d",
                &a2, &a3, &a4,  &b2, &b3, &b4)) < 6) {
                continue;
            }
        }

        std::shared_ptr<ExtSubItem> item = std::shared_ptr<ExtSubItem>(new ExtSubItem());
        item->start = a1 * 360000 + a2 * 6000 + a3 * 100 + a4 / 10;
        item->end = b1 * 360000 + b2 * 6000 + b3 * 100 + b4 / 10;

        for (int i = 0; i < SUB_MAX_TEXT;) {
            // next line, is subtitle
            if (!mReader->getLine(line)) {
                return NULL;
            }

            char *p = NULL;
            len = 0;
            for (p = line; *p != '\n' && *p != '\r' && *p;
                    p++, len++);

            if (len) {
                int skip = 0;

                std::string s;
                p = line;
                if (len >2 && line[0] == '-' && line[1] == ' ') {
                    p += 2;
                    len -= 2;
                }
                s.append(p, len);
                removeHtmlToken(s);
                item->lines.push_back(s);
            } else {
                break;
            }
        }
        return item;
    }
    return nullptr;
}

