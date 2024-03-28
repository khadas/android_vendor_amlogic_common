/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef AML_VICP_H_
#define AML_VICP_H_

#if defined (__cplusplus)
extern "C" {
#endif

/* *********************************************************************** */
/* ************************* enum definitions ****************************.*/
/* *********************************************************************** */
typedef enum vicp_rotation_mode_e {
    VICP_ROTATION_0 = 0,
    VICP_ROTATION_90,
    VICP_ROTATION_180,
    VICP_ROTATION_270,
    VICP_ROTATION_MIRROR_V,
    VICP_ROTATION_MIRROR_H,
    VICP_ROTATION_MAX,
} vicp_rotation_mode_t;

typedef enum vicp_color_format_e {
    VICP_COLOR_FMT_YCrCb_420_SP_NV21,    // NV21         YYYY.....VU....
    VICP_COLOR_FMT_YCbCr_420_SP_NV12,    // NV12         YYYY.....UV....
    VICP_COLOR_FMT_YCbCr_422_SP,         // NV16         YYYY.....UVUV....
    VICP_COLOR_FMT_MAX,
} vicp_color_format_t;

typedef enum vicp_color_depth_e {
    VICP_COLOR_DEPTH_8,
    VICP_COLOR_DEPTH_10,
    VICP_COLOR_DEPTH_12,
    VICP_COLOR_DEPTH_MAX,
} vicp_color_depth_t;

typedef enum vicp_endian_mode_e {
    VICP_ENDIAN_BIG,
    VICP_ENDIAN_LITTLE,
    VICP_ENDIAN_MAX,
} vicp_endian_mode_t;

/* *********************************************************************** */
/* ************************* struct definitions **************************.*/
/* *********************************************************************** */
typedef struct src_data_info_s {
    int buf_fd;
    int buf_align_w;
    int buf_align_h;
    int data_width;
    int data_height;
    vicp_color_format_t color_fmt;
    vicp_color_depth_t color_depth;
    vicp_endian_mode_t endian;
    int crop_x;
    int crop_y;
    int crop_w;
    int crop_h;
} src_data_info_t;

typedef struct dst_data_info_s {
    int buf_fd;
    int buf_width;
    int buf_height;
    vicp_color_format_t color_fmt;
    vicp_color_depth_t color_depth;
    vicp_endian_mode_t endian;
    int axis_x;
    int axis_y;
    int axis_w;
    int axis_h;
} dst_data_info_t;

typedef struct aml_vicp_info_s {
    src_data_info_t src_data_info;
    dst_data_info_t dst_data_info;
    vicp_rotation_mode_t rotation_mode;
} aml_vicp_info_t;

typedef struct vicp_init_info_s {
    int vicp_fd;
    int reserved;
} vicp_init_info_t;

/* ***********************************************************************.*/
/* ************************* function definitions ************************.*/
/* ***********************************************************************.*/
vicp_init_info_t *aml_vicp_init(void);
int aml_vicp_uninit(vicp_init_info_t *init_info);
int aml_vicp_process(vicp_init_info_t *init_info, aml_vicp_info_t *vicp_info);
#if defined (__cplusplus)
}
#endif
#endif
