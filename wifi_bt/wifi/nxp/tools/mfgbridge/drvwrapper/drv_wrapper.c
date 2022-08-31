/** @file drv_wrapper.cpp
 *
 *  @brief This file contains driver ioctl interface related code.
 *
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
 

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdlib.h>
#include <poll.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <linux/if.h>
#include <linux/ioctl.h>
#include <linux/wireless.h>
#ifdef NONPLUG_SUPPORT
#if !defined MARVELL_BT_STACK && !defined RAWUR_BT_STACK
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#endif //#ifndef MARVELL_BT_STACK
#endif
#include "drv_wrapper.h"

#ifdef MFG_UPDATE

#include "mfgbridge.h"
#include "mfgdebug.h"

#endif

#ifdef RAWUR_BT_STACK
#include <termios.h>
#include <sys/param.h>
#include <sys/select.h>
#include <sys/time.h>
#endif


/********************************************************
        Local Variables
********************************************************/
#define IW_MAX_PRIV_DEF   128
#define MLAN_ETH_PRIV               (SIOCDEVPRIVATE + 14)

#define BRDG_OFFSET 4

#define SPEC_NFC_EVENT 0xf

//static int ethio_flag = 1;
extern int ethio_flag;
static int        raw_uart_fd = -1;

extern char RAW_UART_PORT[128];      /* default hci Uart port */
static int jrtrn;
struct timeval tvout = { 0, 50000 }; 
fd_set fds;

/** Private command structure */
typedef struct eth_priv_cmd
{
    /** Command buffer */
    char *buf;
    /** Used length */
    unsigned int used_len;
    /** Total length */
    unsigned int total_len;
} eth_cmd;

static struct ifreq userdata;
static struct iw_priv_args priv_args[IW_MAX_PRIV_DEF];
static int sockfd=0, ioctl_val, subioctl_val;
#ifdef NONPLUG_SUPPORT
static int hci_sock_char_nfc = 0;
static int hci_sock_char_fm = 0;
static int hci_sock_bz = 0;
static char *hci_addr = NULL;
static char addr[18];
static char *hci_intf = "hci0";
static int use_char = 0;
static int multi_hci_sock_bz[2]={0};
#if !defined MARVELL_BT_STACK && !defined RAWUR_BT_STACK
static bdaddr_t  multi_hci_addr;
#endif //#ifndef MARVELL_BT_STACK
#endif

#ifdef MFG_UPDATE
static struct _drv_cb Driver;
struct _new_drv_cb *Driver1, *MultiDevPtr=NULL;
int  WiFidevicecnt=1;

static int BT_IF_MODE = HCI_Mode;
#endif // MFG_UPDATE
/********************************************************
        Local Functions
********************************************************/
/**
 *  @brief Get private info.
 *
 *  @param ifName   A pointer to net name
 *  @return         Number of private IOCTLs, -1 on failure
 */
static int
get_private_info_multi(const char *ifName, int sockfd_multi)
{
	struct iwreq iwReq;
	int status = 0;
	struct iw_priv_args *pPrivArgs = priv_args;

	memset(&iwReq, 0, sizeof(iwReq));
	strncpy(iwReq.ifr_name, ifName, IFNAMSIZ);
	iwReq.u.data.pointer = (caddr_t) pPrivArgs;
	iwReq.u.data.length = IW_MAX_PRIV_DEF;
	iwReq.u.data.flags = 0;

	if (ioctl(sockfd_multi, SIOCGIWPRIV, &iwReq) < 0) {
		perror("ioctl[SIOCGIWPRIV]");
		status = -1;
	} else {
		/* Return the number of private ioctls */
		status = iwReq.u.data.length;
	}
	
	return status;
}

/**
 *  @brief Get Sub command ioctl number
 *
 *  @param cmdIndex		command index
 *  @param privCnt   	Total number of private ioctls availabe in driver
 *  @param ioctlVal    	A pointer to return ioctl number
 *  @param subIoctlVal 	A pointer to return sub-ioctl number
 *  @return             0 on success, otherwise -1
 */
