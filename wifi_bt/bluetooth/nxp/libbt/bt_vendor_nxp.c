/******************************************************************************
 *  Copyright 2012 The Android Open Source Project
 *  Portions copyright (C) 2009-2012 Broadcom Corporation
 *  Portions copyright 2012-2013, 2015, 2018-2021 NXP
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

#include <ctype.h>
#include <errno.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <pthread.h>
#include <pwd.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "bt_vendor_lib.h"
#ifdef FW_LOADER_V2
#include "fw_loader_uart_v2.h"
#else
#include "fw_loader_uart.h"
#endif

#define LOG_TAG "bt-vnd-mrvl"
#include <cutils/log.h>
#include <cutils/properties.h>

#define info(fmt, ...) ALOGI("%s(L%d): " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define debug(fmt, ...) ALOGD("%s(L%d): " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define warn(fmt, ...) ALOGW("WARNING : %s(L%d): " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define error(fmt, ...) ALOGE("ERROR : %s(L%d): " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define asrt(s) \
  if (!(s))     \
  ALOGE("## %s assert %s failed at line: %d", __FUNCTION__, #s, __LINE__)

#define XSTR(x) STR(x)
#define STR(x) #x

/******************************************************************************
 **
 ** Macro's
 **
 ******************************************************************************/
#define HCI_BT_SET_EVENTMASK_OCF 0x0001
#define HCI_CONTROLLER_CMD_OGF 0x03
#define HCI_RESET_OCF 0x03
#define HCI_DISABLE_PAGE_SCAN_OCF 0x001a

#define UINT16_TO_STREAM(p, u16)    \
  {                                 \
    *(p)++ = (uint8_t)(u16);        \
    *(p)++ = (uint8_t)((u16) >> 8); \
  }
#define UINT8_TO_STREAM(p, u8) \
  { *(p)++ = (uint8_t)(u8); }
#define OpCodePack(ogf, ocf) (uint16_t)((ocf & 0x03ff) | (ogf << 10))

/*If NXP_RESET_FW_IN_INITIA is defined, libbt will reset firmware during interface initializaion*/
#define NXP_RESET_FW_IN_INITIA

// 1 byte for event code, 1 byte for parameter length (Volume 2, Part E, 5.4.4)
#define HCI_EVENT_PREAMBLE_SIZE 2
#define HCI_COMMAND_COMPLETE_EVT 0x0E
#define HCI_PACKET_TYPE_EVENT 0x04
// 2 bytes for opcode, 1 byte for parameter length (Volume 2, Part E, 5.4.1)
#define HCI_COMMAND_PREAMBLE_SIZE 3

#define HCI_CMD_NXP_WRITE_BD_ADDRESS        0xFC22
#define WRITE_BD_ADDRESS_SIZE 8

typedef struct {
  uint16_t event;
  uint16_t len;
  uint16_t offset;
  uint16_t layer_specific;
  uint8_t data[];
} HC_BT_HDR;

/*[NK] @NXP - Driver FIX
  ioctl command to release the read thread before driver close */
#define MBTCHAR_IOCTL_RELEASE _IO('M', 1)

#ifndef NXP_INIT_TIME_SCO_CONFIGURATION
#define NXP_INIT_TIME_SCO_CONFIGURATION TRUE
#endif

#define PROP_BLUETOOTH_OPENED "bluetooth.mrvl.uart_configured"
#define PROP_BLUETOOTH_FW_DOWNLOADED "bluetooth.mrvl.fw_downloaded"
#define PROP_BLUETOOTH_DELAY "bluetooth.mrvl.fw_downloaded_delay"

/*
 * * Defines for wait for Bluetooth firmware ready
 * * Specify durations between polls and max wait time
 * */
#define POLL_DRIVER_DURATION_US (100000)
#define POLL_DRIVER_MAX_TIME_MS (20000)

/******************************************************************************
**  Variables
******************************************************************************/
int mchar_fd = 0;
struct termios ti;
int nxp_reset_fw = 1; // when libbt_dl_FW, don't reset FW in NXP_RESET_FW_IN_INITIA
                       // otherwise, reset FW to pass VTS loop back test

static uint8_t adapterState;
unsigned char *bdaddr = NULL;
const bt_vendor_callbacks_t* vnd_cb = NULL;
#ifndef VENDOR_LIB_CONF_FILE
#define VENDOR_LIB_CONF_FILE "/vendor/etc/bluetooth/nxp/bt_vendor.conf"
#endif
// for NXP USB/SD interface
static char mbt_port[512] = "/dev/mbtchar0";
// for NXP Uart interface
static char mchar_port[512] = "/dev/ttyUSB0";
static int is_uart_port = 0;
static int uart_break_before_open = 0;
static int32_t baudrate_fw_init = 115200;
static int32_t baudrate_bt = 3000000;

static uint8_t write_bd_address[WRITE_BD_ADDRESS_SIZE] = {
        0xFE, /* Parameter ID */
        0x06, /* bd_addr length */
        0x00, /* 6th byte of bd_addr */
        0x00, /* 5th */
        0x00, /* 4th */
        0x00, /* 3rd */
        0x00, /* 2nd */
        0x00  /* 1st */
};
int write_bdaddrss = 0;

