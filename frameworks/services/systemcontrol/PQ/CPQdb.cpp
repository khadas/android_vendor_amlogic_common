/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "SystemControl"
#define LOG_TV_TAG "CPQdb"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <utils/String8.h>

#include "CPQdb.h"
#include "CPQLog.h"

#define ID_FIELD            "TableID"
#define CM_LEVEL_NAME       "CMLevel"
#define LEVEL_NAME          "Level"
#define CVBS_NAME_ID        "TVOUT_CVBS"
#define TABLE_NAME_ID       "TableName"

CPQdb::CPQdb()
{
    bri_nodes = 0;
    con_nodes = 0;
    hue_nodes = 0;
    sat_nodes = 0;
    sha0_nodes = 0;
    sha1_nodes = 0;
    sha2_nodes = 0;
    memset(pq_bri_data, 0, sizeof(pq_bri_data));
    memset(pq_con_data, 0, sizeof(pq_con_data));
    memset(pq_sat_data, 0, sizeof(pq_sat_data));
    memset(pq_hue_data, 0, sizeof(pq_hue_data));
    memset(pq_sharpness0_reg_data, 0, sizeof(pq_sharpness0_reg_data));
    memset(pq_sharpness1_reg_data, 0, sizeof(pq_sharpness1_reg_data));
    memset(pq_sharpnesspi_reg_data, 0, sizeof(pq_sharpnesspi_reg_data));
}

CPQdb::~CPQdb()
{
}

int CPQdb::openPqDB(const char *db_path)
{
    SYS_LOGD("openPqDB path = %s", db_path);
    int rval;

    if (access(db_path, 0) < 0) {
        SYS_LOGE("PQ_DB don't exist!\n");
        return -1;
    }

    closeDb();
    rval = openDb(db_path);

    String8 attributeVal;
    if (rval == 0) {
        database_attribute_t databaseAttribute;
        bool ret = PQ_GetDataBaseAttribute(&databaseAttribute);
        if (ret) {
            PQ_GetPqDbMatchType(&databaseAttribute);

            attributeVal = databaseAttribute.ToolVersion + " " +
                           databaseAttribute.ProjectVersion + " " +
                           databaseAttribute.dbversion + " " +
                           databaseAttribute.GenerateTime + " " +
                           databaseAttribute.ChipVersion;
        } else {
            attributeVal = "Get PQ_DB Version failure!!!";
        }
        SYS_LOGD("%s = %s\n", "PQ.db.version", attributeVal.c_str());
    }

    return rval;
}

int CPQdb::closePqDB(void)
{
    return closeDb();
}

int CPQdb::reopenDB(const char *db_path)
{
    int  rval = openDb(db_path);
    return rval;
}

int CPQdb::getRegValues(const char *table_name, am_regs_t *regs)
{
    CSqlite::Cursor c_reg_list;
    int rval = -1;
    int index_am_reg = 0;
    char sqlmaster[256] = {0};
    if (table_name == NULL || !strlen(table_name)) {
        SYS_LOGE("%s, table_name is null\n", __FUNCTION__);
        return rval;
    }

    if (mDbMatchType == MATCH_TYPE_MBOX_S5) {
        char table_name_copy[40] = {0};
        char *table_name_split = NULL;
        const char *delim = "_";
        strcpy(table_name_copy, table_name);
        SYS_LOGD("%s, table_name_copy=%s\n", __FUNCTION__, table_name_copy);
        table_name_split = strtok(table_name_copy, delim);
        if (table_name_split == NULL) {
            SYS_LOGE("%s, table_name_split is null\n", __FUNCTION__);
            return rval;
        }
        SYS_LOGD("%s, table_name_split=%s\n", __FUNCTION__, table_name_split);

        if (strcmp(table_name_split, "Sharpness") == 0) { //Sharpness_xx.xml
            SYS_LOGD("%s, node_number=%d\n", __FUNCTION__, node_number);
            getSqlParams(__FUNCTION__, sqlmaster, "select RegType, RegAddr, RegMask, RegValue from %s where NodeNumber = %d", table_name, node_number);
        } else {
            getSqlParams(__FUNCTION__, sqlmaster, "select RegType, RegAddr, RegMask, RegValue from %s;", table_name);
        }
    } else {
        getSqlParams(__FUNCTION__, sqlmaster, "select RegType, RegAddr, RegMask, RegValue from %s;", table_name);
    }

    this->select(sqlmaster, c_reg_list);
    int count = c_reg_list.getCount();
    if (count < 0 ) {
        SYS_LOGE("%s, Select value error!\n", __FUNCTION__);
        regs->length = 0;
        return rval;
    } else if (count > REGS_MAX_NUMBER) {
        SYS_LOGE("%s, regs is too more, in pq.db count = %d", __FUNCTION__, count);
        regs->length = 0;
        return rval;
    }

    if (c_reg_list.moveToFirst()) {
        int index_type = 0;
        int index_addr = 1;
        int index_mask = 2;
        int index_val = 3;
        do {
            regs->am_reg[index_am_reg].type = c_reg_list.getUInt(index_type);
            regs->am_reg[index_am_reg].addr = c_reg_list.getUInt(index_addr);
            regs->am_reg[index_am_reg].mask = c_reg_list.getUInt(index_mask);
            regs->am_reg[index_am_reg].val = c_reg_list.getUInt(index_val);
            index_am_reg++;
        } while (c_reg_list.moveToNext());
        regs->length = index_am_reg;
        rval = 0;
    } else {
        SYS_LOGE ("Don't have table in %s !\n", table_name);
        regs->length = 0;
    }

    return rval;
}

int CPQdb::getRegValuesByValue(const char *name, const char *f_name, const char *f2_name,
                                 const int val, const int val2, am_regs_t *regs)
{
    CSqlite::Cursor c_reg_list;
    char sqlmaster[256] = {0};
    int rval = -1;

    if ((strlen(f2_name) == 0) && (val2 == 0)) {
        getSqlParams(__FUNCTION__, sqlmaster,
                     "select RegType, RegAddr, RegMask, RegValue from %s where %s = %d;", name, f_name,
                     val);
    } else {
        getSqlParams(__FUNCTION__, sqlmaster,
                     "select RegType, RegAddr, RegMask, RegValue from %s where %s = %d and %s = %d;",
                     name, f_name, val, f2_name, val2);
    }

    rval = this->select(sqlmaster, c_reg_list);
    int count = c_reg_list.getCount();

    if (count < 0 ) {
        SYS_LOGE("%s, Select value error!\n", __FUNCTION__);
        regs->length = 0;
        return -1;
    } else if (count > REGS_MAX_NUMBER) {
        SYS_LOGE("%s, regs is too more, in pq.db count = %d", __FUNCTION__, count);
        regs->length = 0;
        return -1;
    }

    int index_am_reg = 0;
    if (c_reg_list.moveToFirst()) { //reg list for each table
        int index_type = 0;
        int index_addr = 1;
        int index_mask = 2;
        int index_val = 3;
        do {
            regs->am_reg[index_am_reg].type = c_reg_list.getUInt(index_type);
            regs->am_reg[index_am_reg].addr = c_reg_list.getUInt(index_addr);
            regs->am_reg[index_am_reg].mask = c_reg_list.getUInt(index_mask);
            regs->am_reg[index_am_reg].val = c_reg_list.getUInt(index_val);
            index_am_reg++;
        } while (c_reg_list.moveToNext());
        regs->length = index_am_reg;
    } else {
        regs->length = 0;
        rval = -1;
    }

    SYS_LOGI("%s, length = %d", __FUNCTION__, regs->length);
    return rval;
}

int CPQdb::getRegValuesByValue_long(const char *name, const char *f_name, const char *f2_name,
                                      const int val, const int val2, am_regs_t *regs, am_regs_t *regs_1)
{
    CSqlite::Cursor c_reg_list;
    char sqlmaster[256] = {0};
    int rval = -1;

    if ((strlen(f2_name) == 0) && (val2 == 0)) {
        getSqlParams(__FUNCTION__, sqlmaster,
                     "select RegType, RegAddr, RegMask, RegValue from %s where %s = %d;", name, f_name,
                     val);
    } else {
        getSqlParams(__FUNCTION__, sqlmaster,
                     "select RegType, RegAddr, RegMask, RegValue from %s where %s = %d and %s = %d;",
                     name, f_name, val, f2_name, val2);
    }

    rval = this->select(sqlmaster, c_reg_list);

    int index_am_reg = 0;
    int count = c_reg_list.getCount();
    if (count < 0) {
        SYS_LOGE("%s, Select value error!\n", __FUNCTION__);
        regs->length = 0;
        regs_1->length = 0;
        return -1;
    } else if (count > 1024) {
        SYS_LOGE("%s, regs is too more, in pq.db count = %d", __FUNCTION__, count);
        regs->length = 0;
        regs_1->length = 0;
        return -1;
    }
    if (c_reg_list.moveToFirst()) { //reg list for each table
        int index_type = 0;
        int index_addr = 1;
        int index_mask = 2;
        int index_val = 3;
        do {
            if (index_am_reg < 512) {
                regs->am_reg[index_am_reg].type = c_reg_list.getUInt(index_type);
                regs->am_reg[index_am_reg].addr = c_reg_list.getUInt(index_addr);
                regs->am_reg[index_am_reg].mask = c_reg_list.getUInt(index_mask);
                regs->am_reg[index_am_reg].val = c_reg_list.getUInt(index_val);
            } else if (index_am_reg >= 512 && index_am_reg < 1024) {
                regs_1->am_reg[index_am_reg - 512].type = c_reg_list.getUInt(index_type);
                regs_1->am_reg[index_am_reg - 512].addr = c_reg_list.getUInt(index_addr);
                regs_1->am_reg[index_am_reg - 512].mask = c_reg_list.getUInt(index_mask);
                regs_1->am_reg[index_am_reg - 512].val = c_reg_list.getUInt(index_val);
            } else {
            }
            index_am_reg++;
        } while (c_reg_list.moveToNext());

        if (index_am_reg < 512) {
            regs->length = index_am_reg;
        } else if (index_am_reg >= 512 && index_am_reg < 1024) {
            regs->length = 512;
            regs_1->length = index_am_reg - 512;
        }
    } else {
        SYS_LOGE("%s, Select value error!\n", __FUNCTION__);
        regs->length = 0;
        regs_1->length = 0;
        rval = -1;
    }

    return rval;
}

int CPQdb::PQ_GetBlackExtensionParams(source_input_param_t source_input_param, am_regs_t *regs)
{
    int rval = -1;

    if (CheckHdrStatus("GeneralBlackBlueTable"))
        source_input_param.sig_fmt = TVIN_SIG_FMT_HDMI_HDR;

    String8 TableName = GetTableName("GeneralBlackBlueTable", source_input_param);
    if ((TableName.c_str() != NULL) && (TableName.length() != 0) ) {
        rval = getRegValues(TableName.c_str(), regs);
    } else {
        SYS_LOGE("GeneralBlackBlueTable don't have table!!\n");
    }

    return rval;
}

int CPQdb::PQ_GetBlackStretchParams(int level, source_input_param_t source_input_param, am_regs_t *regs)
{
    int rval = -1;

    if (CheckHdrStatus("GeneralBlackTable"))
        source_input_param.sig_fmt = TVIN_SIG_FMT_HDMI_HDR;

    String8 TableName = GetTableName("GeneralBlackTable", source_input_param);
    if ((TableName.c_str() != NULL) && (TableName.length() != 0) ) {
        rval = getRegValuesByValue(TableName.c_str(), LEVEL_NAME, "", level, 0, regs);
    } else {
        SYS_LOGE("GeneralBlackTable don't have table!!\n");
    }

    return rval;
}

int CPQdb::PQ_GetBlueStretchParams(int level, source_input_param_t source_input_param, am_regs_t *regs)
{
    int rval = -1;

    if (CheckHdrStatus("GeneralBlueTable"))
        source_input_param.sig_fmt = TVIN_SIG_FMT_HDMI_HDR;

    String8 TableName = GetTableName("GeneralBlueTable", source_input_param);
    if ((TableName.c_str() != NULL) && (TableName.length() != 0) ) {
        rval = getRegValuesByValue(TableName.c_str(), LEVEL_NAME, "", level, 0, regs);
    } else {
        SYS_LOGE("GeneralBlueTable don't have table!!\n");
    }

    return rval;
}

int CPQdb::PQ_GetChromaCoringParams(int level, source_input_param_t source_input_param, am_regs_t *regs)
{
    int rval = -1;

    if (CheckHdrStatus("GeneralChromaTable"))
        source_input_param.sig_fmt = TVIN_SIG_FMT_HDMI_HDR;

    String8 TableName = GetTableName("GeneralChromaTable", source_input_param);
    if ((TableName.c_str() != NULL) && (TableName.length() != 0) ) {
        rval = getRegValuesByValue(TableName.c_str(), LEVEL_NAME, "", level, 0, regs);
    } else {
        SYS_LOGE("GeneralChromaTable don't have table!!\n");
    }

    return rval;
}

int CPQdb::PQ_GetSharpness0FixedParams(source_input_param_t source_input_param, am_regs_t *regs)
{
    int rval = -1;

    if (CheckHdrStatus("GeneralSharpness0FixedTable"))
        source_input_param.sig_fmt = TVIN_SIG_FMT_HDMI_HDR;

    String8 TableName = GetTableName("GeneralSharpness0FixedTable", source_input_param);
    if ((TableName.c_str() != NULL) && (TableName.length() != 0) ) {
        rval = getRegValues(TableName.c_str(), regs);
    } else {
        SYS_LOGE("GeneralSharpness0FixedTable don't have table!!\n");
    }

    return rval;
}

int CPQdb::PQ_SetSharpness0VariableParams(source_input_param_t source_input_param)
{
    int rval = -1;
    if (CheckHdrStatus("GeneralSharpness0VariableTable"))
        source_input_param.sig_fmt = TVIN_SIG_FMT_HDMI_HDR;

    String8 TableName = GetTableName("GeneralSharpness0VariableTable", source_input_param);
    if ((TableName.c_str() != NULL) && (TableName.length() != 0) ) {
        rval = loadSharpnessData(TableName.c_str(), 0);
    } else {
        SYS_LOGE("%s: GeneralSharpness0VariableTable don't have this table!\n", __FUNCTION__);
    }

    return rval;
}

int CPQdb::PQ_GetSharpness1FixedParams(source_input_param_t source_input_param, am_regs_t *regs)
{
    int rval = -1;

    if (CheckHdrStatus("GeneralSharpness1FixedTable"))
        source_input_param.sig_fmt = TVIN_SIG_FMT_HDMI_HDR;

    String8 TableName = GetTableName("GeneralSharpness1FixedTable", source_input_param);
    if ((TableName.c_str() != NULL) && (TableName.length() != 0) ) {
        rval = getRegValues(TableName.c_str(), regs);
    } else {
        SYS_LOGE("GeneralSharpness1FixedTable don't have table!!\n");
    }

    return rval;
}

int CPQdb::PQ_SetSharpness1VariableParams(source_input_param_t source_input_param)
{
    int rval = -1;
    if (CheckHdrStatus("GeneralSharpness1VariableTable"))
        source_input_param.sig_fmt = TVIN_SIG_FMT_HDMI_HDR;

    String8 TableName = GetTableName("GeneralSharpness1VariableTable", source_input_param);
    if ((TableName.c_str() != NULL) && (TableName.length() != 0) ) {
        rval = loadSharpnessData(TableName.c_str(), 1);
    } else {
        SYS_LOGE("%s: GeneralSharpness1VariableTable don't have this table!\n", __FUNCTION__);
    }

    return rval;
}

int CPQdb::PQ_GetSharpnessPiFixedParams(source_input_param_t source_input_param, am_regs_t *regs)
{
    int rval = -1;

    if (CheckHdrStatus("GeneralSharpnessPIFixedTable"))
        source_input_param.sig_fmt = TVIN_SIG_FMT_HDMI_HDR;

    String8 TableName = GetTableName("GeneralSharpnessPIFixedTable", source_input_param);
    if ((TableName.c_str() != NULL) && (TableName.length() != 0) ) {
        rval = getRegValues(TableName.c_str(), regs);
    } else {
        SYS_LOGE("GeneralSharpnessPIFixedTable don't have table!!\n");
    }

    return rval;
}

int CPQdb::PQ_SetSharpnessPiVariableParams(source_input_param_t source_input_param)
{
    int rval = -1;
    if (CheckHdrStatus("GeneralSharpnessPIVariableTable"))
        source_input_param.sig_fmt = TVIN_SIG_FMT_HDMI_HDR;

    String8 TableName = GetTableName("GeneralSharpnessPIVariableTable", source_input_param);
    if ((TableName.c_str() != NULL) && (TableName.length() != 0) ) {
        rval = loadSharpnessData(TableName.c_str(), 2);
    } else {
        SYS_LOGE("%s: GeneralSharpnessPIVariableTable don't have this table!\n", __FUNCTION__);
    }

    return rval;
}

int CPQdb::PQ_GetCM2Params(vpp_color_management2_t basemode, source_input_param_t source_input_param, am_regs_t *regs)
{
    int rval = -1;

    if (CheckHdrStatus("GeneralCM2Table"))
        source_input_param.sig_fmt = TVIN_SIG_FMT_HDMI_HDR;

    String8 TableName = GetTableName("GeneralCM2Table", source_input_param);
    if ((TableName.c_str() != NULL) && (TableName.length() != 0) ) {
        rval = getRegValuesByValue(TableName.c_str(), CM_LEVEL_NAME, "", (int) basemode, 0, regs);
    } else {
        SYS_LOGE("GeneralCM2Table select error!!\n");
    }

    return rval;
}

int CPQdb::PQ_GetXVYCCParams(vpp_xvycc_mode_t xvycc_mode, source_input_param_t source_input_param, am_regs_t *regs,
                               am_regs_t *regs_1)
{
    int rval = -1;
    String8 TableName = GetTableName("GeneralXVYCCTable", source_input_param);
    if ((TableName.c_str() != NULL) && (TableName.length() != 0) ) {
        rval = getRegValuesByValue_long(TableName.c_str(), LEVEL_NAME, "", (int) xvycc_mode, 0, regs, regs_1);
    } else {
        SYS_LOGE("GeneralXVYCCTable select error!!\n");
    }

    return rval;
}

int CPQdb::PQ_GetDIParams(source_input_param_t source_input_param, am_regs_t *regs)
{
    int rval = -1;
    String8 TableName = GetTableName("GeneralDITable", source_input_param);
    if ((TableName.c_str() != NULL) && (TableName.length() != 0) ) {
        rval = getDIRegValuesByValue(TableName.c_str(), "", "", 0, 0, regs);
    } else {
        SYS_LOGE("GeneralDITable select error!!\n");
    }

    return rval;
}

int CPQdb::PQ_GetMCDIParams(vpp_mcdi_mode_t mcdi_mode, source_input_param_t source_input_param, am_regs_t *regs)
{
    int rval = -1;
    String8 TableName = GetTableName("GeneralMCDITable", source_input_param);
    if ((TableName.c_str() != NULL) && (TableName.length() != 0) ) {
        rval = getDIRegValuesByValue(TableName.c_str(), LEVEL_NAME, "", (int) mcdi_mode, 0, regs);
    } else {
        SYS_LOGE("GeneralMCDITable select error!!\n");
    }

    return rval;
}

int CPQdb::PQ_GetDeblockParams(di_deblock_mode_t deb_mode, source_input_param_t source_input_param, am_regs_t *regs)
{
    int rval = -1;
    String8 TableName = GetTableName("GeneralDeblockTable", source_input_param);
    if ((TableName.c_str() != NULL) && (TableName.length() != 0) ) {
        rval = getDIRegValuesByValue(TableName.c_str(), LEVEL_NAME, "", (int) deb_mode, 0, regs);
    } else {
        SYS_LOGE("GeneralDeblockTable select error!!\n");
    }

    return rval;
}

int CPQdb::PQ_GetNR2Params(vpp_noise_reduction_mode_t nr_mode, source_input_param_t source_input_param, am_regs_t *regs)
{
    int rval = -1;
    String8 TableName = GetTableName("GeneralNR2Table", source_input_param);
    if ((TableName.c_str() != NULL) && (TableName.length() != 0) ) {
        rval = getDIRegValuesByValue(TableName.c_str(), LEVEL_NAME, "", (int) nr_mode, 0, regs);
    } else {
        SYS_LOGE("GeneralNR2Table select error!!\n");
    }

    return rval;
}

int CPQdb::PQ_GetSmoothPlusParams(vpp_smooth_plus_mode_t smoothplus_mode, source_input_param_t source_input_param, am_regs_t *regs)
{
    int rval = -1;
    String8 TableName = GetTableName("GeneralSmoothPlusTable", source_input_param);
    if ((TableName.c_str() != NULL) && (TableName.length() != 0) ) {
        rval = getDIRegValuesByValue(TableName.c_str(), LEVEL_NAME, "", (int)smoothplus_mode, 0, regs);
    } else {
        SYS_LOGE("GeneralSmoothPlusTable select error!!\n");
    }

    return rval;
}

int CPQdb::PQ_GetDemoSquitoParams(di_demosquito_mode_e demosquito, source_input_param_t source_input_param,  am_regs_t *regs)
{
    int rval = -1;
    String8 TableName = GetTableName("GeneralDemosquitoTable", source_input_param);
    if ((TableName.c_str() != NULL) && (TableName.length() != 0) ) {
        rval = getDIRegValuesByValue(TableName.c_str(), LEVEL_NAME, "", (int) demosquito, 0, regs);
    } else {
        SYS_LOGE("GeneralDemosquitoTable select error!!\n");
    }

    return rval;

}

