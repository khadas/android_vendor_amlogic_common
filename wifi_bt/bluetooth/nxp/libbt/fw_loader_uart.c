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
#include "fw_loader_uart.h"
#include "fw_loader_io.h"
#include <errno.h>
#include <memory.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>


#define LOG_TAG "fw_loader"
#include <cutils/log.h>
#include <cutils/properties.h>
#define printf(fmt, ...) ALOGE("ERROR : %s(L%d): " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define PRINT_INFO(fmt, ...) ALOGI("INFO : %s(L%d): " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

/*--------------------------------fw_loader_io_linux.c-------------------------*/
#define TIMEOUT_FOR_READ        4000


/*===================== Macros ===================================================*/
#define VERSION "M304"
#define MAX_LENGTH 0xFFFF  // Maximum 2 byte value
#define END_SIG_TIMEOUT 2500
#define MAX_CTS_TIMEOUT 100  // 100ms
#define STRING_SIZE 6
#define HDR_LEN 16
#define CMD4 0x4
#define CMD6 0x6
#define CMD7 0x7

#define V1_HEADER_DATA_REQ 0xa5
#define V1_REQUEST_ACK 0x5a
#define V1_START_INDICATION 0xaa

#define V3_START_INDICATION 0xab
#define V3_HEADER_DATA_REQ 0xa7
#define V3_REQUEST_ACK 0x7a
#define V3_TIMEOUT_ACK 0x7b
#define V3_CRC_ERROR 0x7c

#define PRINT(...)         printf(__VA_ARGS__)

#define REQ_HEADER_LEN 1
#define A6REQ_PAYLOAD_LEN 8
#define AbREQ_PAYLOAD_LEN 3

#define END_SIG 0x435000

#define GP 0x107 /* x^8 + x^2 + x + 1 */
#define DI 0x07

#define CRC_ERR_BIT 1 << 0
#define NAK_REC_BIT 1 << 1
#define TIMEOUT_REC_ACK_BIT 1 << 2
#define TIMEOUT_REC_HEAD_BIT 1 << 3
#define TIMEOUT_REC_DATA_BIT 1 << 4
#define INVALID_CMD_REC_BIT 1 << 5
#define WIFI_MIC_FAIL_BIT 1 << 6
#define BT_MIC_FAIL_BIT 1 << 7

#define SWAPL(x) \
  (((x >> 24) & 0xff) | ((x >> 8) & 0xff00) | ((x << 8) & 0xff0000L) | ((x << 24) & 0xff000000L))

#define POLYNOMIAL 0x04c11db7L

#define CLKDIVAddr 0x7f00008f
#define UARTDIVAddr 0x7f000090
#define UARTMCRAddr 0x7f000091
#define UARTREINITAddr 0x7f000092
#define UARTICRAddr 0x7f000093

#define MCR 0x00000022
#define INIT 0x00000001
#define ICR 0x000000c7
#define TIMEOUT_VAL_MILLISEC 4000  // Timeout for getting 0xa5 or 0xab

static unsigned char crc8_table[256]; /* 8-bit table */
static int made_table = 0;

static unsigned long crc_table[256];
static BOOLEAN cmd7_Req = FALSE;
static BOOLEAN EntryPoint_Req = FALSE;
static uint32 change_baudrata_buffer_len = 0;


// CMD5 Header to change bootload baud rate
int8 m_Buffer_CMD5_Header[16] = {0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                 0x2c, 0x00, 0x00, 0x00, 0x77, 0xdb, 0xfd, 0xe0};

const UART_BAUDRATE UartCfgTbl[] = {
    {115200, 16, 0x0075F6FD}, {3000000, 1, 0x00C00000},
};

//#define DEBUG_PRINT
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
// Received Header
static uint8 ucRcvdHeader;
static uint8 ucString[STRING_SIZE];
static BOOLEAN b16BytesData = FALSE;

static uint16 uiNewLen;
static uint32 ulNewOffset;
static uint16 uiNewError;
static uint8 uiNewCrc;

static uint8 uiProVer;
static BOOLEAN bVerChecked = FALSE;
static uint8 ucCalCrc[10];

typedef enum {
  Ver1,
  Ver2,
  Ver3,
} Version;

uint8 uiErrCnt[16] = {0};
jmp_buf resync;  // Protocol restart buffer used in timeout cases.
/*==================== Function Prototypes ======================================*/

/*==================== Coded Procedures =========================================*/
/******************************************************************************

 *
 * Name: gen_crc_table
 *
 * Description:
 *   Genrate crc table
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
void fw_upload_gen_crc_table() {
  int i, j;
  unsigned long crc_accum;

  for (i = 0; i < 256; i++) {
    crc_accum = ((unsigned long)i << 24);
    for (j = 0; j < 8; j++) {
      if (crc_accum & 0x80000000L) {
        crc_accum = (crc_accum << 1) ^ POLYNOMIAL;
      } else {
        crc_accum = (crc_accum << 1);
      }
    }
    crc_table[i] = crc_accum;
  }

  return;
}

/******************************************************************************

 *
 * Name: update_crc
 *
 * Description:
 *   update the CRC on the data block one byte at a time
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *   ata_blk_ptr:   the buffer pointer for updating crc.
 *   data_blk_size: the size of buffer
 *
 * Return Value:
 *   CRC value.
 *
 * Notes:
 *   None.
 *
 *****************************************************************************/
