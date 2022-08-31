/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "SystemControl"
#define LOG_TV_TAG "CConfigFile"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

#include "CPQLog.h"
#include "CFile.h"
#include "CConfigFile.h"
#ifdef PRODUCT_SUPPORT_COMPRESS_DB
#include "Minizip.h"
#endif

CConfigFile *CConfigFile::mInstance = NULL;
CConfigFile *CConfigFile::GetInstance()
{
    if (NULL == mInstance) {
        mInstance = new CConfigFile();
    }

    return mInstance;
}

CConfigFile::CConfigFile()
{
    mpFirstSection = NULL;
    mpFileName[0]  = '\0';
    mpConfigFile   = NULL;
    mpFirstLine    = NULL;

    if (isFileExist(PQ_CONFIG_DEFAULT_PATH_0)) {
        LoadFromFile(PQ_CONFIG_DEFAULT_PATH_0);
    } else if (isFileExist(PQ_CONFIG_DEFAULT_PATH_1)) {
        LoadFromFile(PQ_CONFIG_DEFAULT_PATH_1);
    } else {
        SYS_LOGE("no pq_default.ini in %s and %s\n", PQ_CONFIG_DEFAULT_PATH_0, PQ_CONFIG_DEFAULT_PATH_1);
    }
}

CConfigFile::~CConfigFile()
{
    SYS_LOGD("%\n", __FUNCTION__);
    FreeAllMem();
}

bool CConfigFile::isFileExist(const char *file_name)
{
    struct stat tmp_st;
    int ret = -1;

    ret = stat(file_name, &tmp_st);
    if (ret != 0 ) {
       SYS_LOGE("%s don't exist!\n",file_name);
       return false;
    } else {
       return true;
    }
}

int CConfigFile::LoadFromFile(const char *filename)
{
    char   lineStr[MAX_CONFIG_FILE_LINE_LEN];
    char   *pStr;
    LINE *pCurLINE = NULL;
    SECTION *pCurSection = NULL;

    FreeAllMem();

    if (filename == NULL) {
        SYS_LOGE("%s: config file path is null!\n");
        return -1;
    }

    SYS_LOGD("LoadFromFile name = %s", filename);
    strcpy(mpFileName, filename);
    if ((mpConfigFile = fopen (mpFileName, "r")) == NULL) {
        return -1;
    }

    while (fgets (lineStr, MAX_CONFIG_FILE_LINE_LEN, mpConfigFile) != NULL) {
        allTrim(lineStr);

        LINE *pLINE = new LINE();
        pLINE->pKeyStart = pLINE->Text;
        pLINE->pKeyEnd = pLINE->Text;
        pLINE->pValueStart = pLINE->Text;
        pLINE->pValueEnd = pLINE->Text;
        pLINE->pNext = NULL;
        pLINE->type = getLineType(lineStr);
        //LOGD("getline=%s len=%d type=%d", lineStr, strlen(lineStr), pLINE->type);
        strcpy(pLINE->Text, lineStr);
        pLINE->LineLen = strlen(pLINE->Text);

        //head
        if (mpFirstLine == NULL) {
            mpFirstLine = pLINE;
        } else {
            pCurLINE->pNext = pLINE;
        }

        pCurLINE = pLINE;

        switch (pCurLINE->type) {
        case LINE_TYPE_SECTION: {
            SECTION *pSec = new SECTION();
            pSec->pLine = pLINE;
            pSec->pNext = NULL;
            if (mpFirstSection == NULL) { //first section
                mpFirstSection = pSec;
            } else {
                pCurSection->pNext = pSec;
            }
            pCurSection = pSec;
            break;
        }
        case LINE_TYPE_KEY: {
            char *pM = strchr(pCurLINE->Text, '=');
            pCurLINE->pKeyStart = pCurLINE->Text;
            pCurLINE->pKeyEnd = pM - 1;
            pCurLINE->pValueStart = pM + 1;
            pCurLINE->pValueEnd = pCurLINE->Text + pCurLINE->LineLen - 1;
            break;
        }
        case LINE_TYPE_COMMENT:
        default:
            break;
        }
    }

    fclose (mpConfigFile);
    mpConfigFile = NULL;
    return 0;
}

