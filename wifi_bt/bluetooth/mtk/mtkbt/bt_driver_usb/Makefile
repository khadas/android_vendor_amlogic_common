###############################################################################
# Ubuntu build
###############################################################################
#KERNEL_DIR := /lib/modules/$(shell uname -r)/build
#CURRENT_DIR=$(PWD)

###############################################################################
# apollo manual build
###############################################################################
#export ARCH=arm64
#export CROSS_COMPILE=/mtkoss/gnuarm/gcc-aarch64-linux-gun-4.9.3-ubuntu/x86_64/bin/aarch64-linux-gnu-
#KERNEL_DIR = $(PWD)/../../../../android/o-base/out/mediatek_linux/output/mediatek/mt5893_eu/debug/sys_build/debug_obj/glibc_world/kernel/linux_core/kernel/linux-4.9/mt5893_android_smp_mod_defconfig
#CURRENT_DIR=$(PWD)

###############################################################################
# apollo system build
###############################################################################
#include $(LINUX_ROOT)/linux_core/driver/ko/target/target.mak


ifeq "$(UTOPIA)" "true"
KERNEL_DIR := $(UTOPIA_KERNEL_ROOT)/$(subst linux-,,$(KERNEL_VER))
else
KERNEL_DIR=$(KERNEL_OBJ_ROOT)/$(KERNEL_VER)/$(KERNEL_CONFIG)_modules
endif
CURRENT_DIR=$(PWD)/../../../linux_mts/ko_modules/bt_driver

#ccflags-y += -Wunused-function
#MTKCFLAGS = -Wno-Wunused-function
ccflags-y += -Wno-unused-function

###############################################################################
# Don't modify the following
###############################################################################
USB_MOD_NAME = btmtk_usb
USB_CFILES := \
	btmtk_usb_main.c \
	btmtk_usb_fifo.c
$(USB_MOD_NAME)-objs := $(USB_CFILES:.c=.o)
obj-m := $(USB_MOD_NAME).o



all : modules
modules:
	@echo cross_comple = $(CROSS_COMPILE)
	@echo CFLAGS = $(CFLAGS)
	@echo EXTRA_CFLAGS = $(EXTRA_CFLAGS)
	@echo $(M)
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_SRC) M=$(M) modules

modules_install:
	@$(MAKE) INSTALL_MOD_STRIP=1 M=$(M) -C $(KERNEL_SRC) modules_install
	mkdir -p ${OUT_DIR}/../vendor_lib/modules
	cd ${OUT_DIR}/$(M)/; find -name $(USB_MOD_NAME).ko -exec cp {} ${OUT_DIR}/../vendor_lib/modules/$(USB_MOD_NAME).ko \;


driver:
	$(warning ======================================================)
	$(warning Make MTK BT Driver, btmtk_usb.ko)
	$(warning KERNEL_DIR=$(KERNEL_DIR))
	$(warning CURRENT_DIR=$(CURRENT_DIR))
	$(warning CC=$(CC))
	$(warning CROSS_COMPILE=$(CROSS_COMPILE))
	$(warning TARGET_IC=$(TARGET_IC))
	$(warning KERNEL_VER=$(KERNEL_VER))
	$(warning KERNEL_CONFIG=$(KERNEL_CONFIG))
	$(warning OBJ_ROOT=$(OBJ_ROOT))
	$(warning OUTPUT_ROOT=$(OUTPUT_ROOT))
	$(warning ======================================================)
ifeq "$(UTOPIA)" "true"
ifdef BSP_OBJ_OUT
	make -C $(KERNEL_DIR) M=$(CURRENT_DIR)  modules MTK_EXTMOD_OBJ=$(OBJ_ROOT)/linux_mts/ko_modules/bt_driver CROSS_COMPILE=$(KERNEL_CROSS_COMPILE) O=$(KERNEL_OUT_ABS)
else
	make -C $(KERNEL_DIR) M=$(CURRENT_DIR)  modules  CROSS_COMPILE=$(KERNEL_CROSS_COMPILE)
endif
	if [ -d $(OUTPUT_ROOT)/kernel_modules -a -f $(CURRENT_DIR)/btmtk_usb.ko ]; then \
		cp -f $(CURRENT_DIR)/btmtk_usb.ko $(OUTPUT_ROOT)/kernel_modules/btmtk_usb.ko; \
	fi
else
	make -C $(KERNEL_DIR) M=$(CURRENT_DIR) modules
	if [ -d $(OUTPUT_ROOT)/kernel_modules ]; then \
		cp -f $(OBJ_ROOT)/linux_mts/ko_modules/bt_driver/btmtk_usb.ko $(OUTPUT_ROOT)/kernel_modules/btmtk_usb.ko; \
	fi
endif

clean:
	$(warning ======================================================)
	$(warning Clean MTK BT Driver, btmtk_usb.ko)
	$(warning KERNEL_DIR=$(KERNEL_DIR))
	$(warning CURRENT_DIR=$(CURRENT_DIR))
	$(warning OBJ_ROOT=$(OBJ_ROOT))
	$(warning OUTPUT_ROOT=$(OUTPUT_ROOT))
	$(warning ======================================================)
	if [ -d $(OUTPUT_ROOT)/kernel_modules ]; then \
		rm -f $(OUTPUT_ROOT)/kernel_modules/btmtk_usb.ko; \
	fi
	if [ -d $(KERNEL_DIR) ]; then \
		make -C $(KERNEL_DIR) M=$(CURRENT_DIR) clean; \
	fi

###############################################################################
# Check coding style
###############################################################################
ccs:
	./util/checkpatch.pl --no-tree --show-types --max-line-length=120 -f btmtk_usb_main.c
	./util/checkpatch.pl --no-tree --show-types --max-line-length=120 -f btmtk_usb_main.h
	./util/checkpatch.pl --no-tree --show-types --max-line-length=120 -f btmtk_config.h
	./util/checkpatch.pl --no-tree --show-types --max-line-length=120 -f btmtk_define.h
	./util/checkpatch.pl --no-tree --show-types --max-line-length=120 -f btmtk_usb_fifo.c
	./util/checkpatch.pl --no-tree --show-types --max-line-length=120 -f btmtk_usb_fifo.h
