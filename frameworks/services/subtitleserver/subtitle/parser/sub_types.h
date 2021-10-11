/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: h file
*/
#ifndef _PGS_SUB_H
#define _PGS_SUB_H
#include <utils/Log.h>


// TODO: impl as a class item, optimize the structure[currently direct borrow from old impl]
// support sort!!!
typedef struct alm_spuvar
{
    unsigned int sync_bytes;
    unsigned int buffer_size;
    bool useMalloc;
    bool isSimpleText;
    unsigned int pid;
    int64_t pts;
    bool     isImmediatePresent;
    bool     isExtSub;
    bool     isKeepShowing; //no auto fading out, until close

    int64_t m_delay;
    unsigned char *spu_data;
    unsigned short cmd_offset;
    unsigned short length;

    unsigned int r_pt;
    unsigned int frame_rdy;

    unsigned short spu_color;
    unsigned short spu_alpha;
    unsigned short spu_start_x;
    unsigned short spu_start_y;
    unsigned short spu_width;
    unsigned short spu_height;
    unsigned short top_pxd_addr;
    unsigned short bottom_pxd_addr;

    unsigned int spu_origin_display_w; //for bitmap subtitle
    unsigned int spu_origin_display_h;
    unsigned disp_colcon_addr;
    unsigned char display_pending;
    unsigned char displaying;
    unsigned char subtitle_type;
    unsigned char reser[2];

/*
    unsigned rgba_enable;
    unsigned rgba_background;
    unsigned rgba_pattern1;
    unsigned rgba_pattern2;
    unsigned rgba_pattern3;
    */

    //for vob
    int resize_height;
    int resize_width;
    int resize_xstart;
    int resize_ystart;
    int resize_size;

    int disPlayBackground;
    //for qtone data inserted in cc data.
    bool isQtoneData;

    bool dynGen; // generate bitmap data dynamically
    int64_t pos;
    std::function<unsigned char *(struct alm_spuvar *spu, size_t *size)> genMethod;
    void genSubtitle() {
        size_t size;
        if (dynGen) {
            spu_data = genMethod(this, &size);
        }
    }


    alm_spuvar() : sync_bytes(0), buffer_size(0), useMalloc(true), isSimpleText(false),
            pid(0), pts(0), isImmediatePresent(false), isExtSub(false), isKeepShowing(false),
            m_delay(0), spu_data(nullptr), cmd_offset(0), length(0), disPlayBackground(0), isQtoneData(false)
    {
        dynGen = false;
        spu_start_x = spu_start_y = spu_width = spu_height = 0;
        spu_origin_display_w = spu_origin_display_h = 0;
    }

    ~alm_spuvar() {
        if (useMalloc) {
            if(spu_data != nullptr) free(spu_data);
        } else {
            if(spu_data != nullptr) delete[] spu_data;
        }
        spu_data = nullptr;
    }

    void dump(int fd, const char *prefex) {
        dprintf(fd, "%s ", prefex);
        if (isImmediatePresent) {
            dprintf(fd, "ImmediatePresent ");
        } else {
            dprintf(fd, "pts=%lld delayTo=%lld ", pts, m_delay);
        }

        if (isSimpleText) {
            dprintf(fd, "text=[%s]\n", spu_data);
        } else {
            dprintf(fd, "videoW=%u videoH=%u ", spu_origin_display_w, spu_origin_display_h);
            dprintf(fd, "[x,y,w,h][%d %u %u %u] ", spu_start_x, spu_start_y, spu_origin_display_w, spu_origin_display_h);
            dprintf(fd, "data=%p size=%u\n", spu_data, buffer_size);
        }
    }
} AML_SPUVAR;



#endif
