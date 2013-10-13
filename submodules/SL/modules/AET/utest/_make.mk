###############################################################################
#
# AET Unit Test Makefile.
#
###############################################################################
UMODULE := AET
UMODULE_SUBDIR := $(dir $(lastword $(MAKEFILE_LIST)))
include $(BUILDER)/utest.mk
