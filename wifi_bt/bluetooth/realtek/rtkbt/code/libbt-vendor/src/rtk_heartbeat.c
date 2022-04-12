/******************************************************************************
 *
 *  Copyright (C) 2009-2018 Realtek Corporation.
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
#define LOG_TAG "rtk_heartbeat"
#define RTKBT_RELEASE_NAME "20201130_BT_ANDROID_11.0"

#include <utils/Log.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <termios.h>
#include <sys/syscall.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <ctype.h>
#include <cutils/properties.h>
#include <stdlib.h>
#include "bt_hci_bdroid.h"
#include "bt_vendor_rtk.h"
#include "userial.h"
#include "userial_vendor.h"
#include "rtk_btservice.h"
#include "rtk_poll.h"
#include "upio.h"
#include <unistd.h>
#include <sys/eventfd.h>
#include <semaphore.h>
#include <endian.h>
#include <byteswap.h>
#include <sys/un.h>
#include <stddef.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "bt_vendor_lib.h"

#define RTKBT_HEARTBEAT_CONF_FILE         "/vendor/etc/bluetooth/rtkbt_heartbeat.conf"
#define BT_WAKE_EVT     "/sys/module/bt_device/parameters/btwake_evt"
#define BT_WAKE_HDMI    "/sys/class/amhdmitx/amhdmitx0/ready"



#define HCI_EVT_HEARTBEAT_STATUS_OFFSET          (5)
#define HCI_EVT_HEARTBEAT_SEQNUM_OFFSET_L          (6)
#define HCI_EVT_HEARTBEAT_SEQNUM_OFFSET_H          (7)

static const uint32_t DEFALUT_HEARTBEAT_TIMEOUT_MS = 1000; //send a per sercond
int heartBeatLog = -1;
static int heartBeatTimeout= -1;
static bool heartbeatFlag = false;
static int heartbeatCount= 0;
static uint16_t nextSeqNum= 1;
static uint16_t cleanupFlag = 0;
static pthread_mutex_t heartbeat_mutex;

typedef struct Rtk_Service_Data
{
    uint16_t        opcode;
    uint8_t         parameter_len;
    uint8_t         *parameter;
    void            (*complete_cback)(void *);
}Rtk_Service_Data;

extern void Rtk_Service_Vendorcmd_Hook(Rtk_Service_Data *RtkData, int client_sock);
extern uint8_t get_heartbeat_from_hardware();

static char *rtk_trim(char *str) {
    while (isspace(*str))
        ++str;

    if (!*str)
        return str;

    char *end_str = str + strlen(str) - 1;
    while (end_str > str && isspace(*end_str))
        --end_str;

    end_str[1] = '\0';
    return str;
}


static void load_rtkbt_heartbeat_conf()
{
    char *split;
    FILE *fp = fopen(RTKBT_HEARTBEAT_CONF_FILE, "rt");
    if (!fp) {
      ALOGE("%s unable to open file '%s': %s", __func__, RTKBT_HEARTBEAT_CONF_FILE, strerror(errno));
      return;
    }
    int line_num = 0;
    char line[1024];
    //char value[1024];
    while (fgets(line, sizeof(line), fp)) {
        char *line_ptr = rtk_trim(line);
        ++line_num;

        // Skip blank and comment lines.
        if (*line_ptr == '\0' || *line_ptr == '#' || *line_ptr == '[')
          continue;

        split = strchr(line_ptr, '=');
        if (!split) {
            ALOGE("%s no key/value separator found on line %d.", __func__, line_num);
            continue;
        }

        *split = '\0';
        char *endptr;
        if(!strcmp(rtk_trim(line_ptr), "HeartBeatTimeOut")) {
            heartBeatTimeout = strtol(rtk_trim(split+1), &endptr, 0);
        }
        else if(!strcmp(rtk_trim(line_ptr), "HeartBeatLog")) {
            heartBeatLog = strtol(rtk_trim(split+1), &endptr, 0);
        }
    }

    fclose(fp);

}

static void rtkbt_heartbeat_send_hw_error(uint8_t status, uint16_t seqnum, uint16_t next_seqnum, int heartbeatCnt)
{
    if(!heartbeatFlag)
      return;
    unsigned char p_buf[100];
    int length;
    p_buf[0] = HCIT_TYPE_EVENT;//event
    p_buf[1] = HCI_VSE_SUBCODE_DEBUG_INFO_SUB_EVT;//firmwre event log
    p_buf[3] = 0x01;// host log opcode
    length = sprintf((char *)&p_buf[4], "host stack: heartbeat hw error: %d:%d:%d:%d \n",
      status, seqnum, next_seqnum, heartbeatCnt);
    p_buf[2] = length + 2;//len
    length = length + 1 + 4;
    userial_recv_rawdata_hook(p_buf,length);

    length = 4;
    p_buf[0] = HCIT_TYPE_EVENT;//event
    p_buf[1] = HCI_HARDWARE_ERROR_EVT;//hardware error
    p_buf[2] = 0x01;//len
    p_buf[3] = HEARTBEAT_HWERR_CODE_RTK;//heartbeat error code
    userial_recv_rawdata_hook(p_buf,length);
}

static void rtkbt_heartbeat_cmpl_cback (void *p_params)
{
    uint8_t  status = 0;
    uint16_t seqnum = 0;
    int fd,sz1,sz2;
    HC_BT_HDR *p_evt_buf = NULL;
    char buf1[2],buf2[2];
    //uint8_t  *p = NULL;

    if(!heartbeatFlag)
        return;

    if(p_params != NULL)
    {
        p_evt_buf = (HC_BT_HDR *) p_params;
        status = p_evt_buf->data[HCI_EVT_HEARTBEAT_STATUS_OFFSET];
        seqnum = p_evt_buf->data[HCI_EVT_HEARTBEAT_SEQNUM_OFFSET_H]<<8 | p_evt_buf->data[HCI_EVT_HEARTBEAT_SEQNUM_OFFSET_L];
    }

    if(status == 0 && seqnum == nextSeqNum)
    {
        nextSeqNum = (seqnum + 1);
        pthread_mutex_lock(&heartbeat_mutex);
        heartbeatCount = 0;
        pthread_mutex_unlock(&heartbeat_mutex);
    }
    else
    {
        fd = open(BT_WAKE_EVT,O_RDONLY);
        if (fd < 0)
        {
            ALOGE("open(%s) failed: %s (%d)\n",
            BT_WAKE_EVT, strerror(errno), errno);
        }
        sz1 = read(fd, &buf1,sizeof(buf1));

        close(fd);

        fd = open(BT_WAKE_HDMI,O_RDONLY);
        if (fd < 0)
        {
            ALOGE("open(%s) failed: %s (%d)\n", \
            BT_WAKE_HDMI, strerror(errno), errno);
        }
        sz2 = read(fd, &buf2,sizeof(buf2));

        close(fd);

        if ((sz1 >= 1 && memcmp(buf1, "1", 1) == 0) && (sz2 >= 1 && memcmp(buf2, "0", 1) == 0)) {//rtc wakeup host and not Bright screen
            ALOGE("%s,rtc wakeup",__func__);
        }
        else {
            ALOGE("rtkbt_heartbeat_cmpl_cback: Current SeqNum = %d,should SeqNum=%d, status = %d", seqnum, nextSeqNum, status);
            ALOGE("heartbeat event missing:  restart bluedroid stack\n");
            usleep(1000);
            rtkbt_heartbeat_send_hw_error(status, seqnum, nextSeqNum, heartbeatCount);
        }
    }

}


static void heartbeat_timed_out()//(union sigval arg)
{
    Rtk_Service_Data *p_buf;
    int count;
    int fd,sz1,sz2;
    char buf1[2],buf2[2];

    if(!heartbeatFlag)
        return;
    fd = open(BT_WAKE_EVT,O_RDONLY);
    if (fd < 0)
    {
        ALOGE("open(%s) failed: %s (%d)\n", \
            BT_WAKE_EVT, strerror(errno), errno);
    }
    sz1 = read (fd, &buf1,sizeof(buf1));
    close(fd);

    fd = open(BT_WAKE_HDMI,O_RDONLY);
    if (fd < 0)
    {
        ALOGE("open(%s) failed: %s (%d)\n", \
            BT_WAKE_HDMI, strerror(errno), errno);
    }
    sz2 = read (fd, &buf2,sizeof(buf2));
    close(fd);

    if (sz1 >= 1 && memcmp(buf1, "1", 1) == 0) {   //rtc wakeup host
        ALOGE("%s,rtc wakeup,heartbeatCount=%d\n",__func__,heartbeatCount);
        pthread_mutex_lock(&heartbeat_mutex);
        heartbeatCount = 3;
        pthread_mutex_unlock(&heartbeat_mutex);
        poll_timer_flush();
        return;
    }
    else if (sz1 >= 1 && memcmp(buf1, "2", 1) == 0) { //bt wakeup host and Bright screen
        char btwake_init = '0';
        ALOGE("%s [abner ] heartbeatCount=%d  kill bt\n",__func__,heartbeatCount);

        fd = open(BT_WAKE_EVT,O_WRONLY);
        if (fd < 0)
        {
            ALOGE("open(%s) failed: %s (%d)\n", \
            BT_WAKE_EVT, strerror(errno), errno);
        }
        sz1 = write(fd, &btwake_init, 1);
        if (sz1 < 0) {
            ALOGE("write failed: %s",strerror(errno));
        }
        close(fd);
        count = heartbeatCount;
        usleep(1000);
        rtkbt_heartbeat_send_hw_error(0,0,nextSeqNum,count);
        return;
    }

    pthread_mutex_lock(&heartbeat_mutex);
    heartbeatCount++;
    if(heartbeatCount >= 3)
    {
        if(cleanupFlag == 1)
        {
            ALOGW("Already cleanup, ignore.");
            pthread_mutex_unlock(&heartbeat_mutex);
            return;
        }
        ALOGE("heartbeat_timed_out: heartbeatCount = %d, expected nextSeqNum = %d",heartbeatCount, nextSeqNum);
        ALOGE("heartbeat_timed_out,controller may be suspend! Now restart bluedroid stack\n");
        count = heartbeatCount;
        pthread_mutex_unlock(&heartbeat_mutex);
        usleep(1000);
        rtkbt_heartbeat_send_hw_error(0,0,nextSeqNum,count);

        //kill(getpid(), SIGKILL);
        return;
    }
    pthread_mutex_unlock(&heartbeat_mutex);
    if(heartbeatFlag)
    {
        p_buf = (Rtk_Service_Data *)malloc(sizeof(Rtk_Service_Data));
        if (NULL == p_buf)
        {
            ALOGE("p_buf: allocate error");
            return;
        }
        p_buf->opcode = HCI_CMD_VNDR_HEARTBEAT;
        p_buf->parameter = NULL;
        p_buf->parameter_len = 0;
        p_buf->complete_cback = rtkbt_heartbeat_cmpl_cback;

        Rtk_Service_Vendorcmd_Hook(p_buf, -1);
        free(p_buf);
        poll_timer_flush();
    }
}


static void rtkbt_heartbeat_beginTimer_func(void)
{
    Rtk_Service_Data *p_buf;

    if((heartBeatTimeout != -1) && (heartBeatLog != -1))
    {
        poll_init(heartbeat_timed_out,heartBeatTimeout);
    }
    else
    {
        heartBeatLog = 0;
        poll_init(heartbeat_timed_out,DEFALUT_HEARTBEAT_TIMEOUT_MS);
    }
    poll_enable(TRUE);

    p_buf = (Rtk_Service_Data *)malloc(sizeof(Rtk_Service_Data));
    if (NULL == p_buf)
    {
        ALOGE("p_buf: allocate error");
        return;
    }
    p_buf->opcode = HCI_CMD_VNDR_HEARTBEAT;
    p_buf->parameter = NULL;
    p_buf->parameter_len = 0;
    p_buf->complete_cback = rtkbt_heartbeat_cmpl_cback;

    Rtk_Service_Vendorcmd_Hook(p_buf, -1);
    free(p_buf);

    poll_timer_flush();
}

void Heartbeat_cleanup()
{
    if(!heartbeatFlag)
      return;
    heartbeatFlag = false;
    nextSeqNum = 1;
    heartbeatCount = 0;
    cleanupFlag = 1;
    poll_enable(FALSE);
    poll_cleanup();
}

void Heartbeat_init()
{
    int res;
    ALOGD("Heartbeat_init start");
    Heartbeat_cleanup();
    load_rtkbt_heartbeat_conf();
    pthread_mutex_init(&heartbeat_mutex, NULL);
    heartbeatFlag = true;
    heartbeatCount = 0;
    cleanupFlag = 0;
    res = get_heartbeat_from_hardware();
    ALOGD("Heartbeat_init res = %x",res);
    if(res == 1)
      rtkbt_heartbeat_beginTimer_func();
    else
      Heartbeat_cleanup();
    ALOGD("Heartbeat_init end");
}

