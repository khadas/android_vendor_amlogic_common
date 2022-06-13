/** @file mfgdebug.h
 *
 *  @brief This file contains MFG bridge code.
 *
 * (C) Copyright 2011-2017 Marvell International Ltd. All Rights Reserved
 *
 * MARVELL CONFIDENTIAL
 * The source code contained or described herein and all documents related to
 * the source code ("Material") are owned by Marvell International Ltd or its
 * suppliers or licensors. Title to the Material remains with Marvell International Ltd
 * or its suppliers and licensors. The Material contains trade secrets and
 * proprietary and confidential information of Marvell or its suppliers and
 * licensors. The Material is protected by worldwide copyright and trade secret
 * laws and treaty provisions. No part of the Material may be used, copied,
 * reproduced, modified, published, uploaded, posted, transmitted, distributed,
 * or disclosed in any way without Marvell's prior express written permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery
 * of the Materials, either expressly, by implication, inducement, estoppel or
 * otherwise. Any license under such intellectual property rights must be
 * express and approved by Marvell in writing.
 *
 */


#ifndef _MFGDEBUG_H
#define _MFGDEBUG_H

#define BIT(x)    ( 1 << (x) )

#define DBG_ERROR   BIT(0)      // error msg
#define DBG_WARN    BIT(1)      // warning msg
#define DBG_FLOW    BIT(2)      // procedure flow
#define DBG_MSG     BIT(3)      // control msg
#define DBG_SINFO   BIT(4)      // special info msg
#define DBG_MINFO   BIT(5)      // minor info msg
#define DBG_GINFO   BIT(6)      // general info msg
#define DBG_ENTRY   BIT(7)      // function entry
#define DBG_EXIT    BIT(8)      // function exit

#define DBG_MASK   (DBG_ERROR | DBG_MINFO | DBG_GINFO | DBG_MSG)

void mfg_dprint(int level, char *fmt, ...);
void mfg_hexdump(int dbg_level, const char *title, const unsigned char *buf,
                 int len);

#endif
