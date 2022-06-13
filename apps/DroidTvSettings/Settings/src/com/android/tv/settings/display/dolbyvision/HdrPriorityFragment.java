/* Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License
 */

package com.android.tv.settings.display.dolbyvision;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.RemoteException;
import android.provider.Settings;
import androidx.preference.SwitchPreference;
import com.android.tv.settings.SettingsPreferenceFragment;
import androidx.preference.ListPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceScreen;
import androidx.preference.TwoStatePreference;
import android.util.ArrayMap;
import android.util.Log;
import android.text.TextUtils;

import com.droidlogic.app.OutputModeManager;
import com.android.tv.settings.R;
import com.android.tv.settings.RadioPreference;
import com.android.tv.settings.dialog.old.Action;

import java.util.List;
import java.util.Map;
import java.util.ArrayList;
import java.util.Timer;
import java.util.TimerTask;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.LayoutInflater;
import android.view.Window;
import android.view.WindowManager;
import android.widget.TextView;
import android.os.PowerManager;

public class HdrPriorityFragment extends SettingsPreferenceFragment implements OnClickListener {
    private static final String TAG = "HdrPriorityFragment";
    private static HdrPriorityFragment uniqueInstance;
    RadioPreference prePreference;
    RadioPreference curPreference;
    private View view_dialog;
    private TextView tx_title;
    private TextView tx_content;
    private Timer timer;
    private TimerTask task;
    private AlertDialog mAlertDialog = null;
    private int countdown = 10;
    private static final int MSG_FRESH_UI = 0;
    private static final int MSG_COUNT_DOWN = 1;

    private static final int DOLBY_VISION           = 0;
    private static final int HDR10                  = 1;
    private static final int SDR                    = 2;
    public int preType                              = 0;
    public int curType                              = 0;

    private OutputModeManager mOutputModeManager;