int CPQdb::getDIRegValuesByValue(const char *name, const char *f_name, const char *f2_name,
                                                   const int val, const int val2, am_regs_t *regs)
{
    CSqlite::Cursor c_reg_list;
    char sqlmaster[256] = {0};
    int rval = -1;

    if ((strlen(f2_name) == 0) && (val2 == 0)) {
        if ((strlen(f_name) == 0) && (val == 0)) {
            getSqlParams(__FUNCTION__, sqlmaster,
                         "select RegType, RegAddr, RegMask, RegValue from %s ;", name);
        } else {
            getSqlParams(__FUNCTION__, sqlmaster,
                         "select RegType, RegAddr, RegMask, RegValue from %s where %s = %d;", name, f_name,
                         val);
        }
    } else {
        getSqlParams(__FUNCTION__, sqlmaster,
                     "select RegType, RegAddr, RegMask, RegValue from %s where %s = %d and %s = %d;",
                     name, f_name, val, f2_name, val2);
    }

    rval = this->select(sqlmaster, c_reg_list);
    int count = c_reg_list.getCount();
    if (count < 0) {
        SYS_LOGE("%s, select value error!\n", __FUNCTION__);
        return -1;
    } else if (count > REGS_MAX_NUMBER) {
        SYS_LOGE("%s, regs is too more, in pq.db count = %d", __FUNCTION__, count);
        return -1;
    }

    if (c_reg_list.moveToFirst()) { //reg list for each table
        int index_type = 0;
        int index_addr = 1;
        int index_mask = 2;
        int index_val = 3;
        do {
            regs->am_reg[regs->length].type = c_reg_list.getUInt(index_type);
            regs->am_reg[regs->length].addr = c_reg_list.getUInt(index_addr);
            regs->am_reg[regs->length].mask = c_reg_list.getUInt(index_mask);
            regs->am_reg[regs->length].val = c_reg_list.getUInt(index_val);
            regs->length++;
        } while (c_reg_list.moveToNext());
    } else {
        SYS_LOGE("%s, select value error!\n", __FUNCTION__);
        rval = -1;
    }

    SYS_LOGI("%s, length = %d", __FUNCTION__, regs->length);
    return rval;
}

int CPQdb::PQ_GetColorTemperatureParams(vpp_color_temperature_mode_t Tempmode, source_input_param_t source_input_param,
                                                   tcon_rgb_ogo_t *params)
{
    CSqlite::Cursor c;
    char sqlmaster[256] = {0};

    int rval = -1;
    //default
    params->en = 1;
    params->r_pre_offset = 0;
    params->g_pre_offset = 0;
    params->b_pre_offset = 0;
    params->r_gain = 1024;
    params->g_gain = 1024;
    params->b_gain = 1024;
    params->r_post_offset = 0;
    params->g_post_offset = 0;
    params->b_post_offset = 0;

    String8 TableName = GetTableName("GeneralWhiteBalanceTable", source_input_param);
    if ((TableName.c_str() != NULL) && (TableName.length() != 0) ) {
        getSqlParams(
            __FUNCTION__,
            sqlmaster,
            "select Enable, R_Pre_Offset, G_Pre_Offset, B_Pre_Offset, R_Gain, G_Gain, B_Gain, R_Post_Offset, G_Post_Offset, B_Post_Offset  from %s where "
            "Level = %d and def = 0;", TableName.c_str(), (int) Tempmode);

        rval = this->select(sqlmaster, c);

        if (c.moveToFirst()) {
            params->en = c.getInt(0);
            params->r_pre_offset = c.getInt(1);
            params->g_pre_offset = c.getInt(2);
            params->b_pre_offset = c.getInt(3);
            params->r_gain = c.getInt(4);
            params->g_gain = c.getInt(5);
            params->b_gain = c.getInt(6);
            params->r_post_offset = c.getInt(7);
            params->g_post_offset = c.getInt(8);
            params->b_post_offset = c.getInt(9);
        }
    } else {
        SYS_LOGE("GeneralWhiteBalanceTable select error!!\n");
    }

    return rval;
}

int CPQdb::PQ_SetColorTemperatureParams(vpp_color_temperature_mode_t Tempmode,source_input_param_t source_input_param,
                                                   tcon_rgb_ogo_t params)
{
    CSqlite::Cursor c;
    char sqlmaster[256] = {0};
    char sql[512];

    int rval = -1;
    String8 TableName = GetTableName("GeneralWhiteBalanceTable", source_input_param);
    if ((TableName.c_str() != NULL) && (TableName.length() != 0) ) {
        getSqlParams(
            __FUNCTION__,
            sql,
            "update %s set Enable = %d, "
            "R_Pre_Offset = %d, G_Pre_Offset = %d, B_Pre_Offset = %d, R_Gain = %d, G_Gain = %d, B_Gain = %d, "
            "R_Post_Offset = %d, G_Post_Offset = %d, B_Post_Offset = %d  where Level = %d and def = 0;",
            TableName.c_str(), params.en, params.r_pre_offset,
            params.g_pre_offset, params.b_pre_offset, params.r_gain, params.g_gain,
            params.b_gain, params.r_post_offset, params.g_post_offset, params.b_post_offset,
            Tempmode);

        if (this->exeSql(sql)) {
            rval = 0;
        } else {
            SYS_LOGE("%s, update error!\n", __FUNCTION__);
            rval = -1;
        }
    } else {
        SYS_LOGE("%s, GeneralWhiteBalanceTable don't have this table!\n", __FUNCTION__);
        rval = -1;
    }

    return rval;
}

int CPQdb::PQ_ResetAllColorTemperatureParams(void)
{
    CSqlite::Cursor c;
    char sqlmaster[512] = {0};

    int rval = -1;

    getSqlParams(__FUNCTION__, sqlmaster,
                 "select distinct TableName from GeneralWhiteBalanceTable ;");

    rval = this->select(sqlmaster, c);

    if (c.moveToFirst()) {
        int index_TableName = 0;
        do { //delete
            getSqlParams(
                __FUNCTION__,
                sqlmaster,
                "delete from %s where def = 0;"
                "insert into %s( Level , Enable , R_Pre_Offset, G_Pre_Offset, B_Pre_Offset, R_Gain, G_Gain, B_Gain, R_Post_Offset, G_Post_Offset, B_Post_Offset, def ) "
                "select Level, Enable, R_Pre_Offset, G_Pre_Offset, B_Pre_Offset, R_Gain, G_Gain, B_Gain, R_Post_Offset, G_Post_Offset, B_Post_Offset, 0 from %s where def = 1;",
                c.getString(index_TableName).c_str(), c.getString(index_TableName).c_str(),
                c.getString(index_TableName).c_str());
            if (this->exeSql(sqlmaster)) {
                rval = 0;
            } else {
                SYS_LOGE("%s, Delete values error!\n", __FUNCTION__);
                rval = -1;
            }
        } while (c.moveToNext());
    } else {
        SYS_LOGE("%s, GeneralWhiteBalanceTable don't have this table!\n", __FUNCTION__);
        rval = -1;
    }

    return rval;
}

int CPQdb::PQ_GetHDRTMOParams(source_input_param_t source_input_param, hdr_tmo_t mode, hdr_tmo_sw_s *newParams)
{
    CSqlite::Cursor c;
    char sqlmaster[256] = {0};
    int rval = -1;
    char buf[512];
    char *buffer = NULL;
    char *aa = NULL;
    char *aa_save[100];
    unsigned int index = 0;

    if (CheckHdrStatus("GeneralHDRNodeTable"))
        source_input_param.sig_fmt = TVIN_SIG_FMT_HDMI_HDR;

    String8 TableName = GetTableName("GeneralHDRNodeTable", source_input_param);
    if ((TableName.c_str() == NULL) || (TableName.length() == 0) ) {
        SYS_LOGD("%s, GeneralHDRNodeTable don't have this table!\n", __FUNCTION__);
        return -1;
    }

    memset(newParams, 0, sizeof(hdr_tmo_sw_s));

    {// for param
        getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where regnum < %d and level = %d",
                    TableName.c_str(), HDR_oo_init_lut,mode);

        rval = this->select(sqlmaster, c);

        if (c.moveToFirst()) {
            newParams->tmo_en            = c.getInt(0);
            newParams->reg_highlight     = c.getInt(1);
            newParams->reg_hist_th       = c.getInt(2);
            newParams->reg_light_th      = c.getInt(3);
            newParams->reg_highlight_th1 = c.getInt(4);
            newParams->reg_highlight_th2 = c.getInt(5);
            newParams->reg_display_e     = c.getInt(6);
            newParams->reg_middle_a      = c.getInt(7);
            newParams->reg_middle_a_adj  = c.getInt(8);
            newParams->reg_middle_b      = c.getInt(9);
            newParams->reg_middle_s      = c.getInt(10);
            newParams->reg_max_th1       = c.getInt(11);
            newParams->reg_middle_th     = c.getInt(12);
            newParams->reg_thold1        = c.getInt(13);
            newParams->reg_thold2        = c.getInt(14);
            newParams->reg_thold3        = c.getInt(15);
            newParams->reg_thold4        = c.getInt(16);
            newParams->reg_max_th2       = c.getInt(17);
            newParams->reg_pnum_th       = c.getInt(18);
            newParams->reg_hl0           = c.getInt(19);
            newParams->reg_hl1           = c.getInt(20);
            newParams->reg_hl2           = c.getInt(21);
            newParams->reg_hl3           = c.getInt(22);
            newParams->reg_display_adj   = c.getInt(23);
            newParams->reg_avg_th        = c.getInt(24);
            newParams->reg_avg_adj       = c.getInt(25);
            newParams->reg_low_adj       = c.getInt(26);
            newParams->reg_high_en       = c.getInt(27);
            newParams->reg_high_adj1     = c.getInt(28);
            newParams->reg_high_adj2     = c.getInt(29);
            newParams->reg_high_maxdiff  = c.getInt(30);
            newParams->reg_high_mindiff  = c.getInt(31);
            newParams->alpha             = c.getInt(32);
            newParams->reg_ratio         = c.getInt(33);
            newParams->reg_max_th3       = c.getInt(34);

        }else {
            SYS_LOGE("%s, read hdr tmo param fail\n", __FUNCTION__);
        }

        SYS_LOGD("%s - hdr tmo param is tmo_en:%d reg_highlight:%d reg_hist_th:%d reg_light_th:%d"
            " reg_highlight_th1:%d reg_highlight_th2:%d reg_display_e:%d reg_middle_a:%d"
            " reg_middle_a_adj:%d reg_middle_b:%d reg_middle_s:%d reg_max_th1:%d"
            " reg_middle_th:%d reg_thold1:%d reg_thold2:%d reg_thold3:%d reg_thold4:%d"
            " reg_max_th2:%d reg_pnum_th:%d reg_hl0:%d reg_hl1:%d reg_hl2:%d"
            " reg_hl3:%d reg_display_adj:%d reg_avg_th:%d reg_avg_adj:%d reg_low_adj:%d"
            " reg_high_en:%d reg_high_adj1:%d reg_high_adj2:%d reg_high_maxdiff:%d"
            " reg_high_mindiff:%d alpha:%d\n",
            __FUNCTION__, newParams->tmo_en, newParams->reg_highlight, newParams->reg_hist_th, newParams->reg_light_th,
            newParams->reg_highlight_th1, newParams->reg_highlight_th2, newParams->reg_display_e, newParams->reg_middle_a,
            newParams->reg_middle_a_adj, newParams->reg_middle_b, newParams->reg_middle_s, newParams->reg_max_th1,
            newParams->reg_middle_th, newParams->reg_thold1, newParams->reg_thold2, newParams->reg_thold3, newParams->reg_thold4,
            newParams->reg_max_th2, newParams->reg_pnum_th, newParams->reg_hl0, newParams->reg_hl1, newParams->reg_hl2,
            newParams->reg_hl3, newParams->reg_display_adj, newParams->reg_avg_th, newParams->reg_avg_adj, newParams->reg_low_adj,
            newParams->reg_high_en, newParams->reg_high_adj1, newParams->reg_high_adj2, newParams->reg_high_maxdiff,
            newParams->reg_high_mindiff, newParams->alpha);
    }

    //HDR_oo_init_lut
    {
        index = 0;
        aa = NULL;
        getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                    "regnum = %d and level = %d",
                    TableName.c_str(), HDR_oo_init_lut, mode);

        rval = this->select(sqlmaster, c);
        memset(buf, 0, sizeof(buf));
        strcpy(buf, c.getString(index).c_str());
        //SYS_LOGD ("%s - HDR_oo_init_lut is %s\n", __FUNCTION__, buf);
        buffer = buf;
        while ((aa_save[index] = strtok_r(buffer, ",", &aa)) != NULL) {
            newParams->oo_init_lut[index] = atoi(aa_save[index]);
            index ++;
            if (index >= sizeof(newParams->oo_init_lut)/sizeof(unsigned int)) {
                break;
            }
            buffer = NULL;
        }
    }

    return rval;
}

int CPQdb::PQ_GetAADParams(source_input_param_t source_input_param, aad_param_t *newParams)
{
    CSqlite::Cursor c;
    char sqlmaster[256] = {0};
    char buf[512];
    char *buffer = NULL;
    char *aa = NULL;
    char *aa_save[100];
    const char *delim = " ";

    unsigned int index = 0;
    int rval = -1;

    memset(newParams, 0, sizeof(aad_param_t));

    {   // for base param
        index = 0;
        getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                    "regnum >= 100 and regnum < 200",
                    PQ_DB_CABC_TABLE_NAME);

        rval = this->select(sqlmaster, c);
        if (c.moveToFirst()) {
            newParams->aad_param_cabc_aad_en   = c.getInt(0);
            newParams->aad_param_aad_en        = c.getInt(1);
            newParams->aad_param_tf_en         = c.getInt(2);
            newParams->aad_param_force_gain_en = c.getInt(3);
            newParams->aad_param_sensor_mode   = c.getInt(4);
            newParams->aad_param_mode          = c.getInt(5);
            newParams->aad_param_dist_mode     = c.getInt(6);
            newParams->aad_param_tf_alpha      = c.getInt(7);
        } else {
            SYS_LOGE("%s, read cabc param fail\n", __FUNCTION__);
        }

        SYS_LOGD ("%s - aad param is aad_param_cabc_aad_en:%d aad_param_aad_en:%d sensor_mode:%d mode:%d"
            "tf_en:%d tf_alpha:%d dist_mode:%d force_gain_en:%d\n",
            __FUNCTION__, newParams->aad_param_cabc_aad_en, newParams->aad_param_aad_en, newParams->aad_param_sensor_mode, newParams->aad_param_mode,
            newParams->aad_param_tf_en, newParams->aad_param_tf_alpha, newParams->aad_param_dist_mode, newParams->aad_param_force_gain_en);
    }
    {   // for sensor_input
        index = 0;
        aa = NULL;
        getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                    "regnum = %d",
                    PQ_DB_CABC_TABLE_NAME, sensor_input);

        rval = this->select(sqlmaster, c);
        memset(buf, 0, sizeof(buf));
        if (strlen(c.getString(index).c_str()) < sizeof(buf)) {
            strncpy(buf, c.getString(index).c_str(), strlen(c.getString(index).c_str()));
        } else {
            strncpy(buf, c.getString(index).c_str(), sizeof(buf));
        }
        SYS_LOGD ("%s - sensor_input is %s\n", __FUNCTION__, buf);
        buffer = buf;
        while ((aa_save[index] = strtok_r(buffer, delim, &aa)) != NULL) {
            SYS_LOGD ("%s - aa_save[%d]:%s\n", __FUNCTION__, index, aa_save[index]);
            newParams->aad_param_sensor_input[index] = atoi(aa_save[index]);
            index++;
            if (index >= sizeof(newParams->aad_param_sensor_input)/sizeof(int)) {
                break;
            }
            buffer = NULL;
        }
        newParams->aad_param_sensor_input_len = index;
    }

    {   // for LUT_Y_gain
        index = 0;
        aa = NULL;
        getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                    "regnum = %d",
                    PQ_DB_CABC_TABLE_NAME, LUT_Y_gain);

        rval = this->select(sqlmaster, c);
        memset(buf, 0, sizeof(buf));
        if (strlen(c.getString(index).c_str()) < sizeof(buf)) {
            strncpy(buf, c.getString(index).c_str(), strlen(c.getString(index).c_str()));
        } else {
            strncpy(buf, c.getString(index).c_str(), sizeof(buf));
        }
        SYS_LOGD ("%s - LUT_Y_gain is %s\n", __FUNCTION__, buf);
        buffer = buf;
        while ((aa_save[index] = strtok_r(buffer, delim, &aa)) != NULL) {
            SYS_LOGD ("%s - aa_save[%d]:%s\n", __FUNCTION__, index, aa_save[index]);
            newParams->aad_param_LUT_Y_gain[index] = atoi(aa_save[index]);
            index++;
            if (index >= sizeof(newParams->aad_param_LUT_Y_gain)/sizeof(int)) {
                break;
            }
            buffer = NULL;
        }
        newParams->aad_param_LUT_Y_gain_len = index;
    }

    {   // for LUT_RG_gain
        index = 0;
        aa = NULL;
        getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                    "regnum = %d",
                    PQ_DB_CABC_TABLE_NAME, LUT_RG_gain);

        rval = this->select(sqlmaster, c);
        memset(buf, 0, sizeof(buf));
        if (strlen(c.getString(index).c_str()) < sizeof(buf)) {
            strncpy(buf, c.getString(index).c_str(), strlen(c.getString(index).c_str()));
        } else {
            strncpy(buf, c.getString(index).c_str(), sizeof(buf));
        }
        SYS_LOGD ("%s - LUT_RG_gain is %s\n", __FUNCTION__, buf);
        buffer = buf;
        while ((aa_save[index] = strtok_r(buffer, delim, &aa)) != NULL) {
            SYS_LOGD ("%s - aa_save[%d]:%s\n", __FUNCTION__, index, aa_save[index]);
            newParams->aad_param_LUT_RG_gain[index] = atoi(aa_save[index]);
            index++;
            if (index >= sizeof(newParams->aad_param_LUT_RG_gain)/sizeof(int)) {
                break;
            }
            buffer = NULL;
        }
        newParams->aad_param_LUT_RG_gain_len = index;
    }

    {   // for LUT_BG_gain
        index = 0;
        aa = NULL;
        getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                    "regnum = %d",
                    PQ_DB_CABC_TABLE_NAME, LUT_BG_gain);

        rval = this->select(sqlmaster, c);
        memset(buf, 0, sizeof(buf));
        if (strlen(c.getString(index).c_str()) < sizeof(buf)) {
            strncpy(buf, c.getString(index).c_str(), strlen(c.getString(index).c_str()));
        } else {
            strncpy(buf, c.getString(index).c_str(), sizeof(buf));
        }
        SYS_LOGD ("%s - LUT_BG_gain is %s\n", __FUNCTION__, buf);
        buffer = buf;
        while ((aa_save[index] = strtok_r(buffer, delim, &aa)) != NULL) {
            SYS_LOGD ("%s - aa_save[%d]:%s\n", __FUNCTION__, index, aa_save[index]);
            newParams->aad_param_LUT_BG_gain[index] = atoi(aa_save[index]);
            index++;
            if (index >= sizeof(newParams->aad_param_LUT_BG_gain)/sizeof(int)) {
                break;
            }
            buffer = NULL;
        }
        newParams->aad_param_LUT_BG_gain_len = index;
    }

    {   // for gain_lut
        index = 0;
        aa = NULL;
        getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                    "regnum = %d",
                    PQ_DB_CABC_TABLE_NAME, gain_lut);

        rval = this->select(sqlmaster, c);
        memset(buf, 0, sizeof(buf));
        if (strlen(c.getString(index).c_str()) < sizeof(buf)) {
            strncpy(buf, c.getString(index).c_str(), strlen(c.getString(index).c_str()));
        } else {
            strncpy(buf, c.getString(index).c_str(), sizeof(buf));
        }
        SYS_LOGD ("%s - gain_lut is %s\n", __FUNCTION__, buf);
        buffer = buf;
        while ((aa_save[index] = strtok_r(buffer, delim, &aa)) != NULL) {
            SYS_LOGD ("%s - aa_save[%d]:%s\n", __FUNCTION__, index, aa_save[index]);
            newParams->aad_param_gain_lut[index] = atoi(aa_save[index]);
            index++;
            if (index >= sizeof(newParams->aad_param_gain_lut)/sizeof(int)) {
                break;
            }
            buffer = NULL;
        }
        newParams->aad_param_gain_lut_len = index;
    }

    {   // for xy_lut
        index = 0;
        aa = NULL;
        getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                    "regnum = %d",
                    PQ_DB_CABC_TABLE_NAME, xy_lut);

        rval = this->select(sqlmaster, c);
        memset(buf, 0, sizeof(buf));
        if (strlen(c.getString(index).c_str()) < sizeof(buf)) {
            strncpy(buf, c.getString(index).c_str(), strlen(c.getString(index).c_str()));
        } else {
            strncpy(buf, c.getString(index).c_str(), sizeof(buf));
        }
        SYS_LOGD ("%s - xy_lut is %s\n", __FUNCTION__, buf);
        buffer = buf;
        while ((aa_save[index] = strtok_r(buffer, delim, &aa)) != NULL) {
            SYS_LOGD ("%s - aa_save[%d]:%s\n", __FUNCTION__, index, aa_save[index]);
            newParams->aad_param_xy_lut[index] = atoi(aa_save[index]);
            index++;
            if (index >= sizeof(newParams->aad_param_xy_lut)/sizeof(int)) {
                break;
            }
            buffer = NULL;
        }
        newParams->aad_param_xy_lut_len = index;
    }

    return rval;
}

