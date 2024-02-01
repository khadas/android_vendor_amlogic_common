$(warning 2222)
PRODUCT_PACKAGES += \
    logpersist \
    logpersist.vendor

SYSTEM_EXT_PUBLIC_SEPOLICY_DIRS += vendor/amlogic/common/frameworks/services/logging/sepolicy/system_ext/public
SYSTEM_EXT_PRIVATE_SEPOLICY_DIRS += vendor/amlogic/common/frameworks/services/logging/sepolicy/system_ext/private

BOARD_SEPOLICY_DIRS += vendor/amlogic/common/frameworks/services/logging/sepolicy/vendor


PRODUCT_PROPERTY_OVERRIDES += ro.logd.kernel=true
PRODUCT_PRODUCT_PROPERTIES += ro.logd.kernel=true
