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
RELEASE := SwitchLight ($(SWITCHLIGHT_BUILD_CONFIG),$(SWITCHLIGHT_BUILD_TIMESTAMP),$(SWITCHLIGHT_BUILD_SHA1))
endif

ifndef ARCH
$(error $$ARCH is not defined.)
endif

$(SWI).swi: rootfs-$(ARCH).sqsh
	rm -f $@.tmp
	rm -f *.swi
	cp $(KERNELS) $(INITRD) . 
	zip -n $(INITRD_LOCAL):rootfs-$(ARCH).sqsh - $(KERNELS_LOCAL) $(INITRD_LOCAL) rootfs-$(ARCH).sqsh >$@.tmp
	$(SWITCHLIGHT)/tools/swiver $@.tmp $(SWI)-$(SWITCHLIGHT_BUILD_TIMESTAMP).swi "$(RELEASE)"
	ln -s $(SWI)-$(SWITCHLIGHT_BUILD_TIMESTAMP).swi $@
	rm $(KERNELS_LOCAL) $(INITRD_LOCAL) rootfs-$(ARCH).sqsh *.tmp

rootfs-$(ARCH).sqsh:
	$(MAKE) -C rootfs rootfs.all
	cp rootfs/$@ . 

clean:
	@rm -f $(KERNELS_LOCAL) $(INITRD_LOCAL) rootfs-$(ARCH).sqsh *.tmp
	@rm -f *.swi
	@$(MAKE) -C rootfs clean


