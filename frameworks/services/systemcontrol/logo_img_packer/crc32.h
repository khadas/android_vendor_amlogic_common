/*
 * Copyright (c) 2015 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 */
/*
 * crc32.h
 *
 *  Created on: 2013-5-31
 *      Author: binsheng.xu@amlogic.com
 */

#ifndef CRC32_H_
#define CRC32_H_

unsigned calc_logoimg_crc(int fd, off_t offset, unsigned checkSz);
int check_img_crc(int fd, off_t offset, const unsigned orgCrc, unsigned checkSz);

#endif /* CRC32_H_ */