static int
get_subioctl_no(int cmdIndex, int privCnt, int *ioctlVal, int *subIoctlVal)
{
    int j;

	if (priv_args[cmdIndex].cmd >= SIOCDEVPRIVATE) {
		*ioctlVal = priv_args[cmdIndex].cmd;
		*subIoctlVal = 0;
        return 0;
    }

    j = -1;
	while ((++j < privCnt)
           && ((priv_args[j].name[0] != '\0') ||
		   (priv_args[j].set_args != priv_args[cmdIndex].set_args) ||
		   (priv_args[j].get_args != priv_args[cmdIndex].get_args))) {
    }

    /* If not found... */
	if (j == privCnt) {
        printf("Invalid private ioctl definition for: 0x%x\n",
		       priv_args[cmdIndex].cmd);
        return -1;
    }

    /* Save ioctl numbers */
	*ioctlVal = priv_args[j].cmd;
	*subIoctlVal = priv_args[cmdIndex].cmd;

    return 0;
}

/**
 *  @brief Get ioctl number
 *
 *  @param ifName       Interface name
 *  @param privCmd     	Private command name
 *  @param ioctlVal    	A pointer to return ioctl number
 *  @param subIoctlVal 	A pointer to return sub-ioctl number
 *  @return             0 on success, otherwise -1
 */
/*
static int
get_ioctl_no(const char *ifName,
	     const char *privCmd, int *ioctlVal, int *subIoctlVal)
{
	int i, privCnt, status = -1;

  printf("DEBUG>> get_ioctl_no=%s\n",ifName);
	//privCnt = get_private_info(ifName);
  printf("DEBUG>> privCnt=%d\n",privCnt);
	// Are there any private ioctls?  
	if (privCnt <= 0 || privCnt > IW_MAX_PRIV_DEF) {
		// Could skip this message ?  
		printf("%-8.8s  no private ioctls.\n", ifName);
        ethio_flag = 1;
		status = 0;
	} else {
		for (i = 0; i < privCnt; i++) {
            if (priv_args[i].name[0]
			    && !strcmp(priv_args[i].name, privCmd)) {
				status =
				    get_subioctl_no(i, privCnt, ioctlVal,
						    subIoctlVal);
				break;
			}
		}
	}
	return status;
}
*/
static int
get_ioctl_no_multi(const char *ifName,
	     const char *privCmd, int *ioctlVal, int *subIoctlVal, int sockfd)
{
	int i, privCnt, status = -1;

  printf("DEBUG>> get_ioctl_no::ifName=%s\n",ifName);
	privCnt = get_private_info_multi(ifName, sockfd);
  printf("DEBUG>> privCnt=%d\n",privCnt);
	// Are there any private ioctls?  
	if (privCnt <= 0 || privCnt > IW_MAX_PRIV_DEF) {
		// Could skip this message ?  
		printf("%-8.8s  no private ioctls.\n", ifName);
        ethio_flag = 1;
		status = 0;
	} else {
		for (i = 0; i < privCnt; i++) {
            if (priv_args[i].name[0]
			    && !strcmp(priv_args[i].name, privCmd)) {
				status =
				    get_subioctl_no(i, privCnt, ioctlVal,
						    subIoctlVal);
				break;
			}
		}
	}
	return status;
}

#ifdef NONPLUG_SUPPORT
static int
get_hci_dev_info(int s, int dev_id, long arg)
{
#if !defined MARVELL_BT_STACK && !defined RAWUR_BT_STACK
	if (BT_IF_MODE!=MBTCHAR_Mode)
	{
  struct hci_dev_info di = { dev_id:dev_id };

	if (ioctl(s, HCIGETDEVINFO, (void *)&di))
        return 0;

    if (!strcmp(di.name, hci_intf)) {
        ba2str(&di.bdaddr, addr);
	multi_hci_addr = di.bdaddr;
        hci_addr = addr;
    }
	}
#endif //#ifndef MARVELL_BT_STACK

    return 0;
}

/**
 *  @brief Get HCI driver info
 *
 *  @return  0 --- if already loadded, otherwise -1
 */
int
drv_wrapper_get_hci_info(char *cmdname)
{
    int ret = 0;
#if !defined MARVELL_BT_STACK && !defined RAWUR_BT_STACK
	if (BT_IF_MODE!=MBTCHAR_Mode)
	{
    if (!cmdname) {
			printf("Interface name is not present:%s\n",cmdname);
        return -1;
    }
    hci_intf = cmdname;
    hci_for_each_dev(HCI_UP, get_hci_dev_info, 0);

    ret = (hci_addr == NULL) ? -1 : 0;
	}
#endif //
    return ret;
}

