###############################################################################
#
# collecta Unit Test Makefile.
#
###############################################################################
UMODULE := collecta
UMODULE_SUBDIR := $(dir $(lastword $(MAKEFILE_LIST)))
include $(BUILDER)/utest.mk
