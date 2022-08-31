/** @file   fw_loader_io_linux.c
 *
 *  @brief  This file contains the functions that implement the Nxp specific
 *          Helper Protocol for Linux.
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
#include <stdio.h>
#include <stdlib.h>
#include "fw_loader_io_linux.h"

/*===================== Macros ===================================================*/
#define TIMEOUT_SEC             6
#define TIMEOUT_FOR_READ        2000
#define USE_SELECT
#define FAILURE 1000001
/*==================== Typedefs =================================================*/

/*===================== Global Vars ==============================================*/

/*==================== Function Prototypes ======================================*/

/*==================== Coded Procedures =========================================*/
/******************************************************************************
 *
 * Name: fw_upload_DelayInMs_Linux
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
fw_upload_DelayInMs_Linux(uint32 uiMs)
{
	struct timespec ReqTime;
	time_t sec;

	// Initialize to 0
	ReqTime.tv_sec = 0;
	ReqTime.tv_nsec = 0;

	// Calculate the Delay
	sec = (time_t) (uiMs / 1000);
	uiMs = uiMs - (sec * 1000);
	ReqTime.tv_sec = sec;
	ReqTime.tv_nsec = uiMs * 1000000L;	// 1 ms = 1000000 ns  

	// Sleep 
	while (nanosleep(&ReqTime, &ReqTime) == -1) {
		continue;
	}
}

/******************************************************************************
 *
 * Name: fw_upload_ComReadChar_Linux
 *
 * Description:
 *   Read a character from the port specified by nPortID.     
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *   nPortID : Port ID.
 *
 * Return Value:
 *   Returns the character, if Successful.
 *   Returns -1 if no character available (OR TIMED-OUT)
 *
 * Notes:
 *   None.
 *
 *****************************************************************************/
uint8
fw_upload_ComReadChar_Linux(int32 iPortID)
{
	int32 iResult = 0;
	uint8 ucNumCharToRead = 1;

#ifdef USE_SELECT
	fd_set rd;
	struct timeval tv;
#else
	uint64 uTimeOut = TIMEOUT_FOR_READ;
	uint64 uEndTime;
#endif

#ifdef USE_SELECT
	FD_ZERO(&rd);
	FD_SET(iPortID, &rd);
	tv.tv_sec = TIMEOUT_FOR_READ / 1000;
	tv.tv_usec = 0;

	if (select(iPortID + 1, &rd, NULL, NULL, &tv) < 0)
		perror("select error in fw_upload_ComReadChar_Linux!\n");
	else if (FD_ISSET(iPortID, &rd)) {
		if (read(iPortID, &iResult, ucNumCharToRead) == ucNumCharToRead) {
			return (iResult & 0xFF);
		}
	}
#else
	uEndTime = fw_upload_GetTime_Linux() + uTimeOut;
	do {
		if (read(iPortID, &iResult, ucNumCharToRead) == ucNumCharToRead) {
			return (iResult & 0xFF);
		}
	} while (uEndTime > fw_upload_GetTime_Linux());

#endif

	return RW_FAILURE;
}

/******************************************************************************
 *
 * Name: fw_upload_ComReadChars_Linux
 *
 * Description:
 *   Read iCount characters from the port specified by nPortID.     
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *   iPortID   : Port ID.
 *   pBuffer : Destination buffer for the characters read 
 *   iCount    : Number of Characters to be read.
 *
 * Return Value:
 *   Returns the number of characters read if Successful.
 *   Returns -1 if iCount characters could not be read or if Port ID is invalid.
 *
 * Notes:
 *   None.
 *
 *****************************************************************************/
