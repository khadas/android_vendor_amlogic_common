#ifdef GPL_FILE
/** @file  fm_main.c
  *
  * @brief FM application
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
/** @file  fm_main.c
  *
  * @brief FM application
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
/** @file  fm_main.c
  *
  * @brief FM application
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
/** @file  fm_main.c
  *
  * @brief FM application
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

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include "fm.h"

#define FM_PATH_MAX   256

#define	PRINT_CMD(ogf, ocf, buf, len) \
do { \
	printf("< HCI Command: ogf 0x%02x, ocf 0x%04x, plen %d\n", (ogf), (ocf), (len)); \
	mrvl_hex_dump("  ", 20, (buf), (len)); \
	fflush(stdout); \
} while (0);

#define	PRINT_EVT(hdr, ptr, len) \
do { \
	printf("> HCI Event: 0x%02x plen %d\n", (hdr)->evt, (hdr)->plen); \
	mrvl_hex_dump("  ", 20, (ptr), (len)); \
	fflush(stdout); \
} while (0);

#define OpCodePack(ogf, ocf)   (uint16_t)((ocf & 0x03ff) | (ogf << 10))


static uint8_t verbose;

static struct option main_options[] = {
	{ "help",			0, 0, 'h' },
	{ "verbose",		0, 0, 'v' },
	{ 0, 0, 0, 0 }
};

/** 
 *  @brief                dump HCI cmd/evt 
 *  @param pref      data pref for dump
 *  @param width    data line width for dump
 *  @param buf       data buf for dump
 *  @param len        data len for dump
 *  @return      	      N/A
 */
void mrvl_hex_dump(char *pref, int width, uint8_t *buf, int len)
{
	register int i,n;

	for (i = 0, n = 1; i < len; i++, n++) {
		if (n == 1) {
			printf("%s", pref);
		}
		printf("%2.2X ", buf[i]);
		if (n == width) {
			printf("\n");
			n = 0;
		}
	}
	if (i && n!=1) {
		printf("\n");
	}
}

/** 
 *  @brief                open fm char device
 *  @param dev      fm char device name
 *  @return      	      fm char device describe
 */
int init_chardev(char *dev)
{
	int fd;

#ifdef TOOL_DEBUG
		printf("Debug: Calling open on char device %s\n", dev);
#endif

	fd = open(dev, O_RDWR | O_NOCTTY);
	if (fd < 0) {
		perror("Can't open char device");
		exit(1);
	}
	if (verbose) {
		printf("Returning File Descriptor for char device");
	}
	return fd;
}

/** 
 *  @brief                  check evt/cmd status
 *  @param buf_ptr   pointer to HCi evt header
 *  @return      	        0:success,  other: fail
 */
int check_evt_command_status(unsigned char* buf_ptr)
{
	evt_cmd_status *ecs;
	ecs = (evt_cmd_status *)buf_ptr;
	if (btohs(ecs->status) != EVT_CMD_STATUS_SUCCESS) {
		printf("Command Failed. Command Status Event received with Non-zero status.\n");
		return -1;
	}
	return 0;
}

/** 
 *  @brief                wait cmd resp or event
 *  @param dd         file describe of fm char device
 *  @param ogf        Hci cmd ogf
 *  @param ocf        HCI cmd ocf  
 *  @return      	       N/A
 */
