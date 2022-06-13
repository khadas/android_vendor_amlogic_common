/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "tvserver"
#define LOG_TV_TAG "CTvRecord"

#include <tvutils.h>
#include "CTvRecord.h"
#include "CTvLog.h"
#include <cutils/properties.h>
#include "json/json.h"
#define FEND_DEV_NO 0
#define DVR_DEV_NO 0
#define DVR_BUF_SIZE 1024*1024
#define DVR_DEV_COUNT      (2)
#define DEFAULT_TIMESHIFT_BASENAME   "timeshif"

CTvRecord::CTvRecord()
{
#ifdef SUPPORT_ADTV
   bool is_timeshift;
   int dvr_mode;
   int cnt;

    mIsTsplayer = propertyGetBool("vendor.tv.dtv.tsplayer.enable", false);
    mIsTsplayer = true;
    memset(&rec_open_params, 0, sizeof(rec_open_params));
    memset(&rec_start_params, 0, sizeof(rec_start_params));
    rec_state = REC_STOPPED;

    memset(&mCreateParam, 0, sizeof(mCreateParam));
    memset(&mRecParam, 0, sizeof(mRecParam));
    memset(&mRecInfo, 0, sizeof(mRecInfo));
    mCreateParam.fend_dev = FEND_DEV_NO;
    mCreateParam.dvr_dev = DVR_DEV_NO;
    mCreateParam.async_fifo_id = 0;
    mRec = NULL;
    mId = NULL;
    mpObserver = NULL;
#endif
}

CTvRecord::~CTvRecord()
{
    if (mId)
        free((void*)mId);
    LOGD("~ ctvrecord");
#ifdef SUPPORT_ADTV
    if (mRec) {
        AM_REC_Destroy(mRec);
        mRec = NULL;
    }
#endif
}


DVR_VideoFormat_t CTvRecord::toDvrVideoFormat(int codec)
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

DVR_AudioFormat_t CTvRecord::toDvrAudioFormat(int codec)
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
int CTvRecord::setFilePath(const char *name)
{
    LOGD("setFilePath(%s)", toReadable(name));
#ifdef SUPPORT_ADTV
    if (mIsTsplayer == true) {
        strncpy(rec_open_params.location, (char*)name, sizeof(rec_open_params.location));
        //strncpy((char*)rec_open_params.location, (char*)name, DVR_MAX_LOCATION_SIZE);
        //rec_open_params.location[DVR_MAX_LOCATION_SIZE-1] = '\0';
        LOGD("----rec_open_params.location(%s)", toReadable(rec_open_params.location));
        return 0;
    }
    strncpy(mCreateParam.store_dir, name, AM_REC_PATH_MAX);
    mCreateParam.store_dir[AM_REC_PATH_MAX-1] = 0;
#endif
    return 0;
}

int CTvRecord::setFileName(const char *prefix, const char *suffix)
{
    LOGD("setFileName(%s,%s)", toReadable(prefix), toReadable(suffix));
#ifdef SUPPORT_ADTV
    if (mIsTsplayer == true) {
        if (strcmp("TimeShifting",prefix) == 0)
            mIsTimeshift = true;
        else
            mIsTimeshift = false;
        LOGD("prefix.(%s)mIsTimeshift[%d]", toReadable(prefix), (int)mIsTimeshift);
        rec_open_params.is_timeshift = (mIsTimeshift) ? DVR_TRUE : DVR_FALSE;
        return 0;
    }
    strncpy(mRecParam.prefix_name, prefix, AM_REC_NAME_MAX);
    mRecParam.prefix_name[AM_REC_NAME_MAX-1] = 0;
    strncpy(mRecParam.suffix_name, suffix, AM_REC_SUFFIX_MAX);
    mRecParam.suffix_name[AM_REC_SUFFIX_MAX-1] = 0;
#endif
    return 0;
}

#ifdef SUPPORT_ADTV
int CTvRecord::setMediaInfo(AM_REC_MediaInfo_t *info)
{
    LOGD("setMediaInfo()" );

    memcpy(&mRecParam.media_info, info, sizeof(AM_REC_MediaInfo_t));
    return 0;
}

int CTvRecord::getInfo(AM_REC_RecInfo_t *info)
{
    if (!mRec || !info)
        return -1;
    return AM_REC_GetRecordInfo(mRec, info);
}

