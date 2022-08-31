/*
 * Copyright (C) 2011 The Android Open Source Project
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
 *  @author   Luan.Yuan
 *  @version  1.0
 *  @date     2017/02/21
 *  @par function description:
 *  - This is Dolby Vision Manager, control DV feature, sysFs, proc env.
 */
package com.droidlogic.app;

import android.content.Context;
import android.provider.Settings;
import android.util.Log;

public class DolbyVisionSettingManager {
    private static final String TAG                 = "DolbyVisionSettingManager";

    private static final String PROP_SUPPORT_DOLBY_VISION    = "vendor.system.support.dolbyvision";
    private static final String PROP_SOC_SUPPORT_DOLBY_VISION    = "ro.vendor.platform.support.dolbyvision";
    private static final String ENV_IS_BEST_MODE             = "ubootenv.var.is.bestmode";
    private static final String ENV_IS_DV_ENABLE             = "ubootenv.var.dv_enable";

    public static final int DOVISION_DISABLE        = 0;
    public static final int DOVISION_ENABLE         = 1;

    private Context mContext;
    private SystemControlManager mSystemControl;

    public DolbyVisionSettingManager(Context context){
        mContext = context;
        mSystemControl = SystemControlManager.getInstance();
    }

    public void initSetDolbyVision() {
       if (isDolbyVisionEnable()) {
            setDolbyVisionEnable(getDolbyVisionType());
        }
    }

    /* *
     * @Description: Enable/Disable Dolby Vision
     * @params: state: 1:Enable  DV
     *                 0:Disable DV
     */
    public void setDolbyVisionEnable(int state) {
        mSystemControl.setBootenv(ENV_IS_BEST_MODE, "false");
        mSystemControl.setDolbyVisionEnable(state);
    }

    /* *
     * @Description: Determine Whether TV support Dolby Vision
     * @return: if TV support Dolby Vision
     *              return the Highest resolution Tv supported.
     *          else
     *              return ""
     */
    public String isTvSupportDolbyVision() {
        return mSystemControl.isTvSupportDolbyVision();
    }

    /* *
     * @Description: Determine Whether BOX support Dolby Vision
     * @return: if  BOX support Dolby Vision
     *              return true.
     *          else
     *              return fase
     */
    public boolean isMboxSupportDolbyVision() {
        return mSystemControl.getPropertyBoolean(PROP_SUPPORT_DOLBY_VISION, false);
    }

    /* *
     * @Description: Determine Whether soc support Dolby Vision
     * @return: if  soc support Dolby Vision
     *              return true.
     *          else
     *              return fase
     */
    public boolean isSocSupportDolbyVision() {
        return mSystemControl.getPropertyBoolean(PROP_SOC_SUPPORT_DOLBY_VISION, false);
    }

    /* *
     * @Description: get current state of Dolby Vision
     * @return: if DV is Enable  return true
     *                   Disable return false
     */
    public boolean isDolbyVisionEnable() {
        if (isMboxSupportDolbyVision()) {
            String dv_enable = mSystemControl.getBootenv(ENV_IS_DV_ENABLE, "1");
            if (dv_enable.equals("0")) {
                return false;
            } else {
                return true;
            }
        } else {
            return false;
        }
    }

    public int getDolbyVisionType() {
        return mSystemControl.getDolbyVisionType();
    }
    public long resolveResolutionValue(String mode) {
        return mSystemControl.resolveResolutionValue(mode);
    }

    /* *
     * @Description: set Dolby Vision Graphics Priority when DV is Enabled.
     * @params: "0":Video Priority    "1":Graphics Priority
     */
    public void setGraphicsPriority(String mode) {
        mSystemControl.setGraphicsPriority(mode);
    }

    /* *
     * @Description: set Dolby Vision Graphics Priority when DV is Enabled.
     * @return: current Priority
     */
    public String getGraphicsPriority() {
        return mSystemControl.getGraphicsPriority();
    }
}
