MAKE_ARGS := KERNEL_SRC=$(KERNEL_SRC) O=$(O)
INSTALL_ARGS := INSTALL_MOD_STRIP=$(INSTALL_MOD_STRIP) INSTALL_MOD_PATH=$(INSTALL_MOD_PATH)

include $(ROOT_DIR)/vendor/amlogic/common/wifi_bt/wifi/configs/4_9/config.mk
-include $(ROOT_DIR)/device/amlogic/$(PRODUCT_DIR)/wifibt.build.config.trunk.mk
include $(ROOT_DIR)/vendor/amlogic/common/wifi_bt/wifi/configs/driver.mk
