###############################################################################
#
# 
#
###############################################################################

LIBRARY := host_sflow
$(LIBRARY)_SUBDIR := $(dir $(lastword $(MAKEFILE_LIST)))
include $(BUILDER)/lib.mk
