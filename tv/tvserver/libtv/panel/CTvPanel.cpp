/*
    * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
    *
    * This source code is subject to the terms and conditions defined in the
    * file 'LICENSE' which is part of this source code package.
    *
    * Description: c++ file
    */

#define LOG_NDEBUG 0

#define LOG_TAG "tvserver"
#define LOG_TV_TAG "CTvPanel"

#include <CTvLog.h>
#include <stdlib.h>
#include <string.h>
#include <tvutils.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>

#include "CTvPanel.h"

CTvPanel::CTvPanel()
{

}
CTvPanel::~CTvPanel()
{

}
int CTvPanel::PANEL_Flash_Read(const char *partition_name, long offset,unsigned char *buffer, unsigned int length) {
    return tvFlashRead(partition_name, offset, buffer, length);
}

int CTvPanel::PANEL_Flash_Write(const char *partition_name, long offset,unsigned char *buffer, unsigned int length) {
    return tvFlashWrite(partition_name, offset, buffer, length);
}

int CTvPanel::AM_PANEL_Init()
{
    LOGD("%s, AM_PANEL_Init entering\n",__FUNCTION__);
    PANEL_SIZE_CHOICE();
    return 0;
}

int CTvPanel::PANEL_SIZE_CHOICE(){
    //get ini path from unikey
    unsigned char key_data[1024] = {0};
    int ret = readUKeyData(CS_PANEL_INI_SPI_INFO_NAME, key_data, 1024);

    if (ret < 0) {
        LOGE("%s,readUKeyData failed!\n", __FUNCTION__);
        return -1;
    } else {
        LOGD("%s,readUKeyData successed!\n", __FUNCTION__);
    }

    int demura_offset = (int) ((key_data[19]<<24)+(key_data[18]<<16)+(key_data[17]<<8)+key_data[16]);
    int demura_size = (int) ((key_data[23]<<24)+(key_data[22]<<16)+(key_data[21]<<8)+key_data[20]);
    int p_gamma_offset = (int) ((key_data[51]<<24)+(key_data[50]<<16)+(key_data[49]<<8)+key_data[48]);
    int p_gamma_size = (int) ((key_data[55]<<24)+(key_data[54]<<16)+(key_data[53]<<8)+key_data[52]);
    int p_gamma_param_0 = (int) ((key_data[59]<<24)+(key_data[58]<<16)+(key_data[57]<<8)+key_data[56]);
    LOGD("%s, demura_offset = 0x%x,demura_set = 0x%x,p_gamma_offset = 0x%x,p_gamma_size = 0x%x,p_gamma_param_0 = 0x%x\n", __FUNCTION__,demura_offset,demura_size,p_gamma_offset,p_gamma_size,p_gamma_param_0);

    int spi_ret = -1;
    if (demura_offset == 0x0 && demura_size == 0x9b9d0) {
        LOGD("%s, do 43D10 demura data conversion\n", __FUNCTION__);
        spi_ret = PANEL_43D10_data_Conversion(demura_offset, (const char*)P2P_LUT_FILE_PATH, (const char*)P2P_SET_FILE_PATH);
        if (spi_ret == 0) {
            LOGD("%s, do 43D10 demura data conversion successed!\n", __FUNCTION__);
        } else {
            LOGE("%s, do 43D10 demura data conversion filed!\n", __FUNCTION__);
        }
    }else if (demura_offset == 0x200 && demura_size == 0x9b9d0) {
        LOGD("%s, do 65D02 demura data conversion\n", __FUNCTION__);
        spi_ret = PANEL_65D02_H_data_Conversion((const char*)P2P_LUT_FILE_PATH, (const char*)P2P_SET_FILE_PATH);
        if (spi_ret == 0) {
            LOGD("%s, do 65D02 demura data conversion successed!\n", __FUNCTION__);
        } else {
            LOGE("%s, do 65D02 demura data conversion filed!\n", __FUNCTION__);
        }
    }else if (demura_offset == 0x2491 && demura_size == 0x74720) {
        LOGD("%s, do 50 demura data conversion\n", __FUNCTION__);
        spi_ret = PANEL_50INX_data_Conversion((const char*)P2P_LUT_FILE_PATH, (const char*)P2P_SET_FILE_PATH);
        if (spi_ret == 0) {
            LOGD("%s, do 50 demura data conversion successed!\n", __FUNCTION__);
        } else {
            LOGE("%s, do 50 demura data conversion filed!\n", __FUNCTION__);
        }
    } else if (demura_offset == 0x110 && demura_size == 0x9b9d0) {
        LOGD("%s, do 65D02-3 demura data conversion\n", __FUNCTION__);
        spi_ret = PANEL_65D02_3_data_Conversion((const char*)P2P_LUT_FILE_PATH, (const char*)P2P_SET_FILE_PATH);
        if (spi_ret == 0) {
            LOGD("%s, do 65D02-3 demura data conversion successed!\n", __FUNCTION__);
        } else {
            LOGE("%s, do 65D02-3 demura data conversion filed!\n", __FUNCTION__);
        }
    } else if (demura_offset == 0x42 && demura_size == 0x8F360) {
        LOGD("%s, do 50T01 demura data conversion\n", __FUNCTION__);
        spi_ret = PANEL_50T01_data_Conversion((const char*)P2P_LUT_FILE_PATH, (const char*)P2P_SET_FILE_PATH);
        if (spi_ret == 0) {
            LOGD("%s, do 50T01 demura data conversion successed!\n", __FUNCTION__);
        } else {
            LOGE("%s, do 50T01 demura data conversion filed!\n", __FUNCTION__);
        }
    } else if (demura_offset == 0x51000 && demura_size == 0xdec50) {
        LOGD("%s, do 55usit demura data conversion ,acc loading and auto flicker!\n", __FUNCTION__);

        int crc_ret = PANEL_BIN_FILE_CEHCK(0x10004, 0x12fc50, 0x1FB800, 0, 2, 1, 1, 0);
        if (crc_ret == 0) {
            LOGD("%s,   data is for this plane,do not need rebuild demura bin file!\n", __FUNCTION__);
            //return 0;
        } else {
            PANEL_55SDC_Acc_Conversion((const char*)P2P_ACC_FILE_PATH);
            PANEL_55SDC_data_Conversion((const char*)P2P_LUT_FILE_PATH, (const char*)P2P_SET_FILE_PATH);
            LOGD("%s,   data is not for this plane, rebuild demura bin file!\n", __FUNCTION__);
        }
        PANEL_AutoFlicker();
        PANEL_CreatSdcCheckBin();
    }
    /*do p_gamma choice*/
    if (p_gamma_size != 0x0) {
        if (p_gamma_param_0 == 0) {
            LOGD("%s, build gamma.bin for conventional panel!\n", __FUNCTION__);
            ret = PANEL_SPI_PGAMMA(p_gamma_offset, p_gamma_size, P2P_PGAMMA_PATH);
            if (ret < 0) {
                LOGE("%s,build p_gamma.bin failed!\n", __FUNCTION__);
                return -1;
            }
        } else if (p_gamma_param_0 == 1) {
            /*for 65D02-H */
            LOGD("%s, build gamma.bin for 65D02!\n", __FUNCTION__);
            ret = PANEL_SPI_PGAMMA_SPEC(p_gamma_offset, p_gamma_size, P2P_PGAMMA_PATH, P_GAMMA_DEFAULT_PATH_65D02_H);
            if (ret < 0) {
                LOGE("%s,build p_gamma.bin failed!\n", __FUNCTION__);
                return -1;
            }
            return 0;
        } else if (p_gamma_param_0 == 2) {
            /*for  55D09*/
            LOGD("%s, build gamma.bin for 55D09!\n", __FUNCTION__);
            ret = PANEL_SPI_PGAMMA_SPEC(p_gamma_offset, p_gamma_size, P2P_PGAMMA_PATH, P_GAMMA_DEFAULT_PATH_55D09);
            if (ret < 0) {
                LOGE("%s,build p_gamma.bin failed!\n", __FUNCTION__);
                return -1;
            }
            return 0;
        } else if (p_gamma_param_0 == 3) {
            /*for  65_D02-3*/
            LOGD("%s, build gamma.bin for 65D02-3!\n", __FUNCTION__);
            ret = PANEL_SPI_PGAMMA_SPEC_65D02_3(p_gamma_offset, p_gamma_size, P2P_PGAMMA_PATH,P_GAMMA_DEFAULT_PATH_65D02_3);
            if (ret < 0) {
                LOGE("%s,build p_gamma.bin failed!\n", __FUNCTION__);
                return -1;
            }
            return 0;
        } else if (p_gamma_param_0 == 4) {
            /*for 50T01*/
            LOGD("%s, build gamma.bin for 50T01!\n", __FUNCTION__);
            ret = PANEL_SPI_PGAMMA_SPEC(p_gamma_offset, p_gamma_size, P2P_PGAMMA_PATH, P_GAMMA_DEFAULT_PATH_50T01);
            if (ret < 0) {
                LOGE("%s,build p_gamma.bin failed!\n", __FUNCTION__);
                return -1;
            }
            return 0;
        }

        LOGD("%s,build p_gamma.bin successed!\n", __FUNCTION__);
    } else {
        LOGD("%s, no p_gamma function in this panel!\n", __FUNCTION__);
    }

    return 0;
}


/*
offset is Read the starting address of the original demura,the default address is 0x0.
partition_name_Table_data is the address to save the bin file of Table_data(lut)
partition_name_Register is the address to save the bin file of Register
*/
int CTvPanel::PANEL_43D10_data_Conversion(unsigned int offset,const char *partition_name_Table_data,const char *partition_name_Register) {
    ALOGD("%s,Address to read  demura IS %d\n", __FUNCTION__, offset);
    ALOGD("%s,Address to Address to save file of Table_data  IS %s\n", __FUNCTION__, partition_name_Table_data);
    ALOGD("%s,Address to Address to save file of Register IS %s\n", __FUNCTION__, partition_name_Register);
    if (access((const char*)P2P_LUT_FILE_PATH, F_OK) == 0 &&
            access((const char*)P2P_SET_FILE_PATH, F_OK) == 0 &&
            access((const char*)CRC_CHECK_PATH, F_OK) == 0 ) {
        LOGD("%s, bin files exist\n", __FUNCTION__);
        int crc_ret = PANEL_BIN_FILE_CEHCK(32, 0, 0, 0, 4, 0, 0, 0);
        if (crc_ret == 0) {
            LOGD("%s,  data is for this plane,do not need rebuild demura bin file!\n", __FUNCTION__);
            return 0;
        } else {
            LOGD("%s,  data is not for this plane, rebuild demura bin file!\n", __FUNCTION__);
        }
    }
    unsigned int data_size = 0x9B9D0;                        //LUT SIZE IN FLASH
    unsigned int ulength=36;
    unsigned char Parameter[36] = {0x0};
    int fdread1=tvSpiRead(offset, ulength, Parameter);
    if (fdread1<0)
    {
        LOGE("%s,TvSpiRead Failed!\n", __FUNCTION__);
        return -1;
    }

    if (Parameter[31] == 0xff && Parameter[30] == 0xff && Parameter[29] == 0xff && Parameter[28] == 0xff) {
        LOGE("%s,NO DATA IN FLASH!\n", __FUNCTION__);
        return -1;
    }

    unsigned short crc_check;
    crc_check = crc_check = (unsigned short)(Parameter[33]*0x100+Parameter[32]);
    unsigned char crc_bin[4] = {0x0};
    for (int i = 0;i < 4;i++) {
        crc_bin[i] = Parameter[32 + i];
    }
    unsigned char *lutbuffer = NULL;
    lutbuffer = (unsigned char *)malloc(data_size*sizeof(unsigned char));
    if (lutbuffer == NULL) {
        LOGE("%s,Malloc memory failed!\n", __FUNCTION__);
        return -1;
    }
    tvSpiRead(36, data_size, lutbuffer);
    unsigned int data_size_crc_32,data_size_crc=0;
    data_size_crc_32 = data_size/32+1;
    data_size_crc = data_size_crc_32*8;
    unsigned int *crc_buffer = NULL;
    crc_buffer = (unsigned int *)malloc(data_size_crc*sizeof(unsigned int));
    if (crc_buffer == NULL) {
        LOGE("%s,Malloc memory failed!\n", __FUNCTION__);
        free(lutbuffer);
        lutbuffer = NULL;
        return -1;
    }
    for (unsigned int p = 0;p < data_size_crc;p++)
        crc_buffer[p] = 0x0;
    int num_crc = 0;

    for (unsigned int k = 0;k<data_size/4;k++) {
        crc_buffer[k] = (unsigned int)(lutbuffer[num_crc+3]*0x1000000+lutbuffer[num_crc+2]*0x10000+lutbuffer[num_crc+1]*0x100+lutbuffer[num_crc]);
        num_crc = num_crc+4;
    }

    //unsigned short crc = 0xffff;
    unsigned short ret_crc = 0;
    ret_crc = PANEL_43_CRC_Check((unsigned int*)&crc_buffer[0],data_size_crc);
    LOGE("%s,crc after Calculation = 0x%x,crc in flash is 0x%x!\n", __FUNCTION__,ret_crc,crc_check);

    if (ret_crc != crc_check) {
        LOGE("%s,43 crc check failed!\n", __FUNCTION__);
        free(crc_buffer);
        crc_buffer = NULL;
        free(lutbuffer);
        lutbuffer = NULL;
        return -1;
    } else {
        LOGD("%s,43 crc check pass!\n", __FUNCTION__);
    }
    free(crc_buffer);
    crc_buffer = NULL;
    free(lutbuffer);
    lutbuffer = NULL;

    unsigned int ulength1=data_size+36;
    unsigned char *buffer = NULL;
    buffer = (unsigned char *)malloc(ulength1*sizeof(unsigned char));
    if (buffer == NULL) {
        ALOGE("%s,Malloc memory failed!\n", __FUNCTION__);
        return -1;
    }
    int fdread2=tvSpiRead(offset, ulength1, buffer);

    if (fdread2<0)
    {
        LOGE("%s,tvSpiRead Failed!\n", __FUNCTION__);
        free(buffer);
        buffer = NULL;
        return -1;
    } else {
        LOGD("%s,fdread2 IS %d\n", __FUNCTION__, fdread2);
    }

    int a,b;
    int m,n,k;
    unsigned int i = 0;
    k=0;
    int a5,b5,c5;
    a5=(data_size-data_size/16)*2/3;                            //a5 is the data number after conversion（delete every 0 in  each row）
    b5=a5/490*9;                                                //b5 is the number of each plane and line filing 9 0
    c5=a5-b5;

    char *buffer1 = NULL;
    buffer1 = (char *)malloc(c5*sizeof(char));
    if (buffer1 == NULL) {
        ALOGE("%s,Malloc memory failed!\n", __FUNCTION__);
        free(buffer);
        buffer = NULL;
        return -1;
    }
    for (i=36;i<=data_size+36;)                                         //conversion the value of the data    data_size+36
    {

        if ((i != 0) && (i%16 == 3)) {                                  //delete every 0 in  each row
            i++;
        } else {
            m=buffer[i+1]%0x10;
            n=buffer[i+1]/0x10;
            a=buffer[i]+m*0x100;
            b=buffer[i+2]*0x10+n;

            if (i%784 == 20) {                                          //delete each line filing 9 0
                if (a>2047)
                {
                    buffer1[k]=(unsigned char)((a-4096)/4+256);
                    k++;
                }
                else{
                    buffer1[k]=(unsigned char)(a/4);
                    k++;
                }
                i=i+15;
            } else {
                if (a>2047) {
                    buffer1[k]=(unsigned char)((a-4096)/4+256);
                    k++;
                } else {
                    buffer1[k]=(unsigned char)(a/4);
                    k++;
                }
                if (b>2047) {
                    buffer1[k]= (unsigned char)((b-4096)/4+256);
                    k++;
                } else {
                    buffer1[k]=(unsigned char)(b/4);
                    k++;
                }
                i=i+3;
            }
        }

    }
    LOGD("%s,data_conversion is OK!\n", __FUNCTION__);

    int size_plane = c5/3;;
    char *plane1 = NULL;
    plane1 = (char *)malloc(size_plane*sizeof(char));
    if (plane1 == NULL) {
        LOGE("%s,Malloc memory failed!\n", __FUNCTION__);
        free(buffer);
        buffer = NULL;
        free(buffer1);
        buffer1 = NULL;
        return -1;
    }

    int l1,i1,j1,k1;
    j1=0;
    k1=0;
    for (l1=0;l1<c5/1443;l1++) {
        for (i1=0;i1<481;i1++) {
            plane1[j1]=buffer1[k1];
            j1++;
            k1++;
        }
        k1=k1+962;
    }

    char *plane2 = NULL;
    plane2 = (char *)malloc(size_plane*sizeof(char));
    if (plane2 == NULL) {
        LOGE("%s,Malloc memory failed!\n", __FUNCTION__);
        free(buffer);
        buffer = NULL;
        free(buffer1);
        buffer1 = NULL;
        free(plane1);
        plane1 = NULL;
        return -1;
    }

    int l2,i2,j2,k2;
    j2=0;
    k2=481;
    for (l2=0;l2<c5/1443;l2++) {
            for (i2=0;i2<481;i2++) {
                plane2[j2] = buffer1[k2];
                j2++;
                k2++;
            }
            k2=k2+962;
    }

    char *plane3 = NULL;
    plane3 = (char *)malloc(size_plane*sizeof(char));
    if (plane3 == NULL) {
        LOGE("%s,Malloc memory failed!\n", __FUNCTION__);
        free(buffer);
        buffer = NULL;
        free(buffer1);
        buffer1 = NULL;
        free(plane1);
        plane1 = NULL;
        free(plane2);
        plane2 = NULL;
        return -1;
    }

    int l3,i3,j3,k3;
    j3=0;
    k3=962;
    for (l3 = 0;l3 < c5/1443; l3++) {
            for (i3 = 0;i3 < 481; i3++) {
                plane3[j3]=buffer1[k3];
                j3++;
                k3++;
            }
            k3=k3+962;
    }

    int i4,k4,k5,k6;
    unsigned char Register[161];
    k4=0;
    unsigned char re[]={0x0,0x0,0x13,0x33,0x1e,0x11,0xf,0x0,0x0,0x0,0xa0,0x1,0x90,0x3,0xc0,0xe,
                0x10,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xf,0xf0,0x4,0x40,0x1,0xd4,
                0x0,0x63,0x2,0x21,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x44,0x44,0x44,
                0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x0,0x0,0x0,0x0,0x0,0x00,0x0,
                0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
                0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
                0x0,0x0,0x0,0x0,0x0,0x80,0x80,0x80};
    for (k5 = 0; k5 < 110; k5++)                    //filing the data of excel
    {
        Register[k4]=re[k5];
        k4++;
    }
    for (k6 = 0; k6 <51; k6++) {
        Register[k4]=0x0;
        k4++;
    }

    unsigned char *result = NULL;
    result = (unsigned char *)malloc(c5*sizeof(unsigned char));
    if (result == NULL) {
        LOGE("%s,Malloc memory failed!\n", __FUNCTION__);
        free(buffer);
        buffer = NULL;
        free(buffer1);
        buffer1 = NULL;
        free(plane1);
        plane1 = NULL;
        free(plane2);
        plane2 = NULL;
        free(plane3);
        plane3 = NULL;
        return -1;
    }
    k4=0;

    for (i4=0;i4<c5/3;i4++)                 //put data in right position
    {
        result[k4]=plane1[i4];
        k4++;
        result[k4]=plane2[i4];
        k4++;
        result[k4]=plane3[i4];
        k4++;
    }
    LOGD("%s,data_sorting is OK!\n", __FUNCTION__);
    free(buffer);
    buffer = NULL;
    free(buffer1);
    buffer1 = NULL;
    free(plane1);
    plane1 = NULL;
    free(plane2);
    plane2 = NULL;
    free(plane3);
    plane3 = NULL;
    long offset1 =0;                                                            //changed start
    unsigned int length1=c5;
    unsigned int length2=161;
    int ret = -1;
    ret = system("mount -w -o remount /dev/block/vendor /vendor");
    LOGD("mount ret = %d\n", ret);

    int write1=PANEL_Flash_Write(partition_name_Table_data, offset1, result, length1);
    if (write1 !=0) {
        LOGD("%s,write lut error\n", __FUNCTION__);
        free(result);
        result = NULL;
        return -1;
    } else {
        LOGD("%s,write lut is OK\n", __FUNCTION__);
    }
    free(result);
    result = NULL;
    int write2=PANEL_Flash_Write(partition_name_Register, offset1, Register, length2);
    if (write2 !=0) {
        LOGD("%s,write reg error\n", __FUNCTION__);
        return -1;
    } else {
        LOGD("%s,write reg is OK\n", __FUNCTION__);
    }
    write1 = PANEL_Flash_Write(CRC_CHECK_PATH, 0, crc_bin, 4);
    if (write1 !=0) {
        LOGD("%s,write crc_bin error\n", __FUNCTION__);
        return -1;
    } else {
        LOGD("%s,write crc_bin is OK\n", __FUNCTION__);
    }
    ret = system("mount -r -o remount /dev/block/vendor /vendor");
    LOGD("unmount ret = %d\n", ret);
                                                                                //end
    return 0;
}

