/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "ExtSubStreamReader"

#include<sys/types.h>
#include<unistd.h>

#include "StreamReader.h"


static inline void dump(const char *buf, int size) {
    char str[64] = {0};

    for (int i=0; i<size; i++) {
        char chars[6] = {0};
        sprintf(chars, "%02x ", buf[i]);
        strcat(str, chars);
        if (i%8 == 7) {
            ALOGD("%s", str);
            str[0] = str[1] = 0;
        }
    }
    ALOGD("%s", str);
}


ExtSubStreamReader::ExtSubStreamReader(int charset, std::shared_ptr<DataSource> source) {
    mBuffer = NULL;
    mDataSource = source;
    mEncoding = charset;

    mBufferSize = 0;
    mFileReaded = 0;


    mDataSource->lseek(0, SEEK_SET);
    mStreamSize = mDataSource->availableDataSize();

    // if setup is utf8, we do an auto detect.
    if (mEncoding == AML_ENCODING_NONE) {
        detectEncoding();
    }
}

// detect BOM and discard the leading BOM
void ExtSubStreamReader::detectEncoding() {
    if (mDataSource == nullptr) return;

    char first[3];
    mDataSource->lseek(0, SEEK_SET);
    int r = mDataSource->read(first, 3);
    if (r < 3) {
        mDataSource->lseek(0, SEEK_SET);
        return;
    }

    // TODO: utf32 used rarely, we may also need check it.
    if (first[0] == 0xFF && first[1] == 0xFE) {
        mEncoding = AML_ENCODING_UTF16;
        mDataSource->lseek(2, SEEK_SET);
    } else if (first[0] == 0xFE && first[1] == 0xFF) {
        mEncoding = AML_ENCODING_UTF16BE;
        mDataSource->lseek(2, SEEK_SET);
    } else if (first[0] == 0xEF && first[1] == 0xBB && first[2] == 0xBF) {
        mEncoding = AML_ENCODING_UTF8;
        mDataSource->lseek(3, SEEK_SET);
    } else {
         mDataSource->lseek(0, SEEK_SET);
        // NO BOM, auto detect, almost all use utf8, we current NONE treat as utf8
    }
}


ExtSubStreamReader::~ExtSubStreamReader() {
    if (mBuffer != NULL) {
        free(mBuffer);
        mBuffer = NULL;
    }
}

size_t ExtSubStreamReader::totalStreamSize() {
    return mStreamSize;
}

/*
    Not Thread safe, TODO: fix it.
*/
bool ExtSubStreamReader::rewindStream() {
    if (mDataSource != nullptr && mDataSource->lseek(0, SEEK_SET) >= 0) {
        mBufferSize = 0;
        mFileReaded = 0;
        free(mBuffer);
        mBuffer = nullptr;
        return true;
    }
    return false;
}

