/*
**
** Copyright 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#define LOG_NDEBUG 0
#define LOG_TAG "FormatCovert"
#include <sys/mman.h>
#include <utils/Log.h>
#include "FormatCovert.h"
#include "libyuv/convert_argb.h"
#include "libyuv/scale_argb.h"

namespace android {

void argb_scale(unsigned char* src, unsigned char* dst, int width, int height, int dWidth, int dHeight) {
    if (dWidth == 0 || dHeight == 0 || width == 0 || height == 0) {
        return;
    }
    libyuv::ARGBScale((uint8_t*)src, width * 4, width, height, (uint8_t*)dst, dWidth * 4, dWidth, dHeight,
                      libyuv::kFilterNone);
}

static void yuv_to_rgb32(uint8_t y, uint8_t u, uint8_t v, uint8_t* rgb) {
    int r, g, b;

    r = (1192 * (y - 16) + 1634 * (v - 128)) >> 10;
    g = (1192 * (y - 16) - 833 * (v - 128) - 400 * (u - 128)) >> 10;
    b = (1192 * (y - 16) + 2066 * (u - 128)) >> 10;

    r = r > 255 ? 255 : r < 0 ? 0 : r;
    g = g > 255 ? 255 : g < 0 ? 0 : g;
    b = b > 255 ? 255 : b < 0 ? 0 : b;

    /*ARGB*/
    *rgb = (uint8_t)r;
    rgb++;
    *rgb = (uint8_t)g;
    rgb++;
    *rgb = (uint8_t)b;
    rgb++;
    *rgb = 0xff;
}

static void nv21_to_rgb32(uint8_t* buf, uint8_t* rgb, int width, int height) {
    int x, y, z = 0;
    int h, w;
    int blocks;
    uint8_t Y1, Y2, U, V;

    blocks = (width * height) * 2;
    for (h = 0, z = 0; h < height; h += 2) {
        for (y = 0; y < width * 2; y += 2) {
            Y1 = buf[h * width + y + 0];
            V = buf[blocks / 2 + h * width / 2 + y % width + 0];
            Y2 = buf[h * width + y + 1];
            U = buf[blocks / 2 + h * width / 2 + y % width + 1];
            yuv_to_rgb32(Y1, U, V, &rgb[z]);
            yuv_to_rgb32(Y2, U, V, &rgb[z + 4]);
            z += 8;
        }
    }
}

SoftWareFormatCovert::SoftWareFormatCovert() { ALOGI("SoftWareFormatCovert construct "); }

SoftWareFormatCovert::~SoftWareFormatCovert() { ALOGI("~SoftWareFormatCovert"); }

bool SoftWareFormatCovert::covert(const char* src_buff, unsigned int src_fmt, size_t src_w, size_t src_h,
                                  char* dst_buff, unsigned int dst_fmt, size_t dst_w, size_t dst_h) {
    bool ret = false;
    int64_t first_times = getNowTimesUs();
    if (!src_buff || !dst_buff) {
        ALOGE("the address of the buffer is empty !!!");
        return ret;
    }
    if (src_fmt == SCREENCONTROL_PIX_FMT_NV21 && dst_fmt == SCREENCONTROL_PIX_FMT_RGBA888) {
        if ((src_w * src_h) != (dst_w * dst_h)) {
            char* temp = (char*)malloc(src_w * src_h * 4);
            if (!temp)
                return ret;
            memset(temp, 0, src_w * src_h * 4);
            nv21_to_rgb32((unsigned char*)src_buff, (unsigned char*)temp, src_w, src_h);

            argb_scale((unsigned char*)temp, (unsigned char*)dst_buff, src_w, src_h, dst_w, dst_h);
            free(temp);
        } else {
            nv21_to_rgb32((unsigned char*)src_buff, (unsigned char*)dst_buff, dst_w, dst_h);
        }
        int64_t end_times = getNowTimesUs();
        ALOGD("software change format success duration %lld ms", (end_times - first_times) / 1000);
        ret = true;
    } else {
        ALOGE("[%s %d]  the format:%d covert to format:%d don't support now", __FUNCTION__, __LINE__, src_fmt, dst_fmt);
    }
    return ret;
}

HardWareFormatCovert::HardWareFormatCovert() {
    ALOGI("HardWareFormatCovert construct ");
    memset(&m_amlge2d, 0, sizeof(aml_ge2d_t));
    memset(&(m_amlge2d.ge2dinfo.src_info[0]), 0, sizeof(buffer_info_t));
    memset(&(m_amlge2d.ge2dinfo.src_info[1]), 0, sizeof(buffer_info_t));
    memset(&(m_amlge2d.ge2dinfo.dst_info), 0, sizeof(buffer_info_t));
    int32_t ret = aml_ge2d_init(&m_amlge2d);
    if (ret < 0) {
        aml_ge2d_exit(&m_amlge2d);
        ALOGE("%s: %s", __FUNCTION__, strerror(errno));
    }
}

