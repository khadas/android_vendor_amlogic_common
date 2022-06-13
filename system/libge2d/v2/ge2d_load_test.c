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
#include <sys/time.h>
#include "ge2d_port.h"
#include "ge2d_com.h"
#include "aml_ge2d.h"

static int SX = 1920;
static int SY = 1080;
static int DEMOTIME = 3000; /* milliseconds */
static int PIXFORMAT = PIXEL_FORMAT_RGBA_8888;
static int OP = AML_GE2D_STRETCHBLIT;

static void set_ge2dinfo(aml_ge2d_info_t *pge2dinfo)
{
  pge2dinfo->src_info[0].memtype = GE2D_CANVAS_ALLOC;
  pge2dinfo->src_info[0].canvas_w = SX;
  pge2dinfo->src_info[0].canvas_h = SY;
  pge2dinfo->src_info[0].format = PIXFORMAT;
  pge2dinfo->src_info[0].plane_number = 1;

  pge2dinfo->src_info[1].memtype = GE2D_CANVAS_ALLOC;
  pge2dinfo->src_info[1].canvas_w = SX;
  pge2dinfo->src_info[1].canvas_h = SY;
  pge2dinfo->src_info[1].format = PIXFORMAT;
  pge2dinfo->src_info[1].plane_number = 1;

  pge2dinfo->dst_info.memtype = GE2D_CANVAS_ALLOC;
  pge2dinfo->dst_info.canvas_w = SX;
  pge2dinfo->dst_info.canvas_h = SY;
  pge2dinfo->dst_info.format = PIXFORMAT;
  pge2dinfo->dst_info.plane_number = 1;
  pge2dinfo->dst_info.rotation = 0;
  pge2dinfo->offset = 0;
  pge2dinfo->ge2d_op = OP;
  pge2dinfo->blend_mode = BLEND_MODE_PREMULTIPLIED;
  D_GE2D("OP=%d,w=%d,h=%d,format=%x,duration=%d \n", OP, SX, SY, PIXFORMAT,
         DEMOTIME);
}

static void print_usage(void)
{
    printf ("Usage: ge2d_load_test [options]\n\n");
    printf ("Options:\n\n");
    printf ("  --op <0:fillrect, 1:blend, 2:strechblit, 3:blit>    ge2d operation case.\n");
    printf ("  --duration <milliseconds>    Duration of each ge2d operation case.\n");
    printf ("  --size     <width>x<height>  Set ge2d size.\n");
    printf ("  --pixelformat <define as pixel_format_t>  Set ge2d pixelformat.\n");
    printf ("  --help                       Print usage information.\n");
    printf ("\n");
}

static int parse_command_line(int argc, char *argv[])
{
  int i;
  /* parse command line */
  for (i = 1; i < argc; i++) {
    if (strncmp(argv[i], "--", 2) == 0) {
      if (strcmp(argv[i] + 2, "help") == 0) {
        print_usage();
        return GE2D_FAIL;
      } else if (strcmp(argv[i] + 2, "op") == 0 && ++i < argc &&
                 sscanf(argv[i], "%d", &OP) == 1) {
        continue;
      }

      else if (strcmp(argv[i] + 2, "size") == 0 && ++i < argc &&
               sscanf(argv[i], "%dx%d", &SX, &SY) == 2) {
        continue;
      } else if (strcmp(argv[i] + 2, "duration") == 0 && ++i < argc &&
                 sscanf(argv[i], "%d", &DEMOTIME) == 1) {
        continue;
      } else if (strcmp(argv[i] + 2, "pixelformat") == 0 && ++i < argc &&
                 sscanf(argv[i], "%d", &PIXFORMAT) == 1) {
        continue;
      }
    }
  }
  return GE2D_SUCCESS;
}

