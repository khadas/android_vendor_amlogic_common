/******************************************************************************
 *
 *  Copyright 2018-2021 NXP
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

#ifndef _BT_VENDOR_NXP_H
#define _BT_VENDOR_NXP_H
/*===================== Include Files ============================================*/
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>


/*==================== Typedefs =================================================*/
typedef unsigned long long uint64;
typedef unsigned int uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;
typedef int int32;
typedef short int16;
typedef char int8;
typedef unsigned char BOOLEAN;


/*===================== Macros ===================================================*/

#define BT_HAL_VERSION      "008.001"

#define TIMEOUT_SEC 6
#define RW_SUCCESSFUL (1)
#define RW_FAILURE (~RW_SUCCESSFUL)

#define BIT(x) (0x1 << x)

#define TRUE 1
#define FALSE 0
/*===================== Global Vars ==============================================*/

/*==================== Function Prototypes ======================================*/
int32 init_uart(int8 * dev, int32 dwBaudRate, uint8 ucFlowCtrl);
void bt_bdaddress_set(void);
#endif 
