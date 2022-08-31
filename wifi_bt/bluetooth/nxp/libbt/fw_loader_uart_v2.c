/******************************************************************************
 *
 *  Copyright 2009-2021 NXP
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
#include "fw_loader_uart_v2.h"
#include "fw_loader_io.h"
#include <errno.h>
#include <memory.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <malloc.h>

#define LOG_TAG "fw_loader"
#include <cutils/log.h>
#include <cutils/properties.h>
#define printf(fmt, ...) ALOGE("ERROR : %s(L%d): " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

/*--------------------------------fw_loader_io_linux.c-------------------------*/
#define TIMEOUT_FOR_READ        4000


/*===================== Macros ===================================================*/
#define VERSION "M206"
#define MAX_LENGTH 0xFFFF  // Maximum 2 byte value
#define END_SIG_TIMEOUT 2500
#define MAX_CTS_TIMEOUT    5000         //5s 
#define STRING_SIZE 6
#define HDR_LEN 16
#define CMD4 0x4
#define CMD6 0x6
#define CMD7 0x7

#define DELAY_CMD5_PATCH   250   //250ms
#define POLL_AA_TIMEOUT    200
#define TIMEOUT_VAL_MILLISEC  4000       // Timeout for getting 0xa5 or 0xaa or 0xa6, 2 times of helper timeout

#define BOOT_HEADER        0xa5
#define BOOT_HEADER_ACK    0x5a
#define HELPER_HEADER      0xa6
#define HELPER_HEADR_ACK   0x6a
#define HELPER_TIMEOUT_ACK 0x6b
#define VERSION_HEADER     0xaa

#define PRINT(...)         printf(__VA_ARGS__)

/*==================== Typedefs =================================================*/

/*===================== Global Vars ==============================================*/
// Maximum Length that could be asked by the Helper = 2 bytes
static uint8 ucByteBuffer[MAX_LENGTH];

// Size of the File to be downloaded
static uint32 ulTotalFileSize = 0;

// Current size of the Download
static uint32 ulCurrFileSize = 0;
static uint32 ulLastOffsetToSend = 0xFFFF;
static BOOLEAN uiErrCase = FALSE;
static BOOLEAN uiReDownload = FALSE;

// Received Header
static uint8   ucRcvdHeader = 0xFF;
static BOOLEAN ucHelperOn = FALSE;
static uint8   ucString[STRING_SIZE];
static uint8   ucCmd5Sent = 0;
static BOOLEAN b16BytesData = FALSE;

//Handler of File
static FILE* pFile = NULL;

// CMD5 patch to change bootloader timeout to 2 seconds
uint8 ucCmd5Patch[28] = {0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x9D, 0x32, 0xBB, 0x11,
                         0x2C, 0x94, 0x00, 0xA8, 0xEC, 0x70, 0x02, 0x00, 0xB4, 0xD9, 0x9D, 0x26};

jmp_buf resync;  // Protocol restart buffer used in timeout cases.
/*==================== Function Prototypes ======================================*/

/*==================== Coded Procedures =========================================*/

/******************************************************************************
 *
 * Name: fw_upload_WaitForHeaderSignature()
 *
 * Description:
 *   This function basically waits for reception
 *   of character 0xa5 on UART Rx. If no 0xa5 is 
 *   received, it will kind of busy wait checking for
 *   0xa5.
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *   uiMs:   the expired time.
 *
 * Return Value:
 *   TRUE:   0xa5 or 0xaa or 0xa6 is received.
 *   FALSE:  0xa5 or 0xaa or 0xa6 is not received.
 *
 * Notes:
 *   None.
 *
 *****************************************************************************/ 
static BOOLEAN fw_upload_WaitForHeaderSignature(uint32 uiMs)
{
  uint8 ucDone = 0; // signature not Received Yet.
  uint64 startTime = 0;
  uint64 currTime = 0;
  BOOLEAN bResult = TRUE;
  ucRcvdHeader = 0xFF;
  startTime = fw_upload_GetTime();
  while (!ucDone)
  { 
    fw_upload_ComReadChars(mchar_fd, (uint8 *)&ucRcvdHeader,1);
    if ((ucRcvdHeader == BOOT_HEADER) || (ucRcvdHeader == VERSION_HEADER) || (ucRcvdHeader == HELPER_HEADER))
    {
      ucDone = 1;
#ifdef DEBUG_PRINT
      PRINT("\nReceived 0x%x ", ucRcvdHeader);
#endif
    }
    else
    {
      if(uiMs)
      {
        currTime = fw_upload_GetTime();
        if(currTime - startTime > uiMs)
        {
          bResult = FALSE;
          break;
        }
      }
      fw_upload_DelayInMs(1);
    }
  }
  return bResult;
}

