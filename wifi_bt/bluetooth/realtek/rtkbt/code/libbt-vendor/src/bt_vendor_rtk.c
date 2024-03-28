/******************************************************************************
 *
 *  Copyright (C) 2009-2018 Realtek Corporation
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
 *  Filename:      bt_vendor_rtk.c
 *
 *  Description:   Realtek vendor specific library implementation
 *
 ******************************************************************************/

#undef NDEBUG
#define LOG_TAG "libbt_vendor"
#define RTKBT_RELEASE_NAME "20240315_BT_ANDROID_14.0"
#include <utils/Log.h>
#include "bt_vendor_rtk.h"
#include "upio.h"
#include "userial_vendor.h"


/******************************************************************************
**  Externs
******************************************************************************/
extern unsigned int rtkbt_h5logfilter;
extern unsigned int h5_log_enable;
extern bool rtk_btsnoop_dump;
extern bool rtk_btsnoop_net_dump;
extern bool rtk_btsnoop_save_log;
extern char rtk_btsnoop_path[];
extern uint8_t coex_log_enable;
extern rtkbt_cts_info_t rtkbt_cts_info;
extern void hw_config_start(char transtype);
extern void hw_usb_config_start(char transtype, uint32_t val);
extern void hci_close_firmware_log_file(int fd);
extern int hci_firmware_log_fd;
#if (HW_END_WITH_HCI_RESET == TRUE)
void hw_epilog_process(void);
#endif

/******************************************************************************
**  Variables
******************************************************************************/
tPOWERON_CFG pwr_cfg = {0};
tAPCF_CFG *p_apcf_cfg = NULL;

