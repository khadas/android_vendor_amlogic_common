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
#ifndef RTK_HCI_H5_SNOOP_H
#define RTK_HCI_H5_SNOOP_H

#include <utils/Log.h>
#include <errno.h>
#include <stdio.h>
#include <sys/eventfd.h>
#include <cutils/sockets.h>

#include "hci_h5_int.h"
#include <sys/poll.h>
#include <assert.h>
#include "bt_skbuff.h"
#include "rtk_common.h"
#include <cutils/properties.h>

#define H5_CHECKSUM_ERROR       0x10
#define H5_CRC_ERROR            0x20
#define H5_OOR_ERROR            0x30
#define H5_SHORT_WITHOUT_HEAD   0x40
#define H5_SHORT_WITH_HEAD      0x50
#define H5_RETRANS_DATA         0x60

typedef struct
{
    int hci_h5_snoop_fd;
    bool h5_thread_running;
    pthread_t h5_thread_id;
    int epoll_fd;
    int event_fd;
    int signal_fd;
    RTB_QUEUE_HEAD *h5_data;
} h5_snoop_cb_t;

void h5_snoop_open(int signal_fd);
void h5_snoop_capture(uint8_t seq_ack, uint8_t pkt_type, uint8_t *packet, uint8_t flag);
void h5_snoop_cleanup(void);
#endif
