/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#include <string.h>

#define LOG_TAG "tvserver"
#define LOG_TV_TAG "CTvPlayer"

#include "CTvLog.h"
#include <cutils/properties.h>
#include "CTvPlayer.h"
#include "json/json.h"

CTvPlayer::CTvPlayer (CTv *tv) {
    mId = NULL;
    pTv = tv;
}
CTvPlayer::~CTvPlayer() {
    if (mId)
        free((void*)mId);
}
void CTvPlayer::sendTvEvent ( const CTvEv &ev ) {
    if (pTv)
        pTv->sendTvEvent(ev);
}

CDTVTvPlayer::CDTVTvPlayer(CTv *tv) : CTvPlayer(tv) {
    mMode = PLAY_MODE_LIVE;
    mFEParam = NULL;
    mVpid = -1;
    mVfmt = -1;
    mVparam = NULL;
    mApid = -1;
    mAfmt = -1;
    mAparam = NULL;
    mPpid = -1;
    mPparam = NULL;
    mParam = NULL;
    mSourceChanged = true;
    mOffset = -1;
    mDisableTimeShifting = propertyGetBool("vendor.tv.dtv.tvserver.tf.disable", true);
    mIsTsplayer = propertyGetBool("vendor.tv.dtv.tsplayer.enable", false);
}
CDTVTvPlayer::~CDTVTvPlayer() {
    if (mFEParam)
        free((void*)mFEParam);
    if (mVparam)
        free((void*)mVparam);
    if (mAparam)
        free((void*)mAparam);
    if (mPparam)
        free((void*)mPparam);
    if (mParam)
        free((void*)mParam);
    CTvRecord *recorder = RecorderManager::getInstance().getDev("timeshifting");
    if (recorder)
        recorder->stop(NULL);
    tryCloseTFile();
}
DVR_VideoFormat_t CDTVTvPlayer::toDvrVideoFormat(int codec)
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

DVR_AudioFormat_t CDTVTvPlayer::toDvrAudioFormat(int codec)
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
int CDTVTvPlayer::setFEParam(const char *param) {
    LOGD("setFE(%s)", toReadable(param));
    if (mFEParam)
        free((void*)mFEParam);
    mFEParam = param? strdup(param) : NULL;
    mSourceChanged = true;
    return 0;
}
int CDTVTvPlayer::setVideo(int pid, int format, const char *param) {
    LOGD("setVideo(%d:%d:%s)", pid, format, toReadable(param));
    mVpid = pid;
    mVfmt = format;
    if (mVparam)
        free((void*)mVparam);
    mVparam = param? strdup(param) : NULL;
    mSourceChanged = true;
    return 0;
}
int CDTVTvPlayer::setAudio(int pid, int format, const char *param) {
    LOGD("setAudio(%d:%d:%s)", pid, format, toReadable(param));
    mApid = pid;
    mAfmt = format;
    if (mAparam)
        free((void*)mAparam);
    mAparam = param? strdup(param) : NULL;
    return 0;
}
int CDTVTvPlayer::setPcr(int pid, const char *param) {
    LOGD("setPcr(%d:%s)", pid, toReadable(param));
    mPpid = pid;
    if (mPparam)
        free((void*)mPparam);
    mPparam = param? strdup(param) : NULL;
    mSourceChanged = true;
    return 0;
}
int CDTVTvPlayer::setParam(const char *param) {
    LOGD("setParam(%s)", toReadable(param));
    if (mParam)
        free((void*)mParam);
    mParam = param? strdup(param) : NULL;
    mMode = paramGetInt(mParam, NULL, "mode", PLAY_MODE_LIVE);
    mSourceChanged = true;
    //DO NOT use this param, use prop:tv.dtv.tf.disable instead
    //mDisableTimeShifting = paramGetInt(mParam, NULL, "disableTimeShifting", 0) ? true : false;
    return 0;
}


