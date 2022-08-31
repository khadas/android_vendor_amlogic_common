#define LOG_TAG "XmlSubtitle"

#include "XmlSubtitle.h"
#include "tinyxml2.h"

XmlSubtitle::XmlSubtitle(std::shared_ptr<DataSource> source): TextSubtitle(source) {
    mDataSource = source;
    parseXml();
}

XmlSubtitle::~XmlSubtitle() {
    mDataSource = nullptr;
}

void XmlSubtitle::parseXml() {
    int size = mDataSource->availableDataSize();
    char *rdBuffer = new char[size];
    mDataSource->read(rdBuffer, size);
    // no need to keep reference here! one shot parse.
    mDataSource = nullptr;

    tinyxml2::XMLDocument doc;
    doc.Parse(rdBuffer);

    tinyxml2::XMLElement* root = doc.RootElement();
    if (root == NULL) {
        ALOGD("Failed to load file: No root element.\n");
        doc.Clear();
        delete[] rdBuffer;
        return;
    }

    tinyxml2::XMLElement *parag = root->FirstChildElement("Paragraph");
    while (parag != nullptr) {
        const tinyxml2::XMLAttribute *attribute = parag->FirstAttribute();
        std::shared_ptr<ExtSubItem> item = std::shared_ptr<ExtSubItem>(new ExtSubItem());
        tinyxml2::XMLElement *e = parag->FirstChildElement("Number");
        //if (e != nullptr) ALOGD("parseXml num:%s\n", e->GetText());

        e = parag->FirstChildElement("StartMilliseconds");
        if (e!= nullptr) {
            item->start = e->Int64Text() / 10; // subtitle pts multiply
            //ALOGD("parseXml StartMilliseconds:%s  %lld, %lld\n", e->GetText(), item->start, e->Int64Text());
        }

        e = parag->FirstChildElement("EndMilliseconds");
        if (e!= nullptr) {
            //ALOGD("parseXml EndMilliseconds:%s\n", e->GetText());
            item->end = e->Int64Text() / 10; // subtitle pts multiply
        }

        e = parag->FirstChildElement("Text");
        while (e != nullptr) {
            //ALOGD("parseXml Text:%s\n", e->GetText());
            item->lines.push_back(std::string(e->GetText()));
            e = e->NextSiblingElement("Text");
        }
        parag = parag->NextSiblingElement();
        mSubtitles.push_back(item);
    }

    doc.Clear();
    delete[] rdBuffer;
}

std::shared_ptr<ExtSubItem> XmlSubtitle::decodedItem() {
    if (mSubtitles.size() > 0) {
        std::shared_ptr<ExtSubItem> item = mSubtitles.front();
        mSubtitles.pop_front();
        if (item->start < 0) return nullptr;

        if (item->end <= item->start) {
            item->end = item->start + 2000;
        }

        return item;
    }
    return nullptr;
}