/******************************************************************************
 *
 * Name: fw_upload_WaitFor_Len
 *
 * Description:
 *   This function waits to receive the 4 Byte length.
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *   None.
 *
 * Return Value:
 *   2 Byte Length to send back to the Helper.
 *
 * Notes:
 *   None.
 *
 *****************************************************************************/ 
static uint16 fw_upload_WaitFor_Len(FILE* pFile)
{ 
  uint8  uiVersion; 
  // Length Variables
  uint16 uiLen = 0x0;
  uint16 uiLenComp = 0x0;
  // uiLen and uiLenComp are 1's complement of each other. 
  // In such cases, the XOR of uiLen and uiLenComp will be all 1's
  // i.e 0xffff.
  uint16 uiXorOfLen = 0xFFFF;

  // Read the Lengths.
  fw_upload_ComReadChars(mchar_fd, (uint8 *)&uiLen, 2);
  fw_upload_ComReadChars(mchar_fd, (uint8 *)&uiLenComp, 2);

  // Check if the length is valid.
  if ((uiLen ^ uiLenComp) == uiXorOfLen) // All 1's
  {
#ifdef DEBUG_PRINT
    PRINT("\n bootloader asks for %d bytes \n ", uiLen);
#endif
    // Successful. Send back the ack.
    if ((ucRcvdHeader == BOOT_HEADER) || (ucRcvdHeader == VERSION_HEADER))
    {
      fw_upload_ComWriteChar(mchar_fd, (int8)BOOT_HEADER_ACK);
	  if (ucRcvdHeader == VERSION_HEADER)
      {
        // We have received the Chip Id and Rev Num that the 
        // helper intended to send. Ignore the received
	// Chip Id, Rev Num and proceed to Download.
	uiVersion = (uiLen >> 8) & 0xF0;
	uiVersion = uiVersion >> 4;
	PRINT("Helper Version is: %d\n ", uiVersion);
	if(ucHelperOn == TRUE)
	{
	  fseek(pFile, 0, SEEK_SET);
	  ulCurrFileSize = 0; 
          ulLastOffsetToSend = 0xFFFF;
	}
        // Ensure any pending write data is completely written
        if (0 == tcdrain(mchar_fd))
        {
#ifdef DEBUG_PRINT
          PRINT("\n\t tcdrain succeeded\n");
#endif
        }
        else
        {
#ifdef DEBUG_PRINT
          PRINT("\n\t Version ACK, tcdrain failed with errno = %d\n", errno);
#endif
        }

        longjmp(resync, 1);
      }
    }
  }
  else
  {
#ifdef DEBUG_PRINT
    PRINT("\n    NAK case: bootloader LEN = %x bytes \n ", uiLen);
    PRINT("\n    NAK case: bootloader LENComp = %x bytes \n ", uiLenComp);
#endif
    // Failure due to mismatch.	  
    fw_upload_ComWriteChar(mchar_fd, (int8)0xbf);
    // Start all over again.
    longjmp(resync, 1);
  }
  return uiLen;
}

/******************************************************************************
 *
 * Name: fw_upload_GetHeaderStartBytes
 *
 * Description:
 *   This function gets 0xa5 and it's following 4 bytes length.    
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *   None.
 *
 * Return Value:
 *   None.
 *
 * Notes:
 *   None.
 *
 *****************************************************************************/ 
static void fw_upload_GetHeaderStartBytes(uint8 *ucStr)
{
  BOOLEAN ucDone = FALSE, ucStringCnt = 0, i;
  
  while (!ucDone)
  { 
    ucRcvdHeader = 0xFF;
    fw_upload_ComReadChars(mchar_fd, (uint8 *)&ucRcvdHeader,1);

    if(ucRcvdHeader == BOOT_HEADER)
    {
      ucStr[ucStringCnt++] = ucRcvdHeader;
      ucDone = TRUE;
#ifdef DEBUG_PRINT
      PRINT("\nReceived 0x%x\n ", ucRcvdHeader);
#endif
    }
    else
    {
      fw_upload_DelayInMs(1);
    }
  }
  while(!fw_upload_GetBufferSize(mchar_fd));
  for(i = 0; i < 4; i ++)
  {
    ucRcvdHeader = 0xFF;
    fw_upload_ComReadChars(mchar_fd, (uint8 *)&ucRcvdHeader,1);
    ucStr[ucStringCnt++] = ucRcvdHeader;
  }
}

