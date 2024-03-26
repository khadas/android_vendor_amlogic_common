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

#define LOG_TAG "Multi_BT"
#include <cutils/properties.h>
#include <cutils/android_filesystem_config.h>

#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <asm/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <dirent.h>

/* Include c++ header file */
#include <iostream>
#include <string>

#include "multibt_hal.h"

/******************************************************************************
**  Constants & Macros
******************************************************************************/
#ifdef MAILBOX_MODULE_NAME
#define MBOX_USER_MAX_LEN   96
#define PATH_MAX_LEN        64
#define ARMV8_TO_AOCPU      "/dev/ree2aocpu"
#define CMD_SET_MID         0xFA
#endif

#define VND_PORT_NAME_MAXLEN    256

#define ENUM_DIR_PCI "/sys/bus/pci/devices"
#define ENUM_DIR_SDIO "/sys/bus/sdio/devices"
#define ENUM_DIR_USB "/sys/bus/usb/devices"

#define AML_BT_RFKILL_PATH "/sys/devices/platform/aml_bt/rfkill"
#define WIFI_POWER_DEV "/dev/wifi_power"
#define DEFAULT_VND_LIB_NAME "libbt-vendor.so"

#define POWER_EVENT_DEF '0'
#define POWER_EVENT_RESET '1'
#define POWER_EVENT_EN '2'

/******************************************************************************
**  Local type definitions
******************************************************************************/
/* serial control block */
typedef struct
{
    int fd;                     /* fd to Bluetooth device */
    struct termios termios;     /* serial terminal of BT port */
    char port_name[VND_PORT_NAME_MAXLEN];
} uart_cb;

typedef struct {
    unsigned int vid;
    unsigned int pid;
} dev_id;

typedef struct {
    dev_id mod_id;
    char dev_name[PROP_VALUE_MAX];
    char vnd_lib_name[PROP_VALUE_MAX];
    char mod_name[PROP_VALUE_MAX];
    char power_type;
} dev_info;

typedef struct {
    unsigned int vid;
    char dev_name[PROP_VALUE_MAX];
    char vnd_lib_name[PROP_VALUE_MAX];
    char power_type;
} dev_info_uart;

static int set_debug_level(const char *p_name, char *p_value);
static int set_redistinguish(const char *p_name, char *p_value);
static void clr_bt_power_bit(char power_type);

static int insmod(const char *filename, const char *args);
static int rmmod(const char *modname);
static bool set_bt_cfg(void);
static bool get_bt_cfg(void);
static prop_val *get_bt_prop_val(void);
static bool distinguish_bt_module(void);

/******************************************************************************
**  Static variables
******************************************************************************/
static uart_cb bt_uart_cb;
static int rfkill_id = -1;
static char *rfkill_state_path = NULL;
static int VDBG = 0;
static int redistinguish = 0; //default onboot dou't distinguish
static int distinguish = 0; //distinguish Corresponding module set 1
static bool pci_flag = false;

static prop_val bt_prop_val = {
    .dev_name = {'\0'},
    .mod_name = {'\0'},
    .vnd_lib_name = {'\0'},
    .wifi_bt_name = {'\0'},
};

static const uart_cfg uart_cfg_h5 =
{
    (USERIAL_DATABITS_8 | USERIAL_PARITY_EVEN | USERIAL_STOPBITS_1),
    USERIAL_BAUD_115200,
};

static const uart_cfg uart_cfg_h4 =
{
    (USERIAL_DATABITS_8 | USERIAL_PARITY_NONE | USERIAL_STOPBITS_1),
    USERIAL_BAUD_115200,
};

static const tag_table entry_table[] = {
    {"debuglevel", set_debug_level},
    {"redistinguish", set_redistinguish}
};

const vendor_hal bt_vendor_hal = {
    insmod,
    rmmod,
    set_bt_cfg,
    get_bt_cfg,
    get_bt_prop_val,
    distinguish_bt_module,
};

/******************************************************************************
**  init variables
******************************************************************************/
static uint8_t vendor_info[] =     {0x01, 0x01, 0x10, 0x00};
static uint8_t vendor_reset[] =    {0x01, 0x03, 0x0c, 0x00};
static uint8_t vendor_sync[] =     {0xc0, 0x00, 0x2f, 0x00, 0xd0, 0x01, 0x7e, 0xc0}; //{0x01, 0x7E}
//static uint8_t vendor_sync_rsp[] = {0xc0,0x00,0x2f,0x00,0xd0,0x02,0x7d,0xc0}; //{0x02, 0x7D}

/******************************************************************************
**  Bt device info of pci interface
******************************************************************************/
static const dev_info bt_dev_pci[] = {
    // qualcomm pice modules
    {{0x0271, 0x1101}, "qca6391",      QTI_VND_LIB,   "",                POWER_EVENT_RESET},
    {{0x0271, 0x1103}, "qca206x",      QTI_VND_LIB,   "",                POWER_EVENT_RESET},
    // mediatek pice modules
    {{0x14c3, 0x7961}, "mtk7920e",     MT792_VND_LIB, "btmtkuart",       POWER_EVENT_RESET},
    // amlogic pice modules
    {{0x1F35, 0x0602}, "aml_w2_p",     AML_VND_LIB,   "",                POWER_EVENT_RESET},
    {{0x1F35, 0x0642}, "aml_w2_p",     AML_VND_LIB,   "",                POWER_EVENT_RESET},
    // nxp pice modules
    {{0x02DF, 0x2b56}, "nxpiw620",     NXP_VND_LIB,   "",                POWER_EVENT_RESET},
};

/******************************************************************************
**  Bt device info of sdio interface
******************************************************************************/
static const dev_info bt_dev_sdio[] = {
    // broadcom sdio modules
    {{0x02D0, 0x4359}, "ap6398s",      BCM_VND_LIB,   "",                POWER_EVENT_RESET},
    {{0x02D0, 0xaaec}, "ap6276s",      BCM_VND_LIB,   "",                POWER_EVENT_RESET},
    // realtek sdio modules
    {{0x024C, 0xC822}, "rtl8822cs",    RTK_VND_LIB,   "",                POWER_EVENT_RESET},
    // mediatek sdio modules
    {{0x0e8d, 0x7608}, "mtk7668s",     MTK_VND_LIB,   "btmtksdio",       POWER_EVENT_EN},
    {{0x0e8d, 0x7603}, "mtk7661s",     MTK_VND_LIB,   "btmtksdio",       POWER_EVENT_EN},
    // amlogic sdio modules
    {{0x8888, 0x8888}, "aml_w1",       AML_VND_LIB,   "",                POWER_EVENT_EN},
    {{0x1B8E, 0x0500}, "aml_w1u_s",    AML_VND_LIB,   "",                POWER_EVENT_DEF},
    {{0x1B8E, 0x0540}, "aml_w1u_s",    AML_VND_LIB,   "",                POWER_EVENT_DEF},
    {{0x1B8E, 0x0600}, "aml_w2_s",     AML_VND_LIB,   "",                POWER_EVENT_DEF},
    {{0x1B8E, 0x0640}, "aml_w2_s",     AML_VND_LIB,   "",                POWER_EVENT_DEF},
    {{0x1B8E, 0x0800}, "aml_w2l_s",    AML_VND_LIB,   "",                POWER_EVENT_DEF},
    {{0x1B8E, 0x0810}, "aml_w2l_s",    AML_VND_LIB,   "",                POWER_EVENT_DEF},
    {{0x1B8E, 0x0808}, "aml_w2l_s",    AML_VND_LIB,   "",                POWER_EVENT_DEF},
    // nxp sdio modules
    {{0x02DF, 0x9149}, "nxp8987",      NXP_VND_LIB,   "",                POWER_EVENT_RESET},
    {{0x02DF, 0x9141}, "nxp8997",      NXP_VND_LIB,   "",                POWER_EVENT_RESET},
    // unisoc sdio modules
    {{0x0000, 0x0000}, "uwe5621ds",    UWE_VND_LIB,   "sprdbt_tty",      POWER_EVENT_EN},
};

