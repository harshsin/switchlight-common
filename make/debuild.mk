###############################################################################
#
#
###############################################################################

ifndef ARCH
$(error $$ARCH must be specified before including this makefile.)
endif

ifndef SWITCHLIGHT
$(error $$SWITCHLIGHT must be specified before including this makefile.)
endif

ifndef PACKAGE_NAMES
$(error $$PACKAGE_NAMES must be specified.)
endif


DEBUILD = debuild --prepend-path=/usr/lib/ccache -eSWITCHLIGHT $(DEBUILD_ARGS) -a$(ARCH) -b -us -uc

PACKAGE_DIR := $(SWITCHLIGHT)/debian/repo

all:
	$(MAKE) -C ../
	cd debuild; $(DEBUILD)
	mv *.deb $(PACKAGE_DIR)
	rm -rf debuild/debian/tmp $(foreach p,$(PACKAGE_NAMES),debuild/debian/$(p)/ debuild/debian/$(p)-dbg)

clean:
	cd debuild; $(DEBUILD) -Tclean
	rm -f *.deb *.changes *.build
dch:
	cd build; EMAIL="$(USER)@bigswitch.com" dch -i



