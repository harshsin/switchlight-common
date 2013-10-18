###############################################################################
# 
# Component Makefile
#
# Allow dependency resolution prior to actual component build. 
#
###############################################################################
include $(SWITCHLIGHT)/make/config.mk

ifndef NOT_PARALLEL
SL_MAKE_PARALLEL := -j
endif

component: component-deps
	$(SL_V_at)$(MAKE) -f Makefile.comp $(SL_MAKE_PARALLEL) $(SL_MAKEFLAGS)

deb: component-deps
	$(SL_V_at)$(MAKE) -f Makefile.comp deb $(SL_MAKE_PARALLEL) $(SL_MAKEFLAGS)
clean:
	$(SL_V_at)$(MAKE) -f Makefile.comp clean $(SL_MAKEFLAGS)


component-deps:
ifdef SWITCHLIGHT_REQUIRED_SUBMODULES
	$(SL_V_at)set -e ;\
	if $(SL_V_P); then set -x; export V=1; else export V=0; fi ;\
	$(SWITCHLIGHT)/tools/submodules.py $(SWITCHLIGHT_REQUIRED_SUBMODULES) $(SWITCHLIGHT_LOCAL_SUBMODULES) $(SWITCHLIGHT)
endif
ifdef SWITCHLIGHT_REQUIRED_PACKAGES
	$(SL_V_at)$(SWITCHLIGHT_PKG_INSTALL) $(SWITCHLIGHT_REQUIRED_PACKAGES) --build
endif
