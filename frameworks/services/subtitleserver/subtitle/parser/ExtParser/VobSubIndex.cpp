#define LOG_TAG "VobSubIndex"

#include <cutils/properties.h>
#include "VobSubIndex.h"
#include "VideoInfo.h"
#define MIN(a, b)    ((a)<(b)?(a):(b))
#define MAX(a, b)    ((a)>(b)?(a):(b))
#define UINT_MAX 0xFFFFFFFFFFFFFFFLL


static int rar_eof(rar_stream_t *stream) {
    return stream->pos >= stream->size;
}

static long rar_tell(rar_stream_t *stream)
{
    return stream->pos;
}

static int rar_seek(rar_stream_t *stream, long offset, int whence) {
    switch (whence)
    {
        case SEEK_SET:
            if (offset < 0)
            {
                //        errno = EINVAL;
                return -1;
            }
            stream->pos = offset;
            break;
        case SEEK_CUR:
            if (offset < 0 && stream->pos < (unsigned long) - offset)
            {
                //        errno = EINVAL;
                return -1;
            }
            stream->pos += offset;
            break;
        case SEEK_END:
            if (offset < 0 && stream->size < (unsigned long) - offset)
            {
                //        errno = EINVAL;
                return -1;
            }
            stream->pos = stream->size + offset;
            break;
        default:
            //  errno = EINVAL;
            return -1;
    }
    if (stream->fd >= 0)
        lseek(stream->fd, stream->pos, SEEK_SET);
    return 0;
}

static int rar_getc(rar_stream_t *stream)
{
    unsigned char ch;
    int ret_ch = EOF;
    if (stream->fd >= 0)
    {
        if (read(stream->fd, &ch, 1) > 0)
        {
            stream->pos++;
            ret_ch = ch;
        }
    }
    return ret_ch;
}

static size_t
rar_read(void *ptr, size_t size, size_t nmemb, rar_stream_t *stream)
{
    size_t res;
    int remain;
    //when play agin maybe it can: stream->fd=0;
    if (stream->fd >= 0)
    {
        if (rar_eof(stream))
            return 0;
        res = size * nmemb;
        remain = stream->size - stream->pos;
        if (res > remain)
            res = remain / size * size;
        read(stream->fd, ptr, res);
        stream->pos += res;
        res /= size;
        return res;
    }
    else
        return 0;
}

/**********************************************************************
 * MPEG parsing
 **********************************************************************/

static mpeg_t *mpeg_open(int fd) {
    mpeg_t *res = (mpeg_t *)calloc(sizeof(mpeg_t), 1);
    rar_stream_t *stream = (rar_stream_t *)malloc(sizeof(rar_stream_t));
    int err = res == NULL;
    if (!err) {
        res->pts = 0;
        res->aid = -1;
        res->packet = NULL;
        res->packet_size = 0;
        //res->packet_reserve = 0;
        res->stream = stream;

        stream->fd = fd;
        if (stream->fd >= 0) {
            stream->pos = 0;
            stream->size = lseek(stream->fd, 0, SEEK_END);
            lseek(stream->fd, 0, SEEK_SET);
        }

        err = res->stream == NULL;
        if (err)
            ALOGI("fopen Vobsub file failed");
        if (err)
            free(res);
    }
    return err ? NULL : res;
}

static void mpeg_free(mpeg_t *mpeg) {
    if (mpeg->packet)
        free(mpeg->packet);
    if (mpeg->stream) {
        free(mpeg->stream);
        mpeg->stream = nullptr;
    }
    free(mpeg);
}

static int mpeg_eof(mpeg_t *mpeg) {
    return rar_eof(mpeg->stream);
}

static off_t mpeg_tell(mpeg_t *mpeg) {
    return rar_tell(mpeg->stream);
}

