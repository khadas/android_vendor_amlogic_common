#################Copy apks to /system/app/###############
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE        := KTools
LOCAL_MODULE_OWNER  := khadas
LOCAL_MODULE_TAGS   := optional
LOCAL_MODULE_CLASS  := APPS
LOCAL_CERTIFICATE   := platform
LOCAL_MODULE_SUFFIX := .apk
LOCAL_SRC_FILES     := KTools.apk
LOCAL_MODULE_PATH   := $(PRODUCT_OUT)/system_ext/priv-app
LOCAL_PROGUARD_ENABLED := disabled
LOCAL_DEX_PREOPT := false
LOCAL_MULTILIB := 32
include $(BUILD_PREBUILT)


