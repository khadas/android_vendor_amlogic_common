
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "PQTable.h"
#include "pq/adap_pq.h"

#define PQ_BIN_DEFAULT_PATH               "/vendor/etc/tvconfig/pq/pq.bin"

static char PQBinPath[128] = "\0";
static bool IsloadFromBinFile = false;

extern TABLE_VER_PQ                  mVerInfoPQ;
extern PQ_TABLE_PARAM                mPQTableParam;
extern pq_tcon_gamma_table_t         mGammaTable;

PQTable::PQTable()
{
    memset(m_PQTable, 0, sizeof(PQ_TABLE_STRUCT));
}

PQTable::~PQTable()
{

}

void PQTable::Init()
{
    bool ret = false;
    char PQ_TablePath[128];
    *PQ_TablePath = '\0';

    Get_PQBinName(PQ_TablePath);
    ret = Load_PQBin(PQ_TablePath);

    if (!ret) {
        printf("[%s][%d] Load_PQBin can not get = %s use default table!!!\n",__func__,__LINE__, PQ_TablePath);
        IsloadFromBinFile = false;
    } else {
        IsloadFromBinFile = true;
    }

    return;
}

bool PQTable::Get_PQBinName(char *name)
{
    if (name == NULL) {
        return false;
    }

    if (strlen(PQBinPath) > 0) {
        strcpy(name, PQBinPath);
    } else {
        strcpy(name, PQ_BIN_DEFAULT_PATH);
    }

    return true;
}

bool PQTable::Load_PQBin(char *name)
{
    if (name == NULL) {
        return false;
    }

    FILE *pFILE = NULL;
    pFILE = fopen(name, "r");

    if (pFILE == NULL) {
        printf("[%s][%d] open File: %s Fail\n",__func__,__LINE__, name);
        return false;
    }

    for (int i = PQ_TABLE_VERSION; i < PQ_TABLE_MAX; i++) {
        if (PQ_TableLoader_GetTable(pFILE, (PQ_TABLE_TYPE)i, &m_PQTable[i]) == false) {
            printf("[%s][%d] index = %d Fail\n",__func__,__LINE__, i);
            fclose(pFILE);
            return false;
        }
    }
    fclose(pFILE);

    //copy PQ table data to memory
    #if 1
        memcpy(&mVerInfoPQ, (TABLE_VER_PQ*)m_PQTable[PQ_TABLE_VERSION].pTableArray, sizeof(TABLE_VER_PQ));
        memcpy(&mPQTableParam, (PQ_TABLE_PARAM*)m_PQTable[PQ_TABLE_DATA].pTableArray, sizeof(PQ_TABLE_PARAM));
        memcpy(&mGammaTable, (pq_tcon_gamma_table_t*)m_PQTable[PQ_TABLE_GAMMA_DATA].pTableArray, sizeof(pq_tcon_gamma_table_t));
    #else
    //send PQ table data to driver
        ADAP_PQ_SetPQTable((PQ_TABLE_PARAM*)m_PQTable[PQ_TABLE_DATA].pTableArray);
    #endif

    free(m_PQTable);

    return true;
}

bool PQTable::Set_PQBinPath(char *path)
{
    if (path == NULL) {
        return false;
    }

    sprintf(PQBinPath, "%s", path);

    return true;
}

bool PQTable::Set_VPQ_GammaTable(int index)
{
    if (index >= GAMMA_TABLE_NUM_MAX) {
        return false;
    }

    vpp_gamma_ch_table_s Gamma_R;
    vpp_gamma_ch_table_s Gamma_G;
    vpp_gamma_ch_table_s Gamma_B;

    for (int i = 0; i < 256; i++) {
        Gamma_R.data[i] = mGammaTable.GammaData[index].R[i];
        Gamma_G.data[i] = mGammaTable.GammaData[index].G[i];
        Gamma_B.data[i] = mGammaTable.GammaData[index].B[i];
    }

    if (ADAP_PQ_SetGammaChannel_R(&Gamma_R) != ADAP_OK) {
        printf("[%s][%d] fail\n",__func__,__LINE__);
    }

    if (ADAP_PQ_SetGammaChannel_G(&Gamma_G) != ADAP_OK) {
        printf("[%s][%d] fail\n",__func__,__LINE__);
    }

    if (ADAP_PQ_SetGammaChannel_B(&Gamma_B) != ADAP_OK) {
        printf("[%s][%d] fail\n",__func__,__LINE__);
    }

    return true;
}

bool PQTable::Set_VPQ_SharpnessTable(int index)
{
    return true;
}

bool PQTable::Set_VPQ_SRTable(int index)
{
    return true;
}

bool PQTable::Set_VPQ_CM2Table(int index)
{
    return true;
}

bool PQTable::Set_VPQ_LocalContrastTable(int index)
{
    return true;
}

bool PQTable::Set_VPQ_DNLPTable(int index)
{
    return true;
}

void* PQTable::GetPQTableData(PQ_TABLE_TYPE type, int *indexTabLen)
{//Foor test
    if (IsloadFromBinFile == true) {

        switch (type) {
        case PQ_TABLE_VERSION: {
            TABLE_VER_PQ *pTableArray = NULL;
            pTableArray = (TABLE_VER_PQ*)m_PQTable[PQ_TABLE_VERSION].pTableArray;
            *indexTabLen = 1;
            return (void*)pTableArray;
        }
        case PQ_TABLE_DATA: {
            PQ_TABLE_PARAM *pTableArray = NULL;
            pTableArray = (PQ_TABLE_PARAM*)m_PQTable[PQ_TABLE_DATA].pTableArray;
            *indexTabLen = 1;
            return (void*)pTableArray;
        }
        default:
            break;
        }
    }
    else
    {
        switch (type) {
            case PQ_TABLE_VERSION:
                *indexTabLen = 1;
                return (void*)&mVerInfoPQ;
            case PQ_TABLE_DATA:
                *indexTabLen = 1;
                return (void*)&mPQTableParam;
            default:
                break;
        }
    }

    return NULL;
}

PQTable *PQTable::mInstance = NULL;
PQTable *PQTable::GetInstance()
{
    if (NULL == mInstance) {
        mInstance = new PQTable();
    }
    return mInstance;
}
