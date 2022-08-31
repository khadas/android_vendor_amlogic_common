/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef __CDOLBY_VISION_H
#define __CDOLBY_VISION_H

#define DOLBY_DEV_PATH                "/dev/amdolby_vision"

// ***************************************************************************
// *** ENUM definitions **********
// ***************************************************************************
typedef enum dolby_pq_mode_e {
    DOLBY_PQ_MODE_DARK_DV = 0,
    DOLBY_PQ_MODE_BRIGHT_DV,
    DOLBY_PQ_MODE_DYNAMIC_DV,
    DOLBY_PQ_MODE_STANDARD_DV,
    DOLBY_PQ_MODE_PC_DV,
    DOLBY_PQ_MODE_CUSTOMER_DV,
    DOLBY_PQ_MODE_STORE_DV,
    DOLBY_PQ_MODE_DARK_HDR10,
    DOLBY_PQ_MODE_BRIGHT_HDR10,
    DOLBY_PQ_MODE_DYNAMIC_HDR10,
    DOLBY_PQ_MODE_STANDARD_HDR10,
    DOLBY_PQ_MODE_PC_HDR10,
    DOLBY_PQ_MODE_CUSTOMER_HDR10,
    DOLBY_PQ_MODE_STORE_HDR10,
    DOLBY_PQ_MODE_DARK_HLG,
    DOLBY_PQ_MODE_BRIGHT_HLG,
    DOLBY_PQ_MODE_DYNAMIC_HLG,
    DOLBY_PQ_MODE_STANDARD_HLG,
    DOLBY_PQ_MODE_PC_HLG,
    DOLBY_PQ_MODE_CUSTOMER_HLG,
    DOLBY_PQ_MODE_STORE_HLG,
    DOLBY_PQ_MODE_DARK_SDR,
    DOLBY_PQ_MODE_BRIGHT_SDR,
    DOLBY_PQ_MODE_DYNAMIC_SDR,
    DOLBY_PQ_MODE_STANDARD_SDR,
    DOLBY_PQ_MODE_PC_SDR,
    DOLBY_PQ_MODE_CUSTOMER_SDR,
    DOLBY_PQ_MODE_STORE_SDR,
    DOLBY_PQ_MODE_MAX,
} dolby_pq_mode_t;

typedef enum dolby_pq_item_e {
    DOLBY_PQ_BRIGHTNESS = 0,     /*Brightness */
    DOLBY_PQ_CONTRAST = 1,       /*Contrast */
    DOLBY_PQ_COLORSHIFT = 2,     /*ColorShift or Tint*/
    DOLBY_PQ_SATURATION = 3      /*Saturation or color */
} dolby_pq_item_t;

typedef enum dolby_pq_reset_e {
    DOLBY_RESET_ALL = 0,         /*reset picture mode / pq for all picture mode*/
    DOLBY_RESET_PQ_FOR_ALL = 1,  /*reset pq for all picture mode*/
    DOLBY_RESET_PQ_FOR_CUR = 2   /*reset pq for current picture mode */
} dolby_pq_reset_t;


// ***************************************************************************
// *** STRUCT definitions **********
// ***************************************************************************
typedef struct dolby_pic_mode_info_s {
    int dolby_pic_mode_id;
    unsigned char name[32];
}  __attribute__ ((aligned (8))) dolby_pic_mode_info_t;

typedef struct dolby_pq_info_s {
    int dolby_pic_mode_id;
    dolby_pq_item_e item;
    short value;
} __attribute__ ((aligned (8))) dolby_pq_info_t ;

typedef struct dolby_full_pq_info_s {
    int pic_mode_id;
    short brightness;  /*Brightness */
    short contrast;    /*Contrast */
    short colorshift;  /*ColorShift or Tint*/
    short saturation;  /*Saturation or color */
}  __attribute__ ((aligned (8))) dolby_full_pq_info_t;

typedef struct dolby_config_file_s {
	unsigned char bin_name[256];
	unsigned char cfg_name[256];
} __attribute__ ((aligned(8))) dolby_config_file_t;

// ***************************************************************************
// *** IOCTL definitions **********
// ***************************************************************************
#define DOLBY_IOC_MAGIC  'D'
/* get Number of Picture Mode */
#define DOLBY_IOC_GET_DV_PIC_MODE_NUM         _IOR((DOLBY_IOC_MAGIC), 0x0, int)
/* get Picture Mode Name of input pic_mode_id */
#define DOLBY_IOC_GET_DV_PIC_MODE_NAME        _IOWR((DOLBY_IOC_MAGIC), 0x1, struct dolby_pic_mode_info_s)
/* get current active picture mode */
#define DOLBY_IOC_GET_DV_PIC_MODE_ID          _IOR((DOLBY_IOC_MAGIC), 0x2, int)
/* select active picture mode */
#define DOLBY_IOC_SET_DV_PIC_MODE_ID          _IOW((DOLBY_IOC_MAGIC), 0x3, int)
/* get single pq(contrast or brightness or colorshift or saturation) */
#define DOLBY_IOC_GET_DV_SINGLE_PQ_VALUE      _IOWR((DOLBY_IOC_MAGIC), 0x4, struct dolby_pq_info_s)
/* get all pq(contrast, brightness,colorshift ,saturation) */
#define DOLBY_IOC_GET_DV_FULL_PQ_VALUE        _IOWR((DOLBY_IOC_MAGIC), 0x5, struct dolby_full_pq_info_s)
/* set single pq(contrast or brightness or colorshift or saturation) */
#define DOLBY_IOC_SET_DV_SINGLE_PQ_VALUE      _IOWR((DOLBY_IOC_MAGIC), 0x6, struct dolby_pq_info_s)
/* set all pq(contrast,brightness ,colorshift , saturation) */
#define DOLBY_IOC_SET_DV_FULL_PQ_VALUE        _IOWR((DOLBY_IOC_MAGIC), 0x7, struct dolby_full_pq_info_s)
/* reset all pq item  for current picture mode */
#define DOLBY_IOC_SET_DV_PQ_RESET             _IOWR((DOLBY_IOC_MAGIC), 0x8, enum dolby_pq_reset_e)
/* set Amlogic_cfg.txt and dv_config.bin patch */
#define DOLBY_IOC_SET_DV_CONFIG_FILE          _IOW((DOLBY_IOC_MAGIC), 0x9, struct dolby_config_file_s)

class CDolbyVision {
public:
    CDolbyVision(const char *binFilePath, const char *cfgFilePath);
    ~CDolbyVision();
    int SetDolbyCfgFile(const char *binFilePath, const char *cfgFilePath);
    int SetDolbyPQMode(dolby_pq_mode_t mode);
    dolby_pq_mode_t GetDolbyPQMode(void);
    int SetDolbyPQParam(dolby_pq_mode_t mode, dolby_pq_item_t iteamID, int value);
    int GetDolbyPQParam(dolby_pq_mode_t mode, dolby_pq_item_t iteamID);
    int SetDolbyPQFullParam(dolby_full_pq_info_t fullInfo);
    int GetDolbyPQFullParam(dolby_full_pq_info_t *fullInfo);
private:
    int DeviceIOCtl(int request, ...);
};
#endif /*__CDOLBY_VISION_H*/
