/** @file   fw_loader_io_win.h
 *
 *  @brief  This file contains the function prototypes of procedures that implement
 *          the Nxp specific Helper Protocol for Windows.
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

#ifndef _FW_LOADER_IO_WIN_H
#define _FW_LOADER_IO_WIN_H
/*===================== Include Files ============================================*/
#include "windows.h"
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <memory.h>
#include <direct.h>
#include "fw_loader_types.h"

/*===================== Macros ===================================================*/

/*==================== Typedefs =================================================*/

/*===================== Global Vars ==============================================*/

/*==================== Function Prototypes ======================================*/
extern int32 fw_upload_ComReadChar_Win(int32 iPortID);
extern int8 fw_upload_ComWriteChar_Win(int32 iPortID, int8 iChar);
extern int8 fw_upload_ComWriteChars_Win(int32 iPortID, int8 * pChBuffer,
					uint32 uiLen);
extern int32 fw_upload_ComReadChars_Win(int32 iPortID, int8 * pChBuffer,
					uint32 uiCount);
extern int32 fw_upload_init_uart_Win(int8 * pPortName, int32 iBaudRate,
				     uint8 ucParity, uint8 ucStopBits,
				     uint8 ucByteSize, uint8 ucFlowCtrl);
extern void fw_upload_DelayInMs_Win(uint32 uiMs);
extern int32 fw_upload_ComGetCTS_Win(int32 iPortID);
extern uint64 fw_upload_GetTime_Win(void);
extern int32 fw_upload_ComGetBufferSize_Win(int32 iPortID);
extern void fw_upload_Flush_Win(int32 iPortID);
extern void fw_upload_CloseUart_Win(int32 iPortID);
#endif // #define _FW_LOADER_IO_WIN_H
