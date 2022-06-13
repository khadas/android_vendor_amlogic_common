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

class Lyrics : public TextSubtitle {
public:
    Lyrics(std::shared_ptr<DataSource> source);
    ~Lyrics();

protected:
    virtual std::shared_ptr<ExtSubItem> decodedItem();
    char *mBuffer;
    bool mReuseBuffer;
};