/******************************************************************************
**  Bt device info of usb interface
******************************************************************************/
static const dev_info bt_dev_usb[] = {
    // broadcom usb modules
    {{0x02D0, 0x2045}, "ap62x8",       BCM_VND_LIB,   "btusb",           POWER_EVENT_RESET},
    {{0x02D0, 0xBD27}, "ap62x8",       BCM_VND_LIB,   "btusb",           POWER_EVENT_RESET},
    {{0x02D0, 0x0BDC}, "ap62x8",       BCM_VND_LIB,   "btusb",           POWER_EVENT_RESET},
    // qualcomm usb modules
    {{0x0CF3, 0x9378}, "qca9379",      QCA_VND_LIB,   "bt_usb_qcom",     POWER_EVENT_EN},
    {{0x0CF3, 0x7A85}, "qca9379",      QCA_VND_LIB,   "bt_usb_qcom",     POWER_EVENT_EN},
    // realtek usb modules
    {{0x0bda, 0xC820}, "rtl8821cu",    RTK_VND_LIB,   "rtk_btusb",       POWER_EVENT_EN},
    {{0x0bda, 0xC811}, "rtl8821cu",    RTK_VND_LIB,   "rtk_btusb",       POWER_EVENT_EN},
    {{0x0bda, 0xD723}, "rtl8723du",    RTK_VND_LIB,   "rtk_btusb",       POWER_EVENT_EN},
    {{0x0bda, 0xB82C}, "rtl8822bu",    RTK_VND_LIB,   "rtk_btusb",       POWER_EVENT_EN},
    {{0x0bda, 0xB720}, "rtl8723bu",    RTK_VND_LIB,   "rtk_btusb",       POWER_EVENT_EN},
    {{0x0bda, 0x0823}, "rtl8821au",    RTK_VND_LIB,   "rtk_btusb",       POWER_EVENT_EN},
    {{0x0bda, 0x0821}, "rtl8821au",    RTK_VND_LIB,   "rtk_btusb",       POWER_EVENT_EN},
    {{0x0bda, 0x885c}, "rtl8852au",    RTK_VND_LIB,   "rtk_btusb",       POWER_EVENT_EN},
    {{0x0bda, 0x885a}, "rtl8852au",    RTK_VND_LIB,   "rtk_btusb",       POWER_EVENT_EN},
    {{0x0bda, 0xa85b}, "rtl8852bu",    RTK_VND_LIB,   "rtk_btusb",       POWER_EVENT_EN},
    {{0x0bda, 0xB733}, "rtl8733bu",    RTK_VND_LIB,   "rtk_btusb",       POWER_EVENT_EN},
    {{0x0bda, 0xC82C}, "rtl88x2cu",    RTK_VND_LIB,   "rtk_btusb",       POWER_EVENT_EN},
    {{0x0bda, 0xB761}, "rtl8761u",     RTK_VND_LIB,   "rtk_btusb",       POWER_EVENT_RESET},
    {{0x0bda, 0x8771}, "rtl8771u",     RTK_VND_LIB,   "rtk_btusb",       POWER_EVENT_RESET},
    // mediatek usb modules
    {{0x0e8d, 0x7668}, "mtk7668u",     MTK_VND_LIB,   "btmtk_usb",       POWER_EVENT_EN},
    {{0x0e8d, 0x7961}, "mtk7920u",     MT792_VND_LIB, "btmtk_usb_unify", POWER_EVENT_RESET},
    // amlogic usb modules
    {{0x1B8E, 0x4C55}, "aml_w1u",      AML_VND_LIB,   "",                POWER_EVENT_EN},
    {{0x1B8E, 0x0541}, "aml_w1u",      AML_VND_LIB,   "",                POWER_EVENT_EN},
    {{0x1B8E, 0x0601}, "aml_w2_u",     AML_VND_LIB,   "",                POWER_EVENT_EN},
    {{0x1B8E, 0x0641}, "aml_w2_u",     AML_VND_LIB,   "",                POWER_EVENT_EN},
    {{0x1B8E, 0x0801}, "aml_w2l_u",    AML_VND_LIB,   "",                POWER_EVENT_EN},
    {{0x1B8E, 0x0809}, "aml_w2l_u",    AML_VND_LIB,   "",                POWER_EVENT_EN},
    {{0x1B8E, 0x0811}, "aml_w2l_u",    AML_VND_LIB,   "",                POWER_EVENT_EN},
};

/******************************************************************************
**  Bt device info of uart interface
******************************************************************************/
static const dev_info_uart bt_dev_uart[] = {
    {BT_VID_BROADCOM, "bcm_bt", BCM_VND_LIB, POWER_EVENT_RESET},
    {BT_VID_QUALCOMM, "qca_bt", QCA_VND_LIB, POWER_EVENT_RESET},
    {BT_VID_REALTECK, "rtl_bt", RTK_VND_LIB, POWER_EVENT_RESET},
    {BT_VID_MEDIATEK, "mtk_bt", MTK_VND_LIB, POWER_EVENT_RESET},
    {BT_VID_AMLOGIC,  "aml_bt", AML_VND_LIB, POWER_EVENT_RESET},
    {BT_VID_UNISOC,   "uwe_bt", UWE_VND_LIB, POWER_EVENT_RESET},
};

int mailbox_qca_bt_name(void)
{
#ifdef MAILBOX_MODULE_NAME
    struct merge_data {
        int cmd;
        char msg[MBOX_USER_MAX_LEN];
    } merge_data;
    int fd = -1;
    int ret = -1;
    char path[PATH_MAX_LEN] = {'\0'};
    char bt_name[] = {"qca_bt"};

    if (strcmp(bt_prop_val.dev_name, bt_name)) {
        PR_INFO("bt_name:%s, not qca_bt", bt_prop_val.dev_name);
        goto exit;
    }

    sprintf(path, "%s", ARMV8_TO_AOCPU);
    PR_DBG("open %s\n", path);

    fd = open(path, O_RDWR);
    if (fd < 0) {
        PR_ERR("open %s failed: %s (%d)", ARMV8_TO_AOCPU, strerror(errno), errno);
        goto exit;
    }

    merge_data.cmd = CMD_SET_MID;
    memcpy(merge_data.msg, bt_name, strlen(bt_name));
    ret = write(fd, &merge_data, sizeof(merge_data));
    if (ret < 0) {
        PR_ERR("write failed: %s (%d)", strerror(errno), errno);
        goto exit;
    }
    memset(bt_name, 0, strlen(bt_name));
    ret = read(fd, bt_name, strlen(bt_name));
    if (ret < 0) {
        PR_ERR("read failed: %s (%d)", strerror(errno), errno);
        goto exit;
    }

exit:
    if (fd >= 0) {
        close(fd);
    }

    return ret;
#else
    return -1;
#endif
}

