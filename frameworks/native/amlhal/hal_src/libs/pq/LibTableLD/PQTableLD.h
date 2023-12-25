
#ifndef _PQ_TABLE_LD_H
#define _PQ_TABLE_LD_H

#include <cstdio>
#include <cassert>
#include <vector>
#include <algorithm>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "PQTableLDType.h"
#include "PQTableLDLoader.h"

class PQTableLD       {
public:
    PQTableLD();
    ~PQTableLD();
    static PQTableLD *GetInstance();
    void Init(void);
    bool Set_PQBinPath(char *path);
    void* GetPQTableLDData(PQ_TABLE_LD_TYPE type);

    void PQLD_StructTable_Init(void);
    void PQLD_BlMapping_Init(void);
    void PQLD_BlProfile_Init(void);

private:
    static PQTableLD *mInstance;
    bool Get_PQLDBinName(char *name);
    bool Load_PQLDBin(char *name);

protected:
    PQ_LD_TABLE_STRUCT         m_PQTableLD[LD_TABLE_MAX];


};

#endif

