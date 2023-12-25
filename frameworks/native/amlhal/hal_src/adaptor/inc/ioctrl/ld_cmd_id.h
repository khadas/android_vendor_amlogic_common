#ifndef _LD_CMD_ID_H_
#define _LD_CMD_ID_H_

#define LD_DEVICE_NAME                          "aml_ldim"
#define LD_IOC_MAGIC                            'C'


enum ioc_ld_cmd
{
    IOC_LD_CMD_GET_PQ_INIT                              = 0x04,
    IOC_LD_CMD_SET_PQ_INIT                              = 0x05,
    IOC_LD_CMD_GET_LEVEL_IDX                            = 0x06,
    IOC_LD_CMD_SET_LEVEL_IDX                            = 0x07,
    IOC_LD_CMD_GET_FUNC_EN                              = 0X08,
    IOC_LD_CMD_SET_FUNC_EN                              = 0x09,
    IOC_LD_CMD_GET_REMAP_EN                             = 0x0A,
    IOC_LD_CMD_SET_REMAP_EN                             = 0x0B,
    IOC_LD_CMD_GET_BL_MATRIX                            = 0x0C,
    IOC_LD_CMD_SET_BL_MATRIX                            = 0x0D,
    IOC_LD_CMD_GET_DEMOMODE                             = 0x0E,
    IOC_LD_CMD_SET_DEMOMODE                             = 0x0F,

    IOC_LD_CMD_GET_INFO_NEW                             = 0x53,
    IOC_LD_CMD_SET_INFO_NEW                             = 0x54,
    IOC_LD_CMD_GET_BL_MAPPING_PATH                      = 0x55,
    IOC_LD_CMD_SET_MAPPING                              = 0x56,
    IOC_LD_CMD_GET_BL_PROFILE_PATH                      = 0x57,
    IOC_LD_CMD_SET_BL_PROFILE                           = 0x58,
};

#define AML_LDIM_IOC_NR_GET_PQ_INIT                     _IOR(LD_IOC_MAGIC, IOC_LD_CMD_GET_PQ_INIT, am_pq_bin_param_s)
#define AML_LDIM_IOC_NR_SET_PQ_INIT                     _IOW(LD_IOC_MAGIC, IOC_LD_CMD_SET_PQ_INIT, am_pq_bin_param_s)
#define AML_LDIM_IOC_NR_GET_LEVEL_IDX                   _IOR(LD_IOC_MAGIC, IOC_LD_CMD_GET_LEVEL_IDX, int)
#define AML_LDIM_IOC_NR_SET_LEVEL_IDX                   _IOW(LD_IOC_MAGIC, IOC_LD_CMD_SET_LEVEL_IDX, int)
#define AML_LDIM_IOC_NR_GET_FUNC_EN                     _IOR(LD_IOC_MAGIC, IOC_LD_CMD_GET_FUNC_EN, int)
#define AML_LDIM_IOC_NR_SET_FUNC_EN                     _IOW(LD_IOC_MAGIC, IOC_LD_CMD_SET_FUNC_EN, int)
#define AML_LDIM_IOC_NR_GET_REMAP_EN                    _IOR(LD_IOC_MAGIC, IOC_LD_CMD_GET_REMAP_EN, int)
#define AML_LDIM_IOC_NR_SET_REMAP_EN                    _IOW(LD_IOC_MAGIC, IOC_LD_CMD_SET_REMAP_EN, int)
#define AML_LDIM_IOC_NR_GET_BL_MATRIX                   _IOR(LD_IOC_MAGIC, IOC_LD_CMD_GET_BL_MATRIX, int)
#define AML_LDIM_IOC_NR_SET_BL_MATRIX                   _IOW(LD_IOC_MAGIC, IOC_LD_CMD_SET_BL_MATRIX, int)
#define AML_LDIM_IOC_NR_GET_DEMOMODE                    _IOR(LD_IOC_MAGIC, IOC_LD_CMD_GET_DEMOMODE, int)
#define AML_LDIM_IOC_NR_SET_DEMOMODE                    _IOW(LD_IOC_MAGIC, IOC_LD_CMD_SET_DEMOMODE, int)

#define AML_LDIM_IOC_CMD_GET_INFO_NEW                   _IOR(LD_IOC_MAGIC, IOC_LD_CMD_GET_INFO_NEW, struct aml_ldim_pq_s)
#define AML_LDIM_IOC_CMD_SET_INFO_NEW                   _IOW(LD_IOC_MAGIC, IOC_LD_CMD_SET_INFO_NEW, struct aml_ldim_pq_s)
#define AML_LDIM_IOC_NR_GET_BL_MAPPING_PATH             _IOR(LD_IOC_MAGIC, IOC_LD_CMD_GET_BL_MAPPING_PATH, aml_path_s)
#define AML_LDIM_IOC_NR_SET_BL_MAPPING                  _IOW(LD_IOC_MAGIC, IOC_LD_CMD_SET_MAPPING, am_pq_bin_param_s)
#define AML_LDIM_IOC_NR_GET_BL_PROFILE_PATH             _IOR(LD_IOC_MAGIC, IOC_LD_CMD_GET_BL_PROFILE_PATH, aml_path_s)
#define AML_LDIM_IOC_NR_SET_BL_PROFILE                  _IOW(LD_IOC_MAGIC, IOC_LD_CMD_SET_BL_PROFILE, am_pq_bin_param_s)

#endif
