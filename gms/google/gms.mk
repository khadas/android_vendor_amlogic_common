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
  GTVS_PRODUCTS := vendor/google_gtvs/overlays/products
  GTVS_ETC := vendor/google_gtvs/overlays/etc
  GTVS_SEPOLICY := vendor/google_gtvs/sepolicy/tv
  GTVS_VIRTUAL_REMOTE := vendor/google_gtvs/overlays/etc
  ifeq ($(ATV_LAUNCHER),amati)
    GTVS_GTV_PRODUCTS := vendor/google_gtvs_gtv/overlays/products
    GTVS_GTV_ETC := vendor/google_gtvs_gtv/overlays/etc
  endif
else
  ifneq ($(ATV_LAUNCHER),amati)
    GTVS_PRODUCTS := vendor/google/gms/src/products
    GTVS_ETC := vendor/google/data/etc
    GTVS_SEPOLICY := vendor/google/gms/src/sepolicy/tv
  else
    $(call inherit-product, vendor/google_atv/products/atv_google_amati_3p.mk)
  endif
endif

PRODUCT_PACKAGES := \
    AndroidMediaShell \
    AtvRemoteService \
    GooglePackageInstaller \
    FrameworkPackageStubs \
    GoogleCalendarSyncAdapter \
    GoogleFeedback \
    GoogleOneTimeInitializer \
    GooglePartnerSetup \
    GoogleServicesFramework \
    GoogleTTS \
    PlayGamesPano \
    PrebuiltGmsCorePano \
    SmartConnectTV \
    SssAuthbridgePrebuilt \
    talkback \
    Tubesky \
    VideosPano \
    WebViewGoogle \
    YouTubeLeanback \
    YouTubeMusicTVPrebuilt \
    GoogleExtShared

ifneq ($(ATV_LAUNCHER),amati)
PRODUCT_PACKAGES += \
    LatinIMEGoogleTvPrebuilt \
    BugReportSender \
    GoogleExtServices \
    TVCustomization \
    Backdrop \
    Katniss \
    TVLauncher \
    TVRecommendations \
    SetupWraithPrebuilt
else
PRODUCT_PACKAGES += \
    TVDreamXPrebuilt \
    KatnissXPrebuilt \
    KidsGmsCorePrebuiltTv \
    GboardTvXPrebuilt \
    SetupWraithXPrebuilt \
    TVLauncherXPrebuilt \
    DroidTvSettingsTwoPanel \
    com.android.libraries.tv.tvsystem \
    AtvRemoteServiceOverlayX
endif

# Configuration files for GTVS apps
PRODUCT_COPY_FILES := \
    $(GTVS_ETC)/sysconfig/google.xml:$(TARGET_COPY_OUT_PRODUCT)/etc/sysconfig/google.xml \
    $(GTVS_ETC)/permissions/privapp-permissions-google-system.xml:$(TARGET_COPY_OUT_SYSTEM)/etc/permissions/privapp-permissions-google.xml \
    $(GTVS_ETC)/permissions/privapp-permissions-google-system_ext.xml:$(TARGET_COPY_OUT_SYSTEM_EXT)/etc/permissions/privapp-permissions-google-system_ext.xml \
    $(GTVS_ETC)/permissions/privapp-permissions-google-product.xml:$(TARGET_COPY_OUT_PRODUCT)/etc/permissions/privapp-permissions-google-p.xml \
    $(GTVS_ETC)/permissions/split-permissions-google.xml:$(TARGET_COPY_OUT_PRODUCT)/etc/permissions/split-permissions-google.xml \
    $(GTVS_ETC)/permissions/privapp-permissions-atv-product.xml:$(TARGET_COPY_OUT_PRODUCT)/etc/permissions/privapp-permissions-atv-product.xml \
    $(GTVS_ETC)/permissions/privapp-permissions-atv.xml:$(TARGET_COPY_OUT_SYSTEM)/etc/permissions/privapp-permissions-atv.xml \
    $(GTVS_ETC)/sysconfig/google_atv.xml:$(TARGET_COPY_OUT_PRODUCT)/etc/sysconfig/google_atv.xml \
    $(GTVS_ETC)/sysconfig/google-hiddenapi-package-whitelist.xml:$(TARGET_COPY_OUT_PRODUCT)/etc/sysconfig/google-hiddenapi-package-whitelist.xml

