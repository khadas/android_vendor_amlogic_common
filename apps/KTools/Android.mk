#################Copy apks to /system/app/###############
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE        := KTools
LOCAL_MODULE_OWNER  := khadas
LOCAL_MODULE_TAGS   := optional
LOCAL_MODULE_CLASS  := APPS
LOCAL_CERTIFICATE   := platform
LOCAL_MODULE_SUFFIX := .apk
LOCAL_DEX_PREOPT    := true
LOCAL_SRC_FILES     := KTools.apk
LOCAL_MODULE_PATH   := $(PRODUCT_OUT)/system/priv-app
include $(BUILD_PREBUILT)


