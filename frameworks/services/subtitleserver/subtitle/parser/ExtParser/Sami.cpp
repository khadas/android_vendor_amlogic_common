#define LOG_TAG "Sami"

#include "Sami.h"

// TODO: rewrite use xml
Sami::Sami(std::shared_ptr<DataSource> source): TextSubtitle(source) {
    mTextPostProcess = false;
    mSubSlackTime = 20000;
}

Sami::~Sami() {
}

/**
 * Synchronized Accessible Media Interchange (SAMI)
 * Structured markup language subtitle
 * http://msdn.microsoft.com/en-us/library/ms971327.aspx
 *
<SAMI>

<HEAD>
    <TITLE>SAMI Example</TITLE>

    <SAMIParam>
          Media {cheap44.wav}
          Metrics {time:ms;}
          Spec {MSFT:1.0;}
    </SAMIParam>

    <STYLE TYPE="text/css">
        <!--
          P { font-family: Arial; font-weight: normal; color: white; background-color: black; text-align: center; }

          #Source {color: red; background-color: blue; font-family: Courier; font-size: 12pt; font-weight: normal; text-align: left; }

          .ENUSCC { name: English; lang: en-US ; SAMIType: CC ; }
          .FRFRCC { name: French;  lang: fr-FR ; SAMIType: CC ; }
        -->
    </STYLE>

</HEAD>

<BODY>

    <!-- Open play menu, choose Captions and Subtiles, On if available -->
    <!-- Open tools menu, Security, Show local captions when present -->

    <SYNC Start=0>
      <P Class=ENUSCC ID=Source>The Speaker</P>
      <P Class=ENUSCC>SAMI 0000 text</P>

      <P Class=FRFRCC ID=Source>Le narrateur</P>
      <P Class=FRFRCC>Texte SAMI 0000</P>
    </SYNC>

    <SYNC Start=1000>
      <P Class=ENUSCC>SAMI 1000 text</P>
      <P Class=FRFRCC>Texte SAMI 1000</P>
    </SYNC>

    <SYNC Start=2000>
      <P Class=ENUSCC>SAMI 2000 text</P>
      <P Class=FRFRCC>Texte SAMI 2000</P>
    </SYNC>

    <SYNC Start=3000>
      <P Class=ENUSCC>SAMI 3000 text</P>
      <P Class=FRFRCC>Texte SAMI 3000</P>
    </SYNC>

</BODY>
</SAMI>
 *
 */
