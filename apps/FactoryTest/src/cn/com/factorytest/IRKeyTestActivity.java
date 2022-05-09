package cn.com.factorytest;

import java.util.HashMap;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.widget.Button;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.content.Context;
import android.provider.Settings;

public class IRKeyTestActivity extends Activity {

    private static HashMap<Integer, Integer> keyAndIds = new HashMap();
    private RelativeLayout layout;
    private HashMap<Integer, Button> mapView = new HashMap();
    private TextView text1;
    private TextView text2;
    private Button success, fail;
    private Context mContext;

    static {
        keyAndIds.put(Integer.valueOf(KeyEvent.KEYCODE_DPAD_LEFT), Integer.valueOf(R.id.left));
        keyAndIds.put(Integer.valueOf(KeyEvent.KEYCODE_DPAD_DOWN), Integer.valueOf(R.id.down));
        keyAndIds.put(Integer.valueOf(KeyEvent.KEYCODE_DPAD_RIGHT), Integer.valueOf(R.id.right));
        keyAndIds.put(Integer.valueOf(KeyEvent.KEYCODE_DPAD_UP), Integer.valueOf(R.id.up));
        keyAndIds.put(Integer.valueOf(KeyEvent.KEYCODE_BACK), Integer.valueOf(R.id.back));
        keyAndIds.put(Integer.valueOf(KeyEvent.KEYCODE_VOLUME_UP), Integer.valueOf(R.id.va));
        keyAndIds.put(Integer.valueOf(KeyEvent.KEYCODE_VOLUME_DOWN), Integer.valueOf(R.id.vs));
        keyAndIds.put(Integer.valueOf(KeyEvent.KEYCODE_MENU), Integer.valueOf(R.id.menu));
        keyAndIds.put(Integer.valueOf(KeyEvent.KEYCODE_VOLUME_MUTE), Integer.valueOf(R.id.mute));
        keyAndIds.put(Integer.valueOf(KeyEvent.KEYCODE_0), Integer.valueOf(R.id.n0));
        keyAndIds.put(Integer.valueOf(KeyEvent.KEYCODE_1), Integer.valueOf(R.id.n1));
        keyAndIds.put(Integer.valueOf(KeyEvent.KEYCODE_2), Integer.valueOf(R.id.n2));
        keyAndIds.put(Integer.valueOf(KeyEvent.KEYCODE_3), Integer.valueOf(R.id.n3));
        keyAndIds.put(Integer.valueOf(KeyEvent.KEYCODE_4), Integer.valueOf(R.id.n4));
        keyAndIds.put(Integer.valueOf(KeyEvent.KEYCODE_5), Integer.valueOf(R.id.n5));
        keyAndIds.put(Integer.valueOf(KeyEvent.KEYCODE_6), Integer.valueOf(R.id.n6));
        keyAndIds.put(Integer.valueOf(KeyEvent.KEYCODE_7), Integer.valueOf(R.id.n7));
        keyAndIds.put(Integer.valueOf(KeyEvent.KEYCODE_8), Integer.valueOf(R.id.n8));
        keyAndIds.put(Integer.valueOf(KeyEvent.KEYCODE_9), Integer.valueOf(R.id.n9));
    }

    public Button getCodeView(int paramInt) {
        Button localButton1 = (Button) this.mapView.get(Integer.valueOf(paramInt));
        if (localButton1 != null)
            return localButton1;
        String str = KeyEvent.keyCodeToString(paramInt);
        int i = this.layout.getChildCount();
        for (int j = 0; ; j++) {
            if (j >= i) {
                Log.e("keytest", "not found " + str);
                return null;
            }
            View localView = this.layout.getChildAt(j);
            if ((localView instanceof Button)) {
                Button localButton2 = (Button) localView;
                if (str.contains(((Button) localView).getText())) {
                    this.mapView.put(Integer.valueOf(paramInt), localButton2);
                    return localButton2;
                }
            }
        }
    }

    public Button getViewByCode(int paramInt) {
        if (!keyAndIds.containsKey(Integer.valueOf(paramInt))) {
            return null;
        }
        int i = ((Integer) keyAndIds.get(Integer.valueOf(paramInt))).intValue();
        return (Button) this.layout.findViewById(i);
    }

    public void log(String paramString) {
        Log.i("keytest", paramString);
    }


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // TODO Auto-generated method stub
        super.onCreate(savedInstanceState);
        setContentView(R.layout.keytest);
        mContext = this;
        this.layout = ((RelativeLayout) findViewById(R.id.layout));
        this.text1 = ((TextView) findViewById(R.id.text1));
        this.text2 = ((TextView) findViewById(R.id.text2));
        success = (Button) findViewById(R.id.btn_success);
        success.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Settings.System.putInt(mContext.getContentResolver(), "Khadas_irkey_test", 1);
                finish();
            }
        });

        fail = (Button) findViewById(R.id.btn_fail);
        fail.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Settings.System.putInt(mContext.getContentResolver(), "Khadas_irkey_test", 0);
                finish();
            }
        });
    }

    public boolean onKeyDown(int paramInt, KeyEvent paramKeyEvent) {
        log("down:keyCode:" + paramInt + ",event=" + paramKeyEvent);
        Button localButton = getViewByCode(paramInt);
        if (localButton != null)
            localButton.setBackgroundResource(R.drawable.key_down);
        return true;
    }

    public boolean onKeyUp(int paramInt, KeyEvent paramKeyEvent) {
        if (paramInt == KeyEvent.KEYCODE_DPAD_CENTER) {
            log(" ok is finish this keytest acivity");
            this.finish();
            return true;
        }
        log("  up:keyCode:" + paramInt + ",event=" + paramKeyEvent);
        Button localButton = getViewByCode(paramInt);
        if (localButton != null)
            localButton.setBackgroundResource(R.drawable.key_up);
        return true;
    }

    @Override
    protected void onDestroy() {
        // TODO Auto-generated method stub
        super.onDestroy();
    }

    @Override
    protected void onResume() {
        // TODO Auto-generated method stub
        super.onResume();
    }


}
