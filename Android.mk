ifeq ($(AMLOGIC_PRODUCT),true)
LOCAL_PATH:= $(call my-dir)
include $(call all-makefiles-under,$(LOCAL_PATH))

endif
