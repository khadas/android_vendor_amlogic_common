/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */
#define LOG_TAG                 "tvserver"
#define LOG_TV_TAG              "CTvRrt"
#pragma clang diagnostic ignored "-Wundefined-bool-conversion"
#define RRT_DEBUG_EN            0

#include <tinyxml2.h>
#include "CTvRrt.h"
using namespace tinyxml2;

pthread_mutex_t rrt_search_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t rrt_update_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * @Function: GetElementPointerByName
 * @Description: search data from RRT file for save RRT data
 * @Param: pRootElement: Root element of RRT xml file;
           ElementName:  name of TiXmlElement need been search
 * @Return: the TiXmlElement which has been search
 */
XMLElement *GetElementPointerByName(XMLElement* pRootElement, const char *ElementName)
{
    if (strcmp(ElementName, pRootElement->Value()) == 0) {
        return pRootElement;
    }

    XMLElement* pElement = NULL;
    XMLElement* pRetElement = NULL;
    for (pElement=pRootElement->FirstChildElement();pElement;pElement = pElement->NextSiblingElement()) {
         pRetElement = GetElementPointerByName(pElement, ElementName);
    }

    if (pRetElement != NULL) {
        if (RRT_DEBUG_EN) LOGD("GetNodePointerByName: %s", pRetElement->Value());
        return pRetElement;
    } else {
        return NULL;
    }
}

/**
 * @Function: OpenXmlFile
 * @Description: Open XML file
 * @Param:
 * @Return: The pRRTFile which has been opened
 */
XMLDocument *OpenXmlFile(void)
{
    // define TiXmlDocument
    XMLDocument *pRRTFile = new XMLDocument();
    pRRTFile->LoadFile(TV_RRT_DEFINE_PARAM_PATH);
    if (NULL == pRRTFile) {
        LOGD("%s: create RRTFile error!\n", __FUNCTION__);
        return NULL;
    }

    //add Declaration
    if (RRT_DEBUG_EN) LOGD("%s: start create Declaration!\n", __FUNCTION__);
    XMLDeclaration *pNewDeclaration = pRRTFile->NewDeclaration();
    if (NULL == pNewDeclaration) {
        LOGD("%s: create Declaration error!\n", __FUNCTION__);
        return NULL;
    }
    pRRTFile->LinkEndChild(pNewDeclaration);

    //add root element
    LOGD("%s: start create RootElement!\n", __FUNCTION__);
    XMLElement *pRootElement = pRRTFile->NewElement("rating-system-definitions");
    if (NULL == pRootElement) {
        LOGD("%s: create RootElement error!\n", __FUNCTION__);
        return NULL;
    }
    pRRTFile->LinkEndChild(pRootElement);
    pRootElement->SetAttribute("xmlns:android", "http://schemas.android.com/apk/res/android");
    pRootElement->SetAttribute("android:versionCode", "2");

    return pRRTFile;
}

/**
 * @Function: SaveDataToXml
 * @Description: Save data to XML file
 * @Param:pRRTFile:The pRRTFile which has been opened
          rrt_info:Charge for GetRRTRating
 * @Return: true:save success;    false:save failed
 */
