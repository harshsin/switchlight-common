###############################################################################
# 
# Component Makefile
#
# Allow dependency resolution prior to actual component build. 
#
###############################################################################
include $(SWITCHLIGHT)/make/config.mk

component: component-deps
	@$(MAKE) -f Makefile.comp --no-print-directory

deb: component-deps
	@$(MAKE) -f Makefile.comp deb --no-print-directory
clean:
	@$(MAKE) -f Makefile.comp clean --no-print-directory


component-deps:
ifdef SWITCHLIGHT_REQUIRED_SUBMODULES
	@$(SWITCHLIGHT)/tools/submodules.py $(SWITCHLIGHT_REQUIRED_SUBMODULES) $(SWITCHLIGHT_LOCAL_SUBMODULES) $(SWITCHLIGHT)
endif
ifdef SWITCHLIGHT_REQUIRED_PACKAGES
	@$(SWITCHLIGHT_PKG_INSTALL) $(SWITCHLIGHT_REQUIRED_PACKAGES) --build
endif