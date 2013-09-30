###############################################################################
#
#
###############################################################################

ifndef ARCH
$(error $$ARCH must be specified before including this makefile.)
else

ifeq ($(ARCH),all)
ARCH_OPTIONS :=
else
ARCH_OPTIONS := -a$(ARCH)
endif
endif


ifndef SWITCHLIGHT
$(error $$SWITCHLIGHT must be specified before including this makefile.)
endif

ifndef PACKAGE_NAMES
$(error $$PACKAGE_NAMES must be specified.)
endif

include $(SWITCHLIGHT)/make/config.mk

DEBUILD = debuild --prepend-path=/usr/lib/ccache -eSWITCHLIGHT -eBUILD_DIR_BASE $(DEBUILD_ARGS) $(ARCH_OPTIONS) -b -us -uc

PACKAGE_DIR := $(SWITCHLIGHT)/debian/repo

deb:
	$(SL_V_at)$(MAKE) -C ../ $(SL_MAKEFLAGS)
	cd debuild; $(DEBUILD)
	$(SWITCHLIGHT_PKG_INSTALL) --add-pkg *.deb
	rm *.deb 
	rm -rf debuild/debian/tmp $(foreach p,$(PACKAGE_NAMES),debuild/debian/$(p)/ debuild/debian/$(p)-dbg)

clean:
	cd debuild; $(DEBUILD) -Tclean
	rm -f *.deb *.changes *.build
dch:
	cd build; EMAIL="$(USER)@bigswitch.com" dch -i

all: deb


