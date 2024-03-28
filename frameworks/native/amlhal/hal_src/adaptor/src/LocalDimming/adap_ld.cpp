#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>

#include "pq/adap_ld.h"
#include "ioctrl/ld_cmd_id.h"

/**
*** definition
**/
static int mLdFd = -1;
static int mLdFdIsOpened = 0;
pthread_mutex_t ld_mutex = PTHREAD_MUTEX_INITIALIZER;



/**
*** function
**/
ADAP_STATUS_T ADAP_LD_INIT(void)
{
    if (mLdFdIsOpened) {
        LOGD("%s LdFd has been opened.\n", __FUNCTION__);
        return ADAP_OK;
    }

    if (mLdFd < 0) {
        mLdFd = open("/dev/" LD_DEVICE_NAME, O_RDWR);
    }

    mLdFdIsOpened = 1;

    if (mLdFd < 0) {
        LOGE("%s failed err:%s\n", __FUNCTION__, strerror(errno));
        return ADAP_NOT_OK;
    }

    return ADAP_OK;
}

ADAP_STATUS_T ADAP_LD_UNINIT(void)
{
    if (mLdFd >= 0) {
        mLdFd = -1;
        mLdFdIsOpened = 0;
    }

    return ADAP_OK;
}

ADAP_STATUS_T ADAP_LD_DevIoCtl(int request, ...)
{
    /* note: todo
    ** due to there is no ld dev in driver now, and for tvfuse socts.
    ** so we temporary return OK for upper
    */
    if (request == AML_LDIM_IOC_NR_SET_FUNC_EN ||
        request == (int)AML_LDIM_IOC_NR_GET_FUNC_EN ||
        request == AML_LDIM_IOC_NR_SET_DEMOMODE ||
        request == (int)AML_LDIM_IOC_NR_GET_DEMOMODE) {
        return ADAP_OK;
    }

    int ret = -1;
    if (mLdFd < 0) {
        LOGE("%s mLdFd is not opened.\n", __FUNCTION__);
        return ADAP_NOT_OK;
    }

    pthread_mutex_lock(&ld_mutex);
    va_list ap;
    void *arg;
    va_start(ap, request);
    arg = va_arg ( ap, void * );
    va_end(ap);
    ret = ioctl(mLdFd, request, arg);
    LOGI("%s %s\n", __FUNCTION__, (ret < 0) ? "fail" : "success");
    pthread_mutex_unlock(&ld_mutex);

    return (ret < 0) ? ADAP_NOT_OK : ADAP_OK;
}

ADAP_STATUS_T ADAP_LD_GetPqInitStatus(void)
{
    ADAP_STATUS_T ret = ADAP_NOT_OK;

    ret = ADAP_LD_DevIoCtl(AML_LDIM_IOC_NR_GET_PQ_INIT);
    LOGI("%s %d\n", __FUNCTION__, ret);

    return ret;
}

ADAP_STATUS_T ADAP_LD_SetPqInit(void)
{
    ADAP_STATUS_T ret = ADAP_NOT_OK;

    ret = ADAP_LD_DevIoCtl(AML_LDIM_IOC_NR_SET_PQ_INIT);
    LOGI("%s %d\n", __FUNCTION__, ret);

    return ret;
}

ADAP_STATUS_T ADAP_LD_GetLevelIdx(int *pLevelIdx)
{
    if (mLdFd < 0) {
        AML_LOG_ERROR(LOG_LD, "%s OPEN /dev/%s fail\n", __FUNCTION__, LD_DEVICE_NAME);
        return ADAP_NOT_OK;
    }

    if (pLevelIdx == NULL) {
        AML_LOG_ERROR(LOG_LD, "%s pLdmInfo is INVALID PARAMS\n", __FUNCTION__);
        return ADAP_INVALID_PARAMS;
    }

    if (ioctl(mLdFd, AML_LDIM_IOC_NR_GET_LEVEL_IDX, &pLevelIdx) < 0) {
        AML_LOG_ERROR(LOG_LD, "%s VBE ioctl AML_LDIM_IOC_NR_GET_LEVEL_IDX fail\n", __FUNCTION__);
        return ADAP_NOT_OK;
    }

    return ADAP_OK;
}

ADAP_STATUS_T ADAP_LD_SetLevelIdx(int iLevelIdx)
{
    if (mLdFd < 0) {
        AML_LOG_ERROR(LOG_LD, "%s OPEN /dev/%s fail\n", __FUNCTION__, LD_DEVICE_NAME);
        return ADAP_NOT_OK;
    }

    if (ioctl(mLdFd, AML_LDIM_IOC_NR_SET_LEVEL_IDX, &iLevelIdx) < 0) {
        AML_LOG_ERROR(LOG_LD, "%s VBE ioctl AML_LDIM_IOC_NR_SET_LEVEL_IDX fail\n", __FUNCTION__);
        return ADAP_NOT_OK;
    }

    return ADAP_OK;
}

