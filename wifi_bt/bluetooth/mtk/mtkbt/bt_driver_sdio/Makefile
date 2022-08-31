#ifeq ($(LINUX_SRC), )
#	export KERNEL_SRC := /lib/modules/$(shell uname -r)/build
#else
#	export KERNEL_SRC := $(LINUX_SRC)
#endif
###############################################################################
# USB
###############################################################################
###############################################################################
# SDIO
###############################################################################
ifneq ($(TARGET_BUILD_VARIANT), user)
ccflags-y += -DMTK_KERNEL_DEBUG
endif
ifeq ($(SUPPORT_SINK_LOWPOWER), yes)
ccflags-y += -DLOWER_POWER_SINK=1
endif
ccflags-y += -Wmacro-redefined
#VPATH = /opt/toolchains/gcc-linaro-aarch64-linux-gnu-4.9-2014.09_linux
SDIO_MOD_NAME = btmtksdio
SDIO_CFILES := \
	btmtk_sdio.c btmtk_main.c
$(SDIO_MOD_NAME)-objs := $(SDIO_CFILES:.c=.o)
###############################################################################
# Common
###############################################################################
obj-m := $(SDIO_MOD_NAME).o


all : modules

modules:
	@echo cross_comple = $(CROSS_COMPILE)
	@echo CFLAGS = $(CFLAGS)
	@echo EXTRA_CFLAGS = $(EXTRA_CFLAGS)
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_SRC) M=$(M) modules

modules_install:
	@$(MAKE) INSTALL_MOD_STRIP=1 M=$(M) -C $(KERNEL_SRC) modules_install
	mkdir -p ${OUT_DIR}/../vendor_lib/modules
	cd ${OUT_DIR}/$(M)/; find -name $(SDIO_MOD_NAME).ko -exec cp {} ${OUT_DIR}/../vendor_lib/modules/$(SDIO_MOD_NAME).ko \;

driver:
	make -C $(KERNEL_SRC) M=$(PWD) modules

clean:
	make -C $(KERNEL_SRC) M=$(PWD) clean
# Check coding style
ccs:
	./scripts/checkpatch.pl -f btmtk_main.c
	./scripts/checkpatch.pl -f btmtk_sdio.c
	./scripts/checkpatch.pl -f btmtk_sdio.h
	./scripts/checkpatch.pl -f btmtk_config.h
	./scripts/checkpatch.pl -f btmtk_define.h
	./scripts/checkpatch.pl -f btmtk_drv.h