/*demura for 50-inch*/
#define   LUT_SIZE_ALL_50      476960
#define   RES_SIZE_50_PLANEL   130351
#define   RES_SIZE_50_ALL      651755  //5 PLANES
int CTvPanel::PANEL_50INX_data_Conversion(const char *partition_name_Table_data,const char *partition_name_Register) {
    if (access((const char*)P2P_LUT_FILE_PATH, F_OK) == 0 &&
                access((const char*)P2P_SET_FILE_PATH, F_OK) == 0 &&
                access((const char*)CRC_CHECK_PATH, F_OK) == 0 ) {
            LOGD("%s, bin files exist\n", __FUNCTION__);
            int crc_ret = PANEL_BIN_FILE_CEHCK(0x2a, 0, 0, 0, 2, 0, 0, 0);
            if (crc_ret == 0) {
                LOGD("%s,   data is for this plane,do not need rebuild demura bin file!\n", __FUNCTION__);
                return 0;
            } else {
                LOGD("%s,   data is not for this plane, rebuild demura bin file!\n", __FUNCTION__);
            }
        }

    unsigned char address_buffer[2] = {0x0};
    int fdread = tvSpiRead(46,2,address_buffer);
    if (fdread<0)
    {
        LOGE("%s,tvSpiRead Failed!\n", __FUNCTION__);
        return -1;
    }
    int address = (int)(address_buffer[0] * 0x100 +address_buffer[1]);
    LOGE("%s,adress = 0x%x!\n", __FUNCTION__,address);

    unsigned char  data_size[3] = {0x0};
    fdread = tvSpiRead(49,3,data_size);
    if (fdread<0)
    {
        LOGE("%s,tvSpiRead Failed!\n", __FUNCTION__);
        return -1;
    }
    if (data_size[0] == 0xff && data_size[1] == 0xff && data_size[2] == 0xff) {
        LOGD("%s,no demura data in flash,no need to do demura!\n", __FUNCTION__);
        return -1;
    }
    //unsigned char all_buffer_1[LUT_SIZE_ALL_50] = {0x0};
    unsigned char *all_buffer_1 = NULL;
    all_buffer_1 = (unsigned char *)malloc(LUT_SIZE_ALL_50*sizeof(unsigned char));
    if (all_buffer_1 == NULL) {
        LOGE("%s,Malloc memory failed!\n", __FUNCTION__);
        return -1;
    }
    fdread=tvSpiRead(address, LUT_SIZE_ALL_50, all_buffer_1);
    if (fdread<0)
    {
        LOGE("%s,tvSpiRead Failed!\n", __FUNCTION__);
        free(all_buffer_1);
        all_buffer_1 = NULL;
        return -1;
    }
    //crc
    unsigned char crc_buffer[2] = {0x0};
    fdread = tvSpiRead(0x2a,2,crc_buffer);
    if (fdread<0)
    {
        LOGE("%s,tvSpiRead crc Failed!\n", __FUNCTION__);
        free(all_buffer_1);
        all_buffer_1 = NULL;
        return -1;
    }
    unsigned char crc_data = (unsigned char)crc_buffer[0];
    unsigned char crc_out = 0xff;
    unsigned char crc_check = 0;
    PANEL_50INX_DEMURA_CRC((unsigned char *)all_buffer_1, &crc_out, 0, LUT_SIZE_ALL_50);
    crc_check = crc_out;

    LOGD("%s,crc in flash is 0x%x,lut crc after calculation is 0x%x!\n", __FUNCTION__,crc_data,crc_check);
    if (crc_check == crc_data) {
        LOGD("%s,50 crc check pass!\n", __FUNCTION__);
    } else {
        ALOGE("%s,50 crc check failed!\n", __FUNCTION__);
        free(all_buffer_1);
        all_buffer_1 = NULL;
        return -1;
    }
    //end of crc

    //unsigned int all_buffer[LUT_SIZE_ALL_50] = {0x0};
    unsigned int *all_buffer = NULL;
    all_buffer = (unsigned int *)malloc(LUT_SIZE_ALL_50*sizeof(unsigned int));
    if (all_buffer == NULL) {
        LOGE("%s,Malloc memory failed!\n", __FUNCTION__);
        free(all_buffer_1);
        all_buffer_1 = NULL;
        return -1;
    }
    for (int i = 0;i < LUT_SIZE_ALL_50;i++) {
        all_buffer[i] = (unsigned int)all_buffer_1[i];
    }
    free(all_buffer_1);
    all_buffer_1 = NULL;
    int ret = -1;
    unsigned int data_buffer[16] = {0x0};
    //unsigned int res_buffer[RES_SIZE_50_ALL] = {0x0};
    unsigned int *res_buffer = NULL;
    res_buffer = (unsigned int *)malloc(RES_SIZE_50_ALL*sizeof(unsigned int));
    if (res_buffer == NULL) {
        LOGE("%s,Malloc memory failed!\n", __FUNCTION__);
        free(all_buffer);
        all_buffer = NULL;
        return -1;
    }

    unsigned int mode[4] = {0x0};
    unsigned int ts = 0X0;
    unsigned int range = 0x0;
    unsigned int min = 0x0;
    unsigned int ts_buffer[16] = {0x0};
    unsigned int res_num = 0;
    int all_num = 0;
    int i,j,q,k;
    for (i = 0;i < 271;i++) {
        for (j = 0;j < 39;j++) {
            //first of four (per 45 byte)
            data_buffer[0] = all_buffer[all_num] << 28;
            data_buffer[0] = data_buffer[0] >> 28;
            data_buffer[1] = all_buffer[all_num] >> 4;
            all_num++;

            data_buffer[2] = all_buffer[all_num] << 28;
            data_buffer[2] = data_buffer[2] >> 28;
            data_buffer[3] = all_buffer[all_num] >> 4;
            all_num++;

            data_buffer[4] = all_buffer[all_num] << 28;
            data_buffer[4] = data_buffer[4] >> 28;
            data_buffer[5] = all_buffer[all_num] >> 4;
            all_num++;

            data_buffer[6] = all_buffer[all_num] << 28;
            data_buffer[6] = data_buffer[6] >> 28;
            data_buffer[7] = all_buffer[all_num] >> 4;
            all_num++;

            data_buffer[8] = all_buffer[all_num] << 28;
            data_buffer[8] = data_buffer[8] >> 28;
            data_buffer[9] = all_buffer[all_num] >> 4;
            all_num++;

            data_buffer[10] = all_buffer[all_num] << 28;
            data_buffer[10] = data_buffer[10] >> 28;
            data_buffer[11] = all_buffer[all_num] >> 4;
            all_num++;

            data_buffer[12] = all_buffer[all_num] << 28;
            data_buffer[12] = data_buffer[12] >> 28;
            data_buffer[13] = all_buffer[all_num] >> 4;
            all_num++;

            data_buffer[14] = all_buffer[all_num] << 28;
            data_buffer[14] = data_buffer[14] >> 28;
            data_buffer[15] = all_buffer[all_num] >> 4;
            all_num++;

            mode[0] = all_buffer[all_num] << 29;
            mode[0] = mode[0] >> 29;
            mode[1] = all_buffer[all_num] << 26;
            mode[1] = mode[1] >> 29;
            mode[2] = all_buffer[all_num] >> 6;
            all_num++;
            ts = all_buffer[all_num] << 31;
            ts = ts >> 29;
            mode[2] = mode[2] + ts;
            mode[3] = all_buffer[all_num] << 28;
            mode[3] = mode[3] >> 29;

            range = all_buffer[all_num] >> 4;
            all_num++;
            ts = all_buffer[all_num] << 29;
            ts = ts >> 25;
            range = range + ts;

            min = all_buffer[all_num] >> 3;
            all_num++;
            ts = all_buffer[all_num] << 30;
            ts = ts >> 25;
            min = min + ts;
            if (j == 38) {
                ret = PANEL_Decode_50inch((int *)data_buffer, (int *)mode, range, min,(int *)ts_buffer);
                res_buffer[res_num] = ts_buffer[0];
                res_num++;
            } else {
                ret = PANEL_Decode_50inch((int *)data_buffer, (int *)mode, range, min,(int *)ts_buffer);
                for (q = 0;q < 16;q++) {
                    res_buffer[res_num] = ts_buffer[q];
                    res_num++;
                }
            }
            //second of four (per 45 byte)
            data_buffer[0] = all_buffer[all_num] << 26;
            data_buffer[0] = data_buffer[0] >> 28;
            data_buffer[1] = all_buffer[all_num] >> 6;
            all_num++;
            ts = all_buffer[all_num] << 30;
            ts = ts >> 28;
            data_buffer[1] = data_buffer[1] + ts;

            data_buffer[2] = all_buffer[all_num] << 26;
            data_buffer[2] = data_buffer[2] >> 28;
            data_buffer[3] = all_buffer[all_num] >> 6;
            all_num++;
            ts = all_buffer[all_num] << 30;
            ts = ts >> 28;
            data_buffer[3] = data_buffer[3] + ts;

            data_buffer[4] = all_buffer[all_num] << 26;
            data_buffer[4] = data_buffer[4] >> 28;
            data_buffer[5] = all_buffer[all_num] >> 6;
            all_num++;
            ts = all_buffer[all_num] << 30;
            ts = ts >> 28;
            data_buffer[5] = data_buffer[5] + ts;

            data_buffer[6] = all_buffer[all_num] << 26;
            data_buffer[6] = data_buffer[6] >> 28;
            data_buffer[7] = all_buffer[all_num] >> 6;
            all_num++;
            ts = all_buffer[all_num] << 30;
            ts = ts >> 28;
            data_buffer[7] = data_buffer[7] + ts;

            data_buffer[8] = all_buffer[all_num] << 26;
            data_buffer[8] = data_buffer[8] >> 28;
            data_buffer[9] = all_buffer[all_num] >> 6;
            all_num++;
            ts = all_buffer[all_num] << 30;
            ts = ts >> 28;
            data_buffer[9] = data_buffer[9] + ts;

            data_buffer[10] = all_buffer[all_num] << 26;
            data_buffer[10] = data_buffer[10] >> 28;
            data_buffer[11] = all_buffer[all_num] >> 6;
            all_num++;
            ts = all_buffer[all_num] << 30;
            ts = ts >> 28;
            data_buffer[11] = data_buffer[11] + ts;

            data_buffer[12] = all_buffer[all_num] << 26;
            data_buffer[12] = data_buffer[12] >> 28;
            data_buffer[13] = all_buffer[all_num] >> 6;
            all_num++;
            ts = all_buffer[all_num] << 30;
            ts = ts >> 28;
            data_buffer[13] = data_buffer[13] + ts;

            data_buffer[14] = all_buffer[all_num] << 26;
            data_buffer[14] = data_buffer[14] >> 28;
            data_buffer[15] = all_buffer[all_num] >> 6;
            all_num++;
            ts = all_buffer[all_num] << 30;
            ts = ts >> 28;
            data_buffer[15] = data_buffer[15] + ts;

            mode[0] = all_buffer[all_num] << 27;
            mode[0] = mode[0] >> 29;
            mode[1] = all_buffer[all_num] >> 5;
            all_num++;
            mode[2] = all_buffer[all_num] << 29;
            mode[2] = mode[2] >> 29;
            mode[3] = all_buffer[all_num] << 26;
            mode[3] = mode[3] >> 29;

            range = all_buffer[all_num] >> 6;
            all_num++;
            ts = all_buffer[all_num] << 27;
            ts = ts >> 25;
            range = range + ts;

            min = all_buffer[all_num] >> 5;
            all_num++;
            ts = all_buffer[all_num] << 28;
            ts = ts >> 25;
            min = min +ts;

            if (j == 38) {
                ret = PANEL_Decode_50inch((int *)data_buffer, (int *)mode, range, min,(int *)ts_buffer);
                res_buffer[res_num] = ts_buffer[0];
                res_num++;
            } else {
                ret = PANEL_Decode_50inch((int *)data_buffer, (int *)mode, range, min,(int *)ts_buffer);
                for (q = 0;q < 16;q++) {
                    res_buffer[res_num] = ts_buffer[q];
                    res_num++;
                }
            }
            //third of four (per 45 byte)
            data_buffer[0] = all_buffer[all_num] >> 4;
            all_num++;
            data_buffer[1] = all_buffer[all_num] << 28;
            data_buffer[1] = data_buffer[1] >> 28;

            data_buffer[2] = all_buffer[all_num] >> 4;
            all_num++;
            data_buffer[3] = all_buffer[all_num] << 28;
            data_buffer[3] = data_buffer[3] >> 28;

            data_buffer[4] = all_buffer[all_num] >> 4;
            all_num++;
            data_buffer[5] = all_buffer[all_num] << 28;
            data_buffer[5] = data_buffer[5] >> 28;

            data_buffer[6] = all_buffer[all_num] >> 4;
            all_num++;
            data_buffer[7] = all_buffer[all_num] << 28;
            data_buffer[7] = data_buffer[7] >> 28;

            data_buffer[8] = all_buffer[all_num] >> 4;
            all_num++;
            data_buffer[9] = all_buffer[all_num] << 28;
            data_buffer[9] = data_buffer[9] >> 28;

            data_buffer[10] = all_buffer[all_num] >> 4;
            all_num++;
            data_buffer[11] = all_buffer[all_num] << 28;
            data_buffer[11] = data_buffer[11] >> 28;

            data_buffer[12] = all_buffer[all_num] >> 4;
            all_num++;
            data_buffer[13] = all_buffer[all_num] << 28;
            data_buffer[13] = data_buffer[13] >> 28;

            data_buffer[14] = all_buffer[all_num] >> 4;
            all_num++;
            data_buffer[15] = all_buffer[all_num] << 28;
            data_buffer[15] = data_buffer[15] >> 28;

            mode[0] = all_buffer[all_num] << 25;
            mode[0] = mode[0] >> 29;
            mode[1] = all_buffer[all_num] >> 7;
            all_num++;
            ts = all_buffer[all_num] << 30;
            ts = ts >> 29;
            mode[1] = mode[1] + ts;
            mode[2] = all_buffer[all_num] << 27;
            mode[2] = mode[2] >> 29;
            mode[3] = all_buffer[all_num] >> 5;

            all_num++;
            range = all_buffer[all_num] << 25;
            range = range >> 25;

            min = all_buffer[all_num] >> 7;
            all_num++;
            ts = all_buffer[all_num] << 26;
            ts = ts >> 25;
            min = min +ts;

            if (j == 37 || j == 38) {
                ret = PANEL_Decode_50inch((int *)data_buffer, (int *)mode, range, min,(int *)ts_buffer);
                res_buffer[res_num] = ts_buffer[0];
                res_num++;
            } else {
                ret = PANEL_Decode_50inch((int *)data_buffer, (int *)mode, range, min,(int *)ts_buffer);
                for (q = 0;q < 16;q++) {
                    res_buffer[res_num] = ts_buffer[q];
                    res_num++;
                }
            }
            //fourth of four (per 45 byte)
            if (j != 38) {
                data_buffer[0] = all_buffer[all_num] >> 6;
                all_num++;
                ts = all_buffer[all_num] << 30;
                ts = ts >> 28;
                data_buffer[0] = data_buffer[0] + ts;
                data_buffer[1] = all_buffer[all_num] << 26;
                data_buffer[1] = data_buffer[1] >> 28;

                data_buffer[2] = all_buffer[all_num] >> 6;
                all_num++;
                ts = all_buffer[all_num] << 30;
                ts = ts >> 28;
                data_buffer[2] = data_buffer[2] + ts;
                data_buffer[3] = all_buffer[all_num] << 26;
                data_buffer[3] = data_buffer[3] >> 28;

                data_buffer[4] = all_buffer[all_num] >> 6;
                all_num++;
                ts = all_buffer[all_num] << 30;
                ts = ts >> 28;
                data_buffer[4] = data_buffer[4] + ts;
                data_buffer[5] = all_buffer[all_num] << 26;
                data_buffer[5] = data_buffer[5] >> 28;

                data_buffer[6] = all_buffer[all_num] >> 6;
                all_num++;
                ts = all_buffer[all_num] << 30;
                ts = ts >> 28;
                data_buffer[6] = data_buffer[6] + ts;
                data_buffer[7] = all_buffer[all_num] << 26;
                data_buffer[7] = data_buffer[7] >> 28;

                data_buffer[8] = all_buffer[all_num] >> 6;
                all_num++;
                ts = all_buffer[all_num] << 30;
                ts = ts >> 28;
                data_buffer[8] = data_buffer[8] + ts;
                data_buffer[9] = all_buffer[all_num] << 26;
                data_buffer[9] = data_buffer[9] >> 28;

                data_buffer[10] = all_buffer[all_num] >> 6;
                all_num++;
                ts = all_buffer[all_num] << 30;
                ts = ts >> 28;
                data_buffer[10] = data_buffer[10] + ts;
                data_buffer[11] = all_buffer[all_num] << 26;
                data_buffer[11] = data_buffer[11] >> 28;

                data_buffer[12] = all_buffer[all_num] >> 6;
                all_num++;
                ts = all_buffer[all_num] << 30;
                ts = ts >> 28;
                data_buffer[12] = data_buffer[12] + ts;
                data_buffer[13] = all_buffer[all_num] << 26;
                data_buffer[13] = data_buffer[13] >> 28;

                data_buffer[14] = all_buffer[all_num] >> 6;
                all_num++;
                ts = all_buffer[all_num] << 30;
                ts = ts >> 28;
                data_buffer[14] = data_buffer[14] + ts;
                data_buffer[15] = all_buffer[all_num] << 26;
                data_buffer[15] = data_buffer[15] >> 28;

                mode[0] = all_buffer[all_num] >> 6;
                all_num++;
                ts = all_buffer[all_num] << 31;
                ts = ts >> 29;
                mode[0] = mode[0] + ts;
                mode[1] = all_buffer[all_num] << 28;
                mode[1] = mode[1] >> 29;
                mode[2] = all_buffer[all_num] << 25;
                mode[2] = mode[2] >> 29;
                mode[3] = all_buffer[all_num] >> 7;
                all_num++;
                ts = all_buffer[all_num] << 30;
                ts = ts >> 29;
                mode[3] = mode[3] + ts;

                range = all_buffer[all_num] >> 2;
                all_num++;
                ts = all_buffer[all_num] << 31;
                ts = ts >> 25;
                range = range + ts;

                min = all_buffer[all_num] >> 1;
                all_num++;

                if (j != 38) {
                    if (j == 37) {
                        ret = PANEL_Decode_50inch((int *)data_buffer, (int *)mode, range, min,(int *)ts_buffer);
                        res_buffer[res_num] = ts_buffer[0];
                        res_num++;
                    } else {
                        ret = PANEL_Decode_50inch((int *)data_buffer, (int *)mode, range, min,(int *)ts_buffer);
                        for (q = 0;q < 16;q++) {
                            res_buffer[res_num] = ts_buffer[q];
                            res_num++;
                        }
                    }
                }
            }
        }
        all_num = all_num + 17;
    }

    free(all_buffer);
    all_buffer = NULL;

    /*
    unsigned char lut1[RES_SIZE_50_PLANEL] = {0x0};
    unsigned char lut2[RES_SIZE_50_PLANEL] = {0x0};
    unsigned char lut3[RES_SIZE_50_PLANEL] = {0x0};
    unsigned char lut4[RES_SIZE_50_PLANEL] = {0x0};
    unsigned char lut5[RES_SIZE_50_PLANEL] = {0x0};
    */
    unsigned char *lut1 = NULL;
    lut1 = (unsigned char *)malloc(RES_SIZE_50_PLANEL*sizeof(unsigned char));
    if (lut1 == NULL) {
        ALOGE("%s,Malloc memory failed!\n", __FUNCTION__);
        free(all_buffer);
        all_buffer = NULL;
        free(res_buffer);
        res_buffer = NULL;
        return -1;
    }
    unsigned char *lut2 = NULL;
    lut2 = (unsigned char *)malloc(RES_SIZE_50_PLANEL*sizeof(unsigned char));
    if (lut2 == NULL) {
        ALOGE("%s,Malloc memory failed!\n", __FUNCTION__);
        free(all_buffer);
        all_buffer = NULL;
        free(res_buffer);
        res_buffer = NULL;
        free(lut1);
        lut1 = NULL;
        return -1;
    }
    unsigned char *lut3 = NULL;
    lut3 = (unsigned char *)malloc(RES_SIZE_50_PLANEL*sizeof(unsigned char));
    if (lut3 == NULL) {
        ALOGE("%s,Malloc memory failed!\n", __FUNCTION__);
        free(all_buffer);
        all_buffer = NULL;
        free(res_buffer);
        res_buffer = NULL;
        free(lut1);
        lut1 = NULL;
        free(lut2);
        lut2 = NULL;
        return -1;
    }
    unsigned char *lut4 = NULL;
    lut4 = (unsigned char *)malloc(RES_SIZE_50_PLANEL*sizeof(unsigned char));
    if (lut4 == NULL) {
        ALOGE("%s,Malloc memory failed!\n", __FUNCTION__);
        free(all_buffer);
        all_buffer = NULL;
        free(res_buffer);
        res_buffer = NULL;
        free(lut1);
        lut1 = NULL;
        free(lut2);
        lut2 = NULL;
        free(lut3);
        lut3 = NULL;
        return -1;
    }
    unsigned char *lut5 = NULL;
    lut5 = (unsigned char *)malloc(RES_SIZE_50_PLANEL*sizeof(unsigned char));
    if (lut5 == NULL) {
        ALOGE("%s,Malloc memory failed!\n", __FUNCTION__);
        free(all_buffer);
        all_buffer = NULL;
        free(res_buffer);
        res_buffer = NULL;
        free(lut1);
        lut1 = NULL;
        free(lut2);
        lut2 = NULL;
        free(lut3);
        lut3 = NULL;
        free(lut4);
        lut4 = NULL;
        return -1;
    }

    res_num = 0;
    int lut_num_1 = 0;
    int lut_num_2 = 0;
    int lut_num_3 = 0;
    int lut_num_4 = 0;
    int lut_num_5 = 0;

    for (i = 0;i< 271;i++) {
        for (j = 0;j < 31;j++) {
            if (j != 30) {
                for (k = 0;k < 16;k++) {
                    lut1[lut_num_1] = res_buffer[res_num];
                    res_num++;
                    lut_num_1++;
                }
                for (k = 0;k < 16;k++) {
                    lut2[lut_num_2] = res_buffer[res_num];
                    res_num++;
                    lut_num_2++;
                }
                for (k = 0;k < 16;k++) {
                    lut3[lut_num_3] = res_buffer[res_num];
                    res_num++;
                    lut_num_3++;
                }
                for (k = 0;k < 16;k++) {
                    lut4[lut_num_4] = res_buffer[res_num];
                    res_num++;
                    lut_num_4++;
                }
                for (k = 0;k < 16;k++) {
                    lut5[lut_num_5] = res_buffer[res_num];
                    res_num++;
                    lut_num_5++;
                }
            } else {
                lut1[lut_num_1] = res_buffer[res_num];
                res_num++;
                lut_num_1++;
                lut2[lut_num_2] = res_buffer[res_num];
                res_num++;
                lut_num_2++;
                lut3[lut_num_3] = res_buffer[res_num];
                res_num++;
                lut_num_3++;
                lut4[lut_num_4] = res_buffer[res_num];
                res_num++;
                lut_num_4++;
                lut5[lut_num_5] = res_buffer[res_num];
                res_num++;
                lut_num_5++;
            }
        }
    }
    res_num = 0;

    unsigned char plane[6] = {0x0};

    ret = tvSpiRead(0x71,6,plane);
    if (ret < 0)
    {
        LOGE("%s,AML_HAL_SPI_Read for which plane is valid Failed!\n", __FUNCTION__);
        free(all_buffer);
        all_buffer = NULL;
        free(res_buffer);
        res_buffer = NULL;
        free(lut1);
        lut1 = NULL;
        free(lut2);
        lut2 = NULL;
        free(lut3);
        lut3 = NULL;
        free(lut4);
        lut4 = NULL;
        free(lut5);
        lut5 = NULL;
        return -1;
    } else {
        LOGD("%s,0x71 - 0x76 = 0x%x,0x%x,0x%x,0x%x,0x%x,0x%x!\n", __FUNCTION__,plane[0],plane[1],plane[2],
            plane[3],plane[4],plane[5]);
    }

    if (plane[5] == plane[0] && plane[4] == plane[0]) {
        for (i = 0;i < RES_SIZE_50_PLANEL;i++) {
            res_buffer[res_num] = lut1[i];
            res_num++;
            res_buffer[res_num] = lut2[i];
            res_num++;
            res_buffer[res_num] = lut3[i];
            res_num++;
            res_buffer[res_num] = 0x0;
            res_num++;
            res_buffer[res_num] = 0x0;
            res_num++;
        }
    } else if (plane[5] == plane[0] && plane[4] != plane[0]) {
        for (i = 0;i < RES_SIZE_50_PLANEL;i++) {
            res_buffer[res_num] = lut1[i];
            res_num++;
            res_buffer[res_num] = lut2[i];
            res_num++;
            res_buffer[res_num] = lut3[i];
            res_num++;
            res_buffer[res_num] = lut4[i];
            res_num++;
            res_buffer[res_num] = 0x0;
            res_num++;
        }
    } else {
        for (i = 0;i < RES_SIZE_50_PLANEL;i++) {
            res_buffer[res_num] = lut1[i];
            res_num++;
            res_buffer[res_num] = lut2[i];
            res_num++;
            res_buffer[res_num] = lut3[i];
            res_num++;
            res_buffer[res_num] = lut4[i];
            res_num++;
            res_buffer[res_num] = lut5[i];
            res_num++;
        }
    }

    free(lut1);
    lut1 = NULL;
    free(lut2);
    lut2 = NULL;
    free(lut3);
    lut3 = NULL;
    free(lut4);
    lut4 = NULL;
    free(lut5);
    lut5 = NULL;

    //char res_buffer_2[RES_SIZE_50_ALL] = {0x0};//res_buffer_2 is the buffer save the last result of lut data
    unsigned char *res_buffer_2 = NULL;
    res_buffer_2 = (unsigned char *)malloc(RES_SIZE_50_ALL*sizeof(unsigned char));
    if (res_buffer_2 == NULL) {
        LOGE("%s,Malloc memory failed!\n", __FUNCTION__);
        free(all_buffer);
        all_buffer = NULL;
        free(res_buffer);
        res_buffer = NULL;
        return -1;
    }
    for (i = 0;i < RES_SIZE_50_ALL; i++) {
        res_buffer_2[i] = (char) res_buffer[i];
    }
    free(res_buffer);
    res_buffer = NULL;

    unsigned char reg_buffer[159] = {0x00,0x00,0x15,0x33,0x1e,0x11,0x0f,0x00,0x00,0x00,0xa0,0x01,0x90,0x04,0x00,0x08,
                            0x00,0x0a,0x00,0x0c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0f,0x00,0x04,0x40,0x01,
                            0xa4,0x01,0x00,0x02,0x00,0x02,0x00,0x01,0x55,0x00,0x00,0x00,0x00,0x00,0x00,0x44,
                            0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x00,0x00,0x00,0x00,0x00,
                            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x80,0x80,0x00,0x00,
                            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
                        };

    ret = system("mount -w -o remount /dev/block/vendor /vendor");
    LOGD("mount ret = %d\n", ret);
    ret = PANEL_Flash_Write(partition_name_Table_data, 0, res_buffer_2, 651755);
    if (ret !=0) {
        LOGE("%s,write lut error\n", __FUNCTION__);
        free(res_buffer_2);
        res_buffer_2 = NULL;
        return -1;
    } else {
        LOGD("%s,write lut is OK\n", __FUNCTION__);
    }
    free(res_buffer_2);
    res_buffer_2 = NULL;
    ret = PANEL_Flash_Write(partition_name_Register, 0, reg_buffer, 159);
    if (ret !=0) {
        LOGE("%s,write reg error\n", __FUNCTION__);
        return -1;
    } else {
        LOGD("%s,write reg is OK\n", __FUNCTION__);
    }

    ret = PANEL_Flash_Write(CRC_CHECK_PATH, 0, crc_buffer, 2);
    if (ret !=0) {
        LOGE("%s,write crc_bin error\n", __FUNCTION__);
        return -1;
    } else {
        LOGD("%s,write crc_bin is OK\n", __FUNCTION__);
    }
    ret = system("mount -r -o remount /dev/block/vendor /vendor");
    LOGD("unmount ret = %d\n", ret);
    return 0;
}
/*decode for 50-inch demura*/


