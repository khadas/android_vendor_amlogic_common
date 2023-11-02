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
package com.droidlogic.app;

import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.media.MediaCodec;
import android.media.MediaMuxer;
import android.media.MediaMuxer.OutputFormat;
import android.media.MediaCodec.BufferInfo;
import android.media.MediaExtractor;
import android.media.MediaCodec.CodecException;
import android.media.MediaFormat;
import android.media.AudioFormat;

import android.util.Log;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Environment;
import android.os.RemoteException;
import android.os.SystemClock;
import java.util.ArrayDeque;
import java.util.List;
import java.nio.ByteBuffer;
import java.io.IOException;
import java.io.File;
import java.io.FileOutputStream;



class AudioSource {
    private static final String TAG = "AmAudioSource";
    private AudioRecord mAudioRecord = null;
    private AudioDataListener mAudioDataListener = null;
    private MediaCodec  mCodec = null;
    private Handler mCallbackHandler;
    private FileOutputStream mPcmFos = null;
    private File mPcmFile = null;


    private HandlerThread  mCallbackHandlerThread = null;
    MediaCallback mMediaCallback = null;
    private static final String THREADNAME= "AmAudioSourceThread";
    public static final int RECORD_SUCCESS = 0;
    public static final int RECORD_ENCODER_ERROR = -1;
    public static final boolean isDumpPcm = false;

    private int mMinBufferSize = 0;
    private volatile boolean isRecording = false;
    private ArrayDeque<Integer> mInputBuffIndecQueue;

    AudioSource() {
        mInputBuffIndecQueue = new ArrayDeque<>();
        mInputBuffIndecQueue.clear();
    }
    public void setListener(AudioDataListener l) {
        mAudioDataListener = l;
    }
    public static interface AudioDataListener {
        void onAudioAvailable(ByteBuffer data,BufferInfo info);
        void onOutputFormatChanged(MediaFormat format);
    }
    private class MediaCallback extends MediaCodec.Callback {
        private static final String TAG = "AmAudioSource";
        private AudioSource mAudioSource = null;
        MediaCallback(AudioSource source) {
            mAudioSource = source;
        }