bool get_pci_flag(void)
{
    return pci_flag;
}

void set_pci_flag(bool val)
{
    pci_flag = val;
}


static void set_bt_prop(const char *dev_name, const char *mod_name, const char *vnd_lib_name)
{
    if ((dev_name == NULL) ||(mod_name == NULL) || (vnd_lib_name == NULL)) {
        PR_ERR("parameters err");
        return;
    }

    memcpy(bt_prop_val.dev_name, dev_name, (sizeof(bt_prop_val.dev_name) - 1));
    property_set(PROP_BT_NAME, bt_prop_val.dev_name);
    memcpy(bt_prop_val.mod_name, mod_name, (sizeof(bt_prop_val.mod_name) - 1));
    property_set(PROP_BT_MODULE, bt_prop_val.mod_name);
    memcpy(bt_prop_val.vnd_lib_name, vnd_lib_name, (sizeof(bt_prop_val.vnd_lib_name) - 1));
    property_set(PROP_LIBBT_VENDOR, bt_prop_val.vnd_lib_name);
}

static void get_bt_prop(void)
{
    property_get(PROP_BT_NAME, bt_prop_val.dev_name, NULL);
    property_get(PROP_BT_MODULE, bt_prop_val.mod_name, NULL);
    property_get(PROP_LIBBT_VENDOR, bt_prop_val.vnd_lib_name, DEFAULT_VND_LIB_NAME);

    property_get(PROP_WIFI_BT_NAME, bt_prop_val.wifi_bt_name, NULL);

    PR_INFO("bt_prop_val dev_name:%s, mod_name:%s, vnd_lib_name:%s, wifi_bt_name:%s,",
        bt_prop_val.dev_name, bt_prop_val.mod_name, bt_prop_val.vnd_lib_name, bt_prop_val.wifi_bt_name);
}

static prop_val *get_bt_prop_val(void)
{
    return &bt_prop_val;
}

static bool matching_dev_id(const dev_info *dev, unsigned int dev_size, const dev_id *mod_id)
{
    unsigned int cnt;
    bool ret =false;

    if ((dev == NULL) || (mod_id == NULL)) {
        PR_ERR("parameters err");
        return ret;
    }

    for (cnt = 0; cnt < dev_size; cnt++) {
        if ((dev[cnt].mod_id.vid == mod_id->vid) && (dev[cnt].mod_id.pid == mod_id->pid)) {
            PR_INFO("matched vid:%4x, pid:%04x, dev_name:%s, cnt:%u, set property", dev[cnt].mod_id.vid,
                dev[cnt].mod_id.pid, dev[cnt].dev_name, cnt);
            set_bt_prop(dev[cnt].dev_name, dev[cnt].mod_name, dev[cnt].vnd_lib_name);
            ret = true;
            break;
        }
    }

    return ret;
}

static bool matching_dev_id_uart(unsigned int vid)
{
    unsigned int cnt;
    bool ret = false;

    for (cnt = 0; cnt < (sizeof(bt_dev_uart) / sizeof(dev_info_uart)); cnt++) {
        if (bt_dev_uart[cnt].vid == vid) {
            PR_INFO("matched vid:%4x, dev_name:%s, cnt:%u, set property",bt_dev_uart[cnt].vid,
                bt_dev_uart[cnt].dev_name, cnt);
            memcpy(bt_prop_val.dev_name, bt_dev_uart[cnt].dev_name, (sizeof(bt_prop_val.dev_name) - 1));
            property_set(PROP_BT_NAME, bt_prop_val.dev_name);
            memcpy(bt_prop_val.vnd_lib_name, bt_dev_uart[cnt].vnd_lib_name,
                (sizeof(bt_prop_val.vnd_lib_name) - 1));
            property_set(PROP_LIBBT_VENDOR, bt_prop_val.vnd_lib_name);
            ret = true;
            break;
        }
    }

    return ret;
}

static int matching_dev_name(const dev_info *dev, unsigned int dev_size, const char *val)
{
    int ret = -1;
    int cnt;

    if ((dev == NULL) || (val == NULL)) {
        PR_ERR("parameters err");
        return ret;
    }

    for (cnt = 0; cnt < dev_size; cnt++) {
        if (!strcmp(dev[cnt].dev_name, val)) {
            PR_INFO("matched dev_name:%s, cnt:%d", dev[cnt].dev_name, cnt);
            ret = cnt;
            break;
        }
    }

    return ret;
}

static bool get_aml_bt_module(char *str)
{
    int fd;
    bool ret = false;

    fd = open(WIFI_POWER_DEV, O_RDWR);
    if (fd < 0) {
        PR_ERR("open (%s) failed: %s (%d)", WIFI_POWER_DEV, strerror(errno), errno);
        goto exit;
    }

    if (ioctl (fd, GET_AML_WIFI_MODULE, str) < 0) {
        PR_ERR("ioctl GET_AML_WIFI_MODULE failed: %s (%d)", strerror(errno), errno);
        goto exit;
    }

    PR_DBG("get bt module: %s", str);
    if (!strncmp(str, "aml" ,3)) {
        PR_INFO("get aml bt module: %s", str);
        ret = true;
    }

exit:
    if (fd >= 0) {
        close(fd);
    }

    return ret;
}

static bool distinguish_dev_name_specify(void)
{
    int idx;

    if (strlen(bt_prop_val.wifi_bt_name)) {
        idx = matching_dev_name(bt_dev_pci, (sizeof(bt_dev_pci) / sizeof(dev_info)), bt_prop_val.wifi_bt_name);
        if (idx >= 0) {
            set_bt_prop(bt_dev_pci[idx].dev_name, bt_dev_pci[idx].mod_name, bt_dev_pci[idx].vnd_lib_name);
            return true;
        }

        idx = matching_dev_name(bt_dev_sdio, (sizeof(bt_dev_sdio) / sizeof(dev_info)), bt_prop_val.wifi_bt_name);
        if (idx >= 0) {
            set_bt_prop(bt_dev_sdio[idx].dev_name, bt_dev_sdio[idx].mod_name, bt_dev_sdio[idx].vnd_lib_name);
            return true;
        }

        idx = matching_dev_name(bt_dev_usb, (sizeof(bt_dev_usb) / sizeof(dev_info)), bt_prop_val.wifi_bt_name);
        if (idx >= 0) {
            set_bt_prop(bt_dev_usb[idx].dev_name, bt_dev_usb[idx].mod_name, bt_dev_usb[idx].vnd_lib_name);
            return true;
        }
    }

    return false;
}