#ifdef RAWUR_BT_STACK
/** 
 *  @brief raw_init_uart
 *   
 *  @param intfname Interface name 
 *  @return  0 on success, otherwise -1
 */
int raw_init_uart()
{
  struct termios ti;
  //raw_uart_fd = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY);
  raw_uart_fd = open(RAW_UART_PORT, O_RDWR | O_NOCTTY);

  tcflush(raw_uart_fd, TCIOFLUSH);

  if (tcgetattr(raw_uart_fd, &ti) < 0)
  {
    perror("Can't get port settings");
    return -1;
  }

  cfmakeraw(&ti);
  ti.c_cflag |= CLOCAL;

  // Set 1 stop bit & no parity (8-bit data already handled by cfmakeraw)
  ti.c_cflag &= ~(CSTOPB | PARENB);

  ti.c_cflag |= CRTSCTS;

  //FOR READS:  set timeout time w/ no minimum characters needed (since we read only 1 at at time...)
    ti.c_cc[VMIN] = 0;
    ti.c_cc[VTIME] = 6 * 100;

    cfsetispeed(&ti,B115200);
    cfsetospeed(&ti,B115200);


  if (tcsetattr(raw_uart_fd, TCSANOW, &ti) < 0)
  {
    perror("Can't set port settings");
    return -1;
  }
  tcflush(raw_uart_fd, TCIOFLUSH);

  return raw_uart_fd;

}
#endif

/**
 *  @brief drv wrapper initialize HCI
 *
 *  @param intfname Interface name
 *  @return  0 on success, otherwise -1
 */
int
drv_wrapper_init_hci(char *cmdname)
{
    int ret = 0;
	
#ifdef RAWUR_BT_STACK

	raw_init_uart();
	BT_IF_MODE = BTRAWUR_Mode;
#elif MARVELL_BT_STACK

	if(BT_IF_MODE==MBTCHAR_Mode)
	{
		printf("Initialize  drvwrapper for BT(mbtchar) ....\n");
		hci_sock_bz = open(cmdname, O_RDWR | O_NOCTTY);
		if (hci_sock_bz >= 0) {
			printf("Use device %s\n", cmdname);    
		} else {
			printf("Cannot open device %s", cmdname);
			ret = -1;
		}
	}	
#else
		if (!cmdname) {
			printf("Interface name is not present\n");
			return -1;
		}
		hci_intf = cmdname;
		hci_addr = NULL;
		printf("Initialize  drvwrapper for BT(HCI) ....\n");
		hci_for_each_dev(HCI_UP, get_hci_dev_info, 0);
		if (hci_addr == NULL) {
			printf("BT interface is not present:%s\n",cmdname);
			ret = -1;
		} else {
			int hci_dev_id;
        hci_dev_id = hci_devid(hci_intf); 
			hci_sock_bz = hci_open_dev(hci_dev_id);
			ret = 0;
		}
#endif
    return ret;
}

/**
 *  @brief drv wrapper de-initialize HCI
 *
 *  @return  0 on success, otherwise -1
 */
int
drv_wrapper_deinit_hci()
{
    int ret = 0;

    printf("De-Initialize drvwrapper for BT....\n");

#ifdef RAWUR_BT_STACK
	close(raw_uart_fd);
#endif

    if (hci_sock_char_nfc)
        close(hci_sock_char_nfc);
    if (hci_sock_char_fm)
        close(hci_sock_char_fm);
    if (hci_sock_bz)
        close(hci_sock_bz);
    hci_addr = NULL;
    return ret;
}

/**
 *  @brief drv wrapper send HCI command
 *
 *  @return  0 on success, otherwise error code
 */
int
drv_wrapper_send_hci_command(short ogf, short ocf, unsigned char *in_buf,
                             int in_buf_len, unsigned char *out_buf,
                             int *out_buf_len, int HciDeviceID)
{
return 0;
}
#endif /* NONPLUG_SUPPORT */

/**
 *  @brief Get driver info
 *
 *  @return  0 --- if already loadded, otherwise -1
 */
int
drv_wrapper_get_info(char *cmdname)
{
    struct ifreq ifr;
    int sk, ret = 0;

    printf("DEBUG>>drv_wrapper_get_info=%s\n",cmdname);
    /* Open socket to communicate with driver */
    sk = socket(AF_INET, SOCK_STREAM, 0);
    if (sk < 0) {
        printf("Failed to create socket\n");
        return -1;
    }
    strncpy(ifr.ifr_name, cmdname, IFNAMSIZ);
    if (ioctl(sk, SIOCGIFFLAGS, &ifr) < 0)
        ret = -1;
    close(sk);
    return ret;
}

