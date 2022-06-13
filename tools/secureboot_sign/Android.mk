#variable LOCAL_PATH will overwrited by other Android.mk
#	so LOCAL_PATH only valid when Makefile life-time, no target building time
LOCAL_PATH:= $(call my-dir)
$(warning LOCAL_PATH is $(LOCAL_PATH))
$(warning BUILD_NUMBER_FROM_FILE $(BUILD_NUMBER_FROM_FILE))
AML_SECUREBOOT_SIGN_TOOL := $(LOCAL_PATH)/Aml_Linux_SecureBootV3_SignTool/amlogic_secureboot_sign_whole_pkg.bash
ifeq ($(AML_SECUREBOOT_USE_SCS_MODE),true)
AML_SECUREBOOT_SIGN_TOOL := $(LOCAL_PATH)/Aml_Linux_SCS_SignTool/amlogic_scs_sign_whole_pkg.bash
AML_SCS_BOOTLOADER_PARA	 := --bootloader_zip $(BOARD_AML_SECUREBOOT_BOOT_IN)
ifneq ($(BOARD_AML_SECUREBOOT_ARB_CFG),)
AML_SCS_BOOTLOADER_PARA	 += --fw_arb_cfg $(BOARD_AML_SECUREBOOT_ARB_CFG)
endif
endif#

BOARD_AVB_KEY_SIGN_PARA :=
ifeq ($(BOARD_AVB_ENABLE),true)
	BOARD_AVB_KEY_SIGN_PARA := --avb_pem_key $(BOARD_AVB_KEY_PATH)
endif#ifeq ($(BOARD_AVB_ENABLE),true)

INSTALLED_AML_UPGRADE_PACKAGE_SIGNED_TARGET := $(basename $(INSTALLED_AML_UPGRADE_PACKAGE_TARGET)).signed.img

$(INSTALLED_AML_UPGRADE_PACKAGE_SIGNED_TARGET): $(INSTALLED_AML_UPGRADE_PACKAGE_TARGET)
	@echo "Package $@"
	@echo $(AML_SECUREBOOT_SIGN_TOOL) --soc $(BOARD_AML_SECUREBOOT_SOC_TYPE) --aml_key $(BOARD_AML_SECUREBOOT_KEY_DIR) \
		$(BOARD_AVB_KEY_SIGN_PARA) --aml_img $< --output $@ $(AML_SCS_BOOTLOADER_PARA)
	$(hide) (bash $(AML_SECUREBOOT_SIGN_TOOL) --soc $(BOARD_AML_SECUREBOOT_SOC_TYPE) --aml_key $(BOARD_AML_SECUREBOOT_KEY_DIR) \
		$(BOARD_AVB_KEY_SIGN_PARA) --aml_img $< --output $@ $(AML_SCS_BOOTLOADER_PARA)) \
		|| (echo "Failed create $@" && rm -f $@ && exit 66)
	@echo "installed $@"

.PHONY: signed_aml_upgrade
signed_aml_upgrade:$(INSTALLED_AML_UPGRADE_PACKAGE_SIGNED_TARGET)

INSTALLED_AML_FASTBOOT_SIGNED_ZIP := $(basename $(INSTALLED_AML_FASTBOOT_ZIP)).signed.zip
$(INSTALLED_AML_FASTBOOT_SIGNED_ZIP): $(INSTALLED_AML_FASTBOOT_ZIP)
	@echo "Package $@"
	@echo $(AML_SECUREBOOT_SIGN_TOOL) --soc $(BOARD_AML_SECUREBOOT_SOC_TYPE) --aml_key $(BOARD_AML_SECUREBOOT_KEY_DIR) \
		$(BOARD_AVB_KEY_SIGN_PARA) $(AML_SCS_BOOTLOADER_PARA) \
		--fastboot_zip $< --output $@ $(AML_SCS_BOOTLOADER_ZIP)
	$(hide) $(AML_SECUREBOOT_SIGN_TOOL) --soc $(BOARD_AML_SECUREBOOT_SOC_TYPE) --aml_key $(BOARD_AML_SECUREBOOT_KEY_DIR) \
		$(BOARD_AVB_KEY_SIGN_PARA) $(AML_SCS_BOOTLOADER_PARA) \
		--fastboot_zip $< --output $@ \
		|| (echo "Failed create $@" && rm -f $@ && exit 66)
	@echo "installed $@"