int CDTVTvPlayer::start(const char *param) {
    LOGD("start(%s:%s) current mode(%d)", toReadable(getId()), toReadable(param), mMode);
    int ret = -1;
    if (!(pTv->mpTvin->Tvin_SupportResman())) {
        if (!pTv->mpTvin->getSnowStatus() && !isVideoInuse() ) {
            return ret;
        }
    }
#ifdef SUPPORT_ADTV
    // need show first frame when av not sync, then it looks switching channel more fast
    // tvWriteSysfs("/sys/class/video/show_first_frame_nosync", "1");
    switch (mMode) {
        case PLAY_MODE_LIVE: {//start play live and rec in the backgroud
            if (!mDisableTimeShifting) {
                mDisableTimeShifting = propertyGetBool("vendor.tv.dtv.tvserver.tf.disable", true);
                LOGD("prop tv.dtv.tvserver.tf.disable:%s", mDisableTimeShifting? "true":"false");
            }
            LOGD("PLAY_MODE_LIVE try timeshift--[%d]-", bStartInTimeShift);
            if (bStartInTimeShift)
                ret = startLiveTryTimeShift(param);
            else
                ret = startLiveTryRecording(param);
        }break;

        case PLAY_MODE_TIMESHIFT: {//return to play live
            LOGD("rPLAY_MODE_TIMESHIFT---");
            ret = pTv->playDtvProgramUnlocked(mFEParam, mVpid, mVfmt, mApid, mAfmt, mPpid, paramGetInt(mAparam, NULL, "AudComp", 0));
            if (ret == 0)
                mMode = PLAY_MODE_LIVE;
        }break;

        case PLAY_MODE_REC: {//start play rec
            if (mIsTsplayer == true) {
                LOGD("recplay start libdvr error, not support");
            } else {
                AM_AV_TimeshiftPara_t para;
                memset(&para, 0, sizeof(para));
                para.dmx_id = 0;
                para.mode = AM_AV_TIMESHIFT_MODE_PLAYBACK;
                strncpy(para.file_path, paramGetString(mParam, NULL, "file", "").c_str(), sizeof(para.file_path)-1);
                para.file_path[sizeof(para.file_path)-1] = '\0';
                readMediaInfoFromFile(para.file_path, &para.media_info);
                LOGD("recplay start");
    /*            AM_AV_TimeshiftMediaInfo_t *pminfo = &para.media_info;
                pminfo->vid_pid = mVpid;
                pminfo->vid_fmt = mVfmt;
                pminfo->aud_cnt = 1;
                pminfo->audios[0].pid = mApid;
                pminfo->audios[0].fmt = mAfmt;
                pminfo->sub_cnt = 0;
                pminfo->ttx_cnt = 0;
                pminfo->duration = 0;
                pminfo->program_name[0] = '\0';*/
                ret = pTv->playDtvTimeShiftUnlocked(NULL, (void *)&para, paramGetInt(mAparam, NULL, "AudComp", 0));
            }
        }break;
    }
    mSourceChanged = false;
#endif
    return ret;
}

int CDTVTvPlayer::stop(const char *param) {
    LOGD("CDTVTvPlayer stop(%s:%s)", toReadable(getId()), toReadable(param));
    int ret;
    if (!mDisableTimeShifting) {
        ret = pTv->stopRecording("timeshifting", NULL);
        if (ret != 0) {
            LOGD("stop recording(timeshifting) fail(%#x)", ret);
        }
    }

    ret = pTv->stopPlaying(true, false);
    ret = pTv->mAv.stopTimeShift();


    if (!mIsTsplayer && ret == 0)
        tryCloseTFile();

    if (!pTv->mpTvin->Tvin_SupportResman()) {
        tvWriteSysfs(SYS_VIDEO_INUSE_PATH, 0);
    }

    LOGD("stop(%s)=%d", toReadable(param), ret);
    return ret;
}

int CDTVTvPlayer::pause(const char *param) {
    LOGD("pause(%s:%s)", toReadable(getId()), toReadable(param));
    int ret =  -1;
#ifdef SUPPORT_ADTV
    switch (mMode) {
        case PLAY_MODE_LIVE: {
            if (mDisableTimeShifting)
                break;
            CTvRecord *recorder = RecorderManager::getInstance().getDev("timeshifting");
            if (!recorder) {
                LOGD("recorder(timeshifting not found)");
                TvEvent::AVPlaybackEvent AvPlayBackEvt;
                AvPlayBackEvt.mMsgType = TvEvent::AVPlaybackEvent::EVENT_AV_TIMESHIFT_PLAY_FAIL;
                pTv->sendTvEvent(AvPlayBackEvt);
                return -1;
            }
            mOffset = recorder->getWritePosition();
            if (mIsTsplayer) {
                DVR_PlaybackPids_t play_pids;
                memset(&play_pids, 0, sizeof(play_pids));
                play_pids.video.type = DVR_STREAM_TYPE_VIDEO;
                play_pids.audio.type = DVR_STREAM_TYPE_AUDIO;
                play_pids.ad.type = DVR_STREAM_TYPE_AD;

                if (mVpid > 0 && mVpid < 0x1fff) {
                    play_pids.video.pid = mVpid;
                    play_pids.video.format = toDvrVideoFormat(mVfmt);
                } else {
                    play_pids.video.pid = 0x1fff;
                }
                if (mApid > 0 && mApid < 0x1fff) {
                    play_pids.audio.pid = mApid;
                    play_pids.audio.format = toDvrAudioFormat(mAfmt);
                } else {
                    play_pids.audio.pid = 0x1fff;
                }

                play_pids.ad.pid = 0x1fff;
                play_pids.ad.format = toDvrAudioFormat(mAfmt);

                //play_pids.
                LOGD("play Dtv TimeShift--pcr and ad set-mAfmt[%d]-play_pids.audio.format[%d]vpid[%d]vfmt[%d]", mAfmt, play_pids.audio.format, play_pids.video.pid,play_pids.video.format);
                ret = pTv->mAv.startTimeShift((void *)&play_pids);

                if (ret == 0) {
                    LOGD("player(%s)  in timeshift mode---", getId());
                    mMode = PLAY_MODE_TIMESHIFT;
                } else {
                    LOGD("player(%s) timeshift play fail,  in live mode----", getId());
                    ret = startLive(param);
                    mMode = PLAY_MODE_LIVE_WITHOUT_TIMESHIFT;
                }
                return 0;
            }
            AM_AV_TimeshiftPara_t para;
            para.dmx_id = 0;
            para.mode = AM_AV_TIMESHIFT_MODE_TIMESHIFTING;
            para.file_path[0] = '\0';
            para.tfile = mTfile;
            para.offset_ms = mOffset;
            para.start_paused = true;
            AM_AV_TimeshiftMediaInfo_t *pminfo = &para.media_info;
            pminfo->vid_pid = mVpid;
            pminfo->vid_fmt = mVfmt;
            pminfo->aud_cnt = 1;
            pminfo->audios[0].pid = mApid;
            pminfo->audios[0].fmt = mAfmt;
            pminfo->sub_cnt = 0;
            pminfo->ttx_cnt = 0;
            pminfo->duration = paramGetInt(mParam, NULL, "dur", 120);
            pminfo->program_name[0] = '\0';
            ret = pTv->mAv.startTimeShift(&para);
            if (ret == 0)
                mMode = PLAY_MODE_TIMESHIFT;
        }break;

        case PLAY_MODE_REC:
        case PLAY_MODE_TIMESHIFT: {
        ret = pTv->mAv.pauseTimeShift();
        }break;
    }
#endif
    return ret;
}

