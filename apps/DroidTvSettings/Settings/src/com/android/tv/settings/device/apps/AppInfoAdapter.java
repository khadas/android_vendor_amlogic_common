package com.android.tv.settings.device.apps;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.CompoundButton;
import android.widget.ImageView;
import android.widget.Switch;
import android.widget.TextView;
import com.android.tv.settings.R;

import java.util.ArrayList;
import java.util.List;

public class AppInfoAdapter extends BaseAdapter {
    private Context mContext;
    private List<AppInfo> mAppsList = new ArrayList<>();
    private List<ViewHolder> mViewHolderList = new ArrayList<>();
    private boolean isOnlyOneSelect = false;

    public AppInfoAdapter(Context context, List<AppInfo> list, boolean oneSelect) {
        mContext = context;
        mAppsList = list;
        isOnlyOneSelect = oneSelect;
    }

    @Override
    public int getCount() {
        return mAppsList.size();
    }

    @Override
    public Object getItem(int i) {
        return mAppsList.get(i);
    }

    @Override
    public long getItemId(int i) {
        return i;
    }

    @Override
    public View getView(final int i, View view, ViewGroup viewGroup) {
        ViewHolder viewHolder = new ViewHolder();

        view = LayoutInflater.from(mContext).inflate(R.layout.app_item, null);
        viewHolder.mImageView = (ImageView) view.findViewById(R.id.item_iv);
        viewHolder.mTextView = (TextView) view.findViewById(R.id.item_tv);
        viewHolder.mSw = (Switch) view.findViewById(R.id.item_sw);
        view.setTag(viewHolder);

        viewHolder.mImageView.setImageDrawable(mAppsList.get(i).getIcon());
        viewHolder.mTextView.setText(mAppsList.get(i).getAppName());
        viewHolder.mSw.setChecked(mAppsList.get(i).isSelect());

        viewHolder.mSw.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                mOnCheckedChangeListener.onCheckedChanged(i, isChecked);

                if(isOnlyOneSelect && isChecked) {
                    for(int j = 0; j < mViewHolderList.size(); j++) {
                        if(i != j) {
                            mViewHolderList.get(j).mSw.setChecked(false);
                        }
                    }
                }
                notifyDataSetChanged();
            }
        });

        mViewHolderList.add(viewHolder);
        return view;
    }

    public interface onItemCheckedChangedListener {
        void onCheckedChanged(int i, boolean isChecked);
    }

    private onItemCheckedChangedListener mOnCheckedChangeListener;
    public void setOnItemCheckedChangedListener(onItemCheckedChangedListener onCheckedChangeListener) {
        this.mOnCheckedChangeListener = onCheckedChangeListener;
    }

    class ViewHolder {
        ImageView mImageView;
        TextView mTextView;
        Switch mSw;
    }

}