/******************************************************************************
 *
 * Name: fw_upload_GetLast5Bytes
 *
 * Description:
 *   This function gets last valid request.    
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *   *buf: buffer that stores header and following data.
 *
 * Return Value:
 *   None.
 *
 * Notes:
 *   None.
 *
 *****************************************************************************/ 
static void fw_upload_GetLast5Bytes(uint8 *buf)
{
  uint8  a5cnt, i;
  uint8  ucTemp[STRING_SIZE];
  uint16 uiTempLen = 0;
  int32  fifosize;
  BOOLEAN alla5times = FALSE;
  
  // initialise 
  memset(ucString, 0x00, STRING_SIZE);

  fifosize = fw_upload_GetBufferSize(mchar_fd);
  
  fw_upload_GetHeaderStartBytes(ucString);
  fw_upload_lenValid(&uiTempLen, ucString);

  if((fifosize < 6) && ((uiTempLen == HDR_LEN) || (uiTempLen == fw_upload_GetDataLen(buf))))
  {
#ifdef DEBUG_PRINT
    PRINT("=========>success case\n");
#endif
    uiErrCase = FALSE;
  }
  else // start to get last valid 5 bytes
  { 
#ifdef DEBUG_PRINT
    PRINT("=========>fail case\n");
#endif
    while (fw_upload_lenValid(&uiTempLen, ucString) == FALSE)
    {
      fw_upload_GetHeaderStartBytes(ucString);
      fifosize -= 5;
    }
#ifdef DEBUG_PRINT
      PRINT("Error cases 1, 2, 3, 4, 5...\n");
#endif
      if(fifosize > 5)
      {
        fifosize -= 5;
        do
        {
          do
          {
            a5cnt = 0;
            do
            {
              fw_upload_GetHeaderStartBytes(ucTemp);
              fifosize -= 5;
            } while ((fw_upload_lenValid(&uiTempLen, ucTemp) == TRUE) && (!alla5times) && (fifosize > 5));
            //if 5bytes are all 0xa5, continue to clear 0xa5
            for (i = 0; i < 5; i ++)
            {
              if (ucTemp[i] == BOOT_HEADER)
              {
                a5cnt ++; 
              }
            }
            alla5times = TRUE;
          } while (a5cnt == 5);
#ifdef DEBUG_PRINT
          PRINT("a5 count in last 5 bytes: %d\n", a5cnt);
#endif
          if (fw_upload_lenValid(&uiTempLen, ucTemp) == FALSE)
          {
            for (i = 0; i < (5 - a5cnt); i ++)
            {
              ucTemp[i + a5cnt] = fw_upload_ComReadChar(mchar_fd);
            }
            memcpy(ucString, &ucTemp[a5cnt-1], 5);
          }
          else
          {
            memcpy(ucString, ucTemp, 5);
          }
        } while (fw_upload_lenValid(&uiTempLen, ucTemp) == FALSE);
      }
      uiErrCase = TRUE;
  }
}

/******************************************************************************
 *
 * Name: fw_upload_SendBuffer
 *
 * Description:
 *   This function sends buffer with header and following data.
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *   uiLenToSend: len of header request.
 *         ucBuf: the buf to be sent.
 * Return Value:
 *   Returns the len of next header request.
 *
 * Notes:
 *   None.
 *
 *****************************************************************************/

