package com.droidlogic.multidisplay.multicontroller;

import android.content.Context;
import android.graphics.Color;
import android.os.RemoteException;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.BaseAdapter;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.Switch;
import android.widget.TextView;

import com.droidlogic.displaycontrollerservice.IMirrorDisplayInterface;

import java.util.List;

public class MirroredDeviceAdapter extends BaseAdapter {
    private static final String TAG = "MM-C";
    private List<MirroredData> mData;
    private final LayoutInflater mInflater;
    private final Context mContext;
    private int mCurrentDisplayId;
    private MirrorDisplayWrapper mController = null;
    private OnCheckedChangeListener mListener;

    public MirroredDeviceAdapter(Context context) {
        mContext = context;
        mInflater = LayoutInflater.from(context);
    }

    public void setOnItemListener(OnCheckedChangeListener l) {
        mListener = l;
    }

    public void setController(MirrorDisplayWrapper controller) {
        mController = controller;
    }

    public void setData(List<MirroredData> data) {
        mData = data;
    }
    /*public void getData() {
        return mData;
    }*/

    @Override
    public int getCount() {
        return mData.size();
    }
    public void setDisplayId(int displayId) {
        Log.d(TAG,"setDisplayId"+displayId);
        mCurrentDisplayId = displayId;
    }

    @Override
    public Object getItem(int position) {
        return mData.get(position);
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        ViewHolder holder = null;
        if (convertView == null) {
            holder = new ViewHolder();
            convertView = mInflater.inflate(R.layout.list_item, null);
            holder.dataTv = convertView.findViewById(R.id.display_device);
            holder.dataSwitch = convertView.findViewById(R.id.display_device_switch);
            convertView.setTag(holder);
        } else {
            holder = (ViewHolder) convertView.getTag();
        }
        final TextView tvView = holder.dataTv;
        holder.dataSwitch.setOnCheckedChangeListener(new OnCheckedChangeListener() {

            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                MirroredData data = (MirroredData) getItem(position);
                if (isChecked) {
                    Log.d(TAG, "startMirror"+mCurrentDisplayId+""+data.getDisplayId());
                    mController.startMirror(mCurrentDisplayId, data.getDisplayId());
                    tvView.setText(data.getName(true));
                } else {
                    Log.d(TAG, "stopMirror");
                    mController.stopMirror(data.getDisplayId());
                    tvView.setText(data.getName(false));
                }
            }
        });
        MirroredData data = (MirroredData) getItem(position);
        boolean isMirrored = mController != null && mController.isMirrored(data.getDisplayId());
        boolean enabled = true;
        if (!isMirrored && mController != null && mController.isMirroring(data.getDisplayId())) {
            enabled = false;
        }
        Log.d(TAG,"item "+data.getDisplayId()+" isMirrored"+isMirrored+" enabled"+enabled+" mCurrentDisplayId:"+mCurrentDisplayId);
        holder.dataSwitch.setEnabled(enabled);
        holder.dataSwitch.setChecked(isMirrored);
        holder.dataTv.setText(data.getName(isMirrored));
        return convertView;
    }


    private class ViewHolder {
        TextView dataTv;
        Switch dataSwitch;
    }
}