AM_TFile_t CTvRecord::getFileHandler()
{
    if (!mRec)
        return NULL;

    AM_TFile_t file = NULL;
    AM_ErrorCode_t err = DVB_SUCCESS;
    err = AM_REC_GetTFile(mRec, &file, NULL);
    if (err != DVB_SUCCESS)
        return NULL;
    return file;
}

AM_TFile_t CTvRecord::detachFileHandler()
{
    if (!mRec)
        return NULL;

    AM_TFile_t file = NULL;
    int flag;
    AM_ErrorCode_t err = DVB_SUCCESS;
    err = AM_REC_GetTFile(mRec, &file, &flag);
    if (err != DVB_SUCCESS) {
        LOGD("get tfile fail(%d)", err);
        return NULL;
    }
    AM_REC_SetTFile(mRec, file, flag |REC_TFILE_FLAG_DETACH);
    return file;
}
#endif

int CTvRecord::setMediaInfoExt(int type, int val)
{
    LOGD("setMediaInfoExt(%d,%d)", type, val );
#ifdef SUPPORT_ADTV
    switch (type) {
        case REC_EXT_TYPE_PMTPID:
            mRecParam.program.i_pid = val;
            break;
        case REC_EXT_TYPE_PN:
            mRecParam.program.i_number = val;
            break;
        case REC_EXT_TYPE_ADD_PID:
        case REC_EXT_TYPE_REMOVE_PID:
            mExtPids.add(val);
            break;
        default:
            return -1;
            break;
    }
#endif
    return 0;
}
int CTvRecord::recordingSegmentSizeKB()
{
   return property_get_int32("vendor.tv.dtv.pvr.segment_size_kb", 100 * 1024/*100MB*/);;
}

int CTvRecord::setTimeShiftMode(bool enable, int duration, int size)
{
    LOGD("setTimeShiftMode(%d, duration:%d s- size:%d)", enable, duration, size );
#ifdef SUPPORT_ADTV
    if (mIsTsplayer == true) {
      rec_open_params.segment_size = recordingSegmentSizeKB() * 1024;
      rec_open_params.max_size = 0;
      rec_open_params.max_time = 0;
      rec_open_params.is_timeshift = enable? DVR_TRUE : DVR_FALSE;
      if (enable) {//is timeshift
        rec_open_params.max_time = enable ? duration * 1000 : 0;
        rec_open_params.max_size = enable ? size : 0;
        rec_open_params.flags = (DVR_RecordFlag_t)(DVR_RECORD_FLAG_ACCURATE);
      }
      LOGD("setTimeShiftMode(rec_open_params.location:%s) rec_open_params.flush_size(%d)time(%ld)ms max size(%lld)byte seg size(%lld)byte",
      rec_open_params.location,rec_open_params.flush_size, rec_open_params.max_time, rec_open_params.max_size, rec_open_params.segment_size);
      return 0;
    }
    mRecParam.is_timeshift = enable? true : false;
    mRecParam.total_time = enable ? duration : 0;
    mRecParam.total_size = enable ? size : 0;
#endif
    return 0;
}

int CTvRecord::setDev(int type, int id)
{
    LOGD("setDev(%d,%d)", type, id );
#ifdef SUPPORT_ADTV
    switch (type) {
        case REC_DEV_TYPE_FE:
            mCreateParam.fend_dev = id;
            break;
        case REC_DEV_TYPE_DVR:
            if (mIsTsplayer == true) {
                rec_open_params.dmx_dev_id = id;
            } else {
                mCreateParam.dvr_dev = id;
            }
            break;
        case REC_DEV_TYPE_FIFO:
            mCreateParam.async_fifo_id = id;
            break;
        default:
            return -1;
            break;
    }
#endif
    return 0;
}