int CDTVTvPlayer::resume(const char *param) {
    LOGD("resume(%s:%s)", toReadable(getId()), toReadable(param));

    int ret = -1;
    switch (mMode) {
        case PLAY_MODE_LIVE: {
            }break;
        case PLAY_MODE_REC:
        case PLAY_MODE_TIMESHIFT:{
            pTv->mAv.resumeTimeShift();
        }break;
    }
    return ret;
}

int CDTVTvPlayer::seek(const char *param)
{
    LOGD("seek(%s:%s)", toReadable(getId()), toReadable(param));
    int ret = -1;
    switch (mMode) {
        case PLAY_MODE_LIVE: {
            }break;
        case PLAY_MODE_REC:
        case PLAY_MODE_TIMESHIFT:{
            int pos = paramGetInt(param, "", "offset", 0);
            pTv->mAv.seekTimeShift(pos, true);
        }break;
    }
    return ret;
}
int CDTVTvPlayer::set(const char *param)
{
    LOGD("set(%s:%s)", toReadable(getId()), toReadable(param));
    int ret = -1;
    switch (mMode) {
        case PLAY_MODE_LIVE: {
            }break;
        case PLAY_MODE_REC:
        case PLAY_MODE_TIMESHIFT:{
            int speed = paramGetInt(param, NULL, "speed", 0);
            if (speed == 1 || speed == -1)
                speed = 0;
            pTv->mAv.setTimeShiftSpeed(speed);
        }break;
    }
    return ret;
}

int CDTVTvPlayer::setupDefault(const char *param)
{
    std::string type = paramGetString(param, NULL, "type", "dtv");
    if (type.compare("dtv") == 0) {
        LOGD("set def param=%s", param);
        setFEParam(paramGetString(param, NULL, "fe", "").c_str());
        setParam(paramGetString(param, NULL, "para", "").c_str());
        setVideo(paramGetInt(param, "v", "pid", -1), paramGetInt(param, "v", "fmt", -1), paramGetString(param, "v", "para", "").c_str());
        setAudio(paramGetInt(param, "a", "pid", -1), paramGetInt(param, "a", "fmt", -1), paramGetString(param, "a", "para", "").c_str());
        setPcr(paramGetInt(param, "p", "pid", -1), paramGetString(param, "p", "para", "").c_str());
    }
    return 0;
}

int CDTVTvPlayer::tryCloseTFile()
{
#ifdef SUPPORT_ADTV
    if (mTfile) {
        int ret;
        AM_EVT_Unsubscribe((long)mTfile, AM_TFILE_EVT_START_TIME_CHANGED, tfile_evt_callback, (void*)getId());
        AM_EVT_Unsubscribe((long)mTfile, AM_TFILE_EVT_END_TIME_CHANGED, tfile_evt_callback, (void*)getId());
        ret = AM_TFile_Close(mTfile);
        LOGD("close tfile=%d", ret);
    }
#endif
    return 0;
}

