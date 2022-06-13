/*
 *  Copyright (c) 2016 MediaTek Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */
#ifndef __BTMTK_DEFINE_H__
#define __BTMTK_DEFINE_H__

#include "btmtk_config.h"

/**
 * Type definition
 */
#ifndef TRUE
	#define TRUE 1
#endif
#ifndef FALSE
	#define FALSE 0
#endif

#ifndef UNUSED
	#define UNUSED(x) (void)(x)
#endif

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

/**
 * Log level definition
 */
#define BTMTK_LOG_LEVEL_ERROR		1
#define BTMTK_LOG_LEVEL_WARNING		2
#define BTMTK_LOG_LEVEL_INFO		3
#define BTMTK_LOG_LEVEL_DEBUG		4
#define BTMTK_LOG_LEVEL_MAX		BTMTK_LOG_LEVEL_DEBUG
#define BTMTK_LOG_LEVEL_DEFAULT		BTMTK_LOG_LEVEL_INFO	/* default setting */

extern u8 btmtk_log_lvl;

#define BTUSB_ERR(fmt, ...)	 \
	do { if (btmtk_log_lvl >= BTMTK_LOG_LEVEL_ERROR) pr_warn("[btmtk_err] "fmt"\n", ##__VA_ARGS__); } while (0)
#define BTUSB_WARN(fmt, ...)	\
	do { if (btmtk_log_lvl >= BTMTK_LOG_LEVEL_WARNING) pr_warn("[btmtk_warn] "fmt"\n", ##__VA_ARGS__); } while (0)
#define BTUSB_INFO(fmt, ...)	\
	do { if (btmtk_log_lvl >= BTMTK_LOG_LEVEL_INFO) pr_warn("[btmtk_info] "fmt"\n", ##__VA_ARGS__); } while (0)
#define BTUSB_DBG(fmt, ...)	 \
	do { if (btmtk_log_lvl >= BTMTK_LOG_LEVEL_DEBUG) pr_warn("[btmtk_debug] "fmt"\n", ##__VA_ARGS__); } while (0)

#define BTUSB_INFO_RAW(p, l, fmt, ...)							\
		do {									\
			if (btmtk_log_lvl >= BTMTK_LOG_LEVEL_INFO) {			\
				int raw_count = 0;					\
				const unsigned char *ptr = p;				\
				pr_cont("[btmtk_info] "fmt, ##__VA_ARGS__);		\
				for (raw_count = 0; raw_count < l; ++raw_count) {	\
					pr_cont(" %02X", ptr[raw_count]);		\
				}							\
				pr_cont("\n");						\
			}								\
		} while (0)

#define BTUSB_DBG_RAW(p, l, fmt, ...)						\
	do {									\
		if (btmtk_log_lvl >= BTMTK_LOG_LEVEL_DEBUG) {			\
			int raw_count = 0;					\
			const unsigned char *ptr = p;				\
			pr_cont("[btmtk_debug] "fmt, ##__VA_ARGS__);		\
			for (raw_count = 0; raw_count < l; ++raw_count) {	\
				pr_cont(" %02X", ptr[raw_count]);		\
			}							\
			pr_cont("\n");						\
		}								\
	} while (0)

/**
 * SYS control
 */
#define SYSCTL	0x400000

/**
 * WLAN
 */
#define WLAN	0x410000

/**
 * MCUCTL
 */
#define CLOCK_CTL	0x0708
#define INT_LEVEL	0x0718
#define COM_REG0	0x0730
#define SEMAPHORE_00	0x07B0
#define SEMAPHORE_01	0x07B4
#define SEMAPHORE_02	0x07B8
#define SEMAPHORE_03	0x07BC

/**
 * Timeout setting, mescs
 */
#define USB_CTRL_IO_TIMO	300
#define USB_INTR_MSG_TIMO	2000

#if SUPPORT_MI_WOBLE
#define WOBLE_SUSPEND_EVENT_INTERVAL_TIMO	3000
#define WOBLE_SUSPEND_COMP_EVENT_TIMO		15000
#endif

#define WOBLE_EVENT_INTERVAL_TIMO	500
#define WOBLE_COMP_EVENT_TIMO		5000
#define DUMP_WAIT_TIMO_CNT	60
#define WAIT_WLAN_REMOVE_TIMO_CNT	30


/**
 * For chip reset pin
 */
#define RESET_PIN_SET_LOW_TIME		500

/**
 * USB request type definition
 */
#define DEVICE_VENDOR_REQUEST_OUT	0x40
#define DEVICE_VENDOR_REQUEST_IN	0xc0
#define DEVICE_CLASS_REQUEST_OUT	0x20
#define DEVICE_CLASS_REQUEST_IN		0xa0

#define BTUSB_MAX_ISOC_FRAMES	15
#define BTUSB_INTR_RUNNING	0
#define BTUSB_BULK_RUNNING	1
#define BTUSB_ISOC_RUNNING	2
#define BTUSB_SUSPENDING	3
#define BTUSB_DID_ISO_RESUME	4

/**
 * ROM patch related
 */
#define PATCH_HCI_HEADER_SIZE	4
#define PATCH_WMT_HEADER_SIZE	5
#define PATCH_HEADER_SIZE	(PATCH_HCI_HEADER_SIZE + PATCH_WMT_HEADER_SIZE)
#define UPLOAD_PATCH_UNIT	2048
#define PATCH_INFO_SIZE		30
#define PATCH_PHASE1		1
#define PATCH_PHASE2		2
#define PATCH_PHASE3		3
#define PATCH_LEN_ILM		(192 * 1024)

#define META_BUFFER_SIZE	(1024 * 500)
#define USB_IO_BUF_SIZE		(HCI_MAX_EVENT_SIZE > 256 ? HCI_MAX_EVENT_SIZE : 256)
#define HCI_SNOOP_ENTRY_NUM	30
#define HCI_SNOOP_BUF_SIZE	32
#define FW_VERSION_BUF_SIZE	14

/**
 * stpbtfwlog device node
 */
#define HCI_MAX_COMMAND_SIZE		255

/**
 * Write a char to buffer.
 * ex : echo 01 be > /dev/stpbtfwlog
 * "01 " need three bytes.
 */
#define HCI_MAX_COMMAND_BUF_SIZE	(HCI_MAX_COMMAND_SIZE * 3)

/**
 * HCI CMD/ACL/SCO Header length
 */
#define HCI_CMD_HEADER_LEN	(4)
#define HCI_ACL_HEADER_LEN	(5)
#define HCI_SCO_HEADER_LEN	(4)

/**
 * stpbt device node
 */
#define BUFFER_SIZE		(1024 * 4)	/* Size of RX Queue */
#define IOC_MAGIC		0xb0
#define IOCTL_FW_ASSERT		_IOWR(IOC_MAGIC, 0, void *)
#define IOCTL_SET_ISOC_IF_ALT	_IOWR(IOC_MAGIC, 1, int)	/* Set interface & alternate */

/**
 * fw log queue count
 */
#define FWLOG_QUEUE_COUNT		200
#define FWLOG_BLUETOOTH_KPI_QUEUE_COUNT	200
#define FWLOG_ASSERT_QUEUE_COUNT	6000

/**
 * Maximum rom patch file name length
 */
#define MAX_BIN_FILE_NAME_LEN 32

/**
 * Wlan status define
 */
#define WLAN_STATUS_IS_NOT_LOAD		-1
#define WLAN_STATUS_DEFAULT		0
#define WLAN_STATUS_CALL_REMOVE_START	1 /* WIFI driver is inserted */

/**
 * L0 reset
 */
#define L0_RESET_TAG				"[SER][L0] "

/**
 * WoBLE by BLE RC
 */
#ifndef SUPPORT_LEGACY_WOBLE
	#define BT_RC_VENDOR_DEFAULT 1
	#define BT_RC_VENDOR_S0 0
#endif

/**
 * Disable RESUME_RESUME
 */
#ifndef BT_DISABLE_RESET_RESUME
	#define BT_DISABLE_RESET_RESUME 0
#endif

#if SUPPORT_MT7668
#define WOBLE_SETTING_FILE_NAME_7668 "woble_setting_7668.bin"
#endif

#if SUPPORT_MT7663
#define WOBLE_SETTING_FILE_NAME_7663 "woble_setting_7663.bin"
#endif

/* Backward compatibility */
#define WOBLE_SETTING_FILE_NAME "woble_setting.bin"
#define BT_CFG_NAME "bt.cfg"
#define BT_UNIFY_WOBLE "SUPPORT_UNIFY_WOBLE"
#define BT_UNIFY_WOBLE_TYPE "UNIFY_WOBLE_TYPE"
#define BT_LEGACY_WOBLE "SUPPORT_LEGACY_WOBLE"
#define BT_WOBLE_BY_EINT "SUPPORT_WOBLE_BY_EINT"
#define BT_DONGLE_RESET_PIN "BT_DONGLE_RESET_GPIO_PIN"
#define BT_SAVE_FW_DUMP_IN_KERNEL "SAVE_FW_DUMP_IN_KERNEL"
#define BT_SYS_LOG_FILE "SYS_LOG_FILE_NAME"
#define BT_FW_DUMP_FILE "FW_DUMP_FILE_NAME"
#define BT_RESET_DONGLE "SUPPORT_DONGLE_RESET"
#define BT_FULL_FW_DUMP "SUPPORT_FULL_FW_DUMP"
#define BT_WOBLE_WAKELOCK "SUPPORT_WOBLE_WAKELOCK"
#define BT_WOBLE_FOR_BT_DISABLE "SUPPORT_WOBLE_FOR_BT_DISABLE"
#define BT_RESET_STACK_AFTER_WOBLE "RESET_STACK_AFTER_WOBLE"
#define BT_AUTO_PICUS "SUPPORT_AUTO_PICUS"
#define BT_AUTO_PICUS_FILTER "PICUS_FILTER_CMD"
#define BT_WMT_CMD "WMT_CMD"
#define BT_VENDOR_CMD "VENDOR_CMD"

#define WMT_CMD_COUNT 255
#define VENDOR_CMD_COUNT 255
#define WOBLE_SETTING_COUNT 10

#define WOBLE_FAIL -10

#define COMMAND_TIMED_OUT_ERROR_CODE (-110)

#define PM_KEY_BTW (0x0015) /* Notify PM the unify woble type */

#endif /* __BTMTK_DEFINE_H__ */
