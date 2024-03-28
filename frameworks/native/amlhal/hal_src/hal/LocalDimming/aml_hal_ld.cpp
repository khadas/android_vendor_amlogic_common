#include <stdio.h>
#include <pq/aml_hal_ld.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "pq/aml_hal_ld.h"
#include "pq/adap_ld.h"
#include "PQTableLD.h"


#ifdef __cplusplus
extern "C"
{
#endif

HAL_STATUS_T AML_HAL_LD_INIT(void)
{
    AML_LOG_INFO(LOG_LD, "%s Start \n", __FUNCTION__);

    if (ADAP_LD_INIT() != ADAP_OK) {
        AML_LOG_ERROR(LOG_LD, "ADAP_LD_INIT Fail at Function %s\n", __FUNCTION__);
        return API_NOT_OK;
    } else {
        PQTableLD::GetInstance()->Init();
    }

    AML_LOG_INFO(LOG_LD, "%s Done\n", __FUNCTION__);
    return API_OK;
}

HAL_STATUS_T AML_HAL_LD_GetPqInitStatus(void)
{
    if (ADAP_LD_GetPqInitStatus() != ADAP_OK) {
        AML_LOG_ERROR(LOG_LD, "ADAP_LD_SetPqInit Fail at Function %s\n", __FUNCTION__);
        return API_NOT_OK;
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_LD_SetPqInit(void)
{
    if (ADAP_LD_SetPqInit() != ADAP_OK) {
        AML_LOG_ERROR(LOG_LD, "ADAP_LD_SetPqInit Fail at Function %s\n", __FUNCTION__);
        return API_NOT_OK;
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_LD_GetLevelIdx(int *pLevelIdx)
{
    if (pLevelIdx == NULL) {
        AML_LOG_ERROR(LOG_LD, "INVALID Value \n", __FUNCTION__);
        return API_INVALID_PARAMS;
    }

    if (ADAP_LD_GetLevelIdx(pLevelIdx) != ADAP_OK) {
        AML_LOG_ERROR(LOG_LD, "ADAP_LD_GetLevelIdx Fail at Function %s\n", __FUNCTION__);
        return API_NOT_OK;
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_LD_SetLevelIdx(int iLevelIdx)
{
    if (ADAP_LD_SetLevelIdx(iLevelIdx) != ADAP_OK) {
        AML_LOG_ERROR(LOG_LD, "ADAP_LD_SetLevelIdx iLevelIdx = %s Fail at Function %d\n", __FUNCTION__, iLevelIdx);
        return API_NOT_OK;
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_LD_GetFuncEn(int *pFuncEn)
{
    if (pFuncEn == NULL) {
        AML_LOG_ERROR(LOG_LD, "INVALID Value \n", __FUNCTION__);
        return API_INVALID_PARAMS;
    }

    if (ADAP_LD_GetFuncEn(pFuncEn) != ADAP_OK) {
        AML_LOG_ERROR(LOG_LD, "ADAP_LD_GetFuncEn Fail at Function %s\n", __FUNCTION__);
        return API_NOT_OK;
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_LD_SetFuncEn(SINT32 value)
{
    if (ADAP_LD_SetFuncEn(value) != ADAP_OK) {
        AML_LOG_ERROR(LOG_LD, "%s ADAP_LD_SetFuncEn value = %d \n", __FUNCTION__, value);
        return API_NOT_OK;
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_LD_GetRemapEn(int *pRemapEn)
{
    if (pRemapEn == NULL) {
        AML_LOG_ERROR(LOG_LD, "INVALID Value \n", __FUNCTION__);
        return API_INVALID_PARAMS;
    }

    if (ADAP_LD_GetRemapEn(pRemapEn) != ADAP_OK) {
        AML_LOG_ERROR(LOG_LD, "%s ADAP_LD_GetRemapEn Fail \n", __FUNCTION__);
        return API_NOT_OK;
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_LD_SeRemapEn(int iRemapEn)
{
    if (ADAP_LD_SetRemapEn(iRemapEn) != ADAP_OK) {
        AML_LOG_ERROR(LOG_LD, "%s ADAP_LD_SetRemapEn iRemapEn = %d \n", __FUNCTION__, iRemapEn);
        return API_NOT_OK;
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_LD_GetBlMatrix(int *pBlMatrix)
{
    if (pBlMatrix == NULL) {
        AML_LOG_ERROR(LOG_LD, "INVALID Value \n", __FUNCTION__);
        return API_INVALID_PARAMS;
    }

    if (ADAP_LD_GetBlMatrix(pBlMatrix) != ADAP_OK) {
        AML_LOG_ERROR(LOG_LD, "%s ADAP_LD_GetBlMatrix Fail \n", __FUNCTION__);
        return API_NOT_OK;
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_LD_SeBlMatrix(int iMatrix)
{
    if (ADAP_LD_SetBlMatrix(iMatrix) != ADAP_OK) {
        AML_LOG_ERROR(LOG_LD, "%s ADAP_LD_SetBlMatrix iMatrix = %d \n", __FUNCTION__, iMatrix);
        return API_NOT_OK;
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_LD_GetDemoMode(aml_hal_ldim_demo_info_t *pDemoInfo)
{
    return API_NOT_SUPPORTED;
}

HAL_STATUS_T AML_HAL_LD_SetDemoMode(aml_hal_ldim_demo_info_t *pDemoInfo)
{
    if (pDemoInfo == NULL) {
        AML_LOG_ERROR(LOG_LD, "INVALID Value \n", __FUNCTION__);
        return API_INVALID_PARAMS;
    }

    if (ADAP_LD_SetDemoMode((int)pDemoInfo->bOnOff) != ADAP_OK) {
        AML_LOG_ERROR(LOG_LD, "%s ADAP_LD_SetBlMatrix pDemoInfo->bOnOff = %d \n", __FUNCTION__, pDemoInfo->bOnOff);
        return API_NOT_OK;
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_LD_GetLdmInfo(aml_hal_ld_info_t *pLdmInfo)
{
    if (pLdmInfo == NULL) {
        AML_LOG_ERROR(LOG_LD, "INVALID Value \n", __FUNCTION__);
        return API_INVALID_PARAMS;
    }

    if (ADAP_LD_GetLdmInfo((aml_ldim_pq_t *)pLdmInfo) != ADAP_OK) {
        AML_LOG_ERROR(LOG_LD, "%s ADAP_LD_GetLdmInfo Fail \n", __FUNCTION__);
        return API_NOT_OK;
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_LD_SetLocalDimming(aml_hal_ld_info_t *pLdmInfo)
{
    if (pLdmInfo == NULL) {
        AML_LOG_ERROR(LOG_LD, "INVALID Value \n", __FUNCTION__);
        return API_INVALID_PARAMS;
    }

    if (ADAP_LD_SetLdmInfo((aml_ldim_pq_t *)pLdmInfo) != ADAP_OK) {
        AML_LOG_ERROR(LOG_LD, "%s ADAP_LD_SetLdmInfo Fail \n", __FUNCTION__);
        return API_NOT_OK;
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_LD_SetInit(aml_hal_led_panel_info_t *pLedPanelInfo)
{
    return API_OK;
}

HAL_STATUS_T AML_HAL_LD_GetAplInfo(aml_hal_led_apl_info_t *pAplInfo)
{
    return API_OK;
}

HAL_STATUS_T AML_HAL_LD_SetDbIdx(int idx)
{
    return API_OK;
}

HAL_STATUS_T AML_HAL_LD_GetDbIdx(int *pIdx)
{
   return API_OK;
}

HAL_STATUS_T AML_HAL_LD_SetControlSpi(aml_hal_led_spi_ctrl_info_t *pLedControlSpi)
{
    return API_OK;
}

HAL_STATUS_T AML_HAL_LD_GetControlSpi(aml_hal_led_spi_ctrl_info_t *pLedControlSpi)
{
    return API_OK;
}

#ifdef __cplusplus
}
#endif
