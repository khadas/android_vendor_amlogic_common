/******************************************************************************
 *
 *  Copyright (C) 2021-2021 amlogic Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/
#ifndef MULTIBT_HAL_H
#define MULTIBT_HAL_H

#include <utils/Log.h>
#include "userial.h"

#ifndef BT_DBG
#define BT_DBG TRUE
#endif

#if (BT_DBG == TRUE)
#define PR_INFO(param, ...) {ALOGD(param, ## __VA_ARGS__);}
#else
#define PR_INFO(param, ...) {}
#endif

#define PR_ERR(param, ...) {ALOGE(param, ## __VA_ARGS__);}

#ifndef FALSE
#define FALSE  0
#endif

#ifndef TRUE
#define TRUE   (!FALSE)
#endif

#ifndef BLUETOOTH_UART_DEVICE_PORT
#define BLUETOOTH_UART_DEVICE_PORT      "/dev/ttyS1"    /* android */
#endif

#define UPIO_BT_POWER_OFF 0
#define UPIO_BT_POWER_ON  1
#define SDIO_GET_DEV_TYPE       _IO('m',5)


 #define HCI_MAX_EVENT_SIZE     260

/*vendor info*/
#define BT_VENDOR_ID_BROADCOM 0x0F00
#define BT_VENDOR_ID_QUALCOMM 0x1D00
#define BT_VENDOR_ID_REALTECK 0x5D00
#define BT_VENDOR_ID_MEDIATEK 0x4600
#define BT_VENDOR_ID_AMLOGIC  0XFFFF

#define BCM_VENDOR_LIB "libbt-vendor_bcmMulti.so"
#define QCA_VENDOR_LIB "libbt-vendor_qcaMulti.so"
#define RTK_VENDOR_LIB "libbt-vendor_rtlMulti.so"
#define MTK_VENDOR_LIB "libbt-vendor_mtkMulti.so"
#define AML_VENDOR_LIB "libbt-vendor_amlMulti.so"
#define NODE_PATH "/data/misc/bluetooth/bt_module"

/**** baud rates ****/
#define USERIAL_BAUD_300        0
#define USERIAL_BAUD_600        1
#define USERIAL_BAUD_1200       2
#define USERIAL_BAUD_2400       3
#define USERIAL_BAUD_9600       4
#define USERIAL_BAUD_19200      5
#define USERIAL_BAUD_57600      6
#define USERIAL_BAUD_115200     7
#define USERIAL_BAUD_230400     8
#define USERIAL_BAUD_460800     9
#define USERIAL_BAUD_921600     10
#define USERIAL_BAUD_1M         11
#define USERIAL_BAUD_1_5M       12
#define USERIAL_BAUD_2M         13
#define USERIAL_BAUD_3M         14
#define USERIAL_BAUD_4M         15
#define USERIAL_BAUD_AUTO       16

/**** Data Format ****/
/* Stop Bits */
#define USERIAL_STOPBITS_1      1
#define USERIAL_STOPBITS_1_5    (1<<1)
#define USERIAL_STOPBITS_2      (1<<2)

/* Parity Bits */
#define USERIAL_PARITY_NONE     (1<<3)
#define USERIAL_PARITY_EVEN     (1<<4)
#define USERIAL_PARITY_ODD      (1<<5)

/* Data Bits */
#define USERIAL_DATABITS_5      (1<<6)
#define USERIAL_DATABITS_6      (1<<7)
#define USERIAL_DATABITS_7      (1<<8)
#define USERIAL_DATABITS_8      (1<<9)


#if (BT_WAKE_VIA_USERIAL_IOCTL==TRUE)
/* These are the ioctl values used for bt_wake ioctl via UART driver. you may
 * need to redefine them on you platform!
 * Logically they need to be unique and not colide with existing uart ioctl's.
 */
#ifndef USERIAL_IOCTL_BT_WAKE_ASSERT
#define USERIAL_IOCTL_BT_WAKE_ASSERT   0x8003
#endif
#ifndef USERIAL_IOCTL_BT_WAKE_DEASSERT
#define USERIAL_IOCTL_BT_WAKE_DEASSERT 0x8004
#endif
#ifndef USERIAL_IOCTL_BT_WAKE_GET_ST
#define USERIAL_IOCTL_BT_WAKE_GET_ST   0x8005
#endif
#endif // (BT_WAKE_VIA_USERIAL_IOCTL==TRUE)

/******************************************************************************
**  Type definitions
******************************************************************************/

/* Structure used to configure serial port during open */
typedef struct
{
    uint16_t fmt;       /* Data format */
    uint8_t  baud;      /* Baud rate */
} tUSERIAL_CFG;

typedef enum {
#if (BT_WAKE_VIA_USERIAL_IOCTL==TRUE)
    USERIAL_OP_ASSERT_BT_WAKE,
    USERIAL_OP_DEASSERT_BT_WAKE,
    USERIAL_OP_GET_BT_WAKE_STATE,
#endif
    USERIAL_OP_NOP,
} userial_vendor_ioctl_op_t;

typedef int (*vendor_act)(void);
typedef void (*userial_init_act)(void);
typedef void (*userial_close_act)(void);
typedef int (*get_module_name_act)(char* str);
typedef int (*userial_open_act)(tUSERIAL_CFG *p_cfg);
struct vendor_action {
	userial_init_act userial_init;
	userial_open_act userial_open;
	userial_close_act userial_close;
	get_module_name_act get_module_name;
	vendor_act uart_module;
	vendor_act usb_module;
	vendor_act mmc_module;
	vendor_act pci_module;
	vendor_act vendor_lib;
};

extern const struct vendor_action btvendor_hal;

#endif /* MULTIBT_HAL_H */