static int mpeg_run(mpeg_t *mpeg, char read_flag) {
    unsigned int len, idx, version;
    int c;
    /* Goto start of a packet, it starts with 0x000001?? */
    const unsigned char wanted[] = { 0, 0, 1 };
    unsigned char buf[5];
    mpeg->aid = -1;
    mpeg->packet_size = 0;
    if (rar_read(buf, 4, 1, mpeg->stream) != 1)
        return -1;

    while (memcmp(buf, wanted, sizeof(wanted)) != 0) {
        c = rar_getc(mpeg->stream);
        if (c < 0)
            return -1;
        memmove(buf, buf + 1, 3);
        buf[3] = c;
    }

    //ALOGD("mpeg_run: %x %x %x [%x] %x", buf[0], buf[1], buf[2], buf[3], buf[4]);
    switch (buf[3]) {
        case 0xb9:      /* System End Code */
            break;
        case 0xba:      /* Packet start code */
            c = rar_getc(mpeg->stream);
            if (c < 0)
                return -1;
            if ((c & 0xc0) == 0x40)
                version = 4;
            else if ((c & 0xf0) == 0x20)
                version = 2;
            else {
                ALOGE("VobSub: Unsupported MPEG version: 0x%02x\n", c);
                return -1;
            }
            if (version == 4) {
                if (rar_seek(mpeg->stream, 9, SEEK_CUR))
                    return -1;
            } else if (version == 2) {
                if (rar_seek(mpeg->stream, 7, SEEK_CUR))
                    return -1;
            }
            else
                abort();
            break;
        case 0xbd:      /* packet */
            if (rar_read(buf, 2, 1, mpeg->stream) != 1)
                return -1;
            len = buf[0] << 8 | buf[1];
            idx = mpeg_tell(mpeg);
            c = rar_getc(mpeg->stream);
            if (c < 0)
                return -1;
            if ((c & 0xC0) == 0x40) {    /* skip STD scale & size */
                if (rar_getc(mpeg->stream) < 0)
                    return -1;
                c = rar_getc(mpeg->stream);
                if (c < 0)
                    return -1;
            }
            if ((c & 0xf0) == 0x20) {    /* System-1 stream timestamp */
                /* Do we need this? */
                abort();
            } else if ((c & 0xf0) == 0x30) {
                /* Do we need this? */
                abort();
            } else if ((c & 0xc0) == 0x80) {       /* System-2 (.VOB) stream */
                unsigned int pts_flags, hdrlen, dataidx;
                c = rar_getc(mpeg->stream);
                if (c < 0)
                    return -1;
                pts_flags = c;
                c = rar_getc(mpeg->stream);
                if (c < 0)
                    return -1;
                hdrlen = c;
                dataidx = mpeg_tell(mpeg) + hdrlen;
                if (dataidx > idx + len) {
                    ALOGE("Invalid header length: %d (total length: %d, idx: %d, dataidx: %d)\n",
                                        hdrlen, len, idx, dataidx);
                    return -1;
                }
                if ((pts_flags & 0xc0) == 0x80) {
                    if (rar_read(buf, 5, 1, mpeg->stream) != 1)
                        return -1;
                    if (!(((buf[0] & 0xf0) == 0x20) && (buf[0] & 1) && (buf[2] & 1) && (buf[4] & 1))) {
                        ALOGE("vobsub PTS error: 0x%02x %02x%02x %02x%02x \n",
                                                    buf[0], buf[1], buf[2], buf[3], buf[4]);
                        mpeg->pts = 0;
                    } else
                        mpeg->pts = ((buf[0] & 0x0e) << 29 | buf[1] << 22 | (buf[2] & 0xfe) << 14 | buf[3] << 7 | (buf[4] >> 1));
                } else {       /* if ((pts_flags & 0xc0) == 0xc0) */
                    /* what's this? */
                    /* abort(); */
                }
                rar_seek(mpeg->stream, dataidx, SEEK_SET);
                mpeg->aid = rar_getc(mpeg->stream);
                if (mpeg->aid < 0) {
                    ALOGE("Bogus aid %d\n", mpeg->aid);
                    return -1;
                }
                mpeg->packet_size = len - ((unsigned int)mpeg_tell(mpeg) - idx);
                //ALOGD("package size: %d",  mpeg->packet_size);
                if (read_flag) {
                    //if (mpeg->packet_reserve < mpeg->packet_size) {
                    if (mpeg->packet) free(mpeg->packet);
                    mpeg->packet = (unsigned char *)malloc(mpeg->packet_size);
                    //if (mpeg->packet)
                    //    mpeg->packet_reserve = mpeg->packet_size;
                    //}
                    if (mpeg->packet == NULL) {
                        ALOGE("malloc failure");
                        //mpeg->packet_reserve = 0;
                        mpeg->packet_size = 0;
                        return -1;
                    }
                    if (rar_read(mpeg->packet, mpeg->packet_size, 1, mpeg->stream) != 1) {
                        ALOGE("fread failure");
                        mpeg->packet_size = 0;
                        return -1;
                    }
                } else {
                    rar_seek(mpeg->stream, mpeg->packet_size,
                             SEEK_CUR);
                }
                idx = len;
            }
            break;
        case 0xbe:      /* Padding */
            if (rar_read(buf, 2, 1, mpeg->stream) != 1)
                return -1;
            len = buf[0] << 8 | buf[1];
            if (len > 0 && rar_seek(mpeg->stream, len, SEEK_CUR))
                return -1;
            break;
        default:
            if (0xc0 <= buf[3] && buf[3] < 0xf0) {
                /* MPEG audio or video */
                if (rar_read(buf, 2, 1, mpeg->stream) != 1)
                    return -1;
                len = buf[0] << 8 | buf[1];
                if (len > 0 && rar_seek(mpeg->stream, len, SEEK_CUR))
                    return -1;
            } else {
                ALOGE("unknown header 0x%02X%02X%02X%02X\n",
                                buf[0], buf[1], buf[2], buf[3]);
                return -1;
            }
    }
    return 0;
}
static unsigned short DecodeRL(unsigned short RLData, unsigned short *pixelnum,
                               unsigned short *pixeldata)
{
    unsigned short nData = RLData;
    unsigned short nShiftNum;
    unsigned short nDecodedBits;
    if (nData & 0xc000)
        nDecodedBits = 4;
    else if (nData & 0x3000)
        nDecodedBits = 8;
    else if (nData & 0x0c00)
        nDecodedBits = 12;
    else
        nDecodedBits = 16;
    nShiftNum = 16 - nDecodedBits;
    *pixeldata = (nData >> nShiftNum) & 0x0003;
    *pixelnum = nData >> (nShiftNum + 2);
    return nDecodedBits;
}

