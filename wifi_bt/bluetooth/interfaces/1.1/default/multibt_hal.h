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

#ifndef FALSE
#define FALSE  0
#endif

#ifndef TRUE
#define TRUE   (!FALSE)
#endif

#ifndef DBG_IO
#define DBG_IO false
#endif

#ifndef PR_INFO
#define PR_INFO(param, ...)  {ALOGI("[%s-%d]: " param, __func__, __LINE__, ## __VA_ARGS__); }
#endif

#ifndef PR_DBG
#define PR_DBG(param, ...)  {if (DBG_IO) {ALOGD("[%s-%d]: " param, __func__, __LINE__, ## __VA_ARGS__); }}
#endif

#ifndef PR_ERR
#define PR_ERR(param, ...)  {ALOGE("[%s-%d]: " param, __func__, __LINE__, ## __VA_ARGS__); }
#endif

#ifndef MAILBOX_MODULE_NAME
#define MAILBOX_MODULE_NAME
#endif

#ifndef PROP_VALUE_MAX
#define PROP_VALUE_MAX      92
#endif

#ifndef UART_DEV_PORT_BT
#define UART_DEV_PORT_BT      "/dev/ttyS1"    /* android */
#endif

#define UPIO_BT_POWER_OFF 0
#define UPIO_BT_POWER_ON  1
#define SDIO_GET_DEV_TYPE       _IO('m',5)
#define CLR_BT_POWER_BIT        _IO('m',6)
#define GET_AML_WIFI_MODULE     _IO('m',7)

#define MAX_LINE_LEN 256
#define DELIM " =\n\t\r"
#define UNRE_IDENTIFICATION 0
#define RE_IDENTIFICATION 1
#define finit_module(fd, opts, flags) syscall(SYS_finit_module, fd, opts, flags)
extern "C" int delete_module(const char *, unsigned int);

 #define HCI_MAX_EVENT_SIZE     260

/* Manufacturer vendor info */
#define BT_VID_BROADCOM 0x0F00
#define BT_VID_QUALCOMM 0x1D00
#define BT_VID_REALTECK 0x5D00
#define BT_VID_MEDIATEK 0x4600
#define BT_VID_AMLOGIC  0xFFFF
#define BT_VID_UNISOC   0xEC01

#define BCM_VND_LIB "libbt-vendor_bcmMulti.so"
#define QCA_VND_LIB "libbt-vendor_qcaMulti.so"
#define QTI_VND_LIB "libbt-vendor_qtiMulti.so"
#define RTK_VND_LIB "libbt-vendor_rtlMulti.so"
#define MTK_VND_LIB "libbt-vendor_mtkMulti.so"
#define MT792_VND_LIB "libbt-vendor_792Multi.so"
#define AML_VND_LIB "libbt-vendor_amlMulti.so"
#define NXP_VND_LIB "libbt-vendor_nxpMulti.so"
#define UWE_VND_LIB "libbt-vendor_uweMulti.so"

#define NODE_PATH "/data/misc/bluetooth/bt_module"

#define PROP_RO_BTMODULE "ro.vendor.btmodule"
#define PROP_LIBBT_VENDOR "persist.vendor.libbt_vendor"
#define PROP_BT_MODULE "persist.vendor.bt_module"
#define PROP_BT_NAME "persist.vendor.bt_name"
#define PROP_WIFI_BT_NAME "persist.vendor.wifibt_name"  // Developer debugging set it manually

#define BT_POWER_EVT_1 "/sys/module/amlogic_wireless/parameters/btpower_evt"  // Kernel 5.15 btpower_evt path
#define BT_POWER_EVT_2 "/sys/module/bt_device/parameters/btpower_evt"  // Below kernel 5.15 btpower_evt path

#define BT_WAKE_EVT_1 "/sys/module/amlogic_wireless/parameters/btwake_evt"  // Kernel 5.15 btwake_evt path
#define BT_WAKE_EVT_2 "/sys/module/bt_device/parameters/btwake_evt"  // Below kernel 5.15 btwake_evt path

#define CONFIG_PATH "vendor/etc/bluetooth/"
#define CONFIG_NAME "bt_hal.conf"

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
typedef struct {
    char dev_name[PROP_VALUE_MAX];
    char mod_name[PROP_VALUE_MAX];
    char vnd_lib_name[PROP_VALUE_MAX];
    char wifi_bt_name[PROP_VALUE_MAX];
} prop_val;

/* Structure used to configure serial port during open */
typedef struct {
    uint16_t fmt;       // Data format
    uint8_t  baud;      // Baud rate
} uart_cfg;

typedef enum {
#if (BT_WAKE_VIA_USERIAL_IOCTL==TRUE)
    USERIAL_OP_ASSERT_BT_WAKE,
    USERIAL_OP_DEASSERT_BT_WAKE,
    USERIAL_OP_GET_BT_WAKE_STATE,
#endif
    USERIAL_OP_NOP,
} userial_vendor_ioctl_op_t;

typedef int (*action_ops)(const char *name, char *value);

typedef int (*insmod_act)(const char *filename, const char *args);
typedef int (*rmmod_act)(const char *modname);
typedef bool (*set_cfg_act)(void);
typedef bool (*get_cfg_act)(void);
typedef prop_val *(*prop_act)(void);
typedef bool (*vendor_act)(void);

typedef struct {
    const char *tag;
    action_ops tag_ops;
} tag_table;

typedef struct {
    insmod_act insmod_cb;
    rmmod_act rmmod_cb;
    set_cfg_act set_cfg_cb;
    get_cfg_act get_cfg_cb;
    prop_act prop_act_cb;
    vendor_act vendor_act_cb;
} vendor_hal;

extern const vendor_hal bt_vendor_hal;

#endif /* MULTIBT_HAL_H */
