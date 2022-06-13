// Copyright (C) 2020 Amlogic, Inc. All rights reserved.
//
// All information contained herein is Amlogic confidential.
//
// This software is provided to you pursuant to Software License
// Agreement (SLA) with Amlogic Inc ("Amlogic"). This software may be
// used only in accordance with the terms of this agreement.
//
// Redistribution and use in source and binary forms, with or without
// modification is strictly prohibited without prior written permission
// from Amlogic.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


#ifndef _SECMEM_TYPES_H_
#define _SECMEM_TYPES_H_

enum {
    SECMEM_V2_MEM_SOURCE_NONE                          = 0,
    SECMEM_V2_MEM_SOURCE_VDEC,
    SECMEM_V2_MEM_SOURCE_CODEC_MM
};

enum {
    SECMEM_TVP_TYPE_NONE                               = 0,
    SECMEM_TVP_TYPE_FHD,
    SECMEM_TVP_TYPE_UHD
};

enum {
    SECMEM_CODEC_DEFAULT                       = 0,
    SECMEM_CODEC_VP9,
    SECMEM_CODEC_AV1,
    SECMEM_CODEC_AUDIO,
};

enum {
    SECMEM_V2_USAGE_DRM_PLAYBACK                       = 0,
    SECMEM_V2_USAGE_CAS_LIVE,
    SECMEM_V2_USAGE_CAS_RECORD,
    SEMEMM_V2_USAGE_CAS_REPLAY
};

enum {
    SECMEM_ERROR_INVALID_SESSION                       = 0x1000,
    SECMEM_ERROR_INVALID_BLOCK,
    SECMEM_ERROR_INVALID_HANDLE,
    SECMEM_ERROR_INVALID_CONFIG,
    SECMEM_ERROR_SESSION_FULL,
    SECMEM_ERROR_BLOCK_FULL,
    SECMEM_ERROR_OUT_OF_MEMORY,
    SECMEM_ERROR_BUFFER_FULL,
    SECMEM_ERROR_BLOCK_NOT_EMPTY,
    SECMEM_ERROR_BLOCK_IS_EMPTY,
    SECMEM_ERROR_INVALID_RESOURCE,
    SECMEM_ERROR_INVALID_OPERATION,
    SECMEM_ERROR_NO_CONTINUOUS_MEMORY,
    SECMEM_ERROR_OPEN_TVP_CHANNEL,
    SECMEM_ERROR_OPEN_TVP_TIMER,
    SECMEM_ERROR_INVALID_AUDIO_CONTENT,
    SECMEM_ERROR_MAX
};

enum {
    PAD_TYPE_H264_END_HEADER                           = 1,
    PAD_TYPE_H265_END,
    PAD_TYPE_VP9_END,
    PAD_TYPE_ALL_ZERO_DATA,
    PAD_TYPE_DV,
};

enum {
    SECMEM_CAS_ID_TEST                                 = 0,
    SECMEM_CAS_ID_MAX
};

enum {
    AUD_VALID_TYPE_NONE                                = 0,
    AUD_VALID_TYPE_MPGAUD,
    AUD_VALID_TYPE_AC3,
    AUD_VALID_TYPE_AAC_ADTS,
    AUD_VALID_TYPE_AAC_LOAS,
    AUD_VALID_TYPE_END,
};

enum {
    STREAM_TYPE_AVCC                                   = 1,
    STREAM_TYPE_AVC2NALU,
    STREAM_TYPE_VP9,
    STREAM_TYPE_HVCC,
    STREAM_TYPE_HVC2NALU,
    STREAM_TYPE_AV1,
};

enum {
    PARSER_H264_SPS_SEEN                               = 1 << 0,
    PARSER_H264_PPS_SEEN                               = 1 << 1,
    PARSER_H264_IDR_SEEN                               = 1 << 2,
    PARSER_H264_SLICE_SEEN                             = 1 << 3,
};

enum {
    PARSER_H265_SPS_SEEN                               = 1 << 0,
    PARSER_H265_PPS_SEEN                               = 1 << 1,
    PARSER_H265_VPS_SEEN                               = 1 << 2,
    PARSER_H265_IDR_SEEN                               = 1 << 3,
    PARSER_H265_SLICE_SEEN                             = 1 << 4,
};

enum {
    CAS_DSC_SUCCESS                                    = 0,
    CAS_DSC_ERROR                                      = 0x1000,
    CAS_DSC_ITEM_NOT_FOUND,
    CAS_DSC_NOT_SUPPORT,
    CAS_DSC_RETRY,
    CAS_DSC_ERROR_MAX
};

typedef struct {
    uint32_t kt_count;
    uint32_t kt_buf_len;
    uint8_t *kt_buf;
} key_table_info;

#endif /*_SECMEM_TYPES_H_ */