int CPQdb::PQ_GetCABCParams(source_input_param_t source_input_param, cabc_param_t *newParams)
{
    CSqlite::Cursor c;
    char sqlmaster[256] = {0};
    char buf[512];
    char *buffer = NULL;
    char *aa = NULL;
    char *aa_save[100];
    const char *delim = " ";
    unsigned int index = 0;
    int rval = -1;

    memset(newParams, 0, sizeof(cabc_param_t));

    {   // for base param
        getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                    "regnum >= 500 and regnum < 600",
                    PQ_DB_CABC_TABLE_NAME);

        rval = this->select(sqlmaster, c);
        if (c.moveToFirst()) {
            newParams->cabc_param_cabc_en          = c.getInt(0);
            newParams->cabc_param_hist_mode        = c.getInt(1);
            newParams->cabc_param_tf_en            = c.getInt(2);
            newParams->cabc_param_sc_flag          = c.getInt(3);
            newParams->cabc_param_bl_map_mode      = c.getInt(4);
            newParams->cabc_param_bl_map_en        = c.getInt(5);
            newParams->cabc_param_temp_proc        = c.getInt(6);
            newParams->cabc_param_max95_ratio      = c.getInt(7);
            newParams->cabc_param_hist_blend_alpha = c.getInt(8);
            newParams->cabc_param_init_bl_min      = c.getInt(9);
            newParams->cabc_param_init_bl_max      = c.getInt(10);
            newParams->cabc_param_tf_alpha         = c.getInt(11);
            newParams->cabc_param_sc_hist_diff_thd = c.getInt(12);
            newParams->cabc_param_sc_apl_diff_thd  = c.getInt(13);
            newParams->cabc_param_patch_bl_th      = c.getInt(14);
            newParams->cabc_param_patch_on_alpha   = c.getInt(15);
            newParams->cabc_param_patch_bl_off_th  = c.getInt(16);
            newParams->cabc_param_patch_off_alpha  = c.getInt(17);
        } else {
            SYS_LOGE("%s, read cabc param fail\n", __FUNCTION__);
        }

        SYS_LOGD ("%s - cabc param is cabc_en:%d hist_mode:%d"
            "tf_en:%d sc_flag:%d bl_map_mode:%d bl_map_en:%d temp_proc:%d"
            "max95_ratio:%d hist_blend_alpha:%d init_bl_min:%d init_bl_max:%d tf_alpha:%d"
            "sc_hist_diff_thd:%d sc_apl_diff_thd:%d patch_bl_th:%d patch_on_alpha:%d patch_bl_off_th:%d"
            "patch_off_alpha:%d\n",
            __FUNCTION__, newParams->cabc_param_cabc_en, newParams->cabc_param_hist_mode,
            newParams->cabc_param_tf_en, newParams->cabc_param_sc_flag, newParams->cabc_param_bl_map_mode, newParams->cabc_param_bl_map_en, newParams->cabc_param_temp_proc,
            newParams->cabc_param_max95_ratio, newParams->cabc_param_hist_blend_alpha, newParams->cabc_param_init_bl_min, newParams->cabc_param_init_bl_max, newParams->cabc_param_tf_alpha,
            newParams->cabc_param_sc_hist_diff_thd, newParams->cabc_param_sc_apl_diff_thd, newParams->cabc_param_patch_bl_th, newParams->cabc_param_patch_on_alpha, newParams->cabc_param_patch_bl_off_th,
            newParams->cabc_param_patch_off_alpha);
    }
    {   // for o_bl_cv
        index = 0;
        aa = NULL;
        getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                    "regnum = 600",
                    PQ_DB_CABC_TABLE_NAME);

        rval = this->select(sqlmaster, c);
        memset(buf, 0, sizeof(buf));
        if (strlen(c.getString(index).c_str()) < sizeof(buf)) {
            strncpy(buf, c.getString(index).c_str(), strlen(c.getString(index).c_str()));
        } else {
            strncpy(buf, c.getString(index).c_str(), sizeof(buf));
        }
        SYS_LOGD ("%s - db_o_bl_cv is %s\n", __FUNCTION__, buf);
        buffer = buf;
        while ((aa_save[index] = strtok_r(buffer, delim, &aa)) != NULL) {
            SYS_LOGD ("%s - aa_save[%d]:%s\n", __FUNCTION__, index, aa_save[index]);
            newParams->cabc_param_o_bl_cv[index] = atoi(aa_save[index]);
            index++;
            if (index >= sizeof(newParams->cabc_param_o_bl_cv)/sizeof(int)) {
                break;
            }
            buffer = NULL;
        }
        newParams->cabc_param_o_bl_cv_len = index;
    }
    {   // for db_maxbin_bl_cv
        index = 0;
        aa = NULL;
        getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                    "regnum = 601",
                    PQ_DB_CABC_TABLE_NAME);

        rval = this->select(sqlmaster, c);
        memset(buf, 0, sizeof(buf));
        if (strlen(c.getString(index).c_str()) < sizeof(buf)) {
            strncpy(buf, c.getString(index).c_str(), strlen(c.getString(index).c_str()));
        } else {
            strncpy(buf, c.getString(index).c_str(), sizeof(buf));
        }
        SYS_LOGD ("%s - db_o_bl_cv is %s\n", __FUNCTION__, buf);
        buffer = buf;
        while ((aa_save[index] = strtok_r(buffer, delim, &aa)) != NULL) {
            SYS_LOGD ("%s - aa_save[%d]:%s\n", __FUNCTION__, index, aa_save[index]);
            newParams->cabc_param_maxbin_bl_cv[index] = atoi(aa_save[index]);
            index++;
            if (index >= sizeof(newParams->cabc_param_maxbin_bl_cv)/sizeof(int)) {
                break;
            }
            buffer = NULL;
            newParams->cabc_param_maxbin_bl_cv_len = index;
        }
    }

    return rval;
}

int CPQdb::PQ_GetDNLPParams(source_input_param_t source_input_param, Dynamic_contrast_status_t mode, ve_dnlp_curve_param_t *newParams)
{
    CSqlite::Cursor c;
    CSqlite::Cursor c1;
    char sqlmaster[256] = {0};
    char buf[512];
    char *buffer = NULL;
    char *aa = NULL;
    char *aa_save[100];
    unsigned int index = 0;
    int rval = -1;

    memset(newParams, 0, sizeof(ve_dnlp_curve_param_s));

    if (CheckHdrStatus("GeneralDNLPTable"))
        source_input_param.sig_fmt = TVIN_SIG_FMT_HDMI_HDR;

    String8 TableName = GetTableName("GeneralDNLPTable", source_input_param);
    if ((TableName.c_str() != NULL) && (TableName.length() != 0) ) {
        { // for param
            index = 0;
            getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                        "regnum < 1000 and "
                        "level = %d;",
                        TableName.c_str(), mode);

            rval = this->select(sqlmaster, c1);
            if (c1.moveToFirst()) {
                index = 0;
                do {
                    newParams->param[index] = c1.getInt(0);
                    index++;
                    if (index >= sizeof(newParams->param)/sizeof(unsigned int)) {
                        break;
                    }
                } while (c1.moveToNext());
            }
        }
        { // for ve_dnlp_scurv_low
            index = 0;
            aa = NULL;
            getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                        "regnum = %d and "
                        "level =  %d;",
                        TableName.c_str(), scurv_low, mode);

            rval = this->select(sqlmaster, c1);
            memset(buf, 0, sizeof(buf));
            if (strlen(c1.getString(index).c_str()) < sizeof(buf)) {
                strncpy(buf, c1.getString(index).c_str(), strlen(c1.getString(index).c_str()));
            } else {
                strncpy(buf, c1.getString(index).c_str(), sizeof(buf));
            }
            //SYS_LOGD ("%s - ve_dnlp_scurv_low is %s+++++++++++++++++", __FUNCTION__, buf);
            buffer = buf;
            while ((aa_save[index] = strtok_r(buffer, " ", &aa)) != NULL) {
                newParams->ve_dnlp_scurv_low[index] = atoi(aa_save[index]);
                index ++;
                if (index >= sizeof(newParams->ve_dnlp_scurv_low)/sizeof(unsigned int)) {
                    break;
                }
                buffer = NULL;
            }
         }
        { // for scurv_mid1
            index = 0;
            aa = NULL;
            getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                        "regnum = %d and "
                        "level = %d;",
                        TableName.c_str(), scurv_mid1, mode);

            rval = this->select(sqlmaster, c1);
            memset(buf, 0, sizeof(buf));
            if (strlen(c1.getString(index).c_str()) < sizeof(buf)) {
                strncpy(buf, c1.getString(index).c_str(), strlen(c1.getString(index).c_str()));
            } else {
                strncpy(buf, c1.getString(index).c_str(), sizeof(buf));
            }
            //SYS_LOGD ("%s - ve_dnlp_scurv_mid1 is %s+++++++++++++++++", __FUNCTION__, buf);
            buffer = buf;
            while ((aa_save[index] = strtok_r(buffer, " ", &aa)) != NULL) {
                newParams->ve_dnlp_scurv_mid1[index] = atoi(aa_save[index]);
                index ++;
                if (index >= sizeof(newParams->ve_dnlp_scurv_mid1)/sizeof(unsigned int)) {
                    break;
                }
                buffer = NULL;
            }
         }
         { // for scurv_mid2
            index = 0;
            aa = NULL;
            getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                        "regnum = %d and "
                        "level = %d;",
                        TableName.c_str(), scurv_mid2, mode);

            rval = this->select(sqlmaster, c1);
            memset(buf, 0, sizeof(buf));
            if (strlen(c1.getString(index).c_str()) < sizeof(buf)) {
                strncpy(buf, c1.getString(index).c_str(), strlen(c1.getString(index).c_str()));
            } else {
                strncpy(buf, c1.getString(index).c_str(), sizeof(buf));
            }
            //SYS_LOGD ("%s - ve_dnlp_scurv_mid2 is %s+++++++++++++++++", __FUNCTION__, buf);
            buffer = buf;
            while ((aa_save[index] = strtok_r(buffer, " ", &aa)) != NULL) {
                newParams->ve_dnlp_scurv_mid2[index] = atoi(aa_save[index]);
                index ++;
                if (index >= sizeof(newParams->ve_dnlp_scurv_mid2)/sizeof(unsigned int)) {
                    break;
                }
                buffer = NULL;
            }
         }
         { // for scurv_hgh1
            index = 0;
            aa = NULL;
            getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                        "regnum = %d and "
                        "level = %d;",
                        TableName.c_str(), scurv_hgh1, mode);

            rval = this->select(sqlmaster, c1);
            memset(buf, 0, sizeof(buf));
            if (strlen(c1.getString(index).c_str()) < sizeof(buf)) {
                strncpy(buf, c1.getString(index).c_str(), strlen(c1.getString(index).c_str()));
            } else {
                strncpy(buf, c1.getString(index).c_str(), sizeof(buf));
            }
            //SYS_LOGD ("%s - ve_dnlp_scurv_hgh1 is %s+++++++++++++++++", __FUNCTION__, buf);
            buffer = buf;
            while ((aa_save[index] = strtok_r(buffer, " ", &aa)) != NULL) {
                newParams->ve_dnlp_scurv_hgh1[index] = atoi(aa_save[index]);
                index ++;
                if (index >= sizeof(newParams->ve_dnlp_scurv_hgh1)/sizeof(unsigned int)) {
                    break;
                }
                buffer = NULL;
            }
         }
         { // for scurv_hgh2
            index = 0;
            aa = NULL;
            getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                        "regnum = %d and "
                        "level = %d;",
                        TableName.c_str(), scurv_hgh2, mode);

            rval = this->select(sqlmaster, c1);
            memset(buf, 0, sizeof(buf));
            if (strlen(c1.getString(index).c_str()) < sizeof(buf)) {
                strncpy(buf, c1.getString(index).c_str(), strlen(c1.getString(index).c_str()));
            } else {
                strncpy(buf, c1.getString(index).c_str(), sizeof(buf));
            }
            //SYS_LOGD ("%s - ve_dnlp_scurv_hgh2 is %s+++++++++++++++++", __FUNCTION__, buf);
            buffer = buf;
            while ((aa_save[index] = strtok_r(buffer, " ", &aa)) != NULL) {
                newParams->ve_dnlp_scurv_hgh2[index] = atoi(aa_save[index]);
                index ++;
                if (index >= sizeof(newParams->ve_dnlp_scurv_hgh2)/sizeof(unsigned int)) {
                    break;
                }
                buffer = NULL;
            }
         }
         { // for curv_var_lut49
            index = 0;
            aa = NULL;
            getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                        "regnum = %d and "
                        "level = %d;",
                        TableName.c_str(), curv_var_lut49, mode);

            rval = this->select(sqlmaster, c1);
            memset(buf, 0, sizeof(buf));
            if (strlen(c1.getString(index).c_str()) < sizeof(buf)) {
                strncpy(buf, c1.getString(index).c_str(), strlen(c1.getString(index).c_str()));
            } else {
                strncpy(buf, c1.getString(index).c_str(), sizeof(buf));
            }
            //SYS_LOGD ("%s - ve_gain_var_lut49 is %s+++++++++++++++++", __FUNCTION__, buf);
            buffer = buf;
            while ((aa_save[index] = strtok_r(buffer, " ", &aa)) != NULL) {
                newParams->ve_gain_var_lut49[index] = atoi(aa_save[index]);
                index ++;
                if (index >= sizeof(newParams->ve_gain_var_lut49)/sizeof(unsigned int)) {
                    break;
                }
                buffer = NULL;
            }
         }
         { // for curv_wext_gain
            index = 0;
            aa = NULL;
            getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                        "regnum = %d and "
                        "level = %d;",
                        TableName.c_str(), curv_wext_gain, mode);

            rval = this->select(sqlmaster, c1);
            memset(buf, 0, sizeof(buf));
            if (strlen(c1.getString(index).c_str()) < sizeof(buf)) {
                strncpy(buf, c1.getString(index).c_str(), strlen(c1.getString(index).c_str()));
            } else {
                strncpy(buf, c1.getString(index).c_str(), sizeof(buf));
            }
            //SYS_LOGD ("%s - ve_wext_gain is %s+++++++++++++++++", __FUNCTION__, buf);
            buffer = buf;
            while ((aa_save[index] = strtok_r(buffer, " ", &aa)) != NULL) {
                newParams->ve_wext_gain[index] = atoi(aa_save[index]);
                index ++;
                if (index >= sizeof(newParams->ve_wext_gain)/sizeof(unsigned int)) {
                    break;
                }
                buffer = NULL;
            }
         }
         { // for adp_thrd
            index = 0;
            aa = NULL;
            getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                        "regnum = %d and "
                        "level = %d;",
                        TableName.c_str(), adp_thrd, mode);

            rval = this->select(sqlmaster, c1);
            memset(buf, 0, sizeof(buf));
            if (strlen(c1.getString(index).c_str()) < sizeof(buf)) {
                strncpy(buf, c1.getString(index).c_str(), strlen(c1.getString(index).c_str()));
            } else {
                strncpy(buf, c1.getString(index).c_str(), sizeof(buf));
            }
            //SYS_LOGD ("%s - ve_adp_thrd is %s+++++++++++++++++", __FUNCTION__, buf);
            buffer = buf;
            while ((aa_save[index] = strtok_r(buffer, " ", &aa)) != NULL) {
                newParams->ve_adp_thrd[index] = atoi(aa_save[index]);
                index ++;
                if (index >= sizeof(newParams->ve_adp_thrd)/sizeof(unsigned int)) {
                    break;
                }
                buffer = NULL;
            }
         }
         { // for reg_blk_boost_12
            index = 0;
            aa = NULL;
            getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                        "regnum = %d and "
                        "level = %d;",
                        TableName.c_str(), reg_blk_boost_12, mode);

            rval = this->select(sqlmaster, c1);
            memset(buf, 0, sizeof(buf));
            if (strlen(c1.getString(index).c_str()) < sizeof(buf)) {
                strncpy(buf, c1.getString(index).c_str(), strlen(c1.getString(index).c_str()));
            } else {
                strncpy(buf, c1.getString(index).c_str(), sizeof(buf));
            }
            //SYS_LOGD ("%s - ve_reg_blk_boost_12 is %s+++++++++++++++++", __FUNCTION__, buf);
            buffer = buf;
            while ((aa_save[index] = strtok_r(buffer, " ", &aa)) != NULL) {
                newParams->ve_reg_blk_boost_12[index] = atoi(aa_save[index]);
                index ++;
                if (index >= sizeof(newParams->ve_reg_blk_boost_12)/sizeof(unsigned int)) {
                    break;
                }
                buffer = NULL;
            }
         }
         { // for reg_adp_ofset_20
            index = 0;
            aa = NULL;
            getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                        "regnum = %d and "
                        "level = %d;",
                        TableName.c_str(), reg_adp_ofset_20, mode);

            rval = this->select(sqlmaster, c1);
            memset(buf, 0, sizeof(buf));
            if (strlen(c1.getString(index).c_str()) < sizeof(buf)) {
                strncpy(buf, c1.getString(index).c_str(), strlen(c1.getString(index).c_str()));
            } else {
                strncpy(buf, c1.getString(index).c_str(), sizeof(buf));
            }
            //SYS_LOGD ("%s - ve_reg_adp_ofset_20 is %s+++++++++++++++++", __FUNCTION__, buf);
            buffer = buf;
            while ((aa_save[index] = strtok_r(buffer, " ", &aa)) != NULL) {
                newParams->ve_reg_adp_ofset_20[index] = atoi(aa_save[index]);
                index ++;
                if (index >= sizeof(newParams->ve_reg_adp_ofset_20)/sizeof(unsigned int)) {
                    break;
                }
                buffer = NULL;
            }
         }
         { // for reg_mono_protect
            index = 0;
            aa = NULL;
            getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                        "regnum = %d and "
                        "level = %d;",
                        TableName.c_str(), reg_mono_protect, mode);

            rval = this->select(sqlmaster, c1);
            memset(buf, 0, sizeof(buf));
            if (strlen(c1.getString(index).c_str()) < sizeof(buf)) {
                strncpy(buf, c1.getString(index).c_str(), strlen(c1.getString(index).c_str()));
            } else {
                strncpy(buf, c1.getString(index).c_str(), sizeof(buf));
            }
            //SYS_LOGD ("%s - ve_reg_mono_protect is %s+++++++++++++++++", __FUNCTION__, buf);
            buffer = buf;
            while ((aa_save[index] = strtok_r(buffer, " ", &aa)) != NULL) {
                newParams->ve_reg_mono_protect[index] = atoi(aa_save[index]);
                index ++;
                if (index >= sizeof(newParams->ve_reg_mono_protect)/sizeof(unsigned int)) {
                    break;
                }
                buffer = NULL;
            }
         }
         { // for reg_trend_wht_expand_lut8
            index = 0;
            aa = NULL;
            getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                        "regnum = %d and "
                        "level = %d;",
                        TableName.c_str(), reg_trend_wht_expand_lut8, mode);

            rval = this->select(sqlmaster, c1);
            memset(buf, 0, sizeof(buf));
            if (strlen(c1.getString(index).c_str()) < sizeof(buf)) {
                strncpy(buf, c1.getString(index).c_str(), strlen(c1.getString(index).c_str()));
            } else {
                strncpy(buf, c1.getString(index).c_str(), sizeof(buf));
            }
            //SYS_LOGD ("%s - ve_reg_trend_wht_expand_lut8 is %s+++++++++++++++++", __FUNCTION__, buf);
            buffer = buf;
            while ((aa_save[index] = strtok_r(buffer, " ", &aa)) != NULL) {
                newParams->ve_reg_trend_wht_expand_lut8[index] = atoi(aa_save[index]);
                index ++;
                if (index >= sizeof(newParams->ve_reg_trend_wht_expand_lut8)/sizeof(unsigned int)) {
                    break;
                }
                buffer = NULL;
            }
         }
        { // for ve_c_hist_gain
           index = 0;
           aa = NULL;
           getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                       "regnum = %d and "
                       "level = %d;",
                       TableName.c_str(), c_hist_gain, mode);

           rval = this->select(sqlmaster, c1);
           memset(buf, 0, sizeof(buf));
           if (strlen(c1.getString(index).c_str()) < sizeof(buf)) {
                strncpy(buf, c1.getString(index).c_str(), strlen(c1.getString(index).c_str()));
           } else {
                strncpy(buf, c1.getString(index).c_str(), sizeof(buf));
           }
           //SYS_LOGD ("%s - c_hist_gain is %s+++++++++++++++++", __FUNCTION__, buf);
           buffer = buf;
           while ((aa_save[index] = strtok_r(buffer, " ", &aa)) != NULL) {
               newParams->ve_c_hist_gain[index] = atoi(aa_save[index]);
               index ++;
               if (index >= sizeof(newParams->ve_c_hist_gain)/sizeof(unsigned int)) {
                   break;
               }
               buffer = NULL;
           }
        }
        { // for ve_s_hist_gain
           index = 0;
           aa = NULL;
           getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                       "regnum = %d and "
                       "level = %d;",
                       TableName.c_str(), s_hist_gain, mode);

           rval = this->select(sqlmaster, c1);
           memset(buf, 0, sizeof(buf));
           if (strlen(c1.getString(index).c_str()) < sizeof(buf)) {
               strncpy(buf, c1.getString(index).c_str(), strlen(c1.getString(index).c_str()));
           }
           //SYS_LOGD ("%s - s_hist_gain is %s+++++++++++++++++", __FUNCTION__, buf);
           buffer = buf;
           while ((aa_save[index] = strtok_r(buffer, " ", &aa)) != NULL) {
               newParams->ve_s_hist_gain[index] = atoi(aa_save[index]);
               index ++;
               if (index >= sizeof(newParams->ve_s_hist_gain)/sizeof(unsigned int)) {
                   break;
               }
               buffer = NULL;
           }
        }
    } else {
        SYS_LOGE("%s, GeneralDNLPTable don't have this table!\n", __FUNCTION__);
        rval = -1;
    }
    return rval;
}

