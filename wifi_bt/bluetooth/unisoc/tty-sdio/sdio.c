/*
 * Copyright (C) 2018 Spreadtrum Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/tty.h>
#include <linux/vt_kern.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/delay.h>
#include <linux/semaphore.h>
#include <linux/vmalloc.h>
#include <linux/atomic.h>
#include <linux/compat.h>
#include <linux/tty_flip.h>
#include <linux/kthread.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>

#include "include/sdio.h"
#include "include/debug.h"
#include "include/hci.h"

static struct semaphore sem_id;
static struct bt_data_interface_cb_t *bt_data_if_cb;
static int need_indicate_cp2_woble = 0;

int set_need_indicate_cp2_woble(int set)
{
    pr_err("%s %d\n", __func__, set);
    need_indicate_cp2_woble = set;
    return 0;
}

static int marlin_sdio_tx_cb(int chn, struct mbuf_t *head, struct mbuf_t *tail, int num)
{
    int i;
    struct mbuf_t *pos = NULL;
    BT_VER("%s channel: %d, head: %p, tail: %p num: %d\n", __func__, chn, head, tail, num);

    pos = head;

    for (i = 0; i < num; i++, pos = pos->next)
    {
        kfree(pos->buf);
        pos->buf = NULL;
    }

    if ((sprdwcn_bus_list_free(chn, head, tail, num)) == 0)
    {
        BT_VER("%s sprdwcn_bus_list_free() success\n", __func__);
        up(&sem_id);
    }
    else
    {
        pr_err("%s sprdwcn_bus_list_free() fail\n", __func__);
    }

    return 0;
}

static int marlin_sdio_rx_cb(int chn, struct mbuf_t *head, struct mbuf_t *tail, int num)
{
    int block_size;

    block_size = ((head->buf[2] & 0x7F) << 9) + (head->buf[1] << 1) + (head->buf[0] >> 7);
    BT_VER("%s +++\n", __func__);

    if (bt_data_if_cb == NULL)
    {
        return -1;
    }

    BT_VERDUMP((unsigned char *)head->buf + BT_SDIO_HEAD_LEN, block_size);

    bt_data_if_cb->recv((unsigned char *)head->buf + BT_SDIO_HEAD_LEN, block_size);
    sprdwcn_bus_push_list(chn, head, tail, num);
    BT_VER("%s ---\n", __func__);

    return 0;
}


int marlin_sdio_write(const unsigned char *buf, int count)
{
    int num = 1, ret;
    struct mbuf_t *tx_head = NULL, *tx_tail = NULL;
    unsigned char *block = NULL;
    const unsigned char *tmp_buf = buf + 1;
    unsigned short op = 0;
    int i = 0;

    STREAM_TO_UINT16(op, tmp_buf);

    if (buf[0] == 0x1)
    {
        BT_DEBUG("%s +++ op 0x%04X\n", __func__, op);
    }

    block = kmalloc(count + BT_SDIO_HEAD_LEN, GFP_KERNEL);

    if (!block)
    {
        pr_err("%s kmalloc failed\n", __func__);
        return -ENOMEM;
    }

    memset(block, 0, count + BT_SDIO_HEAD_LEN);
    memcpy(block + BT_SDIO_HEAD_LEN, buf, count);


    down(&sem_id);
    ret = sprdwcn_bus_list_alloc(BT_TX_CHANNEL, &tx_head, &tx_tail, &num);

    if (ret)
    {
        pr_err("%s sprdwcn_bus_list_alloc failed: %d\n", __func__, ret);
        up(&sem_id);
        return -ENOMEM;
    }


    tx_head->buf = block;
    tx_head->len = count;
    tx_head->next = NULL;
    if (op == 0xfd09 || op == 0xfd0a || op == 0xfd0d)
    {
        BT_VER("%s\n", __func__);
        for(i = 0 ; i < count; i ++)
        {
            BT_VER("%02x ", buf[i]);
        }
        BT_VER("\n");
    }
    BT_VERDUMP(block, count);

    ret = sprdwcn_bus_push_list(BT_TX_CHANNEL, tx_head, tx_tail, num);

    if (ret)
    {
        pr_err("%s sprdwcn_bus_push_list failed: %d\n", __func__, ret);
        kfree(tx_head->buf);
        tx_head->buf = NULL;
        sprdwcn_bus_list_free(BT_TX_CHANNEL, tx_head, tx_tail, num);
        return -EBUSY;
    }

    if (buf[0] == 0x1)
    {
        BT_DEBUG("%s ---\n", __func__);
    }

    /*
        if (op == 0xfd09)
        {
            unsigned char woble_enable = buf[4];
            unsigned char sleep_mod = buf[5];

            if (woble_enable == 1 && sleep_mod == 1 || woble_enable == 0) //WOBLE_MOD_ENABLE, WOBLE_SLEEP_MOD_COULD_NOT_KNOW
            {
                set_need_indicate_cp2_woble(0);
                pr_err("%s woble_enable %d sleep_mod %d\n", __func__, woble_enable, sleep_mod);
            }

        }
    */
    return count;
}