void CTvRecord::rec_evt_cb(long dev_no, int event_type, void *param, void *data)
{
    CTvRecord *rec;
#ifdef SUPPORT_ADTV
    AM_REC_GetUserData((AM_REC_Handle_t)dev_no, (void**)&rec);
    if (!rec)
        return;

    switch (event_type) {
        case AM_REC_EVT_RECORD_END :{
            AM_REC_RecEndPara_t *endpara = (AM_REC_RecEndPara_t*)param;
            rec->mEvent.type = RecEvent::EVENT_REC_STOP;
            rec->mEvent.id = std::string((const char*)data);
            rec->mEvent.error = endpara->error_code;
            rec->mEvent.size = endpara->total_size;
            rec->mEvent.time = endpara->total_time;
            rec->mpObserver->onEvent(rec->mEvent);
            }break;
        case AM_REC_EVT_RECORD_START: {
            rec->mEvent.type = RecEvent::EVENT_REC_START;
            rec->mEvent.id = std::string((const char*)data);
            rec->mpObserver->onEvent(rec->mEvent);
            }break;
        default:
            break;
    }
#endif
    LOGD ( "rec_evt_callback : dev_no %ld type %d param = %ld\n",
        dev_no, event_type, (long)param);
}

DVR_Result_t CTvRecord::RecEventHandler(DVR_RecordEvent_t event, void *params, void *userdata)
{
    CTvRecord *rec;
    LOGD ( "RecEventHandler : event %d\n", event);
   if (userdata != NULL)
   {
      rec = (CTvRecord *)userdata;
      DVR_WrapperRecordStatus_t *status = (DVR_WrapperRecordStatus_t *)params;

      switch (event)
      {
         case DVR_RECORD_EVENT_STATUS:
         {
            switch (status->state)
            {
               case DVR_RECORD_STATE_STARTED:
                  if (rec->rec_state == CTvRecord::REC_STOPPED) {
                    rec->rec_state = CTvRecord::REC_STARTED;
                    rec->mEvent.type = RecEvent::EVENT_REC_START;
                    rec->mEvent.id = std::string((const char*)rec->getId());
                    rec->mpObserver->onEvent(rec->mEvent);
                  }
               break;
               case DVR_RECORD_STATE_STOPPED:
                    rec->rec_state = CTvRecord::REC_STOPPED;
                    rec->mEvent.type = RecEvent::EVENT_REC_STOP;
                    rec->mEvent.id = std::string((const char*)rec->getId());
                    rec->mEvent.error = 0;
                    rec->mEvent.size = status->info.size;
                    rec->mEvent.time = status->info.time;
                    rec->mpObserver->onEvent(rec->mEvent);
               break;
               default:
               break;
            }
            break;
         }
         case DVR_RECORD_EVENT_WRITE_ERROR:
         {

            break;
         }
         default:
         {
            LOGD("Unhandled recording event %d", event);
            break;
         }
      }
   }
   return DVR_SUCCESS;
}

int CTvRecord::start(const char *param)
{
    int ret = -1;
    LOGD("start(%s:%s)", toReadable(mId), toReadable(param));
#ifdef SUPPORT_ADTV

    if (mIsTsplayer == true) {
        rec_open_params.event_fn = RecEventHandler;
        rec_open_params.event_userdata = this;

        LOGD("start(rec_open_params.location:%s)---",  rec_open_params.location);
        snprintf(rec_open_params.location, DVR_MAX_LOCATION_SIZE,
              "%s/%s", rec_open_params.location, DEFAULT_TIMESHIFT_BASENAME);

        /*flush size for radio*/
        if (has_video == false && has_audio == true)
            rec_open_params.flush_size = 1024;
        else
            rec_open_params.flush_size = 256 * 1024;
        //rec_open_params.flush_size = 256 * 1024;
        LOGD("start(rec_open_params.location:%s) rec_open_params.flush_size(%d) time(%ld)ms max size(%lld)byte seg size(%lld)byte",
        rec_open_params.location, rec_open_params.flush_size, rec_open_params.max_time, rec_open_params.max_size, rec_open_params.segment_size);

        dvr_wrapper_open_record(&wrap_recorder, &rec_open_params);
        dvr_wrapper_start_record(wrap_recorder, &rec_start_params);
        return 0;
    }

    ret = AM_REC_Create(&mCreateParam, &mRec);
    if (ret != DVB_SUCCESS) {
        LOGD("create fail(%d)", ret);
        mRec = NULL;
        return ret;
    }

    AM_REC_SetUserData(mRec, this);
    AM_EVT_Subscribe((long)mRec, AM_REC_EVT_RECORD_START, rec_evt_cb, (void*)getId());
    AM_EVT_Subscribe((long)mRec, AM_REC_EVT_RECORD_END, rec_evt_cb, (void*)getId());

    AM_REC_SetTFile(mRec, NULL, REC_TFILE_FLAG_AUTO_CREATE);
    ret = AM_REC_StartRecord(mRec, &mRecParam);
    if (ret != DVB_SUCCESS) {
        LOGD("start fail(%d)", ret);
        AM_EVT_Unsubscribe((long)mRec, AM_REC_EVT_RECORD_START, rec_evt_cb, (void*)getId());
        AM_EVT_Unsubscribe((long)mRec, AM_REC_EVT_RECORD_END, rec_evt_cb, (void*)getId());
        AM_REC_Destroy(mRec);
        mRec = NULL;
    } else {
        LOGD("start ok.");
    }
#endif
    return ret;
}

