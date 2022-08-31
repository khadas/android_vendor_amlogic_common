#define LOG_TAG "Subrip09"

#include "Subrip09.h"

Subrip09::Subrip09(std::shared_ptr<DataSource> source): TextSubtitle(source) {
}

Subrip09::~Subrip09() {
}
/**
**       Very simple, only parse the content **
[TITLE]
The Island
[AUTHOR]

[SOURCE]

[PRG]

[FILEPATH]

[DELAY]
0
[CD TRACK]
0
[BEGIN]
******** START SCRIPT ********
[00:00:00]
Don't move
[00:00:01]

[00:00:08]
Look at that
[00:00:10]

[00:00:10]
Those are kids.
[00:00:12]

[00:00:21]
Come here. The train is non-stop to LA. You need these to get on.
[00:00:24]

**/
std::shared_ptr<ExtSubItem> Subrip09::decodedItem() {
    char line[LINE_LEN + 1];

    while (mReader->getLine(line)) {
        std::shared_ptr<ExtSubItem> item = std::shared_ptr<ExtSubItem>(new ExtSubItem());

        int a1, a2, a3;
        if (sscanf(line, "[%d:%d:%d]", &a1, &a2, &a3) < 3) {
            continue;
        }

        item->start = a1 * 360000 + a2 * 6000 + a3 * 100;

        while (mReader->getLine(line)) {
            if (isEmptyLine(line)) {
                break;
            }


            if (sscanf(line, "[%d:%d:%d]", &a1, &a2, &a3) == 3) {
                item->end = a1 * 360000 + a2 * 6000 + a3 * 100;
            } else {
                item->lines.push_back(std::string(line));
            }
        }
        return item;
    }

    return nullptr;
}


