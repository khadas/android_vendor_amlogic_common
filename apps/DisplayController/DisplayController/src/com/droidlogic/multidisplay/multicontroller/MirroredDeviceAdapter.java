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
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.Switch;
import android.widget.TextView;

import com.droidlogic.displaycontrollerservice.IMirrorDisplayInterface;

import java.util.List;

public class MirroredDeviceAdapter extends BaseAdapter {
    private static final String TAG = "MM-C";
    private final LayoutInflater mInflater;
    private final Context mContext;
    private List<MirroredData> mData;
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
        Log.d(TAG, "setDisplayId" + displayId);
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
            holder.mCheckBox = convertView.findViewById(R.id.checkBox);
            convertView.setTag(holder);
        } else {
            holder = (ViewHolder) convertView.getTag();
        }
        final TextView tvView = holder.dataTv;
        final CheckBox checkBox = holder.mCheckBox;
        holder.dataSwitch.setOnCheckedChangeListener(new OnCheckedChangeListener() {

            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                MirroredData data = (MirroredData) getItem(position);
                if (isChecked) {
                    if (!mController.isMirrored(data.getDisplayId())) {
                        mController.startMirror(mCurrentDisplayId, data.getDisplayId(), checkBox.isChecked());
                        Log.d(TAG, mCurrentDisplayId + " startMirror " + data.getDisplayId());
                        tvView.setText(data.getName(mController.getMirroredId(data.getDisplayId())));
                        checkBox.setEnabled(false);
                    }
                } else {
                    if (mController.isMirrored(data.getDisplayId())) {
                        Log.d(TAG, "stopMirror" + mController.getMirroredId(data.getDisplayId()) + "?" + data.getDisplayId());
                        mController.stopMirror(data.getDisplayId());
                        tvView.setText(data.getName(mController.getMirroredId(data.getDisplayId())));
                        checkBox.setEnabled(true);
                    }
                }
            }
        });
        MirroredData data = (MirroredData) getItem(position);
        boolean isMirrored = mController != null && mController.isMirrored(data.getDisplayId());
        boolean enabled = isMirrored || mController == null || !mController.isMirroring(data.getDisplayId());
        Log.d(TAG, "item " + data.getDisplayId() + " isMirrored" + isMirrored + " enabled" + enabled + " mCurrentDisplayId:" + mCurrentDisplayId);
        holder.dataSwitch.setEnabled(enabled);
        Log.d(TAG, "item " + isMirrored + " checked " + holder.dataSwitch.isChecked());
        if (isMirrored != holder.dataSwitch.isChecked()) {
            holder.dataSwitch.setChecked(isMirrored);
            holder.mCheckBox.setChecked(mController.isControlled(data.getDisplayId()));
        }
        if (mController != null) {
            Log.d(TAG, "item " + data.getDisplayId() + ":" + mController.getMirroredId(data.getDisplayId()));
        }
        int id = mController != null ? mController.getMirroredId(data.getDisplayId()) : -1;
        if (isMirrored) {
            holder.dataTv.setText(data.getName(id));
        } else {
            holder.dataTv.setText(data.getName(-1));
        }

        convertView.invalidate();
        return convertView;
    }


    private class ViewHolder {
        TextView dataTv;
        Switch dataSwitch;
        CheckBox mCheckBox;
    }
}
