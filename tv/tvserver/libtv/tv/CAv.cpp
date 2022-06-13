/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "tvserver"
#define LOG_TV_TAG "CAv"

#include "CAv.h"
#include "CTvChannel.h"
#include <CFile.h>
#include <tvutils.h>
#include <tvconfig.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <cutils/properties.h>
#define DEFAULT_TIMESHIFT_BASENAME   "timeshif"
#define INVALID_PLAYER_HDLE -1

#define VALID_PID(_pid_) ((_pid_)>0 && (_pid_)<0x1fff)

CAv::CAv()
{
    struct stat st;
    mpObserver = NULL;
    mTvPlayDevId = 0;
    mVideoLayerState = VIDEO_LAYER_NONE;
    mFdAmVideo = -1;
    mIsTsplayer = true;
    mSession = INVALID_PLAYER_HDLE;
    mIsTimeshift = false;
    old_dmx = true;
    if (stat("/sys/class/stb/demux0_source", &st) == -1) {
        old_dmx = false;
    }
    mCurAvEvent.timeshiftStarttime = -1;
    memset(&play_params, 0, sizeof(play_params));
}

CAv::~CAv()
{
    mIsTsplayer = true;
    mSession = INVALID_PLAYER_HDLE;
}

#ifdef SUPPORT_ADTV
void CAv::av_audio_callback(int event_type, AudioParms* param, void *user_data)
{
    LOGD ( "%s\n", __FUNCTION__ );

    CAv *pAv = ( CAv * ) user_data;
    if (NULL == pAv ) {
        LOGD ( "%s, ERROR : av_audio_callback NULL == pTv\n", __FUNCTION__ );
        return ;
    }
    if ( pAv->mpObserver == NULL ) {
        LOGD ( "%s, ERROR : mpObserver NULL == mpObserver\n", __FUNCTION__ );
        return ;
    }

    switch ( event_type ) {
        case AM_AV_EVT_AUDIO_CB:
            pAv->mCurAvEvent.type = AVEvent::EVENT_AUDIO_CB;
            pAv->mCurAvEvent.status = param->cmd;
            pAv->mCurAvEvent.param = param->param1;
            pAv->mCurAvEvent.param1 = param->param2;

            pAv->mpObserver->onEvent(pAv->mCurAvEvent);

            break;
     }
     return ;
}
#endif

int CAv::SetVideoWindow(int x, int y, int w, int h)
{
#ifdef SUPPORT_ADTV
    if (mIsTsplayer == false) {
        return AM_AV_SetVideoWindow (mTvPlayDevId, x, y, w, h );
    }

    am_tsplayer_result ret = AM_TSPLAYER_OK;
    if (mSession != INVALID_PLAYER_HDLE)
        ret = AmTsPlayer_setVideoWindow(mSession, x, y, w, h);
    if (ret != AM_TSPLAYER_OK) {
        LOGD("%s: ret is %d\n", __FUNCTION__, ret);
        return -1;
    }
    return 0;
#else
    return -1;
#endif
}

int CAv::Open()
{
#ifdef SUPPORT_ADTV
    //for debug tsplayer
    //mIsTsplayer = true;
    mIsTsplayer = propertyGetBool("vendor.tv.dtv.tsplayer.enable", false);
    mFdAmVideo = open ( PATH_VIDEO_AMVIDEO, O_RDWR );
    if ( mFdAmVideo < 0 ) {
        LOGE ("mFdAmVideo < 0, error(%s)!\n", strerror ( errno ) );
        return -1;
    }
    if (mIsTsplayer == true) {
        //TS_DEMOD and dmx = 0
        //creatPlayer(TS_DEMOD, 0);
        // tunnel id need match with tvinput hal id
        tunnelid = 1;
        return 0;
    }
    AM_AV_OpenPara_t para_av;
    memset ( &para_av, 0, sizeof ( AM_AV_OpenPara_t ) );
    int rt = AM_AV_Open ( mTvPlayDevId, &para_av );
    if ( rt != DVB_SUCCESS ) {
        LOGD ("%s, dvbplayer_open fail %d %d\n!" , __FUNCTION__,  mTvPlayDevId, rt );
        return -1;
    }

    //open audio channle output
    AM_AOUT_OpenPara_t aout_para;
    memset ( &aout_para, 0, sizeof ( AM_AOUT_OpenPara_t ) );
    rt = AM_AOUT_Open ( mTvPlayDevId, &aout_para );
    if ( DVB_SUCCESS != rt ) {
        LOGD ("%s,  BUG: CANN'T OPEN AOUT\n", __FUNCTION__);
    }

    /*Register events*/
    AM_EVT_Subscribe ( mTvPlayDevId, AM_AV_EVT_AV_NO_DATA, av_evt_callback, this );
    AM_EVT_Subscribe ( mTvPlayDevId, AM_AV_EVT_AV_DATA_RESUME, av_evt_callback, this );
    AM_EVT_Subscribe ( mTvPlayDevId, AM_AV_EVT_VIDEO_SCAMBLED, av_evt_callback, this );
    AM_EVT_Subscribe ( mTvPlayDevId, AM_AV_EVT_AUDIO_SCAMBLED, av_evt_callback, this );
    AM_EVT_Subscribe ( mTvPlayDevId, AM_AV_EVT_VIDEO_NOT_SUPPORT, av_evt_callback, this );
    AM_EVT_Subscribe ( mTvPlayDevId, AM_AV_EVT_VIDEO_AVAILABLE, av_evt_callback, this );
    AM_EVT_Subscribe ( mTvPlayDevId, AM_AV_EVT_PLAYER_UPDATE_INFO, av_evt_callback, this );

    AM_AV_SetAudioCallback(0, av_audio_callback, this);
    return rt;
#else
    return -1;
#endif
}

int CAv::Close()
{
#ifdef SUPPORT_ADTV
LOGD ("%s,  av close\n", __FUNCTION__);
    if (mIsTsplayer == true) {
        //delPlayer();
        if (mFdAmVideo > 0) {
            close(mFdAmVideo);
            mFdAmVideo = -1;
        }
        tunnelid = 0;
        return 0;
    }

    int iRet = -1;

    iRet = AM_AV_Close(mTvPlayDevId);
    iRet = AM_AOUT_Close(mTvPlayDevId);
    if (mFdAmVideo > 0) {
        close(mFdAmVideo);
        mFdAmVideo = -1;
    }

    AM_AV_SetAudioCallback(0, NULL, NULL);

    return iRet;

#else
    return -1;
#endif

}

//TS_DEMOD or TS_MEMORY
int CAv::creatPlayer(am_tsplayer_input_source_type mode, int dmx)
{
    am_tsplayer_init_params stParm;
    uint32_t versionM, versionL;
    uint32_t instanceno;
    am_tsplayer_result ret = AM_TSPLAYER_OK;

    memset(&stParm, 0x0, sizeof(am_tsplayer_init_params));

    // TODO:
    stParm.dmx_dev_id = dmx;
    stParm.event_mask = 0;
    stParm.drmmode = TS_INPUT_BUFFER_TYPE_NORMAL;
    stParm.source = mode;

    ret = AmTsPlayer_create(stParm, &mSession);
    if (ret != AM_TSPLAYER_OK) {
        LOGD("%s: ret is %d\n", __FUNCTION__, ret);
        return -1;
    }

    ret = AmTsPlayer_setSurface(mSession, (void*)&tunnelid);
    if (ret != AM_TSPLAYER_OK) {
        LOGD("%s: ret is %d\n", __FUNCTION__, ret);
        return -1;
    } else {
        LOGD("AmTsPlayer_setSurface:%d\n", __LINE__);
    }

    ret = AmTsPlayer_getVersion(&versionM, &versionL);
    if (ret != AM_TSPLAYER_OK) {
        LOGD("%s: ret is %d\n", __FUNCTION__, ret);
        return -1;
    } else {
        LOGD("%s: tsplayer ver is %d.%d\n", __FUNCTION__, versionM, versionL);
    }

    ret = AmTsPlayer_getInstansNo(mSession, &instanceno);
    if (ret != AM_TSPLAYER_OK) {
        LOGD("%s: ret is %d\n", __FUNCTION__, ret);
        return -1;
    } else {
        LOGD("%s: player InstansNo is %d\n", __FUNCTION__, instanceno);
    }
    this->mCurAvEvent.type = AVEvent::EVENT_PLAY_INSTANCE;
    this->mCurAvEvent.param = (long)DECODE_ID_TYPE;
    this->mCurAvEvent.param1 = (long)instanceno;
    this->mpObserver->onEvent(this->mCurAvEvent);

    ret = AmTsPlayer_setWorkMode(mSession, TS_PLAYER_MODE_NORMAL);
    if (ret != AM_TSPLAYER_OK) {
        LOGD("%s: ret is %d\n", __FUNCTION__, ret);
        return -1;
    }
    ret = AmTsPlayer_setSyncMode(mSession, TS_SYNC_PCRMASTER );
    if (ret != AM_TSPLAYER_OK) {
        LOGD("%s: ret is %d\n", __FUNCTION__, ret);
        return -1;
    }
    ret = AmTsPlayer_registerCb(mSession, av_evt_callback_tsplayer, this);
    if (ret != AM_TSPLAYER_OK) {
        LOGD("%s: ret is %d\n", __FUNCTION__, ret);
        return -1;
    }
    //ui control static or blue or black
    //AmTsPlayer_setVideoBlackOut(mSession, false);
    return 0;
}