bool SaveDataToXml(XMLDocument *pRRTFile, rrt_info_t &rrt_info)
{
    if (pRRTFile == NULL) {
        LOGE("%s: xml file don't open!\n", __FUNCTION__);
        return false;
    }
    pthread_mutex_lock(&rrt_update_mutex);

    XMLElement *pRootElement = pRRTFile->RootElement();
    if (pRootElement->FirstChildElement() == NULL) {
        XMLElement *pRatingSystemElement = pRRTFile->NewElement("rating-system-definition");
        if (NULL == pRatingSystemElement) {
            LOGD("%s: create pRatingSystemElement error!\n", __FUNCTION__);
            pthread_mutex_unlock(&rrt_update_mutex);
            return false;
        }
        pRootElement->LinkEndChild(pRatingSystemElement);
        pRatingSystemElement->SetAttribute("android:name", rrt_info.dimensions_name);
        pRatingSystemElement->SetAttribute("android:rating", rrt_info.rating_region);
        pRatingSystemElement->SetAttribute("android:country",rrt_info.rating_region_name);
        pRatingSystemElement->SetAttribute("android:dimension_id",rrt_info.dimensions_id);

        XMLElement *pNewElement = pRRTFile->NewElement("rating-definition");
        if (NULL == pNewElement) {
            pthread_mutex_unlock(&rrt_update_mutex);
            return false;
        }
        pRatingSystemElement->LinkEndChild(pNewElement);
        pNewElement->SetAttribute("android:title",rrt_info.abbrev_rating_value_text);
        pNewElement->SetAttribute("android:description",rrt_info.rating_value_text);
        pNewElement->SetAttribute("android:rating_id",rrt_info.rating_value_id);

    } else {
        XMLElement *pTmpElement = GetElementPointerByName(pRootElement, "rating-system-definition");
        if ((strcmp(pTmpElement->Attribute("android:name"), rrt_info.dimensions_name) == 0) &&
            (strcmp(pTmpElement->Attribute("android:country"), rrt_info.rating_region_name) == 0) &&
            (pTmpElement->IntAttribute("android:dimension_id") == rrt_info.dimensions_id)) {
            LOGD("%s: add new rating-definition to rating-system-definition!\n", __FUNCTION__);
            XMLElement *pNewElement = pRRTFile->NewElement("rating-definition");
            if (NULL == pNewElement) {
                pthread_mutex_unlock(&rrt_update_mutex);
                return false;
            }
            pTmpElement->LinkEndChild(pNewElement);
            pNewElement->SetAttribute("android:title",rrt_info.abbrev_rating_value_text);
            pNewElement->SetAttribute("android:description",rrt_info.rating_value_text);
            pNewElement->SetAttribute("android:rating_id",rrt_info.rating_value_id);
        } else {
            LOGD("%s: create new rating-system-definition!\n", __FUNCTION__);
            XMLElement *pRatingSystemElement = pRRTFile->NewElement("rating-system-definition");
            if (NULL == pRatingSystemElement) {
                LOGD("%s: create pRatingSystemElement error!\n", __FUNCTION__);
                pthread_mutex_unlock(&rrt_update_mutex);
                return false;
            }
            pRootElement->LinkEndChild(pRatingSystemElement);
            pRatingSystemElement->SetAttribute("android:name", rrt_info.dimensions_name);
            pRatingSystemElement->SetAttribute("android:rating", rrt_info.rating_region);
            pRatingSystemElement->SetAttribute("android:country",rrt_info.rating_region_name);
            pRatingSystemElement->SetAttribute("android:dimension_id",rrt_info.dimensions_id);

            XMLElement *pNewElement = pRRTFile->NewElement("rating-definition");
            if (NULL == pNewElement) {
                pthread_mutex_unlock(&rrt_update_mutex);
                return false;
            }
            pRatingSystemElement->LinkEndChild(pNewElement);
            pNewElement->SetAttribute("android:title",rrt_info.abbrev_rating_value_text);
            pNewElement->SetAttribute("android:description",rrt_info.rating_value_text);
            pNewElement->SetAttribute("android:rating_id",rrt_info.rating_value_id);
        }
    }

    if (!pRRTFile->SaveFile(TV_RRT_DEFINE_PARAM_PATH)) {
        LOGD("%s: save error!\n", __FUNCTION__);
        pthread_mutex_unlock(&rrt_update_mutex);
        return false;
    }

    pthread_mutex_unlock(&rrt_update_mutex);
    return true;
}

CTvRrt *CTvRrt::mInstance;
CTvRrt *CTvRrt::getInstance()
{
    LOGD("%s: start rrt action!\n", __FUNCTION__);
    if (NULL == mInstance) {
        mInstance = new CTvRrt();
    }

    return mInstance;
}

