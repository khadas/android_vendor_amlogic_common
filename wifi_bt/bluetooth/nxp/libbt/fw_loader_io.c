/******************************************************************************
 *
 *  Copyright 2009-2020 NXP
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

/*===================== Include Files ============================================*/
#include "fw_loader_io.h"

#define LOG_TAG "fw_loader_linux"
#include <cutils/log.h>
#include <cutils/properties.h>
#define perror(fmt, ...) ALOGE("ERROR : %s(L%d): " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

/*===================== Macros ===================================================*/
#define TIMEOUT_SEC 6

/*==================== Typedefs =================================================*/

/*===================== Global Vars ==============================================*/

/*==================== Function Prototypes ======================================*/

/*==================== Coded Procedures =========================================*/

/******************************************************************************
 *
 * Name: fw_upload_lenValid
 *
 * Description:
 *   This function validates the length from 5 bytes request.
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *   ucArray: store the 5 bytes request.
 *
 * Return Value:
 *   uiLenToSend: if the length is valid, get value from ucArray.
 *
 * Notes:
 *   None.
 *

*****************************************************************************/
BOOLEAN fw_upload_lenValid(uint16 *uiLenToSend, uint8 *ucArray) {
  uint16 uiLen, uiLenComp;
  uint16 uiXorOfLen = 0xFFFF;
  uiLen = ((ucArray[1] & 0xFF) | ((ucArray[2] << 8) & 0xFF00));
  uiLenComp = ((ucArray[3] & 0xFF) | ((ucArray[4] << 8) & 0xFF00));
  // LEN valid if len & complement match
  if ((uiLen ^ uiLenComp) == uiXorOfLen)  // All 1's
  {
    *uiLenToSend = uiLen;
    return TRUE;
  } else {
    return FALSE;
  }
}

/******************************************************************************
 *
 * Name: fw_upload_GetDataLen
 *
 * Description:
 *   This function gets buf data length.
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *   *buf: buffer that stores header and following data.
 *
 * Return Value:
 *   length of data part in the buffer.
 *
 * Notes:
 *   None.
 *
 *****************************************************************************/
uint16 fw_upload_GetDataLen(uint8 *buf) {
  return (buf[8] | (buf[9] << 8));
}

/******************************************************************************
 *
 * Name: fw_upload_DelayInMs
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
void fw_upload_DelayInMs(uint32 uiMs) {
  struct timespec ReqTime;
  time_t sec;

  // Initialize to 0
  ReqTime.tv_sec = 0;
  ReqTime.tv_nsec = 0;

  // Calculate the Delay
  sec = (int)(uiMs / 1000);
  uiMs = uiMs - (sec * 1000);
  ReqTime.tv_sec = sec;
  ReqTime.tv_nsec = uiMs * 1000000L;  // 1 ms = 1000000 ns

  // Sleep
  while (nanosleep(&ReqTime, &ReqTime) == -1) {
    continue;
  }
}

/******************************************************************************
 *
 * Name: fw_upload_ComReadChar
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
int32 fw_upload_ComReadChar(int32 mchar_fd) {
  int32 iResult = 0;
  uint8 ucNumCharToRead = 1;

  if (read(mchar_fd, &iResult, ucNumCharToRead) == ucNumCharToRead) {
    return (iResult & 0xFF);
  } else {
    return RW_FAILURE;
  }
}

/******************************************************************************
 *
 * Name: fw_upload_ComReadChars
 *
 * Description:
 *   Read iCount characters from the port specified by nPortID.
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *   mchar_fd   : Port ID.
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
int32 fw_upload_ComReadChars(int32 mchar_fd, uint8* pBuffer, uint32 uiCount) {
  if ((uint32)read(mchar_fd, pBuffer, uiCount) == uiCount) {
    return uiCount;
  } else {
    return RW_FAILURE;
  }
}

/******************************************************************************
 *
 * Name: fw_upload_ComWriteChar
 *
 * Description:
 *   Write a character to the port specified by mchar_fd.
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *   mchar_fd : Port ID.
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
int8 fw_upload_ComWriteChar(int32 mchar_fd, int8 iChar) {
  uint8 ucNumCharToWrite = 1;

  if (write(mchar_fd, &iChar, ucNumCharToWrite) == ucNumCharToWrite) {
    return RW_SUCCESSFUL;
  } else {
    return RW_FAILURE;
  }
}

/******************************************************************************
 *
 * Name: fw_upload_ComWriteChars
 *
 * Description:
 *   Write iLen characters to the port specified by mchar_fd.
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *   mchar_fd : Port ID.
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
int8 fw_upload_ComWriteChars(int32 mchar_fd, uint8* pBuffer, uint32 uiLen) {
  if ((uint32)write(mchar_fd, pBuffer, uiLen) == uiLen) {
    return RW_SUCCESSFUL;
  } else {
    return RW_FAILURE;
  }
}

/******************************************************************************
 *
 * Name: fw_upload_ComGetCTS
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
int32 fw_upload_ComGetCTS(int32 mchar_fd) {
  int32 status;
  ioctl(mchar_fd, TIOCMGET, &status);
  if (status & TIOCM_CTS) {
    return 0;
  } else {
    return 1;
  }
}
/******************************************************************************
 *
 * Name: fw_upload_ComGetBufferSize
 *
 * Description:
 *   Check buffer size
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *   mchar_fd
 *
 * Return Value:
 *   size in buffer
 *
 * Notes:
 *   None.
 *
 *****************************************************************************/
int32 fw_upload_GetBufferSize(int32 mchar_fd) {
  int32 bytes = 0;
  ioctl(mchar_fd, FIONREAD, &bytes);
  return bytes;
}

/******************************************************************************
 *
 * Name: fw_upload_GetTime
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

double fw_upload_GetTime(void) {
  struct timeval time;
  double millsectime;
  gettimeofday(&time, NULL);
  millsectime = (time.tv_sec * 1000000 + time.tv_usec) / 1000;
  return millsectime;
}

