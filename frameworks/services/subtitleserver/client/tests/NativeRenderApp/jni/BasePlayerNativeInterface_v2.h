/*
 * Copyright 2001-2021 by Alticast, Corp., All rights reserved.
 *
 * This software is the confidential and proprietary information
 * of Alticast, Corp. ("Confidential Information"). You shall
 * not disclose such Confidential Information and shall use
 * it only in accordance with the terms of the license agreement
 * you entered into with Alticast, Corp.
 */


#ifndef BASEPLAYER_NATIVE_INTERFACE_H
#define BASEPLAYER_NATIVE_INTERFACE_H

#include <stdbool.h>

#include <jni.h>
#include <android/native_window.h>


#ifdef __cplusplus
extern "C" {
#endif


/* definition of player handle ID */
typedef enum {
    BPNI_PLAYER_HANDLE_ID_MAIN = 1,    // MAIN
    BPNI_PLAYER_HANDLE_ID_PIP =  2,    // PIP
    BPNI_PLAYER_HANDLE_ID_SUB1 = 3,    // SUB1  // PIP and SUB1 have different ID value, but they will use same player.
    BPNI_PLAYER_HANDLE_ID_SUB2 = 4,    // SUB2
    BPNI_PLAYER_HANDLE_ID_SUB3 = 5    // SUB3
} BPNI_PLAYER_HANDLE_ID;

/**
 * Event callback function from Player
 *
 * @param[in] handleId handle ID. (player handle)
 * @param[in] event
 */
typedef void (*BPNI_PlayerEventCallback)(BPNI_PLAYER_HANDLE_ID handleId, int event);

/**
 * register event callback
 *
 * @param[in] handleId handle ID.
 * @param[in] callback function.
  */
void bpni_registerPlayerEventCallback(BPNI_PLAYER_HANDLE_ID handleId, BPNI_PlayerEventCallback callback);


/*** AML NOTE: tunerId usage (normally use this way, but not always, there will be exceptional case)
 *	1-channel playback: tunerId = 1
 *  2-Channel PIP :		tunerId = 1, 2
 *  4-Channel Multiview :	tunerId = 1, 3, 4, 5
 ***/
/**
 * Set TunerID(or Demux Id), TS stream data will feed this Tuner(or Demux)
 * And CAS will use this Tuner ID
 * @param[in] handleId handle ID.
 * @param[in] tunerId tuner ID.
 */
void bpni_setTunerId(BPNI_PLAYER_HANDLE_ID handleId, int tunerId);

/**
 * Return Tuner Id(or demux Id)
 *
 * @param[in] handleId handle ID.
 * @return Tuner ID. if empty, then return "-1" (if not set before, or source were some kind of URLs)
 */
int bpni_getTunerId(BPNI_PLAYER_HANDLE_ID handleId);


/**
 * Feed TS stream data for Live Channel, VoD to given player
 * @param[in] handleId handle ID.
 * @param[in] data TS stream data.
 * @param[in] len data length.
 */
void bpni_feedData(BPNI_PLAYER_HANDLE_ID handleId, char* data, int len);

/**
 * Set ANativeWindow (came from upper JAVA layer's Surface object) for setting video surface
 *
 * 		NOTE: Java layer will use this API after conversion by ANativeWindow_fromSurface() from jobject surface to ANativeWindow
 *		conversion: ANativeWindow* ANativeWindow_fromSurface(JNIEnv* env, jobject surface)
 *
 * @param[in] handleId handle ID.
 * @param[in] pointer of native window surface for video
 */
void bpni_setSurfaceNativeWindow(BPNI_PLAYER_HANDLE_ID handleId, ANativeWindow* window);

/**
 *
 * @param[in] handleId handle ID.
 * @return return Player's pointer of ANativeWindow
 */
ANativeWindow* bpni_getSurfaceNativeWindow(BPNI_PLAYER_HANDLE_ID handleId);



/*** AML NOTE:
 *	bpni_prepare is almost same as CreatePlayer
 ***/
/**
 * Prepare playback.
 * blocks until MediaPlayer is ready for playback
 *
 * Note : calling sequence - this function would be called after setTunerId() and setSurfaceNativeWindow()
 *
 * @param[in] handleId handle ID.
 * @return 0 Success. -1 fail.
 */
int bpni_prepare(BPNI_PLAYER_HANDLE_ID handleId);

/**
 * Play A/V with given a-v PID
 * <br> this API can be called multiple times without calling stop()
 * <br> i.e.) playStream() -> stop() -> playStream()
 * <br> ex 1) playStream(8006, 8005) -> playStream(8007, 8005)  : change audio language or change audio track
 * <br> ex 2) playStream(8006, 8005) -> playSteram(0, 8005) : normal AV play -> play video ONLY
 * <br> ex 3) playStream(0, 8005) -> playSteram(8006, 8005) : play video ONLY -> player both A/V
 * <br> ex 4) playStream(8006, 8005) -> playStream(0, 0) : same as stop(), no A/V playing
 *
 * @param[in] handleId handle ID.
 * @param[in] audiPID - audio PID. "0" means NO audio or stop audio (audio decoder stop)
 * @param[in] videoPID - video PID. "0" means NO video or stop video (video decoder stop)
 *
 * @return 0 Success. -1 fail.
 */
int bpni_playStream(BPNI_PLAYER_HANDLE_ID handleId, int audiPID, int videoPID);

/**
 * Stop A/V.
 * Stop selective Audio or Video. (choiceable)
 *
 * @param[in] handleId handle ID.
 * @param[in] isAudioStop if true,  audio stop
 * @param[in] isVideoStop if true, video stop
 * @return 0 Success, -1 fail.
 */
int bpni_stop(BPNI_PLAYER_HANDLE_ID handleId, bool isAudioStop, bool isVideoStop);

/**
 * set volume control
 *
 * @param[in] handleId handle ID.
 * @param[in] volume value: range = 0.0f ~ 1.0f, step => 0.01
 * @return 0 Success. -1 fail.
 */
int bpni_setVolume(BPNI_PLAYER_HANDLE_ID handleId, float volume);

/**
 * return current volume level
 *
 * @param[in] handleId handle ID.
 * @return current volume level. If not available or fail, return -1.0f.
 */
float bpni_getVolume(BPNI_PLAYER_HANDLE_ID handleId);


/**
 * Configure audio PID for Bluetooth device
 *
 * @param[in] handleId handle ID.
 * @param[in] audioPid audio PID.
 * @return success: 0,  fail: -1
 */
int bpni_setBTAudioOutputPid(BPNI_PLAYER_HANDLE_ID handleId, int audioPid);

/**
 * Set trick playback rate
 * In case of normal trick (NOT 0.8x, 1.2x, 1.5x), flush remained buffer
 *
 * @param[in] handleId handle ID.
 * @param rate  : playback rate
 * @return 0 Success. -1 fail.
 */
int bpni_setRate(BPNI_PLAYER_HANDLE_ID handleId, float rate);

/**
 * return current playback rate
 *
 * @param[in] handleId handle ID.
 * @return current playback rate. If not playing, return 0f.
 */
float bpni_getRate(BPNI_PLAYER_HANDLE_ID handleId);

/**
 * Return PTS
 *
 * @param[in] handleId handle ID.
 * @return PTS. fail: -1 return.
 */
double bpni_getPts(BPNI_PLAYER_HANDLE_ID handleId);

/**
 *  Flush stream data buffer inside of Player
 *
 * @param[in] handleId handle ID.
 */
void bpni_flush(BPNI_PLAYER_HANDLE_ID handleId);

/**
 * Set ANativeWindow surface for drawing ClosedCaption
 *
 * @param[in] handleId handle ID.
 * @param[in] window pointer of ANativeWindow surface for CC
 */
void bpni_setSubtitleSurfaceNativeWindow(BPNI_PLAYER_HANDLE_ID handleId, ANativeWindow* window);

/**
 * Subtitle (Closed Caption / VoD SMI subtitle)  On / Off
 *
 * @param[in] handleId handle ID.
 * @param[in] enabled true = On , false = Off
 */
void bpni_setSubtitleEnabled(BPNI_PLAYER_HANDLE_ID handleId, bool enabled);

/**
 * Return Subtitle On or Off status
 *
 * @param[in] handleId handle ID.
 * @return true:On, false: Off
 */
bool bpni_getSubtitleEnabled(BPNI_PLAYER_HANDLE_ID handleId);


/**
 * Set SMI URL for VoD
 *
 * @param[in] handleId handle ID.
 * @param[in] url SMI subtitle URL
 */
void bpni_setSubtitleSmiUrl(BPNI_PLAYER_HANDLE_ID handleId, char* url);


typedef enum {
    BPNI_ASPECT_RATIO_UNKNOWN = -1, /**< Unknown aspect ratio. */
    BPNI_ASPECT_RATIO_4_3= 2,       /**< Aspect ratio 4:3. */
    BPNI_ASPECT_RATIO_16_9,         /**< Aspect ratio 16:9. */
    BPNI_ASPECT_RATIO_2_21_1        /**< Aspect ratio 2.21:1. */
} BPNI_VIDEO_AR_TYPE;

typedef enum {
    BPNI_VIDEO_CODEC_UNKNOWN,
    BPNI_VIDEO_CODEC_NONE,
    BPNI_VIDEO_CODEC_MPEG1,
    BPNI_VIDEO_CODEC_MPEG2,
    BPNI_VIDEO_CODEC_MPEG4,
    BPNI_VIDEO_CODEC_H263,
    BPNI_VIDEO_CODEC_H264,
    BPNI_VIDEO_CODEC_VC1,
    BPNI_VIDEO_CODEC_DIVX,
    BPNI_VIDEO_CODEC_AVS,
    BPNI_VIDEO_CODEC_RV40,
    BPNI_VIDEO_CODEC_VP6,
    BPNI_VIDEO_CODEC_VP7,
    BPNI_VIDEO_CODEC_VP8,
    BPNI_VIDEO_CODEC_SPARK,
    BPNI_VIDEO_CODEC_MJPEG
} BPNI_VIDEO_CODEC;

typedef enum {
    BPNI_AUDIO_CODEC_UNKNOWN,
    BPNI_AUDIO_CODEC_NONE,
    BPNI_AUDIO_CODEC_MPEG,
    BPNI_AUDIO_CODEC_MP3,
    BPNI_AUDIO_CODEC_AAC,
    BPNI_AUDIO_CODEC_AC3,
    BPNI_AUDIO_CODEC_DTS,
    BPNI_AUDIO_CODEC_LPCM,
    BPNI_AUDIO_CODEC_WMA,
    BPNI_AUDIO_CODEC_AVS,
    BPNI_AUDIO_CODEC_PCM,
    BPNI_AUDIO_CODEC_AMR,
    BPNI_AUDIO_CODEC_DRA,
    BPNI_AUDIO_CODEC_COOK,
    BPNI_AUDIO_CODEC_ADPCM,
    BPNI_AUDIO_CODEC_SBC,
    BPNI_AUDIO_CODEC_VORBIS,
    BPNI_AUDIO_CODEC_G711,
    BPNI_AUDIO_CODEC_G723,
    BPNI_AUDIO_CODEC_G726,
    BPNI_AUDIO_CODEC_G729,
    BPNI_AUDIO_CODEC_FLAC,
    BPNI_AUDIO_CODEC_MLP,
    BPNI_AUDIO_CODEC_APE
} BPNI_AUDIO_CODEC;


typedef struct _MediaMetaData {

    /**
     * aspect ratio
     */
    BPNI_VIDEO_AR_TYPE ar_type;

    /**
     * width
     */
    int width;

    /**
     * height
     */
    int height;

    /**
     * audio codec.
     */
    BPNI_AUDIO_CODEC audioCodec;

    /**
     * video codec
     */
    BPNI_VIDEO_CODEC videoCodec;

} BPNI_MediaMetaData;

/**
 * Get A/V meta data
 *
 * @param[in] handleId handle ID.
 * @return BaseMediaMetaData
 */
BPNI_MediaMetaData bpni_getMetaData(BPNI_PLAYER_HANDLE_ID handleId);

/* for CAS */

/**
  * Send CAS Session Event
  *
  * @param[in] jsonString
  * @return 0 Success. -1 fail.
  */
int bpni_cas_sendCasSessionEvent(char* jsonString);


/**
  * When event is received from CAS plugin (CAS plugin -> AML Hal player -> Alticast App)
  *
  * @param[in] event event from CAS plugin
  */
typedef void (*BPNI_CAS_CasEventCallback)(char* event);

/**
 * Register CAS event callback
 *
 * @param[in] callback Callback function
  */
void bpni_cas_registerCasEventCallback(BPNI_CAS_CasEventCallback callback);

/**
  * Setting EMM Address
  *
  * @return 0 Success. -1 fail.
  */
int bpni_cas_setEMMAddress(char* address);


#ifdef __cplusplus
};
#endif


#endif // BASEPLAYER_NATIVE_INTERFACE_H
