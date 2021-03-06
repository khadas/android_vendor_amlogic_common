#
# Copyright 2020 The Android Open Source Project
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

# Mainline configuration for the ATV devices.

# Non updatable APEX
OVERRIDE_TARGET_FLATTEN_APEX := true
PRODUCT_PROPERTY_OVERRIDES += ro.apex.updatable=false

# Minimum set of modules
PRODUCT_PACKAGES += \
    com.google.android.modulemetadata \
    com.android.permission.gms \
    com.android.extservices.gms

# Overlay packages for APK-type modules
PRODUCT_PACKAGES += \
    ModuleMetadataGoogleOverlay \
    GooglePermissionControllerOverlay \
    GooglePermissionControllerFrameworkOverlay \
    GoogleExtServicesConfigOverlay

# Alternative packages for the Networking modules
PRODUCT_PACKAGES += \
    InProcessNetworkStack \
    PlatformCaptivePortalLogin \
    PlatformNetworkPermissionConfig \
    com.android.tethering.inprocess
