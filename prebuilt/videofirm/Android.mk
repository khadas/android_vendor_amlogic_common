#####################################################
# amlogic video decoder firmware
####################################################
LOCAL_PATH := $(call my-dir)
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
OUT_PATH := $(TARGET_OUT_VENDOR)
else
OUT_PATH := $(TARGET_OUT)/
endif

ifneq ($(USE_PRESIGNED_TA),true)
include $(CLEAR_VARS)
TA_UUID := 526fc4fc-7ee6-4a12-96e3-83da9565bce8
TA_SUFFIX := .ta

ifeq ($(PLATFORM_TDK_VERSION), 38)
	PLATFORM_TDK_PATH := $(BOARD_AML_VENDOR_PATH)/tdk_v3
	ifeq ($(BOARD_AML_SOC_TYPE),)
		LOCAL_TA := ta/v3/signed/$(TA_UUID)$(TA_SUFFIX)
	else
		LOCAL_TA := ta/v3/dev/$(BOARD_AML_SOC_TYPE)/$(TA_UUID)$(TA_SUFFIX)
	endif
else
	PLATFORM_TDK_PATH := $(BOARD_AML_VENDOR_PATH)/tdk
	LOCAL_TA := ta/v2/signed/$(TA_UUID)$(TA_SUFFIX)
endif

LOCAL_SRC_FILES := $(LOCAL_TA)
LOCAL_MODULE := $(TA_UUID)
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0 SPDX-license-identifier-BSD SPDX-license-identifier-LGPL legacy_by_exception_only legacy_proprietary
LOCAL_LICENSE_CONDITIONS := by_exception_only notice restricted proprietary by_exception_only
LOCAL_MODULE_SUFFIX := $(TA_SUFFIX)
LOCAL_STRIP_MODULE := false
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/lib/teetz

FORCE:

include $(BUILD_PREBUILT)
endif # USE_PRESIGNED_TA != true