CTvRrt::CTvRrt()
{
    mRrtScanStatus      = INVALID_ID;
    mDmx_id             = INVALID_ID;
    mLastRatingRegion   = INVALID_ID;
    mLastDimensionsDefined = INVALID_ID;
    mLastVersion        = INVALID_ID;
    mScanResult         = 0;
    mpObserver          = NULL;
    int kerverVersion = getKernelMajorVersion();
    char defaultRRTXMLPath[128] = {0};
    /*if (kerverVersion > 4) {//kernel 5.4
        strcpy(defaultRRTXMLPath, TV_RRT_DEFINE_SYSTEM_PATH_1);
    } else {//kernel 4.9
        strcpy(defaultRRTXMLPath, TV_RRT_DEFINE_SYSTEM_PATH_0);
    }*/
    //strcpy(defaultRRTXMLPath, TV_RRT_DEFINE_SYSTEM_PATH_1);
    int nLen =  strlen(TV_RRT_DEFINE_SYSTEM_PATH_1) < (sizeof(defaultRRTXMLPath) - 1) ? strlen(TV_RRT_DEFINE_SYSTEM_PATH_1) :(sizeof(defaultRRTXMLPath)-1);
    strncpy(defaultRRTXMLPath, TV_RRT_DEFINE_SYSTEM_PATH_1, nLen);
    defaultRRTXMLPath[nLen] = '\0';

    if (isFileExist(defaultRRTXMLPath)) {
        if (!isFileExist(TV_RRT_DEFINE_PARAM_PATH)) {
            CFile file ( defaultRRTXMLPath );

            if ( file.copyTo ( TV_RRT_DEFINE_PARAM_PATH ) != 0 ) {
                LOGE ( "%s, copy file = %s , error", __FUNCTION__, TV_RRT_DEFINE_PARAM_PATH );
            }

            char sysCmd[1024];
            sprintf(sysCmd, "chmod 644 %s", TV_RRT_DEFINE_PARAM_PATH);
            if (system(sysCmd)) {
               LOGE("exec cmd:%s fail\n", sysCmd);
            }
        }
    } else {
        LOGE("%s don't exist.\n", defaultRRTXMLPath);
    }
}

CTvRrt::~CTvRrt()
{
    if (mInstance != NULL) {
        delete mInstance;
        mInstance = NULL;
    }
}

/**
 * @Function: StartRrtUpdate
 * @Description: Start Update rrt info
 * @Param:mode:RRT_AUTO_SEARCH:auto search;    RRT_MANU_SEARCH:manual search
 * @Return: 0 success, -1 fail
 */
int CTvRrt::StartRrtUpdate(rrt_search_mode_t mode)
{
    int ret;
    pthread_mutex_lock(&rrt_search_mutex);

    ret = RrtCreate(0, 2, 0, NULL);    //2 is demux id which according to DVB moudle!
    if (ret < 0) {
        LOGD("%s: RrtCreate failed!\n", __FUNCTION__);
        pthread_mutex_unlock(&rrt_search_mutex);
        return 0;
    }

    ret = RrtScanStart();
    if (ret < 0) {
        LOGD("%s: RrtScanStart failed!\n", __FUNCTION__);
        pthread_mutex_unlock(&rrt_search_mutex);
        return 0;
    } else {
        if (mode == RRT_MANU_SEARCH) {//manual
            mRrtScanStatus = RrtEvent::EVENT_RRT_SCAN_SCANING;
            sleep(5);//scan 5s
            mRrtScanStatus = RrtEvent::EVENT_RRT_SCAN_END;
            LOGD("%s: ScanResult = %d!\n", __FUNCTION__, mScanResult);
            pthread_mutex_unlock(&rrt_search_mutex);
            return mScanResult;
        } else {//auto
            pthread_mutex_unlock(&rrt_search_mutex);
            return 1;
        }
    }
}

/**
 * @Function: StopRrtUpdate
 * @Description: Stop Update rrt info
 * @Param:
 * @Return: 0 success, -1 fail
 */
int CTvRrt::StopRrtUpdate(void)
{
    int ret = -1;

    ret = RrtScanStop();
    if (ret < 0) {
        LOGD("%s: RrtScanStop failed!\n", __FUNCTION__);
    }

    ret = RrtDestroy();
    if (ret < 0) {
        LOGE("%s: RrtDestroy failed!\n", __FUNCTION__);
    }

    return ret;
}

/**
 * @Function: GetRRTRating
 * @Description: search data for livetv from RRT file
 * @Param: rating_region_id: rating region id;
           dimension_id:  dimension id;
           value_id:     value id;
           ret:search results
 * @Return: 0 success, -1 fail
 */