static int ge2d_info_set(aml_ge2d_info_t *pge2dinfo)
{
  switch (pge2dinfo->ge2d_op) {
    case AML_GE2D_FILLRECTANGLE:
      pge2dinfo->color = 0xffffffff;
      pge2dinfo->src_info[0].rect.x = 0;
      pge2dinfo->src_info[0].rect.y = 0;
      pge2dinfo->src_info[0].rect.w = pge2dinfo->src_info[0].canvas_w;
      pge2dinfo->src_info[0].rect.h = pge2dinfo->src_info[0].canvas_h;
      pge2dinfo->dst_info.rect.x = 0;
      pge2dinfo->dst_info.rect.y = 0;
      pge2dinfo->dst_info.rect.w = pge2dinfo->src_info[0].canvas_w;
      pge2dinfo->dst_info.rect.h = pge2dinfo->src_info[0].canvas_h;
      break;
    case AML_GE2D_BLIT:
      pge2dinfo->src_info[0].rect.x = 0;
      pge2dinfo->src_info[0].rect.y = 0;
      pge2dinfo->src_info[0].rect.w = pge2dinfo->src_info[0].canvas_w / 2;
      pge2dinfo->src_info[0].rect.h = pge2dinfo->src_info[0].canvas_h / 2;
      pge2dinfo->dst_info.rect.x = 0;
      pge2dinfo->dst_info.rect.y = 0;
      pge2dinfo->dst_info.rect.w = pge2dinfo->src_info[0].canvas_w / 2;
      pge2dinfo->dst_info.rect.h = pge2dinfo->src_info[0].canvas_h / 2;
      break;
    case AML_GE2D_STRETCHBLIT:
      pge2dinfo->src_info[0].rect.x = 0;
      pge2dinfo->src_info[0].rect.y = 0;
      pge2dinfo->src_info[0].rect.w = pge2dinfo->src_info[0].canvas_w;
      pge2dinfo->src_info[0].rect.h = pge2dinfo->src_info[0].canvas_h;
      pge2dinfo->dst_info.rect.x = 0;
      pge2dinfo->dst_info.rect.y = 0;
      pge2dinfo->dst_info.rect.w = pge2dinfo->src_info[0].canvas_w / 2;
      pge2dinfo->dst_info.rect.h = pge2dinfo->src_info[0].canvas_h / 2;
      break;
    case AML_GE2D_BLEND:
      pge2dinfo->src_info[0].rect.x = 0;
      pge2dinfo->src_info[0].rect.y = 0;
      pge2dinfo->src_info[0].rect.w = pge2dinfo->src_info[0].canvas_w;
      pge2dinfo->src_info[0].rect.h = pge2dinfo->src_info[0].canvas_h;
      pge2dinfo->src_info[1].rect.x = 0;
      pge2dinfo->src_info[1].rect.y = 0;
      pge2dinfo->src_info[1].rect.w = pge2dinfo->src_info[1].canvas_w;
      pge2dinfo->src_info[1].rect.h = pge2dinfo->src_info[1].canvas_h;
      pge2dinfo->dst_info.rect.x = 0;
      pge2dinfo->dst_info.rect.y = 0;
      pge2dinfo->dst_info.rect.w = pge2dinfo->src_info[1].canvas_w;
      pge2dinfo->dst_info.rect.h = pge2dinfo->src_info[1].canvas_h;

      break;
    default:
      E_GE2D("ge2d(%d) opration not support!\n", pge2dinfo->ge2d_op);
      return GE2D_FAIL;
  }
  return GE2D_SUCCESS;
}

static inline unsigned long myclock()
{
  struct timeval tv;

  gettimeofday(&tv, NULL);

  return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

int main(int argc, char **argv)
{
  int ret = -1;
  int rc = GE2D_SUCCESS;
  int i = 0;
  unsigned long stime;
  aml_ge2d_t amlge2d;

  memset(&amlge2d, 0x0, sizeof(aml_ge2d_t));
  memset(&(amlge2d.ge2dinfo.src_info[0]), 0, sizeof(buffer_info_t));
  memset(&(amlge2d.ge2dinfo.src_info[1]), 0, sizeof(buffer_info_t));
  memset(&(amlge2d.ge2dinfo.dst_info), 0, sizeof(buffer_info_t));
  ret = parse_command_line(argc, argv);
  if (ret == GE2D_FAIL)
    return GE2D_FAIL;

  set_ge2dinfo(&amlge2d.ge2dinfo);

  ret = ge2d_info_set(&amlge2d.ge2dinfo);
  if (ret < 0) {
    rc = GE2D_FAIL;
    goto exit;
  }

  ret = aml_ge2d_init(&amlge2d);
  if (ret < 0)
    return GE2D_FAIL;

  ret = aml_ge2d_mem_alloc(&amlge2d);
  if (ret < 0) {
    rc = GE2D_FAIL;
    goto exit;
  }

  stime = myclock();
  for (i = 0; myclock() < (stime + DEMOTIME); i++) {
    ret = aml_ge2d_process(&amlge2d.ge2dinfo);
    if (ret < 0) {
      rc = GE2D_FAIL;
      goto exit;
    }
    D_GE2D("stime=%lx, myclock()=%lx,i=%d\n", (stime + DEMOTIME), myclock(), i);
  }
  printf("Total %d loops run!\n", i);

exit:
  aml_ge2d_mem_free(&amlge2d);
  aml_ge2d_exit(&amlge2d);
  printf("ge2d example exit!!!\n");
  return rc;
}
