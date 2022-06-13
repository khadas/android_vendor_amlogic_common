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

class Mplayer1: public TextSubtitle {

public:
    Mplayer1(std::shared_ptr<DataSource> source);
    ~Mplayer1();

protected:
    virtual std::shared_ptr<ExtSubItem> decodedItem();
    int mPtsRate;
};


