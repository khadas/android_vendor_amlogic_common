/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#include <fstream>
#include <unistd.h>
#include <sys/time.h>
#include <cstdlib>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <thread>
#include <atomic>
#include <map>
#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>
#include <chrono>
#include <sys/time.h>
#include <memory>
#include <chrono>

#include <termios.h>
#include <pthread.h>
#include <utils/Log.h>
#include <cutils/properties.h>
#include <amlogic/am_gralloc_ext.h>
#include <sys/utsname.h>
#include <string.h>

#include "bootvideo.h"

#define PROPERTY_BOOTANIM_EXIT "service.bootanim.exit"
#define PROPERTY_BOOTVIDEO_EXIT "service.bootvideo.exit"

#define PROPERTY_TSPLAYER_PATH "persist.bootvideo.path"
#define PROPERTY_TSPLAYER_VCODEC "persist.bootvideo.vcodec"
#define PROPERTY_TSPLAYER_ACODEC "persist.bootvideo.acodec"
#define PROPERTY_TSPLAYER_VPID "persist.bootvideo.vpid"
#define PROPERTY_TSPLAYER_APID "persist.bootvideo.apid"
#define PROPERTY_TSPLAYER_PLAYBACK "persist.bootvideo.playback"
#define PROPERTY_TSPLAYER_WAITFINISH "persist.bootvideo.waitfinish"

#define TSPLAYER_PATH_DEF "/system/etc/bootvideo.ts"
#define TSPLAYER_VCODEC_DEF AV_VIDEO_CODEC_H264
#define TSPLAYER_ACODEC_DEF AV_AUDIO_CODEC_AAC
#define TSPLAYER_VPID_DEF 0x100
#define TSPLAYER_APID_DEF 0x101

const int LAYER_VIDEO = 0x30000000;
const int kRwSize = 188*300;
const int kRwTimeout = 500;


#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

static int amsysfs_set_str(const char *path, const char *val) {
    int fd;
    int bytes;
    fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd >= 0) {
        bytes = write(fd, val, strlen(val));
        close(fd);
        return 0;
    } else {
        ALOGD("amsysfs_set_sysfs_str open %s fail: %d", path, errno);
    }
    return -1;
}

static int set_dmx_source()
{
    amsysfs_set_str("/sys/class/stb/source", "dmx0");
    amsysfs_set_str("/sys/class/stb/demux0_source", "hiu");
    return 0;
}

static void disaleDI(bool off) {
    int ch = property_get_int32("vendor.hwc.di_channel_number", -1);
    ALOGD("disaleDI: ch=%d, %s", ch, off?"disable":"enable");
    if (off) {
        if (ch < 0) {
            amsysfs_set_str("/sys/module/decoder_common/parameters/max_di_instance", "0");
            amsysfs_set_str("/sys/module/decoder_common/parameters/v4lvideo_add_di", "0");
        } else {
            property_set("vendor.hwc.di_channel_number", "0");
            property_set("vendor.media.mediahal.tsplayer.vtbuffer_number_limit", "4");
            amsysfs_set_str("/sys/module/decoder_common/parameters/max_di_instance", "0");
            amsysfs_set_str("/sys/module/decoder_common/parameters/v4lvideo_add_di", "0");
            amsysfs_set_str("/sys/class/di_process/di_proc_enable", "0");
        }
    } else {
        if (ch < 0) {
            amsysfs_set_str("/sys/module/decoder_common/parameters/max_di_instance", "2");
            amsysfs_set_str("/sys/module/decoder_common/parameters/v4lvideo_add_di", "1");
        } else {
            property_set("vendor.hwc.di_channel_number", "2");
            property_set("vendor.media.mediahal.tsplayer.vtbuffer_number_limit", "1");
            amsysfs_set_str("/sys/module/decoder_common/parameters/max_di_instance", "2");
            amsysfs_set_str("/sys/module/decoder_common/parameters/v4lvideo_add_di", "0");
            amsysfs_set_str("/sys/class/di_process/di_proc_enable", "1");
        }
    }
}