int CTvRecord::stop(const char *param)
{
    LOGD("stop(%s:%s)", toReadable(mId), toReadable(param));
#ifdef SUPPORT_ADTV
    int ret = 0;
    if (mIsTsplayer == true) {
        dvr_wrapper_stop_record(wrap_recorder);
        dvr_wrapper_close_record(wrap_recorder);
        rec_state = CTvRecord::REC_STOPPED;
        return 0;
    }
    if (!mRec)
        return -1;

    ret = AM_REC_StopRecord(mRec);

    AM_EVT_Unsubscribe((long)mRec, AM_REC_EVT_RECORD_START, rec_evt_cb, (void*)getId());
    AM_EVT_Unsubscribe((long)mRec, AM_REC_EVT_RECORD_END, rec_evt_cb, (void*)getId());

    return ret;
#else
    return -1;
#endif
}

int CTvRecord::setRecCurTsOrCurProgram(int sel)
{
    LOGD("setRecCurTsOrCurProgram(%s:%d)", toReadable(mId), sel);
#ifdef SUPPORT_ADTV
    char buf[64];
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf), "/sys/class/stb/dvr%d_mode", mCreateParam.dvr_dev);
    if (sel)
        tvWriteSysfs(buf, "ts");
    else
        tvWriteSysfs(buf, "pid");
#endif
    return 0;
}

bool CTvRecord::equals(CTvRecord &recorder)
{
#ifdef SUPPORT_ADTV
    return mCreateParam.fend_dev == recorder.mCreateParam.fend_dev
        && mCreateParam.dvr_dev == recorder.mCreateParam.dvr_dev
        && mCreateParam.async_fifo_id == recorder.mCreateParam.async_fifo_id;
#else
    return false;
#endif
}
char* CTvRecord::getLocation()
{
    return rec_open_params.location;
}
int CTvRecord::getStartPosition()
{
#ifdef SUPPORT_ADTV
    if (!mRec)
        return 0;
#endif
    return 0;
}

int CTvRecord::getWritePosition()
{
    return 0;
}

