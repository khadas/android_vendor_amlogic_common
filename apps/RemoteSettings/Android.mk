LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_STATIC_JAVA_LIBRARIES := gson
LOCAL_JAVA_LIBRARIES := droidlogic
LOCAL_SRC_FILES := $(call all-java-files-under, src)
LOCAL_PACKAGE_NAME := RemoteSettings
LOCAL_CERTIFICATE := platform
LOCAL_PROGUARD_ENABLED := disabled

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 28 && echo OK),OK)
LOCAL_PRIVATE_PLATFORM_APIS := true
else
LOCAL_SDK_VERSION := current
endif

include $(BUILD_PACKAGE)

##############################################

include $(CLEAR_VARS)
LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES := gson:libs/gson.jar

ifeq (($(shell test $(PLATFORM_SDK_VERSION) -ge 26 ) && ($(shell test $(PLATFORM_SDK_VERSION) -lt 28)  && echo OK),OK))
LOCAL_PROPRIETARY_MODULE := true
endif
include $(BUILD_MULTI_PREBUILT)