int CDTVTvPlayer::startLive(const char *param __unused)
{
    int ret = -1;

    ret = pTv->playDtvProgramUnlocked(mFEParam, mVpid, mVfmt, mApid, mAfmt, mPpid, paramGetInt(mAparam, NULL, "AudComp", 0));
    mMode = PLAY_MODE_LIVE;
    return ret;
}
int CDTVTvPlayer::startRecording(const char *name, const char *param)
{

    Json::Value root;
    root["timeshift"] = 1;
    root["dvr"] = 1;
    root["path"] = paramGetString(param, NULL, "path", "/storage").c_str();
    root["prefix"] = "TimeShifting";

    Json::Value v;
    v["pid"] = mVpid;
    v["fmt"] = mVfmt;
    root["v"] = v;

    Json::Value a;
    a["pid"] = mApid;
    a["fmt"] = mAfmt;
    root["a"] = a;

    {
        Json::Reader reader;
        Json::Value r;
        if (reader.parse(param, param + strlen(param), r)) {
            if (r.isMember("max"))
                root["max"] = r["max"];
            if (r.isMember("as"))
                root["as"] = r["as"];
        }
    }
    LOGD("startRecording--[%s]--[%s]", jsonToString(root).c_str(), param);
    return pTv->startRecording(name, jsonToString(root).c_str(), this);
}
int CDTVTvPlayer::startLiveTryRecording(const char *param)
{

    int ret = -1;
#ifdef SUPPORT_ADTV
    if (!mDisableTimeShifting && mSourceChanged) {
        pTv->stopRecording("timeshifting", NULL);
        tryCloseTFile();
    }

    //AM_EVT_Subscribe(0, AM_AV_EVT_PLAYER_UPDATE_INFO, player_info_callback, (void*)getId());

    LOGD("player(%s) startLive,  in live mode", getId());
    ret = startLive(param);

    if (!mDisableTimeShifting && mSourceChanged) {
        // char buf[256];
        // sprintf(buf,
        //     "{\"timeshift\":1,\"dvr\":1,\"path\":\"%s\",\"prefix\":\"TimeShifting\",\"v\":{\"pid\":%d,\"fmt\":%d},\"a\":{\"pid\":%d,\"fmt\":%d},\"max\":%s}",
        //     paramGetString(mParam, NULL, "path", "/storage").c_str(),
        //     mVpid, mVfmt, mApid, mAfmt,
        //     paramGetString(mParam, NULL, "max", "{\"time\":15}").c_str());
        ret = startRecording("timeshifting", mParam);
        //ret = pTv->startRecording("timeshifting", buf, this);
        if (ret != 0) {
            TvEvent::AVPlaybackEvent AvPlayBackEvt;
            AvPlayBackEvt.mMsgType = TvEvent::AVPlaybackEvent::EVENT_AV_TIMESHIFT_REC_FAIL;
            pTv->sendTvEvent(AvPlayBackEvt);
        }
    }

    CTvRecord *recorder = RecorderManager::getInstance().getDev("timeshifting");
    if (recorder) {
        mTfile = recorder->detachFileHandler();
        if (mTfile) {
            AM_EVT_Subscribe((long)mTfile, AM_TFILE_EVT_START_TIME_CHANGED, tfile_evt_callback, (void*)getId());
            AM_EVT_Subscribe((long)mTfile, AM_TFILE_EVT_END_TIME_CHANGED, tfile_evt_callback, (void*)getId());
            AM_TFile_TimeStart(mTfile);
        }
    } else {
        LOGD("player(%s) rec(tf) fail,  in live(no rec) mode", getId());
        mMode = PLAY_MODE_LIVE_WITHOUT_TIMESHIFT;
    }
#endif
    return ret;
}