/**
 *  @brief drv wrapper initialize
 *
 *  @return  0 on success, otherwise -1
 */
int
drv_wrapper_init(char *cmdname, int devid)
{
    int ret = 0;

    printf("Initialize drvwrapper ....\n");
     printf("DEBUG>>drv_wrapper_init =%s\n", cmdname);
    /* Open socket to communicate with driver */
    sockfd= socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("Invalid sockfd, function return -1\n");
        return -1;
    }
  //;  if (get_ioctl_no(cmdname, "hostcmd", &ioctl_val, &subioctl_val)
  //;      == -1) 
  {
        printf("No private IOCTL present for command \"hostcmd\"\n");
        close(sockfd);
        sockfd = 0;
        return -1;
    }

    return ret;
}

/**
 *  @brief drv wrapper initialize for multi WiF dev
 *
 *  @return  0 on success, otherwise -1
 */
int
drv_wrapper_init_multi(new_drv_cb *drv)
{
    int ret = 0;

    printf("Initialize drv_wrapper_init_multi ....\n");
     printf("DEBUG>>drv_wrapper_init =%s\n", &drv->wlan_ifname);
    /* Open socket to communicate with driver */
    drv->sockfd= socket(AF_INET, SOCK_STREAM, 0);
    printf("DEBUG>>sockfd=%d\n", drv->sockfd);
    if (drv->sockfd < 0) {
        printf("Invalid sockfd, function return -1\n");
        return -1;
    }
    if (get_ioctl_no_multi(&drv->wlan_ifname, "hostcmd", &drv->ioctl_val, &drv->subioctl_val,drv->sockfd)
        == -1) {
        printf("No private IOCTL present for command \"hostcmd\"\n");
        close(drv->sockfd);
		
        drv->sockfd=0;
		printf("drv->sockfd=0\n");
        return -1;
    }
    
    printf("Debug>>get_ioctl_no_multi::drv->ioctl_val=0x%x, drv->subioctl_val=0x%x, drv->sockfd=0x%x\n",drv->ioctl_val, drv->subioctl_val, drv->sockfd);
    return ret;
}
/**
 *  @brief drv wrapper de-initialize
 *
 *  @return  0 on success, otherwise -1
 */
int
drv_wrapper_deinit()
{
    int ret = 0;

    printf("De-Initialize drvwrapper ....\n");
    if (sockfd)
        close(sockfd);
    if(sockfd)
    		close(sockfd);
    ioctl_val =0;
    subioctl_val =0;
    sockfd =0;		
    
    return ret;
}
drv_wrapper_deinit_multi()
{
    int ret = 0;
    int cnt=0;
    printf("De-Initialize drvwrapper multi....\n");
    MultiDevPtr = Driver1; 
      for (cnt=0; cnt<WiFidevicecnt; cnt++)
      {     
    		printf("DEBUG>>drv_wrapper_deinit_multi:: wlan_ifname=%s\n", &MultiDevPtr->wlan_ifname);
    		if (MultiDevPtr->sockfd)
        	close(MultiDevPtr->sockfd);
        MultiDevPtr->ioctl_val =0;
         MultiDevPtr->subioctl_val =0;
         MultiDevPtr->sockfd =0;
    		MultiDevPtr++;
    	}
    return ret;
}

#ifdef NONPLUG_SUPPORT

/*
 * int drv_wrapper_send_hci_command(short ogf, short ocf,  unsigned char *in_buf, int in_buf_len,
 * unsigned char *out_buf, int *out_buf_len)
 */
