/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */
#pragma once

#include "TextSubtitle.h"


class XmlSubtitle: public TextSubtitle {
public:
    XmlSubtitle(std::shared_ptr<DataSource> source);
    ~XmlSubtitle();

protected:
    virtual std::shared_ptr<ExtSubItem> decodedItem();

private:
    std::shared_ptr<DataSource> mDataSource;
    void parseXml();
    std::list<std::shared_ptr<ExtSubItem>> mSubtitles;
};