static bool write_power_type(const char *power_type, unsigned int len)
{
    int fd;
    bool ret = false;

    if (access(BT_POWER_EVT_1, F_OK) == 0) {
        fd = open(BT_POWER_EVT_1, O_WRONLY);
    } else {
        fd = open(BT_POWER_EVT_2, O_WRONLY);
    }

    if (fd < 0) {
        PR_ERR("open btpower_evt failed: %s (%d)", strerror(errno), errno);
        goto exit;
    } else {
        if (write(fd, power_type, len) < len) {
            PR_ERR("write btpower_evt failed: %s (%d)", strerror(errno), errno);
        } else {
            ret = true;
        }
    }

exit:
    if (fd >= 0) {
        close(fd);
    }

    return ret;
}

static bool get_power_type(char *power_type)
{
    int idx = -1;

    if (power_type == NULL) {
        PR_ERR("parameters err");
        return false;
    }

    if (strlen(bt_prop_val.dev_name)) {
        idx = matching_dev_name(bt_dev_pci, (sizeof(bt_dev_pci) / sizeof(dev_info)), bt_prop_val.dev_name);
        if (idx >= 0) {
            *power_type = bt_dev_pci[idx].power_type;
            set_pci_flag(true);
            goto exit;
        }

        idx = matching_dev_name(bt_dev_sdio, (sizeof(bt_dev_sdio) / sizeof(dev_info)), bt_prop_val.dev_name);
        if (idx >= 0) {
            *power_type = bt_dev_sdio[idx].power_type;
            goto exit;
        }

        idx = matching_dev_name(bt_dev_usb, (sizeof(bt_dev_usb) / sizeof(dev_info)), bt_prop_val.dev_name);
        if (idx >= 0) {
            *power_type = bt_dev_usb[idx].power_type;
            goto exit;
        }

        for (idx = 0; idx < ( sizeof(bt_dev_uart) / sizeof(dev_info_uart)); idx++) {
            if (!strcmp(bt_dev_uart[idx].dev_name, bt_prop_val.dev_name)) {
                *power_type = bt_dev_uart[idx].power_type;
                goto exit;
            }
        }
    }

    return false;

exit:
    PR_DBG("matched dev_name:%s, idx:%d, power_type:%c", bt_prop_val.dev_name, idx, *power_type);
    return true;
}

static bool set_power_type(void)
{
    char power_type = '\0';

    if (!get_power_type(&power_type)) {
        PR_INFO("get power type failed");
        return false;
    }

    if (!write_power_type(&power_type, sizeof(char))) {
        PR_INFO("write power type failed");
        return false;
    }

    clr_bt_power_bit(power_type);

    return true;
}

static int set_debug_level(const char *p_name, char *p_value)
{
    VDBG = std::strtol(p_value, nullptr, 10);
    if (VDBG) {
        PR_INFO("%s = %d", p_name, VDBG);
    }
    return 0;
}

static int set_redistinguish(const char *p_name, char *p_value)
{
    redistinguish = std::strtol(p_value, nullptr, 10);
    if (VDBG) {
        PR_INFO("%s = %d", p_name, redistinguish);
    }
    return 0;
}

static int insmod(const char *filename, const char *args)
{
    int ret = -1;
    int fd;

    if ((filename == NULL) || (args == NULL)) {
        PR_ERR("parameters err");
        return ret;
    }

    fd = TEMP_FAILURE_RETRY(open(filename, O_RDONLY | O_CLOEXEC | O_NOFOLLOW));
    if (fd < 0) {
        PR_ERR("Failed to open %s", filename);
        return -1;
    }

    ret = syscall(__NR_finit_module, fd, args, 0);

    close(fd);
    if (ret < 0) {
        PR_ERR("finit_module return: %d", ret);
    }

    return ret;
}

static int rmmod(const char *modname)
{
    int ret = -1;
    int maxtry = 10;

    if (modname == NULL) {
        PR_ERR("parameters err");
        return ret;
    }

    while (maxtry-- > 0) {
        ret = delete_module(modname, O_NONBLOCK | O_EXCL);
        if ((ret < 0) && (errno == EAGAIN)) {
            usleep(500000);
        } else {
            break;
        }
    }

    if (ret != 0) {
        PR_ERR("Unable to unload driver module %s",modname);
    }

    return ret;
}

static void rmmod_aml_drv(void)
{
    char mod_name[20] = {'\0'};

    if (get_aml_bt_module(mod_name)) {
        PR_INFO("aml modules need rmmod wifi_comm");
        if (!rmmod("wifi_comm")) {
            usleep(100000);
        }
    }
}

static bool find_target_file(const char *path, const char * file)
{
    DIR *dir;
    struct dirent *next;
    bool ret = false;

    if ((path == NULL) || (file == NULL)) {
         PR_ERR("parameters err");
         return ret;
    }

    dir = opendir(path);
    if (dir == NULL) {
       PR_ERR("opendir (%s) failed: %s (%d)", path, strerror(errno), errno);
       goto exit;
    }

    while ((next = readdir(dir)) != NULL) {
        if (!strncmp(next->d_name, file, strlen(file))) {
            ret = true;
            goto exit;
        }
    }

exit:
    if (dir) {
        closedir(dir);
    }

    return ret;
}

static int get_config_param(std::string path, std::string file)
{
    FILE *fp;
    std::string target;
    tag_table *temp_table;
    int ret = -1;
    char line[MAX_LINE_LEN] = {'\0'};
    char *name,*value;

    if (!find_target_file(path.c_str(), file.c_str())) {
        PR_ERR("debug config file not exist");
        return ret;
    }

    target = std::string(path) + std::string(file);
    PR_DBG("open target: %s", target.c_str());
    fp = fopen(target.c_str(), "r");
    if (fp == NULL) {
        PR_ERR("open %s failed: %s (%d)", target.c_str(), strerror(errno), errno);
        goto exit;
    }

    while(fgets(line, MAX_LINE_LEN, fp)) {
        if (line[0] == '#') {
            continue;
        }

        name = strtok(line, DELIM);
        if (name == NULL) {
            continue;
        }

        value = strtok(NULL, DELIM);
        temp_table = (tag_table *)entry_table;

        while (temp_table->tag) {
            if (!strcmp(temp_table->tag, name)) {
                temp_table->tag_ops(temp_table->tag, value);
                break;
            }

            temp_table++;
        }
    }

    ret = redistinguish;

exit:
    if (fp) {
        fclose(fp);
        fp = NULL;
    }

    return ret;
}

static int get_redistinguish(void)
{
/*
    power on or reboot later:    distinguish = 0;
    distinguish BT successfully: distinguish = 1;
*/

    get_config_param(CONFIG_PATH, CONFIG_NAME);
    if (redistinguish && !distinguish) {
        property_set("persist.vendor.libbt_vendor", "re_libbt");
        PR_INFO();
    }

    return 0;
}

static bool set_bt_cfg(void)
{
    bool ret;

    mailbox_qca_bt_name();
    rmmod_aml_drv();
    ret = set_power_type();

    return ret;
}

