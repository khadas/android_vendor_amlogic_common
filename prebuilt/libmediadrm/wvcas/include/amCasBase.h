/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef AMCAS_BASE_H
#define AMCAS_BASE_H
#include <stdint.h>

typedef enum
{
	AM_CAS_SUCCESS,
	AM_CAS_ERROR,
	AM_CAS_ERR_SYS,
} AmCasCode_t;

class  AmCasBase {

public:
	AmCasBase() {};
	virtual ~AmCasBase() {};
	virtual AmCasCode_t openSession(uint8_t *sessionId) = 0;
	virtual AmCasCode_t provision();
	virtual AmCasCode_t setPrivateData(void *iDate, int iSize);
	virtual AmCasCode_t closeSession(uint8_t *sessionId) = 0;

private:
	unsigned int mCaSystemId;
};

#endif
