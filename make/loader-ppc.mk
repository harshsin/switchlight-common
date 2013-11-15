###############################################################################
#
# Powerpc Loader Build Rules (ONIE loader)
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

# FLASHFS_SIZE_KB must be set
ifndef FLASHFS_SIZE_KB
$(error $$FLASHFS_SIZE_KB is not set)
endif

# FLASH_ERASEBLOCK_KB must be set
ifndef FLASH_ERASEBLOCK_KB
$(error $$(FLASH_ERASEBLOCK_KB)
endif

all: switchlight.$(PLATFORM_NAME).loader switchlight.$(PLATFORM_NAME).jffs2

ifdef LOADER_SIZE
all: switchlight.$(PLATFORM_NAME).flash
endif

# Rule to build the UBoot Loader Image
switchlight.$(PLATFORM_NAME).loader: $(KERNEL.BIN.GZ) $(INITRD) $(DTB)
	$(SL_V_GEN)set -e ;\
	if $(SL_V_P); then set -x; fi ;\
	f=$$(mktemp) ;\
	trap "rm -f $$f" 0 1 ;\
	$(SWITCHLIGHT)/tools/powerpc-linux-gnu-mkimage -A ppc -T multi -C gzip -d $(KERNEL.BIN.GZ):$(INITRD):$(DTB) $$f ;\
	cat $$f > $@

# package jffs2 filesystem separately
switchlight.$(PLATFORM_NAME).jffs2:
	$(SL_V_GEN)set -e ;\
	if $(SL_V_P); then set -x; fi ;\
	f=$$(mktemp) ;\
	trap "rm -f $$f" 0 1 ;\
	$(SWITCHLIGHT)/tools/make-jffs2-image $(FLASHFS_SIZE_KB) $(FLASH_ERASEBLOCK_KB) $(SWI_URL) $$f ;\
	cat $$f > $@

# Rule to build the platform flash image (loader plus jffs2 fs)
switchlight.$(PLATFORM_NAME).flash: switchlight.$(PLATFORM_NAME).loader switchlight.$(PLATFORM_NAME).jffs2
	$(SL_V_GEN)set -e ;\
	if $(SL_V_P); then set -x; fi ;\
	f=$$(mktemp) ;\
	trap "rm -f $$f" 0 1 ;\
	if test -z "$(LOADER_SIZE)"; then \
	  echo "*** missing LOADER_SIZE" ;\
	  exit 1 ;\
	fi ;\
	$(SWITCHLIGHT)/tools/make-flash-image $(LOADER_SIZE) switchlight.$(PLATFORM_NAME).loader switchlight.$(PLATFORM_NAME).jffs2 $$f ;\
	cat $$f > $@

clean:
	rm -f *.loader *.jffs2 *.flash