HardWareFormatCovert::~HardWareFormatCovert() {
    ALOGI("~HardWareFormatCovert");
    aml_ge2d_exit(&m_amlge2d);
}

int32_t HardWareFormatCovert::ge2DFmtConvert(int32_t dst_fd, int32_t dst_fmt, size_t dst_w, size_t dst_h,
                                             int32_t src_fd, int32_t src_fmt, size_t src_w, size_t src_h) {

    memset(&(m_amlge2d.ge2dinfo.src_info[0]), 0, sizeof(buffer_info_t));
    memset(&(m_amlge2d.ge2dinfo.src_info[1]), 0, sizeof(buffer_info_t));
    memset(&(m_amlge2d.ge2dinfo.dst_info), 0, sizeof(buffer_info_t));
    m_amlge2d.ge2dinfo.src_info[0].shared_fd[0] = src_fd;
    m_amlge2d.ge2dinfo.src_info[0].memtype = GE2D_CANVAS_ALLOC;
    m_amlge2d.ge2dinfo.src_info[0].mem_alloc_type = AML_GE2D_MEM_ION;
    m_amlge2d.ge2dinfo.src_info[1].memtype = GE2D_CANVAS_TYPE_INVALID;
    m_amlge2d.ge2dinfo.src_info[1].mem_alloc_type = AML_GE2D_MEM_INVALID;

    m_amlge2d.ge2dinfo.src_info[0].plane_number = 1;
    m_amlge2d.ge2dinfo.src_info[0].canvas_w = src_w;
    m_amlge2d.ge2dinfo.src_info[0].canvas_h = src_h;
    m_amlge2d.ge2dinfo.src_info[0].rect.x = 0;
    m_amlge2d.ge2dinfo.src_info[0].rect.y = 0;
    m_amlge2d.ge2dinfo.src_info[0].rect.w = src_w;
    m_amlge2d.ge2dinfo.src_info[0].rect.h = src_h;

    m_amlge2d.ge2dinfo.src_info[0].format = src_fmt;   // PIXEL_FORMAT_YCbCr_422_UYVY; //PIXEL_FORMAT_YCbCr_420_SP_NV12
    m_amlge2d.ge2dinfo.src_info[0].plane_alpha = 0xFF; /* global plane alpha*/

    m_amlge2d.ge2dinfo.dst_info.shared_fd[0] = dst_fd;
    m_amlge2d.ge2dinfo.dst_info.memtype = GE2D_CANVAS_ALLOC;
    m_amlge2d.ge2dinfo.dst_info.mem_alloc_type = AML_GE2D_MEM_ION;
    m_amlge2d.ge2dinfo.dst_info.plane_number = 1;
    m_amlge2d.ge2dinfo.dst_info.canvas_w = dst_w;
    m_amlge2d.ge2dinfo.dst_info.canvas_h = dst_h;
    m_amlge2d.ge2dinfo.dst_info.rect.x = 0;
    m_amlge2d.ge2dinfo.dst_info.rect.y = 0;
    m_amlge2d.ge2dinfo.dst_info.rect.w = dst_w;
    m_amlge2d.ge2dinfo.dst_info.rect.h = dst_h;
    m_amlge2d.ge2dinfo.dst_info.rotation = GE2D_ROTATION_0;
    m_amlge2d.ge2dinfo.dst_info.format = dst_fmt;   // PIXEL_FORMAT_YCbCr_420_SP_NV12; //PIXEL_FORMAT_RGBA_8888;
    m_amlge2d.ge2dinfo.dst_info.plane_alpha = 0xFF; /* global plane alpha*/

    m_amlge2d.ge2dinfo.ge2d_op = AML_GE2D_STRETCHBLIT;
    int32_t ret = aml_ge2d_process(&m_amlge2d.ge2dinfo);
    if (ret < 0) {
        ALOGE("ge2d process failed, %s (%d)\n", __func__, __LINE__);
    }
    return ret;
}

