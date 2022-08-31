#include <utils/Log.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include "bt_hci_bdroid.h"
#include "bt_vendor_brcm.h"
#include "wole/utility.h"

//
//User should define the manufacture specific data that can support wake up feature below.
//=========================================================================
const uint8_t WOLE_MANUFACTURE_PATTERN[] = {0xff,0xff,0x41,0x6d,0x6c,0x6f,0x67,0x69,0x63};  //amlogic rc(Amlogic_RC20,B12)
//=========================================================================
const uint8_t WOLE_PULSE_TIME_POWER[] = {0x00,0x01,0x10};   //idx is 0, key is 0x01, pulse time is 0x10*12.5ms
const uint8_t WOLE_PULSE_TIME_NETFLIX[] = {0x01,0x02,0x20}; //idx is 0, key is 0x02, pulse time is 0x20*12.5ms

#define BDADDR_TO_STREAM(p, a)   {register int ijk; for (ijk = 0; ijk < 6;  ijk++) *(p)++ = (uint8_t) a[6 - 1 - ijk];}
#define STREAM_TO_UINT16(u16, p) {u16 = ((uint16_t)(*(p)) + (((uint16_t)(*((p) + 1))) << 8)); (p) += 2;}
#define UINT8_TO_STREAM(p, u8)   {*(p)++ = (uint8_t)(u8);}
#define UINT16_TO_STREAM(p, u16) {*(p)++ = (uint8_t)(u16); *(p)++ = (uint8_t)((u16) >> 8);}
#define UINT32_TO_STREAM(p, u32) {*(p)++ = (uint8_t)(u32); *(p)++ = (uint8_t)((u32) >> 8); *(p)++ = (uint8_t)((u32) >> 16); *(p)++ = (uint8_t)((u32) >> 24);}
#define HCI_CMD_MAX_LEN                         258
#define HCI_CMD_PREAMBLE_SIZE                   3
#define HCI_EVT_CMD_CMPL_STATUS_RET_BYTE        5
#define HCI_EVT_CMD_CMPL_OPCODE                 3
#define BT_WAKE_EVT       "/sys/module/bt_device/parameters/btwake_evt"


pthread_mutex_t s_vsclock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t s_vsccond = PTHREAD_COND_INITIALIZER;
int wake_signal_sent=0;


void thread_exit_handler(int sig)
{
	ALOGE("signal %d caught", sig);
	pthread_exit(0);
}

void* wole_vsc_write_thread( void *ptr)
{
	(void) ptr;
	struct sigaction actions;
	int fd,sz;
	char buf[2];
	memset(&actions, 0, sizeof(actions));
	sigemptyset(&actions.sa_mask);
	actions.sa_flags = 0;
	actions.sa_handler = thread_exit_handler;
	sigaction(SIGUSR1,&actions,NULL);

	pthread_detach(pthread_self());
	pthread_mutex_lock(&s_vsclock);
	pthread_cond_wait(&s_vsccond, &s_vsclock);
	pthread_mutex_unlock(&s_vsclock);
	//to prevent the the command is sent before HCI_RESET which sent from stack
	usleep(200000);
	wole_config_write_pulse_time(WOLE_PULSE_TIME_POWER,sizeof(WOLE_PULSE_TIME_POWER));
	usleep(10000);
	wole_config_write_pulse_time(WOLE_PULSE_TIME_NETFLIX,sizeof(WOLE_PULSE_TIME_NETFLIX));
	usleep(10000);
	wole_config_write_default_hostwake_state(1);
	usleep(10000);
	wole_config_write_manufacture_pattern();

	while (1)
	{
		//poll every seconds
		usleep(800000);
		fd = open(BT_WAKE_EVT,O_RDONLY);
		if (fd < 0)
		{
			ALOGE("open(%s) failed: %s (%d)\n", \
				BT_WAKE_EVT, strerror(errno), errno);
		}
		sz = read (fd, &buf,sizeof(buf));

		close(fd);

		if (sz >= 1 && memcmp(buf, "1", 1) == 0)
		{
			ALOGE("%s,rtc wakeup",__func__);
			continue;
		}
		pthread_mutex_lock(&s_vsclock);
		wole_config_start();
		while (wake_signal_sent == 0)
			pthread_cond_wait(&s_vsccond, &s_vsclock);
		wake_signal_sent = 0;
		pthread_mutex_unlock(&s_vsclock);
	}
}

