/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef VICP_FUNC_H_
#define VICP_FUNC_H_

#if defined (__cplusplus)
extern "C" {
#endif

#define    VICP_DEV        "/dev/vicp"

#define VICP_IOC_MAGIC          'V'
#define VICP_PROCESS            _IOW(VICP_IOC_MAGIC, 0x00, vicp_data_info_t)

/* *********************************************************************** */
/* ************************* struct definitions **************************.*/
/* *********************************************************************** */
typedef struct vicp_data_info_s {
    int src_buf_fd;
    int src_buf_alisg_w;
    int src_buf_alisg_h;
    int src_data_w;
    int src_data_h;
    int src_color_fmt;
    int src_color_depth;
    int src_endian;
    int src_swap_cbcr;
    int dst_buf_fd;
    int dst_buf_w;
    int dst_buf_h;
    int dst_color_fmt;
    int dst_color_depth;
    int dst_endian;
    int dst_swap_cbcr;
    int crop_x;
    int crop_y;
    int crop_w;
    int crop_h;
    int rotation_mode;
    int output_x;
    int output_y;
    int output_w;
    int output_h;
    int shrink_mode;
    int skip_mode;
    int rdma_enable;
    int input_source_count;
    int input_source_number;
    int security_enable;
    int reserved;
}vicp_data_info_t;

/* ***********************************************************************.*/
/* ************************* function definitions ************************.*/
/* ***********************************************************************.*/
int open_vicp(void);
int close_vicp(int fd);
int process_vicp(int fd, vicp_data_info_t *vicp_data);
#if defined (__cplusplus)
}
#endif

#endif