int CTvRrt::GetRRTRating(int rating_region_id, int dimension_id, int value_id, int program_id, rrt_select_info_t *ret)
{
    int r = -1;

    LOGD("program_id=%d, rating_region_id = %d, dimension_id = %d, value_id = %d\n", program_id, rating_region_id, dimension_id, value_id);

    //check rrt_define_file exist
    struct stat tmp_st;
    if (stat(TV_RRT_DEFINE_PARAM_PATH, &tmp_st) != 0) {
        LOGD("program_id=%d, file don't exist!\n", program_id);
        ret->status = -1;
        return -1;
    }

    XMLDocument *pRRTFile = new XMLDocument();
    if (!pRRTFile->LoadFile(TV_RRT_DEFINE_PARAM_PATH)) {
        LOGD("program_id=%d,load %s error!\n", program_id, TV_RRT_DEFINE_PARAM_PATH);
        ret->status = -1;
        return -1;
    }

    memset(ret, 0, sizeof(rrt_select_info_t));
    XMLElement* pTmpElement = pRRTFile->RootElement()->FirstChildElement();
    if (pTmpElement != NULL) {
        do {
            if ((pTmpElement->FirstAttribute()->Next()->IntValue() ==rating_region_id) &&
                (pTmpElement->IntAttribute("android:dimension_id") == dimension_id )) {
                LOGD("program_id=%d,%s\n",program_id, pTmpElement->FirstAttribute()->Next()->Next()->Value());
                int RationSize = strlen(pTmpElement->FirstAttribute()->Next()->Next()->Value());
                ret->rating_region_name_count = RationSize;
                const char *rating_region_name = pTmpElement->FirstAttribute()->Next()->Next()->Value();
                memcpy(ret->rating_region_name, rating_region_name, RationSize+1);
                LOGD("program_id=%d,%s\n",program_id, pTmpElement->FirstAttribute()->Value());
                int DimensionSize = strlen(pTmpElement->FirstAttribute()->Value());
                ret->dimensions_name_count = DimensionSize;
                memcpy(ret->dimensions_name, pTmpElement->FirstAttribute()->Value(), DimensionSize+1);

                XMLElement* pElement = NULL;
                for (pElement=pTmpElement->FirstChildElement();pElement;pElement = pElement->NextSiblingElement()) {
                    if (pElement->IntAttribute("android:rating_id") == value_id ) {
                        int ValueSize = strlen(pElement->FirstAttribute()->Value());
                        ret->rating_value_text_count = ValueSize;
                        LOGD("program_id=%d,%s\n",program_id, pElement->FirstAttribute()->Value());
                        memcpy(ret->rating_value_text, pElement->FirstAttribute()->Value(), ValueSize+1);
                        r = 0;
                        goto end;
                    }
                }
            }
            pTmpElement = pTmpElement->NextSiblingElement();
        } while(pTmpElement);
        LOGD("program_id=%d,Don't find value!\n", program_id);
        ret->status = -1;
    } else {
        LOGD("program_id=%d,XML file is NULL!\n", program_id);
        ret->status = -1;
    }

end:
    if (pRRTFile)
        delete pRRTFile;

    return r;
}

/**
 * @Function: RrtCreate
 * @Description: open dev for RRT and set RRT event
 * @Param: fend_id: fend dev id;
           dmx_id:  demux dev id;
           src:     source;
           textLangs:language;
 * @Return: 0 success, -1 fail
 */
int CTvRrt::RrtCreate(int fend_id, int dmx_id, int src, char * textLangs)
{
#ifdef SUPPORT_ADTV
    AM_EPG_CreatePara_t para;
    AM_ErrorCode_t  ret;
    AM_DMX_OpenPara_t dmx_para;
    mDmx_id = dmx_id;

    memset(&dmx_para, 0, sizeof(dmx_para));
    LOGD("Opening demux%d ...", dmx_id);
    ret = AM_DMX_Open(mDmx_id, &dmx_para);
    if (ret != DVB_SUCCESS) {
        LOGD("AM_DMX_Open failed");
        return - 1;
    }

    para.fend_dev       = fend_id;
    para.dmx_dev        = dmx_id;
    para.source         = src;
    para.hdb            = NULL;
    snprintf(para.text_langs, sizeof(para.text_langs), "%s", textLangs);

    ret = AM_EPG_Create(&para, &mRrtScanHandle);

    if (ret != DVB_SUCCESS) {
        LOGD("AM_EPG_Create failed");
        return - 1;
    }

    /*disable internal default table procedure*/
    ret = AM_EPG_DisableDefProc(mRrtScanHandle, true);
    if (ret != DVB_SUCCESS) {
        LOGD("AM_EPG_DisableDefProc failed");
        return - 1;
    }

    /*handle tables directly by user*/
    ret = AM_EPG_SetTablesCallback(mRrtScanHandle, AM_EPG_TAB_RRT, RrtTableCallback, NULL);
    if (ret != DVB_SUCCESS) {
        LOGD("AM_EPG_SetTablesCallback failed");
        return - 1;
    }
#endif
    return 0;
}