/*******************************************************************************
**
** Function         wole_config_cback
**
** Description      WoLE Callback function for controller configuration
**
** Returns          None
**
*******************************************************************************/
void wole_config_cback(void *p_mem)
{
	HC_BT_HDR *p_evt_buf = (HC_BT_HDR *) p_mem;
	uint8_t     *p, status,wole_status;
	uint16_t    opcode;

	status = *((uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_STATUS_RET_BYTE);
	p = (uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_OPCODE;
	STREAM_TO_UINT16(opcode,p);
	ALOGI("%s, status = %d, opcode=0x%x",__FUNCTION__,status,opcode);

	if (status == 0) //command is supported and everything is fine.
	{
		pthread_mutex_lock(&s_vsclock);
		wake_signal_sent=1;
		pthread_cond_signal(&s_vsccond);
		pthread_mutex_unlock(&s_vsclock);

	}
	else if (status == 3)
		kill(getpid(),SIGKILL);


	/* Free the RX event buffer */
	if (bt_vendor_cbacks)
		bt_vendor_cbacks->dealloc(p_evt_buf);

}

void wole_config_write_default_hostwake_state(int default_state)
{
    HC_BT_HDR  *p_buf = NULL;
    uint8_t     *p;

    ALOGI("%s",__FUNCTION__);

    if (bt_vendor_cbacks)
    {
        p_buf = (HC_BT_HDR *) bt_vendor_cbacks->alloc(BT_HC_HDR_SIZE + \
                                                       HCI_CMD_MAX_LEN);
    }

    if (p_buf)
    {
        p_buf->event = MSG_STACK_TO_HC_HCI_CMD;
        p_buf->offset = 0;
        p_buf->layer_specific = 0;
        p_buf->len = HCI_CMD_PREAMBLE_SIZE + 1 + 1 + 1;

        p = (uint8_t *) (p_buf + 1);
        UINT16_TO_STREAM(p,HCI_VSC_WAKE_ON_BLE);
        *p++ = 1 + 1 + 1;
        *p++ = CUSTOMER_AMLOGIC;
        *p++ = DEFAULT_HOSTWAKE_PIN_STATE;
        *p = default_state;

        bt_vendor_cbacks->xmit_cb(HCI_VSC_WAKE_ON_BLE, p_buf, wole_config_cback);
    }
    else
    {
        if (bt_vendor_cbacks)
        {
            ALOGE("vendor lib wole conf aborted [no buffer]");
        }
    }
}


void wole_config_write_pulse_time(const uint8_t *pattern,int len)
{
    HC_BT_HDR  *p_buf = NULL;
    uint8_t     *p;

    ALOGI("%s,pattern len = %d",__FUNCTION__,len);

    if (bt_vendor_cbacks)
    {
        p_buf = (HC_BT_HDR *) bt_vendor_cbacks->alloc(BT_HC_HDR_SIZE + \
                                                       HCI_CMD_MAX_LEN);
    }

    if (p_buf)
    {
        p_buf->event = MSG_STACK_TO_HC_HCI_CMD;
        p_buf->offset = 0;
        p_buf->layer_specific = 0;
        p_buf->len = HCI_CMD_PREAMBLE_SIZE + 1 + 1 + len; //Customer (05) 1 bytes/ wake up pattern 03 1 bytes

        p = (uint8_t *) (p_buf + 1);
        UINT16_TO_STREAM(p,HCI_VSC_WAKE_ON_BLE);
        *p++ = 1 + 1  + len;
        *p++ = CUSTOMER_AMLOGIC;
        *p++ = HOSTWAKE_PIN_PATTERN;

        for (int i=0;i<len;i++)
        {
            if (i == len-1)
                *p = pattern[i];
            else
                *p++ = pattern[i];
        }

        bt_vendor_cbacks->xmit_cb(HCI_VSC_WAKE_ON_BLE, p_buf, wole_config_cback);
    }
    else
    {
        if (bt_vendor_cbacks)
        {
            ALOGE("vendor lib wole conf aborted [no buffer]");
        }
    }
}


void wole_config_write_manufacture_pattern(void)
{
	HC_BT_HDR  *p_buf = NULL;
	uint8_t     *p;
	int i;

	ALOGI("%s",__FUNCTION__);

	if (bt_vendor_cbacks)
	{
		p_buf = (HC_BT_HDR *) bt_vendor_cbacks->alloc(BT_HC_HDR_SIZE + \
				HCI_CMD_MAX_LEN);
	}

	if (p_buf)
	{
		p_buf->event = MSG_STACK_TO_HC_HCI_CMD;
		p_buf->offset = 0;
		p_buf->layer_specific = 0;
		p_buf->len = HCI_CMD_PREAMBLE_SIZE + 1 + 1 + sizeof(WOLE_MANUFACTURE_PATTERN) + sizeof(WOLE_MANUFACTURE_PATTERN);  //Customer (05) 1 bytes/ common pattern 01 1 bytes

		p = (uint8_t *) (p_buf + 1);
		UINT16_TO_STREAM(p,HCI_VSC_WAKE_ON_BLE);
        *p++ = 1 + 1 + sizeof(WOLE_MANUFACTURE_PATTERN)+ sizeof(WOLE_MANUFACTURE_PATTERN);
        *p++ = CUSTOMER_AMLOGIC;
        *p++ = MANUFACTURE_PATTERN;
		for (i=0;i< (int)sizeof(WOLE_MANUFACTURE_PATTERN);i++)
			*p++ = WOLE_MANUFACTURE_PATTERN[i];
		for (i=0;i< (int)sizeof(WOLE_MANUFACTURE_PATTERN);i++)
		{
			if (i == sizeof(WOLE_MANUFACTURE_PATTERN)-1)
				*p = 0xFF;
			else
				*p++ = 0xFF;
		}

		bt_vendor_cbacks->xmit_cb(HCI_VSC_WAKE_ON_BLE, p_buf, wole_config_cback);
	}
	else
	{
		if (bt_vendor_cbacks)
		{
			ALOGE("vendor lib wole conf aborted [no buffer]");
		}
	}
}

/*******************************************************************************
**
** Function        wole_config_start
**
** Description     Kick off controller initialization process
**
** Returns         None
**
*******************************************************************************/
void wole_config_start()
{
	HC_BT_HDR  *p_buf = NULL;
	uint8_t     *p;

	/* Start from sending HCI_RESET */
	if (bt_vendor_cbacks)
	{
		p_buf = (HC_BT_HDR *) bt_vendor_cbacks->alloc(BT_HC_HDR_SIZE + \
				HCI_CMD_MAX_LEN);
	}

	if (p_buf)
	{
		p_buf->event = MSG_STACK_TO_HC_HCI_CMD;
		p_buf->offset = 0;
		p_buf->layer_specific = 0;
		p_buf->len = HCI_CMD_PREAMBLE_SIZE+1 +1;

		p = (uint8_t *) (p_buf + 1);
		UINT16_TO_STREAM(p,HCI_VSC_WAKE_ON_BLE);
        *p++ =   1 + 1; /* parameter length */
        *p++ = CUSTOMER_AMLOGIC;
        *p = POLLING_WAKE_ON_BLE;

		bt_vendor_cbacks->xmit_cb(HCI_VSC_WAKE_ON_BLE, p_buf, wole_config_cback);
	}
	else
	{
		if (bt_vendor_cbacks)
		{
			ALOGE("vendor lib wole conf aborted [no buffer]");
		}
	}
}