int ExtSubStreamReader::_convertToUtf8(int charset, const UTF16 *in, int inLen, UTF8 *out, int outMax) {
    int outLen = 0;
    if (out) {
        // Output buffer passed in; actually encode data.
        while (inLen > 0) {
            UTF16 ch = *in;
            if (charset == AML_ENCODING_UTF16BE) {
                ch = ((ch << 8) & 0xFF00) | ch >> 8;
            }
            inLen--;
            if (ch < 0x80) {
                if (--outMax < 0) {
                    return -1;
                }
                *out++ = (UTF8) ch;
                outLen++;
            } else if (ch < 0x800) {
                if ((outMax -= 2) < 0) {
                    return -1;
                }
                *out++ = (UTF8)(0xC0 | ((ch >> 6) & 0x1F));//1
                *out++ = (UTF8)(0x80 | (ch & 0x3F));
                outLen += 2;
            } else if (ch >= 0xD800 && ch <= 0xDBFF) {
                UTF16 ch2;
                UTF32 ucs4;
                if (--inLen < 0) {
                    return -1;
                }
                ch2 = *++in;
                if (ch2 < 0xDC00 || ch2 > 0xDFFF) {
                    // This is an invalid UTF-16 surrogate pair sequence
                    // Encode the replacement character instead
                    ch = 0xFFFD;
                    goto Encode3;
                }
                ucs4 =
                    ((ch - 0xD800) << 10) + (ch2 - 0xDC00) +
                    0x10000;
                if ((outMax -= 4) < 0) {
                    return -1;
                }
                *out++ = (UTF8)(0xF0 | ((ucs4 >> 18) & 0x07));//2
                *out++ = (UTF8)(0x80 | ((ucs4 >> 12) & 0x3F));
                *out++ = (UTF8)(0x80 | ((ucs4 >> 6) & 0x3F));
                *out++ = (UTF8)(0x80 | (ucs4 & 0x3F));
                outLen += 4;
            } else {
                if (ch >= 0xDC00 && ch <= 0xDFFF) {
                    // This is an invalid UTF-16 surrogate pair sequence
                    // Encode the replacement character instead
                    ch = 0xFFFD;
                }
Encode3:
                if ((outMax -= 3) < 0) {
                    return -1;
                }
                *out++ = (UTF8)(0xE0 | ((ch >> 12) & 0x0F));
                *out++ = (UTF8)(0x80 | ((ch >> 6) & 0x3F));
                *out++ = (UTF8)(0x80 | (ch & 0x3F));
                outLen += 3;
            }
            in++;
        }
    } else {
        // Count output characters without actually encoding.
        while (inLen > 0) {
            UTF16 ch = *in, ch2;
            inLen--;
            if (ch < 0x80) {
                outLen++;
            } else if (ch < 0x800) {
                outLen += 2;
            } else if (ch >= 0xD800 && ch <= 0xDBFF) {
                if (--inLen < 0) {
                    return -1;
                }
                ch2 = *++in;
                if (ch2 < 0xDC00 || ch2 > 0xDFFF) {
                    // Invalid...
                    // We'll encode 0xFFFD for this
                    outLen += 3;
                } else {
                    outLen += 4;
                }
            } else {
                outLen += 3;
            }
            in++;
        }
    }
    return outLen;
}

bool ExtSubStreamReader::convertToUtf8(int charset, char *s, int inLen) {
    UTF8 *utf8Str = NULL;
    int outLen = 0;

    if (charset == AML_ENCODING_UTF8 || charset == AML_ENCODING_NONE) {
        // No need convert
        return true;
    }

    utf8Str = (unsigned char *)malloc(inLen * 2 + 1);
    if (utf8Str == NULL) {
        return false;
    }
    memset(utf8Str, 0x0, inLen * 2 + 1);
    outLen = _convertToUtf8(charset, (const UTF16 *)s, inLen / 2, utf8Str, inLen * 2);
    if (outLen > 0) {
        memcpy(s, utf8Str, outLen);
        s[outLen] = '\0';
    }

    free(utf8Str);
    return true;
}


/* internal string manipulation functions */
int ExtSubStreamReader::ExtSubtitleEol(char p) {
    return (p == '\r' || p == '\n' || p == '\0');
}

char *ExtSubStreamReader::getLineFromString(char *source, char **dest) {
    int len = 0;
    char *p = source;
    while (!ExtSubtitleEol(*p) && *p != '|') {
        p++, len++;
    }
    *dest = (char *)malloc(len + 1);
    if (!dest) {
        return (char *)ERR;
    }
    strncpy(*dest, source, len);
    (*dest)[len] = 0;
    while (*p == '\r' || *p == '\n' || *p == '|') {
        p++;
    }
    if (*p) {
        /* not-last text field */
        return p;
    } else {
        /* last text field */
        return NULL;
    }
}

char *ExtSubStreamReader::strdup(char *src) {
    char *ret;
    int len;
    len = strlen(src);
    ret = (char *)MALLOC(len + 1);
    if (ret) {
        strcpy(ret, src);
    }
    return ret;
}

char *ExtSubStreamReader::strIStr(const char *haystack, const char *needle) {
    int len = 0;
    const char *p = haystack;
    if (!(haystack && needle)) {
        return NULL;
    }
    len = strlen(needle);
    while (*p != '\0') {
        if (strncasecmp(p, needle, len) == 0)
            return (char *)p;
        p++;
    }
    return NULL;
}

void ExtSubStreamReader::trimSpace(char *s) {
    int i = 0;
    int len = strlen(s) + 1;
    char *r = (char *)malloc(len);
    memset(r, 0, len);

    while (isspace(s[i]))
        ++i;

    strcpy(r, s + i);

    int k = strlen(r) - 1;
    while (k > 0 && isspace(r[k]))
        r[k--] = '\0';

    memcpy(s, r, len);  // avoid strcpy memory overlap warning
    free(r);
}