int CPQdb::PQ_GetLocalContrastNodeParams(source_input_param_t source_input_param, local_contrast_mode_t mode, ve_lc_curve_parm_t *Params)
{
    CSqlite::Cursor c;
    char sqlmaster[256] = {0};
    char buf[512];
    char *buffer = NULL;
    char *aa = NULL;
    char *aa_save[100];
    unsigned int index = 0;
    int rval = -1;

    if (CheckHdrStatus("GeneralLocalContrastNodeTable"))
        source_input_param.sig_fmt = TVIN_SIG_FMT_HDMI_HDR;

    String8 TableName = GetTableName("GeneralLocalContrastNodeTable", source_input_param);
    if ((TableName.c_str() != NULL) && (TableName.length() != 0) ) {
        { //for param
            index = 0;
            getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                        "regnum > 256 and "
                        "level = %d;",
                        TableName.c_str(), mode);

            rval = this->select(sqlmaster, c);
            if (c.moveToFirst()) {
                index = 0;
                do {
                    Params->param[index] = c.getInt(0);
                    index++;
                    if (index >= sizeof(Params->param)/sizeof(unsigned int)) {
                        break;
                    }
                } while (c.moveToNext());
            }
        }
        { //for ve_lc_saturation
            index = 0;
            aa = NULL;
            getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                        "regnum = %d and "
                        "level =  %d;",
                        TableName.c_str(), LC_SATURATION_NUM, mode);

            rval = this->select(sqlmaster, c);
            memset(buf, 0, sizeof(buf));
            if (strlen(c.getString(index).c_str()) < sizeof(buf)) {
                strncpy(buf, c.getString(index).c_str(), strlen(c.getString(index).c_str()));
            } else {
                strncpy(buf, c.getString(index).c_str(), sizeof(buf));
            }
            //SYS_LOGD("%s: ve_lc_saturation is %s\n", __FUNCTION__, buf);
            buffer = buf;
            while ((aa_save[index] = strtok_r(buffer, " ", &aa)) != NULL) {
                Params->ve_lc_saturation[index] = atoi(aa_save[index]);
                index ++;
                if (index >= sizeof(Params->ve_lc_saturation)/sizeof(unsigned int)) {
                    break;
                }
                buffer = NULL;
            }
         }
         { //for ve_lc_yminval_lmt
            index = 0;
            aa = NULL;
            getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                        "regnum = %d and "
                        "level = %d;",
                        TableName.c_str(), LC_YMINVAL_LMT_NUM, mode);

            rval = this->select(sqlmaster, c);
            memset(buf, 0, sizeof(buf));
            if (strlen(c.getString(index).c_str()) < sizeof(buf)) {
                strncpy(buf, c.getString(index).c_str(), strlen(c.getString(index).c_str()));
            } else {
                strncpy(buf, c.getString(index).c_str(), sizeof(buf));
            }
            //SYS_LOGD ("%s: ve_lc_yminval_lmt is %s\n", __FUNCTION__, buf);
            buffer = buf;
            while ((aa_save[index] = strtok_r(buffer, " ", &aa)) != NULL) {
                Params->ve_lc_yminval_lmt[index] = atoi(aa_save[index]);
                index ++;
                if (index >= sizeof(Params->ve_lc_yminval_lmt)/sizeof(unsigned int)) {
                    break;
                }
                buffer = NULL;
            }
         }
         { //for ve_lc_ypkbv_ymaxval_lmt
            index = 0;
            aa = NULL;
            getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                        "regnum = %d and "
                        "level = %d;",
                        TableName.c_str(), LC_YPKBV_YMAXVAL_LMT_NUM, mode);

            rval = this->select(sqlmaster, c);
            memset(buf, 0, sizeof(buf));
            if (strlen(c.getString(index).c_str()) < sizeof(buf)) {
                strncpy(buf, c.getString(index).c_str(), strlen(c.getString(index).c_str()));
            } else {
                strncpy(buf, c.getString(index).c_str(), sizeof(buf));
            }
            //SYS_LOGD ("%s: ve_lc_ypkbv_ymaxval_lmt is %s\n", __FUNCTION__, buf);
            buffer = buf;
            while ((aa_save[index] = strtok_r(buffer, " ", &aa)) != NULL) {
                Params->ve_lc_ypkbv_ymaxval_lmt[index] = atoi(aa_save[index]);
                index ++;
                if (index >= sizeof(Params->ve_lc_ypkbv_ymaxval_lmt)/sizeof(unsigned int)) {
                    break;
                }
                buffer = NULL;
            }
         }
         { //for ve_lc_ypkbv_ratio
            index = 0;
            aa = NULL;
            getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                        "regnum = %d and "
                        "level = %d;",
                        TableName.c_str(), LC_YPKBV_RATIO_NUM, mode);

            rval = this->select(sqlmaster, c);
            memset(buf, 0, sizeof(buf));
            if (strlen(c.getString(index).c_str()) < sizeof(buf)) {
                strncpy(buf, c.getString(index).c_str(), strlen(c.getString(index).c_str()));
            } else {
                strncpy(buf, c.getString(index).c_str(), sizeof(buf));
            }
            //SYS_LOGD ("%s: ve_lc_ypkbv_ratio is %s\n", __FUNCTION__, buf);
            buffer = buf;
            while ((aa_save[index] = strtok_r(buffer, " ", &aa)) != NULL) {
                Params->ve_lc_ypkbv_ratio[index] = atoi(aa_save[index]);
                index ++;
                if (index >= sizeof(Params->ve_lc_ypkbv_ratio)/sizeof(unsigned int)) {
                    break;
                }
                buffer = NULL;
            }
         }
         { //for ve_lc_ymaxval_lmt
            index = 0;
            aa = NULL;
            getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                        "regnum = %d and "
                        "level = %d;",
                        TableName.c_str(), LC_YMAXVAL_LMT_NUM, mode);

            rval = this->select(sqlmaster, c);
            if (rval < 0) {
                SYS_LOGE("%s: Table don't have this option!\n", __FUNCTION__);
                rval = 0;
            } else {
                memset(buf, 0, sizeof(buf));
                if (strlen(c.getString(index).c_str()) < sizeof(buf)) {
                    strncpy(buf, c.getString(index).c_str(), strlen(c.getString(index).c_str()));
                } else {
                    strncpy(buf, c.getString(index).c_str(), sizeof(buf));
                }
                //SYS_LOGD ("%s: ve_lc_ymaxval_lmt is %s\n", __FUNCTION__, buf);
                buffer = buf;
                while ((aa_save[index] = strtok_r(buffer, " ", &aa)) != NULL) {
                    Params->ve_lc_ymaxval_lmt[index] = atoi(aa_save[index]);
                    index ++;
                    if (index >= sizeof(Params->ve_lc_ymaxval_lmt)/sizeof(unsigned int)) {
                        break;
                    }
                    buffer = NULL;
                }
            }
         }
         { //for ve_lc_ypkbv_lmt
            index = 0;
            aa = NULL;
            getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                        "regnum = %d and "
                        "level = %d;",
                        TableName.c_str(), LC_YPKBV_LMT_NUM, mode);

            rval = this->select(sqlmaster, c);
            if (rval < 0) {
                SYS_LOGE("%s: Table don't have this option!\n", __FUNCTION__);
                rval = 0;
            } else {
                memset(buf, 0, sizeof(buf));
                if (strlen(c.getString(index).c_str()) < sizeof(buf)) {
                    strncpy(buf, c.getString(index).c_str(), strlen(c.getString(index).c_str()));
                }
                //SYS_LOGD ("%s: ve_lc_ypkbv_lmt is %s\n", __FUNCTION__, buf);
                buffer = buf;
                while ((aa_save[index] = strtok_r(buffer, " ", &aa)) != NULL) {
                    Params->ve_lc_ypkbv_lmt[index] = atoi(aa_save[index]);
                    index ++;
                    if (index >= sizeof(Params->ve_lc_ypkbv_lmt)/sizeof(unsigned int)) {
                        break;
                    }
                    buffer = NULL;
                }
            }
         }
    } else {
        SYS_LOGE("%s, GeneralLocalContrastNodeTable don't have this table!\n", __FUNCTION__);
        rval = -1;
    }
    return rval;
}

int CPQdb::PQ_GetLocalContrastRegParams(source_input_param_t source_input_param, local_contrast_mode_t mode, am_regs_t *regs)
{
    int ret = -1;

    if (CheckHdrStatus("GeneralLocalContrastRegTable"))
        source_input_param.sig_fmt = TVIN_SIG_FMT_HDMI_HDR;

    String8 TableName = GetTableName("GeneralLocalContrastRegTable", source_input_param);
    if ((TableName.c_str() != NULL) && (TableName.length() != 0) ) {
        ret = getRegValuesByValue(TableName.c_str(), LEVEL_NAME, "", (int) mode, 0, regs);
    } else {
        SYS_LOGE("GeneralLocalContrastRegTable select error!!\n");
    }

    return ret;
}

int CPQdb::PQ_SetDNLPGains(source_input_param_t source_input_param, Dynamic_contrast_status_t level, int final_gain)
{
    char sqlmaster[256] = {0};
    int final_gain_reg_num = 46;
    int rval = -1;

    String8 TableName = GetTableName("GeneralDNLPTable", source_input_param);
    if ((TableName.c_str() != NULL) && (TableName.length() != 0) ) {
        getSqlParams(__FUNCTION__, sqlmaster, "update  %s set value = %d where "
                      "regnum = %d and "
                      "level = %d;",
                      TableName.c_str(), final_gain, final_gain_reg_num, level);
        rval = this->exeSql(sqlmaster);
    } else {
        SYS_LOGE("%s: GeneralDNLPTable don't have this table!\n", __FUNCTION__);
    }

    return rval;
}

int CPQdb::PQ_GetDNLPGains(source_input_param_t source_input_param, Dynamic_contrast_status_t level)
{
    CSqlite::Cursor c;
    char sqlmaster[256] = {0};
    int final_gain = -1;
    int final_gain_reg_num = 46;

    String8 TableName = GetTableName("GeneralDNLPTable", source_input_param);
    if ((TableName.c_str() != NULL) && (TableName.length() != 0) ) {
        getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                      "regnum = %d and "
                      "level = %d;",
                      TableName.c_str(), final_gain_reg_num, level);
        this->select(sqlmaster, c);

        if (c.moveToFirst()) {
            final_gain = c.getInt(0);
        }
    } else {
        SYS_LOGE("%s: GeneralDNLPTable don't have this table!\n", __FUNCTION__);
    }

    SYS_LOGD("PQ_GetDNLPGains, get final_gain: %d", final_gain);
    return final_gain;
}

int CPQdb::PQ_GetBEParams(source_input_param_t source_input_param, int addr, am_regs_t *regs)
{
     int rval = -1;

     String8 TableName = GetTableName("GeneralBlackBlueTable", source_input_param);
     if ((TableName.c_str() != NULL) && (TableName.length() != 0) ) {
         rval = getRegValuesByValue(TableName.c_str(), "RegAddr", "", addr, 0, regs);
     } else {
         SYS_LOGE("%s: GeneralBlackBlueTable don't have this table!\n", __FUNCTION__);
     }

     return rval;
}

int CPQdb::PQ_SetBEParams(source_input_param_t source_input_param, int addr, unsigned int reg_val)
{
     int rval = -1;

     String8 TableName = GetTableName("GeneralBlackBlueTable", source_input_param);
     if ((TableName.c_str() != NULL) && (TableName.length() != 0) ) {
         String8 cmd = String8("update ") + String8::format("%s", TableName.c_str()) +
                       String8(" set RegValue = ") + String8::format("%d", reg_val) +
                       String8(" where RegAddr = ") + String8::format("%d", addr);
         rval = this->exeSql(cmd.c_str());
     } else {
         SYS_LOGE("%s: GeneralBlackBlueTable don't have this table!\n", __FUNCTION__);
     }
     return rval;
}

int CPQdb::PQ_SetRGBCMYFcolor(source_input_param_t source_input_param, int data_Rank ,int val)
{
    int rval = -1;
    char sql[256];
    String8 tableName;
    tv_source_input_t source_input = source_input_param.source_input;

    if (source_input == SOURCE_TV) {
        tableName = String8("CMS_ATV");
    } else if ((source_input == SOURCE_AV1) || (source_input == SOURCE_AV2)) {
        tableName = String8("CMS_AV");
    } else if ((source_input >= SOURCE_HDMI1) && (source_input <= SOURCE_HDMI4)) {
        tableName = String8("CMS_HDMI");
    } else if (source_input >= SOURCE_DTV) {
        tableName = String8("CMS_DTV");
    } else if (source_input >= SOURCE_MPEG) {
        tableName = String8("CMS_MPEG_HD");
    }

    if (mHdrStatus) {
        tableName = String8("CMS_HDR");
    }

    getSqlParams(__FUNCTION__,sql,
                "update %s set Value = %d where Rank = %d;",
                tableName.c_str(), val, data_Rank);

    if (this->exeSql(sql)) {
        rval = 0;
    } else {
        SYS_LOGE("%s: SQL error!\n",__FUNCTION__);
        rval = -1;
    }

    return rval;
}

int CPQdb::PQ_GetRGBCMYFcolor(source_input_param_t source_input_param, int data_Rank)
{
    int rval = -1;;
    CSqlite::Cursor c;
    char sqlmaster[256] = {0};
    String8 tableName;
    tv_source_input_t source_input = source_input_param.source_input;
    if (source_input == SOURCE_TV) {
        tableName = String8("CMS_ATV");
    } else if ((source_input == SOURCE_AV1) || (source_input == SOURCE_AV2)) {
        tableName = String8("CMS_AV");
    } else if ((source_input >= SOURCE_HDMI1) && (source_input <= SOURCE_HDMI4)) {
        tableName = String8("CMS_HDMI");
    } else if (source_input >= SOURCE_DTV) {
        tableName = String8("CMS_DTV");
    } else if (source_input >= SOURCE_MPEG) {
        tableName = String8("CMS_MPEG_HD");
    }

    if (mHdrStatus) {
        tableName = String8("CMS_HDR");
    }

    getSqlParams(__FUNCTION__, sqlmaster,
                 "select Value from %s where "
                 "Rank = %d ", tableName.c_str(), data_Rank);

    rval = this->select(sqlmaster, c);

    if (c.moveToFirst()) {
        rval = c.getInt(0);
    } else {
        SYS_LOGE("%s error!\n",__FUNCTION__);
        rval = -1;
    }

    return rval;
}

int CPQdb::PQ_SetNoLineAllBrightnessParams(tv_source_input_t source_input, int osd0,
        int osd25, int osd50, int osd75, int osd100)
{
    return SetNonlinearMapping(TVPQ_DATA_BRIGHTNESS, source_input, osd0, osd25, osd50, osd75, osd100);
}

int CPQdb::PQ_GetNoLineAllBrightnessParams(tv_source_input_t source_input, int *osd0,
        int *osd25, int *osd50, int *osd75, int *osd100)
{
    int osdvalue[5] = { 0 };
    int rval = GetNonlinearMappingByOSDFac(TVPQ_DATA_BRIGHTNESS, source_input, osdvalue);
    *osd0 = osdvalue[0];
    *osd25 = osdvalue[1];
    *osd50 = osdvalue[2];
    *osd75 = osdvalue[3];
    *osd100 = osdvalue[4];
    if (rval) {
        SYS_LOGE("PQ_GetNoLineAllBrightnessParams Error %d\n", rval);
    }
    return rval;

}

int CPQdb::PQ_GetBrightnessParams(source_input_param_t source_input_param, int level, int *params)
{
    int val = 0;
    GetNonlinearMapping(TVPQ_DATA_BRIGHTNESS, source_input_param.source_input, level, &val);
    *params = CalculateLevelParam(pq_bri_data, bri_nodes, val);
    return 0;

}

int CPQdb::PQ_SetBrightnessParams(source_input_param_t source_input_param, int level __unused, int params __unused)
{
    return 0;
}

int CPQdb::PQ_SetNoLineAllContrastParams(tv_source_input_t source_input, int osd0, int osd25,
        int osd50, int osd75, int osd100)
{
    return SetNonlinearMapping(TVPQ_DATA_CONTRAST, source_input, osd0, osd25, osd50, osd75, osd100);
}

int CPQdb::PQ_GetNoLineAllContrastParams(tv_source_input_t source_input, int *osd0,
        int *osd25, int *osd50, int *osd75, int *osd100)
{
    int osdvalue[5] = { 0 };
    int rval = GetNonlinearMappingByOSDFac(TVPQ_DATA_CONTRAST, source_input, osdvalue);
    *osd0 = osdvalue[0];
    *osd25 = osdvalue[1];
    *osd50 = osdvalue[2];
    *osd75 = osdvalue[3];
    *osd100 = osdvalue[4];
    if (rval) {
        SYS_LOGE("PQ_GetNoLineAllContrastParams Error %d\n", rval);
    }
    return rval;
}

int CPQdb::PQ_GetContrastParams(source_input_param_t source_input_param, int level, int *params)
{
    int val = 0;

    GetNonlinearMapping(TVPQ_DATA_CONTRAST, source_input_param.source_input, level, &val);
    *params = CalculateLevelParam(pq_con_data, con_nodes, val);
    return 0;
}

int CPQdb::PQ_SetContrastParams(source_input_param_t source_input_param, int level __unused, int params __unused)
{
    return 0;
}

int CPQdb::PQ_SetNoLineAllSaturationParams(tv_source_input_t source_input, int osd0,
        int osd25, int osd50, int osd75, int osd100)
{
    return SetNonlinearMapping(TVPQ_DATA_SATURATION, source_input, osd0, osd25, osd50, osd75, osd100);
}

int CPQdb::PQ_GetNoLineAllSaturationParams(tv_source_input_t source_input, int *osd0,
        int *osd25, int *osd50, int *osd75, int *osd100)
{
    int osdvalue[5] = { 0 };
    int rval = GetNonlinearMappingByOSDFac(TVPQ_DATA_SATURATION, source_input, osdvalue);
    *osd0 = osdvalue[0];
    *osd25 = osdvalue[1];
    *osd50 = osdvalue[2];
    *osd75 = osdvalue[3];
    *osd100 = osdvalue[4];
    if (rval) {
        SYS_LOGE("PQ_GetNoLineAllSaturationParams Error %d\n", rval);
    }
    return rval;
}
int CPQdb::PQ_GetSaturationParams(source_input_param_t source_input_param, int level, int *params)
{
    int val = 0;

    GetNonlinearMapping(TVPQ_DATA_SATURATION, source_input_param.source_input, level, &val);
    *params = CalculateLevelParam(pq_sat_data, sat_nodes, val);
    return 0;
}

int CPQdb::PQ_SetSaturationParams(source_input_param_t source_input_param, int level __unused, int params __unused)
{
    return 0;
}

int CPQdb::PQ_SetNoLineAllHueParams(tv_source_input_t source_input, int osd0, int osd25,
                                      int osd50, int osd75, int osd100)
{
    return SetNonlinearMapping(TVPQ_DATA_HUE, source_input, osd0, osd25, osd50, osd75, osd100);
}

int CPQdb::PQ_GetNoLineAllHueParams(tv_source_input_t source_input, int *osd0, int *osd25,
                                      int *osd50, int *osd75, int *osd100)
{
    int osdvalue[5] = { 0 };
    int rval = GetNonlinearMappingByOSDFac(TVPQ_DATA_HUE, source_input, osdvalue);
    *osd0 = osdvalue[0];
    *osd25 = osdvalue[1];
    *osd50 = osdvalue[2];
    *osd75 = osdvalue[3];
    *osd100 = osdvalue[4];
    if (rval) {
        SYS_LOGE("PQ_GetNoLineAllHueParams Error %d\n", rval);
    }
    return rval;
}
int CPQdb::PQ_GetHueParams(source_input_param_t source_input_param, int level, int *params)
{
    int val = 0;

    GetNonlinearMapping(TVPQ_DATA_HUE, source_input_param.source_input, level, &val);
    *params = CalculateLevelParam(pq_hue_data, hue_nodes, val);
    return 0;
}

int CPQdb::PQ_SetHueParams(source_input_param_t source_input_param, int level __unused, int params __unused)
{
    return 0;
}

