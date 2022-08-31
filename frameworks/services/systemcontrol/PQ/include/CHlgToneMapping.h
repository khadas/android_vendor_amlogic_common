/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */


#ifndef _C_CHLGTONEMAPPING_H
#define _C_CHLGTONEMAPPING_H

#define OE_X 149
#define MAX_32 0xffffffff

typedef unsigned long long u64;

class CHlgToneMapping {
public:
    CHlgToneMapping();
    ~CHlgToneMapping();
    int hlg_sdr_process(unsigned int panell, int *gainValue);

private:
    int _gen_oe_x_val(u64 *oe_x_val);
    double hlg_sdr_alg(unsigned int panell, double in_o);
};

#endif