int CDTVTvPlayer::startLiveTryTimeShift(const char *param)
{
    int ret = -1;
#ifdef SUPPORT_ADTV
    if (!mDisableTimeShifting && mSourceChanged) {
        pTv->stopRecording("timeshifting", NULL);
        tryCloseTFile();

        // char buf[256];
        // char pid_string[256] = {0};
        // char target_pid[16] = {0};
        // int sub_cnt;
        // int i;
        // sub_cnt = strtoul(paramGetString(mParam, NULL, "subcnt", "0").c_str(), NULL, 10);
        // for (i=0; i<sub_cnt; i++)
        // {
        //     sprintf(target_pid, "pid%d", i);
        //     sprintf(pid_string, "%s,\"%s\":%s", pid_string, target_pid, paramGetString(mParam, "subpid", target_pid,"0").c_str());
        // }

        // sprintf(buf,
        //     "{\"timeshift\":1,\"dvr\":1,\"path\":\"%s\",\"prefix\":\"TimeShifting\",\"v\":{\"pid\":%d,\"fmt\":%d},\"a\":{\"pid\":%d,\"fmt\":%d},\"max\":%s,\"subcnt\":%s%s}",
        //     paramGetString(mParam, NULL, "path", "/storage").c_str(),
        //     mVpid, mVfmt, mApid, mAfmt,
        //     paramGetString(mParam, NULL, "max", "{\"time\":15}").c_str(),
        //     paramGetString(mParam, NULL, "subcnt", "0").c_str(),
        //     pid_string);
        // ret = pTv->startRecording("timeshifting", buf, this);
        ret = startRecording("timeshifting", mParam);
        if (ret != 0) {
            LOGD("start timeshifting rec fail");
            TvEvent::AVPlaybackEvent AvPlayBackEvt;
            AvPlayBackEvt.mMsgType = TvEvent::AVPlaybackEvent::EVENT_AV_TIMESHIFT_REC_FAIL;
            pTv->sendTvEvent(AvPlayBackEvt);
        }
    }

    CTvRecord *recorder = RecorderManager::getInstance().getDev("timeshifting");
    if (!recorder) {
        LOGD("recorder(timeshifting not found)");
        TvEvent::AVPlaybackEvent AvPlayBackEvt;
        AvPlayBackEvt.mMsgType = TvEvent::AVPlaybackEvent::EVENT_AV_TIMESHIFT_PLAY_FAIL;
        pTv->sendTvEvent(AvPlayBackEvt);

        LOGD("player(%s) in live mode", getId());
        ret = startLive(param);

        mMode = PLAY_MODE_LIVE_WITHOUT_TIMESHIFT;

    } else {

        if (!mDisableTimeShifting) {
            if (mIsTsplayer == true) {
                DVR_PlaybackPids_t play_pids;
                memset(&play_pids, 0, sizeof(play_pids));
                play_pids.video.type = DVR_STREAM_TYPE_VIDEO;
                play_pids.audio.type = DVR_STREAM_TYPE_AUDIO;
                play_pids.audio.type = DVR_STREAM_TYPE_AD;


                if (mVpid > 0 && mVpid < 0x1fff) {
                    play_pids.video.pid = mVpid;
                    play_pids.video.format = toDvrVideoFormat(mVfmt);
                } else {
                    play_pids.video.pid = 0x1fff;
                }
                if (mApid > 0 && mApid < 0x1fff) {
                    play_pids.audio.pid = mApid;
                    play_pids.audio.format = toDvrAudioFormat(mAfmt);
                } else {
                    play_pids.audio.pid = 0x1fff;
                }

                play_pids.ad.pid = 0x1fff;
                play_pids.ad.format = toDvrAudioFormat(mAfmt);

                //play_pids.
                LOGD("play Dtv TimeShift--");
                ret = pTv->playDtvTimeShiftUnlocked(mFEParam, (void *)&play_pids, paramGetInt(mAparam, NULL, "AudComp", 0));

                if (ret == 0) {
                    //LOGD("subscribe update, %s", getId());
                    LOGD("player(%s)  in timeshift mode", getId());
                    mMode = PLAY_MODE_TIMESHIFT;
                } else {
                    LOGD("player(%s) timeshift play fail,  in live mode", getId());
                    ret = startLive(param);
                    mMode = PLAY_MODE_LIVE_WITHOUT_TIMESHIFT;
                }
                return 0;
            }


            mOffset = recorder->getWritePosition();
            mTfile = recorder->detachFileHandler();

            if (mTfile) {
                AM_EVT_Subscribe((long)mTfile, AM_TFILE_EVT_START_TIME_CHANGED, tfile_evt_callback, (void*)getId());
                AM_EVT_Subscribe((long)mTfile, AM_TFILE_EVT_END_TIME_CHANGED, tfile_evt_callback, (void*)getId());
                AM_TFile_TimeStart(mTfile);
            }

            AM_AV_TimeshiftPara_t para;
            para.dmx_id = 0;
            para.mode = AM_AV_TIMESHIFT_MODE_TIMESHIFTING;
            para.file_path[0] = '\0';
            para.tfile = mTfile;
            para.offset_ms = mOffset;
            para.start_paused = false;
            AM_AV_TimeshiftMediaInfo_t *pminfo = &para.media_info;
            pminfo->vid_pid = mVpid;
            pminfo->vid_fmt = mVfmt;
            pminfo->aud_cnt = 1;
            pminfo->audios[0].pid = mApid;
            pminfo->audios[0].fmt = mAfmt;
            pminfo->sub_cnt = 0;
            pminfo->ttx_cnt = 0;
            pminfo->duration = paramGetInt(mParam, "max", "time", 0);
            pminfo->program_name[0] = '\0';
            LOGD("play Dtv TimeShift--a");
            ret = pTv->playDtvTimeShiftUnlocked(mFEParam, (void *)&para, paramGetInt(mAparam, NULL, "AudComp", 0));

            if (ret == 0) {
                //LOGD("subscribe update, %s", getId());
                //AM_EVT_Subscribe(0, AM_AV_EVT_PLAYER_UPDATE_INFO, player_info_callback, (void*)getId());

                LOGD("player(%s)  in timeshift mode", getId());
                mMode = PLAY_MODE_TIMESHIFT;
            } else {
                LOGD("player(%s) timeshift play fail,  in live mode", getId());
                ret = startLive(param);
                mMode = PLAY_MODE_LIVE_WITHOUT_TIMESHIFT;
            }
        } else {
            LOGD("player(%s) force in live mode", getId());
            ret = startLive(param);
            mMode = PLAY_MODE_LIVE_WITHOUT_TIMESHIFT;
        }
    }
#endif
    return ret;
}

