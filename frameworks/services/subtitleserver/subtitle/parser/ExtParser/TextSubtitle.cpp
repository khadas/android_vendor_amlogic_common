#define LOG_TAG "TextSubtitle"

#include "TextSubtitle.h"
#include "StreamReader.h"

#include <utils/Log.h>

TextSubtitle::TextSubtitle(std::shared_ptr<DataSource> source) {
    mSource = source;
    mReader = std::shared_ptr<ExtSubStreamReader>(new ExtSubStreamReader(AML_ENCODING_NONE, source));

}

bool TextSubtitle::decodeSubtitles() {

    mSource->lseek(0, SEEK_SET);
    //mPtsRate = 15;       //24;//dafault value

    ALOGD("decodeSubtitles....");
    while (true) {

        std::shared_ptr<ExtSubItem> item = this->decodedItem();
        if (item == nullptr) {
            break; // No more data, EOF found.
        }


        // TODO: how to handle error states.


        item->start = sub_ms2pts(item->start);
        item->end = sub_ms2pts(item->end);

        mSubData.subtitles.push_back(item);
    }

    dump(0, nullptr);
    return true;
}

/* consume subtitle */
std::shared_ptr<AML_SPUVAR> TextSubtitle::popDecodedItem() {
    if (totalItems() <= 0) {
        return nullptr;
    }

    std::shared_ptr<ExtSubItem> item = mSubData.subtitles.front();
    mSubData.subtitles.pop_front();
    std::shared_ptr<AML_SPUVAR> spu(new AML_SPUVAR());

    spu->pts = item->start;
    spu->m_delay = item->end;

    std::string str;
    std::for_each(item->lines.begin(), item->lines.end(), [&](std::string &s) {
        str.append(s);
        str.append("\n");
    });

    spu->spu_data = (unsigned char *)malloc(str.length()+1);
    memcpy(spu->spu_data, str.c_str(), str.length());
    spu->spu_data[str.length()] = 0;
    spu->buffer_size = str.length();
    spu->isExtSub = true;

    return spu;
}

// return total decoded, not consumed subtitles
int TextSubtitle::totalItems() {
    return mSubData.subtitles.size();
}


void TextSubtitle::dump(int fd, const char *prefix) {
    if (fd <= 0) {
        ALOGD("Total: %d", mSubData.subtitles.size());
        for (auto i : mSubData.subtitles) {
            ALOGD("[%08lld:%08lld]", sub_pts2ms(i->start), sub_pts2ms(i->end));
            for (auto s :i->lines) {
                ALOGD("    %s", s.c_str());
            }
        }
     }
}


