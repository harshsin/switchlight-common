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

ifndef DEBUILD_DIR
DEBUILD_DIR := debuild
endif

deb:
	$(SL_V_at)$(MAKE) -C ../ $(SL_MAKEFLAGS)
	cd $(DEBUILD_DIR); $(DEBUILD)
	$(SWITCHLIGHT_PKG_INSTALL) --add-pkg *$(ARCH)*.deb
	rm *$(ARCH)*.deb 
	rm -rf $(DEBUILD_DIR)/debian/tmp $(foreach p,$(PACKAGE_NAMES),$(DEBUILD_DIR)/debian/$(p)/ $(DEBUILD_DIR)/debian/$(p)-dbg)

clean:
	cd $(DEBUILD_DIR); $(DEBUILD) -Tclean
	rm -f *$(ARCH)*.deb *.changes *.build
dch:
	cd build; EMAIL="$(USER)@bigswitch.com" dch -i

all: deb