int CPQdb::PQ_SetNoLineAllSharpnessParams(tv_source_input_t source_input, int osd0,
        int osd25, int osd50, int osd75, int osd100)
{
    return SetNonlinearMapping(TVPQ_DATA_SHARPNESS, source_input, osd0, osd25, osd50, osd75, osd100);
}

int CPQdb::PQ_GetNoLineAllSharpnessParams(tv_source_input_t source_input, int *osd0,
        int *osd25, int *osd50, int *osd75, int *osd100)
{
    int osdvalue[5] = { 0 };
    int rval = GetNonlinearMappingByOSDFac(TVPQ_DATA_SHARPNESS, source_input, osdvalue);
    *osd0 = osdvalue[0];
    *osd25 = osdvalue[1];
    *osd50 = osdvalue[2];
    *osd75 = osdvalue[3];
    *osd100 = osdvalue[4];
    if (rval) {
        SYS_LOGE("PQ_GetNoLineAllSharpnessParams Error %d\n", rval);
    }
    return rval;
}
int CPQdb::PQ_GetSharpness0Params(source_input_param_t source_input_param, int level, am_regs_t *regs)
{
    int val = 0;

    GetNonlinearMapping(TVPQ_DATA_SHARPNESS, source_input_param.source_input, level, &val);
    *regs = CalculateLevelRegsParam(pq_sharpness0_reg_data, val, 0);
    return 0;
}

int CPQdb::PQ_GetSharpness1Params(source_input_param_t source_input_param, int level, am_regs_t *regs)
{
    int val = 0;

    GetNonlinearMapping(TVPQ_DATA_SHARPNESS, source_input_param.source_input, level, &val);
    *regs = CalculateLevelRegsParam(pq_sharpness1_reg_data, val, 1);
    return 0;
}

int CPQdb::PQ_GetSharpnessPiParams(source_input_param_t source_input_param, int level, am_regs_t *regs)
{
    int val = 0;

    GetNonlinearMapping(TVPQ_DATA_SHARPNESS, source_input_param.source_input, level, &val);
    *regs = CalculateLevelRegsParam(pq_sharpnesspi_reg_data, val, 2);
    return 0;
}

int CPQdb::PQ_GetPLLParams(source_input_param_t source_input_param, am_regs_t *regs)
{
    int ret = -1;
    int i = 0;

    ret = getRegValuesByValue("ADC_Settings", "Port", "Format", source_input_param.source_input, source_input_param.sig_fmt, regs);

    if (ret == 0) {
        for (i = 0; i < regs->length; i++) {
            SYS_LOGD("%s, =================================================", "TV");
            SYS_LOGD("%s, regData.am_reg[%d].type = %d", "TV", i, regs->am_reg[i].type);
            SYS_LOGD("%s, regData.am_reg[%d].addr = %d", "TV", i, regs->am_reg[i].addr);
            SYS_LOGD("%s, regData.am_reg[%d].mask = %d", "TV", i, regs->am_reg[i].mask);
            SYS_LOGD("%s, regData.am_reg[%d].val  = %d", "TV", i, regs->am_reg[i].val);
        }
    }

    if (regs->am_reg[0].val == 0 && regs->am_reg[1].val == 0 && regs->am_reg[2].val == 0
            && regs->am_reg[3].val == 0) {
        SYS_LOGE("%s,db's value is all zeros, that's not OK!!!\n", "TV");
        return -1;
    }
    return ret;
}

int CPQdb::PQ_GetAIParams(aipq_mode_e mode, source_input_param_t source_input_param, ai_pic_table_t *aiRegs)
{
    CSqlite::Cursor c;
    char sqlmaster[256];
    char buf[512];
    int ret = -1;

    SYS_LOGD("%s: mode:%d\n", __FUNCTION__, mode);

    if (CheckHdrStatus("GeneralAITable")) {
        source_input_param.sig_fmt = TVIN_SIG_FMT_HDMI_HDR;
    }

    String8 TableName = GetTableName("GeneralAITable", source_input_param);
    SYS_LOGD("%s: TableName:%s\n", __FUNCTION__, TableName.string());
    if ((TableName.c_str() != NULL) && (TableName.length() != 0) ) {
        char sqlmaster[256] = {0};
        getSqlParams(
            __FUNCTION__,
            sqlmaster,
            "select Width, Height, Array from %s where Level = %d;", TableName.string(), (int)mode);
        this->select(sqlmaster, c);
        if (c.moveToFirst()) {
            aiRegs->width = c.getInt(0);
            aiRegs->height = c.getInt(1);
            SYS_LOGD("%s: aiRegs->width:%d aiRegs->height:%d\n", __FUNCTION__, aiRegs->width, aiRegs->height);
            if (strlen(c.getString(2).c_str()) < sizeof(buf)/sizeof(char)) {
                strncpy(buf, c.getString(2).c_str(), strlen(c.getString(2).c_str()));
            }
            aiRegs->table_ptr = buf;
            ret = 0;
        } else {
            SYS_LOGE("%s: select action error!\n", __FUNCTION__);
        }
    } else {
        SYS_LOGE("%s: GeneralAITable don't have table!\n", __FUNCTION__);
    }

    return ret;
}

int CPQdb::PQ_SetSharpnessParams(source_input_param_t source_input_param, int level __unused, am_regs_t regs __unused)
{
    return 0;
}

int CPQdb::PQ_SetNoLineAllVolumeParams(tv_source_input_t source_input, int osd0, int osd25,
        int osd50, int osd75, int osd100)
{
    return SetNonlinearMapping(TVPQ_DATA_VOLUME, source_input, osd0, osd25, osd50, osd75, osd100);
}

int CPQdb::PQ_GetNoLineAllVolumeParams(tv_source_input_t source_input, int *osd0, int *osd25,
        int *osd50, int *osd75, int *osd100)
{
    int osdvalue[5] = { 0 };
     int rval = GetNonlinearMappingByOSDFac(TVPQ_DATA_VOLUME, source_input, osdvalue);
    *osd0 = osdvalue[0];
    *osd25 = osdvalue[1];
    *osd50 = osdvalue[2];
    *osd75 = osdvalue[3];
    *osd100 = osdvalue[4];
    if (rval) {
        SYS_LOGE("PQ_GetNoLineAllSharpnessParams Error %d\n", rval);
    }
    return rval;
}

int CPQdb::PQ_ResetAllNoLineParams(void)
{
    int rval = 0;
    char sqlmaster[256] = {0};

    getSqlParams(
        __FUNCTION__,
        sqlmaster,
        "delete from NonlinearMapping; "
        "insert into NonlinearMapping(TVIN_PORT, Item_ID, Level, Value) select TVIN_PORT, Item_ID, Level, Value from NonlinearMapping_Default;");

    if (this->exeSql(sqlmaster)) {
        rval = 0;
    } else {
        rval = -1;
        SYS_LOGE("%s--SQL error!\n",__FUNCTION__);
    }

    return rval;
}
int CPQdb::PQ_GetNoiseReductionParams(vpp_noise_reduction_mode_t nr_mode, source_input_param_t source_input_param, int reg_addr)
{
    CSqlite::Cursor c;
    char sqlmaster[256] = {0};
    int reg_val = -1;

    String8 TableName = GetTableName("GeneralNR2Table", source_input_param);
    if ((TableName.c_str() != NULL) && (TableName.length() != 0) ) {
		getSqlParams(__FUNCTION__, sqlmaster, "select RegValue from %s where RegAddr = %d and Level = %d;", TableName.c_str(), reg_addr, nr_mode);

        this->select(sqlmaster, c);
        if (c.moveToFirst()) {
            reg_val = c.getInt(0);
        }
    } else {
        SYS_LOGE("%s: GeneralNR2Table don't have this table!\n", __FUNCTION__);
    }

    return reg_val;
}

int CPQdb::PQ_SetNoiseReductionParams(vpp_noise_reduction_mode_t nr_mode, source_input_param_t source_input_param , int reg_addr, int value)
{
    CSqlite::Cursor c;
    char sqlmaster[256] = {0};
    int err = -1;

    String8 TableName = GetTableName("GeneralNR2Table", source_input_param);
    if ((TableName.c_str() != NULL) && (TableName.length() != 0) ) {
        getSqlParams(__FUNCTION__, sqlmaster, "update %s set RegValue = %d where "
                 "RegAddr = %u and "
                 "Level = %d;",
                 TableName.c_str(), value, reg_addr, nr_mode);
        err = this->exeSql(sqlmaster);
    } else {
        SYS_LOGE("%s: GeneralNR2Table don't have this table!\n", __FUNCTION__);
    }

    return err;
}

int CPQdb::PQ_GetCVD2Param(source_input_param_t source_input_param, int reg_addr, int param_type, int reg_mask)
{
    CSqlite::Cursor c;
    char sqlmaster[256] = {0};
    int rval = -1;
    int reg_val = -1;

    if (param_type == CVD_YC_DELAY || param_type == DECODE_CTI)
        source_input_param.sig_fmt = TVIN_SIG_FMT_CVBS_NTSC_M;

    String8 TableName = GetTableName("GeneralCVD2Table", source_input_param);
    if ((TableName.c_str() != NULL) && (TableName.length() != 0) ) {
        getSqlParams(__FUNCTION__, sqlmaster, "select RegValue from %s where "
                     "RegAddr = %u and "
                     "RegMask = %u;",
                     TableName.c_str(), reg_addr, reg_mask);

        rval = this->select(sqlmaster, c);
        if (rval < 0) {
            return rval;
        }

        if (c.moveToFirst()) {
            reg_val = c.getInt(0);
        }
    } else {
        SYS_LOGE("%s: GeneralCVD2Table don't have this table!\n", __FUNCTION__);
    }

    SYS_LOGD("%s, addr:%d, sourde_input:%d, sig_fmt: %d, get value is %u",
              __FUNCTION__, reg_addr, source_input_param.source_input, source_input_param.sig_fmt, reg_val);

    return reg_val;
}

int CPQdb::PQ_SetCVD2Param(source_input_param_t source_input_param, int reg_addr,
                                 int value, int param_type, int reg_mask)
{
    char sqlmaster[256] = {0};
    int rval = -1;

    if (param_type == CVD_YC_DELAY || param_type == DECODE_CTI)
        source_input_param.sig_fmt = TVIN_SIG_FMT_CVBS_NTSC_M;
    SYS_LOGD("%s:, param_type is %d, sig_fmt is %d, source is %d, addr is %d, reg_mask is %u\n",
              __FUNCTION__, param_type, source_input_param.sig_fmt, source_input_param.source_input, reg_addr, reg_mask);

    String8 TableName = GetTableName("GeneralCVD2Table", source_input_param);
    if ((TableName.c_str() != NULL) && (TableName.length() != 0) ) {
        getSqlParams(__FUNCTION__, sqlmaster, "update %s set RegValue = %d where "
                     "RegAddr = %u and "
                     "RegMask = %u;",
                     TableName.c_str(), value, reg_addr, reg_mask);
        rval = this->exeSql(sqlmaster);
    } else {
        SYS_LOGE("%s: GeneralCVD2Table don't have this table!\n", __FUNCTION__);
    }

    return rval;
}


int CPQdb::PQ_GetCVD2Params(source_input_param_t source_input_param, am_regs_t *regs)
{
    int ret = -1;
    String8 TableName = GetTableName("GeneralCVD2Table", source_input_param);
    if ((TableName.c_str() != NULL) && (TableName.length() != 0) ) {
        ret = getRegValues(TableName.c_str(), regs);
        if (regs->am_reg[0].val == 0 && regs->am_reg[1].val == 0 && regs->am_reg[2].val == 0
                && regs->am_reg[3].val == 0) {
            SYS_LOGE("%s: db's value is all zeros, that's not OK!!!\n", __FUNCTION__);
            return -1;
        }
    } else {
        SYS_LOGE("%s: GeneralCVD2Table don't have this table!\n", __FUNCTION__);
    }
    return ret;
}

int CPQdb::PQ_GetSharpnessCTIParams(source_input_param_t source_input_param, int reg_addr,
                                              int param_type, int reg_mask)
{
    CSqlite::Cursor c, c1;
    char sqlmaster[256] = {0};
    int err = -1;
    unsigned int reg_val = 0;
    char table_name[128];
    unsigned int sr0_reg_val = 0;
    unsigned int sr1_reg_val = 0;

    if (param_type == CVD_YC_DELAY || param_type == DECODE_CTI) {
        return PQ_GetCVD2Param(source_input_param, reg_addr, param_type, reg_mask);
    }

    getSqlParams(__FUNCTION__,sqlmaster,
                 "select TableName from %s where "
                 "TVIN_PORT = %d and "
                 "TVIN_SIG_FMT = %d and "
                 "TVIN_TRANS_FMT = %d and "
                 "%s = %d;",
                 "GeneralSharpnessG9Table", source_input_param.source_input,
                 source_input_param.sig_fmt, source_input_param.trans_fmt, ID_FIELD, 1);

    err = this->select(sqlmaster, c);

    if (c.getCount() <= 0) {
        source_input_param.sig_fmt = TVIN_SIG_FMT_NULL;
        c.close();
        SYS_LOGE ("%s - Load default", __FUNCTION__);
        getSqlParams(__FUNCTION__,sqlmaster,
                 "select TableName from %s where "
                 "TVIN_PORT = %d and "
                 "TVIN_SIG_FMT = %d and "
                 "TVIN_TRANS_FMT = %d and "
                 "%s = %d;",
                 "GeneralSharpnessG9Table", source_input_param.source_input,
                 source_input_param.sig_fmt, source_input_param.trans_fmt, ID_FIELD, 1);
        err = this->select(sqlmaster, c);
    }

    if (c.moveToFirst()) {
        do {
            SYS_LOGD("%s: get table is %s\n", __FUNCTION__, c.getString(0).c_str());
            SYS_LOGD("%s, addr is %d, reg mask is: %u\n", __FUNCTION__, reg_addr, reg_mask);
            getSqlParams(__FUNCTION__, sqlmaster,
                         "select RegValue from %s where NodeNumber = %d and RegAddr = %d and RegMask = %u;",
                         c.getString(0).c_str(), 1, reg_addr, reg_mask);
            err = this->select(sqlmaster, c1);

            if (c1.moveToFirst()) {
                do {
                    if (param_type >= SR0_CTI_GAIN0 && param_type <= SR0_CTI_GAIN3
                        && !strncmp("Sharpness_0", c.getString(0).c_str(), 11)) {//SR0

                        sr0_reg_val = c1.getUInt(0);
                        SYS_LOGD("%s, addr is %d, reg_get sr0_reg_val: %u, %u\n", __FUNCTION__,
                                                         reg_addr, c1.getUInt(0), sr0_reg_val);
                        return sr0_reg_val;
                    } else if (param_type >= SR1_CTI_GAIN0 && param_type <= SR1_CTI_GAIN3
                        && !strncmp("Sharpness_1", c.getString(0).c_str(), 11)) {//SR1

                        sr1_reg_val = c1.getUInt(0);
                        SYS_LOGD("%s, addr is %d, get reg_val: %u, %u\n", __FUNCTION__,
                                                         reg_addr, c1.getUInt(0), sr1_reg_val);
                        return sr1_reg_val;
                    }
                } while (c1.moveToNext());
            }
        } while (c.moveToNext());
    }

    SYS_LOGE("%s, get value from db error", __FUNCTION__);
    return err;
}

int CPQdb::PQ_SetSharpnessCTIParams(source_input_param_t source_input_param, int reg_addr,
                                              int value, int param_type, int reg_mask)
{
    CSqlite::Cursor c, c1;
    char sqlmaster[256] = {0};
    int err = -1;

    if (param_type == CVD_YC_DELAY || param_type == DECODE_CTI) {
        return PQ_SetCVD2Param(source_input_param, reg_addr, value, param_type, reg_mask);
    }

    getSqlParams(__FUNCTION__,sqlmaster,
                 "select TableName from %s where "
                 "TVIN_PORT = %d and "
                 "TVIN_SIG_FMT = %d and "
                 "TVIN_TRANS_FMT = %d and "
                 "%s = %d;",
                 "GeneralSharpnessG9Table", source_input_param.source_input,
                 source_input_param.sig_fmt, source_input_param.trans_fmt, ID_FIELD, 1);

    err = this->select(sqlmaster, c);

    if (c.getCount() <= 0) {
        source_input_param.sig_fmt = TVIN_SIG_FMT_NULL;
        c.close();
        SYS_LOGD ("%s - Load default", __FUNCTION__);
        getSqlParams(__FUNCTION__,sqlmaster,
                 "select TableName from %s where "
                 "TVIN_PORT = %d and "
                 "TVIN_SIG_FMT = %d and "
                 "TVIN_TRANS_FMT = %d and "
                 "%s = %d;",
                 "GeneralSharpnessG9Table", source_input_param.source_input,
                 source_input_param.sig_fmt, source_input_param.trans_fmt, ID_FIELD, 1);
        err = this->select(sqlmaster, c);
    }

    if (c.moveToFirst()) {
        do {
            SYS_LOGD("%s: get table is %s\n", __FUNCTION__, c.getString(0).c_str());
            getSqlParams(__FUNCTION__, sqlmaster,
                         "select RegValue from %s where NodeNumber = %d and RegAddr = %d;",
                         c.getString(0).c_str(), 1, reg_addr);
            err = this->select(sqlmaster, c1);

            if (c1.moveToFirst()) {
                do {
                    if (param_type >= SR0_CTI_GAIN0 && param_type <= SR0_CTI_GAIN3
                        && !strncmp("Sharpness_0", c.getString(0).c_str(), 11)) {//SR0
                        SYS_LOGD("%s, addr is %d, last sr0_reg_val: %u, try to set %u\n",
                                     __FUNCTION__, reg_addr, c1.getUInt(0), value);
                        getSqlParams(__FUNCTION__, sqlmaster,
                                 "update %s set RegValue = %u where NodeNumber = %d and RegAddr = %d "
                                 "and RegMask = %u;",
                                 c.getString(0).c_str(), value, 1, reg_addr, reg_mask);
                        err |= this->exeSql(sqlmaster);
                        return err;
                    } else if (param_type >= SR1_CTI_GAIN0 && param_type <= SR1_CTI_GAIN3
                        && !strncmp("Sharpness_1", c.getString(0).c_str(), 11)) {//SR1
                        SYS_LOGD("%s, addr is %d, last sr1_reg_val: %u, try to set %u\n",
                                     __FUNCTION__, reg_addr, c1.getUInt(0), value);
                        getSqlParams(__FUNCTION__, sqlmaster,
                                 "update %s set RegValue = %u where NodeNumber = %d and RegAddr = %d "
                                 "and RegMask = %u;",
                                 c.getString(0).c_str(), value, 1, reg_addr, reg_mask);
                        err |= this->exeSql(sqlmaster);
                        return err;
                    }
                } while (c1.moveToNext());
            }
        } while (c.moveToNext());
    }

    return err;
}

const char *CPQdb::getSharpnessTableName(source_input_param_t source_input_param, int isHd)
{
    switch (source_input_param.source_input) {
        case SOURCE_TV: {
          if (!isHd) {
              return "Sharpness_0_ATV_Fixed";
          }
          return "Sharpness_1_ATV_Fixed";
        }
        case SOURCE_AV1:
        case SOURCE_AV2:{
          if (source_input_param.sig_fmt == 0x601 || source_input_param.sig_fmt == 0x602) {
              if (!isHd) {
                  return "Sharpness_0_AV_NTSC_Fixed";
              }
              return "Sharpness_1_AV_NTSC_Fixed";
          } else {
              if (!isHd) {
                  return "Sharpness_0_AV_PAL_Fixed";
              }
              return "Sharpness_1_AV_PAL_Fixed";
          }
        }
        case SOURCE_HDMI1:
        case SOURCE_HDMI2:
        case SOURCE_HDMI3:
        case SOURCE_HDMI4: {
          if (!isHd) {
              return "Sharpness_0_HDMI_SD_Fixed";
          }
          return "Sharpness_1_HDMI_HD_Fixed";
        }
        case SOURCE_MPEG: {
          if (!isHd) {
              return "Sharpness_0_MPEG_SD_Fixed";
          }
          return "Sharpness_1_MPEG_HD_Fixed";
        }
        case SOURCE_DTV: {
          if (!isHd) {
              return "Sharpness_0_DTV_SD_Fixed";
          }
          return "Sharpness_1_DTV_HD_Fixed";
        }
        default: {
          break;
        }
    }
    return NULL;
}


int CPQdb::PQ_GetSharpnessAdvancedParams(source_input_param_t source_input_param, int reg_addr, int isHd)
{
  int reg_val = 0;
  am_regs_t regs;
  memset(&regs, 0, sizeof(am_regs_t));

  reg_val = getSharpnessRegValues("GeneralCommonTable", source_input_param, &regs, reg_addr, isHd);
  SYS_LOGD("%s, get value is %u\n", __FUNCTION__, reg_val);

  return reg_val;
}

