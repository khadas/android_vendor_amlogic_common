/** @file   fw_loader_io_win.c
 *
 *  @brief  This file contains the functions that implement the Nxp specific
 *          Helper Protocol for Windows.
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
#include "fw_loader_io_win.h"
/*===================== Macros ===================================================*/
#define TIMEOUT_FOR_READ        2000

/*==================== Typedefs =================================================*/

/*===================== Global Vars ==============================================*/

/*==================== Function Prototypes ======================================*/

/*==================== Coded Procedures =========================================*/
/******************************************************************************
 *
 * Name: fw_upload_DelayInMs_Win
 *
 * Description:
 *   This function delays the execution of the program for the time
 *   specified in uiMs.
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *   uiMs - Delay in Milliseconds.
 *
 * Return Value:
 *   None.
 *
 * Notes:
 *   None.
 *
 *****************************************************************************/
void
fw_upload_DelayInMs_Win(uint32 uiMs)
{
	// Call the Sleep Function provided by Windows
	// Note: The Sleep function takes Millisecond units
	Sleep(uiMs);
}

/******************************************************************************
 *
 * Name: fw_upload_ComReadChar_Win
 *
 * Description:
 *   Read a character from the port specified by iPortID.
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *   None.
 *
 * Return Value:
 *   Returns the character, if Successful.
 *   Returns RW_FAILURE if no character available (OR TIMED-OUT)
 *
 * Notes:
 *   None.
 *
 *****************************************************************************/
int32
fw_upload_ComReadChar_Win(int32 iPortID)
{
	uint32 uibuf;
	uint64 uEndTime;
	int32 iResult = 0;
	uint8 ucNumCharToRead = 1;
	uint64 uTimeOut = TIMEOUT_FOR_READ;

	uEndTime = fw_upload_GetTime_Win() + uTimeOut;

	// Read from the com port.
	// Note: Parameters 4 & 5 - uiBuf and NULL are inconsequential - Optional
	// parameters.
	// uiBuf - A pointer to the variable that receives the number of bytes read
	// when using a synchronous hFile parameter.
	// Parameter 5 - A pointer to an  OVERLAPPED structure is required if the iPortID parameter
	// was opened with FILE_FLAG_OVERLAPPED, otherwise it can be NULL.
	do {
		if (ReadFile
		    ((HANDLE) iPortID, &iResult, ucNumCharToRead, &uibuf,
		     NULL)) {
			return (iResult & 0xFF);
		}
	} while (uEndTime > fw_upload_GetTime_Win());
	ClearCommError((HANDLE) iPortID, NULL, NULL);
	return RW_FAILURE;
}

/******************************************************************************
 *
 * Name: fw_upload_ComReadChars_Win
 *
 * Description:
 *   Read iCount characters from the port specified by iPortID.
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *   pBuffer   : Destination buffer for the characters read
 *   iCount    : Number of Characters to be read.
 *
 * Return Value:
 *   Returns the number of characters read if Successful.
 *   Returns RW_FAILURE if iCount characters could not be read or if Port ID is invalid.
 *
 * Notes:
 *   None.
 *
 *****************************************************************************/
int32
fw_upload_ComReadChars_Win(int32 iPortID, int8 * pBuffer, uint32 uiCount)
{
	uint64 uEndTime;
	uint32 uiRetBytes = 0;
	uint64 uTimeOut = TIMEOUT_FOR_READ;

	uEndTime = fw_upload_GetTime_Win() + uTimeOut;

	// Read from the com port.
	// Parameter 5 - A pointer to an  OVERLAPPED structure is required if the iPortID parameter
	// was opened with FILE_FLAG_OVERLAPPED, otherwise it can be NULL.
	do {
		if (fw_upload_ComGetBufferSize_Win(iPortID) >= (int32) uiCount) {
			if (ReadFile
			    ((HANDLE) iPortID, pBuffer, uiCount, &uiRetBytes,
			     NULL) && uiRetBytes == uiCount) {
				// Read Successful. Return the number of characters read.
				return uiCount;
			}
		}
	} while (uEndTime > fw_upload_GetTime_Win());

	ClearCommError((HANDLE) iPortID, NULL, NULL);
	return RW_FAILURE;
}

