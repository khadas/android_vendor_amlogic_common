
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(call all-java-files-under, src)
LOCAL_PACKAGE_NAME := FactoryTest
LOCAL_CERTIFICATE := platform
LOCAL_JNI_SHARED_LIBRARIES := libserial_port_jni
#LOCAL_DEX_PREOPT := false


LOCAL_PRIVATE_PLATFORM_APIS := true

include $(BUILD_PACKAGE)

include $(call all-makefiles-under, $(LOCAL_PATH))