int CPQdb::getSharpnessRegValues(const char *table_name, source_input_param_t source_input_param,
                                         am_regs_t *regs, int reg_addr, int isHd)
{
    CSqlite::Cursor c_tablelist, c_reg_list;
    int index_am_reg = 0;
    char sqlmaster[256] = {0};
    tvin_sig_fmt_t signal = source_input_param.sig_fmt;

    if (table_name == NULL || !strlen(table_name)) {
        SYS_LOGE("%s, table_name is null\n", __FUNCTION__);
        return index_am_reg;
    }

    getSqlParams(__FUNCTION__, sqlmaster,
               "select TableName from %s where "
               "TVIN_PORT = %d and "
               "TVIN_SIG_FMT = %d and "
               "TVIN_TRANS_FMT = %d ;", table_name, source_input_param.source_input, signal, source_input_param.trans_fmt);
    this->select(sqlmaster, c_tablelist);

    if (c_tablelist.getCount() <= 0) {
        signal = TVIN_SIG_FMT_NULL;
        c_tablelist.close();
        SYS_LOGD ("%s - Load default", __FUNCTION__);

        getSqlParams(__FUNCTION__, sqlmaster,
                   "select TableName from %s where "
                   "TVIN_PORT = %d and "
                   "TVIN_SIG_FMT = %d and "
                   "TVIN_TRANS_FMT = %d ;", table_name, source_input_param.source_input, signal, source_input_param.trans_fmt);
        this->select(sqlmaster, c_tablelist);
    }

    if (c_tablelist.moveToFirst()) { //for table list
        do {
            getSqlParams(__FUNCTION__, sqlmaster,
                       "select RegType, RegAddr, RegMask, RegValue from %s;",
                       c_tablelist.getString(0).c_str());
            this->select(sqlmaster, c_reg_list);
            SYS_LOGD("%s, addr id 0x%x, [%d]table name is %s-----\n",
                    __FUNCTION__, reg_addr, index_am_reg, c_tablelist.getString(0).c_str());
            if (c_reg_list.moveToFirst()) { //reg list for each table
                int index_type = 0;
                int index_addr = 1;
                int index_mask = 2;
                int index_val = 3;
                do {
                    if ((strcmp(c_tablelist.getString(0).c_str(), getSharpnessTableName(source_input_param, isHd)) == 0)
                        && reg_addr == c_reg_list.getInt(index_addr)) {
                        regs->am_reg[index_am_reg].type = c_reg_list.getUInt(index_type);
                        regs->am_reg[index_am_reg].addr = c_reg_list.getUInt(index_addr);
                        regs->am_reg[index_am_reg].mask = c_reg_list.getUInt(index_mask);
                        regs->am_reg[index_am_reg].val = c_reg_list.getUInt(index_val);

                        SYS_LOGD("getSharpnessRegValues, addr is 0x%x, [%d]get value is %u\n",
                                 reg_addr, index_am_reg, regs->am_reg[index_am_reg].val);
                        return regs->am_reg[index_am_reg].val;
                    }

                    index_am_reg++;
                } while (c_reg_list.moveToNext());
            }
        } while (c_tablelist.moveToNext());
        regs->length = index_am_reg;
      }else {
          regs->length = 0;
          SYS_LOGE ("Don't have table in %s !\n",table_name);
    }
    return 0;
}

int CPQdb::PQ_SetSharpnessAdvancedParams(source_input_param_t source_input_param, int reg_addr, int value, int isHd)
{
  CSqlite::Cursor c_tablelist, c_reg_list;
  int err = -1;
  int index_am_reg = 0;
  char sqlmaster[256] = {0};
  const char *table_name = "GeneralCommonTable";
  tvin_sig_fmt_t signal = source_input_param.sig_fmt;

  getSqlParams(__FUNCTION__, sqlmaster,
               "select TableName from %s where "
               "TVIN_PORT = %d and "
               "TVIN_SIG_FMT = %d and "
               "TVIN_TRANS_FMT = %d ;", table_name, source_input_param.source_input, signal, source_input_param.trans_fmt);
  this->select(sqlmaster, c_tablelist);

  if (c_tablelist.getCount() <= 0) {
      signal = TVIN_SIG_FMT_NULL;
      c_tablelist.close();
      SYS_LOGD ("%s - Load default", __FUNCTION__);

      getSqlParams(__FUNCTION__, sqlmaster,
                   "select TableName from %s where "
                   "TVIN_PORT = %d and "
                   "TVIN_SIG_FMT = %d and "
                   "TVIN_TRANS_FMT = %d ;", table_name, source_input_param.source_input, signal, source_input_param.trans_fmt);
      err = this->select(sqlmaster, c_tablelist);
  }

  if (c_tablelist.moveToFirst()) { //for table list
      do {
          getSqlParams(__FUNCTION__, sqlmaster,
                       "select RegType, RegAddr, RegMask, RegValue from %s;",
                       c_tablelist.getString(0).c_str());
          err = this->select(sqlmaster, c_reg_list);
          SYS_LOGD("%s, addr id 0x%x, [%d]table name is %s-----\n",
                    __FUNCTION__, reg_addr, index_am_reg, c_tablelist.getString(0).c_str());
          if (c_reg_list.moveToFirst()) { //reg list for each table
              int index_type = 0;
              int index_addr = 1;
              int index_mask = 2;
              int index_val = 3;
              do {
                  if ((strcmp(c_tablelist.getString(0).c_str(), getSharpnessTableName(source_input_param, isHd)) == 0)
                      && reg_addr == c_reg_list.getInt(index_addr)) {
                      SYS_LOGD("PQ_SetSharpnessAdvancedParams, addr is 0x%x, [%d]last reg value is %u, try to set %u\n",
                                 reg_addr, index_am_reg, c_reg_list.getUInt(index_val), value);
                      getSqlParams(__FUNCTION__,sqlmaster,
                                   "update %s set RegValue = %d where RegAddr = %u;",
                                   c_tablelist.getString(0).c_str(), value, reg_addr);
                      err = this->exeSql(sqlmaster);
                      return err;
                  }

                  index_am_reg++;
              } while (c_reg_list.moveToNext());
          }
      } while (c_tablelist.moveToNext());
  }else {
      SYS_LOGE ("Don't have table in %s !\n",table_name);
  }

  return err;
}

int CPQdb::PQ_ResetAllOverscanParams(void)
{
    int rval;
    char sqlmaster[256] = {0};

    getSqlParams(
        __FUNCTION__,
        sqlmaster,
        "delete from OVERSCAN; insert into OVERSCAN(TVIN_PORT, TVIN_SIG_FMT, TVIN_TRANS_FMT, hs, he, vs, ve) select TVIN_PORT, TVIN_SIG_FMT, TVIN_TRANS_FMT, hs, he, vs, ve from OVERSCAN_default;");
    if (this->exeSql(sqlmaster)) {
        rval = 0;
    } else {
        SYS_LOGE("%s--SQL error!\n",__FUNCTION__);
        rval = -1;
    }

    return rval;
}

bool CPQdb::PQ_GetPqVersion(String8& ToolVersion, String8& ProjectVersion, String8& GenerateTime)
{
    bool ret = false;
    CSqlite::Cursor c;
    char sqlmaster[256] = {0};

    getSqlParams(__FUNCTION__, sqlmaster,"select ToolVersion,ProjectVersion,GenerateTime from PQ_VersionTable;");

    int rval = this->select(sqlmaster, c);

    if (!rval && c.getCount() > 0) {
        ToolVersion = c.getString(0);
        ProjectVersion = c.getString(1);
        GenerateTime = c.getString(2);
        ret = true;
    }

    return ret;
}

bool CPQdb::PQ_GetDataBaseAttribute(database_attribute_t *DbAttribute)
{
    bool ret = false;
    if (DbAttribute == NULL) {
        SYS_LOGE("%s: DbAttribute is NULL!\n", __FUNCTION__);
    } else {
        CSqlite::Cursor c;
        char sqlmaster[256] = {0};
        bool chipVersionExist = false;
        if (CheckIdExistInDb("ChipVersion", "PQ_VersionTable")) {
            chipVersionExist = true;
            getSqlParams(__FUNCTION__, sqlmaster,
                         "select ToolVersion,ProjectVersion,ChipVersion,dbversion,GenerateTime from PQ_VersionTable;");
        } else {
            chipVersionExist = false;
            getSqlParams(__FUNCTION__, sqlmaster,
                         "select ToolVersion,ProjectVersion,dbversion,GenerateTime from PQ_VersionTable;");
        }

        int rval = this->select(sqlmaster, c);

        if (!rval && c.getCount() > 0) {
            DbAttribute->ToolVersion = c.getString(0);
            DbAttribute->ProjectVersion = c.getString(1);
            if (chipVersionExist) {
                DbAttribute->ChipVersion = c.getString(2);
                DbAttribute->dbversion = c.getString(3);
                DbAttribute->GenerateTime = c.getString(4);
            } else {
                DbAttribute->ChipVersion = String8("");
                DbAttribute->dbversion = c.getString(2);
                DbAttribute->GenerateTime = c.getString(3);
            }

            SYS_LOGD("%s DbAttribute->ToolVersion %s\n", __FUNCTION__, DbAttribute->ToolVersion.c_str());
            SYS_LOGD("%s DbAttribute->ProjectVersion %s\n", __FUNCTION__, DbAttribute->ProjectVersion.c_str());
            SYS_LOGD("%s DbAttribute->dbversion %s\n", __FUNCTION__, DbAttribute->dbversion.c_str());
            SYS_LOGD("%s DbAttribute->ChipVersion %s\n", __FUNCTION__, DbAttribute->ChipVersion.c_str());
            SYS_LOGD("%s DbAttribute->GenerateTime %s\n", __FUNCTION__, DbAttribute->GenerateTime.c_str());

            ret = true;
        } else {
            SYS_LOGE("%s: select action failed!\n", __FUNCTION__);
            ret = false;
        }
    }

    return ret;
}

void CPQdb::PQ_GetPqDbMatchType(database_attribute_t *DbAttribute) {
    /* db ver new format, ex:
    ** ver[][] = {
    **                   ToolVer  ProVer             ChipVer   DbVer      Oem_model   Panel_Index   GeneTime
    **    //old project  xxx      20191113                                                          yyy
    **    //new project1 xxx      20221018           s928x     20221020                             yyy
    **    //new project2 xxx      20221110           t962d4    20221115                             yyy
    ** }
    */

    //old project logic, keep it, judge by ProjectVersion with date 20191113
    if (DbAttribute->ProjectVersion.isEmpty()) {
        mDbMatchType = MATCH_TYPE_NO_DBVERSION;
    } else {
        unsigned int ProVerSelectValue = atoi(DbAttribute->ProjectVersion.c_str());
        SYS_LOGD("%s ProVerSelectValue %d\n", __FUNCTION__, ProVerSelectValue);
        if (ProVerSelectValue == PQ_DB_CODE_MATCH_MASK) {
            mDbMatchType = MATCH_TYPE_MATCH;
        } else if (ProVerSelectValue > PQ_DB_CODE_MATCH_MASK) {
            mDbMatchType = MATCH_TYPE_OLDCODE_NEWDB;
        } else{
            mDbMatchType = MATCH_TYPE_NEWCODE_OLDDB;
        }
    }

    //new project logic, judge by ChipVersion
    std::string chipVer = std::string(DbAttribute->ChipVersion.c_str());
    SYS_LOGD("%s chipVer %s\n", __FUNCTION__, chipVer.c_str());

    if (chipVer == "s928x") {
        SYS_LOGD("%s this project is mbox s5(%s)\n", __FUNCTION__, chipVer.c_str());
        mDbMatchType = MATCH_TYPE_MBOX_S5;
    } else {
        SYS_LOGD("%s this project is others\n", __FUNCTION__);
    }

    SYS_LOGD("db-code match type is %d.\n", mDbMatchType);
    return;
}

int CPQdb::PQ_GetPQModeParams(tv_source_input_t source_input, vpp_picture_mode_t pq_mode,
                                vpp_pq_para_t *params)
{
    CSqlite::Cursor c;
    char sqlmaster[256] = {0};

    int rval = -1;

    getSqlParams(__FUNCTION__, sqlmaster,
                 "select Brightness, Contrast, Saturation, Hue, Sharpness, Backlight, NR from Picture_Mode where "
                 "TVIN_PORT = %d and "
                 "Mode = %d ;", source_input, pq_mode);

    rval = this->select(sqlmaster, c);

    if (c.moveToFirst()) {
        params->brightness = c.getInt(0);
        params->contrast = c.getInt(1);
        params->saturation = c.getInt(2);
        params->hue = c.getInt(3);
        params->sharpness = c.getInt(4);
        params->backlight = c.getInt(5);
        params->nr = c.getInt(6);
    } else {
        SYS_LOGE("%s error!\n",__FUNCTION__);
        rval = -1;
    }
    return rval;
}

int CPQdb::PQ_SetPQModeParams(tv_source_input_t source_input, vpp_picture_mode_t pq_mode, vpp_pq_para_t *params)
{
    int rval;
    char sql[256];

    getSqlParams(__FUNCTION__, sql,
        "update Picture_Mode set Brightness = %d, Contrast = %d, Saturation = %d, Hue = %d, Sharpness = %d, Backlight = %d, NR= %d "
        " where TVIN_PORT = %d and Mode = %d;", params->brightness, params->contrast,
        params->saturation, params->hue, params->sharpness, params->backlight, params->nr,
        source_input, pq_mode);
    if (this->exeSql(sql)) {
        rval = 0;
    } else {
        SYS_LOGE("%s--SQL error!\n",__FUNCTION__);
        rval = -1;
    }
    return rval;
}

int CPQdb::PQ_SetPQModeParamsByName(const char *name, tv_source_input_t source_input,
                                      vpp_picture_mode_t pq_mode, vpp_pq_para_t *params)
{
    int rval;
    char sql[256];

    getSqlParams(__FUNCTION__, sql,
                 "insert into %s(TVIN_PORT, Mode, Brightness, Contrast, Saturation, Hue, Sharpness, Backlight, NR)"
                 " values(%d,%d,%d,%d,%d,%d,%d,%d,%d);", name, source_input, pq_mode,
                 params->brightness, params->contrast, params->saturation, params->hue,
                 params->sharpness, params->backlight, params->nr);

    if (this->exeSql(sql)) {
        rval = 0;
    } else {
        SYS_LOGE("%s--SQL error!\n",__FUNCTION__);
        rval = -1;
    }

    return rval;
}

int CPQdb::PQ_ResetAllPQModeParams(void)
{
    int rval;
    char sqlmaster[256] = {0};

    getSqlParams(
        __FUNCTION__,
        sqlmaster,
        "delete from Picture_Mode; insert into Picture_Mode(TVIN_PORT, Mode, Brightness, Contrast, Saturation, Hue, Sharpness, Backlight, NR) select TVIN_PORT, Mode, Brightness, Contrast, Saturation, Hue, Sharpness, Backlight, NR from picture_mode_default;");

    if (this->exeSql(sqlmaster)) {
        rval = 0;
    } else {
        SYS_LOGE("%s--SQL error!\n",__FUNCTION__);
        rval = -1;
    }

    return rval;
}

int CPQdb::PQ_GetPictureModeParams(pq_src_param_t source_input, vpp_picture_mode_t pq_mode,
                                vpp_pictur_mode_para_t *params)
{
    CSqlite::Cursor c;
    char sqlmaster[256] = {0};

    int rval = -1;

    //for picture mode 5
    String8 TableName = GetPqOsdTableName("GeneralPictureMode5Table", source_input);
    if (TableName.length() != 0) {
        getSqlParams(
            __FUNCTION__,
            sqlmaster,
            "select Type, Value from %s where Mode = %d;", TableName.c_str(), (int)pq_mode);

        rval = this->select(sqlmaster, c);
        char type[50];
        if (c.moveToFirst()) {
            do {
                //if custom want to improve performance,can follow the code of before picture mode 5
                SYS_LOGD("%s type:%s value:%d\n", __FUNCTION__, c.getString(0).c_str(), c.getInt(1));
                memset(type, 0, sizeof(type));
                strncpy(type, c.getString(0).c_str(), sizeof(type) - 1);
                if (!strcmp(type, "Brightness")) {
                    params->Brightness = c.getInt(1);
                } else if (!strcmp(type, "Contrast")) {
                    params->Contrast = c.getInt(1);
                } else if (!strcmp(type, "Saturation")) {
                    params->Saturation = c.getInt(1);
                } else if (!strcmp(type, "Hue")) {
                    params->Hue = c.getInt(1);
                } else if (!strcmp(type, "Sharpness")) {
                    params->Sharpness = c.getInt(1);
                } else if (!strcmp(type, "Backlight")) {
                    params->Backlight = c.getInt(1);
                } else if (!strcmp(type, "NR")) {
                    params->Nr = c.getInt(1);
                } else if (!strcmp(type, "ColorTemperature")) {
                    params->ColorTemperature = c.getInt(1);
                } else if (!strcmp(type, "ColorGamut")) {
                    params->ColorGamut = c.getInt(1);
                } else if (!strcmp(type, "LocalContrast")) {
                    params->LocalContrast = c.getInt(1);
                } else if (!strcmp(type, "DynamicContrast")) {
                    params->DynamicContrast = c.getInt(1);
                } else if (!strcmp(type, "BlackExtension")) {
                    params->BlackStretch = c.getInt(1);
                } else if (!strcmp(type, "BlueStretch")) {
                    params->BlueStretch = c.getInt(1);
                } else if (!strcmp(type, "ChromaCoring")) {
                    params->ChromaCoring = c.getInt(1);
                } else if (!strcmp(type, "MpegNr")) {
                    params->MpegNr = c.getInt(1);
                } else if (!strcmp(type, "DvMode")) {
                    params->DvMode = c.getInt(1);
                } else if (!strcmp(type, "DvDarkDetail")) {
                    params->DvDarkDetail = c.getInt(1);
                } else if (!strcmp(type, "SmoothPlus")) {
                    params->SmoothPlus = c.getInt(1);
                }
            } while (c.moveToNext());
        } else {
            SYS_LOGE("%s select error\n", __FUNCTION__);
            rval = -1;
        }
    } else {
        SYS_LOGE("%s not find %s for source:%d, fmt:%d  pq_mode:%d\n",
            __FUNCTION__, "GeneralPictureMode5Table", source_input.pq_source_input, source_input.pq_sig_fmt, pq_mode);
    }

    return rval;
}

int CPQdb::PQ_GetTconGammaTable(int gamma_curve, gm_tbl_t *gamma_value)
{
    CSqlite::Cursor c;
    int rval;
    char sqlmaster[256] = {0};

    getSqlParams(__FUNCTION__, sqlmaster, "select Red, Green, Blue from TconGAMMA_%d;", gamma_curve);
    rval = this->select(sqlmaster, c);
    int index = 0;

    if (c.moveToFirst()) {
        do {
            for (int i = 0; i < 3; i++) {
                gamma_value->gm_tb[gamma_curve][i].data[index] = c.getInt(i);
            }
            index++;
        } while (c.moveToNext());
    } else {
        SYS_LOGE("%s: select TconGAMMA_%d  error!\n", __FUNCTION__, gamma_curve);
        rval = -1;
    }

    return rval;
}

int CPQdb::PQ_GetGammaSpecialTable(vpp_gamma_curve_t gamma_curve, const char *f_name,
                                     GAMMA_TABLE *gamma_value)
{
    CSqlite::Cursor c;
    char sqlmaster[256] = {0};
    int rval = -1;

    if (gamma_curve == VPP_GAMMA_CURVE_DEFAULT) {
        getSqlParams(__FUNCTION__, sqlmaster, "select %s from GAMMA_ALL", f_name);
    } else {
        getSqlParams(__FUNCTION__, sqlmaster, "select %s from GAMMA_%d", f_name, gamma_curve);
    }

    rval = this->select(sqlmaster, c);
    if (c.moveToFirst()) {
        int index = 0;
        do {
            gamma_value->data[index] = c.getInt(0);
            index++;
        } while (c.moveToNext());
    } else {
        SYS_LOGE("%s, select %s error!\n", __FUNCTION__, f_name);
        rval = -1;
    }
    return rval;
}

int CPQdb::PQ_GetWhiteBalanceGammaSpecialTable(vpp_color_temperature_mode_t mode, const char *f_name, tcon_gamma_table_t *gamma_value)
{
    CSqlite::Cursor c;
    char sqlmaster[256] = {0};
    int rval = -1;

    switch (mode) {
    case VPP_COLOR_TEMPERATURE_MODE_COLD:
        getSqlParams(__FUNCTION__, sqlmaster, "select %s from GAMMA_cool", f_name);
        break;
    case VPP_COLOR_TEMPERATURE_MODE_WARM:
        getSqlParams(__FUNCTION__, sqlmaster, "select %s from GAMMA_warm", f_name);
        break;
    case VPP_COLOR_TEMPERATURE_MODE_USER:
        getSqlParams(__FUNCTION__, sqlmaster, "select %s from GAMMA_user", f_name);
        break;
    case VPP_COLOR_TEMPERATURE_MODE_STANDARD:
        getSqlParams(__FUNCTION__, sqlmaster, "select %s from GAMMA_normal", f_name);
        break;
    default:
        SYS_LOGE("%s: invalid color temperature mode!\n", __FUNCTION__);
        break;
    }

    rval = this->select(sqlmaster, c);
    if (c.moveToFirst()) {
        int index = 0;
        do {
            gamma_value->data[index] = c.getInt(0);
            index++;
        } while (c.moveToNext());
    } else {
        SYS_LOGE("%s, select %s error!\n", __FUNCTION__, f_name);
        rval = -1;
    }
    return rval;
}
									 
int CPQdb::PQ_GetGammaTableR(int panel_id, source_input_param_t source_input_param, tcon_gamma_table_t *gamma_r)
{
    return PQ_GetGammaTable(panel_id, source_input_param, "Red", gamma_r);
}

