/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#define LOG_TAG "aml_vicp"

#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <log/log.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>
#include "vicp_func.h"
#include "include/aml_vicp.h"

int map_colorfmt_to_device(vicp_color_format_t color_fmt)
{
    int ret = -1;
    switch (color_fmt) {
    case VICP_COLOR_FMT_YCrCb_420_SP_NV21:
    case VICP_COLOR_FMT_YCbCr_420_SP_NV12:
        ret = 2;
        break;
    case VICP_COLOR_FMT_YCbCr_422_SP:
        ret = 1;
        break;
    default:
        ALOGE("%s: invalid color_fmt.\n", __func__);
        ret = 2;
    }

    return ret;
}

int map_colordepth_to_device(vicp_color_depth_t color_depth)
{
    int ret = -1;
    switch (color_depth) {
    case VICP_COLOR_DEPTH_8:
        ret = 8;
        break;
    case VICP_COLOR_DEPTH_10:
        ret = 10;
        break;
    case VICP_COLOR_DEPTH_12:
        ret = 12;
        break;
    default:
        ALOGE("%s: invalid color_fmt.\n", __func__);
        ret = 8;
    }

    return ret;
}

void dump_params(vicp_data_info_t *vicp_data)
{
        ALOGD("###########param info start ##########.\n");
        ALOGD("input: buf: fd:%d, alisg_w:%d, alisg_h:%d.\n",
                vicp_data->src_buf_fd,
                vicp_data->src_buf_alisg_w,
                vicp_data->src_buf_alisg_h);
        ALOGD("input: data: w:%d, h:%d, fmt:%d, depth:%d.\n",
                vicp_data->src_data_w,
                vicp_data->src_data_h,
                vicp_data->src_color_fmt,
                vicp_data->src_color_depth);
        ALOGD("input: src_endian:%d.\n", vicp_data->src_endian);
        ALOGD("input: src_swap_cbcr:%d.\n", vicp_data->src_swap_cbcr);
        ALOGD("input: crop: x:%d y:%d w:%d h:%d.\n",
                vicp_data->crop_x,
                vicp_data->crop_y,
                vicp_data->crop_w,
                vicp_data->crop_h);
        ALOGD("output: buf: fd:%d, w:%d, h:%d.\n",
                vicp_data->dst_buf_fd,
                vicp_data->dst_buf_w,
                vicp_data->dst_buf_h);
        ALOGD("output: fmt:%d, depth:%d.\n",
                vicp_data->dst_color_fmt,
                vicp_data->dst_color_depth);
        ALOGD("output: dst_endian:%d.\n", vicp_data->dst_endian);
        ALOGD("output: dst_swap_cbcr:%d.\n", vicp_data->dst_swap_cbcr);
        ALOGD("output: axis: x:%d y:%d w:%d h:%d.\n",
                vicp_data->output_x,
                vicp_data->output_y,
                vicp_data->output_w,
                vicp_data->output_h);
        ALOGD("###########param info end ##########.\n");

}

vicp_init_info_t *aml_vicp_init(void)
{
    vicp_init_info_t *vicp_init;

    vicp_init = (vicp_init_info_t *)malloc(sizeof(vicp_init_info_t));
    memset(vicp_init, 0, sizeof(vicp_init_info_t));
    if (!vicp_init) {
            ALOGE("%s: alloc buf failed.\n", __func__);
            return NULL;
    }

    vicp_init->vicp_fd = open_vicp();

    return vicp_init;
}

int aml_vicp_uninit(vicp_init_info_t *init_info)
{
    if (!init_info) {
        ALOGE("%s: NULL param.\n", __func__);
        return -1;
    }

    close_vicp(init_info->vicp_fd);
    free(init_info);

    return 0;
}

int aml_vicp_process(vicp_init_info_t *init_info, aml_vicp_info_t *vicp_info)
{
    vicp_data_info_t vicp_data;

    if (!init_info) {
        ALOGE("%s: init_info is NULL.\n", __func__);
        return -1;
    }

    if (!vicp_info) {
        ALOGE("%s: vicp_info is NULL.\n", __func__);
        return -1;
    }

    memset(&vicp_data, 0, sizeof(vicp_data_info_t));
    vicp_data.src_buf_fd = vicp_info->src_data_info.buf_fd;
    vicp_data.src_buf_alisg_w = vicp_info->src_data_info.buf_align_w;
    vicp_data.src_buf_alisg_h = vicp_info->src_data_info.buf_align_h;
    vicp_data.src_data_w = vicp_info->src_data_info.data_width;
    vicp_data.src_data_h = vicp_info->src_data_info.data_height;
    vicp_data.src_color_fmt = map_colorfmt_to_device(vicp_info->src_data_info.color_fmt);
    vicp_data.src_color_depth = map_colordepth_to_device(vicp_info->src_data_info.color_depth);
    vicp_data.src_endian = vicp_info->src_data_info.endian;
    if (vicp_info->src_data_info.color_fmt == VICP_COLOR_FMT_YCrCb_420_SP_NV21)
        vicp_data.src_swap_cbcr = 1;
    else
        vicp_data.src_swap_cbcr = 0;
    vicp_data.crop_x = vicp_info->src_data_info.crop_x;
    vicp_data.crop_y = vicp_info->src_data_info.crop_y;
    vicp_data.crop_w = vicp_info->src_data_info.crop_w;
    vicp_data.crop_h = vicp_info->src_data_info.crop_h;
    vicp_data.dst_buf_fd = vicp_info->dst_data_info.buf_fd;
    vicp_data.dst_buf_w = vicp_info->dst_data_info.buf_width;
    vicp_data.dst_buf_h = vicp_info->dst_data_info.buf_height;
    vicp_data.dst_color_fmt = map_colorfmt_to_device(vicp_info->dst_data_info.color_fmt);
    vicp_data.dst_color_depth = map_colordepth_to_device(vicp_info->dst_data_info.color_depth);
    vicp_data.dst_endian = vicp_info->dst_data_info.endian;
    if (vicp_info->dst_data_info.color_fmt == VICP_COLOR_FMT_YCbCr_420_SP_NV12)
        vicp_data.dst_swap_cbcr = 1;
    else
        vicp_data.dst_swap_cbcr = 0;
    vicp_data.output_x = vicp_info->dst_data_info.axis_x;
    vicp_data.output_y = vicp_info->dst_data_info.axis_y;
    vicp_data.output_w = vicp_info->dst_data_info.axis_w;
    vicp_data.output_h = vicp_info->dst_data_info.axis_h;
    vicp_data.rotation_mode = vicp_info->rotation_mode;
    vicp_data.input_source_count = 0;
    vicp_data.input_source_number = 0;
    vicp_data.rdma_enable = 0;
    vicp_data.security_enable = 0;
    vicp_data.shrink_mode = 0;
    vicp_data.skip_mode = 0;

    dump_params(&vicp_data);

    return process_vicp(init_info->vicp_fd, &vicp_data);
}