uint16 fw_upload_SendBuffer(uint16 uiLenToSend, uint8 *ucBuf)
{
  uint16 uiBytesToSend = HDR_LEN, uiFirstChunkSent = 0;
  uint16 uiDataLen = 0;
  uint8 ucSentDone = 0;
  BOOLEAN uiValidLen = FALSE;
  // Get data len
  uiDataLen = fw_upload_GetDataLen(ucBuf);
  // Send buffer
  while (!ucSentDone)
  {
    if (uiBytesToSend == uiLenToSend)
    {
      // All good
      if ((uiBytesToSend == HDR_LEN) && (!b16BytesData))
      {
        if ((uiFirstChunkSent == 0) || ((uiFirstChunkSent == 1) && uiErrCase == TRUE))
     	{
     	  // Write first 16 bytes of buffer
#ifdef DEBUG_PRINT
     	  PRINT("\n====>  Sending first chunk...\n");
     	  PRINT("\n====>  Sending %d bytes...\n", uiBytesToSend);
#endif
     	  fw_upload_ComWriteChars(mchar_fd, (uint8 *)ucBuf, uiBytesToSend); 
     	  uiBytesToSend = uiDataLen;
     	  if(uiBytesToSend == HDR_LEN)
     	  {
            b16BytesData = TRUE;
     	  }
     	  uiFirstChunkSent = 0;
	}
     	else
        {
     	  // Done with buffer
     	  ucSentDone = 1;
     	  break;
        }
      }
      else
      {
     	 // Write remaining bytes
#ifdef DEBUG_PRINT
     	PRINT("\n====>  Sending %d bytes...\n", uiBytesToSend);
#endif
        if(uiBytesToSend != 0)
        {
          fw_upload_ComWriteChars(mchar_fd, (uint8 *)&ucBuf[HDR_LEN], uiBytesToSend);
     	  uiFirstChunkSent = 1;
     	  // We should expect 16, then next block will start
          uiBytesToSend = HDR_LEN;
          b16BytesData = FALSE;
        }
        else  //end of bin download
        {
#ifdef DEBUG_PRINT
          PRINT("\n ========== Download Complete =========\n\n");
#endif
     	  return 0;
        }
      }
    }
    else
    {
      // Something not good
      if ((uiLenToSend & 0x01) == 0x01)
      {
        // some kind of error
        if (uiLenToSend == (HDR_LEN + 1))
        {
          // Send first chunk again
#ifdef DEBUG_PRINT
          PRINT("\n1. Resending first chunk...\n");
#endif
          fw_upload_ComWriteChars(mchar_fd, (uint8 *)ucBuf, (uiLenToSend - 1));
          uiBytesToSend = uiDataLen;
          uiFirstChunkSent = 0;
        }
        else if (uiLenToSend == (uiDataLen + 1))
        {
          // Send second chunk again
#ifdef DEBUG_PRINT
          PRINT("\n2. Resending second chunk...\n");
#endif
          fw_upload_ComWriteChars(mchar_fd, (uint8 *)&ucBuf[HDR_LEN], (uiLenToSend - 1));
          uiBytesToSend = HDR_LEN;
          uiFirstChunkSent = 1;
        }
      }
      else if (uiLenToSend == HDR_LEN)
      {
        // Out of sync. Restart sending buffer
#ifdef DEBUG_PRINT
        PRINT("\n3.  Restart sending the buffer...\n");
#endif
        fw_upload_ComWriteChars(mchar_fd, (uint8 *)ucBuf, uiLenToSend);
        uiBytesToSend = uiDataLen;
        uiFirstChunkSent = 0;
      }
    }
    // Ensure any pending write data is completely written
    if (0 == tcdrain(mchar_fd))
    {
#ifdef DEBUG_PRINT
      PRINT("\n\t tcdrain succeeded\n");
#endif
    }
    else
    {
#ifdef DEBUG_PRINT
      PRINT("\n\t tcdrain failed with errno = %d\n", errno);
#endif
    }
    if(!ucCmd5Sent && uiFirstChunkSent == 1)
    {
      fw_upload_DelayInMs(DELAY_CMD5_PATCH);
    }
    // Get last 5 bytes now
    fw_upload_GetLast5Bytes(ucBuf);
    // Get next length
    uiValidLen = FALSE;
    do
    {
      if (fw_upload_lenValid(&uiLenToSend, ucString) == TRUE)
      {
        // Valid length received
        uiValidLen = TRUE;
#ifdef DEBUG_PRINT
        PRINT("\n Valid length = %d \n", uiLenToSend);
#endif
        // ACK the bootloader
       	fw_upload_ComWriteChar(mchar_fd, (int8)BOOT_HEADER_ACK);
#ifdef DEBUG_PRINT
        PRINT("\n  BOOT_HEADER_ACK 0x5a sent \n");
#endif
      }
    } while (!uiValidLen);
  }
#ifdef DEBUG_PRINT
  PRINT("\n ========== Buffer is successfully sent =========\n\n");
#endif
  return uiLenToSend;
}

