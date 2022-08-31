modules:
	mkdir -p $(OUT_DIR)/../../../../../vendor/amlogic/common/wifi_bt/bluetooth/realtek/rtk_btusb
	$(MAKE) -C $(ROOT_DIR)/vendor/amlogic/common/wifi_bt/bluetooth/realtek/rtk_btusb M=../../../../../vendor/amlogic/common/wifi_bt/bluetooth/realtek/rtk_btusb KERNEL_SRC=$(ROOT_DIR)/$(KERNEL_DIR) O=$(OUT_DIR) SUBDIRS=$(ROOT_DIR)/vendor/amlogic/common/wifi_bt/bluetooth/realtek/rtk_btusb
	mkdir -p $(OUT_DIR)/../../../../../vendor/amlogic/common/wifi_bt/bluetooth/amlogic/sdio_driver_bt
	$(MAKE) -C $(ROOT_DIR)/vendor/amlogic/common/wifi_bt/bluetooth/amlogic/sdio_driver_bt M=../../../../../vendor/amlogic/common/wifi_bt/bluetooth/amlogic/sdio_driver_bt KERNEL_SRC=$(ROOT_DIR)/$(KERNEL_DIR) O=$(OUT_DIR) SUBDIRS=.$(ROOT_DIR)/vendor/amlogic/common/wifi_bt/bluetooth/amlogic/sdio_driver_bt

modules_install:
	$(MAKE) -C $(ROOT_DIR)/vendor/amlogic/common/wifi_bt/bluetooth/realtek/rtk_btusb M=../../../../../vendor/amlogic/common/wifi_bt/bluetooth/realtek/rtk_btusb KERNEL_SRC=$(ROOT_DIR)/$(KERNEL_DIR) O=$(OUT_DIR) INSTALL_MOD_STRIP=1 INSTALL_MOD_PATH=$(INSTALL_MOD_PATH) SUBDIRS=$(ROOT_DIR)/vendor/amlogic/common/wifi_bt/bluetooth/realtek/rtk_btusb modules_install
	$(MAKE) -C $(ROOT_DIR)/vendor/amlogic/common/wifi_bt/bluetooth/amlogic/sdio_driver_bt M=../../../../../vendor/amlogic/common/wifi_bt/bluetooth/amlogic/sdio_driver_bt KERNEL_SRC=$(ROOT_DIR)/$(KERNEL_DIR) O=$(OUT_DIR) INSTALL_MOD_STRIP=1 INSTALL_MOD_PATH=$(INSTALL_MOD_PATH) SUBDIRS=$(ROOT_DIR)/vendor/amlogic/common/wifi_bt/bluetooth/amlogic/sdio_driver_bt modules_install

