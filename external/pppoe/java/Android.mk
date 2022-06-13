LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(call all-subdir-java-files)
LOCAL_JNI_SHARED_LIBRARIES := libpppoejni
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := droidlogic.external.pppoe

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
#LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_JAVA_LIBRARY)

