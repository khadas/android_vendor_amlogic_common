#ifndef __PQ_TABLE_LD_LOADER_H__
#define __PQ_TABLE_LD_LOADER_H__

#include <stdio.h>
#include "PQTableLDLoaderTypes.h"
#include "PQTableLDType.h"

bool PQTableLDGenerate(char* pPanelFile);
bool PQLD_TableLoader_GetTable(FILE *pPanelFile, PQ_TABLE_LD_TYPE type, PQ_LD_TABLE_STRUCT* pTable);
bool PQLD_TableLoader_GetTableNew(FILE *pPanelFile, void *pTable);

#endif
