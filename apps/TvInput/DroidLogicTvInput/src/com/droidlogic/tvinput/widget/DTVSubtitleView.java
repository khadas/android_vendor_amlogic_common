/*
 * Copyright (C) 2017 Amlogic Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.droidlogic.tvinput.widget;

import android.content.Context;
import android.graphics.BitmapFactory;
import android.media.tv.TvInputService;
import android.os.Handler;
import android.content.res.Resources;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.os.Handler;
import android.os.Message;
import android.text.TextUtils;
import android.util.DisplayMetrics;
import android.view.KeyEvent;
import android.view.View;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.graphics.Paint;
import android.graphics.PaintFlagsDrawFilter;
import android.graphics.Color;
import android.graphics.Typeface;
import android.util.AttributeSet;

import java.io.File;
import java.io.FileInputStream;
import java.lang.Exception;
import java.lang.reflect.Type;
import java.util.Locale;

import android.util.Log;
import android.view.accessibility.CaptioningManager;
import android.widget.Toast;
import com.droidlogic.app.DataProviderManager;
import com.droidlogic.app.SystemControlManager;
import com.droidlogic.app.SubtitleManager;
import com.droidlogic.app.tv.ChannelInfo;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import com.droidlogic.tvinput.R;
import com.droidlogic.tvinput.widget.CcImplement;

public class DTVSubtitleView extends View {
    private static final String TAG = "DTVSubtitleView";

    private static Object lock = new Object();
    private static final int BUFFER_W = 1920;
    private static final int BUFFER_H = 1080;

    private static final int MODE_NONE = 0;
    private static final int MODE_DTV_TT = 1;
    private static final int MODE_DTV_CC = 2;
    private static final int MODE_DVB_SUB = 3;
    private static final int MODE_ATV_TT = 4;
    private static final int MODE_ATV_CC = 5;
    private static final int MODE_AV_CC = 6;
    private static final int MODE_ISDB_CC = 7;
    private static final int MODE_SCTE27_SUB = 8;

    private static final int EDGE_NONE = 0;
    private static final int EDGE_OUTLINE = 1;
    private static final int EDGE_DROP_SHADOW = 2;
    private static final int EDGE_RAISED = 3;
    private static final int EDGE_DEPRESSED = 4;

    private static final int PLAY_NONE = 0;
    private static final int PLAY_SUB = 1;
    private static final int PLAY_TT  = 2;
    private static final int PLAY_ISDB  = 3;

    private static final int DECODE_ID_TYPE = 0;
    private static final int SYNC_ID_TYPE = 1;

    public static final int COLOR_RED = 0;
    public static final int COLOR_GREEN = 1;
    public static final int COLOR_YELLOW = 2;
    public static final int COLOR_BLUE = 3;

    public static final int TT_NOTIFY_SEARCHING = 0;
    public static final int TT_NOTIFY_NOSIG = 1;
    public static final int TT_NOTIFY_CANCEL = 2;

    public static final int CC_CAPTION_DEFAULT = 0;
    /*NTSC CC channels*/
    public static final int CC_CAPTION_CC1 = 1;
    public static final int CC_CAPTION_CC2 = 2;
    public static final int CC_CAPTION_CC3 = 3;
    public static final int CC_CAPTION_CC4 = 4;
    public static final int CC_CAPTION_TEXT1 =5;
    public static final int CC_CAPTION_TEXT2 = 6;
    public static final int CC_CAPTION_TEXT3 = 7;
    public static final int CC_CAPTION_TEXT4 = 8;
    /*DTVCC services*/
    public static final int CC_CAPTION_SERVICE1 = 9;
    public static final int CC_CAPTION_SERVICE2 = 10;
    public static final int CC_CAPTION_SERVICE3 = 11;
    public static final int CC_CAPTION_SERVICE4 = 12;
    public static final int CC_CAPTION_SERVICE5 = 13;
    public static final int CC_CAPTION_SERVICE6 = 14;

    public static final int CC_FONTSIZE_DEFAULT = 0;
    public static final int CC_FONTSIZE_SMALL = 1;
    public static final int CC_FONTSIZE_STANDARD = 2;
    public static final int CC_FONTSIZE_BIG = 3;

    public static final int CC_FONTSTYLE_DEFAULT = 0;
    public static final int CC_FONTSTYLE_MONO_SERIF = 1;
    public static final int CC_FONTSTYLE_PROP_SERIF = 2;
    public static final int CC_FONTSTYLE_MONO_NO_SERIF = 3;
    public static final int CC_FONTSTYLE_PROP_NO_SERIF = 4;
    public static final int CC_FONTSTYLE_CASUAL = 5;
    public static final int CC_FONTSTYLE_CURSIVE = 6;
    public static final int CC_FONTSTYLE_SMALL_CAPITALS = 7;

    public static final int CC_OPACITY_DEFAULT = 0;
    public static final int CC_OPACITY_TRANSPARET = 1;
    public static final int CC_OPACITY_TRANSLUCENT= 2;
    public static final int CC_OPACITY_SOLID = 3;
    public static final int CC_OPACITY_FLASH = 4;

    public static final int CC_COLOR_DEFAULT = 0;
    public static final int CC_COLOR_WHITE = 1;
    public static final int CC_COLOR_BLACK = 2;
    public static final int CC_COLOR_RED = 3;
    public static final int CC_COLOR_GREEN = 4;
    public static final int CC_COLOR_BLUE = 5;
    public static final int CC_COLOR_YELLOW = 6;
    public static final int CC_COLOR_MAGENTA = 7;
    public static final int CC_COLOR_CYAN = 8;
    static final int VFMT_ATV = 100;

    private static SystemControlManager mSystemControlManager;

    public static final int JSON_MSG_NORMAL = 0;
    public static final int SUB_VIEW_SHOW = 1;
    public static final int SUB_VIEW_CLEAN = 2;
    public static final int TT_STOP_REFRESH = 3;
    public static final int TT_ZOOM = 4;
    public static final int TT_PAGE_TYPE = 5;
    public static final int TT_NO_SIGAL = 6;

    private static final int TT_DETECT_TIMEOUT = 5 * 1000;

    private static int init_count = 0;
    private static CaptioningManager captioningManager = null;
    private static CcImplement.CaptionWindow cw = null;
    private static IsdbImplement isdbi = null;
    private static CustomFonts cf = null;
    private static CcImplement ci = null;
    private static PorterDuffXfermode mXfermode;
    private static PaintFlagsDrawFilter paint_flag;
    private static String json_str;
    private static Bitmap bitmap = null;
    private static Paint mPaint;
    private static Paint clear_paint;
    private static boolean teletext_have_data = false;
    private static boolean tt_reveal_mode = false;
    private static int tt_zoom_state = 0;
    private static boolean hide_reveal_state = false;
    private static boolean draw_no_subpg_notification = false;
    private static int tt_notify_status = TT_NOTIFY_CANCEL;
    private static int notify_pgno = 0;

    private static int tt_page_type;
    public static String TT_REGION_DB = "teletext_region_id";

    private boolean need_clear_canvas;
    private boolean tt_refresh_switch = true;
    private boolean decoding_status;

    public static boolean cc_is_started = false;

    private boolean isPreWindowMode = false;
    private static SubtitleManager mSubtitleManager = null;
    private static SubtitleManagerListener mSubtitleManagerListener = null;
    public final int SUBTITLE_DEMUX_SOURCE  = 4;
    public final int SUBTITLE_VBI_SOURCE    = 5;
    public final int SUBTITLE_IOTYPE_SUB    = 5;
    public final int SUBTITLE_IOTYPE_TTX    = 6;
    public final int SUBTITLE_IOTYPE_SCTE27 = 7;

    public static final int TTX_MIX_MODE_NORAML = 0;
    public static final int TTX_MIX_MODE_TRANSPARENT = 1;
    public static final int TTX_MIX_MODE_LEFT_RIGHT = 2;
    private int mMixMode = TTX_MIX_MODE_NORAML;
    private int subtitlemanager_init(Context context) {
        synchronized(lock) {
            if (mSubtitleManager == null) {
                mSubtitleManager = new SubtitleManager(context);
            }
        }
        return 0;
    }

    private int subtitlemanager_destroy() {
        synchronized(lock) {
            if (mSubtitleManager != null) {
                mSubtitleManager.close();
                mSubtitleManager.destory();
                mSubtitleManager = null;
            }
        }
        return 0;
    }
    private int subtitlemanager_clear() {
        synchronized(lock) {
            if (mSubtitleManager != null)
            {
                mSubtitleManager.clear();
                return 0;
            }
        }
        return -1;
    }
    private int subtitlemanager_start_dvb_sub(int dmx_id, int pid, int page_id, int anc_page_id) {
        synchronized(lock) {
            if (mSubtitleManager != null)
            {
                mSubtitleManager.setSubType(SUBTITLE_IOTYPE_SUB);
                mSubtitleManager.setSubPid(pid);
                mSubtitleManager.open("", (dmx_id << 16)|SUBTITLE_DEMUX_SOURCE);
            }
        }
        return 0;
    }
    private int subtitlemanager_start_dtv_tt(int dmx_id, int region_id, int pid, int page, int sub_page, boolean is_sub) {
        synchronized(lock) {
            if (mSubtitleManager != null)
            {
                mSubtitleManager.setSubType(SUBTITLE_IOTYPE_TTX);
                mSubtitleManager.setSubPid(pid);
                if (mSubtitleManager.open("", (dmx_id << 16)|SUBTITLE_DEMUX_SOURCE)) {
                    if (!is_sub) {
                        mSubtitleManager.ttGoHome();
                    }
                }
            }
        }
        return 0;
    }
    private int subtitlemanager_start_atv_tt(int region_id, int page_no, int sub_page_no, boolean is_sub)
    {
        synchronized(lock) {
            if (mSubtitleManager != null)
            {
                mSubtitleManager.setSubType(SUBTITLE_IOTYPE_TTX);
                mSubtitleManager.setSubPid(region_id);
                if (mSubtitleManager.open("", SUBTITLE_VBI_SOURCE)) {
                    if (!is_sub) {
                        mSubtitleManager.ttGoHome();
                    }
                }
            }
        }
        return 0;
    }
    private int subtitlemanager_stop()
    {
        synchronized(lock) {
            if (mSubtitleManager != null) {
                mSubtitleManagerListener = null;
                mSubtitleManager.setSubtitleDataListner(null);
                mSubtitleManager.close();
            }
        }
        return 0;
    }
    private int subtitlemanager_tt_control(int event, int para0, int para1) {
        synchronized(lock) {
            if (mSubtitleManager != null) {
                return mSubtitleManager.ttControl(event, para0, para1);
            }
        }
        return 0;
    }
    private int subtitlemanager_tt_set_region(int region_id)
    {
        synchronized(lock) {
            if (mSubtitleManager != null) {
                return mSubtitleManager.ttControl(SubtitleManager.TT_EVENT_SET_REGION_ID, -1, -1, region_id);
            }
        }
        return 0;
    }

    private int subtitlemanager_tt_set_search_pattern(String pattern, boolean casefold)
    {
        return 0;
    }

    private int subtitlemanager_start_atsc_cc(int source, int vfmt, int caption, int decoder_param, String lang, int fg_color, int fg_opacity, int bg_color, int bg_opacity, int font_style, int font_size)
    {
        synchronized(lock) {
            if (mSubtitleManager != null) {
                //mSubtitleManager.open("", SUBTITLE_DEMUX_SOURCE);
                mSubtitleManager.startCCchanel(caption|source<<8);
            }
        }
        return 0;
    }
    private int subtitlemanager_start_atsc_atvcc(int caption, int decoder_param, String lang, int fg_color, int fg_opacity, int bg_color, int bg_opacity, int font_style, int font_size)
    {
        return 0;
    }
    private int subtitlemanager_set_atsc_cc_options(int fg_color, int fg_opacity, int bg_color, int bg_opacity, int font_style, int font_size)
    {
        return 0;
    }
    private int subtitlemanager_set_active(boolean active)
    {
        return 0;
    }
    private int subtitlemanager_start_isdbt(int dmx_id, int pid, int caption_id)
    {
        return 0;
    }
    private int subtitlemanager_start_scte27(int dmx_id, int pid)
    {
        synchronized(lock) {
            if (mSubtitleManager != null)
            {
                mSubtitleManager.setSubType(SUBTITLE_IOTYPE_SCTE27);
                mSubtitleManager.setSubPid(pid);
                mSubtitleManager.open("", (dmx_id << 16)|SUBTITLE_DEMUX_SOURCE);
            }
            }
        return 0;

    }

    static public class DVBSubParams {
        private int dmx_id;
        private int pid;
        private int composition_page_id;
        private int ancillary_page_id;
        private int player_instance_id;
        private int sync_instance_id;

        public DVBSubParams(int dmx_id, int pid, int page_id, int anc_page_id, int play_id, int sync_id) {
            this.dmx_id              = dmx_id;
            this.pid                 = pid;
            this.composition_page_id = page_id;
            this.ancillary_page_id   = anc_page_id;
            this.player_instance_id  = play_id;
            this.sync_instance_id    = sync_id;
        }
    }

    static public class ISDBParams {
        private int dmx_id;
        private int pid;
        private int caption_id;

        public ISDBParams(int dmx_id, int pid, int caption_id) {
            this.pid = pid;
            this.dmx_id = dmx_id;
            this.caption_id = caption_id;
        }
    }

    static public class DTVTTParams {
        private int dmx_id;
        private int pid;
        private int page_no;
        private int sub_page_no;
        private int region_id;
        private int type;
        private int stype;
        private int player_instance_id;
        private int sync_instance_id;

        public DTVTTParams(int dmx_id, int pid, int page_no, int sub_page_no, int region_id, int type, int stype, int play_id, int sync_id) {
            this.dmx_id      = dmx_id;
            this.pid         = pid;
            this.page_no     = page_no;
            this.sub_page_no = sub_page_no;
            this.region_id   = region_id;
            this.type = type;
            this.stype = stype;
            this.player_instance_id = play_id;
            this.sync_instance_id = sync_id;
        }
    }

    static public class Scte27Params {
        private int dmx_id;
        private int pid;

        public Scte27Params(int dmx_id, int pid) {
            this.dmx_id      = dmx_id;
            this.pid         = pid;
        }
    }

    static public class ATVTTParams {
        private int page_no;
        private int sub_page_no;
        private int region_id;
        public ATVTTParams(int page_no, int sub_page_no, int region_id) {
            this.page_no     = page_no;
            this.sub_page_no = sub_page_no;
            this.region_id   = region_id;
        }
    }

    static public class DTVCCParams {
        protected int vfmt;
        protected int caption_mode;
        protected int decoder_param;
        protected int fg_color;
        protected int fg_opacity;
        protected int bg_color;
        protected int bg_opacity;
        protected int font_style;
        protected float font_size;
        protected String lang;
        private int player_instance_id;
        private int sync_instance_id;

        public DTVCCParams(int vfmt, int caption, int decoder_param, String lang, int fg_color, int fg_opacity,
                int bg_color, int bg_opacity, int font_style, float font_size, int play_id, int sync_id) {
            this.vfmt = vfmt;
            this.caption_mode = caption;
            this.decoder_param = decoder_param;
            this.lang = lang;
            this.fg_color = fg_color;
            this.fg_opacity = fg_opacity;
            this.bg_color = bg_color;
            this.bg_opacity = bg_opacity;
            this.font_style = font_style;
            this.font_size = font_size;
            this.player_instance_id = play_id;
            this.sync_instance_id = sync_id;
        }
    }

    static public class ATVCCParams extends DTVCCParams {
        public ATVCCParams(int caption, int decoder_param, String lang, int fg_color, int fg_opacity,
                int bg_color, int bg_opacity, int font_style, float font_size) {
            super(VFMT_ATV, caption, decoder_param, lang, fg_color, fg_opacity,
                    bg_color, bg_opacity, font_style, font_size, 0, 0);
        }
    }
    static public class AVCCParams extends DTVCCParams {
        public AVCCParams(int caption, int decoder_param, String lang, int fg_color, int fg_opacity,
                int bg_color, int bg_opacity, int font_style, float font_size) {
            super(VFMT_ATV, caption, decoder_param, lang, fg_color, fg_opacity,
                    bg_color, bg_opacity, font_style, font_size, 0, 0);
        }
    }
    private class SubParams {
        int mode;
        int vfmt;
        DVBSubParams dvb_sub;
        DTVTTParams  dtv_tt;
        ATVTTParams  atv_tt;
        DTVCCParams  dtv_cc;
        ATVCCParams  atv_cc;
        AVCCParams  av_cc;
        ISDBParams isdb_cc;
        Scte27Params scte27_sub;

        private SubParams() {
            mode = MODE_NONE;
        }
    }

    private class TTParams {
        int mode;
        DTVTTParams dtv_tt;
        ATVTTParams atv_tt;

        private TTParams() {
            mode = MODE_NONE;
        }
    }

    private int disp_left = 0;
    private int disp_right = 0;
    private int disp_top = 0;
    private int disp_bottom = 0;
    private boolean active = true;

    private static SubParams sub_params;
    private static int       play_mode;
    private boolean   visible;
    private boolean   destroy;
    private static DTVSubtitleView activeView = null;

    private void update() {
        postInvalidate();
    }

    private void stopDecoder() {
        Log.d(TAG, "subtitleView stopSub cc:" + cc_is_started + " playmode " + play_mode + " sub_mode " + sub_params.mode);
        synchronized(lock) {
            if (!cc_is_started)
                return;

            switch (play_mode) {
                case PLAY_NONE:
                    break;
                case PLAY_TT:
                    handler.removeMessages(TT_NO_SIGAL);
                    tt_notify_status = TT_NOTIFY_CANCEL;
                    need_clear_canvas = true;
                    postInvalidate();
                    switch (sub_params.mode) {
                        case MODE_DTV_TT:
                            teletext_have_data = false;
                            break;
                        case MODE_ATV_TT:
                            teletext_have_data = false;
                        default:
                            break;
                    }
                    break;
                case PLAY_SUB:
                    switch (sub_params.mode) {
                        case MODE_DTV_TT:
                            teletext_have_data = false;
                            break;
                        case MODE_ATV_TT:
                            teletext_have_data = false;
                            break;
                        case MODE_DVB_SUB:
                        case MODE_DTV_CC:
                        case MODE_ATV_CC:
                        case MODE_AV_CC:
                        case MODE_ISDB_CC:
                        case MODE_SCTE27_SUB:
                        default:
                            break;
                    }
                    break;
            }
            subtitlemanager_stop();
            cc_is_started = false;
            play_mode = PLAY_NONE;
            need_clear_canvas = true;
        }
    }

    private void init(Context context) {
        synchronized(lock) {
            if (init_count == 0) {
                play_mode = PLAY_NONE;
                visible = true;
                destroy = false;
                sub_params = new SubParams();
                need_clear_canvas = true;

                if (subtitlemanager_init(context) < 0) {
                }

                decoding_status = false;
                cf = new CustomFonts(context);
                ci = new CcImplement(context, cf);
                cw = ci.new CaptionWindow();
                isdbi = new IsdbImplement(context);
                mPaint = new Paint();
                clear_paint = new Paint();
                clear_paint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.CLEAR));
                mSystemControlManager = SystemControlManager.getInstance();
                mXfermode = new PorterDuffXfermode(PorterDuff.Mode.SRC);
                paint_flag = new PaintFlagsDrawFilter(0, Paint.ANTI_ALIAS_FLAG | Paint.FILTER_BITMAP_FLAG);
                captioningManager = (CaptioningManager) context.getSystemService(Context.CAPTIONING_SERVICE);
                captioningManager.addCaptioningChangeListener(new CaptioningManager.CaptioningChangeListener() {
                    @Override
                    public void onEnabledChanged(boolean enabled) {
                        super.onEnabledChanged(enabled);
                        Log.e(TAG, "onenableChange from " + ci.cc_setting.is_enabled + " to " + captioningManager.isEnabled());
                        ci.cc_setting.is_enabled = captioningManager.isEnabled();
                    }

                    @Override
                    public void onFontScaleChanged(float fontScale) {
                        super.onFontScaleChanged(fontScale);
                        Log.e(TAG, "onfontscaleChange");
                        ci.cc_setting.font_scale = captioningManager.getFontScale();
                    }

                    @Override
                    public void onLocaleChanged(Locale locale) {
                        super.onLocaleChanged(locale);
                        Log.e(TAG, "onlocaleChange");
                        ci.cc_setting.cc_locale = captioningManager.getLocale();
                    }

                    @Override
                    public void onUserStyleChanged(CaptioningManager.CaptionStyle userStyle) {
                        super.onUserStyleChanged(userStyle);
                        Log.e(TAG, "onUserStyleChange");
                        ci.cc_setting.has_foreground_color = userStyle.hasForegroundColor();
                        ci.cc_setting.has_background_color = userStyle.hasBackgroundColor();
                        ci.cc_setting.has_window_color = userStyle.hasWindowColor();
                        ci.cc_setting.has_edge_color = userStyle.hasEdgeColor();
                        ci.cc_setting.has_edge_type = userStyle.hasEdgeType();
                        ci.cc_setting.edge_type = userStyle.edgeType;
                        ci.cc_setting.edge_color = userStyle.edgeColor;
                        ci.cc_setting.foreground_color = userStyle.foregroundColor;
                        ci.cc_setting.foreground_opacity = userStyle.foregroundColor >>> 24;
                        ci.cc_setting.background_color = userStyle.backgroundColor;
                        ci.cc_setting.background_opacity = userStyle.backgroundColor >>> 24;
                        ci.cc_setting.window_color = userStyle.windowColor;
                        ci.cc_setting.window_opacity = userStyle.windowColor >>> 24;
                        /* Typeface is obsolete, we use local font */
                        ci.cc_setting.type_face = userStyle.getTypeface();
                    }
                });
                ci.cc_setting.UpdateCcSetting(captioningManager);
            }
            Log.e(TAG, "subtitle view init");
            init_count += 1;
            teletext_have_data = false;
        }
    }

    private void reset_bitmap_to_black()
    {
        if (bitmap != null) {
            Canvas newcan = new Canvas(bitmap);
            newcan.drawColor(Color.BLACK);
        }
    }

    public DTVSubtitleView(Context context) {
        super(context);
        init(context);
    }

    public DTVSubtitleView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(context);
    }

    public DTVSubtitleView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        init(context);
    }

    public void setMargin(int left, int top, int right, int bottom) {
        disp_left   = left;
        disp_top    = top;
        disp_right  = right;
        disp_bottom = bottom;
    }

    public void setActive(boolean active) {
        synchronized (lock) {
            if (active && (activeView != this) && (activeView != null)) {
                activeView.stopDecoder();
                activeView.active = false;
            }
            subtitlemanager_set_active(active);
            this.active = active;
            if (active) {
                activeView = this;
                /*}else if (activeView == this){
                  activeView = null;*/
            }
            if (!isPreWindowMode) {
                postInvalidate();
            }
        }
    }

    public void setSubParams(ATVTTParams params) {
        synchronized(lock) {
            sub_params.mode = MODE_ATV_TT;
            sub_params.atv_tt = params;

            if (play_mode == PLAY_TT)
                startSub();
        }
    }

    public void setSubParams(Scte27Params params) {
        synchronized(lock) {
            sub_params.mode = MODE_SCTE27_SUB;
            sub_params.scte27_sub = params;

            if (play_mode == PLAY_SUB)
                startSub();
        }
    }

    public void setSubParams(DVBSubParams params) {
        synchronized(lock) {
            sub_params.mode = MODE_DVB_SUB;
            sub_params.dvb_sub = params;

            if (play_mode == PLAY_SUB)
                startSub();
        }
    }

    public void setSubParams(ISDBParams params) {
        synchronized(lock) {
            sub_params.mode = MODE_ISDB_CC;
            sub_params.isdb_cc = params;
            if (play_mode == PLAY_SUB)
                startSub();
        }
    }

    public void setSubParams(DTVTTParams params) {
        synchronized(lock) {
            sub_params.mode = MODE_DTV_TT;
            sub_params.dtv_tt = params;

            if (play_mode == PLAY_TT)
                startSub();
        }
    }

    public void setSubParams(DTVCCParams params) {
        synchronized(lock) {
            sub_params.mode = MODE_DTV_CC;
            sub_params.dtv_cc = params;
            sub_params.vfmt = params.vfmt;

            if (play_mode == PLAY_SUB)
                startSub();
        }
    }

    public void setSubParams(ATVCCParams params) {
        synchronized(lock) {
            sub_params.mode = MODE_ATV_CC;
            sub_params.atv_cc = params;

            if (play_mode == PLAY_SUB)
                startSub();
        }
    }

    public void setSubParams(AVCCParams params) {
        synchronized(lock) {
            sub_params.mode = MODE_AV_CC;
            sub_params.av_cc = params;

            if (play_mode == PLAY_SUB)
                startSub();
        }
    }

    public void setTTParams(DTVTTParams params) {
        synchronized(lock) {
            if (play_mode == PLAY_TT)
                startSub();
        }
    }

    public void setCaptionParams(DTVCCParams params) {
        synchronized(lock) {
            sub_params.dtv_cc = params;
            subtitlemanager_set_atsc_cc_options(
                    params.fg_color,
                    params.fg_opacity,
                    params.bg_color,
                    params.bg_opacity,
                    params.font_style,
                    new Float(params.font_size).intValue());
        }
    }

    public void show() {
        if (visible)
            return;

        handler.obtainMessage(SUB_VIEW_SHOW, true).sendToTarget();
        update();
    }

    public void hide() {
        if (!visible)
            return;

        handler.obtainMessage(SUB_VIEW_SHOW, false).sendToTarget();
        update();
    }

    public void DTVSubtitleTune(int type, int param1) {
        Log.e(TAG, "DTVSubtitleTune " + type + " " + param1);
        if (type == DECODE_ID_TYPE)
            mSubtitleManager.SubtitleTune(DECODE_ID_TYPE, param1, 0, 0);
        if (type == SYNC_ID_TYPE)
            mSubtitleManager.SubtitleTune(SYNC_ID_TYPE, param1, 0, 0);
    }

    public void startSub() {
        Log.e(TAG, "startSub " + play_mode + " " + sub_params.mode);
        synchronized(lock) {
            if (activeView != this)
                return;

            stopDecoder();

            if (sub_params.mode == MODE_NONE)
                return;
            int ret = 0;
            switch (sub_params.mode) {
                case MODE_DVB_SUB:
                    ret = subtitlemanager_start_dvb_sub(sub_params.dvb_sub.dmx_id,
                            sub_params.dvb_sub.pid,
                            sub_params.dvb_sub.composition_page_id,
                            sub_params.dvb_sub.ancillary_page_id);
                    if (ret >= 0) {
                        play_mode = PLAY_SUB;
                    }
                    mSubtitleManager.SubtitleTune(DECODE_ID_TYPE, sub_params.dvb_sub.player_instance_id, 0, 0);
                    mSubtitleManager.SubtitleTune(SYNC_ID_TYPE, sub_params.dvb_sub.sync_instance_id, 0, 0);
                    break;
                case MODE_ATV_TT:
                    reset_bitmap_to_black();
                    Log.e(TAG, "subtitlemanager_start_atv_tt");
                    tt_notify_status = TT_NOTIFY_SEARCHING;
                    notify_pgno = 0;
                    postInvalidate();
                    handler.sendEmptyMessageDelayed(TT_NO_SIGAL, TT_DETECT_TIMEOUT);
                    ret = subtitlemanager_start_atv_tt(
                            sub_params.atv_tt.region_id,
                            sub_params.atv_tt.page_no,
                            sub_params.atv_tt.sub_page_no,
                            false);
                    if (ret >= 0) {
                        play_mode = PLAY_TT;
                    }
                    break;
                case MODE_DTV_TT:
                    boolean is_subtitle;
                    if (sub_params.dtv_tt.type == ChannelInfo.Subtitle.TYPE_DTV_TELETEXT_IMG)
                        is_subtitle = false;
                    else
                        is_subtitle = true;
                    tt_notify_status = TT_NOTIFY_SEARCHING;
                    postInvalidate();
                    handler.sendEmptyMessageDelayed(TT_NO_SIGAL, TT_DETECT_TIMEOUT);
                    ret = subtitlemanager_start_dtv_tt(sub_params.dtv_tt.dmx_id,
                            sub_params.dtv_tt.region_id,
                            sub_params.dtv_tt.pid,
                            sub_params.dtv_tt.page_no,
                            sub_params.dtv_tt.sub_page_no,
                            is_subtitle);
                    if (ret >= 0) {
                        play_mode = PLAY_TT;
                    }
                    mSubtitleManager.SubtitleTune(DECODE_ID_TYPE, sub_params.dtv_tt.player_instance_id, 0, 0);
                    mSubtitleManager.SubtitleTune(SYNC_ID_TYPE, sub_params.dtv_tt.sync_instance_id, 0, 0);
                    break;
                case MODE_DTV_CC:
                    ret = subtitlemanager_start_atsc_cc(
                            0, sub_params.vfmt,
                            sub_params.dtv_cc.caption_mode,
                            sub_params.dtv_cc.decoder_param,
                            sub_params.dtv_cc.lang,
                            sub_params.dtv_cc.fg_color,
                            sub_params.dtv_cc.fg_opacity,
                            sub_params.dtv_cc.bg_color,
                            sub_params.dtv_cc.bg_opacity,
                            sub_params.dtv_cc.font_style,
                            new Float(sub_params.dtv_cc.font_size).intValue());
                    if (ret >= 0) {
                        play_mode = PLAY_SUB;
                    }
                    mSubtitleManager.SubtitleTune(DECODE_ID_TYPE, sub_params.dtv_cc.player_instance_id, 0, 0);
                    mSubtitleManager.SubtitleTune(SYNC_ID_TYPE, sub_params.dtv_cc.sync_instance_id, 0, 0);
                    break;
                case MODE_ATV_CC:
                    ret = subtitlemanager_start_atsc_cc(
                            1, sub_params.vfmt, sub_params.atv_cc.caption_mode,
                            sub_params.atv_cc.decoder_param,
                            sub_params.atv_cc.lang,
                            sub_params.atv_cc.fg_color,
                            sub_params.atv_cc.fg_opacity,
                            sub_params.atv_cc.bg_color,
                            sub_params.atv_cc.bg_opacity,
                            sub_params.atv_cc.font_style,
                            new Float(sub_params.atv_cc.font_size).intValue());
                    if (ret >= 0) {
                        play_mode = PLAY_SUB;
                    }
                    break;
                case MODE_AV_CC:
                    ret = subtitlemanager_start_atsc_cc(
                            1, sub_params.vfmt, sub_params.av_cc.caption_mode,
                            sub_params.av_cc.decoder_param,
                            sub_params.av_cc.lang,
                            sub_params.av_cc.fg_color,
                            sub_params.av_cc.fg_opacity,
                            sub_params.av_cc.bg_color,
                            sub_params.av_cc.bg_opacity,
                            sub_params.av_cc.font_style,
                            new Float(sub_params.av_cc.font_size).intValue());
                    if (ret >= 0) {
                        play_mode = PLAY_SUB;
                    }
                    break;
                case MODE_ISDB_CC:
                    ret = subtitlemanager_start_isdbt(
                            sub_params.isdb_cc.dmx_id,
                            sub_params.isdb_cc.pid,
                            sub_params.isdb_cc.caption_id);
                    if (ret >= 0) {
                        play_mode = PLAY_SUB;
                    }
                    break;
                case MODE_SCTE27_SUB:
                    subtitlemanager_start_scte27(sub_params.scte27_sub.dmx_id, sub_params.scte27_sub.pid);
                    if (ret >= 0) {
                        play_mode = PLAY_SUB;
                    }
                    break;
                default:
                    ret = -1;
                    break;
            }

            if (ret >= 0) {
                mSubtitleManagerListener = new SubtitleManagerListener();
                mSubtitleManager.setSubtitleDataListner(mSubtitleManagerListener);
                cc_is_started = true;
            }
        }
    }

    public void stop() {
        synchronized(lock) {
            Log.d(TAG, "subtitleView stop");
            if (activeView != this) {
                Log.e(TAG, "activeView not this");
                return;
            }
            stopDecoder();
            mMixMode = TTX_MIX_MODE_NORAML;
            Log.d(TAG, "subtitleView stop end");
        }
    }

    public void clear() {
        synchronized(lock) {
            if (activeView != this)
                return;

            handler.obtainMessage(SUB_VIEW_CLEAN).sendToTarget();
            //stopDecoder();
            subtitlemanager_clear();
            sub_params.mode = MODE_NONE;
        }
    }

    public void nextPage() {
        synchronized(lock) {
            if (activeView != this)
                return;
            if (play_mode != PLAY_TT)
                return;
            subtitlemanager_tt_control(SubtitleManager.TT_EVENT_NEXTPAGE, -1, -1);
        }
    }

    public void previousPage() {
        synchronized(lock) {
            if (activeView != this)
                return;
            if (play_mode != PLAY_TT)
                return;
            Log.e(TAG, "previousPage");

            subtitlemanager_tt_control(SubtitleManager.TT_EVENT_PREVIOUSPAGE, -1, -1);
        }
    }

    public void gotoPage(int page) {
        final int TT_ANY_SUBNO = 0x3F7F;
        synchronized(lock) {
            if (activeView != this)
                return;
            if (play_mode != PLAY_TT)
                return;

            subtitlemanager_tt_control(SubtitleManager.TT_EVENT_GO_TO_PAGE, page, TT_ANY_SUBNO);
        }
    }
    public void gotoPage(int page, int subpg) {
        synchronized(lock) {
            if (activeView != this)
                return;
            if (play_mode != PLAY_TT)
                return;

            subtitlemanager_tt_control(SubtitleManager.TT_EVENT_GO_TO_PAGE, page, subpg);
        }
    }

    public void goHome() {
        synchronized(lock) {
            if (activeView != this)
                return;
            if (play_mode != PLAY_TT)
                return;

            subtitlemanager_tt_control(SubtitleManager.TT_EVENT_INDEXPAGE, -1, -1);
        }
    }

    public void colorLink(int color) {
        synchronized(lock) {
            if (activeView != this)
                return;
            if (play_mode != PLAY_TT)
                return;
            Log.e(TAG, "colorlink");

            subtitlemanager_tt_control(color, -1, -1);
        }
    }

    public void setSearchPattern(String pattern, boolean casefold) {
        synchronized(lock) {
            if (activeView != this)
                return;
            if (play_mode != PLAY_TT)
                return;

            subtitlemanager_tt_set_search_pattern(pattern, casefold);
        }
    }

    public int setTTRegion(int region_id)
    {
        Log.e(TAG, "setTTRegion " + region_id);
        DataProviderManager.putIntValue(getContext(), TT_REGION_DB, region_id);
        return subtitlemanager_tt_set_region(region_id);
    }

    public void setTTRevealMode() {
        synchronized(lock) {
            if (activeView != this)
                return;
            if (play_mode != PLAY_TT)
                return;

            subtitlemanager_tt_control(SubtitleManager.TT_EVENT_REVEAL, -1, -1);
        }
    }

    public void setTTSubpgLock() {
        synchronized(lock) {
            if (activeView != this)
                return;
            if (play_mode != PLAY_TT)
                return;

            subtitlemanager_tt_control(SubtitleManager.TT_EVENT_HOLD, -1, -1);
        }
    }

    public void tt_goto_subtitle()
    {
        synchronized(lock) {
            if (activeView != this)
                return;
            if (play_mode != PLAY_TT)
                return;

            subtitlemanager_tt_control(SubtitleManager.TT_EVENT_GO_TO_SUBTITLE, -1, -1);
        }
    }

    public void tt_get_nextsubpage() {
        synchronized(lock) {
            if (activeView != this)
                return;
            if (play_mode != PLAY_TT)
                return;

            subtitlemanager_tt_control(SubtitleManager.TT_EVENT_NEXTSUBPAGE, -1, -1);
        }
    }

    public void tt_handle_number_event(int number)
    {
        int start_number_event = SubtitleManager.TT_EVENT_0;
        synchronized(lock) {
            if (activeView != this)
                return;
            if (play_mode != PLAY_TT)
                return;

            subtitlemanager_tt_control(number + start_number_event, -1, -1);
        }
    }

    public void tt_switch_mix_mode(int mix_mode)
    {
        synchronized(lock) {
            if (activeView != this)
                return;
            if (play_mode != PLAY_TT)
                return;
            mMixMode = mix_mode;
            Log.d(TAG, "MixMode = " + mMixMode);
            subtitlemanager_tt_control(SubtitleManager.TT_EVENT_MIX_VIDEO, -1, -1);
            update();
        }
    }

    public void tt_switch_clock_mode()
    {
        synchronized(lock) {
            if (activeView != this)
                return;
            if (play_mode != PLAY_TT)
                return;

            subtitlemanager_tt_control(SubtitleManager.TT_EVENT_CLOCK, -1, -1);
        }
    }

    public void tt_clear()
    {
        synchronized(lock) {
            if (activeView != this)
                return;
            if (play_mode != PLAY_TT)
                return;

            subtitlemanager_tt_control(SubtitleManager.TT_EVENT_CLEAR, -1, -1);
        }
    }

    private boolean tt_show_switch = true;
    public void setTTSwitch(boolean show)
    {
        tt_show_switch = show;
        if (!tt_show_switch)
            need_clear_canvas = true;
        postInvalidate();
    }

    public void tt_reset_status()
    {
    }

    public void searchNext() {
        synchronized(lock) {
            if (activeView != this)
                return;
            if (play_mode != PLAY_TT)
                return;

            subtitlemanager_tt_control(SubtitleManager.TT_EVENT_NEXTPAGE, -1, -1);
        }
    }

    public void searchPrevious() {
        synchronized(lock) {
            if (activeView != this)
                return;
            if (play_mode != PLAY_TT)
                return;

            subtitlemanager_tt_control(SubtitleManager.TT_EVENT_PREVIOUSPAGE, -1, -1);
        }
    }

    public void reset_atv_status()
    {
        synchronized (lock) {
            tt_zoom_state = 0;
            tt_reveal_mode = false;
        }
        postInvalidate();
    }

    public void tt_zoom_in()
    {
        synchronized(lock) {
            if (activeView != this)
                return;
            if (play_mode != PLAY_TT)
                return;

            subtitlemanager_tt_control(SubtitleManager.TT_EVENT_DOUBLE_HEIGHT, -1, -1);
        }

    }

    public void tt_stop_refresh()
    {
        tt_refresh_switch = !tt_refresh_switch;
    }

    /**
     * set the flag to indecate the preview window mode
     * @param flag [description]
     */
    public void setPreviewWindowMode(boolean flag) {
        isPreWindowMode = flag;
    }

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        super.onSizeChanged(w, h, oldw, oldh);
        ci.caption_screen.updateCaptionScreen(w, h);
    }

    boolean valid_page_no(int pgno)
    {
        if (pgno < 899 && pgno >= 100)
            return true;
        else
            return false;
    }

    @Override
    public void onDraw(Canvas canvas) {
        Rect sr, dr;
        String json_data;
        String screen_mode;
        String video_status;
        String ratio;
        String video_frame_width;
        String video_frame_height;
        int row_height = 0;
        int navi_left = 0;
        int navi_right = 0;

        if (!ci.cc_setting.is_enabled)
            return;

        if (need_clear_canvas) {
            /* Clear canvas */
            canvas.drawPaint(clear_paint);
            json_str = null;
            need_clear_canvas = false;
            return;
        }
        //Log.e(TAG, "onDraw active " + active + " visible " + visible + " pm " + play_mode);
        if (!active || !visible || (play_mode == PLAY_NONE)) {
            return;
        }

        synchronized(lock) {
            switch (sub_params.mode)
            {
                case MODE_AV_CC:
                case MODE_DTV_CC:
                case MODE_ATV_CC:
                    /* For atsc */
                    screen_mode = mSystemControlManager.readSysFs("/sys/class/video/screen_mode");
                    video_status = mSystemControlManager.readSysFs("/sys/class/video/video_state");
                    //ratio = mSystemControlManager.readSysFs("/sys/class/video/frame_aspect_ratio");
                    video_frame_width = mSystemControlManager.readSysFs("/sys/class/video/frame_width");
                    video_frame_height = mSystemControlManager.readSysFs("/sys/class/video/frame_height");
                    if (("NA".equals(video_frame_width) || "NA".equals(video_frame_height))
                        || video_frame_width.isEmpty() || video_frame_height.isEmpty()) {
                        ratio = "0x90";
                    } else {
                        int frame_width = Integer.parseInt(video_frame_width);
                        int frame_height = Integer.parseInt(video_frame_height);
                        if (frame_width * 9 >= frame_height * 16) {
                            ratio = "0x90";
                        } else {
                            ratio = "0x01";
                        }
                    }
                    cw.UpdatePositioning(ratio, screen_mode, video_status);
                    ci.caption_screen.updateCaptionScreen(canvas.getWidth(), canvas.getHeight());
                    cw.style_use_broadcast = ci.isStyle_use_broadcast();

                    cw.updateCaptionWindow(json_str);
                    if (cw.init_flag)
                        decoding_status = true;
                    if (decoding_status)
                        cw.draw(canvas);
                    break;
                case MODE_DTV_TT:
                case MODE_ATV_TT:
                    if (bitmap == null) return;
                    sr = new Rect(0, 0, bitmap.getWidth(), bitmap.getHeight());
                    if (mMixMode == TTX_MIX_MODE_LEFT_RIGHT) {
                        dr = new Rect(getWidth()/2, 0, getWidth(), getHeight());
                    } else {
                        dr = new Rect(0, 0, getWidth(), getHeight());
                    }
                    canvas.setDrawFilter(new PaintFlagsDrawFilter(0, Paint.ANTI_ALIAS_FLAG | Paint.FILTER_BITMAP_FLAG));
                    canvas.drawBitmap(bitmap, sr, dr, null);
                    //bitmap.recycle();
                   // bitmap = null;
                    break;
                case MODE_DVB_SUB:
                case MODE_SCTE27_SUB:
                    if (bitmap == null) return;
                    sr = new Rect(0, 0, bitmap.getWidth(), bitmap.getHeight());
                    dr = new Rect(0, 0, getWidth(), getHeight());
                    canvas.setDrawFilter(new PaintFlagsDrawFilter(0, Paint.ANTI_ALIAS_FLAG | Paint.FILTER_BITMAP_FLAG));
                    canvas.drawBitmap(bitmap, sr, dr, mPaint);
                    bitmap.recycle();
                    bitmap = null;
                    break;
                case MODE_ISDB_CC:
                    screen_mode = mSystemControlManager.readSysFs("/sys/class/video/screen_mode");
                    video_status = mSystemControlManager.readSysFs("/sys/class/video/video_state");
                    ratio = mSystemControlManager.readSysFs("/sys/class/video/frame_aspect_ratio");
                    isdbi.updateVideoPosition(ratio, screen_mode, video_status);
                    isdbi.draw(canvas, json_str);
                    break;
                default:
                    break;
            };
        }
    }

    protected void finalize() throws Throwable {
        Log.e(TAG, "Finalize");
        super.finalize();
    }

    public void setVisible(boolean value) {
        Log.d(TAG, "force set visible to:" + value);
        handler.obtainMessage(SUB_VIEW_SHOW, value).sendToTarget();
        postInvalidate();
    }

    private SubtitleDataListener mSubtitleDataListener = null;
    Handler handler = new Handler()
    {
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case JSON_MSG_NORMAL:
                    json_str = (String)msg.obj;
                    if (play_mode != PLAY_NONE)
                        postInvalidate();
                    break;
                case SUB_VIEW_SHOW:
                    visible = (Boolean) msg.obj;
                    if (!visible) {
                        json_str = null;
                        need_clear_canvas = true;
                        decoding_status = false;
                    }
                    postInvalidate();
                    break;
                case SUB_VIEW_CLEAN:
                    json_str = null;
                    need_clear_canvas = true;
                    decoding_status = false;
                    postInvalidate();
                    break;
                case TT_STOP_REFRESH:
                    postInvalidate();
                    break;
                case TT_ZOOM:
                    tt_zoom_state = (int)msg.obj;
                    postInvalidate();
                    break;
                case TT_PAGE_TYPE:
                    tt_page_type = (int)msg.obj;
                    break;
                case TT_NO_SIGAL:
                    tt_notify_status = TT_NOTIFY_NOSIG;
                    postInvalidate();
                    break;
                default:
                    Log.e(TAG, "Wrong message what");
                    break;
            }
        }
    };

    public void saveJsonStr(String str) {
        if (activeView != this) {
            return;
        }

        if (!TextUtils.isEmpty(str)) {
            handler.obtainMessage(JSON_MSG_NORMAL, str).sendToTarget();
        }
    }

    public void updateData(String json) {
        if (mSubtitleDataListener != null) {
            mSubtitleDataListener.onSubtitleData(json);
        }
    }

    public String readSysFs(String name) {
        String value = null;
        if (mSubtitleDataListener != null) {
            value = mSubtitleDataListener.onReadSysFs(name);
        }
        return value;
    }

    public void writeSysFs(String name, String cmd) {
        if (mSubtitleDataListener != null) {
            mSubtitleDataListener.onWriteSysFs(name, cmd);
        }
    }

    public void setSubtitleDataListener(SubtitleDataListener l) {
        mSubtitleDataListener = l;
    }

    public interface SubtitleDataListener {
        public void onSubtitleData(String json);
        public String onReadSysFs(String node);
        public void onWriteSysFs(String node, String value);
    }

    public class SubtitleManagerListener implements SubtitleManager.SubtitleDataListener {
        @Override
        public void onSubtitleEvent(int type, Object data, byte[] subdata, int x, int y,
            int width ,int height, int videoW, int videoH, boolean showOrHide) {
            //Log.d(TAG, "DTVSubtitleView type:" + type+"; height=" +height+"; width="+width+",videoW:"+videoW+",videoH:"+videoH+", show="+showOrHide);
            synchronized(lock) {
                if (type == SubtitleManager.SUBTITLE_CC_JASON) {
                    saveJsonStr(new String(subdata));
                } else if (type == SubtitleManager.SUBTITLE_VCHIP_RATE) {
                    String rating_json = "";
                    if ((x ==- 1) && (y == -1) && (width == -1))
                        rating_json = "{Aratings:{},cc:{data:" + height + "}}";
                    else
                        rating_json = "{Aratings:{g:" + x + ",i:" + y + ",dlsv:" + width + "}}";
                    if (mSubtitleDataListener != null)
                        mSubtitleDataListener.onSubtitleData(rating_json);
                } else {
                    int dis_w = videoW;
                    int dis_h = videoH;
                    int src_x = x;
                    int src_y = y;
                    int widtmp = width;
                    int heitmp = height;
                    if (type == SubtitleManager.SUBTITLE_IMAGE_CENTER) {
                        dis_w = widtmp;//720;
                        dis_h = heitmp;//576;
                        //widtmp = 480;
                        //heitmp = 525;
                        //display full screen
                        src_x = 0;//(dis_w - widtmp)/2;
                        src_y = 0;//(dis_h - heitmp)/2;
                    }
                    if (showOrHide == true
                        && width > 0
                        && height > 0
                        && dis_w > 0
                        && dis_h > 0) {
                        int[] colors = (int [])data;
                        if (bitmap != null)
                            bitmap.recycle();
                        bitmap = Bitmap.createBitmap(dis_w, dis_h, Bitmap.Config.ARGB_8888);
                        Canvas canvas = new Canvas(bitmap);
                        Log.d(TAG, "mixmode = " + mMixMode);
                        //if ((type == SubtitleManager.SUBTITLE_IMAGE_CENTER) && mMixMode == TTX_MIX_MODE_NORAML)
                        //    canvas.drawColor(0xFF000000);
                        Bitmap databm = Bitmap.createBitmap(colors, 0, width, width, height, Bitmap.Config.ARGB_8888);
                        Rect src = new Rect(0, 0, width, height);
                        Rect dsc = new Rect(src_x, src_y, src_x + widtmp, src_y + heitmp);
                        canvas.drawBitmap(databm, src, dsc, null);
                        databm.recycle();
                    } else {
                        need_clear_canvas = true;
                    }
                    update();
                }
            }
        }
    }

}

