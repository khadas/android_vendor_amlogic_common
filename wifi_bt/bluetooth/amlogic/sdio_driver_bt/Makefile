KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build
M ?= $(PWD)
###############################################################################
# AMLBT
###############################################################################
AMLBT_MOD_NAME = sdio_bt
AMLBT_CFILES := \
	bt_hal_plateform.c

$(AMLBT_MOD_NAME)-objs := $(AMLBT_CFILES:.c=.o)

###############################################################################
# Common
###############################################################################
obj-m := $(AMLBT_MOD_NAME).o

all:
	make -C $(KERNEL_SRC) M=$(M) modules

modules_install:
	@$(MAKE) INSTALL_MOD_STRIP=1 M=$(M) -C $(KERNEL_SRC) modules_install
	mkdir -p ${OUT_DIR}/../vendor_lib/modules
	cd ${OUT_DIR}/$(M)/; find -name "*.ko" -exec cp {} ${OUT_DIR}/../vendor_lib/modules/ \;

usb:
	make -C $(KERNEL_SRC) M=$(PWD) $(AMLBT_MOD_NAME).ko

clean:
	make -C $(KERNEL_SRC) M=$(PWD) clean