#ifdef UART_DOWNLOAD_FW
int uart_break_before_change_baudrate = 0;
static int enable_download_fw = 0;
static int uart_break_after_dl_helper = 0;
static int uart_sleep_after_dl = 700;
static int download_helper = 0;
static int32_t baudrate_dl_helper = 115200;
static int32_t baudrate_dl_image = 3000000;
static char pFileName_helper[512] = "/vendor/firmware/mrvl/helper_uart_3000000.bin";
static char pFileName_image[512] = "/vendor/firmware/mrvl/uart8997_bt_v4.bin";
static int32_t iSecondBaudrate = 0;
#endif

static pthread_mutex_t dev_file_lock = PTHREAD_MUTEX_INITIALIZER;
void hw_mrvl_config_start(void);
void hw_mrvl_sco_config(void);

/*****************************************************************************
**
**   HELPER FUNCTIONS
**
*****************************************************************************/
#define CONF_COMMENT '#'
#define CONF_DELIMITERS " =\n\r\t"
#define CONF_VALUES_DELIMITERS "=\n\r\t"
#define CONF_MAX_LINE_LEN 255
#define UNUSED(x) (void)(x)

typedef int(conf_action_t)(char *p_conf_name, char *p_conf_value, int param);

typedef struct {
  const char *conf_entry;
  conf_action_t *p_action;
  int param;
} conf_entry_t;

static int set_mchar_port(char *p_conf_name, char *p_conf_value, int param) {
  UNUSED(p_conf_name);
  UNUSED(param);
  strcpy(mchar_port, p_conf_value);
  is_uart_port = 1;
  return 0;
}

static int set_mbt_port(char *p_conf_name, char *p_conf_value, int param) {
  UNUSED(p_conf_name);
  UNUSED(param);
  strcpy(mbt_port,p_conf_value);
  is_uart_port = 0;
  return 0;
}

static int set_is_uart_port(char *p_conf_name, char *p_conf_value, int param) {
  UNUSED(p_conf_name);
  UNUSED(param);
  is_uart_port = atoi(p_conf_value);
  return 0;
}

static int set_uart_break_before_open(char *p_conf_name, char *p_conf_value, int param) {
  UNUSED(p_conf_name);
  UNUSED(param);
  uart_break_before_open = atoi(p_conf_value);
  return 0;
}

static int set_baudrate_bt(char *p_conf_name, char *p_conf_value, int param) {
  UNUSED(p_conf_name);
  UNUSED(param);
  baudrate_bt = atoi(p_conf_value);
  return 0;
}

static int set_baudrate_fw_init(char *p_conf_name, char *p_conf_value, int param) {
  UNUSED(p_conf_name);
  UNUSED(param);
  baudrate_fw_init = atoi(p_conf_value);
  return 0;
}

static int set_bd_address_buf(char *p_conf_name, char *p_conf_value, int param) {
  UNUSED(p_conf_name);
  UNUSED(param);
  int i = 0;
  int j = 7;
  int len = 0;
  if(p_conf_value == NULL)
    return 0;
  len = strlen(p_conf_value);
  if(len != 17)
    return 0;
  for (i = 0; i < len; i++){
    if(((i + 1) % 3) == 0 && p_conf_value[i] != ':')
      return 0;
    if(((i + 1) % 3) != 0 && !isxdigit(p_conf_value[i]))
      return 0;
    char tmp = p_conf_value[i];
    if (isupper(p_conf_value[i])){
      p_conf_value[i] = p_conf_value[i] - 'A' + 10;
    }
    else if (islower(p_conf_value[i])){
      p_conf_value[i] = p_conf_value[i] - 'a' + 10;
    }
    else if(isdigit(p_conf_value[i])){
      p_conf_value[i] = p_conf_value[i] - '0';
    }
    else if(p_conf_value[i] == ':')
      p_conf_value[i] = tmp;
    else
      return 0;
  }
  for(i = 0; i < 17; i++){
    write_bd_address[j--] = (p_conf_value[i] << 4) | p_conf_value[i + 1];
    i = i + 2;
  }
  write_bdaddrss = 1;
  return 0;
}

#ifdef UART_DOWNLOAD_FW
static int set_enable_download_fw(char *p_conf_name, char *p_conf_value, int param) {
  UNUSED(p_conf_name);
  UNUSED(param);
  enable_download_fw = atoi(p_conf_value);
  return 0;
}

static int set_uart_break_before_change_baudrate(char *p_conf_name, char *p_conf_value, int param) {
  UNUSED(p_conf_name);
  UNUSED(param);
  uart_break_before_change_baudrate = atoi(p_conf_value);
  return 0;
}

static int set_uart_break_after_dl_helper(char *p_conf_name, char *p_conf_value, int param) {
  UNUSED(p_conf_name);
  UNUSED(param);
  uart_break_after_dl_helper = atoi(p_conf_value);
  return 0;
}