static inline unsigned short doDCSQC(unsigned char *pdata, unsigned char *pend) {
    unsigned short cmdDelay, cmdDelaynew;
    unsigned short temp;
    unsigned short cmdAddress;
    int Done, stoped;

    cmdDelay = *pdata++;
    cmdDelay <<= 8;
    cmdDelay += *pdata++;
    cmdAddress = *pdata++;
    cmdAddress <<= 8;
    cmdAddress += *pdata++;
    cmdDelaynew = 0;
    Done = 0;
    stoped = 0;
    while (!Done) {
        switch (*pdata) {
            case FSTA_DSP:
                pdata++;
                break;
            case STA_DSP:
                pdata++;
                break;
            case STP_DSP:
                pdata++;
                stoped = 1;
                break;
            case SET_COLOR:
                pdata += 3;
                break;
            case SET_CONTR:
                pdata += 3;
                break;
            case SET_DAREA:
                pdata += 7;
                break;
            case SET_DSPXA:
                pdata += 7;
                break;
            case CHG_COLCON:
                temp = *pdata++;
                temp = temp << 8;
                temp += *pdata++;
                pdata += temp;
                break;
            case CMD_END:
                pdata++;
                Done = 1;
                break;
            default:
                pdata = pend;
                Done = 1;
                break;
        }
    }
    if ((pdata < pend) && (stoped == 0))
        cmdDelaynew = doDCSQC(pdata, pend);
    return cmdDelaynew > cmdDelay ? cmdDelaynew : cmdDelay;
}

static unsigned short GetWordFromPixBuffer(char *buffer, unsigned short bitpos) {
    unsigned char hi = 0, lo = 0, hi_ = 0, lo_ = 0;
    char *tmp = buffer;
    hi = *(tmp + 0);
    lo = *(tmp + 1);
    hi_ = *(tmp + 2);
    lo_ = *(tmp + 3);
    if (bitpos == 0) {
        return (hi << 0x8 | lo);
    } else {
        return (((hi << 0x8 | lo) << bitpos) |
                ((hi_ << 0x8 | lo_) >> (16 - bitpos)));
    }
}


static int vobsub_parse_delay(const char *line) {
    int h, m, s, ms;
    int forward = 1;
    if (*(line + 7) == '+')  {
        forward = 1;
        line++;
    } else if (*(line + 7) == '-') {
        forward = -1;
        line++;
    }

    h = atoi(line + 7);
    m = atoi(line + 10);
    s = atoi(line + 13);
    ms = atoi(line + 16);
    return (ms + 1000 * (s + 60 * (m + 60 * h))) * forward;
}

static int vobsub_parse_palette(vobsub_t *vob, const char *line) {
    // palette: XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX
    unsigned int n;
    n = 0;
    while (1) {
        const char *p;
        int r, g, b, y, u, v, tmp;

        while (isspace(*line))
            ++line;
        p = line;

        while (isxdigit(*p))
            ++p;
        if (p - line != 6)
            return -1;

        tmp = strtoul(line, NULL, 16);
        r = tmp >> 16 & 0xff;
        g = tmp >> 8 & 0xff;
        b = tmp & 0xff;
        y = MIN(MAX((int)(0.1494 * r + 0.6061 * g + 0.2445 * b), 0), 0xff);
        u = MIN(MAX((int)(0.6066 * r - 0.4322 * g - 0.1744 * b) + 128, 0), 0xff);
        v = MIN(MAX((int)(-0.08435 * r - 0.3422 * g + 0.4266 * b) + 128, 0), 0xff);
        //vob->palette[n++] = y << 16 | u << 8 | v;
        vob->palette[n++]=tmp;
        if (n == 16)
            break;

        if (*p == ',')
            ++p;

        line = p;
    }
    vob->have_palette = 1;
    return 0;
}

static int vobsub_parse_cuspal(vobsub_t *vob, const char *line) {
    //colors: XXXXXX, XXXXXX, XXXXXX, XXXXXX
    unsigned int n;
    n = 0;
    line += 40;
    while (1)
    {
        const char *p;
        while (isspace(*line))
            ++line;
        p = line;
        while (isxdigit(*p))
            ++p;
        if (p - line != 6)
            return -1;
        vob->cuspal[n++] = strtoul(line, NULL, 16);
        if (n == 4)
            break;
        if (*p == ',')
            ++p;
        line = p;
    }
    return 0;
}

/* don't know how to use tridx */
static int vobsub_parse_tridx(const char *line)
{
    //tridx: XXXX
    int tridx;
    tridx = strtoul((line + 26), NULL, 16);
    tridx =
        ((tridx & 0x1000) >> 12) | ((tridx & 0x100) >> 7) | ((tridx & 0x10)
                >> 2) | ((tridx
                          & 1)
                         << 3);
    return tridx;
}

static int vobsub_parse_custom(vobsub_t *vob, const char *line)
{
    //custom colors: OFF/ON(0/1)
    if ((strncmp("ON", line + 15, 2) == 0)
            || strncmp("1", line + 15, 1) == 0)
        vob->custom = 1;
    else if ((strncmp("OFF", line + 15, 3) == 0)
             || strncmp("0", line + 15, 1) == 0)
        vob->custom = 0;
    else
        return -1;
    return 0;
}

