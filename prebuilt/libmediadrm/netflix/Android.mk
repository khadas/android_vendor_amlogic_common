LOCAL_PATH:= $(call my-dir)
ifeq ($(TARGET_BUILD_NETFLIX_MGKID), true)
#####################################################################
# libnetflixplugin.so
include $(CLEAR_VARS)
LOCAL_MODULE := libnetflixplugin
LOCAL_MULTILIB := both
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0 SPDX-license-identifier-BSD SPDX-license-identifier-LGPL legacy_by_exception_only
LOCAL_LICENSE_CONDITIONS := by_exception_only notice restricted
LOCAL_PROPRIETARY_MODULE := true
ifneq (0, $(shell expr $(PLATFORM_SDK_VERSION) \>= 30))
LOCAL_MODULE_PATH_32 := $(TARGET_OUT_VENDOR)/lib/
LOCAL_MODULE_PATH_64 := $(TARGET_OUT_VENDOR)/lib64/
else
LOCAL_MODULE_PATH_32 := $(TARGET_OUT_VENDOR)/lib/mediadrm
LOCAL_MODULE_PATH_64 := $(TARGET_OUT_VENDOR)/lib64/mediadrm
endif
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES_32 := libnetflixplugin.so
LOCAL_SRC_FILES_64 := arm64/libnetflixplugin.so
LOCAL_PROPRIETARY_MODULE := true
LOCAL_STRIP_MODULE := false
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := libcutils liblog libteec libutils
LOCAL_CHECK_ELF_FILES := false
include $(BUILD_PREBUILT)

# 00d1ca22-1764-4e35-90aa-5b8c12630764.ta
include $(CLEAR_VARS)
TA_UUID := 00d1ca22-1764-4e35-90aa-5b8c12630764
TA_SUFFIX := .ta

ifeq ($(PLATFORM_TDK_VERSION), 38)
PLATFORM_TDK_PATH := $(BOARD_AML_VENDOR_PATH)/tdk_v3
ifeq ($(BOARD_AML_SOC_TYPE),)
LOCAL_TA := ta/v3/$(TA_UUID)$(TA_SUFFIX)
else
LOCAL_TA := ta/v3/dev/$(BOARD_AML_SOC_TYPE)/$(TA_UUID)$(TA_SUFFIX)
endif
else
PLATFORM_TDK_PATH := $(BOARD_AML_VENDOR_PATH)/tdk
LOCAL_TA := ta/v2/$(TA_UUID)$(TA_SUFFIX)
endif

ifeq ($(TARGET_ENABLE_TA_ENCRYPT), true)
ENCRYPT := 1
else
ENCRYPT := 0
endif

LOCAL_SRC_FILES := $(LOCAL_TA)
LOCAL_MODULE := $(TA_UUID)
LOCAL_32_BIT_ONLY := true
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0 SPDX-license-identifier-BSD SPDX-license-identifier-LGPL legacy_by_exception_only
LOCAL_LICENSE_CONDITIONS := by_exception_only notice restricted
LOCAL_MODULE_SUFFIX := $(TA_SUFFIX)
LOCAL_STRIP_MODULE := false
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/lib/teetz

ifeq ($(TARGET_ENABLE_TA_SIGN), true)
LOCAL_POST_INSTALL_CMD = $(PLATFORM_TDK_PATH)/ta_export/scripts/sign_ta_auto.py \
		--in=$(shell pwd)/$(LOCAL_MODULE_PATH)/$(TA_UUID)$(LOCAL_MODULE_SUFFIX) \
		--keydir=$(shell pwd)/$(BOARD_AML_TDK_KEY_PATH) \
		--encrypt=$(ENCRYPT)
endif
include $(BUILD_PREBUILT)

endif

# -----------------------------------------------------------------------------
# Builds android.hardware.drm@1.3-service.netflix
#
ifneq (0, $(shell expr $(PLATFORM_SDK_VERSION) \>= 30))
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/bin/hw
LOCAL_MODULE := android.hardware.drm@1.3-service.netflix
ifneq ($(TARGET_ARCH),arm64)
LOCAL_SRC_FILES := $(LOCAL_MODULE)
else
LOCAL_SRC_FILES := arm64/$(LOCAL_MODULE)
endif
LOCAL_INIT_RC := android.hardware.drm@1.3-service.netflix.rc
LOCAL_VINTF_FRAGMENTS := manifest_android.hardware.drm@1.3-service.netflix.xml
LOCAL_CHECK_ELF_FILES := false
include $(BUILD_PREBUILT)
endif
