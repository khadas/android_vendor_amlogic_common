LOCAL_PATH:= $(call my-dir)


#MAKE_XML
include $(CLEAR_VARS)
LOCAL_MODULE := droidlogic.software.pppoe.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC

LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/permissions

LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)


include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_JAVA_LIBRARIES := droidlogic.external.pppoe droidlogic.frameworks.pppoe droidlogic

LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_PACKAGE_NAME := PPPoE
LOCAL_CERTIFICATE := platform

LOCAL_PRIVATE_PLATFORM_APIS := true
#LOCAL_SDK_VERSION := current

include $(BUILD_PACKAGE)

# Use the folloing include to make our test apk.
include $(call all-makefiles-under,$(LOCAL_PATH))

