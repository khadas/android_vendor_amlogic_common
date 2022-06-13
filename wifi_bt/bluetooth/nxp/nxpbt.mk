# RELEASE

BOARD_HAVE_BLUETOOTH_NXP := true

ifneq ($(BOARD_HAVE_BLUETOOTH_MULTIBT),true)
BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR := $(LOCAL_PATH)/btbuild
endif

PRODUCT_COPY_FILES += vendor/amlogic/common/wifi_bt/bluetooth/nxp/firmware/sd8987/uart8987_bt.bin:$(TARGET_COPY_OUT_VENDOR)/lib/firmware/nxp/uart8987_bt.bin
PRODUCT_COPY_FILES += vendor/amlogic/common/wifi_bt/bluetooth/nxp/firmware/sd8987/fw_loader:$(TARGET_COPY_OUT_VENDOR)/bin/fw_loader

PRODUCT_PACKAGES += nxp_fwloader fmapp

#bt addr
PRODUCT_PROPERTY_OVERRIDES += \
  ro.bt.bdaddr_path=/sys/module/kernel/parameters/btmac

PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.bluetooth.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.bluetooth.xml \
                      frameworks/native/data/etc/android.hardware.bluetooth_le.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.bluetooth_le.xml