/**
 * @Function: RrtDestroy
 * @Description: close dev for RRT and reset RRT event
 * @Param:
 * @Return: 0 success, -1 fail
 */
int CTvRrt::RrtDestroy()
{
#ifdef SUPPORT_ADTV
    AM_ErrorCode_t  ret;

    if (mRrtScanHandle != NULL) {
        ret = AM_EPG_Destroy(mRrtScanHandle);
        if (ret != DVB_SUCCESS) {
            LOGD("AM_EPG_Destroy failed");
            return - 1;
        }
        mRrtScanHandle = NULL;
    }

    if (mDmx_id != INVALID_ID) {
        ret = AM_DMX_Close(mDmx_id);

        if (ret != DVB_SUCCESS) {
            LOGD("AM_DMX_Close failed");
            return - 1;
        }
        mDmx_id = INVALID_ID;
    }
#endif
    return 0;
}

/**
 * @Function: RrtChangeMode
 * @Description: change epg mode
 * @Param: op: epg modul;
           mode:  epg mode;
 * @Return: 0 success, -1 fail
 */
int CTvRrt::RrtChangeMode(int op, int mode)
{
#ifdef SUPPORT_ADTV
    AM_ErrorCode_t  ret;

    ret = AM_EPG_ChangeMode(mRrtScanHandle, op, mode);
    if (ret != DVB_SUCCESS) {
        LOGD("AM_EPG_ChangeMode failed");
        return - 1;
    }
#endif
    return 0;
}

/**
 * @Function: RrtScanStart
 * @Description: start scan RRT info
 * @Param:
 * @Return: 0 success, -1 fail
 */
int CTvRrt::RrtScanStart(void)
{
    int ret = -1;
    ret = RrtChangeMode(MODE_ADD, SCAN_RRT);

    return ret;
}

/**
 * @Function: RrtScanStop
 * @Description: stop scan RRT info
 * @Param:
 * @Return: 0 success, -1 fail
 */
int CTvRrt::RrtScanStop(void)
{
    int ret = -1;
    ret = RrtChangeMode(MODE_REMOVE, SCAN_RRT);
    return ret;
}

/**
 * @Function: RrtTableCallback
 * @Description: RRT event callback function
 * @Param:void *: dev handle
          event_type:RRT event type
          param:callback data
          user_data:
 * @Return:
 */
void CTvRrt::RrtTableCallback(void * handle, int event_type, void *param, void *user_data)
{
#ifdef SUPPORT_ADTV
    if (mInstance == NULL) {
        LOGD("rrt mInstance is NULL!\n");
        return;
    }

    if (mInstance->mpObserver == NULL) {
        LOGD("rrt mpObserver is NULL!\n");
        return;
    }

    if (!param) {
        LOGD("rrt data is NULL!\n");
        if (mInstance->mRrtScanStatus == RrtEvent::EVENT_RRT_SCAN_SCANING) {
            mInstance->mScanResult = 0;
        }

        return;
    }

    switch (event_type) {
    case AM_EPG_TAB_RRT: {
        if (mInstance->mRrtScanStatus == RrtEvent::EVENT_RRT_SCAN_SCANING) {
            mInstance->mScanResult = 1;
        }

        mInstance->mCurRrtEv.satus = CTvRrt::RrtEvent::EVENT_RRT_SCAN_START;
        mInstance->mpObserver->onEvent(mInstance->mCurRrtEv);

        mInstance->RrtDataUpdate(handle, event_type, param, user_data);

        mInstance->mCurRrtEv.satus = CTvRrt::RrtEvent::EVENT_RRT_SCAN_END;
        mInstance->mpObserver->onEvent(mInstance->mCurRrtEv);

        break;
    }
    default:
        break;
    }
#endif
}

