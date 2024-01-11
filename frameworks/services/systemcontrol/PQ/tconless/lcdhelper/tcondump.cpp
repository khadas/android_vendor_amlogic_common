/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <map>

#include "TconRegHandler.h"

#define LCD_TCON_DIR_0  "/sys/class/lcd"
#define LCD_TCON_DIR_1  "/sys/class/aml_lcd/lcd0"

#define BIT(n) (1 << (n))

// for dump PQ
#define DUMP_OPT_ACC BIT(0)
#define DUMP_OPT_OD  BIT(1)
#define DUMP_OPT_LOD BIT(2)
#define DUMP_OPT_VAC BIT(3)
#define DUMP_OPT_DEMURA BIT(4)
#define DUMP_OPT_DITHER BIT(5)

// for dump reg
#define DUMP_OPT_REG BIT(8)

// lod gain mode
#define LOD_MODE_GAIN 'G'

// vac mode
#define VAC_MODE_AVAC 'A'
#define VAC_MODE_IP01 'I'

struct RegDumpInfo {
    TconRegHandler * reghandler;
    int reg;
    int num;
};

#define STEP_TYPE_READ_REG 0
#define STEP_TYPE_WRITE_REG 1
#define STEP_TYPE_WRITE_MASK_REG 2
#define STEP_TYPE_STORE 3
#define STEP_TYPE_RECOVER 4
struct RegDumpStep {
    int type;
    unsigned int reg;
    unsigned int mask;   // for write
    unsigned int val;    // for write
    int num;    // num to read
    const char *desc; //description
};

struct PqDumpInfo {
    int pqtype;  //DUMP_OPT_XXX
    int pqmode;
    const char *name;
    struct RegDumpStep *dumpstep;
    int stepnum;
};