/*demura for 50T01*/
#define      LUT_SIZE_FLASH_50T01      0x8F360
#define      LUT_SIZE_CON_50T01        391053
int CTvPanel::PANEL_50T01_data_Conversion(const char *partition_name_Table_data,const char *partition_name_Register) {
    if (NULL == partition_name_Table_data || NULL == partition_name_Register) {
        LOGE("%s,partition_name_Table_data OR partition_name_Register is NULL!\n", __FUNCTION__);
        return -1;
    }
    if (access((const char*)P2P_LUT_FILE_PATH, F_OK) == 0 &&
            access((const char*)P2P_SET_FILE_PATH, F_OK) == 0 &&
            access((const char*)CRC_CHECK_PATH, F_OK) == 0 ) {
        LOGD("%s, bin files exist\n", __FUNCTION__);
        int crc_ret = PANEL_BIN_FILE_CEHCK(0x40, 0, 0, 0, 2, 0,0, 0);
        if (crc_ret == 0) {
            LOGD("%s,  data is for this plane,do not need rebuild demura bin file!\n", __FUNCTION__);
            return 0;
        }else{
            LOGD("%s,  data is not for this plane, rebuild demura bin file!\n", __FUNCTION__);
        }
    }
    int i = 0;
    unsigned char *buffer_flash_lut = NULL;
    buffer_flash_lut = (unsigned char *)malloc(sizeof(unsigned char) * LUT_SIZE_FLASH_50T01);
    if (NULL == buffer_flash_lut) {
        LOGE("%s,buffer_flash_lut malloc failed\n",__FUNCTION__);
        return -1;
    }
    memset(buffer_flash_lut, 0x0, sizeof(unsigned char) * LUT_SIZE_FLASH_50T01);
    if (0 > tvSpiRead(0x42,LUT_SIZE_FLASH_50T01,buffer_flash_lut)) {
        LOGE("%s,buffer_flash_lut read flash failed\n",__FUNCTION__);
        free(buffer_flash_lut);
        buffer_flash_lut = NULL;
        return -1;
    }

    /*
    buffer_flash_lut has dummy 0 at last,include in crc Calculation
    crc address start in 0x40 in flash,length is 2
    */
    unsigned char buffer_crc[2] = {0x0};
    if (0 > tvSpiRead(0x40,2,buffer_crc)) {
        LOGE("%s,buffer_crc read flash failed\n",__FUNCTION__);
        free(buffer_flash_lut);
        buffer_flash_lut = NULL;
        return -1;
    }
    unsigned short crc_flash = (unsigned short)(buffer_crc[0] * 0x100 + buffer_crc[1]);
    unsigned short crc_cal = PANEL_50T01_DEMURA_CRC((unsigned char *)buffer_flash_lut, LUT_SIZE_FLASH_50T01);
    LOGD("%s,crc in flash = 0x%x,crc after cal is 0x%x\n",__FUNCTION__, crc_flash, crc_cal);
    if (crc_flash == crc_cal) {
        LOGD("%s,crc check pass!\n",__FUNCTION__);
    } else {
        LOGE("%s,crc check failed!\n",__FUNCTION__);
        free(buffer_flash_lut);
        buffer_flash_lut = NULL;
        return -1;
    }

    /*
    LUT_SIZE_CON_50T01 = point(481) * line(271) * plane(3) = 391053
    buffer_flash_lut has dummy 0 at last,not include in lut data
    */
    int *buffer_ts = NULL;
    buffer_ts = (int *)malloc(sizeof(int) * LUT_SIZE_CON_50T01);
    if (NULL == buffer_ts) {
        LOGE("%s,buffer_flash_lut malloc failed\n",__FUNCTION__);
        free(buffer_flash_lut);
        buffer_flash_lut = NULL;
        return -1;
    }
    memset(buffer_ts, 0x0, sizeof(int) * LUT_SIZE_CON_50T01);
    int point = 0;
    int flash_num = 0;
    for (i = 0;i < 195527;i++) {
        if (195526 == i) {
            buffer_ts[point] = (int)((buffer_flash_lut[flash_num] << 4) + ((buffer_flash_lut[flash_num + 1] & 0xf0) >> 4));
        }else{
            buffer_ts[point] = (int)((buffer_flash_lut[flash_num] << 4) + ((buffer_flash_lut[flash_num + 1] & 0xf0) >> 4));
            buffer_ts[point + 1] = (int)(((buffer_flash_lut[flash_num + 1] & 0xf) << 8) + buffer_flash_lut[flash_num + 2]);
        }
        flash_num += 3;
        point += 2;
    }
    free(buffer_flash_lut);
    buffer_flash_lut = NULL;
    unsigned char *buffer_res = NULL;
    buffer_res = (unsigned char *)malloc(sizeof(unsigned char) * LUT_SIZE_CON_50T01);
    if (NULL == buffer_res) {
        LOGE("%s,buffer_res malloc failed\n",__FUNCTION__);
        free(buffer_ts);
        buffer_ts = NULL;
        return -1;
    }
    memset(buffer_res, 0x0, sizeof(unsigned char) * LUT_SIZE_CON_50T01);
    for (i = 0;i < LUT_SIZE_CON_50T01;i++) {
        if (2047 < buffer_ts[i]) {
            buffer_res[i] = (unsigned char)((buffer_ts[i] - 4096) / 4 + 256);
        }else{
            buffer_res[i] = (unsigned char)(buffer_ts[i] / 4);
        }
    }
    free(buffer_ts);
    buffer_ts = NULL;

    unsigned char buffer_reg[159] = {0x00,0x00,0x13,0x33,0x1e,0x11,0x0f,0x00,0x00,0x00,0x00,0x01,0x90,0x03,0x00,0x08,
                                     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0f,0xfc,0x02,0x8e,0x02,
                                     0xc8,0x00,0xcc,0x00,0x81,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x44,
                                     0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x00,0x00,0x00,0x00,0x00,
                                     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x80,0x80,0x00,0x00,
                                     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    };

    int ret = system("mount -w -o remount /dev/block/vendor /vendor");
    ALOGD("%s,mount ret = %d\n",__FUNCTION__, ret);
    if (PANEL_Flash_Write(partition_name_Table_data, 0, buffer_res, LUT_SIZE_CON_50T01) == -1) {
        free(buffer_res);
        buffer_res = NULL;
        LOGD("%s,write lut bin fail\n",__FUNCTION__);
        return -1;
    }
    if (PANEL_Flash_Write(partition_name_Register, 0, buffer_reg, 159) == -1) {
        free(buffer_res);
        buffer_res = NULL;
        LOGD("%s,write reg bin fail\n",__FUNCTION__);
        return -1;
    }
    if (PANEL_Flash_Write(CRC_CHECK_PATH, 0, buffer_crc, 2) == -1) {
        LOGD("%s,write crc_check bin fail\n",__FUNCTION__);
        return -1;
    }
    ret = system("mount -r -o remount /dev/block/vendor /vendor");
    LOGD("unmount ret = %d\n", ret);
    return 0;
}