int CConfigFile::SaveToFile(const char *filename)
{
    const char *filepath = NULL;
    FILE *pFile = NULL;

    if (filename == NULL) {
        if (strlen(mpFileName) == 0) {
            SYS_LOGE("error save file is null");
            return -1;
        } else {
            filepath = mpFileName;
        }
    } else {
        filepath = filename;
    }
    //SYS_LOGD("Save to file name = %s", file);

    if ((pFile = fopen (filepath, "wb")) == NULL) {
        SYS_LOGD("Save to file open error = %s", filepath);
        return -1;
    }

    LINE *pCurLine = NULL;
    for (pCurLine = mpFirstLine; pCurLine != NULL; pCurLine = pCurLine->pNext) {
        fprintf (pFile, "%s\r\n", pCurLine->Text);
    }

    fflush(pFile);
    fsync(fileno(pFile));
    fclose(pFile);
    return 0;
}

int CConfigFile::SetString(const char *section, const char *key, const char *value)
{
    SECTION *pNewSec = NULL;
    LINE *pNewSecLine = NULL;
    LINE *pNewKeyLine = NULL;

    SECTION *pSec = getSection(section);
    if (pSec == NULL) {
        pNewSec = new SECTION();
        pNewSecLine = new LINE();
        pNewKeyLine = new LINE();

        pNewKeyLine->type = LINE_TYPE_KEY;
        pNewSecLine->type = LINE_TYPE_SECTION;


        sprintf(pNewSecLine->Text, "[%s]", section);
        pNewSec->pLine = pNewSecLine;

        InsertSection(pNewSec);

        int keylen = strlen(key);
        sprintf(pNewKeyLine->Text, "%s=%s", key, value);
        pNewKeyLine->LineLen = strlen(pNewKeyLine->Text);
        pNewKeyLine->pKeyStart = pNewKeyLine->Text;
        pNewKeyLine->pKeyEnd = pNewKeyLine->pKeyStart + keylen - 1;
        pNewKeyLine->pValueStart = pNewKeyLine->pKeyStart + keylen + 1;
        pNewKeyLine->pValueEnd = pNewKeyLine->Text + pNewKeyLine->LineLen - 1;

        InsertKeyLine(pNewSec, pNewKeyLine);

    } else { //find section
        LINE *pLine = getKeyLineAtSec(pSec, key);
        if (pLine == NULL) { //, not find key
            pNewKeyLine = new LINE();
            pNewKeyLine->type = LINE_TYPE_KEY;

            int keylen = strlen(key);
            sprintf(pNewKeyLine->Text, "%s=%s", key, value);
            pNewKeyLine->LineLen = strlen(pNewKeyLine->Text);
            pNewKeyLine->pKeyStart = pNewKeyLine->Text;
            pNewKeyLine->pKeyEnd = pNewKeyLine->pKeyStart + keylen - 1;
            pNewKeyLine->pValueStart = pNewKeyLine->pKeyStart + keylen + 1;
            pNewKeyLine->pValueEnd = pNewKeyLine->Text + pNewKeyLine->LineLen - 1;

            InsertKeyLine(pSec, pNewKeyLine);
        } else { //all find, change it
            sprintf(pLine->Text, "%s=%s", key, value);
            pLine->LineLen = strlen(pLine->Text);
            pLine->pValueEnd = pLine->Text + pLine->LineLen - 1;
        }
    }

    //save
    SaveToFile(NULL);
    return 0;
}

int CConfigFile::SetInt(const char *section, const char *key, int value)
{
    char tmp[64];
    sprintf(tmp, "%d", value);
    SetString(section, key, tmp);
    return 0;
}

const char *CConfigFile::GetString(const char *section, const char *key, const char *def_value)
{
    SECTION *pSec = getSection(section);
    if (pSec == NULL) {
        return def_value;
    }
    LINE *pLine = getKeyLineAtSec(pSec, key);
    if (pLine == NULL) {
        return def_value;
    }
    return pLine->pValueStart;
}

int CConfigFile::GetInt(const char *section, const char *key, int def_value)
{
    const char *num = GetString(section, key, NULL);
    if (num != NULL) {
        return atoi(num);
    }
    return def_value;
}