static bool get_bt_cfg(void)
{
    get_redistinguish();
    get_bt_prop();

    return true;
}

static void clr_bt_power_bit(char power_type)
{
    int fd;

    if (get_pci_flag()) {
        PR_INFO("pcie needn't clr bt power bit");
        return;
    }

    if (!strcmp(bt_prop_val.dev_name, "aml_w2_s")) {
        PR_INFO("aml w2_s needn't clr bt power bit");
        return;
    }

    if (power_type == POWER_EVENT_RESET) {
        PR_INFO("power_type:(%c) on separately", power_type);
    } else {
        PR_INFO("power_type:(%c) not on separately",  power_type);
        return;
    }

    fd = open(WIFI_POWER_DEV, O_RDWR);
    if (fd < 0) {
        PR_ERR("open (%s) failed: %s (%d)", WIFI_POWER_DEV, strerror(errno), errno);
        return;
    }

    if (ioctl(fd, CLR_BT_POWER_BIT) < 0) {
        PR_ERR("ioctl CLR_BT_POWER_BIT failed: %s (%d)", strerror(errno), errno);
    }

    if (fd >= 0) {
        close(fd);
    }

    return;
}

static bool distinguish_bt_module_pci(void)
{
    DIR *dir;
    struct dirent *next;
    FILE *fp = NULL;
    bool ret = false;

    dir = opendir(ENUM_DIR_PCI);
    if (!dir) {
        PR_ERR("opendir (%s) failed: %s (%d)", ENUM_DIR_PCI, strerror(errno), errno);
        goto exit;
    }

    while ((next = readdir(dir)) != NULL) {
        char line[256] = {'\0'};
        char uevent_file[512] = {'\0'};

        /* Read pci uevent file, uevent's data like below:
         * DRIVER=w2_comm
         * PCI_CLASS=78000
         * PCI_ID=1F35:0602
         * PCI_SUBSYS_ID=1556:1111
         * PCI_SLOT_NAME=0000:01:00.0
         * MODALIAS=pci:v00001F35d00000602sv00001556sd00001111bc07sc80i00
         */
        sprintf(uevent_file, "%s/%s/uevent", ENUM_DIR_PCI, next->d_name);
        fp = fopen(uevent_file, "r");
        if (fp == NULL) {
            continue;
        }

        while (fgets(line, sizeof(line), fp)) {
            dev_id mod_id = {0, 0};
            char *pos = NULL;

            pos = strstr(line, "PCI_ID=");
            if (!pos) {
                continue;
            }

            if (sscanf(pos + 7, "%x:%x", &(mod_id.vid), &(mod_id.pid)) <= 0) {
                continue;
            }

            PR_DBG("list vid:0x%04x, pid:0x%04x", mod_id.vid, mod_id.pid);
            ret = matching_dev_id(bt_dev_pci, (sizeof(bt_dev_pci) / sizeof(dev_info)), &mod_id);
            if (ret) {
                goto exit;
            }
        }

        if (fp) {
            fclose(fp);
            fp = NULL;
        }
    }

exit:
    if (fp) {
        fclose(fp);
        fp = NULL;
    }

    if (dir) {
        closedir(dir);
    }

    return ret;
}

static bool distinguish_bt_module_sdio(void)
{
    DIR *dir;
    struct dirent *next;
    FILE *fp = NULL;
    bool ret = false;

    dir = opendir(ENUM_DIR_SDIO);
    if (!dir) {
        PR_ERR("opendir (%s) failed: %s (%d)", ENUM_DIR_SDIO, strerror(errno), errno);
        goto exit;
    }

    while ((next = readdir(dir)) != NULL) {
        char line[256] = {'\0'};
        char uevent_file[512] = {'\0'};

        /* Read sdio uevent file, uevent's data like below:
         * DRIVER=aml_sdio
         * OF_NAME=wifi
         * OF_FULLNAME=/soc/sdio@fe088000/wifi@1
         * OF_COMPATIBLE_0=brcm,bcm4329-fmac
         * OF_COMPATIBLE_N=1
         * SDIO_CLASS=07
         * SDIO_ID=1B8E:0540
         * MODALIAS=sdio:c07v1B8Ed0540
         */
        sprintf(uevent_file, "%s/%s/uevent", ENUM_DIR_SDIO, next->d_name);
        fp = fopen(uevent_file, "r");
        if (fp == NULL) {
            continue;
        }

        while (fgets(line, sizeof(line), fp)) {
            dev_id mod_id = {0, 0};
            char *pos = NULL;

            pos = strstr(line, "SDIO_ID=");
            if (!pos) {
                continue;
            }

            if (sscanf(pos + 8, "%x:%x", &(mod_id.vid), &(mod_id.pid)) <= 0) {
                continue;
            }

            PR_DBG("list vid:0x%04x, pid:0x%04x", mod_id.vid, mod_id.pid);
            ret = matching_dev_id(bt_dev_sdio, (sizeof(bt_dev_sdio) / sizeof(dev_info)), &mod_id);
            if (ret) {
                goto exit;
            }
        }

        if (fp) {
            fclose(fp);
            fp = NULL;
        }
    }

exit:
    if (fp) {
        fclose(fp);
        fp = NULL;
    }

    if (dir) {
        closedir(dir);
    }

    return ret;
}

static bool distinguish_bt_module_usb(void)
{
    DIR *dir;
    struct dirent *next;
    FILE *fp = NULL;
    bool ret = false;

    dir = opendir(ENUM_DIR_USB);
    if (!dir) {
        PR_ERR("opendir (%s) failed: %s (%d)", ENUM_DIR_USB, strerror(errno), errno);
        goto exit;
    }

    while ((next = readdir(dir)) != NULL) {
        char line[256] = {'\0'};
        char uevent_file[512] = {'\0'};

        /* Read usb uevent file, uevent's data like below:
         * MAJOR=189
         * MINOR=1
         * DEVNAME=bus/usb/001/002
         * DEVTYPE=usb_device
         * DRIVER=usb
         * PRODUCT=1b8e/601/101
         * TYPE=0/0/0
         * BUSNUM=001
         * DEVNUM=002
         */
        sprintf(uevent_file, "%s/%s/uevent", ENUM_DIR_USB, next->d_name);
        fp = fopen(uevent_file, "r");
        if (fp == NULL) {
            continue;
        }

        while (fgets(line, sizeof(line), fp)) {
            unsigned int bcdev;
            dev_id mod_id = {0, 0};
            char *pos = NULL;

            pos = strstr(line, "PRODUCT=");
            if (!pos) {
                continue;
            }

            if (sscanf(pos + 8, "%x/%x/%x", &(mod_id.vid), &(mod_id.pid), &bcdev) <= 0) {
                continue;
            }

            PR_DBG("list vid:0x%04x, pid:0x%04x", mod_id.vid, mod_id.pid);
            ret = matching_dev_id(bt_dev_usb, (sizeof(bt_dev_usb) / sizeof(dev_info)), &mod_id);
            if (ret) {
                goto exit;
            }
        }

        if (fp) {
            fclose(fp);
            fp = NULL;
        }
    }

exit:
    if (fp) {
        fclose(fp);
        fp = NULL;
    }

    if (dir) {
        closedir(dir);
    }

    return ret;
}

