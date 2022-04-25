MAKE_ARGS := KERNEL_SRC=$(KERNEL_SRC) O=$(O) CC=$(CC) HOSTCC=$(HOSTCC) LD=$(LD) NM=$(NM) OBJCOPY=$(OBJCOPY)
INSTALL_ARGS := INSTALL_MOD_PATH=$(INSTALL_MOD_PATH)

include $(ROOT_DIR)/vendor/amlogic/common/wifi_bt/wifi/configs/5_4/config.mk
-include $(ROOT_DIR)/device/khadas/$(PRODUCT_DIR)/wifibt.build.config.trunk.mk
include $(ROOT_DIR)/vendor/amlogic/common/wifi_bt/wifi/configs/driver.mk
