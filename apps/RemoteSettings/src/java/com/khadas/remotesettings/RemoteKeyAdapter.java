package com.khadas.remotesettings;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.CompoundButton;
import android.widget.Switch;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.List;

public class RemoteKeyAdapter extends BaseAdapter {

    private static final String TAG = "IRKeyAdapter";
    private Context mContext;
    private List<RemoteKeyInfo> mList = new ArrayList<>();
    private List<ViewHolder> mViewHolderList = new ArrayList<>();

    public RemoteKeyAdapter(Context context, List<RemoteKeyInfo> list) {
        mContext = context;
        mList = list;
    }

    @Override
    public int getCount() {
        return mList.size();
    }

    @Override
    public Object getItem(int i) {
        return mList.get(i);
    }

    @Override
    public long getItemId(int i) {
        return i;
    }

    @Override
    public View getView(final int i, View view, ViewGroup viewGroup) {
        ViewHolder viewHolder = new ViewHolder();
        view = LayoutInflater.from(mContext).inflate(R.layout.key_item, null);
        viewHolder.mTextView = (TextView) view.findViewById(R.id.item_tv);
        viewHolder.mSw = (Switch) view.findViewById(R.id.item_sw);
        view.setTag(viewHolder);

        viewHolder.mTextView.setText(mList.get(i).getName());
        viewHolder.mSw.setChecked(mList.get(i).isSelect());
        viewHolder.mSw.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if(mOnItemBtnListener != null) {
                    mOnItemBtnListener.onItemSwChanged(i, isChecked);
                }
                mList.get(i).setSelect(isChecked);
            }
        });

        mViewHolderList.add(viewHolder);
        return view;
    }

    public interface onItemSwChangedListener {
        void onItemSwChanged(int i, boolean isChecked);
    }

    private onItemSwChangedListener mOnItemBtnListener;

    public void setonItemSwChangedListener(onItemSwChangedListener onItemBtnListener) {
        this.mOnItemBtnListener = onItemBtnListener;
    }

    class ViewHolder {
        TextView mTextView;
        Switch mSw;
    }

}