/******************************************************************************
 *
 * Name: fw_upload_WaitFor_Offset
 *
 * Description:
 *   This function gets offset value from helper.    
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *   None.
 *
 * Return Value:
 *   offset value.
 *
 * Notes:
 *   None.
 *
 *****************************************************************************/
static uint32 fw_upload_WaitFor_Offset()
{  
  uint32 ulOffset = 0x0;
  uint32 ulOffsetComp = 0x0;

  // uiLen and uiLenComp are 1's complement of each other. 
  // In such cases, the XOR of uiLen and uiLenComp will be all 1's
  // i.e 0xffff.
  uint32 uiXorOfOffset = 0xFFFFFFFF;

  // Read the Offset.
  fw_upload_ComReadChars(mchar_fd, (uint8 *)&ulOffset, 4);
  fw_upload_ComReadChars(mchar_fd, (uint8 *)&ulOffsetComp, 4);

  // Check if the length is valid.
  if ((ulOffset ^ ulOffsetComp) == uiXorOfOffset) // All 1's
  { 
#ifdef DEBUG_PRINT
    PRINT("\n    Helper ask for offset %d \n ", ulOffset);
#endif
  }
  else
  {
#ifdef DEBUG_PRINT
    PRINT("\n    NAK case: helper Offset = %x bytes \n ", ulOffset);
    PRINT("\n    NAK case: helper OffsetComp = %x bytes \n ", ulOffsetComp);
#endif
    // Failure due to mismatch.	  
    fw_upload_ComWriteChar(mchar_fd, (int8)0xbf);

    // Start all over again.
    longjmp(resync, 1);
  }
  return ulOffset;
}

/******************************************************************************
 *
 * Name: fw_upload_WaitFor_ErrCode
 *
 * Description:
 *   This function gets error code from helper.    
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *   None.
 *
 * Return Value:
 *   error code.
 *
 * Notes:
 *   None.
 *
 *****************************************************************************/ 
static uint16 fw_upload_WaitFor_ErrCode()
{
  uint16 uiError = 0x0;
  uint16 uiErrorCmp = 0x0;
  uint16 uiXorOfErrCode = 0xFFFF;
  
  // Read the Error Code.
  fw_upload_ComReadChars(mchar_fd, (uint8 *)&uiError, 2);
  fw_upload_ComReadChars(mchar_fd, (uint8 *)&uiErrorCmp, 2);

  // Check if the Err Code is valid.
  if ((uiError ^ uiErrorCmp) == uiXorOfErrCode) // All 1's
  {
#ifdef DEBUG_PRINT
    PRINT("\n    Error Code is %d \n ", uiError);
#endif
    if(uiError == 0)
    {
      // Successful. Send back the ack.
      fw_upload_ComWriteChar(mchar_fd, (int8)HELPER_HEADR_ACK);
    }
    else
    {
#ifdef DEBUG_PRINT
      PRINT("\n    Helper NAK or CRC or Timeout \n ");
#endif
      // NAK/CRC/Timeout
      fw_upload_ComWriteChar(mchar_fd, (int8)HELPER_TIMEOUT_ACK);
    }
  }
  else
  {
#ifdef DEBUG_PRINT
    PRINT("\n    NAK case: helper ErrorCode = %x bytes \n ", uiError);
    PRINT("\n    NAK case: helper ErrorCodeComp = %x bytes \n ", uiErrorCmp);
#endif
    // Failure due to mismatch.	  
    fw_upload_ComWriteChar(mchar_fd, (int8)0xbf);
    // Start all over again.
    longjmp(resync, 1);
  }
  return uiError;
}

/******************************************************************************
 *
 * Name: fw_upload_SendLenBytesToHelper
 *
 * Description:
 *   This function sends Len bytes to the Helper.    
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *   pFileBuffer: bin file buffer being sent.
 *   uiLenTosend: the length will be sent.
 *   ulOffset: the offset of current sending.
 *
 * Return Value:
 *   None.
 *
 * Notes:
 *   None.
 *
 *****************************************************************************/ 
static void fw_upload_SendLenBytesToHelper(uint8* pFileBuffer, uint16 uiLenToSend, uint32 ulOffset)

