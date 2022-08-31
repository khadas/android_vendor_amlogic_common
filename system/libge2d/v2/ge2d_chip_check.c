/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <malloc.h>
#include <string.h>
#include <sys/time.h>  /* for gettimeofday() */
#include "ge2d_port.h"
#include "ge2d_com.h"
#include "aml_ge2d.h"

static int SX_SRC1 = 640;
static int SY_SRC1 = 480;
static int SX_SRC2 = 640;
static int SY_SRC2 = 480;
static int SX_DST = 640;
static int SY_DST = 480;

static int SRC1_PIXFORMAT = PIXEL_FORMAT_RGBA_8888;
static int SRC2_PIXFORMAT = PIXEL_FORMAT_RGBA_8888;
static int DST_PIXFORMAT = PIXEL_FORMAT_RGBA_8888;

static int OP = AML_GE2D_BLIT;
static int src1_layer_mode = LAYER_MODE_PREMULTIPLIED;
static int src2_layer_mode = 0;
static int gb1_alpha = 0xff;
static int gb2_alpha = 0xff;
static aml_ge2d_t amlge2d;
unsigned short color_bar_code[614400];
unsigned short color_bar_rotate[614400];
unsigned short color_bar_blend[614400];
#define TEST_NUM 3

static void set_ge2dinfo(aml_ge2d_info_t *pge2dinfo)
{
    pge2dinfo->src_info[0].memtype = GE2D_CANVAS_ALLOC;
    pge2dinfo->src_info[0].canvas_w = SX_SRC1;
    pge2dinfo->src_info[0].canvas_h = SY_SRC1;
    pge2dinfo->src_info[0].format = SRC1_PIXFORMAT;
    pge2dinfo->src_info[0].plane_number = 1;
    pge2dinfo->src_info[1].memtype = GE2D_CANVAS_ALLOC;
    pge2dinfo->src_info[1].canvas_w = SX_SRC2;
    pge2dinfo->src_info[1].canvas_h = SY_SRC2;
    pge2dinfo->src_info[1].format = SRC2_PIXFORMAT;
    pge2dinfo->src_info[1].plane_number = 1;

    pge2dinfo->dst_info.memtype = GE2D_CANVAS_ALLOC;
    pge2dinfo->dst_info.canvas_w = SX_DST;
    pge2dinfo->dst_info.canvas_h = SY_DST;
    pge2dinfo->dst_info.format = DST_PIXFORMAT;
    pge2dinfo->dst_info.rotation = GE2D_ROTATION_0;
    pge2dinfo->dst_info.plane_number = 1;
    pge2dinfo->offset = 0;
    pge2dinfo->ge2d_op = OP;
    pge2dinfo->blend_mode = BLEND_MODE_PREMULTIPLIED;
}

static void print_usage(void)
{
    printf ("Usage: ge2d_chip_check [options]\n\n");
    printf ("Options:\n\n");
    printf ("  --help                       Print usage information.\n");
    printf ("\n");
}

static int parse_command_line(int argc, char *argv[])
{
    int i;
    /* parse command line */
    for (i = 1; i < argc; i++)
        if (strncmp (argv[i], "--", 2) == 0)
            if (strcmp (argv[i] + 2, "help") == 0) {
                print_usage();
                return GE2D_FAIL;
            }
    return GE2D_SUCCESS;
}

static int compare_data(char *data1, char *data2, int size)
{
    int i = 0, err_hit = 0;
    int thresh_hold = 1;//size * 5/100;

    if (!data1 || !data2 || !size)
        return 0;
    for (i = 0; i < size; i++) {
        if (*data1 != *data2) {
            printf("data1=%x,data2=%x\n",*data1,*data2);
            err_hit++;
        }
        data1++;
        data2++;
        if (err_hit > thresh_hold) {
            printf("bad chip found,err_hit=%d\n",err_hit);
            return -1;
        }
    }
    return 0;
}