// TODO: rewrite this part
char *ExtSubStreamReader::getLine(char *s/*, int fd*/) {
    int offset = mFileReaded;
    int copied;

    if (!mBuffer) {
        mBuffer = (char *)MALLOC(LINE_LEN*2);
        if (mBuffer == nullptr) {
            ALOGE("1???");
            return nullptr;
        }

        mFileReaded = 0;
        offset = 0;
        mBufferSize = mDataSource->read(mBuffer, LINE_LEN);
    }

    if (mBufferSize <= 0 || offset >= mBufferSize) {
        return nullptr;
    }

    int lineLen = 0;
    int lineCharLen = 0;

     //find one line end, TODO: maybe write in a function
     while (offset <= mBufferSize) {
        bool found = false;

        // found line end position.
        if ((mEncoding == AML_ENCODING_NONE) || (mEncoding == AML_ENCODING_UTF8)) {
            lineCharLen = 1; // '\n'
            if (mBuffer[offset] == '\n' || mBuffer[offset] == '\0') {//eof maybe '\0', offset: mBufferSize+1
                found = true;
            } else {
                offset++;
                if (offset > mBufferSize) {
                    ALOGE("Error! Cannot find end of line, discarsd following line.");
                    return nullptr;
                }
                continue;
            }
        } else if (mEncoding == AML_ENCODING_UTF16BE) {//TV-35678
            lineCharLen = 4; // '00 0d 00 0a'
            if (offset < mBufferSize -lineCharLen) {// should not access out of bound.
                if ((mBuffer[offset] == 0) && (mBuffer[offset+1] == 0xd)
                  && (mBuffer[offset+2] == 0)  && (mBuffer[offset+3] == 0xa)) {
                    found = true;
                } else {
                  offset++;
                  continue;
                }
            }
        } else if (mEncoding == AML_ENCODING_UTF16) {
            lineCharLen = 4; // '0d 00 0a 0d'
            if (offset < mBufferSize -lineCharLen) { // should not access out of bound.
                if ((mBuffer[offset] == 0xd) && (mBuffer[offset+1] == 0x0)
                  && (mBuffer[offset+2] == 0xa)  && (mBuffer[offset+3] == 0)) {
                    found = true;
                } else {
                  offset++;
                  continue;
                }
            }
        } else {
            ALOGE("Error! invalid encoding, current not support");
            return nullptr;
        }

        // found one line string! copy to outbuffer and translate to utf8!
        if (found) {
            lineLen = offset - mFileReaded;
            MEMCPY(s, mBuffer + mFileReaded, lineLen);
            s[lineLen] = '\0';
            //ALOGD("found: %p %s lineLen=%d [%d %d] \n", mBuffer + mFileReaded, s, lineLen, offset, mFileReaded);

            // eat the last newline!
            mFileReaded = offset + lineCharLen;
            break;
        } else {
            // not found end of line. resume reading data and do again.

            if ((mBufferSize -mFileReaded) >= LINE_LEN) {
                ALOGE("Error! line is too long( > %d byte), ignore", LINE_LEN);
                free(mBuffer);
                mBufferSize = mFileReaded = 0;
                mBuffer = nullptr;
                return nullptr;
            }

            // copy the remainder to search line again.
            int remainderSize = mBufferSize -mFileReaded;
            memmove(mBuffer, mBuffer + mFileReaded, remainderSize);

            int readed = mDataSource->read(mBuffer+remainderSize, LINE_LEN);
            if (readed > 0) {
                // resume check again...
                mBufferSize = remainderSize + readed;
                mFileReaded = offset = 0;
                ALOGD("read more: %d", readed);
                continue;
            } else {
                // no more data, then, then the remainder is the last line in sub file.
                if (remainderSize > 0) {
                    lineLen = remainderSize;
                    MEMCPY(s, mBuffer, lineLen);
                    s[lineLen] = '\0';
                    free(mBuffer);
                    mBufferSize = mFileReaded = 0;
                    mBuffer = nullptr;
                    ALOGE("End of line found!");
                    break;
                } else {
                    return nullptr;
                }
            }
        }
    }

    if (lineLen > 0) {
        convertToUtf8(mEncoding, s, lineLen);
        // kill windows \r\n
        if (s[lineLen-1] =='\r') s[lineLen-1] = 0;
        return s;
    } else if (lineLen == 0) { // a blank new line
        return s;
    } else {
        ALOGE("??? lineLen=%d %d %d", lineLen, offset, mFileReaded);
        return nullptr;
    }
}