static int bt_tx_powerchange(int channel, int is_resume)
{
    pr_err("%s channel %d is_resume %d need_indicate_cp2_woble %d", __func__, channel, is_resume, need_indicate_cp2_woble);

    if (strcmp(WOBLE_TYPE, "disable"))
    {
        unsigned long power_state = marlin_get_power_state();
        pr_info("%s is_resume =%d", __func__, is_resume);
        if (test_bit(MARLIN_BLUETOOTH, &power_state)) {
            if (!is_resume)
            {
                hci_set_ap_sleep_mode(WOBLE_IS_NOT_SHUTDOWN, WOBLE_IS_NOT_RESUME);
                hci_set_scan_parameters();
                hci_set_scan_enable(1);
                hci_add_device_to_wakeup_list();
                hci_set_ap_start_sleep();
            }
            else
            {
                unsigned char payload[4] = {0};
                int block_size = 4;
                payload[0] = 0x04;
                payload[1] = 0x10;
                payload[2] = 0x01;
                payload[3] = 0x00;
                hci_set_ap_sleep_mode(WOBLE_IS_NOT_SHUTDOWN, WOBLE_IS_RESUME);
                bt_data_if_cb->recv(payload, block_size);
            }
        }
        /*
        if (!need_indicate_cp2_woble)
        {
            return 0;
        }

        if (BT_TX_CHANNEL != channel)
        {
            return -1;
        }

        if (!is_resume)
        {
            hci_woble_enable();
        }
        */
    }

    return 0;
}

static struct mchn_ops_t bt_rx_ops =
{
    .channel = BT_RX_CHANNEL,
    .hif_type = HW_TYPE_SDIO,
    .inout = BT_RX_INOUT,
    .pool_size = BT_RX_POOL_SIZE,
    .pop_link = marlin_sdio_rx_cb,
};

static struct mchn_ops_t bt_tx_ops =
{
    .channel = BT_TX_CHANNEL,
    .hif_type = HW_TYPE_SDIO,
    .inout = BT_TX_INOUT,
    .pool_size = BT_TX_POOL_SIZE,
    .pop_link = marlin_sdio_tx_cb,
    .power_notify = bt_tx_powerchange,
};


int marlin_sdio_init(struct bt_data_interface_cb_t *cb)
{
    int ret = 0;
    pr_err("%s\n", __func__);
    ret = sprdwcn_bus_chn_init(&bt_rx_ops);

    if (ret)
    {
        pr_err("%s sprdwcn_bus_chn_init err chn %d\n", __func__, bt_rx_ops.channel);
    }

    ret = sprdwcn_bus_chn_init(&bt_tx_ops);

    if (ret)
    {
        pr_err("%s sprdwcn_bus_chn_init err chn %d\n", __func__, bt_tx_ops.channel);
    }

    bt_data_if_cb = cb;
    sema_init(&sem_id, BT_TX_POOL_SIZE - 1);
    return 0;
}


void marlin_sdio_cleanup(void)
{
    sprdwcn_bus_chn_deinit(&bt_rx_ops);
    sprdwcn_bus_chn_deinit(&bt_tx_ops);
}


static struct bt_data_interface_t marlin_sdio_interface =
{
    .size = sizeof(struct bt_data_interface_t),
    .init = marlin_sdio_init,
    .cleanup = marlin_sdio_cleanup,
    .write = marlin_sdio_write
};

struct bt_data_interface_t *get_marlin_sdio_interface(void)
{
    return &marlin_sdio_interface;
}