int CPQdb::PQ_GetGammaTableG(int panel_id, source_input_param_t source_input_param, tcon_gamma_table_t *gamma_g)
{
    return PQ_GetGammaTable(panel_id, source_input_param, "Green", gamma_g);
}

int CPQdb::PQ_GetGammaTableB(int panel_id, source_input_param_t source_input_param, tcon_gamma_table_t *gamma_b)
{
    return PQ_GetGammaTable(panel_id, source_input_param, "Blue", gamma_b);
}

int CPQdb::PQ_GetGammaTable(int panel_id, source_input_param_t source_input_param, const char *f_name, tcon_gamma_table_t *val)
{
    CSqlite::Cursor c;
    char sqlmaster[256] = {0};
    int rval = -1;

    getSqlParams(__FUNCTION__, sqlmaster, "select TableName from GeneralGammaTable where "
                 "TVIN_PORT = %d and "
                 "TVIN_SIG_FMT = %d;", source_input_param.source_input, source_input_param.sig_fmt);

    rval = this->select(sqlmaster, c);
    if (c.moveToFirst()) {
        int index_TableName = 0;
        getSqlParams(__FUNCTION__, sqlmaster, "select %s from %s;", f_name,
                     c.getString(index_TableName).c_str());

        rval = this->select(sqlmaster, c);
        if (c.moveToFirst()) {
            int index = 0;
            do {
                val->data[index] = c.getInt(0);
                index++;
            } while (c.moveToNext());
        }
    }
    return rval;
}

int CPQdb::PQ_GetVGAAdjustPara(tvin_sig_fmt_t vga_fmt, tvafe_vga_parm_t *adjparam)
{
    CSqlite::Cursor c;
    char sqlmaster[256] = {0};
    int rval = -1;

    adjparam->clk_step = 0;
    adjparam->phase = 0;
    adjparam->hpos_step = 0;
    adjparam->vpos_step = 0;
    adjparam->vga_in_clean = 0;

    getSqlParams(
        __FUNCTION__,
        sqlmaster,
        "select Clk, Phase, HPos, VPos, Vga_in_clean from VGA_AutoParams where TVIN_SIG_FMT = %d",
        vga_fmt);

    rval = this->select(sqlmaster, c);

    if (c.getCount() <= 0) {
        vga_fmt = TVIN_SIG_FMT_NULL;
        c.close();
        SYS_LOGE ("%s - Load default", __FUNCTION__);

        getSqlParams(
                    __FUNCTION__,
                    sqlmaster,
                    "select Clk, Phase, HPos, VPos, Vga_in_clean from VGA_AutoParams where TVIN_SIG_FMT = %d",
                    vga_fmt);
        this->select(sqlmaster, c);
    }

    if (c.moveToFirst()) {
        adjparam->clk_step = c.getInt(0);
        adjparam->phase = c.getInt(1);
        adjparam->hpos_step = c.getInt(2);
        adjparam->vpos_step = c.getInt(3);
        adjparam->vga_in_clean = c.getInt(4);
    }
    return rval;
}

int CPQdb::PQ_SetVGAAdjustPara(tvin_sig_fmt_t vga_fmt, tvafe_vga_parm_t adjparam)
{
    CSqlite::Cursor c;
    char sql[256];
    int rval = -1;

    getSqlParams(__FUNCTION__, sql, "select * from VGA_AutoParams where TVIN_SIG_FMT = %d;",
                 vga_fmt);

    rval = this->select(sql, c);

    if (c.getCount() <= 0) {
        vga_fmt = TVIN_SIG_FMT_NULL;
        c.close();
        SYS_LOGE ("%s - Load default", __FUNCTION__);

        getSqlParams(
                    __FUNCTION__,
                    sql,
                    "select * from VGA_AutoParams where TVIN_SIG_FMT = %d;",
                    vga_fmt);
        this->select(sql, c);
    }

    if (c.moveToFirst()) {
        getSqlParams(
            __FUNCTION__,
            sql,
            "Insert into VGA_AutoParams(TVIN_SIG_FMT, Clk, Phase, HPos, VPos, Vga_in_clean) values(%d, %d, %d ,%d ,%d, %d);",
            vga_fmt, adjparam.clk_step, adjparam.phase, adjparam.hpos_step, adjparam.vpos_step,
            adjparam.vga_in_clean);
    } else {
        getSqlParams(
            __FUNCTION__,
            sql,
            "update VGA_AutoParams set Clk = %d, Phase = %d, HPos = %d, VPos = %d, Vga_in_clean = %d where TVIN_SIG_FMT = %d;",
            adjparam.clk_step, adjparam.phase, adjparam.hpos_step, adjparam.vpos_step,
            adjparam.vga_in_clean, vga_fmt);
    }
    if (this->exeSql(sql)) {
        rval = 0;
    } else {
        SYS_LOGE("%s--SQL error!\n",__FUNCTION__);
        rval = -1;
    }

    return rval;
}

String8 CPQdb::GetTableName(const char *GeneralTableName, source_input_param_t source_input_param)
{
    CSqlite::Cursor c;
    char sqlmaster[256] = {0};
    int ret = -1;

    SYS_LOGD("%s: mDbMatchType %d\n", __FUNCTION__, mDbMatchType);

    switch (mDbMatchType) {
    case MATCH_TYPE_NEWCODE_OLDDB:
        SYS_LOGE("%s: new systemcontrol don't match old pq.db!\n", __FUNCTION__);
        break;
    case MATCH_TYPE_OLDCODE_NEWDB:
        SYS_LOGE("%s: old pq.db don't match new systemcontrol!\n", __FUNCTION__);
        break;
    case MATCH_TYPE_MATCH:
    case MATCH_TYPE_NO_DBVERSION:
    default:
        if ((strcmp(GeneralTableName, "GeneralSharpness0FixedTable") == 0)
            || (strcmp(GeneralTableName, "GeneralSharpness0VariableTable") == 0)
            || (strcmp(GeneralTableName, "GeneralSharpness1FixedTable") == 0)
            || (strcmp(GeneralTableName, "GeneralSharpness1VariableTable") == 0)) {
            if (mOutPutType == OUTPUT_TYPE_LVDS) {//TV
                getSqlParams(__FUNCTION__, sqlmaster, "select TableName from %s where "
                             "TVIN_PORT = %d and "
                             "TVIN_SIG_FMT = %d and "
                             "TVIN_TRANS_FMT = %d ;", GeneralTableName, source_input_param.source_input,
                             source_input_param.sig_fmt, source_input_param.trans_fmt);
            } else {//HDMI OUTPUT
                getSqlParams(__FUNCTION__, sqlmaster, "select TableName from %s where "
                             "TVIN_PORT = %d and "
                             "TVIN_SIG_FMT = %d and "
                             "TVIN_TRANS_FMT = %d and "
                             "TVOUT_CVBS = %d ;", GeneralTableName, source_input_param.source_input,
                             source_input_param.sig_fmt, source_input_param.trans_fmt, mOutPutType);
            }
        } else {
            getSqlParams(__FUNCTION__, sqlmaster, "select TableName from %s where "
                         "TVIN_PORT = %d and "
                         "TVIN_SIG_FMT = %d and "
                         "TVIN_TRANS_FMT = %d and "
                         "TVOUT_CVBS = %d ;", GeneralTableName, source_input_param.source_input,
                         source_input_param.sig_fmt, source_input_param.trans_fmt, OUTPUT_TYPE_LVDS);
        }
        ret = 0;
        break;
    case MATCH_TYPE_MBOX_S5:
        if ((strcmp(GeneralTableName, "GeneralSharpness0FixedTable") == 0)
            || (strcmp(GeneralTableName, "GeneralSharpness0VariableTable") == 0)
            || (strcmp(GeneralTableName, "GeneralSharpness1FixedTable") == 0)
            || (strcmp(GeneralTableName, "GeneralSharpness1VariableTable") == 0)
            || (strcmp(GeneralTableName, "GeneralSharpnessPIFixedTable") == 0)
            || (strcmp(GeneralTableName, "GeneralSharpnessPIVariableTable") == 0)
            || (strcmp(GeneralTableName, "GeneralNNSRTable") == 0)) {
            getSqlParams(__FUNCTION__, sqlmaster, "select TableName from %s where "
                         "TVOUT_CVBS = %d ;", GeneralTableName, mOutPutType);
        } else {
            getSqlParams(__FUNCTION__, sqlmaster, "select TableName from %s where "
                         "TVIN_PORT = %d and "
                         "TVIN_SIG_FMT = %d and "
                         "TVIN_TRANS_FMT = %d and "
                         "TVOUT_CVBS = %d ;", GeneralTableName, source_input_param.source_input,
                         source_input_param.sig_fmt, source_input_param.trans_fmt, OUTPUT_TYPE_LVDS);
        }
        ret = 0;
        break;
    }

    if (ret < 0) {
        return String8("");
    } else {
        ret = this->select(sqlmaster, c);
        if (ret == 0) {
            if (c.moveToFirst()) {
                SYS_LOGD("%s table name is %s!\n", __FUNCTION__, c.getString(0).c_str());
                return c.getString(0);
            } else {
                SYS_LOGE("%s %s don't have this table!\n", __FUNCTION__, GeneralTableName);
                return String8("");
            }
        } else {
            SYS_LOGE("%s: select action error!\n", __FUNCTION__);
            return String8("");
        }
    }
}

String8 CPQdb::GetPqOsdTableName(const char *GeneralTableName, pq_src_param_t source_input_param)
{
    CSqlite::Cursor c;
    char sqlmaster[256] = {0};
    int ret = -1;

    getSqlParams(__FUNCTION__, sqlmaster, "select TableName from %s where "
                 "TVIN_PORT = %d and "
                 "TVIN_SIG_FMT = %d and "
                 "TVOUT_CVBS = %d ;", GeneralTableName, source_input_param.pq_source_input,
                 source_input_param.pq_sig_fmt, OUTPUT_TYPE_LVDS);

    ret = this->select(sqlmaster, c);
    if (ret == 0) {
        if (c.moveToFirst()) {
            SYS_LOGD("table name is %s!\n", c.getString(0).c_str());
            return c.getString(0);
        } else {
            SYS_LOGE("%s don't have this table!\n", GeneralTableName);
            return String8("");
        }
    } else {
        SYS_LOGE("%s: select action error!\n", __FUNCTION__);
        return String8("");
    }
}

int CPQdb::CalculateLevelParam(tvpq_data_t *pq_data, int nodes, int level)
{
    int i;

    for (i = 0; i < nodes; i++) {
        if (level < pq_data[i].IndexValue) {
            break;
        }
    }

    if (i == 0) {
        return pq_data[i].RegValue;
    } else if (i == nodes) {
        return pq_data[i - 1].RegValue;
    } else {
        return pq_data[i - 1].RegValue + (level - pq_data[i - 1].IndexValue) * pq_data[i - 1].step;
    }
}

am_regs_t CPQdb::CalculateLevelRegsParam(tvpq_sharpness_regs_t *pq_regs, int level, int sharpness_number)
{
    int i;
    am_regs_t regs;
    memset(&regs, 0, sizeof(am_regs_t));
    int *pq_nodes = NULL;
    if (sharpness_number == 1) {//sharpness1
        pq_nodes = &sha1_nodes;
    } else if (sharpness_number == 0){//sharpness0
        pq_nodes = &sha0_nodes;
    } else if (sharpness_number == 2){//sharpnesspi
        pq_nodes = &sha2_nodes;
    } else {
        SYS_LOGE("%s: sharpness_number invalid!\n", __FUNCTION__);
        return regs;
    }

    for (i = 0; i < *pq_nodes; i++) {
        if (level < pq_regs[i].reg_data[0].IndexValue) {
            break;
        }
    }

    if (i == 0) {
        regs.length = pq_regs[i].length;
        for (int j = 0; j < pq_regs[i].length; j++) {
            regs.am_reg[j].type = pq_regs[i].reg_data[j].Value.type;
            regs.am_reg[j].addr = pq_regs[i].reg_data[j].Value.addr;
            regs.am_reg[j].mask = pq_regs[i].reg_data[j].Value.mask;
            regs.am_reg[j].val = pq_regs[i].reg_data[j].Value.val;
        }

    } else if (i == *pq_nodes) {
        regs.length = pq_regs[i - 1].length;
        for (int j = 0; j < pq_regs[i - 1].length; j++) {
            regs.am_reg[j].type = pq_regs[i - 1].reg_data[j].Value.type;
            regs.am_reg[j].addr = pq_regs[i - 1].reg_data[j].Value.addr;
            regs.am_reg[j].mask = pq_regs[i - 1].reg_data[j].Value.mask;
            regs.am_reg[j].val = pq_regs[i - 1].reg_data[j].Value.val;
        }
    } else {
        regs.length = pq_regs[i - 1].length;
        for (int j = 0; j < pq_regs[i - 1].length; j++) {
            regs.am_reg[j].type = pq_regs[i - 1].reg_data[j].Value.type;
            regs.am_reg[j].addr = pq_regs[i - 1].reg_data[j].Value.addr;
            regs.am_reg[j].mask = pq_regs[i - 1].reg_data[j].Value.mask;
            regs.am_reg[j].val = pq_regs[i - 1].reg_data[j].Value.val + (level
                                 - pq_regs[i - 1].reg_data[j].IndexValue) * pq_regs[i - 1].reg_data[j].step;
            regs.am_reg[j].val &= regs.am_reg[j].mask;
        }
    }

    return MergeSameAddrVal(regs);
}

am_regs_t CPQdb::MergeSameAddrVal(am_regs_t regs)
{
    am_regs_t tmp_reg;
    unsigned int i = 0, k = 0;
    memset(&tmp_reg, 0, sizeof(am_regs_t));

    for (i=0;i<regs.length;i++) {
        if (regs.am_reg[i].addr == regs.am_reg[i+1].addr) {
            regs.am_reg[i+1].mask |=  regs.am_reg[i].mask;
            regs.am_reg[i+1].val |=  regs.am_reg[i].val;
        } else {
            tmp_reg.am_reg[k].type = regs.am_reg[i].type;
            tmp_reg.am_reg[k].addr = regs.am_reg[i].addr;
            tmp_reg.am_reg[k].mask = regs.am_reg[i].mask;
            tmp_reg.am_reg[k].val = regs.am_reg[i].val;
            k++;
        }
    }
    tmp_reg.length = k;

    /*SYS_LOGD("###length = %d\n", tmp_reg.length);
    for (i=0;i<tmp_reg.length;i++) {
        SYS_LOGD("###type: %u, addr: %u, mask: %u, val: %u\n", tmp_reg.am_reg[i].type, tmp_reg.am_reg[i].addr, tmp_reg.am_reg[i].mask, tmp_reg.am_reg[i].val);
    }*/

    return tmp_reg;
}

int CPQdb::GetNonlinearMapping(tvpq_data_type_t data_type, tv_source_input_t source_input, int level,
                                 int *params)
{
    CSqlite::Cursor c;
    char sqlmaster[256] = {0};
    int rval = -1;

    getSqlParams(__FUNCTION__, sqlmaster, "select Value from NonlinearMapping where "
                 "TVIN_PORT = %d and "
                 "Item_ID = %d and "
                 "Level = %d ;", source_input, data_type, level);

    rval = this->select(sqlmaster, c);

    if (c.moveToFirst()) {
        *params = c.getInt(0);
    }
    return rval;
}

int CPQdb::GetNonlinearMappingByOSDFac(tvpq_data_type_t data_type, tv_source_input_t source_input, int *params)
{
    CSqlite::Cursor c;
    char sqlmaster[256] = {0};
    int rval = -1;

    getSqlParams(__FUNCTION__, sqlmaster, "select Value from NonlinearMapping where "
                 "TVIN_PORT = %d and "
                 "Item_ID = %d and ("
                 "Level = 0 or Level = 25 or Level = 50 or Level = 75 or Level = 100);", source_input, data_type);

    rval = this->select(sqlmaster, c);

    if (c.moveToFirst()) {
        params[0] = c.getInt(0);
        params[1] = c.getInt(1);
        params[2] = c.getInt(2);
        params[3] = c.getInt(3);
        params[4] = c.getInt(4);
    }
    return rval;
}

int CPQdb::SetNonlinearMapping(tvpq_data_type_t data_type, tv_source_input_t source_input,
                                 int osd0, int osd25, int osd50, int osd75, int osd100)
{
    int rval;
    char *err = NULL;
    int osdvalue[101];
    double step[4];
    char sql[256];

    step[0] = (osd25 - osd0) / 25.0;
    step[1] = (osd50 - osd25) / 25.0;
    step[2] = (osd75 - osd50) / 25.0;
    step[3] = (osd100 - osd75) / 25.0;

    for (int i = 0; i <= 100; i++) {
        if (i == 0) {
            osdvalue[i] = osd0;
        } else if ((i > 0) && (i <= 25)) {
            osdvalue[i] = osd0 + (int) (i * step[0]);
        } else if ((i > 25) && (i <= 50)) {
            osdvalue[i] = osd25 + (int) ((i - 25) * step[1]);
        } else if ((i > 50) && (i <= 75)) {
            osdvalue[i] = osd50 + (int) ((i - 50) * step[2]);
        } else if ((i > 75) && (i <= 100)) {
            osdvalue[i] = osd75 + (int) ((i - 75) * step[3]);
        }
        getSqlParams(
            __FUNCTION__,
            sql,
            "update NonLinearMapping set Value = %d where TVIN_PORT = %d and Item_ID = %d and Level = %d ;",
            osdvalue[i], source_input, data_type, i);
        if (this->exeSql(sql)) {
            rval = 0;
        } else {
            SYS_LOGE("%s--SQL error!\n",__FUNCTION__);
            rval = -1;
        }
    }

    return rval;
}

int CPQdb::SetNonlinearMappingByName(const char *name, tvpq_data_type_t data_type,
                                       tv_source_input_t source_input, int osd0, int osd25, int osd50, int osd75, int osd100)
{
    int rval;
    char *err = NULL;
    int osdvalue[101];
    double step[4];
    char sql[256];

    step[0] = (osd25 - osd0) / 25.0;
    step[1] = (osd50 - osd25) / 25.0;
    step[2] = (osd75 - osd50) / 25.0;
    step[3] = (osd100 - osd75) / 25.0;

    for (int i = 0; i <= 100; i++) {
        if (i == 0) {
            osdvalue[i] = osd0;
        } else if ((i > 0) && (i <= 25)) {
            osdvalue[i] = osd0 + (int) (i * step[0]);
        } else if ((i > 25) && (i <= 50)) {
            osdvalue[i] = osd25 + (int) ((i - 25) * step[1]);
        } else if ((i > 50) && (i <= 75)) {
            osdvalue[i] = osd50 + (int) ((i - 50) * step[2]);
        } else if ((i > 75) && (i <= 100)) {
            osdvalue[i] = osd75 + (int) ((i - 75) * step[3]);
        }
        memset(sql, '\0', 256);
        getSqlParams(__FUNCTION__, sql,
                     "insert into %s(TVIN_PORT, Item_ID, Level, Value) values(%d,%d,%d,%d);", name,
                     source_input, data_type, i, osdvalue[i]);
        if (this->exeSql(sql)) {
            rval = 0;
        } else {
            SYS_LOGE("%s--SQL error!\n",__FUNCTION__);
            rval = -1;
        }
    }

    return rval;
}