#define dump_data 0

VobSubIndex::VobSubIndex(std::shared_ptr<DataSource> source): TextSubtitle(source) {
    mSubFd = source->getExtraFd();

    if (mSubFd <= 0) {
        ALOGE("Error! no subtitle .sub files!");
    }

    mpg = mpeg_open(mSubFd);
}




VobSubIndex::~VobSubIndex() {

}

std::shared_ptr<ExtSubItem> VobSubIndex::decodedItem() {
    char *line = (char *)malloc(LINE_LEN + 1);
    char *p = NULL;
    int i, len;

    if (line == nullptr) return nullptr;

    if (mIdxSubTrackId >= 0) {
        mSelectedTrackId = mIdxSubTrackId;
    }

    while (mReader->getLine(line)) {

        ALOGD("%s", line);
        if (*line == 0 || *line == '\r' || *line == '\n' || *line == '#') {
            continue;
        }

        //ALOGD(">> %s [%c %c %c %c] %d", line, line[0], line[1], line[2], line[3], strncmp("timestamp:", line, 10));
        if (strncmp("langidx:", line, 8) == 0) {
            vobsubId = atoi(line + 8);
            // use the default idx for playing
            if (mIdxSubTrackId == -1) {
                mSelectedTrackId = vobsubId;
            }
        } else if (strncmp("delay:", line, 6) == 0) {
            mDelay = vobsub_parse_delay(line);
        } else if (strncmp("id:", line, 3) == 0) {
            char lang[16]; // todo handle lang

            sscanf(line,"id: %2s, index: %d", lang, &vobsubId);
            ALOGD("\n\n\n\n\n\n\n %s  vobsubId=%d\n\n\n\n\n", line, vobsubId);
            // No select, select the first we encounter.
            if (mIdxSubTrackId == -1) {
                mSelectedTrackId = mIdxSubTrackId = vobsubId;
            }

        } else if (strncmp("palette:", line, 8) == 0) {
            vobsub_parse_palette(&mVobParam, line + 8);
        } else if (strncmp("size:", line, 5) == 0) {
            sscanf(line,"size: %dx%d", &mOrigFrameWidth, &mOrigFrameHeight);
        } else if (strncmp("org:", line, 4) == 0) {
            sscanf(line,"org: %d, %d", &mOriginX, &mOriginY);
        } else if (strncmp("timestamp:", line, 10) == 0) {
            //res = vobsub_parse_timestamp(vob, line + 10);
            // timestamp: HH:MM:SS.mmm, filepos: 0nnnnnnnnn
            int hour=0, min=0, sec=0, ms=0;
            long long pos=0;
            if (sscanf(line, "timestamp: %d:%d:%d:%d, filepos:%llx",
                    &hour, &min, &sec, &ms, &pos) != 5) {
                //continue;
            }
            //ALOGD("%d %d %d %d %llx", hour, min, sec, ms, pos);

            if (vobsubId != mSelectedTrackId) {
                ALOGD("cur:%d sel:%d ignore", vobsubId, mSelectedTrackId);
                continue;
            }
            std::shared_ptr<ExtSubItem> item = std::shared_ptr<ExtSubItem>(new ExtSubItem());

            // TOD: currently, only debug:
           // if (vobsubId != 0) continue;
            // TODO: tune delay
            item->start = (hour * 60 * 60 + min * 60 + sec) * 100 + ms/10;
            item->subId = vobsubId;
            item->filePos = pos;
            // TODO: tune end

            free(line);
            return item;

        } else if (strncmp("custom colors:", line, 14) == 0) {
            //custom colors: ON/OFF, tridx: XXXX, colors: XXXXXX, XXXXXX, XXXXXX,XXXXXX
            vobsub_parse_cuspal(&mVobParam, line);
            vobsub_parse_tridx(line);
            vobsub_parse_custom(&mVobParam, line);
        } else if (strncmp("forced subs:", line, 12) == 0) {
            //res = vobsub_parse_forced_subs(vob, line + 12);
        } else {
            continue;
        }

    }
    free(line);
    return nullptr;
}



/* consume subtitle */
std::shared_ptr<AML_SPUVAR> VobSubIndex::popDecodedItem() {
    if (totalItems() <= 0) {
        return nullptr;
    }

    std::shared_ptr<ExtSubItem> item = nullptr;


    // should not use the default one.
    // add functor when needed
    for (auto it = mSubData.subtitles.begin(); it != mSubData.subtitles.end(); it++) {
        // TODO: add sub select
        if ((*it)->subId  == mSelectedTrackId) {
            item = *it;
            mSubData.subtitles.erase(it);
            break;
        }
    }

    if (item ==  nullptr) return nullptr;

//    ALOGD("%s item:[%lld %lld]", __func__, item->start, item->end);
    std::shared_ptr<AML_SPUVAR> spu(new AML_SPUVAR());

    spu->pts = item->start;
    spu->m_delay = item->end;

    // gen Bitmap dynamically, assign functor here
    spu->dynGen = true;
    spu->pos = item->filePos;
    spu->isExtSub = true;
    // TODO: construct render and data for sub-idx
    //spu->m_delay = item->start + getDuration(spu->pos, vobsubId)/90; // units are 90KHz clock

    using std::placeholders::_1;
    using std::placeholders::_2;
    spu->genMethod =   std::bind(&VobSubIndex::genSubBitmap, this, _1, _2);

    return spu;
}

