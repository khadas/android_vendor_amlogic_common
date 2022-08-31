/******************************************************************************
*
*  Copyright (C) 2029-2021 Amlogic Corporation
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

/******************************************************************************
*
*  Filename:      bt_vendor_aml.c
*
*  Description:   Amlogic vendor specific library implementation
*
******************************************************************************/

#define LOG_TAG "bt_vendor"

#include <unistd.h>
#include <utils/Log.h>
#include <cutils/properties.h>
#include <string.h>
#include "bt_vendor_aml.h"
#include "upio.h"
#include "userial_vendor.h"
#include <errno.h>

#ifndef BTVND_DBG
#define BTVND_DBG TRUE
#endif

#if (BTVND_DBG == TRUE)
#define BTVNDDBG(param, ...) { ALOGD(param, ## __VA_ARGS__); }
#else
#define BTVNDDBG(param, ...) {}
#endif

#ifndef PROPERTY_VALUE_MAX
#define PROPERTY_VALUE_MAX 92
#endif

/******************************************************************************
**  Externs
******************************************************************************/

void hw_config_start(void);
uint8_t hw_lpm_enable(uint8_t turn_on);
uint32_t hw_lpm_get_idle_timeout(void);
void hw_lpm_set_wake_state(uint8_t wake_assert);
#if (SCO_CFG_INCLUDED == TRUE)
void hw_sco_config(void);
#endif
void vnd_load_conf(const char *p_path);
#if (HW_END_WITH_HCI_RESET == TRUE)
void hw_epilog_process(void);
#endif
extern void ms_delay(uint32_t timeout);

/******************************************************************************
**  Variables
******************************************************************************/

bt_vendor_callbacks_t *bt_vendor_cbacks = NULL;
uint8_t vnd_local_bd_addr[6] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static int g_userial_fd = -1;

/******************************************************************************
**  Local type definitions
******************************************************************************/
#define HCI_MAX_EVENT_SIZE     260

/******************************************************************************
**  Static Variables
******************************************************************************/
static const char DRIVER_PROP_NAME[] = "vendor.sys.amlbtsdiodriver";
static const char PWR_PROP_NAME[] = "sys.shutdown.requested";

static const tUSERIAL_CFG userial_init_cfg =
{
	(USERIAL_DATABITS_8 | USERIAL_PARITY_NONE | USERIAL_STOPBITS_1),
	USERIAL_BAUD_115200
};



/******************************************************************************
**  Functions
******************************************************************************/

/*****************************************************************************
**
**   BLUETOOTH VENDOR INTERFACE LIBRARY FUNCTIONS
**
*****************************************************************************/
static int init(const bt_vendor_callbacks_t *p_cb, unsigned char *local_bdaddr)
{
	ALOGI("init");

	if (p_cb == NULL)
	{
		ALOGE("init failed with no user callbacks!");
		return -1;
	}

#if (VENDOR_LIB_RUNTIME_TUNING_ENABLED == TRUE)
	ALOGW("*****************************************************************");
	ALOGW("*****************************************************************");
	ALOGW("** Warning - BT Vendor Lib is loaded in debug tuning mode!");
	ALOGW("**");
	ALOGW("** If this is not intentional, rebuild libbt-vendor.so ");
	ALOGW("** with VENDOR_LIB_RUNTIME_TUNING_ENABLED=FALSE and ");
	ALOGW("** check if any run-time tuning parameters needed to be");
	ALOGW("** carried to the build-time configuration accordingly.");
	ALOGW("*****************************************************************");
	ALOGW("*****************************************************************");
#endif

	userial_vendor_init();
	upio_init();

	vnd_load_conf(VENDOR_LIB_CONF_FILE);

	/* store reference to user callbacks */
	bt_vendor_cbacks = (bt_vendor_callbacks_t *)p_cb;

	/* This is handed over from the stack */
	memcpy(vnd_local_bd_addr, local_bdaddr, 6);

	return 0;
}

