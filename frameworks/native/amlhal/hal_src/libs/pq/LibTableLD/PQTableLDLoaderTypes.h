#ifndef __PQ_TABLE_LD_LOADER_TYPES_H__
#define __PQ_TABLE_LD_LOADER_TYPES_H__

#pragma pack(4) //4byte

typedef enum _LD_TABLE_TYPE {
    LD_TABLE_VERSION = 0,
    LD_TABLE_DATA,
    LD_TABLE_MAX,
} LD_TABLE_TYPE;

typedef struct _PQ_LD_FILE_HEADER {
    int Size;
    int LDVerOffset;
    int LDDataOffset;
    int chip;
    unsigned int crc;
} PQ_LD_FILE_HEADER;

typedef struct _PQ_LD_TABLE_STRUCT_HEADER {
    unsigned int    TotalSize;
    unsigned int    TableSize;
    unsigned int    IndexTableSize;
    unsigned char   TableOffset;
    unsigned char   TableNum;
    unsigned char   IndexTableNum;
    unsigned char   Reserved;
} PQ_LD_TABLE_STRUCT_HEADER;

typedef struct _PQ_LD_TABLE_STRUCT {
    PQ_LD_TABLE_STRUCT_HEADER  header;
    void*                      pTableArray;
    void*                      pIndexTable;
} PQ_LD_TABLE_STRUCT;

typedef struct _PQ_LD_TABLE_DATA_STRUCT_SAVE {
        char source;
        char timing;
        short tableDataIdx;
        unsigned int tableDataLen;
} PQ_LD_TABLE_DATA_STRUCT_SAVE;

#pragma pack()

#endif
