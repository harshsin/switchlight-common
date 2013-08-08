###############################################################################
#
#
###############################################################################
BUILD_INITRD_POWERPC := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

include $(SWITCHLIGHT)/make/config.mk

initrd-powerpc: $(BUILD_INITRD_POWERPC)/initrd-powerpc

$(SWITCHLIGHT_LOADER)/.setup.done:
	$(MAKE) -C $(SWITCHLIGHT_LOADER) setup
	touch $(SWITCHLIGHT_LOADER)/.setup.done

$(BUILD_INITRD_POWERPC)/initrd-powerpc: $(SWITCHLIGHT_LOADER)/.setup.done
	$(MAKE) -C $(SWITCHLIGHT_LOADER) buildroot-powerpc
	cat $(SWITCHLIGHT_LOADER)/buildroot-powerpc/images/rootfs.cpio.gz >initrd-powerpc

.PHONY: deb

deb:
	$(MAKE) -C deb












