LOCAL_PATH:= $(call my-dir)

PPPOE_VERSION="\"3.0\""

include $(CLEAR_VARS)
LOCAL_SRC_FILES := src/pppoe.c \
                   src/if.c \
                   src/debug.c \
                   src/common.c \
                   src/ppp.c \
                   src/discovery.c \
                   src/netwrapper.c
#LOCAL_C_INCLUDES := $(KERNEL_HEADERS)
#LOCAL_C_INCLUDES += external/selinux/libselinux/include/ \
                    external/selinux/libselinux/src

LOCAL_SHARED_LIBRARIES := liblog libcutils libselinux
LOCAL_MODULE = pppoe
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -DVERSION=$(PPPOE_VERSION)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
#LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= src/pppoe_status.c \
        pppoe_jni.cpp \
        src/netwrapper.c


LOCAL_SHARED_LIBRARIES := \
        libcutils \
        liblog \
        libselinux


LOCAL_SHARED_LIBRARIES += libandroid_runtime   libnativehelper
LOCAL_SHARED_LIBRARIES += libc libcutils libnetutils
LOCAL_C_INCLUDES :=  $(JNI_H_INCLUDE) \
    $(LOCAL_PATH)/src \
    libnativehelper/include/nativehelper

#LOCAL_C_INCLUDES += external/selinux/libselinux/include/ \
                    external/selinux/libselinux/src
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libpppoejni
LOCAL_PRELINK_MODULE := false

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
#LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_SRC_FILES:= src/pppoe_cli.c \
        src/common.c \
        src/netwrapper.c

#LOCAL_C_INCLUDES += external/selinux/libselinux/include/ \
                    external/selinux/libselinux/src

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := pcli
LOCAL_SHARED_LIBRARIES := liblog libcutils libnetutils libselinux

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
#LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= src/pppoe_wrapper.c \
        src/common.c \
        src/netwrapper.c

LOCAL_SHARED_LIBRARIES := \
        liblog libcutils libcrypto libnetutils libselinux

LOCAL_C_INCLUDES := \
        $(LOCAL_PATH)/include

#LOCAL_C_INCLUDES += external/selinux/libselinux/include/ \
                    external/selinux/libselinux/src

LOCAL_CFLAGS := -DANDROID_CHANGES

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE:= pppoe_wrapper

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
#LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_EXECUTABLE)

