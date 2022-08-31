package com.droidlogic.NativeRenderSubTest;

import android.media.AudioManager;
import android.media.MediaPlayer;
import android.net.Uri;
import android.os.Bundle;


import android.app.Activity;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.Surface;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.os.*;
import android.graphics.PixelFormat;

import com.droidlogic.app.MediaPlayerExt;

import android.media.MediaPlayer;
import android.media.MediaPlayer.OnBufferingUpdateListener;
import android.media.MediaPlayer.OnCompletionListener;
import android.media.MediaPlayer.OnPreparedListener;
import android.media.MediaPlayer.OnVideoSizeChangedListener;

import java.io.UnsupportedEncodingException;
import java.lang.Thread.UncaughtExceptionHandler;

public class PlayerActivity extends Activity implements
        OnBufferingUpdateListener, OnCompletionListener,
        OnPreparedListener, OnVideoSizeChangedListener, SurfaceHolder.Callback {
    private static final String TAG = "PlayerActivity";
    private String mFilePath;

    private MediaPlayerExt mMediaPlayer;
    MediaPlayer.TrackInfo[] mTrackInfo;

    private SurfaceView mVideoView;
    private SurfaceHolder mHolder;
    private int mVideoWidth;
    private int mVideoHeight;
    private SubtitleAPI mApiWrapper;

    private Button mOnBtn;
    private Button mOffBtn;
    private SurfaceHolder mCreatHolder;


    private SurfaceView mSubtitleView;
    private SurfaceHolder.Callback mSubtitleSurfaceCallback = new SurfaceHolder.Callback() {
        @Override
        public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
            // This just makes sure the holder is still the one we expect.
        }

        @Override
        public void surfaceCreated(SurfaceHolder holder) {
            Log.d(TAG, "here", new Throwable());
            mCreatHolder = holder;
            nStartNativeRender(holder.getSurface());
        }

        @Override
        public void surfaceDestroyed (SurfaceHolder holder) {
            nStopNativeRender();
        }
    };

    private native void nStartNativeRender(Surface surface);
    private native void nStopNativeRender();

    static {
        try {
            System.loadLibrary("subcontrol_jni");
        } catch (UnsatisfiedLinkError e) {
            Log.d(TAG, "Error try next!", e);
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_player);
        mFilePath = getIntent().getStringExtra("filePath");

        // init video view!
        mVideoView = (SurfaceView) findViewById(R.id.videoView);
        mHolder = mVideoView.getHolder();
        mHolder.addCallback(this);
        mHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);

        mSubtitleView = (SurfaceView) findViewById(R.id.subtitle);
        mSubtitleView.getHolder().addCallback(mSubtitleSurfaceCallback);
        mSubtitleView.getHolder().setFormat(PixelFormat.RGBA_8888);
        mSubtitleView.setZOrderMediaOverlay(true);


        mOnBtn = (Button) findViewById(R.id.subtitleOn);
        mOffBtn = (Button) findViewById(R.id.subtitleOff);
        mOnBtn.setOnClickListener (new Button.OnClickListener() {
            public void onClick (View v) {
                Log.d (TAG, "mOnBtn onClick");
                nStartNativeRender(mCreatHolder.getSurface());
            }
        });
        mOffBtn.setOnClickListener (new Button.OnClickListener() {
            public void onClick (View v) {
                Log.d (TAG, "mOffBtn onClick");
                nStopNativeRender();
            }
        });
        mOnBtn.requestFocus();

        mMediaPlayer = new MediaPlayerExt();

    }

    @Override
    protected void onStop() {
        super.onStop();
         try {
            mMediaPlayer.stop();
            mMediaPlayer.reset();
        } catch (Exception ex) {
            Log.d(TAG, "Unable to open  ex:", ex);
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
    }


    private void playVideo() {
        try {
            // Create a new media player and set the listeners
            mMediaPlayer.setDataSource (this, Uri.parse(mFilePath), null);
            mMediaPlayer.setDisplay(mHolder);

            mMediaPlayer.setUseLocalExtractor(mMediaPlayer);
            mMediaPlayer.prepare();
            mMediaPlayer.setOnBufferingUpdateListener(this);
            mMediaPlayer.setOnCompletionListener(this);
            mMediaPlayer.setOnPreparedListener(this);
            mMediaPlayer.setOnVideoSizeChangedListener(this);
            //mMediaPlayer.setAudioStreamType(AudioManager.STREAM_MUSIC);
        } catch (Exception e) {
            Log.e(TAG, "error: " + e.getMessage(), e);
        }
    }

    @Override
    public void onBufferingUpdate(MediaPlayer mp, int percent) {
        Log.d(TAG, "onBufferingUpdate called");
    }

    @Override
    public void onCompletion(MediaPlayer mp) {
        Log.d(TAG, "onCompletion called");
        mMediaPlayer.stop();
        mMediaPlayer.reset();
        Log.d(TAG, "onCompletion called stopped");

        try {
            mMediaPlayer.setDataSource (this, Uri.parse(mFilePath), null);
            mMediaPlayer.setUseLocalExtractor(mMediaPlayer);
            mMediaPlayer.prepare();
        } catch (Exception ex) {
            Log.d(TAG, "Unable to open  ex:", ex);
        }
    }

    @Override
    public void onPrepared(MediaPlayer mp) {
        Log.d(TAG, "onPrepared called mVideoView=" + mVideoView);
        mTrackInfo = mp.getTrackInfo();
        if (mTrackInfo != null) {
            for (int j = 0; j < mTrackInfo.length; j++) {
                if (mTrackInfo[j] != null) {
                    int trackType = mTrackInfo[j].getTrackType();
                    Log.d(TAG, "get Track, track info="+mTrackInfo[j]);
                    if (trackType == MediaPlayer.TrackInfo.MEDIA_TRACK_TYPE_SUBTITLE || trackType == MediaPlayer.TrackInfo.MEDIA_TRACK_TYPE_TIMEDTEXT) {
                        Log.d(TAG, "get  subtitle Track, track info="+mTrackInfo[j]);
                    }
                }

            }
        }

        mHolder.setFixedSize(mVideoWidth, mVideoHeight);
        mMediaPlayer.start();
        mSubtitleView.getHolder().setFixedSize(mVideoWidth, mVideoHeight);

    }

    @Override
    public void onVideoSizeChanged(MediaPlayer mp, int width, int height) {
        Log.d(TAG, "onVideoSizeChanged called width="+width +", height="+height);
        mVideoWidth = width;
        mVideoHeight = height;
        if (width != 0 && width != 0) {
            mHolder.setFixedSize(mVideoWidth, mVideoHeight);
            mSubtitleView.getHolder().setFixedSize(mVideoWidth, mVideoHeight);
            mVideoView.requestLayout();
            mSubtitleView.requestLayout();
        }
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        Log.d(TAG, "surfaceCreated called");
        playVideo();
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        Log.d(TAG, "surfaceChanged called");
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        Log.d(TAG, "surfaceDestroyed called");
        // StopVideo
    }
}