static bool is_rfkill_bt_dev(int id)
{
    FILE *fp = NULL;
    char line[256]= {'\0'};
    char uevent_file[256] = {'\0'};
    char *pos = NULL;
    bool ret = false;

    /* Read rfkill uevent file, uevent's data like below:
     * RFKILL_NAME=bt-dev
     * RFKILL_TYPE=bluetooth
     * RFKILL_STATE=0
     * RFKILL_HW_BLOCK_REASON=0x0
     */
    snprintf(uevent_file, sizeof(uevent_file), "/sys/class/rfkill/rfkill%d/uevent", id);
    fp = fopen(uevent_file, "r");
    if (fp == NULL) {
        PR_ERR("open (%s) failed: %s (%d)", uevent_file, strerror(errno), errno);
        goto exit;
    }

    while (fgets(line, sizeof(line), fp)) {
        pos = strstr(line, "RFKILL_NAME=");
        if (!pos) {
            continue;
        }

        if (!strncmp(pos + 12, "bt-dev", 6)) {
            ret = true;
            goto exit;
        }
    }

exit:
    if (fp) {
        fclose(fp);
        fp = NULL;
    }

    return ret;
}

static bool init_rfkill_aml_bt(void)
{
    DIR* dir;
    struct dirent *next;
    int id = -1;
    bool ret = false;

    dir = opendir(AML_BT_RFKILL_PATH);
    if (dir == NULL) {
        PR_ERR("opendir (%s) failed: %s (%d)", AML_BT_RFKILL_PATH, strerror(errno), errno);
        goto exit;
    }

    while ((next = readdir(dir)) != NULL) {
        if (!strncmp(next->d_name, "rfkill", 6)) {
            id = atoi(&(next->d_name[6]));
            if (is_rfkill_bt_dev(id)) {
                rfkill_id = id;
                PR_INFO("rfkill_id: %d", rfkill_id);
                asprintf(&rfkill_state_path, "/sys/class/rfkill/rfkill%d/state", rfkill_id);
                ret = true;
                goto exit;
            }
        }
    }

exit:
    if (dir) {
        closedir(dir);
    }

    return ret;
}

/*******************************************************************************
**
** Function        upio_set_bluetooth_power
**
** Description     Interact with low layer driver to set Bluetooth power
**                 on/off.
**
** Returns         0  : SUCCESS or Not-Applicable
**                 <0 : ERROR
**
*******************************************************************************/
static int upio_set_bluetooth_power(int on)
{
    int fd;
    int len = -1;
    int ret = -1;

    if (rfkill_id == -1) {
        if (!init_rfkill_aml_bt()) {
            PR_ERR("init rfkill failed");
            return ret;
        }
    }

    fd = open(rfkill_state_path, O_WRONLY);
    if (fd < 0) {
        PR_ERR("open (%s) failed: %s (%d)", rfkill_state_path, strerror(errno), errno);
        return ret;
    }

    if (on == UPIO_BT_POWER_OFF) {
        len = write(fd, "0", 1);
    } else if (on == UPIO_BT_POWER_ON) {
        len = write(fd, "1", 1);
    }

    if (len < 1) {
        PR_ERR("write (%s) failed: %s (%d)", rfkill_state_path, strerror(errno), errno);
    } else {
        ret = 0;
    }

    close(fd);

    return ret;
}

/*******************************************************************************
**
** Function        userial_to_tcio_baud
**
** Description     helper function converts USERIAL baud rates into TCIO
**                  conforming baud rates
**
** Returns         TRUE/FALSE
**
*******************************************************************************/
static bool userial_to_tcio_baud(uint8_t cfg_baud, uint32_t *baud)
{
    if (cfg_baud == USERIAL_BAUD_115200)
        *baud = B115200;
    else if (cfg_baud == USERIAL_BAUD_4M)
        *baud = B4000000;
    else if (cfg_baud == USERIAL_BAUD_3M)
        *baud = B3000000;
    else if (cfg_baud == USERIAL_BAUD_2M)
        *baud = B2000000;
    else if (cfg_baud == USERIAL_BAUD_1M)
        *baud = B1000000;
    else if (cfg_baud == USERIAL_BAUD_921600)
        *baud = B921600;
    else if (cfg_baud == USERIAL_BAUD_460800)
        *baud = B460800;
    else if (cfg_baud == USERIAL_BAUD_230400)
        *baud = B230400;
    else if (cfg_baud == USERIAL_BAUD_57600)
        *baud = B57600;
    else if (cfg_baud == USERIAL_BAUD_19200)
        *baud = B19200;
    else if (cfg_baud == USERIAL_BAUD_9600)
        *baud = B9600;
    else if (cfg_baud == USERIAL_BAUD_1200)
        *baud = B1200;
    else if (cfg_baud == USERIAL_BAUD_600)
        *baud = B600;
    else
    {
        PR_ERR( "userial vendor open: unsupported baud idx %i", cfg_baud);
        *baud = B115200;
        return false;
    }

    return true;
}

#if (BT_WAKE_VIA_USERIAL_IOCTL==TRUE)
/*******************************************************************************
**
** Function        userial_ioctl_init_bt_wake
**
** Description     helper function to set the open state of the bt_wake if ioctl
**                  is used. it should not hurt in the rfkill case but it might
**                  be better to compile it out.
**
** Returns         none
**
*******************************************************************************/
static void userial_ioctl_init_bt_wake(int fd)
{
    uint32_t bt_wake_state;

#if (BT_WAKE_USERIAL_LDISC==TRUE)
    int ldisc = N_BRCM_HCI; /* brcm sleep mode support line discipline */

    /* attempt to load enable discipline driver */
    if (ioctl(bt_uart_cb.fd, TIOCSETD, &ldisc) < 0)
    {
        PR_INFO("USERIAL_Open():fd %d, TIOCSETD failed: error %d for ldisc: %d",
                      fd, errno, ldisc);
    }
#endif



    /* assert BT_WAKE through ioctl */
    ioctl(fd, USERIAL_IOCTL_BT_WAKE_ASSERT, NULL);
    ioctl(fd, USERIAL_IOCTL_BT_WAKE_GET_ST, &bt_wake_state);
    PR_INFO("userial_ioctl_init_bt_wake read back BT_WAKE state=%i", \
               bt_wake_state);
}
#endif // (BT_WAKE_VIA_USERIAL_IOCTL==TRUE)


/*****************************************************************************
**   Userial Vendor API Functions
*****************************************************************************/

/*******************************************************************************
**
** Function        userial_vendor_init
**
** Description     Initialize userial vendor-specific control block
**
** Returns         None
**
*******************************************************************************/
static void userial_vendor_init(void)
{
    bt_uart_cb.fd = -1;
    snprintf(bt_uart_cb.port_name, VND_PORT_NAME_MAXLEN, "%s", UART_DEV_PORT_BT);
}