int CConfigFile::SetFloat(const char *section, const char *key, float value)
{
    char tmp[64];
    sprintf(tmp, "%.2f", value);
    SetString(section, key, tmp);
    return 0;
}

float CConfigFile::GetFloat(const char *section, const char *key, float def_value)
{
    const char *num = GetString(section, key, NULL);
    if (num != NULL) {
        return atof(num);
    }
    return def_value;
}

LINE_TYPE CConfigFile::getLineType(char *Str)
{
    LINE_TYPE type = LINE_TYPE_COMMENT;
    if (strchr(Str, '#') != NULL) {
        type = LINE_TYPE_COMMENT;
    } else if ( (strstr (Str, "[") != NULL) && (strstr (Str, "]") != NULL) ) { /* Is Section */
        type = LINE_TYPE_SECTION;
    } else {
        if (strstr (Str, "=") != NULL) {
            type = LINE_TYPE_KEY;
        } else {
            type = LINE_TYPE_COMMENT;
        }
    }
    return type;
}

void CConfigFile::FreeAllMem()
{
    //line
    LINE *pCurLine = NULL;
    LINE *pNextLine = NULL;
    for (pCurLine = mpFirstLine; pCurLine != NULL;) {
        pNextLine = pCurLine->pNext;
        delete pCurLine;
        pCurLine = pNextLine;
    }
    mpFirstLine = NULL;
    //section
    SECTION *pCurSec = NULL;
    SECTION *pNextSec = NULL;
    for (pCurSec = mpFirstSection; pCurSec != NULL;) {
        pNextSec = pCurSec->pNext;
        delete pCurSec;
        pCurSec = pNextSec;
    }
    mpFirstSection = NULL;
}

int CConfigFile::InsertSection(SECTION *pSec)
{
    //insert it to sections list ,as first section
    pSec->pNext = mpFirstSection;
    mpFirstSection = pSec;
    //insert it to lines list, at first
    pSec->pLine->pNext = mpFirstLine;
    mpFirstLine = pSec->pLine;
    return 0;
}

int CConfigFile::InsertKeyLine(SECTION *pSec, LINE *line)
{
    LINE *line1 = pSec->pLine;
    LINE *line2 = line1->pNext;
    line1->pNext = line;
    line->pNext = line2;
    return 0;
}

SECTION *CConfigFile::getSection(const char *section)
{
    //section
    for (SECTION *psec = mpFirstSection; psec != NULL; psec = psec->pNext) {
        if (strncmp((psec->pLine->Text) + 1, section, strlen(section)) == 0)
            return psec;
    }
    return NULL;
}

LINE *CConfigFile::getKeyLineAtSec(SECTION *pSec, const char *key)
{
    //line
    for (LINE *pline = pSec->pLine->pNext; (pline != NULL && pline->type != LINE_TYPE_SECTION); pline = pline->pNext) {
        if (pline->type == LINE_TYPE_KEY) {
            if (strncmp(pline->Text, key, strlen(key)) == 0) {
                return pline;
            }
        }
    }
    return NULL;
}

void CConfigFile::allTrim(char *Str)
{
    char *pStr;
    pStr = strchr (Str, '\n');
    if (pStr != NULL) {
        *pStr = 0;
    }
    int Len = strlen(Str);
    if ( Len > 0 ) {
        if ( Str[Len - 1] == '\r' ) {
            Str[Len - 1] = '\0';
        }
    }
    pStr = Str;
    while (*pStr != '\0') {
        if (*pStr == ' ') {
            char *pTmp = pStr;
            while (*pTmp != '\0') {
                *pTmp = *(pTmp + 1);
                pTmp++;
            }
        } else {
            pStr++;
        }
    }
    return;
}

