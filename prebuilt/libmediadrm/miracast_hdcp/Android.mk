ifeq ($(wildcard $(TOP)/vendor/amlogic/common/miracast-hdcp2),)
LOCAL_PATH:= $(call my-dir)

ifneq ($(filter arm arm64, $(TARGET_ARCH)),)

include $(CLEAR_VARS)
LOCAL_MODULE := libstagefright_hdcp
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
LOCAL_SRC_FILES := $(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_STRIP_MODULE := false
LOCAL_32_BIT_ONLY := true
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := libc++ libc libcutils libdl liblog libm libteec libutils
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)

TA_UUID := 807798e0-f011-11e5-a5fe-0002a5d5c51b
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
LOCAL_TA := ta/v2/$(TA_UUID)$(TA_SUFFIX)
endif

ifeq ($(TARGET_ENABLE_TA_ENCRYPT), true)
ENCRYPT := 1
else
ENCRYPT := 0
endif

LOCAL_SRC_FILES := $(LOCAL_TA)
LOCAL_MODULE := $(TA_UUID)
LOCAL_MODULE_SUFFIX := $(TA_SUFFIX)
LOCAL_STRIP_MODULE := false
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/lib/teetz
ifneq ($(PLATFORM_TDK_VERSION), 38)
ifeq ($(TARGET_ENABLE_TA_SIGN), true)
LOCAL_POST_INSTALL_CMD = $(PLATFORM_TDK_PATH)/ta_export/scripts/sign_ta_auto.py \
		--in=$(shell pwd)/$(LOCAL_MODULE_PATH)/$(TA_UUID)$(LOCAL_MODULE_SUFFIX) \
		--keydir=$(shell pwd)/$(BOARD_AML_TDK_KEY_PATH) \
		--encrypt=$(ENCRYPT)
endif
endif
include $(BUILD_PREBUILT)

endif # TARGET_ARCH == arm
endif



