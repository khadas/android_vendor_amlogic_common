package com.droidlogic.googletv.settings.sliceprovider.manager;

import android.content.Context;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.provider.Settings;
import android.text.TextUtils;
import android.util.Log;
import com.droidlogic.app.SystemControlManager;
import com.droidlogic.googletv.settings.R;
import com.droidlogic.googletv.settings.sliceprovider.MediaSliceConstants;
import com.droidlogic.googletv.settings.sliceprovider.utils.MediaSliceUtil;

import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

public class AdvancedSoundManager {
  private static String TAG = AdvancedSoundManager.class.getSimpleName();
  private final Context mContext;
  private volatile Map<Integer, Boolean> mFormats;
  private volatile Map<Integer, Boolean> mReportedFormats;
  private final AudioManager mAudioManager;

  public static final String VAL_SURROUND_SOUND_AUTO = "auto";
  public static final String VAL_SURROUND_SOUND_NEVER = "never";
  public static final String VAL_SURROUND_SOUND_ALWAYS = "always";
  public static final String VAL_SURROUND_SOUND_MANUAL = "manual";

  private static final String AUDIO_CAP_PATH = "/sys/class/amhdmitx/amhdmitx0/aud_cap";

  private static volatile AdvancedSoundManager mAdvancedSoundManager;

  private final SystemControlManager mSystemControlManager;

  public static boolean isInit() {
    return mAdvancedSoundManager != null;
  }

  public static AdvancedSoundManager getAdvancedSoundManager(final Context context) {
    if (mAdvancedSoundManager == null)
      synchronized (AdvancedSoundManager.class) {
        if (mAdvancedSoundManager == null)
          mAdvancedSoundManager = new AdvancedSoundManager(context);
      }
    return mAdvancedSoundManager;
  }

  private AdvancedSoundManager(final Context context) {
    mContext = context;
    mAudioManager = context.getSystemService(AudioManager.class);
    mSystemControlManager = SystemControlManager.getInstance();
    refresh();
  }

  public boolean refresh() {
    boolean ret = false;
    ret |= initFormats();
    ret |= initReportedFormats();
    return ret;
  }

  private boolean initFormats() {
    final Map<Integer, Boolean> preFormats = mFormats;
    mFormats = mAudioManager.getSurroundFormats();

    // DTS is not supported on Chromecast
    mFormats.remove(AudioFormat.ENCODING_DTS);
    mFormats.remove(AudioFormat.ENCODING_DTS_HD);
    return !mFormats.equals(preFormats);
  }

  private boolean initReportedFormats() {
    final Map<Integer, Boolean> preReportedFormats = mReportedFormats;
    final Map<Integer, Boolean> reportedFormats = new HashMap<>();
    if (mFormats.containsKey(AudioFormat.ENCODING_PCM_16BIT)) {
      reportedFormats.put(AudioFormat.ENCODING_PCM_16BIT, true);
    }

    final String audioCap = mSystemControlManager.readSysFs(AUDIO_CAP_PATH);
    if (audioCap.contains("Dobly_Digital+") && mFormats.containsKey(AudioFormat.ENCODING_E_AC3)) {
      reportedFormats.put(AudioFormat.ENCODING_E_AC3, true);
    }
    if (audioCap.contains("ATMOS") && mFormats.containsKey(AudioFormat.ENCODING_E_AC3_JOC)) {
      reportedFormats.put(AudioFormat.ENCODING_E_AC3_JOC, true);
    }
    if (audioCap.contains("AC-3") && mFormats.containsKey(AudioFormat.ENCODING_AC3)) {
      reportedFormats.put(AudioFormat.ENCODING_AC3, true);
    }
    if (audioCap.contains("MAT") && mFormats.containsKey(AudioFormat.ENCODING_DOLBY_TRUEHD)) {
      reportedFormats.put(AudioFormat.ENCODING_DOLBY_TRUEHD, true);
    }
    mReportedFormats = reportedFormats;

    return !mReportedFormats.equals(preReportedFormats);
  }

  /** @return the formats that are enabled in manual mode, from global settings */
  public Set<Integer> getFormatsEnabledInManualMode() {
    final Set<Integer> formats = new HashSet<>();
    final String enabledFormats =
        Settings.Global.getString(
            mContext.getContentResolver(), Settings.Global.ENCODED_SURROUND_OUTPUT_ENABLED_FORMATS);
    if (enabledFormats == null) {
      // Starting with Android P passthrough setting ALWAYS has been replaced with MANUAL.
      // In that case all formats will be enabled when in MANUAL mode.
      formats.addAll(mFormats.keySet());
    } else {
      try {
        Arrays.stream(TextUtils.split(enabledFormats, ","))
            .mapToInt(Integer::parseInt)
            .forEach(formats::add);
      } catch (final NumberFormatException e) {
        if (MediaSliceUtil.CanDebug()) Log.w(TAG, "ENCODED_SURROUND_OUTPUT_ENABLED_FORMATS misformatted.", e);
      }
    }
    return formats;
  }

