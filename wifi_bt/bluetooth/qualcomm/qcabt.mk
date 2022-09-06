# RELEASE

BOARD_HAS_QCA_BT_ROME := true
BOARD_HAVE_BLUETOOTH_BLUEZ := false
QCOM_BT_USE_SIBS := false
BOARD_HAVE_BLUETOOTH_QCOM := true

ifneq ($(BOARD_HAVE_BLUETOOTH_MULTIBT),true)
BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR := $(LOCAL_PATH)/btbuild
endif

PRODUCT_COPY_FILES += vendor/amlogic/common/wifi_bt/wifi/qcom/config/qca9377/bt/nvm_tlv_tf_1.1.bin:$(TARGET_COPY_OUT_VENDOR)/etc/bluetooth/qca9377/ar3k/nvm_tlv_tf_1.1.bin \
	                  vendor/amlogic/common/wifi_bt/wifi/qcom/config/qca9377/bt/rampatch_tlv_tf_1.1.tlv:$(TARGET_COPY_OUT_VENDOR)/etc/bluetooth/qca9377/ar3k/rampatch_tlv_tf_1.1.tlv \
					  vendor/amlogic/common/wifi_bt/wifi/qcom/config/qca6174/bt/nvm_tlv_3.2.bin:$(TARGET_COPY_OUT_VENDOR)/etc/bluetooth/qca6174/ar3k/nvm_tlv_3.2.bin \
					  vendor/amlogic/common/wifi_bt/wifi/qcom/config/qca6174/bt/rampatch_tlv_3.2.tlv:$(TARGET_COPY_OUT_VENDOR)/etc/bluetooth/qca6174/ar3k/rampatch_tlv_3.2.tlv

PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.bluetooth.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.bluetooth.xml \
                      frameworks/native/data/etc/android.hardware.bluetooth_le.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.bluetooth_le.xml

PRODUCT_PRODUCT_PROPERTIES += poweroff.doubleclick=1
PRODUCT_PRODUCT_PROPERTIES += qcom.bluetooth.soc=rome_uart
PRODUCT_PRODUCT_PROPERTIES += \
    wc_transport.soc_initialized=0
