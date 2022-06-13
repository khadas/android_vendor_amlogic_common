LOCAL_PATH := $(call my-dir)

DVB_PATH := $(wildcard external/dvb)
ifeq ($(DVB_PATH), )
	DVB_PATH := $(wildcard $(BOARD_AML_VENDOR_PATH)/external/dvb)
endif
ifeq ($(DVB_PATH), )
	DVB_PATH := $(wildcard vendor/amlogic/external/dvb)
endif
ifeq ($(DVB_PATH), )
	DVB_PATH := $(wildcard $(BOARD_AML_VENDOR_PATH)/dvb)
endif
ifeq ($(DVB_PATH), )
	DVB_PATH := $(wildcard vendor/amlogic/dvb)
endif

ifeq (,$(wildcard $(LOCAL_PATH)/../../../../external/libzvbi))
LIBZVBI_C_INCLUDES:=vendor/amlogic/common/prebuilt/libzvbi/include
else
LIBZVBI_C_INCLUDES:=vendor/amlogic/common/external/libzvbi/src
endif

ifeq (,$(wildcard $(LOCAL_PATH)/../../../../external/dvb))
DVB_C_INCLUDES:=vendor/amlogic/common/prebuilt/dvb/include/am_adp \
  vendor/amlogic/common/prebuilt/dvb/include/am_mw \
  vendor/amlogic/common/prebuilt/dvb/include/am_ver \
  vendor/amlogic/common/prebuilt/dvb/ndk/include
else
DVB_C_INCLUDES:=vendor/amlogic/common/external/dvb/include/am_adp \
  vendor/amlogic/common/external/dvb/include/am_mw \
  vendor/amlogic/common/external/dvb/include/am_ver \
  vendor/amlogic/common/external/dvb/android/ndk/include
endif

#######################################################################

include $(CLEAR_VARS)

LOCAL_MODULE    := libjnidtvepgscanner
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := DTVEpgScanner.c
LOCAL_MULTILIB := 32
LOCAL_ARM_MODE := arm
LOCAL_C_INCLUDES := \
	external/sqlite/dist \
	external/skia/include\
	external/skia/include/core \
	external/skia/include/config \
	bionic/libc/include

LOCAL_HEADER_LIBRARIES := jni_headers
LOCAL_SHARED_LIBRARIES += \
  liblog \
  libcutils

#LOCAL_STATIC_LIBRARIES := libskia

LOCAL_PRELINK_MODULE := false

#DVB define
ifeq ($(BOARD_HAS_ADTV),true)
LOCAL_CFLAGS += -DSUPPORT_ADTV

LOCAL_C_INCLUDES += \
  $(LIBZVBI_C_INCLUDES)\
  $(DVB_C_INCLUDES)

LOCAL_SHARED_LIBRARIES += \
  libam_mw \
  libzvbi \
  libam_adp
endif

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)
#######################################################################

#include $(CLEAR_VARS)

#LOCAL_MODULE    := libvendorfont
#LOCAL_MULTILIB := both
#LOCAL_MODULE_SUFFIX := .so
#LOCAL_MODULE_TAGS := optional
#LOCAL_MODULE_CLASS := SHARED_LIBRARIES

#LOCAL_VENDOR_MODULE := true
#LOCAL_SRC_FILES_arm := arm/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)
#LOCAL_SRC_FILES_arm64 := arm64/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)

#include $(BUILD_PREBUILT)

#######################################################################

include $(CLEAR_VARS)

LOCAL_MODULE    := libjnifont
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := Fonts.cpp
LOCAL_ARM_MODE := arm
LOCAL_MULTILIB := 32
LOCAL_C_INCLUDES := \
    system/core/libutils/include \
    system/core/liblog/include

LOCAL_HEADER_LIBRARIES := jni_headers
LOCAL_SHARED_LIBRARIES += libvendorfont liblog libcutils

LOCAL_PRELINK_MODULE := false

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)

#######################################################################
