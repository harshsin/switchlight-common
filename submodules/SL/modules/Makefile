###############################################################################
#
# SwitchLight Modules Makefile
#
###############################################################################
include ../init.mk

-include Manifest.mk

include $(BUILDER)/moduledir.mk
#
# Available targets
#
show_targets:
	@echo ""
	@echo "Available targets:"
	@echo "    manifest:    Rebuild Manifest.mk"
	@echo ""

autogen:
	$(MAKE) autogen -C sl
	$(MAKE) autogen -C AET

