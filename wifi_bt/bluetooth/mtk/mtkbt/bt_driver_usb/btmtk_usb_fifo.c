/*
 *  Copyright (c) 2016 MediaTek Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */
#include "btmtk_config.h"
#include <linux/kthread.h>
#include <linux/freezer.h>
#include <asm/uaccess.h>
#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci.h>
#include <linux/signal.h>
#include <linux/time.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/rtc.h>

#include "btmtk_usb_fifo.h"
#include "btmtk_usb_main.h"

/**
 * Definition
 */
#define FW_DUMP_SIZE (1024 * 16)
#define SYS_LOG_SIZE (1024 * 16)

#define KFIO_D(pData, t)	(((struct btmtk_fifo_data_t *)(pData))->fifo_l[t].kfifo.data)
#define pKFIO(pData, t)		(&((struct btmtk_fifo_data_t *)(pData))->fifo_l[t].kfifo)
#define pFIO(pData, t)		(&((struct btmtk_fifo_data_t *)(pData))->fifo_l[t])

static struct btmtk_fifo_data_t g_fifo_data;
static struct btmtk_timer_t g_timer;

static struct btmtk_fifo_t btmtk_fifo_list[] = {
	{ FIFO_SYSLOG, SYS_LOG_SIZE, NULL, {0}, NULL, 0, {0} },
	{ FIFO_COREDUMP, FW_DUMP_SIZE, NULL, {0}, NULL, 0, {0} },
	{ FIFO_END, 0, NULL, {0}, NULL, 0, {0} },
};