    // Adjust this value to keep things relatively responsive without janking
    // animations
    private static final int HDR_SET_DELAY_MS = 100;
    private final Handler mDelayHandler = new Handler();
    private String mNewMode;
    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_FRESH_UI:
                    curPreference.clearOtherRadioPreferences(getPreferenceScreen());
                    curPreference.setChecked(true);
                    break;
                case MSG_COUNT_DOWN:
                    tx_title.setText(Integer.toString(countdown) + " " + getResources().getString(R.string.hdr_priority_countdown));
                    if (countdown == 0) {
                        if (mAlertDialog != null) {
                            mAlertDialog.dismiss();
                        }
                        recoverHdrPriority();
                        task.cancel();
                    }
                    countdown--;
                    break;
            }
        }
    };

    public static HdrPriorityFragment newInstance() {
        if (uniqueInstance == null) {
            uniqueInstance = new HdrPriorityFragment();
        }
        return uniqueInstance;
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        mOutputModeManager = OutputModeManager.getInstance(getActivity());
        final Context themedContext = getPreferenceManager().getContext();
        final PreferenceScreen screen = getPreferenceManager().createPreferenceScreen(themedContext);
        screen.setTitle(R.string.hdr_priority);
        Preference activePref = null;

        final List<Action> hdrInfoList = getActions();
        for (final Action hdrInfo : hdrInfoList) {
            final String hdrTag = hdrInfo.getKey();
            final RadioPreference radioPreference = new RadioPreference(themedContext);
            radioPreference.setKey(hdrTag);
            radioPreference.setPersistent(false);
            radioPreference.setTitle(hdrInfo.getTitle());
            radioPreference.setLayoutResource(R.layout.preference_reversed_widget);

            if (hdrInfo.isChecked()) {
                radioPreference.setChecked(true);
                prePreference = curPreference = radioPreference;
                activePref = radioPreference;
            }
            screen.addPreference(radioPreference);
        }
        if (activePref != null && savedInstanceState == null) {
            scrollToPreference(activePref);
        }
        setPreferenceScreen(screen);
    }

    private ArrayList<Action> getActions() {
        int mode = mOutputModeManager.getHdrPriority();
        boolean customConfig       = mOutputModeManager.isSupportNetflix();
        boolean displaydebugConfig = mOutputModeManager.isSupportDisplayDebug();
        Log.d(TAG,"Current Hdr Priority: " + mode);
        Log.d(TAG,"customConfig "+ customConfig);
        Log.d(TAG,"displaydebugConfig "+ displaydebugConfig);

        ArrayList<Action> actions = new ArrayList<Action>();
        actions.add(new Action.Builder().key(Integer.toString(DOLBY_VISION)).title(getString(R.string.dolby_vision))
                .checked(mode == DOLBY_VISION).build());
        actions.add(new Action.Builder().key(Integer.toString(HDR10)).title(getString(R.string.hdr10))
                .checked(mode == HDR10).build());
        //netflix not display sdr
        if (displaydebugConfig) {
            actions.add(new Action.Builder().key(Integer.toString(SDR)).title(getString(R.string.sdr))
                .checked(mode == SDR).build());
        }

        return actions;
    }

    private void showDialog () {
        if (mAlertDialog == null) {
            LayoutInflater inflater = (LayoutInflater)getActivity().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            view_dialog = inflater.inflate(R.layout.dialog_outputmode, null);

            AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
            mAlertDialog = builder.create();
            mAlertDialog.getWindow().setType(WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY);

            tx_title = (TextView)view_dialog.findViewById(R.id.dialog_title);
            tx_content = (TextView)view_dialog.findViewById(R.id.dialog_content);

            TextView button_cancel = (TextView)view_dialog.findViewById(R.id.dialog_cancel);
            button_cancel.setOnClickListener(this);

            TextView button_ok = (TextView)view_dialog.findViewById(R.id.dialog_ok);
            button_ok.setOnClickListener(this);
        }
        mAlertDialog.show();
        mAlertDialog.getWindow().setContentView(view_dialog);
        mAlertDialog.setCancelable(false);

        tx_content.setText(getResources().getString(R.string.hdr_priority_change)
                + " " + (getHDRPriorityString(mOutputModeManager.getHdrPriority()))
                + "\nMust restart the system for this modification to take effect");

        countdown = 15;
        if (timer == null)
            timer = new Timer();
        if (task != null)
            task.cancel();
        task = new DialogTimerTask();
        timer.schedule(task, 0, 1000);
    }

    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.dialog_cancel:
                if (mAlertDialog != null) {
                    mAlertDialog.dismiss();
                }
                recoverHdrPriority();
                break;
            case R.id.dialog_ok:
                if (mAlertDialog != null) {
                    mAlertDialog.dismiss();
                    prePreference = curPreference;
                    Context context = (Context) (getActivity());
                    ((PowerManager) context.getSystemService(Context.POWER_SERVICE)).reboot("");
                }
                break;
        }
        task.cancel();
    }

    private class DialogTimerTask extends TimerTask {
        @Override
        public void run() {
            if (mHandler != null) {
                mHandler.sendEmptyMessage(MSG_COUNT_DOWN);
            }
        }
    };

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        if (preference instanceof RadioPreference) {
            final RadioPreference radioPreference = (RadioPreference) preference;
            radioPreference.clearOtherRadioPreferences(getPreferenceScreen());
            if (radioPreference.isChecked()) {
                preType = mOutputModeManager.getHdrPriority();
                curType = Integer.valueOf(radioPreference.getKey().toString());
                curPreference = radioPreference;
                mOutputModeManager.setHdrPriority(curType);
                showDialog();
                curPreference.setChecked(true);
            } else {
                radioPreference.setChecked(true);
            }
        }
        return super.onPreferenceTreeClick(preference);
    }

    @Override
    public int getMetricsCategory() {
        return 0;
    }

    private void recoverHdrPriority() {
       mOutputModeManager.setHdrPriority(preType);
       // need revert Preference display.
       curPreference = prePreference;
       mHandler.sendEmptyMessage(MSG_FRESH_UI);
    }

    public String getHDRPriorityString(int curType) {
        String HDRPriorityString;
        switch (curType) {
            case 0 :
               HDRPriorityString = getString(R.string.dolby_vision);
               break;
            case 1 :
               HDRPriorityString = getString(R.string.hdr10);
               break;
            case 2 :
               HDRPriorityString = getString(R.string.sdr);
               break;
            default :
               HDRPriorityString = getString(R.string.dolby_vision);
               break;
        }

        return HDRPriorityString;
    }
}