void fm_wait_for_cmd_complete(int dd, uint8_t ogf, uint16_t ocf)
{
	unsigned char *ptr;
	hci_event_hdr *hdr;
	evt_cmd_complete evt_cc, *ecc = &evt_cc;
	evt_mode_change emc, *emc_ptr = &emc;
	evt_phy_link_complete eplc, *eplc_ptr = &eplc;
	evt_log_link_complete ellc, *ellc_ptr = &ellc;
	uint8_t phandle;
	uint16_t lhandle;
	uint16_t opcode = 0;
	int len = 0;
	uint8_t buf[HCI_MAX_ACL_SIZE];

	memset(ecc, 0, sizeof(evt_cmd_complete));

	while(1) {
		len = read(dd, buf, HCI_MAX_EVENT_SIZE);
		if (len < 0) {
			perror("Read failed");
			exit(EXIT_FAILURE);
		}

		hdr = (void *)(buf + 1);
		ptr = buf + (1 + HCI_EVENT_HDR_SIZE);
		len -= (1 + HCI_EVENT_HDR_SIZE);

		if (hdr->evt == EVT_CMD_COMPLETE) {
			ecc = (evt_cmd_complete *)ptr;
			opcode = cmd_opcode_pack(ogf, ocf);
			if (btohs(ecc->opcode) == opcode) {
				PRINT_EVT(hdr, ptr, len);
				return;
			}
		} else if (hdr->evt == EVT_CMD_STATUS) {
			if (check_evt_command_status(ptr)) {
				return;
			}
		}else {
				printf("Received Event Code %X\n", hdr->evt);
				PRINT_EVT(hdr, ptr, len);
				return ;
		}
	}
}

/** 
 *  @brief                send cmd to fm char device
 *  @param fd         file describe of fm char device
 *  @param argc     number of arguments
 *  @param argv     A pointer to arguments array  (cmd content)  
 *  @return      	      0:success,  other: fail
 */
int fm_send_cmd(int fd, int argc, char** argv)
{
        uint16_t opcode;
        uint8_t cmd[512];
        int len = 0,i=0,j=0;
        uint16_t ogf,ocf;

        if (argc <1) {
        	printf("Did not provide cmd content\n");
        	printf("Usage: drvtest <device>  [content]\n");
        	exit(0);
        }

        ogf = strtol(argv[0], NULL, 16);
        ocf = strtol(argv[1], NULL, 16);

        opcode = OpCodePack(ogf,ocf);
        printf("ogf:%X, ocf:%X opcode:%x,  argc = %d\n",ogf,ocf,opcode, argc);

        cmd[0] = HCI_COMMAND_PKT;  
        cmd[1] = (uint8_t)opcode;
        cmd[2] = (uint8_t)(opcode >> 8);

        for(i = 2; i < argc;i++,len++)
        {
           cmd[j+4] = (uint8_t)strtol(argv[i],NULL,16);
           j++;
        }

        cmd[3] = len;

        PRINT_CMD(ogf, ocf, cmd, len+4)

        if (write(fd, cmd, len+4) != (len+4)) {
                perror("Can't write raw command");
                return -1;
        }

        fm_wait_for_cmd_complete(fd, ogf, ocf);
        return 0;
}

/** 
 *  @brief Display usage
 *  @return      	N/A
 */
static void usage(void)
{
	printf("fmapp - ver 1.0.0.2\n");

	printf("Usage:\n"
		"\tfmapp [options] devicename ogf 0cf [command content]\n"
		"\tdevicename example mfmchar0\n"
		"\togf example 0x3f\n"
		"\tocf example 0x280\n"
		"\tCommands supported are as follows:\n");
	printf("Command Options:\n"
		"\t-h\tDisplay help\n"
		"\t-v\tVerbose\n");
}

/** 
 *  @brief Entry function for fmapp
 *  @param argc		number of arguments
 *  @param argv     A pointer to arguments array    
 *  @return      	0/1
 */
int main(int argc, char *argv[])
{
	int opt = 0;
	int send_break = 0; 
	int n = 0;
	char dev[FM_PATH_MAX];
	int init_speed = 0;
	int fd = 0;
	char* optstr;
	int i = 0;
        
	optind = 0;

	while ((opt=getopt_long(argc, argv, "+i:hv", main_options, NULL)) != -1) {
		switch (opt) {
		case 'v':
			verbose = 1;
			break;
		case 'h':
		default:
			usage();
			return 0;
		}
	}

	argc -= optind;
	argv += optind;
	optind = 0;

	if (argc < 2) {
		usage();
		return 0;
	}

	optstr = argv[0];

	dev[0] = 0;
	strcat(dev, "/dev/");
	strcat(dev, optstr);

        fd = init_chardev(dev);

	argv++;
	argc--;

	fm_send_cmd(fd, argc, argv);
	if (close(fd) < 0) {
		perror("Can't close serial port");
	}

	return 0;
}

