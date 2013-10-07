###############################################################################
# 
# Component Makefile
#
# Allow dependency resolution prior to actual component build. 
#
###############################################################################
include $(SWITCHLIGHT)/make/config.mk

component: component-deps
	$(SL_V_at)$(MAKE) -f Makefile.comp $(SL_MAKEFLAGS)

deb: component-deps
	$(SL_V_at)$(MAKE) -f Makefile.comp deb $(SL_MAKEFLAGS)
clean:
	$(SL_V_at)$(MAKE) -f Makefile.comp clean $(SL_MAKEFLAGS)


component-deps:
ifdef SWITCHLIGHT_REQUIRED_SUBMODULES
	$(SL_V_at)$(SWITCHLIGHT)/tools/submodules.py $(SWITCHLIGHT_REQUIRED_SUBMODULES) $(SWITCHLIGHT_LOCAL_SUBMODULES) $(SWITCHLIGHT)
endif
ifdef SWITCHLIGHT_REQUIRED_PACKAGES
	$(SL_V_at)$(SWITCHLIGHT_PKG_INSTALL) $(SWITCHLIGHT_REQUIRED_PACKAGES) --build
endif
