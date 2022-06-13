/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.tvinput.widget;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.view.LayoutInflater;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.TextView;
import android.graphics.drawable.Drawable;
import android.util.Log;
import android.view.animation.Animation;
import android.view.animation.LinearInterpolator;
import android.view.animation.TranslateAnimation;
import android.widget.HorizontalScrollView;
import android.graphics.Color;

public abstract class DroidLogicOverlayView extends FrameLayout {
    private static final String TAG = "DroidLogicOverlayView";
    private static final float SCOLL_V = 0.2f;

    protected ImageView mImageView;
    protected ImageView mTuningImageView;
    protected TextView mTextView;
    protected View mSubtitleView;
    protected TextView mEasTextView;
    protected TextView mTeletextNumber;
    protected ImageView mDoblyVisionImageView;
    protected HorizontalScrollView mEasScrollView;
    protected TranslateAnimation mRigthToLeftAnim;

    public DroidLogicOverlayView(Context context) {
        this(context, null);
    }

    public DroidLogicOverlayView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public DroidLogicOverlayView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    protected abstract void initSubView();

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        initSubView();
    }

    public void setImage(int resId) {
        mImageView.setImageResource(resId);
    }

    public void setImage(Drawable draw) {
        mImageView.setImageDrawable(draw);
    }

    public void setImageVisibility(boolean visible) {
        mImageView.setVisibility(visible ? VISIBLE : GONE);
    }

    public void setTuningImageVisibility(boolean visible) {
        if (mTuningImageView != null) {
            mTuningImageView.setVisibility(visible ? VISIBLE : GONE);
        }
    }

    public void setDoblyVisionVisibility(boolean visible) {
        if (mDoblyVisionImageView != null) {
            mDoblyVisionImageView.setVisibility(visible ? VISIBLE : GONE);
        }
    }

    public boolean isDoblyVisionVisible() {
        boolean result = false;
        if (mDoblyVisionImageView != null) {
            result = mDoblyVisionImageView.getVisibility() == VISIBLE;
        }
        return result;
    }

    public void setText(int resId) {
        mTextView.setText(resId);
    }

    public void setTextForEas(String text){
        mEasTextView.setText(text);
        mEasTextView.post(new Runnable() {
                @Override
                public void run() {
                    mEasScrollView.setBackgroundColor(Color.BLACK);
                    if (mEasScrollView.getWidth() < mEasTextView.getWidth()) {
                        Log.i(TAG,"scroll");
                        startEasAnimation(true);
                    }else {
                        Log.i(TAG,"not scroll");
                        startEasAnimation(false);
                    }
                }
            });
    }

    private void startEasAnimation(boolean isScroll) {
        if (isScroll)
            mRigthToLeftAnim = new TranslateAnimation(mEasScrollView.getWidth(), -mEasTextView.getWidth(), 0, 0);
        else
            mRigthToLeftAnim = new TranslateAnimation((mEasScrollView.getWidth()-mEasTextView.getWidth())/2, (mEasScrollView.getWidth()-mEasTextView.getWidth())/2, 0, 0);

        mRigthToLeftAnim.setRepeatCount(Animation.INFINITE);
        mRigthToLeftAnim.setInterpolator(new LinearInterpolator());
        mRigthToLeftAnim.setDuration((long) ((mEasScrollView.getWidth() + mEasTextView.getWidth()) / SCOLL_V));
        mEasTextView.startAnimation(mRigthToLeftAnim);
    }

    public boolean isEasTextShown() {
        return mEasScrollView.isShown();
    }

    public void setEasTextVisibility(boolean visible) {
        if (visible) {
            mEasTextView.setVisibility(VISIBLE);
            mEasScrollView.setVisibility(VISIBLE);
        }else {
            mEasScrollView.setBackgroundColor(Color.TRANSPARENT);
            mEasTextView.setVisibility(GONE);
            mEasScrollView.setVisibility(GONE);
        }
    }

    public void setTextForTeletextNumber(CharSequence text){
        if (mTeletextNumber != null) {
            mTeletextNumber.setText(text);
        }
    }

    public void setTextVisibility(boolean visible) {
        mTextView.setVisibility(visible ? VISIBLE : GONE);
    }

    public void setTeleTextNumberVisibility(boolean visible) {
        if (mTeletextNumber != null) {
            mTeletextNumber.setVisibility(visible ? VISIBLE : GONE);
        }
    }

    public void setSubtitleVisibility(boolean visible) {
        if (mSubtitleView != null) {
            mSubtitleView.setVisibility(visible ? VISIBLE : GONE);
        }
    }

    public View getSubtitleView() {
        return mSubtitleView;
    }

    public void releaseResource() {
        mImageView = null;
        mTextView = null;
        mSubtitleView = null;
    }

}
