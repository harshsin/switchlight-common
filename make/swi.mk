# -*- Makefile -*-
###############################################################################
#
# Common SWI build rules
#
###############################################################################
ifndef SWITCHLIGHT
$(error $$SWITCHLIGHT is not defined.)
else
include $(SWITCHLIGHT)/make/config.mk
export SWITCHLIGHT
endif

ifndef SWI
$(error $$SWI is not defined.)
endif

.DEFAULT_GOAL := $(SWI).swi

# We need a set of kernels
ifndef KERNELS
$(error $$KERNELS is not defined.)
else
KERNELS_LOCAL := $(foreach k,$(KERNELS),$(notdir $(k)))
endif

# We need an initrd
ifndef INITRD
$(error $$INITRD is not defined.)
else
INITRD_LOCAL := $(notdir $(INITRD))
endif

# Build config
ifndef SWITCHLIGHT_BUILD_CONFIG
$(error $$SWITCHLIGHT_BUILD_CONFIG is not defined.)
endif

# Release string
ifndef RELEASE
RELEASE := SwitchLight$(SWITCHLIGHT_RELEASE_BANNER)($(SWITCHLIGHT_BUILD_CONFIG),$(SWITCHLIGHT_BUILD_TIMESTAMP),$(SWITCHLIGHT_BUILD_SHA1))
endif

ifndef ARCH
$(error $$ARCH is not defined.)
endif

# ZTN Platforms are required for the manifest
ifndef ZTN_PLATFORMS
$(error $$ZTN_PLATFORMS is not defined)
endif

ZTN_MANIFEST := zerotouch.json

# Always regenerate
.PHONY: $(ZTN_MANIFEST)

$(ZTN_MANIFEST):
	$(SWITCHLIGHT)/tools/py/zerotouch.py --release "$(RELEASE)" --operation swi --platforms $(ZTN_PLATFORMS) --sha1 $(SWITCHLIGHT_BUILD_SHA1) > $(ZTN_MANIFEST)

$(SWI).swi: rootfs-$(ARCH).sqsh $(ZTN_MANIFEST)
	rm -f $@.tmp
	rm -f *.swi
	cp $(KERNELS) $(INITRD) .
	zip -n $(INITRD_LOCAL):rootfs-$(ARCH).sqsh - $(KERNELS_LOCAL) $(INITRD_LOCAL) rootfs-$(ARCH).sqsh $(ZTN_MANIFEST) >$@.tmp
	$(SWITCHLIGHT)/tools/swiver $@.tmp $(SWI)-$(SWITCHLIGHT_BUILD_TIMESTAMP).swi "$(RELEASE)"
	ln -s $(SWI)-$(SWITCHLIGHT_BUILD_TIMESTAMP).swi $@
	rm $(KERNELS_LOCAL) $(INITRD_LOCAL) rootfs-$(ARCH).sqsh *.tmp #$(ZTN_MANIFEST)

rootfs-$(ARCH).sqsh:
	$(MAKE) -C rootfs rootfs.all
	cp rootfs/$@ .

clean:
	$(SL_V_at)rm -f $(KERNELS_LOCAL) $(INITRD_LOCAL) rootfs-$(ARCH).sqsh *.tmp
	$(SL_V_at)rm -f *.swi
	$(SL_V_at)$(MAKE) -C rootfs clean