/*******************************************************************************
**
** Function        userial_vendor_open
**
** Description     Open the serial port with the given configuration
**
** Returns         device fd
**
*******************************************************************************/
static int userial_vendor_open(uart_cfg *p_cfg)
{
    uint32_t baud;
    uint8_t data_bits;
    uint16_t parity;
    uint8_t stop_bits;

    bt_uart_cb.fd = -1;

    if (!userial_to_tcio_baud(p_cfg->baud, &baud))
    {
        return -1;
    }

    if(p_cfg->fmt & USERIAL_DATABITS_8)
        data_bits = CS8;
    else if(p_cfg->fmt & USERIAL_DATABITS_7)
        data_bits = CS7;
    else if(p_cfg->fmt & USERIAL_DATABITS_6)
        data_bits = CS6;
    else if(p_cfg->fmt & USERIAL_DATABITS_5)
        data_bits = CS5;
    else
    {
        PR_ERR("userial vendor open: unsupported data bits");
        return -1;
    }

    if(p_cfg->fmt & USERIAL_PARITY_NONE)
        parity = 0;
    else if(p_cfg->fmt & USERIAL_PARITY_EVEN)
        parity = PARENB;
    else if(p_cfg->fmt & USERIAL_PARITY_ODD)
        parity = (PARENB | PARODD);
    else
    {
        PR_ERR("userial vendor open: unsupported parity bit mode");
        return -1;
    }

    if(p_cfg->fmt & USERIAL_STOPBITS_1)
        stop_bits = 0;
    else if(p_cfg->fmt & USERIAL_STOPBITS_2)
        stop_bits = CSTOPB;
    else
    {
        PR_ERR("userial vendor open: unsupported stop bits");
        return -1;
    }

    PR_INFO("userial vendor open: opening %s", bt_uart_cb.port_name);

    if ((bt_uart_cb.fd = open(bt_uart_cb.port_name, O_RDWR)) < 0)
    {
        PR_ERR("userial vendor open: unable to open %s", bt_uart_cb.port_name);
        return -1;
    }

    PR_INFO("userial vendor open success!!");

    tcflush(bt_uart_cb.fd, TCIOFLUSH);

    tcgetattr(bt_uart_cb.fd, &bt_uart_cb.termios);
    cfmakeraw(&bt_uart_cb.termios);

    /* Set UART Control Modes */
    bt_uart_cb.termios.c_cflag |= CLOCAL;
    bt_uart_cb.termios.c_cflag |= (CRTSCTS | stop_bits| parity);


    tcsetattr(bt_uart_cb.fd, TCSANOW, &bt_uart_cb.termios);

    /* set input/output baudrate */
    cfsetospeed(&bt_uart_cb.termios, baud);
    cfsetispeed(&bt_uart_cb.termios, baud);
    tcsetattr(bt_uart_cb.fd, TCSANOW, &bt_uart_cb.termios);

#if (BT_WAKE_VIA_USERIAL_IOCTL==TRUE)
    userial_ioctl_init_bt_wake(bt_uart_cb.fd);
#endif
    tcflush(bt_uart_cb.fd, TCIOFLUSH);


    PR_INFO("device fd = %d open", bt_uart_cb.fd);

    return bt_uart_cb.fd;
}

/*******************************************************************************
**
** Function        do_write
**
** Description     write
**
** Returns         len
**
*******************************************************************************/
static int do_write(int fd, unsigned char *buf,int len)
{
    int ret = 0;
    int write_offset = 0;
    int write_len = len;
    do {
        ret = write(fd,buf+write_offset,write_len);
        if (ret < 0)
        {
            PR_ERR("write failed: %s (%d)", strerror(errno), errno);
            return -1;
        } else if (ret == 0) {
            PR_ERR("write failed: %s (%d)", strerror(errno), errno);
            return 0;
        } else {
            if (ret < write_len) {
                PR_INFO("write pending, do write ret: %d, %s (%d)", ret, strerror(errno), errno);
                write_len = write_len - ret;
                write_offset = ret;
            } else {
                PR_INFO("Write successful");
                break;
            }
        }
    } while(1);
    return len;
}

/*******************************************************************************
**
** Function        check vendor event
**
** Description     check info
**
** Returns         success return 1
**
*******************************************************************************/
static int check_event(unsigned char * rsp, int size, unsigned char *cmd)
{

#if 0
    int i = 0;
    for(i = 0; i< size; i++)
        PR_INFO("%02x", rsp[i]);
#endif
    if (!(size >= 7))
        return 0;

    if (rsp[4] != cmd[1] || rsp[5] != cmd[2] || rsp[6] != 0X00)
    {
        return 0;
    }
    return 1;
}