static struct RegDumpStep acc256step[] = {
    { STEP_TYPE_WRITE_REG, 0x600, 0xffffffff, 0x1, 1,   "BL Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 128, "BL"},
    { STEP_TYPE_WRITE_REG, 0x600, 0xffffffff, 0x2, 1,   "BH Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 128, "BH"},
    { STEP_TYPE_WRITE_REG, 0x600, 0xffffffff, 0x4, 1,   "GL Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 128, "GL"},
    { STEP_TYPE_WRITE_REG, 0x600, 0xffffffff, 0x8, 1,   "GH Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 128, "GH"},
    { STEP_TYPE_WRITE_REG, 0x600, 0xffffffff, 0x10, 1,  "RL Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 128, "RL"},
    { STEP_TYPE_WRITE_REG, 0x600, 0xffffffff, 0x20, 1,  "RH Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 128, "RH"},
    { STEP_TYPE_WRITE_REG, 0x600, 0xffffffff, 0x0, 1,   "End Operation"},
};

static struct RegDumpStep acc1024step[] = {
    { STEP_TYPE_WRITE_REG, 0x600, 0xffffffff, 0x1, 1,   "BL Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 512, "BL"},
    { STEP_TYPE_WRITE_REG, 0x600, 0xffffffff, 0x2, 1,   "BH Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 512, "BH"},
    { STEP_TYPE_WRITE_REG, 0x600, 0xffffffff, 0x4, 1,   "GL Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 512, "GL"},
    { STEP_TYPE_WRITE_REG, 0x600, 0xffffffff, 0x8, 1,   "GH Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 512, "GH"},
    { STEP_TYPE_WRITE_REG, 0x600, 0xffffffff, 0x10, 1,  "RL Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 512, "RL"},
    { STEP_TYPE_WRITE_REG, 0x600, 0xffffffff, 0x20, 1,  "RH Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 512, "RH"},
    { STEP_TYPE_WRITE_REG, 0x600, 0xffffffff, 0x0, 1,   "End Operation"},
};

static struct RegDumpStep od17step[] = {
    { STEP_TYPE_WRITE_REG, 0x601, 0xffffffff, 0x1, 1,   "B0 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 81,  "B0"},
    { STEP_TYPE_WRITE_REG, 0x601, 0xffffffff, 0x2, 1,   "G0 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 81,  "G0"},
    { STEP_TYPE_WRITE_REG, 0x601, 0xffffffff, 0x4, 1,   "R0 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 81,  "R0"},
    { STEP_TYPE_WRITE_REG, 0x601, 0xffffffff, 0x8, 1,   "B1 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 72,  "B1"},
    { STEP_TYPE_WRITE_REG, 0x601, 0xffffffff, 0x10, 1,  "G1 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 72,  "G1"},
    { STEP_TYPE_WRITE_REG, 0x601, 0xffffffff, 0x20, 1,  "R1 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 72,  "R1"},
    { STEP_TYPE_WRITE_REG, 0x601, 0xffffffff, 0x40, 1,  "B2 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 72,  "B2"},
    { STEP_TYPE_WRITE_REG, 0x601, 0xffffffff, 0x80, 1,  "G2 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 72,  "G2"},
    { STEP_TYPE_WRITE_REG, 0x601, 0xffffffff, 0x100, 1, "R2 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 72,  "R2"},
    { STEP_TYPE_WRITE_REG, 0x601, 0xffffffff, 0x200, 1, "B3 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 64,  "B3"},
    { STEP_TYPE_WRITE_REG, 0x601, 0xffffffff, 0x400, 1, "G3 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 64,  "G3"},
    { STEP_TYPE_WRITE_REG, 0x601, 0xffffffff, 0x800, 1, "R3 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 64,  "R3"},
    { STEP_TYPE_WRITE_REG, 0x601, 0xffffffff, 0x0, 1,   "End Operation"},
};

static struct RegDumpStep od19step[] = {
    { STEP_TYPE_WRITE_REG, 0x601, 0xffffffff, 0x1, 1,   "B0 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 100,  "B0"},
    { STEP_TYPE_WRITE_REG, 0x601, 0xffffffff, 0x2, 1,   "G0 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 100,  "G0"},
    { STEP_TYPE_WRITE_REG, 0x601, 0xffffffff, 0x4, 1,   "R0 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 100,  "R0"},
    { STEP_TYPE_WRITE_REG, 0x601, 0xffffffff, 0x8, 1,   "B1 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 90,  "B1"},
    { STEP_TYPE_WRITE_REG, 0x601, 0xffffffff, 0x10, 1,  "G1 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 90,  "G1"},
    { STEP_TYPE_WRITE_REG, 0x601, 0xffffffff, 0x20, 1,  "R1 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 90,  "R1"},
    { STEP_TYPE_WRITE_REG, 0x601, 0xffffffff, 0x40, 1,  "B2 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 90,  "B2"},
    { STEP_TYPE_WRITE_REG, 0x601, 0xffffffff, 0x80, 1,  "G2 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 90,  "G2"},
    { STEP_TYPE_WRITE_REG, 0x601, 0xffffffff, 0x100, 1, "R2 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 90,  "R2"},
    { STEP_TYPE_WRITE_REG, 0x601, 0xffffffff, 0x200, 1, "B3 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 81,  "B3"},
    { STEP_TYPE_WRITE_REG, 0x601, 0xffffffff, 0x400, 1, "G3 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 81,  "G3"},
    { STEP_TYPE_WRITE_REG, 0x601, 0xffffffff, 0x800, 1, "R3 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 81,  "R3"},
    { STEP_TYPE_WRITE_REG, 0x601, 0xffffffff, 0x0, 1,   "End Operation"},
};

static struct RegDumpStep od33step[] = {
    { STEP_TYPE_WRITE_REG, 0x601, 0xffffffff, 0x1, 1,   "B0 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 289, "B0"},
    { STEP_TYPE_WRITE_REG, 0x601, 0xffffffff, 0x2, 1,   "G0 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 289, "G0"},
    { STEP_TYPE_WRITE_REG, 0x601, 0xffffffff, 0x4, 1,   "R0 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 289, "R0"},
    { STEP_TYPE_WRITE_REG, 0x601, 0xffffffff, 0x8, 1,   "B1 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 272, "B1"},
    { STEP_TYPE_WRITE_REG, 0x601, 0xffffffff, 0x10, 1,  "G1 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 272, "G1"},
    { STEP_TYPE_WRITE_REG, 0x601, 0xffffffff, 0x20, 1,  "R1 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 272, "R1"},
    { STEP_TYPE_WRITE_REG, 0x601, 0xffffffff, 0x40, 1,  "B2 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 272, "B2"},
    { STEP_TYPE_WRITE_REG, 0x601, 0xffffffff, 0x80, 1,  "G2 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 272, "G2"},
    { STEP_TYPE_WRITE_REG, 0x601, 0xffffffff, 0x100, 1, "R2 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 272, "R2"},
    { STEP_TYPE_WRITE_REG, 0x601, 0xffffffff, 0x200, 1, "B3 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 256, "B3"},
    { STEP_TYPE_WRITE_REG, 0x601, 0xffffffff, 0x400, 1, "G3 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 256, "G3"},
    { STEP_TYPE_WRITE_REG, 0x601, 0xffffffff, 0x800, 1, "R3 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 256, "R3"},
    { STEP_TYPE_WRITE_REG, 0x601, 0xffffffff, 0x0, 1,   "End Operation"},
};

static struct RegDumpStep lod17step[] = {
    { STEP_TYPE_WRITE_REG, 0x606, 0xffffffff, 0x1, 1,   "B0 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 81,  "B0"},
    { STEP_TYPE_WRITE_REG, 0x606, 0xffffffff, 0x2, 1,   "G0 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 81,  "G0"},
    { STEP_TYPE_WRITE_REG, 0x606, 0xffffffff, 0x4, 1,   "R0 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 81,  "R0"},
    { STEP_TYPE_WRITE_REG, 0x606, 0xffffffff, 0x8, 1,   "B1 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 72,  "B1"},
    { STEP_TYPE_WRITE_REG, 0x606, 0xffffffff, 0x10, 1,  "G1 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 72,  "G1"},
    { STEP_TYPE_WRITE_REG, 0x606, 0xffffffff, 0x20, 1,  "R1 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 72,  "R1"},
    { STEP_TYPE_WRITE_REG, 0x606, 0xffffffff, 0x40, 1,  "B2 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 72,  "B2"},
    { STEP_TYPE_WRITE_REG, 0x606, 0xffffffff, 0x80, 1,  "G2 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 72,  "G2"},
    { STEP_TYPE_WRITE_REG, 0x606, 0xffffffff, 0x100, 1, "R2 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 72,  "R2"},
    { STEP_TYPE_WRITE_REG, 0x606, 0xffffffff, 0x200, 1, "B3 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 64,  "B3"},
    { STEP_TYPE_WRITE_REG, 0x606, 0xffffffff, 0x400, 1, "G3 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 64,  "G3"},
    { STEP_TYPE_WRITE_REG, 0x606, 0xffffffff, 0x800, 1, "R3 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 64,  "R3"},
    { STEP_TYPE_WRITE_REG, 0x606, 0xffffffff, 0x0, 1,   "End Operation"},
};

static struct RegDumpStep lod19step[] = {
    { STEP_TYPE_WRITE_REG, 0x606, 0xffffffff, 0x1, 1,   "B0 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 100,  "B0"},
    { STEP_TYPE_WRITE_REG, 0x606, 0xffffffff, 0x2, 1,   "G0 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 100,  "G0"},
    { STEP_TYPE_WRITE_REG, 0x606, 0xffffffff, 0x4, 1,   "R0 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 100,  "R0"},
    { STEP_TYPE_WRITE_REG, 0x606, 0xffffffff, 0x8, 1,   "B1 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 90,  "B1"},
    { STEP_TYPE_WRITE_REG, 0x606, 0xffffffff, 0x10, 1,  "G1 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 90,  "G1"},
    { STEP_TYPE_WRITE_REG, 0x606, 0xffffffff, 0x20, 1,  "R1 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 90,  "R1"},
    { STEP_TYPE_WRITE_REG, 0x606, 0xffffffff, 0x40, 1,  "B2 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 90,  "B2"},
    { STEP_TYPE_WRITE_REG, 0x606, 0xffffffff, 0x80, 1,  "G2 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 90,  "G2"},
    { STEP_TYPE_WRITE_REG, 0x606, 0xffffffff, 0x100, 1, "R2 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 90,  "R2"},
    { STEP_TYPE_WRITE_REG, 0x606, 0xffffffff, 0x200, 1, "B3 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 81,  "B3"},
    { STEP_TYPE_WRITE_REG, 0x606, 0xffffffff, 0x400, 1, "G3 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 81,  "G3"},
    { STEP_TYPE_WRITE_REG, 0x606, 0xffffffff, 0x800, 1, "R3 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 81,  "R3"},
    { STEP_TYPE_WRITE_REG, 0x606, 0xffffffff, 0x0, 1,   "End Operation"},
};

static struct RegDumpStep lod33step[] = {
    { STEP_TYPE_WRITE_REG, 0x606, 0xffffffff, 0x1, 1,   "B0 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 289, "B0"},
    { STEP_TYPE_WRITE_REG, 0x606, 0xffffffff, 0x2, 1,   "G0 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 289, "G0"},
    { STEP_TYPE_WRITE_REG, 0x606, 0xffffffff, 0x4, 1,   "R0 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 289, "R0"},
    { STEP_TYPE_WRITE_REG, 0x606, 0xffffffff, 0x8, 1,   "B1 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 272, "B1"},
    { STEP_TYPE_WRITE_REG, 0x606, 0xffffffff, 0x10, 1,  "G1 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 272, "G1"},
    { STEP_TYPE_WRITE_REG, 0x606, 0xffffffff, 0x20, 1,  "R1 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 272, "R1"},
    { STEP_TYPE_WRITE_REG, 0x606, 0xffffffff, 0x40, 1,  "B2 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 272, "B2"},
    { STEP_TYPE_WRITE_REG, 0x606, 0xffffffff, 0x80, 1,  "G2 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 272, "G2"},
    { STEP_TYPE_WRITE_REG, 0x606, 0xffffffff, 0x100, 1, "R2 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 272, "R2"},
    { STEP_TYPE_WRITE_REG, 0x606, 0xffffffff, 0x200, 1, "B3 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 256, "B3"},
    { STEP_TYPE_WRITE_REG, 0x606, 0xffffffff, 0x400, 1, "G3 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 256, "G3"},
    { STEP_TYPE_WRITE_REG, 0x606, 0xffffffff, 0x800, 1, "R3 Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 256, "R3"},
    { STEP_TYPE_WRITE_REG, 0x606, 0xffffffff, 0x0, 1,   "End Operation"},
};

static struct RegDumpStep lodgainstep[] = {
    { STEP_TYPE_WRITE_REG, 0x608, 0xffffffff, 0x1, 1,    "LOD Gain Switch on"},
    { STEP_TYPE_READ_REG,  0x700, 0xffffffff, 0x0, 2135, "LOD Gain"},
    { STEP_TYPE_WRITE_REG, 0x608, 0xffffffff, 0x0, 1,    "End Operation"},
};

static struct RegDumpStep avacstep[] = {
    { STEP_TYPE_STORE,           0x2a4, 0xffffffff, 0x0,   1,   "Store reg 0x2a4"},
    { STEP_TYPE_WRITE_MASK_REG,  0x2a4, 0x12,       0x0,   1,   "Get operate permission"},
    { STEP_TYPE_WRITE_REG,       0x60a, 0xffffffff, 0x10,  1,   "ram_t3_1 switch on"},
    { STEP_TYPE_READ_REG,        0x700, 0xffffffff, 0x0,   128, "ram_t3_1(RH, 0x10)"},
    { STEP_TYPE_WRITE_REG,       0x60a, 0xffffffff, 0x20,  1,   "ram_t3_2 switch on"},
    { STEP_TYPE_READ_REG,        0x700, 0xffffffff, 0x0,   128, "ram_t3_2(RL, 0x20)"},
    { STEP_TYPE_WRITE_REG,       0x60a, 0xffffffff, 0x40,  1,   "ram_t3_3 switch on"},
    { STEP_TYPE_READ_REG,        0x700, 0xffffffff, 0x0,   128, "ram_t3_3(GH, 0x40)"},
    { STEP_TYPE_WRITE_REG,       0x60a, 0xffffffff, 0x80,  1,   "ram_t3_4 switch on"},
    { STEP_TYPE_READ_REG,        0x700, 0xffffffff, 0x0,   128, "ram_t3_4(GL, 0x80)"},
    { STEP_TYPE_WRITE_REG,       0x60a, 0xffffffff, 0x100, 1,   "ram_t3_5 switch on"},
    { STEP_TYPE_READ_REG,        0x700, 0xffffffff, 0x0,   128, "ram_t3_5(BH, 0x100)"},
    { STEP_TYPE_WRITE_REG,       0x60a, 0xffffffff, 0x200, 1,   "ram_t3_6 switch on"},
    { STEP_TYPE_READ_REG,        0x700, 0xffffffff, 0x0,   128, "ram_t3_6(BL, 0x200)"},
    { STEP_TYPE_WRITE_REG,       0x60a, 0xffffffff, 0x400, 1,   "ram_t3_1 switch on"},
    { STEP_TYPE_READ_REG,        0x700, 0xffffffff, 0x0,   128, "ram_t3_1(RH, 0x400)"},
    { STEP_TYPE_WRITE_REG,       0x60a, 0xffffffff, 0x800, 1,   "ram_t3_2 switch on"},
    { STEP_TYPE_READ_REG,        0x700, 0xffffffff, 0x0,   128, "ram_t3_2(RL, 0x800)"},
    { STEP_TYPE_WRITE_REG,       0x60a, 0xffffffff, 0x1000,1,   "ram_t3_3 switch on"},
    { STEP_TYPE_READ_REG,        0x700, 0xffffffff, 0x0,   128, "ram_t3_3(GH, 0x1000)"},
    { STEP_TYPE_WRITE_REG,       0x60a, 0xffffffff, 0x2000,1,   "ram_t3_4 switch on"},
    { STEP_TYPE_READ_REG,        0x700, 0xffffffff, 0x0,   128, "ram_t3_4(GL, 0x2000)"},
    { STEP_TYPE_WRITE_REG,       0x60a, 0xffffffff, 0x4000,1,   "ram_t3_5 switch on"},
    { STEP_TYPE_READ_REG,        0x700, 0xffffffff, 0x0,   128, "ram_t3_5(BH, 0x4000)"},
    { STEP_TYPE_WRITE_REG,       0x60a, 0xffffffff, 0x8000,1,   "ram_t3_6 switch on"},
    { STEP_TYPE_READ_REG,        0x700, 0xffffffff, 0x0,   128, "ram_t3_6(BL, 0x8000)"},
    { STEP_TYPE_WRITE_REG,       0x60a, 0xffffffff, 0x0,   1,   "End Operation"},
    { STEP_TYPE_RECOVER,         0x2a4, 0xffffffff, 0x0,   1,   "Recover reg 0x2a4"},
};

static struct RegDumpStep vacip01step[] = {
    { STEP_TYPE_STORE,           0x2a4, 0xffffffff, 0x0,   1,   "Store reg 0x2a4"},
    { STEP_TYPE_WRITE_MASK_REG,  0x2a4, 0x12,       0x0,   1,   "Get operate permission"},
    { STEP_TYPE_WRITE_REG,       0x60a, 0xffffffff, 0x1,   1,   "ram_t1 switch on"},
    { STEP_TYPE_READ_REG,        0x700, 0xffffffff, 0x0,   256, "ram_t1(0x1)"},
    { STEP_TYPE_WRITE_REG,       0x60a, 0xffffffff, 0x2,   1,   "ram_t1 switch on"},
    { STEP_TYPE_READ_REG,        0x700, 0xffffffff, 0x0,   256, "ram_t1(0x2)"},
    { STEP_TYPE_WRITE_REG,       0x60a, 0xffffffff, 0x4,   1,   "ram_t2 switch on"},
    { STEP_TYPE_READ_REG,        0x700, 0xffffffff, 0x0,   256, "ram_t2(0x4)"},
    { STEP_TYPE_WRITE_REG,       0x60a, 0xffffffff, 0x8,   1,   "ram_t2 switch on"},
    { STEP_TYPE_READ_REG,        0x700, 0xffffffff, 0x0,   256, "ram_t2(0x8)"},
    { STEP_TYPE_WRITE_REG,       0x60a, 0xffffffff, 0x10,  1,   "ram_t3_1 switch on"},
    { STEP_TYPE_READ_REG,        0x700, 0xffffffff, 0x0,   128, "ram_t3_1(RH, 0x10)"},
    { STEP_TYPE_WRITE_REG,       0x60a, 0xffffffff, 0x20,  1,   "ram_t3_2 switch on"},
    { STEP_TYPE_READ_REG,        0x700, 0xffffffff, 0x0,   128, "ram_t3_2(RL, 0x20)"},
    { STEP_TYPE_WRITE_REG,       0x60a, 0xffffffff, 0x40,  1,   "ram_t3_3 switch on"},
    { STEP_TYPE_READ_REG,        0x700, 0xffffffff, 0x0,   128, "ram_t3_3(GH, 0x40)"},
    { STEP_TYPE_WRITE_REG,       0x60a, 0xffffffff, 0x80,  1,   "ram_t3_4 switch on"},
    { STEP_TYPE_READ_REG,        0x700, 0xffffffff, 0x0,   128, "ram_t3_4(GL, 0x80)"},
    { STEP_TYPE_WRITE_REG,       0x60a, 0xffffffff, 0x100, 1,   "ram_t3_5 switch on"},
    { STEP_TYPE_READ_REG,        0x700, 0xffffffff, 0x0,   128, "ram_t3_5(BH, 0x100)"},
    { STEP_TYPE_WRITE_REG,       0x60a, 0xffffffff, 0x200, 1,   "ram_t3_6 switch on"},
    { STEP_TYPE_READ_REG,        0x700, 0xffffffff, 0x0,   128, "ram_t3_6(BL, 0x200)"},
    { STEP_TYPE_WRITE_REG,       0x60a, 0xffffffff, 0x400, 1,   "ram_t3_1 switch on"},
    { STEP_TYPE_READ_REG,        0x700, 0xffffffff, 0x0,   128, "ram_t3_1(RH, 0x400)"},
    { STEP_TYPE_WRITE_REG,       0x60a, 0xffffffff, 0x800, 1,   "ram_t3_2 switch on"},
    { STEP_TYPE_READ_REG,        0x700, 0xffffffff, 0x0,   128, "ram_t3_2(RL, 0x800)"},
    { STEP_TYPE_WRITE_REG,       0x60a, 0xffffffff, 0x1000,1,   "ram_t3_3 switch on"},
    { STEP_TYPE_READ_REG,        0x700, 0xffffffff, 0x0,   128, "ram_t3_3(GH, 0x1000)"},
    { STEP_TYPE_WRITE_REG,       0x60a, 0xffffffff, 0x2000,1,   "ram_t3_4 switch on"},
    { STEP_TYPE_READ_REG,        0x700, 0xffffffff, 0x0,   128, "ram_t3_4(GL, 0x2000)"},
    { STEP_TYPE_WRITE_REG,       0x60a, 0xffffffff, 0x4000,1,   "ram_t3_5 switch on"},
    { STEP_TYPE_READ_REG,        0x700, 0xffffffff, 0x0,   128, "ram_t3_5(BH, 0x4000)"},
    { STEP_TYPE_WRITE_REG,       0x60a, 0xffffffff, 0x8000,1,   "ram_t3_6 switch on"},
    { STEP_TYPE_READ_REG,        0x700, 0xffffffff, 0x0,   128, "ram_t3_6(BL, 0x8000)"},
    { STEP_TYPE_WRITE_REG,       0x60a, 0xffffffff, 0x0,   1,   "End Operation"},
    { STEP_TYPE_RECOVER,         0x2a4, 0xffffffff, 0x0,   1,   "Recover reg 0x2a4"},
};

static struct PqDumpInfo pqDumpTbl[] = {
    {DUMP_OPT_ACC, 256,  "ACC 256",  acc256step,  sizeof(acc256step)/sizeof(struct RegDumpStep)},
    {DUMP_OPT_ACC, 1024, "ACC 1024", acc1024step, sizeof(acc1024step)/sizeof(struct RegDumpStep)},
    {DUMP_OPT_OD,  17,   "OD 17",    od17step,    sizeof(od17step)/sizeof(struct RegDumpStep)},
    {DUMP_OPT_OD,  19,   "OD 19",    od19step,    sizeof(od19step)/sizeof(struct RegDumpStep)},
    {DUMP_OPT_OD,  33,   "OD 33",    od33step,    sizeof(od33step)/sizeof(struct RegDumpStep)},
    {DUMP_OPT_LOD, 17,   "LOD 17",   lod17step,   sizeof(lod17step)/sizeof(struct RegDumpStep)},
    {DUMP_OPT_LOD, 19,   "LOD 19",   lod19step,   sizeof(lod19step)/sizeof(struct RegDumpStep)},
    {DUMP_OPT_LOD, 33,   "LOD 33",   lod33step,   sizeof(lod33step)/sizeof(struct RegDumpStep)},
    {DUMP_OPT_LOD, LOD_MODE_GAIN, "LOD GAIN", lodgainstep, sizeof(lodgainstep)/sizeof(struct RegDumpStep)},
    {DUMP_OPT_VAC, VAC_MODE_AVAC, "AVAC",     avacstep,    sizeof(avacstep)/sizeof(struct RegDumpStep)},
    {DUMP_OPT_VAC, VAC_MODE_IP01, "VAC IP01", vacip01step, sizeof(vacip01step)/sizeof(struct RegDumpStep)},
};

static struct PqDumpInfo* findMatchedForDump(int opt, int mode)
{
    unsigned int i = 0;
    for (i = 0; i < (sizeof(pqDumpTbl)/sizeof(struct PqDumpInfo)); i++) {
        if (pqDumpTbl[i].pqtype == opt && pqDumpTbl[i].pqmode == mode)
            return &pqDumpTbl[i];
    }
    return NULL;
}

static const char *stepTypeToStr(int type)
{
    switch (type) {
    case STEP_TYPE_READ_REG:       return "READ_REG";
    case STEP_TYPE_WRITE_REG:      return "WRITE_REG";
    case STEP_TYPE_WRITE_MASK_REG: return "WRITE_MASK_REG";
    case STEP_TYPE_STORE:          return "STORE";
    case STEP_TYPE_RECOVER:        return "RECOVER";
    default:                       return "UNKNOWN";
    }
}

static void pqdump(TconRegHandler *regHandler, struct PqDumpInfo *pqinfo)
{
    int i = 0, j = 0, tblidx = 0;
    int val = -1;
    bool result = false;
    std::vector<unsigned int> pqtbl;
    std::map<unsigned int, unsigned int> storedreg;  //store
    std::map<unsigned int, unsigned int>::iterator it;

    if (!pqinfo || !regHandler)
        return;

    // get table
    for (i = 0; i < pqinfo->stepnum; i++) {
        struct RegDumpStep *step = &pqinfo->dumpstep[i];
        printf("<step %d>: type=%s, reg=%#x, mask=%#x, val=%#x, num=%d, desc=%s\n",
            i, stepTypeToStr(step->type), step->reg, step->mask, step->val,
            step->num, step->desc);
        switch (step->type) {
        case STEP_TYPE_READ_REG:
            result = regHandler->getRegs(step->reg, step->num, pqtbl);
            break;
        case STEP_TYPE_WRITE_REG:
            result = regHandler->setReg(step->reg, step->val);
            break;
        case STEP_TYPE_WRITE_MASK_REG:
            result = regHandler->setReg(step->reg, step->mask, step->val);
            break;
        case STEP_TYPE_STORE:
            val = regHandler->getReg(step->reg);
            if (val >= 0) {
                storedreg.insert(std::pair<unsigned int, unsigned int>(step->reg, val));
                result = true;
            }
            break;
        case STEP_TYPE_RECOVER:
            it = storedreg.find(step->reg);
            if (it != storedreg.end()) {
                result = regHandler->setReg(step->reg, it->second);
                storedreg.erase(it);
            }
            break;
        default: break;
        }
        if (!result)
            goto __pqdump_exit;
    }

    // display lut table
    if (pqinfo->name) printf("*********************%s*********************\n", pqinfo->name);
    else printf("*********************Dump begin*********************\n");
    for (i = 0; i < pqinfo->stepnum; i++) {
        struct RegDumpStep *step = &pqinfo->dumpstep[i];
        if (STEP_TYPE_READ_REG == step->type) {
            if (step->desc) printf("[%s]-->\n", step->desc);
            else printf("[step %d]-->\n", i);
            for (j = 0; j < step->num; j++, tblidx++) {
                if (!j)  // first one
                    printf("%#x", pqtbl[tblidx]);
                else
                    printf(",%#x", pqtbl[tblidx]);
            }
            printf("\n");
        }
    }

__pqdump_exit:
    if (!result)
        printf("Reg operate fail, pls check...\n");
}

static void regdump(TconRegHandler *regHandler, int reg, int regnum, bool fmtaddr)
{
    int i = 0;
    std::vector<unsigned int> vec;
    if (regHandler->getRegs(reg, regnum, vec)) {
        if (regnum != vec.size())
            printf("Warning: Regnum(%d) != Readednum(%d)...\n", regnum, vec.size());
        if (fmtaddr) {
            for (i = 0; i < vec.size(); i++) {
                if (i % 4 == 0) {
                    printf("%s%08x: ", i==0?"":"\n", reg + i);
                }
                printf("%08x ", vec[i]);
            }
            printf("\n");
        } else {
            for (i = 0; i < regnum; i++)
                printf("Reg [%#x]=%#010x\n", reg+i, vec[i]);
        }
    }
}

static void help(char *appName)
{
    printf(
        "Usage:\n"
        "  %s [-h] [-d] [-a <mode>] [-o <mode>] [-l <mode>] [-v <mode>]\n"
        "  %s [-r <reg> [num]] [-f]\n"
        "\n"
        "Parameters:\n"
        "  -h : show this help\n"
        "  -d : show more detail debug message\n"
        "  -a <mode> : dump acc lut, supported mode: 256/1024\n"
        "  -o <mode> : dump od  lut, supported mode: 17/19/33\n"
        "  -l <mode> : dump lod lut/gain, supported mode: 17/19/33/gain\n"
        "  -v <mode> : dump vac lut, supported mode: avac/ip01\n"
        "  -r <reg> <num>: dump a field of regs, from reg to reg+num\n"
        "  -r <reg>      : dump reg value\n"
        "  -f        : display regs in formatted address\n",
        appName,
        appName
    );
}

int main(int argc, char *argv[])
{
    int result = -1;
    int ch = 0;
    unsigned int dump_opt = 0;
    int reg=0, regnum=1;
    int accmode=0, odmode=0, lodmode=0, vacmode=-1;
    bool fmtaddr = false;
    struct PqDumpInfo *pqinfo = NULL;
    char *ptr = NULL;
    const char *opt_str = "hda:o:l:v:r::f";
    struct option opt_l_str[] = {
        {"help", no_argument,       NULL, 'h'},
        {"debug",no_argument,       NULL, 'd'},
        {"acc",  required_argument, NULL, 'a'},
        {"od",   required_argument, NULL, 'o'},
        {"lod",  required_argument, NULL, 'l'},
        {"vac",  required_argument, NULL, 'v'},
        {"reg",  optional_argument, NULL, 'r'},
        {"fmtaddr", no_argument,    NULL, 'f'},
    };
    TconRegHandler *regHandler = new TconRegHandler;
    if (!regHandler)
        goto __main_exit;
    while ((ch = getopt_long(argc, argv, opt_str, opt_l_str, NULL)) != -1) {
        printf("argc=%d, ch=%c, optind=%d, optarg=%s\n", argc, ch, optind, optarg);
        switch (ch) {
        case 'h': help(argv[0]); exit(0); break;
        case 'd': regHandler->setDumpTo(0, true); break;
        case 'a':
            dump_opt |= DUMP_OPT_ACC;
            if (optarg != NULL) accmode = atoi(optarg);
            break;
        case 'o':
            dump_opt |= DUMP_OPT_OD;
            if (optarg != NULL) odmode = atoi(optarg);
            break;
        case 'l':
            dump_opt |= DUMP_OPT_LOD;
            if (optarg != NULL) {
                if (!strcasecmp(optarg, "gain"))
                    lodmode = LOD_MODE_GAIN;
                else
                    lodmode = atoi(optarg);
            }
            break;
        case 'v':
            dump_opt |= DUMP_OPT_VAC;
            if (optarg != NULL) {
                if (!strcasecmp(optarg, "avac"))
                    vacmode = VAC_MODE_AVAC;
                else if (!strcasecmp(optarg, "ip01"))
                    vacmode = VAC_MODE_IP01;
            }
            break;
        case 'r':
            dump_opt |= DUMP_OPT_REG;
            if (optind < argc)
                reg = strtol(argv[optind], &ptr, 0);
            if ((optind + 1) < argc) {
                regnum = strtol(argv[optind+1], &ptr, 0);
                if (regnum == LONG_MIN || regnum == LONG_MAX)
                    regnum = 1;
            }
            break;
        case 'f': fmtaddr = true; break;
        default: help(argv[0]); exit(0); break;
        }
    }

    if (argc < 2) {  //no any argument
        help(argv[0]);
        return 0;
    }

    if (!regHandler->init(LCD_TCON_DIR_0)
         && !regHandler->init(LCD_TCON_DIR_1)) {
        printf("Init fail...\n");
        goto __main_exit;
    }
    if (dump_opt & DUMP_OPT_REG) {
        regdump(regHandler, reg, regnum, fmtaddr);
    }
    if (dump_opt & DUMP_OPT_ACC) {
        pqinfo = findMatchedForDump(DUMP_OPT_ACC, accmode);
        if (pqinfo) pqdump(regHandler, pqinfo);
        else printf("Not support ACC %d\n", accmode);
    }
    if (dump_opt & DUMP_OPT_OD) {
        pqinfo = findMatchedForDump(DUMP_OPT_OD, odmode);
        if (pqinfo) pqdump(regHandler, pqinfo);
        else printf("Not support OD %d\n", odmode);
    }
    if (dump_opt & DUMP_OPT_LOD) {
        pqinfo = findMatchedForDump(DUMP_OPT_LOD, lodmode);
        if (pqinfo) pqdump(regHandler, pqinfo);
        else printf("Not support LOD %d\n", lodmode);
    }
    if (dump_opt & DUMP_OPT_VAC) {
        pqinfo = findMatchedForDump(DUMP_OPT_VAC, vacmode);
        if (pqinfo) pqdump(regHandler, pqinfo);
        else printf("Not support VAC mode, pls check\n");
    }
    result = 0;

__main_exit:
    if (regHandler) {
        regHandler->uninit();
        delete regHandler;
    }
    return result;
}