int
drv_proc_hci_command(drv_cb * drv, unsigned char *buf, int *msglen, int buflen,int HciDeviceID)
{
#define EVENT_BUF_SIZE 400
    int status = 0;
    int hci_sock = 0;
    int len;
#if !defined MARVELL_BT_STACK && !defined RAWUR_BT_STACK
    struct hci_filter flt;
#endif //#ifndef MARVELL_BT_STACK
    int dummy_buf[512];
    int avail = 0;

    unsigned short ogf;
    unsigned short ocf;
    unsigned char *evntbuf = drv->cb_buf;

 if(BT_IF_MODE == MBTCHAR_Mode || BT_IF_MODE == BTRAWUR_Mode)
 {
#ifdef RAWUR_BT_STACK
	hci_sock = raw_uart_fd;
#else
   	hci_sock = multi_hci_sock_bz[0];
#endif
    len = write(hci_sock, buf, *msglen);
    if (len != *msglen) {
        printf("Failed to write %d bytes (written %d)\n", *msglen, len);
        status = -1;
    }
   }
#if !defined MARVELL_BT_STACK && !defined RAWUR_BT_STACK
  else
	{
	    ogf = buf[2] >> 2;
		ocf = ((buf[2] & 0x03) << 8) + buf[1];
	
		hci_sock = multi_hci_sock_bz[HciDeviceID];
		printf("CRH_drv_proc_hci_command: hci_sock =%d\n",hci_sock);
		/* Setup filter */
		hci_filter_clear(&flt);
		hci_filter_set_ptype(HCI_EVENT_PKT, &flt);
		hci_filter_all_events(&flt);
		if (setsockopt(hci_sock, SOL_HCI, HCI_FILTER, &flt, sizeof(flt)) < 0) {
			mfg_dprint(DBG_ERROR, "HCI filter setup failed\n");
			return -1;
		}

		status = hci_send_cmd(hci_sock, ogf, ocf, *msglen - 4, buf + 4);
    
		if (status < 0) {
			mfg_dprint(DBG_ERROR,
					"Failed to send command (OGF = %04x OCF = %04x)\n",
					ogf, ocf);
			return status;
		}
	}
#endif //#ifndef MARVELL_BT_STACK

    do {

#ifdef RAWUR_BT_STACK
	tcflush(hci_sock, TCIFLUSH);
	usleep(50000);
	FD_ZERO(&fds);
	FD_SET(hci_sock, &fds);
	do
	{ jrtrn = select(hci_sock+1, &fds, NULL, NULL, &tvout);}
	while(jrtrn == -1);
//	if(ioctl(hci_sock, FIONREAD, &number_bytes_availt) < 0) { //file descriptor, call, unsigned int
//	          perror("ioctl");
//	            return 1;
//	}
//	printf("%d bytes available for reading.\n", number_bytes_availt);
#endif

        *msglen = read(hci_sock, evntbuf, EVENT_BUF_SIZE);
        printf("Rx Event %X for %2.2X %2.2X \n", evntbuf[1], evntbuf[4],
               evntbuf[5]);
        printf("In Cmd  %2.2X %2.2X \n", buf[1], buf[2]);

    } while ((evntbuf[1] != 0xFF)
             && !(evntbuf[1] != 0xF && evntbuf[4] == buf[1]
                  && evntbuf[5] == buf[2]));

    if (evntbuf[1] == 0xFF && evntbuf[3] != 0xc0)
        avail = read(hci_sock, dummy_buf, EVENT_BUF_SIZE);

    memcpy(buf, evntbuf, *msglen);
    return status;
}

#endif /* NONPLUG_SUPPORT */

int
drv_config_default(drv_config * drv_conf)
{
    memset(drv_conf, 0, sizeof(drv_config));

#ifdef NONPLUG_SUPPORT
    sprintf(drv_conf->hci_port[0], "hci0");
	sprintf(drv_conf->hci_port[1], "hci1");
#endif
    sprintf(drv_conf->wlan_port, "mlan0");

    return 0;
}

