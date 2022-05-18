#ifndef ANDROID_DROIDLOGIC_SCREENCONTROL_H264_H
#define ANDROID_DROIDLOGIC_SCREENCONTROL_H264_H


//读取字节结构体
typedef struct Tag_bs_t
{
	unsigned char *p_start;	               //缓冲区首地址(这个开始是最低地址)
	unsigned char *p;			           //缓冲区当前的读写指针 当前字节的地址，这个会不断的++，每次++，进入一个新的字节
	unsigned char *p_end;		           //缓冲区尾地址		//typedef unsigned char   uint8_t;
	int     i_left;				           // p所指字节当前还有多少 “位” 可读写 count number of available(可用的)位
}bs_t;
//H264一帧数据的结构体
typedef struct Tag_NALU_t
{
	unsigned char forbidden_bit;           //! Should always be FALSE
	unsigned char nal_reference_idc;       //! NALU_PRIORITY_xxxx
	unsigned char nal_unit_type;           //! NALU_TYPE_xxxx
	unsigned int  startcodeprefix_len;      //! 前缀字节数
	unsigned int  len;                     //! 包含nal 头的nal 长度，从第一个00000001到下一个000000001的长度
	unsigned int  max_size;                //! 最多一个nal 的长度
	unsigned char * buf;                   //! 包含nal 头的nal 数据
	unsigned char Frametype;               //! 帧类型
	unsigned int  lost_packets;            //! 预留
} NALU_t;

//nal类型
enum nal_unit_type_e
{
	NAL_UNKNOWN        = 0,
	NAL_SLICE          = 1,
	NAL_SLICE_DPA      = 2,
	NAL_SLICE_DPB      = 3,
	NAL_SLICE_DPC      = 4,
	NAL_SLICE_IDR      = 5,    /* ref_idc != 0 */
	NAL_SEI            = 6,    /* ref_idc == 0 */
	NAL_SPS            = 7,
	NAL_PPS            = 8,
    NAL_DELIMITER      = 9,
    NAL_SEQUENCE_END   = 10,
    NAL_STEAM_END      = 11

};

//帧类型
enum Frametype_e
{
	FRAME_I  = 15,
	FRAME_P  = 16,
	FRAME_B  = 17
};


enum screen_control_frame_type
{
    /* 片数据A分区*/
    AVC_TYPE_FRAME_TYPE_SLICE_A            = 0,
    /* 片数据B分区*/
    AVC_TYPE_FRAME_TYPE_SLICE_B            = 1,
    /* 片数据C分区*/
    AVC_TYPE_FRAME_TYPE_SLICE_C            = 2,
    /* IDR帧*/
    AVC_TYPE_FRAME_TYPE_IDR                = 3,
    /* 补充增强信息单元 SEI*/
    AVC_TYPE_FRAME_TYPE_SEI                = 4,
    /* 序列参数集 SPS */
    AVC_TYPE_FRAME_TYPE_SPS                = 5,
    /* 图像参数集 PPS*/
    AVC_TYPE_FRAME_TYPE_PPS                = 6,
    /* 分界符*/
    AVC_TYPE_FRAME_TYPE_DELIMITER          = 7,
    /* 序列结束*/
    AVC_TYPE_FRAME_TYPE_SEQUENCE_END       = 8,
    /* 码流结束*/
    AVC_TYPE_FRAME_TYPE_STEAM_END          = 9,
     /* I帧*/
     AVC_TYPE_FRAME_TYPE_FRAME_I           = 10,
     /* B帧*/
     AVC_TYPE_FRAME_TYPE_FRAME_B           = 11,
     /* P帧*/
     AVC_TYPE_FRAME_TYPE_FRAME_P           = 12,
     /* UNKNOWN */
     AVC_TYPE_FRAME_TYPE_UNKNOWN           = 13
};
int GetFrameType(NALU_t * nal);


#endif