bool HardWareFormatCovert::covert(const char* src_buff, unsigned int src_fmt, size_t src_w, size_t src_h,
                                  char* dst_buff, unsigned int dst_fmt, size_t dst_w, size_t dst_h) {
    bool ret = false;
    int32_t src_size, dst_size;
    int32_t src_dma_fd, dst_dma_fd;
    char* input = nullptr;
    char* output = nullptr;
    int64_t first_times = getNowTimesUs();
    if (!src_buff || !dst_buff) {
        ALOGE("the address of the buffer is empty !!!");
        return ret;
    }
    auto getPixFormat = [](int32_t format) -> int32_t {
        int32_t pix_format = -1;
        switch (format) {
            case SCREENCONTROL_PIX_FMT_NV21:
                pix_format = PIXEL_FORMAT_YCrCb_420_SP;
                break;
            case SCREENCONTROL_PIX_FMT_RGBA888:
                pix_format = PIXEL_FORMAT_RGBA_8888;
                break;
            case SCREENCONTROL_PIX_FMT_RGB565:
                pix_format = PIXEL_FORMAT_RGB_565;
                break;
            case SCREENCONTROL_PIX_FMT_NV12:
            default:
                pix_format = -1;
                break;
        }
        return pix_format;
    };
    if (getPixFormat(src_fmt) == -1 || getPixFormat(dst_fmt) == -1) {
        ALOGE("[%s %d]  the format:%d covert to format:%d don't support now", __FUNCTION__, __LINE__, src_fmt, dst_fmt);
        return ret;
    }
    int32_t ion_fd = ion_mem_init();
    if (ion_fd < 0) {
        ALOGE("[%s %d]  open ion dev fail", __FUNCTION__, __LINE__);
        return ret;
    }
    auto getBufferSize = [](int32_t format, int32_t w, int32_t h) -> int32_t {
        int32_t size = -1;
        switch (format) {
            case SCREENCONTROL_PIX_FMT_NV12:
            case SCREENCONTROL_PIX_FMT_NV21:
                size = w * h * 3 / 2;
                break;
            case SCREENCONTROL_PIX_FMT_RGBA888:
                size = w * h * 4;
                break;
            case SCREENCONTROL_PIX_FMT_RGB565:
                size = w * h * 3;
                break;
            default:
                size = -1;
                break;
        }
        return size;
    };
    auto ion_alloc_mem = [](int32_t fd, int32_t size) -> int32_t {
        int32_t ret = -1;
        IONMEM_AllocParams ion_alloc_param;
        if (fd < 0 || size < 0) {
            return ret;
        }
        ion_alloc_param.mImageFd = 0;
        ret = ion_mem_alloc(fd, size, &ion_alloc_param, false);
        if (ret < 0) {
            ALOGE("[%s %d]  alloc data fail,size = %d", __FUNCTION__, __LINE__, size);
        }

        return ion_alloc_param.mImageFd;
    };
    src_size = getBufferSize(src_fmt, src_w, src_h);
    if (src_size <= 0) {
        ALOGE("[%s %d]  get the size of src format src_fmt:%d (%d:%d) ", __FUNCTION__, __LINE__, src_fmt, src_w, src_h);
        goto exit;
    }

    src_dma_fd = ion_alloc_mem(ion_fd, src_size);
    if (src_dma_fd < 0) {
        goto exit;
    }
    ALOGD("[%s %d]  alloc memory src dma buffer fd = %d, size = %d ", __FUNCTION__, __LINE__, src_dma_fd, src_size);
    dst_size = getBufferSize(dst_fmt, dst_w, dst_h);
    if (dst_size <= 0) {
        ALOGE("[%s %d]  get the size of dst format dst_fmt:%d (%d:%d) ", __FUNCTION__, __LINE__, dst_fmt, dst_w, dst_h);
        goto exit;
    }
    dst_dma_fd = ion_alloc_mem(ion_fd, dst_size);

    if (dst_dma_fd < 0) {
        ALOGE("dst_dma_fd alloc failed,dst_size=%d\n", dst_size);
        goto exit2;
    }

    input = (char*)mmap(NULL, src_size, PROT_READ | PROT_WRITE, MAP_SHARED, src_dma_fd, 0);
    if (!input) {
        ALOGE("[%s %d]  mmap failed,Not enough memory,size = %d", __FUNCTION__, __LINE__, src_size);
        goto exit3;
    }
    if (strlen(src_buff) < src_size) {
        ALOGE("[%s %d]  the src data buffer don't have enough length", __FUNCTION__, __LINE__);
        goto exit4;
    }
    memcpy(input, src_buff, src_size);
    if (ge2DFmtConvert(dst_dma_fd, getPixFormat(dst_fmt), dst_w, dst_h, src_dma_fd, getPixFormat(src_fmt), src_w,
                       src_h) < 0) {
        goto exit4;
    }
    output = (char*)mmap(NULL, dst_size, PROT_READ | PROT_WRITE, MAP_SHARED, dst_dma_fd, 0);
    if (!output) {
        ALOGE("[%s %d]  mmap failed,Not enough memory,size = %d", __FUNCTION__, __LINE__, dst_size);
        goto exit4;
    }
    memcpy(dst_buff, output, dst_size);
    munmap(output, dst_size);
    ret = true;
exit4:
    munmap(input, src_size);
exit3:
    close(dst_dma_fd);
exit2:
    close(src_dma_fd);
exit:
    ion_mem_exit(ion_fd);

    if (ret) {
        int64_t end_times = getNowTimesUs();
        ALOGD("hardware change format success duration %lld ms", (end_times - first_times) / 1000);
    }
    return ret;
}

}; // namespace android