/*******************************************************************************
**
** Function        read_vendor_event
**
** Description     read_vendor_event
**
** Returns         str
**
*******************************************************************************/
static int read_vendor_event(int fd, unsigned char* buf, int size)
{
    int remain, r;
    int count = 0;

    if (size <= 0)
        return -1;

    struct pollfd pfd;
    int poll_ret;
    pfd.fd = fd;
    pfd.events = POLLIN | POLLHUP;

    poll_ret = poll(&pfd, 1, 100);

    if (poll_ret <= 0) {
        PR_ERR("receive hci event timeout! ret=%d", poll_ret);
        return -1;
    }
    PR_DBG("poll ret=%d", poll_ret);

    while (1) {
        r = read(fd, buf, 1);
        if (r <= 0)
            return -1;
        if (buf[0] == 0x04)
            break;
    }
    count++;

    while (count < 3) {
        r = read(fd, buf + count, 3 - count);
        if (r <= 0)
            return -1;
        count += r;
    }

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

/*******************************************************************************
**
** Function        h5_read_vendor_event
**
** Description     h5_read_vendor_event
**
** Returns         count
**
*******************************************************************************/
static int h5_read_vendor_event(int fd, unsigned char* buf, int size)
{
    int r;
    int count = 0;

    if (size <= 0)
        return -1;

    struct pollfd pfd;
    int poll_ret;
    pfd.fd = fd;
    pfd.events = POLLIN | POLLHUP;

    poll_ret = poll(&pfd, 1, 100);

    if (poll_ret <= 0) {
        PR_ERR("receive hci event timeout! ret=%d", poll_ret);
        return -1;
    }
    PR_DBG("poll ret=%d", poll_ret);

    while (1) {
        r = read(fd, buf, 1);
        if (r <= 0)
            return -1;
        if (buf[0] == 0xc0)
            break;
    }
    count++;

    while (count < 2) {
        r = read(fd, buf + count, 2 - count);
        if (r <= 0)
            return -1;
        count += r;
    }

    return count;
}

static int h5_send_vendor_cmd(int fd, unsigned char* cmd, int size)
{
    if (do_write(fd, cmd, size) != size)
    {
        PR_ERR("cmd send is error");
        goto error;
    }
    return 0;
error:
    return 1;
}

/*******************************************************************************
**
** Function        hci_vendor_reset
**
** Description     hci_reset
**
** Returns         int
**
*******************************************************************************/
static int start_vendor_cmd(int fd, unsigned char* cmd, int size)
{
    int rsp_size = 0;

    unsigned char rsp[HCI_MAX_EVENT_SIZE];

    memset(rsp, 0x0, HCI_MAX_EVENT_SIZE);
    if (do_write(fd, cmd, size) != size)
    {
        PR_ERR("cmd send is error");
        goto error;
    }

    rsp_size = read_vendor_event(fd, rsp, HCI_MAX_EVENT_SIZE);

    if (rsp_size < 0)
    {
        PR_ERR("read vendor event error");
        goto error;
    }

    if (!check_event(rsp, rsp_size, cmd))
    {
        PR_ERR("rsp event is error");
        goto error;
    }
    return 0;
error:
    return 1;

}

/*******************************************************************************
**
** Function        get_vendor_info
**
** Description     get_vendor_info
**
** Returns         str
**
*******************************************************************************/
static unsigned char * get_vendor_info(int fd, unsigned char * cmd, int size, unsigned char * event, int *event_size)
{
    int rsp_size = 0;
    unsigned char rsp[HCI_MAX_EVENT_SIZE];

    PR_DBG();

    memset(rsp, 0x0, HCI_MAX_EVENT_SIZE);
    if (do_write(fd, cmd, size) != size)
    {
        PR_ERR("cmd send is error");
        goto error;
    }

    rsp_size = read_vendor_event(fd, rsp, HCI_MAX_EVENT_SIZE);

    if (rsp_size < 0)
    {
        PR_ERR("get_vendor_info error");
        goto error;
    }

    if (!check_event(rsp, rsp_size, cmd))
    {
        PR_ERR("rsp event is error");
        goto error;
    }

    *event_size = rsp_size;
    memcpy(event, rsp, rsp_size);

    return event;
error:
    return NULL;
}

static bool matching_mfrs_vid(const unsigned char *buf, int size)
{
    bool ret = false;
    unsigned int vid = 0;

    if (size >= 13) {
        vid = (((unsigned int)buf[11])<< 8) | ((unsigned int)buf[12]);
    } else {
        if ((buf[0] == 0xc0) && (buf[1] == 0x00)) {
            vid = BT_VID_REALTECK;
        }
    }

    if (matching_dev_id_uart(vid)) {
        ret = true;
    } else {
        PR_ERR("vendor id don't match :0x%04x", vid);
    }

    return ret;
}

/*******************************************************************************
**
** Function        userial_vendor_close
**
** Description     Conduct vendor-specific close work
**
** Returns         None
**
*******************************************************************************/
static void userial_vendor_close(void)
{
    int result;

    if (bt_uart_cb.fd == -1)
        return;

#if (BT_WAKE_VIA_USERIAL_IOCTL==TRUE)
    /* de-assert bt_wake BEFORE closing port */
    ioctl(bt_uart_cb.fd, USERIAL_IOCTL_BT_WAKE_DEASSERT, NULL);
#endif

    PR_INFO("device fd = %d close", bt_uart_cb.fd);
    // flush Tx before close to make sure no chars in buffer
    tcflush(bt_uart_cb.fd, TCIOFLUSH);
    if ((result = close(bt_uart_cb.fd)) < 0)
        PR_ERR( "close(fd:%d) FAILED result:%d", bt_uart_cb.fd, result);

    bt_uart_cb.fd = -1;
}

static bool distinguish_bt_module_uart_h4(void)
{
    int fd;
    int event_size = 0;
    unsigned char event[HCI_MAX_EVENT_SIZE] = {'\0'};
    bool ret = false;

    PR_DBG("start uart h4 init");

    userial_vendor_init();
    fd = userial_vendor_open((uart_cfg *) &uart_cfg_h4);
    if (fd < 0) {
        PR_ERR("open uart h4 fail");
        goto exit;
    }

    if (start_vendor_cmd(fd,(unsigned char *)vendor_reset, sizeof(vendor_reset))) {
        PR_ERR("start_vendor_cmd err");
        goto exit;
    }

    if (get_vendor_info(fd, (unsigned char *)vendor_info, sizeof(vendor_info),  event, &event_size) == NULL) {
        PR_ERR("get_vendor_info err");
        goto exit;
    }

    if (matching_mfrs_vid(event, event_size)) {
        PR_INFO("uart h4 matching vid success");
        ret = true;
    } else {
        PR_ERR("uart h4 matching vid fail");
    }

exit:
    userial_vendor_close();
    return ret;
}

static bool distinguish_bt_module_uart_h5(void)
{
    int fd;
    int event_size = 0;
    unsigned char event[HCI_MAX_EVENT_SIZE] = {'\0'};
    bool ret = false;

    PR_DBG("start uart h5 init");

    userial_vendor_init();
    fd = userial_vendor_open((uart_cfg *) &uart_cfg_h5);
    if (fd < 0) {
        PR_ERR("open uart h5 fail");
        goto exit;
    }

    if (h5_send_vendor_cmd(fd, (unsigned char *)vendor_sync, sizeof(vendor_sync))) {
        PR_ERR("h5_send_vendor_cmd err");
        goto exit;
    }

    event_size = h5_read_vendor_event(fd, event, HCI_MAX_EVENT_SIZE);
    if (event_size < 0) {
        PR_ERR("h5_read_vendor_event err");
        goto exit;
    }

    if (matching_mfrs_vid(event, event_size)) {
        PR_INFO("uart h5 matching vid success");
        ret = true;
        goto exit;
    } else {
        PR_ERR("uart h5 matching vid fail");
    }

exit:
    userial_vendor_close();
    return ret;
}

static bool distinguish_bt_module_uart(void)
{
    bool ret;

    ret = distinguish_bt_module_uart_h4();
    if(!ret) {
        ret = distinguish_bt_module_uart_h5();
    }

    return ret;

}

static bool distinguish_bt_module(void)
{
    unsigned int retry_cnt = 1;
    unsigned int cnt = 0;
    unsigned int retry_cnt_usb = 0;

    PR_DBG();

    while (cnt <= retry_cnt) {
        if (distinguish_dev_name_specify()) {
            goto exit;
        }

        if (distinguish_bt_module_pci()) {
            goto exit;
        }

        if (distinguish_bt_module_sdio()) {
            goto exit;
        }

        if (distinguish_bt_module_usb()) {
            goto exit;
        }

        if (cnt > 0) { // uart recognition takes too long, ignore it first when unsure if bt is en
            if (distinguish_bt_module_uart()) {
                goto exit;
            }

            while (retry_cnt_usb < 20) {  // usb distinguish retry maximum delay 400ms
                usleep(20000);
                retry_cnt_usb ++;
                PR_INFO("usb distinguish retry_cnt_usb:%u", retry_cnt_usb);
                if (distinguish_bt_module_usb()) {
                    goto exit;
                } else {
                    continue;
                }
            }
        }

        cnt ++;

        if (cnt <= retry_cnt) {
            upio_set_bluetooth_power(UPIO_BT_POWER_ON);
            PR_INFO("retry cnt:%u", cnt);
        }
    }

    return false;

exit:
    distinguish = 1;
    return  true;
}

