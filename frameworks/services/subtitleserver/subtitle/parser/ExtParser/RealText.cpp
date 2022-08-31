#define LOG_TAG "RealText"

#include "RealText.h"

RealText::RealText(std::shared_ptr<DataSource> source): TextSubtitle(source) {
}

RealText::~RealText() {
}

/**
 *    http://service.real.com/help/library/guides/realtext/realtext.htm
 *
 <window width="320" height="140">
 Countdown starts in five seconds...<br/>
 <time begin="3"/>Get ready!<br/>
 <time begin="5"/>Five...<br/>
 <time begin="6"/>... four... <br/>
 <time begin="7"/>... three...<br/>
 <time begin="8"/>... two... <br/>
 <time begin="9"/>... one...<br/>
 <time begin="10"/>Launching the Battle Squadron!
 </window>
 *
 */
std::shared_ptr<ExtSubItem> RealText::decodedItem() {
    char line[LINE_LEN + 1];

    while (mReader->getLine(line)) {
        ALOGD(" read: %s", line);
        int plen = 0;
        int a1=0, a2=0, a3=0, a4=0, b1=0, b2=0, b3=0, b4=0;

        if ((sscanf(line, "<%*[tT]ime %*[bB]egin=\"%d:%d\" %*[Ee]nd=\"%d:%d\"%*[^<]<clear/>%n", &a2, &a3, &b2, &b3, &plen) < 4) &&
            (sscanf(line, "<%*[tT]ime %*[bB]egin=\"%d:%d\" %*[Ee]nd=\"%d:%d.%d\"%*[^<]<clear/>%n", &a2,&a3,&b2,&b3,&b4,&plen) < 5) &&
            (sscanf(line, "<%*[tT]ime %*[bB]egin=\"%d.%d\" %*[Ee]nd=\"%d.%d\"%*[^<]<clear/>%n", &a3, &a4, &b3, &b4, &plen) < 4) &&
            (sscanf(line, "<%*[tT]ime %*[bB]egin=\"%d.%d\" %*[Ee]nd=\"%d:%d.%d\"%*[^<]<clear/>%n", &a3, &a4, &b2, &b3, &b4, &plen) < 5) &&
            (sscanf(line, "<%*[tT]ime %*[bB]egin=\"%d:%d.%d\" %*[Ee]nd=\"%d:%d.%d\"%*[^<]<clear/>%n", &a2,&a3,&a4,&b2,&b3,&b4,&plen) < 6) &&
            (sscanf(line, "<%*[tT]ime %*[bB]egin=\"%d:%d:%d.%d\" %*[Ee]nd=\"%d:%d:%d.%d\"%*[^<]<clear/>%n", &a1,&a2,&a3,&a4,&b1,&b2,&b3,&b4,&plen) < 8)
        ) {
            // try without end
            if ((sscanf(line, "<%*[tT]ime %*[bB]egin=\"%d.%d\"%*[^<]<clear/>%n", &a3, &a4, &plen) < 2) &&
                (sscanf(line, "<%*[tT]ime %*[bB]egin=\"%d:%d\"%*[^<]<clear/>%n", &a2, &a3, &plen) < 2) &&
                (sscanf(line, "<%*[tT]ime %*[bB]egin=\"%d:%d.%d\"%*[^<]<clear/>%n", &a2, &a3, &a4, &plen) < 3) &&
                (sscanf(line, "<%*[tT]ime %*[bB]egin=\"%d:%d:%d.%d\"%*[^<]<clear/>%n", &a1, &a2, &a3, &a4, &plen) < 4)) {
                continue;
            }
        }

        std::shared_ptr<ExtSubItem> item = std::shared_ptr<ExtSubItem>(new ExtSubItem());
        item->start = a1 * 360000 + a2 * 6000 + a3 * 100 + a4 / 10;
        item->end = b1 * 360000 + b2 * 6000 + b3 * 100 + b4 / 10;
        if (b1 == 0 && b2 == 0 && b3 == 0 && b4 == 0) {
            item->end = item->start + 200;
        }

        char* p = line;
        p += plen;

        std::string subtitle(p);
        while (true) {
            size_t pos = subtitle.find("<br/>", strlen("<br/>"));
            if (pos == std::string::npos) {
                break;
            }
            subtitle.replace(pos, strlen("<br/>"), "\n");
        }

        removeHtmlToken(subtitle);
        item->lines.push_back(subtitle);
        return item;
    }

    return nullptr;
}


