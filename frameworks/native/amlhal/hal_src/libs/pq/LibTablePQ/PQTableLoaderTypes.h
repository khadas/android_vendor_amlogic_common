#ifndef __PQ_TABLE_LOADER_TYPES_H__
#define __PQ_TABLE_LOADER_TYPES_H__

#pragma pack(4) //4byte

typedef enum _PQ_TABLE_TYPE {
    PQ_TABLE_VERSION = 0,
    PQ_TABLE_DATA,
    PQ_TABLE_GAMMA_DATA,
    PQ_TABLE_MAX,
} PQ_TABLE_TYPE;

typedef struct _PQ_FILE_HEADER {
    int Size;
    int PqVerOffset;
    int PqTableDataOffset;
    int PqGammaDataOffset;
    int chip;
    unsigned int crc;
} PQ_FILE_HEADER;

typedef struct _PQ_TABLE_STRUCT_HEADER {
    unsigned int    TotalSize;
    unsigned int    TableSize;
    unsigned int    IndexTableSize;
    unsigned char   TableOffset;
    unsigned char   TableNum;
    unsigned char   IndexTableNum;
    unsigned char   Reserved;
} PQ_TABLE_STRUCT_HEADER;

typedef struct _PQ_TABLE_STRUCT {
    PQ_TABLE_STRUCT_HEADER     header;
    void*                      pTableArray;
    void*                      pIndexTable;
} PQ_TABLE_STRUCT;

typedef struct _PQ_TABLE_DATA_STRUCT_SAVE {
        char source;
        char timing;
        short tableDataIdx;
        unsigned int tableDataLen;
} PQ_TABLE_DATA_STRUCT_SAVE;

#pragma pack()

#endif
