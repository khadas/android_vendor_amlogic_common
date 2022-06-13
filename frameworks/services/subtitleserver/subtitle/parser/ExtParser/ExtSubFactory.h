#ifndef _EXT_PARSER_FACTORY_H__
#define _EXT_PARSER_FACTORY_H__
#include <memory>

#include "DataSource.h"
#include "TextSubtitle.h"

class ExtSubFactory {

public:
    static std::shared_ptr<TextSubtitle> create(std::shared_ptr<DataSource> source);

private:
    static int detect(std::shared_ptr<DataSource> source);
    static int detectXml(std::shared_ptr<DataSource> source);
};
#endif

