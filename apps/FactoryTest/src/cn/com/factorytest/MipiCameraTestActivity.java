package cn.com.factorytest;

import java.util.HashMap;

import android.widget.RelativeLayout;

import java.io.File;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.media.AudioManager;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.os.StatFs;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.Window;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.TextView;

import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.ViewGroup;
import android.hardware.Camera;
import android.view.MotionEvent;
import android.provider.Settings;

public class MipiCameraTestActivity extends Activity implements SurfaceHolder.Callback {
    private static final String TAG = "MIPI";
    private Button success, fail;
    private SurfaceView mSurfaceview = null;
    private SurfaceHolder mSurfaceHolder = null;
    private Camera mCamera = null;
    private Context mContext;

    @Override
    protected void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);
        setContentView(R.layout.camera_mipi);
        mContext = this;
        initSurfaceView();

        success = (Button) findViewById(R.id.btn_success);
        success.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Settings.System.putInt(mContext.getContentResolver(), "Khadas_mipi_camera_test", 1);
                finish();
            }
        });

        fail = (Button) findViewById(R.id.btn_fail);
        fail.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Settings.System.putInt(mContext.getContentResolver(), "Khadas_mipi_camera_test", 0);
                finish();
            }
        });
    }

    private void initSurfaceView() {
        Log.d(TAG, "CameraTest initSurfaceView");
        mSurfaceview = (SurfaceView) findViewById(R.id.mSurfaceView_mipi);

        mSurfaceHolder = mSurfaceview.getHolder();
        mSurfaceHolder.addCallback(MipiCameraTestActivity.this);
        //mSurfaceHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
        mSurfaceview.getHolder().setFixedSize(800, 480);
        mSurfaceview.getHolder().setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holer) {
        try {
            Log.d(TAG, "CameraTest SurfaceHolder.Callback surface Created");

            mCamera = Camera.open(0);
            mCamera.setPreviewDisplay(mSurfaceHolder);

            initCamera();
        } catch (Exception ex) {
            if (null != mCamera) {
                mCamera.release();
                mCamera = null;
            }
            Log.d(TAG, ex.getMessage());
        }
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        Log.d(TAG, "CameraTest SurfaceHolder.Callback Surface Changed");
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder surfaceHolder) {
        Log.d(TAG, "CameraTest SurfaceHolder.Callback Surface Destroyed");
        if (null != mCamera) {
            mCamera.setPreviewCallback(null);
            mCamera.stopPreview();
            mCamera.release();
            mCamera = null;
        }
    }

    private void initCamera() {
        if (mCamera != null) {
            try {
                Camera.Parameters parameters = mCamera.getParameters();
                mCamera.setParameters(parameters);
                mCamera.startPreview();
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }


    @Override
    public void onDestroy() {
        super.onDestroy();
        Log.d(TAG, "CameraTest onDestroy");
    }

    public boolean dispatchKeyEvent(KeyEvent event) {
        if (event.getKeyCode() == KeyEvent.KEYCODE_BACK) {
            return false;
        }
        return super.dispatchKeyEvent(event);
    }
}