static int set_pFileName_image(char *p_conf_name, char *p_conf_value, int param) {
  UNUSED(p_conf_name);
  UNUSED(param);
  strcpy(pFileName_image, p_conf_value);
  return 0;
}

static int set_pFileName_helper(char *p_conf_name, char *p_conf_value, int param) {
  UNUSED(p_conf_name);
  UNUSED(param);
  strcpy(pFileName_helper, p_conf_value);
  download_helper = 1;
  return 0;
}

static int set_baudrate_dl_helper(char *p_conf_name, char *p_conf_value, int param) {
  UNUSED(p_conf_name);
  UNUSED(param);
  baudrate_dl_helper = atoi(p_conf_value);
  return 0;
}

static int set_baudrate_dl_image(char *p_conf_name, char *p_conf_value, int param) {
  UNUSED(p_conf_name);
  UNUSED(param);
  baudrate_dl_image = atoi(p_conf_value);
  return 0;
}

static int set_iSecondBaudrate(char *p_conf_name, char *p_conf_value, int param) {
  UNUSED(p_conf_name);
  UNUSED(param);
  iSecondBaudrate = atoi(p_conf_value);
  return 0;
}

#endif

static const conf_entry_t conf_table[] = {{"mchar_port", set_mchar_port, 0},
                                          {"mbt_port", set_mbt_port, 0},
                                          {"is_uart_port", set_is_uart_port, 0}, 
                                          {"uart_break_before_open", set_uart_break_before_open, 0},
                                          {"baudrate_bt", set_baudrate_bt, 0},
                                          {"baudrate_fw_init", set_baudrate_fw_init, 0},
                                          {"bd_address", set_bd_address_buf, 0},
#ifdef UART_DOWNLOAD_FW
                                          {"enable_download_fw", set_enable_download_fw, 0},
                                          {"uart_break_before_change_baudrate", set_uart_break_before_change_baudrate, 0},
                                          {"uart_break_after_dl_helper", set_uart_break_after_dl_helper, 0},
                                          {"pFileName_image", set_pFileName_image, 0},
                                          {"pFileName_helper", set_pFileName_helper, 0},
                                          {"baudrate_dl_helper", set_baudrate_dl_helper, 0},
                                          {"baudrate_dl_image", set_baudrate_dl_image, 0},
                                          {"iSecondBaudrate", set_iSecondBaudrate, 0},
#endif
                                          {(const char *)NULL, NULL, 0}};

static void vnd_load_conf(const char *p_path) {
  FILE *p_file;
  char *p_name;
  char *p_value;
  conf_entry_t *p_entry;
  char line[CONF_MAX_LINE_LEN + 1]; /* add 1 for \0 char */

  ALOGI("Attempt to load conf from %s", p_path);

  if ((p_file = fopen(p_path, "r")) != NULL) {
    /* read line by line */
    while (fgets(line, CONF_MAX_LINE_LEN + 1, p_file) != NULL) {
      if (line[0] == CONF_COMMENT)
        continue;

      p_name = strtok(line, CONF_DELIMITERS);

      if (NULL == p_name) {
        continue;
      }

      p_value = strtok(NULL, CONF_DELIMITERS);

      if (NULL == p_value) {
        ALOGW("vnd_load_conf: missing value for name: %s", p_name);
        continue;
      }

      p_entry = (conf_entry_t *)conf_table;

      while (p_entry->conf_entry != NULL) {
        if (strcmp(p_entry->conf_entry, (const char *)p_name) == 0) {
          p_entry->p_action(p_name, p_value, p_entry->param);
          break;
        }

        p_entry++;
      }
    }

    fclose(p_file);
  } else {
    ALOGI("vnd_load_conf file >%s< not found", p_path);
  }
}

static int set_speed(int fd, struct termios* ti, int speed) {
  if (cfsetospeed(ti, speed) < 0) {
    debug("Set O speed failed!\n");
    return -1;
  }

  if (cfsetispeed(ti, speed) < 0) {
    debug("Set I speed failed!\n");
    return -1;
  }

  if (tcsetattr(fd, TCSANOW, ti) < 0) {
    debug("Set Attr speed failed!\n");
    return -1;
  }

  return 0;
}

static int read_hci_event(int fd, unsigned char* buf, int size) {
  int remain, r;
  int count = 0;
  int k = 0;

  if (size <= 0)
    return -1;

  /* The first byte identifies the packet type. For HCI event packets, it
   * should be 0x04, so we read until we get to the 0x04. */
  debug("start read hci event 0x4\n");
  while (k < 20) {
    r = read(fd, buf, 1);
    if (r <= 0) {
      debug("read hci event 0x04 failed, retry\n");
      k++;
      usleep(50*1000);
      continue;
    }
    if (buf[0] == 0x04)
      break;
  }
  if(k >=20) {
      debug("read hci event 0x04 failed, return error. k = %d\n", k);
      return -1;
  }
  count++;

  /* The next two bytes are the event code and parameter total length. */
  debug("start read hci event code and len\n");
  while (count < 3) {
    r = read(fd, buf + count, 3 - count);
    if (r <= 0) {
      debug("read hci event code and len failed\n");
      return -1;
    }
    count += r;
  }

  /* Now we read the parameters. */
  debug("start read hci event para\n");
  if (buf[2] < (size - 3))
    remain = buf[2];
  else
    remain = size - 3;

  while ((count - 3) < remain) {
    r = read(fd, buf + count, remain - (count - 3));
    if (r <= 0) {
      debug("read hci event para failed\n");
      return -1;
    }
    count += r;
  }

  debug("over read count = %d\n", count);
  return count;
}

