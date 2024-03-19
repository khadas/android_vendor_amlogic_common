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
#define LOG_TAG "hci_h5_snoop"
#include <unistd.h>
#include "bt_vendor_rtk.h"
#include "hci_h5_snoop.h"

#define HCI_COMMAND_PKT         0x01
#define HCI_ACLDATA_PKT         0x02
#define HCI_SCODATA_PKT         0x03
#define HCI_EVENT_PKT           0x04
#define HCI_ISODATA_PKT         0x05

#define HCI_H5_SNOOP_MAX        100

static h5_snoop_cb_t h5_snoop;

// File descriptor for btsnoop file.
static char h5_btsnoop_path[1024] = {'\0'};
extern char rtk_btsnoop_path[];
// Epoch in microseconds since 01/01/0000.
static const uint64_t BTSNOOP_EPOCH_DELTA = 0x00dcddb30f2f8000ULL;


struct h5_object_t
{
    int fd;                              // the file descriptor to monitor for events.
    void *context;                       // a context that's passed back to the *_ready functions..

    void (*read_ready)(void *context);   // function to call when the file descriptor becomes readable.
    void (*write_ready)(void *context);  // function to call when the file descriptor becomes writeable.
};

static struct h5_object_t h5_object;

static void h5_snoop_write(uint8_t *data, size_t length)
{
    if (h5_snoop.hci_h5_snoop_fd != -1)
    {
        write(h5_snoop.hci_h5_snoop_fd, data, length);
    }
}

static void h5_snoop_close(void)
{
    if (h5_snoop.hci_h5_snoop_fd != -1)
    {
        close(h5_snoop.hci_h5_snoop_fd);
    }
    h5_snoop.hci_h5_snoop_fd = -1;
}

static uint64_t h5_snoop_timestamp(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    // Timestamp is in microseconds.
    uint64_t timestamp = tv.tv_sec * 1000LL * 1000LL;
    timestamp += tv.tv_usec;
    timestamp += BTSNOOP_EPOCH_DELTA;
    return timestamp;
}

static void *h5_snoop_thread(void *arg)
{
    RTK_UNUSED(arg);
    struct epoll_event events[64];
    memset(&events, 0, sizeof(struct epoll_event) * 64);
    int j;
    bool is_signal_out = false;
    prctl(PR_SET_NAME, (unsigned long)"h5_snoop_thread", 0, 0, 0);
    while (h5_snoop.h5_thread_running && !is_signal_out)
    {
        int ret;
        do
        {
            ret = epoll_wait(h5_snoop.epoll_fd, events, 32, -1);
        }
        while (h5_snoop.h5_thread_running && ret == -1 && errno == EINTR);

        if (ret == -1)
        {
            ALOGE("%s error in epoll_wait: %s", __func__, strerror(errno));
        }
        for (j = 0; j < ret; ++j)
        {
            struct h5_object_t *object = (struct h5_object_t *)events[j].data.ptr;
            if (events[j].data.ptr == NULL)
            {
                is_signal_out = true;
            }
            else
            {
                if (events[j].events & (EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR) && object->read_ready)
                {
                    object->read_ready(object->context);
                }
                if (events[j].events & EPOLLOUT && object->write_ready)
                {
                    object->write_ready(object->context);
                }
            }
        }
    }
    ALOGD("%s exit", __func__);
    return NULL;

}

static void h5_snoop_handler(void *context)
{
    RTK_UNUSED(context);
    RTK_BUFFER *skb_data;
    eventfd_t value;
    memset(&value, 0, sizeof(eventfd_t));
    eventfd_read(h5_snoop.event_fd, &value);
    if (!value && !h5_snoop.h5_thread_running)
    {
        return;
    }

    while (!RtbQueueIsEmpty(h5_snoop.h5_data))
    {
        skb_data = RtbDequeueHead(h5_snoop.h5_data);
        h5_snoop_write(skb_data->Data, skb_data->Length);
    }
}