        public void onInputBufferAvailable(MediaCodec codec, int index) {
            Log.i(TAG, codec + " onInputBufferAvailable index = " + index);

            synchronized (mAudioSource.mInputBuffIndecQueue) {
                mAudioSource.mInputBuffIndecQueue.add(index);
            }

        }
        public void onOutputBufferAvailable(MediaCodec codec, int index, MediaCodec.BufferInfo bufferInfo) {
            Log.i(TAG, codec + " onOutputBufferAvailable index = " + index + ",flag = "+bufferInfo.flags+",pts = "+bufferInfo.presentationTimeUs);
            ByteBuffer outputBuffer = mCodec.getOutputBuffer(index);
            if (mAudioDataListener != null && outputBuffer != null) {
                mAudioDataListener.onAudioAvailable(outputBuffer,bufferInfo);
            }
            mCodec.releaseOutputBuffer(index, false);
        }
        public void onOutputFormatChanged(MediaCodec codec, MediaFormat format) {
            Log.i(TAG, codec + " outputFormatChanged " + format);
            if (mAudioDataListener != null) {
                mAudioDataListener.onOutputFormatChanged(format);
            }
        }
        public void onError(MediaCodec codec, MediaCodec.CodecException exception) {
            Log.e(TAG, codec + " onError " + exception);
        }



    }
    public int startRecord(String mine, int sampleRate, int channelCount, int bitRate ,int accProfile,int priority,int sourceType,int pcmFormat) {
        int real_count = 0;
        if (isRecording || mAudioDataListener == null)
            return RECORD_ENCODER_ERROR;
        mMinBufferSize = AudioRecord.getMinBufferSize(sampleRate, channelCount, pcmFormat);
        if (mMinBufferSize <= 0 )
            return RECORD_ENCODER_ERROR;
        MediaFormat format = new MediaFormat();
        format.setString(MediaFormat.KEY_MIME,mine);

        switch (channelCount) {
        case AudioFormat.CHANNEL_IN_DEFAULT: // AudioFormat.CHANNEL_CONFIGURATION_DEFAULT
        case AudioFormat.CHANNEL_IN_MONO:
        case AudioFormat.CHANNEL_CONFIGURATION_MONO:
            real_count = 1;
            break;
        case AudioFormat.CHANNEL_IN_STEREO:
        case AudioFormat.CHANNEL_CONFIGURATION_STEREO:
        case (AudioFormat.CHANNEL_IN_FRONT | AudioFormat.CHANNEL_IN_BACK):
            real_count = 2;
            break;
        case AudioFormat.CHANNEL_INVALID:
        default:
            throw new IllegalStateException("the channel count don't support mAudioChannelCount ：" + channelCount );
        }
        format.setInteger(MediaFormat.KEY_CHANNEL_COUNT,real_count);
        format.setInteger(MediaFormat.KEY_SAMPLE_RATE,sampleRate);
        format.setInteger(MediaFormat.KEY_BIT_RATE,bitRate);
        format.setInteger(MediaFormat.KEY_AAC_PROFILE,accProfile);
        format.setInteger(MediaFormat.KEY_PRIORITY,priority);
        format.setInteger(MediaFormat.KEY_MAX_INPUT_SIZE,mMinBufferSize);
        boolean ret = initAudioEncoder(format);
        if (!ret)
            return RECORD_ENCODER_ERROR;
        if (isDumpPcm) {
            mPcmFile = new File(Environment.getExternalStorageDirectory(), "dump.pcm");
            if (mPcmFile == null )
                return RECORD_ENCODER_ERROR;
            try {
                mPcmFos = new FileOutputStream(mPcmFile, true);
            }catch (Exception e) {
                Log.e(TAG, "File ["+mPcmFile.getName()+"] cannot open: "+e+", exit...");
                return RECORD_ENCODER_ERROR;
            }
        }

        Log.i(TAG," startRecord  getMinBufferSize mMinBufferSize = " + mMinBufferSize);
        mAudioRecord = new AudioRecord(sourceType,sampleRate,channelCount,pcmFormat,mMinBufferSize);
        if (mAudioRecord == null || mAudioRecord.getState() != AudioRecord.STATE_INITIALIZED)
            return RECORD_ENCODER_ERROR;
        mAudioRecord.startRecording();
        isRecording = true;
        startPullingData();
        return RECORD_SUCCESS;

    }
    private void startPullingData() {
      new Thread() {
        @Override
        public void run(){
            ByteBuffer data = ByteBuffer.allocateDirect(mMinBufferSize);
            while (isRecording) {
                int size = mAudioRecord.read(data, mMinBufferSize, AudioRecord.READ_BLOCKING);
                if (size > 0 && data != null) {
                    Log.d(TAG,"get data from audio recorder size = " + size);
                    if (isDumpPcm) {
                         try {
                            byte [] raw = new byte[data.remaining()];
                            data.get(raw);
                            if (mPcmFos != null) {
                                mPcmFos.write(raw);
                            }
                            data.flip();

                        } catch (Exception e) {
                            Log.e(TAG, "Write record data error: "+e);
                        }

                    }
                    synchronized (mInputBuffIndecQueue) {
                        if (!mInputBuffIndecQueue.isEmpty()) {
                            int index = mInputBuffIndecQueue.pop();
                            try{
                                ByteBuffer inputBuffer = mCodec.getInputBuffer(index);
                                Log.d(TAG,"getInputBuffer inputBuffer = " + inputBuffer.toString());
                                if (size <= inputBuffer.remaining()) {
                                    inputBuffer.put(data);
                                    data.flip();
                                }

                            }catch (Exception e){
                                Log.e(TAG,"get un-documented exception as a result of getInputBuffer " + e );
                                e.printStackTrace();
                                break;
                            }
                            long timeUs = SystemClock.elapsedRealtimeNanos() / 1000;
                            Log.d(TAG,"queueInputBuffer timeUs = " + timeUs+ ",index="+index);
                            mCodec.queueInputBuffer(index, 0, size, timeUs, 0);

                        }
                    }
                }
                if (size <= 0) {
                    continue;
                }
            }
        }
      }.start();
    }
    public void stopRecord() {
        Log.d(TAG,"stopRecord begin");
        if (!isRecording)
            return;
        isRecording = false;
        if (mAudioRecord != null) {
            mAudioRecord.stop();
            mAudioRecord.release();
            mAudioRecord = null;
        }
        if (mCallbackHandlerThread != null) {
            mCallbackHandlerThread.quitSafely();
            mCallbackHandlerThread = null;
        }
        if (mCodec != null) {
            try{
                mCodec.stop();
            }catch (Exception e){
                Log.d(TAG,"get un-documented exception as a result of stop() ", e);
                e.printStackTrace();
            }

            try{
                mCodec.release();
            }catch (Exception e){
                Log.d(TAG,"get un-documented exception as a result of release() ", e);
                e.printStackTrace();
            }
        }
        if (isDumpPcm) {
            try {
                if (mPcmFos != null) {
                    mPcmFos.close();
                    mPcmFos = null;
                }
            }catch (Exception e) {
                Log.e(TAG, "File ["+mPcmFile.getName()+"] cannot open: "+e+", exit...");
                return ;
            }

        }

        mInputBuffIndecQueue.clear();
        mInputBuffIndecQueue = null;
        Log.d(TAG,"stopRecord done");
    }
    public boolean isRecording() {
      return isRecording;
    }
    public boolean initAudioEncoder(MediaFormat format) {
        Log.i(TAG," initAudioEncoder in" );
        if (mCodec != null || format == null)
            return false;
        try {
            String mine = format.getString(MediaFormat.KEY_MIME,"default");
            mCodec = MediaCodec.createEncoderByType(mine);
        }catch (IllegalArgumentException | IOException ex) {
            ex.printStackTrace();
            Log.e(TAG, "Failed to create encoder !!" );
            return false;
        }
        if (mCodec == null) {
            Log.e(TAG, "enocoder create fail !!");
            return false;
        }
        try {
            mCodec.configure(format, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
        } catch(IllegalArgumentException
            | MediaCodec.CryptoException e) {
            Log.e(TAG, "Failed to configure " + mCodec.getName() + " encoder.");
            e.printStackTrace();
            return false;
        }
        mCallbackHandlerThread = new HandlerThread(THREADNAME);
        mCallbackHandlerThread.start();
        mCallbackHandler = new Handler(mCallbackHandlerThread.getLooper());

        mMediaCallback = new MediaCallback(this);
        mCodec.setCallback(mMediaCallback,mCallbackHandler);
        mCodec.start();
        Log.i(TAG," initAudioEncoder finish" );
        return true;
    }
}

class VideoSource {
    private static final String TAG = "AmVideoSource";
    private ScreenControlManager mScreenControl = null;
    private AvcCallbackListener mAvcCallbackListener = null;
    private VideoDataListener mVideoDataListener = null;
    public static final int RECORD_SUCCESS = 0;
    public static final int RECORD_ENCODER_ERROR = -1;

