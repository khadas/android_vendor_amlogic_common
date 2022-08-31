/** @file mfgbridge.h
 *
 *  @brief This file contains MFG bridge code definitions
 *
 * (C) Copyright 2011-2017 Marvell International Ltd. All Rights Reserved
 *
 * MARVELL CONFIDENTIAL
 * The source code contained or described herein and all documents related to
 * the source code ("Material") are owned by Marvell International Ltd or its
 * suppliers or licensors. Title to the Material remains with Marvell International Ltd
 * or its suppliers and licensors. The Material contains trade secrets and
 * proprietary and confidential information of Marvell or its suppliers and
 * licensors. The Material is protected by worldwide copyright and trade secret
 * laws and treaty provisions. No part of the Material may be used, copied,
 * reproduced, modified, published, uploaded, posted, transmitted, distributed,
 * or disclosed in any way without Marvell's prior express written permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery
 * of the Materials, either expressly, by implication, inducement, estoppel or
 * otherwise. Any license under such intellectual property rights must be
 * express and approved by Marvell in writing.
 *
 */


#ifndef _MFGBRIDGE_H
#define _MFGBRIDGE_H

/** Type definition: boolean */
typedef enum
{
    FALSE,
    TRUE
} boolean;

/** Type definition:Protocol */
typedef enum
{
    UDP,
    TCP
} protocol;

/** UART start pattern*/
typedef struct _uart_header
{
    /** pattern */
    short pattern;
    /** Command length */
    short length;
} uart_header;

#ifndef MFG_UPDATE

/* Command type */
/** Command type: Local */
#define TYPE_LOCAL 0x0001
/** Command type: WLAN */
#define TYPE_WLAN  0x0002
/** Command type: BT */
#define TYPE_HCI   0x0003
/** mode auto */
#define MODE_AUTO 0x00
/** mode ethernet*/
#define MODE_ETHERNET 0x01

#else // MFG_UPDATE

#define UART_BUF_SIZE    2048

typedef enum
{
    UART,
    NET,                        // Ethernet
} port_type;

typedef struct _uart_cb
{                               // uart control block
    int uart_fd;
    unsigned int crc32_table[256];

    unsigned char uart_buf[UART_BUF_SIZE];      // uart buffer

    int (*uart_download) (struct _uart_cb * uart, char *file, int filelen);
} uart_cb, *puart_cb;

typedef struct _net_cb
{
    int proto;                  /* Default protocol UDP */
    int skfd;                   /* socket for listen */
    int currfd;                 /* current socket for process */
    int server_port;
    int client_port;

//    struct sockaddr_in  client;
    int (*net_download) (struct _net_cb * net, char *file, int filelen);

} net_cb, *pnet_cb;

struct _drv_cb;                 // forward declaration
struct _drv_config;

typedef struct _bridge_cb
{
    int mode;                   /* Default mode Auto */

#ifdef NONPLUG_SUPPORT
    uart_cb uart;
#endif

    net_cb net;

//    int (*drv_load)(struct _drv_cb *drv, char* ifname, char *script);
//    int (*drv_unload)(struct _drv_cb *drv, char* ifname, char *script);
    int (*drv_load) (struct _drv_cb * drv, int drv_if);
    int (*drv_unload) (struct _drv_cb * drv, int drv_if);
    int (*drv_proc_wlan_cmd) (struct _drv_cb * drv, unsigned char *cmdbuf,
                              int *cmdlen, int buflen);
    int (*drv_proc_hci_cmd) (struct _drv_cb * drv, unsigned char *cmdbuf,
                             int *cmdlen, int buflen, int HciDeviceID);

    struct _drv_cb *drv;

} bridge_cb, *pbridge_cb;

struct bridge_ops
{

    int (*uart_download) (struct _uart_cb * uart, char *file, int filelen);

    int (*net_download) (struct _net_cb * net, char *file, int filelen);

    int (*drv_load) (struct _drv_cb * drv, char *ifname, char *script);

    int (*drv_unload) (struct _drv_cb * drv, char *ifname, char *script);

    int (*drv_proc_wifi_cmd) (struct _drv_cb * drv,
                              char *ifname,
                              unsigned char *cmdbuf, int *cmdlen, int buflen);

    int (*drv_proc_hci_cmd) (struct _drv_cb * drv,
                             unsigned char *cmdbuf, int *cmdlen, int buflen);

};

/** mode auto */
#define MODE_AUTO 0x00
/** mode ethernet*/
#define MODE_ETHERNET 0x01

/* Command type */
/** Command type: Local */
#define TYPE_LOCAL 0x0001
/** Command type: WLAN */
#define TYPE_WLAN  0x0002
/** Command type: BT */
#define TYPE_HCI   0x0003

/* Command sub-type */
/** Command sub-type: Load Driver */
#define SUB_TYPE_LOAD_DRV         0x0001
/** Command sub-type: Unload Driver */
#define SUB_TYPE_UNLOAD_DRV       0x0002
/** Command sub-type: Get bridge config */
#define SUB_TYPE_GET_BRIDGE_CFG   0x0003
/** Command sub-type: Set bridge config */
#define SUB_TYPE_SET_BRIDGE_CFG   0x0004
/** Command sub-type: upgrade fw image*/
#define SUB_TYPE_UPDT_FW 0x0005

/** Labtool command header */
typedef struct _cmd_header
{
    /** Command Type */
    short type;
    /** Command Sub-type */
    short sub_type;
    /** Command length (header+payload) */
    short length;
    /** Command status */
    short status;
    /** reserved */
    union
    {
	int DeviceID;
        int Reserved1;
     }Demux;
} cmd_header;

#endif // MFG_UPDATE

#endif /** _MFGBRIDGE_H */
