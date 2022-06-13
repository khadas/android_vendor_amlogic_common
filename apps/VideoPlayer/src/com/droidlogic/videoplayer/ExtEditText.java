package com.droidlogic.videoplayer;

import android.annotation.TargetApi;
import android.content.Context;
import android.graphics.Color;
import android.graphics.Rect;
import android.graphics.drawable.ColorDrawable;
import android.os.Build;
import android.os.Handler;
import android.text.Editable;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.EditText;

public class ExtEditText extends EditText implements AdapterView.OnItemClickListener {
    private static final String TAG = "ExtEditText";
    private static final boolean DEBUG = true;

    private static final String mDpadChars = "0123456789";
    private int mLocation = 0;
    private CandidateListView mCandidateListView;

    public ExtEditText(Context context) {
        super(context);
    }

    public ExtEditText(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public ExtEditText(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    public ExtEditText(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (DEBUG)
            Log.d(TAG, String.format("keyCode=%d, event=%s", keyCode, event.toString()));

        updateLocation(keyCode);

        Editable editableText = getEditableText();
        String charString = String.valueOf((char) event.getUnicodeChar());
        if (mDpadChars.contains(charString)) {
            editableText.insert(mLocation, charString);
            return false;
        }

        //Processing start/end.
        if (keyCode == KeyEvent.KEYCODE_DPAD_CENTER || keyCode == KeyEvent.KEYCODE_DPAD_DOWN) {
            moveCursorToEnd();
        } else if (keyCode == KeyEvent.KEYCODE_DPAD_UP) {
            moveCursorToStart();
        }

        return super.onKeyDown(keyCode, event);
    }

    private void moveCursorTo(int location) {
        if (mLocation != location) {
            mLocation = location;
            setSelection(mLocation);
        }
    }

    private void moveCursorToStart() {
        moveCursorTo(0);
    }

    private void moveCursorToEnd() {
        moveCursorTo(getEditableText().length());
    }

    private void updateLocation(int keyCode) {
        int location = mLocation;
        if (keyCode == KeyEvent.KEYCODE_DPAD_LEFT) {
            location = --mLocation;
        } else if (keyCode == KeyEvent.KEYCODE_DPAD_RIGHT) {
            location = ++mLocation;
        }

        mLocation = clamp(location, 0, getText().length());
    }

    @Override
    protected void onTextChanged(CharSequence text, int start, int lengthBefore, int lengthAfter) {
        if (DEBUG)
            Log.d(TAG, String.format("onTextChanged start," +
                            "mLocation=%d, text=%s, start= %d,lengthBefore= %d, lengthAfter= %d",
                    mLocation, text, start, lengthBefore, lengthAfter));

        moveCursorTo(start + lengthAfter);
        if ((lengthBefore > lengthAfter) && text.length() <= 0) {
            showCandidateList(true);
        }

        if (DEBUG)
            Log.d(TAG, "onTextChanged end, mLocation= " + mLocation);
    }

    @Override
    protected void onFocusChanged(boolean focused, int direction, Rect previouslyFocusedRect) {
        super.onFocusChanged(focused, direction, previouslyFocusedRect);

        if (DEBUG)
            Log.d(TAG, "onFocusChanged, focused= " + focused);

        if (!focused) {
            moveCursorToEnd();
            showCandidateList(false);
        } else {
            if (getText().toString().isEmpty()) {
                showCandidateList(true);
            }
        }
    }

    private void showCandidateList(final boolean show) {
        new Handler().post(new Runnable() {
            @Override
            public void run() {
                if (mCandidateListView != null) {
                    mCandidateListView.setVisible(show);
                }
            }
        });
    }

    private int clamp(int val, int min, int max) {
        if (min > max) {
            //swap
            int tmp = min;
            min = max;
            max = tmp;
        }

        if (val < min) {
            val = min;
        } else if (val > max) {
            val = max;
        }

        return val;
    }

    @Override
    protected void onDetachedFromWindow() {
        super.onDetachedFromWindow();

        if (mCandidateListView != null) {
            if (mCandidateListView.isShowing()) {
                mCandidateListView.setVisible(false);
            }
            mCandidateListView.setAnchorView(null);
        }
    }

    public void setCandidateListView(CandidateListView candidateListView) {
        mCandidateListView = candidateListView;

        if (mCandidateListView != null) {
            mCandidateListView.setSize(WindowManager.LayoutParams.WRAP_CONTENT,
                    WindowManager.LayoutParams.WRAP_CONTENT);
            mCandidateListView.setModal(true);
            mCandidateListView.setAnchorView(this);
            mCandidateListView.setBackgroundDrawable(new ColorDrawable(Color.DKGRAY));
            mCandidateListView.setDropDownGravity(Gravity.LEFT);
            mCandidateListView.setOnItemClickListener(this);
        }
    }

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        String url = mCandidateListView.getUrl(position);
        if (url != null) {
            setText(url);
        }

        mCandidateListView.setVisible(false);
    }
}
