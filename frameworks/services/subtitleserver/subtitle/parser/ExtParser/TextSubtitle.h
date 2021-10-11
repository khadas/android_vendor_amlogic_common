#pragma once

#include <list>
#include "DataSource.h"
#include "StreamReader.h"
#include "sub_types.h"

#define SUB_MAX_TEXT                30

#define sub_ms2pts(x) ((x) * 900)
#define sub_pts2ms(x) ((x) / 900)

typedef enum
{
    SUB_ALIGNMENT_BOTTOMLEFT = 1,
    SUB_ALIGNMENT_BOTTOMCENTER,
    SUB_ALIGNMENT_BOTTOMRIGHT,
    SUB_ALIGNMENT_MIDDLELEFT,
    SUB_ALIGNMENT_MIDDLECENTER,
    SUB_ALIGNMENT_MIDDLERIGHT,
    SUB_ALIGNMENT_TOPLEFT,
    SUB_ALIGNMENT_TOPCENTER,
    SUB_ALIGNMENT_TOPRIGHT
} sub_alignment_t;

struct ExtSubItem{
    bool valid;     // get one item, but invalid!
    int64_t start;      /* start time */
    int64_t end;        /* end time */

    // === for IDX-SUB
    int subId; // one subtitle file may have many lang subs, this for select...
    long long filePos;
    // === END IDX-SUB

    /// number of subtitle lines, can be multi-line
    std::list<std::string> lines;

    /// alignment of subtitles
    sub_alignment_t alignment;
};

struct ExtSubData {
    int totalItems;
    int errorItems;
    std::string format;

    std::list<std::shared_ptr<ExtSubItem>> subtitles;
};

class TextSubtitle {
public:
    TextSubtitle() = delete;
    TextSubtitle(std::shared_ptr<DataSource> source);
    virtual  ~TextSubtitle() {}

    bool decodeSubtitles(int idxSubTrackId);

    int totalItems();
    virtual std::shared_ptr<AML_SPUVAR> popDecodedItem();

    virtual void dump(int fd, const char *prefix);

protected:
    virtual std::shared_ptr<ExtSubItem> decodedItem() = 0;
    ExtSubData mSubData;
    std::shared_ptr<DataSource> mSource;
    std::shared_ptr<ExtSubStreamReader> mReader;

    int mIdxSubTrackId;
};