unsigned long fw_upload_update_crc(unsigned long crc_accum, char *data_blk_ptr, int data_blk_size) {
  int i, j;

  for (j = 0; j < data_blk_size; j++) {
    i = ((int)(crc_accum >> 24) ^ *data_blk_ptr++) & 0xff;
    crc_accum = (crc_accum << 8) ^ crc_table[i];
  }
  return crc_accum;
}

/******************************************************************************
 *
 * Name: init_crc8
 *
 * Description:
 *   This function init crc.
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
void init_crc8() {
  int i, j;
  unsigned char crc;

  if (!made_table) {
    for (i = 0; i < 256; i++) {
      crc = i;
      for (j = 0; j < 8; j++) crc = (crc << 1) ^ ((crc & 0x80) ? DI : 0);
      crc8_table[i] = crc & 0xFF;
      /* printf("table[%d] = %d (0x%X)\n", i, crc, crc); */
    }
    made_table = 1;
  }
}
/******************************************************************************
 *
 * Name: crc8
 *
 * Description:
 *   This function calculate crc.
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *   array: array to be calculated.
 *   len :  len of array.
 *
 * Return Value:
 *   None.
 *
 * Notes:
 *   None.
 *
 *****************************************************************************/
static unsigned char crc8(unsigned char *array, unsigned char len) {
  unsigned char CRC = 0;
  for (; len > 0; len--) {
    CRC = crc8_table[CRC ^ *array];
    array++;
  }
  return CRC;
}

/******************************************************************************
 *
 * Name: fw_upload_WaitForHeaderSignature(uint32 uiMs)
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
 *   TRUE:   0xa5 or 0xab is received.
 *   FALSE:  0xa5 or 0xab is not received.
 *
 * Notes:
 *   None.
 *
 *****************************************************************************/