void CConfigFile::GetPqdbPath(char *file_path)
{
    if (!isFileExist(PARAM_PQ_DB_PATH)) {
        //read pq db path from pq_default.ini
        const char *pqDBConfigPath = NULL;
        pqDBConfigPath = GetString(CFG_SECTION_PQ, CFG_PQ_DB_PATH, PQ_DB_DEFAULT_PATH_0);

#ifdef PRODUCT_SUPPORT_COMPRESS_DB
        char pqDbFilePath[128]  = {0};
        char pqBinFilePath[128] = {0};
        char tempPath[128] = {0};

        char *pqdBDirPath = NULL;
        Minizip *pMiniz   = NULL;

        if (isFileExist(pqDBConfigPath)) {
            strcpy(pqDbFilePath, pqDBConfigPath);
        } else if (isFileExist(PQ_DB_DEFAULT_PATH_0)) {
            strcpy(pqDbFilePath, PQ_DB_DEFAULT_PATH_0);
        } else if (isFileExist(PQ_DB_DEFAULT_PATH_1)) {
            strcpy(pqDbFilePath, PQ_DB_DEFAULT_PATH_1);
        } else {
            SYS_LOGE("no pq.db in %s and %s\n", PQ_DB_DEFAULT_PATH_0, PQ_DB_DEFAULT_PATH_1);
        }

        // get pq.bin file path
        pqdBDirPath = strtok(pqDbFilePath, ".");
        if (pqdBDirPath != NULL) {
            strcpy(tempPath, pqdBDirPath);
            strcat(tempPath, ".bin\0");
        }
        strcpy(pqBinFilePath, tempPath);
        SYS_LOGD("%s:db file path: %s, bin flie path: %s\n", __FUNCTION__, pqDbFilePath, pqBinFilePath);

        //Uncompress pq.bin
        if (!isFileExist(pqBinFilePath)) {
            SYS_LOGE("%s is not exit!\n", pqBinFilePath);
        } else {
            SYS_LOGD("run CheckAndUpdateUncompressFile\n");
            pMiniz = new Minizip();
            ret = pMiniz->CheckAndUpdateUncompressFile(PARAM_PQ_DB_PATH, pqBinFilePath);
            pMiniz->freeAll();
            delete pMiniz;
            pMiniz = NULL;
            if (ret != 0) {
                SYS_LOGE("Uncompress %s failed!!!!\n", pqBinFilePath);
            } else {
                SYS_LOGD("Uncompress %s success!!!!\n", pqBinFilePath);
            }
        }
#else
        if (isFileExist(pqDBConfigPath)) {
            CFile FilePq(pqDBConfigPath);
            if (FilePq.copyTo(PARAM_PQ_DB_PATH) != 0) {
                SYS_LOGE("copy file to %s error!\n", PARAM_PQ_DB_PATH);
            }
        } else if (isFileExist(PQ_DB_DEFAULT_PATH_0)) {
            CFile FilePq(PQ_DB_DEFAULT_PATH_0);
            if (FilePq.copyTo(PARAM_PQ_DB_PATH) != 0) {
                SYS_LOGE("copy file to %s error!\n", PARAM_PQ_DB_PATH);
            }
        } else if (isFileExist(PQ_DB_DEFAULT_PATH_1)) {
            CFile FilePq(PQ_DB_DEFAULT_PATH_1);
            if (FilePq.copyTo(PARAM_PQ_DB_PATH) != 0) {
                SYS_LOGE("copy file to %s error!\n", PARAM_PQ_DB_PATH);
            }
        } else {
            SYS_LOGE("no pq.db in %s and %s\n", PQ_DB_DEFAULT_PATH_0, PQ_DB_DEFAULT_PATH_1);
        }
#endif
    }

    strcpy(file_path, PARAM_PQ_DB_PATH);
}

void CConfigFile::GetOverscandbPath(char *file_path)
{
    if (!isFileExist(PARAM_OVERSCAN_DB_PATH)) {
        //read overscan db path from pq_default.ini
        const char *overscanDBConfigPath = NULL;
        overscanDBConfigPath = GetString(CFG_SECTION_PQ, CFG_PQ_OVERSCAN_DB_PATH, OVERSCAN_DB_DEFAULT_PATH_0);

        if (isFileExist(overscanDBConfigPath)) {
            CFile FileOverscan(overscanDBConfigPath);
            if (FileOverscan.copyTo(PARAM_OVERSCAN_DB_PATH) != 0 ) {
                SYS_LOGE("copy file to %s error!\n", PARAM_OVERSCAN_DB_PATH );
            }
        } else if (isFileExist(OVERSCAN_DB_DEFAULT_PATH_0)) {
            CFile FileOverscan(OVERSCAN_DB_DEFAULT_PATH_0);
            if (FileOverscan.copyTo(PARAM_OVERSCAN_DB_PATH) != 0 ) {
                SYS_LOGE("copy file to %s error!\n", PARAM_OVERSCAN_DB_PATH );
            }
        } else if (isFileExist(OVERSCAN_DB_DEFAULT_PATH_1)) {
            CFile FileOverscan(OVERSCAN_DB_DEFAULT_PATH_1);
            if (FileOverscan.copyTo(PARAM_OVERSCAN_DB_PATH) != 0 ) {
                SYS_LOGE("copy file to %s error!\n", PARAM_OVERSCAN_DB_PATH );
            }
        } else {
            SYS_LOGE("no overscan.db in %s and %s\n", OVERSCAN_DB_DEFAULT_PATH_0, OVERSCAN_DB_DEFAULT_PATH_1);
        }
    }

    strcpy(file_path, PARAM_OVERSCAN_DB_PATH);
}

