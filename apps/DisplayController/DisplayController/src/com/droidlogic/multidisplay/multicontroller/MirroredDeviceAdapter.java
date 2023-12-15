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
    private final List<MirroredData> mData;
    private final LayoutInflater mInflater;
    private final Context mContext;
    private MirrorDisplayWrapper mController = null;
    private OnCheckedChangeListener mListener;

    public MirroredDeviceAdapter(Context context, List<MirroredData> data) {
        mContext = context;
        mData = data;
        mInflater = LayoutInflater.from(context);
    }

    public void setOnItemListener(OnCheckedChangeListener l) {
        mListener = l;
    }

    public void setController(MirrorDisplayWrapper controller) {
        mController = controller;
    }

    @Override
    public int getCount() {
        return mData.size();
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

        holder.dataSwitch.setOnCheckedChangeListener(new OnCheckedChangeListener() {

            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                MirroredData data = (MirroredData) getItem(position);
                if (isChecked) {
                    Log.d(TAG, "startMirror");
                    mController.startMirror(mContext.getDisplay().getDisplayId(), data.getDisplayId());
                } else {
                    Log.d(TAG, "stopMirror");
                    mController.stopMirror(data.getDisplayId());
                }
            }
        });
        MirroredData data = (MirroredData) getItem(position);
        boolean isMirrored = mController != null && mController.isMirrored(data.getDisplayId());
        Log.d(TAG,"item "+data.getDisplayId()+" isMirrored"+isMirrored);
        holder.dataSwitch.setChecked(isMirrored);
        holder.dataTv.setText(data.getName());
        return convertView;
    }


    private class ViewHolder {
        TextView dataTv;
        Switch dataSwitch;
    }
}