    public static interface VideoDataListener {
        void onVideoAvailable(byte[] data, int frameType, long pts);
    }
    public class AvcCallbackListener implements ScreenControlManager.AvcCallbackListener {
		@Override
		public void onAvcAvailable(byte[] data,int frameType, long pts){
			if (null != data) {
                if (mVideoDataListener != null)
                    mVideoDataListener.onVideoAvailable(data,frameType,pts);
                Log.d(TAG, "onAvcAvailable "+",frameType="+frameType+ ",pts=" + pts);
	        }
		}
	}
    public int startRecord(int left, int top, int right, int bottom, int width, int height, int frameRate, int bitRate, int sourceType) {
        mScreenControl = new ScreenControlManager();
        if (mScreenControl == null)
            return RECORD_ENCODER_ERROR;
        mAvcCallbackListener = new AvcCallbackListener();
        mScreenControl.setAvcCallbackListener(mAvcCallbackListener);
        return mScreenControl.startAvcScreenRecord(left, top, right, bottom, width, height, frameRate, bitRate, sourceType);
    }
    public int startRecord(int width, int height, int frameRate, int bitRate, int sourceType) {
        return startRecord(0, 0, width, height, width, height, frameRate, bitRate, sourceType);

    }
    public void stop() {
        if (mScreenControl != null) {
            mScreenControl.stopRecord();
            mScreenControl.release();
            mScreenControl = null;
        }
    }
    public void setListener(VideoDataListener l) {
        mVideoDataListener = l;
    }
}

public class ScreenRecorder {
    private static final String TAG = "ScreenRecorder";
    private VideoCallBack mVideoCallBack = null;
    private AudioCallBack mAudioCallBack = null;
    private ByteBuffer mVideoSPSBuffer = null;
    private ByteBuffer mVideoPPSBuffer = null;