#ifdef DUMP_OUTPUT
static int aml_write_file(const char* url , aml_ge2d_info_t *pge2dinfo)
{
    int fd = -1;
    int length = 0;
    int write_num = 0;
    unsigned int *value;

    if (amlge2d.dst_size[0] == 0)
        return 0;
    if ((GE2D_CANVAS_OSD0 == pge2dinfo->dst_info.memtype)
        || (GE2D_CANVAS_OSD1 == pge2dinfo->dst_info.memtype))
        return 0;

    fd = open(url,O_RDWR | O_CREAT,0660);
    if (fd < 0) {
        E_GE2D("write file:%s open error\n",url);
        return GE2D_FAIL;
    }

    amlge2d.dst_data[0] = (char*)malloc(amlge2d.dst_size[0]);
    if (!amlge2d.dst_data[0]) {
        E_GE2D("malloc for dst_data failed\n");
        return GE2D_FAIL;
    }

    memcpy(amlge2d.dst_data[0],pge2dinfo->dst_info.vaddr[0],amlge2d.dst_size[0]);

    printf("dst pixel: 0x%2x, 0x%2x,0x%2x,0x%2x, 0x%2x,0x%2x,0x%2x,0x%2x\n",
        pge2dinfo->dst_info.vaddr[0][0],
        pge2dinfo->dst_info.vaddr[0][1],
        pge2dinfo->dst_info.vaddr[0][2],
        pge2dinfo->dst_info.vaddr[0][3],
        pge2dinfo->dst_info.vaddr[0][4],
        pge2dinfo->dst_info.vaddr[0][5],
        pge2dinfo->dst_info.vaddr[0][6],
        pge2dinfo->dst_info.vaddr[0][7]);
    write_num = write(fd,amlge2d.dst_data[0],amlge2d.dst_size[0]);
    if (write_num <= 0) {
        E_GE2D("read file read_num=%d error\n",write_num);
    }
    close(fd);
    return GE2D_SUCCESS;
}
#endif

static int do_strechblit_rotate(aml_ge2d_info_t *pge2dinfo)
{
    int ret = -1;

    pge2dinfo->src_info[0].memtype = GE2D_CANVAS_ALLOC;
    pge2dinfo->dst_info.memtype = GE2D_CANVAS_ALLOC;
    pge2dinfo->src_info[0].canvas_w = SX_SRC1;
    pge2dinfo->src_info[0].canvas_h = SY_SRC1;
    pge2dinfo->src_info[0].format = SRC1_PIXFORMAT;
    pge2dinfo->dst_info.canvas_w = SX_DST;
    pge2dinfo->dst_info.canvas_h = SY_DST;
    pge2dinfo->dst_info.format = DST_PIXFORMAT;

    pge2dinfo->src_info[0].rect.x = 0;
    pge2dinfo->src_info[0].rect.y = 0;
    pge2dinfo->src_info[0].rect.w = SX_SRC1;
    pge2dinfo->src_info[0].rect.h = SY_SRC1;
    pge2dinfo->dst_info.rect.x = 0;
    pge2dinfo->dst_info.rect.y = 0;
    pge2dinfo->dst_info.rect.w = SX_DST;
    pge2dinfo->dst_info.rect.h = SY_DST;
    pge2dinfo->dst_info.rotation = GE2D_ROTATION_180;
    pge2dinfo->src_info[0].layer_mode = src1_layer_mode;
    pge2dinfo->src_info[0].plane_alpha = gb1_alpha;

    ret = aml_ge2d_process(pge2dinfo);

    return ret;
}

static int do_blend(aml_ge2d_info_t *pge2dinfo)
{
    int ret = -1;

    pge2dinfo->src_info[0].memtype = GE2D_CANVAS_ALLOC;
    pge2dinfo->src_info[1].memtype = GE2D_CANVAS_ALLOC;
    pge2dinfo->dst_info.memtype = GE2D_CANVAS_ALLOC;
    pge2dinfo->blend_mode = BLEND_MODE_PREMULTIPLIED;
    /* do blend src1 blend src2(dst) to dst */

    pge2dinfo->src_info[0].canvas_w = SX_SRC1;
    pge2dinfo->src_info[0].canvas_h = SY_SRC1;
    pge2dinfo->src_info[0].format = SRC1_PIXFORMAT;
    pge2dinfo->src_info[0].rect.x = 0;
    pge2dinfo->src_info[0].rect.y = 0;
    pge2dinfo->src_info[0].rect.w = pge2dinfo->src_info[0].canvas_w;
    pge2dinfo->src_info[0].rect.h = pge2dinfo->src_info[0].canvas_h;
    pge2dinfo->src_info[0].fill_color_en = 0;

    pge2dinfo->src_info[1].canvas_w = SX_SRC2;
    pge2dinfo->src_info[1].canvas_h = SY_SRC2;
    pge2dinfo->src_info[1].format = SRC2_PIXFORMAT;
    pge2dinfo->src_info[1].rect.x = 0;
    pge2dinfo->src_info[1].rect.y = 0;
    pge2dinfo->src_info[1].rect.w = pge2dinfo->src_info[0].canvas_w;
    pge2dinfo->src_info[1].rect.h = pge2dinfo->src_info[0].canvas_h;
    pge2dinfo->src_info[1].fill_color_en = 0;
    if (src2_layer_mode == LAYER_MODE_NON)
        pge2dinfo->src_info[0].format = PIXEL_FORMAT_RGBX_8888;
    pge2dinfo->dst_info.canvas_w = SX_DST;
    pge2dinfo->dst_info.canvas_h = SY_DST;
    pge2dinfo->dst_info.format = DST_PIXFORMAT;
    pge2dinfo->dst_info.rect.x = 0;
    pge2dinfo->dst_info.rect.y = 0;
    pge2dinfo->dst_info.rect.w = pge2dinfo->src_info[0].canvas_w;
    pge2dinfo->dst_info.rect.h = pge2dinfo->src_info[0].canvas_h;
    pge2dinfo->dst_info.rotation = GE2D_ROTATION_0;

    pge2dinfo->src_info[0].layer_mode = src1_layer_mode;
    pge2dinfo->src_info[1].layer_mode = src2_layer_mode;
    pge2dinfo->src_info[0].plane_alpha = gb1_alpha;
    pge2dinfo->src_info[1].plane_alpha = gb2_alpha;
    ret = aml_ge2d_process(pge2dinfo);

    return ret;
}

