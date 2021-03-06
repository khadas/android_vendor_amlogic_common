LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    com_droidlogic_common.cpp \
    com_droidlogic_pppoe.cpp\

LOCAL_C_INCLUDES += \
    libnativehelper/include/nativehelper


LOCAL_SHARED_LIBRARIES := \
    liblog \
    libcutils \
    libgui \
    libnativehelper \
    libandroid_runtime

LOCAL_MODULE    := libpppoe

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
#LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)