.PHONY: signed_fastboot_zip
signed_fastboot_zip:$(INSTALLED_AML_FASTBOOT_SIGNED_ZIP)

OTA_KEY_DIR := $(PRODUCT_OUT)/ota_$(notdir $(DEFAULT_SYSTEM_DEV_CERTIFICATE))
OTA_SIGN_KEYS :=

define OTA_KEY_4_SIGN_template
OTA_SIGN_KEYS += $(1)
$(1): $(2)
	mkdir -p $(OTA_KEY_DIR)
	cp -f $$< $$@
	@echo installed $$@
endef
$(foreach otakey,$(wildcard $(DEFAULT_SYSTEM_DEV_CERTIFICATE)*),$(eval $(call OTA_KEY_4_SIGN_template,$(OTA_KEY_DIR)/$(notdir $(otakey)),$(otakey))))
$(warning OTA_SIGN_KEYS $(OTA_SIGN_KEYS))

$(warning BUILT_TARGET_FILES_PACKAGE $(BUILT_TARGET_FILES_PACKAGE))
BUILT_TARGET_SIGNED_PACKAGE := $(AML_TARGET).signed.zip
$(BUILT_TARGET_SIGNED_PACKAGE): $(AML_TARGET).zip $(OTA_SIGN_KEYS)
	@echo "Package $@"
	@echo $(AML_SECUREBOOT_SIGN_TOOL) --soc $(BOARD_AML_SECUREBOOT_SOC_TYPE) --aml_key $(BOARD_AML_SECUREBOOT_KEY_DIR) \
		$(BOARD_AVB_KEY_SIGN_PARA) $(AML_SCS_BOOTLOADER_PARA) --ota_key $(OTA_KEY_DIR) \
		--target_zip $< --output $@
	$(hide) $(AML_SECUREBOOT_SIGN_TOOL) --soc $(BOARD_AML_SECUREBOOT_SOC_TYPE) --aml_key $(BOARD_AML_SECUREBOOT_KEY_DIR) \
		$(BOARD_AVB_KEY_SIGN_PARA) $(AML_SCS_BOOTLOADER_PARA) --ota_key $(OTA_KEY_DIR) \
		--target_zip $< --output $@ \
		|| (echo "Failed create $@" && rm -f $@ && exit 66)
	@echo "installed $@"

.PHONY: signed_target_zip
signed_target_zip:$(BUILT_TARGET_SIGNED_PACKAGE)

$(warning INTERNAL_OTA_PACKAGE_TARGET $(INTERNAL_OTA_PACKAGE_TARGET))
INSTALLED_OTA_SIGNED_PACKAGE := $(basename $(INTERNAL_OTA_PACKAGE_TARGET)).signed.zip
$(INSTALLED_OTA_SIGNED_PACKAGE): $(INTERNAL_OTA_PACKAGE_TARGET) $(OTA_SIGN_KEYS)
	@echo "Package $@"
	@echo $(AML_SECUREBOOT_SIGN_TOOL) --soc $(BOARD_AML_SECUREBOOT_SOC_TYPE) --aml_key $(BOARD_AML_SECUREBOOT_KEY_DIR) \
		$(BOARD_AVB_KEY_SIGN_PARA) --ota_key $(OTA_KEY_DIR) \
		--ota_zip $< --output $@ $(AML_SCS_BOOTLOADER_PARA)
	$(hide) $(AML_SECUREBOOT_SIGN_TOOL) --soc $(BOARD_AML_SECUREBOOT_SOC_TYPE) --aml_key $(BOARD_AML_SECUREBOOT_KEY_DIR) \
		$(BOARD_AVB_KEY_SIGN_PARA) --ota_key $(OTA_KEY_DIR) \
		--ota_zip $< --output $@ $(AML_SCS_BOOTLOADER_PARA) \
		|| (echo "Failed create $@" && rm -f $@ && exit 66)
	@echo "installed $@"

.PHONY: signed_otapackage
signed_otapackage: $(INSTALLED_OTA_SIGNED_PACKAGE) otapackage


.PHONY: signed_aml_all
signed_aml_all: signed_target_zip signed_otapackage signed_aml_upgrade signed_fastboot_zip

