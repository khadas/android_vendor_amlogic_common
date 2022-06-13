LOCAL_PATH := $(my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := mfgbridge
LOCAL_SRC_FILES := mfgbridge.c mfgdebug.c ../drvwrapper/drv_wrapper.c
#LOCAL_MODULE_TAGS := eng development
LOCAL_CFLAGS += -Wno-Wreturn-type -Wno-error -DNONPLUG_SUPPORT -DMFG_UPDATE -DRAWUR_BT_STACK
LOCAL_C_INCLUDES := $(INCLUDES)
LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_EXECUTABLE)