int CAv::delPlayer()
{
    am_tsplayer_result ret = AM_TSPLAYER_OK;

    if (mSession != INVALID_PLAYER_HDLE) {
        LOGD("%s: release------------------\n", __FUNCTION__);
        ret = AmTsPlayer_release(mSession);
        if (ret != AM_TSPLAYER_OK) {
            LOGD("%s: ret is %d\n", __FUNCTION__, ret);
            return -1;
        }
    }
    mSession = INVALID_PLAYER_HDLE;
    return 0;
}


DVR_VideoFormat_t CAv::toDvrVideoFormat(int codec)
{
   DVR_VideoFormat_t fmt = DVR_VIDEO_FORMAT_MPEG2;

   switch (codec)
   {
      case VFORMAT_MPEG12:
         fmt = DVR_VIDEO_FORMAT_MPEG1;
      break;
      case VFORMAT_MPEG4:
         fmt = DVR_VIDEO_FORMAT_MPEG2;
      break;
      case VFORMAT_H264:
      case VFORMAT_H264MVC:
         fmt = DVR_VIDEO_FORMAT_H264;
      break;
      case VFORMAT_HEVC:
         fmt = DVR_VIDEO_FORMAT_HEVC;
      break;
      case VFORMAT_VP9:
        fmt = DVR_VIDEO_FORMAT_VP9;
      break;
      default:
      break;
   }
   LOGD("video codec[%d]to fmt(%d)", codec, fmt);
   return fmt;
}

DVR_AudioFormat_t CAv::toDvrAudioFormat(int codec)
{
   DVR_AudioFormat_t fmt = DVR_AUDIO_FORMAT_MPEG;

   switch (codec)
   {
      case AFORMAT_MPEG:
         fmt = DVR_AUDIO_FORMAT_MPEG;
      break;
      case AFORMAT_EAC3:
         fmt = DVR_AUDIO_FORMAT_EAC3;
      break;
      case AFORMAT_AC3:
         fmt = DVR_AUDIO_FORMAT_AC3;
      break;
      case AFORMAT_DTS:
        fmt = DVR_AUDIO_FORMAT_DTS;
      break;
     case AFORMAT_AAC_LATM:
        fmt = DVR_AUDIO_FORMAT_LATM;
      break;
      case AFORMAT_AAC:
         fmt = DVR_AUDIO_FORMAT_AAC;
      break;
      case AFORMAT_PCM_S16LE:
      case AFORMAT_PCM_S16BE:
      case AFORMAT_PCM_BLURAY:
      case AFORMAT_ALAW:
      case AFORMAT_MULAW:
      case AFORMAT_PCM_U8:
      case AFORMAT_ADPCM:
        fmt = DVR_AUDIO_FORMAT_PCM;
        break;
      default:
      break;
   }
   LOGD("audio codec[%d]to fmt(%d)", codec, fmt);
   return fmt;
}

am_tsplayer_audio_codec CAv::toTsPlayerAudioFormat(int afmt)
{
    am_tsplayer_audio_codec fmt = AV_AUDIO_CODEC_AUTO;
    switch (afmt) {
        case AFORMAT_MPEG:
            fmt = AV_AUDIO_CODEC_MP2;
            break;
        case AFORMAT_AAC:
            fmt = AV_AUDIO_CODEC_AAC;
            break;
        case AFORMAT_AC3:
            fmt = AV_AUDIO_CODEC_AC3;
            break;
        case AFORMAT_DTS:
            fmt = AV_AUDIO_CODEC_DTS;
            break;
        case AFORMAT_AAC_LATM:
            fmt = AV_AUDIO_CODEC_LATM;
            break;
        case AFORMAT_EAC3:
            fmt = AV_AUDIO_CODEC_EAC3;
            break;
        case AFORMAT_DRA:
            fmt = AV_AUDIO_CODEC_DRA;
            break;
        default:
            fmt = AV_AUDIO_CODEC_AUTO;
            break;
    }
    LOGD("audio codec[%d]to tsplayer(%d)", afmt, fmt);
    return fmt;
}

am_tsplayer_video_codec CAv::toTsPlayerVideoFormat(int vfmt)
{
    am_tsplayer_video_codec fmt = AV_VIDEO_CODEC_AUTO;
    switch (vfmt) {
        case VFORMAT_MPEG12:
            fmt = AV_VIDEO_CODEC_MPEG2;
            break;
        case VFORMAT_MPEG4:
            fmt = AV_VIDEO_CODEC_MPEG4;
            break;
        case VFORMAT_H264:
        case VFORMAT_H264MVC:
            fmt = AV_VIDEO_CODEC_H264;
            break;
        case VFORMAT_HEVC:
            fmt = AV_VIDEO_CODEC_H265;
            break;
        case VFORMAT_VP9:
            fmt = AV_VIDEO_CODEC_VP9;
            break;
        case VFORMAT_AVS:
        case VFORMAT_AVS2:
            fmt = AV_VIDEO_CODEC_AVS;
            break;
        default:
            fmt = AV_VIDEO_CODEC_AUTO;
            break;
    }
    LOGD("video codec[%d]to fmt(%d)", vfmt, fmt);
    return fmt;
}

int CAv::GetVideoStatus(int *w, int *h, int *fps, int *interlace)
{
#ifdef SUPPORT_ADTV
    if (mIsTsplayer == true) {
        am_tsplayer_video_info stVidInfo;
        am_tsplayer_vdec_stat stVidStat;

        am_tsplayer_result ret = AM_TSPLAYER_OK;
        if (mSession == INVALID_PLAYER_HDLE) {
            return -1;
        }
        memset(&stVidInfo, 0x0, sizeof(am_tsplayer_video_info));
        ret = AmTsPlayer_getVideoInfo(mSession, &stVidInfo);
        if (ret != AM_TSPLAYER_OK) {
            LOGD("%s: ret is %d\n", __FUNCTION__, ret);
            return -1;
        }

        *w = stVidInfo.width;
        *h = stVidInfo.height;
        *fps = stVidInfo.framerate;

        memset(&stVidStat, 0x0, sizeof(am_tsplayer_vdec_stat));
        ret = AmTsPlayer_getVideoStat(mSession, &stVidStat);
        if (ret != AM_TSPLAYER_OK) {
            LOGD("%s: ret is %d\n", __FUNCTION__, ret);
            return -1;
        }

        *interlace = stVidStat.vf_type;

        return 0;
    }

    AM_AV_VideoStatus_t status;
    int ret = AM_AV_GetVideoStatus(mTvPlayDevId, &status);
    *w = status.src_w;
    *h = status.src_h;
    *fps = status.fps;
    *interlace = status.interlaced;
    return ret;

#else
    return -1;
#endif
}

