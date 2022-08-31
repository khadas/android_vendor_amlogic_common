#ifdef GPL_FILE
/** @file  fm.h
  *
  * @brief This file contains definitions for application
  *
  *
  * Copyright 2014-2020 NXP
  *
  * This software file (the File) is distributed by NXP
  * under the terms of the GNU General Public License Version 2, June 1991
  * (the License).  You may use, redistribute and/or modify the File in
  * accordance with the terms and conditions of the License, a copy of which
  * is available by writing to the Free Software Foundation, Inc.,
  * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA or on the
  * worldwide web at http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt.
  *
  * THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE
  * ARE EXPRESSLY DISCLAIMED.  The License provides additional details about
  * this warranty disclaimer.
  *
  */
#elif defined(APACHE)
/** @file  fm.h
  *
  * @brief This file contains definitions for application
  *
  *
  * Copyright 2014-2020 NXP
  *
  * Licensed under the Apache License, Version 2.0 (the License);
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *   http://www.apache.org/licenses/LICENSE-2.0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an ASIS BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  */
#elif defined(FREE_BSD)
/** @file  fm.h
  *
  * @brief This file contains definitions for application
  *
  *
  * Copyright 2014-2020 NXP
  *
  * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
  * following conditions are met:
  *
  * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
  * disclaimer.
  *
  * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
  * disclaimer in the documentation and/or other materials provided with the distribution.
  *
  * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote
  * products derived from this software without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ASIS AND
  * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
  * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
  * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
  * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  */
#else
/** @file  fm.h
  *
  * @brief This file contains definitions for application
  * 
  *
  * Copyright 2014-2020 NXP
  *
  * NXP CONFIDENTIAL
  * The source code contained or described herein and all documents related to
  * the source code (Materials) are owned by NXP, its
  * suppliers and/or its licensors. Title to the Materials remains with NXP,
  * its suppliers and/or its licensors. The Materials contain
  * trade secrets and proprietary and confidential information of NXP, its
  * suppliers and/or its licensors. The Materials are protected by worldwide copyright
  * and trade secret laws and treaty provisions. No part of the Materials may be
  * used, copied, reproduced, modified, published, uploaded, posted,
  * transmitted, distributed, or disclosed in any way without NXP's prior
  * express written permission.
  *
  * No license under any patent, copyright, trade secret or other intellectual
  * property right is granted to or conferred upon you by disclosure or delivery
  * of the Materials, either expressly, by implication, inducement, estoppel or
  * otherwise. Any license under such intellectual property rights must be
  * express and approved by NXP in writing.
  *
  */
#endif
/************************************************************************
Change log:
     03/15/2012: initial version
************************************************************************/

#ifndef __FM_H
#define __FM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/socket.h>

/* Byte order conversions */
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define htobs(d)  (d)
#define htobl(d)  (d)
#define btohs(d)  (d)
#define btohl(d)  (d)
#else
#if __BYTE_ORDER == __BIG_ENDIAN
#define htobs(d)  bswap_16(d)
#define htobl(d)  bswap_32(d)
#define btohs(d)  bswap_16(d)
#define btohl(d)  bswap_32(d)
#else
#error "Unknown byte order"
#endif
#endif

#define HCI_MAX_DEV	16

#define HCI_MAX_ACL_SIZE	2048
#define HCI_MAX_SCO_SIZE	255
#define HCI_MAX_EVENT_SIZE	260
#define HCI_MAX_FRAME_SIZE	(HCI_MAX_ACL_SIZE + 4)


#define EVT_CMD_STATUS          0x0F
#define EVT_CMD_STATUS_SUCCESS  0x00

#define HCI_COMMAND_PKT         0x01
#define HCI_EVENT_HDR_SIZE      2
#define HCI_COMMAND_HDR_SIZE    3
#define EVT_CMD_COMPLETE        0x0E
#define EVT_PHY_LINK_COMPLETE   0x40
#define EVT_CHANNEL_SELECTED    0x41
#define EVT_LOG_LINK_COMPLETE   0x45
#define EVT_MODE_CHANGE         0x14

/* Command opcode pack/unpack */
#define cmd_opcode_pack(ogf, ocf)       (uint16_t)((ocf & 0x03ff)|(ogf << 10))
#define cmd_opcode_ogf(op)              (op >> 10)
#define cmd_opcode_ocf(op)              (op & 0x03ff)

typedef struct {
        uint8_t         status;
        uint8_t         ncmd;
        uint16_t        opcode;
} __attribute__ ((packed)) evt_cmd_status;


typedef struct {
        uint8_t         evt;
        uint8_t         plen;
} __attribute__ ((packed))      hci_event_hdr;

typedef struct {
        uint8_t         ncmd;
        uint16_t        opcode;
} __attribute__ ((packed)) evt_cmd_complete;

typedef struct {
        uint8_t     status;
        uint8_t     plink_handle;
} __attribute__ ((packed)) evt_phy_link_complete;

typedef struct {
        uint8_t     status;
        uint16_t    llink_handle;
        uint8_t     plink_handle;
        uint8_t     tx_flowspecid;
} __attribute__ ((packed)) evt_log_link_complete;

typedef struct {
        uint8_t         status;
        uint16_t        handle;
        uint8_t         mode;
        uint16_t        interval;
} __attribute__ ((packed)) evt_mode_change;

#ifdef __cplusplus
}
#endif
#endif /* __FM_H */
