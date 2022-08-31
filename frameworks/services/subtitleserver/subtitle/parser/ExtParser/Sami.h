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

class Sami: public TextSubtitle {

public:
    Sami(std::shared_ptr<DataSource> source);
    ~Sami();

protected:
    virtual std::shared_ptr<ExtSubItem> decodedItem();

private:
    bool mTextPostProcess;
    int mSubSlackTime;
};

