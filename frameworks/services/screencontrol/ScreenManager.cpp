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

#define LOG_TAG "ScreenManager"
//#define LOG_NDEBUG 0
#include <utils/Log.h>
#include <cutils/properties.h>
#include <linux/videodev2.h>
#include "libyuv/scale_argb.h"
#include "libyuv/convert_argb.h"

#include <ScreenManager.h>



namespace android {

#define PORTTYPE_VALUE_VPP0_VIDEO_ONLY 0x11000000
#define PORTTYPE_VALUE_VPP0_VIDEO_OSD  0x11000001
#define PORTTYPE_VALUE_VPP1_VIDEO_ONLY 0x11000002
#define PORTTYPE_VALUE_VPP1_VIDEO_OSD  0x11000003
#define PORTTYPE_VALUE_VPP0_OSD_ONLY   0x11000004

#define kMetadataBufferTypeCanvasSource 3

#define SCREENMANAGER_DUMP_BASEDIR "/data/temp/sm-drvin"
#define PERSIST_SYS_ROTATION_PROP "persist.vendor.sys.builtinrotation"

static void VdinDataCallBack(void *user, aml_screen_buffer_info_t *buffer){
    ScreenManager *source = static_cast<ScreenManager *>(user);
    source->dataCallBack(buffer);
    return;
}
void argb_scale(uint8_t *src, uint8_t* dst, int width, int height, int dWidth, int dHeight)
{
    if (dWidth == 0 || dHeight == 0 || width == 0 || height == 0) {
        return;
    }
    libyuv::ARGBScale((uint8_t*)src, width * 4, width, height, (uint8_t*)dst, dWidth * 4, dWidth, dHeight, libyuv::kFilterNone);
}

static void yuv_to_rgb32(uint8_t y,uint8_t u,uint8_t v,uint8_t *rgb)
{
    int r,g,b;

    r = (1192 * (y - 16) + 1634 * (v - 128) ) >> 10;
    g = (1192 * (y - 16) - 833 * (v - 128) - 400 * (u -128) ) >> 10;
    b = (1192 * (y - 16) + 2066 * (u - 128) ) >> 10;

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

static void nv21_to_rgb32(uint8_t *buf, uint8_t *rgb, int width, int height)
{
    int x,y,z=0;
    int h,w;
    int blocks;
    uint8_t Y1, Y2, U, V;

    blocks = (width * height) * 2;
    for (h=0, z=0; h< height; h+=2) {
        for (y = 0; y < width*2; y+=2) {
            Y1 = buf[ h*width + y + 0];
            V = buf[ blocks/2 + h*width/2 + y%width + 0 ];
            Y2 = buf[ h*width + y + 1];
            U = buf[ blocks/2 + h*width/2 + y%width + 1 ];
            yuv_to_rgb32(Y1, U, V, &rgb[z]);
            yuv_to_rgb32(Y2, U, V, &rgb[z + 4]);
            z+=8;
        }
    }
}




static int32_t getRotationDegree(){
    char prop[PROPERTY_VALUE_MAX];
    if (property_get(PERSIST_SYS_ROTATION_PROP, prop, "0") > 0) {
       ALOGI("start prop =%s",prop);
        char *tmp = nullptr;
        long degree = strtol(prop, &tmp, 0);
        ALOGI("propValue =%ld",degree);
        if (LONG_MIN != degree && LONG_MAX != degree ) {
            return degree;
        }
    }
    return -1;
}

ScreenManager::ScreenManager():
    mScreenMangerCallback(nullptr),
    mScreenModule(nullptr),
    mScreenDev(nullptr),
    mBufferSize(0),
    mFormat(0),
    mPortType(0),
    mClientNum(0),
    mIsMultiAcquire(true),
    mStart(false)
{
    ALOGI("[%s %d] Construct", __FUNCTION__, __LINE__);
    mMultiClientMap.clear();

}


ScreenManager::~ScreenManager() {
    ALOGI("~ScreenManager");

    if (mStart) {
        int num = mClientNum;
        for (int i = 0;i < num;i++) {
            stop(i);
        }
    }
}


bool ScreenManager::start(std::unique_ptr<InputParmeter>& input, ScreenMangerCallback *client, int32_t *id, bool multi_acquire) {
    std::lock_guard<std::mutex> lock(mLock);
    if (mStart && (!mIsMultiAcquire || (input->source_type != mInputParmeter->source_type))) {
        ALOGE("[%s %d] the module has been opened and the user is not multi acquire! %d:%d", __FUNCTION__, __LINE__,mStart,mIsMultiAcquire);
        return false;
    }
    if (mStart)
        return startMoreClient(input,client,id);

    if (client)
        mScreenMangerCallback = client;
    mInputParmeter = std::move(input);
    if (mInputParmeter->source_type == AML_CAPTURE_VIDEO) { //video only
        mPortType = PORTTYPE_VALUE_VPP0_VIDEO_ONLY;
    } else if(mInputParmeter->source_type == AML_CAPTURE_OSD_VIDEO) {
        mPortType = PORTTYPE_VALUE_VPP0_VIDEO_OSD;
    } else if(mInputParmeter->source_type == AML_CAPTURE_OSD_ONLY) {
        mPortType = PORTTYPE_VALUE_VPP0_OSD_ONLY;
    } else {
        ALOGE("[%s %d] For now ,we don't capture %d by AML_SCREEN_HARDWARE_MODULE_ID module!",
                                                __FUNCTION__, __LINE__,mInputParmeter->source_type);
        return false;
    }
    if (!isSupportFormat()) {
        return false;
    }
    mBufferSize = getBufferSize(mInputParmeter->size,mInputParmeter->format);
    if (mBufferSize == 0 )
        return false;

    if (hw_get_module(AML_SCREEN_HARDWARE_MODULE_ID, (const hw_module_t **)&mScreenModule) < 0) {
        ALOGE("[%s %d] can`t get AML_SCREEN_HARDWARE_MODULE_ID module", __FUNCTION__, __LINE__);
        return false;
    }

    ALOGI("[%s %d] mPortType=%#x(%s)", __FUNCTION__, __LINE__,
        mPortType, (PORTTYPE_VALUE_VPP0_VIDEO_ONLY == mPortType?"video only":
            (PORTTYPE_VALUE_VPP0_VIDEO_OSD == mPortType?"video+osd":"osd only")));
    if (mScreenModule->common.methods->open((const hw_module_t *)mScreenModule, "1",
            (struct hw_device_t**)&mScreenDev) < 0 || !mScreenDev) {
        mScreenModule = nullptr;
        ALOGE("[%s %d] open AML_SCREEN_SOURCE fail", __FUNCTION__, __LINE__);
        return false;
    }
    int32_t degree = getRotationDegree();
    if ( degree > 0) {
        setVideoRotation(degree);
        if (degree == 90 || degree == 270) {
            int32_t right = mInputParmeter->area->right();
            int32_t bottom = mInputParmeter->area->bottom();
            mInputParmeter->area->set_bottom(right);
            mInputParmeter->area->set_right(bottom);
        }

    }
    if (multi_acquire) {
        mScreenDev->ops.set_mode(mScreenDev, AML_SCREEN_RECODE_MODE );
    }else
        mScreenDev->ops.set_mode(mScreenDev, AML_SCREEN_CATCH_MODE);
    mIsMultiAcquire = multi_acquire;
    mScreenDev->ops.set_port_type(mScreenDev, mPortType);
    mScreenDev->ops.set_frame_rate(mScreenDev, mInputParmeter->frame_rate);
    ALOGD("[%s %d] set_format width=%d,height=%d", __FUNCTION__, __LINE__,mInputParmeter->size->width(),mInputParmeter->size->height());
    mScreenDev->ops.set_format(mScreenDev, mInputParmeter->size->width(), mInputParmeter->size->height(), mFormat);
    mScreenDev->ops.setDataCallBack(mScreenDev, VdinDataCallBack, (void*)this);
    mScreenDev->ops.set_amlvideo2_crop(mScreenDev,mInputParmeter->area->x(), mInputParmeter->area->y(),
                                    mInputParmeter->area->right(), mInputParmeter->area->bottom());
    mScreenDev->ops.start(mScreenDev);
    mStart = true;
    *id = 0;
    mClientNum = 1;
    ALOGD("[%s %d] start finish", __FUNCTION__, __LINE__);
    return true;
}

bool ScreenManager::startMoreClient(std::unique_ptr<InputParmeter>& input, ScreenMangerCallback *client, int32_t *id) {
    if (!client || input->format < 0)
        return false;
    auto info = std::make_unique<MultiClientInfo>(input->format,client);
    info->size = std::move(input->size);
    *id = mClientNum;
    mClientNum++;
    ALOGI("[%s %d]  id=%d,mClientNum=%d", __FUNCTION__, __LINE__,*id,mClientNum);
    mMultiClientMap.insert(std::pair<int32_t, std::unique_ptr<MultiClientInfo>>(*id, std::move(info)));
    return true;
}

void ScreenManager::stop(int32_t client_id) {
    std::lock_guard<std::mutex> lock(mLock);
    ALOGI("[%s %d] client_id = %d", __FUNCTION__, __LINE__,client_id);
    if (client_id > 0) {
        auto it = mMultiClientMap.find(client_id);
        if (it != mMultiClientMap.end()) {
            mMultiClientMap.erase(it);
            mClientNum--;
        }
        if (mClientNum > 0)
            return;
    }else if (client_id == 0 && mClientNum > 1) {
        if (mScreenMangerCallback)
            mScreenMangerCallback = nullptr;
        return;
    }
    mScreenDev->ops.stop(mScreenDev);
    mOutputRecordQueue.clear();
    mMultiClientMap.clear();
    mScreenDev->common.close((struct hw_device_t *)mScreenDev);
    mScreenModule = nullptr;
    mScreenMangerCallback = nullptr;
    mStart = false;
    mBufferSize = 0;
    mFormat = 0;
    mPortType = 0;
    mIsMultiAcquire = true;
    ALOGI("[%s %d] stop finish", __FUNCTION__, __LINE__);
    return;
}

bool ScreenManager::isSupportFormat(){
    int32_t format;
    if (mInputParmeter->format == SCREENCONTROL_PIX_FMT_NV21) {
        ALOGI("[%s %d] format = V4L2_PIX_FMT_NV21 ", __FUNCTION__, __LINE__);
        format = V4L2_PIX_FMT_NV21;
    }else if (mInputParmeter->format == SCREENCONTROL_PIX_FMT_NV12) {
        ALOGI("[%s %d] format = V4L2_PIX_FMT_NV12 ", __FUNCTION__, __LINE__);
        format = V4L2_PIX_FMT_NV12;
    }else if (mInputParmeter->format == SCREENCONTROL_PIX_FMT_RGBA888) {
        ALOGI("[%s %d] format = V4L2_PIX_FMT_RGB32 ", __FUNCTION__, __LINE__);
        format = V4L2_PIX_FMT_RGB32;
    }else if (mInputParmeter->format == SCREENCONTROL_PIX_FMT_RGB565) {
        ALOGI("[%s %d] format = V4L2_PIX_FMT_RGB565X ", __FUNCTION__, __LINE__);
        format = V4L2_PIX_FMT_RGB565X;
    }else {
        ALOGE("[%s %d] dont't support the format!", __FUNCTION__, __LINE__);
        return false;
    }
    mFormat = format;
    return true;

}

bool ScreenManager::getBufferWithFormat(uint8_t *src ,int32_t src_size, uint8_t *dst,
                                    std::unique_ptr<InputParmeter>& src_parmeter,
                                    std::unique_ptr<MultiClientInfo>& dst_parmeter ) {
    ALOGI("[%s %d] src_size =%d,src=%p ", __FUNCTION__, __LINE__,src_size,src);
    if (!src || src_size <= 0)
        return false;
    if (src_parmeter->format == SCREENCONTROL_PIX_FMT_NV21 && dst_parmeter->format == SCREENCONTROL_PIX_FMT_RGBA888) {
        if (src_parmeter->size->width() == dst_parmeter->size->width() && src_parmeter->size->height() == dst_parmeter->size->height()) {
            nv21_to_rgb32(src, dst , dst_parmeter->size->width(), dst_parmeter->size->height());
            ALOGI("[%s %d] nv21_to_rgb32 finish", __FUNCTION__, __LINE__);
        }else {
            int32_t temp_size = src_parmeter->size->width() * src_parmeter->size->height() * 4;
            uint8_t* temp = new uint8_t[temp_size];
            nv21_to_rgb32(src, temp , src_parmeter->size->width(), src_parmeter->size->height());
            argb_scale(temp, dst, src_parmeter->size->width(), src_parmeter->size->height(), dst_parmeter->size->width(), dst_parmeter->size->height());
            ALOGI("[%s %d] argb_scale finish ", __FUNCTION__, __LINE__);
            delete []temp;
        }
    } else {
        ALOGE("[%s %d] don't support from %d to %d ", __FUNCTION__, __LINE__,src_parmeter->format,dst_parmeter->format);
        return false;
        //TODO
    }
    return true;
}

int32_t ScreenManager::getBufferSize(std::unique_ptr<Size>& size,aml_screencontrol_format format) {
    int32_t buffer_size = 0;
    int32_t width = size->width();
    int32_t height = size->height();
    if (format == SCREENCONTROL_PIX_FMT_NV21 || format == SCREENCONTROL_PIX_FMT_NV12) {
        buffer_size = width * height * 3 / 2;
    }else if (format == SCREENCONTROL_PIX_FMT_RGBA888) {
        buffer_size = width * height * 4;
    }else if (format == SCREENCONTROL_PIX_FMT_RGB565) {
        buffer_size = width * height * 3;
    }
    return buffer_size;
}

bool ScreenManager::setVideoRotation(int32_t degree)
{
    int32_t angle;

    ALOGI("[%s %d] setVideoRotation degree:%x", __FUNCTION__, __LINE__, degree);
    if (degree == 0)
        angle = 0;
    else if (degree == 1)
        angle = 270;
    else if (degree == 2)
        angle = 180;
    else if (degree == 3)
        angle = 90;
    else {
        ALOGE("degree is not right");
        return false;
    }

    if (mScreenDev != NULL) {
        ALOGI("[%s %d] setVideoRotation angle:%d", __FUNCTION__, __LINE__, angle);
        mScreenDev->ops.set_rotation(mScreenDev, angle);
    }

    return true;
}


bool ScreenManager::realseBuffer(int32_t client_id, int32_t index) {
    std::lock_guard<std::mutex> lock(mLock);
    if (index < 0 || mOutputRecordQueue.size() == 0 || client_id > 0) {
        ALOGE("realseBuffer failed, index %d, mOutputRecordQueue size %d client_id =%d\n",
                    index,(int32_t)mOutputRecordQueue.size(),client_id);
        return false;
    }
    auto outinfo = std::find_if(mOutputRecordQueue.begin(), mOutputRecordQueue.end(),
                        [=](std::unique_ptr<OutputRecord>& info) {
                            return info->index == index;
                        });
    if (outinfo == mOutputRecordQueue.end()) {
        ALOGE("realseBuffer failed: index %d", index);
        return false;
    }
    ALOGI("[%s %d] client_id:%d,index:%d,pts:%lld", __FUNCTION__, __LINE__, client_id,index,(*outinfo)->tv_usec);
    free((*outinfo)->canvas_buffer);
    mScreenDev->ops.release_buffer(mScreenDev, (long *)(*outinfo)->raw_buffer);
    mOutputRecordQueue.erase(outinfo);
    return true;

}

int32_t ScreenManager::dataCallBack(aml_screen_buffer_info_t *buffer) {
    std::unique_lock<std::mutex> lg(mLock);
    int64_t tv_usec = 0;
    uint8_t* canvas_buffer = nullptr;
    if (!mStart) {
        ALOGE("the modules has been not started");
        return 0;
    }
    auto output = std::make_unique<OutputRecord>();
    output->index = buffer->index;
    struct timeval timeNow;
    gettimeofday(&timeNow, NULL);
    tv_usec = (int64_t)timeNow.tv_sec*1000*1000 + (int64_t)timeNow.tv_usec;
    output->tv_usec = tv_usec;
    ALOGI("[%s %d] index:%d pts=%lld", __FUNCTION__, __LINE__,buffer->index,output->tv_usec);
    output->raw_buffer = (uint8_t *)buffer->buffer_mem;
    long buff_info[3] = {0,0,0};
    buff_info[0] = kMetadataBufferTypeCanvasSource;
    buff_info[1] = (long)buffer->buffer_mem;
    buff_info[2] = buffer->buffer_canvas;
    output->canvas_buffer = malloc(3 *sizeof(long));
    memset(output->canvas_buffer, 0, 3 *sizeof(long));
    memcpy((long *)output->canvas_buffer, &buff_info[0],sizeof(buff_info));
    canvas_buffer = (uint8_t*)output->canvas_buffer;

    const OutputRecord picture(output->index, mBufferSize,output->tv_usec, output->raw_buffer, output->canvas_buffer,mInputParmeter->format);
    mOutputRecordQueue.push_back(std::move(output));
    if (ScreenControlDebug::isNeedDumpYuv()) {
        static int32_t count = 0;
        char filename[64] = {0};
        const char *dump_path = SCREENMANAGER_DUMP_BASEDIR;
        snprintf(filename, 64, "%s/screensource-%d.yuv", dump_path, count++);
        auto dumper = std::make_unique<DataDumper>(filename);
        dumper->dump((uint8_t *)buffer->buffer_mem,mBufferSize);
        ALOGI("[%s %d] dump raw data dir = %s", __FUNCTION__, __LINE__,filename);
    }
    if (!mMultiClientMap.empty()) {
        for (auto it = mMultiClientMap.begin(); it != mMultiClientMap.end(); it++) {
            int32_t size = getBufferSize(it->second->size,it->second->format);
            uint8_t* dst = new uint8_t[size];
            if (getBufferWithFormat((uint8_t*)buffer->buffer_mem,mBufferSize,dst,mInputParmeter,it->second) && dst && size > 0) {
                const OutputRecord record(buffer->index, size,tv_usec, dst, canvas_buffer,it->second->format);
                it->second->cb->PictureReady(record);
            }else
                delete []dst;
        }
    }
    lg.unlock();
    if (mScreenMangerCallback) {
        mScreenMangerCallback->PictureReady(picture);
    }else
        realseBuffer(0,picture.index);
    return 0;
}

};