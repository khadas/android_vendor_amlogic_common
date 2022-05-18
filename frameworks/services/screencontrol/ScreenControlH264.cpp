#define LOG_TAG "ScreenControlH264"

#include <string.h>
#include <stdlib.h>
#include <utils/Log.h>

#include "ScreenControlH264.h"

	void bs_init( bs_t *s, void *p_data, int i_data )
{
	s->p_start = (unsigned char *)p_data;		//用传入的p_data首地址初始化p_start，只记下有效数据的首地址
	s->p       = (unsigned char *)p_data;		//字节首地址，一开始用p_data初始化，每读完一个整字节，就移动到下一字节首地址
	s->p_end   = s->p + i_data;	                //尾地址，最后一个字节的首地址?
	s->i_left  = 8;
}


int bs_read( bs_t *s, int i_count )
{
	 static unsigned int i_mask[33] ={0x00,
                                  0x01,      0x03,      0x07,      0x0f,
                                  0x1f,      0x3f,      0x7f,      0xff,
                                  0x1ff,     0x3ff,     0x7ff,     0xfff,
                                  0x1fff,    0x3fff,    0x7fff,    0xffff,
                                  0x1ffff,   0x3ffff,   0x7ffff,   0xfffff,
                                  0x1fffff,  0x3fffff,  0x7fffff,  0xffffff,
                                  0x1ffffff, 0x3ffffff, 0x7ffffff, 0xfffffff,
                                  0x1fffffff,0x3fffffff,0x7fffffff,0xffffffff};
    int      i_shr;
    int i_result = 0;	        //用来存放读取到的的结果 typedef unsigned   uint32_t;

    while ( i_count > 0 )	    //要读取的比特数
    {
        if ( s->p >= s->p_end )
        {						//
            break;
        }
		ALOGD("bs_read i_shr=%d,i_count=%d,s->i_left=%d,s->p[0]=%02X",i_shr,i_count,s->i_left,s->p[0]);

        if ( ( i_shr = s->i_left - i_count ) >= 0 )
        {
            i_result |= ( *s->p >> i_shr )&i_mask[i_count];
			ALOGD("bs_read i_result=%d");
            s->i_left -= i_count;
            if ( s->i_left == 0 )
            {
                s->p++;
                s->i_left = 8;
            }
            return( i_result );
        }
        else
        {

			i_result |= (*s->p&i_mask[s->i_left]) << -i_shr;
			i_count  -= s->i_left;
			s->p++;
			s->i_left = 8;
        }
    }

    return( i_result );
}

int bs_read1( bs_t *s )
{

	if ( s->p < s->p_end )
	{
		unsigned int i_result;

		s->i_left--;
		i_result = ( *s->p >> s->i_left )&0x01;
		if ( s->i_left == 0 )
		{
			s->p++;
			s->i_left = 8;
		}
		return i_result;
	}

	return 0;
}

int bs_read_ue( bs_t *s )
{
	int i = 0;

	while ( bs_read1( s ) == 0 && s->p < s->p_end && i < 32 )
	{
		i++;
	}
	ALOGD("bs_read_ue i=%d",i);
	return( ( 1 << i) - 1 + bs_read( s, i ) );
}

int GetFrameType(NALU_t * nal)
{
	bs_t s;
	int frame_type = 0;
	unsigned char * OneFrameBuf_H264 = NULL ;
	if ((OneFrameBuf_H264 = (unsigned char *)calloc(nal->len + 4,sizeof(unsigned char))) == NULL)
	{
		ALOGE("Error malloc OneFrameBuf_H264");
		return -1;
	}
	if (nal->startcodeprefix_len == 3)
	{
		OneFrameBuf_H264[0] = 0x00;
		OneFrameBuf_H264[1] = 0x00;
		OneFrameBuf_H264[2] = 0x01;
		memcpy(OneFrameBuf_H264 + 3,nal->buf,nal->len);
	}
	else if (nal->startcodeprefix_len == 4)
	{
		OneFrameBuf_H264[0] = 0x00;
		OneFrameBuf_H264[1] = 0x00;
		OneFrameBuf_H264[2] = 0x00;
		OneFrameBuf_H264[3] = 0x01;
		memcpy(OneFrameBuf_H264 + 4,nal->buf,nal->len);
	}
	else
	{
		ALOGE("H264 read error");
	}
	bs_init( &s,OneFrameBuf_H264 + nal->startcodeprefix_len + 1  ,nal->len - 1 );
	ALOGD("GetFrameType bs_init s->p_start[0]=0x%02X,s->p_start[1]=0x%02X", s.p_start[0], s.p_start[1]);

	if (nal->nal_unit_type == NAL_SLICE || nal->nal_unit_type ==  NAL_SLICE_IDR )
	{
		/* i_first_mb */
		ALOGD("GetFrameType i_first_mb");
		bs_read_ue( &s );
		/* picture type */
		ALOGD("GetFrameType i_first_mb picture type");
		frame_type =  bs_read_ue( &s );
		ALOGD("GetFrameType frame_type=%d",frame_type);
		switch (frame_type)
		{
		case 0: case 5: /* P */
			nal->Frametype = FRAME_P;
			break;
		case 1: case 6: /* B */
			nal->Frametype = FRAME_B;
			break;
		case 3: case 8: /* SP */
			nal->Frametype = FRAME_P;
			break;
		case 2: case 7: /* I */
			nal->Frametype = FRAME_I;
			break;
		case 4: case 9: /* SI */
			nal->Frametype = FRAME_I;
			break;
		}
	}
	else if (nal->nal_unit_type == NAL_SEI)
	{
		nal->Frametype = NAL_SEI;
	}
	else if(nal->nal_unit_type == NAL_SPS)
	{
		nal->Frametype = NAL_SPS;
	}
	else if(nal->nal_unit_type == NAL_PPS)
	{
		nal->Frametype = NAL_PPS;
	}
	if (OneFrameBuf_H264)
	{
		free(OneFrameBuf_H264);
		OneFrameBuf_H264 = NULL;
	}
	return 1;
}