int
drv_init(struct _bridge_cb *bridge, drv_config * drv_conf)
{
    drv_cb *drv;

    int cnt=0;
    
    bridge->drv = &Driver;
    drv = bridge->drv;

#ifdef NONPLUG_SUPPORT
    drv->hci_ifname_0 = drv_conf->hci_port[0];
	drv->hci_ifname_1 = drv_conf->hci_port[1];

    //
   printf("DEBUG>>drv_init>>HCI0=%s\n",drv->hci_ifname_0);
   printf("DEBUG>>drv_init>>HCI1=%s\n",drv->hci_ifname_1);
#endif

    drv->wlan_ifname = drv_conf->wlan_port;
    drv->load_script = drv_conf->load_script;
    drv->unload_script = drv_conf->unload_script;

    /** Initialize drvwrapper, if driver already loadded */
    /** Init WiFi */
/*
    if (!drv_wrapper_get_info(drv->wlan_ifname))
        drv_wrapper_init(drv->wlan_ifname, 0);
  */
    printf("DEBUG>>drv_init\n");
    MultiDevPtr = Driver1;
    for (cnt =0 ;cnt <WiFidevicecnt; cnt++)
    {
    	 
    	 if (!drv_wrapper_get_info(&MultiDevPtr->wlan_ifname))
        drv_wrapper_init_multi(MultiDevPtr);
        MultiDevPtr++;
    	
    }       
#ifdef NONPLUG_SUPPORT
    /** Init BT */
	if (strstr(drv->hci_ifname_0,"/dev/mbtchar"))
	{
		BT_IF_MODE=MBTCHAR_Mode;
	}

#ifdef MARVELL_BT_STACK	
	else
         {
	  	printf("BT Interface name is not present(must be /dev/mbtcharX):%s\n",drv->hci_ifname_0);	
	 }
#endif //#ifdef MARVELL_BT_STACK	
    if (!drv_wrapper_get_hci_info(drv->hci_ifname_0))
	{
        if (!drv_wrapper_init_hci(drv->hci_ifname_0))
		  multi_hci_sock_bz[0] =hci_sock_bz;
	   printf("DEBUG>>multi_hci_sock_bz[0]=0x%x\n",multi_hci_sock_bz[0]);
	}
   if(BT_IF_MODE !=MBTCHAR_Mode) //Current bridge support one btchar only.
   {
	if (!drv_wrapper_get_hci_info(drv->hci_ifname_1))
	{
			if (!drv_wrapper_init_hci(drv->hci_ifname_1))
			multi_hci_sock_bz[1] =hci_sock_bz;
		printf("DEBUG>>multi_hci_sock_bz[1]=0x%x\n",multi_hci_sock_bz[1]);
	}
   }
 
#endif
    mfg_dprint(DBG_MINFO, "DRV:  driver is initialized.\n");
    return 0;
}

