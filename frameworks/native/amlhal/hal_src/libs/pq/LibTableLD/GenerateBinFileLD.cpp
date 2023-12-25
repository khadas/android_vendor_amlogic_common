#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "PQTableLDLoader.h"

typedef struct _OUTPUT_BIN {
    const char *PQFileName;
    const char *PQBinName;
} OUTPUT_BIN;

OUTPUT_BIN BIN[] = {
    {"AML_LD_Table_Public.cpp", "ldim.bin"},
};

int main(int argc, char** argv)
{
    char sPanelFilePath[128], sTmp[128];
    *sPanelFilePath = '\0';
    *sTmp = '\0';

    FILE *pCurPanel = fopen("./curFile", "r");
    if (pCurPanel) {
        if (fscanf(pCurPanel, "%s", sTmp) < 0) {
            return 0;
        }
        fclose(pCurPanel);
    }

    char *pTok = NULL;
    pTok = strtok(sTmp, "=");
    if (pTok != NULL) {
        pTok = strtok(NULL, "=");
        if (pTok != NULL) {
            if (*pTok == '.')
                pTok ++;
            if (*pTok == '/')
                pTok ++;

            unsigned int iCnt = 0;
            for (iCnt = 0; iCnt < sizeof(BIN) / sizeof(OUTPUT_BIN); iCnt ++) {
                if (!strcmp(BIN[iCnt].PQFileName, pTok)) {
                    sprintf(sPanelFilePath, "OutPut/%s", BIN[iCnt].PQBinName);
                    PQTableLDGenerate(sPanelFilePath);
                    return 1;
                }
            }
        }
    }

    return 0;
}