int CPQdb::loadSharpnessData(const char *table_name, int sharpness_number)
{
    CSqlite::Cursor c;
    int rval;
    int *pq_nodes = NULL;
    char sqlmaster[256] = {0};

    getSqlParams(__FUNCTION__, sqlmaster,
                 "select TotalNode, NodeNumber, RegType, RegAddr, RegMask,"
                 "IndexValue, RegValue, StepUp from %s order by NodeNumber asc;",
                 table_name);
    rval = this->select(sqlmaster, c);
    int length = 0;
    int index = 0;

    if (sharpness_number == 1) {//for Sharpness_1
        pq_nodes = &sha1_nodes;
        if (c.moveToFirst()) {
            *pq_nodes = c.getInt(0);
            if (*pq_nodes != 0) {
                length = c.getCount() / (*pq_nodes);
            }
            for (int i = 0; i < *pq_nodes; i++) {
                pq_sharpness1_reg_data[i].length = length;
            }
            do {
                if (length == 0) {
                    rval = -1;
                    break;
                }
                pq_sharpness1_reg_data[index / length].reg_data[index % length].TotalNode
                    = c.getInt(0);
                pq_sharpness1_reg_data[index / length].reg_data[index % length].NodeValue
                    = c.getInt(1);
                pq_sharpness1_reg_data[index / length].reg_data[index % length].Value.type
                    = c.getUInt(2);
                pq_sharpness1_reg_data[index / length].reg_data[index % length].Value.addr
                    = c.getUInt(3);
                pq_sharpness1_reg_data[index / length].reg_data[index % length].Value.mask
                    = c.getUInt(4);
                pq_sharpness1_reg_data[index / length].reg_data[index % length].IndexValue
                    = c.getInt(5);
                pq_sharpness1_reg_data[index / length].reg_data[index % length].Value.val
                    = c.getUInt(6);
                pq_sharpness1_reg_data[index / length].reg_data[index % length].step = c.getF(7);
                index++;
            } while (c.moveToNext());
        }else {
            SYS_LOGE("%s: select sharpness1 value error!\n", __FUNCTION__);
            rval = -1;
        }
    } else if (sharpness_number == 0) {//for Sharpness_0
        pq_nodes = &sha0_nodes;
        if (c.moveToFirst()) {
            *pq_nodes = c.getInt(0);//TotalNode?
            if (*pq_nodes != 0) {
                length = c.getCount() / (*pq_nodes);
            }
            for (int i = 0; i < *pq_nodes; i++) {
                pq_sharpness0_reg_data[i].length = length;
            }
            do {
                if (length == 0) {
                    rval = -1;
                    break;
                }
                pq_sharpness0_reg_data[index / length].reg_data[index % length].TotalNode
                    = c.getInt(0);
                pq_sharpness0_reg_data[index / length].reg_data[index % length].NodeValue
                    = c.getInt(1);
                pq_sharpness0_reg_data[index / length].reg_data[index % length].Value.type
                    = c.getUInt(2);
                pq_sharpness0_reg_data[index / length].reg_data[index % length].Value.addr
                    = c.getUInt(3);
                pq_sharpness0_reg_data[index / length].reg_data[index % length].Value.mask
                    = c.getUInt(4);
                pq_sharpness0_reg_data[index / length].reg_data[index % length].IndexValue
                    = c.getInt(5);
                pq_sharpness0_reg_data[index / length].reg_data[index % length].Value.val
                    = c.getUInt(6);
                pq_sharpness0_reg_data[index / length].reg_data[index % length].step = c.getF(7);
                index++;
            } while (c.moveToNext());
        }else {
            SYS_LOGE("%s: select sharpness0 value error!\n", __FUNCTION__);
            rval = -1;
        }
    } else if (sharpness_number == 2) {//for Sharpness_pi
        pq_nodes = &sha2_nodes;
        if (c.moveToFirst()) {
            *pq_nodes = c.getInt(0);//TotalNode?
            if (*pq_nodes != 0) {
                length = c.getCount() / (*pq_nodes);
            }
            for (int i = 0; i < *pq_nodes; i++) {
                pq_sharpnesspi_reg_data[i].length = length;
            }
            do {
                if (length == 0) {
                    rval = -1;
                    break;
                }
                pq_sharpnesspi_reg_data[index / length].reg_data[index % length].TotalNode
                    = c.getInt(0);
                pq_sharpnesspi_reg_data[index / length].reg_data[index % length].NodeValue
                    = c.getInt(1);
                pq_sharpnesspi_reg_data[index / length].reg_data[index % length].Value.type
                    = c.getUInt(2);
                pq_sharpnesspi_reg_data[index / length].reg_data[index % length].Value.addr
                    = c.getUInt(3);
                pq_sharpnesspi_reg_data[index / length].reg_data[index % length].Value.mask
                    = c.getUInt(4);
                pq_sharpnesspi_reg_data[index / length].reg_data[index % length].IndexValue
                    = c.getInt(5);
                pq_sharpnesspi_reg_data[index / length].reg_data[index % length].Value.val
                    = c.getUInt(6);
                pq_sharpnesspi_reg_data[index / length].reg_data[index % length].step = c.getF(7);
                index++;
            } while (c.moveToNext());
        }else {
            SYS_LOGE("%s: select sharpnesspi value error!\n", __FUNCTION__);
            rval = -1;
        }
    } else {
        SYS_LOGE("%s: sharpness_number invalid!\n", __FUNCTION__);
        rval = -1;
    }

    return rval;
}

int CPQdb::LoadVppBasicParam(tvpq_data_type_t data_type, source_input_param_t source_input_param)
{
    CSqlite::Cursor c;
    int rval = -1;
    char sqlmaster[256] = {0};
    char table_name[128];
    tvpq_data_t *pq_data = NULL;
    int *pq_nodes = NULL;
    String8 tableName[] = {String8("GeneralBrightnessTable"), String8("GeneralContrastTable"),
                           String8("GeneralSaturationTable"), String8("GeneralHueTable")};

    switch (data_type) {
    case TVPQ_DATA_BRIGHTNESS:
        pq_data = pq_bri_data;
        pq_nodes = &bri_nodes;
        break;
    case TVPQ_DATA_CONTRAST:
        pq_data = pq_con_data;
        pq_nodes = &con_nodes;
        break;
    case TVPQ_DATA_SATURATION:
        pq_data = pq_sat_data;
        pq_nodes = &sat_nodes;
        break;
    case TVPQ_DATA_HUE:
        pq_data = pq_hue_data;
        pq_nodes = &hue_nodes;
        break;
    default:
        return rval;
    }

    if (CheckHdrStatus(tableName[data_type]))
        source_input_param.sig_fmt = TVIN_SIG_FMT_HDMI_HDR;

    String8 TableName = GetTableName(tableName[data_type].c_str(), source_input_param);
    if ((TableName.c_str() != NULL) && (TableName.length() != 0) ) {
        getSqlParams(
            __FUNCTION__,
            sqlmaster,
            "select TotalNode, NodeNumber, IndexValue, RegValue, StepUp from %s order by NodeNumber asc;",
            TableName.c_str());

        rval = this->select(sqlmaster, c);
        if (c.moveToFirst()) {
            int index = 0;
            do {
                pq_data[index].TotalNode = c.getInt(0);
                pq_data[index].NodeValue = c.getInt(1);
                pq_data[index].IndexValue = c.getInt(2);
                pq_data[index].RegValue = c.getInt(3);
                pq_data[index].step = c.getF(4);
                index++;
            } while (c.moveToNext());
            *pq_nodes = index;
        }else {
            SYS_LOGE("%s: select value error!\n", __FUNCTION__);
        }
    } else {
        SYS_LOGE("%s: %s don't have this table!\n", __FUNCTION__, tableName[data_type].c_str());
    }
    return rval;
}

const char *Pmode_name[6] = { "Picture_Mode", "Picture_Mode_Default", "NonlinearMapping",
                              "NonlinearMapping_Default", "VGA_AutoParams", "OVERSCAN"
                            };

void CPQdb::initialTable(int type)
{
    vpp_pq_para_t pmode_default;

    pmode_default.backlight = 100;
    pmode_default.brightness = 50;
    pmode_default.contrast = 50;
    pmode_default.hue = 50;
    pmode_default.nr = 0;
    pmode_default.saturation = 50;
    pmode_default.sharpness = 50;

    switch (type) {
    case TYPE_PMode:
    case TYPE_PMode_Default:
        for (int i = 0; i < 6; i++) {
            for (int j = 0; j < 4; j++) {
                PQ_SetPQModeParamsByName(Pmode_name[type], (tv_source_input_t) i,
                                         (vpp_picture_mode_t) j, &pmode_default);
            }
        }
        break;
    case TYPE_Nonlinear:
    case TYPE_NonLinear_Default:
        for (int i = 0; i < 6; i++) {
            for (int j = 0; j < 5; j++) {
                SetNonlinearMappingByName(Pmode_name[type], (tvpq_data_type_t) j,
                                          (tv_source_input_t) i, 0, (int) 255 / 4.0, (int) 255 * 2 / 4.0,
                                          (int) 255 * 3 / 4.0, 255);
            }
        }
        break;
    case TYPE_VGA_AUTO:
        break;
    case TYPE_OVERSCAN:
        break;
    }
}

int CPQdb::PQ_GetPhaseArray(am_phase_t *am_phase)
{
    CSqlite::Cursor c;
    int iOutRet = 0;
    char sqlmaster[256] = {0};
    getSqlParams(__FUNCTION__, sqlmaster, "select Phase from Phase order by Format ASC; ");

    this->select(sqlmaster, c);
    int nums = 0;
    am_phase->length = c.getCount();
    if (c.moveToFirst()) {
        do {
            am_phase->phase[nums] = c.getInt(0);
            nums++;
        } while (c.moveToNext());
    }

    return nums;
}

bool CPQdb::PQ_GetLDIM_Regs(vpu_ldim_param_s *vpu_ldim_param)
{
    CSqlite::Cursor c;
    bool ret = true;
    int i = 0;
    int ldimMemsSize = sizeof (vpu_ldim_param_s) / sizeof (int);

    SYS_LOGD ("%s, entering...\n", __FUNCTION__);
    SYS_LOGD ("ldimMemsSize = %d\n", ldimMemsSize);

    if (vpu_ldim_param != NULL) {
        int* temp = reinterpret_cast<int*>(vpu_ldim_param);

        if (this->select("select value from LDIM_1; ", c) != -1 ) {
            int retNums = c.getCount();

            SYS_LOGD ("retNums = %d\n", retNums);

            if ( retNums > 0 && retNums == ldimMemsSize ) {
                do {
                    temp[i] = c.getUInt(0);
                    SYS_LOGD ("%d - %d\n", i + 1, temp[i]);

                    i++;
                } while (c.moveToNext());
            } else {
                SYS_LOGV ("DataBase not match vpu_ldim_param_s\n");
                ret = false;
            }
        }
        else {
            SYS_LOGV ("select value from LDIM_1; failure\n");
            ret = false;
        }
    }

    return ret;
}

int CPQdb::PQ_GetLocalDimmingParams(int level, source_input_param_t source_input_param, aml_ldim_pq_s *newParams)
{
    CSqlite::Cursor c;
    char sqlmaster[256] = {0};
    char buf[512];
    char *buffer = NULL;
    char *aa = NULL;
    char *aa_save[100];
    unsigned int index = 0;
    int rval = -1;

    memset(newParams, 0, sizeof(aml_ldim_pq_s));

    String8 TableName = GetTableName("GeneralLocalDimmingTable", source_input_param);
    if ((TableName.c_str() != NULL) && (TableName.length() != 0) ) {
        {// for param
            index = 0;
            getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                        "regnum < %d and Level = %d",
                        TableName.c_str(), LD_bl_remap_curve, level);
            rval = this->select(sqlmaster, c);

            if (c.moveToFirst()) {
                newParams->func_en                      = c.getInt(0);
                newParams->remapping_en                 = c.getInt(1);
                newParams->fw_sel                       = c.getInt(2);
                newParams->ldc_hist_mode                = c.getInt(3);
                newParams->ldc_hist_blend_mode          = c.getInt(4);
                newParams->ldc_hist_blend_alpha         = c.getInt(5);
                newParams->ldc_hist_adap_blend_max_gain = c.getInt(6);
                newParams->ldc_hist_adap_blend_diff_th1 = c.getInt(7);
                newParams->ldc_hist_adap_blend_diff_th2 = c.getInt(8);
                newParams->ldc_hist_adap_blend_th0      = c.getInt(9);
                newParams->ldc_hist_adap_blend_thn      = c.getInt(10);
                newParams->ldc_hist_adap_blend_gain_0   = c.getInt(11);
                newParams->ldc_hist_adap_blend_gain_1   = c.getInt(12);
                newParams->ldc_init_bl_min              = c.getInt(13);
                newParams->ldc_init_bl_max              = c.getInt(14);
                newParams->ldc_sf_mode                  = c.getInt(15);
                newParams->ldc_sf_gain_up               = c.getInt(16);
                newParams->ldc_sf_gain_dn               = c.getInt(17);
                newParams->ldc_sf_tsf_3x3               = c.getInt(18);
                newParams->ldc_sf_tsf_5x5               = c.getInt(19);
                newParams->ldc_bs_bl_mode               = c.getInt(20);
                newParams->ldc_bs_glb_apl_gain          = c.getInt(21);
                newParams->ldc_bs_dark_scene_bl_th      = c.getInt(22);
                newParams->ldc_bs_gain                  = c.getInt(23);
                newParams->ldc_bs_limit_gain            = c.getInt(24);
                newParams->ldc_bs_loc_apl_gain          = c.getInt(25);
                newParams->ldc_bs_loc_max_min_gain      = c.getInt(26);
                newParams->ldc_bs_loc_dark_scene_bl_th  = c.getInt(27);
                newParams->ldc_tf_en                    = c.getInt(28);
                newParams->ldc_tf_low_alpha             = c.getInt(29);
                newParams->ldc_tf_high_alpha            = c.getInt(30);
                newParams->ldc_tf_low_alpha_sc          = c.getInt(31);
                newParams->ldc_tf_high_alpha_sc         = c.getInt(32);
                newParams->ldc_dimming_curve_en         = c.getInt(33);
                newParams->ldc_sc_hist_diff_th          = c.getInt(34);
                newParams->ldc_sc_apl_diff_th           = c.getInt(35);
                newParams->ldc_bl_buf_diff              = c.getInt(36);
                newParams->ldc_glb_gain                 = c.getInt(37);
                newParams->ldc_dth_en                   = c.getInt(38);
                newParams->ldc_dth_bw                   = c.getInt(39);
            }else {
                SYS_LOGE("%s, read LocalDimming Params fail\n", __FUNCTION__);
            }
        }

        //bl_remap_curve
        {
            index = 0;
            aa = NULL;
            getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                        "regnum = %d and level = %d",
                        TableName.c_str(), LD_bl_remap_curve, level);

            rval |= this->select(sqlmaster, c);
            memset(buf, 0, sizeof(buf));
            if (strlen(c.getString(index).c_str()) < sizeof(buf)/sizeof(char)) {
                strcpy(buf, c.getString(index).c_str());
            }
            //SYS_LOGD ("%s - bl_remap_curve is %s\n", __FUNCTION__, buf);
            buffer = buf;
            while ((aa_save[index] = strtok_r(buffer, " ", &aa)) != NULL) {
                newParams->bl_remap_curve[index] = atoi(aa_save[index]);
                index ++;
                if (index >= sizeof(newParams->bl_remap_curve)/sizeof(unsigned int)) {
                    break;
                }
                buffer = NULL;
            }
        }

        //ldc_gain_lut
        {
            int i = 0;
            int lut_id = LD_remap_LUT_0;
            for (i = 0; i < 16; i++) {
                index = 0;
                aa = NULL;
                lut_id = LD_remap_LUT_0 + i;
                getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                            "regnum = %d and level = %d",
                            TableName.c_str(), lut_id, level);

                rval |= this->select(sqlmaster, c);
                memset(buf, 0, sizeof(buf));
                if (strlen(c.getString(index).c_str()) < sizeof(buf)/sizeof(char)) {
                    strcpy(buf, c.getString(index).c_str());
                }
                //SYS_LOGD ("%s - ldc_gain_lut[%d] is %s\n", __FUNCTION__, i, buf);
                buffer = buf;
                while ((aa_save[index] = strtok_r(buffer, " ", &aa)) != NULL) {
                    if (index > 0) {
                        newParams->ldc_gain_lut[i][index - 1] = atoi(aa_save[index]);
                    }
                    index++;
                    if (index > sizeof(newParams->ldc_gain_lut[0])/sizeof(unsigned int)) {
                        break;
                    }
                    buffer = NULL;
                }
            }
        }

        //ldc_min_gain_lut
        {
            index = 0;
            aa = NULL;
            getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                        "regnum = %d and level = %d",
                        TableName.c_str(), LD_min_gain_lut, level);

            rval |= this->select(sqlmaster, c);
            memset(buf, 0, sizeof(buf));
            if (strlen(c.getString(index).c_str()) < sizeof(buf)/sizeof(char)) {
                strcpy(buf, c.getString(index).c_str());
            }
            //SYS_LOGD ("%s - ldc_min_gain_lut is %s\n", __FUNCTION__, buf);
            buffer = buf;
            while ((aa_save[index] = strtok_r(buffer, " ", &aa)) != NULL) {
                newParams->ldc_min_gain_lut[index] = atoi(aa_save[index]);
                index ++;
                if (index >= sizeof(newParams->ldc_min_gain_lut)/sizeof(unsigned int)) {
                    break;
                }
                buffer = NULL;
            }
        }

        //ldc_dither_lut
        {
            int i = 0, j = 0;
            index = 0;
            aa = NULL;
            char sqlmasterext[1024];
            char bufext[1024];
            getSqlParams(__FUNCTION__, sqlmasterext, "select value from %s where "
                        "regnum = %d and level = %d",
                        TableName.c_str(), LD_dither_lut, level);

            rval |= this->select(sqlmasterext, c);
            memset(bufext, 0, sizeof(bufext));
            if (strlen(c.getString(index).c_str()) < sizeof(bufext)/sizeof(char)) {
                strcpy(bufext, c.getString(index).c_str());
            }
            //SYS_LOGD ("%s - ldc_dither_lut is %s\n", __FUNCTION__, bufext);
            buffer = bufext;

            for (i = 0; i < 32; i++) {
                for (j = 0; j < 16; j++) {
                    if ((aa_save[j] = strtok_r(buffer, " ", &aa)) != NULL) {
                        newParams->ldc_dither_lut[i][j] = atoi(aa_save[j]);
                        //SYS_LOGD ("%s - ldc_dither_lut[%d][%d] is %d\n", __FUNCTION__, i, j, newParams->ldc_dither_lut[i][j]);
                    }
                    buffer = NULL;
                }
            }
        }
    } else {
        SYS_LOGE("GeneralLocalDimmingTable select error!!\n");
    }
    return rval;
}

bool CPQdb::CheckHdrStatus(const char *tableName)
{
    bool ret = false;
    char sqlmaster[256] = {0};
    CSqlite::Cursor tempCursor;

    if (mHdrStatus) {
        getSqlParams(__FUNCTION__, sqlmaster,
                     "select TableName from %s where TVIN_SIG_FMT = %d;",
                     tableName, TVIN_SIG_FMT_HDMI_HDR);

        this->select(sqlmaster, tempCursor);
        if (tempCursor.getCount() > 0) {
            ret = true;
        }
    }

    return ret;
}

int CPQdb::GetFileAttrIntValue(const char *fp, int flag)
{
    int fd = -1, ret = -1;
    int temp = -1;
    char temp_str[32];

    memset(temp_str, 0, 32);

    fd = open(fp, flag);

    if (fd < 0) {
        SYS_LOGE("open %s ERROR(%s)!!\n", fp, strerror(errno));
        return -1;
    }

    ret = read(fd, temp_str, sizeof(temp_str));
    if (ret > 0) {
        if (sscanf(temp_str, "%d", &temp) >= 0) {
            SYS_LOGD("get %s value =%d!\n", fp, temp);
            close(fd);
            return temp;
        } else {
            SYS_LOGE("get %s value error(%s)\n", fp, strerror(errno));
            close(fd);
            return -1;
        }
    }

    close(fd);
    return -1;
}

bool CPQdb::CheckCVBSParamValidStatus()
{
    bool ret = false;
    char sqlmaster[256] = {0};
    CSqlite::Cursor tempCursor;

    if (CheckIdExistInDb(CVBS_NAME_ID, "GeneralNR2Table")) {
        getSqlParams(__FUNCTION__, sqlmaster, "select %s from GeneralNR2Table where "
                     "TVIN_PORT = %d and "
                     "TVIN_SIG_FMT = %d and "
                     "TVIN_TRANS_FMT = %d and "
                     "TVOUT_CVBS = %d;", TABLE_NAME_ID, SOURCE_MPEG,
                     TVIN_SIG_FMT_HDMI_1920X1080P_60HZ, TVIN_TFMT_2D, 1);
        if ((this->select(sqlmaster, tempCursor) == 0) && (tempCursor.moveToFirst())) {
            ret = true;
        } else {
            SYS_LOGE("%s: new db, but don't have cvbs param!\n", __FUNCTION__);
            ret = false;
        }
    } else {
        SYS_LOGE("%s: old db, don't have cvbs param!\n", __FUNCTION__);
        ret = false;
    }
    return ret;
}

bool CPQdb::CheckIdExistInDb(const char *Id, const char *TableName)
{
    bool ret = false;
    char sqlmaster[256] = {0};
    CSqlite::Cursor tempCursor;

    getSqlParams(__FUNCTION__, sqlmaster,
                 "select sql from sqlite_master where type = 'table' and tbl_name = '%s';", TableName);

    int retVal = this->select(sqlmaster, tempCursor);
    if ((retVal == 0) && (tempCursor.moveToFirst())) {
        if (strstr(tempCursor.getString(0).c_str(), Id) != NULL) {
            ret = true;
        } else {
            ret = false;
        }
    } else {
        SYS_LOGE("%s: error!\n", __FUNCTION__);
        ret = false;
    }

    /*if (ret) {
        SYS_LOGE("%s: %s exist in %s!\n", __FUNCTION__, Id, TableName);
    } else {
        SYS_LOGE("%s: %s don't exist in %s!\n", __FUNCTION__, Id, TableName);
    }*/

    return ret;
}

int CPQdb::PQ_GetAiSrParams(aisr_mode_e mode, source_input_param_t source_input_param, am_regs_t *regs)
{
    int rval = -1;

    String8 TableName = GetTableName("GeneralNNSRTable", source_input_param);

    if ((TableName.c_str() != NULL) && (TableName.length() != 0) ) {
        rval = getRegValuesByValue(TableName.c_str(), LEVEL_NAME, "", (int)mode, 0, regs);
        am_reg_t tmp_buf[regs->length];
        for (unsigned int i = 0; i < regs->length; i++) {
              tmp_buf[i].addr = regs->am_reg[i].addr;
              tmp_buf[i].mask = regs->am_reg[i].mask;
              tmp_buf[i].type = regs->am_reg[i].type;
              tmp_buf[i].val  = regs->am_reg[i].val;
        }

    } else {
        SYS_LOGE("%s GeneralNNSRTable don't have table!!\n", __FUNCTION__);
    }

    return rval;
}