int CAv::GetAudioStatus( int fmt[2], int sample_rate[2], int resolution[2], int channels[2],
    int lfepresent[2], int *frames, int *ab_size, int *ab_data, int *ab_free)
{
#ifdef SUPPORT_ADTV
    if (mIsTsplayer == true) {
        am_tsplayer_audio_info stAudioInfo;
        am_tsplayer_adec_stat stAudioStat;
        am_tsplayer_buffer_stat stAudBufStat;
        am_tsplayer_result ret = AM_TSPLAYER_OK;

        memset(&stAudioInfo, 0x0, sizeof(am_tsplayer_audio_info));
        memset(&stAudioStat, 0x0, sizeof(am_tsplayer_adec_stat));
        if (mSession == INVALID_PLAYER_HDLE) {
            return -1;
        }
        ret = AmTsPlayer_getAudioInfo(mSession, &stAudioInfo);
        if (ret != AM_TSPLAYER_OK) {
            LOGD("%s: ret is %d\n", __FUNCTION__, ret);
            return -1;
        }
        ret = AmTsPlayer_getAudioStat(mSession, &stAudioStat);
        if (ret != AM_TSPLAYER_OK) {
            LOGD("%s: ret is %d\n", __FUNCTION__, ret);
            return -1;
        }

        ret = AmTsPlayer_getBufferStat(mSession, TS_STREAM_AUDIO, &stAudBufStat);
        if (ret != AM_TSPLAYER_OK) {
            LOGD("%s: ret is %d\n", __FUNCTION__, ret);
            return -1;
        }

        if (sample_rate) {
            sample_rate[0] = stAudioInfo.sample_rate;
            //sample_rate[1] = stAudioInfo.sample_rate_orig;
        }

        if (resolution) {
            resolution[0] = stAudioInfo.bitrate;
            //resolution[1] = status.resolution_orig;
        }

        if (channels) {
            channels[0] = stAudioInfo.channels;
            //channels[1] = status.channels_orig;
        }

        if (frames)
            *frames = stAudioStat.frame_count;

        if (ab_size)
            *ab_size = stAudBufStat.size;

        if (ab_data)
            *ab_data = stAudBufStat.data_len;

        if (ab_free)
            *ab_free = stAudBufStat.free_len;

        return 0;
    }

    AM_AV_AudioStatus_t status;
    int ret = AM_AV_GetAudioStatus(mTvPlayDevId, &status);
    if (fmt) {
        fmt[0] = status.aud_fmt;
        fmt[1] = status.aud_fmt_orig;
    }
    if (sample_rate) {
        sample_rate[0] = status.sample_rate;
        sample_rate[1] = status.sample_rate_orig;
    }
    if (resolution) {
        resolution[0] = status.resolution;
        resolution[1] = status.resolution_orig;
    }
    if (channels) {
        channels[0] = status.channels;
        channels[1] = status.channels_orig;
    }
    if (lfepresent) {
        lfepresent[0] = status.lfepresent;
        lfepresent[1] = status.lfepresent_orig;
    }
    if (frames)
        *frames = status.frames;
    if (ab_size)
        *ab_size = status.ab_size;
    if (ab_data)
        *ab_data = status.ab_data;
    if (ab_free)
        *ab_free = status.ab_free;

    return ret;
#else
    return -1;
#endif
}

int CAv::SwitchTSAudio(int apid, int afmt)
{
#ifdef SUPPORT_ADTV

    if (mIsTsplayer == false) {
        return AM_AV_SwitchTSAudio(mTvPlayDevId, (unsigned short)apid, (AM_AV_AFormat_t)afmt);
    }
    if (mSession == INVALID_PLAYER_HDLE) {
        return -1;
    }
    if (mIsTimeshift) {
        //for timeshift
        if (player && mSession != INVALID_PLAYER_HDLE ) {
            LOGD("%s: set audio ,  start\n", __FUNCTION__);
            mPids.ad.pid = 0x1fff;
            mPids.audio.format = toDvrAudioFormat(afmt);
            mPids.audio.pid = apid;
            mPids.audio.format = toDvrAudioFormat(afmt);
            if ( player && mSession != INVALID_PLAYER_HDLE)
            dvr_wrapper_update_playback(player, &mPids);
        } else {
            LOGD("%s: set audio error, not start\n", __FUNCTION__);
        }
        return 0;
    }
    am_tsplayer_audio_params aparm;
    am_tsplayer_result ret = AM_TSPLAYER_OK;

    aparm.codectype = toTsPlayerAudioFormat(afmt);
    aparm.pid = apid;

    ret = AmTsPlayer_stopAudioDecoding(mSession);
    if (ret != AM_TSPLAYER_OK) {
        LOGD("%s: ret is %d\n", __FUNCTION__, ret);
        return -1;
    }

    ret = AmTsPlayer_setAudioParams(mSession, &aparm);
    if (ret != AM_TSPLAYER_OK) {
        LOGD("%s: ret is %d\n", __FUNCTION__, ret);
        return -1;
    }

    ret = AmTsPlayer_startAudioDecoding(mSession);
    if (ret != AM_TSPLAYER_OK) {
        LOGD("%s: ret is %d\n", __FUNCTION__, ret);
        return -1;
    }

    return 0;
#else
    return -1;
#endif
}

int CAv::ResetAudioDecoder()
{
#ifdef SUPPORT_ADTV
    if (mIsTsplayer == false) {
        return AM_AV_ResetAudioDecoder(mTvPlayDevId);
    }
    if (mIsTsplayer == true) {
        if (mIsTimeshift == false) {
            am_tsplayer_result ret = AM_TSPLAYER_OK;
            if (mSession == INVALID_PLAYER_HDLE) {
                return -1;
            }
            ret = AmTsPlayer_stopAudioDecoding(mSession);
            if (ret != AM_TSPLAYER_OK) {
                LOGD("%s: ret is %d\n", __FUNCTION__, ret);
                return -1;
            }

            ret = AmTsPlayer_startAudioDecoding(mSession);
            if (ret != AM_TSPLAYER_OK) {
                LOGD("%s: ret is %d\n", __FUNCTION__, ret);
                return -1;
            }
        } else {
            //for timeshift
            if (player && mSession != INVALID_PLAYER_HDLE ) {
                LOGD("%s: restart audio error, not support\n", __FUNCTION__);
            } else {
                LOGD("%s: restart audio error, not support\n", __FUNCTION__);
            }
        }
    }
    return 0;
#else
    return -1;
#endif
}

int CAv::SetADAudio(unsigned int enable, int apid, int afmt)
{
    LOGD("%s:  start\n", __FUNCTION__);
#ifdef SUPPORT_ADTV
    if (mIsTsplayer == false) {
        return AM_AV_SetAudioAd(mTvPlayDevId, enable, apid, (AM_AV_AFormat_t) afmt);
    } else if (mIsTsplayer == true) {
        if (mIsTimeshift == false) {
            //for live mode
            am_tsplayer_audio_params aparm;
            if (mSession == INVALID_PLAYER_HDLE) {
                return -1;
            }
            switch (afmt) {
                case 0:
                    aparm.codectype = AV_AUDIO_CODEC_MP2;
                break;
                case 2:
                    aparm.codectype = AV_AUDIO_CODEC_AAC;
                break;
                case 3:
                    aparm.codectype = AV_AUDIO_CODEC_AC3;
                break;
                case 6:
                    aparm.codectype = AV_AUDIO_CODEC_DTS;
                break;
                case 19:
                    aparm.codectype = AV_AUDIO_CODEC_LATM;
                break;
                default:
                    aparm.codectype = AV_AUDIO_CODEC_AUTO;
                break;
            }
            aparm.pid = apid;
            am_tsplayer_result ret = AM_TSPLAYER_OK;
            ret = AmTsPlayer_setADParams(mSession, &aparm);
            if (ret != AM_TSPLAYER_OK) {
                LOGD("%s: ret is %d\n", __FUNCTION__, ret);
                return -1;
            }

            ret = AmTsPlayer_enableADMix(mSession);
            if (ret != AM_TSPLAYER_OK) {
                LOGD("%s: ret is %d\n", __FUNCTION__, ret);
                return -1;
            }
        } else {
            //for timeshift
            if (player && mSession != INVALID_PLAYER_HDLE ) {
                LOGD("%s: set ad ,  start\n", __FUNCTION__);
                mPids.ad.pid = apid;
                mPids.ad.format = toDvrAudioFormat(afmt);
                if ( player && mSession != INVALID_PLAYER_HDLE)
                dvr_wrapper_update_playback(player, &mPids);
            } else {
                LOGD("%s: set ad error, not start\n", __FUNCTION__);
            }
        }
    }
    return 0;
#else
    return -1;
#endif
}