/**************************************************************************
**    data struct : Length(1byte) | flag(1 byte) | timestamp(HSB 8 bytes) | seq_ack(1 byte) |packet type(1 byte)|packet_content(16 bytes)
**    flag -----bit[0]-->direction, bit[7~4]--> Error detect
**    seq_ack ----bit[3~0]-->ack, bit[7~4]--> tx seq
**
***************************************************************************/
void h5_snoop_capture(uint8_t seq_ack, uint8_t pkt_type, uint8_t *packet, uint8_t flag)
{
    if (h5_snoop.hci_h5_snoop_fd == -1)
    {
        return;
    }
    uint8_t length = 11;
    uint8_t temp;
    if ((flag & 0xF0) == 0 || (flag & 0xF0) == H5_RETRANS_DATA)
    {
        switch (pkt_type)
        {
        case HCI_ACLDATA_PKT:
        case HCI_ISODATA_PKT:
            temp = (packet[3] << 8) + packet[2];
            length += (temp < 16 ? temp : 16) + 2;
            break;

        case HCI_EVENT_PKT:
            length += (packet[1] < 16 ? packet[1] : 16) + 1;
            break;

        case HCI_SCODATA_PKT:
            length += (packet[2] < 16 ? packet[2] : 16) + 2;
            break;

        case HCI_COMMAND_PKT:
            length += (packet[2] < 16 ? packet[2] : 16) + 2;
            break;

        case H5_LINK_CTL_PKT:

            break;

        case H5_ACK_PKT:

            break;

        default:

            break;
        }
    }

    uint64_t timestamp = h5_snoop_timestamp() + BTSNOOP_EPOCH_DELTA;
    uint32_t time_hi = timestamp >> 32;
    uint32_t time_lo = timestamp & 0xFFFFFFFF;

    RTK_BUFFER *skb_data = RtbAllocate(length + 1, 0);
    if (skb_data == NULL)
    {
        ALOGE("%s RtbAllocate fail.", __func__);
        return;
    }
    skb_data->Data[0] = length;
    skb_data->Data[1] = flag;
    memcpy(skb_data->Data + 2, &time_hi, 4);
    memcpy(skb_data->Data + 6, &time_lo, 4);
    skb_data->Data[10] = seq_ack;
    skb_data->Data[11] = pkt_type;
    if (length > 11)
    {
        memcpy(skb_data->Data + 12, packet, length - 11);
    }
    skb_data->Length = length + 1;
    RtbQueueTail(h5_snoop.h5_data, skb_data);

    if (eventfd_write(h5_snoop.event_fd, 1) == -1)
    {
        ALOGE("%s unable to write for h5 event fd.", __func__);
    }
}