BootVideo::BootVideo() {
    property_get(PROPERTY_TSPLAYER_PATH, mTsplayParam.filePath, TSPLAYER_PATH_DEF);
    mTsplayParam.vCodec = (am_tsplayer_video_codec)property_get_int32(PROPERTY_TSPLAYER_VCODEC, (int32_t)TSPLAYER_VCODEC_DEF);
    mTsplayParam.aCodec = (am_tsplayer_audio_codec)property_get_int32(PROPERTY_TSPLAYER_ACODEC, (int32_t)TSPLAYER_ACODEC_DEF);
    mTsplayParam.vPid = property_get_int32(PROPERTY_TSPLAYER_VPID, TSPLAYER_VPID_DEF);
    mTsplayParam.aPid = property_get_int32(PROPERTY_TSPLAYER_APID, TSPLAYER_APID_DEF);
    mTsplayParam.emPlaybackType = (am_tsplayer_playback_type)property_get_int32(PROPERTY_TSPLAYER_PLAYBACK, (int32_t)TS_PLAYBACK_DISABLE);
    mTsplayParam.tsType = TS_MEMORY;
    mTsplayParam.avsyncMode = TS_SYNC_AMASTER;
    mTsplayParam.vTrickMode = AV_VIDEO_TRICK_MODE_NONE;
    mWaitPlayFinish = property_get_bool(PROPERTY_TSPLAYER_WAITFINISH, false);
    mLastPlayTs = -1;
}

bool BootVideo::CreateVideoTunnelId(int* id) {
    sp<IProducerListener> producerListener = NULL;
    sp<IGraphicBufferProducer> producer = NULL;
    sp<NativeHandle> sourceHandle = NULL;
    native_handle_t * native_handle = NULL;

    int x = 0, y = 0, w = 960, h = 540;
    int tunnelId = 0;

    if (mSurface == NULL) {
        mComposerClient = new SurfaceComposerClient;
        if (mComposerClient->initCheck() != 0) {
            //printf("mSurface == NULL in line 79");
            return false;
        }
        char test[20];
        sprintf(test,"BootvideoSurface_%d",tunnelId);
        ALOGD("CreateVideoTunnelId name:%s \n",test);
        mControl = mComposerClient->createSurface(String8(test),
                w, h, HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED);
        if (mControl == NULL) {
            printf("mControl == NULL");
            return false;
        }
        if (!mControl->isValid()) {
            printf("! mControl->isValid  no ");
            return false;
        }
        SurfaceComposerClient::Transaction{}
        .setLayer(mControl, LAYER_VIDEO)
        .setFlags(mControl, android::layer_state_t::eLayerOpaque, android::layer_state_t::eLayerOpaque)
        .show(mControl)
        .setPosition(mControl, x, y)
        .apply();
        mSurface = mControl->getSurface();
        if (mSurface == NULL) {
            printf("mSurface == NULL");
            return false;
        }

        producerListener = new StubProducerListener;
        mSurface->connect(NATIVE_WINDOW_API_CPU, producerListener);

        if (mSurface) {
            producer = mSurface->getIGraphicBufferProducer();
            if (native_handle == NULL) {
                native_handle = am_gralloc_create_sideband_handle(AM_FIXED_TUNNEL, tunnelId);
            }
            if (native_handle != NULL) {
                sourceHandle = NativeHandle::create(native_handle, false);
            }
            if (producer != NULL && sourceHandle != NULL) {
                producer->setSidebandStream(sourceHandle);
            }
            printf("----->tunnelId:%d\n",tunnelId);
            *id = tunnelId;
        }
    }
    return true;
}