int CAv::SetTSSource(int ts_source)
{
#ifdef SUPPORT_ADTV
    char *cmd;

    if (mIsTsplayer == false) {
        return AM_AV_SetTSSource(mTvPlayDevId, (AM_AV_TSSource_t)ts_source);
    }

    switch (ts_source)
    {
        case 0:
            cmd = "ts0";
        break;
        case 1:
            cmd = "ts1";
        break;
        case 2:
            cmd = "ts2";
        break;
        case 3:
            cmd = "hiu";
        break;
        case 4:
            cmd = "hiu1";
        break;
        case 5:
            cmd = "dmx0";
        break;
        case 6:
            cmd = "dmx1";
        break;
        case 7:
            cmd = "dmx2";
        break;
        default:
            LOGD("illegal ts source %d", ts_source);
        return -1;
        break;
    }

    tvWriteSysfs("/sys/class/stb/source", cmd);
    return 0;
#else
    return -1;
#endif
}

int CAv::StartTS(unsigned short vpid, unsigned short apid, unsigned short pcrid, int vfmt, int afmt)
{
#ifdef SUPPORT_ADTV
    if (mIsTsplayer == false) {
        return AM_AV_StartTSWithPCR (mTvPlayDevId, vpid, apid, pcrid, (AM_AV_VFormat_t)vfmt, (AM_AV_AFormat_t)afmt);
    }
    LOGD("%s: ---------\n", __FUNCTION__);
    delPlayer();
    creatPlayer(TS_DEMOD, 0);
    LOGD("%s: ---------\n", __FUNCTION__);
    if (mSession == INVALID_PLAYER_HDLE) {
        LOGD("%s: creat tsplayer error for live mode\n", __FUNCTION__);
        return -1;
    }
    check_scramble_time = 0;
    am_tsplayer_result ret = AM_TSPLAYER_OK;

    am_tsplayer_avsync_mode avsyncMode = TS_SYNC_PCRMASTER;
    am_tsplayer_video_params vparm;
    am_tsplayer_audio_params aparm;

    LOGD("%s:%d afmt:%d\n", __FUNCTION__, __LINE__, afmt);
    if (mSession != INVALID_PLAYER_HDLE)
        ret = AmTsPlayer_setSyncMode(mSession, avsyncMode);
    if (ret != AM_TSPLAYER_OK) {
        LOGD("%s: ret is %d\n", __FUNCTION__, ret);
        return -1;
    }
    if (mSession != INVALID_PLAYER_HDLE)
        ret = AmTsPlayer_setPcrPid(mSession, pcrid);
    if (ret != AM_TSPLAYER_OK) {
        LOGD("%s: ret is %d\n", __FUNCTION__, ret);
        return -1;
    }

    vparm.codectype = toTsPlayerVideoFormat(vfmt);
    vparm.pid = vpid;
    if (mSession != INVALID_PLAYER_HDLE && VALID_PID(vparm.pid)) {
        ret = AmTsPlayer_setVideoParams(mSession, &vparm);
        if (ret != AM_TSPLAYER_OK) {
            LOGD("%s: ret is %d\n", __FUNCTION__, ret);
            return -1;
        }
        ret = AmTsPlayer_startVideoDecoding(mSession);
        if (ret != AM_TSPLAYER_OK) {
            LOGD("%s: ret is %d\n", __FUNCTION__, ret);
            return -1;
        }
    }

    aparm.codectype = toTsPlayerAudioFormat(afmt);
    LOGD("%s:%d acodectype:%d\n", __FUNCTION__, __LINE__, aparm.codectype);

    aparm.pid = apid;
    if (mSession != INVALID_PLAYER_HDLE)
        ret = AmTsPlayer_setAudioParams(mSession, &aparm);
    if (ret != AM_TSPLAYER_OK) {
        LOGD("%s: ret is %d\n", __FUNCTION__, ret);
        return -1;
    }
    if (mSession != INVALID_PLAYER_HDLE)
        ret = AmTsPlayer_startAudioDecoding(mSession);
    if (ret != AM_TSPLAYER_OK) {
        LOGD("%s: ret is %d\n", __FUNCTION__, ret);
        return -1;
    }

    return 0;
#else
    return -1;
#endif
}

int CAv::StopTS()
{
#ifdef SUPPORT_ADTV
    if (mIsTsplayer == false) {
        return AM_AV_StopTS(mTvPlayDevId);
    }

    am_tsplayer_result ret = AM_TSPLAYER_OK;
    //only stop live mode
    if (mIsTimeshift == false) {
        if (mSession != INVALID_PLAYER_HDLE)
            ret = AmTsPlayer_setPcrPid(mSession, 0x1fff);
        if (ret != AM_TSPLAYER_OK) {
            LOGD("%s: ret is %d\n", __FUNCTION__, ret);
            return -1;
        }
        if (mSession != INVALID_PLAYER_HDLE)
            ret = AmTsPlayer_stopAudioDecoding(mSession);
        if (ret != AM_TSPLAYER_OK) {
            LOGD("%s: ret is %d\n", __FUNCTION__, ret);
            return -1;
        }
        if (mSession != INVALID_PLAYER_HDLE)
            ret = AmTsPlayer_stopVideoDecoding(mSession);
        if (ret != AM_TSPLAYER_OK) {
            LOGD("%s: ret is %d\n", __FUNCTION__, ret);
            return -1;
        }
        if (mSession != INVALID_PLAYER_HDLE) {
            LOGD("%s: ---------\n", __FUNCTION__);
            delPlayer();
            LOGD("%s: ---------\n", __FUNCTION__);
            return 0;
        }
    } else {
        LOGD("%s: -----stoptimeshift when stop ts--when in timeshift mode--\n", __FUNCTION__);
        stopTimeShift();
    }

    return 0;
#else
    return -1;
#endif
}

int CAv::AudioGetOutputMode(int *mode)
{
#ifdef SUPPORT_ADTV
    if (mIsTsplayer == false) {
        return AM_AOUT_GetOutputMode(mTvPlayDevId, (AM_AOUT_OutputMode_t *)mode);
    }

    am_tsplayer_result ret = AM_TSPLAYER_OK;

    am_tsplayer_audio_stereo_mode emAudioMode = AV_AUDIO_STEREO;
    if (mSession != INVALID_PLAYER_HDLE)
        ret = AmTsPlayer_getAudioStereoMode(mSession, &emAudioMode);
    if (ret != AM_TSPLAYER_OK) {
        LOGD("%s: ret is %d\n", __FUNCTION__, ret);
        return -1;
    }

    switch (emAudioMode) {
        case AV_AUDIO_STEREO:
            *mode = TV_AOUT_OUTPUT_STEREO;
        break;
        case AV_AUDIO_LEFT:
            *mode = TV_AOUT_OUTPUT_DUAL_LEFT;
        break;
        case AV_AUDIO_RIGHT:
            *mode = TV_AOUT_OUTPUT_DUAL_RIGHT;
        break;
        case AV_AUDIO_SWAP:
            *mode = TV_AOUT_OUTPUT_SWAP;
        break;
        default:
            *mode = TV_AOUT_OUTPUT_STEREO;
        break;
    }

    return 0;
#else
    return -1;
#endif
}

int CAv::AudioSetOutputMode(int mode)
{
#ifdef SUPPORT_ADTV
    if (mIsTsplayer == false) {
        return AM_AOUT_SetOutputMode(mTvPlayDevId, (AM_AOUT_OutputMode_t)mode);
    }

    am_tsplayer_result ret = AM_TSPLAYER_OK;

    am_tsplayer_audio_stereo_mode emAudioMode = AV_AUDIO_STEREO;

    switch (mode) {
        case TV_AOUT_OUTPUT_STEREO :
            emAudioMode = AV_AUDIO_STEREO;
        break;
        case TV_AOUT_OUTPUT_DUAL_LEFT:
            emAudioMode = AV_AUDIO_LEFT;
        break;
        case TV_AOUT_OUTPUT_DUAL_RIGHT:
            emAudioMode = AV_AUDIO_RIGHT;
        break;
        case TV_AOUT_OUTPUT_SWAP:
            emAudioMode = AV_AUDIO_SWAP;
        break;
        default:
        break;
    }
    if (mSession != INVALID_PLAYER_HDLE)
        ret = AmTsPlayer_setAudioStereoMode(mSession, emAudioMode);
    if (ret != AM_TSPLAYER_OK) {
        LOGD("%s: ret is %d\n", __FUNCTION__, ret);
        return -1;
    }

    return 0;
#else
    return -1;
#endif
}

