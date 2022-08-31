/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: h file
*/
#pragma once

#include "Parser.h"
#include "DataSource.h"
#include "sub_types.h"
#include "StreamReader.h"
#include "TextSubtitle.h"

#define SUBAPI
/* Maximal length of line of a subtitle */
#define ENABLE_PROBE_UTF8_UTF16
#define SUBTITLE_PROBE_SIZE     1024

/**
 * Subtitle struct unit
 */
typedef struct {
    /// number of subtitle lines
    int lines;

    /// subtitle strings
    char *text[SUB_MAX_TEXT];

    /// alignment of subtitles
    sub_alignment_t alignment;
} subtext_t;

struct subdata_s {
    list_t list;        /* head node of subtitle_t list */
    list_t list_temp;

    int sub_num;
    int sub_error;
    int sub_format;
};

typedef struct subdata_s subdata_t;

typedef struct subtitle_s subtitle_t;

struct subtitle_s {
    list_t list;        /* linked list */
    int start;      /* start time */
    int end;        /* end time */
    subtext_t text;     /* subtitle text */
    unsigned char *subdata; /* data for divx bmp subtitle */
};




class ExtParser: public Parser {
public:
    ExtParser(std::shared_ptr<DataSource> source);
    virtual ~ExtParser();
    virtual int parse();

    virtual void dump(int fd, const char *prefix) {};
    void resetForSeek();


protected:
    ExtSubData mSubData;
    std::shared_ptr<TextSubtitle> mSubDecoder;

    int mPtsRate = 24;
    bool mGotPtsRate = true;
    int mNoTextPostProcess = 0;  // 1 => do not apply text post-processing
    /* read one line of string from data source */
    int mSubIndex = 0;

private:
    //bool decodeSubtitles();

    int getSpu();
};