void BootVideo::video_callback(void *user_data, am_tsplayer_event *event) {
    UNUSED(user_data);
    ALOGD("video_callback type %d\n", event? event->type : 0);
    switch (event->type) {
        case AM_TSPLAYER_EVENT_TYPE_VIDEO_CHANGED:
        {
            ALOGD("[evt] AM_TSPLAYER_EVENT_TYPE_VIDEO_CHANGED: %d x %d @%d [%d]\n",
                event->event.video_format.frame_width,
                event->event.video_format.frame_height,
                event->event.video_format.frame_rate,
                event->event.video_format.frame_aspectratio);
            break;
        }
        case AM_TSPLAYER_EVENT_TYPE_AUDIO_CHANGED:
        {
            ALOGD("[evt] AM_TSPLAYER_EVENT_TYPE_AUDIO_CHANGED: ch=%u ch_mask=%u samplerate=%u\n",
                event->event.audio_format.channels ,
                event->event.audio_format.channel_mask,
                event->event.audio_format.sample_rate);
            break;
        }
        case AM_TSPLAYER_EVENT_TYPE_USERDATA_AFD:
        {
            ALOGD("[evt] AM_TSPLAYER_EVENT_TYPE_USERDATA_AFD\n");
            break;
        }
        case AM_TSPLAYER_EVENT_TYPE_USERDATA_CC:
        {
            uint8_t* pbuf = event->event.mpeg_user_data.data;
            uint32_t size = event->event.mpeg_user_data.len;
            ALOGD("[evt] USERDATA [%d] : %x-%x-%x-%x %x-%x-%x-%x ,size %d\n",
                event->type, pbuf[0], pbuf[1], pbuf[2], pbuf[3],
                pbuf[4], pbuf[5], pbuf[6], pbuf[7], size);
            UNUSED(pbuf);
            UNUSED(size);
            break;
        }
        case AM_TSPLAYER_EVENT_TYPE_FIRST_FRAME:
        {
            ALOGD("[evt] AM_TSPLAYER_EVENT_TYPE_FIRST_FRAME\n");
            //mPlayStatus = 1;
            break;
        }
        case AM_TSPLAYER_EVENT_TYPE_DECODE_FIRST_FRAME_VIDEO:
        {
            ALOGD("[evt] AM_TSPLAYER_EVENT_TYPE_DECODE_FIRST_FRAME_VIDEO\n");
            break;
        }
        case AM_TSPLAYER_EVENT_TYPE_DECODE_FIRST_FRAME_AUDIO:
        {
            ALOGD("[evt] AM_TSPLAYER_EVENT_TYPE_DECODE_FIRST_FRAME_AUDIO\n");
            break;
        }
        case AM_TSPLAYER_EVENT_TYPE_AV_SYNC_DONE:
        {
            ALOGD("[evt] AM_TSPLAYER_EVENT_TYPE_AV_SYNC_DONE\n");
            break;
        }
        case AM_TSPLAYER_EVENT_TYPE_INPUT_VIDEO_BUFFER_DONE:
        {
        //    ALOGD("[evt] AM_TSPLAYER_EVENT_TYPE_INPUT_VIDEO_BUFFER_DONE,%p\n",event->event.ptr);
            break;
        }
        case AM_TSPLAYER_EVENT_TYPE_VIDEO_OVERFLOW:
        {
            ALOGD("[evt] AM_TSPLAYER_EVENT_TYPE_VIDEO_OVERFLOW video_overflow_num %u\n",
                event->event.av_flow_cnt.video_overflow_num);
            break;
        }
        case AM_TSPLAYER_EVENT_TYPE_VIDEO_UNDERFLOW:
        {
            ALOGD("[evt] AM_TSPLAYER_EVENT_TYPE_VIDEO_UNDERFLOW video_underflow_num %u\n",
                event->event.av_flow_cnt.video_underflow_num);
            break;
        }
        case AM_TSPLAYER_EVENT_TYPE_AUDIO_OVERFLOW:
        {
            ALOGD("[evt] AM_TSPLAYER_EVENT_TYPE_AUDIO_OVERFLOW audio_overflow_num %u\n",
                event->event.av_flow_cnt.audio_overflow_num);
            break;
        }
        case AM_TSPLAYER_EVENT_TYPE_AUDIO_UNDERFLOW:
        {
            ALOGD("[evt] AM_TSPLAYER_EVENT_TYPE_AUDIO_UNDERFLOW audio_underflow_num %u\n",
                event->event.av_flow_cnt.audio_underflow_num);
            break;
        }
        case AM_TSPLAYER_EVENT_TYPE_VIDEO_INVALID_TIMESTAMP:
        {
            ALOGD("[evt] AM_TSPLAYER_EVENT_TYPE_VIDEO_INVALID_TIMESTAMP\n");
            break;
        }
        case AM_TSPLAYER_EVENT_TYPE_VIDEO_INVALID_DATA:
        {
            ALOGD("[evt] AM_TSPLAYER_EVENT_TYPE_VIDEO_INVALID_DATA\n");
            break;
        }
        case AM_TSPLAYER_EVENT_TYPE_AUDIO_INVALID_TIMESTAMP:
        {
            ALOGD("[evt] AM_TSPLAYER_EVENT_TYPE_AUDIO_INVALID_TIMESTAMP\n");
            break;
        }
        case AM_TSPLAYER_EVENT_TYPE_AUDIO_INVALID_DATA:
        {
            ALOGD("[evt] AM_TSPLAYER_EVENT_TYPE_AUDIO_INVALID_DATA\n");
            break;
       }
        case AM_TSPLAYER_EVENT_TYPE_DATA_LOSS:
        {
            ALOGD("[evt] AM_TSPLAYER_EVENT_TYPE_DATA_LOSS\n");
            //if (mPlayStatus == 1) {
            //    mPlayStatus = 0;
            //}
            break;
        }
        default:
            break;
    }
}