int sdio_bt_completed()
{
	int retry_cnt = 1;
	int bt_fd = -1;
	int ret = -1;

	while (retry_cnt < 200)
	{
		usleep(20000);
		bt_fd = open("/dev/stpbt", O_RDWR | O_NOCTTY | O_NONBLOCK);
		if (bt_fd >= 0)
			break;
		else
			ALOGE("%s: Can't open stpbt: %s. retry_cnt=%d\n", __FUNCTION__, strerror(errno), retry_cnt);

		retry_cnt++;
	}

	if (bt_fd >= 0)
	{
		ALOGD("%s: open stpbt successfully.[%d]...\n", __FUNCTION__, bt_fd);
		close(bt_fd);
		usleep(10000);
		return bt_fd;
	}
	else
	{
		ALOGE("%s: Can't open stpbt: %s.\n", __FUNCTION__, strerror(errno));
		return -1;
	}

	return ret;
}

int insmod_bt_sdio_driver()
{
	char driver_status[PROPERTY_VALUE_MAX];

	property_get(DRIVER_PROP_NAME, driver_status, "amldriverunkown");
	ALOGD("%s: driver_status = %s ", __FUNCTION__, driver_status);
	if (strcmp("true", driver_status) == 0)
	{
		ALOGW("%s: sdio_bt.ko is already insmod!", __FUNCTION__);
		return 0;
	}
	ALOGD("%s: set vendor.sys.amlbtsdiodriver true\n", __FUNCTION__);
	property_set(DRIVER_PROP_NAME, "true");
	//ms_delay(2000);
	if (sdio_bt_completed() >= 0)
		ALOGD("%s: insmod sdio_bt.ko successfully!", __FUNCTION__);
	else
		ALOGE("%s: insmod sdio_bt.ko failed!!!!!!!!!!!!!", __FUNCTION__);
	return 0;
}

int rmmod_bt_sdio_driver()
{
	ALOGD("%s: set vendor.sys.amlbtsdiodriver false\n", __FUNCTION__);
	property_set(DRIVER_PROP_NAME, "false");
	ms_delay(500);
	/*when disable bt, can't use open btsdio node to communicate with sdio_bt driver,
	 *  because the rmmod sdio_bt driver procedure will prevent libbt to open btsdio node.
	 */
	return 0;
}

int do_write(int fd, unsigned char *buf, int len)
{
	int ret = 0;
	int write_offset = 0;
	int write_len = len;
	do {
		ret = write(fd, buf + write_offset, write_len);
		if (ret < 0)
		{
			ALOGE("%s, write failed ret = %d err = %s", __func__, ret, strerror(errno));
			return -1;
		} else if (ret == 0) {
			ALOGE("%s, write failed with ret 0 err = %s", __func__, strerror(errno));
			return 0;
		} else {
			if (ret < write_len) {
				ALOGD("%s, Write pending,do write ret = %d err = %s", __func__, ret,
					  strerror(errno));
				write_len = write_len - ret;
				write_offset = ret;
			} else {
				ALOGV("Write successful");
				break;
			}
		}
	} while (1);

	return len;
}

/*******************************************************************************
**
** Function        read_hci_event
**
** Description     Read HCI event during vendor initialization
**
** Returns         int: size to read
**
*******************************************************************************/
int read_hci_event(int fd, unsigned char* buf, int size)
{
	int remain, r;
	int count = 0;

	if (size <= 0) {
		ALOGE("Invalid size arguement!");
		return -1;
	}

	ALOGI("%s: Wait for Command Compete Event from SOC", __FUNCTION__);

	/* The first byte identifies the packet type. For HCI event packets, it
	 * should be 0x04, so we read until we get to the 0x04. */
	while (1) {
		r = read(fd, buf, 1);
		if (r <= 0)
			return -1;
		if (buf[0] == 0x04)
			break;
	}
	count++;

	/* The next two bytes are the event code and parameter total length. */
	while (count < 3) {
		r = read(fd, buf + count, 3 - count);
		if (r <= 0)
			return -1;
		count += r;
	}

	/* Now we read the parameters. */
	if (buf[2] < (size - 3))
		remain = buf[2];
	else
		remain = size - 3;

	while ((count - 3) < remain) {
		r = read(fd, buf + count, remain - (count - 3));
		if (r <= 0)
			return -1;
		count += r;
	}
	return count;
}

