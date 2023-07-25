/*
 * Copyright (c) 2023 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
package vendor.amlogic.hardware.vmx_webclient;

enum Status {

    OK,

    ERROR_DRM_NO_LICENSE,

    ERROR_DRM_LICENSE_EXPIRED,

    ERROR_DRM_SESSION_NOT_OPENED,

    ERROR_DRM_CANNOT_HANDLE,

    ERROR_DRM_INVALID_STATE,

    BAD_VALUE,

    ERROR_DRM_NOT_PROVISIONED,

    ERROR_DRM_RESOURCE_BUSY,

    ERROR_DRM_INSUFFICIENT_OUTPUT_PROTECTION,

    ERROR_DRM_DEVICE_REVOKED,

    ERROR_DRM_DECRYPT,

    ERROR_DRM_UNKNOWN,
}