void CConfigFile::GetSSMDataPath(char *file_path)
{
    //read ui setting path from pq_default.ini
    const char *pqSsmDataPath = NULL;
    pqSsmDataPath = GetString(CFG_SECTION_PQ, CFG_PQ_SSM_DATAT_PATH, PARAM_SSM_DATA_PATH);

    strcpy(file_path, pqSsmDataPath);
}

void CConfigFile::GetSSMDataHandlerPath(char *file_path)
{
    //read ui setting header path from pq_default.ini
    const char *pqSsmDataHandlerPath = NULL;
    pqSsmDataHandlerPath = GetString(CFG_SECTION_PQ, CFG_PQ_SSM_DATA_HANDLER_PATH, PARAM_SSM_HANDLER_PATH);

    strcpy(file_path, pqSsmDataHandlerPath);
}

void CConfigFile::GetWBFilePath(char *file_path)
{
    //read ui setting header path from pq_default.ini
    const char *pqWBPath = NULL;
    pqWBPath = GetString(CFG_SECTION_PQ, CFG_PQ_WB_PATH, WB_FILE_PATH);

    strcpy(file_path, pqWBPath);
}

void CConfigFile::GetDvFilePath(char *bin_file_path, char *cfg_file_path)
{
    //read dv file path from pq_default.ini
    const char *pqdvbinPath    = NULL;
    pqdvbinPath = GetString(CFG_SECTION_PQ, CFG_PQ_DV_BIN_PATH, DOLBY_BIN_FILE_DEFAULT_PATH_0);

    if (isFileExist(pqdvbinPath)) {
        strcpy(bin_file_path, pqdvbinPath);
    } else if (isFileExist(DOLBY_BIN_FILE_DEFAULT_PATH_0)) {
        strcpy(bin_file_path, DOLBY_BIN_FILE_DEFAULT_PATH_0);
    } else if (isFileExist(DOLBY_BIN_FILE_DEFAULT_PATH_1)) {
        strcpy(bin_file_path, DOLBY_BIN_FILE_DEFAULT_PATH_1);
    } else {
        SYS_LOGE("no dv_config.bin in %s and %s\n", DOLBY_BIN_FILE_DEFAULT_PATH_0, DOLBY_BIN_FILE_DEFAULT_PATH_1);
    }

    const char *pqdvcfgPath = NULL;
    pqdvcfgPath = GetString(CFG_SECTION_PQ, CFG_PQ_DV_CFG_PATH, DOLBY_CFG_FILE_DEFAULT_PATH_0);

    if (isFileExist(pqdvcfgPath)) {
        strcpy(cfg_file_path, pqdvcfgPath);
    } else if (isFileExist(DOLBY_CFG_FILE_DEFAULT_PATH_0)) {
        strcpy(cfg_file_path, DOLBY_CFG_FILE_DEFAULT_PATH_0);
    } else if (isFileExist(DOLBY_CFG_FILE_DEFAULT_PATH_1)) {
        strcpy(cfg_file_path, DOLBY_CFG_FILE_DEFAULT_PATH_1);
    } else {
        SYS_LOGE("no Amlogic_dv.cfg in %s and %s\n", DOLBY_CFG_FILE_DEFAULT_PATH_0, DOLBY_CFG_FILE_DEFAULT_PATH_1);
    }
}