unsigned char VobSubIndex::vob_fill_pixel(unsigned char *rawData, unsigned char *outData, int n) {
    unsigned short nPixelNum = 0, nPixelData = 0;
    unsigned short nRLData, nBits;
    unsigned short nDecodedPixNum = 0;
    unsigned short i, j;
    unsigned short rownum = VobSPU.spu_width;
    unsigned short _alpha = VobSPU.spu_alpha;
    unsigned short PXDBufferBitPos = 0, WrOffset = 16;
    //  unsigned short totalBits = 0;
    unsigned short change_data = 0;
    unsigned short PixelDatas[4] = { 0, 1, 2, 3 };
    unsigned short *vob_ptrPXDWrite;

    unsigned short *vob_ptrPXDRead;

    // 4 buffer for pix data
    if (n == 1) {    // 1 for odd
        vob_ptrPXDRead = (unsigned short *)((unsigned long)(rawData) + VobSPU.top_pxd_addr);

        memset(outData, 0, OSD_HALF_SIZE);
        vob_ptrPXDWrite = (unsigned short *)outData;
    } else if (n == 2) {    // 2 for even
        vob_ptrPXDRead = (unsigned short *)((unsigned long)(rawData) + VobSPU.bottom_pxd_addr);

        memset(outData + OSD_HALF_SIZE, 0, OSD_HALF_SIZE);
        vob_ptrPXDWrite =  (unsigned short *)(outData + OSD_HALF_SIZE);
    } else {
        return -1;
    }

    if (_alpha & 0xF)  {
        _alpha = _alpha >> 4;
        change_data++;
        while (_alpha & 0xF) {
            change_data++;
            _alpha = _alpha >> 4;
        }
        PixelDatas[0] = change_data;
        PixelDatas[change_data] = 0;
        if (n == 2)
            VobSPU.spu_alpha = (VobSPU.spu_alpha & 0xFFF0) | (0x000F << (change_data << 2));
    }

    for (j = 0; j < VobSPU.spu_height/2; j++) {
        while (nDecodedPixNum < rownum) {
            nRLData = GetWordFromPixBuffer((char *)vob_ptrPXDRead, PXDBufferBitPos);
            nBits = DecodeRL(nRLData, &nPixelNum, &nPixelData);
            PXDBufferBitPos += nBits;
            if (PXDBufferBitPos >= 16) {
                PXDBufferBitPos -= 16;
                vob_ptrPXDRead++;
            }

            if (nPixelNum == 0) {
                nPixelNum = rownum - nDecodedPixNum % rownum;
            }

            if (change_data) {
                nPixelData = PixelDatas[nPixelData];
            }

            for (i = 0; i < nPixelNum; i++) {
                WrOffset -= 2;
                *vob_ptrPXDWrite |= nPixelData << WrOffset;
                if (WrOffset == 0) {
                    WrOffset = 16;
                    vob_ptrPXDWrite++;
                }
            }
            //          totalBits += nBits;
            nDecodedPixNum += nPixelNum;
        }

        if (PXDBufferBitPos == 4)   //Rule 6
        {
            PXDBufferBitPos = 8;
        } else if (PXDBufferBitPos == 12) {
            PXDBufferBitPos = 0;
            vob_ptrPXDRead++;
        }
        if (WrOffset != 16)
        {
            WrOffset = 16;
            vob_ptrPXDWrite++;
        }
        nDecodedPixNum -= rownum;
    }
    /*if (totalBits == subpichdr->field_offset){
       return 1;
       }
     */
    return 0;
}

unsigned int VobSubIndex::getDuration(int64_t pos, int trackId) {
    if (rar_seek(mpg->stream, pos, SEEK_SET) == 0) {
        while (!mpeg_eof(mpg)) {
            if (mpeg_run(mpg, 1) < 0) {
                if (!mpeg_eof(mpg))
                    ALOGE("VobSub: mpeg_run error\n");
                break;
            }

            if (mpg->packet_size == 0 )  {
                continue;
            }
            if ((mpg->packet) && ((mpg->aid & 0xe0) == 0x20)
                    && ((mpg->aid & 0x1f) == trackId)) {
                unsigned char *rawsubdata, *subdata_ptr;
                int sublen, len;
                sublen = (mpg-> packet[0] << 8) | (mpg->packet[1]);
                rawsubdata = (unsigned char *)malloc(sublen);
                subdata_ptr =rawsubdata;
                if (!rawsubdata) {
                    ALOGE("Error! out of memory!!");
                    return 0;
                }

                len = mpg->packet_size;
                if (len > sublen)
                    len = sublen;
                memcpy(rawsubdata, mpg->packet, len);
                free(mpg->packet);
                mpg->packet = NULL;
                sublen -= len;
                subdata_ptr += len;
                ALOGD("sublen=%d", sublen);
                while ((!mpeg_eof(mpg)) && (sublen > 0)) {
                    if (mpeg_run(mpg, 1) < 0) {
                        if (!mpeg_eof(mpg)) {
                            ALOGE("VobSub: mpeg_run error\n");
                            break;
                        }
                    }
                    if (mpg->packet_size) {
                        if ((mpg->packet) && ((mpg->aid & 0xe0) == 0x20)
                            && ((mpg->aid & 0x1f) == trackId)) {
                            len = mpg->packet_size;
                            if (len > sublen)
                                len = sublen;
                            memcpy (subdata_ptr, mpg->packet, len);
                            sublen -= len;
                            subdata_ptr += len;
                            free(mpg->packet);
                            mpg->packet = NULL;
                        } else {
                            free(mpg->packet);
                            mpg->packet = NULL;
                            break;
                        }
                    }
                }
                duration = 0;
                do_vob_sub_cmd(rawsubdata);
                free(rawsubdata);
                break;
            }
        }
    }
    return duration;
}

