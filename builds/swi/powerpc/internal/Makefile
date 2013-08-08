###############################################################################
#
# Switchlight Powerpc Internal Image
#
###############################################################################
ifndef SWITCHLIGHT
$(error $$SWITCHLIGHT is not defined.)
else
export SWITCHLIGHT
endif

.DEFAULT_GOAL := switchlight-powerpc-internal.swi

# We need the following package binaries
kernel-85xx    := $(shell $(SWITCHLIGHT)/debian/install.py kernel-85xx powerpc --find-file kernel-85xx)
kernel-e500mc  := $(shell $(SWITCHLIGHT)/debian/install.py kernel-e500mc powerpc --find-file kernel-e500mc)
initrd-powerpc := $(shell $(SWITCHLIGHT)/debian/install.py initrd-powerpc powerpc --find-file initrd-powerpc)

RELEASE := switchlight-internal-ng

switchlight-powerpc-internal.swi: rootfs-powerpc.sqsh
	rm -f $@.tmp
	cp $(kernel-85xx) $(kernel-e500mc) $(initrd-powerpc) .
	zip -n initrd-powerpc:rootfs-powerpc.sqsh - kernel-85xx kernel-e500mc initrd-powerpc rootfs-powerpc.sqsh >$@.tmp
	$(SWITCHLIGHT)/tools/swiver $@.tmp $@ "$(RELEASE)"
	rm kernel-85xx kernel-e500mc initrd-powerpc
	rm rootfs-powerpc.sqsh
	rm $@.tmp

rootfs-powerpc.sqsh:
	$(MAKE) -C rootfs rootfs.all
	cp rootfs/$@ . 


clean:
	@rm -f *.swi
	@$(MAKE) -C rootfs clean


