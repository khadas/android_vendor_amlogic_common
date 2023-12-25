#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "pq/aml_hal_pq.h"

#include "pq/adap_pq.h"
#include "pq/adap_memc.h"
#include "pq/adap_ld.h"
#include "pq/adap_di.h"

#include "PQTableOSD.h"
#include "PQTableLD.h"
#include "PQTable.h"

#ifdef __cplusplus
extern "C"
{
#endif

HAL_STATUS_T AML_HAL_PQ_INIT(void)
{
    if (ADAP_PQ_INIT() != ADAP_OK) {
        LOGE("%s VPP INIT fail  \n", __FUNCTION__);
    } else {
        PQTable::GetInstance()->Init();
        PQTableOSD::GetInstance()->Init();
    }

    if (ADAP_DI_INIT() != ADAP_OK) {
        LOGE("%s DI INIT fail  \n", __FUNCTION__);
    }

    if (ADAP_MEMC_INIT() != ADAP_OK) {
        LOGE("%s MEMC INIT fail  \n", __FUNCTION__);
    }

    if (ADAP_LD_INIT() != ADAP_OK) {
        LOGE("%s LD INIT fail  \n", __FUNCTION__);
    } else {
        PQTableLD::GetInstance()->Init();
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_UNINIT(void)
{
    ADAP_PQ_UNINIT();
    ADAP_DI_Uninit();
    ADAP_MEMC_UNINIT();
    ADAP_LD_UNINIT();

    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_SetBrightness(SINT32 value)
{
    if (PQTableOSD::GetInstance()->SetBrightness(value) != true) {
        LOGE("%s Fail  Contrast = %d\n", __FUNCTION__, value);
        return API_NOT_OK;
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_GetBrightness(SINT32 *pValue)
{
    if (pValue == NULL) {
        LOGE("%s pValue is NULL, return  \n", __FUNCTION__);
        return API_INVALID_PARAMS;
    }

    if (PQTableOSD::GetInstance()->GetBrightness(pValue) != true) {
        return API_NOT_OK;
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_SetContrast(SINT32 value)
{
    if (PQTableOSD::GetInstance()->SetContrast(value) != true) {
        LOGE("%s Fail  Contrast = %d\n", __FUNCTION__, value);
        return API_NOT_OK;
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_GetContrast(SINT32 *pValue)
{
    if (pValue == NULL) {
        LOGE("%s pValue is NULL, return  \n", __FUNCTION__);
        return API_INVALID_PARAMS;
    }

    if (PQTableOSD::GetInstance()->GetContrast(pValue) != true) {
        return API_NOT_OK;
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_SetSaturation(SINT32 value)
{
    if (PQTableOSD::GetInstance()->SetSaturation(value) != true) {
        LOGE("%s Fail  Saturation = %d\n", __FUNCTION__, value);
        return API_NOT_OK;
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_GetSaturation(SINT32 *pValue)
{
    if (pValue == NULL) {
        LOGE("%s pValue is NULL, return  \n", __FUNCTION__);
        return API_INVALID_PARAMS;
    }

    if (PQTableOSD::GetInstance()->GetSaturation(pValue) != true) {
        return API_NOT_OK;
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_SetHue(SINT32 value)
{
    if (PQTableOSD::GetInstance()->SetHue(value) != true) {
        LOGE("%s Fail  Hue = %d\n", __FUNCTION__, value);
        return API_NOT_OK;
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_GetHue(SINT32 *pValue)
{
    if (pValue == NULL) {
        LOGE("%s pValue is NULL, return  \n", __FUNCTION__);
        return API_INVALID_PARAMS;
    }

    if (PQTableOSD::GetInstance()->GetHue(pValue) != true) {
        return API_NOT_OK;
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_SetSharpness(SINT32 value)
{
    if (PQTableOSD::GetInstance()->SetSharpnessLevel(value) != true) {
        LOGE("%s fail  Sharpness = %d\n", __FUNCTION__, value);
        return API_NOT_OK;
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_GetSharpness(SINT32 *pValue)
{
    if (pValue == NULL) {
        LOGE("%s pValue is NULL, return  \n", __FUNCTION__);
        return API_INVALID_PARAMS;
    }

    if (PQTableOSD::GetInstance()->GetSharpnessLevel(pValue) != true) {
        return API_NOT_OK;
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_SetBacklight(SINT32 value)
{
    if (PQTableOSD::GetInstance()->SetBackLight(value) != true) {
        LOGE("%s fail  backlight = %d\n", __FUNCTION__, value);
        return API_NOT_OK;
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_GetBacklight(SINT32 *pValue)
{
    if (pValue == NULL) {
        LOGD("%s fail INVALID_PARAMS \n", __FUNCTION__);
        return API_INVALID_PARAMS;
    }

    if (PQTableOSD::GetInstance()->GetBackLight(pValue) != true) {
        return API_NOT_OK;
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_SetColorTempData(aml_hal_white_balance_t *pamlHalWb)
{
    if (pamlHalWb == NULL) {
        LOGE("%s fail INVALID_PARAMS \n", __FUNCTION__);
        return API_INVALID_PARAMS;
    }

    if (PQTableOSD::GetInstance()->SetColorTempData((COLORTEMP_DATA *)pamlHalWb) != true) {
        LOGD("%s fail\n", __FUNCTION__);
        return API_NOT_OK;
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_GetColorTempData(aml_hal_white_balance_t *pamlHalWb)
{
    if (pamlHalWb == NULL) {
        LOGE("%s fail INVALID_PARAMS \n", __FUNCTION__);
        return API_INVALID_PARAMS;
    }

    if (PQTableOSD::GetInstance()->GetColorTempData((COLORTEMP_DATA *)pamlHalWb) != true) {
        LOGD("%s fail\n", __FUNCTION__);
        return API_NOT_OK;
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_SetColorTemp_RGain(UINT32 r_gain)
{
    if (PQTableOSD::GetInstance()->SetColorTempData_RGain(r_gain) != true) {
        LOGE("%s fail  r_gain = %d\n", __FUNCTION__, r_gain);
        return API_NOT_OK;
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_SetColorTemp_GGain(UINT32 g_gain)
{
    if (PQTableOSD::GetInstance()->SetColorTempData_GGain(g_gain) != true) {
        LOGE("%s fail  g_gain = %d\n", __FUNCTION__, g_gain);
        return API_NOT_OK;
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_SetColorTemp_BGain(UINT32 b_gain)
{
    if (PQTableOSD::GetInstance()->SetColorTempData_BGain(b_gain) != true) {
        LOGE("%s fail  b_gain = %d\n", __FUNCTION__, b_gain);
        return API_NOT_OK;
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_SetColorTemp_ROffSet(SINT32 r_offset)
{
    if (PQTableOSD::GetInstance()->SetColorTempData_ROffset(r_offset) != true) {
        LOGE("%s fail  r_offset = %d\n", __FUNCTION__, r_offset);
        return API_NOT_OK;
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_SetColorTemp_GOffSet(SINT32 g_offset)
{
    if (PQTableOSD::GetInstance()->SetColorTempData_GOffset(g_offset) != true) {
        LOGE("%s fail  g_offset = %d\n", __FUNCTION__, g_offset);
        return API_NOT_OK;
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_SetColorTemp_BOffSet(SINT32 b_offset)
{
    if (PQTableOSD::GetInstance()->SetColorTempData_BOffset(b_offset) != true) {
        LOGE("%s fail  b_offset = %d\n", __FUNCTION__, b_offset);
        return API_NOT_OK;
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_SetPreGamma(aml_hal_pre_gamma_table_t *pamlHalPreGamma)
{
    if (pamlHalPreGamma == NULL) {
        return API_INVALID_PARAMS;
    }

    if (ADAP_PQ_SetPreGamma((vpp_pre_gamma_table_s *)pamlHalPreGamma) != ADAP_OK) {
        return API_NOT_OK;
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_GetPreGamma(void)
{
    return API_NOT_SUPPORTED;
}

HAL_STATUS_T AML_HAL_PQ_SetGammaCurve(aml_hal_gamma_curve_e eGammaCurve)
{
    if (eGammaCurve > AML_HAL_GAMMA_CURVE_MAX) {
        LOGE("%s input parameter out of range\n", __FUNCTION__);
        return API_NOT_OK;
    }

    if (PQTable::GetInstance()->Set_VPQ_GammaTable((int) eGammaCurve) != true) {
        return API_NOT_OK;
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_GetGammaCurve(void)
{
    LOGD("%s\n", __FUNCTION__);
    return API_NOT_SUPPORTED;
}

HAL_STATUS_T AML_HAL_PQ_SetModuleCtrl(aml_hal_module_ctrl_t *pModuleCtrl)
{
    if (pModuleCtrl == NULL) {
        return API_INVALID_PARAMS;
    }

    if (ADAP_PQ_SetModuleCtrl((vpp_module_ctrl_s*)pModuleCtrl) != ADAP_OK) {
        return API_NOT_OK;
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_GetModuleCtrl(aml_hal_module_ctrl_t *pModuleCtrl)
{
    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_SetMatrixParam(aml_hal_mtrx_info_t *pMatrixInfo)
{
    CHECK_EXPRESSION_RET((pMatrixInfo == NULL));

    ADAP_STATUS_T ret = ADAP_NOT_OK;


    struct vpp_mtrx_info_s st_mtrx_info_adap;
    memcpy(&st_mtrx_info_adap, pMatrixInfo, sizeof(aml_hal_mtrx_info_t));

    ret = ADAP_PQ_SetMatrixParam(&st_mtrx_info_adap);
    LOGD("%s %s\n", __FUNCTION__, (ret == ADAP_OK) ? "success" : "faile");

    return (ret == ADAP_OK) ? API_OK : API_NOT_OK;
}

HAL_STATUS_T AML_HAL_PQ_SetPqState(aml_hal_pq_state_t *pPqState)
{
    CHECK_EXPRESSION_RET((pPqState == NULL));

    ADAP_STATUS_T ret = ADAP_NOT_OK;

    struct vpp_pq_state_s st_pq_state_adap;
    memcpy(&st_pq_state_adap, pPqState, sizeof(aml_hal_pq_state_t));

    ret = ADAP_PQ_SetPqState(&st_pq_state_adap);
    LOGD("%s %s\n", __FUNCTION__, (ret == ADAP_OK) ? "success" : "faile");

    return (ret == ADAP_OK) ? API_OK : API_NOT_OK;
}

HAL_STATUS_T AML_HAL_PQ_GetPqState(aml_hal_pq_state_t *pPqState)
{
    CHECK_EXPRESSION_RET((pPqState == NULL));

    ADAP_STATUS_T ret = ADAP_NOT_OK;
    LOGD("%s\n", __FUNCTION__);

    struct vpp_pq_state_s st_pq_state_adap;
    memset(&st_pq_state_adap, 0, sizeof(struct vpp_pq_state_s));

    ret = ADAP_PQ_GetPqState(&st_pq_state_adap);
    LOGD("%s %s\n", __FUNCTION__, (ret == ADAP_OK) ? "success" : "faile");

    memcpy(pPqState, &st_pq_state_adap, sizeof(struct vpp_pq_state_s));

    return (ret == ADAP_OK) ? API_OK : API_NOT_OK;
}

HAL_STATUS_T AML_HAL_PQ_SetPcMode(aml_hal_pc_mode_e ePcMode)
{
    ADAP_STATUS_T ret = ADAP_NOT_OK;
    LOGD("%s %d\n", __FUNCTION__, ePcMode);

    if (ePcMode > AML_HAL_PC_MODE_ON) {
        LOGE("%s input parameter out of range\n", __FUNCTION__);
        return API_NOT_OK;
    }

    enum vpp_pc_mode_e e_pc_mode_adap = (enum vpp_pc_mode_e)ePcMode;
    ret = ADAP_PQ_SetPcMode(e_pc_mode_adap);
    LOGD("%s %s\n", __FUNCTION__, (ret == ADAP_OK) ? "success" : "faile");

    return (ret ==ADAP_OK) ? API_OK : API_NOT_OK;
}

HAL_STATUS_T AML_HAL_PQ_GetPcMode(aml_hal_pc_mode_e *pPcMode)
{
    ADAP_STATUS_T ret = ADAP_NOT_OK;
    LOGD("%s\n", __FUNCTION__);

    enum vpp_pc_mode_e e_pc_mode_adap = EN_PC_MODE_OFF;

    ret = ADAP_PQ_GetPcMode(&e_pc_mode_adap);
    LOGD("%s %s %d\n", __FUNCTION__, (ret == ADAP_OK) ? "success" : "faile", e_pc_mode_adap);

    *pPcMode = (aml_hal_pc_mode_e)e_pc_mode_adap;

    return (ret ==ADAP_OK) ? API_OK : API_NOT_OK;
}

HAL_STATUS_T AML_HAL_PQ_SetDc(aml_hal_pq_dc_mode_e eDcMode)
{
    ADAP_STATUS_T ret = ADAP_NOT_OK;
    LOGD("%s %d\n", __FUNCTION__, eDcMode);

    if (eDcMode > AML_HAL_DC_HIGH) {
        LOGE("%s input parameter out of range\n", __FUNCTION__);
        return API_NOT_OK;
    }

    adap_pq_dnlp_mode_e e_dc_mode_adap = (adap_pq_dnlp_mode_e)eDcMode;

    ret = ADAP_PQ_SetDnlpMode(e_dc_mode_adap);
    LOGD("%s %s\n", __FUNCTION__, (ret == ADAP_OK) ? "success" : "faile");

    return (ret ==ADAP_OK) ? API_OK : API_NOT_OK;
}

HAL_STATUS_T AML_HAL_PQ_GetDc(SINT32 *pValue)
{
    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_SetLcMode(aml_hal_pq_lc_mode_e eLcMode)
{
    ADAP_STATUS_T ret = ADAP_NOT_OK;
    LOGD("%s %d\n", __FUNCTION__, eLcMode);

    if (eLcMode >= AML_HAL_LC_MAX) {
        LOGE("%s input parameter out of range\n", __FUNCTION__);
        return API_NOT_OK;
    }

    adap_pq_lc_mode_e lc_mode_adap = (adap_pq_lc_mode_e)eLcMode;

    ret = ADAP_PQ_SetLcMode(lc_mode_adap);
    LOGD("%s %s\n", __FUNCTION__, (ret == ADAP_OK) ? "success" : "faile");

    return (ret ==ADAP_OK) ? API_OK : API_NOT_OK;
}

HAL_STATUS_T AML_HAL_PQ_SetCscType(aml_hal_csc_type_e eCscType)
{
    ADAP_STATUS_T ret = ADAP_NOT_OK;
    LOGD("%s %d\n", __FUNCTION__, eCscType);

    /*
    if (eCscType >= AML_HAL_CSC_MATRIX_DEFAULT_CSCTYPE) {
        LOGE("%s input parameter out of range\n", __FUNCTION__);
        return API_NOT_OK;
    }
    */

    enum vpp_csc_type_e csc_type_adap = (enum vpp_csc_type_e)eCscType;

    ret = ADAP_PQ_SetCscType(csc_type_adap);
    LOGD("%s %s\n", __FUNCTION__, (ret == ADAP_OK) ? "success" : "faile");

    return (ret ==ADAP_OK) ? API_OK : API_NOT_OK;
}

HAL_STATUS_T AML_HAL_PQ_GetCscType(aml_hal_csc_type_e *pCscType)
{
    ADAP_STATUS_T ret = ADAP_NOT_OK;
    LOGD("%s\n", __FUNCTION__);

    enum vpp_csc_type_e csc_type_adap = EN_CSC_MATRIX_NULL;

    ret = ADAP_PQ_GetCscType(&csc_type_adap);
    LOGD("%s %s %d\n", __FUNCTION__, (ret == ADAP_OK) ? "success" : "faile", csc_type_adap);

    *pCscType = (aml_hal_csc_type_e)csc_type_adap;

    return (ret ==ADAP_OK) ? API_OK : API_NOT_OK;
}

HAL_STATUS_T AML_HAL_PQ_Set3DLutData(SINT32 value)
{
    ADAP_STATUS_T ret = ADAP_NOT_OK;
    LOGD("%s %d\n", __FUNCTION__, value);

    ret = ADAP_PQ_Set3DLutData(value);
    LOGD("%s %s\n", __FUNCTION__, (ret == ADAP_OK) ? "success" : "faile");

    return (ret ==ADAP_OK) ? API_OK : API_NOT_OK;
}

HAL_STATUS_T AML_HAL_PQ_Get3DLutData(void)
{
    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_GetHdrType(aml_hal_hdr_type_e *pHdrType)
{
    ADAP_STATUS_T ret = ADAP_NOT_OK;
    LOGD("%s\n", __FUNCTION__);

    enum vpp_hdr_type_e hdr_type_adap = EN_TYPE_NONE;

    ret = ADAP_PQ_GetHdrType(&hdr_type_adap);
    LOGD("%s %s %d\n", __FUNCTION__, (ret == ADAP_OK) ? "success" : "faile", hdr_type_adap);

    *pHdrType = (aml_hal_hdr_type_e)hdr_type_adap;

    return (ret ==ADAP_OK) ? API_OK : API_NOT_OK;
}

HAL_STATUS_T AML_HAL_PQ_GetColorPrim(aml_hal_color_primary_e *pColorPrim)
{
    ADAP_STATUS_T ret = ADAP_NOT_OK;
    LOGD("%s\n", __FUNCTION__);

    enum vpp_color_primary_e color_primary_adap = EN_COLOR_PRI_NULL;

    ret = ADAP_PQ_GetColorPrim(&color_primary_adap);
    LOGD("%s %s %d\n", __FUNCTION__, (ret == ADAP_OK) ? "success" : "faile", color_primary_adap);

    *pColorPrim = (aml_hal_color_primary_e)color_primary_adap;

    return (ret ==ADAP_OK) ? API_OK : API_NOT_OK;
}

HAL_STATUS_T AML_HAL_PQ_GetHdrMetadata(aml_hal_hdr_metadata_t *pHdrMetadata)
{
    CHECK_EXPRESSION_RET((pHdrMetadata == NULL));

    ADAP_STATUS_T ret = ADAP_NOT_OK;
    LOGD("%s\n", __FUNCTION__);


    struct vpp_hdr_metadata_s hdr_metadata_adap;
    memset(&hdr_metadata_adap, 0, sizeof(struct vpp_hdr_metadata_s));

    ret = ADAP_PQ_GetHdrMetadata(&hdr_metadata_adap);
    LOGD("%s %s\n", __FUNCTION__, (ret == ADAP_OK) ? "success" : "faile");

    memcpy(pHdrMetadata, &hdr_metadata_adap, sizeof(struct vpp_hdr_metadata_s));
    for (int i = 0; i < 3; i++) {
        for (int j=0; j < 2; j++) {
            LOGD("%s primaries[%d][%d]=%d\n", __FUNCTION__, i, j, pHdrMetadata->primaries[i][j]);
        }
    }
    for (int i = 0; i < 2; i++) {
        LOGD("%s white_point[%d]=%d\n", __FUNCTION__, i, pHdrMetadata->white_point[i]);
        LOGD("%s luminance[%d]=%d\n", __FUNCTION__, i, pHdrMetadata->luminance[i]);
    }

    return (ret ==ADAP_OK) ? API_OK : API_NOT_OK;
}

HAL_STATUS_T AML_HAL_PQ_GetHistAvg(aml_hal_histgm_ave_t *pHistAve)
{
    CHECK_EXPRESSION_RET((pHistAve == NULL));

    ADAP_STATUS_T ret = ADAP_NOT_OK;
    LOGD("%s\n", __FUNCTION__);

    struct vpp_histgm_ave_s his_ave_adap;
    memset(&his_ave_adap, 0, sizeof(struct vpp_histgm_ave_s));

    ret = ADAP_PQ_GetHistAvg(&his_ave_adap);
    //LOGD("%s %s\n", __FUNCTION__, (ret == ADAP_OK) ? "success" : "faile");

    memcpy(pHistAve, &his_ave_adap, sizeof(struct vpp_histgm_ave_s));
    //LOGD("%s %d %d %d %d\n", __FUNCTION__, pHistAve->sum, pHistAve->width, pHistAve->height, pHistAve->ave);

    return (ret ==ADAP_OK) ? API_OK : API_NOT_OK;
}

HAL_STATUS_T AML_HAL_PQ_GetHistParam(aml_hal_histgm_param_t *pHistParam)
{
    if (pHistParam == NULL) {
        LOGD("%spHistParam is NULL\n", __FUNCTION__);
        return API_INVALID_PARAMS;
    }

    ADAP_STATUS_T ret = ADAP_NOT_OK;

    struct vpp_histgm_param_s his_param_adap;
    memset(&his_param_adap, 0, sizeof(struct vpp_histgm_param_s));

    ret = ADAP_PQ_GetHistParam(&his_param_adap);

    memcpy(pHistParam, &his_param_adap, sizeof(struct vpp_histgm_param_s));

    return (ret ==ADAP_OK) ? API_OK : API_NOT_OK;
}

HAL_STATUS_T AML_HAL_PQ_SetDcColorGain(SINT32 value)
{
    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_GetDcColorGain(SINT32 *pValue)
{
    return API_OK;
}
HAL_STATUS_T AML_HAL_PQ_SetInnerPattern(SINT32 value)
{
    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_SetRealCinema(SINT32 value)
{
    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_GetRealCinema(SINT32 *pValue)
{
    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_SetCmDbData(aml_hal_cmd_info_t *pCmInfo)
{
    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_GetCmDbData(aml_hal_cmd_info_t *pCmInfo)
{
    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_SetBlackLevel(aml_hal_black_level_info_t *pBlackInfo)
{
    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_GetBlackLevel(aml_hal_black_level_info_t *pBlackInfo)
{
    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_SetGammaData(aml_hal_gamma_lut_t *pGammaLut)
{
    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_GetGammaData(aml_hal_gamma_lut_t *pGammaLut)
{
    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_SetGamutMatrixPre(aml_hal_gamut_pre_t *pGamutPre)
{    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_GetGamutMatrixPre(aml_hal_gamut_pre_t *pGamutPre)
{
    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_SetGamutMatrixPost(aml_hal_gamut_post_t *pGamutPost)
{
    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_GetGamutMatrixPost(aml_hal_gamut_post_t *pGamutPost)
{
    return API_OK;
}
HAL_STATUS_T AML_HAL_PQ_SetPqModeInfo(aml_hal_pqmode_info_t *pPqModeInfo)
{
    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_GetPqModeInfo(aml_hal_pqmode_info_t *pPqModeInfo)
{
    return API_OK;
}
HAL_STATUS_T AML_HAL_PQ_SetExtraPattern(aml_hal_ptn_info_v2_t *pPtnInfo)
{
    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_SetNr(SINT32 value)
{
    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_GetNr(SINT32 *pValue)
{
    return API_OK;
}
HAL_STATUS_T AML_HAL_PQ_SetMpegNr(SINT32 value)
{
    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_GetMpegNr(SINT32 *pValue)
{    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_SetDecontour(SINT32 value)
{
    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_GetDecontour(SINT32 *pValue)
{
    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_SetPictureModeFlag(SINT32 Mode)
{
    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_GetPictureModeFlag(int *Mode)
{
    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_SetBrightness_OSD(SINT32 value)
{
    if (PQTableOSD::GetInstance()->SetBrightness_OSD(value) != true) {
        LOGE("%s Fail  Contrast = %d\n", __FUNCTION__, value);
        return API_NOT_OK;
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_GetBrightness_OSD(SINT32 *pValue)
{
    if (pValue == NULL) {
        LOGE("%s pValue is NULL, return  \n", __FUNCTION__);
        return API_INVALID_PARAMS;
    }

    if (PQTableOSD::GetInstance()->GetBrightness(pValue) != true) {
        return API_NOT_OK;
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_SetContrast_OSD(SINT32 value)
{
    if (PQTableOSD::GetInstance()->SetContrast_OSD(value) != true) {
        LOGE("%s Fail OSD Contrast = %d\n", __FUNCTION__, value);
        return API_NOT_OK;
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_GetContrast_OSD(SINT32 *pValue)
{
    if (pValue == NULL) {
        LOGE("%s pValue is NULL, return  \n", __FUNCTION__);
        return API_INVALID_PARAMS;
    }

    if (PQTableOSD::GetInstance()->GetContrast(pValue) != true) {
        return API_NOT_OK;
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_SetSaturation_OSD(SINT32 value)
{
    if (PQTableOSD::GetInstance()->SetSaturation_OSD(value) != true) {
        LOGE("%s Fail OSD Saturation = %d\n", __FUNCTION__, value);
        return API_NOT_OK;
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_GetSaturation_OSD(SINT32 *pValue)
{
    if (pValue == NULL) {
        LOGE("%s pValue is NULL, return  \n", __FUNCTION__);
        return API_INVALID_PARAMS;
    }

    if (PQTableOSD::GetInstance()->GetSaturation(pValue) != true) {
        return API_NOT_OK;
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_SetHue_OSD(SINT32 value)
{
    if (PQTableOSD::GetInstance()->SetHue_OSD(value) != true) {
        LOGE("%s Fail OSD Saturation = %d\n", __FUNCTION__, value);
        return API_NOT_OK;
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_GetHue_OSD(SINT32 *pValue)
{
    if (pValue == NULL) {
        LOGE("%s pValue is NULL, return  \n", __FUNCTION__);
        return API_INVALID_PARAMS;
    }

    if (PQTableOSD::GetInstance()->GetHue(pValue) != true) {
        return API_NOT_OK;
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_SetStructNonlinearModeType(Hal_NonlinearModeType *pData)
{
    if (pData == NULL) {
        return API_INVALID_PARAMS;
    }

    if (PQTableOSD::GetInstance()->SetNonlinearModeType((NonlinearModeType *)pData) != true) {
        LOGE("%s FAIL \n", __FUNCTION__);
        return API_NOT_OK;
    }

    return API_OK;
}

HAL_STATUS_T AML_HAL_PQ_SetPQSrcTiming(aml_hal_pq_source_timing_e eSrcTiming)
{
    return API_OK;
}

//LD
HAL_STATUS_T AML_HAL_PQ_LD_SetLevelIdx(int iLevelIdx)
{
    if (ADAP_LD_SetLevelIdx(iLevelIdx) != true) {
        return API_NOT_OK;
    }

    return API_OK;
}

//for pqserver get table data
void* AML_HAL_PQ_GetPQOsdVerData(void)
{
    UINT32 tableLen = 0;

    return PQTableOSD::GetInstance()->GetTableData(PQ_OSD_TABLE_VERSION, &tableLen);
}

void* AML_HAL_PQ_GetPQOsdNonlinearData(UINT32 *TableNum)
{
    return PQTableOSD::GetInstance()->GetTableData(PQ_OSD_TABLE_NONLINEARMAPPING, TableNum);
}

void* AML_HAL_PQ_GetPQOsdPictureData(UINT32 *TableNum)
{
    return PQTableOSD::GetInstance()->GetTableData(PQ_OSD_TABLE_PICTUREMODE, TableNum);
}

void* AML_HAL_PQ_GetPQOsdColorTempData(UINT32 *TableNum)
{
    return PQTableOSD::GetInstance()->GetTableData(PQ_OSD_TABLE_COLORTEMP, TableNum);
}

#ifdef __cplusplus
}
#endif
