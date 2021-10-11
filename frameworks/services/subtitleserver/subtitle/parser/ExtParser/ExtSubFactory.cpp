#define LOG_TAG "ExtSubFactory"

#include "ParserFactory.h"
#include "ExtSubFactory.h"
#include "StreamReader.h"

#include "tinyxml2.h"

#include "SubStationAlpha.h"
#include "Subrip.h"
#include "Aqtitle.h"
#include "Jacosub.h"
#include "Mircodvd.h"
#include "Mplayer1.h"
#include "Mplayer2.h"
#include "Mpsub.h"
#include "Pjs.h"
#include "RealText.h"
#include "Sami.h"
#include "SubViewer.h"
#include "XmlSubtitle.h"
#include "Ttml.h"
#include "Vplayer.h"
#include "Lyrics.h"
#include "Subrip09.h"
#include "SubViewer2.h"
#include "SubViewer3.h"
#include "VobSubIndex.h"


#if 0
#include "ExtParserEbuttd.h"
#endif
#include "WebVtt.h"

int ExtSubFactory::detect(std::shared_ptr<DataSource> source) {
    char line[LINE_LEN + 1];
    int i, maxLineDetect = 100;
    char p;
    //char q[512];

    std::shared_ptr<ExtSubStreamReader> reader = std::shared_ptr<ExtSubStreamReader>(new ExtSubStreamReader(0, source));
    if (reader == NULL) {
        ALOGD("can't get ext subtitle format");
        return SUB_INVALID;
    }


    while (maxLineDetect-- > 0) {
        ALOGD("%d", maxLineDetect);

        if (!reader->getLine(line)) {
            return SUB_INVALID;
        }
        ALOGD("%s", line);
        if (sscanf(line, "{%d}{%d}", &i, &i) == 2) {
            return SUB_MICRODVD;
        }

        if (sscanf(line, "{%d}{}", &i) == 1) {
            return SUB_MICRODVD;
        }

        if (strncmp(line, "WEBVTT", 6) == 0) {
            return SUB_WEBVTT;
        }


        if (sscanf(line, "%d,%d,%d", &i, &i, &i) == 3) {
            return SUB_MPL1;
        }

        if (sscanf(line, "[%d][%d]", &i, &i) == 2) {
            return SUB_MPL2;
        }

        if (sscanf(line, "%d:%d:%d.%d,%d:%d:%d.%d", &i, &i, &i, &i, &i, &i,&i, &i) == 8) {
            return SUB_SUBRIP;
        }

        if (sscanf(line, "%d:%d:%d%[,.:]%d --> %d:%d:%d%[,.:]%d", &i, &i, &i, (char *)&i, &i, &i, &i, &i, (char *)&i, &i) == 10) {
            return SUB_SUBVIEWER;
        }

        if (sscanf(line, "{T %d:%d:%d:%d", &i, &i, &i, &i) == 4) {
            return SUB_SUBVIEWER2;
        }

        if (sscanf(line, "%d  %d:%d:%d,%d  %d:%d:%d,%d", &i, &i, &i, &i, &i, &i, &i, &i, &i) == 9) {
            return SUB_SUBVIEWER3;
        }

        if (strstr(line, "<SAMI>") || strstr(line, "<sami>")) {
            return SUB_SAMI;
        }

        if (sscanf(line, "%d:%d:%d.%d %d:%d:%d.%d", &i, &i, &i, &i, &i, &i, &i, &i) == 8) {
            return SUB_JACOSUB;
        }

        if (sscanf(line, "@%d @%d", &i, &i) == 2) {
            return SUB_JACOSUB;
        }

        if (sscanf(line, "%d:%d:%d:", &i, &i, &i) == 3) {
            return SUB_VPLAYER;
        }

        if (sscanf(line, "[%d:%d.%d:", &i, &i, &i) == 3) {
            return SUB_LRC;
        }

        if (sscanf(line, "%d:%d:%d ", &i, &i, &i) == 3) {
            return SUB_VPLAYER;
        }

        if (strstr(line, "<?xml")) {
            //return SUB_XML;
            return detectXml(source);
        }

        if (strstr(line, "<tt")) {
            return SUB_TTML;
        }

        //TODO: just checking if first line of sub starts with "<" is WAY
        // too weak test for RT
        // Please someone who knows the format of RT... FIX IT!!!
        // It may conflict with other sub formats in the future (actually it doesn't)
        if (*line == '<') {
            return SUB_RT;
        }

        if (!memcmp(line, "Dialogue: Marked", 16)) {
            return SUB_SSA;
        }
        if (!memcmp(line, "Dialogue: ", 10)) {
            return SUB_SSA;
        }
        if (sscanf(line, "%d,%d,\"%c", &i, &i, (char *)&i) == 3) {
            return SUB_PJS;
        }
        if (sscanf(line, "%d,%d, \"%c", &i, &i, (char *)&i) == 3) {
            return SUB_PJS;
        }
        if (sscanf(line, "FORMAT=%d", &i) == 1) {
            return SUB_MPSUB;
        }
        if (sscanf(line, "FORMAT=TIM%c", &p) == 1 && p == 'E') {
            return SUB_MPSUB;
        }
        if (strstr(line, "-->>")) {
            return SUB_AQTITLE;
        }
        if (sscanf(line, "[%d:%d:%d]", &i, &i, &i) == 3) {
            return SUB_SUBRIP09;
        }

        if ((strstr(line, "VobSub index file") != nullptr)
            || sscanf(line, "timestamp: [%d]:[%d]:[%d]:[%d], filepos: [%d]", &i, &i, &i, &i, &i) == 5) {
            return SUB_IDXSUB;
        }

        if (sscanf(line, "[%d:%d:%d]", &i, &i, &i) == 3) {
            return SUB_SUBRIP09;
        }
    }
    return SUB_INVALID; // too many bad lines

}