unsigned char *VobSubIndex::genSubBitmap(AML_SPUVAR *spu, size_t *size) {
    unsigned char *bitmap = nullptr;
    ALOGD("genSubBitmap! pts:%lld pos:%lld\n\n\n", spu->pts, spu->pos);
    memset(&VobSPU, 0, sizeof(VobSPU));

    if (rar_seek(mpg->stream, spu->pos, SEEK_SET) == 0) {
        while (!mpeg_eof(mpg)) {
            if (mpeg_run(mpg, 1) < 0) {
                if (!mpeg_eof(mpg))
                    ALOGE("VobSub: mpeg_run error\n");
                    break;
            }

            ALOGD("seek to %d %lld package_size:%d ", mpg->stream->fd, spu->pos, mpg->packet_size);

            if (mpg->packet_size)  {
                // initialize, current sub duration.
                duration = 0;

                if ((mpg->packet) && ((mpg->aid & 0xe0) == 0x20)
                        && ((mpg->aid & 0x1f) == mSelectedTrackId) /*subtitlevobsub->cur_track_id) ?*/
                        ) {
                    unsigned char *rawsubdata, *subdata_ptr;
                    int sublen, len;
                    sublen = (mpg-> packet[0] << 8) | (mpg->packet[1]);
                    rawsubdata = (unsigned char *)malloc(sublen);
                    subdata_ptr =rawsubdata;
                    if (rawsubdata) {
                        len = mpg->packet_size;
                        if (len > sublen)
                            len = sublen;
                        memcpy(rawsubdata, mpg->packet, len);
                        free(mpg->packet);
                        mpg->packet = NULL;
                        sublen -= len;
                        subdata_ptr += len;
                        ALOGD("sublen=%d", sublen);
                        while ((!mpeg_eof(mpg)) && (sublen > 0)) {
                            if (mpeg_run(mpg, 1) < 0) {
                                if (!mpeg_eof(mpg)) {
                                    ALOGE("VobSub: mpeg_run error\n");
                                    break;
                                }
                            }
                            if (mpg->packet_size) {
                                if ((mpg->packet) && ((mpg->aid & 0xe0) == 0x20)
                                    && ((mpg->aid & 0x1f) == mSelectedTrackId)
                                    //&& ((mpg->aid & 0x1f) == subtitlevobsub->cur_track_id) ?
                                    )  {
                                    len = mpg->packet_size;
                                    if (len > sublen)
                                        len = sublen;
                                    memcpy (subdata_ptr, mpg->packet, len);
                                    sublen -= len;
                                    subdata_ptr += len;
                                    free(mpg->packet);
                                    mpg->packet = NULL;
                                } else {
                                    free(mpg->packet);
                                    mpg->packet = NULL;
                                    break;
                                }
                            }
                        }
                        if (do_vob_sub_cmd(rawsubdata) == SUCCESS) {
                            ALOGD("do_vob_sub_cmd success");
                            unsigned char *vob_pixData = (unsigned char *)malloc(OSD_HALF_SIZE*2);

                            if (vob_pixData) {
                                //subtitlevobsub->vob_ptrPXDRead  = (unsigned long *)((unsigned long)(rawsubdata) + VobSPU.top_pxd_addr);
                                //vob_fill_pixel(subtitlevobsub, 1);  // 1 for odd, 2 for even
                                //subtitlevobsub->vob_ptrPXDRead = (unsigned long *)((unsigned long)(rawsubdata) + VobSPU.bottom_pxd_addr);
                                //vob_fill_pixel(subtitlevobsub, 2);  // 1 for odd, 2 for even
                                //show_vob_subtitle(subtitlevobsub);
                                vob_fill_pixel(rawsubdata, vob_pixData, 1);  // 1 for odd, 2 for even
                                vob_fill_pixel(rawsubdata, vob_pixData, 2);  // 1 for odd, 2 for even
                                //ret = 1;

                                VobSPU.spu_width = (((VobSPU.spu_width + 7) >> 3) << 3); // check this ok or not??

                                // RGBA color
                                bitmap = (unsigned char *)malloc(VobSPU.spu_height*VobSPU.spu_width * 4);
                                int rawByte = VobSPU.spu_height *VobSPU.spu_width / 4;  //byte

                                convert2bto32b(vob_pixData, rawByte, VobSPU.spu_width/4, (unsigned int *)bitmap, VobSPU.spu_alpha);

                                 spu->spu_alpha = VobSPU.spu_alpha;
                                 spu->spu_color = VobSPU.spu_color;
                                 spu->buffer_size = VobSPU.spu_height*VobSPU.spu_width * 4;
                                 spu->spu_data = bitmap;
                                 spu->useMalloc = true;
                                 spu->spu_start_x = VobSPU.spu_start_x;
                                 spu->spu_start_y = VobSPU.spu_start_y;
                                 spu->spu_height = VobSPU.spu_height;
                                 spu->spu_width = VobSPU.spu_width;
                                 spu->m_delay = spu->pts + duration;

                                spu->spu_origin_display_w = mOrigFrameWidth;
                                spu->spu_origin_display_h = mOrigFrameHeight;

                                if (spu->spu_origin_display_w <= 0 || spu->spu_origin_display_h <= 0) {
                                    spu->spu_origin_display_w = VideoInfo::Instance()->getVideoWidth();
                                    spu->spu_origin_display_h = VideoInfo::Instance()->getVideoHeight();
                                }

                                ALOGD("get subtitle ---------------vob_pixData:%p spu:%p spu_data:%p\n", vob_pixData, spu, spu->spu_data);
                                free(vob_pixData);
                            }
                        }
                        free(rawsubdata);
                    }
                }

                break;
            }

        }
    }
    return bitmap;
}