/******************************************************************************
 *
 * Name: fw_upload_ComWriteChar_Win
 *
 * Description:
 *   Write a character to the port specified by iPortID.
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *   iChar   : Character to be written
 *
 * Return Value:
 *   Returns success or failure of the Write.
 *
 * Notes:
 *   None.
 *
 *****************************************************************************/
int8
fw_upload_ComWriteChar_Win(int32 iPortID, int8 iChar)
{
	uint32 uibuf;
	uint8 ucNumCharToWrite = 1;

	// Write 1 char to the com port.
	// Note: Parameters 4 & 5 - uiBuf and NULL are inconsequential - Optional
	// parameters.
	// uiBuf - A pointer to the variable that receives the number of bytes written
	// when using a synchronous iPortID parameter. In this case,
	// it will be 1 after the write.
	// Parameter 5 - A pointer to an  OVERLAPPED structure is required if the
	// iPortID parameter was opened with FILE_FLAG_OVERLAPPED, otherwise it can be NULL.
	if (!WriteFile
	    ((HANDLE) iPortID, &iChar, ucNumCharToWrite, &uibuf, NULL)) {
		ClearCommError((HANDLE) iPortID, NULL, NULL);
		return RW_FAILURE;
	}
	return RW_SUCCESSFUL;
}

/******************************************************************************
 *
 * Name: fw_upload_ComWriteChars_Win
 *
 * Description:
 *   Write iLen characters to the port specified by iPortID.
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *   pBuffer : Buffer where characters are available to be written to the Port.
 *   iLen    : Number of Characters to write.
 *
 * Return Value:
 *   Returns success or failure of the Write.
 *
 * Notes:
 *   None.
 *
 *****************************************************************************/
int8
fw_upload_ComWriteChars_Win(int32 iPortID, int8 * pBuffer, uint32 uiLen)
{
	uint32 uibuf;

	// Write to the com port.
	// Parameter 5 - A pointer to an  OVERLAPPED structure is required if the iPortID parameter
	// was opened with FILE_FLAG_OVERLAPPED, otherwise it can be NULL.
	if (!WriteFile((HANDLE) iPortID, pBuffer, uiLen, &uibuf, NULL)) {
		ClearCommError((HANDLE) iPortID, NULL, NULL);
		return RW_FAILURE;
	}
	return RW_SUCCESSFUL;
}

/******************************************************************************
 *
 * Name: fw_upload_ComGetCTS_Win
 *
 * Description:
 *   Check CTS status
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *
 * Return Value:
 *   return the status of CTS
 *
 * Notes:
 *   None.
 *
 
*****************************************************************************/

int32
fw_upload_ComGetCTS_Win(int32 iPortID)
{
	COMSTAT lpStat;

	ClearCommError((HANDLE) iPortID, NULL, &lpStat);
	if (lpStat.fCtsHold == 1) {
		return 1;
	} else {
		return 0;
	}
}

/******************************************************************************
 *
 * Name: fw_upload_GetTime_Win
 *
 * Description:
 *   Get the current time
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *
 * Return Value: 
 *   return the current time
 *
 * Notes:
 *   None.
 *
 *****************************************************************************/

uint64
fw_upload_GetTime_Win(void)
{
	uint64 time;
	time = GetTickCount();
	return time;

}

/******************************************************************************
 *
 * Name: fw_upload_ComGetBufferSize_Win
 *
 * Description:
 *   get buffer size.
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *   iPortID
 *
 * Return Value:
 *   buffer size.
 * Notes:
 *   None.
 *
 *****************************************************************************/
int32
fw_upload_ComGetBufferSize_Win(int32 iPortID)
{
	int32 bytes = 0;
	DWORD dwErrors;
	COMSTAT Rcs;

	ClearCommError((HANDLE) iPortID, &dwErrors, &Rcs);
	bytes = Rcs.cbInQue;

	return bytes;
}