int CAv::AudioSetPreGain(float pre_gain)
{
#ifdef SUPPORT_ADTV
    if (mIsTsplayer == false) {
        return AM_AOUT_SetPreGain(0, pre_gain);
    }

    return -1;
#else
    return -1;
#endif

}

int CAv::AudioGetPreGain(float *gain)
{
#ifdef SUPPORT_ADTV
    if (mIsTsplayer == false) {
        return AM_AOUT_GetPreGain(0, gain);
    }

    return -1;
#else
    return -1;
#endif

}

int CAv::AudioSetPreMute(unsigned int mute)
{
#ifdef SUPPORT_ADTV
    if (mIsTsplayer == false) {
        return AM_AOUT_SetPreMute(0, mute);
    }

    return -1;
#else
    return -1;
#endif

}

int CAv::AudioGetPreMute(unsigned int *mute)
{
    int ret = -1;
#ifdef SUPPORT_ADTV
    if (mIsTsplayer == false) {
        bool btemp_mute = 0;
        ret = AM_AOUT_GetPreMute(0, (AM_Bool_t *) &btemp_mute);
        *mute = btemp_mute ? 1 : 0;
        return ret;
    }

    return -1;
#endif
    return ret;
}

int CAv::EnableVideoBlackout()
{
    LOGD("%s: mVideoLayerState is %d\n", __FUNCTION__, mVideoLayerState);
    mVideoLayerState = VIDEO_LAYER_NONE;
#ifdef SUPPORT_ADTV
    if (mIsTsplayer == false) {
        return AM_AV_EnableVideoBlackout(mTvPlayDevId);
    }
    //tvWriteSysfs(VIDEO_BLACKOUT_POLICY, 1);
    // am_tsplayer_result ret = AM_TSPLAYER_OK;
    // if (mSession != INVALID_PLAYER_HDLE)
    //     ret = AmTsPlayer_setVideoBlackOut(mSession, 1);
    // if (ret != AM_TSPLAYER_OK) {
    //     LOGD("%s: ret is %d\n", __FUNCTION__, ret);
    //     return -1;
    // }

    return 0;
#else
    return -1;
#endif

}

int CAv::DisableVideoBlackout()
{
    LOGD("%s: mVideoLayerState is %d\n", __FUNCTION__, mVideoLayerState);
    mVideoLayerState = VIDEO_LAYER_NONE;
#ifdef SUPPORT_ADTV
    if (mIsTsplayer == false) {
        return AM_AV_DisableVideoBlackout(mTvPlayDevId);
    }
    //tvWriteSysfs(VIDEO_BLACKOUT_POLICY, 0);
    // am_tsplayer_result ret = AM_TSPLAYER_OK;
    // if (mSession != INVALID_PLAYER_HDLE)
    //     ret = AmTsPlayer_setVideoBlackOut(mSession, 0);
    // if (ret != AM_TSPLAYER_OK) {
    //     LOGD("%s: ret is %d\n", __FUNCTION__, ret);
    //     return -1;
    // }

    return 0;
#else
    return -1;
#endif

}

int CAv::DisableVideoWithBlueColor()
{
    LOGD("%s: mVideoLayerState is %d\n", __FUNCTION__, mVideoLayerState);

    mVideoLayerState = VIDEO_LAYER_BLUE;
    SetVideoScreenColor(VIDEO_LAYER_BLUE); // Show blue with vdin0, postblending disabled
#ifdef SUPPORT_ADTV
    if (mIsTsplayer == false) {
        return AM_AV_DisableVideo(mTvPlayDevId);
    }
    tvWriteSysfs(VIDEO_DISABLE_VIDEO, 1);
    // am_tsplayer_result ret = AM_TSPLAYER_OK;
    // if (mSession != INVALID_PLAYER_HDLE)
    //     ret = AmTsPlayer_hideVideo(mSession);
    // if (ret != AM_TSPLAYER_OK) {
    //     LOGD("%s: ret is %d\n", __FUNCTION__, ret);
    //     return -1;
    // }

    return 0;
#else
    return -1;
#endif

}

int CAv::DisableVideoWithBlackColor()
{
    LOGD("%s: mVideoLayerState is %d\n", __FUNCTION__, mVideoLayerState);

    mVideoLayerState = VIDEO_LAYER_BLACK;
    SetVideoScreenColor(VIDEO_LAYER_BLACK);; // Show black with vdin0, postblending disabled

#ifdef SUPPORT_ADTV
    if (mIsTsplayer == false) {
        return AM_AV_DisableVideo(mTvPlayDevId);
    }
    tvWriteSysfs(VIDEO_DISABLE_VIDEO, 1);
    // am_tsplayer_result ret = AM_TSPLAYER_OK;
    // if (mSession != INVALID_PLAYER_HDLE)
    //     ret = AmTsPlayer_hideVideo(mSession);
    // if (ret != AM_TSPLAYER_OK) {
    //     LOGD("%s: ret is %d\n", __FUNCTION__, ret);
    //     return -1;
    // }

    return 0;
#else
    return -1;
#endif
}

//just enable video
int CAv::EnableVideoNow(bool IsShowTestScreen)
{
    LOGD("%s:IsShowTestScreen is %d, mVideoLayerState is %d\n", __FUNCTION__, IsShowTestScreen, mVideoLayerState);

    if (mVideoLayerState == VIDEO_LAYER_ENABLE) {
        LOGW("video is enabled");
        return 0;
    }
    mVideoLayerState = VIDEO_LAYER_ENABLE;
    if (IsShowTestScreen) {
        LOGD("%s: eableVideoWithBlackColor SwitchSourceTime = %fs", __FUNCTION__,getUptimeSeconds());
        SetVideoScreenColor(VIDEO_LAYER_BLACK);;
    }

#ifdef SUPPORT_ADTV
    if (mIsTsplayer == false) {
        return AM_AV_EnableVideo(mTvPlayDevId);
    }
    tvWriteSysfs(VIDEO_DISABLE_VIDEO, 0);
    // am_tsplayer_result ret = AM_TSPLAYER_OK;
    // if (mSession != INVALID_PLAYER_HDLE)
    //     ret = AmTsPlayer_showVideo(mSession);
    // if (ret != AM_TSPLAYER_OK) {
    //     LOGD("%s: ret is %d\n", __FUNCTION__, ret);
    //     return -1;
    // }

    return 0;
#else
    return -1;
#endif
}

//call disable video 2
int CAv::ClearVideoBuffer()
{
    LOGD("%s", __FUNCTION__);
#ifdef SUPPORT_ADTV
    if (mIsTsplayer == false) {
        return AM_AV_ClearVideoBuffer(mTvPlayDevId);
    }
    //
    tvWriteSysfs(VIDEO_DISABLE_VIDEO, 2);
    return 0;
#else
    return -1;
#endif
}

int CAv::startTimeShift(void *para)
{
    LOGD("---set dmxsource hiu--||%s", __FUNCTION__);
#ifdef SUPPORT_ADTV
    if (mIsTsplayer == false) {
        return AM_AV_StartTimeshift(mTvPlayDevId, (AM_AV_TimeshiftPara_t *)para);
    }
    mIsTimeshift = true;
    // TODO:
    //del player first
    int error;
    delPlayer();
    creatPlayer(TS_MEMORY, 0);
    if (mSession == INVALID_PLAYER_HDLE) {
        LOGD("creat tsplayer error for inject mode :%s", __FUNCTION__);
        return -1;
    }
    check_scramble_time = 0;
    //set dmx source to hiu
    AM_DMX_SetSource ( 0, AM_DMX_SRC_HIU );
    this->mCurAvEvent.timeshiftStarttime = -1;

    memset(&play_params, 0, sizeof(play_params));
    memset(&mPids, 0, sizeof(mPids));
    memcpy(&mPids, (DVR_PlaybackPids_t*)para, sizeof(DVR_PlaybackPids_t));

    play_params.dmx_dev_id = 0;
    play_params.event_fn = PlayEventHandler;
    play_params.event_userdata = this;
    if (mPids.video.pid > 0 && mPids.video.pid < 0x1fff) {
        LOGD("%s: -----play video----\n", __FUNCTION__);
        play_params.block_size = 188 * 1024;
    } else {
        LOGD("%s: -----play radio----\n", __FUNCTION__);
        play_params.block_size = 188 * 5;
    }
    play_params.vendor = DVR_PLAYBACK_VENDOR_AML;
    play_params.is_notify_time = DVR_TRUE;
    play_params.is_timeshift = DVR_TRUE;// : DVR_FALSE;
    play_params.playback_handle =
            (Playback_DeviceHandle_t)mSession;

    snprintf(play_params.location, DVR_MAX_LOCATION_SIZE,
              "%s/%s", "/data/vendor/tvserver", DEFAULT_TIMESHIFT_BASENAME);
    error = dvr_wrapper_open_playback(&player, &play_params);

    LOGD("Start play/play loc[%s] audio fmt [%d]play_params.block_size[%d]pid[%d]", play_params.location, mPids.audio.format, play_params.block_size, mPids.video.pid);
    error = dvr_wrapper_start_playback(player, (DVR_PlaybackFlag_t)0, (DVR_PlaybackPids_t*)para);
    if (error)
    {
     LOGD("Start play/play failed, error %d", error);
     return -1;
    }

    return 0;
#else
    return -1;
#endif
}