void CDTVTvPlayer::tfile_evt_callback ( long dev_no __unused, int event_type, void *param, void *user_data )
{
#ifdef SUPPORT_ADTV
    switch (event_type) {
        case AM_TFILE_EVT_START_TIME_CHANGED: {
            LOGD("player(%s) : start time(%d)", (char*)user_data, (int)(long)param );
            TvEvent::AVPlaybackEvent AvPlayBackEvt;
            AvPlayBackEvt.mMsgType = TvEvent::AVPlaybackEvent::EVENT_AV_TIMESHIFT_START_TIME_CHANGED;
            AvPlayBackEvt.mProgramId = (int)(long)param;
            CTvPlayer *player = PlayerManager::getInstance().getDev((char *)user_data);
            if (player)
                player->sendTvEvent(AvPlayBackEvt);
            }
            break;
        case AM_TFILE_EVT_END_TIME_CHANGED:
            LOGD("player(%s) : end time(%d)", (char*)user_data, (int)(long)param );
            /*TvEvent::AVPlaybackEvent AvPlayBackEvt;
            AvPlayBackEvt.mMsgType = TvEvent::AVPlaybackEvent::EVENT_AV_TIMESHIFT_END_TIME_CHANGED;
            AvPlayBackEvt.mProgramId = (int)(long)param;
            CTvPlayer *player = PlayerManager::getInstance().getDev((char *)user_data);
            if (player)
                player->sendTvEvent(AvPlayBackEvt);*/
            break;
        default:
            break;
    }
#endif
}

void CDTVTvPlayer::onPlayUpdate(const CAv::AVEvent &ev)
{
    if (ev.type == CAv::AVEvent::EVENT_PLAY_UPDATE)
    {
        /*typedef enum
        {
            AV_TIMESHIFT_STAT_STOP,
            AV_TIMESHIFT_STAT_PLAY,
            AV_TIMESHIFT_STAT_PAUSE,
            AV_TIMESHIFT_STAT_FFFB,
            AV_TIMESHIFT_STAT_EXIT,
            AV_TIMESHIFT_STAT_INITOK,
            AV_TIMESHIFT_STAT_SEARCHOK,
        } AV_TimeshiftState_t;
        */
        LOGD("player(%s) update status:%d, para:%d", getId(), ev.status, ev.param);
        switch (ev.status) {
            case 1:
            case 2:
            case 3:
            {
                if (mMode == PLAY_MODE_REC && ev.param == 0) {// play back need a time start
                    TvEvent::AVPlaybackEvent AvPlayBackEvt;
                    AvPlayBackEvt.mMsgType = TvEvent::AVPlaybackEvent::EVENT_AV_TIMESHIFT_START_TIME_CHANGED;
                    AvPlayBackEvt.mProgramId = ev.param;
                    sendTvEvent(AvPlayBackEvt);
                    LOGD("player(%s) : start time(%d)", getId(), AvPlayBackEvt.mProgramId);
                }
                TvEvent::AVPlaybackEvent AvPlayBackEvt;
                AvPlayBackEvt.mMsgType = TvEvent::AVPlaybackEvent::EVENT_AV_TIMESHIFT_CURRENT_TIME_CHANGED;
                AvPlayBackEvt.mProgramId = ev.param;
                sendTvEvent(AvPlayBackEvt);
                LOGD("player(%s) : current time(%d)mMode(%d)", getId(), AvPlayBackEvt.mProgramId, mMode);
            }break;
        }
    } else if (ev.type == CAv::AVEvent::EVENT_PLAY_CURTIME_CHANGE) {
        LOGD("player(%s) update status:%d, para:%d", getId(), ev.status, ev.param);
        switch (ev.status) {
            case 1:
            case 2:
            case 3:
            {
                if (mMode == PLAY_MODE_REC && ev.param == 0) {// play back need a time start
                    TvEvent::AVPlaybackEvent AvPlayBackEvt;
                    AvPlayBackEvt.mMsgType = TvEvent::AVPlaybackEvent::EVENT_AV_TIMESHIFT_START_TIME_CHANGED;
                    AvPlayBackEvt.mProgramId = ev.param;
                    sendTvEvent(AvPlayBackEvt);
                    LOGD("player(%s) : start time(%d)", getId(), AvPlayBackEvt.mProgramId);
                }
                TvEvent::AVPlaybackEvent AvPlayBackEvt;
                AvPlayBackEvt.mMsgType = TvEvent::AVPlaybackEvent::EVENT_AV_TIMESHIFT_CURRENT_TIME_CHANGED;
                AvPlayBackEvt.mProgramId = ev.param;
                sendTvEvent(AvPlayBackEvt);
                LOGD("player(%s) : current time(%d)mMode(%d)", getId(), AvPlayBackEvt.mProgramId, mMode);
            }break;
        }
    } else if (ev.type == CAv::AVEvent::EVENT_PLAY_STARTTIME_CHANGE) {
        LOGD("player(%s) update status:%d, para:%d", getId(), ev.status, ev.param);
        switch (ev.status) {
            case 1:
            case 2:
            case 3:
            {
                    TvEvent::AVPlaybackEvent AvPlayBackEvt;
                    AvPlayBackEvt.mMsgType = TvEvent::AVPlaybackEvent::EVENT_AV_TIMESHIFT_START_TIME_CHANGED;
                    AvPlayBackEvt.mProgramId = ev.param;
                    sendTvEvent(AvPlayBackEvt);
                    LOGD("player(%s) : start time(%d)", getId(), AvPlayBackEvt.mProgramId);
            }break;
        }
    }
}
//no used
void CDTVTvPlayer::player_info_callback(long dev_no __unused, int event_type, void *param, void *data)
{
#ifdef SUPPORT_ADTV
    if (event_type == AM_AV_EVT_PLAYER_UPDATE_INFO)
    {
        /*typedef enum
        {
        AV_TIMESHIFT_STAT_STOP,
        AV_TIMESHIFT_STAT_PLAY,
        AV_TIMESHIFT_STAT_PAUSE,
        AV_TIMESHIFT_STAT_FFFB,
        AV_TIMESHIFT_STAT_EXIT,
        AV_TIMESHIFT_STAT_INITOK,
        AV_TIMESHIFT_STAT_SEARCHOK,
        } AV_TimeshiftState_t;
        */
        AM_AV_TimeshiftInfo_t *info = (AM_AV_TimeshiftInfo_t*)param;
        if (info == NULL)
            return;
        switch (info->status) {
            case 5:
                //EVENT_PLAYBACK_START;
                break;
            case 4:
                //EVENT_PLAYBACK_END;
                break;
            case 1:
            case 2:
            case 3: {
            LOGD("player(%s) : current time(%d)", (char*)data, info->current_time);
            TvEvent::AVPlaybackEvent AvPlayBackEvt;
            AvPlayBackEvt.mMsgType = TvEvent::AVPlaybackEvent::EVENT_AV_TIMESHIFT_CURRENT_TIME_CHANGED;
            AvPlayBackEvt.mProgramId = info->current_time;
            CTvPlayer *player = PlayerManager::getInstance().getDev((char *)data);
            if (player)
                player->sendTvEvent(AvPlayBackEvt);
            }
        }
    }
#endif
}

