/*
    * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
    *
    * This source code is subject to the terms and conditions defined in the
    * file 'LICENSE' which is part of this source code package.
    *
    * Description: header file
    */

#ifndef _C_TV_PANEL_H_
#define _C_TV_PANEL_H_
/*
#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif
*/
#define PANEL_BUFFER_MAX_LEN            128
#define P2P_LUT_FILE_PATH               "/mnt/vendor/odm_ext/etc/tvconfig/panel/p2p_lut.bin"    //Address to save file of Table_data
#define P2P_SET_FILE_PATH               "/mnt/vendor/odm_ext/etc/tvconfig/panel/p2p_reg.bin"    //Address to save file of Register
#define P2P_PGAMMA_PATH                 "/mnt/vendor/odm_ext/etc/tvconfig/panel/p_gamma.bin"
#define P2P_ACC_FILE_PATH               "/mnt/vendor/odm_ext/etc/tvconfig/panel/p2p_acc.bin"    //Address to save file of ACC

#define CRC_CHECK_PATH                  "/mnt/vendor/odm_ext/etc/tvconfig/panel/crc_check.bin"
#define P_GAMMA_DEFAULT_PATH_65D02_3    "/mnt/vendor/odm_ext/etc/tvconfig/panel/tcon_p_gamma.bin"    //Path to save p_Gamma default data bin file of 65D02-3
#define P_GAMMA_DEFAULT_PATH_65D02_H    "/mnt/vendor/odm_ext/etc/tvconfig/panel/tcon_p_gamma.bin"    //Path to save p_Gamma default data bin file of 65D02-H
#define P_GAMMA_DEFAULT_PATH_55D09      "/mnt/vendor/odm_ext/etc/tvconfig/panel/tcon_p_gamma.bin"    //Path to save p_Gamma default data bin file of 55D09
#define P_GAMMA_DEFAULT_PATH_50T01      "/mnt/vendor/odm_ext/etc/tvconfig/panel/tcon_p_gamma.bin"    //Path to save p_Gamma default data bin file of 55D09

#define DVR_I2C_dev                     "/dev/i2c-1"
#define DVR_ADDR                        0x4f
#define UBOOTENV_MODEL_TCON_GAMMA       "ubootenv.var.model_tcon_ext_b0"

class CTvPanel {
public:
    CTvPanel();
    ~CTvPanel();

    int AM_PANEL_Init();
    int PANEL_Init();
    int PANEL_DeInit();

    //panel tcon bin path
    int PANEL_SetTconBinPath(const char* path);

    //panel file path
    int PANEL_SetFilePath(const char* path);

    //panel info
    int PANEL_GetIniPath(char* panel_ini_path);

    int PANEL_43D10_data_Conversion(unsigned int offset,const char *partition_name_Table_data,const char *partition_name_Register);
    int PANEL_65D02_H_data_Conversion(const char *partition_name_Table_data,const char *partition_name_Register);
    int PANEL_50T01_data_Conversion(const char *partition_name_Table_data,const char *partition_name_Register);
    int PANEL_65D02_3_data_Conversion(const char *partition_name_Table_data,const char *partition_name_Register);
    int PANEL_50INX_data_Conversion(const char *partition_name_Table_data,const char *partition_name_Register);
    int PANEL_55SDC_data_Conversion(const char *partition_name_Table_data,const char *partition_name_Register);
    int PANEL_55SDC_Acc_Conversion(const char *partition_name_Table_acc);
    int PANEL_SPI_PGAMMA(int spi_offset, int spi_len ,const char *spi_bin_path);
    int PANEL_SPI_PGAMMA_SPEC(int spi_offset, int spi_len ,const char *spi_bin_path, const char *default_bin_path);
    int PANEL_SPI_PGAMMA_SPEC_65D02_3(int spi_offset, int spi_len ,const char *spi_bin_path, const char *default_bin_path);
    int PANEL_Decode_50inch(int *Code,int *Mode,int range,int min,int *DeLut);
    int PANEL_SIZE_CHOICE();
    int PANEL_REMOVE_BIN_FILE(int config);
    int PANEL_CreatSdcCheckBin();
    int PANEL_BIN_FILE_CEHCK(int address_1,int address_2, int address_3, int address_4, int len_1, int len_2,int len_3, int len_4);
    unsigned short PANEL_50T01_DEMURA_CRC(unsigned char *buf, int length);
    unsigned short PANEL_65D02_H_DEMURA_CRC(unsigned char *buffer, unsigned int data_size);
    unsigned char PANEL_50INX_DEMURA_CRC(unsigned char *data, unsigned char *const crc_out, int crcStart, int crcLength);
    unsigned short PANEL_43_CRC_Check(unsigned int *addr, int num);
    unsigned short CalcCRC16(unsigned char *dataBuffer,unsigned long len);
    int PANEL_AutoFlicker(void);
    int PANEL_Flash_Read(const char *partition_name, long offset,unsigned char *buffer, unsigned int length);
    int PANEL_Flash_Write(const char *partition_name, long offset,unsigned char *buffer, unsigned int length) ;
};
/*
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
*/

#endif
