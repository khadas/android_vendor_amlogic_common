#include <string.h>
#include <stdlib.h>
#include "PQTableLDLoader.h"

extern TABLE_VER_PQ_LD               mVerInfoPQLD;
extern TABLE_STRUCT_PQ_LD            mLDTable;

#define MAX_TABLE_SIZE 128
#define INT_VALUE_MAX_RANGE    2147483647 //1<<31 -1


bool GetLDVerBuffer(void** ppBuffer, int* piBufferLen)
{
    int iSize = sizeof(PQ_LD_TABLE_STRUCT_HEADER) + sizeof(TABLE_VER_PQ_LD);
    void* pBuffer = malloc(iSize);
    unsigned char* pCurPtr = (unsigned char*)pBuffer;
    if (pCurPtr == NULL) {
        return false;
    }

    PQ_LD_TABLE_STRUCT_HEADER header;
    memset(&header, 0, sizeof(PQ_LD_TABLE_STRUCT_HEADER));
    header.TotalSize = iSize;
    header.TableSize = sizeof(TABLE_VER_PQ_LD);
    header.TableOffset = LD_TABLE_VERSION;
    header.TableNum = 1;

    memcpy(pCurPtr, &header, sizeof(header));
    pCurPtr += sizeof(header);
    memcpy(pCurPtr, &mVerInfoPQLD, sizeof(TABLE_VER_PQ_LD));

    *ppBuffer = pBuffer;
    *piBufferLen = iSize;
    return true;
}

bool GetLDTableBuffer(void** ppBuffer, int* piBufferLen)
{
    int iSize = sizeof(PQ_LD_TABLE_STRUCT_HEADER) + sizeof(TABLE_STRUCT_PQ_LD);
    void* pBuffer = malloc(iSize);
    unsigned char* pCurPtr = (unsigned char*)pBuffer;
    if (pCurPtr == NULL) {
        return false;
    }

    PQ_LD_TABLE_STRUCT_HEADER header;
    memset(&header, 0, sizeof(PQ_LD_TABLE_STRUCT_HEADER));
    header.TotalSize = iSize;
    header.TableSize = sizeof(TABLE_STRUCT_PQ_LD);
    header.TableOffset = LD_TABLE_DATA;
    header.TableNum = 1;

    memcpy(pCurPtr, &header, sizeof(header));
    pCurPtr += sizeof(header);
    memcpy(pCurPtr, &mLDTable, sizeof(TABLE_STRUCT_PQ_LD));

    *ppBuffer = pBuffer;
    *piBufferLen = iSize;

    return true;
}

bool PQTableLDGenerate(char* pPanelFile)
{
    if (NULL == pPanelFile)
        return false;

    FILE *pFile = fopen(pPanelFile, "wb");
    if (NULL == pFile)
        return false;

    bool ret = false;
    PQ_LD_FILE_HEADER header;
    memset(&header, 0, sizeof(header));

    void* PqLdVerBuf = NULL;
    int PqLdVerLen = 0;
    ret |= !GetLDVerBuffer(&PqLdVerBuf, &PqLdVerLen);

    void* PqLdTableDataBuf = NULL;
    int PqLdTableDataLen = 0;
    ret |= !GetLDTableBuffer(&PqLdTableDataBuf, &PqLdTableDataLen);

    if (!ret) {
        header.Size = sizeof(header);
        header.LDVerOffset = 0;
        header.LDDataOffset = PqLdVerLen;
        header.chip = 0;
        header.crc = 0;

        fwrite(&header, sizeof(header), 1, pFile);
        fwrite(PqLdVerBuf, PqLdVerLen, 1, pFile);
        fwrite(PqLdTableDataBuf, PqLdTableDataLen, 1, pFile);
    }

    if (PqLdVerBuf != NULL)
        free(PqLdVerBuf);
    if (PqLdTableDataBuf != NULL)
        free(PqLdTableDataBuf);

    fclose(pFile);

    return true;
}

bool PQLD_TableLoader_GetTable(FILE *pPanelFile, PQ_TABLE_LD_TYPE type, PQ_LD_TABLE_STRUCT* pTable)
{
    if (NULL == pPanelFile || NULL == pTable)
        return false;

    int ret_read = -1, ret_t_header = -1, ret_t_arry = -1, ret_index_t = -1, ret_seek = -1;
    unsigned int iOffset = 0;

    printf("%s %d type = %d\n", __func__, __LINE__, type);

    rewind(pPanelFile);

    PQ_LD_FILE_HEADER header;
    memset(&header, 0, sizeof(header));

    ret_read = fread(&header, sizeof(PQ_LD_FILE_HEADER), 1, pPanelFile);
    if (ret_read <= 0)    return false;

    switch (type) {
        case LD_TABLE_VERSION:
            iOffset = header.LDVerOffset;
            break;
        case LD_TABLE_DATA:
            iOffset = header.LDDataOffset;
            break;
        default:
            return false;
    }

    if (iOffset > INT_VALUE_MAX_RANGE) {
        printf("%s %d iOffset out of range\n",__func__, __LINE__);
        return false;
    }
    ret_seek = fseek(pPanelFile, iOffset, SEEK_CUR);
    if (ret_seek != 0) {
        printf("%s %d fseek fail!!!\n",__func__, __LINE__);
        return false;
    }

    ret_t_header = fread(&(pTable->header), sizeof(PQ_LD_TABLE_STRUCT_HEADER), 1, pPanelFile);
    if (ret_t_header <= 0) {
        printf("%s %d fread fail!!!\n",__func__, __LINE__);
        return false;
    }

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

bool PQLD_TableLoader_GetTableNew(FILE *pPanelFile, void *pTable)
{
    if (NULL == pPanelFile || NULL == pTable)
        return false;

    rewind(pPanelFile);

    int BinVer = -1;
    if (fread(&BinVer, sizeof(int), 1, pPanelFile) <= 0) {
        printf("%s %d fread binVer fail!!!\n",__func__, __LINE__);
        return false;
    }

    int ret_t_arry = fread(pTable, sizeof(TABLE_STRUCT_PQ_LD), 1, pPanelFile);
    if (ret_t_arry <= 0) {
        printf("%s %d read pTableArray fail!!!\n",__func__, __LINE__);
        return false;
    }

    printf("%s %d load bin data end\n", __func__, __LINE__);
    return true;
}

