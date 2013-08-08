###############################################################################
#
# Powerpc Linux for 85xx
#
###############################################################################
BUILD_POWERPC_LINUX_85XX := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

include $(SWITCHLIGHT)/make/config.mk

all: kernel-85xx linux-85xx-headers 

kernel-85xx: $(BUILD_POWERPC_LINUX_85XX)/kernel-85xx
linux-85xx-headers: $(BUILD_POWERPC_LINUX_85XX)/linux-85xx-headers

KERNEL_85XX := $(BUILD_POWERPC_LINUX_85XX)/kernel-85xx
KERNEL_85XX_HEADERS := $(BUILD_POWERPC_LINUX_85XX)/linux-85xx-headers

$(BUILD_POWERPC_LINUX_85XX)/kernel-85xx: $(SWITCHLIGHT_LINUX)/arch/powerpc/configs/85xx/pandora_mpc85xx_defconfig
	mkdir -p $(BUILD_POWERPC_LINUX_85XX)/linux-build-85xx
	$(MAKE) -C $(SWITCHLIGHT_LINUX) 85xx/pandora_mpc85xx_defconfig ARCH=powerpc O=$(BUILD_POWERPC_LINUX_85XX)/linux-build-85xx
	PATH=$$PATH:$(SWITCHLIGHT)/tools $(MAKE) -C $(SWITCHLIGHT_LINUX) uImage ARCH=powerpc O=$(BUILD_POWERPC_LINUX_85XX)/linux-build-85xx
	powerpc-linux-gnu-strip -o $(BUILD_POWERPC_LINUX_85XX)/kernel-85xx $(BUILD_POWERPC_LINUX_85XX)/linux-build-85xx/vmlinux


$(BUILD_POWERPC_LINUX_85XX)/linux-85xx-headers/.done: $(SWITCHLIGHT_LINUX)/arch/powerpc/configs/85xx/pandora_mpc85xx_defconfig
	mkdir -p $(BUILD_POWERPC_LINUX_85XX)/linux-85xx-headers
	cp -a $(SWITCHLIGHT_LINUX)/include $(SWITCHLIGHT_LINUX)/arch/powerpc/include $(BUILD_POWERPC_LINUX_85XX)/linux-85xx-headers
	$(MAKE) -C $(SWITCHLIGHT_LINUX) ARCH=powerpc O=$(BUILD_POWERPC_LINUX_85XX)/linux-85xx-headers 85xx/pandora_mpc85xx_defconfig
	$(MAKE) -C $(SWITCHLIGHT_LINUX) ARCH=powerpc O=$(BUILD_POWERPC_LINUX_85XX)/linux-85xx-headers headers_install modules_prepare arch/powerpc/lib/crtsavres.o
	touch $@

$(BUILD_POWERPC_LINUX_85XX)/linux-85xx-headers: $(BUILD_POWERPC_LINUX_85XX)/linux-85xx-headers/.done


.PHONY: deb

deb:
	$(MAKE) -C deb