{
  // Retransmittion of previous block
  if (ulOffset == ulLastOffsetToSend)
  {
#ifdef DEBUG_PRINT
    PRINT("\nRetx offset %d...\n", ulOffset);
#endif
    fw_upload_ComWriteChars(mchar_fd, (uint8 *)ucByteBuffer, uiLenToSend);
  }
  else  
  {
    //uint16 uiNumRead = 0;
    // The length requested by the Helper is equal to the Block
    // sizes used while creating the FW.bin. The usual
    // block sizes are 128, 256, 512.
    // uiLenToSend % 16 == 0. This means the previous packet
    // was error free (CRC ok) or this is the first packet received.
    //  We can clear the ucByteBuffer and populate fresh data. 
    memset (ucByteBuffer, 0, sizeof(ucByteBuffer));
    memcpy(ucByteBuffer,pFileBuffer+ulOffset,uiLenToSend);
    ulCurrFileSize += uiLenToSend;
    fw_upload_ComWriteChars(mchar_fd, (uint8 *)ucByteBuffer, uiLenToSend);
    ulLastOffsetToSend = ulOffset;
  }
}

/******************************************************************************
 *
 * Name: fw_upload_SendIntBytes
 *
 * Description:
 *   This function sends 4 bytes and 4bytes' compare.    
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *   4 bytes need to be sent.
 *
 * Return Value:
 *   None.
 *
 * Notes:
 *   None.
 *
 *****************************************************************************/ 
static void fw_upload_SendIntBytes(uint32 ulBytesToSent)
{
  uint8 i, uTemp[9], uiLocalCnt = 0;
  uint32 ulBytesToSentCmp;
  
  ulBytesToSentCmp = ulBytesToSent ^ 0xFFFFFFFF;

  for (i = 0; i < 4; i ++)
  {
    uTemp[uiLocalCnt++] = (uint8)(ulBytesToSent >> (i * 8)) & 0xFF;
  }
  for (i = 0; i < 4; i ++)
  {
    uTemp[uiLocalCnt++] = (uint8)(ulBytesToSentCmp >> (i * 8)) & 0xFF;
  }

  fw_upload_ComWriteChars(mchar_fd, (uint8 *)uTemp, 8);
}

/******************************************************************************
 *
 * Name: fw_upload_SendLenBytes
 *
 * Description:
 *   This function sends Len bytes(header+data) to the boot code.    
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *   pFileBuffer: bin file buffer being sent.
 *   uiLenTosend: the length will be sent.
 *
 * Return Value:
 *   the 'len' of next header request.
 *
 * Notes:
 *   None.
 *
 *****************************************************************************/ 
static uint16 fw_upload_SendLenBytes(uint8* pFileBuffer, uint16 uiLenToSend)
{
  uint16 ucDataLen, uiLen;
  //uint16 uiNumRead = 0;
#ifdef DEBUG_PRINT
  uint16 i;
#endif
  memset (ucByteBuffer, 0, sizeof(ucByteBuffer));
  if(!ucCmd5Sent)
  {
    // put header and data into temp buffer first
    memcpy(ucByteBuffer, ucCmd5Patch, uiLenToSend); 
    //get data length from header
    ucDataLen = fw_upload_GetDataLen(ucByteBuffer);
    memcpy(&ucByteBuffer[uiLenToSend], &ucCmd5Patch[uiLenToSend], ucDataLen);
    uiLen = fw_upload_SendBuffer(uiLenToSend, ucByteBuffer);
    ucCmd5Sent = 1;
#ifdef DEBUG_PRINT
    PRINT("\ncmd5 patch is sent\n");
#endif
  }
  else
  {
    // fread(void *buffer, size_t size, size_t count, FILE *stream)
    memcpy(ucByteBuffer,pFileBuffer+ulCurrFileSize,uiLenToSend);
    ulCurrFileSize += uiLenToSend;
    ucDataLen = fw_upload_GetDataLen(ucByteBuffer);
    memcpy(&ucByteBuffer[uiLenToSend],pFileBuffer+ulCurrFileSize,ucDataLen);
    ulCurrFileSize += ucDataLen;
#ifdef DEBUG_PRINT
    PRINT("The buffer is to be sent: %d", uiLenToSend + ucDataLen);
    for(i = 0; i < (uiLenToSend + ucDataLen); i ++)
    {
      if(i % 16 == 0)
      {
        PRINT("\n");
      }
      PRINT(" %02x ", ucByteBuffer[i]);
    }
#endif
    //start to send Temp buffer
    uiLen = fw_upload_SendBuffer(uiLenToSend, ucByteBuffer);
    PRINT("File downloaded: %8d:%8d\r", ulCurrFileSize, ulTotalFileSize);
  }
  return uiLen;
}

