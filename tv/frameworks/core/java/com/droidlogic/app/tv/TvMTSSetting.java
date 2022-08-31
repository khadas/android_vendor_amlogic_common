/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.app.tv;

import java.util.ArrayList;
import java.util.List;

import android.util.Log;

/* Analog tv Multi-channel television sound (Atv MTS) */
public class TvMTSSetting {
    private static String TAG = "TvMTSSetting";

    public static final String TITLE_MONO = "mono";
    public static final String TITLE_STEREO = "stereo";
    public static final String TITLE_SAP = "sap";
    public static final String TITLE_STEREO_SAP = "stereo_sap";
    public static final String TITLE_NICAM_MONO = "nicam_mono";
    public static final String TITLE_DUAL = "dual";
    public static final String TITLE_DUALI = "dualI";
    public static final String TITLE_DUALII = "dualII";
    public static final String TITLE_DUALI_II = "dualI_II";

    private static TvMTSSetting mInstance = null;

    public static synchronized TvMTSSetting getInstance() {
        if (null == mInstance) {
            mInstance = new TvMTSSetting();
        }

        return mInstance;
    }

    public class Mode {
        private String Name = "";
        private int Value = 0;

        private Mode() {
            set("", 0);
        }

        private Mode(String name, int value) {
            set(name, value);
        }

        private void set(String name, int value) {
            Name = name;
            Value = value;
        }

        public String getName() {
            return Name;
        }

        public int getValue() {
            return Value;
        }
    }

    public class MTSMode {
        /* Sound Standard */
        private Mode STD = new Mode();

        /* Sound signal input mode */
        private Mode In = new Mode();

        /* Sound signal output mode */
        private Mode Out = new Mode();

        /* The output mode that the sound signal can support list */
        /* App can use it as a menu display and option setting */
        private List<Mode> OutList = new ArrayList<>();

        private void setSTD(String name, int value) {
            STD.set(name, value);
        }

        public Mode getSTD() {
            return STD;
        }

        private void setIn(String name, int value) {
            In.set(name, value);
        }

        public Mode getIn() {
            return In;
        }

        private void setOut(String name, int value) {
            Out.set(name, value);
        }

        public Mode getOut() {
            return Out;
        }

        private void addOutList(String name, int value) {
            OutList.add(new Mode(name, value));
        }

        public List<Mode> getOutList() {
            return OutList;
        }

        public String toString() {
            String str = "STD: " + STD.getName() + "(" + STD.getValue() + ")\n" +
                   "In: " + In.getName() + "(" + In.getValue() + ")\n" +
                   "Out: " + Out.getName() + "(" + Out.getValue() + ")\n" +
                   "Out Mode List: \n";

           for (Mode mode : OutList) {
               if (mode.getName() == null || mode.getName().equals(""))
                   continue;

               str = str + mode.getName() + "(" + mode.getValue() + ")\n";
           }

            return str;
        }
    }

    public static final int BTSC_STD = TvControlManager.AUDIO_STANDARD_BTSC;
    public static final String BTSC_Name = "BTSC";

    public static final int BTSC_OUT_MONO   = TvControlManager.AUDIO_OUTMODE_MONO;
    public static final int BTSC_OUT_STEREO = TvControlManager.AUDIO_OUTMODE_STEREO;
    public static final int BTSC_OUT_SAP    = TvControlManager.AUDIO_OUTMODE_SAP;

    public static final int BTSC_IN_MONO       = TvControlManager.AUDIO_INMODE_MONO;
    public static final int BTSC_IN_STEREO     = TvControlManager.AUDIO_INMODE_STEREO;
    public static final int BTSC_IN_SAP        = TvControlManager.AUDIO_INMODE_MONO_SAP;
    public static final int BTSC_IN_STEREO_SAP = TvControlManager.AUDIO_INMODE_STEREO_SAP;