int CAv::stopTimeShift()
{
    LOGD("%s", __FUNCTION__);

#ifdef SUPPORT_ADTV
    if (mIsTsplayer == false) {
        return AM_AV_StopTimeshift(mTvPlayDevId);
    }
    // TODO:
    if (mIsTimeshift == true) {
        if (player && mSession != INVALID_PLAYER_HDLE)
            dvr_wrapper_stop_playback(player);
        if (player && mSession != INVALID_PLAYER_HDLE)
            dvr_wrapper_close_playback(player);
        player = NULL;
        mIsTimeshift = false;
        if (mSession != INVALID_PLAYER_HDLE) {
            LOGD("%s: ---------\n", __FUNCTION__);
            delPlayer();
            LOGD("%s: ---------\n", __FUNCTION__);
            return 0;
        }
    }

    return -1;
#else
    return -1;
#endif
}

int CAv::pauseTimeShift()
{
    LOGD("%s", __FUNCTION__);

#ifdef SUPPORT_ADTV
    if (mIsTsplayer == false) {
        return AM_AV_PauseTimeshift (mTvPlayDevId);
    }
    // TODO:
    if (player && mSession != INVALID_PLAYER_HDLE)
         dvr_wrapper_pause_playback(player);
    return -1;
#else
    return -1;
#endif
}

int CAv::resumeTimeShift()
{
    LOGD("%s", __FUNCTION__);

#ifdef SUPPORT_ADTV
    if (mIsTsplayer == false) {
        return AM_AV_ResumeTimeshift (mTvPlayDevId);
    }
    // TODO:
    if (player && mSession != INVALID_PLAYER_HDLE) {
        dvr_wrapper_set_playback_speed(player, 100);
        dvr_wrapper_resume_playback(player);
    }
    return -1;
#else
    return -1;
#endif
}

int CAv::seekTimeShift(int pos, bool start)
{
    LOGD ( "%s: [pos:%d start:%d]", __FUNCTION__, pos, start);

#ifdef SUPPORT_ADTV
    if (mIsTsplayer == false) {
        return AM_AV_SeekTimeshift (mTvPlayDevId, pos, start);
    }
    // TODO:
    if (player && mSession != INVALID_PLAYER_HDLE )
         dvr_wrapper_seek_playback (player, pos);
    return -1;
#else
    return -1;
#endif
}

int CAv::setTimeShiftSpeed(int speed)
{
    LOGD ( "%s: [%d]", __FUNCTION__, speed);
    int ret = 0;
#ifdef SUPPORT_ADTV
    if (mIsTsplayer == false) {
        if (speed == 0)
            ret = AM_AV_ResumeTimeshift (mTvPlayDevId);
        else if (speed < 0)
            ret = AM_AV_FastBackwardTimeshift(mTvPlayDevId, -speed);
        else
            ret = AM_AV_FastForwardTimeshift(mTvPlayDevId, speed);
    }
    // TODO:
    float dvr_speed = speed * 100;
    if (player  && mSession != INVALID_PLAYER_HDLE )
        dvr_wrapper_set_playback_speed(player, dvr_speed);
    return -1;
#endif
    return ret;
}

int CAv::switchTimeShiftAudio(int apid, int afmt)
{
    LOGD ( "%s: [pid:%d, fmt:%d]", __FUNCTION__, apid, afmt);
#ifdef SUPPORT_ADTV
    if (mIsTsplayer == false) {
        return AM_AV_SwitchTimeshiftAudio (mTvPlayDevId, apid, afmt);
    }
    // TODO:
    LOGD ( "%s: [pid:%d, fmt:%d] not code error", __FUNCTION__, apid, afmt);
    mPids.audio.pid = apid;
    mPids.audio.format = toDvrAudioFormat(afmt);
    if ( player && mSession != INVALID_PLAYER_HDLE)
        dvr_wrapper_update_playback(player, &mPids);
    return -1;
#else
    return -1;
#endif
}

int CAv::playTimeShift()
{
    LOGD("%s", __FUNCTION__);

#ifdef SUPPORT_ADTV
    if (mIsTsplayer == false) {
        return AM_AV_PlayTimeshift (mTvPlayDevId);
    }
    // TODO:
    LOGD("%s not code", __FUNCTION__);
    return -1;
#else
    return -1;
#endif
}

//auto enable,
int CAv::EnableVideoAuto()
{
    LOGD("%s: mVideoLayerState is %d\n", __FUNCTION__, mVideoLayerState);
    if (mVideoLayerState == VIDEO_LAYER_ENABLE) {
        LOGW("video is enable");
        return 0;
    }
    mVideoLayerState = VIDEO_LAYER_ENABLE;
    LOGD("%s: eableVideoWithBlackColor\n", __FUNCTION__);
    SetVideoScreenColor(VIDEO_LAYER_BLACK); // Show black with vdin0, postblending disabled
    ClearVideoBuffer();//disable video 2
    return 0;
}

int CAv::WaittingVideoPlaying(int minFrameCount , int waitTime )
{
    LOGD("%s", __FUNCTION__);
    static const int COUNT_FOR_TIME = 20;
    int times = waitTime / COUNT_FOR_TIME;

    for (int i = 0; i < times; i++) {
        if (videoIsPlaying(minFrameCount)) {
            return 0;
        }
    }

    return -1;
}

int CAv::EnableVideoWhenVideoPlaying(int minFrameCount, int waitTime)
{
    LOGD("%s", __FUNCTION__);
    int ret = WaittingVideoPlaying(minFrameCount, waitTime);
    if (ret == 0) { //ok to playing
        EnableVideoNow(true);
    }
    return ret;
}

bool CAv::videoIsPlaying(int minFrameCount)
{
    LOGD("%s", __FUNCTION__);
    int value[2] = {0};
    value[0] = getVideoFrameCount();
    usleep(20 * 1000);
    value[1] = getVideoFrameCount();
    LOGD("videoIsPlaying framecount =%d = %d", value[0], value[1]);

    if (value[1] >= minFrameCount && (value[1] > value[0]))
        return true;

    return false;
}

int CAv::getVideoFrameCount()
{
    char buf[32] = {0};
    const char* path = NULL;
    int linux_version = getKernelMajorVersion();

    if (linux_version > 4)
      path = PATH_FRAME_COUNT_54;
    else
      path = PATH_FRAME_COUNT_49;

    tvReadSysfs(path, buf);
    return atoi(buf);
}

int CAv::setFEStatus(int value __unused)
{
#ifdef SUPPORT_ADTV
    //return AM_AV_setFEStatus(mTvPlayDevId,value);
    return -1;
#else
    return -1;
#endif
}

tvin_sig_fmt_t CAv::getVideoResolutionToFmt()
{
    char buf[32] = {0};
    tvin_sig_fmt_e sig_fmt = TVIN_SIG_FMT_HDMI_1920X1080P_60HZ;

    tvReadSysfs(SYS_VIDEO_FRAME_HEIGHT, buf);
    int height = atoi(buf);
    if (height <= 576) {
        sig_fmt = TVIN_SIG_FMT_HDMI_720X480P_60HZ;
    } else if (height > 576 && height <= 720) {
        sig_fmt = TVIN_SIG_FMT_HDMI_1280X720P_60HZ;
    } else if (height > 720 && height <= 1088) {
        sig_fmt = TVIN_SIG_FMT_HDMI_1920X1080P_60HZ;
    } else {
        sig_fmt = TVIN_SIG_FMT_HDMI_3840_2160_00HZ;
    }
    return sig_fmt;
}

