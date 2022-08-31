/** @file   fw_loader_uart.h
 *
 *  @brief  This file contains the function prototypes of the Nxp specific
 *          Helper Protocol.
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

#ifndef FW_LOADER_H
#define FW_LOADER_H
/*===================== Include Files ============================================*/
#include "fw_loader_types.h"

/*===================== Macros ===================================================*/
#define FW_LOADER_WIN   1
#define FW_LOADER_LINUX 0

// Note: _WIN32 or _WIN64 are pre-defined macros
// for Windows applications.
#if defined(_WIN32) || defined(_WIN64)
#define OS_TYPE   FW_LOADER_WIN
#include "fw_loader_io_win.h"
#else // Linux
#define OS_TYPE   FW_LOADER_LINUX
#include "fw_loader_io_linux.h"
#endif // defined (__WIN32) || defined (__WIN64)

typedef struct {
	uint32 iBaudRate;
	uint32 iUartDivisor;
	uint32 iClkDivisor;
} UART_BAUDRATE;

/*==================== Typedefs =================================================*/

/*===================== Global Vars ==============================================*/

/*==================== Function Prototypes ======================================*/
uint8(*fw_upload_ComReadChar) (int32 iPortID);
int8(*fw_upload_ComWriteChar) (int32 iPortID, int8 iChar);
int8(*fw_upload_ComWriteChars) (int32 iPortID, int8 * pChBuffer, uint32 uiLen);
int32(*fw_upload_ComReadChars) (int32 iPortID, int8 * pChBuffer, int32 uiCount);
int32(*fw_upload_init_uart) (int8 * pPortName, int32 iBaudRate, uint8 ucParity,
			     uint8 ucStopBits, uint8 ucByteSize,
			     uint8 ucFlowCtrl);
void (*fw_upload_DelayInMs) (uint32 uiMs);
int32(*fw_upload_ComGetCTS) (int32 iPortID);
uint64(*fw_upload_GetTime) (void);
int32(*fw_upload_GetBufferSize) (int32 iPortID);
void (*fw_upload_Flush) (int32 iPortID);
void (*fw_upload_CloseUart) (int32 iPortID);
void fw_upload_gen_crc_table(void);
unsigned long fw_upload_update_crc(unsigned long crc_accum, char *data_blk_ptr,
				   int data_blk_size);
BOOLEAN fw_upload_Check_ReqCrc(uint8 * uiStr, uint8 uiReq);
BOOLEAN fw_upload_lenValid(uint16 * uiLenToSend, uint8 * ucArray);
void closeFileorDescriptor(int fileDescriptor);
void fw_upload_io_func_init(void);
static void init_crc8(void);
extern uint8 fw_upload_ComReadChar_Linux(int32 iPortID);
extern int32 fw_upload_ComReadChar_Win(int32 iPortID);
extern void fw_upload_CloseUart_Linux(int32 iPortID);
extern void fw_upload_CloseUart_Win(int32 iPortID);
extern int8 fw_upload_ComWriteChars_Linux(int32 iPortID, int8 * pChBuffer,
					  uint32 uiLen);
extern int8 fw_upload_ComWriteChars_Win(int32 iPortID, int8 * pChBuffer,
					uint32 uiLen);
extern int32 fw_upload_ComReadChars_Linux(int32 iPortID, int8 * pChBuffer,
					  int32 uiCount);
extern int32 fw_upload_ComReadChars_Win(int32 iPortID, int8 * pChBuffer,
					uint32 uiCount);
extern int32 fw_upload_init_uart_Linux(int8 * dev, int32 iBaudRate,
				       uint8 ucParity, uint8 ucStopBits,
				       uint8 ucByteSize, uint8 ucFlowCtrl);
extern int32 fw_upload_init_uart_Win(int8 * pPortName, int32 iBaudRate,
				     uint8 ucParity, uint8 ucStopBits,
				     uint8 ucByteSize, uint8 ucFlowCtrl);
extern void fw_upload_DelayInMs_Linux(uint32 uiMs);
extern void fw_upload_DelayInMs_Win(uint32 uiMs);
extern int32 fw_upload_ComGetCTS_Linux(int32 iPortID);
extern int32 fw_upload_ComGetCTS_Win(int32 iPortID);
extern uint64 fw_upload_GetTime_Linux(void);
extern uint64 fw_upload_GetTime_Win(void);
extern int8 fw_upload_ComWriteChar_Linux(int32 iPortID, int8 iChar);
extern int8 fw_upload_ComWriteChar_Win(int32 iPortID, int8 iChar);
extern void fw_upload_Flush_Linux(int32 iPortID);
extern void fw_upload_Flush_Win(int32 iPortID);
extern int32 fw_upload_ComGetBufferSize_Linux(int32 iPortID);
extern int32 fw_upload_ComGetBufferSize_Win(int32 iPortID);
#endif // FW_LOADER_H