    public MTSMode createMtsBtsc(int std, int in, int out) {

        if (!equalsBTSCStandard(std)) {
            Log.d(TAG, "BTSC createMTSMode error: std = " + std);
            return null;
        }

        MTSMode mode = new MTSMode();

        mode.setSTD(BTSC_Name, std);

        switch (in) {
            case BTSC_IN_MONO:
                mode.setIn(TITLE_MONO, in);
                mode.setOut(TITLE_MONO, BTSC_OUT_MONO);
                mode.addOutList(TITLE_MONO, BTSC_OUT_MONO);
                break;
            case BTSC_IN_STEREO:
                mode.setIn(TITLE_STEREO, in);
                if (out == BTSC_OUT_STEREO) {
                    mode.setOut(TITLE_STEREO, BTSC_OUT_STEREO);
                } else {
                    mode.setOut(TITLE_MONO, BTSC_OUT_MONO);
                }
                mode.addOutList(TITLE_MONO, BTSC_OUT_MONO);
                mode.addOutList(TITLE_STEREO, BTSC_OUT_STEREO);
                break;
            case BTSC_IN_SAP:
                mode.setIn(TITLE_SAP, in);
                if (out == BTSC_OUT_SAP) {
                    mode.setOut(TITLE_SAP, BTSC_OUT_SAP);
                } else {
                    mode.setOut(TITLE_MONO, BTSC_OUT_MONO);
                }
                mode.addOutList(TITLE_MONO, BTSC_OUT_MONO);
                mode.addOutList(TITLE_SAP, BTSC_OUT_SAP);
                break;
            case BTSC_IN_STEREO_SAP:
                mode.setIn(TITLE_STEREO_SAP, in);
                if (out == BTSC_OUT_STEREO) {
                    mode.setOut(TITLE_STEREO, BTSC_OUT_STEREO);
                } else if (out == BTSC_OUT_SAP) {
                    mode.setOut(TITLE_SAP, BTSC_OUT_SAP);
                } else {
                    mode.setOut(TITLE_MONO, BTSC_OUT_MONO);
                }
                mode.addOutList(TITLE_MONO, BTSC_OUT_MONO);
                mode.addOutList(TITLE_STEREO, BTSC_OUT_STEREO);
                mode.addOutList(TITLE_SAP, BTSC_OUT_SAP);
                break;
            default:
                Log.d(TAG, BTSC_Name + " createMTSMode error: in = " + in + ", out = " + out);
                break;
        }

        return mode;
    }

    public boolean equalsBTSCStandard(int std) {
        return (std == BTSC_STD);
    }

    public static final int A2_STD_K   = TvControlManager.AUDIO_STANDARD_A2_K;
    public static final int A2_STD_BG  = TvControlManager.AUDIO_STANDARD_A2_BG;
    public static final int A2_STD_DK1 = TvControlManager.AUDIO_STANDARD_A2_DK1;
    public static final int A2_STD_DK2 = TvControlManager.AUDIO_STANDARD_A2_DK2;
    public static final int A2_STD_DK3 = TvControlManager.AUDIO_STANDARD_A2_DK3;
    public static final String A2_Name = "A2";

    public static final int A2_OUT_MONO     = TvControlManager.AUDIO_OUTMODE_A2_MONO;
    public static final int A2_OUT_STEREO   = TvControlManager.AUDIO_OUTMODE_A2_STEREO;
    public static final int A2_OUT_DUALI    = TvControlManager.AUDIO_OUTMODE_A2_DUAL_A;
    public static final int A2_OUT_DUALII   = TvControlManager.AUDIO_OUTMODE_A2_DUAL_B;
    public static final int A2_OUT_DUALI_II = TvControlManager.AUDIO_OUTMODE_A2_DUAL_AB;

    public static final int A2_IN_MONO   = TvControlManager.AUDIO_INMODE_MONO;
    public static final int A2_IN_STEREO = TvControlManager.AUDIO_INMODE_STEREO;
    public static final int A2_IN_DUAL   = TvControlManager.AUDIO_INMODE_DUAL;

    public MTSMode createMtsA2(int std, int in, int out) {

        if (!equalsA2Standard(std)) {
            Log.d(TAG, "A2 createMTSMode error: std = " + std);
            return null;
        }

        MTSMode mode = new MTSMode();

        mode.setSTD(A2_Name, std);

        switch (in) {
            case A2_IN_MONO:
                mode.setIn(TITLE_MONO, in);
                mode.setOut(TITLE_MONO, A2_OUT_MONO);
                mode.addOutList(TITLE_MONO, A2_OUT_MONO);
                break;
            case A2_IN_STEREO:
                mode.setIn(TITLE_STEREO, in);
                if (out == A2_OUT_STEREO) {
                    mode.setOut(TITLE_STEREO, A2_OUT_STEREO);
                } else {
                    mode.setOut(TITLE_MONO, A2_OUT_MONO);
                }
                mode.addOutList(TITLE_MONO, A2_OUT_MONO);
                mode.addOutList(TITLE_STEREO, A2_OUT_STEREO);
                break;
            case A2_IN_DUAL:
                mode.setIn(TITLE_DUAL, in);
                if (out == A2_OUT_DUALI_II) {
                    mode.setOut(TITLE_DUALI_II, A2_OUT_DUALI_II);
                } else if (out == A2_OUT_DUALII) {
                    mode.setOut(TITLE_DUALII, A2_OUT_DUALII);
                } else {
                    mode.setOut(TITLE_DUALI, A2_OUT_DUALI);
                }
                mode.addOutList(TITLE_DUALI, A2_OUT_DUALI);
                mode.addOutList(TITLE_DUALII, A2_OUT_DUALII);
                mode.addOutList(TITLE_DUALI_II, A2_OUT_DUALI_II);
                break;
            default:
                Log.d(TAG, A2_Name + " createMTSMode error: in = " + in + ", out = " + out);
                break;
        }

        return mode;
    }