void CDTVTvPlayer::onEvent(const CTvRecord::RecEvent &ev) {
    if (ev.id.compare("timeshifting") != 0)
        return;

    LOGD ( "on RecEvent = %d", ev.type);
    switch ( ev.type ) {
        case CTvRecord::RecEvent::EVENT_REC_START: {
            }break;
        case CTvRecord::RecEvent::EVENT_REC_STOP: {
            if (ev.error != 0) {
                    LOGD("player(%s) : rec stop, current err(%d)", getId(), ev.error);
                    TvEvent::AVPlaybackEvent AvPlayBackEvt;
                    AvPlayBackEvt.mMsgType = TvEvent::AVPlaybackEvent::EVENT_AV_TIMESHIFT_REC_FAIL;
                    AvPlayBackEvt.mProgramId = ev.error;
                    sendTvEvent(AvPlayBackEvt);
            }
            }break;
        default:
            break;
    }
}

#ifdef SUPPORT_ADTV
int CDTVTvPlayer::readMediaInfoFromFile(const char *file_path, AM_AV_TimeshiftMediaInfo_t *info)
{
    uint8_t buffer[2][sizeof(AM_REC_MediaInfo_t) + 4*(sizeof(AM_REC_MediaInfo_t)/188 + 1)];
    uint8_t *buf = buffer[0];
    uint8_t *pkt_buf = buffer[1];
    int pos = 0, info_len, i, fd, name_len, data_len;

#define READ_INT(_i)\
    AM_MACRO_BEGIN\
        if ((info_len-pos) >= 4) {\
            (_i) = ((int)buf[pos]<<24) | ((int)buf[pos+1]<<16) | ((int)buf[pos+2]<<8) | (int)buf[pos+3];\
            pos += 4;\
        } else {\
            goto read_error;\
        }\
    AM_MACRO_END

    fd = open(file_path, O_RDONLY);
    if (fd < 0) {
        LOGE("Cannot open file '%s'", file_path);
        return -1;
    }

    info_len = read(fd, pkt_buf, sizeof(buffer[1]));

    data_len = 0;
    /*skip the packet headers*/
    for (i=0; i<info_len; i++) {
        if ((i%188) > 3) {
            buf[data_len++] = pkt_buf[i];
        }
    }

    info_len = data_len;

    READ_INT(info->duration);

    name_len = sizeof(info->program_name);
    if ((info_len-pos) >= name_len) {
        memcpy(info->program_name, buf+pos, name_len);
        info->program_name[name_len - 1] = 0;
        pos += name_len;
    } else {
        goto read_error;
    }
    READ_INT(info->vid_pid);
    READ_INT(info->vid_fmt);

    READ_INT(info->aud_cnt);
    LOGD("audio count %d", info->aud_cnt);
    for (i=0; i<info->aud_cnt; i++) {
        READ_INT(info->audios[i].pid);
        READ_INT(info->audios[i].fmt);
        memcpy(info->audios[i].lang, buf+pos, 4);
        pos += 4;
    }
    READ_INT(info->sub_cnt);
    LOGD("subtitle count %d", info->sub_cnt);
    for (i=0; i<info->sub_cnt; i++) {
        READ_INT(info->subtitles[i].pid);
        READ_INT(info->subtitles[i].type);
        READ_INT(info->subtitles[i].composition_page);
        READ_INT(info->subtitles[i].ancillary_page);
        READ_INT(info->subtitles[i].magzine_no);
        READ_INT(info->subtitles[i].page_no);
        memcpy(info->subtitles[i].lang, buf+pos, 4);
        pos += 4;
    }
    READ_INT(info->ttx_cnt);
    LOGD("teletext count %d", info->ttx_cnt);
    for (i=0; i<info->ttx_cnt; i++) {
        READ_INT(info->teletexts[i].pid);
        READ_INT(info->teletexts[i].magzine_no);
        READ_INT(info->teletexts[i].page_no);
        memcpy(info->teletexts[i].lang, buf+pos, 4);
        pos += 4;
    }
    close(fd);
    return 0;

read_error:
    LOGE("Read media info from file error, len %d, pos %d", info_len, pos);
    close(fd);

    return -1;
}
#endif