static BOOLEAN
fw_upload_WaitForHeaderSignature(uint32 uiMs)
{
  uint8 ucDone = 0;	// signature not Received Yet.
  uint64 startTime = 0;
  uint64 currTime = 0;
  BOOLEAN bResult = TRUE;
  startTime = fw_upload_GetTime();
  while (!ucDone) {
  ucRcvdHeader = fw_upload_ComReadChar(mchar_fd);
  if ((ucRcvdHeader == V1_HEADER_DATA_REQ) ||(ucRcvdHeader == V1_START_INDICATION) ||
      (ucRcvdHeader == V3_START_INDICATION) ||(ucRcvdHeader == V3_HEADER_DATA_REQ)) {
    ucDone = 1;
#ifdef DEBUG_PRINT
    PRINT("\nReceived 0x%x ", ucRcvdHeader);
#endif
    if (!bVerChecked) {
      if ((ucRcvdHeader == V1_HEADER_DATA_REQ) ||(ucRcvdHeader == V1_START_INDICATION)) {
        uiProVer = Ver1;
      } else {
          uiProVer = Ver3;
	}
      bVerChecked = TRUE;
    }
  } else {
      if (uiMs) {
        currTime = fw_upload_GetTime();
        if (currTime - startTime > uiMs) {
#ifdef DEBUG_PRINT
	  PRINT("WaitForHeaderSignature time out");
#endif
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
 *   pFile: The handler of file
 *
 * Return Value:
 *   2 Byte Length to send back to the Helper.
 *
 * Notes:
 *   None.
 *
 *****************************************************************************/
static uint16 fw_upload_WaitFor_Len(FILE *pFile) {
  // Length Variables
  uint16 uiLen;
  uint16 uiLenComp;
  // uiLen and uiLenComp are 1's complement of each other.
  // In such cases, the XOR of uiLen and uiLenComp will be all 1's
  // i.e 0xffff.
  uint16 uiXorOfLen = 0xFFFF;

  // Read the Lengths.
  fw_upload_ComReadChars(mchar_fd, (uint8 *)&uiLen, 2);
  fw_upload_ComReadChars(mchar_fd, (uint8 *)&uiLenComp, 2);

  // Check if the length is valid.
  if ((uiLen ^ uiLenComp) == uiXorOfLen)  // All 1's
  {
#ifdef DEBUG_PRINT
    printf("\n       bootloader asks for %d bytes \n ", uiLen);
#endif
    // Successful. Send back the ack.
    if ((ucRcvdHeader == V1_HEADER_DATA_REQ) || (ucRcvdHeader == V1_START_INDICATION)) {
      fw_upload_ComWriteChar(mchar_fd, V1_REQUEST_ACK);
      if (ucRcvdHeader == V1_START_INDICATION) {
        longjmp(resync, 1);
      }
    }
  } else {
#ifdef DEBUG_PRINT
    printf("\n    NAK case: bootloader LEN = %x bytes \n ", uiLen);
    printf("\n    NAK case: bootloader LENComp = %x bytes \n ", uiLenComp);
#endif
    // Failure due to mismatch.
    fw_upload_ComWriteChar(mchar_fd, (int8)0xbf);
    // Start all over again.
    if (pFile != NULL) {
      longjmp(resync, 1);
    } else {
      uiLen = 0;
    }
  }
  return uiLen;
}

/******************************************************************************
 *
 * Name: fw_upload_StoreBytes
 *
 * Description:
 *   This function stores mul-bytes variable to array.
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *   ulVal: variable to be stored.
 *   uiSize: size of bytes of this variable.
 *   uiStored: array to store variable.
 *
 * Return Value:
 *   None.
 *
 * Notes:
 *   None.
 *
 *****************************************************************************/
static void fw_upload_StoreBytes(uint32 ulVal, uint8 uiSize, uint8 *uiStored) {
  uint8 i;
  for (i = 0; i < uiSize; i++) {
    uiStored[i] = (uint8)(ulVal >> (i * 8)) & 0xFF;
  }
}

/******************************************************************************
 *
 * Name: fw_upload_Send_Ack
 *
 * Description:
 *   This function sends ack to per req.
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *   uiAck: the ack type.
 *
 * Return Value:
 *   None.
 *
 * Notes:
 *   None.
 *
 *****************************************************************************/
static void fw_upload_Send_Ack(uint8 uiAck) {
  uint8 uiAckCrc = 0;
  if ((uiAck == V3_REQUEST_ACK) || (uiAck == V3_CRC_ERROR)) {
    fw_upload_ComWriteChar(mchar_fd, uiAck);

    // prepare crc for 0x7A or 0x7C
    ucCalCrc[0] = uiAck;
    uiAckCrc = crc8(ucCalCrc, 1);
    fw_upload_ComWriteChar(mchar_fd, uiAckCrc);
  } else if (uiAck == V3_TIMEOUT_ACK) {
    fw_upload_ComWriteChar(mchar_fd, uiAck);

    // prepare crc for 0x7B
    ucCalCrc[0] = uiAck;
    fw_upload_StoreBytes(ulNewOffset, sizeof(ulNewOffset), &ucCalCrc[1]);
    fw_upload_ComWriteChars(mchar_fd, (uint8 *)&ulNewOffset, 4);
    uiAckCrc = crc8(ucCalCrc, 5);
    fw_upload_ComWriteChar(mchar_fd, uiAckCrc);
  }
#ifdef DEBUG_PRINT
  printf("\n ===> ACK = %x, CRC = %x \n", uiAck, uiAckCrc);
#endif
}
/******************************************************************************
 *
 * Name: fw_upload_Check_ReqCrc
 *
 * Description:
 *   This function check the request crc.
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *   uiStr: array to put req header + payload.
 *   uiReq: the request type.
 *
 * Return Value:
 *   result of crc check.
 *
 * Notes:
 *   None.
 *
 *****************************************************************************/
BOOLEAN fw_upload_Check_ReqCrc(uint8 *uiStr, uint8 uiReq) {
  uint8 uiCalCrc;

  if (uiReq == V3_HEADER_DATA_REQ) {
    uiCalCrc = crc8(uiStr, A6REQ_PAYLOAD_LEN + REQ_HEADER_LEN);
    if (uiCalCrc != uiStr[A6REQ_PAYLOAD_LEN + REQ_HEADER_LEN]) {
      return FALSE;
    }

  } else if (uiReq == V3_START_INDICATION) {
    uiCalCrc = crc8(uiStr, AbREQ_PAYLOAD_LEN + REQ_HEADER_LEN);
    if (uiCalCrc != uiStr[AbREQ_PAYLOAD_LEN + REQ_HEADER_LEN]) {
      return FALSE;
    }
  }

  return TRUE;
}

/******************************************************************************
 *
 * Name: fw_upload_WaitFor_Req
 *
 * Description:
 *   This function waits for req from bootcode or helper.
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
static void fw_upload_WaitFor_Req() {
  uint16 uiChipId;
  uint8 uiVersion, uiReqCrc, uiTmp[20];
  BOOLEAN bCrcMatch = FALSE;

  if (ucRcvdHeader == V3_HEADER_DATA_REQ) {
    // 0xA7 <LEN><Offset><ERR><CRC8>
    fw_upload_ComReadChars(mchar_fd, (uint8 *)&uiNewLen, 2);
    fw_upload_ComReadChars(mchar_fd, (uint8 *)&ulNewOffset, 4);
    fw_upload_ComReadChars(mchar_fd, (uint8 *)&uiNewError, 2);
    fw_upload_ComReadChars(mchar_fd, (uint8 *)&uiNewCrc, 1);
#ifdef DEBUG_PRINT
    printf("\n <=== REQ = 0xA6, Len = %x,Off = %x,Err = %x,CRC = %x\n ", uiNewLen, ulNewOffset,
           uiNewError, uiNewCrc);
#endif
    // check crc
    uiTmp[0] = V3_HEADER_DATA_REQ;
    fw_upload_StoreBytes((uint32)uiNewLen, sizeof(uiNewLen), &uiTmp[1]);
    fw_upload_StoreBytes(ulNewOffset, sizeof(ulNewOffset), &uiTmp[3]);
    fw_upload_StoreBytes(uiNewError, sizeof(uiNewError), &uiTmp[7]);
    uiTmp[9] = uiNewCrc;
    bCrcMatch = fw_upload_Check_ReqCrc(uiTmp, V3_HEADER_DATA_REQ);

    if (!bCrcMatch) {
#ifdef DEBUG_PRINT
      printf("\n === REQ = 0xA7, CRC Mismatched === ");
#endif
      fw_upload_Send_Ack(V3_CRC_ERROR);
    }
  } else if (ucRcvdHeader == V3_START_INDICATION) {
    // 0xAB <CHIP ID> <SW loader REV 1 byte> <CRC8>
    fw_upload_ComReadChars(mchar_fd, (uint8 *)&uiChipId, 2);
    uiVersion = fw_upload_ComReadChar(mchar_fd);
    uiReqCrc = fw_upload_ComReadChar(mchar_fd);
    PRINT_INFO("\nChipID is : %x, Version is : %x\n", uiChipId, uiVersion);

    // check crc
    uiTmp[0] = V3_START_INDICATION;
    fw_upload_StoreBytes((uint32)uiChipId, sizeof(uiChipId), &uiTmp[1]);
    uiTmp[3] = uiVersion;
    uiTmp[4] = uiReqCrc;
    bCrcMatch = fw_upload_Check_ReqCrc(uiTmp, V3_START_INDICATION);

    if (bCrcMatch) {
#ifdef DEBUG_PRINT
      printf("\n === REQ = 0xAB, CRC Matched === ");
#endif
      fw_upload_Send_Ack(V3_REQUEST_ACK);
      longjmp(resync, 1);
    } else {
#ifdef DEBUG_PRINT
      printf("\n === REQ = 0xAB, CRC Mismatched === ");
#endif
      fw_upload_Send_Ack(V3_CRC_ERROR);
      longjmp(resync, 1);
    }
  }
}

/******************************************************************************
 *
 * Name: fw_upload_GetCmd
 *
 * Description:
 *   This function gets CMD value in the header.
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *   *buf: buffer that stores header and following data.
 *
 * Return Value:
 *   CMD value part in the buffer.
 *
 * Notes:
 *   None.
 *
 *****************************************************************************/
static uint32 fw_upload_GetCmd(uint8 *buf) {
  return (buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24));
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
static void fw_upload_GetHeaderStartBytes(uint8 *ucStr) {
  BOOLEAN ucDone = FALSE, ucStringCnt = 0, i;
  while (!ucDone) {
    ucRcvdHeader = fw_upload_ComReadChar(mchar_fd);

    if (ucRcvdHeader == V1_HEADER_DATA_REQ) {
      ucStr[ucStringCnt++] = ucRcvdHeader;
      ucDone = TRUE;
#ifdef DEBUG_PRINT
      printf("\nReceived 0x%x\n ", ucRcvdHeader);
#endif
    } else {
      fw_upload_DelayInMs(1);
    }
  }
  while (!fw_upload_GetBufferSize(mchar_fd))
    ;
  for (i = 0; i < 4; i++) {
    ucRcvdHeader = fw_upload_ComReadChar(mchar_fd);
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
static void fw_upload_GetLast5Bytes(uint8 *buf) {
  uint8 a5cnt, i;
  uint8 ucTemp[STRING_SIZE];
  uint16 uiTempLen = 0;
  int32 fifosize;
  BOOLEAN alla5times = FALSE;

  // initialise
  memset(ucString, 0x00, STRING_SIZE);

  fifosize = fw_upload_GetBufferSize(mchar_fd);

  fw_upload_GetHeaderStartBytes(ucString);
  fw_upload_lenValid(&uiTempLen, ucString);

  if ((fifosize < 6) && ((uiTempLen == HDR_LEN) || (uiTempLen == fw_upload_GetDataLen(buf)))) {
#ifdef DEBUG_PRINT
    printf("=========>success case\n");
#endif
    uiErrCase = FALSE;
  } else  // start to get last valid 5 bytes
  {
#ifdef DEBUG_PRINT
    printf("=========>fail case\n");
#endif
    while (fw_upload_lenValid(&uiTempLen, ucString) == FALSE) {
      fw_upload_GetHeaderStartBytes(ucString);
      fifosize -= 5;
    }
#ifdef DEBUG_PRINT
    printf("Error cases 1, 2, 3, 4, 5...\n");
#endif
    if (fifosize > 5) {
      fifosize -= 5;
      do {
        do {
          a5cnt = 0;
          do {
            fw_upload_GetHeaderStartBytes(ucTemp);
            fifosize -= 5;
          } while ((fw_upload_lenValid(&uiTempLen, ucTemp) == TRUE) && (!alla5times) && (fifosize > 5));
          // if 5bytes are all 0xa5, continue to clear 0xa5
          for (i = 0; i < 5; i++) {
            if (ucTemp[i] == V1_HEADER_DATA_REQ) {
              a5cnt++;
            }
          }
          alla5times = TRUE;
        } while (a5cnt == 5);
#ifdef DEBUG_PRINT
        printf("a5 count in last 5 bytes: %d\n", a5cnt);
#endif
        if (fw_upload_lenValid(&uiTempLen, ucTemp) == FALSE) {
          for (i = 0; i < (5 - a5cnt); i++) {
            ucTemp[i + a5cnt] = fw_upload_ComReadChar(mchar_fd);
          }
          memcpy(ucString, &ucTemp[a5cnt - 1], 5);
        } else {
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
 *      uiLenToSend: len of header request.
 *            ucBuf: the buf to be sent.
 *   uiHighBaudrate: send the buffer for high baud rate change.
 * Return Value:
 *   Returns the len of next header request.
 *
 * Notes:
 *   None.
 *

*****************************************************************************/
static uint16 fw_upload_SendBuffer(uint16 uiLenToSend, uint8 *ucBuf, BOOLEAN uiHighBaudrate) {
  uint16 uiBytesToSend = HDR_LEN, uiFirstChunkSent = 0;
  uint16 uiDataLen = 0;
  uint8 ucSentDone = 0;
  BOOLEAN uiValidLen = FALSE;
  // Get data len
  uiDataLen = fw_upload_GetDataLen(ucBuf);
  // Send buffer
  while (!ucSentDone) {
    if (uiBytesToSend == uiLenToSend) {
      // All good
      if ((uiBytesToSend == HDR_LEN) && (!b16BytesData)) {
        if ((uiFirstChunkSent == 0) || ((uiFirstChunkSent == 1) && (uiErrCase == TRUE))) {
// Write first 16 bytes of buffer
#ifdef DEBUG_PRINT
          printf("\n====>  Sending first chunk...\n");
          printf("\n====>  Sending %d bytes...\n", uiBytesToSend);
#endif
          fw_upload_ComWriteChars(mchar_fd, (uint8 *)ucBuf, uiBytesToSend);
          if (cmd7_Req == TRUE || EntryPoint_Req == TRUE) {
            uiBytesToSend = HDR_LEN;
            uiFirstChunkSent = 1;
          } else {
            uiBytesToSend = uiDataLen;
            uiFirstChunkSent = 0;
            if (uiBytesToSend == HDR_LEN) {
              b16BytesData = TRUE;
            }
          }
        } else {
          // Done with buffer
#ifdef DEBUG_PRINT
          printf("\nDone with this buffer\n");
#endif
          ucSentDone = 1;
          break;
        }
      } else {
// Write remaining bytes
#ifdef DEBUG_PRINT
        printf("\n====>  Sending %d bytes...\n", uiBytesToSend);
#endif
        if (uiBytesToSend != 0) {
          fw_upload_ComWriteChars(mchar_fd, (uint8 *)&ucBuf[HDR_LEN], uiBytesToSend);
          uiFirstChunkSent = 1;
          // We should expect 16, then next block will start
          uiBytesToSend = HDR_LEN;
		  b16BytesData = FALSE;
          if (uiHighBaudrate) {
            return 0;
          }
        } else  // end of bin download
        {
#ifdef DEBUG_PRINT
          printf("\n ========== Download Complete =========\n\n");
#endif
          return 0;
        }
      }
    } else {
      // Something not good
      if ((uiLenToSend & 0x01) == 0x01) {
        // some kind of error
        if (uiLenToSend == (HDR_LEN + 1)) {
// Send first chunk again
#ifdef DEBUG_PRINT
          printf("\n1. Resending first chunk...\n");
#endif
          fw_upload_ComWriteChars(mchar_fd, (uint8 *)ucBuf, (uiLenToSend - 1));
          uiBytesToSend = uiDataLen;
          uiFirstChunkSent = 0;
        } else if (uiLenToSend == (uiDataLen + 1)) {
// Send second chunk again
#ifdef DEBUG_PRINT
          printf("\n2. Resending second chunk...\n");
#endif
          fw_upload_ComWriteChars(mchar_fd, (uint8 *)&ucBuf[HDR_LEN], (uiLenToSend - 1));
          uiBytesToSend = HDR_LEN;
          uiFirstChunkSent = 1;
        }
      } else if (uiLenToSend == HDR_LEN) {
// Out of sync. Restart sending buffer
#ifdef DEBUG_PRINT
        printf("\n3.  Restart sending the buffer...\n");
#endif
        fw_upload_ComWriteChars(mchar_fd, (uint8 *)ucBuf, uiLenToSend);
        uiBytesToSend = uiDataLen;
        uiFirstChunkSent = 0;
      }
    }
    // Get last 5 bytes now
    fw_upload_GetLast5Bytes(ucBuf);
    // Get next length
    uiValidLen = FALSE;
    do {
      if (fw_upload_lenValid(&uiLenToSend, ucString) == TRUE) {
        // Valid length received
        uiValidLen = TRUE;
#ifdef DEBUG_PRINT
        printf("\n Valid length = %d \n", uiLenToSend);
#endif
        // ACK the bootloader
        fw_upload_ComWriteChar(mchar_fd, V1_REQUEST_ACK);
#ifdef DEBUG_PRINT
        printf("\n  BOOT_HEADER_ACK 0x5a sent \n");
#endif
      }
    } while (!uiValidLen);
  }
#ifdef DEBUG_PRINT
  printf("\n ========== Buffer is successfully sent =========\n\n");
#endif
  return uiLenToSend;
}

/******************************************************************************
 *
 * Name: fw_upload_V1SendLenBytes
 *
 * Description:
 *   This function sends Len bytes(header+data) to the boot code.
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *   pFile: bin file being sent.
 *   uiLenTosend: the length will be sent.
 *
 * Return Value:
 *   the 'len' of next header request.
 *
 * Notes:
 *   None.
 *
 *****************************************************************************/
static uint16 fw_upload_V1SendLenBytes(uint8 * pFileBuffer, uint16 uiLenToSend) {
  uint16 ucDataLen, uiLen;
  uint32 ulCmd;

  memset(ucByteBuffer, 0, sizeof(ucByteBuffer));

  cmd7_Req = FALSE;
  EntryPoint_Req = FALSE;
  memcpy(ucByteBuffer, pFileBuffer + ulCurrFileSize, uiLenToSend);
  ulCurrFileSize += uiLenToSend;
  ulCmd = fw_upload_GetCmd(ucByteBuffer);
  if (ulCmd == CMD7) {
    cmd7_Req = TRUE;
    ucDataLen = 0;
  } else {
    ucDataLen = fw_upload_GetDataLen(ucByteBuffer);
    memcpy(&ucByteBuffer[uiLenToSend], pFileBuffer + ulCurrFileSize,ucDataLen);
    ulCurrFileSize += ucDataLen;
    if ((ulCurrFileSize < ulTotalFileSize) && (ulCmd == CMD6 || ulCmd == CMD4)) {
      EntryPoint_Req = TRUE;
    }
  }

#ifdef DEBUG_PRINT
  printf("The buffer is to be sent: %d", uiLenToSend + ucDataLen);
#endif
  // start to send Temp buffer
  uiLen = fw_upload_SendBuffer(uiLenToSend, ucByteBuffer, FALSE);
  PRINT_INFO("File downloaded: %8d:%8d\r", ulCurrFileSize, ulTotalFileSize);

  return uiLen;
}

/******************************************************************************
 *
 * Name: fw_upload_V3SendLenBytes
 *
 * Description:
 *   This function sends Len bytes to the Helper.
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *   pFile: bin file being sent.
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
static void fw_upload_V3SendLenBytes(uint8 * pFileBuffer, uint16 uiLenToSend, uint32 ulOffset) {
  // Retransmittion of previous block
  if (ulOffset == ulLastOffsetToSend) {
#ifdef DEBUG_PRINT
    printf("\nResend offset %d...\n", ulOffset);
#endif
    fw_upload_ComWriteChars(mchar_fd, (uint8 *)&ucByteBuffer, uiLenToSend);
  } else {
    // The length requested by the Helper is equal to the Block
    // sizes used while creating the FW.bin. The usual
    // block sizes are 128, 256, 512.
    // uiLenToSend % 16 == 0. This means the previous packet
    // was error free (CRC ok) or this is the first packet received.
    //  We can clear the ucByteBuffer and populate fresh data.
    memset(ucByteBuffer, 0, sizeof(ucByteBuffer));
    memcpy(ucByteBuffer,pFileBuffer + ulOffset - change_baudrata_buffer_len,uiLenToSend);
    ulCurrFileSize =ulOffset - change_baudrata_buffer_len + uiLenToSend;
    fw_upload_ComWriteChars(mchar_fd, (uint8 *)&ucByteBuffer, uiLenToSend);
    ulLastOffsetToSend = ulOffset;
  }
}

/******************************************************************************
 *
 * Name: fw_Change_Baudrate
 *
 * Description:
 *   This function changes the baud rate of bootrom.
 *
 * Conditions For Use:
 *   None.
 *
 * Arguments:
 *   pPortName:        Serial port value.
 *   iFirstBaudRate:   The default baud rate of boot rom.
 *   iSecondBaudRate:  The chaned baud rate.
 *
 * Return Value:
 *   TRUE:            Change baud rate successfully
 *   FALSE:           Change baud rate unsuccessfully
 *
 * Notes:
 *   None.
 *
 *****************************************************************************/
static int32 fw_Change_Baudrate(int8 *pPortName, int32 iFirstBaudRate, int32 iSecondBaudRate) {
  uint8 uartConfig[60];
  uint8 ucBuffer[80];
  uint32 j;
  uint32 uartClk = 0x00C00000;
  uint32 uartDiv = 0x1;
  uint16 uiLenToSend = 0;
  uint32 uiCrc = 0;
  uint32 uiLen = 0;
  BOOLEAN bRetVal = FALSE;
  int32 ucResult = -1;
  uint8 ucLoadPayload = 0;
  uint32 waitHeaderSigTime = 0;
  BOOLEAN uiReUsedInitBaudrate = FALSE;

  uint32 mcr = MCR;
  uint32 init = INIT;
  uint32 icr = ICR;
  uint32 brAddr = CLKDIVAddr;
  uint32 divAddr = UARTDIVAddr;
  uint32 mcrAddr = UARTMCRAddr;
  uint32 reInitAddr = UARTREINITAddr;
  uint32 icrAddr = UARTICRAddr;

  for (j = 0; j < sizeof(UartCfgTbl) / sizeof(UART_BAUDRATE); j++) {
    if (iSecondBaudRate == (int32)UartCfgTbl[j].iBaudRate) {
      uartDiv = UartCfgTbl[j].iUartDivisor;
      uartClk = UartCfgTbl[j].iClkDivisor;
      ucResult = 0;
      break;
    }
  }

  if (ucResult != 0) {
    return ucResult;
  }

  // Generate CRC value for CMD5 payload
  memcpy(uartConfig + uiLen, &brAddr, 4);
  uiLen += 4;
  memcpy(uartConfig + uiLen, &uartClk, 4);
  uiLen += 4;
  memcpy(uartConfig + uiLen, &divAddr, 4);
  uiLen += 4;
  memcpy(uartConfig + uiLen, &uartDiv, 4);
  uiLen += 4;
  memcpy(uartConfig + uiLen, &mcrAddr, 4);
  uiLen += 4;
  memcpy(uartConfig + uiLen, &mcr, 4);
  uiLen += 4;
  memcpy(uartConfig + uiLen, &reInitAddr, 4);
  uiLen += 4;
  memcpy(uartConfig + uiLen, &init, 4);
  uiLen += 4;
  memcpy(uartConfig + uiLen, &icrAddr, 4);
  uiLen += 4;
  memcpy(uartConfig + uiLen, &icr, 4);
  uiLen += 4;

  fw_upload_gen_crc_table();

  uiCrc = fw_upload_update_crc(0, (char *)&uartConfig, uiLen);
  uiCrc = SWAPL(uiCrc);
  memcpy(uartConfig + uiLen, &uiCrc, 4);
  uiLen += 4;

  while (!bRetVal) {
    if (ucLoadPayload != 0 || uiReUsedInitBaudrate) {
      waitHeaderSigTime = TIMEOUT_VAL_MILLISEC;
    } else {
      waitHeaderSigTime = 0;
    }
    // Wait to Receive 0xa5, 0xaa, 0xab, 0xa7
    // If the second baudrate is used, wait for 2s to check 0xa5
    if (fw_upload_WaitForHeaderSignature(waitHeaderSigTime)) {
      if (ucLoadPayload) {
	  	if (uiProVer == Ver3) {
					change_baudrata_buffer_len =
						HDR_LEN + uiNewLen;
				}
        break;
      }
    } else {
      if (uiReUsedInitBaudrate) {
        ucResult = -2;
        return ucResult;
      }
      if (ucLoadPayload) {
        // If 0xa5 or 0xa7 is not received by using the second baudrate, change baud rate to the first
        // baudrate.
        close(mchar_fd);
        mchar_fd = init_uart(pPortName, iFirstBaudRate, 0);
        ucLoadPayload = 0;
        uiReUsedInitBaudrate = TRUE;
        continue;
      }
    }
    if (uiProVer == Ver1) {
      uiLenToSend = fw_upload_WaitFor_Len(NULL);
      if (uiLenToSend == 0) {
        continue;
      } else if (uiLenToSend == HDR_LEN) {
        // Download CMD5 header and Payload packet.
        memcpy(ucBuffer, m_Buffer_CMD5_Header, HDR_LEN);
        memcpy(ucBuffer + HDR_LEN, uartConfig, uiLen);
        fw_upload_SendBuffer(uiLenToSend, ucBuffer, TRUE);
        close(mchar_fd);
        mchar_fd = init_uart(pPortName, iSecondBaudRate, 1);
        ucLoadPayload = 1;
      } else {
        // Download CMD5 header and Payload packet
        fw_upload_ComWriteChars(mchar_fd, (uint8 *)&uartConfig, uiLen);
        close(mchar_fd);
        mchar_fd = init_uart(pPortName, iSecondBaudRate, 1);
        ucLoadPayload = 1;
      }
    } else if (uiProVer == Ver3) {
      fw_upload_WaitFor_Req();
      if (uiNewLen != 0) {
        if (uiNewError == 0) {
          fw_upload_Send_Ack(V3_REQUEST_ACK);
          if (ulNewOffset == ulLastOffsetToSend) {
            if (uiLenToSend == 16) {
              fw_upload_ComWriteChars(mchar_fd, (uint8 *)m_Buffer_CMD5_Header, uiLenToSend);
              ulLastOffsetToSend = ulNewOffset;
            } else {
              fw_upload_ComWriteChars(mchar_fd, (uint8 *)&uartConfig, uiLenToSend);
              // Reopen Uart by using the second baudrate after downloading the payload.
              close(mchar_fd);
              mchar_fd = init_uart(pPortName, iSecondBaudRate, 1);
              ucLoadPayload = 1;
            }
          }
        } else  // NAK,TIMEOUT,INVALID COMMAND...
        {
          tcflush(mchar_fd, TCIFLUSH);
          fw_upload_Send_Ack(V3_TIMEOUT_ACK);
        }
      }
    }
  }
  return ucResult;
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
static BOOLEAN fw_upload_FW(int8 *pPortName, int32 iBaudRate, int8 *pFileName, int32 iSecondBaudRate) {
  uint8 *pFileBuffer = NULL;
  uint32 ulReadLen = 0;
  FILE *pFile = NULL;
  BOOLEAN bRetVal = FALSE;
  int32 result = 0;
  uint16 uiLenToSend = 0;

  // Open File for reading.
  pFile = fopen(pFileName, "rb");

  if ((mchar_fd < 0) || (pFile == NULL)) {
    PRINT_INFO("\nPort is not open or file not found\n");
    return bRetVal;
  }

  if (iSecondBaudRate != 0) {
    uint32 j = 0;
    result = fw_Change_Baudrate(pPortName, iBaudRate, iSecondBaudRate);
    switch (result) {
      case -1:
        PRINT_INFO("\nSecond baud rate %d is not support\n", iSecondBaudRate);
        PRINT_INFO("\nFw loader only supports the baud rate as");
        for (j = 0; j < sizeof(UartCfgTbl) / sizeof(UART_BAUDRATE); j++) {
          PRINT_INFO(" %d ", UartCfgTbl[j].iBaudRate);
        }
        PRINT_INFO("\n");
        break;
      case -2:
        PRINT_INFO("\n0xa5 or 0xaa is not received after changing baud rate in 2s.\n");
        break;
      default:
        break;
    }
    if (result != 0) {
      return bRetVal;
    }
  }

  // Calculate the size of the file to be downloaded.
  result = fseek(pFile, 0, SEEK_END);
  if (result) {
    PRINT_INFO("\nfseek failed\n");
    return bRetVal;
  }

  ulTotalFileSize = (uint32)ftell(pFile);
  if (!ulTotalFileSize) {
    PRINT_INFO("\nError:Download Size is 0\n");
    return bRetVal;
  }

  pFileBuffer = (uint8 *) malloc(ulTotalFileSize);

  fseek(pFile, 0, SEEK_SET);
  if (pFileBuffer) {
    ulReadLen = fread(pFileBuffer, 1, ulTotalFileSize, pFile);
    if (ulReadLen != ulTotalFileSize) {
      PRINT("\nError:Read File Fail\n");
      return bRetVal;
    }

  }
   ulCurrFileSize = 0;

  // Jump to here in case of protocol resync.
  setjmp(resync);

  while (!bRetVal) {
    // Wait to Receive 0xa5, 0xaa, 0xab, 0xa7
    if (!fw_upload_WaitForHeaderSignature(TIMEOUT_VAL_MILLISEC)) {
      PRINT("\n0xa5,0xaa,0xab or 0xa7 is not received in 4s.\n");
      return bRetVal;
    }

    if (uiProVer == Ver1) {
      // Read the 'Length' bytes requested by Helper
      uiLenToSend = fw_upload_WaitFor_Len(pFile);
      do {
        uiLenToSend = fw_upload_V1SendLenBytes(pFileBuffer, uiLenToSend);
      } while (uiLenToSend != 0);
      // If the Length requested is 0, download is complete.
      if (uiLenToSend == 0) {
        bRetVal = TRUE;
        break;
      }
    } else if (uiProVer == Ver3) {
      fw_upload_WaitFor_Req();
      if (uiNewLen != 0) {
        if (uiNewError == 0) {
#ifdef DEBUG_PRINT
          printf("\n === Succ: REQ = 0xA7, Errcode = 0 ");
#endif
          fw_upload_Send_Ack(V3_REQUEST_ACK);
          fw_upload_V3SendLenBytes(pFileBuffer, uiNewLen, ulNewOffset);

#ifdef DEBUG_PRINT
          printf("\n sent %d bytes..\n", uiNewLen);
#endif
        } else  // NAK,TIMEOUT,INVALID COMMAND...
        {
#ifdef DEBUG_PRINT
          uint8 i;
          printf("\n === Fail: REQ = 0xA7, Errcode != 0 ");
          for (i = 0; i < 7; i++) {
            uiErrCnt[i] += (uiNewError >> i) & 0x1;
          }
#endif
          tcflush(mchar_fd, TCIFLUSH);
          fw_upload_Send_Ack(V3_TIMEOUT_ACK);
        }
      } else {
        /* check if download complete */
        if (uiNewError == 0) {
          fw_upload_Send_Ack(V3_REQUEST_ACK);
          bRetVal = TRUE;
          break;
        } else if (uiNewError & BT_MIC_FAIL_BIT) {
#ifdef DEBUG_PRINT
          uiErrCnt[7] += 1;
#endif
          fw_upload_Send_Ack(V3_REQUEST_ACK);
          fseek(pFile, 0, SEEK_SET);
		  change_baudrata_buffer_len = 0;
          ulCurrFileSize = 0;
          ulLastOffsetToSend = 0xFFFF;
        }
      }
      PRINT_INFO("File downloaded: %8d:%8d\r", ulCurrFileSize, ulTotalFileSize);
    }
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
BOOLEAN bt_vnd_mrvl_check_fw_status() {
  BOOLEAN bRetVal = FALSE;

  if (mchar_fd < 0) {
    PRINT_INFO("\nPort is not open or file not found\n");
    return bRetVal;
  }

  // Wait to Receive 0xa5, 0xaa, 0xab, 0xa7
  bRetVal = fw_upload_WaitForHeaderSignature(4000);

  PRINT_INFO("fw_upload_WaitForHeaderSignature return %d", bRetVal);

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
int bt_vnd_mrvl_download_fw(int8 *pPortName, int32 iBaudrate, int8 *pFileName, int32 iSecondBaudrate) {
  double endTime;
  double endTime1;
  double start;
  double cost;
  uint32 end_sig;

  start = fw_upload_GetTime();

  PRINT_INFO("Protocol: NXP Proprietary\n");
  PRINT_INFO("FW Loader Version: %s\n", VERSION);
  PRINT_INFO("ComPort : %s\n", pPortName);
  PRINT_INFO("BaudRate: %d\n", iBaudrate);
  PRINT_INFO("Filename: %s\n", pFileName);
  PRINT_INFO("iSecondBaudrate: %d\n", iSecondBaudrate);

  if (fw_upload_FW(pPortName, iBaudrate, pFileName, iSecondBaudrate)) {
    PRINT_INFO("\nDownload Complete\n");
    cost = fw_upload_GetTime() - start;
    PRINT_INFO("time:%f\n", cost);
    if (uiProVer == Ver1) {
      fw_upload_DelayInMs(MAX_CTS_TIMEOUT);
      endTime = fw_upload_GetTime() + MAX_CTS_TIMEOUT;
      do {
        if (!fw_upload_ComGetCTS(mchar_fd)) {
          PRINT_INFO("CTS is low\n");
          goto done;
        }
      } while (endTime > fw_upload_GetTime());
      PRINT_INFO("wait CTS low timeout \n");
      goto done;
    } else if (uiProVer == Ver3) {
      endTime = fw_upload_GetTime() + END_SIG_TIMEOUT;
      do {
        fw_upload_ComReadChars(mchar_fd, (uint8 *)&end_sig, 3);
        if (end_sig == END_SIG) {
          endTime1 = fw_upload_GetTime() + MAX_CTS_TIMEOUT;
          do {
            if (!fw_upload_ComGetCTS(mchar_fd)) {
              PRINT_INFO("CTS is low\n");
              goto done;
            }
          } while (endTime > fw_upload_GetTime());
          goto done;
        }
      } while (endTime > fw_upload_GetTime());
      goto done;
    }
  } else {
    PRINT_INFO("\nDownload Error\n");
    return 1;
  }

done:
  bVerChecked = FALSE;
  return 0;
}