  /** Writes enabled/disabled state for a given format to the global settings. */
  public void setSurroundManualFormatsSetting(final boolean enabled, final int formatId) {
    final Set<Integer> formats = getFormatsEnabledInManualMode();
    if (enabled) {
      formats.add(formatId);
    } else {
      formats.remove(formatId);
    }
    Settings.Global.putString(
        mContext.getContentResolver(),
        Settings.Global.ENCODED_SURROUND_OUTPUT_ENABLED_FORMATS,
        TextUtils.join(",", formats));
  }

  /** @return the display name for each surround sound format. */
  public String getFormatDisplayName(final int formatId) {
    switch (formatId) {
      case AudioFormat.ENCODING_AC3:
        return mContext.getString(R.string.surround_sound_format_ac3);
      case AudioFormat.ENCODING_E_AC3:
        return mContext.getString(R.string.surround_sound_format_e_ac3);
      default:
        // Fallback in case new formats have been added that we don't know of.
        return AudioFormat.toDisplayName(formatId);
    }
  }

  public Map<Integer, Boolean> getAllSoundFormats() {
    return new HashMap<>(mFormats);
  }

  public Map<Integer, Boolean> getAllReportedSoundFormats() {
    return new HashMap<>(mReportedFormats);
  }

  public void setSurroundPassthroughSetting(final String key) {
    int newVal;
    switch (key) {
      case VAL_SURROUND_SOUND_AUTO:
        newVal = Settings.Global.ENCODED_SURROUND_OUTPUT_AUTO;
        break;
      case VAL_SURROUND_SOUND_MANUAL:
        newVal = Settings.Global.ENCODED_SURROUND_OUTPUT_MANUAL;
        break;
      case VAL_SURROUND_SOUND_NEVER:
        Settings.Global.putInt(
            mContext.getContentResolver(),
            MediaSliceConstants.PREVIOUS_SURROUND_SOUND_GLOBAL_SETTING,
            VAL_SURROUND_SOUND_MANUAL.equals(getSurroundPassthroughSetting()) ? 0 : 1);
        newVal = Settings.Global.ENCODED_SURROUND_OUTPUT_NEVER;
        break;

      default:
        throw new IllegalArgumentException("Unknown surround sound pref value: " + key);
    }

    Settings.Global.putInt(
        mContext.getContentResolver(), Settings.Global.ENCODED_SURROUND_OUTPUT, newVal);
  }

  public String getSurroundPassthroughSetting() {
    final int value =
        Settings.Global.getInt(
            mContext.getContentResolver(),
            Settings.Global.ENCODED_SURROUND_OUTPUT,
            Settings.Global.ENCODED_SURROUND_OUTPUT_AUTO);

    switch (value) {
      case Settings.Global.ENCODED_SURROUND_OUTPUT_AUTO:
      default:
        return VAL_SURROUND_SOUND_AUTO;
      case Settings.Global.ENCODED_SURROUND_OUTPUT_NEVER:
        return VAL_SURROUND_SOUND_NEVER;
        // On Android P ALWAYS is replaced by MANUAL.
      case Settings.Global.ENCODED_SURROUND_OUTPUT_ALWAYS:
      case Settings.Global.ENCODED_SURROUND_OUTPUT_MANUAL:
        return VAL_SURROUND_SOUND_MANUAL;
    }
  }

  public Map<Integer, Boolean> getAllSoundFormatStatuses() {
    final Map<Integer, Boolean> statuses = new HashMap<>();
    final String surroundPassthroughSetting = getSurroundPassthroughSetting();
    final Set<Integer> formatsEnabledInManualMode = getFormatsEnabledInManualMode();

    for (final Integer key : mFormats.keySet()) {
      switch (surroundPassthroughSetting) {
        case VAL_SURROUND_SOUND_AUTO:
          statuses.put(key, mReportedFormats.containsKey(key));
          break;
        case VAL_SURROUND_SOUND_NEVER:
          statuses.put(key, false);
          break;
        case VAL_SURROUND_SOUND_MANUAL:
          statuses.put(key, formatsEnabledInManualMode.contains(key));
          break;
      }
    }
    return statuses;
  }

  public void restoreSurroundPassthroughSetting() {
    final int val =
        Settings.Global.getInt(
            mContext.getContentResolver(),
            MediaSliceConstants.PREVIOUS_SURROUND_SOUND_GLOBAL_SETTING,
            1);
    setSurroundPassthroughSetting(val == 1 ? VAL_SURROUND_SOUND_AUTO : VAL_SURROUND_SOUND_MANUAL);
  }
}