int VobSubIndex::do_vob_sub_cmd(unsigned char *packet) {
    unsigned short m_OffsetToCmd;
    unsigned short m_SubPicSize;
    unsigned short temp;
    unsigned char *pCmdData;
    unsigned char *pCmdEnd;
    unsigned char data_byte0, data_byte1;
    unsigned char spu_cmd;
    m_SubPicSize = (packet[0] << 8) | (packet[1]);
    m_OffsetToCmd = (packet[2] << 8) | (packet[3]);
    if (m_OffsetToCmd >= m_SubPicSize)
        return FAIL;
    pCmdData = packet;
    pCmdEnd = pCmdData + m_SubPicSize;
    pCmdData += m_OffsetToCmd;
    pCmdData += 4;
    while (pCmdData < pCmdEnd)
    {
        spu_cmd = *pCmdData++;
        ALOGD("cmd: %d", spu_cmd);
        switch (spu_cmd)
        {
            case FSTA_DSP:
                VobSPU.display_pending = 2;
                break;
            case STA_DSP:
                VobSPU.display_pending = 1;
                break;
            case STP_DSP:
                VobSPU.display_pending = 0;
                break;
            case SET_COLOR:
                temp = *pCmdData++;
                VobSPU.spu_color = temp << 8;
                temp = *pCmdData++;
                VobSPU.spu_color += temp;
                break;
            case SET_CONTR:
                temp = *pCmdData++;
                VobSPU.spu_alpha = temp << 8;
                temp = *pCmdData++;
                VobSPU.spu_alpha += temp;
                break;
            case SET_DAREA:
                data_byte0 = *pCmdData++;
                data_byte1 = *pCmdData++;
                VobSPU.spu_start_x =
                    ((data_byte0 & 0x3f) << 4) | (data_byte1 >> 4);
                data_byte0 = *pCmdData++;
                VobSPU.spu_width = ((data_byte1 & 0x07) << 8) | (data_byte0);
                VobSPU.spu_width = VobSPU.spu_width - VobSPU.spu_start_x + 1;
                data_byte0 = *pCmdData++;
                data_byte1 = *pCmdData++;
                VobSPU.spu_start_y = ((data_byte0 & 0x3f) << 4) | (data_byte1 >> 4);
                data_byte0 = *pCmdData++;
                VobSPU.spu_height = ((data_byte1 & 0x07) << 8) | (data_byte0);
                VobSPU.spu_height = VobSPU.spu_height - VobSPU.spu_start_y + 1;
#if 0
                if ((VobSPU.spu_width > 720) ||
                        (VobSPU.spu_height > 576)
                   )
                {
                    VobSPU.spu_width = 720;
                    VobSPU.spu_height = 576;
                }
#endif
                break;
            case SET_DSPXA:
                temp = *pCmdData++;
                VobSPU.top_pxd_addr = temp << 8;
                temp = *pCmdData++;
                VobSPU.top_pxd_addr += temp;
                temp = *pCmdData++;
                VobSPU.bottom_pxd_addr = temp << 8;
                temp = *pCmdData++;
                VobSPU.bottom_pxd_addr += temp;
                break;
            case CHG_COLCON:
                temp = *pCmdData++;
                temp = temp << 8;
                temp += *pCmdData++;
                pCmdData += temp;
                /*
                   VobSPU.disp_colcon_addr = VobSPU.point + VobSPU.point_offset;
                   VobSPU.colcon_addr_valid = 1;
                   temp = VobSPU.disp_colcon_addr + temp - 2;

                   uSPU.point = temp & 0x1fffc;
                   uSPU.point_offset = temp & 3;
                 */
                break;

            case CMD_END:
                if (pCmdData <= (pCmdEnd - 6)) {
                    duration = doDCSQC(pCmdData, pCmdEnd - 6);
                    if (duration > 0)
                        duration *= 1024;
                } else {
                   duration = 0;
                }

                ALOGD("spu_alpha:%d spu_color:0x%x p:%d spu_start(%d %d) width:%d height:%d top_pxd_addr:%x bottom_pxd_addr:%x",
                    VobSPU.spu_alpha, VobSPU.spu_color, VobSPU.display_pending, VobSPU.spu_start_x, VobSPU.spu_start_y,
                    VobSPU.spu_width, VobSPU.spu_height, VobSPU.top_pxd_addr, VobSPU.bottom_pxd_addr);

                ALOGD("duration:%d", duration);
                return SUCCESS;

            default:
                return FAIL;
        }
    }
    return FAIL;
}

