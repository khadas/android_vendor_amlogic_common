/*
 * Copyright (c) 2023 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
package vendor.amlogic.hardware.vmx_webclient;

import android.hardware.common.NativeHandle;
import vendor.amlogic.hardware.vmx_webclient.Mode;
import vendor.amlogic.hardware.vmx_webclient.Pattern;
import vendor.amlogic.hardware.vmx_webclient.SubSample;

/**
 * VmxWebClientDecryptParam describes a decrypt
 */
parcelable VmxWebClientDecryptParam
{
    int secure;
    int sampleAES;
    int keySeq;
    byte[] key;
    byte[] iv;
    Mode mode;
    Pattern pattern;
    SubSample[] subSamples;
    byte[] src;
    byte[] dst;
    NativeHandle sourceDesc;
    NativeHandle secureDesc;
    long srcOffset;
    long offset;
}
