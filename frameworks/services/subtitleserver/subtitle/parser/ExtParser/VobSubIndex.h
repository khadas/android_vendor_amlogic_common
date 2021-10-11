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

#define SUCCESS     1
#define FAIL        0

//#define OSD_HALF_SIZE 720*480/8
#define OSD_HALF_SIZE 1920*1280/8

// Command enum
typedef enum
{
    FSTA_DSP = 0,
    STA_DSP = 1,
    STP_DSP = 2,
    SET_COLOR = 3,
    SET_CONTR = 4,
    SET_DAREA = 5,
    SET_DSPXA = 6,
    CHG_COLCON = 7,
    CMD_END = 0xFF,
} CommandID;

// TODO: revise this
typedef struct {

    unsigned int pts100;    /* from idx */
    off_t filepos;

    //unsigned int size;
    //unsigned char *data;
} packet_t;

typedef struct {
    char *id;
    packet_t *packets;
    unsigned int packets_reserve;
    unsigned int packets_size;
    unsigned int current_index;
} packet_queue_t;

typedef struct {
    //FILE *file;

    int fd;
    unsigned char *data;
    unsigned long size;
    unsigned long pos;
} rar_stream_t;

typedef struct {

    rar_stream_t *stream;
    unsigned int pts;
    int aid;
    unsigned char *packet;
    //unsigned int packet_reserve;
    unsigned int packet_size;
} mpeg_t;

typedef struct {

    unsigned int palette[16];
    unsigned int cuspal[4];
    int delay;
    unsigned int custom;
    unsigned int have_palette;
    unsigned int orig_frame_width, orig_frame_height;
    unsigned int origin_x, origin_y;
    unsigned int forced_subs;

    /* index */

    packet_queue_t *spu_streams;
    unsigned int spu_streams_size;
    unsigned int spu_streams_current;

} vobsub_t;


typedef struct _VOB_SPUVAR
{
    unsigned short spu_color;
    unsigned short spu_alpha;
    unsigned short spu_start_x;
    unsigned short spu_start_y;
    unsigned short spu_width;
    unsigned short spu_height;
    unsigned short top_pxd_addr;    // CHIP_T25
    unsigned short bottom_pxd_addr; // CHIP_T25

    unsigned mem_start; // CHIP_T25
    unsigned mem_end;   // CHIP_T25
    unsigned mem_size;  // CHIP_T25
    unsigned mem_rp;
    unsigned mem_wp;
    unsigned mem_rp2;
    unsigned char spu_cache[8];
    int spu_cache_pos;
    int spu_decoding_start_pos; //0~OSD_HALF_SIZE*2-1, start index to vob_pixData1[0~OSD_HALF_SIZE*2]

    unsigned disp_colcon_addr;  // CHIP_T25
    unsigned char display_pending;
    unsigned char displaying;
    unsigned char reser[2];
} VOB_SPUVAR;


class VobSubIndex: public TextSubtitle {

public:
    VobSubIndex(std::shared_ptr<DataSource> source);
    ~VobSubIndex();

virtual std::shared_ptr<AML_SPUVAR> popDecodedItem();

protected:
    virtual std::shared_ptr<ExtSubItem> decodedItem();

private:
    /***** VOB header/control info *****/
    unsigned int mPalette[16];
    unsigned int mCuspal[4];
    int mDelay;
    unsigned int mCustom;
    unsigned int mHavePalette;
    unsigned int mOrigFrameWidth;
    unsigned int mOrigFrameHeight;
    unsigned int mOriginX;
    unsigned int mOriginY;
    unsigned int mForcedSubs;

    int vobsubId;

    unsigned char *genSubBitmap(AML_SPUVAR *spu, size_t *size);
    int do_vob_sub_cmd(unsigned char *packet);
    unsigned char vob_fill_pixel(unsigned char * rawData, unsigned char *outData, int n);
    void convert2bto32b(const unsigned char *source, long length, int bytesPerLine, unsigned int *dist, int subtitle_alpha);

    unsigned int getDuration(int64_t pos, int trackId);

    // We parse the .idx file for subtitle.
    // render sub picture, need read the .sub file.
    int mSubFd;

    vobsub_t mVobParam;


    mpeg_t *mpg;
    VOB_SPUVAR VobSPU;
    unsigned int duration; // TODO: move to spu list.
    int mSelectedTrackId;
};