bt_vendor_callbacks_t *bt_vendor_cbacks = NULL;
uint8_t vnd_local_bd_addr[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
bool rtkbt_auto_restart = false;
bool rtkbt_capture_fw_log = false;
bool fwlog_acl = false;
pthread_t thrd_dl;
int poll_dl_fd;
int event_dl_fd;

/******************************************************************************
**  Local type definitions
******************************************************************************/
#define DEVICE_NODE_MAX_LEN     512
#define PATH_MAX_LEN            100
#define RTKBT_CONF_FILE         "/vendor/etc/bluetooth/rtkbt.conf"
#define USB_DEVICE_DIR          "/sys/bus/usb/devices"
#define DEBUG_SCAN_USB          FALSE

/******************************************************************************
**  Static Variables
******************************************************************************/
//transfer_type(4 bit) | transfer_interface(4 bit)
char rtkbt_transtype = 0;
static char rtkbt_device_node[DEVICE_NODE_MAX_LEN] = {0};

static const tUSERIAL_CFG userial_H5_cfg =
{
    (USERIAL_DATABITS_8 | USERIAL_PARITY_EVEN | USERIAL_STOPBITS_1),
    USERIAL_BAUD_115200,
    USERIAL_HW_FLOW_CTRL_OFF
};
const tUSERIAL_CFG userial_H45_cfg =
{
    (USERIAL_DATABITS_8 | USERIAL_PARITY_EVEN | USERIAL_STOPBITS_1),
    USERIAL_BAUD_1_5M,
    USERIAL_HW_FLOW_CTRL_OFF
};
static const tUSERIAL_CFG userial_H4_cfg =
{
    (USERIAL_DATABITS_8 | USERIAL_PARITY_NONE | USERIAL_STOPBITS_1),
    USERIAL_BAUD_115200,
    USERIAL_HW_FLOW_CTRL_OFF
};


/******************************************************************************
**  Functions
******************************************************************************/
static int Check_Key_Value(char *path, char *key, int value)
{
    FILE *fp;
    char newpath[PATH_MAX_LEN];
    char string_get[6];
    int value_int = 0;
    memset(newpath, 0, PATH_MAX_LEN);
    snprintf(newpath, PATH_MAX_LEN, "%s/%s", path, key);
    if ((fp = fopen(newpath, "r")) != NULL)
    {
        memset(string_get, 0, 6);
        if (fgets(string_get, 5, fp) != NULL)
            if (DEBUG_SCAN_USB)
            {
                ALOGE("string_get %s =%s\n", key, string_get);
            }
        fclose(fp);
        value_int = strtol(string_get, NULL, 16);
        if (value_int == value)
        {
            return 1;
        }
    }
    return 0;
}

static int Scan_Usb_Devices_For_RTK(char *path)
{
    char newpath[PATH_MAX_LEN];
    char subpath[PATH_MAX_LEN];
    DIR *pdir;
    DIR *newpdir;
    struct dirent *ptr;
    struct dirent *newptr;
    struct stat filestat;
    struct stat subfilestat;
    if (stat(path, &filestat) != 0)
    {
        ALOGE("The file or path(%s) can not be get stat!\n", newpath);
        return -1;
    }
    if ((filestat.st_mode & S_IFDIR) != S_IFDIR)
    {
        ALOGE("(%s) is not be a path!\n", path);
        return -1;
    }
    pdir = opendir(path);
    if (!pdir)
    {
        ALOGE("(%s) open fail: %s", path, strerror(errno));
        return -1;
    }
    /*enter sub direc*/
    while ((ptr = readdir(pdir)) != NULL)
    {
        if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0)
        {
            continue;
        }
        memset(newpath, 0, PATH_MAX_LEN);
        snprintf(newpath, PATH_MAX_LEN, "%s/%s", path, ptr->d_name);
        if (DEBUG_SCAN_USB)
        {
            ALOGI("The file or path(%s)\n", newpath);
        }
        if (stat(newpath, &filestat) != 0)
        {
            ALOGE("The file or path(%s) can not be get stat!\n", newpath);
            continue;
        }
        /* Check if it is path. */
        if ((filestat.st_mode & S_IFDIR) == S_IFDIR)
        {
            if (!Check_Key_Value(newpath, "idVendor", 0x0bda))
            {
                continue;
            }
            newpdir = opendir(newpath);
            /*read sub directory*/
            while ((newptr = readdir(newpdir)) != NULL)
            {
                if (strcmp(newptr->d_name, ".") == 0 || strcmp(newptr->d_name, "..") == 0)
                {
                    continue;
                }
                memset(subpath, 0, PATH_MAX_LEN);
                snprintf(subpath, PATH_MAX_LEN, "%s/%s", newpath, newptr->d_name);
                if (DEBUG_SCAN_USB)
                {
                    ALOGI("The file or path(%s)\n", subpath);
                }
                if (stat(subpath, &subfilestat) != 0)
                {
                    ALOGE("The file or path(%s) can not be get stat!\n", newpath);
                    continue;
                }
                /* Check if it is path. */
                if ((subfilestat.st_mode & S_IFDIR) == S_IFDIR)
                {
                    if (Check_Key_Value(subpath, "bInterfaceClass", 0xe0) && \
                        Check_Key_Value(subpath, "bInterfaceSubClass", 0x01) && \
                        Check_Key_Value(subpath, "bInterfaceProtocol", 0x01))
                    {
                        closedir(newpdir);
                        closedir(pdir);
                        return 1;
                    }
                }
            }
            closedir(newpdir);
        }
    }
    closedir(pdir);
    return 0;
}
static char *rtk_trim(char *str)
{
    while (isspace(*str))
    {
        ++str;
    }

    if (!*str)
    {
        return str;
    }

    char *end_str = str + strlen(str) - 1;
    while (end_str > str && isspace(*end_str))
    {
        --end_str;
    }

    end_str[1] = '\0';
    return str;
}

