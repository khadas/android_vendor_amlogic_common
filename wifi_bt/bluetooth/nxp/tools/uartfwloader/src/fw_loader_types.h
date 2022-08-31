/** @file   fw_loader_types.h
 *
 *  @brief  This file contains the Nxp specific typedefinitions of 
 *          standatd ANSI-C data types.
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
#ifndef FW_LOADER_TYPES_H
#define FW_LOADER_TYPES_H

/*===================== Macros ===================================================*/
#define RW_SUCCESSFUL   (1)
#define RW_FAILURE      (0)

#define	BIT(x)		(0x1 << x)

#define TRUE  1
#define FALSE 0
/*==================== Typedefs =================================================*/
typedef unsigned long long uint64;
typedef unsigned int uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;
typedef int int32;
typedef short int16;
typedef char int8;
typedef unsigned char BOOLEAN;
/*===================== Global Vars ==============================================*/

/*==================== Function Prototypes ======================================*/

#endif // FW_LOADER_TYPES_H