ADAP_STATUS_T ADAP_LD_GetFuncEn(int *pFuncEn)
{
    ADAP_STATUS_T ret = ADAP_NOT_OK;

    ret = ADAP_LD_DevIoCtl(AML_LDIM_IOC_NR_GET_FUNC_EN, pFuncEn);
    LOGI("%s %d\n", __FUNCTION__, ret);

    return ret;
}

ADAP_STATUS_T ADAP_LD_SetFuncEn(int iFuncEn)
{
    ADAP_STATUS_T ret = ADAP_NOT_OK;

    ret = ADAP_LD_DevIoCtl(AML_LDIM_IOC_NR_SET_FUNC_EN, &iFuncEn);
    LOGI("%s %d\n", __FUNCTION__, ret);

    return ret;
}

ADAP_STATUS_T ADAP_LD_GetRemapEn(int *pRemapEn)
{
    ADAP_STATUS_T ret = ADAP_NOT_OK;

    ret = ADAP_LD_DevIoCtl(AML_LDIM_IOC_NR_GET_REMAP_EN, pRemapEn);
    LOGI("%s %d\n", __FUNCTION__, ret);

    return ret;
}

ADAP_STATUS_T ADAP_LD_SetRemapEn(int iRemapEn)
{
    ADAP_STATUS_T ret = ADAP_NOT_OK;

    ret = ADAP_LD_DevIoCtl(AML_LDIM_IOC_NR_SET_REMAP_EN, &iRemapEn);
    LOGI("%s %d\n", __FUNCTION__, ret);

    return ret;
}

ADAP_STATUS_T ADAP_LD_GetBlMatrix(int *pBlMatrix)
{
    ADAP_STATUS_T ret = ADAP_NOT_OK;

    ret = ADAP_LD_DevIoCtl(AML_LDIM_IOC_NR_GET_BL_MATRIX, pBlMatrix);
    LOGI("%s %d %d\n", __FUNCTION__, ret, *pBlMatrix);

    return ret;
}

ADAP_STATUS_T ADAP_LD_SetBlMatrix(int iMatrix)
{
    ADAP_STATUS_T ret = ADAP_NOT_OK;

    ret = ADAP_LD_DevIoCtl(AML_LDIM_IOC_NR_SET_BL_MATRIX, &iMatrix);
    LOGI("%s %d\n", __FUNCTION__, ret);

    return ret;
}

ADAP_STATUS_T ADAP_LD_GetDemoMode(int *pDemoMode)
{
    ADAP_STATUS_T ret = ADAP_NOT_OK;

    ret = ADAP_LD_DevIoCtl(AML_LDIM_IOC_NR_GET_DEMOMODE, pDemoMode);
    LOGI("%s %d %d\n", __FUNCTION__, ret, *pDemoMode);

    return ret;
}

ADAP_STATUS_T ADAP_LD_SetDemoMode(int iDemoMode)
{
    ADAP_STATUS_T ret = ADAP_NOT_OK;

    ret = ADAP_LD_DevIoCtl(AML_LDIM_IOC_NR_SET_DEMOMODE, &iDemoMode);
    LOGI("%s %d\n", __FUNCTION__, ret);

    return ret;
}

ADAP_STATUS_T ADAP_LD_GetLdmInfo(aml_ldim_pq_t *pLdmInfo)
{
    ADAP_STATUS_T ret = ADAP_NOT_OK;

    ret = ADAP_LD_DevIoCtl(AML_LDIM_IOC_CMD_GET_INFO_NEW, pLdmInfo);
    LOGI("%s %d\n", __FUNCTION__, ret);

    return ret;
}

ADAP_STATUS_T ADAP_LD_SetLdmInfo(aml_ldim_pq_t *pLdmInfo)
{
    if (mLdFd < 0) {
        AML_LOG_ERROR(LOG_LD, "%s OPEN /dev/%s fail\n", __FUNCTION__, LD_DEVICE_NAME);
        return ADAP_NOT_OK;
    }

    if (pLdmInfo == NULL) {
        AML_LOG_ERROR(LOG_LD, "%s pLdmInfo is INVALID PARAMS\n", __FUNCTION__);
        return ADAP_INVALID_PARAMS;
    }

    if (ioctl(mLdFd, AML_LDIM_IOC_CMD_SET_INFO_NEW, pLdmInfo) < 0) {
        AML_LOG_ERROR(LOG_LD, "%s VBE ioctl AML_LDIM_IOC_CMD_SET_INFO_NEW fail\n", __FUNCTION__);
        return ADAP_NOT_OK;
    }

    return ADAP_OK;
}

ADAP_STATUS_T ADAP_LD_GetLdBLMappingPath(aml_path_t *Path)
{
    if (mLdFd < 0) {
        AML_LOG_ERROR(LOG_LD, "%s OPEN /dev/%s fail\n", __FUNCTION__, LD_DEVICE_NAME);
        return ADAP_NOT_OK;
    }

    if (Path == NULL) {
        AML_LOG_ERROR(LOG_LD, "%s BlMapping_Path is INVALID PARAMS\n", __FUNCTION__);
        return ADAP_INVALID_PARAMS;
    }

    if (ioctl(mLdFd, AML_LDIM_IOC_NR_GET_BL_MAPPING_PATH, Path) < 0) {
        AML_LOG_ERROR(LOG_LD, "%s VBE ioctl AML_LDIM_IOC_NR_GET_BL_MAPPING_PATH fail\n", __FUNCTION__);
        return ADAP_NOT_OK;
    }

    return ADAP_OK;
}