CATVTvPlayer::CATVTvPlayer(CTv *tv) : CTvPlayer(tv) {
    mFEParam = NULL;
    mVparam = NULL;
    mAparam = NULL;
    mParam = NULL;
}

CATVTvPlayer::~CATVTvPlayer() {
    if (mFEParam)
        free((void*)mFEParam);
    if (mVparam)
        free((void*)mVparam);
    if (mAparam)
        free((void*)mAparam);
    if (mParam)
        free((void*)mParam);
}

int CATVTvPlayer::setFEParam(const char *param) {
    LOGD("setFE(%s)", toReadable(param));
    if (mFEParam)
        free((void*)mFEParam);
    mFEParam = param? strdup(param) : NULL;
    return 0;
}

int CATVTvPlayer::setVideo(const char *param) {
    LOGD("setVideo(%s)", toReadable(param));
    if (mVparam)
        free((void*)mVparam);
    mVparam = param? strdup(param) : NULL;
    return 0;
}
int CATVTvPlayer::setAudio(const char *param) {
    LOGD("setAudio(%s)", toReadable(param));
    if (mAparam)
        free((void*)mAparam);
    mAparam = param? strdup(param) : NULL;
    return 0;
}

int CATVTvPlayer::setParam(const char *param) {
    LOGD("setParam(%s)", toReadable(param));
    if (mParam)
        free((void*)mParam);
    mParam = param? strdup(param) : NULL;
    return 0;
}

int CATVTvPlayer::start(const char *param) {
    LOGD("start(%s:%s) current mode(%d)", toReadable(getId()), toReadable(param));
    int ret = -1;
    ret = pTv->playAtvProgram(paramGetInt(mFEParam, NULL, "freq", 44250000),
                            paramGetInt(mFEParam, NULL, "vtd", 1),
                            paramGetInt(mFEParam, NULL, "atd", 0),
                            paramGetInt(mFEParam, NULL, "vfmt", 0),
                            paramGetInt(mFEParam, NULL, "soundsys", -1),
                            0,
                            paramGetInt(mAparam, NULL, "AudComp", 0));
    return ret;
}

int CATVTvPlayer::stop(const char *param) {
    LOGD("CATVTvPlayer stop(%s:%s)", toReadable(getId()), toReadable(param));
    int ret = -1;

    ret = pTv->stopPlaying(true, false);

    LOGD("stop(%s)=%d", toReadable(param), ret);
    return ret;
}

int CATVTvPlayer::set(const char *param)
{
    LOGD("set(%s:%s)", toReadable(getId()), toReadable(param));
    int ret = 0;
    return ret;
}

int CATVTvPlayer::setupDefault(const char *param)
{
    std::string type = paramGetString(param, NULL, "type", "dtv");
    if (type.compare("atv") == 0) {
        setFEParam(paramGetString(param, NULL, "fe", "").c_str());
        setAudio(paramGetString(param, "a", "para", "").c_str());
        setParam(paramGetString(param, NULL, "para", "").c_str());
    }
    return 0;
}