    private MediaMuxer mMuxer = null;
    private AudioSource mAudio = null;
    private VideoSource mVideo = null;
    private final Object mLock = new Object();


    private static final int ERROR_BAD_VALUE = -1;

    private  String mAudioMine;
    private  String mOutputFileName;

    private int mAudioSampleRate;
    private int mAudioChannelCount;
    private int mAudioBitRate;
    private int mAudioAccProfile;
    private int mAudioPriority;
    private int mAudioSourceType;
    private int mAudioPcmFormat;
    private int mVideoLeft;
    private int mVideoTop;
    private int mVideoRight;
    private int mVideoBottom;
    private int mVideoWidth;
    private int mVideoHeight;
    private int mVideoFrameRate;
    private int mVideoBitRate;
    private int mAvcFrameType;
    private int mRecordTime = ERROR_BAD_VALUE;
    private int mAudioTrack = -1;
    private int mVideoTrack = -1;

    private boolean mStarted = false;
    private boolean mMuxerStrated = false;

    private long mFirstAudioTimeUs = 0;
    private long mFirstVideoTimeUs = 0;
    private long mLastAudioTimeUs = 0;
    private long mLastVideoTimeUs = 0;


    /**
     * Class constructor.
     * @param fileName the name of MP4 file.
     * @param limitTimeSec the time of record.
     * if the limitTimeSec is less than 0 ,
        must call {@code start} to stop record.
     */
    public ScreenRecorder(String fileName,int limitTimeSec) {
        Log.d(TAG, "ScreenRecorder fileName = " + fileName + ",limitTimeSec = "+limitTimeSec);
        mOutputFileName = fileName;
        mAudioSourceType = ERROR_BAD_VALUE;
        mAvcFrameType = ERROR_BAD_VALUE;
        mRecordTime = limitTimeSec;
    }
    public class AudioCallBack implements AudioSource.AudioDataListener {
        @Override
        public void onAudioAvailable(ByteBuffer data, BufferInfo info) {
            Log.d(TAG, "onAudioAvailable pts = "+ info.presentationTimeUs);
                if (!mStarted)
                    return;
                if (!((info.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) != 0
                        && info.size != 0) && mMuxerStrated) {
                    if (mFirstAudioTimeUs == 0)
                        mFirstAudioTimeUs = info.presentationTimeUs;
                    mLastAudioTimeUs = info.presentationTimeUs;
                    mMuxer.writeSampleData(mAudioTrack, data, info);
                }
        }
        @Override
        public void onOutputFormatChanged(MediaFormat format) {
            Log.i(TAG," outputFormatChanged  format= " + format);

            if (mMuxer != null) {
                mAudioTrack = mMuxer.addTrack(format);
                Log.i(TAG," outputFormatChanged mAudioTrack = " + mAudioTrack);
                if (mAudioTrack >= 0 && mVideoTrack >= 0 && !mMuxerStrated) {
                    Log.i(TAG," outputFormatChanged mMuxer.start " );
                    mMuxer.start();
                    mMuxerStrated = true;
                }
            }

        }
    }
    public class VideoCallBack implements VideoSource.VideoDataListener {
        @Override
        public void onVideoAvailable(byte[] data, int frameType, long pts) {
            Log.d(TAG, "onVideoAvailable pts = "+ pts + ",frameType = " + frameType);
                if (!mStarted)
                    return;
                BufferInfo bufferInfo = new BufferInfo();
                ByteBuffer metaData = null;
                int buffSize = 0;
                int flag = 0;
                if (frameType == ScreenControlManager.AvcFrameType.AVC_TYPE_FRAME_TYPE_SPS ||
                        (frameType == ScreenControlManager.AvcFrameType.AVC_TYPE_FRAME_TYPE_PPS)) {
                    if (frameType == ScreenControlManager.AvcFrameType.AVC_TYPE_FRAME_TYPE_SPS  && mVideoSPSBuffer == null) {
                        mVideoSPSBuffer  = ByteBuffer.allocate(data.length);
                        mVideoSPSBuffer.put(data);
                        mVideoSPSBuffer.flip();
                        Log.d(TAG, "onVideoAvailable get sps data");
                    }else if (frameType == ScreenControlManager.AvcFrameType.AVC_TYPE_FRAME_TYPE_PPS  && mVideoPPSBuffer == null) {
                        mVideoPPSBuffer  = ByteBuffer.allocate(data.length);
                        mVideoPPSBuffer.put(data);
                        mVideoPPSBuffer.flip();
                        Log.d(TAG, "onVideoAvailable get pps data");
                    }
                    if (mVideoPPSBuffer != null && mVideoSPSBuffer != null && mVideoTrack < 0) {
                        MediaFormat format = new MediaFormat();
                        format.setString(MediaFormat.KEY_MIME,"video/avc");
                        format.setInteger(MediaFormat.KEY_BIT_RATE,mVideoBitRate);
                        format.setInteger(MediaFormat.KEY_WIDTH ,mVideoWidth);
                        format.setInteger(MediaFormat.KEY_HEIGHT,mVideoHeight);
                        format.setInteger(MediaFormat.KEY_FRAME_RATE, mVideoFrameRate);
                        format.setByteBuffer("csd-0", mVideoSPSBuffer);
                        format.setByteBuffer("csd-1", mVideoPPSBuffer);
                        mVideoTrack = mMuxer.addTrack(format);
                        if (mAudioTrack >= 0 && mVideoTrack >= 0 && !mMuxerStrated) {
                            Log.i(TAG," onVideoAvailable mMuxer.start " );
                            mMuxer.start();
                            mMuxerStrated = true;
                        }
                    }
                    return;
                }

                if (frameType == ScreenControlManager.AvcFrameType.AVC_TYPE_FRAME_TYPE_IDR ) {
                    Log.d(TAG, "onVideoAvailable pts set BUFFER_FLAG_KEY_FRAME flag");
                    flag = MediaCodec.BUFFER_FLAG_KEY_FRAME;
                    boolean temp = false;
                        buffSize = data.length;
                    metaData = ByteBuffer.allocate(buffSize);
                    if (metaData == null)
                        return;
                    metaData.put(data);
                    metaData.flip();

                }else {
                    buffSize = data.length;
                    metaData = ByteBuffer.allocate(buffSize);
                    metaData.put(data);
                    metaData.flip();
                }
                if (mFirstVideoTimeUs == 0)
                    mFirstVideoTimeUs = pts;
                mLastVideoTimeUs = pts;
                bufferInfo.set(0,buffSize,pts,flag);
                if (mVideoTrack >=0 && mMuxerStrated)
                    mMuxer.writeSampleData(mVideoTrack,metaData,bufferInfo);

        }
    }
    /**
     * set the screen recrord parameter.
     * @param left,top,right,bottom set the four coordinate values of the recording area.
     * @param width set the width of the output video.
     * @param height set the height of the output video.
     * @param frameRate set the frame rate of the output video.
     * @param bitRate set the bit rate of the output video.
     * @param sourceType set the layer for recording.
     *  See {@link ScreenControlManager#VideoSourceType#VIDEO_VPP0_ONLY},
            {@link creenControlManager#VideoSourceType#VIDEO_VPP0_OSD},
     *   and {@link creenControlManager#VideoSourceType#OSD_VPP0_ONLY}..
     * @throws IllegalArgumentException the parameter is unacceptable.
     */
    public void setVideoParameter(int left, int top, int right, int bottom, int width, int height,
                                    int frameRate, int bitRate, int sourceType) throws IllegalArgumentException {
        Log.d(TAG, "setVideoParameter left:" + left + ",top:"+ top + ",right:" + right + ",bottom:" + bottom + ",width:" + width + ",height:"+ height +
                ",frameRate:" + frameRate  + ",sourceType:" + sourceType + ",bitRate:" + bitRate);
        synchronized (mLock) {
            if (left <=  ERROR_BAD_VALUE || top <= ERROR_BAD_VALUE || right <= ERROR_BAD_VALUE  || bottom <= ERROR_BAD_VALUE) {
                throw new IllegalArgumentException ("the crop video area is not legal left = " + left + ",top = " +
                                                    top + ",right = " + right + ",bottom = " + bottom);
            }
            mVideoLeft = left;
            mVideoTop = top;
            mVideoRight = right;
            mVideoBottom = bottom;
            if (width <=  ERROR_BAD_VALUE || height <= ERROR_BAD_VALUE || frameRate <= ERROR_BAD_VALUE  || bitRate <= ERROR_BAD_VALUE) {
                throw new IllegalArgumentException ("the parmater of video is not legal width = " + width + ",height = " +
                                                    height + ",frameRate = " + frameRate + ",bitRate = " + bitRate);
            }
            mVideoWidth = width;
            mVideoHeight = height;
            mVideoFrameRate = frameRate;
            mVideoBitRate = bitRate;
            if (sourceType < ScreenControlManager.VideoSourceType.VIDEO_VPP0_ONLY ||
                    sourceType > ScreenControlManager.VideoSourceType.OSD_VPP0_ONLY) {
                throw new IllegalArgumentException ("don't support the video source type : " + sourceType);
            }
            mAvcFrameType = sourceType;
        }

    }
    /**
     * set the AudioRecord and the audio encoder parameter.
     * @param source the recording source.
     *   See {@link MediaRecorder.AudioSource} for the recording source definitions.
     * @param format  The desired format of the output data (encoder). Passing {@code null}
     *               as {@code format} is equivalent to passing an
     *                   {@link MediaFormat#MediaFormat an empty mediaformat}.
     * these parameter must be set.
     *{@link MediaFormat#KEY_MIME}  The type of the format
     *           Pass {@link MediaFormat#MIMETYPE_AUDIO_AAC}.
     *{@link MediaFormat#KEY_SAMPLE_RATE} the sample rate of an audio format
     *{@link MediaFormat#KEY_CHANNEL_COUNT} the number of channels in an audio format
     *   See {@link AudioFormat#CHANNEL_IN_MONO} and
     *   {@link AudioFormat#CHANNEL_IN_STEREO}.  {@link AudioFormat#CHANNEL_IN_MONO} is guaranteed
     *{@link MediaFormat#KEY_BIT_RATE} the average bitrate in bits/sec.
     *{@link MediaFormat#KEY_AAC_PROFILE} the AAC profile to be used (AAC audio formats only)
     * See {@link MediaCodecInfo#CodecProfileLevel}
     *{@link MediaFormat#KEY_PRIORITY} the desired codec priority.
     *   0: realtime priority - meaning that the codec shall support the given performance configuration
        (e.g. framerate) at realtime. This should only be used by media playback, capture,
        and possibly by realtime communication scenarios if best effort performance is not suitable.
     *   1: non-realtime priority (best effort).
     * @param audioFormat the format in which the audio data is to be returned.
     *   See {@link AudioFormat#ENCODING_PCM_8BIT}, {@link AudioFormat#ENCODING_PCM_16BIT},
     *   and {@link AudioFormat#ENCODING_PCM_FLOAT}.
     * @throws IllegalArgumentException the parameter is unacceptable.
     */

