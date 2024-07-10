define root-dir
$(strip \
 $(shell bash -c "\
 while [[ ( ! ( -d device/khadas/common ) ) && ( \`pwd\` != "/" ) ]]; do\
  cd ..;\
 done;\
 if [[ -d \`pwd\`/device/khadas/common ]]; then\
  echo \`pwd\`;\
 else\
  echo "";\
 fi"\
 )\
)
endef

define cur-dir
$(strip \
  $(eval LOCAL_MODULE_MAKEFILE := $$(lastword $$(MAKEFILE_LIST))) \
  $(shell cd $(patsubst %/,%,$(dir $(LOCAL_MODULE_MAKEFILE))) && pwd) \
)
endef

define to-root-path
$(strip $(patsubst %/,%,$(shell echo\
 $(shell bash -c "\
 cd $(1);\
 while [[ ( ! ( -d device/khadas/common ) ) && ( \`pwd\` != "/" ) ]]; do\
  cd ..;\
  echo \"../\";\
 done;"\
 )|sed 's/[[:space:]]//g'))\
)
endef

####################################################################################
DRIVER_DIR ?= common14-5.15/driver_modules/wifi_bt/wifi
ANDROID_ROOT_DIR     ?= $(shell cd $(call to-root-path,.) && pwd)
KERNEL_TO_ROOT_PATH  ?= $(call to-root-path,$(KERNEL_SRC))
WIFI_SUPPORT_DRIVERS ?= $(EXTRA_WIFI_SUPPORT_DRIVERS)
####################################################################################

WIFI_DEFAULT_CONFIG := $(call cur-dir)/config.mk
WIFI_TRUNK_CONFIG ?= $(if $(PRODUCT_DIRNAME),$(call root-dir)/$(PRODUCT_DIRNAME)/wifibt.build.config.trunk.mk,$(call root-dir)/device/khadas/$(PRODUCT_DIR)/wifibt.build.config.trunk.mk)
WIFI_BUILD_RULES := $(call cur-dir)/../driver.mk

MAKE_ARGS := KERNEL_SRC=$(KERNEL_SRC) O=$(O) LLVM=$(LLVM) DEPMOD=$(DEPMOD)
INSTALL_ARGS := INSTALL_MOD_PATH=$(INSTALL_MOD_PATH)

$(warning loading default wifi config: $(WIFI_DEFAULT_CONFIG))
include $(WIFI_DEFAULT_CONFIG)

$(warning loading trunk wifi config: $(WIFI_TRUNK_CONFIG))
-include $(WIFI_TRUNK_CONFIG)

$(warning loading wifi build rules: $(WIFI_BUILD_RULES))
include $(WIFI_BUILD_RULES)

modules_install: additional
additional: all_modules_install
	@echo "######wifi start additional processing.######"
	@order_file="";\
	modules_order_file=`find $(INSTALL_MOD_PATH)/lib/modules/*/$(INSTALL_MOD_DIR) -name "modules.order.*"`;\
	for f in $$modules_order_file; do\
	 if test -z "$$order_file";then\
	  order_file=$$f;\
	 else\
	  cat $$f >> $$order_file;\
	  rm -rf $$f;\
	 fi;\
	done
