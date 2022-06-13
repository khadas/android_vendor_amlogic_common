#define LOG_TAG "Ttml"

#include "Ttml.h"
#include "tinyxml2.h"

TTML::TTML(std::shared_ptr<DataSource> source): TextSubtitle(source) {
    mDataSource = source;
    ALOGD("TTML");
    parseXml();
}

TTML::~TTML() {
}

void TTML::parseXml() {
    int size = mDataSource->availableDataSize();
    char *rdBuffer = new char[size];
    mDataSource->read(rdBuffer, size);
    // no need to keep reference here! one shot parse.
    mDataSource = nullptr;

    tinyxml2::XMLDocument doc;
    doc.Parse(rdBuffer);

    tinyxml2::XMLElement* root = doc.RootElement();
    if (root == NULL) {
        ALOGD("Failed to load file: No root element.");
        doc.Clear();
        delete[] rdBuffer;
        return;
    }

    // TODO: parse header, if we support style metadata and layout property

    // parse body
    tinyxml2::XMLElement *body = root->FirstChildElement("body");
    if (body == nullptr) body = root->FirstChildElement("tt:body");
    if (body == nullptr) {
        ALOGD("Error. no body found!");
        doc.Clear();
        delete[] rdBuffer;
        return;
    }

    tinyxml2::XMLElement *div = body->FirstChildElement("div");
    if (div == nullptr) div = body->FirstChildElement("tt:div");
    tinyxml2::XMLElement *parag;
    if (div == nullptr) {
        ALOGD("%d", __LINE__);
        parag = root->FirstChildElement("p");
    } else {
        ALOGD("%d", __LINE__);
        parag = div->FirstChildElement("p");
    }

    while (parag != nullptr) {
        const tinyxml2::XMLAttribute *attr = parag->FindAttribute("xml:id");
        if (attr != nullptr) ALOGD("parseXml xml:id=%s\n", attr->Value());

        std::shared_ptr<ExtSubItem> item = std::shared_ptr<ExtSubItem>(new ExtSubItem());
        attr = parag->FindAttribute("begin");
        if (attr != nullptr) {
            item->start = attr->FloatValue() *100; // subtitle pts multiply
            ALOGD("parseXml begin:%s\n", attr->Value());
        }

        attr = parag->FindAttribute("end");
        if (attr != nullptr) {
            item->end = attr->FloatValue() *100; // subtitle pts multiply
            ALOGD("parseXml end:%s\n", attr->Value());
        }

        ALOGD("parseXml Text:%s\n", parag->GetText());
        item->lines.push_back(std::string(parag->GetText()));

        parag = parag->NextSiblingElement();
        mSubtitles.push_back(item);
    }

    doc.Clear();
    delete[] rdBuffer;
}

std::shared_ptr<ExtSubItem> TTML::decodedItem() {
    if (mSubtitles.size() > 0) {
        std::shared_ptr<ExtSubItem> item = mSubtitles.front();
        mSubtitles.pop_front();
        if (item->start < 0) return nullptr;

        if (item->end <= item->start) {
            item->end = item->start + 200;
        }

        return item;
    }
    return nullptr;
}