int CAv::SetVideoScreenColor ( int color )
{
    int ret = 0;
    switch (color) {
        case VIDEO_LAYER_BLUE:
            ret = SetVideoScreenColor ( 0, 41, 240, 110 );
            break;
        case VIDEO_LAYER_BLACK:
            ret = SetVideoScreenColor ( 0, 16, 128, 128 );
            break;
        default:
            ret = SetVideoScreenColor ( 0, 16, 128, 128 );
            break;
    }
    return ret;
}

int CAv::SetVideoScreenColor ( int vdin_blending_mask, int y, int u, int v )
{
    unsigned long value = vdin_blending_mask << 24;
    value |= ( unsigned int ) ( y << 16 ) | ( unsigned int ) ( u << 8 ) | ( unsigned int ) ( v );

    LOGD("%s, vdin_blending_mask:%d,y:%d,u:%d,v:%d", __FUNCTION__, vdin_blending_mask, y, u, v);

    char val[64] = {0};
    sprintf(val, "0x%lx", ( unsigned long ) value);
    tvWriteSysfs(VIDEO_TEST_SCREEN, val);
    return 0;
}

int CAv::SetVideoLayerStatus ( int value )
{
    LOGD("%s, value = %d" , __FUNCTION__, value);

    char val[64] = {0};
    sprintf(val, "%d", value);
    tvWriteSysfs(VIDEO_DISABLE_VIDEO, val);
    return 0;
}

int CAv::setVideoScreenMode ( int value )
{
    char val[64] = {0};
    sprintf(val, "%d", value);
    tvWriteSysfs(VIDEO_SCREEN_MODE, val);
    return 0;
}

int CAv::getVideoScreenMode()
{
    char buf[32] = {0};

    tvReadSysfs(VIDEO_SCREEN_MODE, buf);
    return atoi(buf);
}

/**
 * @function: set test pattern on video layer.
 * @param r,g,b int 0~255.
 */
