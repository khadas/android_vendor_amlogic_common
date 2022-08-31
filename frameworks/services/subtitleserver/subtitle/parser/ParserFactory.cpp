#include "ParserFactory.h"

#include "AssParser.h"
#include "DvbParser.h"
#include "DvdParser.h"
#include "PgsParser.h"
#include "TeletextParser.h"
#include "ClosedCaptionParser.h"
#include "Scte27Parser.h"
#include "ExtParser.h"

std::shared_ptr<Parser> ParserFactory::create(
            std::shared_ptr<SubtitleParamType> subParam,
            std::shared_ptr<DataSource> source) {
    int type = subParam->subType;


    // TODO: unless we can determin CC type, or default start CC parser
    if (type == TYPE_SUBTITLE_INVALID) {
        type = TYPE_SUBTITLE_CLOSED_CATPTION;
     }

    switch (type) {
        case TYPE_SUBTITLE_VOB:
            return std::shared_ptr<Parser>(new DvdParser(source));

        case TYPE_SUBTITLE_PGS:
            return std::shared_ptr<Parser>(new PgsParser(source));

        case TYPE_SUBTITLE_MKV_STR:
            return std::shared_ptr<Parser>(new AssParser(source));

        case TYPE_SUBTITLE_MKV_VOB:
            return std::shared_ptr<Parser>(new AssParser(source));

        case TYPE_SUBTITLE_SSA:
            return std::shared_ptr<Parser>(new AssParser(source));

        case TYPE_SUBTITLE_DVB:
        case TYPE_SUBTITLE_DTVKIT_DVB:
            return std::shared_ptr<Parser>(new DvbParser(source));

        case TYPE_SUBTITLE_TMD_TXT:
            return std::shared_ptr<Parser>(new AssParser(source));

        case TYPE_SUBTITLE_IDX_SUB:
            return std::shared_ptr<Parser>(new AssParser(source));

        case TYPE_SUBTITLE_DVB_TELETEXT:
        case TYPE_SUBTITLE_DTVKIT_TELETEXT:
            return std::shared_ptr<Parser>(new TeletextParser(source));

        case TYPE_SUBTITLE_CLOSED_CATPTION: {
            std::shared_ptr<Parser> p = std::shared_ptr<Parser>(new ClosedCaptionParser(source));
            if (p != nullptr) {
                p->updateParameter(type, &subParam->ccParam);
                p->setPipId(PIP_PLAYER_ID, subParam->playerId);
                p->setPipId(PIP_MEDIASYNC_ID, subParam->mediaId);
            }
            return p;
        }

        case TYPE_SUBTITLE_DTVKIT_SCTE27:
        case TYPE_SUBTITLE_SCTE27: {
            std::shared_ptr<Parser> p = std::shared_ptr<Parser>(new Scte27Parser(source));
            if (p != nullptr) {
                p->updateParameter(type, &subParam->scteParam);
                p->setPipId(PIP_MEDIASYNC_ID, subParam->mediaId);
            }
            return p;
        }

        case TYPE_SUBTITLE_EXTERNAL:
            return std::shared_ptr<Parser>(new ExtParser(source));

    }

    // TODO: change to stubParser
    return std::shared_ptr<Parser>(new AssParser(source));
}

DisplayType ParserFactory::getDisplayType(int type)
{
    switch (type) {
        case TYPE_SUBTITLE_VOB:
        case TYPE_SUBTITLE_PGS:
        case TYPE_SUBTITLE_MKV_VOB:
        case TYPE_SUBTITLE_DVB:
        case TYPE_SUBTITLE_DVB_TELETEXT:
        case TYPE_SUBTITLE_SCTE27:
          return SUBTITLE_IMAGE_DISPLAY;
        case TYPE_SUBTITLE_MKV_STR:
        case TYPE_SUBTITLE_TMD_TXT:
        case TYPE_SUBTITLE_IDX_SUB:
        case  TYPE_SUBTITLE_SSA:
            return SUBTITLE_TEXT_DISPLAY;

        default:
            return SUBTITLE_TEXT_DISPLAY;
    }

}
