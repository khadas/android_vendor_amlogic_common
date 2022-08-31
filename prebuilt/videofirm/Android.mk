#####################################################
# amlogic video decoder firmware
####################################################
LOCAL_PATH := $(call my-dir)
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
OUT_PATH := $(TARGET_OUT_VENDOR)
else
OUT_PATH := $(TARGET_OUT)/
endif


include $(CLEAR_VARS)
LOCAL_MODULE := libtee_load_video_fw
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_SUFFIX := .so
LOCAL_MULTILIB := both
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH_64 := $(OUT_PATH)/lib64/
LOCAL_MODULE_PATH_32 := $(OUT_PATH)/lib/
LOCAL_SRC_FILES_arm :=  ca/lib/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)
LOCAL_SRC_FILES_arm64 := ca/lib64/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)
LOCAL_STRIP_MODULE := false
LOCAL_SHARED_LIBRARIES := libteec
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := tee_preload_fw
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH_64 := $(OUT_PATH)/bin
LOCAL_MODULE_PATH_32 := $(OUT_PATH)/bin
LOCAL_SRC_FILES_arm := ca/bin/$(LOCAL_MODULE)
LOCAL_SRC_FILES_arm64 := ca/bin64/$(LOCAL_MODULE)
LOCAL_INIT_RC := ca/rc/tee_preload_fw.rc
include $(BUILD_PREBUILT)

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
LOCAL_TA := ta/$(TA_UUID)$(TA_SUFFIX)
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

FORCE:

endif
include $(BUILD_PREBUILT)