/******************************************************************************
 *
 * Name: fw_upload_FW
 *
 * Description:
 *   This function performs the task of FW load over UART.
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *   pPortName:       Com port number.
 *   iBaudRate:       the initial baud rate.
 *   ucFlowCtrl:      the flow ctrl of uart.
 *   pFileName:       the file name for downloading.
 *   iSecondBaudRate: the second baud rate.
 *
 * Return Value:
 *   TRUE:            Download successfully
 *   FALSE:           Download unsuccessfully
 *
 * Notes:
 *   None.
 *
 *****************************************************************************/
static BOOLEAN fw_upload_FW(int8 *pFileName) {
  uint8 *pFileBuffer = NULL;
  uint32 ulReadLen = 0;
  BOOLEAN bRetVal = FALSE;
  int32 result = 0;
  uint16 uiLenToSend = 0;

  uint32 ulOffsettoSend = 0;
  uint16 uiErrCode = 0;

  // Open File for reading.
  pFile = fopen(pFileName, "rb");

  if ((mchar_fd < 0) || (pFile == NULL)) {
    printf("\nPort is not open or file not found\n");
    return bRetVal;
  }

  // Calculate the size of the file to be downloaded.
  result = fseek(pFile, 0, SEEK_END);
  if (result)
  {
    printf("\nfseek failed\n");
    return bRetVal;
  }

  ulTotalFileSize = (uint32)ftell(pFile);
  if (!ulTotalFileSize)
  {
    printf("\nError:Download Size is 0\n");
    return bRetVal;
  }

  pFileBuffer = (uint8 *) malloc(ulTotalFileSize);

  fseek(pFile, 0, SEEK_SET);
  if (pFileBuffer)
  {
    ulReadLen = fread(pFileBuffer, 1, ulTotalFileSize, pFile);
    if (ulReadLen != ulTotalFileSize)
    {
      PRINT("\nError:Read File Fail\n");
      return bRetVal;
    }

  }
  ulCurrFileSize = 0;

  // Jump to here in case of protocol resync.
  setjmp(resync);

  while (!bRetVal)
  {
    // Wait to Receive 0xa5, 0xaa, 0xa6
    if (!fw_upload_WaitForHeaderSignature(TIMEOUT_VAL_MILLISEC))
    {
      PRINT("\n0xa5,0xaa,or 0xa6 is not received in 4s.\n");
      return bRetVal;
    }

    // Read the 'Length' bytes requested by Helper
    uiLenToSend = fw_upload_WaitFor_Len(pFile);
  
    if(ucRcvdHeader == HELPER_HEADER)
    {
      ucHelperOn = TRUE;
      ulOffsettoSend = fw_upload_WaitFor_Offset();
      uiErrCode = fw_upload_WaitFor_ErrCode();
      if(uiErrCode == 0)
      {
        if(uiLenToSend != 0)
        {
          fw_upload_SendLenBytesToHelper(pFileBuffer, uiLenToSend, ulOffsettoSend);
#ifdef DEBUG_PRINT
          PRINT("\n sent %d bytes..\n", ulOffsettoSend);
#endif
        }
        else    //download complete
        {
          tcflush(mchar_fd, TCIFLUSH);
          fw_upload_SendIntBytes(ulCurrFileSize);
          fw_upload_DelayInMs(20);
          if( fw_upload_GetBufferSize(mchar_fd) == 0)
          {
            bRetVal = TRUE;
          }
        }
      }
      else if (uiErrCode > 0)
      {
      /*delay 20ms to make multiple uiErrCode == 1 has been sent, after 20ms, if get 
       *uiErrCode = 1 again, we consider 0x6b is missing.
       */
        fw_upload_DelayInMs(20);
        tcflush(mchar_fd, TCIFLUSH);
        fw_upload_SendIntBytes(ulOffsettoSend);
      }
      PRINT("File downloaded: %8d:%8d\r", ulCurrFileSize, ulTotalFileSize);

      // Ensure any pending write data is completely written
      if (0 == tcdrain(mchar_fd))
      {
#ifdef DEBUG_PRINT
        PRINT("\n\t tcdrain succeeded\n");
#endif
      }
      else
      {
#ifdef DEBUG_PRINT
        PRINT("\n\t FW download tcdrain failed with errno = %d\n", errno);
#endif
      }
    }

    if(!ucHelperOn)
    {
      do
      {
        uiLenToSend = fw_upload_SendLenBytes(pFileBuffer, uiLenToSend);
      }while(uiLenToSend != 0);
  	  // If the Length requested is 0, download is complete.
      if (uiLenToSend == 0)
      {	  
        bRetVal = TRUE;
        break;
      }
    }
    
  }
  if(pFileBuffer != NULL)
  {
    free(pFileBuffer);
    pFileBuffer = NULL;
  }
  return bRetVal;
}