    public boolean equalsA2Standard(int std) {
        return (std == A2_STD_K
                || std == A2_STD_BG
                || std == A2_STD_DK1
                || std == A2_STD_DK2
                || std == A2_STD_DK3);
    }

    public static final int NICAM_STD_I  = TvControlManager.AUDIO_STANDARD_NICAM_I;
    public static final int NICAM_STD_BG = TvControlManager.AUDIO_STANDARD_NICAM_BG;
    public static final int NICAM_STD_L  = TvControlManager.AUDIO_STANDARD_NICAM_L;
    public static final int NICAM_STD_DK = TvControlManager.AUDIO_STANDARD_NICAM_DK;
    public static final String NICAM_Name = "NICAM";

    public static final int NICAM_OUT_MONO     = TvControlManager.AUDIO_OUTMODE_NICAM_MONO;
    public static final int NICAM_OUT_MONO1    = TvControlManager.AUDIO_OUTMODE_NICAM_MONO1;
    public static final int NICAM_OUT_STEREO   = TvControlManager.AUDIO_OUTMODE_NICAM_STEREO;
    public static final int NICAM_OUT_DUALI    = TvControlManager.AUDIO_OUTMODE_NICAM_DUAL_A;
    public static final int NICAM_OUT_DUALII   = TvControlManager.AUDIO_OUTMODE_NICAM_DUAL_B;
    public static final int NICAM_OUT_DUALI_II = TvControlManager.AUDIO_OUTMODE_NICAM_DUAL_AB;

    public static final int NICAM_IN_MONO   = TvControlManager.AUDIO_INMODE_MONO;
    public static final int NICAM_IN_STEREO = TvControlManager.AUDIO_INMODE_STEREO;
    public static final int NICAM_IN_DUAL   = TvControlManager.AUDIO_INMODE_DUAL;
    public static final int NICAM_IN_MONO1  = TvControlManager.AUDIO_INMODE_NICAM_MONO;

    public MTSMode createMtsNicam(int std, int in, int out) {

        if (!equalsNICAMStandard(std)) {
            Log.d(TAG, "NICAM createMTSMode error: std = " + std);
            return null;
        }

        MTSMode mode = new MTSMode();

        mode.setSTD(NICAM_Name, std);

        switch (in) {
            case NICAM_IN_MONO:
                mode.setIn(TITLE_MONO, in);
                mode.setOut(TITLE_MONO, NICAM_OUT_MONO);
                mode.addOutList(TITLE_MONO, NICAM_OUT_MONO);
                break;
            case NICAM_IN_STEREO:
                mode.setIn(TITLE_STEREO, in);
                if (out == NICAM_OUT_STEREO) {
                    mode.setOut(TITLE_STEREO, NICAM_OUT_STEREO);
                } else {
                    mode.setOut(TITLE_MONO, NICAM_OUT_MONO);
                }
                mode.addOutList(TITLE_MONO, NICAM_OUT_MONO);
                mode.addOutList(TITLE_STEREO, NICAM_OUT_STEREO);
                break;
            case NICAM_IN_DUAL:
                mode.setIn(TITLE_DUAL, in);
                if (out == NICAM_OUT_DUALI_II) {
                    mode.setOut(TITLE_DUALI_II, NICAM_OUT_DUALI_II);
                } else if (out == NICAM_OUT_DUALII) {
                    mode.setOut(TITLE_DUALII, NICAM_OUT_DUALII);
                } else if (out == NICAM_OUT_DUALI) {
                    mode.setOut(TITLE_DUALI, NICAM_OUT_DUALI);
                } else {
                    mode.setOut(TITLE_MONO, NICAM_OUT_MONO);
                }
                mode.addOutList(TITLE_MONO, NICAM_OUT_MONO);
                mode.addOutList(TITLE_DUALI, NICAM_OUT_DUALI);
                mode.addOutList(TITLE_DUALII, NICAM_OUT_DUALII);
                mode.addOutList(TITLE_DUALI_II, NICAM_OUT_DUALI_II);
                break;
            case NICAM_IN_MONO1:
                mode.setIn(TITLE_NICAM_MONO, in);
                if (out == NICAM_OUT_MONO1) {
                    mode.setOut(TITLE_NICAM_MONO, NICAM_OUT_MONO1);
                } else {
                    mode.setOut(TITLE_MONO, NICAM_OUT_MONO);
                }
                mode.addOutList(TITLE_MONO, NICAM_OUT_MONO);
                mode.addOutList(TITLE_NICAM_MONO, NICAM_OUT_MONO1);
                break;
            default:
                Log.d(TAG, NICAM_Name + " createMTSMode error: in = " + in + ", out = " + out);
                break;
        }

        return mode;
    }

