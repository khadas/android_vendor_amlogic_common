/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "SystemControl"
#define LOG_TV_TAG "SSMHandler"

#include "CPQLog.h"
#include "SSMHandler.h"

android::Mutex SSMHandler::sLock;
SSMHandler* SSMHandler::mSSMHandler = NULL;

struct SSMHeader_section2_t gSSMHeader_section2[] = {
    {.id =CHKSUM_PROJECT_ID_OFFSET, .addr = 0, .size = 2, .valid = 0, .rsv = {0}},
    {.id =CHKSUM_MAC_ADDRESS_OFFSET, .addr = 0, .size = 2, .valid = 0, .rsv = {0}},
    {.id =CHKSUM_HDCP_KEY_OFFSET, .addr = 0, .size = 2, .valid = 0, .rsv = {0}},
    {.id =CHKSUM_BARCODE_OFFSET, .addr = 0, .size = 4, .valid = 0, .rsv = {0}},
    {.id =4, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =5, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =6, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =7, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =8, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =9, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =SSM_RSV_W_CHARACTER_CHAR_START, .addr = 0, .size = 10, .valid = 0, .rsv = {0}},
    {.id =SSM_CR_START, .addr = 0, .size = 1536, .valid = 0, .rsv = {0}},
    {.id =SSM_MARK_01_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =SSM_MARK_02_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =SSM_MARK_03_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =15, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =16, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =17, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =18, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =19, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =SSM_RSV0, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =SSM_RW_FBMF_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =SSM_RW_DEF_HDCP_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =SSM_RW_POWER_CHANNEL_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =SSM_RW_LAST_SOURCE_INPUT_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =SSM_RW_SYS_LANGUAGE_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =SSM_RW_AGING_MODE_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =SSM_RW_PANEL_TYPE_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =SSM_RW_POWER_ON_MUSIC_SWITCH_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =SSM_RW_POWER_ON_MUSIC_VOL_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =SSM_RW_SYS_SLEEP_TIMER_START, .addr = 0, .size = 4, .valid = 0, .rsv = {0}},
    {.id =SSM_RW_INPUT_SRC_PARENTAL_CTL_START, .addr = 0, .size = 4, .valid = 0, .rsv = {0}},
    {.id =SSM_RW_PARENTAL_CTL_SWITCH_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =SSM_RW_PARENTAL_CTL_PASSWORD_START, .addr = 0, .size = 16, .valid = 0, .rsv = {0}},
    {.id =SSM_RW_SEARCH_NAVIGATE_FLAG_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =SSM_RW_INPUT_NUMBER_LIMIT_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =SSM_RW_SERIAL_ONOFF_FLAG_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =SSM_RW_STANDBY_MODE_FLAG_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =SSM_RW_HDMIEQ_MODE_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =SSM_RW_LOGO_ON_OFF_FLAG_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =SSM_RW_HDMIINTERNAL_MODE_START, .addr = 0, .size = 4, .valid = 0, .rsv = {0}},
    {.id =SSM_RW_DISABLE_3D_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =SSM_RW_GLOBAL_OGO_ENABLE_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =SSM_RW_LOCAL_DIMING_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =SSM_RW_VDAC_2D_START, .addr = 0, .size = 2, .valid = 0, .rsv = {0}},
    {.id =SSM_RW_VDAC_3D_START, .addr = 0, .size = 2, .valid = 0, .rsv = {0}},
    {.id =SSM_RW_NON_STANDARD_START, .addr = 0, .size = 2, .valid = 0, .rsv = {0}},
    {.id =SSM_RW_ADB_SWITCH_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =SSM_RW_SERIAL_CMD_SWITCH_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =SSM_RW_CA_BUFFER_SIZE_START, .addr = 0, .size = 2, .valid = 0, .rsv = {0}},
    {.id =SSM_RW_NOISE_GATE_THRESHOLD_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =SSM_RW_DTV_TYPE_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =SSM_RW_UI_GRHPHY_BACKLIGHT_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =SSM_RW_FASTSUSPEND_FLAG_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =SSM_RW_BLACKOUT_ENABLE_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =SSM_RW_PANEL_ID_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =56, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =57, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =58, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =59, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =60, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =61, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =62, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =63, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =64, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =65, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =66, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =67, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =68, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =69, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =70, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =71, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =72, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =73, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =74, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =75, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =76, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =77, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =78, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =79, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =SSM_RSV_1, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =81, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =82, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =83, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =84, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =85, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =86, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =87, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =88, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =89, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =90, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =91, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =92, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =93, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =94, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =95, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =96, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =97, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =98, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =99, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =100, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =101, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =102, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =103, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =104, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =105, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =106, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =107, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =108, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =109, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =110, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =111, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =112, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =113, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =114, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =115, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =116, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =117, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =118, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =119, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =120, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =121, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =122, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =123, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =124, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =125, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =126, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =127, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =128, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =129, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =130, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =131, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =132, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =133, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =134, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =135, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =136, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =137, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =138, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =139, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =140, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =141, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =142, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =143, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =144, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =145, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =146, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =147, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =148, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =149, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =SSM_RSV_2, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_COLOR_DEMO_MODE_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_COLOR_BASE_MODE_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_TEST_PATTERN_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_DDR_SSC_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_LVDS_SSC_START, .addr = 0, .size = 2, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_DREAM_PANEL_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_BACKLIGHT_REVERSE_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_BRIGHTNESS_START, .addr = 0, .size = SSM_SOURCE_MAX, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_CONTRAST_START, .addr = 0, .size = SSM_SOURCE_MAX, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_SATURATION_START, .addr = 0, .size = SSM_SOURCE_MAX, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_HUE_START, .addr = 0, .size = SSM_SOURCE_MAX, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_SHARPNESS_START, .addr = 0, .size = SSM_SOURCE_MAX, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_COLOR_TEMPERATURE_START, .addr = 0, .size = SSM_SOURCE_MAX, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_NOISE_REDUCTION_START, .addr = 0, .size = SSM_SOURCE_MAX, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_SCENE_MODE_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_PICTURE_MODE_START, .addr = 0, .size = SSM_SOURCE_MAX, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_DISPLAY_MODE_START, .addr = 0, .size = SSM_SOURCE_MAX, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_BACKLIGHT_START, .addr = 0, .size = SSM_SOURCE_MAX, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_RGB_GAIN_R_START, .addr = 0, .size = 4, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_RGB_GAIN_G_START, .addr = 0, .size = 4, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_RGB_GAIN_B_START, .addr = 0, .size = 4, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_RGB_POST_OFFSET_R_START, .addr = 0, .size = 4, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_RGB_POST_OFFSET_G_START, .addr = 0, .size = 4, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_RGB_POST_OFFSET_B_START, .addr = 0, .size = 4, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_DBC_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_PROJECT_ID_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_DNLP_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_PANORAMA_START, .addr = 0, .size = SSM_SOURCE_MAX, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_APL_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_APL2_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_BD_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_BP_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_RGB_START, .addr = 0, .size = 18, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_COLOR_SPACE_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_USER_NATURE_SWITCH_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_GAMMA_VALUE_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_DBC_BACKLIGHT_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_DBC_STANDARD_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_DBC_ENABLE_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_FBC_BACKLIGHT_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_FBC_ELECMODE_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_FBC_COLORTEMP_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_FBC_N310_BACKLIGHT_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_FBC_N310_COLORTEMP_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_FBC_N310_LIGHTSENSOR_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_FBC_N310_MEMC_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_FBC_N310_DREAMPANEL_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_FBC_N310_MULTI_PQ_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_N311_VBYONE_SPREAD_SPECTRUM_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_N311_BLUETOOTH_VAL_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_EYE_PROTECTION_MODE_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_DNLP_MODE_START, .addr = 0, .size = SSM_SOURCE_MAX, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_DNLP_GAIN_START, .addr = 0, .size = SSM_SOURCE_MAX, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_LOCAL_CONTRAST_MODE_START, .addr = 0, .size = SSM_SOURCE_MAX, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_LAST_PICTURE_MODE_START, .addr = 0, .size = SSM_SOURCE_MAX, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_AIPQ_ENABLE_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_SMOOTH_PLUS_START, .addr = 0, .size = SSM_SOURCE_MAX, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_HDR_TMO_START, .addr = 0, .size = SSM_SOURCE_MAX, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_BLACK_EXTENSION_MODE_START, .addr = 0, .size = SSM_SOURCE_MAX, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_DEBLOCK_MODE_START, .addr = 0, .size = SSM_SOURCE_MAX, .valid = 0, .rsv = {0}},
    {.id =VPP_DATA_POS_DEMOSQUITO_MODE_START, .addr = 0, .size = SSM_SOURCE_MAX, .valid = 0, .rsv = {0}},
    {.id =212, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =213, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =214, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =215, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =216, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =217, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =218, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =219, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =220, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =221, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =222, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =223, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =224, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =225, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =226, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =227, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =228, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =229, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =230, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =231, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =232, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =233, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =234, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =235, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =236, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =237, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =238, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =239, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =240, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =241, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =242, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =243, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =244, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =245, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =246, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =247, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =248, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =249, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =SSM_RSV3, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =TVIN_DATA_POS_SOURCE_INPUT_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =TVIN_DATA_CVBS_STD_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =TVIN_DATA_POS_3D_MODE_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =TVIN_DATA_POS_3D_LRSWITCH_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =TVIN_DATA_POS_3D_DEPTH_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =TVIN_DATA_POS_3D_TO2D_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =TVIN_DATA_POS_3D_TO2DNEW_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =258, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =259, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =260, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =261, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =262, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =263, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =264, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =265, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =266, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =267, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =268, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =269, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =270, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =271, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =272, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =273, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =274, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =275, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =276, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =277, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =278, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =279, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =SSM_RSV4, .addr = 0, .size = 0, .valid = 0, .rsv = {0}},
    {.id =CUSTOMER_DATA_POS_HDMI1_EDID_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =CUSTOMER_DATA_POS_HDMI2_EDID_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =CUSTOMER_DATA_POS_HDMI3_EDID_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =CUSTOMER_DATA_POS_HDMI4_EDID_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =CUSTOMER_DATA_POS_HDMI_HDCP_SWITCHER_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =CUSTOMER_DATA_POS_HDMI_COLOR_RANGE_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
    {.id =CUSTOMER_DATA_POS_DYNAMIC_BACKLIGHT, .addr = 0, .size = SSM_SOURCE_MAX, .valid = 0, .rsv = {0}},
    {.id =CUSTOMER_DATA_POS_EDGE_ENHANCER, .addr = 0, .size = SSM_SOURCE_MAX, .valid = 0, .rsv = {0}},
    {.id =CUSTOMER_DATA_POS_MPEG_NOISE_REDUCTION, .addr = 0, .size = SSM_SOURCE_MAX, .valid = 0, .rsv = {0}},
    {.id =CUSTOMER_DATA_POS_DYNAMIC_CONTRAST, .addr = 0, .size = SSM_SOURCE_MAX, .valid = 0, .rsv = {0}},
    {.id =CUSTOMER_DATA_POS_AUTO_ASPECT, .addr = 0, .size = SSM_SOURCE_MAX, .valid = 0, .rsv = {0}},
    {.id =CUSTOMER_DATA_POS_43_STRETCH, .addr = 0, .size = SSM_SOURCE_MAX, .valid = 0, .rsv = {0}},
    {.id =CUSTOMER_DATA_POS_SCREEN_COLOR_START, .addr = 0, .size = 1, .valid = 0, .rsv = {0}},
};

struct SSMHeader_section1_t gSSMHeader_section1 =
{
    .magic = 0x8f8f8f8f, .version = 2021811022, .count = SSM_DATA_MAX, .rsv = {0}
};

SSMHandler* SSMHandler::GetSingletonInstance(const char *SSMHandlerPath)
{
    android::Mutex::Autolock _l(sLock);

    if (!mSSMHandler) {
        mSSMHandler = new SSMHandler();
        strcpy(mSSMHandler->mSSMHandlerPath, SSMHandlerPath);

        if (mSSMHandler && !mSSMHandler->Construct()) {
            delete mSSMHandler;
            mSSMHandler = NULL;
        }
    }

    return mSSMHandler;
}

SSMHandler::SSMHandler()
{
    unsigned int sum = 0;

    memset(&mSSMHeader_section1, 0, sizeof (SSMHeader_section1_t));

    for (unsigned int i = 1; i < gSSMHeader_section1.count; i++) {
        sum += gSSMHeader_section2[i-1].size;

        gSSMHeader_section2[i].addr = sum;
    }
}

SSMHandler::~SSMHandler()
{
    if (mFd > 0) {
        close(mFd);
        mFd = -1;
    }
}

bool SSMHandler::Construct()
{
    bool ret = true;

    mFd = open(mSSMHandlerPath, O_RDWR | O_SYNC | O_CREAT, S_IRUSR | S_IWUSR);

    if (-1 == mFd) {
        ret = false;
        SYS_LOGD ("%s, Open %s failure\n", __FUNCTION__, mSSMHandlerPath);
    }

    return ret;
}

SSM_status_t SSMHandler::SSMSection1Verify()
{
    SSM_status_t ret = SSM_HEADER_VALID;

    lseek(mFd, 0, SEEK_SET);
    ssize_t ssize = read(mFd, &mSSMHeader_section1, sizeof(SSMHeader_section1_t));

    if (ssize != sizeof(SSMHeader_section1_t) ||
        mSSMHeader_section1.magic != gSSMHeader_section1.magic ||
        mSSMHeader_section1.count != gSSMHeader_section1.count) {
        ret = SSM_HEADER_INVALID;
    }

    if (ret != SSM_HEADER_INVALID &&
        mSSMHeader_section1.version != gSSMHeader_section1.version) {
        ret = SSM_HEADER_STRUCT_CHANGE;
    }

    return ret;
}

SSM_status_t SSMHandler::SSMSection2Verify(SSM_status_t SSM_status)
{
    return SSM_status;
}

bool SSMHandler::SSMRecreateHeader()
{
    bool ret = true;

    ftruncate(mFd, 0);
    lseek(mFd, 0, SEEK_SET);

    //cal Addr and write
    write(mFd, &gSSMHeader_section1, sizeof (SSMHeader_section1_t));
    write(mFd, gSSMHeader_section2, gSSMHeader_section1.count * sizeof (SSMHeader_section2_t));

    return ret;
}

unsigned int SSMHandler::SSMGetActualAddr(int id)
{
    return gSSMHeader_section2[id].addr;
}

unsigned int SSMHandler::SSMGetActualSize(int id)
{
    return gSSMHeader_section2[id].size;
}

SSM_status_t SSMHandler::SSMVerify()
{
    return  SSMSection2Verify(SSMSection1Verify());
}

SSMHandler& SSMHandler::operator = (const SSMHandler& obj)
{
    return const_cast<SSMHandler&>(obj);
}
