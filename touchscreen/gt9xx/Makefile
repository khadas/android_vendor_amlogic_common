# Copyright (C) 2017 Amlogic Corporation.
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

# default module name if not called or get name from Android.mk
KBUILD_CFLAGS_MODULE += $(GKI_EXT_MODULE_PREDEFINE)


MODULE_NAME = aml_gt9xx

# generic form for all modules


obj-m += $(MODULE_NAME).o

$(MODULE_NAME)-y += gt9xx.o
modules:
	@$(MAKE) -C $(KERNEL_SRC) M=$(M)  modules

all:modules

modules_install:
	$(MAKE) M=$(M) -C $(KERNEL_SRC) modules_install
	mkdir -p ${OUT_DIR}/../vendor_lib/modules
	cd ${OUT_DIR}/$(M)/; find -name "*.ko" -exec cp {} ${OUT_DIR}/../vendor_lib/modules/aml_gt9xx.ko \;
clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) clean