void h5_snoop_open(int signal_fd)
{
    char last_log_path[PATH_MAX];
    uint64_t timestamp;
    uint32_t usec;
    char value[200] = {0};
    struct epoll_event event;

    memset(&h5_snoop, 0, sizeof(h5_snoop));
    h5_snoop.hci_h5_snoop_fd = -1;
    h5_snoop.h5_thread_id = -1;

    //set true, just capture, set save the last capture log with time stamp
    property_get("persist.vendor.bluetooth.h5mode", value, "false");
    if (strncmp(value, "false", 5) == 0)
    {
        return;
    }

    h5_snoop.h5_data = RtbQueueInit();

    snprintf(h5_btsnoop_path, 1024, "%s_H5", rtk_btsnoop_path);

    if (strncmp(value, "save", 4) == 0)
    {
        time_t current_time = time(NULL);
        struct tm *time_created = localtime(&current_time);
        if (time_created)
        {
            char config_time_created[sizeof("YYYY-MM-DD-HH-MM-SS")];
            strftime(config_time_created, sizeof("YYYY-MM-DD-HH-MM-SS"), "%Y-%m-%d-%H-%M-%S",
                     time_created);
            timestamp = h5_snoop_timestamp() - BTSNOOP_EPOCH_DELTA;
            usec = (uint32_t)(timestamp % 1000000LL);
            snprintf(last_log_path, PATH_MAX, "%s.%s.%uUS", h5_btsnoop_path, config_time_created, usec);
        }
        else
        {
            snprintf(last_log_path, PATH_MAX, "%s.last", h5_btsnoop_path);
        }
        if (!rename(h5_btsnoop_path, last_log_path) && errno != ENOENT)
        {
            ALOGE("%s unable to rename '%s' to '%s': %s", __func__, h5_btsnoop_path, last_log_path,
                  strerror(errno));
        }
    }
    else
    {
        snprintf(last_log_path, PATH_MAX, "%s.last", h5_btsnoop_path);
        if (!rename(h5_btsnoop_path, last_log_path) && errno != ENOENT)
        {
            ALOGE("%s unable to rename '%s' to '%s': %s", __func__, h5_btsnoop_path, last_log_path,
                  strerror(errno));
        }
    }

    h5_snoop.hci_h5_snoop_fd = open(h5_btsnoop_path,
                                    O_WRONLY | O_CREAT | O_TRUNC,
                                    S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);

    if (h5_snoop.hci_h5_snoop_fd == -1)
    {
        ALOGE("%s unable to open '%s': %s", __func__, rtk_btsnoop_path, strerror(errno));
        return;
    }

    write(h5_snoop.hci_h5_snoop_fd, "hci_h5\0\0\0\0\1\0\0\x3\xea", 15);

    h5_snoop.signal_fd = signal_fd;
    h5_snoop.epoll_fd = epoll_create(64);
    assert(h5_snoop.epoll_fd != -1);

    h5_snoop.event_fd = eventfd(HCI_H5_SNOOP_MAX, EFD_NONBLOCK);
    assert(h5_snoop.event_fd != -1);

    h5_object.fd = h5_snoop.event_fd;
    h5_object.read_ready = h5_snoop_handler;
    memset(&event, 0, sizeof(event));
    event.events |= EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR;
    event.data.ptr = (void *)&h5_object;
    if (epoll_ctl(h5_snoop.epoll_fd, EPOLL_CTL_ADD, h5_snoop.event_fd, &event) == -1)
    {
        ALOGE("%s unable to register fd %d to cpoll set: %s", __func__, h5_snoop.event_fd, strerror(errno));
        assert(false);
    }

    event.data.ptr = NULL;
    if (epoll_ctl(h5_snoop.epoll_fd, EPOLL_CTL_ADD, signal_fd, &event) == -1)
    {
        ALOGE("%s unable to register fd %d to cpoll set: %s", __func__, signal_fd, strerror(errno));
        assert(false);
    }

    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);

    h5_snoop.h5_thread_running = true;
    if (pthread_create(&h5_snoop.h5_thread_id, &thread_attr, h5_snoop_thread, NULL) != 0)
    {
        ALOGE("pthread_create : %s", strerror(errno));
        h5_snoop_close();
        return;
    }

}

void h5_snoop_cleanup(void)
{
    int result;
    if (h5_snoop.h5_thread_id == -1)
    {
        return;
    }
    h5_snoop.h5_thread_running = false;
    if (epoll_ctl(h5_snoop.epoll_fd, EPOLL_CTL_DEL, h5_snoop.event_fd, NULL) == -1)
    {
        ALOGE("%s unable to unregister fd %d from epoll set: %s", __func__, h5_snoop.event_fd,
              strerror(errno));
    }

    if (epoll_ctl(h5_snoop.epoll_fd, EPOLL_CTL_DEL, h5_snoop.signal_fd, NULL) == -1)
    {
        ALOGE("%s unable to unregister fd %d from cpoll set: %s", __func__, h5_snoop.signal_fd,
              strerror(errno));
    }

    if ((result = close(h5_snoop.event_fd)) < 0)
    {
        ALOGE("%s (fd:%d) FAILED result:%d", __func__, h5_snoop.event_fd, result);
    }

    close(h5_snoop.epoll_fd);
    if (h5_snoop.h5_thread_id != -1)
    {
        if (pthread_join(h5_snoop.h5_thread_id, NULL) != 0)
        {
            ALOGE("%s h5_snoop.h5_thread_id  pthread_join_failed", __func__);
        }
        else
        {
            h5_snoop.h5_thread_id = -1;
        }
    }
    h5_snoop.epoll_fd = -1;
    h5_snoop.event_fd = -1;
    h5_snoop_close();
    RtbQueueFree(h5_snoop.h5_data);
}

