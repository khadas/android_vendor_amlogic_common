LOCAL_PATH := $(my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := nxp_fwloader
OBJS = ../src/fw_loader_uart.c ../src/fw_loader_io_linux.c
LOCAL_SRC_FILES := $(OBJS)
LOCAL_CFLAGS=-g -I../src/
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := \
    libcutils \
    liblog

LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_EXECUTABLE)