    public boolean equalsNICAMStandard(int std) {
        return (std == NICAM_STD_I
                || std == NICAM_STD_BG
                || std == NICAM_STD_L
                || std == NICAM_STD_DK);
    }

    public static final int MONO_STD_BG  = TvControlManager.AUDIO_STANDARD_MONO_BG;
    public static final int MONO_STD_DK  = TvControlManager.AUDIO_STANDARD_MONO_DK;
    public static final int MONO_STD_I   = TvControlManager.AUDIO_STANDARD_MONO_I;
    public static final int MONO_STD_M   = TvControlManager.AUDIO_STANDARD_MONO_M;
    public static final int MONO_STD_L   = TvControlManager.AUDIO_STANDARD_MONO_L;
    public static final String MONO_Name = "MONO";

    public static final int MONO_OUT_MONO = TvControlManager.AUDIO_OUTMODE_MONO;

    public static final int MONO_IN_MONO = TvControlManager.AUDIO_INMODE_MONO;

    public MTSMode createMtsMono(int std, int in, int out) {

        if (!equalsMONOStandard(std)) {
            Log.d(TAG, "MONO createMTSMode error: std = " + std);
            return null;
        }

        if (in != MONO_IN_MONO || out != MONO_OUT_MONO && in != out) {
            Log.d(TAG, MONO_Name + " createMTSMode error: in = " + in + ", out = " + out);
            return null;
        }

        MTSMode mode = new MTSMode();

        mode.setSTD(MONO_Name, std);

        mode.setIn(TITLE_MONO, in);
        mode.setOut(TITLE_MONO, MONO_OUT_MONO);
        mode.addOutList(TITLE_MONO, MONO_OUT_MONO);

        return mode;
    }

    public boolean equalsMONOStandard(int std) {
        return (std == MONO_STD_BG
                || std == MONO_STD_DK
                || std == MONO_STD_I
                || std == MONO_STD_M
                || std == MONO_STD_L);
    }

    public static void setAtvMTSOutModeValue(int mode) {
        Log.d(TAG, "setAtvMTSOutModeValue: " + mode);

        TvControlManager.getInstance().SetAudioOutmode(mode);
    }

    public static void setAtvMTSOutMode(MTSMode mode) {
        Log.d(TAG, "setAtvMTSOutMode mode[" + mode.getOut().getName() + "] : " + mode.getOut().getValue());

        TvControlManager.getInstance().SetAudioOutmode(mode.getOut().getValue());
    }

    public static int getAtvMTSOutModeValue() {

        int value = TvControlManager.getInstance().GetAudioOutmode();

        Log.d(TAG, "getAtvMTSOutModeValue: " + value);

        return value;
    }

    public static int getAtvMTSInSTDValue() {
        int value = TvControlManager.getInstance().GetAudioStreamOutmode();

        value = (value >> 8) & 0xFF;

        Log.d(TAG, "getAtvMTSInSTDValue: " + value);

        return value;
    }

    public static int getAtvMTSInModeValue() {
        int value = TvControlManager.getInstance().GetAudioStreamOutmode();

        value = value & 0xFF;

        Log.d(TAG, "getAtvMTSInModeValue: " + value);

        return value;
    }

    public MTSMode getAtvMTSMode() {
        MTSMode mode = null;

        int inmode = getAtvMTSInModeValue();
        int outmode = getAtvMTSOutModeValue();
        int std = getAtvMTSInSTDValue();

        switch (std) {
            case BTSC_STD:
                mode = createMtsBtsc(std, inmode, outmode);
                break;
            case A2_STD_K:
            case A2_STD_BG:
            case A2_STD_DK1:
            case A2_STD_DK2:
            case A2_STD_DK3:
                mode = createMtsA2(std, inmode, outmode);
                break;
            case NICAM_STD_I:
            case NICAM_STD_BG:
            case NICAM_STD_L:
            case NICAM_STD_DK:
                mode = createMtsNicam(std, inmode, outmode);
                break;
            case MONO_STD_BG:
            case MONO_STD_DK:
            case MONO_STD_I:
            case MONO_STD_M:
            case MONO_STD_L:
                mode = createMtsMono(std, inmode, outmode);
                break;
            default:
                Log.d(TAG, "Unsupport audio std: 0x" + Integer.toHexString(std));
                break;
        }

        if (mode != null) {
            Log.d(TAG, "getAtvMTSMode: \n" + mode.toString());
        }

        return mode;
    }

    public static void setVolumeCompensate(int value) {
        TvControlManager.getInstance().SetCurProgVolumeCompesition(value);
    }
}
