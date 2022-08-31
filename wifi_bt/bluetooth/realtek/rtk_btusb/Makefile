obj-m	= rtk_btusb.o
MODULE_NAME = rtk_btusb

M ?= $(shell pwd)

all: modules

modules:
	@echo cross_comple = $(CROSS_COMPILE)
	@echo CFLAGS = $(CFLAGS)
	@echo EXTRA_CFLAGS = $(EXTRA_CFLAGS)
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_SRC) M=$(M)  modules

modules_install:
	@$(MAKE) INSTALL_MOD_STRIP=1 M=$(M) -C $(KERNEL_SRC) modules_install
	mkdir -p ${OUT_DIR}/../vendor_lib/modules
	cd ${OUT_DIR}/$(M)/; find -name $(MODULE_NAME).ko -exec cp {} ${OUT_DIR}/../vendor_lib/modules/$(MODULE_NAME).ko \;