int CAv::setRGBScreen(int r, int g, int b)
{
    int value = ((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff);
    return tvWriteSysfs(VIDEO_RGB_SCREEN, value, 16);
}

int CAv::getRGBScreen()
{
    char value[32] = {0};
    tvReadSysfs(VIDEO_RGB_SCREEN, value);
    return strtol(value, NULL, 10);
}


int CAv::setVideoAxis ( int h, int v, int width, int height )
{
    LOGD("%s, %d %d %d %d", __FUNCTION__, h, v, width, height);

    char value[64] = {0};
    sprintf(value, "%d %d %d %d", h, v, width, height);
    tvWriteSysfs(VIDEO_AXIS, value);
    return 0;
}

video_display_resolution_t CAv::getVideoDisplayResolution()
{
    video_display_resolution_t  resolution;
    char attrV[SYS_STR_LEN] = {0};

    tvReadSysfs(VIDEO_DEVICE_RESOLUTION, attrV);

    if (strncasecmp(attrV, "1366x768", strlen ("1366x768")) == 0) {
        resolution = VPP_DISPLAY_RESOLUTION_1366X768;
    } else if (strncasecmp(attrV, "3840x2160", strlen ("3840x2160")) == 0) {
        resolution = VPP_DISPLAY_RESOLUTION_3840X2160;
    } else if (strncasecmp(attrV, "1920x1080", strlen ("1920x1080")) == 0) {
        resolution = VPP_DISPLAY_RESOLUTION_1920X1080;
    } else {
        LOGW("video display resolution is = (%s) not define , default it", attrV);
        resolution = VPP_DISPLAY_RESOLUTION_1920X1080;
    }

    return resolution;
}
#ifdef SUPPORT_ADTV
void CAv::av_evt_callback_tsplayer(void *user_data, am_tsplayer_event *event)
{
    CAv *pAv = (CAv *)user_data;
    am_tsplayer_result ret = AM_TSPLAYER_OK;
    int syncintsno = 0;
    int feState = CFrontEnd::getInstance()->getStatus();

    if (NULL == pAv) {
        LOGD ( "%s, ERROR : av_evt_callback_tsplayer NULL == pTv\n", __FUNCTION__ );
        return;
    }
    if (pAv->mpObserver == NULL) {
        LOGD ( "%s, ERROR : mpObserver NULL == mpObserver\n", __FUNCTION__ );
        return;
    }

    LOGD("video_callback type %d\n", event? event->type : 0);

    switch (event->type) {
        case AM_TSPLAYER_EVENT_TYPE_DATA_LOSS:
            LOGD("event type demod data loss");
            if ((feState & TV_FE_HAS_LOCK) == TV_FE_HAS_LOCK) {
                pAv->check_scramble_time++;
                if (pAv->old_dmx == true ||
                    pAv->check_scramble_time > 3) {
                    pAv->mCurAvEvent.type = AVEvent::EVENT_AV_SCAMBLED;
                    pAv->mCurAvEvent.param = (long)event->type;
                    pAv->mpObserver->onEvent(pAv->mCurAvEvent);
                    pAv->check_scramble_time = 0;
                }
            } else if ((feState & TV_FE_TIMEDOUT) == TV_FE_TIMEDOUT) {
                pAv->check_scramble_time = 0;
                LOGD("tv fe unlock");
            }
            break;
        case AM_TSPLAYER_EVENT_TYPE_DATA_RESUME:
            pAv->mCurAvEvent.type = AVEvent::EVENT_AV_RESUEM;
            //pAv->mCurAvEvent.param = ( long )event;
            pAv->mpObserver->onEvent(pAv->mCurAvEvent);
            break;
        case AM_TSPLAYER_EVENT_TYPE_SCRAMBLING:
            pAv->mCurAvEvent.type = AVEvent::EVENT_AV_SCAMBLED;
            pAv->mCurAvEvent.param = (long)event->event.scramling.scramling;
            pAv->mpObserver->onEvent(pAv->mCurAvEvent);
            break;
            /*case AM_AV_EVT_VIDEO_NOT_SUPPORT: {
            pAv->mCurAvEvent.type = AVEvent::EVENT_AV_UNSUPPORT;
            pAv->mCurAvEvent.param = ( long )param;
            pAv->mpObserver->onEvent(pAv->mCurAvEvent);
            break;
            }*/
        case AM_TSPLAYER_EVENT_TYPE_FIRST_FRAME:
        case AM_TSPLAYER_EVENT_TYPE_DECODE_FIRST_FRAME_AUDIO: {
            pAv->mCurAvEvent.type = AVEvent::EVENT_AV_VIDEO_AVAILABLE;
            pAv->mCurAvEvent.param = ( long )&event->event.video_format;
            pAv->mpObserver->onEvent(pAv->mCurAvEvent);

            ret = AmTsPlayer_getSyncInstansNo(pAv->mSession, &syncintsno);
            if (ret != AM_TSPLAYER_OK) {
                LOGD("%s: ret is %d\n", __FUNCTION__, ret);
            } else {
                LOGD("%s: mediasync InstansNo is %d\n", __FUNCTION__, syncintsno);
            }

            pAv->mCurAvEvent.type = AVEvent::EVENT_PLAY_INSTANCE;
            pAv->mCurAvEvent.param = (long)SYNC_ID_TYPE;
            pAv->mCurAvEvent.param1 = (long)syncintsno;
            pAv->mpObserver->onEvent(pAv->mCurAvEvent);

            break;
        }
        /*case AM_AV_EVT_PLAYER_UPDATE_INFO: {
            AM_AV_TimeshiftInfo_t *info = (AM_AV_TimeshiftInfo_t*)param;
            if (info) {
                pAv->mCurAvEvent.type = AVEvent::EVENT_PLAY_UPDATE;
                pAv->mCurAvEvent.param = info->current_time;
                pAv->mCurAvEvent.status = info->status;
                pAv->mpObserver->onEvent(pAv->mCurAvEvent);
            }
            break;
        }*/
        default:
            break;
    }
}

DVR_Result_t CAv::PlayEventHandler(DVR_PlaybackEvent_t event, void *params, void *userdata)
{

    CAv *pAv = (CAv *)userdata;

    if (NULL == pAv) {
        LOGD ( "%s, ERROR : PlayEventHandler NULL == pTv\n", __FUNCTION__ );
        return DVR_SUCCESS;
    }
    if (pAv->mpObserver == NULL) {
        LOGD ( "%s, ERROR :PlayEventHandler  mpObserver NULL == mpObserver\n", __FUNCTION__ );
        return DVR_SUCCESS;
    }
   if (userdata != NULL)
   {
      switch (event)
      {
         case DVR_PLAYBACK_EVENT_TRANSITION_OK:
         {
            /**< Update the current player information*/
            DVR_WrapperPlaybackStatus_t *status = (DVR_WrapperPlaybackStatus_t *)params;
            {
                LOGD("Info update: current=%lu, full=%lu, state=%d, obsolete=%lu ",
                   status->info_cur.time,
                   status->info_full.time,
                   status->info_obsolete.time,
                   status->state);
            }
            break;
         }
         case DVR_PLAYBACK_EVENT_REACHED_END:
         {
            /**< File player's EOF*/
            LOGD("dvr reach EOF");
            break;
         }
         case DVR_PLAYBACK_EVENT_ERROR:
         {
            /**< Playback fail*/
            LOGD("Playback has been failed");
            break;
         }
         case DVR_PLAYBACK_EVENT_NODATA:
         {
            LOGD ( "%s, send waek signal\n", __FUNCTION__ );
            //pAv->mCurAvEvent.type = AVEvent::EVENT_AV_STOP;
            //pAv->mCurAvEvent.param = ( long )param;
            //pAv->mpObserver->onEvent(pAv->mCurAvEvent);
             break;
         }
         case DVR_PLAYBACK_EVENT_DATARESUME:
         {
             LOGD ( "%s, send data resume signal\n", __FUNCTION__ );
             pAv->mCurAvEvent.type = AVEvent::EVENT_AV_VIDEO_AVAILABLE;
             //pAv->mCurAvEvent.param = ( long )&event->event.video_format;
             pAv->mpObserver->onEvent(pAv->mCurAvEvent);
             break;
         }
        case DVR_PLAYBACK_EVENT_NOTIFY_PLAYTIME: {
            LOGD("Playback notify play time");
            DVR_WrapperPlaybackStatus_t *status = (DVR_WrapperPlaybackStatus_t *)params;
            if (status) {
                // AV_TIMESHIFT_STAT_STOP,
                // AV_TIMESHIFT_STAT_PLAY,
                // AV_TIMESHIFT_STAT_PAUSE,
                // AV_TIMESHIFT_STAT_FFFB,
                // AV_TIMESHIFT_STAT_EXIT,
                // AV_TIMESHIFT_STAT_INITOK,
                // AV_TIMESHIFT_STAT_SEARCHOK,
                switch (status->state) {
                     case DVR_PLAYBACK_STATE_START:
                        {
                            pAv->mCurAvEvent.status = 1;
                            break;
                        }
                    case DVR_PLAYBACK_STATE_STOP:
                        {
                            pAv->mCurAvEvent.status = 0;
                            break;
                        }
                    case DVR_PLAYBACK_STATE_PAUSE:
                        {
                            pAv->mCurAvEvent.status = 2;
                            break;
                        }
                    case DVR_PLAYBACK_STATE_FF:
                        {
                            pAv->mCurAvEvent.status = 3;
                            break;
                        }
                    case DVR_PLAYBACK_STATE_FB:
                        {
                            pAv->mCurAvEvent.status = 3;
                            break;
                        }
                    default:
                        {
                            LOGD("Unhandled event %d", event);
                            break;
                        }
                }
                if (pAv->mCurAvEvent.timeshiftStarttime !=
                    status->info_obsolete.time) {
                    LOGD("START TIME CHANGE----------[0x%x][0x%x]", pAv->mCurAvEvent.timeshiftStarttime, status->info_obsolete.time);
                    pAv->mCurAvEvent.type = AVEvent::EVENT_PLAY_STARTTIME_CHANGE;
                    pAv->mCurAvEvent.param = status->info_obsolete.time;
                    pAv->mCurAvEvent.timeshiftStarttime = status->info_obsolete.time;
                    pAv->mpObserver->onEvent(pAv->mCurAvEvent);
                }
                LOGD("CURRENT TIME CHANGE----------");
                pAv->mCurAvEvent.type = AVEvent::EVENT_PLAY_CURTIME_CHANGE;
                pAv->mCurAvEvent.param = status->info_cur.time + status->info_obsolete.time;
                pAv->mpObserver->onEvent(pAv->mCurAvEvent);
            }
            break;
         }
         default:
         {
            LOGD("Unhandled event %d", event);
            break;
         }
      }
   }
   return DVR_SUCCESS;
}

#endif

void CAv::av_evt_callback(long dev_no, int event_type, void *param, void *user_data )
{
    CAv *pAv = (CAv *) user_data;
    if (NULL == pAv ) {
        LOGD ( "%s, ERROR : av_evt_callback NULL == pTv\n", __FUNCTION__ );
        return ;
    }
    if ( pAv->mpObserver == NULL ) {
        LOGD ( "%s, ERROR : mpObserver NULL == mpObserver\n", __FUNCTION__ );
        return;
    }
#ifdef SUPPORT_ADTV
    switch ( event_type ) {
    case AM_AV_EVT_AV_NO_DATA:
        pAv->mCurAvEvent.type = AVEvent::EVENT_AV_STOP;
        pAv->mCurAvEvent.param = ( long )param;
        pAv->mpObserver->onEvent(pAv->mCurAvEvent);
        break;
    case AM_AV_EVT_AV_DATA_RESUME:
        pAv->mCurAvEvent.type = AVEvent::EVENT_AV_RESUEM;
        pAv->mCurAvEvent.param = ( long )param;
        pAv->mpObserver->onEvent(pAv->mCurAvEvent);
        break;
    case AM_AV_EVT_VIDEO_SCAMBLED:
    case AM_AV_EVT_AUDIO_SCAMBLED:
        pAv->mCurAvEvent.type = AVEvent::EVENT_AV_SCAMBLED;
        pAv->mCurAvEvent.param = ( long )param;
        pAv->mpObserver->onEvent(pAv->mCurAvEvent);
        break;
    case AM_AV_EVT_VIDEO_NOT_SUPPORT: {
        pAv->mCurAvEvent.type = AVEvent::EVENT_AV_UNSUPPORT;
        pAv->mCurAvEvent.param = ( long )param;
        pAv->mpObserver->onEvent(pAv->mCurAvEvent);
        break;
    }
    case AM_AV_EVT_VIDEO_AVAILABLE: {
        pAv->mCurAvEvent.type = AVEvent::EVENT_AV_VIDEO_AVAILABLE;
        pAv->mCurAvEvent.param = ( long )param;
        pAv->mpObserver->onEvent(pAv->mCurAvEvent);
        break;
    }
    case AM_AV_EVT_PLAYER_UPDATE_INFO: {
        AM_AV_TimeshiftInfo_t *info = (AM_AV_TimeshiftInfo_t*)param;
        if (info) {
            pAv->mCurAvEvent.type = AVEvent::EVENT_PLAY_UPDATE;
            pAv->mCurAvEvent.param = info->current_time;
            pAv->mCurAvEvent.status = info->status;
            pAv->mpObserver->onEvent(pAv->mCurAvEvent);
        }
        break;
    }
    default:
        break;
    }
#endif
    LOGD ( "%s, av_evt_callback : dev_no %ld type %d param = %d\n",
        __FUNCTION__, dev_no, pAv->mCurAvEvent.type , (long)param);
}

int CAv::setLookupPtsForDtmb(int enable)
{
    LOGD ( "%s: status %d", __FUNCTION__, enable);

    char value[64] = {0};
    sprintf(value, "%d", enable);
    int linux_version = getKernelMajorVersion();
    if (linux_version > 4)
        tvWriteSysfs(PATH_MEPG_DTMB_LOOKUP_PTS_FLAG_54, value);
    else
        tvWriteSysfs(PATH_MEPG_DTMB_LOOKUP_PTS_FLAG_49, value);
    return 0;
}