/******************************************************************************
 *
 * Name: fw_upload_check_FW
 *
 * Description:
 *   This function performs the task of FW load over UART.
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *   pPortName:       Com port number.
 *   iBaudRate:       the initial baud rate.
 *   ucFlowCtrl:      the flow ctrl of uart.
 *
 * Return Value:
 *   TRUE:            Need Download FW
 *   FALSE:           No need Download FW
 *
 * Notes:
 *   None.
 *
 *****************************************************************************/
BOOLEAN bt_vnd_mrvl_check_fw_status_v2() {
  BOOLEAN bRetVal = FALSE;

  if (mchar_fd < 0) {
    printf("\nPort is not open or file not found\n");
    return bRetVal;
  }

  // Wait to Receive 0xa5, 0xaa, 0xa6
  bRetVal = fw_upload_WaitForHeaderSignature(4000);

  printf("fw_upload_WaitForHeaderSignature return %d", bRetVal);

  return bRetVal;
}

/******************************************************************************
*
* Name: bt_vnd_mrvl_download_fw
*
* Description:
*   Wrapper of fw_upload_FW.
*
* Conditions For Use:
*   None.
*
* Arguments:
*   pPortName:       Com port number.
*   iBaudRate:       the initial baud rate.
*   ucFlowCtrl:      the flow ctrl of uart.
*   pFileName:       the file name for downloading.
*   iSecondBaudRate: the second baud rate.
*
* Return Value:
*   0:            Download successfully
*   1:           Download unsuccessfully
*
* Notes:
*   None.
*
*****************************************************************************/
int bt_vnd_mrvl_download_fw_v2(int8 *pPortName, int32 iBaudrate, int8 *pFileName) {
  double endTime;
  double start;
  double cost;
  uint32 ulResult;
  uint8 ucByte;

  start = fw_upload_GetTime();

  printf("Protocol: NXP Proprietary\n");
  printf("FW Loader Version: %s\n", VERSION);
  printf("ComPort : %s\n", pPortName);
  printf("BaudRate: %d\n", iBaudrate);
  printf("Filename: %s\n", pFileName);

  do
  {
    ulResult = fw_upload_FW(pFileName);
    if(ulResult)
    {
      printf("\nDownload Complete\n");
      cost = fw_upload_GetTime() - start;
      printf("time:%f\n", cost);
      if(ucHelperOn == TRUE)
      {
        endTime = fw_upload_GetTime() + POLL_AA_TIMEOUT;
        do
        {
          if(fw_upload_GetBufferSize(mchar_fd) != 0)
          {
            ucByte = 0xff;
            fw_upload_ComReadChars(mchar_fd, (uint8 *)&ucByte,1);
            if (ucByte == VERSION_HEADER)
            {
              PRINT("\nReDownload\n");
              uiReDownload = TRUE;
              ulLastOffsetToSend = 0xFFFF;
              memset (ucByteBuffer, 0, sizeof(ucByteBuffer));
            }
            break;
          }
        }while(endTime > fw_upload_GetTime());
      }
      if (uiReDownload == FALSE)
      {
        endTime = fw_upload_GetTime() + MAX_CTS_TIMEOUT;
        do
        {
          if (!fw_upload_ComGetCTS(mchar_fd))
          {
            PRINT("CTS is low\n");
            if(pFile)
            {
              fclose(pFile);
              pFile = NULL;
            }
            goto done;
          }
        } while (endTime > fw_upload_GetTime());
        PRINT("wait CTS low timeout \n");
        PRINT("Error code is %d\n",ulResult);
        if(pFile)
        {
          fclose(pFile);
          pFile = NULL;
        }
      	goto done;
      }
    }
    else
    {
      printf("\nDownload Error\n");
      return 1;
    }
  } while(uiReDownload);

done:
  return 0;
}

