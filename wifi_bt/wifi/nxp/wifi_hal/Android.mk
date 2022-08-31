# Copyright 2012-2020 NXP
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH := $(call my-dir)

# Make the HAL library
# ============================================================
include $(CLEAR_VARS)

LOCAL_REQUIRED_MODULES :=

LOCAL_CFLAGS += -Wno-unused-parameter -Wno-int-to-pointer-cast
LOCAL_CFLAGS += -Wno-maybe-uninitialized -Wno-parentheses
LOCAL_CFLAGS += -Werror
LOCAL_CPPFLAGS += -Wno-conversion-null

LOCAL_CFLAGS += -DPLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)
LOCAL_CPPFLAGS += -DPLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)

ifneq ($(filter %64,$(LINUX_ARCH)),)
LOCAL_CFLAGS += -DMLAN_64BIT
endif

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH) \
	$(LOCAL_PATH)/include \
	external/libnl/include \
	system/core/liblog/include \
	system/core/libcutils/include \
	system/core/libutils/include \
	$(call include-path-for, libhardware_legacy)/hardware_legacy \
	external/wpa_supplicant_8/src/drivers \
	$(TARGET_OUT_HEADERS)/libwpa_client \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include

LOCAL_SRC_FILES := \
	wifi_hal.cpp \
	common.cpp \
	cpp_bindings.cpp \
	link_layer_stats.cpp \
	rtt.cpp \
	wifi_logger.cpp \
	roam.cpp \
	wifi_nan.cpp \
	wifi_offload.cpp

LOCAL_SHARED_LIBRARIES += libnetutils libnl liblog libcutils libutils libwpa_client
LOCAL_PROPRIETARY_MODULE := true

LOCAL_MODULE := libwifi-hal-nxp

include $(BUILD_STATIC_LIBRARY)

