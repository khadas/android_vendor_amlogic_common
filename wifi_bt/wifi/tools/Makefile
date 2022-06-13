ROOT_DIR := $(shell bash -c "\
 while [[ ( ! ( -f build/core/envsetup.mk ) ) && ( \`pwd\` != "/" ) ]]; do\
  cd ..;\
 done;\
 if [[ -f \`pwd\`/build/core/envsetup.mk ]]; then\
  echo \`pwd\`;\
 else\
  echo "";\
 fi\
")
ifeq ($(ROOT_DIR),)
$(error Couldn't locate the android root directory!)
endif
ifneq ($(CONFIG_K_VERSION),)
include $(ROOT_DIR)/vendor/amlogic/common/wifi_bt/wifi/configs/$(CONFIG_K_VERSION)/config.mk
endif
##for build wifi driver
ifneq ($(CONFIG_WIFI_BUILD_DRIVER),)
ifeq ($(CONFIG_K_VERSION),4_9)
MAKE_ARGS := KERNEL_SRC=$(KERNEL_SRC) O=$(O)
INSTALL_ARGS := INSTALL_MOD_STRIP=$(INSTALL_MOD_STRIP) INSTALL_MOD_PATH=$(INSTALL_MOD_PATH)
endif
ifeq ($(CONFIG_K_VERSION),5_4)
MAKE_ARGS := KERNEL_SRC=$(KERNEL_SRC) O=$(O) CC=$(CC) HOSTCC=$(HOSTCC) LD=$(LD) NM=$(NM) OBJCOPY=$(OBJCOPY)
INSTALL_ARGS := INSTALL_MOD_PATH=$(INSTALL_MOD_PATH)
endif
CONFIG_WIFI_MODULES := $($(CONFIG_WIFI_BUILD_DRIVER)_modules)
include $(ROOT_DIR)/vendor/amlogic/common/wifi_bt/wifi/configs/driver.mk
##for get wifi drivers or modules
else
driver_list :=\
$(foreach driver,\
 $(WIFI_SUPPORT_DRIVERS),\
 $(if $(filter true,$($(driver)_build)),"(y)$(driver)","(n)$(driver)")\
)
module_list :=\
$(foreach driver,\
 $(WIFI_SUPPORT_DRIVERS),\
  $(foreach module,\
   $($(driver)_modules),\
    $(if $(filter true,$($(driver)_build)),"(y)$(module)","(n)$(module)")\
 )\
)
endif

all:
	@$(ROOT_DIR)/vendor/amlogic/common/wifi_bt/wifi/tools/build_tool.sh

get_modules:
	@$(ROOT_DIR)/vendor/amlogic/common/wifi_bt/wifi/tools/build_tool.sh --get_modules

get_drivers:
	@$(ROOT_DIR)/vendor/amlogic/common/wifi_bt/wifi/tools/build_tool.sh --get_drivers

supported_modules:
	@for module in $(module_list); do\
	 echo $$module;\
	done

supported_drivers:
	@for driver in $(driver_list); do\
	 echo $$driver;\
	done

help:
	@echo "make                --build wifi driver"
	@echo "make get_modules    --get supported modules"
	@echo "make get_drivers    --get supported drivers"
	@echo "make help           --Print help information"

