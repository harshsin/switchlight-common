###############################################################################
#
# Including in a component's Makefile.comp to inherit the rules
# necessary to build that component's debian package. 
#
###############################################################################
.PHONY: deb
deb:
	$(MAKE) -C deb



