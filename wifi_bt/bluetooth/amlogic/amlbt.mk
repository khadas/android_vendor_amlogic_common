# RELEASE

BOARD_HAVE_BLUETOOTH_AMLOGIC := true
AML_BLUETOOTH_LPM_ENABLE := true



PRODUCT_COPY_FILES += vendor/amlogic/common/wifi_bt/bluetooth/amlogic/w1/aml_bt_rf.txt:$(TARGET_COPY_OUT_VENDOR)/etc/bluetooth/w1/aml_bt_rf.txt

PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.bluetooth.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.bluetooth.xml \
	frameworks/native/data/etc/android.hardware.bluetooth_le.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.bluetooth_le.xml


