/** @file drv_wrapper.h
 *
 *  @brief This file contains driver ioctl interface related definitions.
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


#ifndef _DRVWRAPPER_H
#define _DRVWRAPPER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>

#ifndef MFG_UPDATE

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
		int DeviceId;
		int Reserved;
	}Demux;

    } cmd_header;

#endif                          // MFG_UPDATE

#define HCI_Mode  0x00
#define MBTCHAR_Mode  0x01
#define BTRAWUR_Mode 0x02
/** HostCmd_DS_Gen */
    typedef struct _HostCmd_DS_Gen
    {
   /** Command */
        short command;
   /** Size */
        short size;
   /** Sequence number */
        short seq_num;
   /** Result */
        short result;
    } HostCmd_DS_Gen;

    int drv_wrapper_get_info(char *cmdname);
    int drv_wrapper_init(char *cmdname, int devid);
    int drv_wrapper_deinit();
    int drv_wrapper_ioctl(char *cmdname, unsigned char *p_buf, int *resp_len);
    int drv_wrapper_bridge_command(char *cmdname, unsigned char *buf,
                                   int *length, char *script);
    //multi WiFi dev

   typedef struct _new_drv_cb
    {

#ifdef NONPLUG_SUPPORT
        int hci_sock;           // socket for hci

        short ogf;
        short ocf;
#endif

        int wlan_sock;          // socket for wlan
        int sockfd;
        int ioctl_val;
        int subioctl_val;
        
        char *hci_ifname;
        char wlan_ifname[2048];

        char command[2048];
        unsigned char cb_buf[2048];
        char *load_script;
        char *unload_script;

#ifdef NONPLUG_SUPPORT
#endif

    } new_drv_cb;

    int drv_wrapper_init_multi(new_drv_cb *drv);
#ifdef NONPLUG_SUPPORT
    int drv_wrapper_get_hci_info(char *cmdname);
    int drv_wrapper_init_hci(char *cmdname);
    int drv_wrapper_deinit_hci();
    int drv_wrapper_send_hci_command(short ogf, short ocf,
                                     unsigned char *in_buf, int in_buf_len,
                                     unsigned char *out_buf, int *out_buf_len, int HciDeviceID);
#endif

#ifdef MFG_UPDATE

#define MAXBUF 8192

#define DRV_IF_HCI     BIT(0)
#define DRV_IF_WLAN    BIT(1)

    typedef struct _drv_config
    {
#ifdef NONPLUG_SUPPORT
        char hci_port[2][256];
#endif
        char wlan_port[256];
        char load_script[256];
        char unload_script[256];

    } drv_config, *pdrv_config;

    typedef struct _drv_cb
    {

#ifdef NONPLUG_SUPPORT
        int hci_sock;           // socket for hci

        short ogf;
        short ocf;
#endif

        int wlan_sock;          // socket for wlan
        char *hci_ifname_0;
		 char *hci_ifname_1;
        char *wlan_ifname;

        char command[MAXBUF];
        unsigned char cb_buf[MAXBUF];
        char *load_script;
        char *unload_script;

#ifdef NONPLUG_SUPPORT
#endif

    } drv_cb, *pdrv_cb;

#define WORD   short
#define DWORD  int

		typedef struct  PkHeader
		{
			WORD		cmd;		// message type
      WORD		len;
      WORD		seq;
      WORD		result;
      DWORD       MfgCmd;
      } PkHeader;

    typedef struct mfg_Cmd_t
   {
     PkHeader	header;
	   WORD		action;
	   WORD		deviceId;
	   DWORD		Error;  
    }mfg_Cmd_t;		//mfg_CmdMfg_t
    //;char secondWlanif_name[256] = "Wmlan0";
    
    struct _bridge_cb;          // forward declaration

    int drv_config_default(drv_config * drv_conf);
    int drv_init(struct _bridge_cb *bridge, drv_config * drv_conf);
//int drv_ioctl(drv_cb *drv, char *ifname, unsigned char *buf, int *msglen, int buflen);
    int drv_proc_wlan_command(drv_cb * drv, unsigned char *buf, int *msglen,
                              int buflen);
    int drv_proc_hci_command(drv_cb * drv, unsigned char *buf, int *msglen,
                             int buflen, int HciDeviceID);
    int drv_load_driver(struct _drv_cb *drv, int drv_if);
    int drv_unload_driver(struct _drv_cb *drv, int drv_if);

#endif                          // MFG_UPDATE

#ifdef __cplusplus
}
#endif
#endif                          /* _DRVWRAPPER_H */
