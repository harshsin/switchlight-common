###############################################################################
#
# Powerpc Loader Build Rules
#
###############################################################################

# The kernel image must be specified:
ifndef KERNEL.BIN.GZ
$(error $$KERNEL.BIN.GZ is not set)
endif

# The initrd must be specified:
ifndef INITRD
$(error $$INITRD is not set)
endif

# The dtb must be specified:
ifndef DTB
$(error $$DTB is not set)
endif

# Platform name must be set
ifndef PLATFORM_NAME
$(error $$PLATFORM_NAME is not set)
endif

# LOADER_SIZE must be set
ifndef LOADER_SIZE
$(error $$LOADER_SIZE is not set)
endif

# FLASHFS_SIZE_KB must be set
ifndef FLASHFS_SIZE_KB
$(error $$FLASHFS_SIZE_KB is not set)
endif

# FLASH_ERASEBLOCK_KB must be set
ifndef FLASH_ERASEBLOCK_KB
$(error $$(FLASH_ERASEBLOCK_KB)
endif

all: switchlight.$(PLATFORM_NAME).loader switchlight.$(PLATFORM_NAME).flash

# Rule to build the UBoot Loader Image
switchlight.$(PLATFORM_NAME).loader: $(KERNEL.BIN.GZ) $(INITRD) $(DTB)
	f=$$(mktemp); $(SWITCHLIGHT)/tools/powerpc-linux-gnu-mkimage -A ppc -T multi -C gzip -d $(KERNEL.BIN.GZ):$(INITRD):$(DTB) $$f && cat $$f > switchlight.$(PLATFORM_NAME).loader 


# Rule to build the platform flash image
switchlight.$(PLATFORM_NAME).flash: switchlight.$(PLATFORM_NAME).loader
	f=$$(mktemp); $(SWITCHLIGHT)/tools/make-flash-image $(LOADER_SIZE) $< $(FLASHFS_SIZE_KB) $(FLASH_ERASEBLOCK_KB) $(SWI_URL) $$f && cat $$f > $@