int aml_hci_send_cmd(int fd, unsigned char *cmd, int cmdsize, unsigned char *rsp)
{
	int err = 0;

	ALOGD("%s [abner test]: ", __FUNCTION__);
        err = do_write(fd, cmd, cmdsize);
	if (err != cmdsize)
	{
	        ALOGE("%s: Send failed with ret value: %d", __FUNCTION__, err);
	        err = -1;
	        goto error;
	}

	memset(rsp, 0, HCI_MAX_EVENT_SIZE);

	/* Wait for command complete event */
	err = read_hci_event(fd, rsp, HCI_MAX_EVENT_SIZE);
	if ( err < 0) {
	        ALOGE("%s: Failed to set patch info on Controller", __FUNCTION__);
	        goto error;
	}

	error:
	        return err;
}

int aml_woble_configure(int fd)
{
	unsigned char rsp[HCI_MAX_EVENT_SIZE];
        unsigned char reset_cmd[] = {0x01, 0x03, 0x0C, 0x00};
	unsigned char read_BD_ADDR[] = {0x01, 0x09, 0x10, 0x00};
	unsigned char APCF_set_filtering_param[] = {0x01, 0x57, 0xFD, 0x12, 0x01, 0x00, 0x00, 0x20, 0x00, 0x00,
		0x00, 0x00, 0xA6, 0x00, 0x00, 0x00, 0x00, 0xA6, 0x00, 0x00, 0x02, 0x00};
	/**
	 * bit0 : cmd type
	 * bit(1-2) : cmd
	 * bit3: adv pd length
	 * bit4:0:trunk;1:index
	 * bit(5-9):default 0
	 * bit10: wakeup pd data length
	 * bit(11-(10+bit10)) : wakeup pd data
	 * bit(bit3--) : default 0
	 ** /



	/*unsigned char APCF_config_manf_data[] = {0x01, 0x22, 0xFC, 0x1f,0x01,0x00,0x00, 0x00,0x00,0x00, 0x05, 0xff, 0x5d, 0x00, 0x03, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };*/
	unsigned char APCF_config_manf_data[] = {0x01, 0x22, 0xFC, 0x19,0x00,0x00,0x00,0x00,0x00,0x00, 0x0A, 0xff, 0xff, 0xff, 0x41, 0x6d, 0x6c, 0x6f,0x67, 0x69, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00};
	unsigned char APCF_enable[] = {0x01, 0x57, 0xFD, 0x02, 0x00, 0x01};
	unsigned char le_set_evt_mask[] = {0x01, 0x01, 0x20, 0x08, 0x7F, 0x1A, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00};
	unsigned char le_scan_param_setting[] = {0x01, 0x0b, 0x20, 0x07, 0x00, 0x10, 0x00, 0x10, 0x00, 0x00, 0x00};
	unsigned char le_scan_enable[] = {0x01, 0x0c, 0x20, 0x02, 0x01, 0x00};
	unsigned char host_sleep_VSC[] = {0x01, 0x21, 0xfc, 0x01,0x01};

	//aml_hci_send_cmd(fd, (unsigned char*)read_BD_ADDR, sizeof(read_BD_ADDR), (unsigned char*)rsp);
	//ALOGD("%s, BT_MAC: 0x%x:%x:%x:%x:%x:%x", __FUNCTION__, rsp[12], rsp[11], rsp[10], rsp[9], rsp[8], rsp[7]);
	//memcpy((unsigned char*)APCF_config_manf_data+15, rsp+7, 6);
        aml_hci_send_cmd(fd, (unsigned char*)reset_cmd, sizeof(reset_cmd), (unsigned char*)rsp);
	aml_hci_send_cmd(fd, (unsigned char*)host_sleep_VSC, sizeof(host_sleep_VSC), (unsigned char*)rsp);
	aml_hci_send_cmd(fd, (unsigned char*)APCF_config_manf_data, sizeof(APCF_config_manf_data), (unsigned char*)rsp);
	aml_hci_send_cmd(fd, (unsigned char*)le_set_evt_mask, sizeof(le_set_evt_mask), (unsigned char*)rsp);
	aml_hci_send_cmd(fd, (unsigned char*)le_scan_param_setting, sizeof(le_scan_param_setting), (unsigned char*)rsp);
	aml_hci_send_cmd(fd, (unsigned char*)le_scan_enable, sizeof(le_scan_enable), (unsigned char*)rsp);

	return 0;
}