static inline unsigned long myclock()
{
     struct timeval tv;

     gettimeofday (&tv, NULL);

     return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

int aml_generate_src1(char *src, aml_ge2d_info_t *pge2dinfo)
{
    if (amlge2d.src_size[0] == 0)
        return 0;
    memcpy(pge2dinfo->src_info[0].vaddr[0], src, amlge2d.src_size[0]);
    return GE2D_SUCCESS;
}

int aml_generate_src2(int color, aml_ge2d_info_t *pge2dinfo)
{
    unsigned int i = 0;

    if (amlge2d.src2_size[0] == 0)
        return 0;

    for (i=0; i < amlge2d.src2_size[0]; ) {
        pge2dinfo->src_info[1].vaddr[0][i] = color & 0xff;
        pge2dinfo->src_info[1].vaddr[0][i+1] = (color & 0xff00)>>8;
        pge2dinfo->src_info[1].vaddr[0][i+2] = (color & 0xff0000)>>16;
        pge2dinfo->src_info[1].vaddr[0][i+3] = (color & 0xff000000)>>24;
        i+=4;
    }
    return GE2D_SUCCESS;
}

static void prepare_test_date(void)
{
    int i, j, index;
    int lineinterval = SX_SRC1 * 2;

    for (i = 0; i < SY_SRC1; i++)
        for (j = 0; j < SX_SRC1; j++) {
            index = i * lineinterval + 2 * j;
            switch (j / 80) {
            case 0:
                color_bar_code[index] = 0xffff;
                color_bar_code[index + 1] = 0x80ff;
                color_bar_rotate[index] = 0x0000;
                color_bar_rotate[index + 1] = 0x8000;
                color_bar_blend[index] = 0x0000;
                color_bar_blend[index + 1] = 0xa000;
                break;
            case 1:
                color_bar_code[index] = 0xffff;
                color_bar_code[index + 1] = 0x8000;
                color_bar_rotate[index] = 0x0000;
                color_bar_rotate[index + 1] = 0x80ff;
                color_bar_blend[index] = 0x0000;
                color_bar_blend[index + 1] = 0xa0ff;
                break;
            case 2:
                color_bar_code[index] = 0xff00;
                color_bar_code[index + 1] = 0x80ff;
                color_bar_rotate[index] = 0x00ff;
                color_bar_rotate[index + 1] = 0x8000;
                color_bar_blend[index] = 0x00ff;
                color_bar_blend[index + 1] = 0xa000;
                break;
            case 3:
                color_bar_code[index] = 0xff00;
                color_bar_code[index + 1] = 0x8000;
                color_bar_rotate[index] = 0x00ff;
                color_bar_rotate[index + 1] = 0x80ff;
                color_bar_blend[index] = 0x00ff;
                color_bar_blend[index + 1] = 0xa0ff;
                break;
            case 4:
                color_bar_code[index] = 0x00ff;
                color_bar_code[index + 1] = 0x80ff;
                color_bar_rotate[index] = 0xff00;
                color_bar_rotate[index + 1] = 0x8000;
                color_bar_blend[index] = 0xff00;
                color_bar_blend[index + 1] = 0xa000;
                break;
            case 5:
                color_bar_code[index] = 0x00ff;
                color_bar_code[index + 1] = 0x8000;
                color_bar_rotate[index] = 0xff00;
                color_bar_rotate[index + 1] = 0x80ff;
                color_bar_blend[index] = 0xff00;
                color_bar_blend[index + 1] = 0xa0ff;
                break;
            case 6:
                color_bar_code[index] = 0x0000;
                color_bar_code[index + 1] = 0x80ff;
                color_bar_rotate[index] = 0xffff;
                color_bar_rotate[index + 1] = 0x8000;
                color_bar_blend[index] = 0xffff;
                color_bar_blend[index + 1] = 0xa000;
                break;
            case 7:
                color_bar_code[index] = 0x0000;
                color_bar_code[index + 1] = 0x8000;
                color_bar_rotate[index] = 0xffff;
                color_bar_rotate[index + 1] = 0x80ff;
                color_bar_blend[index] = 0xffff;
                color_bar_blend[index + 1] = 0xa0ff;
                break;
            default:
                printf("width is not supported\n");
            }
        }
}

int main(int argc, char **argv)
{
    int ret = -1;
    int i = 0, err_cnt = 0;
    unsigned long stime,etime;

    memset(&amlge2d,0x0,sizeof(aml_ge2d_t));
    memset(&(amlge2d.ge2dinfo.src_info[0]), 0, sizeof(buffer_info_t));
    memset(&(amlge2d.ge2dinfo.src_info[1]), 0, sizeof(buffer_info_t));
    memset(&(amlge2d.ge2dinfo.dst_info), 0, sizeof(buffer_info_t));
    ret = parse_command_line(argc,argv);
    if (ret < 0)
        return GE2D_SUCCESS;

    set_ge2dinfo(&amlge2d.ge2dinfo);

    ret = aml_ge2d_init(&amlge2d);
    if (ret < 0)
        return GE2D_FAIL;

    ret = aml_ge2d_mem_alloc(&amlge2d);
    if (ret < 0)
        goto exit;

    prepare_test_date();
    printf("Start GE2D chip check...\n");
    stime = myclock();
    for (i = 0; i < TEST_NUM; i++) {
        /* strechblit + rotate */
        aml_generate_src1((char *)color_bar_code,&amlge2d.ge2dinfo);
        amlge2d.ge2dinfo.ge2d_op = AML_GE2D_STRETCHBLIT;
        ret = do_strechblit_rotate(&amlge2d.ge2dinfo);
        if (ret < 0) {
            printf("do_strechblit_rotate err\n");
        }
        ret = aml_ge2d_invalid_cache(&amlge2d.ge2dinfo);
        if (ret < 0) {
            printf("aml_ge2d_invalid_cache err\n");
        }
        ret = compare_data((char *)color_bar_rotate, amlge2d.ge2dinfo.dst_info.vaddr[0], amlge2d.dst_size[0]);
        if (ret < 0) {
            err_cnt++;
            printf("GE2D: strechbilt + rotate 180 [FAILED]\n");
            #ifdef DUMP_OUTPUT
            ret = aml_write_file("stretchblit_rotate_err.raw", &amlge2d.ge2dinfo);
            if (ret < 0)
                printf("stretchblit+rotate dump err\n");
            #endif
        }

        /* blend */
        aml_generate_src1((char *)color_bar_rotate,&amlge2d.ge2dinfo);
        aml_generate_src2(0x40000000,&amlge2d.ge2dinfo);
        amlge2d.ge2dinfo.ge2d_op = AML_GE2D_BLEND;
        ret = do_blend(&amlge2d.ge2dinfo);
        if (ret < 0) {
            printf("do_blend err\n");
        }
        ret = aml_ge2d_invalid_cache(&amlge2d.ge2dinfo);
        if (ret < 0) {
            printf("aml_ge2d_invalid_cache err\n");
        }
        ret = compare_data((char *)color_bar_blend, amlge2d.ge2dinfo.dst_info.vaddr[0], amlge2d.dst_size[0]);
        if (ret < 0) {
            err_cnt++;
            printf("GE2D: blend [FAILED]\n");
            #ifdef DUMP_OUTPUT
            ret = aml_write_file("stretchblit_rotate_err.raw", &amlge2d.ge2dinfo);
            if (ret < 0)
                printf("stretchblit+rotate dump err\n");
            #endif
        }
    }
    etime = myclock();
    D_GE2D("used time %d ms\n",etime - stime);

    if (err_cnt >= TEST_NUM)
        printf("===ge2d_slt_test:failed===\n");
    else
        printf("===ge2d_slt_test:pass===\n");

exit:
    aml_ge2d_mem_free(&amlge2d);
    aml_ge2d_exit(&amlge2d);
    return GE2D_SUCCESS;
}