// TODO: more....
std::shared_ptr<TextSubtitle> ExtSubFactory::create(std::shared_ptr<DataSource> source) {
    int format = detect(source);
    ALOGD("detect ext subtitle format = %d", format);

   switch (format) {

        case SUB_MICRODVD://0
            return std::shared_ptr<TextSubtitle> (new Mircodvd(source));

        case SUB_SUBRIP://1
            return std::shared_ptr<TextSubtitle> (new Subrip(source));

        case SUB_SAMI://3
            return std::shared_ptr<TextSubtitle> (new Sami(source));

        case SUB_SUBVIEWER://2
            return std::shared_ptr<TextSubtitle> (new SubViewer(source));

        case SUB_VPLAYER://4
            return std::shared_ptr<TextSubtitle> (new Vplayer(source));

        case SUB_RT://5
            return std::shared_ptr<TextSubtitle> (new RealText(source));

        case SUB_SSA://6
            return std::shared_ptr<TextSubtitle> (new SubStationAlpha(source));

        case SUB_PJS://7
            return std::shared_ptr<TextSubtitle> (new Pjs(source));

        case SUB_MPSUB://8
            return std::shared_ptr<TextSubtitle> (new Mpsub(source));

        case SUB_AQTITLE://9
            return std::shared_ptr<TextSubtitle> (new Aqtitle(source));

        case SUB_SUBVIEWER2://10
            return std::shared_ptr<TextSubtitle> (new SubViewer2(source));

        case SUB_SUBVIEWER3://11
            return std::shared_ptr<TextSubtitle> (new SubViewer3(source));

        case SUB_SUBRIP09://12
            return std::shared_ptr<TextSubtitle> (new Subrip09(source));

        case SUB_JACOSUB://13
           return std::shared_ptr<TextSubtitle> (new Jacosub(source));

        case SUB_MPL1://14
            return std::shared_ptr<TextSubtitle> (new Mplayer1(source));

        case SUB_MPL2://15
            return std::shared_ptr<TextSubtitle> (new Mplayer2(source));

        case SUB_XML://16
            return std::shared_ptr<TextSubtitle> (new XmlSubtitle(source));

        case SUB_TTML://17
            return std::shared_ptr<TextSubtitle> (new TTML(source));

        case SUB_LRC://18
            return std::shared_ptr<TextSubtitle> (new Lyrics(source));

/*        case SUB_EBUTTD:
            return std::shared_ptr<Parser> (new ExtParserEbuttd(source));
        */
        case SUB_WEBVTT:
            return std::shared_ptr<TextSubtitle>(new SimpleWebVtt(source));

        case SUB_IDXSUB:
        return std::shared_ptr<TextSubtitle>(new VobSubIndex(source));

        default:
            ALOGD("ext subtitle format is invaild! format = %d", format);
            return NULL;
    }
    return NULL;
}

int ExtSubFactory::detectXml(std::shared_ptr<DataSource> source) {
    int type = SUB_XML;
    tinyxml2::XMLDocument doc;

    source->lseek(0, SEEK_SET);
    int size = source->availableDataSize();
    char *rdBuffer = new char[size]();
    source->read(rdBuffer, size);
    doc.Parse(rdBuffer);

    if (doc.FirstChildElement("tt") || doc.FirstChildElement("tt:tt")) {
        //tinyxml2::XMLElement* tt = doc.RootElement();
        //if (tt->Attribute("xmlns:ebuttm")) {
        //    type = SUB_EBUTTD;
        //} else {
            type = SUB_TTML;
        //}
    }

    doc.Clear();
    delete[] rdBuffer;
    return type;
}