/** Requested operations */
static int op(bt_vendor_opcode_t opcode, void *param)
{
	int retval = 0;
	char shutdwon_status[PROPERTY_VALUE_MAX];

	BTVNDDBG("op for %d", opcode);

	switch (opcode)
	{
	case BT_VND_OP_POWER_CTRL:
	{
		int *state = (int *)param;
		if (*state == BT_VND_PWR_OFF)
		{
			property_get(PWR_PROP_NAME, shutdwon_status, "unkown");
			if (strstr(shutdwon_status, "0userrequested") != NULL)
			{
				return 0;
			}
			ALOGD("=== power off BT ===");
			rmmod_bt_sdio_driver();
			upio_set_bluetooth_power(UPIO_BT_POWER_OFF);
		}
		else if (*state == BT_VND_PWR_ON)
		{
			ALOGD("=== power on BT ===");
			upio_set_bluetooth_power(UPIO_BT_POWER_ON);
			insmod_bt_sdio_driver();
		}
	}
	break;

	case BT_VND_OP_FW_CFG:
	{
		hw_config_start();
	}
	break;

	case BT_VND_OP_SCO_CFG:
	{
#if (SCO_CFG_INCLUDED == TRUE)
		hw_sco_config();
#else
		retval = -1;
#endif
	}
	break;

	case BT_VND_OP_USERIAL_OPEN:
	{
		int (*fd_array)[] = (int (*)[])param;
		int fd, idx;

		fd = userial_vendor_open((tUSERIAL_CFG *)&userial_init_cfg);
		g_userial_fd = fd;
		if (fd != -1)
		{
			for (idx = 0; idx < CH_MAX; idx++)
				(*fd_array)[idx] = fd;

			retval = 1;
		}
		/* retval contains numbers of open fd of HCI channels */
	}
	break;

	case BT_VND_OP_USERIAL_CLOSE:
	{
		property_get(PWR_PROP_NAME, shutdwon_status, "unkown");
		ALOGD("%s: shutdwon_status = %s ", __FUNCTION__, shutdwon_status);
		if (strstr(shutdwon_status, "0userrequested") != NULL)
		{
			ALOGD("amlbt shutdown");
			aml_woble_configure(g_userial_fd);
			return 0;
		}

		userial_vendor_close();
	}
	break;

	case BT_VND_OP_GET_LPM_IDLE_TIMEOUT:
	{
		uint32_t *timeout_ms = (uint32_t *)param;
		*timeout_ms = hw_lpm_get_idle_timeout();
	}
	break;

	case BT_VND_OP_LPM_SET_MODE:
	{
		usleep(100000);
		uint8_t *mode = (uint8_t *)param;
		retval = hw_lpm_enable(*mode);
	}
	break;

	case BT_VND_OP_LPM_WAKE_SET_STATE:
	{
		uint8_t *state = (uint8_t *)param;
		uint8_t wake_assert = (*state == BT_VND_LPM_WAKE_ASSERT) ? \
				      TRUE : FALSE;

		hw_lpm_set_wake_state(wake_assert);
	}
	break;

	case BT_VND_OP_SET_AUDIO_STATE:
	{
		retval = hw_set_audio_state((bt_vendor_op_audio_state_t *)param);
	}
	break;

	case BT_VND_OP_EPILOG:
	{
#if (HW_END_WITH_HCI_RESET == FALSE)
		if (bt_vendor_cbacks)
		{
			bt_vendor_cbacks->epilog_cb(BT_VND_OP_RESULT_SUCCESS);
		}
#else
		hw_epilog_process();
#endif
	}
	break;

	case BT_VND_OP_A2DP_OFFLOAD_START:
	case BT_VND_OP_A2DP_OFFLOAD_STOP:
	default:
		break;
	}

	return retval;
}

/** Closes the interface */
static void cleanup(void)
{
	BTVNDDBG("cleanup");

	upio_cleanup();

	bt_vendor_cbacks = NULL;
}

// Entry point of DLib
const bt_vendor_interface_t BLUETOOTH_VENDOR_LIB_INTERFACE = {
	sizeof(bt_vendor_interface_t),
	init,
	op,
	cleanup
};