bool BootVideo::checkExit() {
    int64_t playtime = 0;
    int readyToExit = 0;
    bool playend = false;
    int64_t nowus = systemTime(CLOCK_MONOTONIC);

    AmTsPlayer_getCurrentTime(mSession, &playtime);
    if (playtime > 0) {
        if (mLastPlayTs == playtime) {
            //ALOGD("bootvideo:checkExit %lld time diff: %lld", playtime, nowus - mLastGetTs);
            if (nowus - mLastGetTs > 125000000) {
                playend = true;
            }
        } else {
            mLastPlayTs = playtime;
            mLastGetTs = nowus;
        }
    }
    readyToExit = property_get_int32(PROPERTY_BOOTANIM_EXIT, 0);
    if ((readyToExit > 0 && !mWaitPlayFinish) || playend) {
        ALOGD("service.bootanim.exit %d, play end: %d, exit", readyToExit, playend);
        return true;
    }
    return false;
}

int BootVideo::play() {
    char* buf = new char[kRwSize];
    uint64_t fsize = 0;
    ifstream file(mTsplayParam.filePath, ifstream::binary);
    if (mTsplayParam.tsType) {
       file.seekg(0, file.end);
       fsize = file.tellg();
       if (fsize <= 0) {
           ALOGD("file %s size %lld return\n", mTsplayParam.filePath,(long long)fsize);
           return 0;
       }
       file.seekg(0, file.beg);
    }
    ALOGD("file name = %s, is_open %d, size %lld, tsType %d\n",
                mTsplayParam.filePath, file.is_open(),(long long) fsize, mTsplayParam.tsType);

    //am_tsplayer_handle session;
    am_tsplayer_init_params parm = {mTsplayParam.tsType, TS_INPUT_BUFFER_TYPE_NORMAL, 0, 0};
    int32_t bootplay_mode = 1;
    AmTsPlayer_setParams(mSession, AM_TSPLAYER_KEY_BOOTPLAY_MODE , (void*)&bootplay_mode);
    AmTsPlayer_create(parm, &mSession);

    bool isTsyncNonTunelflag = false;
    if (access("/sys/class/stb/demux0_source",F_OK) != 0) {
       isTsyncNonTunelflag = true;
    } else {
        struct utsname kernel_msg;
        set_dmx_source();
        uname(&kernel_msg);
        if (strstr(kernel_msg.release, "5.15") != NULL) {
            ALOGD("single dmx nontunelmode need set VideoTunnelId\n");
            isTsyncNonTunelflag = true;
        }
    }
    if (isTsyncNonTunelflag) {
        int VideoTunnelId = 0;
        if (CreateVideoTunnelId(&VideoTunnelId) == true) {
            ALOGD("Set VideoTunnelId %d\n", VideoTunnelId);
            AmTsPlayer_setSurface(mSession,(void*)&VideoTunnelId);
        } else {
            ALOGD("CreateVideoTunnelId error \n");
            return 0;
        }
    }
    disaleDI(true);

    uint32_t versionM, versionL;
    AmTsPlayer_getVersion(&versionM, &versionL);
    uint32_t instanceno;
    AmTsPlayer_getInstansNo(mSession, &instanceno);
    AmTsPlayer_setWorkMode(mSession, TS_PLAYER_MODE_NORMAL);
    AmTsPlayer_registerCb(mSession, video_callback, NULL);
    AmTsPlayer_setSyncMode(mSession, mTsplayParam.avsyncMode);
    AmTsPlayer_setVideoBlackOut(mSession, false);

    am_tsplayer_video_params vparam;
    vparam.codectype = mTsplayParam.vCodec;
    vparam.pid = mTsplayParam.vPid;
    AmTsPlayer_setVideoParams(mSession, &vparam);
    AmTsPlayer_startVideoDecoding(mSession);

    am_tsplayer_audio_params aparam;
    aparam.codectype = mTsplayParam.aCodec;
    aparam.pid = mTsplayParam.aPid;
    AmTsPlayer_setAudioParams(mSession, &aparam);
    AmTsPlayer_startAudioDecoding(mSession);

    AmTsPlayer_showVideo(mSession);
    AmTsPlayer_setTrickMode(mSession, mTsplayParam.vTrickMode);

    am_tsplayer_input_buffer ibuf = {TS_INPUT_BUFFER_TYPE_NORMAL, (char*)buf, 0};
    size_t readSize = 0;
    bool isEof = false;

    while (mTsplayParam.tsType)
    {
        if (checkExit()) {
            break;
        }
        if (isEof) {
            usleep(50000);
            continue;
        }
        if (file.eof()) {
            if (mTsplayParam.emPlaybackType == TS_PLAYBACK_ENABLE) {
                ALOGI("file read eof will playback soon \n");
                file.clear();
                file.seekg(0, file.beg);
            } else {
                ALOGI("file read eof will stop playing soon \n");
                isEof = true;
                continue;
            }
        }
        if (!(readSize = file.read(buf, (int)kRwSize).gcount())) {
            ALOGI("read fail:%zu continue\n",readSize);
            continue;
        }
        ibuf.buf_size = (int)readSize;

        int retry = 100;
        am_tsplayer_result res;
        do {
            res = AmTsPlayer_writeData(mSession, &ibuf, kRwTimeout);
            if (res == AM_TSPLAYER_ERROR_RETRY) {
                usleep(50000);
            } else
                break;
        } while(retry-- > 0);
        usleep(500);
    }

    disaleDI(false);
    property_set(PROPERTY_BOOTVIDEO_EXIT, "0");
    delete [](buf);

    if (file.is_open())
        file.close();

    AmTsPlayer_stopVideoDecoding(mSession);
    AmTsPlayer_stopAudioDecoding(mSession);
    AmTsPlayer_release(mSession);

    while (property_get_int32(PROPERTY_BOOTANIM_EXIT, 0) == 0) {
        usleep(100000);
    }

    if (mSurface && mComposerClient && mControl) {
        mSurface.clear();
        mSurface = nullptr;
        mControl.clear();
        mControl = nullptr;
        mComposerClient.clear();
        mComposerClient = nullptr;
    }
    ALOGD("bootvideo play exit");
    return 0;
}