ADAP_STATUS_T ADAP_LD_SetLdBLMapping(am_pq_bin_param_t *pData)
{
    if (mLdFd < 0) {
        AML_LOG_ERROR(LOG_LD, "%s OPEN /dev/%s fail\n", __FUNCTION__, LD_DEVICE_NAME);
        return ADAP_NOT_OK;
    }

    if (pData == NULL) {
        AML_LOG_ERROR(LOG_LD, "%s pData is INVALID PARAMS\n", __FUNCTION__);
        return ADAP_INVALID_PARAMS;
    }

    if (ioctl(mLdFd, AML_LDIM_IOC_NR_SET_BL_MAPPING, pData) < 0) {
        AML_LOG_ERROR(LOG_LD, "%s VBE ioctl AML_LDIM_IOC_NR_GET_BL_MAPPING_PATH fail\n", __FUNCTION__);
        return ADAP_NOT_OK;
    }

    return ADAP_OK;
}

ADAP_STATUS_T ADAP_LD_GetLdBLProfilePath(aml_path_t *Path)
{
    if (mLdFd < 0) {
        AML_LOG_ERROR(LOG_LD, "%s OPEN /dev/%s fail\n", __FUNCTION__, LD_DEVICE_NAME);
        return ADAP_NOT_OK;
    }

    if (Path == NULL) {
        AML_LOG_ERROR(LOG_LD, "%s BlProfile_Path is INVALID PARAMS\n", __FUNCTION__);
        return ADAP_INVALID_PARAMS;
    }

    if (ioctl(mLdFd, AML_LDIM_IOC_NR_GET_BL_PROFILE_PATH, Path) < 0) {
        AML_LOG_ERROR(LOG_LD, "%s VBE ioctl AML_LDIM_IOC_NR_GET_BL_PROFILE_PATH fail\n", __FUNCTION__);
        return ADAP_NOT_OK;
    }

    return ADAP_OK;
}

ADAP_STATUS_T ADAP_LD_SetLdBLProfile(am_pq_bin_param_t *pData)
{
    if (mLdFd < 0) {
        AML_LOG_ERROR(LOG_LD, "%s OPEN /dev/%s fail\n", __FUNCTION__, LD_DEVICE_NAME);
        return ADAP_NOT_OK;
    }

    if (pData == NULL) {
        AML_LOG_ERROR(LOG_LD, "%s pData is INVALID PARAMS\n", __FUNCTION__);
        return ADAP_INVALID_PARAMS;
    }

    if (ioctl(mLdFd, AML_LDIM_IOC_NR_SET_BL_PROFILE, pData) < 0) {
        AML_LOG_ERROR(LOG_LD, "%s VBE ioctl AML_LDIM_IOC_NR_SET_BL_PROFILE fail\n", __FUNCTION__);
        return ADAP_NOT_OK;
    }

    return ADAP_OK;
}

ADAP_STATUS_T ADAP_LD_GetLdStructTable(am_pq_bin_param_t *pData)
{
    if (mLdFd < 0) {
        AML_LOG_ERROR(LOG_LD, "%s OPEN /dev/%s fail\n", __FUNCTION__, LD_DEVICE_NAME);
        return ADAP_NOT_OK;
    }

    if (pData == NULL) {
        AML_LOG_ERROR(LOG_LD, "%s pData is INVALID PARAMS\n", __FUNCTION__);
        return ADAP_INVALID_PARAMS;
    }

    if (ioctl(mLdFd, AML_LDIM_IOC_NR_GET_PQ_INIT, pData) < 0) {
        AML_LOG_ERROR(LOG_LD, "%s VBE ioctl AML_LDIM_IOC_NR_GET_PQ_INIT fail\n", __FUNCTION__);
        return ADAP_NOT_OK;
    }

    return ADAP_OK;
}

ADAP_STATUS_T ADAP_LD_SetLdStructTable(am_pq_bin_param_t *pData)
{
    if (mLdFd < 0) {
        AML_LOG_ERROR(LOG_LD, "%s OPEN /dev/%s fail\n", __FUNCTION__, LD_DEVICE_NAME);
        return ADAP_NOT_OK;
    }

    if (pData == NULL) {
        AML_LOG_ERROR(LOG_LD, "%s pData is INVALID PARAMS\n", __FUNCTION__);
        return ADAP_INVALID_PARAMS;
    }

    if (ioctl(mLdFd, AML_LDIM_IOC_NR_SET_PQ_INIT, pData) < 0) {
        AML_LOG_ERROR(LOG_LD, "%s VBE ioctl AML_LDIM_IOC_NR_SET_PQ_INIT fail\n", __FUNCTION__);
        return ADAP_NOT_OK;
    }

    return ADAP_OK;
}

