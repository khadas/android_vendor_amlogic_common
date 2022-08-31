/** @file   fw_loader_io_linux.h
 *
 *  @brief  This file contains the function prototypes of procedures that implement
 *          the Nxp specific Helper Protocol for Linux.
 *
 *  Copyright 2014-2020 NXP
 *
 *  This software file (the File) is distributed by NXP
 *  under the terms of the GNU General Public License Version 2, June 1991
 *  (the License).  You may use, redistribute and/or modify the File in
 *  accordance with the terms and conditions of the License, a copy of which
 *  is available by writing to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA or on the
 *  worldwide web at http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt.
 *
 *  THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE
 *  ARE EXPRESSLY DISCLAIMED.  The License provides additional details about
 *  this warranty disclaimer.
 *
 */

/*===================== Include Files ============================================*/
#ifndef FW_LOADER_IO_LINUX_H
#define FW_LOADER_IO_LINUX_H

#include "fw_loader_types.h"
#include <unistd.h>
#include <time.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
/*===================== Macros ===================================================*/

/*==================== Typedefs =================================================*/

/*===================== Global Vars ==============================================*/

/*==================== Function Prototypes ======================================*/
extern uint8 fw_upload_ComReadChar_Linux(int32 iPortID);
extern int8 fw_upload_ComWriteChar_Linux(int32 iPortID, int8 iChar);
extern int8 fw_upload_ComWriteChars_Linux(int32 iPortID, int8 * pChBuffer,
					  uint32 uiLen);
extern int32 fw_upload_ComReadChars_Linux(int32 iPortID, int8 * pChBuffer,
					  int32 uiCount);
extern int32 fw_upload_init_uart_Linux(int8 * dev, int32 iBaudRate,
				       uint8 ucParity, uint8 ucStopBits,
				       uint8 ucByteSize, uint8 ucFlowCtrl);
extern void fw_upload_DelayInMs_Linux(uint32 uiMs);
extern int32 fw_upload_ComGetCTS_Linux(int32 iPortID);
extern uint64 fw_upload_GetTime_Linux(void);
extern int32 fw_upload_ComGetBufferSize_Linux(int32 iPortID);
extern void fw_upload_Flush_Linux(int32 iPortID);
extern void fw_upload_CloseUart_Linux(int32 iPortID);

#endif // FW_LOADER_IO_LINUX_H