int CTvRecord::setupDefault(const char *param)
{
#ifdef SUPPORT_ADTV
    int i = 0,sub_count = 0;
    char buff[32] = {0};
    int apid = 0x1fff;
    int vpid = 0x1fff;
    setDev(CTvRecord::REC_DEV_TYPE_FE, paramGetInt(param, NULL, "fe", 0));
    setDev(CTvRecord::REC_DEV_TYPE_DVR, paramGetInt(param, NULL, "dvr", 0));
    setDev(CTvRecord::REC_DEV_TYPE_FIFO, paramGetInt(param, NULL, "fifo", 0));
    setFilePath(paramGetString(param, NULL, "path", "/storage").c_str());
    setFileName(paramGetString(param, NULL, "prefix", "REC").c_str(), paramGetString(param, NULL, "suffix", "ts").c_str());

    if (mIsTsplayer == true) {

        memset(&rec_start_params, 0, sizeof(rec_start_params));
        rec_start_params.pids_info.nb_pids = 0;
        //video
        has_video = false;
        vpid = paramGetInt(param, "v", "pid", -1);
        if (vpid > 0 && vpid < 0x1fff) {
            has_video = true;
            rec_start_params.pids_info.pids[rec_start_params.pids_info.nb_pids].pid = paramGetInt(param, "v", "pid", -1);
            rec_start_params.pids_info.pids[rec_start_params.pids_info.nb_pids].type =
                (DVR_StreamType_t)((DVR_STREAM_TYPE_VIDEO << 24) | toDvrVideoFormat(paramGetInt(param, "v", "fmt", -1)));
            rec_start_params.pids_info.nb_pids++;
        } else {
            vpid = 0x1fff;
        }

        //audio
        has_audio = false;
        apid = paramGetInt(param, "a", "pid", -1);
        if (apid > 0 && apid < 0x1fff) {
            has_audio = true;
            rec_start_params.pids_info.pids[rec_start_params.pids_info.nb_pids].type =
                (DVR_StreamType_t)((DVR_STREAM_TYPE_AUDIO << 24) | toDvrAudioFormat(paramGetInt(param, "a", "fmt", -1)));
            rec_start_params.pids_info.pids[rec_start_params.pids_info.nb_pids].pid = paramGetInt(param, "a", "pid", -1);
            rec_start_params.pids_info.nb_pids++;
        } else {
            apid = 0x1fff;
        }
        //sub
        sub_count = paramGetInt(param, NULL, "subcnt", 0);
        for (i = 0; i < sub_count; i++)
        {
            sprintf(buff, "pid%d", i);
            rec_start_params.pids_info.pids[rec_start_params.pids_info.nb_pids].pid = paramGetInt(param, NULL, buff, 0);
            rec_start_params.pids_info.pids[rec_start_params.pids_info.nb_pids].type = (DVR_StreamType_t)(DVR_STREAM_TYPE_SUBTITLE << 24);
            rec_start_params.pids_info.nb_pids++;
          //LOGE("Get subpid%d %x count %d", i, info.subtitles[i].pid,info.sub_cnt);
        }

        {
            Json::Reader reader;
            Json::Value root;
            int acnt = 0;
            if (reader.parse(param, param + strlen(param), root)) {
                if (root.isMember("as") && (acnt = root["as"].size())) {
                    Json::Value as = root["as"];
                    int i;
                    for (i = 0; i < acnt && rec_start_params.pids_info.nb_pids < DVR_MAX_RECORD_PIDS_COUNT; i++) {
                        if (apid == as[i]["pid"].asInt())
                          continue;
                        rec_start_params.pids_info.pids[rec_start_params.pids_info.nb_pids].type =
                            (DVR_StreamType_t)((DVR_STREAM_TYPE_AUDIO << 24) | toDvrAudioFormat(as[i]["fmt"].asInt()));
                        rec_start_params.pids_info.pids[rec_start_params.pids_info.nb_pids].pid = as[i]["pid"].asInt();
                        rec_start_params.pids_info.nb_pids++;
                    }
                  }
            } else {
                LOGD("parse fail:(%s)", param);
            }
        }

        //default 30min and 500 M
        setTimeShiftMode(
        paramGetInt(param, NULL, "timeshift", 0) ? true : false,
        paramGetInt(param, "max", "time", 30 * 60),
        paramGetInt(param, "max", "size", 500 * 1024 * 1024));
        return 0;
    }


    AM_REC_MediaInfo_t info;
    info.duration = 0;
    info.vid_pid = paramGetInt(param, "v", "pid", -1);
    info.vid_fmt = paramGetInt(param, "v", "fmt", -1);
    info.aud_cnt = 1;
    info.audios[0].pid = paramGetInt(param, "a", "pid", -1);
    info.audios[0].fmt = paramGetInt(param, "a", "fmt", -1);
    info.sub_cnt = paramGetInt(param, NULL, "subcnt", 0);
    for (i=0; i<info.sub_cnt; i++)
    {
        sprintf(buff, "pid%d", i);
        info.subtitles[i].pid = paramGetInt(param, NULL, buff, 0);
        //LOGE("Get subpid%d %x count %d", i, info.subtitles[i].pid,info.sub_cnt);
    }
    info.ttx_cnt = 0;
    memset(info.program_name, 0, sizeof(info.program_name));
    setMediaInfo(&info);
    setTimeShiftMode(
        paramGetInt(param, NULL, "timeshift", 0) ? true : false,
        paramGetInt(param, "max", "time", 60),
        paramGetInt(param, "max", "size", -1));
#endif
    return 0;
}