static int set_prop_int32(char* name, int value) {
  char init_value[PROPERTY_VALUE_MAX];
  int ret;

  sprintf(init_value, "%d", value);
  ret = property_set(name, init_value);
  if (ret < 0) {
    error("set_prop_int32 failed: %d", ret);
  }
  return ret;
}

static int get_prop_int32(char* name) {
  int ret;

  ret = property_get_int32(name, -1);
  debug("get_prop_int32: %d", ret);
  if (ret < 0) {
    return 0;
  }
  return ret;
}

/******************************************************************************
 *
 * Name: uart_speed
 *
 * Description:
 *   Return the baud rate corresponding to the frequency.
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *
 * Return Value:
 *
 * Notes:
 *   None.
 *
 *****************************************************************************/
static int32 uart_speed(int32 s) {
  switch (s) {
    case 9600:
      return B9600;
    case 19200:
      return B19200;
    case 38400:
      return B38400;
    case 57600:
      return B57600;
    case 115200:
      return B115200;
    case 230400:
      return B230400;
    case 460800:
      return B460800;
    case 500000:
      return B500000;
    case 576000:
      return B576000;
    case 921600:
      return B921600;
    case 1000000:
      return B1000000;
    case 1152000:
      return B1152000;
    case 1500000:
      return B1500000;
    case 3000000:
      return B3000000;
    default:
      return B0;
  }
}

/******************************************************************************
 *
 * Name: uart_set_speed
 *
 * Description:
 *   Set the baud rate speed.
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *
 * Return Value:
 *
 * Notes:
 *   None.
 *
 *****************************************************************************/
static int32 uart_set_speed(int32 fd, struct termios* ti, int32 speed) {
  cfsetospeed(ti, uart_speed(speed));
  return tcsetattr(fd, TCSANOW, ti);
}


/******************************************************************************
 *
 * Name: init_uart
 *
 * Description:
 *   Initialize UART.
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *
 * Return Value:
 *
 * Notes:
 *   None.
 *
 *****************************************************************************/
int32 init_uart(int8 * dev, int32 dwBaudRate, uint8 ucFlowCtrl)
{
 int32 fd = open(dev, O_RDWR | O_NOCTTY | O_NONBLOCK);
 if (fd < 0) {
	printf("Can't open serial port");
	return -1;
 }

 tcsendbreak(fd, 0);
 usleep(500000);

 tcflush(fd, TCIOFLUSH);

 if (tcgetattr(fd, &ti) < 0) {
	printf("Can't get port settings");
        close(fd);
	return -1;
 }

 cfmakeraw(&ti);
#ifdef FW_LOADER_V2
ti.c_cflag |= CLOCAL | CREAD;
#else
 ti.c_cflag |= CLOCAL;
#endif

 // Set 1 stop bit & no parity (8-bit data already handled by cfmakeraw)
 ti.c_cflag &= ~(CSTOPB | PARENB);

 if (ucFlowCtrl) {
	ti.c_cflag |= CRTSCTS;
 } else {
	ti.c_cflag &= ~CRTSCTS;
   }

 // FOR READS: set timeout time w/ no minimum characters needed (since
 // we read only 1 at at time...)
 ti.c_cc[VMIN] = 0;
 ti.c_cc[VTIME] = TIMEOUT_SEC * 10;

 if (tcsetattr(fd, TCSANOW, &ti) < 0) {
	printf("Can't set port settings");
        close(fd);
	return -1;
 }
 tcflush(fd, TCIOFLUSH);

 /* Set actual baudrate */
 if (uart_set_speed(fd, &ti, dwBaudRate) < 0) {
	printf("Can't set baud rate");
        close(fd);
	return -1;
 }

	return fd;
}

static int uart_init_open(int8* dev, int32 dwBaudRate, uint8 ucFlowCtrl) {
  int fd = 0, num = 0;
  do {
	fd = init_uart(dev, dwBaudRate, ucFlowCtrl);
    if (fd < 0) {
      num++;
      if (num >= 8) {
        error("exceed max retry count, return error");
        return -1;
      } else {
        error("open uart port %s failed fd: %d, retrying\n", dev, fd);
        usleep(50*1000);
        continue;
      }
    }
  } while (fd < 0);

  return fd;
}