#if (KERNEL_VERSION(4, 14, 0) > LINUX_VERSION_CODE)
static void btmtk_coredump_timo_func(void *data)
#else
static void btmtk_coredump_timo_func(struct timer_list *tl)
#endif
{
	struct btmtk_fifo_t *cd = NULL;
#if (KERNEL_VERSION(4, 14, 0) > LINUX_VERSION_CODE)
	if (data) {
		cd = (struct btmtk_fifo_t *)pFIO(data, FIFO_COREDUMP);
#else
	struct btmtk_timer_t *timer = from_timer(timer, tl, coredump_timer);

	if (timer && timer->data_p) {
		cd = (struct btmtk_fifo_t *)pFIO(timer->data_p, FIFO_COREDUMP);
#endif
		cd->tried_all_path = 0;
		if (cd->filp) {
			filp_close(cd->filp, NULL);
			cd->filp = NULL;
		}
	}
	/* trigger HW reset */
	BTUSB_WARN("%s: coredump may fail, hardware reset directly", __func__);
	btmtk_usb_toggle_rst_pin();
}

static void btmtk_add_timer(struct timer_list *timer, u16 sec)
{
	if (timer == NULL || sec == 0) {
		BTUSB_ERR("%s: Incorrect timer(%p, %d)", __func__, timer, sec);
		return;
	}

	if (!timer_pending(timer)) {
		BTUSB_DBG("Add new timer");
		timer->expires = jiffies + HZ * sec;
		add_timer(timer);
	} else {
		BTUSB_DBG("Modify the timer");
		mod_timer(timer, jiffies + HZ * sec);
	}
}

static void btmtk_del_timer(struct timer_list *timer)
{
	if (timer == NULL) {
		BTUSB_ERR("%s: Incorrect timer", __func__);
		return;
	}
	BTUSB_INFO("%s", __func__);

	del_timer_sync(timer);
}

static int fifo_alloc(struct btmtk_fifo_t *bt_fifo)
{
	int ret = 0;
	struct __kfifo *kfio = &bt_fifo->kfifo;

	ret = __kfifo_alloc(kfio, bt_fifo->size, 1, GFP_KERNEL);
	if (ret == 0)
		bt_fifo->size = kfio->mask + 1;

	return ret;
}

static void fifo_free(struct btmtk_fifo_t *bt_fifo)
{
	struct __kfifo *kfio = &bt_fifo->kfifo;

	__kfifo_free(kfio);
}

static int fifo_init(struct btmtk_fifo_t *bt_fifo)
{
	int ret = 0;
	struct __kfifo *kfio = &bt_fifo->kfifo;

	BTUSB_INFO("%s: %d", __func__, bt_fifo->type);

	ret = __kfifo_init(kfio, kfio->data, bt_fifo->size, kfio->esize);

	if (bt_fifo->filp) {
		BTUSB_INFO("%s: call filp_close", __func__);
		vfs_fsync(bt_fifo->filp, 0);
		filp_close(bt_fifo->filp, NULL);
		bt_fifo->filp = NULL;
		BTUSB_WARN("%s: FW dump file closed, rx=0x%X, tx =0x%x",
				__func__, bt_fifo->stat.rx, bt_fifo->stat.tx);
	}

	bt_fifo->stat.rx = 0;
	bt_fifo->stat.tx = 0;
	return ret;
}

static void fifo_out_info(struct btmtk_fifo_data_t *data_p)
{
	struct __kfifo *fifo = NULL;

	if (data_p == NULL) {
		BTUSB_ERR("%s: data_p == NULL return", __func__);
		return;
	}

	fifo = pKFIO(data_p, FIFO_SYSLOG);

	if (fifo->data != NULL) {
		if (fifo->in > fifo->out + fifo->mask)
			BTUSB_WARN("sys_log buffer full");

		if (fifo->in == fifo->out + fifo->mask)
			BTUSB_WARN("sys_log buffer empty");

		BTUSB_DBG("[syslog][fifo in - fifo out = 0x%x]", fifo->in - fifo->out);
	}

	fifo = pKFIO(data_p, FIFO_COREDUMP);

	if (fifo->data != NULL) {
		if (fifo->in > fifo->out + fifo->mask)
			BTUSB_WARN("coredump buffer full");

		if (fifo->in == fifo->out + fifo->mask)
			BTUSB_WARN("coredump buffer empty");
	}
}

static u32 fifo_out_accessible(struct btmtk_fifo_data_t *data_p)
{
	int i;
	struct btmtk_fifo_t *fio_p = NULL;

	if (data_p == NULL)
		return 0;

	if (test_bit(FIFO_TSK_SHOULD_STOP, &data_p->tsk_flag)) {
		BTUSB_WARN("%s: task should stop", __func__);
		return 1;
	}

	for (i = 0; i < FIFO_END; i++) {

		fio_p = &data_p->fifo_l[i];

		if (fio_p->type != i)
			continue;

		if (fio_p->kfifo.data != NULL) {
			if (fio_p->kfifo.in != fio_p->kfifo.out)
				return 1;
		}
	}

	return 0;
}

static struct file *fifo_filp_open(const char *file, int flags, umode_t mode)
{
	struct file *f = NULL;

	f = filp_open(file, flags, mode);
	if (IS_ERR(f)) {
		f = NULL;
		BTUSB_WARN("%s: open file %s failed!", __func__, file);
	}
	return f;
}

static void fifo_kernel_write(struct file *file, const char *buf, u32 len,
				u32 tail_len, u32 off)
{
	unsigned int l = min(len, tail_len);

#if (KERNEL_VERSION(4, 14, 0) > LINUX_VERSION_CODE)
	mm_segment_t old_fs;

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	set_fs(old_fs);

	vfs_write(file, buf + off, l, &file->f_pos);
	vfs_write(file, buf, len - l, &file->f_pos);
	set_fs(old_fs);
#else
	kernel_write(file, buf + off, l, &file->f_pos);
	kernel_write(file, buf, len - l, &file->f_pos);
#endif
}

static void btmtk_get_local_time(char *local_time, u32 size)
{
	struct timeval tv;
	struct rtc_time tm;

	btmtk_do_gettimeofday(&tv);
	tv.tv_sec -= sys_tz.tz_minuteswest * 60;
	rtc_time_to_tm(tv.tv_sec, &tm);

	(void)snprintf(local_time, size, "%04d%02d%02d%02d%02d%02d", tm.tm_year + 1900, tm.tm_mon + 1,
		tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
}

static u32 fifo_out_to_file(struct btmtk_fifo_t *fio, u32 len, u32 off, u8 *end)
{
	struct __kfifo *fifo = &fio->kfifo;
	unsigned int size = fifo->mask + 1;
	unsigned int esize = fifo->esize;
	unsigned int l;
	unsigned int tail_len;
	char *dst_buf = NULL;
	char timestamp_buffer[20];
	char fw_dump_file_name[96];

	off &= fifo->mask;

	if (esize != 1) {
		off *= esize;
		size *= esize;
		len *= esize;
	}

	tail_len = size - off;
	l = min(len, tail_len);

	if (l > 0 && fifo->data != NULL) {
		if (fio->filp == NULL && fio->tried_all_path == 0) {
			memset(timestamp_buffer, 0, sizeof(timestamp_buffer));
			memset(fw_dump_file_name, 0, sizeof(fw_dump_file_name));
			/* get current timestamp */
			btmtk_get_local_time(timestamp_buffer, sizeof(timestamp_buffer));

			/* combine file path and file name */
			(void)snprintf(fw_dump_file_name, sizeof(fw_dump_file_name), "%s_%s",
					fio->folder_name, timestamp_buffer);
			BTUSB_WARN("%s: FW Dump started, try to open file %s", __func__,
				fw_dump_file_name);

			fio->filp = fifo_filp_open(fw_dump_file_name, O_RDWR | O_CREAT, 0644);
			if (fio->filp == NULL) {
				fio->tried_all_path = 1;
				BTUSB_ERR("%s: Dump is failed to save, please check SELinux mode(# setenforce 0)",
						__func__);
				return 0;
			}
		}

		if (fio->filp != NULL) {
			fifo_kernel_write(fio->filp, fifo->data, len, tail_len, off);
			fio->stat.tx += len;

			dst_buf = kzalloc(len, GFP_KERNEL);
			if (!dst_buf) {
				BTUSB_ERR("%s: alloc memory fail (dst_buf)", __func__);
				fio->tried_all_path = 0;
				filp_close(fio->filp, NULL);
				fio->filp = NULL;
				return 0;
			}

			memcpy(dst_buf, fifo->data + off, l);
			memcpy(dst_buf + l, fifo->data, len - l);
			if (dst_buf[len - 6] == ' ' &&
				dst_buf[len - 5] == 'e' &&
				dst_buf[len - 4] == 'n' &&
				dst_buf[len - 3] == 'd') {
				*end = 1;
				fio->tried_all_path = 0;
				BTUSB_INFO("%s: FW Dump finished(success)", __func__);
				filp_close(fio->filp, NULL);
				fio->filp = NULL;
			}

			kfree(dst_buf);
		}
	}

	/*
	 * make sure that the data is copied before
	 * incrementing the fifo->out index counter
	 */
	smp_wmb();
	return len;
}

static u32 fifo_out_peek_fs(struct btmtk_fifo_t *fio, u32 len, u8 *end)
{
	unsigned int l;
	struct __kfifo *fifo = &fio->kfifo;

	l = fifo->in - fifo->out;
	if (len > l)
		len = l;

	fifo_out_to_file(fio, len, fifo->out, end);

	return len;
}

static u32 fifo_out_fs(void *fifo_d)
{
	u8 end = 0;
	u32 len = 0;
	struct btmtk_fifo_t *syslog_p = NULL;
	struct btmtk_fifo_t *coredump_p = NULL;

	if (fifo_d == NULL) {
		BTUSB_ERR("[%s: fifo data is null]", __func__);
		return len;
	}

	if (KFIO_D(fifo_d, FIFO_SYSLOG) != NULL) {
		syslog_p = pFIO(fifo_d, FIFO_SYSLOG);
		len = fifo_out_peek_fs(syslog_p, SYS_LOG_SIZE, &end);
		syslog_p->kfifo.out += len;
	}

	if (KFIO_D(fifo_d, FIFO_COREDUMP) != NULL) {
		coredump_p = pFIO(fifo_d, FIFO_COREDUMP);
		len = fifo_out_peek_fs(coredump_p, FW_DUMP_SIZE, &end);
		coredump_p->kfifo.out += len;

		if (end) {
			BTUSB_INFO("%s: please do reset pin", __func__);
			btmtk_del_timer(&g_timer.coredump_timer);
		}
	}

	return len;
}

static int btmtk_fifo_thread(void *ptr)
{
	struct btmtk_fifo_data_t *data_p = (struct btmtk_fifo_data_t *)ptr;

	if (data_p == NULL) {
		BTUSB_ERR("%s: [FIFO Data is null]", __func__);
		return -1;
	}

	set_freezable();
	while (!kthread_should_stop() || test_bit(FIFO_TSK_START, &data_p->tsk_flag)) {
		wait_event_freezable(data_p->rx_wait_q,
				fifo_out_accessible(data_p) || kthread_should_stop());
		if (test_bit(FIFO_TSK_SHOULD_STOP, &data_p->tsk_flag)) {
			BTUSB_INFO("%s loop break", __func__);
			break;
		}

		fifo_out_info(data_p);
		fifo_out_fs(data_p);
		fifo_out_info(data_p);
	}

	set_bit(FIFO_TSK_EXIT, &data_p->tsk_flag);
	BTUSB_INFO("%s: end: down != 0", __func__);

	return 0;
}

void *btmtk_fifo_init(void)
{
	int i;
	struct btmtk_fifo_data_t *data_p = &g_fifo_data;
	struct btmtk_fifo_t *fio_p = NULL;
	struct btmtk_usb_data *p_usb_data = btmtk_usb_get_data();

	if (!p_usb_data)
		return NULL;

	btmtk_fifo_list[FIFO_SYSLOG].folder_name = p_usb_data->bt_cfg.sys_log_file_name;
	btmtk_fifo_list[FIFO_COREDUMP].folder_name = p_usb_data->bt_cfg.fw_dump_file_name;

	data_p->fifo_l = btmtk_fifo_list;
	data_p->tsk_flag = 0;

	for (i = 0; i < FIFO_END; i++) {
		fio_p = &(data_p->fifo_l[i]);

		if (fio_p->kfifo.data == NULL)
			fifo_alloc(fio_p);

		fifo_init(fio_p);
	}

	init_waitqueue_head(&data_p->rx_wait_q);
	set_bit(FIFO_TSK_INIT, &data_p->tsk_flag);

	return (void *)(data_p);
}

void btmtk_fifo_deinit(void *fio_d)
{
	int i;
	struct btmtk_fifo_t *fio_p = NULL;
	struct btmtk_fifo_data_t *data_p = (struct btmtk_fifo_data_t *)fio_d;

	while (test_bit(FIFO_TSK_INIT, &data_p->tsk_flag)) {
		clear_bit(FIFO_TSK_INIT, &data_p->tsk_flag);
		if (test_bit(FIFO_TSK_EXIT, &data_p->tsk_flag))
			break;

		set_bit(FIFO_TSK_SHOULD_STOP, &data_p->tsk_flag);
		wake_up_process(data_p->fifo_tsk);
		msleep(100);
	}

	data_p->tsk_flag = 0;

	for (i = 0; i < FIFO_END; i++) {
		fio_p = &(data_p->fifo_l[i]);
		fifo_free(fio_p);
	}
}

u32 btmtk_fifo_in(unsigned int type, void *fifo_d, const void *buf,
			 unsigned int length)
{
	u32 len = 0;
	u8 hci_pkt = HCI_EVENT_PKT;
	struct btmtk_fifo_t *fio_p = NULL;
	struct __kfifo *fifo = NULL;

	if (fifo_d == NULL) {
		BTUSB_ERR("%s: [fifo data is null]", __func__);
		return len;
	}

	g_timer.data_p = (struct btmtk_fifo_data_t *)fifo_d;
	if (!test_bit(FIFO_TSK_START, &g_timer.data_p->tsk_flag)) {
		BTUSB_ERR("%s: Fail task not start", __func__);
		return 0;
	}

	fio_p = pFIO(fifo_d, type);
	fifo = pKFIO(fifo_d, type);

	if (fifo->data != NULL) {
		if (type == FIFO_SYSLOG) {
			len = __kfifo_in(pKFIO(fifo_d, type), (const void *)&hci_pkt,
					sizeof(hci_pkt));

			if (len != sizeof(hci_pkt))
				return len;
		}

		len = __kfifo_in(pKFIO(fifo_d, type), buf, length);
		fio_p->stat.rx += len;
	}

	if (len != 0) {
		wake_up_interruptible(&g_timer.data_p->rx_wait_q);

#if (KERNEL_VERSION(4, 19, 0) > LINUX_VERSION_CODE)
		g_timer.coredump_timer.function = (void *)btmtk_coredump_timo_func;
		g_timer.coredump_timer.data = (unsigned long)&g_timer.coredump_timer;
#endif
		btmtk_add_timer(&g_timer.coredump_timer, 1);
	}

	return len;
}

int btmtk_fifo_start(void *fio_d)
{
	struct btmtk_fifo_data_t *data_p = (struct btmtk_fifo_data_t *)fio_d;
	int err;

	if (data_p == NULL) {
		BTUSB_ERR("%s: [fail][fifo data is null]", __func__);
		return -1;
	}

	if (!test_bit(FIFO_TSK_INIT, &data_p->tsk_flag)) {
		BTUSB_ERR("%s: [fail task not init ]", __func__);
		return -1;
	}

	if (!KFIO_D(data_p, FIFO_SYSLOG) && !KFIO_D(data_p, FIFO_COREDUMP)) {
		BTUSB_ERR("%s: There are no syslog or coredump data", __func__);
		return -1;
	}

	data_p->fifo_tsk = kthread_create(btmtk_fifo_thread, fio_d,
			"btmtk_fifo_thread");
	if (IS_ERR(data_p->fifo_tsk)) {
		BTUSB_ERR("%s: create FIFO thread failed!", __func__);
		err = PTR_ERR(data_p->fifo_tsk);
		data_p->fifo_tsk = NULL;
		return -1;
	}
#if (KERNEL_VERSION(4, 19, 0) > LINUX_VERSION_CODE)
	init_timer(&g_timer.coredump_timer);
#else
	timer_setup(&g_timer.coredump_timer, btmtk_coredump_timo_func, 0);
#endif

	set_bit(FIFO_TSK_START, &data_p->tsk_flag);
	BTUSB_INFO("%s: set FIFO_TSK_START", __func__);
	wake_up_process(data_p->fifo_tsk);
	BTUSB_INFO("%s: [ok]", __func__);

	return 0;
}
