
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "PQTableLD.h"
#include "pq/adap_ld.h"

#define LDIM_BIN_DEFAULT_PATH               "/mnt/vendor/param/pq/ldim.bin"

static char LdimBinPath[128] = "\0";
static bool IsloadFromBinFile = false;

#define MAX_TABLE_SIZE                  0x300000

extern TABLE_VER_PQ_LD                  mVerInfoPQLD;
extern TABLE_STRUCT_PQ_LD               mLDTable;

PQTableLD::PQTableLD()
{
    memset(m_PQTableLD, 0, sizeof(PQ_LD_TABLE_STRUCT));
}

PQTableLD::~PQTableLD()
{

}

void PQTableLD::Init(void)
{
    PQLD_StructTable_Init();
    PQLD_BlMapping_Init();
    PQLD_BlProfile_Init();

    return;
}

void PQTableLD::PQLD_StructTable_Init(void)
{
    bool ret = false;
    char PQ_LD_TablePath[128];
    *PQ_LD_TablePath = '\0';

    Get_PQLDBinName(PQ_LD_TablePath);
    ret = Load_PQLDBin(PQ_LD_TablePath);

    if (!ret) {
        AML_LOG_DEBUG(LOG_LD, "%s ldim bin can not get = %s use default table!!! \n", __FUNCTION__, PQ_LD_TablePath);
        IsloadFromBinFile = false;
    } else {
        AML_LOG_DEBUG(LOG_LD, "%s ldim bin from %s \n", __FUNCTION__, PQ_LD_TablePath);
        IsloadFromBinFile = true;
    }

    am_pq_bin_param_t pData;
    memset(&pData, 0x0,sizeof(am_pq_bin_param_t));

    pData.table_index  = 0;
    pData.table_len = sizeof(TABLE_STRUCT_PQ_LD);
    pData.table_ptr = malloc(pData.table_len);
    pData.table_ptr = (void *)&mLDTable;

    ADAP_LD_SetLdStructTable(&pData);

    return;
}

void PQTableLD::PQLD_BlMapping_Init(void)
{
    int fd = -1;
    int dataSize = 0;
    unsigned char *dataBuff = NULL;
    unsigned char buf[MAX_TABLE_SIZE/sizeof(unsigned char)] = {0};

    am_pq_bin_param_t para;
    memset(&para, 0x0, sizeof(am_pq_bin_param_t));

    aml_path_t Path;

    if (ADAP_LD_GetLdBLMappingPath(&Path) != ADAP_OK) {
        AML_LOG_DEBUG(LOG_LD, "%s can not ADAP_LD_GetLdBLMappingPath!! \n", __FUNCTION__);
        return;
    }

    if (!(access(Path.string, F_OK) == 0)) {
        AML_LOG_DEBUG(LOG_LD, "%s BlMapping_Path : %s is not access!! \n", __FUNCTION__, Path.string);
        return;
    }

    if ((fd = open(Path.string, O_RDONLY)) < 0) {
        AML_LOG_DEBUG(LOG_LD, "%s open BlMapping_Path : %s fail!! \n", __FUNCTION__, Path.string);
        return;
    } else {
        dataSize = read(fd, buf, sizeof(buf));
        if (dataSize > 0) {
            dataBuff = (unsigned char *)malloc(dataSize);
            if (dataBuff != NULL) {
                memset(dataBuff, 0x0, dataSize);
            } else {
                AML_LOG_DEBUG(LOG_LD, "%s malloc memory fail!!! \n", __FUNCTION__);
                close(fd);
                free(dataBuff);
                return;
            }
            memcpy((void *)dataBuff, buf, dataSize);
            para.table_index = LD_BIN_BL_MAPPING;
            para.table_len = dataSize;
            para.table_ptr = (void *)dataBuff;
        }
    }

    ADAP_LD_SetLdBLMapping(&para);

    close(fd);
    if (dataBuff)
        free(dataBuff);

    AML_LOG_DEBUG(LOG_LD, "%s DONE \n", __FUNCTION__);

    return;
}

