/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef AMCAS_IPTV_H
#define AMCAS_IPTV_H

#include "amCasBase.h"

typedef struct __tagCAS_STREAM_INFO {
    unsigned int        ca_system_id;
    uint16_t            desc_num;
    unsigned int        ecm_pid[4];
    uint16_t            audio_pid;
    uint16_t            video_pid;
    int                 audio_channel;
    int                 video_channel;
    bool                av_diff_ecm;
    uint8_t             *private_data;
    unsigned int        pri_data_len;
    void                *headers;
} CasStreamInfo;

class  AmCasIPTV:public AmCasBase {

public:
    AmCasIPTV();
    ~AmCasIPTV();
    AmCasCode_t setPrivateData(void *iDate, int iSize);
    AmCasCode_t provision();
    AmCasCode_t openSession(uint8_t *sessionId);
    AmCasCode_t closeSession(uint8_t *sessionId);
    AmCasCode_t setPids(int vPid, int aPid);
    AmCasCode_t processEcm(int isSection, int iPid, uint8_t *pBuffer, int iBufferLength);//use for aml mp multi-channel cas
    AmCasCode_t processEcm(int isSection, int isVideoEcm, int vEcmPid, int aEcmPid, unsigned char *pBuffer, int iBufferLength);
    AmCasCode_t processEmm(int isSection, int iPid, uint8_t *pBuffer, int iBufferLength);
    AmCasCode_t setCasInstanceId(int casInstanceId);
    int getCasInstanceId();
    int             mCasObjIdx;//use for aml mp multi-channel cas

private:
    int             mCasInstanceId;
    unsigned int    mCaSystemId;
    unsigned int    *mEcmPid;
    unsigned int    *mEmmPid;
    char            *mLaUrl; //cas server url
    char            *mPort;//cas server port
    char            *mLicStore;//cas license store
    int             mvPid;
    int             maPid;
    CasStreamInfo   *mCasStreamInfo;
    void            *mAmcasHandle;
};

#endif
