LOCAL_PATH := $(my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := mlanutl
LOCAL_SRC_FILES := mlanutl.c
LOCAL_CFLAGS += -Wall -DUSERSPACE_32BIT_OVER_KERNEL_64BIT
LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_EXECUTABLE)