int
drv_proc_wlan_command(drv_cb * drv, unsigned char *buf, int *rsplen, int buflen)
{
    int status = -1;
    HostCmd_DS_Gen *hostcmd_hdr = (HostCmd_DS_Gen *) buf;
    char *ifname = drv->wlan_ifname;
    eth_cmd ether_cmd;
  
    int eth_header_len;
    char *buffer;
    mfg_Cmd_t *CMDPtr= (mfg_Cmd_t*) buf;
     
    {
     	printf("DEBUG>>drv_proc_wlan_command::DeviceId=0x%x\n",CMDPtr->deviceId);
		//Device ID check
		MultiDevPtr = Driver1;
		  if (abs(CMDPtr->deviceId) > WiFidevicecnt)
		{
			printf("DEBUG>>drv_proc_wlan_command::Fail!!Invalid DeviceId=%d(%d), force to Device 0\n", CMDPtr->deviceId, WiFidevicecnt);
			MultiDevPtr = Driver1;
		}			
		else
			MultiDevPtr +=(CMDPtr->deviceId);
		if(MultiDevPtr->sockfd ==0)
		{
			printf("DEBUG>>MultiDevPtr->wlan_ifname =%s not exist, force command to ", MultiDevPtr->wlan_ifname);
			MultiDevPtr = Driver1;
			printf("%s\n", MultiDevPtr->wlan_ifname);
		}
     	printf("DEBUG>>drv_proc_wlan_command::wlan_ifname =%s\n", MultiDevPtr->wlan_ifname);
     	ifname = MultiDevPtr->wlan_ifname;
    }
    if (ethio_flag == 1) {
        struct ifreq ifr;
        char *p_cmd = "MRVL_CMDhostcmd";
        eth_header_len = strlen(p_cmd) + BRDG_OFFSET;
        eth_cmd *cmd = &ether_cmd;

        buffer = (char *) malloc(buflen + eth_header_len);
        if (buffer == NULL) {
            printf("can't allocate buffer \n");
            exit(0);
        }

        strncpy((char *) buffer, p_cmd, strlen(p_cmd));
        memcpy(buffer + eth_header_len, buf, buflen);

        /* Fill up buffer */
        cmd->buf = buffer;
        cmd->used_len = 0;
        cmd->total_len = buflen + eth_header_len;

        /* Perform IOCTL */
        memset(&ifr, 0, sizeof(struct ifreq));
        strncpy(ifr.ifr_ifrn.ifrn_name, ifname, strlen(ifname));
        ifr.ifr_ifru.ifru_data = (void *) cmd;
        //Check for device number 
        //Needs to open
         printf("DEBUG>>drv_proc_wlan_command::MultiDevPtr->sockfd=%d\n", MultiDevPtr->sockfd);
         printf("DEBUG>>drv_proc_wlan_command::MultiDevPtr->wlan_ifname =%s\n", MultiDevPtr->wlan_ifname);
         printf("DEBUG>>drv_proc_wlan_command::MultiDevPtr->ioctl_val =%d\n", MultiDevPtr->ioctl_val);
        if (ioctl(MultiDevPtr->sockfd, MLAN_ETH_PRIV, &ifr)) {
			   printf("ether bridge: hostcmd fail\n");
		    }
		{
			//RSP
			mfg_Cmd_t *RspCMDPtr= (mfg_Cmd_t*) (cmd->buf+eth_header_len);
			printf("ether bridge MLAN_ETH_PRIV: RSP=0x%x \n", RspCMDPtr->header.len);
			*rsplen = RspCMDPtr->header.len;
		}
        memcpy(buf, cmd->buf + eth_header_len, *rsplen);
        free(buffer);
    } else {
        //Check for device number 
        *rsplen = 0;
        printf("DEBUG>>drv_proc_wlan_command:ifname=%s\n", ifname);

        memset(&userdata, 0,  sizeof(struct ifreq));
        strncpy(userdata.ifr_name, ifname, strlen(ifname));
        userdata.ifr_data = (char *) buf;
        mfg_dprint(DBG_GINFO, "DRV:  send host cmd to ioctl\n");
         printf("DEBUG>>drv_proc_wlan_command::MultiDevPtr->sockfd=%d\n", MultiDevPtr->sockfd);
         printf("DEBUG>>drv_proc_wlan_command::MultiDevPtr->wlan_ifname =%s\n", MultiDevPtr->wlan_ifname);
         printf("DEBUG>>drv_proc_wlan_command::MultiDevPtr->ioctl_val =%d\n", MultiDevPtr->ioctl_val);
	printf("\n");
#if 0
        {
           int *data = (int *)&userdata;
           printf("DEBUG>>%d\n", sizeof(struct ifreq));
           
	   for(int i=0; i<sizeof(struct ifreq); i++)
             {
		printf("%x ", *data);
			data++;

                }

		printf("\n");
	}
#endif //#if 0
        status = ioctl(MultiDevPtr->sockfd, MultiDevPtr->ioctl_val, &userdata);
       printf("DEBUG>>drv_proc_wlan_command::Response Status =%d with MultiDevPtr->ioctl_val =%d\n", status, MultiDevPtr->ioctl_val);
       printf("DEBUG>>drv_proc_wlan_command::MultiDevPtr->wlan_ifname =%s\n", MultiDevPtr->wlan_ifname);
        *rsplen = hostcmd_hdr->size;

    }
    mfg_dprint(DBG_GINFO, "DRV:  host cmd is completed\n");
    return status;
}

int
drv_load_driver(struct _drv_cb *drv, int drv_if)
{
    char *command = drv->command;
    char *script = drv->load_script;

    if (!script) {
        printf("Load script is not provided\n");
        return -1;
    }

    if (!drv_wrapper_get_info(drv->wlan_ifname)) {
        printf("Driver already loaded\n");
    } else {
        sprintf(command, "sh %s", script);
        printf("Load driver ......\n");
        if (system(command) != 0) {
            printf("Failed to run the script\n");
            return -1;
        } else {
            drv_wrapper_init(drv->wlan_ifname, 1);
        }
    }
    return 0;
}

int
drv_unload_driver(struct _drv_cb *drv, int drv_if)
{

    int ret = -1;
    char *command = drv->command;
    char *script = drv->unload_script;

    if (!script) {
        printf("Unload script is not provided\n");
        return -1;
    }
    if (!drv_wrapper_get_info(drv->wlan_ifname)) {
        sprintf(command, "sh %s", script);
        drv_wrapper_deinit();
        printf("Unload driver ......\n");
        ret = system(command);
        if (ret) {
            printf("Failed to run the script\n");
            return -1;
        }
    } else {
        printf("No such device\n");
    }
    return 0;
}
