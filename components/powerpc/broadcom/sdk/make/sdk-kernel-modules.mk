############################################################
#
# SDK Kernel Modules
#
############################################################
#
# This makefile provides general rules for building the SDK
# kernel modules for a given SDK and kernel version.
#
############################################################
ifndef MODULE_DIR
# The caller should specify the target directory for the
# resulting modules
$(error $$MODULE_DIR must be specified.)
endif

ifndef SDK
# This is the actual SDK tree we're building from
$(error $$SDK is not set)
endif

ifndef SDK_KMOD_VERSION
# This is the version suffix applied to the resulting libraries.
$(error $$SDK_KMOD_VERSION is not set)
endif

ifndef KERNEL_HEADERS
# This is the kernel-headers package we should build against.
$(error $$KERNEL_HEADERS is not defined)
endif

ifndef SDK_CONFIG
$(error $$SDK_CONFIG must be specified)
endif

BLDCONFIG=$(notdir $(KERNEL_HEADERS))

include $(SWITCHLIGHT)/make/config.mk

.DEFAULT_GOAL := modules

.PHONY: deb modules

modules:
	$(MAKE) -C $(SDK)/systems/switchlight/powerpc/$(SDK_CONFIG) SDK=$(SDK) \
		SWITCHLIGHT_LINUX_KERNEL=$(KERNEL_HEADERS) BLDCONFIG=$(BLDCONFIG) modules
	cp $(SDK)/build/$(BLDCONFIG)/switchlight/powerpc/$(SDK_CONFIG)/linux-kernel-bde.ko $(MODULE_DIR)/linux-kernel-bde-$(SDK_KMOD_VERSION).ko
	cp $(SDK)/build/$(BLDCONFIG)/switchlight/powerpc/$(SDK_CONFIG)/linux-user-bde.ko $(MODULE_DIR)/linux-user-bde-$(SDK_KMOD_VERSION).ko

deb:
	$(MAKE) -C deb SDK=$(SDK)