#define ld_2_apcf_cfg(n,b) \
    else if(!strncmp(rtk_trim(line_ptr),#n,strlen(#n))){ \
        snprintf(t,200,""#n"%d",i+1); \
        if(!strncmp(rtk_trim(line_ptr),t,strlen(t))){ \
            p_apcf_cfg[i].b = malloc(strlen(rtk_trim(split+1)) +1);\
            p_apcf_cfg[i].b##_len = strlen(rtk_trim(split+1));\
            strncpy(p_apcf_cfg[i].b,rtk_trim(split+1),strlen(rtk_trim(split+1)) +1);\
        }\
    } \

static void load_rtkbt_stack_conf()
{
    char *split;
    FILE *fp = fopen(RTKBT_CONF_FILE, "rt");
    if (!fp)
    {
        ALOGE("%s unable to open file '%s': %s", __func__, RTKBT_CONF_FILE, strerror(errno));
        return;
    }
    int line_num = 0, i = 0;
    char line[1024], t[200] = {0};
    //char value[1024];
    while (fgets(line, sizeof(line), fp))
    {
        char *line_ptr = rtk_trim(line);
        ++line_num;

        // Skip blank and comment lines.
        if (*line_ptr == '\0' || *line_ptr == '#' || *line_ptr == '[')
        {
            continue;
        }

        split = strchr(line_ptr, '=');
        if (!split)
        {
            ALOGE("%s no key/value separator found on line %d.", __func__, line_num);
            continue;
        }

        *split = '\0';
        char *endptr;
        if (!strcmp(rtk_trim(line_ptr), "RtkbtLogFilter"))
        {
            rtkbt_h5logfilter = strtol(rtk_trim(split + 1), &endptr, 0);
        }
        else if (!strcmp(rtk_trim(line_ptr), "H5LogOutput"))
        {
            h5_log_enable = strtol(rtk_trim(split + 1), &endptr, 0);
        }
        else if (!strcmp(rtk_trim(line_ptr), "RtkBtsnoopNetDump"))
        {
            if (!strcmp(rtk_trim(split + 1), "true"))
            {
                rtk_btsnoop_net_dump = true;
            }
        }
        else if (!strcmp(rtk_trim(line_ptr), "BtSnoopFileName"))
        {
            snprintf(rtk_btsnoop_path, 1024, "%s_rtk", rtk_trim(split + 1));
        }
        else if (!strcmp(rtk_trim(line_ptr), "BtCoexLogOutput"))
        {
            coex_log_enable = strtol(rtk_trim(split + 1), &endptr, 0);
        }
        else if (!strcmp(rtk_trim(line_ptr), "RtkBtAutoRestart"))
        {
            if (!strcmp(rtk_trim(split + 1), "true"))
            {
                rtkbt_auto_restart = true;
            }
        }
        else if (!strcmp(rtk_trim(line_ptr), "RtkBtCaptureFwLog"))
        {
            if (!strcmp(rtk_trim(split + 1), "true"))
            {
                rtkbt_capture_fw_log = true;
            }
        }
        else if (!strcmp(rtk_trim(line_ptr), "RtkBtFwLog_ACL"))
        {
            if (!strcmp(rtk_trim(split + 1), "true"))
            {
                fwlog_acl = true;
            }
        }
        else if (!strcmp(rtk_trim(line_ptr), "RtkAPCFWakeUpEn"))
        {
            if (!strcmp(rtk_trim(split + 1), "true"))
            {
                pwr_cfg.rtkbt_apcf_wp_en = true;
            }
        }
        else if (!strncmp(rtk_trim(line_ptr), "RtkAPCFWakeUpSets", strlen("RtkAPCFWakeUpSets")))
        {
            pwr_cfg.nm_filter_idx = strtol(rtk_trim(split + 1), &endptr, 0);
            p_apcf_cfg = (tAPCF_CFG *)malloc(pwr_cfg.nm_filter_idx * sizeof(tAPCF_CFG));
        }
        ld_2_apcf_cfg(RtkAPCFWakeUpLocalName, local_name)
        ld_2_apcf_cfg(RtkAPCFWakeUpServiceUUID, service_uuid)
        ld_2_apcf_cfg(RtkAPCFWakeUpServiceData, service_data)
        ld_2_apcf_cfg(RtkAPCFWakeUpServiceDMask, service_data_mask)
        ld_2_apcf_cfg(RtkAPCFWakeUpCompanyId, company_id)
        ld_2_apcf_cfg(RtkAPCFWakeUpCIdMask, company_id_mask)
        ld_2_apcf_cfg(RtkAPCFWakeUpManuData, manu_data)
        ld_2_apcf_cfg(RtkAPCFWakeUpManuDMask, manu_data_mask)
        ld_2_apcf_cfg(RtkAPCFWakeUpBdAddr, bd)
        ld_2_apcf_cfg(RtkAPCFWakeUpADType, ad_type)
        ld_2_apcf_cfg(RtkAPCFWakeUpADData, ad_data)
        ld_2_apcf_cfg(RtkAPCFWakeUpADMask, ad_data_mask)
        ld_2_apcf_cfg(RtkAPCFWakeUpVDData, vd_data)
        else if (!strncmp(rtk_trim(line_ptr), "RtkAPCFWakeUpWaveDur", strlen("RtkAPCFWakeUpWaveDur")))
        {
            snprintf(t, 200, "RtkAPCFWakeUpWaveDur%d", i + 1);
            if (!strncmp(rtk_trim(line_ptr), t, strlen(t)))
            {
                pwr_cfg.rtkbt_apcf_wp_wd[i] = strtoul(rtk_trim(split + 1), &endptr, 0);
            }
        }
        else if (!strncmp(rtk_trim(line_ptr), "RtkAPCFWakeUpWaveFre", strlen("RtkAPCFWakeUpWaveFre")))
        {
            snprintf(t, 200, "RtkAPCFWakeUpWaveFre%d", i + 1);
            if (!strncmp(rtk_trim(line_ptr), t, strlen(t)))
            {
                pwr_cfg.rtkbt_apcf_wp_wf[i] = strtoul(rtk_trim(split + 1), NULL, 16);
            }
        }
        else if (!strncmp(rtk_trim(line_ptr), "RtkAPCFWakeUpWaveTime", strlen("RtkAPCFWakeUpWaveTime")))
        {
            snprintf(t, 200, "RtkAPCFWakeUpWaveTime%d", i + 1);
            if (!strncmp(rtk_trim(line_ptr), t, strlen(t)))
            {
                pwr_cfg.rtkbt_apcf_wp_tm[i++] = strtoul(rtk_trim(split + 1), NULL, 0);
            }
        }
    }
    fclose(fp);
}

#define free_apcf_cfg(n) \
    if(p_apcf_cfg[i].n##_len > 0){\
        free(p_apcf_cfg[i].n);\
        p_apcf_cfg[i].n = NULL;\
        p_apcf_cfg[i].n##_len = 0;\
    }\

#define free_sub_apcf_cfg(a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12)\
    free_apcf_cfg(local_name);free_apcf_cfg(service_uuid);free_apcf_cfg(service_data);free_apcf_cfg(service_data_mask);\
    free_apcf_cfg(company_id);free_apcf_cfg(company_id_mask);free_apcf_cfg(manu_data);free_apcf_cfg(manu_data_mask);free_apcf_cfg(bd);\
    free_apcf_cfg(ad_type);free_apcf_cfg(ad_data);free_apcf_cfg(ad_data_mask);\

#define rtkbt_wakeup_cfg_clean() \
    for(int i =0 ;i < pwr_cfg.nm_filter_idx;i++){ \
        free_sub_apcf_cfg(local_name,service_uuid,service_data,service_data_mask,company_id,company_id_mask,manu_data,manu_data_mask,bd,ad_type,ad_data,ad_data_mask);\
    }\
    free(p_apcf_cfg);\
    p_apcf_cfg= NULL;\

static void rtkbt_stack_conf_cleanup()
{
    rtkbt_h5logfilter = 0;
    h5_log_enable = 0;
    rtk_btsnoop_dump = false;
    rtk_btsnoop_net_dump = false;
    rtkbt_capture_fw_log = false;
}

static void load_rtkbt_conf()
{
    char *split;
    memset(rtkbt_device_node, 0, sizeof(rtkbt_device_node));
    FILE *fp = fopen(RTKBT_CONF_FILE, "rt");
    if (!fp)
    {
        ALOGE("%s unable to open file '%s': %s", __func__, RTKBT_CONF_FILE, strerror(errno));
        strncpy(rtkbt_device_node, "/dev/rtkbt_dev", DEVICE_NODE_MAX_LEN);
        return;
    }

    int line_num = 0;
    char line[1024];
    while (fgets(line, sizeof(line), fp))
    {
        char *line_ptr = rtk_trim(line);
        ++line_num;

        // Skip blank and comment lines.
        if (*line_ptr == '\0' || *line_ptr == '#' || *line_ptr == '[')
        {
            continue;
        }

        split = strchr(line_ptr, '=');
        if (!split)
        {
            ALOGE("%s no key/value separator found on line %d.", __func__, line_num);
            strncpy(rtkbt_device_node, "/dev/rtkbt_dev", DEVICE_NODE_MAX_LEN);
            fclose(fp);
            return;
        }

        *split = '\0';
        if (!strcmp(rtk_trim(line_ptr), "BtDeviceNode"))
        {
            strncpy(rtkbt_device_node, rtk_trim(split + 1), DEVICE_NODE_MAX_LEN);
        }
    }

    fclose(fp);

    rtkbt_transtype = 0;
    if (rtkbt_device_node[0] == '?')
    {
        /*1.Scan_Usb_Device*/
        if (Scan_Usb_Devices_For_RTK(USB_DEVICE_DIR) == 0x01)
        {
            strncpy(rtkbt_device_node, "/dev/rtkbt_dev", DEVICE_NODE_MAX_LEN);
        }
        else
        {
            int i = 0;
            while (rtkbt_device_node[i] != '\0')
            {
                rtkbt_device_node[i] = rtkbt_device_node[i + 1];
                i++;
            }
        }
    }

    if ((split = strchr(rtkbt_device_node, ':')) != NULL)
    {
        *split = '\0';
        if (!strcmp(rtk_trim(split + 1), "H5"))
        {
            rtkbt_transtype |= RTKBT_TRANS_H5;
        }
        else if (!strcmp(rtk_trim(split + 1), "H4"))
        {
            rtkbt_transtype |= RTKBT_TRANS_H4;
        }
        else if (!strcmp(rtk_trim(split + 1), "H45"))
        {
            rtkbt_transtype |= RTKBT_TRANS_H45;
        }
    }
    else if (strcmp(rtkbt_device_node, "/dev/rtkbt_dev"))
    {
        //default use h5
        rtkbt_transtype |= RTKBT_TRANS_H5;
    }

    if (strcmp(rtkbt_device_node, "/dev/rtkbt_dev"))
    {
        rtkbt_transtype |= RTKBT_TRANS_UART;
    }
    else
    {
        rtkbt_transtype |= RTKBT_TRANS_USB;
        rtkbt_transtype |= RTKBT_TRANS_H4;
    }
}

static void byte_reverse(unsigned char *data, int len)
{
    int i;
    int tmp;

    for (i = 0; i < len / 2; i++)
    {
        tmp = len - i - 1;
        data[i] ^= data[tmp];
        data[tmp] ^= data[i];
        data[i] ^= data[tmp];
    }
}

static void *vendor_dl_fw_thrd()
{
    int ret, evt_ret = -1;
    eventfd_t value;
    struct epoll_event events[64] = {{0}};
    prctl(PR_SET_NAME, (unsigned long)"dl_fw_thread", 0, 0, 0);

    do
    {
        do
        {
            ret = epoll_wait(poll_dl_fd, events, 64, -1);
        }
        while (ret == -1 && errno == EINTR);
        if (ret == -1)
        {
            ALOGE("%s wait for poll_dl_fd error=%s", __func__, strerror(errno));
            return NULL;
        }
        int i;
        for (i = 0; i < ret; i++)
        {
            evt_ret = eventfd_read(event_dl_fd, &value);
            ALOGI("%s eventfd_read i:%d evt_ret:%d value=%ld", __func__, i, evt_ret, (long)value);
            if (evt_ret == 0)
            {
                break;
            }
        }
    }
    while (evt_ret);

    if (rtkbt_transtype & RTKBT_TRANS_UART)
    {
        hw_config_start(rtkbt_transtype);
    }
    else
    {
        int usb_info = 0;
        ret = userial_vendor_usb_ioctl(GET_USB_INFO, &usb_info);
        if (ret == -1)
        {
            ALOGE("get usb info fail");
            bt_vendor_cbacks->fwcfg_cb(BT_VND_OP_RESULT_FAIL);
            return NULL;
        }
        else
        {
            hw_usb_config_start(RTKBT_TRANS_H4, usb_info);
        }
    }
    return NULL;
}

static void rtk_create_dl_fw_thrd()
{
    struct epoll_event event;
    int policy, result;
    struct sched_param param;

    poll_dl_fd = epoll_create(64);
    assert(poll_dl_fd != -1);
    event_dl_fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    assert(event_dl_fd != -1);
    if (poll_dl_fd != -1 && event_dl_fd != -1)
    {
        memset(&event, 0, sizeof(event));
        event.events |= EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR;
        event.data.ptr = NULL;
        if (epoll_ctl(poll_dl_fd, EPOLL_CTL_ADD, event_dl_fd, &event) == -1)
        {
            ALOGE("%s unable to register fd %d to poll set: %s", __func__, event_dl_fd, strerror(errno));
            assert(false);
        }
    }

    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);

    if (pthread_create(&thrd_dl, &thread_attr, vendor_dl_fw_thrd, NULL) != 0)
    {
        ALOGE("pthread vendor dl fw : %s", strerror(errno));
        thrd_dl = -1;
        assert(false);
    }
    if (pthread_getschedparam(thrd_dl, &policy, &param) == 0)
    {
        policy = SCHED_FIFO;
        param.sched_priority  = 99;
        result = pthread_setschedparam(thrd_dl, policy, &param);
        if (result != 0)
        {
            ALOGE("%s : pthread_setschedparam failed (%s)", __func__,
                  strerror(result));
        }
    }
}
/*****************************************************************************
**
**   BLUETOOTH VENDOR INTERFACE LIBRARY FUNCTIONS
**
*****************************************************************************/

static int init(const bt_vendor_callbacks_t *p_cb, unsigned char *local_bdaddr)
{
    ALOGI("RTKBT_RELEASE_NAME: %s", RTKBT_RELEASE_NAME);
    ALOGI("init");

    char value[100] = {0};
    load_rtkbt_conf();
    load_rtkbt_stack_conf();
    if (p_cb == NULL)
    {
        ALOGE("init failed with no user callbacks!");
        return -1;
    }

    userial_vendor_init(rtkbt_device_node);

#if 0
    if (rtkbt_transtype & RTKBT_TRANS_UART)
    {
        upio_init();
        ALOGI("bt_wake_up_host_mode_set(1)");
        bt_wake_up_host_mode_set(1);
    }
#endif

    /* store reference to user callbacks */
    bt_vendor_cbacks = (bt_vendor_callbacks_t *) p_cb;

    /* This is handed over from the stack */
    memcpy(vnd_local_bd_addr, local_bdaddr, 6);
    byte_reverse(vnd_local_bd_addr, 6);

    property_get("persist.vendor.btsnoop.enable", value, "false");
    if (strncmp(value, "true", 4) == 0)
    {
        rtk_btsnoop_dump = true;
    }

    property_get("persist.vendor.btsnoopsavelog", value, "false");
    if (strncmp(value, "true", 4) == 0)
    {
        rtk_btsnoop_save_log = true;
    }

    property_get("vendor.realtek.bluetooth.en", value, "false");
    memset(rtkbt_cts_info.addr, 0xff, 6);
    if (strncmp(value, "true", 4) == 0)
    {
        rtkbt_cts_info.finded = true;
    }

    ALOGI("rtk_btsnoop_dump = %d, rtk_btsnoop_save_log = %d", rtk_btsnoop_dump, rtk_btsnoop_save_log);
    if (rtk_btsnoop_dump)
    {
        rtk_btsnoop_open();
    }
    if (rtk_btsnoop_net_dump)
    {
        rtk_btsnoop_net_open();
    }
    rtk_create_dl_fw_thrd();

    return 0;
}



/** Requested operations */
static int op(bt_vendor_opcode_t opcode, void *param)
{
    int retval = 0;

    BTVNDDBG("op for %d", opcode);

    switch (opcode)
    {
    case BT_VND_OP_POWER_CTRL:
        {
            if (rtkbt_transtype & RTKBT_TRANS_UART)
            {
                int *state = (int *) param;
                if (*state == BT_VND_PWR_OFF)
                {
                    upio_set_bluetooth_power(UPIO_BT_POWER_OFF);
                    usleep(200000);
                    BTVNDDBG("set power off and delay 200ms");
                }
                else if (*state == BT_VND_PWR_ON)
                {
                    upio_set_bluetooth_power(UPIO_BT_POWER_OFF);
                    usleep(200000);
                    BTVNDDBG("set power off and delay 200ms");
                    upio_set_bluetooth_power(UPIO_BT_POWER_ON);
                    if (rtkbt_transtype & RTKBT_TRANS_H4)
                    {
                        usleep(1000000);
                        BTVNDDBG("set power on and delay 1000ms");
                    }
                    else
                    {
                        //usleep(200000);
                        BTVNDDBG("set power on and delay 00ms");
                    }
                }
            }
        }
        break;

    case BT_VND_OP_FW_CFG:
        retval = eventfd_write(event_dl_fd, 1);
        if (retval == -1)
        {
            ALOGE("%s eventfd_write error=%s exit", __func__, strerror(errno));
        }
        break;

    case BT_VND_OP_SCO_CFG:
        {
            retval = -1;
        }
        break;

    case BT_VND_OP_USERIAL_OPEN:
        {
            if ((rtkbt_transtype & RTKBT_TRANS_UART) && (rtkbt_transtype & RTKBT_TRANS_H5))
            {
                int fd, idx;
                int (*fd_array)[] = (int (*)[]) param;
                if (userial_vendor_open((tUSERIAL_CFG *) &userial_H5_cfg) != -1)
                {
                    retval = 1;
                }

                fd = userial_socket_open();
                if (fd != -1)
                {
                    for (idx = 0; idx < CH_MAX; idx++)
                    {
                        (*fd_array)[idx] = fd;
                    }
                }
                else
                {
                    retval = 0;
                }

                /* retval contains numbers of open fd of HCI channels */
            }
            else if ((rtkbt_transtype & RTKBT_TRANS_UART) && ((rtkbt_transtype & RTKBT_TRANS_H4) ||
                                                              (rtkbt_transtype & RTKBT_TRANS_H45)))
            {
                int (*fd_array)[] = (int (*)[]) param;
                int fd, idx;
                if (userial_vendor_open((tUSERIAL_CFG *) &userial_H4_cfg) != -1)
                {
                    retval = 1;
                }
                fd = userial_socket_open();
                if (fd != -1)
                {
                    for (idx = 0; idx < CH_MAX; idx++)
                    {
                        (*fd_array)[idx] = fd;
                    }
                }
                else
                {
                    retval = 0;
                }
                /* retval contains numbers of open fd of HCI channels */
            }
            else
            {
                BTVNDDBG("USB op for %d", opcode);
                int fd, idx = 0;
                int (*fd_array)[] = (int (*)[]) param;
                for (idx = 0; idx < 10; idx++)
                {
                    if (userial_vendor_usb_open() != -1)
                    {
                        retval = 1;
                        break;
                    }
                }
                fd = userial_socket_open();
                if (fd != -1)
                {
                    for (idx = 0; idx < CH_MAX; idx++)
                    {
                        (*fd_array)[idx] = fd;
                    }
                }
                else
                {
                    retval = 0;
                }

            }
        }
        break;

    case BT_VND_OP_USERIAL_CLOSE:
        {
            userial_vendor_close();
        }
        break;

    case BT_VND_OP_GET_LPM_IDLE_TIMEOUT:
        {

        }
        break;

    case BT_VND_OP_LPM_SET_MODE:
        {
            bt_vendor_lpm_mode_t mode = *(bt_vendor_lpm_mode_t *) param;
            //for now if the mode is BT_VND_LPM_DISABLE, we guess the hareware bt
            //interface is closing, we shall not send any cmd to the interface.
            if (mode == BT_VND_LPM_DISABLE)
            {
                userial_set_bt_interface_state(0);
            }
        }
        break;

    case BT_VND_OP_LPM_WAKE_SET_STATE:
        {

        }
        break;
    case BT_VND_OP_EPILOG:
        {
            if (rtkbt_transtype & RTKBT_TRANS_USB)
            {
                if (bt_vendor_cbacks)
                {
                    bt_vendor_cbacks->epilog_cb(BT_VND_OP_RESULT_SUCCESS);
                }
            }
            else
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
        }
        break;

    default:
        break;
    }

    return retval;
}

/** Closes the interface */
static void cleanup(void)
{
    BTVNDDBG("cleanup");

#if 0
    if (rtkbt_transtype & RTKBT_TRANS_UART)
    {
        upio_cleanup();
        bt_wake_up_host_mode_set(0);
    }
#endif

    bt_vendor_cbacks = NULL;

    if (rtk_btsnoop_dump)
    {
        rtk_btsnoop_close();
    }
    if (rtk_btsnoop_net_dump)
    {
        rtk_btsnoop_net_close();
    }
    if (rtkbt_capture_fw_log)
    {
        hci_close_firmware_log_file(hci_firmware_log_fd);
    }
    if (epoll_ctl(poll_dl_fd, EPOLL_CTL_DEL, event_dl_fd, NULL) == -1)
    {
        ALOGE("%s unable to unregister fd %d from epoll set: %s", __func__, event_dl_fd, strerror(errno));
    }
    if (poll_dl_fd > 0)
    {
        close(poll_dl_fd);
    }
    rtkbt_stack_conf_cleanup();
    rtkbt_wakeup_cfg_clean();
}

// Entry point of DLib
const bt_vendor_interface_t BLUETOOTH_VENDOR_LIB_INTERFACE =
{
    sizeof(bt_vendor_interface_t),
    init,
    op,
    cleanup
};