    public void setAudioParameter(int source, MediaFormat format, int audioFormat) throws IllegalArgumentException {
        synchronized (mLock) {
            Log.d(TAG, "setAudioParameter source = " + source + ",audioFormat = "+audioFormat);
            if (format == null) {
                throw new IllegalArgumentException ("the format is null");
            }
            mAudioMine = format.getString(MediaFormat.KEY_MIME,"default");
            if (mAudioMine == null || mAudioMine.equals("default")) {
                throw new IllegalArgumentException ("setAudioParameter() don't find the parameter :" + MediaFormat.KEY_MIME);
            }
            mAudioSampleRate = format.getInteger(MediaFormat.KEY_SAMPLE_RATE,ERROR_BAD_VALUE);
            if (mAudioSampleRate == ERROR_BAD_VALUE ) {
                throw new IllegalArgumentException ("setAudioParameter() don't find the parameter :" + MediaFormat.KEY_SAMPLE_RATE);
            }
            mAudioChannelCount = format.getInteger(MediaFormat.KEY_CHANNEL_COUNT,ERROR_BAD_VALUE);
            if (mAudioChannelCount == ERROR_BAD_VALUE ) {
                throw new IllegalArgumentException ("setAudioParameter() don't find the parameter :" + MediaFormat.KEY_CHANNEL_COUNT);
            }

            mAudioBitRate = format.getInteger(MediaFormat.KEY_BIT_RATE,ERROR_BAD_VALUE);
            if (mAudioBitRate == ERROR_BAD_VALUE ) {
                throw new IllegalArgumentException ("setAudioParameter() don't find the parameter :" + MediaFormat.KEY_BIT_RATE);
            }
            if (mAudioMine.equals(MediaFormat.MIMETYPE_AUDIO_AAC)) {
                mAudioAccProfile = format.getInteger(MediaFormat.KEY_AAC_PROFILE,ERROR_BAD_VALUE);
                if (mAudioAccProfile == ERROR_BAD_VALUE ) {
                    throw new IllegalArgumentException ("setAudioParameter() don't find the parameter :" + MediaFormat.KEY_AAC_PROFILE);
                }
                mAudioPriority = format.getInteger(MediaFormat.KEY_PRIORITY,ERROR_BAD_VALUE);
                if (mAudioPriority == ERROR_BAD_VALUE ) {
                    throw new IllegalArgumentException ("setAudioParameter() don't find the parameter :" + MediaFormat.KEY_PRIORITY);
                }
            }
            if (audioFormat <= AudioFormat.ENCODING_INVALID) {
                throw new IllegalArgumentException ("the pcm format is not legal audioFormat = " + audioFormat);
            }
            mAudioPcmFormat = audioFormat;
            if (source < MediaRecorder.AudioSource.DEFAULT) {
                throw new IllegalArgumentException ("the audio source is not legal source = " + source);
            }
            mAudioSourceType = source;
        }

    }
     /**
       * After successfully set the video and audio parameter, call {@code start}.
       * @throws IllegalStateException if not call {@code setAudioParameter}
       *    or {@code setVideoParameter}.
       * @throws RemoteExceptionupon create mediamuxer error ,and start the video track or
       * the audio track record fail.
       */
    public void start() throws IllegalStateException , RemoteException{
        synchronized (mLock) {
            Log.d(TAG, "start begin");
            if (mAudioSourceType == ERROR_BAD_VALUE || mAvcFrameType == ERROR_BAD_VALUE) {
                throw new IllegalStateException("the video parmater or audio parmater dont't set to screenrecorder");
            }
            try {
                mMuxer = new MediaMuxer(mOutputFileName, OutputFormat.MUXER_OUTPUT_MPEG_4);
            }catch (IOException e) {
                throw new RemoteException("the media muxer create error  the file name is " + mOutputFileName);
            }
            mStarted = true;
            mMuxerStrated = false;
            mAudio = new AudioSource();
            mAudioCallBack = new AudioCallBack();
            mAudio.setListener(mAudioCallBack);

            if (AudioSource.RECORD_SUCCESS !=mAudio.startRecord(mAudioMine,mAudioSampleRate,mAudioChannelCount,
                                        mAudioBitRate,mAudioAccProfile,mAudioPriority,mAudioSourceType,mAudioPcmFormat)) {
                throw new RemoteException("the audio source start record fail !! ");
            }
            mVideo = new VideoSource();
            mVideoCallBack = new VideoCallBack();
            mVideo.setListener(mVideoCallBack);
            if (VideoSource.RECORD_SUCCESS != mVideo.startRecord(mVideoLeft,mVideoTop,mVideoRight,mVideoBottom,
                                mVideoWidth,mVideoHeight,mVideoFrameRate,mVideoBitRate,mAvcFrameType)) {
                throw new RemoteException("the video source start record fail !! ");
            }
            if (mRecordTime > 0 )
                startCheckRecordTime();
        }
    }
    public void stop() {
        synchronized (mLock) {
            Log.d(TAG, "stop begin");
            mStarted = false;
            stopVideoRecord();
            stopAudioRecord();
            if (mMuxer != null) {
                mMuxer.stop();
                mMuxer.release();
                mMuxer =null;
            }
            Log.d(TAG, "stop finfish");
        }

    }
    private void stopAudioRecord() {
        if (mAudio == null)
            return;
        mAudio.stopRecord();
        mAudio = null;
    }
    private void stopVideoRecord() {
        if (mVideo == null)
            return;
        mVideo.stop();
        mVideo = null;
    }
    private void startCheckRecordTime() {
        new Thread() {
        @Override
        public void run(){
            while (mStarted) {
                if ((mLastVideoTimeUs - mFirstVideoTimeUs) > (mRecordTime * 1000 * 1000) && mVideo != null) {
                    Log.d(TAG, "check the video time is enough need stop mLastVideoTimeUs " + mLastVideoTimeUs + ",mFirstVideoTimeUs =" + mFirstVideoTimeUs);
                    stopVideoRecord();
                }
                if ((mLastAudioTimeUs - mFirstAudioTimeUs) > (mRecordTime * 1000 * 1000) && mAudio != null) {
                    Log.d(TAG, "check the audio time is enough need stop mLastAudioTimeUs " + mLastAudioTimeUs + ",mFirstAudioTimeUs =" + mFirstAudioTimeUs);
                    stopAudioRecord();
                }
                if (mAudio == null && mVideo == null) {
                    Log.d(TAG, "all tracks have been stopped,need really stop muxer");
                    mMuxer.stop();
                    mMuxer.release();
                    mMuxer =null;
                    return;
                }
            }
        }
      }.start();

    }


}