/******************************************************************************
 *
 * Name: fw_upload_Flush_Win
 *
 * Description:
 *   flush buffer.
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *   iPortID
 *
 * Return Value:
 *
 * Notes:
 *   None.
 *
 *****************************************************************************/
void
fw_upload_Flush_Win(int32 iPortID)
{
	PurgeComm((HANDLE) iPortID, PURGE_RXCLEAR);
}

/******************************************************************************
 *
 * Name: fw_upload_init_uart_Win
 *
 * Description:
 *   Initialize UART.
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *
 * Return Value:
 *
 * Notes:
 *   None.
 *
 *****************************************************************************/
extern int32
fw_upload_init_uart_Win(int8 * pPortName, int32 iBaudRate,
			uint8 ucParity, uint8 ucStopBits,
			uint8 ucByteSize, uint8 ucFlowCtrl)
{
	HANDLE myport;
	DCB dcb = { 0 };
	COMMTIMEOUTS timeouts;
	HANDLE iresult = (void *)-1;
	char aucPortName[32];

	//for COM port
	sprintf(aucPortName, "\\\\.\\%s", pPortName);

	// Open the COM Port
	myport = CreateFile(aucPortName, GENERIC_READ | GENERIC_WRITE,	//access ( read and write)
			    0,	//(share) 0:cannot share the COM port
			    0,	//security  (None)
			    OPEN_EXISTING,	// creation : open_existing
			    FILE_ATTRIBUTE_NORMAL,	// we want overlapped operation
			    0	// no templates file for COM port...
		);
	if (myport == INVALID_HANDLE_VALUE) {
		return (int32) iresult;
	}
	// Now start to read but first we need to set the COM port settings and the timeouts
	if (!SetCommMask(myport, EV_RXCHAR | EV_TXEMPTY)) {
		//ASSERT(0);
		return (int32) iresult;
	}
	// Now we need to set baud rate etc,
	dcb.DCBlength = sizeof(DCB);

	if (!GetCommState(myport, &dcb)) {
		return (int32) iresult;
	}

	dcb.BaudRate = iBaudRate;
	dcb.ByteSize = ucByteSize;
	dcb.Parity = ucParity;
	if (ucStopBits == 1) {
		dcb.StopBits = ONESTOPBIT;
	} else if (ucStopBits == 2) {
		dcb.StopBits = TWOSTOPBITS;
	} else {
		dcb.StopBits = ONE5STOPBITS;
	}

	dcb.fDsrSensitivity = 0;
	dcb.fDtrControl = DTR_CONTROL_DISABLE;
	if (ucFlowCtrl) {
		dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
		dcb.fOutxCtsFlow = 1;
	} else {
		dcb.fRtsControl = RTS_CONTROL_DISABLE;
		dcb.fOutxCtsFlow = 0;
	}
	dcb.fOutxDsrFlow = 0;

	if (!SetCommState(myport, &dcb)) {
		return (int32) iresult;
	}
	// Now set the timeouts ( we control the timeout overselves using WaitForXXX()
	timeouts.ReadIntervalTimeout = MAXDWORD;
	timeouts.ReadTotalTimeoutMultiplier = 0;
	timeouts.ReadTotalTimeoutConstant = 0;
	timeouts.WriteTotalTimeoutMultiplier = 0;
	timeouts.WriteTotalTimeoutConstant = 0;

	if (!SetCommTimeouts(myport, &timeouts)) {
		return (int32) iresult;
	}
	return (int32) myport;
}

/******************************************************************************
 *
 * Name: fw_upload_CloseUart_Win
 *
 * Description:
 *   Close Uart.
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *
 * Return Value:
 *   None.
 *
 * Notes:
 *   None.
 *
 
*****************************************************************************/
void
fw_upload_CloseUart_Win(int32 iPortID)
{
	HANDLE myport = (HANDLE) iPortID;
	if (myport) {
		CloseHandle(myport);
	}
}