PRODUCT_COPY_FILES += \
    $(GTVS_VIRTUAL_REMOTE)/virtual-remote/virtual-remote.idc:$(TARGET_COPY_OUT_SYSTEM)/usr/idc/virtual-remote.idc \
    $(GTVS_VIRTUAL_REMOTE)/virtual-remote/virtual-remote.kl:$(TARGET_COPY_OUT_SYSTEM)/usr/keylayout/virtual-remote.kl \
    $(GTVS_VIRTUAL_REMOTE)/virtual-remote/virtual-remote.kcm:$(TARGET_COPY_OUT_SYSTEM)/usr/keychars/virtual-remote.kcm

ifeq ($(ATV_LAUNCHER),amati)
PRODUCT_COPY_FILES += \
    $(GTVS_GTV_ETC)/permissions/privapp-permissions-atv-amati-product.xml:$(TARGET_COPY_OUT_PRODUCT)/etc/permissions/privapp-permissions-atv-amati-product.xml \
    $(GTVS_GTV_ETC)/permissions/unavailable-features-atv-amati-product.xml:$(TARGET_COPY_OUT_PRODUCT)/etc/permissions/unavailable-features-atv-amati-product.xml \
    $(GTVS_GTV_ETC)/default-permissions/default-atv-amati-permissions.xml:$(TARGET_COPY_OUT_PRODUCT)/etc/default-permissions/default-atv-amati-permissions.xml \
    $(GTVS_GTV_ETC)/sysconfig/amati_experience.xml:$(TARGET_COPY_OUT_PRODUCT)/etc/sysconfig/amati_experience.xml

PRODUCT_COPY_FILES += \
    vendor/google_gtvs_gtv/overlays/bootanimations/bootanimation-gtv.zip:$(TARGET_COPY_OUT_PRODUCT)/media/bootanimation.zip
endif

# Play FSI certificate for fs-verity verification.
ifeq ($(PRODUCT_USE_PREBUILT_GTVS),yes)
PRODUCT_COPY_FILES += \
    $(GTVS_ETC)/play/play_store_fsi_cert.der:$(TARGET_COPY_OUT_PRODUCT)/etc/security/fsverity/play_store_fsi_cert.der
else
PRODUCT_PACKAGES += \
    play_store_fsi_cert
endif

ifeq ($(ATV_LAUNCHER),amati)
# Add TvSettings overlay
PRODUCT_PACKAGES += \
    TvSettingsGoogleResOverlay \
    TvSettingsVendorResOverlay

# Add GTVS GTV specific overlay
PRODUCT_PACKAGE_OVERLAYS += $(GTVS_GTV_PRODUCTS)/gms_tv_x_overlay
endif

# Overlay for GTVS devices
$(call inherit-product, device/sample/products/backup_overlay.mk)
$(call inherit-product, device/sample/products/location_overlay.mk)
ifneq ($(ATV_LAUNCHER),amati)
PRODUCT_PACKAGE_OVERLAYS += $(GTVS_PRODUCTS)/gms_tv_overlay
endif
PRODUCT_PACKAGE_OVERLAYS += $(GTVS_PRODUCTS)/gms_overlay

# GTVS apps additional sepolicy (eg. AndroidMediaShell access to Widevine)
PRODUCT_PRIVATE_SEPOLICY_DIRS += $(GTVS_SEPOLICY)

ifneq ($(ATV_LAUNCHER),amati)
# Overrides
PRODUCT_PROPERTY_OVERRIDES += \
    ro.com.google.gmsversion=GTVS_R_release
else
# Overrides
PRODUCT_PROPERTY_OVERRIDES += \
    ro.com.google.gmsversion=GTVS_GTV_R_release

# GTV namespace
PRODUCT_SOONG_NAMESPACES += vendor/google_gtvs_gtv
endif
