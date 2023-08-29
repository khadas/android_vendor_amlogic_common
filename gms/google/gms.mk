#
# Copyright (C) 2014 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

ifeq ($(PRODUCT_USE_PREBUILT_GTVS), yes)
  $(call inherit-product, vendor/google_gtvs/products/mainline_modules_atv.mk)
  $(call inherit-product, vendor/google_gtvs/products/gtvs.mk)
  ifeq ($(ATV_LAUNCHER),amati)
    $(call inherit-product, vendor/google_gtvs_gtv/products/gtvs_gtv.mk)

    # gsi test will missing product partition, copy one to vendor
    PRODUCT_COPY_FILES += \
        vendor/google_gtvs_gtv/etc/permissions/unavailable-features-atv-amati-product.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/unavailable-features-atv-amati-product.xml
  endif
else
  $(call inherit-product-if-exists, vendor/google_atv/products/atv_mainline_modules.mk)
  $(call inherit-product, $(SRC_TARGET_DIR)/product/updatable_apex.mk)
  ifeq ($(ATV_LAUNCHER),amati)
    $(call inherit-product-if-exists, vendor/google_atv/products/atv_google_amati_3p.mk)
  else
    $(call inherit-product-if-exists, vendor/google_atv/products/atv_google_watson.mk)
  endif
endif

# Overrides
PRODUCT_PRODUCT_PROPERTIES += \
    ro.com.google.gmsversion=S_amlogic