std::shared_ptr<ExtSubItem> Sami::decodedItem() {
    char * line = (char *)MALLOC(LINE_LEN+1);
    char * text = (char *)MALLOC(LINE_LEN+1);
    int state = 0;
    int mSubSlackTime= 0;
    char *p = NULL, *q;
    memset(line, 0, LINE_LEN+1);
    memset(text, 0, LINE_LEN+1);
    std::shared_ptr<ExtSubItem> item = std::shared_ptr<ExtSubItem>(new ExtSubItem());

    if (!mReader->getLine(line)) {
        free(line);
        free(text);
        return nullptr;
    }
    char *s = line;

    while (state != 99) {
        //if (s != nullptr) s = triml(s, "\t ");
        //ALOGD("state=%d %s", state, s);

        switch (state) {
            case 0: {/* find "START=" or "Slacktime:" */
                char *slacktime_s = mReader->strIStr(s, "Slacktime:");
                if (slacktime_s) {
                    mSubSlackTime = strtol(slacktime_s + 10, NULL, 0) / 10;
                }

                s = mReader->strIStr(s, "Start=");
                if (s) {
                    item->start = strtol(s + 6, &s, 0) / 10;
                    /* eat '>' */
                    for (; *s != '>' && *s != '\0'; s++) ;
                    s++;
                    state = 1;
                    continue;
                }
                break;
            }
            case 1: /* find (optionnal) "<P", skip other TAGs */
                //for (; *s == ' ' || *s == '\t'; s++) ;  /* strip blanks, if any */
                if (*s == '\0')
                    break;
                if (*s != '<') {
                    state = 3;
                    p = text;
                    memset(text, 0, LINE_LEN + 1);
                    continue;
                }
                /* not a TAG */
                s++;
                if (*s == 'P' || *s == 'p') {
                    s++;
                    state = 2;
                    continue;
                }
                /* found '<P' */
                for (; *s != '>' && *s != '\0'; s++) ;  /* skip remains of non-<P> TAG */
                if (*s == '\0')
                    break;
                s++;
                break;;
            case 2: /* find ">" */
                s = strchr(s, '>');
                if (s) {
                    s++;
                    state = 3;
                    p = text;
                    memset(text, 0, LINE_LEN + 1);
                    continue;
                }
                break;

            case 3: /* get all text until '<' appears */
                if (*s == '\0')
                    break;
                else if (!strncasecmp(s, "<br>", 4)) {
                    *p = '\0';
                    p = text;
                    mReader->trimSpace(text);
                    if (text[0] != '\0') {
                        item->lines.push_back(std::string(text));
                    }
                    s += 4;
                } else if ((*s == '{') && mTextPostProcess) {
                    state = 5;
                    ++s;
                    continue;
                } else if (*s == '<') {
                    state = 4;
                } else if (!strncasecmp(s, "&nbsp;", 6)) {
                    *p++ = ' ';
                    s += 6;
                } else if (*s == '\t') {
                    *p++ = ' ';
                    s++;
                } else if (*s == '\r' || *s == '\n') {
                    s++;
                } else
                    *p++ = *s++;

                /* skip duplicated space */
                if (p > text + 2)
                    if (*(p - 1) == ' ' && *(p - 2) == ' ')
                        p--;
                continue;

            case 4: /* get current->end or skip <TAG> */
                q = mReader->strIStr(s, "Start=");
                if (q) {
                    item->end = strtol(q + 6, &q, 0) / 10 - 1;
                    *p = '\0';
                    mReader->trimSpace(text);
                    if (text[0] != '\0') {
                        item->lines.push_back(std::string(text));
                    }
                    if (item->lines.size() > 0) {
                        state = 99;
                        break;
                    }
                    state = 0;
                    continue;
                }
                s = strchr(s, '>');
                if (s) {
                    s++;
                    state = 3;
                    continue;
                }
                break;
#if SUPPORT_ALIGNMENT
            case 5: /* get rid of {...} text, but read the alignment code */
                if ((*s == '\\') && (*(s + 1) == 'a') && mTextPostProcess) {
                    if (strstr(s, "\\a1") != NULL) {
                        //current->text.alignment = SUB_ALIGNMENT_BOTTOMLEFT;
                        s = s + 3;
                    }
                    if (strstr(s, "\\a2") != NULL) {
                        //current->text.alignment = SUB_ALIGNMENT_BOTTOMCENTER;
                        s = s + 3;
                    } else if (strstr(s, "\\a3") != NULL) {
                        //current->text.alignment = SUB_ALIGNMENT_BOTTOMRIGHT;
                        s = s + 3;
                    } else if ((strstr(s, "\\a4") != NULL)
                             || (strstr(s, "\\a5") != NULL)
                             || (strstr(s, "\\a8") != NULL)) {
                        //current->text.alignment = SUB_ALIGNMENT_TOPLEFT;
                        s = s + 3;
                    } else if (strstr(s, "\\a6") != NULL) {
                        //current->text.alignment = SUB_ALIGNMENT_TOPCENTER;
                        s = s + 3;
                    } else if (strstr(s, "\\a7") != NULL) {
                        //current->text.alignment = SUB_ALIGNMENT_TOPRIGHT;
                        s = s + 3;
                    } else if (strstr(s, "\\a9") != NULL) {
                        //current->text.alignment = SUB_ALIGNMENT_MIDDLELEFT;
                        s = s + 3;
                    } else if (strstr(s, "\\a10") != NULL) {
                        //current->text.alignment = SUB_ALIGNMENT_MIDDLECENTER;
                        s = s + 4;
                    } else if (strstr(s, "\\a11") != NULL) {
                        //current->text.alignment = SUB_ALIGNMENT_MIDDLERIGHT;
                        s = s + 4;
                    }
                }
                if (*s == '}')
                    state = 3;
                ++s;
                continue;
#endif
        }

        /* read next line  and handle EOF*/
        if (state != 99) {
            if ((s = mReader->getLine(line)) == nullptr) {
                s = line;
                if (item->start > 0) {
                     break;  // if it is the last subtitle
                } else {
                    free(line);
                    free(text);
                    return nullptr;
                }
            }
        }
     }

    /* read next line */
    if (item->end <= 0) {
        item->end = item->start + mSubSlackTime;
        mReader->trimSpace(text);
        if (text[0] != '\0') {
            item->lines.push_back(std::string(text));
        } else {
            free(line);
            free(text);
            return nullptr;
        }
    }
/*
    ALOGD("[%lld %lld]", item->start, item->end);
    for (auto s :item->lines) {
        ALOGD("    %s", s.c_str());
    }
*/
    free(line);
    free(text);
    return item;
}