#define INFO_LEN_65D2_3 272
#define RES_LEN_65D2_3  391053
#define LUT_LEN_65D2_3  637392
#define PARA_OFFSET     0x40
#define PARA_LEN    0xD0
#define LUT_OFFSET  0x110

int CTvPanel::PANEL_65D02_3_data_Conversion(const char *partition_name_Table_data,const char *partition_name_Register) {
    ALOGD("%s: start create\n", __FUNCTION__);
    int fdread = -1;
    if (access((const char*)P2P_LUT_FILE_PATH, F_OK) == 0 &&
            access((const char*)P2P_SET_FILE_PATH, F_OK) == 0 &&
            access((const char*)CRC_CHECK_PATH, F_OK) == 0 ) {
        LOGD("%s, bin files exist\n", __FUNCTION__);
        int crc_ret = PANEL_BIN_FILE_CEHCK(0x3C, 0, 0, 0, 2, 0,0, 0);
        if (crc_ret == 0) {
            LOGD("%s,  data is for this plane,do not need rebuild demura bin file!\n", __FUNCTION__);
            return 0;
        } else {
            LOGD("%s,  data is not for this plane, rebuild demura bin file!\n", __FUNCTION__);
        }
    }

    unsigned char *flash_info = NULL;
    flash_info = (unsigned char *)malloc(sizeof(unsigned char)*INFO_LEN_65D2_3);
    if (flash_info == NULL) {
        LOGD("%s,flash_info malloc failed\n",__FUNCTION__);
        return -1;
    }

    memset(flash_info, 0x0, sizeof(unsigned char)*INFO_LEN_65D2_3);

    fdread=tvSpiRead(0,INFO_LEN_65D2_3,flash_info);
    if (fdread < 0) {
        LOGD("%s,tvSpiRead info failed\n",__FUNCTION__);
        free(flash_info);
        flash_info = NULL;
        return -1;
    }

    //Customers suggest add this judgment
    if (flash_info[36] == 0xff && flash_info[37] == 0xff && flash_info[38] == 0xff && flash_info[39] == 0xff) {
        LOGE("%s,NO DATA IN FLASH\n", __FUNCTION__);
        free(flash_info);
        flash_info = NULL;
        return -1;
    }

    if (flash_info[36] == 0x00 && flash_info[37] == 0x00 && flash_info[38] == 0x00 && flash_info[39] == 0x00) {
        LOGE("%s,NO DATA IN FLASH\n", __FUNCTION__);
        free(flash_info);
        flash_info = NULL;
        return -1;
    }

    unsigned int para_crc = (flash_info[44] | ((unsigned int)flash_info[45] << 8));
    unsigned int lut_crc = (flash_info[60] | ((unsigned int)flash_info[61] << 8));

    unsigned char crc_buffer[2] = {0x0};
    memcpy(crc_buffer, flash_info+60, 2);

    //check para data crc
    unsigned short para_cal_crc = CalcCRC16((unsigned char*)flash_info+PARA_OFFSET, PARA_LEN);
    LOGD("%s, para_cal_crc:0x%x\n",__FUNCTION__,para_cal_crc);
    if (para_cal_crc == para_crc) {
        LOGD("%s,demura para crc check right\n",__FUNCTION__);
    } else {
        LOGD("%s,demura para crc check failed\n",__FUNCTION__);
        free(flash_info);
        flash_info = NULL;
        return -1;
    }
    //para_offset=64
    int panel_num = flash_info[65];
    int HLutNum = flash_info[69]+(flash_info[70] << 8);
    int VLutNum = flash_info[71]+(flash_info[72] << 8);
    LOGD("%s,demura_para_crc:0x%x,demura_lut_crc:0x%x\n",__FUNCTION__,para_crc,lut_crc);
    LOGD("%s,panel_num:%d,demura_HLutNum:0x%x,demura_VLutNum:0x%x\n",__FUNCTION__, panel_num, HLutNum, VLutNum);

    free(flash_info);
    flash_info = NULL;

    unsigned char *flash_demura = NULL;
    flash_demura = (unsigned char*)malloc(sizeof(unsigned char)*LUT_LEN_65D2_3);
    if (flash_demura == NULL) {
        printf("flash_demura malloc failed\n");
        return -1;
    }

    memset(flash_demura, 0x0, sizeof(unsigned char)*LUT_LEN_65D2_3);

    fdread = tvSpiRead(LUT_OFFSET, LUT_LEN_65D2_3, flash_demura);
    if (fdread < 0) {
        ALOGD("%s,tvSpiRead lut failed\n",__FUNCTION__);
        free(flash_demura);
        flash_demura = NULL;
        return -1;
    }


    unsigned short lut_cal_crc = CalcCRC16((unsigned char*)flash_demura,LUT_LEN_65D2_3);
    LOGD("%s, lut_cal_crc:0x%x\n",__FUNCTION__,lut_cal_crc);

    if (lut_cal_crc == lut_crc) {
        LOGD("%s,demura lut crc check right\n",__FUNCTION__);
    } else {
        LOGD("%s,demura lut crc check failed\n",__FUNCTION__);
        free(flash_demura);
        flash_demura = NULL;
        return -1;
    }

    //handle flash_demura data
    unsigned short *point = NULL;
    point = (unsigned short *)malloc(sizeof(unsigned short)*RES_LEN_65D2_3);
    if (point == NULL) {
        LOGD("%s,point malloc failed\n",__FUNCTION__);
        free(flash_demura);
        flash_demura = NULL;
        return -1;
    }

    int offset = 0;
    int line_offset = 0;
    int panel_offset = 0;
    int local = 0;
    for (int l = 0; l < 3; l++) {
        panel_offset = 784 * l;
        local = 0;
        for (int k = 0; k < 271; k++) {
            line_offset = k * 784 *3;
            for (int j = 0; j < 49; j++) {
                offset = 16*j + line_offset + panel_offset;
                if (j == 48) {
                    point[local*3+l] = flash_demura[offset] + ((flash_demura[1+offset] & 0xf) << 8);
                    local++;
                    continue;
                }
                for (int i = 0; i < 10; i++) {
                    if (i%2 == 0) {
                        point[local*3+l] = flash_demura[i/2*3+offset] + ((flash_demura[i/2*3+1+offset] & 0xf) << 8);
                        local++;
                    } else {
                        point[local*3+l] = ((flash_demura[i/2*3+1+offset] & 0xf0) >> 4) + ((flash_demura[i/2*3+2+offset] & 0xff) << 4);
                        local++;
                    }
                }
            }
        }
    }

    free(flash_demura);
    flash_demura = NULL;

    unsigned char * result = NULL;
    result = (unsigned char *)malloc(sizeof(unsigned char)*RES_LEN_65D2_3);
    if (result == NULL) {
        LOGD("%s,reslut malloc failed\n",__FUNCTION__);
        free(point);
        point = NULL;
        return -1;
    }

    for (int i = 0; i < RES_LEN_65D2_3; i++) {
        if (point[i] > 2047) {
            result[i] = (unsigned char)((point[i] - 4096)/4 + 256);
        } else {
            result[i] = (unsigned char)(point[i]/4);
        }
    }

    free(point);
    point = NULL;

    int ret = system("mount -w -o remount /dev/block/vendor /vendor");
    LOGD("%s,mount ret = %d\n",__FUNCTION__, ret);
    if (PANEL_Flash_Write(partition_name_Table_data, 0, result, RES_LEN_65D2_3) == -1) {
        free(result);
        result = NULL;
        LOGD("%s,write lut fail\n",__FUNCTION__);
        return -1;
    }

    free(result);
    result = NULL;

    //handle register
    unsigned char Register[159]={0x00,0x00,0x13,0x33,0x1e,0x11,0x0f,0x00,0x00,0x00,0x00,0x01,0x90,0x03,0xc0,0x0e,
            0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0f,0xfc,0x02,0x8f,0x01,
            0xD4,0x00,0x63,0x02,0x14,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x44,
            0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x00,0x00,0x00,0x00,0x00,
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x80,0x80,0x00,0x00,
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

    if (PANEL_Flash_Write(partition_name_Register, 0, Register, 159) == -1) {
        LOGD("%s,write reg fail\n",__FUNCTION__);
        return -1;
    }


    if (PANEL_Flash_Write(CRC_CHECK_PATH, 0, crc_buffer, 2) == -1) {
        LOGD("%s,write crc_check fail\n",__FUNCTION__);
        return -1;
    }

    ret = system("mount -r -o remount /dev/block/vendor /vendor");
    LOGD("unmount ret = %d\n", ret);
    return 0;
}

#define   RES_SIZE_65_ALL      391053
#define   RES_SIZE_65_PLANEL   130351
int CTvPanel::PANEL_65D02_H_data_Conversion(const char *partition_name_Table_data,const char *partition_name_Register){
    if (access((const char*)P2P_LUT_FILE_PATH, F_OK) == 0 &&
            access((const char*)P2P_SET_FILE_PATH, F_OK) == 0 &&
            access((const char*)CRC_CHECK_PATH, F_OK) == 0 ) {
        LOGD("%s, bin files exist\n", __FUNCTION__);
        int crc_ret = PANEL_BIN_FILE_CEHCK(0x99, 0, 0, 0, 2, 0,0, 0);
        if (crc_ret == 0) {
            LOGD("%s,  data is for this plane,do not need rebuild demura bin file!\n", __FUNCTION__);
            return 0;
        } else {
            LOGD("%s,  data is not for this plane, rebuild demura bin file!\n", __FUNCTION__);
        }
    }

    unsigned int data_size = 590238;
    unsigned char *buffer = NULL;
    buffer = (unsigned char *)malloc(data_size*sizeof(unsigned char));
    if (buffer == NULL) {
        ALOGE("%s,Malloc memory failed!\n", __FUNCTION__);
        return -1;
    }

    unsigned int offset_1 = 0x200;
    int fdread=tvSpiRead( offset_1, data_size, buffer);
    if (fdread<0)
    {
        free(buffer);
        buffer = NULL;
        LOGE("%s,tvSpiRead Failed!\n", __FUNCTION__);
        return -1;
    }

    unsigned char crc_buffer[2] = {0x0};
    fdread=tvSpiRead(0x99, 2, crc_buffer);
    if (fdread<0)
    {
        free(buffer);
        buffer = NULL;
        LOGE("%s,tvSpiRead Failed!\n", __FUNCTION__);
        return -1;
    }
    unsigned short crc = (unsigned short)(crc_buffer[0] * 0x100 + crc_buffer[1]);
    unsigned short crc_check  = PANEL_65D02_H_DEMURA_CRC((unsigned char *)buffer, data_size);
    if (crc == crc_check) {
        LOGD("%s,crc check passed!\n", __FUNCTION__);
    } else {
        LOGD("%s,crc check failed!crc data in flash is 0x%x,the demura table crc is 0x%x.\n", __FUNCTION__,crc,crc_check);
        free(buffer);
        buffer = NULL;
        return -1;
    }

    int a,b,m,n;
    //char buffer1[RES_SIZE_65_ALL] = {0};
    unsigned char *buffer1 = NULL;
    buffer1 = (unsigned char *)malloc(data_size*sizeof(unsigned char));
    if (buffer1 == NULL) {
        LOGE("%s,Malloc memory failed!\n", __FUNCTION__);
        free(buffer);
        buffer = NULL;
        return -1;
    }
    unsigned int i = 0;

    int k = 0;
    for (i = 0;i < data_size;) {
        m=buffer[i+1]%0x10;
        n=buffer[i+1]/0x10;
        a=buffer[i]+m*0x100;
        b=buffer[i+2]*0x10+n;
        if (i%726 == 720) {
            if (a >= 2048) {
                buffer1[k] = (unsigned char)((a-2048)/4);
            } else {
                buffer1[k] = (unsigned char)((a-2048)/4+256);
            }
            k++;
            i = i+6;
        } else {
            if (a >= 2048) {
                buffer1[k] = (unsigned char)((a-2048)/4);
                k++;
            } else {
                buffer1[k] = (unsigned char)((a-2048)/4+256);
                k++;
            }
            if (b >= 2048) {
                buffer1[k] = (unsigned char)((b-2048)/4);
                k++;
            } else {
                buffer1[k] = (unsigned char)((b-2048)/4+256);
                k++;
            }
            i = i+3;
        }
    }
    //char plane1[RES_SIZE_65_PLANEL];
    char *plane1 = NULL;
    plane1 = (char *)malloc(RES_SIZE_65_PLANEL*sizeof(char));
    if (plane1 == NULL) {
        LOGE("%s,Malloc memory failed!\n", __FUNCTION__);
        free(buffer);
        buffer = NULL;
        free(buffer1);
        buffer1 = NULL;
        return -1;
    }

    int l1,i1,j1,k1;
    j1=0;
    k1=0;
    for (l1=0;l1<271;l1++) {
            for (i1=0;i1<481;i1++) {
                plane1[j1]=buffer1[k1];
                j1++;
                k1++;
            }
            k1=k1+962;
    }
    //char plane2[RES_SIZE_65_PLANEL];
    char *plane2 = NULL;
    plane2 = (char *)malloc(RES_SIZE_65_PLANEL*sizeof(char));
    if (plane2 == NULL) {
        LOGE("%s,Malloc memory failed!\n", __FUNCTION__);
        free(buffer);
        buffer = NULL;
        free(plane1);
        plane1 = NULL;
        free(buffer1);
        buffer1 = NULL;
        return -1;
    }

    j1=0;
    k1=481;
    for (l1=0;l1<271;l1++) {
            for (i1=0;i1<481;i1++) {
                plane2[j1]=buffer1[k1];
                j1++;
                k1++;
            }
            k1=k1+962;
    }
    //char plane3[RES_SIZE_65_PLANEL];
    char *plane3 = NULL;
    plane3 = (char *)malloc(RES_SIZE_65_PLANEL*sizeof(char));
    if (plane3 == NULL) {
        LOGE("%s,Malloc memory failed!\n", __FUNCTION__);
        free(buffer);
        buffer = NULL;
        free(buffer1);
        buffer1 = NULL;
        free(plane1);
        plane1 = NULL;
        free(plane2);
        plane2 = NULL;
        return -1;
    }

    j1=0;
    k1=962;
    for (l1=0;l1<271;l1++) {
            for (i1=0;i1<481;i1++) {
                plane3[j1]=buffer1[k1];
                j1++;
                k1++;
            }
            k1=k1+962;
    }
    //unsigned char result[RES_SIZE_65_ALL];
    unsigned char *result = NULL;
    result = (unsigned char *)malloc(RES_SIZE_65_ALL*sizeof(unsigned char));
    if (result == NULL) {
        ALOGE("%s,Malloc memory failed!\n", __FUNCTION__);
        free(buffer);
        buffer = NULL;
        free(buffer1);
        buffer1 = NULL;
        free(plane1);
        plane1 = NULL;
        free(plane2);
        plane2 = NULL;
        free(plane3);
        plane3 = NULL;
        return -1;
    }

    int num = 0;
    for (i=0;i<RES_SIZE_65_ALL/3;i++)
    {
        result[num]=plane1[i];
        num++;
        result[num]=plane2[i];
        num++;
        result[num]=plane3[i];
        num++;
    }
    unsigned char Register[159] = {0x0};
    unsigned char re[]={0x0,0x0,0x13,0x33,0x1e,0x11,0xf,0x0,0x0,0x0,0x30,0x1,0x90,0x3,0xc0,
        0x0e,0x10,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0f,0xff,0x2,0xe7,
        0x1,0xd4,0x0,0x63,0x2,0x11,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
        0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x0,0x0,0x0,0x0,
        0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
        0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
        0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x80,0x80,0x80,0x0};
    int reg_num = 0;
    for (i=0;i<111;i++)
    {
        Register[reg_num]=re[i];
        reg_num++;
    }
    for (i=0;i<48;i++) {
        Register[reg_num]=0x0;
        reg_num++;
    }
    free(buffer);
    buffer = NULL;
    free(buffer1);
    buffer1 = NULL;

    long offset1 = 0;
    unsigned int length1 = 391053;
    unsigned int length2 = 159;
    int ret = -1;
    ret = system("mount -w -o remount /dev/block/vendor /vendor");
    LOGD("mount ret = %d\n", ret);
    int write1=PANEL_Flash_Write(partition_name_Table_data, offset1, result, length1);
    if (write1 !=0) {
        LOGE("%s,write lut error\n", __FUNCTION__);
        free(result);
        result = NULL;
        return -1;
    } else {
        LOGD("%s,write lut is OK\n", __FUNCTION__);
    }
    free(result);
    result = NULL;
    int write2=PANEL_Flash_Write(partition_name_Register, offset1, Register, length2);
    if (write2 !=0) {
        LOGE("%s,write reg error\n", __FUNCTION__);
        return -1;
    } else {
        LOGD("%s,write reg is OK\n", __FUNCTION__);
    }
    write1 = PANEL_Flash_Write(CRC_CHECK_PATH, 0, crc_buffer, 2);
    if (write1 !=0) {
        LOGE("%s,write crc_bin error\n", __FUNCTION__);
        return -1;
    } else {
        LOGD("%s,write crc_bin is OK\n", __FUNCTION__);
    }
    ret = system("mount -r -o remount /dev/block/vendor /vendor");
    LOGD("unmount ret = %d\n", ret);
    return 0;
}


#define DEMURA_LEN          912457
#define DEMURA_FLASH_LEN    912465      //DEMURA_LEN+dummy+checksum
#define DEMURA_FLASH_ADDR   0x51000
#define GAIN_FLASH_ADDR     0x50000

int CTvPanel::PANEL_55SDC_data_Conversion(const char *partition_name_Table_data,const char *partition_name_Register) {
    ALOGD("%s: start create", __FUNCTION__);
    int ret = -1;
    //int base = 0x50000;
    unsigned char check_sum_para = 0;
    unsigned char check_sum_mura = 0;
    unsigned char check = 0;
    unsigned char check_p = 0;
    int demura_check_status = 0;
    int demura_check_count = 0;
    int demura_p_check_status = 0;
    int demura_p_check_count = 0;

    unsigned char *Demura_Data = NULL;
    unsigned char *flash_demura = NULL;
    unsigned char gain[7] = {0};
    unsigned char tmp[129]= {0};

    memset(tmp,0,129);

    Demura_Data = (unsigned char *)malloc(sizeof(unsigned char) * DEMURA_LEN);
    if (Demura_Data == NULL) {
        LOGE("%s,demura_data malloc failed\n", __FUNCTION__);
        return -1;
    }

    flash_demura = (unsigned char *)malloc(sizeof(unsigned char) * DEMURA_FLASH_LEN);
    if (flash_demura == NULL) {
        free(Demura_Data);
        Demura_Data = NULL;
        LOGE("%s,flash_demura malloc failed\n", __FUNCTION__);
        return -1;
    }

    memset(Demura_Data, 0, DEMURA_LEN);
    memset(flash_demura, 0, DEMURA_FLASH_LEN);

    for (int j = 0; j < 3; j++) {
        memset(flash_demura,0,DEMURA_FLASH_LEN);
        check = 0;
        check_sum_mura = 0;

        for (int i = 0; i < 892; i++) {
            if (i == 891) {
                tvSpiRead((DEMURA_FLASH_ADDR+1024*i),81,(flash_demura+1024*i));
            } else {
                tvSpiRead((DEMURA_FLASH_ADDR+1024*i),1024,(flash_demura+1024*i));
            }
        }
        for (int i = 0; i< (DEMURA_FLASH_LEN - 1); i++) {
            check_sum_mura += flash_demura[i];
        }
        check = 0xFC - check_sum_mura;
        tvSpiRead(0x12FC50,1,(flash_demura+912464));
        demura_check_count++;

        if (flash_demura[912464] == check) {
            demura_check_status = 1;
            LOGD("%s,demura_check_ok,check_count:%d",__FUNCTION__, demura_check_count);
            break;
        }
    }

    if (demura_check_status == 0) {
        LOGD("%s,demura_check_fail, check_count:%d",__FUNCTION__, demura_check_count);
        free(Demura_Data);
        Demura_Data = NULL;
        free(flash_demura);
        flash_demura = NULL;
        return -1;
    }

   //read gain value;
    for (int j = 0; j < 3; j++) {
        memset(tmp,0,129);
        check_sum_para = 0;
        check_p = 0;
        tvSpiRead(GAIN_FLASH_ADDR, 129, tmp);
        for (int i = 0;i <128;i++) {
            check_sum_para += tmp[i];
        }
        demura_p_check_count++;
        check_p = 0xFC - check_sum_para;
    //  ALOGD("%s,check_sum_para:%x,check_value:%x",__FUNCTION__,check_sum_para,tmp[128]);
        if (tmp[128] == check_p) {
            demura_p_check_status = 1;
            LOGD("%s,demura_p_check_ok,check_count:%d",__FUNCTION__, demura_p_check_count);
            break;
        }
    }

    if (demura_p_check_status == 0) {
        LOGD("%s,demura_p_check_fail, check_count:%d",__FUNCTION__, demura_p_check_count);
        free(Demura_Data);
        Demura_Data = NULL;
        free(flash_demura);
        flash_demura = NULL;
        return -1;
    }

    gain[6] = ((tmp[0] >> 0) & 1)*2 + ((tmp[1] >> 7) & 1);
    gain[5] = ((tmp[1] >> 6) & 1)*2 + ((tmp[1] >> 5) & 1);
    gain[4] = ((tmp[1] >> 4) & 1)*2 + ((tmp[1] >> 3) & 1);
    gain[3] = ((tmp[1] >> 2) & 1)*2 + ((tmp[1] >> 1) & 1);
    gain[2] = ((tmp[1] >> 0) & 1)*2 + ((tmp[2] >> 7) & 1);
    gain[1] = ((tmp[2] >> 6) & 1)*2 + ((tmp[2] >> 5) & 1);
    gain[0] = ((tmp[2] >> 4) & 1)*2 + ((tmp[2] >> 3) & 1);
    //ALOGD("%s,gain1:%d,gain2:%d,gain3:%d,gain4:%d,gain5:%d,gain6:%d,gain7:%d",__FUNCTION__,gain[0],gain[1],gain[2],gain[3],gain[4],gain[5],gain[6]);

    //j: panel[j]   i: panelx_N[i]
    for (int j = 0; j < 7; j++) {
        for (int i = 0; i < 130351; i++) {
            if (flash_demura[i+130352*j] > 127) {
                Demura_Data[(i+1)*7-1-j] = ((float)flash_demura[i+130352*j]-256)*255*(1 << gain[6-j])/1024+256;
            } else {
                Demura_Data[(i+1)*7-1-j] = (float)flash_demura[i+130352*j]*255*(1 << gain[6-j])/1024;
            }
        }
    }

    free(flash_demura);
    flash_demura = NULL;

    //ALOGD("%s,check_sum_mura:%x,mura_value:%x", __FUNCTION__, check_sum_mura, flash_demura[912464]);
    ret = system("mount -w -o remount /dev/block/vendor /vendor");

    //write into demura.bin
    if (PANEL_Flash_Write(partition_name_Table_data, 0, Demura_Data, DEMURA_LEN) == -1) {
        free(Demura_Data);
        Demura_Data = NULL;
        return -1;
    }

    free(Demura_Data);
    Demura_Data = NULL;

    //handle register
    unsigned char Register[159]={0x00,0x00,0x17,0x33,0x1e,0x11,0x0f,0x00,0x00,0x00,0x40,0x01,0x00,0x01,0x80,0x02,
            0x00,0x04,0x00,0x08,0x00,0x0c,0x00,0x0f,0xf0,0x00,0x00,0x0f,0xfc,0x05,0x4f,0x08,
            0x00,0x08,0x00,0x02,0x00,0x01,0x00,0x01,0x00,0x01,0x04,0x4e,0xc5,0x00,0x00,0x44,
            0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x00,0x00,0x00,0x00,0x00,
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x80,0x80,0x00,0x00,
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

    //write into reg.bin
    if (PANEL_Flash_Write(partition_name_Register, 0, Register, 159) == -1) {
        return -1;
    }

    ret = system("mount -r -o remount /dev/block/vendor /vendor");
    LOGD("%s finish",__FUNCTION__);
    return 0;
}


#define ACC_FLASH_LEN   6145        //ACC_LEN++checksum
int CTvPanel::PANEL_55SDC_Acc_Conversion(const char *partition_name_Table_ACC) {
    ALOGD("%s: start create", __FUNCTION__);

    int ret = -1;
    int checksum1 = 0;
    int checksum2 = 0;
    unsigned char *ACC_Data = NULL;
    int *ACC_Data_12 = NULL;
    int Data[771] = {0};
    int Data2[771] = {0};
    unsigned char result[1161] = {0};

    ACC_Data = (unsigned char *)malloc(sizeof(unsigned char)*ACC_FLASH_LEN);
    if (ACC_Data == NULL) {
        LOGE("%s,acc data malloc fail\n", __FUNCTION__);
        return -1;
    }

    ACC_Data_12 = (int *)malloc(sizeof(int)*3072);
    if (ACC_Data_12 == NULL) {
        LOGE("%s,ACC_Data_12 malloc fail\n", __FUNCTION__);
        free(ACC_Data);
        ACC_Data = NULL;
        return -1;
    }

    //read acc data in flash
    ret = tvSpiRead(0x1FA000,ACC_FLASH_LEN, ACC_Data);
    if (ret < 0) {
        LOGE("%s,AML_HAL_SPI_Read Failed!\n", __FUNCTION__);
        free(ACC_Data_12);
        ACC_Data_12 = NULL;
        free(ACC_Data);
        ACC_Data = NULL;
        return -1;
    }


    //CheckSum value +  mod(sum(0x1FA000:0x1FB7FF), 256) =  0xFC
    for (int i = 0; i < ACC_FLASH_LEN - 1; i++) {
        checksum1 += ACC_Data[i];
    }
    checksum2 = checksum1 % 256;

    LOGD("%s checksum1: %d, checksum2: %d,check_value: %d, acc_lut[6142]:%d", __FUNCTION__, checksum1, checksum2,ACC_Data[6144],ACC_Data[6143]);

    unsigned char check = ACC_Data[ACC_FLASH_LEN - 1] + checksum2;
    if (check == 0xFC) {
        LOGD("%s,check right",__FUNCTION__);
    }else{
        LOGD("%s,check fail,remove acc data", __FUNCTION__);
        if (remove(partition_name_Table_ACC) == 0) {
            LOGD("%s,remove acc bin file success", __FUNCTION__);
        } else {
            LOGD("%s,remove fail", __FUNCTION__);
        }
        free(ACC_Data);
        ACC_Data = NULL;
        free(ACC_Data_12);
        ACC_Data_12 = NULL;
        return -1;
    }

    //change acc data format to 12bit
    for (int i = 0; i < 3072; i++) {
        ACC_Data_12[i] = ACC_Data[2*i+1]*256+ACC_Data[i*2];
    }

    //sample 1/4 data and fill 257th data with 0xfff
    //int step = 0;
    for (int i = 0; i < 768; i++) {
        Data[i] = ACC_Data_12[i*4];
    }

    for (int i = 0; i < 256; i++) {
        Data2[i] = Data[i];
    }

    if (Data2[255] == 0) {
        Data2[256] = 0xfff;
    } else {
        Data2[256] = Data2[255];
    }

    for (int i = 256; i < 512; i++) {
        Data2[i+1] = Data[i];
    }

    if (Data2[512] == 0) {
        Data2[513] = 0xfff;
    } else {
        Data2[513] = Data2[512];
    }

    for (int i = 512; i < 768; i++) {
        Data2[i+2] = Data[i];
    }

    if (Data2[769] == 0) {
        Data2[770] = 0xfff;
    } else {
        Data2[770] = Data2[769];
    }
    //change 12bit acc data format to 8bit
    for (int j = 0;j<3;j++) {
        for (int i = 0; i < 257; i++) {
            if (i%2 == 0) {
                result[(i/2)*3+j*387] = Data2[i+j*257]&0xff;
                result[(i/2)*3+1+j*387] = Data2[i+j*257]>>8;
            } else if (i%2==1) {
                result[(i/2)*3+1+j*387] = (result[(i/2)*3+1+j*387]&0xf) | ((Data2[i+257*j]&0xf) << 4);
                result[(i/2)*3+2+j*387] = Data2[i+257*j]>>4;
            }
        }

    }

    result[386] = 0;
    result[773] = 0;
    result[1160] = 0;

    free(ACC_Data);
    ACC_Data = NULL;
    free(ACC_Data_12);
    ACC_Data_12 = NULL;
    ret = system("mount -w -o remount /dev/block/vendor /vendor");

    if (PANEL_Flash_Write(partition_name_Table_ACC, 0, result, 1161) == -1) {
        return -1;
    }

    ret = system("mount -r -o remount /dev/block/vendor /vendor");

    LOGD("%s finish",__FUNCTION__);
    return 0;
}

int CTvPanel::PANEL_AutoFlicker(void) {
    LOGD("%s: start ", __FUNCTION__);
    int ret = -1;
    int dvr = 0;
    unsigned char buf[2];
    unsigned char tmp[8];
    int data[4];

    //read Dvr value;
    ret=tvSpiRead(0x10000,8, tmp);
    if (ret < 0) {
        LOGE("%s,AML_HAL_SPI_Read flicker Failed!\n", __FUNCTION__);
        return -1;
    }

    int i2c_fd = open(DVR_I2C_dev, O_RDWR);
    if (i2c_fd < 0) {
        LOGD("%s,open i2c error\n", __FUNCTION__);
        return -1;
    }

    ioctl(i2c_fd, I2C_SLAVE, DVR_ADDR);
    memset(buf,0,1);
    if (read(i2c_fd, buf, 1) == -1) {
        LOGD("%s,read i2c error\n", __FUNCTION__);
    }
    dvr = buf[0];
    LOGD("%s,exist dvr value is:%x\n", __FUNCTION__,dvr);

    data[0] = tmp[0]*256 + tmp[1];
    data[1] = tmp[2]*256 + tmp[3];
    data[2] = tmp[4]*256 + tmp[5];
    data[3] = tmp[6]*256 + tmp[7];

    LOGD("%s,data[0]:0x%x,ADDR:0x%x,checksum:0x%x 0x%x,DVR:0x%x,0x%x",__FUNCTION__,data[0],data[1],tmp[4],tmp[5],tmp[6],tmp[7]);
    if (data[1]+data[3]+data[0] == data[2]) {
        LOGD("%s, auto flicker check ok\n", __FUNCTION__);
        if (dvr == data[3]) {
            LOGD("%s, PMIC value is right,no need to set",__FUNCTION__);
        } else {
            buf[0] = data[3];
            if (write(i2c_fd,buf,1) == -1) {
                 LOGD("%s,write i2c error\n", __FUNCTION__);
            }
            LOGD("%s, PMIC value write:0x%x",__FUNCTION__,buf[0]);
            buf[0] = 0;
            if (read(i2c_fd, buf, 1) == -1) {
                LOGD("%s,check i2c error\n", __FUNCTION__);
            }
            LOGD("%s, PMIC read check:0x%x",__FUNCTION__,buf[0]);
        }
        close(i2c_fd);
        return 0;
    } else {
        LOGD("%s, auto flicker check fail\n", __FUNCTION__);
        close(i2c_fd);
        return -1;
    }
}

int CTvPanel::PANEL_SPI_PGAMMA(int spi_offset, int spi_len ,const char *spi_bin_path) {
    if (NULL == spi_bin_path) {
        LOGE("%s,spi_bin_path OR default_bin_path is NULL!\n", __FUNCTION__);
        return -1;
    }

    //int ret_open = -1;

    LOGD("%s,p_gamma len = %d.\n", __FUNCTION__,spi_len);
    unsigned char buffer[PANEL_BUFFER_MAX_LEN] = {0x0};
    int ret = tvSpiRead(spi_offset, spi_len, buffer);
    if (ret < 0) {
        LOGE("%s,Read spi falsh failed!\n", __FUNCTION__);
        PANEL_REMOVE_BIN_FILE(1);
        return -1;
    }
    if (buffer[0] == 0x0 && buffer[1] == 0x0 && buffer[2] == 0x0 && buffer[3] == 0x0) {
        LOGE("%s,No p_gamma data in flash,use defult data!\n", __FUNCTION__);
        PANEL_REMOVE_BIN_FILE(1);
        return 0;
    }
    if (buffer[0] == 0xff && buffer[1] == 0xff && buffer[2] == 0xff && buffer[3] == 0xff) {
        if (buffer[40] ==0xff && buffer[41] == 0xff) {
            LOGE("%s,No p_gamma data in flash,use defult data!\n", __FUNCTION__);
            PANEL_REMOVE_BIN_FILE(1);
            return 0;
        }
    }
    unsigned long crc_ret = CalcCRC16((unsigned char*)buffer,40);
    unsigned char crc_low =(unsigned char) (crc_ret & 0x00FF);
    unsigned char crc_heignt = (unsigned char)(crc_ret >> 8);
    if (crc_low == buffer[41] && crc_heignt == buffer[40]) {
        LOGE("%s,Crc check pass!\n", __FUNCTION__);
    } else {
        LOGE("%s,Crc check not pass,use defult gamma data!\n", __FUNCTION__);
        PANEL_REMOVE_BIN_FILE(1);
        return -1;
    }
    if (access((const char*)P2P_PGAMMA_PATH, F_OK) == 0) {
        unsigned char check_buffer[42] = {0x0};
        ret = PANEL_Flash_Read(P2P_PGAMMA_PATH, 0, check_buffer, 42);
        if (ret == -1) {
            LOGE("%s,Failed to read default data!\n", __FUNCTION__);
            PANEL_REMOVE_BIN_FILE(1);
            return -1;
        }

        LOGD("%s,check_buffer[40] = 0x%x,check_buffer[41] = 0x%x,buffer_res[40] = 0x%x,buffer_res[41] = 0x%x!\n",
                                __FUNCTION__,check_buffer[40],check_buffer[41],buffer[40],buffer[41]);
        if (check_buffer[40] == buffer[40] && check_buffer[41] == buffer[41]) {
            LOGD("%s,p_gamma.bin is exist,no need to  build!\n", __FUNCTION__);
            return 0;
        }
    }
    system("mount -w -o remount /dev/block/vendor /vendor");
    ret = PANEL_Flash_Write((char*)spi_bin_path, 0, buffer, spi_len);
    if (ret < 0) {
        LOGE("%s,Write p_gamma data to bin file failed!\n", __FUNCTION__);
        return -1;
    } else {
        LOGD("%s,Write p_gamma data to bin file successed!\n", __FUNCTION__);
    }
    system("mount -r -o remount /dev/block/vendor /vendor");
    return 0;
}


/*for 65D02-H and 55d09 and 50T01*/
int CTvPanel::PANEL_SPI_PGAMMA_SPEC(int spi_offset, int spi_len ,const char *spi_bin_path, const char *default_bin_path) {
    if (NULL == spi_bin_path || NULL == default_bin_path) {
        LOGE("%s,spi_bin_path OR default_bin_path is NULL!\n", __FUNCTION__);
        return -1;
    }

    LOGD("%s,p_gamma len = %d.\n", __FUNCTION__,spi_len);
    unsigned char buffer[PANEL_BUFFER_MAX_LEN] = {0x0};
    int ret = tvSpiRead(spi_offset, spi_len, buffer);
    if (ret < 0) {
        LOGE("%s,Read spi falsh failed!\n", __FUNCTION__);
        PANEL_REMOVE_BIN_FILE(1);
        return -1;
    }
    if (buffer[0] == 0x0 && buffer[1] == 0x0 && buffer[2] == 0x0 && buffer[3] == 0x0) {
        LOGE("%s,No p_gamma data in flash,use defult data!\n", __FUNCTION__);
        PANEL_REMOVE_BIN_FILE(1);
        return 0;
    }
    if (buffer[0] == 0xff && buffer[1] == 0xff && buffer[2] == 0xff && buffer[3] == 0xff) {
        LOGE("%s,No p_gamma data in flash,use defult data!\n", __FUNCTION__);
        PANEL_REMOVE_BIN_FILE(1);
        return 0;
    }
    unsigned char crc_check[23] = {0x0};
    for (int i = 0;i < 23;i++) {
        crc_check[i] = buffer[i];
    }
    unsigned long crc_ret = CalcCRC16((unsigned char*)crc_check,23);
    unsigned char crc_low =(unsigned char) (crc_ret & 0x00FF);
    unsigned char crc_heignt = (unsigned char)(crc_ret >> 8);
    if (crc_low == buffer[24] && crc_heignt == buffer[23]) {
        LOGE("%s,Crc check pass!\n", __FUNCTION__);
    } else {
        LOGE("%s,Crc check not pass,use defult gamma data!\n", __FUNCTION__);
        PANEL_REMOVE_BIN_FILE(1);
        return -1;
    }
    unsigned char buffer_crc[40] = {0};
    unsigned char buffer_res[42] = {0};
    unsigned char buffer_1[40] = {0x0};
    char default_path_ini[128] = {'\0'};
    if (getBootEnv(UBOOTENV_MODEL_TCON_GAMMA, default_path_ini,P_GAMMA_DEFAULT_PATH_50T01) == 0) {
    //if (AML_HAL_READ_Env("ubootenv.var.model_tcon_default_pgamma_path", default_path_ini) == 0) {
        if (access((const char*)default_path_ini, F_OK) == 0) {
            ALOGD("%s, use %s in panel ini!\n", __FUNCTION__,default_path_ini);
            ret = PANEL_Flash_Read((char *)default_path_ini, 0, buffer_1, 40);
            if (ret == -1) {
                LOGE("%s,Failed to read default data!\n", __FUNCTION__);
                PANEL_REMOVE_BIN_FILE(1);
                return -1;
            }
        } else {
            ALOGD("%s, use default path %s!\n", __FUNCTION__,default_bin_path);
            ret = PANEL_Flash_Read((char *)default_bin_path, 0, buffer_1, 40);
            if (ret == -1) {
                LOGE("%s,Failed to read default data!\n", __FUNCTION__);
                PANEL_REMOVE_BIN_FILE(1);
                return -1;
            }
        }
    } else {
        LOGD("%s, use default path %s!\n", __FUNCTION__,default_bin_path);
        ret = PANEL_Flash_Read((char *)default_bin_path, 0, buffer_1, 40);
        if (ret == -1) {
            LOGE("%s,Failed to read default data!\n", __FUNCTION__);
            PANEL_REMOVE_BIN_FILE(1);
            return -1;
        }
    }
    for (int w = 0;w < 16;w++) {
        buffer_crc[w] = (unsigned char)buffer_1[w];
    }
    int j = 16;
    int i;
    for (i=0;i<22;i++) {
        buffer_crc[j] = buffer[i];
        j++;
    }
    unsigned char height;
    unsigned char low;
    unsigned char b = buffer_1[38];
    low = b << 4;
    low = low >> 4;
    height = buffer[22] >> 4;
    height = height << 4;
    buffer_crc[38] = height + low;
    buffer_crc[39] = buffer_1[39];
    unsigned long ret1 = CalcCRC16((unsigned char *) buffer_crc,40);
    for (i = 0;i < 40;i++) {
        buffer_res[i] = buffer_crc[i];
    }
    buffer_res[40] = (unsigned char)(ret1 >> 8);
    buffer_res[41] = (unsigned char)(ret1 & 0x00FF);

    unsigned char check_buffer[42] = {0x0};

    if (access(P2P_PGAMMA_PATH, F_OK) == 0 ) {
        ret = PANEL_Flash_Read(P2P_PGAMMA_PATH, 0, check_buffer, 42);
        if (0 > ret) {
            LOGD("%s,Read %s failed!\n", __FUNCTION__,P2P_PGAMMA_PATH);
            return -1;
        }
        LOGD("%s,check_buffer[40] = 0x%x,check_buffer[41] = 0x%x,buffer_res[40] = 0x%x,buffer_res[41] = 0x%x!\n",
                                __FUNCTION__,check_buffer[40],check_buffer[41],buffer_res[40],buffer_res[41]);
        if (check_buffer[40] == buffer_res[40] && check_buffer[41] == buffer_res[41]) {
            LOGD("%s,p_gamma.bin is exist,no need to build!\n", __FUNCTION__);
            return 0;
        }
    }

    ret = PANEL_Flash_Write((char*)spi_bin_path, 0, buffer_res, 42);
    if (ret < 0) {
        LOGE("%s,Write p_gamma data to bin file failed!\n", __FUNCTION__);
        return -1;
    } else {
        LOGD("%s,Write p_gamma data to bin file successed!\n", __FUNCTION__);
    }
    return 0;
}


/*for 65D02-3 */
int CTvPanel::PANEL_SPI_PGAMMA_SPEC_65D02_3(int spi_offset, int spi_len ,const char *spi_bin_path, const char *default_bin_path) {

    if (spi_bin_path == NULL || default_bin_path == NULL) {
        LOGE("%s,illegal parameter\n", __FUNCTION__);
        return -1;
    }

    int crc_check_len = spi_len - 2;

    LOGD("%s,p_gamma len = %d.\n", __FUNCTION__,spi_len);
    unsigned char buffer[PANEL_BUFFER_MAX_LEN] = {0x0};
    int ret = tvSpiRead(spi_offset, spi_len, buffer);
    if (ret < 0) {
        LOGE("%s,Read spi flash failed!\n", __FUNCTION__);
        PANEL_REMOVE_BIN_FILE(1);
        return -1;
    }

    unsigned char flash_crc_high = buffer[spi_len-2];
    unsigned char flash_crc_low = buffer[spi_len-1];
    ALOGE("%s,flash_crc_high:0x%x,flash_crc_low:0x%x,crc_check_len:%d!\n", __FUNCTION__, flash_crc_high, flash_crc_low, crc_check_len);
    if (buffer[0] == 0x0 && buffer[1] == 0x0 && buffer[2] == 0x0 && buffer[3] == 0x0) {
        PANEL_REMOVE_BIN_FILE(1);
        return 0;
    }
    if (buffer[0] == 0xff && buffer[1] == 0xff && buffer[2] == 0xff && buffer[3] == 0xff) {
        PANEL_REMOVE_BIN_FILE(1);
        return 0;
    }
    unsigned char crc_check[PANEL_BUFFER_MAX_LEN] = {0x0};
    for (int i = 0;i < crc_check_len;i++) {
        crc_check[i] = buffer[i];
    }
    unsigned long crc_ret = CalcCRC16((unsigned char*)crc_check,crc_check_len);
    unsigned char crc_low =(unsigned char) (crc_ret & 0x00FF);
    unsigned char crc_heignt = (unsigned char)(crc_ret >> 8);
    if (crc_low == flash_crc_low && crc_heignt == flash_crc_high) {
        LOGE("%s,Crc check pass!\n", __FUNCTION__);
    } else {
        PANEL_REMOVE_BIN_FILE(1);
        return -1;
    }
    unsigned char buffer_crc[40] = {0};
    unsigned char buffer_res[42] = {0};
    unsigned char buffer_1[40] = {0x0};
    char default_path_ini[128] = {'\0'};
    if (getBootEnv(UBOOTENV_MODEL_TCON_GAMMA, default_path_ini,P_GAMMA_DEFAULT_PATH_50T01) == 0) {
        if (access((const char*)default_path_ini, F_OK) == 0) {
            LOGD("%s, use %s in panel ini!\n", __FUNCTION__,default_path_ini);
            ret = PANEL_Flash_Read((char *)default_path_ini, 0, buffer_1, 40);
            if (ret == -1) {
                LOGE("%s,Failed to read default data!\n", __FUNCTION__);
                PANEL_REMOVE_BIN_FILE(1);
                return -1;
            }
        }else{
            LOGD("%s, use default path %s!\n", __FUNCTION__,default_bin_path);
            ret = PANEL_Flash_Read((char *)default_bin_path, 0, buffer_1, 40);
            if (ret == -1) {
                LOGE("%s,Failed to read default data!\n", __FUNCTION__);
                PANEL_REMOVE_BIN_FILE(1);
                return -1;
            }
        }
    } else {
        LOGD("%s, use default path %s!\n", __FUNCTION__,default_bin_path);
        ret = PANEL_Flash_Read((char *)default_bin_path, 0, buffer_1, 40);
        if (ret == -1) {
            LOGE("%s,Failed to read default data!\n", __FUNCTION__);
            PANEL_REMOVE_BIN_FILE(1);
            return -1;
        }
    }
    for (int w = 0;w < 16;w++) {
        buffer_crc[w] = (unsigned char)buffer_1[w];
    }
    int j = 16;
    int i;
    for (i=16;i<38;i++) {
        buffer_crc[j] = buffer[i];
        j++;
    }
    unsigned char height;
    unsigned char low;
    unsigned char b = buffer_1[38];
    low = b << 4;
    low = low >> 4;
    height = buffer[38] >> 4;
    height = height << 4;
    buffer_crc[38] = height + low;
    buffer_crc[39] = buffer_1[39];
    unsigned long ret1 = CalcCRC16((unsigned char *) buffer_crc,40);
    for (i = 0;i < 40;i++) {
        buffer_res[i] = buffer_crc[i];
    }
    buffer_res[40] = (unsigned char)(ret1 >> 8);
    buffer_res[41] = (unsigned char)(ret1 & 0x00FF);

    unsigned char check_buffer[42] = {0x0};

    if (access((const char*)P2P_PGAMMA_PATH, F_OK) == 0 ) {
        PANEL_Flash_Read(P2P_PGAMMA_PATH, 0, check_buffer, 42);
        LOGD("%s,check_buffer[40] = 0x%x,check_buffer[41] = 0x%x,buffer_res[40] = 0x%x,buffer_res[41] = 0x%x!\n",
                __FUNCTION__,check_buffer[40],check_buffer[41],buffer_res[40],buffer_res[41]);
        if (check_buffer[40] == buffer_res[40] && check_buffer[41] == buffer_res[41]) {
            LOGD("%s,p_gamma.bin is exist,no need to  build!\n", __FUNCTION__);
            return 0;
        }
    }

    ret = system("mount -w -o remount /dev/block/vendor /vendor");
    ret = PANEL_Flash_Write((char*)spi_bin_path, 0, buffer_res, 42);
    if (ret < 0) {
        LOGE("%s,Write p_gamma data to bin file failed!\n", __FUNCTION__);
        return -1;
    } else {
        LOGD("%s,Write p_gamma data to bin file successed!\n", __FUNCTION__);
    }
    ret = system("mount -r -o remount /dev/block/vendor /vendor");
    return 0;
}


int CTvPanel::PANEL_Decode_50inch(int *Code,int *Mode,int range,int min,int *DeLut) {
/*
    if (min > 127) {
        ALOGE("min out of 127,min is 0x%x\n",min);
        return -1;
    }
    if (range > 127) {
        ALOGE("range out of 127,range is 0x%x\n",range);
        return -1;
    }
    for (int i = 0;i < 4;i++) {
        if (Mode[i] > 7)
            ALOGE("Mode out of 7,mode[%d] = 0x%x\n",i,Mode[i]);
        return -1;
    }
    for (int i = 0;i < 16;i++) {
        if (Code[i] > 0xf)
            ALOGE("Code out of 0xf,Code[%d] = 0x%x\n",i,Code[i]);
        return -1;
    }
*/
    double Min = (double)min;
    double Range = (double)range;
    double M, N, Q;
    double subRange1 = (int)((Range * 1 / 4) + Min + 0.5);
    double subRange2 = (int)((Range * 2 / 4) + Min + 0.5);
    double subRange3 = (int)((Range * 3 / 4) + Min + 0.5);
    int mode;
    int m,i;
    for (i = 0;i < 4;i++) {
        mode = Mode[i];
        switch (mode) {
        case 0:
            M = Min+Range;
            N = Min;
        break;

        case 1:
            M = subRange3;
            N = Min;
        break;

        case 2:
            M = subRange2;
            N = Min;
        break;

        case 3:
            M = subRange1;
            N = Min;
        break;

        case 4:
            M = Min + Range;
            N = subRange1;
        break;

        case 5:
            M = Min + Range;
            N = subRange2;
        break;

        case 6:
            M = Min + Range;
            N = subRange3;
        break;

        case 7:
            M = subRange3;
            N = subRange1;
        break;

        default:
            M = Min + Range;
            N = Min;
        break;
        }
        Q = M - N;
        for (m = 0;m < 4;m++) {
            if (Q != 0) {
                DeLut[m+i*4] = (int)(((double)Code[m+i*4] * 546 / 8192) * Q + N + 0.5);
                DeLut[m+i*4] = DeLut[m+i*4] - 64;
            } else {
                DeLut[m+i*4] = (int)(N - 64);
            }
            if (DeLut[m+i*4] < 0) {
                DeLut[m+i*4] = DeLut[m+i*4] + 256;
            }
        }
    }
    return 0;
}

int CTvPanel::PANEL_BIN_FILE_CEHCK(int address_1,int address_2 __unused, int address_3 __unused, int address_4 __unused, int len_1, int len_2,int len_3, int len_4) {
    ALOGD("%s,addr_1 = 0x%x, len_1 = 0x%x!\n", __FUNCTION__,address_1,len_1);
    int i;
    int ret = -1;
    unsigned char crc_data[128] = {0x0};
    unsigned int data_crc = 0;
    unsigned int crc_data_int[128] = {0x0};
    unsigned char crc_check[128] = {0x0};
    unsigned int check_crc = 0;
    unsigned int crc_check_int[128] = {0x0};
    unsigned char crc_sdc_data[4]= {0x0};
    unsigned char crc_sdc_check[4]= {0x0};

    if (len_1 != 0 && len_2 == 0 && len_3 == 0 && len_4 == 0) {
        ret = tvSpiRead(address_1, len_1, crc_data);
        if (ret < 0)
        {
            LOGE("%s,AML_HAL_SPI_Read1 Failed!\n", __FUNCTION__);
            return -1;
        }
        for (i = 0;i < len_1;i++) {
            crc_data_int[i] = (unsigned int)crc_data[i];
        }

        for (i = 0;i < len_1;i++) {
            data_crc = (unsigned int)((unsigned int)(crc_data_int[i] << (8 * (len_1 - i -1))) + data_crc);
        }
        ret = PANEL_Flash_Read(CRC_CHECK_PATH, 0x0, crc_check, len_1);
        for (i = 0;i < len_1;i++) {
            crc_check_int[i] = (unsigned int)crc_check[i];
        }
        for (i = 0;i < len_1;i++) {
            check_crc = (unsigned int)((unsigned int)(crc_check_int[i] << (8 * (len_1 - i -1))) + check_crc);
        }
        LOGD("%s,crc in flash is 0x%x, crc in bin file is 0x%x!\n", __FUNCTION__,data_crc,check_crc);
        if (check_crc == data_crc) {
            LOGD("%s,crc is same, the data is for this panel!\n", __FUNCTION__);
            return 0;
        } else {
            LOGD("%s,crc is not same, rebuild bin file!\n", __FUNCTION__);
            PANEL_REMOVE_BIN_FILE(0);
            return -1;
        }
    } else if ( len_1 != 0 && len_2 != 0 && len_3 != 0 && len_4 == 0) {
        LOGD("%s,crc_sdc check,len1:%x,len2:%x,len3:%x\n", __FUNCTION__, len_1, len_2, len_3);

        ret = tvSpiRead(0x10004, 2, crc_sdc_data);
        if (ret < 0) {
            LOGD("%s,AML_HAL_SPI_Read1 Failed!\n", __FUNCTION__);
            return -1;
        }
        ret = tvSpiRead(0x12fc50, 1, crc_sdc_data+2);
        if (ret < 0) {
            LOGD("%s,AML_HAL_SPI_Read2 Failed!\n", __FUNCTION__);
            return -1;
        }
        ret = tvSpiRead(0x1FB800, 1, crc_sdc_data+3);
        if (ret < 0) {
            LOGD("%s,AML_HAL_SPI_Read3 Failed!\n", __FUNCTION__);
            return -1;
        }
        ret = PANEL_Flash_Read(CRC_CHECK_PATH, 0x0, crc_sdc_check, 4);
        if (crc_sdc_check[0] == crc_sdc_data[0] && crc_sdc_check[1] == crc_sdc_data[1] && crc_sdc_check[2] == crc_sdc_data[2] && crc_sdc_check[3] == crc_sdc_data[3]) {
            LOGD("%s,crc_sdc is same, the data is for this panel!\n", __FUNCTION__);
            return 0;
        } else {
            LOGD("%s,crc_sdc changed, crc_sdc:0x%x,0x%x,0x%x,0x%x,crc_check:0x%x,0x%x,0x%x,0x%x!\n", __FUNCTION__,
            crc_sdc_data[0], crc_sdc_data[1], crc_sdc_data[2],crc_sdc_data[3],crc_sdc_check[0],crc_sdc_check[1],crc_sdc_check[2],crc_sdc_check[3]);
            LOGD("%s,crc is not same, rebuild bin file!\n", __FUNCTION__);
            PANEL_REMOVE_BIN_FILE(0);
            return -1;
        }
    }
    return -1;
}


/*43 crc check*/
unsigned short CTvPanel::PANEL_43_CRC_Check(unsigned int *addr, int num) {
    int  i, j;
    unsigned int   data_int;
    unsigned int   data_temp;
    unsigned short data_short;
    unsigned short crc_data = 0xffff;

    while (num--)
    {
        data_int = *addr;
        for (j = 4; j > 0; j--)
        {
            {
                data_temp  = data_int >> 8 * (j - 1);
                data_short = data_temp << 8;
                crc_data = crc_data ^ data_short;
                for (i = 0; i < 8; i++)
                {
                    if (crc_data & 0x8000)
                    {
                        crc_data = (crc_data << 1) ^ 0x8005;
                    }
                    else
                    {
                        crc_data <<= 1;
                    }
                }
                crc_data &= 0xFFFF;
            }
        }
        addr++;
    }
    return crc_data;
}


#define   LUT_SIZE_65_CRC        590238
#define   LUT_SIZE_65_CRC_HALF  295119
unsigned short CTvPanel::PANEL_65D02_H_DEMURA_CRC(unsigned char *buffer,unsigned int data_size) {
    int i = 0;
    //unsigned short cat[LUT_SIZE_65_CRC_HALF] = {0x0};
    int data_size_half = data_size/2;
    unsigned short *cat = NULL;
    cat = (unsigned short *)malloc(data_size_half*sizeof(unsigned short));
    if (cat == NULL) {
        LOGE("%s,Malloc memory failed!\n", __FUNCTION__);
        return -1;
    }
    //unsigned short buffer_1[LUT_SIZE_65_CRC] = {0x0};
    unsigned short *buffer_1 = NULL;
    buffer_1 = (unsigned short *)malloc(data_size*sizeof(unsigned short));
    if (buffer_1 == NULL) {
        LOGE("%s,Malloc memory failed!\n", __FUNCTION__);
        free(cat);
        cat = NULL;
        return -1;
    }
    int j = 0;
    while (data_size--) {
        buffer_1[j] = (unsigned short)buffer[j];
        i++;
    }
    unsigned short demura_crc = 0;
    int buffer_num = 0;
    while (data_size_half--) {
        cat[i] = buffer_1[buffer_num] * 256 + buffer_1[buffer_num + 1];
        demura_crc = (unsigned short)(demura_crc + cat[i]);
        buffer_num = buffer_num + 2;
        i++;
    }
    free(cat);
    cat = NULL;
    free(buffer_1);
    buffer_1 = NULL;
    return demura_crc;
}

unsigned char CTvPanel::PANEL_50INX_DEMURA_CRC(unsigned char *data, unsigned char *const crc_out, int crcStart, int crcLength) {
    unsigned char coef[8] = {1, 2, 4, 8, 16, 32, 64, 128};
    unsigned char shift[9];
    unsigned char crc[9];
    unsigned char din[9];
    for (int index = crcStart; index < crcStart + crcLength; index += 1)
    {

        for (int i = 0; i < 8; i += 1)
        {
            if ((*crc_out & coef[i]) != 0)
                crc[i] = 1;
            else
                crc[i] = 0;

            if ((data[index] & coef[i]) != 0)
                din[i] = 1;
            else
                din[i] = 0;
        }

        shift[1] = (unsigned char)(din[7] ^ crc[7]);
        shift[2] = (unsigned char)(din[6] ^ crc[6]);
        shift[3] = (unsigned char)(din[5] ^ crc[5]);
        shift[4] = (unsigned char)(din[4] ^ crc[4]);
        shift[5] = (unsigned char)(din[3] ^ crc[3] ^ shift[1]);
        shift[6] = (unsigned char)(din[2] ^ crc[2] ^ shift[1] ^ shift[2]);
        shift[7] = (unsigned char)(din[1] ^ crc[1] ^ shift[1] ^ shift[2] ^ shift[3]);
        shift[8] = (unsigned char)(din[0] ^ crc[0] ^ shift[2] ^ shift[3] ^ shift[4]);

        crc[7] = (unsigned char)(shift[1] ^ shift[3] ^ shift[4] ^ shift[5]);
        crc[6] = (unsigned char)(shift[2] ^ shift[4] ^ shift[5] ^ shift[6]);
        crc[5] = (unsigned char)(shift[3] ^ shift[5] ^ shift[6] ^ shift[7]);
        crc[4] = (unsigned char)(shift[4] ^ shift[6] ^ shift[7] ^ shift[8]);
        crc[3] = (unsigned char)(shift[5] ^ shift[7] ^ shift[8]);
        crc[2] = (unsigned char)(shift[6] ^ shift[8]);
        crc[1] = (unsigned char)(shift[7]);
        crc[0] = (unsigned char)(shift[8]);

        *crc_out = 0;

        for (int i = 0; i < 8; i += 1)
        {
            *crc_out += (unsigned char)(crc[i] * coef[i]);
        }
    }
    return 0;
}

/*crc for 50T01 demura*/
unsigned short CTvPanel::PANEL_50T01_DEMURA_CRC(unsigned char *buf, int length) {
    unsigned int i,j;
    unsigned short crc = 0x0;
    unsigned short crc_temp_1,crc_temp_2,crc_temp_3;
    unsigned char data_buffer;
    i = 0;
    while (length--) {
        data_buffer = buf[i++];
        crc_temp_1 = 0;
        crc_temp_2 = 0;
        crc_temp_3 = 0;
        for (j = 0;j < 8;j++) {
            crc_temp_1 = ((crc >> 15) ^ data_buffer) & 0x0001;
            crc_temp_2 = ((crc >> 1) ^ crc_temp_1) & 0x0001;
            crc_temp_3 = ((crc >> 14) ^ crc_temp_1) & 0x0001;
            crc = crc << 1;
            crc &= 0x7ffa;
            crc |= crc_temp_1;
            crc |= (crc_temp_2 << 2);
            crc |= (crc_temp_3 << 15);
            data_buffer >>= 1;
        }
    }
    return (crc);
}

/*for auto pgamma and demura data crc*/
unsigned short CTvPanel::CalcCRC16(unsigned char *dataBuffer,unsigned long len) {
    const unsigned short CRC16_TABLE[256] = {
        0x0000, 0x8005, 0x800F, 0x000A, 0x801B, 0x001E, 0x0014, 0x8011, 0x8033, 0x0036, 0x003C, 0x8039, 0x0028, 0x802D, 0x8027, 0x0022, 0x8063, 0x0066, 0x006C, 0x8069,
        0x0078, 0x807D, 0x8077, 0x0072, 0x0050, 0x8055, 0x805F, 0x005A, 0x804B, 0x004E, 0x0044, 0x8041, 0x80C3, 0x00C6, 0x00CC, 0x80C9, 0x00D8, 0x80DD, 0x80D7, 0x00D2,
        0x00F0, 0x80F5, 0x80FF, 0x00FA, 0x80EB, 0x00EE, 0x00E4, 0x80E1, 0x00A0, 0x80A5, 0x80AF, 0x00AA, 0x80BB, 0x00BE, 0x00B4, 0x80B1, 0x8093, 0x0096, 0x009C, 0x8099,
        0x0088, 0x808D, 0x8087, 0x0082, 0x8183, 0x0186, 0x018C, 0x8189, 0x0198, 0x819D, 0x8197, 0x0192, 0x01B0, 0x81B5, 0x81BF, 0x01BA, 0x81AB, 0x01AE, 0x01A4, 0x81A1,
        0x01E0, 0x81E5, 0x81EF, 0x01EA, 0x81FB, 0x01FE, 0x01F4, 0x81F1, 0x81D3, 0x01D6, 0x01DC, 0x81D9, 0x01C8, 0x81CD, 0x81C7, 0x01C2, 0x0140, 0x8145, 0x814F, 0x014A,
        0x815B, 0x015E, 0x0154, 0x8151, 0x8173, 0x0176, 0x017C, 0x8179, 0x0168, 0x816D, 0x8167, 0x0162, 0x8123, 0x0126, 0x012C, 0x8129, 0x0138, 0x813D, 0x8137, 0x0132,
        0x0110, 0x8115, 0x811F, 0x011A, 0x810B, 0x010E, 0x0104, 0x8101, 0x8303, 0x0306, 0x030C, 0x8309, 0x0318, 0x831D, 0x8317, 0x0312, 0x0330, 0x8335, 0x833F, 0x033A,
        0x832B, 0x032E, 0x0324, 0x8321, 0x0360, 0x8365, 0x836F, 0x036A, 0x837B, 0x037E, 0x0374, 0x8371, 0x8353, 0x0356, 0x035C, 0x8359, 0x0348, 0x834D, 0x8347, 0x0342,
        0x03C0, 0x83C5, 0x83CF, 0x03CA, 0x83DB, 0x03DE, 0x03D4, 0x83D1, 0x83F3, 0x03F6, 0x03FC, 0x83F9, 0x03E8, 0x83ED, 0x83E7, 0x03E2, 0x83A3, 0x03A6, 0x03AC, 0x83A9,
        0x03B8, 0x83BD, 0x83B7, 0x03B2, 0x0390, 0x8395, 0x839F, 0x039A, 0x838B, 0x038E, 0x0384, 0x8381, 0x0280, 0x8285, 0x828F, 0x028A, 0x829B, 0x029E, 0x0294, 0x8291,
        0x82B3, 0x02B6, 0x02BC, 0x82B9, 0x02A8, 0x82AD, 0x82A7, 0x02A2, 0x82E3, 0x02E6, 0x02EC, 0x82E9, 0x02F8, 0x82FD, 0x82F7, 0x02F2, 0x02D0, 0x82D5, 0x82DF, 0x02DA,
        0x82CB, 0x02CE, 0x02C4, 0x82C1, 0x8243, 0x0246, 0x024C, 0x8249, 0x0258, 0x825D, 0x8257, 0x0252, 0x0270, 0x8275, 0x827F, 0x027A, 0x826B, 0x026E, 0x0264, 0x8261,
        0x0220, 0x8225, 0x822F, 0x022A, 0x823B, 0x023E, 0x0234, 0x8231, 0x8213, 0x0216, 0x021C, 0x8219, 0x0208, 0x820D, 0x8207, 0x0202};
    unsigned char dat;
    unsigned long offset = 0;
    unsigned long crcData = 0;
    while (len--) {
        dat = (unsigned char)(crcData>>8);
        crcData <<= 8;
        crcData ^= CRC16_TABLE[dat^dataBuffer[offset++]];
    }
    return crcData;
}

/*
remove bin file of tconless
config == 0, remove all bin file of tconless(demura.bin * 2 + p_gamma.bin + demura_check.bin + acc .bin)
config == 1, remove only p_gamma.bin
*/
int CTvPanel::PANEL_REMOVE_BIN_FILE(int config) {
    int ret = system("mount -w -o remount /dev/block/vendor /vendor");
    LOGD("mount ret = %d\n", ret);
    if (0 == config) {
        if (-1 == remove(P2P_LUT_FILE_PATH)) {
            ALOGE("%s, remove %s failed!\n", __FUNCTION__, P2P_LUT_FILE_PATH);
        }
        if (-1 == remove(P2P_SET_FILE_PATH)) {
            ALOGE("%s, remove %s failed!\n", __FUNCTION__, P2P_SET_FILE_PATH);
        }
        if (-1 == remove(P2P_PGAMMA_PATH)) {
            ALOGE("%s, remove %s failed!\n", __FUNCTION__, P2P_PGAMMA_PATH);
        }
        if (-1 == remove(P2P_ACC_FILE_PATH)) {
            ALOGE("%s, remove %s failed!\n", __FUNCTION__, P2P_ACC_FILE_PATH);
        }
        if (-1 == remove(CRC_CHECK_PATH)) {
            ALOGE("%s, remove %s failed!\n", __FUNCTION__, CRC_CHECK_PATH);
        }
    }else if(1 == config){
        if (-1 == remove(P2P_PGAMMA_PATH)) {
            ALOGE("%s, remove %s failed!\n", __FUNCTION__, P2P_PGAMMA_PATH);
        }
    }
    ret = system("mount -r -o remount /dev/block/vendor /vendor");
    LOGD("unmount ret = %d\n", ret);
    return 0;
}

int CTvPanel::PANEL_CreatSdcCheckBin() {
    int ret_value = -1;
    int ret = -1;
    LOGD("%s,creat crc_sdc check bin \n", __FUNCTION__);
    unsigned char crc_sdc_data[4]= {0x0};

    ret = tvSpiRead( 0x10004, 2, crc_sdc_data);
    if (ret < 0) {
        LOGE("%s,tvSpiRead Failed!\n", __FUNCTION__);
        return -1;
    }

    ret = tvSpiRead( 0x12fc50, 1, crc_sdc_data+2);
    if (ret < 0) {
        LOGE("%s,tvSpiRead1 Failed!\n", __FUNCTION__);
        return -1;
    }

    ret = tvSpiRead(0x1FB800, 1, crc_sdc_data+3);
    if (ret < 0) {
        LOGE("%s,tvSpiRead2 Failed!\n", __FUNCTION__);
        return -1;
    }

    LOGE("%s,AF:0x%x,0x%x,DEM:0x%x,ACC:0x%x\n", __FUNCTION__, crc_sdc_data[0],crc_sdc_data[1],crc_sdc_data[2],crc_sdc_data[3]);
    ret = system("mount -w -o remount /dev/block/vendor /vendor");
    int check_fd = open(CRC_CHECK_PATH, O_RDWR | O_SYNC | O_CREAT, S_IRUSR | S_IWUSR);
    if (check_fd < 0) {
            LOGE("%s, check_fd open error", __FUNCTION__);
            return -1;
        }

    if (write(check_fd,crc_sdc_data,4) == -1) {
        LOGE("%s,check_fd write error", __FUNCTION__);
        close(check_fd);
        return -1;
    }

    ret = system("mount -r -o remount /dev/block/vendor /vendor");
    close(check_fd);
    return ret_value;
}