void VobSubIndex::convert2bto32b(const unsigned char *source, long length, int bytesPerLine,
                   unsigned int *dist, int subtitle_alpha)
{
    if (dist == NULL) {
        return;
    }

    char value[PROPERTY_VALUE_MAX] = { 0 };
    unsigned int RGBA_Pal[4];
    RGBA_Pal[0] = RGBA_Pal[1] = RGBA_Pal[2] = RGBA_Pal[3] = 0;
    int aAlpha[4];
    int aPalette[4];
    int rgb0 = 0;
    int rgb1 = 0xffffff;
    int rgb2 = 0;
    int rgb3 = 0;
    int set_rgb = 0;
    /*  update Alpha */
    aAlpha[1] = ((subtitle_alpha >> 8) >> 4) & 0xf;
    aAlpha[0] = (subtitle_alpha >> 8) & 0xf;
    aAlpha[3] = (subtitle_alpha >> 4) & 0xf;
    aAlpha[2] = subtitle_alpha & 0xf;
    /* update Palette */
    aPalette[0] = ((VobSPU.spu_color >> 8) >> 4) & 0xf;
    aPalette[1] = (VobSPU.spu_color >> 8) & 0xf;
    aPalette[2] = (VobSPU.spu_color >> 4) & 0xf;
    aPalette[3] = VobSPU.spu_color & 0xf;


/*
 The tridx: is used to turn off/on the four colors. 1-off, 0=on
 The first digit is for the background color ( always set to 1 unless you
 desire an ugly border )
 The second digit is for the subtitle text color
 The third digit is for the shadow or outline color
 The fourth digit is for a second layer of shadow or outline color ( some
 DVDs don't use it )

 */

    aAlpha[0] = 1; // background need transparent

    RGBA_Pal[0] = ((aAlpha[0] == 0) ? 0xff000000 : 0x0) + mVobParam.palette[aPalette[0]-1];
    RGBA_Pal[1] = ((aAlpha[1] == 0) ? 0xff000000 : 0x0) + mVobParam.palette[aPalette[1]-1];
    RGBA_Pal[2] = ((aAlpha[2] == 0) ? 0xff000000 : 0x0) + mVobParam.palette[aPalette[2]-1];
    RGBA_Pal[3] = ((aAlpha[3] == 0) ? 0xff000000 : 0x0) + mVobParam.palette[aPalette[3]-1];

    static int k = 0;
     char name[1024];

#if dump_data
        sprintf(name, "/sdcard/subfrom-%d", k);
        int fd =open(name, O_WRONLY|O_CREAT);
        if (fd == NULL) {
          ALOGE("fd ================NULL");
        }
        long bytes = write(fd, source,length );
        close(fd);

      ALOGE("write bytes %d  / %d ",bytes, length);
#endif

    for (int i = 0; i < length; i += 2) {
        const unsigned char *sourcemodify;

        int linenumber = i / bytesPerLine;
        if (linenumber & 1) {
            //sourcemodify=source+(720*576/8);
            sourcemodify = source + OSD_HALF_SIZE;
        } else {
            sourcemodify = source;
        }
        unsigned char a = sourcemodify[(linenumber / 2) * bytesPerLine + i % bytesPerLine];
        unsigned char b = sourcemodify[(linenumber / 2) * bytesPerLine + i % bytesPerLine + 1];
        int j = i * 4;
        dist[j] = RGBA_Pal[(b & 0xc0) >> 6];
        dist[j + 1] = RGBA_Pal[(b & 0x30) >> 4];
        dist[j + 2] = RGBA_Pal[(b & 0x0c) >> 2];
        dist[j + 3] = RGBA_Pal[(b & 0x03)];
        dist[j + 4] = RGBA_Pal[(a & 0xc0) >> 6];
        dist[j + 5] = RGBA_Pal[(a & 0x30) >> 4];
        dist[j + 6] = RGBA_Pal[(a & 0x0c) >> 2];
        dist[j + 7] = RGBA_Pal[(a & 0x03)];
    }
#if dump_data
    sprintf(name, "/sdcard/subto-%d", k);
        int fdto =open(name, O_WRONLY|O_CREAT);
        if (fdto == NULL) {
          ALOGE("fd ================NULL");
        }
        bytes = write(fdto, dist,length*16 );
        close(fdto);

      ALOGE("write bytes %d  / %d ",bytes, length*16);
#endif
      k++;
}