#ifdef UART_DOWNLOAD_FW
/* return: 0 fw ready, 1 fw not ready */
static int detect_and_download_fw() {
  int download_ret = 0;
  int fw_downloaded = get_prop_int32(PROP_BLUETOOTH_FW_DOWNLOADED);

  if (fw_downloaded){
    debug("already downloaded");
    goto done;
  }

  /* detect fw status */
#ifdef FW_LOADER_V2
  if (bt_vnd_mrvl_check_fw_status_v2()) {
#else
  if (bt_vnd_mrvl_check_fw_status()) {
#endif
    /* force download only when header is received */
    fw_downloaded = 0;
  } else {
    if (mchar_fd) {
      tcflush(mchar_fd, TCIFLUSH);
      close(mchar_fd);
      mchar_fd = 0;
    }
    /* ignore download */
    fw_downloaded = 1;
    set_prop_int32(PROP_BLUETOOTH_OPENED, 1);
    set_prop_int32(PROP_BLUETOOTH_FW_DOWNLOADED, 1);
    goto done;
  }

  debug(" fw_downloaded %d", fw_downloaded);

  if (!fw_downloaded) {
#ifndef FW_LOADER_V2
    init_crc8();
#endif
    /* download helper */
    if (download_helper) {
#ifdef FW_LOADER_V2
      download_ret = bt_vnd_mrvl_download_fw_v2(mchar_port, baudrate_dl_helper, pFileName_helper);
#else
      download_ret = bt_vnd_mrvl_download_fw(mchar_port, baudrate_dl_helper, pFileName_helper, iSecondBaudrate);
#endif
      if (download_ret != 0) {
        debug("helper download failed");
        goto done;
      }
      tcflush(mchar_fd, TCIFLUSH);

      /* set baud rate to baudrate_dl_image,
         the chip's baud rate already set to baudrate_dl_image
         after download helper*/
      if (uart_set_speed(mchar_fd, &ti, baudrate_dl_image) < 0) {
        printf("Can't set baud rate");
        return -1;
      }
    }

    /* enable flow control*/
    ti.c_cflag |= CRTSCTS;
    if (tcsetattr(mchar_fd, TCSANOW, &ti) < 0) {
      debug("Set Flow Control failed!\n");
      return -1;
    }
    tcflush(mchar_fd, TCIOFLUSH);

    /* download fw image */
#ifdef FW_LOADER_V2
      download_ret = bt_vnd_mrvl_download_fw_v2(mchar_port, baudrate_dl_image, pFileName_image);
#else
      download_ret = bt_vnd_mrvl_download_fw(mchar_port, baudrate_dl_image, pFileName_image, iSecondBaudrate);
#endif
    if (download_ret != 0) {
      debug("fw download failed");
      goto done;
    }

    tcflush(mchar_fd, TCIFLUSH);
    if (uart_sleep_after_dl)
      usleep(uart_sleep_after_dl * 1000);

    set_prop_int32(PROP_BLUETOOTH_FW_DOWNLOADED, 1);
    nxp_reset_fw = 0;
  }
done:
  return download_ret;
}
#endif

static int config_uart() {
  int clen;
  unsigned char set_speed_cmd_3m[8] = {0x01, 0x09, 0xFC, 0x04, 0xC0, 0xC6, 0x2D, 0x00};
  unsigned char set_speed_cmd[8] = {0x01, 0x09, 0xFC, 0x04, 0x00, 0xC2, 0x01, 0x00};
  unsigned char reset_cmd[4] = {0x01, 0x03, 0x0c, 0x00};
  int resp_size;
  unsigned char resp[10] = {0};
  unsigned char resp_cmp[7] = {0x4, 0xe, 0x4, 0x1, 0x9, 0xfc, 0x0};
  unsigned char resp_cmp_reset[7] = {0x4, 0xe, 0x4, 0x1, 0x3, 0xc, 0x0};

  if (baudrate_fw_init != baudrate_bt) {
    /* set baud rate to baudrate_fw_init */
    if (uart_set_speed(mchar_fd, &ti, baudrate_fw_init) < 0) {
      printf("Can't set baud rate");
      return -1;
    }

    // ----------- HCI reset CMD
    debug("start send bt hci reset\n");
    memset(resp, 0x00, 10);
    clen = sizeof(reset_cmd);
    debug("Write HCI Reset command\n");
    if (write(mchar_fd, reset_cmd, clen) != clen) {
      debug("Failed to write reset command \n");
      return -1;
    }

    if ((resp_size = read_hci_event(mchar_fd, resp, 10)) < 0 || memcmp(resp, resp_cmp_reset, 7)) {
      debug("Failed to read HCI RESET CMD response! \n");
      return -1;
    }
    debug("over send bt hci reset\n");

    /* Set bt chip Baud rate CMD */
    debug("start set fw baud rate according to baudrate_bt\n");
    clen = sizeof(set_speed_cmd);
    if (baudrate_bt == 3000000) {
      debug("set fw baudrate as 3000000\n");
      if (write(mchar_fd, set_speed_cmd_3m, clen) != clen) {
        debug("Failed to write set baud rate command \n");
        return -1;
      }
    }
    else if (baudrate_bt == 115200) {
      debug("set fw baudrate as 115200");
      if (write(mchar_fd, set_speed_cmd, clen) != clen) {
        debug("Failed to write set baud rate command \n");
        return -1;
      }
    }

    debug("start read hci event\n");
    memset(resp, 0x00, 10);
    if ((resp_size = read_hci_event(mchar_fd, resp, 100)) < 0 || memcmp(resp, resp_cmp, 7)) {
      debug("Failed to read set baud rate command response! \n");
      return -1;
    }
    debug("over send bt chip baudrate\n");
    /* set host uart speed according to baudrate_bt */
    debug("start set host baud rate as baudrate_bt\n");
    tcflush(mchar_fd, TCIOFLUSH);
    if (set_speed(mchar_fd, &ti, uart_speed(baudrate_bt))) {
      debug("Failed to  set baud rate \n");
      return -1;
    }
    ti.c_cflag |= CRTSCTS;
    if (tcsetattr(mchar_fd, TCSANOW, &ti) < 0) {
      debug("Set Flow Control failed!\n");
      return -1;
    }
    tcflush(mchar_fd, TCIOFLUSH);

  }
  else {
    /* set host uart speed according to baudrate_bt */
    debug("start set host baud rate as baudrate_bt\n");
    tcflush(mchar_fd, TCIOFLUSH);
    if (set_speed(mchar_fd, &ti, uart_speed(baudrate_bt))) {
      debug("Failed to  set baud rate \n");
      return -1;
    }
    ti.c_cflag |= CRTSCTS;
    if (tcsetattr(mchar_fd, TCSANOW, &ti) < 0) {
      debug("Set Flow Control failed!\n");
      return -1;
    }
    tcflush(mchar_fd, TCIOFLUSH);
  }

  usleep(200 * 1000);
  set_prop_int32(PROP_BLUETOOTH_OPENED, 1);
  return 0;
}

static void bt_cmpltevet_callback(void *packet) {
  uint8_t *stream, event, event_code, status, opcode_offset;
  uint16_t opcode;

  stream = ((HC_BT_HDR *)packet)->data;
  event = ((HC_BT_HDR *)packet)->event;
  opcode_offset = HCI_EVENT_PREAMBLE_SIZE + 1;  // Skip num packets.

  if (event == HCI_PACKET_TYPE_EVENT) {
    event_code = stream[0];
    opcode = stream[opcode_offset] | (stream[opcode_offset + 1] << 8);
    if (event_code == HCI_COMMAND_COMPLETE_EVT) {
      status = stream[opcode_offset + 2];
      debug("opcode 0x%04x status 0x%02x\n", opcode, status);
      if (opcode == OpCodePack(HCI_CONTROLLER_CMD_OGF, HCI_RESET_OCF)) {
        debug("Receive hci reset complete event");
        /* bd_address is not set in bt_vendor.conf */
        if(write_bdaddrss == 0){
          if(bdaddr == NULL){
#if (NXP_INIT_TIME_SCO_CONFIGURATION == TRUE)
            hw_mrvl_sco_config();
#else
            /* fw config succeeds */
            debug("FW config succeeds!");
            if (vnd_cb)
              vnd_cb->fwcfg_cb(BT_VND_OP_RESULT_SUCCESS);
#endif
            return;
          }
          else if(bdaddr){
            for(int i = 0; i < 6; i++){
              write_bd_address[5-i] = bdaddr[i];
            }
          }
        }
        bt_bdaddress_set();
        if(write_bdaddrss == 1)
          write_bdaddrss = 0;
      }
      else if(opcode == HCI_CMD_NXP_WRITE_BD_ADDRESS) {
        debug("Receive BD_ADDRESS write config event.\n");
#if (NXP_INIT_TIME_SCO_CONFIGURATION == TRUE)
        hw_mrvl_sco_config();
#else
        /* fw config succeeds */
        debug("FW config succeeds!");
        if (vnd_cb) {
          vnd_cb->fwcfg_cb(BT_VND_OP_RESULT_SUCCESS);
        }
#endif
      }
    }
  }
}

static HC_BT_HDR *make_command(uint16_t opcode, size_t parameter_size) {
  HC_BT_HDR *packet = (HC_BT_HDR *)malloc(sizeof(HC_BT_HDR) + HCI_COMMAND_PREAMBLE_SIZE + parameter_size);
  if (!packet) {
    debug("%s Failed to allocate buffer\n", __func__);
    return NULL;
  }
  uint8_t *stream = packet->data;
  packet->event = 0;
  packet->offset = 0;
  packet->layer_specific = 0;
  packet->len = HCI_COMMAND_PREAMBLE_SIZE + parameter_size;
  UINT16_TO_STREAM(stream, opcode);
  UINT8_TO_STREAM(stream, parameter_size);
  if(opcode == HCI_CMD_NXP_WRITE_BD_ADDRESS){
    memcpy(stream, write_bd_address, parameter_size);
  }
  return packet;
}

static void bt_hci_reset(void) {
  uint16_t opcode;
  HC_BT_HDR *packet;
  opcode = OpCodePack(HCI_CONTROLLER_CMD_OGF, HCI_RESET_OCF);
  packet = make_command(opcode, 0);
  if (packet) {
    if (vnd_cb->xmit_cb(opcode, packet, bt_cmpltevet_callback))
      debug("%s send out command successfully\n", __func__);
  } else {
    debug("%s no valid packet \n", __func__);
  }
}

void bt_bdaddress_set(void){
  uint16_t opcode;
  HC_BT_HDR* packet;
  opcode =  HCI_CMD_NXP_WRITE_BD_ADDRESS;
  packet = make_command(opcode, WRITE_BD_ADDRESS_SIZE);
  if(packet){
    if(vnd_cb->xmit_cb(opcode, packet, bt_cmpltevet_callback))
      debug("%s send out command successfully\n", __func__);
  }
  else{
    debug("%s no valid packet \n", __func__);
  }
}

/*****************************************************************************
**
**   BLUETOOTH VENDOR INTERFACE LIBRARY FUNCTIONS
**
*****************************************************************************/
static int bt_vnd_mrvl_if_init(const bt_vendor_callbacks_t* p_cb, unsigned char* local_bdaddr) {
  vnd_cb = p_cb;
  if(vnd_cb == NULL)
    debug("vnd_cb is NULL\n");
    debug("bt_vnd_mrvl_if_init ---------------------------------\n");
    debug("bt_vnd_mrvl_if_init ---BT Vendor HAL Ver: %s ---\n", BT_HAL_VERSION);
  if(local_bdaddr){
    bdaddr = local_bdaddr;
    //memcpy(bdaddr, local_bdaddr, sizeof(bdaddr));
    if(bdaddr)
      debug("bdaddr is %02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX\n",
            bdaddr[0], bdaddr[1], bdaddr[2],bdaddr[3], bdaddr[4], bdaddr[5]);
  }
  vnd_load_conf(VENDOR_LIB_CONF_FILE);
  return 0;
}

static int bt_vnd_mrvl_if_op(bt_vendor_opcode_t opcode, void* param) {
  int ret = 0;
  int local_st = 0;

  debug("opcode = %d\n", opcode);
  switch (opcode) {
    case BT_VND_OP_POWER_CTRL: {
      int* state = (int*)param;

      if (*state == BT_VND_PWR_OFF) {
        debug("power off ***************************************\n");
        if (adapterState == BT_VND_PWR_ON) {
          debug("BT adapter switches from ON to OFF .. \n");
          adapterState = BT_VND_PWR_OFF;
        }
      } else if (*state == BT_VND_PWR_ON) {
        debug("power on --------------------------------------\n");
        adapterState = BT_VND_PWR_ON;
      }
    }
    break;
    case BT_VND_OP_FW_CFG:
      if (nxp_reset_fw) {
#if defined(NXP_RESET_FW_IN_INITIA)
        debug("Reset Firmware\n");
        bt_hci_reset();
#else
      if (vnd_cb) {
        vnd_cb->fwcfg_cb(ret);
      }
#endif
      }
      else{
        if (vnd_cb) {
          vnd_cb->fwcfg_cb(ret);
        }
        nxp_reset_fw = 1;
    }
    break;

    case BT_VND_OP_SCO_CFG:
      if (vnd_cb) {
        vnd_cb->scocfg_cb(ret);
      }
      break;
    case BT_VND_OP_USERIAL_OPEN: {
      debug("open serial port--------------------------------------\n");
      int(*fd_array)[] = (int(*)[])param;
      int idx;
      int bluetooth_opened;
      int num = 0;
      int32_t baudrate = 0;

      if (is_uart_port) {
        debug("baudrate_bt %d\n", baudrate_bt);
        debug("baudrate_fw_init %d\n", baudrate_fw_init);
#ifdef UART_DOWNLOAD_FW
        if (enable_download_fw){
          debug("download_helper %d\n", download_helper);
          debug("uart_break_before_change_baudrate %d\n", uart_break_before_change_baudrate);
          debug("baudrate_dl_helper %d\n", baudrate_dl_helper);
          debug("baudrate_dl_image %d\n", baudrate_dl_image);
          debug("pFileName_helper %s\n", pFileName_helper);
          debug("pFileName_image %s\n", pFileName_image);
          debug("iSecondBaudrate %d\n", iSecondBaudrate);
          debug("uart_break_before_open %d\n", uart_break_before_open);
          debug("enable_download_fw %d\n", enable_download_fw);
          debug("uart_break_after_dl_helper %d\n", uart_break_after_dl_helper);
          debug("uart_sleep_after_dl %d\n", uart_sleep_after_dl);
        }
#endif
      }
      pthread_mutex_lock(&dev_file_lock);

      if (is_uart_port) {
        /* ensure libbt can talk to the driver, only need open port once */
        if(get_prop_int32(PROP_BLUETOOTH_OPENED))
          mchar_fd = uart_init_open(mchar_port, baudrate_bt, 1);
        else {
#ifdef UART_DOWNLOAD_FW
          if (enable_download_fw) {
          /* if define micro UART_DOWNLOAD_FW, then open uart must with baudrate 115200,
             since libbt can only communicate with bootloader with baudrate 115200*/
          /* for 9098 helper is not need, so baudrate_dl_image is 115200, and iSecondBaudrate is true
             to set baudrate to 3000000 before download FW*/
             baudrate = (download_helper) ? baudrate_dl_helper : baudrate_dl_image;
          }
          else {
            baudrate = baudrate_fw_init;
          }
#else
          baudrate = baudrate_fw_init;
#endif
          mchar_fd = uart_init_open(mchar_port, baudrate, 0);
        }
        if (mchar_fd > 0)
          debug("open uart port successfully, fd=%d, mchar_port=%s\n", mchar_fd, mchar_port);
        else {
          error("open UART bt port %s failed fd: %d\n", mchar_port, mchar_fd);
          pthread_mutex_unlock(&dev_file_lock);
          return -1;
        }
      }
      else{
        do {
          mchar_fd = open(mbt_port, O_RDWR | O_NOCTTY);
          if (mchar_fd < 0) {
            num++;
            if (num >= 8){
              error("exceed max retry count, return error");
              pthread_mutex_unlock(&dev_file_lock);
              return -1;
            }
            else {
              error("open USB/SD port %s failed fd: %d, retrying\n", mbt_port, mchar_fd);
              sleep(1);
              continue;
            }
          }
          else {
            debug("open USB or SD port successfully, fd=%d, mbt_port=%s\n", mchar_fd, mbt_port);
            is_uart_port = 0;
          }
        } while (mchar_fd < 0);
      }

      if (is_uart_port) {
#ifdef UART_DOWNLOAD_FW
        if (enable_download_fw){
          if (detect_and_download_fw()) {
            error("detect_and_download_fw failed");
            set_prop_int32(PROP_BLUETOOTH_OPENED, 0);
            set_prop_int32(PROP_BLUETOOTH_FW_DOWNLOADED, 0);
            pthread_mutex_unlock(&dev_file_lock);
            return -1;
          }
        }
        else {
          ti.c_cflag |= CRTSCTS;
          if (tcsetattr(mchar_fd, TCSANOW, &ti) < 0) {
            debug("Set Flow Control failed!\n");
            return -1;
          }
          tcflush(mchar_fd, TCIOFLUSH);
        }
#else
        ti.c_cflag |= CRTSCTS;
        if (tcsetattr(mchar_fd, TCSANOW, &ti) < 0) {
          debug("Set Flow Control failed!\n");
          return -1;
        }
        tcflush(mchar_fd, TCIOFLUSH);
#endif
        bluetooth_opened = get_prop_int32(PROP_BLUETOOTH_OPENED);
        if (!bluetooth_opened) {
#ifdef UART_DOWNLOAD_FW
        if (!enable_download_fw)
#endif
        {
          // NXP Bluetooth use combo firmware which is loaded at wifi driver probe.
          // This function will wait to make sure basic client netdev is created
          int count = (POLL_DRIVER_MAX_TIME_MS * 1000) / POLL_DRIVER_DURATION_US;
          FILE *fd;

          while(count-- > 0) {
            if ((fd = fopen("/sys/class/net/wlan0", "r")) != NULL) {
              fclose(fd);
              break;
            }
            usleep(POLL_DRIVER_DURATION_US);
          }
        }

          if (config_uart()) {
            error("config_uart failed");
            set_prop_int32(PROP_BLUETOOTH_OPENED, 0);
            set_prop_int32(PROP_BLUETOOTH_FW_DOWNLOADED, 0);
            pthread_mutex_unlock(&dev_file_lock);
            return -1;
          }
        }
      }

      for (idx = 0; idx < CH_MAX; idx++) {
        (*fd_array)[idx] = mchar_fd;
        ret = 1;
      }
      pthread_mutex_unlock(&dev_file_lock);
      debug("open serial port over--------------------------------------\n");
    } break;
    case BT_VND_OP_USERIAL_CLOSE:
      /* mBtChar port is blocked on read. Release the port before we close it */
      pthread_mutex_lock(&dev_file_lock);
      if (is_uart_port) {
        if (mchar_fd) {
          tcflush(mchar_fd, TCIFLUSH);
          close(mchar_fd);
          mchar_fd = 0;
        }
      } else {
        ioctl(mchar_fd, MBTCHAR_IOCTL_RELEASE, &local_st);
        /* Give it sometime before we close the mbtchar */
        usleep(1000);
        if (mchar_fd) {
          if (close(mchar_fd) < 0) {
            error("");
            ret = -1;
          }
        }
      }
      pthread_mutex_unlock(&dev_file_lock);
      break;
    case BT_VND_OP_GET_LPM_IDLE_TIMEOUT:
      break;
    case BT_VND_OP_LPM_SET_MODE:
      if (vnd_cb) {
        vnd_cb->lpm_cb(ret);
      }
      break;
    case BT_VND_OP_LPM_WAKE_SET_STATE:
      break;
    default:
      ret = -1;
      break;
  }
  return ret;
}

static void bt_vnd_mrvl_if_cleanup(void) {
  debug("cleanup ...");
  vnd_cb = NULL;
}

const bt_vendor_interface_t BLUETOOTH_VENDOR_LIB_INTERFACE = {
    sizeof(bt_vendor_interface_t), bt_vnd_mrvl_if_init, bt_vnd_mrvl_if_op, bt_vnd_mrvl_if_cleanup,
};
