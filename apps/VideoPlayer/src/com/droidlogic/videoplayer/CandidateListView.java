package com.droidlogic.videoplayer;

import android.content.Context;
import android.content.SharedPreferences;
import android.util.AttributeSet;
import android.util.Log;
import android.widget.ArrayAdapter;
import android.widget.ListPopupWindow;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

public class CandidateListView extends ListPopupWindow {
    private static final String TAG = "CandidateListView";
    private SharedPreferences mPreference;

    public static final String KEY_CANDIDATES_URLS = "key_candidates_urls";
    private Context mContext;
    private ArrayAdapter<String> mAdapter;

    public CandidateListView(Context context) {
        this(context, null);
    }

    public CandidateListView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public CandidateListView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);

        init(context);
    }

    private void init(Context context) {
        mContext = context;
        mAdapter = new ArrayAdapter<>(mContext, R.layout.candidate_list_item, new ArrayList<String>());
        mAdapter.setNotifyOnChange(true);
    }

    public void setSize(int width, int height) {
        setWidth(width);
        setHeight(height);
    }

    public void setPreference(SharedPreferences preference) {
        mPreference = preference;
    }

    public String getUrl(int index) {
        return mAdapter.getItem(index);
    }

    public void setVisible(boolean v) {
        Log.d(TAG ,"setVisible, v=" + v);
        if (v) {
            if (!isShowing()) {
                Set<String> candidateList = getCandidateList();
                if (candidateList == null)
                    return;

                Log.d(TAG, "candidateList= " + candidateList.toString());

                mAdapter.clear();
                mAdapter.addAll(Arrays.asList(candidateList.toArray(new String[]{})));
                setAdapter(mAdapter);
                show();
            }
        } else {
            if (isShowing()) {
                Log.d(TAG, "dismiss");
                setAdapter(null);
                mAdapter.clear();
                dismiss();
            }
        }
    }

    private Set<String> getCandidateList() {
        Set<String> candidateSet = null;
        if (mPreference != null) {
            candidateSet = mPreference.getStringSet(KEY_CANDIDATES_URLS, new HashSet<String>());
        }
        if (candidateSet == null || candidateSet.isEmpty()) {
            return null;
        }

        return candidateSet;
    }
}