/**
 * @Function: RrtDataUpdate
 * @Description: RRT data parser
 * @Param:dev_no: dev id
          event_type:RRT event type
          param:callback data
          user_data:
 * @Return:
 */
void CTvRrt::RrtDataUpdate(void * dev_no __unused, int event_type, void *param, void *user_data __unused)
{
#ifdef SUPPORT_ADTV
    switch (event_type) {
    case AM_EPG_TAB_RRT: {
        INT8U j;
        rrt_info_t rrt_info;
        memset(&rrt_info, 0, sizeof(rrt_info_t));

        rrt_section_info_t * pNewRrt = (rrt_section_info_t *)param;

        //open xml file
        XMLDocument *pRRTFile = OpenXmlFile();
        if (pRRTFile == NULL) {
            LOGD("open xml file failed!\n");
            return;
        }

        while (pNewRrt != NULL) {
            LOGD("T [RRT:0x%02x][rr:0x%04x][dd:0x%04x] v[0x%x]\n", pNewRrt->i_table_id, pNewRrt->rating_region,
                    pNewRrt->dimensions_defined, pNewRrt->version_number);

            //save rating_region
            rrt_info.rating_region = pNewRrt->rating_region;
            rrt_info.dimensions_id = 0;

            //parser rating_region_name
            MultipleStringParser(pNewRrt->rating_region_name, rrt_info.rating_region_name);

            //parser dimensions_info
            rrt_dimensions_info  *dimensions_info = pNewRrt->dimensions_info;

            while (dimensions_info != NULL) {
                //parser dimensions_name
                MultipleStringParser(dimensions_info->dimensions_name, rrt_info.dimensions_name);
                LOGD("graduated_scale[%d] values_defined[%d]\n", pNewRrt->dimensions_info->graduated_scale,pNewRrt->dimensions_info->values_defined);

                //paser and save data to xml
                for (j=1;j<dimensions_info->values_defined;j++) {
                    //save rating_id
                    rrt_info.rating_value_id = j;
                    MultipleStringParser(dimensions_info->rating_value[j].abbrev_rating_value_text, rrt_info.abbrev_rating_value_text);
                    MultipleStringParser(dimensions_info->rating_value[j].rating_value_text, rrt_info.rating_value_text);

                    bool ret = SaveDataToXml(pRRTFile, rrt_info);
                    if (!ret) {
                        LOGD("Save XML element error!\n");
                    }
                }
                //save dimensions_id
                rrt_info.dimensions_id ++ ;

                dimensions_info = dimensions_info->p_next;
            }
            pNewRrt = pNewRrt->p_next;
        }

        delete pRRTFile;
        break;
    }
    default:
        break;
    }
#endif
}

#ifdef SUPPORT_ADTV
/**
 * @Function: MultipleStringParser
 * @Description: Multiple string data parser
 * @Param:atsc_multiple_string: Multiple string data
          ret: data after parser
 * @Return:
 */
void CTvRrt::MultipleStringParser(atsc_multiple_string_t &atsc_multiple_string, char *ret)
{
    int i;
    for (i=0;i<atsc_multiple_string.i_string_count;i++) {
        int size = strlen((char *)atsc_multiple_string.string[0].string);
        if (ret != NULL) {
            memcpy(ret, atsc_multiple_string.string[0].string, size+1);
        }
    }

    return;
}
#endif

/**
 * @Function: RrtUpdataCheck
 * @Description: Check RRT xml file need update or not
 * @Param:atsc_multiple_string: Multiple string data
          ret: data after parser
 * @Return:
 */

bool CTvRrt::RrtUpdataCheck(int rating_region, int dimensions_defined, int version_number)
{
    if ((mLastRatingRegion == rating_region)
        && (mLastDimensionsDefined == dimensions_defined)
        && (mLastVersion == version_number)){
        return true;
    } else {
        mLastRatingRegion = rating_region;
        mLastDimensionsDefined = dimensions_defined;
        mLastVersion =version_number;
        return false;
    }
}
