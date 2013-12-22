###############################################################################
#
# Powerpc Loader and Platform Package Build Rules 
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

all: switchlight.$(PLATFORM_NAME).loader 

# Rule to build the UBoot Loader Image
switchlight.$(PLATFORM_NAME).loader: $(KERNEL.BIN.GZ) $(INITRD) $(DTB)
	$(SL_V_GEN)set -e ;\
	if $(SL_V_P); then set -x; fi ;\
	f=$$(mktemp) ;\
	trap "rm -f $$f" 0 1 ;\
	$(SWITCHLIGHT)/tools/powerpc-linux-gnu-mkimage -A ppc -T multi -C gzip -d $(KERNEL.BIN.GZ):$(INITRD):$(DTB) $$f ;\
	cat $$f > $@

clean:
	rm -f *.loader
