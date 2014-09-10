############################################################
# <bsn.cl fy=2013 v=none>
#
#        Copyright 2013, 2014 BigSwitch Networks, Inc.
#
#
#
# </bsn.cl>
############################################################
ifndef SWITCHLIGHT
$(error $$SWITCHLIGHT is undefined.)
endif

#
# These are the local packages that must be build
# for this SWI configuration.
#
SWITCHLIGHT_REQUIRED_PACKAGES := kernel-x86-64:amd64 \
				 initrd-amd64:amd64

include $(SWITCHLIGHT)/make/component.mk


