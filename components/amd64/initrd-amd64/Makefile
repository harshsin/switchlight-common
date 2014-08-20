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

include /.pandora-ws
ifneq ($(WS_ARCH),amd64)
$(error an amd64 workspace is required to build this package (current is $(WS_ARCH)))
endif

SWITCHLIGHT_REQUIRED_SUBMODULES := loader
include $(SWITCHLIGHT)/make/component.mk


