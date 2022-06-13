/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */


#define LOG_TAG "SystemControl"
#define LOG_TV_TAG "CHlgToneMapping"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "CHlgToneMapping.h"
#include "CPQLog.h"

CHlgToneMapping::CHlgToneMapping() {

}

CHlgToneMapping::~CHlgToneMapping() {

}

int CHlgToneMapping::_gen_oe_x_val(u64 *oe_x_val) {
    unsigned int i = 0, j = 0;
    int bin_num = 0;
    int temp1 = 0, temp2 = 0;

    oe_x_val[0] = 0;

    for (i = 1; i < 8; i++) {
        bin_num++;
        temp1 = 1 << ((i - 1) + 4);
        oe_x_val[i] = (u64)temp1;
    }

    for (j = 11; j < 20; j++) {
        for (i = 0; i <  4; i++) {
            bin_num++;
            temp1 = 1 << (j - 2);
            temp2 = 1 << j;
            oe_x_val[bin_num] = (u64)temp1 * i + (u64)temp2;
        }
    }

    for (j = 20; j < 31; j++) {
        for (i = 0; i < 8; i++) {
            bin_num++;
            temp1 = 1 << (j - 3);
            temp2 = 1 << j;
            oe_x_val[bin_num] = (u64)temp1 * i + (u64)temp2;
        }
    }

    for (i = 0; i < 16; i++) {
        bin_num++;
        oe_x_val[bin_num] = (1 << (31 - 4)) * i + (1 << 31);
    }
    oe_x_val[OE_X - 1] = MAX_32;

    return 0;
}

double CHlgToneMapping::hlg_sdr_alg(unsigned int panell, double in_o) {
    double r, p;
    double gain;

    r = 1.2 + 0.42 * log10((double)panell / (double)1000);
    p = r - 1;
    if (in_o == 0)
        in_o+= 1 / pow(2, 32);
    gain = pow(in_o, p);
    // SYS_LOGD("%s: gain = %lf\n", __FUNCTION__, gain);
    return gain;
}

int CHlgToneMapping::hlg_sdr_process(unsigned int panell, int *gainValue) {
    int ret = -1;
    if (gainValue == NULL) {
        SYS_LOGD("%s: gainValue is NULL.\n", __FUNCTION__);
    } else {
        u64 eo_x_u[OE_X] = {0};
        double eo_x[OE_X] = {0};
        double gain[OE_X] = {0};
        int i = 0;

        _gen_oe_x_val(eo_x_u);

        for (i = 0; i < OE_X; i++) {
            eo_x[i] = eo_x_u[i] / (double)MAX_32;
            gain[i] = hlg_sdr_alg(panell, eo_x[i]);
        }

        for (i = 0; i < OE_X; i++) {
            gain[i] *= 1 << 9;
            // SYS_LOGD("%s: gain[%d] = %lf\n", __FUNCTION__, i, gain[i]);
            gainValue[i] = (int)gain[i];
        }
        ret = 0;
    }

    return ret;
}
