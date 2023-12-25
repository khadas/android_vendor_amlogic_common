#include <string.h>
#include <stdlib.h>
#include "PQTableLoader.h"
#include "PQTableType.h"

extern TABLE_VER_PQ                  mVerInfoPQ;
extern PQ_TABLE_PARAM                mPQTableParam;
extern pq_tcon_gamma_table_t         mGammaTable;

#define MAX_TABLE_SIZE 128
#define INT_VALUE_MAX_RANGE    2147483647 //1<<31 -1


bool GetPqVerBuffer(void** ppBuffer, int* piBufferLen)
{
    int iSize = sizeof(PQ_TABLE_STRUCT_HEADER) + sizeof(TABLE_VER_PQ);
    void* pBuffer = malloc(iSize);
    unsigned char* pCurPtr = (unsigned char*)pBuffer;
    if (pCurPtr == NULL) {
        return false;
    }

    PQ_TABLE_STRUCT_HEADER header;
    memset(&header, 0, sizeof(PQ_TABLE_STRUCT_HEADER));
    header.TotalSize = iSize;
    header.TableSize = sizeof(TABLE_VER_PQ);
    header.TableOffset = PQ_TABLE_VERSION;
    header.TableNum = 1;

    memcpy(pCurPtr, &header, sizeof(header));
    pCurPtr += sizeof(header);
    memcpy(pCurPtr, &mVerInfoPQ, sizeof(TABLE_VER_PQ));

    *ppBuffer = pBuffer;
    *piBufferLen = iSize;
    return true;
}

bool GetPQTableParamBuffer(void** ppBuffer, int* piBufferLen)
{
    int iSize = sizeof(PQ_TABLE_STRUCT_HEADER) + sizeof(PQ_TABLE_PARAM);
    void* pBuffer = malloc(iSize);
    unsigned char* pCurPtr = (unsigned char*)pBuffer;
    if (pCurPtr == NULL) {
        return false;
    }

    PQ_TABLE_STRUCT_HEADER header;
    memset(&header, 0, sizeof(PQ_TABLE_STRUCT_HEADER));
    header.TotalSize = iSize;
    header.TableSize = sizeof(PQ_TABLE_PARAM);
    header.TableOffset = PQ_TABLE_DATA;
    header.TableNum = 1;

    memcpy(pCurPtr, &header, sizeof(header));
    pCurPtr += sizeof(header);
    memcpy(pCurPtr, &mPQTableParam, sizeof(PQ_TABLE_PARAM));

    *ppBuffer = pBuffer;
    *piBufferLen = iSize;

    return true;
}

bool GetGammaTableBuffer(void** ppBuffer, int* piBufferLen)
{
    int iSize = sizeof(PQ_TABLE_STRUCT_HEADER) + sizeof(pq_tcon_gamma_table_t);
    void* pBuffer = malloc(iSize);
    unsigned char* pCurPtr = (unsigned char*)pBuffer;
    if (pCurPtr == NULL) {
        return false;
    }

    PQ_TABLE_STRUCT_HEADER header;
    memset(&header, 0, sizeof(PQ_TABLE_STRUCT_HEADER));
    header.TotalSize = iSize;
    header.TableSize = sizeof(pq_tcon_gamma_table_t);
    header.TableOffset = PQ_TABLE_GAMMA_DATA;
    header.TableNum = 1;

    memcpy(pCurPtr, &header, sizeof(header));
    pCurPtr += sizeof(header);
    memcpy(pCurPtr, &mGammaTable, sizeof(pq_tcon_gamma_table_t));


    *ppBuffer = pBuffer;
    *piBufferLen = iSize;

    return true;
}