int32
fw_upload_ComReadChars_Linux(int32 iPortID, int8 * pBuffer, int32 uiCount)
{

#ifdef USE_SELECT
	fd_set rd;
	struct timeval tv;
	int32 rdCount, retCount, nLoop;
	int8 *rdPtr;
	int ret;
#else
	uint64 uEndTime;
#endif

	uint64 uTimeOut = TIMEOUT_FOR_READ;

#ifdef USE_SELECT
	FD_ZERO(&rd);
	FD_SET(iPortID, &rd);
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	rdPtr = pBuffer;
	rdCount = uiCount;
	nLoop = 0;

	while (1) {
		ret = select(iPortID + 1, &rd, NULL, NULL, &tv);
		if (ret < 0) {
			perror("select error in fw_upload_ComReadChars_Linux!\n");
			break;
		}

		/* Select Timeout */
		if (ret == 0) {
			nLoop++;
			if (nLoop > uTimeOut / 1000)
				break;
		}

		if (FD_ISSET(iPortID, &rd)) {
			retCount = read(iPortID, rdPtr, rdCount);
			if (retCount < rdCount) {
				rdCount -= retCount;
				rdPtr += retCount;
			} else {
				return uiCount;
			}
		}
	}
#else

	uEndTime = fw_upload_GetTime_Linux() + uTimeOut;

	do {
		if (fw_upload_ComGetBufferSize_Linux(iPortID) >= uiCount) {
			uiCount = read(iPortID, pBuffer, uiCount);
			return uiCount;
		}
	} while (uEndTime > fw_upload_GetTime_Linux());
#endif
	return RW_FAILURE;
}

/******************************************************************************
 *
 * Name: fw_upload_ComWriteChar_Linux
 *
 * Description:
 *   Write a character to the port specified by iPortID.     
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *   iPortID : Port ID.
 *   iChar   : Character to be written
 *
 * Return Value:
 *   Returns TRUE, if write is Successful.
 *   Returns FALSE if write is a failure.
 *
 * Notes:
 *   None.
 *
 *****************************************************************************/
int8
fw_upload_ComWriteChar_Linux(int32 iPortID, int8 iChar)
{
	uint8 ucNumCharToWrite = 1;

	if (iPortID > 1000000) {
		return RW_FAILURE;
	}
	if (write(iPortID, &iChar, ucNumCharToWrite) == ucNumCharToWrite) {
		return RW_SUCCESSFUL;
	} else {
		return RW_FAILURE;
	}
}

/******************************************************************************
 *
 * Name: fw_upload_ComWriteChars_Linux
 *
 * Description:
 *   Write iLen characters to the port specified by iPortID.     
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *   iPortID : Port ID.
 *   pBuffer : Buffer where characters are available to be written to the Port.
 *   iLen    : Number of Characters to write.
 *
 * Return Value:
 *   Returns TRUE, if write is Successful.
 *   Returns FALSE if write is a failure.
 *
 * Notes:
 *   None.
 *
 *****************************************************************************/
int8
fw_upload_ComWriteChars_Linux(int32 iPortID, int8 * pBuffer, uint32 uiLen)
{
	if (write(iPortID, pBuffer, uiLen) == uiLen) {
		return RW_SUCCESSFUL;
	} else {
		return RW_FAILURE;
	}
}

// UART Set up Related Functions.
// adapted from hciattach.c (bluez-util-3.7)
/******************************************************************************
 *
 * Name: fw_upload_uart_speed
 *
 * Description:
 *   Return the baud rate corresponding to the frequency.     
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

static int32
fw_upload_uart_speed(int32 s)
{
	switch (s) {
	case 9600:
		return B9600;
		break;
	case 19200:
		return B19200;
		break;
	case 38400:
		return B38400;
		break;
	case 57600:
		return B57600;
		break;
	case 115200:
		return B115200;
		break;
#ifdef B230400
	case 230400:
		return B230400;
		break;
#endif // B230400
#ifdef B460800
	case 460800:
		return B460800;
		break;
#endif // B460800
#ifdef B500000
	case 500000:
		return B500000;
		break;
#endif // B500000
#ifdef B576000
	case 576000:
		return B576000;
		break;
#endif // B576000
#ifdef B921600
	case 921600:
		return B921600;
		break;
#endif // B921600
#ifdef B1000000
	case 1000000:
		return B1000000;
		break;
#endif // B1000000
#ifdef B1152000
	case 1152000:
		return B1152000;
		break;
#endif // B1152000
#ifdef B1500000
	case 1500000:
		return B1500000;
		break;
#endif // B1500000
#ifdef B3000000
	case 3000000:
		return B3000000;
		break;
#endif // B3000000
	default:
		return B0;
		break;
	}
}

// adapted from hciattach.c (bluez-util-3.7)
/******************************************************************************
 *
 * Name: fw_upload_set_speed
 *
 * Description:
 *   Set the baud rate speed.
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
static int32
fw_upload_set_speed(int32 fd, struct termios *ti, int32 speed)
{
	cfsetospeed(ti, fw_upload_uart_speed(speed));
	return tcsetattr(fd, TCSANOW, ti);
}

/******************************************************************************
 *
 * Name: fw_upload_ComGetCTS_Linux
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
 *
 * Notes:
 *   None.
 *
 *****************************************************************************/