void PQTableLD::PQLD_BlProfile_Init(void)
{
    int fd = -1;
    int dataSize = 0;
    unsigned char *dataBuff = NULL;
    unsigned char buf[MAX_TABLE_SIZE/sizeof(unsigned char)] = {0};

    am_pq_bin_param_t para;
    memset(&para, 0x0, sizeof(am_pq_bin_param_t));

    aml_path_t Path;

    if (ADAP_LD_GetLdBLProfilePath(&Path) != ADAP_OK) {
        AML_LOG_DEBUG(LOG_LD, "%s can not ADAP_LD_GetLdBLProfilePath!! \n", __FUNCTION__);
        return;
    }

    if (!(access(Path.string, F_OK) == 0)) {
        AML_LOG_DEBUG(LOG_LD, "%s BlProfile path : %s is not access!! \n", __FUNCTION__, Path.string);
        return;
    }

    if ((fd = open(Path.string, O_RDONLY)) < 0) {
        AML_LOG_DEBUG(LOG_LD, "%s open BlProfile path : %s fail!! \n", __FUNCTION__, Path.string);
        return;
    } else {
        dataSize = read(fd, buf, sizeof(buf));
        if (dataSize > 0) {
            dataBuff = (unsigned char *)malloc(dataSize);
            if (dataBuff != NULL) {
                memset(dataBuff, 0x0, dataSize);
            } else {
                AML_LOG_DEBUG(LOG_LD, "%s malloc memory fail!!! \n", __FUNCTION__);
                close(fd);
                free(dataBuff);
                return;
            }
            memcpy((void *)dataBuff, buf, dataSize);
            para.table_index = LD_BIN_BL_PROFILE;
            para.table_len = dataSize;
            para.table_ptr = (void *)dataBuff;
        } else {
            AML_LOG_DEBUG(LOG_LD, "%s bin fila is NULL!!! \n", __FUNCTION__);
            close(fd);
            return;
        }
    }

    ADAP_LD_SetLdBLProfile(&para);

    close(fd);
    if (dataBuff)
        free(dataBuff);

    AML_LOG_DEBUG(LOG_LD, "%s DONE \n", __FUNCTION__);

    return;
}



bool PQTableLD::Get_PQLDBinName(char *name)
{
    if (name == NULL) {
        return false;
    }

    if (strlen(LdimBinPath) > 0) {
        strcpy(name, LdimBinPath);
    } else {
        strcpy(name, LDIM_BIN_DEFAULT_PATH);
    }

    return true;
}

bool PQTableLD::Load_PQLDBin(char *name)
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

#if 0 // support gen bin from script
    for (int i = PQ_TABLE_LD_VERSION; i < PQ_TABLE_LD_MAX; i++) {
        if (PQLD_TableLoader_GetTable(pFILE, (PQ_TABLE_LD_TYPE)i, &m_PQTableLD[i]) == false) {
            printf("[%s][%d] index = %d Fail\n",__func__,__LINE__, i);
            fclose(pFILE);
            return false;
        }
    }

    memcpy(&mVerInfoPQLD, (TABLE_VER_PQ_LD*)m_PQTableLD[PQ_TABLE_LD_VERSION].pTableArray, sizeof(TABLE_VER_PQ_LD));
    memcpy(&mLDTable, (TABLE_STRUCT_PQ_LD*)m_PQTableLD[PQ_TABLE_LD_DATA].pTableArray, sizeof(TABLE_STRUCT_PQ_LD));


    fclose(pFILE);
    free(m_PQTableLD);
#else //support gen bin from tool
    TABLE_STRUCT_PQ_LD pdata;
    memset(&pdata, 0, sizeof(TABLE_STRUCT_PQ_LD));

    if (PQLD_TableLoader_GetTableNew(pFILE, (void *)&pdata) != true) {
        fclose(pFILE);
        return false;
    }
    memcpy(&mLDTable, &pdata, sizeof(TABLE_STRUCT_PQ_LD));

    fclose(pFILE);
#endif

    return true;
}

bool PQTableLD::Set_PQBinPath(char *path)
{
    if (path == NULL) {
        return false;
    }

    sprintf(LdimBinPath, "%s", path);

    return true;
}


void* PQTableLD::GetPQTableLDData(PQ_TABLE_LD_TYPE type)
{

    switch (type) {
        case PQ_TABLE_LD_VERSION:
            return (void*)&mVerInfoPQLD;
        case PQ_TABLE_LD_DATA:
            return (void*)&mLDTable;
        default:
            break;
    }

    return NULL;
}

PQTableLD *PQTableLD::mInstance = NULL;
PQTableLD *PQTableLD::GetInstance()
{
    if (NULL == mInstance) {
        mInstance = new PQTableLD();
    }
    return mInstance;
}