bool PQTableGenerate(char* pPanelFile)
{
    if (NULL == pPanelFile)
        return false;

    FILE *pFile = fopen(pPanelFile, "wb");
    if (NULL == pFile)
        return false;

    bool ret = false;
    PQ_FILE_HEADER header;
    memset(&header, 0, sizeof(header));

    void* PqVerBuf = NULL;
    int PqVerLen = 0;
    ret |= !GetPqVerBuffer(&PqVerBuf, &PqVerLen);

    void* PqTableDataBuf = NULL;
    int PqTableDataLen = 0;
    ret |= !GetPQTableParamBuffer(&PqTableDataBuf, &PqTableDataLen);

    void* PqGammaDataBuf = NULL;
    int PqGammaDataLen = 0;
    ret |= !GetGammaTableBuffer(&PqGammaDataBuf, &PqGammaDataLen);

    if (!ret) {
        header.Size = sizeof(header);
        header.PqVerOffset = 0;
        header.PqTableDataOffset = PqVerLen;
        header.PqGammaDataOffset = PqVerLen + PqTableDataLen;
        header.chip = 0;
        header.crc = 0;

        fwrite(&header, sizeof(header), 1, pFile);
        fwrite(PqVerBuf, PqVerLen, 1, pFile);
        fwrite(PqTableDataBuf, PqTableDataLen, 1, pFile);
        fwrite(PqGammaDataBuf, PqGammaDataLen, 1, pFile);

    }

    if (PqVerBuf != NULL)
        free(PqVerBuf);
    if (PqTableDataBuf != NULL)
        free(PqTableDataBuf);
    if (PqGammaDataBuf != NULL)
        free(PqGammaDataBuf);

    fclose(pFile);

    return true;
}

bool PQ_TableLoader_GetTable(FILE *pPanelFile, PQ_TABLE_TYPE type, PQ_TABLE_STRUCT* pTable)
{
    if (NULL == pPanelFile || NULL == pTable)
        return false;

    int ret_read = -1, ret_t_header = -1, ret_t_arry = -1, ret_index_t = -1, ret_seek = -1;
    unsigned int iOffset = 0;

    printf("%s %d type = %d\n", __func__, __LINE__, type);

    rewind(pPanelFile);

    PQ_FILE_HEADER header;
    memset(&header, 0, sizeof(header));

    ret_read = fread(&header, sizeof(PQ_FILE_HEADER), 1, pPanelFile);
    if (ret_read <= 0)    return false;

    switch (type) {
        case PQ_TABLE_VERSION:
            iOffset = header.PqVerOffset;
            break;
        case PQ_TABLE_DATA:
            iOffset = header.PqTableDataOffset;
            break;
        case PQ_TABLE_GAMMA_DATA:
            iOffset = header.PqGammaDataOffset;
            break;
        default:
            return false;
    }
    printf("%s %d iOffset = %d\n", __func__, __LINE__, iOffset);
    if (iOffset > INT_VALUE_MAX_RANGE)     return false;


    ret_seek = fseek(pPanelFile, iOffset, SEEK_CUR);
    if (ret_seek != 0) {
        printf("%s %d fseek fail!!!\n",__func__, __LINE__);
        return false;
    }

    ret_t_header = fread(&(pTable->header), sizeof(PQ_TABLE_STRUCT_HEADER), 1, pPanelFile);
    if (ret_t_header <= 0) {
        printf("%s %d fread fail!!!\n",__func__, __LINE__);
        return false;
    }

    printf("%s %d TableSize = %d, IndexTableSize = %d, TotalSize = %d\n", __func__, __LINE__,
            pTable->header.TableSize, pTable->header.IndexTableSize, pTable->header.TotalSize);

    if (pTable->header.TableSize > 0 && pTable->header.TableSize < INT_VALUE_MAX_RANGE) {
        pTable->pTableArray = malloc(pTable->header.TableSize);
        if (pTable->pTableArray == NULL) {
            printf("%s %d malloc TableSize memory fail!!!\n",__func__, __LINE__);
            return false;
        }

        ret_t_arry = fread(pTable->pTableArray, pTable->header.TableSize, 1, pPanelFile);
        if (ret_t_arry <= 0) {
            printf("%s %d read pTableArray fail!!!\n",__func__, __LINE__);
            return false;
        }
    } else {
        printf("%s %d header.TableSize size NG %d\n", __func__, __LINE__, pTable->header.TableSize);
    }

    if (pTable->header.IndexTableSize > 0 && pTable->header.IndexTableSize < INT_VALUE_MAX_RANGE) {
        pTable->pIndexTable = malloc(pTable->header.IndexTableSize);
        if (pTable->pIndexTable == NULL) {
            printf("%s %d malloc IndexTableSize memory fail!!!\n",__func__, __LINE__);
            return false;
        }

        ret_index_t = fread(pTable->pIndexTable, pTable->header.IndexTableSize, 1, pPanelFile);
        if (ret_index_t <= 0) {
            printf("%s %d read iIndexTableSize fail!!!\n",__func__, __LINE__);
            return false;
        }
    } else {
        printf("%s %d header.IndexTableSize size NG\n", __func__, __LINE__);
    }

    printf("%s %d load bin data end\n", __func__, __LINE__);

    return true;
}