int32
fw_upload_ComGetCTS_Linux(int32 iPortID)
{
	int32 status = 0;
	ioctl(iPortID, TIOCMGET, &status);
	if (status & TIOCM_CTS) {
		return 0;
	} else {
		return 1;
	}
}

/******************************************************************************
 *
 * Name: fw_upload_ComGetBufferSize_Linux
 *
 * Description:
 *   Check buffer size
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *   iPortID
 *
 * Return Value:
 *   size in buffer
 *
 * Notes:
 *   None.
 *
 *****************************************************************************/
int32
fw_upload_ComGetBufferSize_Linux(int32 iPortID)
{
	int32 bytes = 0;
	ioctl(iPortID, FIONREAD, &bytes);
	return bytes;
}

/******************************************************************************
 *
 * Name: fw_upload_Flush_Linux
 *
 * Description:
 *   flush buffer
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *   iPortID
 *
 * Return Value:
 *   None.
 *
 * Notes:
 *   None.
 *
 *****************************************************************************/
void
fw_upload_Flush_Linux(int32 iPortID)
{
	tcflush(iPortID, TCIFLUSH);
}

/******************************************************************************
 *
 * Name: fw_upload_GetTime_Linux
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
fw_upload_GetTime_Linux(void)
{
	int8 retValue;
	struct timespec time;
	uint64 millsectime;

	retValue = clock_gettime(CLOCK_MONOTONIC, &time);
	if (retValue == -1) {
		perror("clock gettime");
		exit(-1);
	}
	millsectime =
		(((uint64) time.tv_sec * 1000 * 1000 * 1000) +
		 time.tv_nsec) / 1000000;
	return millsectime;

}

// adapted from hciattach.c (bluez-util-3.7)
/******************************************************************************
 *
 * Name: fw_upload_init_uart_Linux
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
int32
fw_upload_init_uart_Linux(int8 * dev, int32 dwBaudRate, uint8 ucParity,
			  uint8 ucStopBits, uint8 ucByteSize, uint8 ucFlowCtrl)
{
	struct termios ti;
	int32 fd = open(dev, O_RDWR | O_NOCTTY);

	if (fd < 0) {
		perror("Can't open serial port");
		return FAILURE;
	}
#if defined(W9098)
	tcsendbreak(fd, 0);
	usleep(500000);
#endif
	//tcflush(fd, TCIOFLUSH);

	if (tcgetattr(fd, &ti) < 0) {
		perror("Can't get port settings");
		close(fd);
		return FAILURE;
	}
	tcflush(fd, TCIOFLUSH);
	cfmakeraw(&ti);
	ti.c_cflag |= CLOCAL | CREAD;

	// Set 1 stop bit & no parity (8-bit data already handled by cfmakeraw)
	ti.c_cflag &= ~(CSTOPB | PARENB);

#ifdef CRTSCTS
	if (ucFlowCtrl) {
		ti.c_cflag |= CRTSCTS;
	} else {
		ti.c_cflag &= ~CRTSCTS;
	}
#else
	if (ucFlowCtrl) {
		ti.c_cflag |= IHFLOW;
		ti.c_cflag |= OHFLOW;
	} else {
		ti.c_cflag &= ~IHFLOW;
		ti.c_cflag &= ~OHFLOW;
	}
#endif

	//FOR READS:  set timeout time w/ no minimum characters needed (since we read only 1 at at time...)
	ti.c_cc[VMIN] = 0;
	ti.c_cc[VTIME] = TIMEOUT_SEC * 10;

	tcflush(fd, TCIOFLUSH);

	if (tcsetattr(fd, TCSANOW, &ti) < 0) {
		perror("Can't set port settings");
		close(fd);
		return FAILURE;
	}
	tcflush(fd, TCIOFLUSH);

	/* Set actual baudrate */
	if (fw_upload_set_speed(fd, &ti, dwBaudRate) < 0) {
		perror("Can't set baud rate");
		close(fd);
		return FAILURE;
	}

	return fd;
}

/******************************************************************************
 *
 * Name: fw_upload_CloseUart_Linux
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
fw_upload_CloseUart_Linux(int32 iPortID)
{
	if (iPortID > 1000000) {
		exit(-1);
	}

	close(iPortID);
}
